/*****************************************************************************
 * projectiles.c                           Part of LuminariMUD
 * Shared launcher and thrown-projectile state and selection.
 *****************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "assign_wpn_armor.h"
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
  return weapon_type >= WEAPON_TYPE_UNDEFINED && weapon_type < NUM_WEAPON_TYPES;
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

  if (!ch)
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
