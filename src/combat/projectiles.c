/*****************************************************************************
 * projectiles.c                           Part of LuminariMUD
 * Shared launcher and thrown-projectile state and selection.
 *****************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "assign_wpn_armor.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "projectiles.h"
#include "spec_abilities.h"

static const int projectile_wear_slots[] = {WEAR_WIELD_2H, WEAR_WIELD_1, WEAR_WIELD_OFFHAND};

static bool is_projectile_wear_slot(int wear_slot)
{
  size_t i;

  for (i = 0; i < sizeof(projectile_wear_slots) / sizeof(projectile_wear_slots[0]); i++)
  {
    if (projectile_wear_slots[i] == wear_slot)
      return TRUE;
  }

  return FALSE;
}

static bool is_valid_weapon_object(const struct obj_data *obj)
{
  int weapon_type;

  if (!obj || GET_OBJ_TYPE(obj) != ITEM_WEAPON)
    return FALSE;

  weapon_type = GET_OBJ_VAL(obj, 0);
  return weapon_type > WEAPON_TYPE_UNDEFINED && weapon_type < NUM_WEAPON_TYPES;
}

static bool can_transfer_throwable(struct char_data *ch, const struct obj_data *obj)
{
  if (!ch || !obj || OBJ_FLAGGED(obj, ITEM_NODROP))
    return FALSE;

  if (GET_OBJ_BOUND_ID(obj) != (int)NOBODY && GET_OBJ_BOUND_ID(obj) != GET_IDNUM(ch))
    return FALSE;

  return TRUE;
}

static struct obj_data *find_equipped_matching(struct char_data *ch, int *wear_slot, bool launcher)
{
  struct obj_data *obj;
  size_t i;

  if (wear_slot)
    *wear_slot = -1;

  if (!ch || IS_WILDSHAPED(ch) || IS_MORPHED(ch))
    return NULL;

  for (i = 0; i < sizeof(projectile_wear_slots) / sizeof(projectile_wear_slots[0]); i++)
  {
    obj = GET_EQ(ch, projectile_wear_slots[i]);
    if ((launcher && is_launcher_weapon(obj)) || (!launcher && is_throwable_weapon(ch, obj)))
    {
      if (wear_slot)
        *wear_slot = projectile_wear_slots[i];
      return obj;
    }
  }

  return NULL;
}

static struct obj_data *get_thrown_anchor(struct char_data *ch, obj_vnum anchor_vnum, int wear_slot)
{
  struct obj_data *anchor;

  if (!ch || !is_projectile_wear_slot(wear_slot))
    return NULL;

  anchor = GET_EQ(ch, wear_slot);
  if (!is_throwable_weapon(ch, anchor) || GET_OBJ_VNUM(anchor) != anchor_vnum)
    return NULL;

  return anchor;
}

static bool character_is_live(const struct char_data *ch)
{
  const struct char_data *current;

  if (!ch)
    return FALSE;

  for (current = character_list; current; current = current->next)
  {
    if (current == ch)
      return TRUE;
  }

  return FALSE;
}

static bool character_can_receive_projectile(const struct char_data *ch)
{
  if (!character_is_live(ch))
    return FALSE;

  return !DEAD(ch) && GET_POS(ch) > POS_DEAD && IN_ROOM(ch) != NOWHERE &&
         IN_ROOM(ch) <= top_of_world;
}

bool projectile_object_is_unplaced(const struct obj_data *projectile)
{
  return projectile && !projectile->carried_by && !projectile->worn_by && !projectile->in_obj &&
         IN_ROOM(projectile) == NOWHERE;
}

static enum projectile_disposition
get_existing_disposition(const struct projectile_attack_context *context,
                         const struct obj_data *projectile, const struct char_data *attacker,
                         const struct char_data *target)
{
  bool attacker_live;
  bool target_live;

  attacker_live = character_is_live(attacker);
  target_live = character_is_live(target);

  if (attacker_live && projectile->worn_by == attacker)
    return PROJECTILE_DISPOSITION_ATTACKER_EQUIPMENT;
  if (attacker_live && projectile->carried_by == attacker)
    return PROJECTILE_DISPOSITION_ATTACKER_INVENTORY;
  if (target_live && projectile->carried_by == target)
    return PROJECTILE_DISPOSITION_TARGET_INVENTORY;
  if (IN_ROOM(projectile) == context->target_room || projectile->in_obj)
    return PROJECTILE_DISPOSITION_TARGET_ROOM;
  if (attacker_live && IN_ROOM(projectile) == IN_ROOM(attacker))
    return PROJECTILE_DISPOSITION_ATTACKER_ROOM;

  return PROJECTILE_DISPOSITION_TARGET_ROOM;
}

bool is_launcher_attack(int attack_type)
{
  return attack_type == ATTACK_TYPE_RANGED;
}

bool is_thrown_attack(int attack_type)
{
  return attack_type == ATTACK_TYPE_THROWN;
}

bool is_ranged_weapon_attack(int attack_type)
{
  return is_launcher_attack(attack_type) || is_thrown_attack(attack_type);
}

bool has_physical_projectile(int attack_type)
{
  return is_ranged_weapon_attack(attack_type);
}

bool is_launcher_weapon(const struct obj_data *obj)
{
  int weapon_type;

  if (!is_valid_weapon_object(obj))
    return FALSE;

  weapon_type = GET_OBJ_VAL(obj, 0);
  return IS_SET(weapon_list[weapon_type].weaponFlags, WEAPON_FLAG_RANGED);
}

bool is_throwable_weapon(struct char_data *ch, const struct obj_data *obj)
{
  int weapon_type;

  if (!is_valid_weapon_object(obj) || !can_transfer_throwable(ch, obj))
    return FALSE;

  weapon_type = GET_OBJ_VAL(obj, 0);
  if (IS_SET(weapon_list[weapon_type].weaponFlags, WEAPON_FLAG_THROWN))
    return TRUE;
  if (IS_SET(weapon_list[weapon_type].weaponFlags, WEAPON_FLAG_RANGED))
    return FALSE;

  return obj_has_special_ability((struct obj_data *)obj, WEAPON_SPECAB_THROWING);
}

bool can_store_projectile_in_ammo_pouch(struct char_data *ch, const struct obj_data *obj)
{
  if (!obj)
    return FALSE;

  if (GET_OBJ_TYPE(obj) == ITEM_MISSILE)
    return TRUE;

  return is_throwable_weapon(ch, obj);
}

bool ammo_pouch_has_capacity(const struct obj_data *ammo_pouch)
{
  int capacity;

  if (!ammo_pouch || GET_OBJ_TYPE(ammo_pouch) != ITEM_AMMO_POUCH)
    return FALSE;

  capacity = GET_OBJ_VAL(ammo_pouch, 0);
  return capacity == -1 || num_obj_in_obj(ammo_pouch->contains) < capacity;
}

struct obj_data *find_equipped_launcher(struct char_data *ch, int *wear_slot)
{
  return find_equipped_matching(ch, wear_slot, TRUE);
}

struct obj_data *find_equipped_throwable(struct char_data *ch, int *wear_slot)
{
  return find_equipped_matching(ch, wear_slot, FALSE);
}

bool is_compatible_launcher_ammo(const struct obj_data *launcher, const struct obj_data *ammo)
{
  int ammo_type;
  int weapon_type;

  if (!is_launcher_weapon(launcher) || !ammo || GET_OBJ_TYPE(ammo) != ITEM_MISSILE)
    return FALSE;

  ammo_type = GET_OBJ_VAL(ammo, 0);
  weapon_type = GET_OBJ_VAL(launcher, 0);

  switch (ammo_type)
  {
  case AMMO_TYPE_ARROW:
    switch (weapon_type)
    {
    case WEAPON_TYPE_LONG_BOW:
    case WEAPON_TYPE_SHORT_BOW:
    case WEAPON_TYPE_COMPOSITE_LONGBOW:
    case WEAPON_TYPE_COMPOSITE_LONGBOW_2:
    case WEAPON_TYPE_COMPOSITE_LONGBOW_3:
    case WEAPON_TYPE_COMPOSITE_LONGBOW_4:
    case WEAPON_TYPE_COMPOSITE_LONGBOW_5:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_2:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_3:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_4:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_5:
      return TRUE;
    }
    break;

  case AMMO_TYPE_BOLT:
    switch (weapon_type)
    {
    case WEAPON_TYPE_HAND_CROSSBOW:
    case WEAPON_TYPE_HEAVY_REP_XBOW:
    case WEAPON_TYPE_LIGHT_REP_XBOW:
    case WEAPON_TYPE_HEAVY_CROSSBOW:
    case WEAPON_TYPE_LIGHT_CROSSBOW:
      return TRUE;
    }
    break;

  case AMMO_TYPE_STONE:
    return weapon_type == WEAPON_TYPE_SLING;

  case AMMO_TYPE_DART:
    return weapon_type == WEAPON_TYPE_BLOWGUN;
  }

  return FALSE;
}

struct obj_data *find_compatible_launcher_ammo(struct char_data *ch,
                                               const struct obj_data *launcher)
{
  struct obj_data *ammo;
  struct obj_data *ammo_pouch;

  if (!ch || !is_launcher_weapon(launcher))
    return NULL;

  ammo_pouch = GET_EQ(ch, WEAR_AMMO_POUCH);
  if (!ammo_pouch || GET_OBJ_TYPE(ammo_pouch) != ITEM_AMMO_POUCH)
    return NULL;

  for (ammo = ammo_pouch->contains; ammo; ammo = ammo->next_content)
  {
    if (is_compatible_launcher_ammo(launcher, ammo))
      return ammo;
  }

  return NULL;
}

void clear_projectile_mode(struct char_data *ch)
{
  if (!ch)
    return;

  PROJECTILE_MODE(ch) = PROJECTILE_MODE_NONE;
  THROWN_ANCHOR_VNUM(ch) = NOTHING;
  THROWN_ANCHOR_WEAR_SLOT(ch) = -1;
}

void clear_launcher_projectile_mode(struct char_data *ch)
{
  if (ch && IS_LAUNCHER_MODE(ch))
    clear_projectile_mode(ch);
}

void set_launcher_projectile_mode(struct char_data *ch)
{
  if (!ch)
    return;

  PROJECTILE_MODE(ch) = PROJECTILE_MODE_LAUNCHER;
  THROWN_ANCHOR_VNUM(ch) = NOTHING;
  THROWN_ANCHOR_WEAR_SLOT(ch) = -1;
}

bool set_thrown_projectile_mode(struct char_data *ch, obj_vnum anchor_vnum, int wear_slot)
{
  if (!get_thrown_anchor(ch, anchor_vnum, wear_slot))
    return FALSE;

  PROJECTILE_MODE(ch) = PROJECTILE_MODE_THROWN;
  THROWN_ANCHOR_VNUM(ch) = anchor_vnum;
  THROWN_ANCHOR_WEAR_SLOT(ch) = wear_slot;
  return TRUE;
}

bool validate_thrown_projectile_mode(struct char_data *ch)
{
  if (!character_is_live(ch) || !IS_THROWN_MODE(ch))
    return FALSE;

  if (!get_thrown_anchor(ch, THROWN_ANCHOR_VNUM(ch), THROWN_ANCHOR_WEAR_SLOT(ch)))
  {
    clear_projectile_mode(ch);
    return FALSE;
  }

  return TRUE;
}

bool can_throw_projectile(struct char_data *ch, bool silent)
{
  struct projectile_attack_context context;

  if (!ch || !IS_THROWN_MODE(ch))
    return FALSE;

  if (!select_thrown_projectile(ch, THROWN_ANCHOR_VNUM(ch), THROWN_ANCHOR_WEAR_SLOT(ch), &context))
  {
    if (!silent)
      send_to_char(ch, "You no longer have the throwable weapon you readied.\r\n");
    clear_projectile_mode(ch);
    return FALSE;
  }

  return TRUE;
}

void initialize_projectile_attack_context(struct projectile_attack_context *context,
                                          int attack_kind)
{
  if (!context)
    return;

  memset(context, 0, sizeof(*context));
  context->attack_kind = attack_kind;
  context->original_source = PROJECTILE_SOURCE_NONE;
  context->anchor_vnum = NOTHING;
  context->anchor_wear_slot = -1;
  context->target_room = NOWHERE;
  context->disposition = PROJECTILE_DISPOSITION_NONE;
}

bool prepare_launcher_projectile(struct char_data *ch, struct projectile_attack_context *context)
{
  int wear_slot;

  if (!context)
    return FALSE;

  initialize_projectile_attack_context(context, ATTACK_TYPE_RANGED);
  context->attack_weapon = find_equipped_launcher(ch, &wear_slot);
  if (!context->attack_weapon)
    return FALSE;

  context->physical_projectile = find_compatible_launcher_ammo(ch, context->attack_weapon);
  if (!context->physical_projectile)
    return FALSE;

  context->original_source = PROJECTILE_SOURCE_AMMO_POUCH;
  context->anchor_vnum = GET_OBJ_VNUM(context->attack_weapon);
  context->anchor_wear_slot = wear_slot;
  return TRUE;
}

bool select_thrown_projectile(struct char_data *ch, obj_vnum anchor_vnum, int wear_slot,
                              struct projectile_attack_context *context)
{
  struct obj_data *ammo_pouch;
  struct obj_data *anchor;
  struct obj_data *obj;

  if (!context)
    return FALSE;

  initialize_projectile_attack_context(context, ATTACK_TYPE_THROWN);
  context->anchor_vnum = anchor_vnum;
  context->anchor_wear_slot = wear_slot;

  anchor = get_thrown_anchor(ch, anchor_vnum, wear_slot);
  if (!anchor)
    return FALSE;

  if (anchor_vnum != NOTHING)
  {
    ammo_pouch = GET_EQ(ch, WEAR_AMMO_POUCH);
    if (ammo_pouch && GET_OBJ_TYPE(ammo_pouch) == ITEM_AMMO_POUCH)
    {
      for (obj = ammo_pouch->contains; obj; obj = obj->next_content)
      {
        if (GET_OBJ_VNUM(obj) == anchor_vnum && is_throwable_weapon(ch, obj))
        {
          context->attack_weapon = obj;
          context->physical_projectile = obj;
          context->original_source = PROJECTILE_SOURCE_AMMO_POUCH;
          return TRUE;
        }
      }
    }

    for (obj = ch->carrying; obj; obj = obj->next_content)
    {
      if (!obj->in_obj && GET_OBJ_VNUM(obj) == anchor_vnum && is_throwable_weapon(ch, obj))
      {
        context->attack_weapon = obj;
        context->physical_projectile = obj;
        context->original_source = PROJECTILE_SOURCE_INVENTORY;
        return TRUE;
      }
    }
  }

  context->attack_weapon = anchor;
  context->physical_projectile = anchor;
  context->original_source = PROJECTILE_SOURCE_WIELDED;
  return TRUE;
}

void set_projectile_target(struct projectile_attack_context *context,
                           const struct char_data *target)
{
  if (!context || !target || IN_ROOM(target) == NOWHERE || IN_ROOM(target) > top_of_world)
    return;

  context->target_room = IN_ROOM(target);
  context->target_x = world[context->target_room].coords[0];
  context->target_y = world[context->target_room].coords[1];
}

bool detach_physical_projectile(struct char_data *ch, struct projectile_attack_context *context)
{
  struct obj_data *projectile;

  if (!ch || !context || context->detached || !context->physical_projectile)
    return FALSE;

  projectile = context->physical_projectile;
  switch (context->original_source)
  {
  case PROJECTILE_SOURCE_AMMO_POUCH:
    if (!projectile->in_obj || projectile->in_obj != GET_EQ(ch, WEAR_AMMO_POUCH))
      return FALSE;
    obj_from_obj(projectile);
    break;

  case PROJECTILE_SOURCE_INVENTORY:
    if (projectile->carried_by != ch || projectile->in_obj)
      return FALSE;
    obj_from_char(projectile);
    break;

  case PROJECTILE_SOURCE_WIELDED:
    if (context->anchor_wear_slot < 0 || context->anchor_wear_slot >= NUM_WEARS ||
        GET_EQ(ch, context->anchor_wear_slot) != projectile ||
        unequip_char(ch, context->anchor_wear_slot) != projectile)
      return FALSE;
    break;

  case PROJECTILE_SOURCE_NONE:
  default:
    return FALSE;
  }

  MISSILE_ID(projectile) = GET_IDNUM(ch);
  context->detached = TRUE;
  return TRUE;
}

bool projectile_object_is_live(const struct obj_data *obj)
{
  const struct obj_data *current;

  if (!obj)
    return FALSE;

  for (current = object_list; current; current = current->next)
  {
    if (current == obj)
      return TRUE;
  }

  return FALSE;
}

void finalize_physical_projectile(struct projectile_attack_context *context,
                                  struct char_data *attacker, struct char_data *target,
                                  enum projectile_disposition disposition)
{
  struct obj_data *projectile;
  room_rnum destination;
  bool returning;

  if (!context || !context->detached || context->disposition != PROJECTILE_DISPOSITION_NONE)
    return;

  projectile = context->physical_projectile;
  if (!projectile_object_is_live(projectile))
  {
    context->physical_projectile = NULL;
    context->disposition = PROJECTILE_DISPOSITION_DESTROYED;
    return;
  }

  if (!projectile_object_is_unplaced(projectile))
  {
    context->disposition = get_existing_disposition(context, projectile, attacker, target);
    return;
  }

  returning = is_thrown_attack(context->attack_kind) && !context->snatched &&
              disposition != PROJECTILE_DISPOSITION_DESTROYED &&
              obj_has_special_ability(projectile, WEAPON_SPECAB_RETURNING);

  if (returning && character_can_receive_projectile(attacker))
  {
    if (context->original_source == PROJECTILE_SOURCE_WIELDED && context->anchor_wear_slot >= 0 &&
        context->anchor_wear_slot < NUM_WEARS && !GET_EQ(attacker, context->anchor_wear_slot))
    {
      act("$p arcs through the air and returns to your hand.", FALSE, attacker, projectile, 0,
          TO_CHAR);
      act("$p arcs through the air and returns to $n's hand.", FALSE, attacker, projectile, 0,
          TO_ROOM);
      MISSILE_ID(projectile) = NOBODY;
      equip_char(attacker, projectile, context->anchor_wear_slot);
      if (projectile->worn_by == attacker)
      {
        context->disposition = PROJECTILE_DISPOSITION_ATTACKER_EQUIPMENT;
        return;
      }
      if (projectile->carried_by == attacker)
      {
        context->disposition = PROJECTILE_DISPOSITION_ATTACKER_INVENTORY;
        return;
      }
    }

    disposition = PROJECTILE_DISPOSITION_ATTACKER_INVENTORY;
  }

  destination = context->target_room;
  if (destination == NOWHERE && character_is_live(attacker))
    destination = IN_ROOM(attacker);

  switch (disposition)
  {
  case PROJECTILE_DISPOSITION_DESTROYED:
    context->physical_projectile = NULL;
    context->disposition = PROJECTILE_DISPOSITION_DESTROYED;
    extract_obj(projectile);
    return;

  case PROJECTILE_DISPOSITION_TARGET_INVENTORY:
    if (character_can_receive_projectile(target) &&
        (!context->snatched || CAN_CARRY_OBJ(target, projectile)))
    {
      obj_to_char(projectile, target);
      context->disposition = PROJECTILE_DISPOSITION_TARGET_INVENTORY;
      return;
    }
    break;

  case PROJECTILE_DISPOSITION_ATTACKER_INVENTORY:
    if (character_can_receive_projectile(attacker) && CAN_CARRY_OBJ(attacker, projectile))
    {
      MISSILE_ID(projectile) = NOBODY;
      obj_to_char(projectile, attacker);
      context->disposition = PROJECTILE_DISPOSITION_ATTACKER_INVENTORY;
      return;
    }
    if (character_is_live(attacker))
    {
      destination = IN_ROOM(attacker);
      disposition = PROJECTILE_DISPOSITION_ATTACKER_ROOM;
    }
    break;

  case PROJECTILE_DISPOSITION_ATTACKER_ROOM:
    if (character_is_live(attacker))
      destination = IN_ROOM(attacker);
    break;

  case PROJECTILE_DISPOSITION_TARGET_ROOM:
  case PROJECTILE_DISPOSITION_ATTACKER_EQUIPMENT:
  case PROJECTILE_DISPOSITION_NONE:
  default:
    break;
  }

  if (destination != NOWHERE && destination <= top_of_world)
  {
    obj_to_room(projectile, destination);
    context->disposition = disposition == PROJECTILE_DISPOSITION_ATTACKER_ROOM
                               ? PROJECTILE_DISPOSITION_ATTACKER_ROOM
                               : PROJECTILE_DISPOSITION_TARGET_ROOM;
    return;
  }

  context->physical_projectile = NULL;
  context->disposition = PROJECTILE_DISPOSITION_DESTROYED;
  extract_obj(projectile);
}
