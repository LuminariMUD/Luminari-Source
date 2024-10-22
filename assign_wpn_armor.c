/*****************************************************************************
 * assign_wpn_armor.c                        Part of LuminariMUD
 * author: Zusuk
 * Assigning weapon and armor values for respective types
 *****************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "db.h"
#include "mud_event.h"
#include "actions.h"
#include "actionqueues.h"
#include "assign_wpn_armor.h"
#include "craft.h"
#include "feats.h"
#include "constants.h"
#include "modify.h"
#include "domains_schools.h"
#include "spec_abilities.h"
#include "handler.h"
#include "spells.h"

/* global */
struct armor_table armor_list[NUM_SPEC_ARMOR_TYPES];
struct weapon_table weapon_list[NUM_WEAPON_TYPES];
const char *weapon_type[NUM_WEAPON_TYPES];

/* simply checks if ch has proficiency with given weapon_type */
int is_proficient_with_weapon(struct char_data *ch, int weapon)
{

  if (affected_by_spell(ch, SPELL_BESTOW_WEAPON_PROFICIENCY))
    return true;

  if (HAS_FEAT(ch, FEAT_WEAPON_PROFICIENCY_KENDER) && weapon == WEAPON_TYPE_HOOPAK);
    return TRUE;

  if (HAS_REAL_FEAT(ch, FEAT_PALE_MASTER_WEAPONS) && weapon == WEAPON_TYPE_SCYTHE)
    return TRUE;

  /* :) */
  if (weapon == WEAPON_TYPE_UNARMED && MONK_TYPE((ch)))
    return TRUE;

  if ((HAS_FEAT(ch, FEAT_SIMPLE_WEAPON_PROFICIENCY) || HAS_FEAT(ch, FEAT_WEAPON_EXPERT)) &&
      IS_SET(weapon_list[weapon].weaponFlags, WEAPON_FLAG_SIMPLE))
    return TRUE;

  if ((HAS_FEAT(ch, FEAT_MARTIAL_WEAPON_PROFICIENCY) || HAS_FEAT(ch, FEAT_WEAPON_EXPERT)) &&
      IS_SET(weapon_list[weapon].weaponFlags, WEAPON_FLAG_MARTIAL))
    return TRUE;

  if (HAS_FEAT(ch, FEAT_EXOTIC_WEAPON_PROFICIENCY) &&
      IS_SET(weapon_list[weapon].weaponFlags, WEAPON_FLAG_EXOTIC))
    return TRUE;

  if (HAS_FEAT(ch, FEAT_WEAPON_PROFICIENCY_MONK) &&
      weapon_list[weapon].weaponFamily == WEAPON_FAMILY_MONK)
    return TRUE;

  if (CLASS_LEVEL(ch, CLASS_INQUISITOR) && GET_1ST_DOMAIN(ch) && domain_list[GET_1ST_DOMAIN(ch)].favored_weapon == weapon)
    return TRUE;

  /* updated by zusuk: Druids are proficient with the following weapons: club,
   * dagger, dart, quarterstaff, scimitar, scythe, sickle, shortspear, sling,
   * and spear. They are also proficient with all natural attacks (claw, bite,
   * and so forth) of any form they assume with wild shape.*/
  if (HAS_FEAT(ch, FEAT_WEAPON_PROFICIENCY_DRUID) ||
      CLASS_LEVEL(ch, CLASS_DRUID) > 0)
  {
    switch (weapon)
    {
    case WEAPON_TYPE_CLUB:
    case WEAPON_TYPE_DAGGER:
    case WEAPON_TYPE_KNIFE:
    case WEAPON_TYPE_QUARTERSTAFF:
    case WEAPON_TYPE_SCIMITAR:
    case WEAPON_TYPE_SCYTHE:
    case WEAPON_TYPE_SICKLE:
    case WEAPON_TYPE_SHORTSPEAR:
    case WEAPON_TYPE_SLING:
    case WEAPON_TYPE_SPEAR:
      return TRUE;
    }
  }

  if (HAS_FEAT(ch, FEAT_INQUISITOR_WEAPON_PROFICIENCY) ||
      CLASS_LEVEL(ch, CLASS_INQUISITOR) > 0)
  {
    switch (weapon)
    {
    case WEAPON_TYPE_HAND_CROSSBOW:
    case WEAPON_TYPE_LONG_BOW:
    case WEAPON_TYPE_SHORT_BOW:
    case WEAPON_TYPE_LIGHT_REP_XBOW:
    case WEAPON_TYPE_HEAVY_REP_XBOW:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_2:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_3:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_4:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_5:
    case WEAPON_TYPE_COMPOSITE_LONGBOW:
    case WEAPON_TYPE_COMPOSITE_LONGBOW_2:
    case WEAPON_TYPE_COMPOSITE_LONGBOW_3:
    case WEAPON_TYPE_COMPOSITE_LONGBOW_4:
    case WEAPON_TYPE_COMPOSITE_LONGBOW_5:
      return TRUE;
    }
  }

  if (HAS_FEAT(ch, FEAT_WEAPON_PROFICIENCY_BARD) ||
      CLASS_LEVEL(ch, CLASS_BARD) > 0)
  {
    switch (weapon)
    {
    case WEAPON_TYPE_LONG_SWORD:
    case WEAPON_TYPE_RAPIER:
    case WEAPON_TYPE_SAP:
    case WEAPON_TYPE_SHORT_SWORD:
    case WEAPON_TYPE_SHORT_BOW:
    case WEAPON_TYPE_WHIP:
      return TRUE;
    }
  }

  if (HAS_FEAT(ch, FEAT_WEAPON_PROFICIENCY_ASSASSIN) ||
      CLASS_LEVEL(ch, CLASS_ASSASSIN) > 0)
  {
    switch (weapon)
    {
    case WEAPON_TYPE_HAND_CROSSBOW:
    case WEAPON_TYPE_LIGHT_CROSSBOW:
    case WEAPON_TYPE_HEAVY_CROSSBOW:
    case WEAPON_TYPE_DAGGER:
    case WEAPON_TYPE_KNIFE:
    case WEAPON_TYPE_DART:
    case WEAPON_TYPE_RAPIER:
    case WEAPON_TYPE_SHORT_BOW:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_2:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_3:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_4:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_5:
    case WEAPON_TYPE_SHORT_SWORD:
      return TRUE;
    }
  }

  if (HAS_FEAT(ch, FEAT_WEAPON_PROFICIENCY_ROGUE) ||
      CLASS_LEVEL(ch, CLASS_ROGUE) > 0)
  {
    switch (weapon)
    {
    case WEAPON_TYPE_HAND_CROSSBOW:
    case WEAPON_TYPE_RAPIER:
    case WEAPON_TYPE_SAP:
    case WEAPON_TYPE_SHORT_SWORD:
    case WEAPON_TYPE_SHORT_BOW:
      return TRUE;
    }
  }

  if (HAS_FEAT(ch, FEAT_WEAPON_PROFICIENCY_WIZARD) ||
      CLASS_LEVEL(ch, CLASS_WIZARD) > 0)
  {
    switch (weapon)
    {
    case WEAPON_TYPE_DAGGER:
    case WEAPON_TYPE_KNIFE:
    case WEAPON_TYPE_QUARTERSTAFF:
    case WEAPON_TYPE_CLUB:
    case WEAPON_TYPE_HEAVY_CROSSBOW:
    case WEAPON_TYPE_LIGHT_CROSSBOW:
      return TRUE;
    }
  }

  if (HAS_FEAT(ch, FEAT_WEAPON_PROFICIENCY_PSIONICIST) ||
      CLASS_LEVEL(ch, CLASS_PSIONICIST) > 0)
  {
    switch (weapon)
    {
    case WEAPON_TYPE_DAGGER:
    case WEAPON_TYPE_KNIFE:
    case WEAPON_TYPE_QUARTERSTAFF:
    case WEAPON_TYPE_CLUB:
    case WEAPON_TYPE_HEAVY_CROSSBOW:
    case WEAPON_TYPE_LIGHT_CROSSBOW:
    case WEAPON_TYPE_SHORTSPEAR:
      return TRUE;
    }
  }

  if (HAS_FEAT(ch, FEAT_WEAPON_PROFICIENCY_SHADOWDANCER) ||
      CLASS_LEVEL(ch, CLASS_SHADOWDANCER) > 0)
  {
    switch (weapon)
    {
    case WEAPON_TYPE_CLUB:
    case WEAPON_TYPE_HEAVY_CROSSBOW:
    case WEAPON_TYPE_LIGHT_CROSSBOW:
    case WEAPON_TYPE_HAND_CROSSBOW:
    case WEAPON_TYPE_DAGGER:
    case WEAPON_TYPE_KNIFE:
    case WEAPON_TYPE_KUKRI:
    case WEAPON_TYPE_DART:
    case WEAPON_TYPE_LIGHT_MACE:
    case WEAPON_TYPE_HEAVY_MACE:
    case WEAPON_TYPE_MORNINGSTAR:
    case WEAPON_TYPE_QUARTERSTAFF:
    case WEAPON_TYPE_RAPIER:
    case WEAPON_TYPE_SAP:
    case WEAPON_TYPE_SHORT_BOW:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_2:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_3:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_4:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_5:
    case WEAPON_TYPE_SHORT_SWORD:
      return TRUE;
    }
  }

  if (HAS_FEAT(ch, FEAT_WEAPON_PROFICIENCY_DROW) ||
      IS_DROW(ch))
  {
    switch (weapon)
    {
    case WEAPON_TYPE_HAND_CROSSBOW:
    case WEAPON_TYPE_RAPIER:
    case WEAPON_TYPE_SHORT_SWORD:
      return TRUE;
    }
  }

  if (HAS_FEAT(ch, FEAT_WEAPON_PROFICIENCY_ELF) ||
      IS_ELF(ch))
  {
    switch (weapon)
    {
    case WEAPON_TYPE_LONG_SWORD:
    case WEAPON_TYPE_RAPIER:
    case WEAPON_TYPE_LONG_BOW:
    case WEAPON_TYPE_COMPOSITE_LONGBOW:
    case WEAPON_TYPE_COMPOSITE_LONGBOW_2:
    case WEAPON_TYPE_COMPOSITE_LONGBOW_3:
    case WEAPON_TYPE_COMPOSITE_LONGBOW_4:
    case WEAPON_TYPE_COMPOSITE_LONGBOW_5:
    case WEAPON_TYPE_SHORT_BOW:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_2:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_3:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_4:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_5:
      return TRUE;
    }
  }

  if (IS_DWARF(ch))
  {
    switch (weapon)
    {
    case WEAPON_TYPE_BATTLE_AXE:
    case WEAPON_TYPE_HEAVY_PICK:
    case WEAPON_TYPE_WARHAMMER:
    case WEAPON_TYPE_DWARVEN_WAR_AXE:
    case WEAPON_TYPE_DWARVEN_URGOSH:
      return TRUE;
    }
  }

  if (HAS_FEAT(ch, FEAT_DWARVEN_WEAPON_PROFICIENCY))
  {
    switch (weapon)
    {
    case WEAPON_TYPE_BATTLE_AXE:
    case WEAPON_TYPE_HEAVY_PICK:
    case WEAPON_TYPE_LIGHT_PICK:
    case WEAPON_TYPE_WARHAMMER:
    case WEAPON_TYPE_LIGHT_HAMMER:
    case WEAPON_TYPE_DWARVEN_WAR_AXE:
    case WEAPON_TYPE_DWARVEN_URGOSH:
      return TRUE;
    }
  }

  if (IS_DUERGAR(ch))
  {
    switch (weapon)
    {
    case WEAPON_TYPE_BATTLE_AXE:
    case WEAPON_TYPE_HEAVY_PICK:
    case WEAPON_TYPE_WARHAMMER:
    case WEAPON_TYPE_DWARVEN_WAR_AXE:
    case WEAPON_TYPE_DWARVEN_URGOSH:
      return TRUE;
    }
  }

  /* cleric domain, favored weapons */
  if (!IS_NPC(ch) && domain_list[GET_1ST_DOMAIN(ch)].favored_weapon == weapon)
    return TRUE;
  if (!IS_NPC(ch) && domain_list[GET_2ND_DOMAIN(ch)].favored_weapon == weapon)
    return TRUE;

  /* TODO: Adapt this - Focus on an aspect of the divine, not a deity. */
  /*  if (HAS_FEAT((char_data *) ch, FEAT_DEITY_WEAPON_PROFICIENCY) && weapon == deity_list[GET_DEITY(ch)].favored_weapon)
      return TRUE;
   */

  /*  //deprecated
  if (HAS_COMBAT_FEAT(ch, CFEAT_EXOTIC_WEAPON_PROFICIENCY, DAMAGE_TYPE_SLASHING) &&
          IS_SET(weapon_list[weapon].weaponFlags, WEAPON_FLAG_EXOTIC) &&
          IS_SET(weapon_list[weapon].damageTypes, DAMAGE_TYPE_SLASHING)) {
    return TRUE;
  }
  if (HAS_COMBAT_FEAT(ch, CFEAT_EXOTIC_WEAPON_PROFICIENCY, DAMAGE_TYPE_PIERCING) &&
          IS_SET(weapon_list[weapon].weaponFlags, WEAPON_FLAG_EXOTIC) &&
          IS_SET(weapon_list[weapon].damageTypes, DAMAGE_TYPE_PIERCING)) {
    return TRUE;
  }
  if (HAS_COMBAT_FEAT(ch, CFEAT_EXOTIC_WEAPON_PROFICIENCY, DAMAGE_TYPE_BLUDGEONING) &&
          IS_SET(weapon_list[weapon].weaponFlags, WEAPON_FLAG_EXOTIC) &&
          IS_SET(weapon_list[weapon].damageTypes, DAMAGE_TYPE_BLUDGEONING)) {
    return TRUE;
  }
   */

  /* nope not proficient with given weapon! */
  return FALSE;
}

/* is weapon out of ammo? */
bool weapon_needs_reload(struct char_data *ch, struct obj_data *weapon, bool silent_mode)
{
  /* object value 5 is for loaded status */
  if (GET_OBJ_VAL(weapon, 5) > 0)
  {
    if (!silent_mode)
      send_to_char(ch, "Your weapon is not empty yet!\r\n");
    return FALSE;
  }
  return TRUE;
}

bool ready_to_reload(struct char_data *ch, struct obj_data *wielded, bool silent_mode)
{
  switch (GET_OBJ_VAL(wielded, 0))
  {
  case WEAPON_TYPE_HEAVY_REP_XBOW:
  case WEAPON_TYPE_LIGHT_REP_XBOW:
  case WEAPON_TYPE_HEAVY_CROSSBOW:

    /* RAPID RELOAD! */
    if (HAS_FEAT(ch, FEAT_RAPID_RELOAD))
    {
      if (is_action_available(ch, atMOVE, FALSE))
      {
        if (reload_weapon(ch, wielded, silent_mode))
        {
          USE_MOVE_ACTION(ch); /* success! */
        }
        else
        {
          /* failed reload */
          if (!silent_mode)
            send_to_char(ch, "You need a move action to reload!\r\n");
          return FALSE;
        }
      }
      else
      {
        /* reloading requires a move action */
        if (!silent_mode)
          send_to_char(ch, "You need a move action to reload!\r\n");
        return FALSE;
      }

      /* no rapid reload */
    }
    else if (is_action_available(ch, atSTANDARD, FALSE) &&
             is_action_available(ch, atMOVE, FALSE))
    {
      if (reload_weapon(ch, wielded, silent_mode))
      {
        USE_FULL_ROUND_ACTION(ch); /* success! */
      }
      else
      {
        if (!silent_mode)
          send_to_char(ch, "You need a full round action to reload!\r\n");
        /* failed reload */
        return FALSE;
      }
    }
    else
    {
      /* reloading requires a full round action */
      if (!silent_mode)
        send_to_char(ch, "You need a full round action to reload!\r\n");
      return FALSE;
    }

    break;
  case WEAPON_TYPE_HAND_CROSSBOW:
  case WEAPON_TYPE_LIGHT_CROSSBOW:
  case WEAPON_TYPE_SLING:

    /* RAPID RELOAD! */
    if (HAS_FEAT(ch, FEAT_RAPID_RELOAD))
      reload_weapon(ch, wielded, silent_mode);

    else if (is_action_available(ch, atMOVE, FALSE))
    {
      if (reload_weapon(ch, wielded, silent_mode))
      {
        USE_MOVE_ACTION(ch); /* success! */
      }
      else
      {
        /* failed reload */
        if (!silent_mode)
          send_to_char(ch, "You need a move action to reload!\r\n");
        return FALSE;
      }
    }
    else
    {
      /* reloading requires a move action */
      if (!silent_mode)
        send_to_char(ch, "You need a move action to reload!\r\n");
      return FALSE;
    }

    break;
  default:
    /* shouldn't get here */
    if (!silent_mode)
      send_to_char(ch, "The cucumber you are wielding is fully loaded! (error)\r\n");
    return FALSE;
  }

  /* we made it! */
  return TRUE;
}

/* trying to put shared proces between auto_reload_weapon
   and do_reload:
   disqualifiers such as position
   appropriate weapon?  (ranged + xbow type)
   does this actually need reloading (still has ammo)
   can reload (appropriate action available)
   then run reload_weapon() for actual reloading
   finally burn up appropriate action
 * @returns:  true if success */
bool process_load_weapon(struct char_data *ch, struct obj_data *weapon,
                         bool silent_mode)
{

  /* position check */
  if (GET_POS(ch) <= POS_STUNNED)
  {
    if (!silent_mode)
      send_to_char(ch, "You are in no position to do this!\r \n");
    return FALSE;
  }

  /* can't do this if stunned */
  if (AFF_FLAGGED(ch, AFF_STUN) || char_has_mud_event(ch, eSTUNNED))
  {
    if (!silent_mode)
      send_to_char(ch, "You can not reload a weapon while stunned!\r\n");
    return FALSE;
  }

  /* ranged weapon? */
  if (!is_using_ranged_weapon(ch, silent_mode))
  {
    return FALSE;
  }

  /* weapon that needs reloading? */
  if (!is_reloading_weapon(ch, weapon, silent_mode))
  {
    return FALSE;
  }

  /* emptied out yet? */
  if (!weapon_needs_reload(ch, weapon, silent_mode))
  {
    return FALSE;
  }

  /* check for actions, if available, reload */
  if (!ready_to_reload(ch, weapon, silent_mode))
  {
    return FALSE;
  }

  /* success! */
  send_to_char(ch, "You reload %s.\r\n", weapon->short_description);
  if (FIGHTING(ch))
    FIRING(ch) = TRUE;
  return TRUE;
}

/* ranged-weapons, reload mechanic for slings, crossbows */
bool auto_reload_weapon(struct char_data *ch, bool silent_mode)
{
  struct obj_data *wielded = is_using_ranged_weapon(ch, silent_mode);

  if (!process_load_weapon(ch, wielded, silent_mode))
    return FALSE;

  return TRUE;
}

#define MAX_AMMO_INSIDE_WEAPON 5 // unused

bool reload_weapon(struct char_data *ch, struct obj_data *wielded, bool silent_mode)
{
  int load_amount = 0;

  switch (GET_OBJ_VAL(wielded, 0))
  {
  case WEAPON_TYPE_HEAVY_REP_XBOW:
    load_amount = 5;
    break;
  case WEAPON_TYPE_LIGHT_REP_XBOW:
    load_amount = 3;
    break;
  case WEAPON_TYPE_HAND_CROSSBOW:
  case WEAPON_TYPE_HEAVY_CROSSBOW:
  case WEAPON_TYPE_LIGHT_CROSSBOW:
  case WEAPON_TYPE_SLING:
    load_amount = 1;
    break;
  default:
    return FALSE;
  }

  /* load her up! Object Value 5 is "loaded status" */
  GET_OBJ_VAL(wielded, 5) = load_amount;

  /* if we are in combat, let's make sure we start firing! */
  if (FIGHTING(ch))
    FIRING(ch) = TRUE;

  return TRUE;
}

/* this function checks if weapon is loaded (like crossbows) */
bool weapon_is_loaded(struct char_data *ch, struct obj_data *wielded, bool silent)
{

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTORELOAD) && FIGHTING(ch))
    silent = TRUE; /* ornir suggested this */

  if (GET_OBJ_VAL(wielded, 5) <= 0)
  { /* object value 5 is for loaded status */
    if (!silent)
      send_to_char(ch, "You have to reload your weapon!\r\n");
    FIRING(ch) = FALSE;
    return FALSE;
  }

  return TRUE;
}

/* this function will check to make sure ammo is ready for firing */
bool has_ammo_in_pouch(struct char_data *ch, struct obj_data *wielded,
                       bool silent)
{
  struct obj_data *ammo_pouch = GET_EQ(ch, WEAR_AMMO_POUCH);

  if (!wielded)
  {
    if (!silent)
      send_to_char(ch, "You have no weapon!\r\n");
    FIRING(ch) = FALSE;
    return FALSE;
  }

  if (!ammo_pouch)
  {
    if (!silent)
      send_to_char(ch, "You have no ammo pouch!\r\n");
    FIRING(ch) = FALSE;
    return FALSE;
  }

  if (!ammo_pouch->contains)
  {
    if (!silent)
      send_to_char(ch, "Your ammo pouch is empty!\r\n");
    FIRING(ch) = FALSE;
    return FALSE;
  }

  if (GET_OBJ_TYPE(ammo_pouch->contains) != ITEM_MISSILE)
  {
    if (!silent)
      send_to_char(ch, "Your ammo pouch needs to be filled with only ammo!\r\n");
    FIRING(ch) = FALSE;
    return FALSE;
  }

  switch (GET_OBJ_VAL(ammo_pouch->contains, 0))
  {

  case AMMO_TYPE_ARROW:
    switch (GET_OBJ_VAL(wielded, 0))
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
      break;
    default:
      if (!silent)
        act("Your $p requires a bow.", FALSE, ch, ammo_pouch->contains, NULL, TO_CHAR);
      FIRING(ch) = FALSE;
      return FALSE;
    }
    break;

  case AMMO_TYPE_BOLT:
    switch (GET_OBJ_VAL(wielded, 0))
    {
    case WEAPON_TYPE_HAND_CROSSBOW:
    case WEAPON_TYPE_HEAVY_REP_XBOW:
    case WEAPON_TYPE_LIGHT_REP_XBOW:
    case WEAPON_TYPE_HEAVY_CROSSBOW:
    case WEAPON_TYPE_LIGHT_CROSSBOW:
      break;
    default:
      if (!silent)
        act("Your $p requires a crossbow.", FALSE, ch, ammo_pouch->contains, NULL, TO_CHAR);
      FIRING(ch) = FALSE;
      return FALSE;
    }
    break;

  case AMMO_TYPE_STONE:
    switch (GET_OBJ_VAL(wielded, 0))
    {
    case WEAPON_TYPE_SLING:
      break;
    default:
      if (!silent)
        act("Your $p requires a sling.", FALSE, ch, ammo_pouch->contains, NULL, TO_CHAR);
      FIRING(ch) = FALSE;
      return FALSE;
    }
    break;

  case AMMO_TYPE_DART:
    switch (GET_OBJ_VAL(wielded, 0))
    {
    case WEAPON_TYPE_DART:
      break;
    default:
      if (!silent)
        act("Your $p requires a dart-gun.", FALSE, ch, ammo_pouch->contains, NULL, TO_CHAR);
      FIRING(ch) = FALSE;
      return FALSE;
    }
    break;

  case AMMO_TYPE_UNDEFINED:
  default:
    if (!silent)
      act("Your $p does not fit your weapon...", FALSE, ch, ammo_pouch->contains, NULL, TO_CHAR);
    FIRING(ch) = FALSE;
    return FALSE;
  }

  /* cleared all checks */
  return TRUE;
}

/* ranged combat (archery, etc)
 * this function will check for a ranged weapon, ammo and does
 * a check of loaded-status (like x-bow) and "has_ammo_in_pouch" */
bool can_fire_ammo(struct char_data *ch, bool silent)
{
  struct obj_data *wielded = NULL;

  if (!(wielded = is_using_ranged_weapon(ch, silent)))
  {
    FIRING(ch) = FALSE;
    return FALSE;
  }

  if (!has_ammo_in_pouch(ch, wielded, silent))
  {
    FIRING(ch) = FALSE;
    return FALSE;
  }

  /* ranged weapons that need reloading such as crossbows */
  /* this is handled in hit() now*
  if (is_reloading_weapon(ch, wielded)) {
    if (!weapon_is_loaded(ch, wielded, silent)) {
      //a message is sent in weapon_is_loaded()
      FIRING(ch) = FALSE;
      return FALSE;
    }
  }
   **/

  /* ok! */
  return TRUE;
}

/*check all wielded slots looking for ranged weapon*/
struct obj_data *is_using_ranged_weapon(struct char_data *ch, bool silent_mode)
{
  struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD_2H);

  if (!wielded)
    wielded = GET_EQ(ch, WEAR_WIELD_1);
  if (!wielded)
    wielded = GET_EQ(ch, WEAR_WIELD_OFFHAND);

  if (!wielded)
  {
    if (!silent_mode)
      send_to_char(ch, "You are not wielding a ranged weapon!\r\n");
    return NULL;
  }

  if (IS_WILDSHAPED(ch) || IS_MORPHED(ch))
  {
    if (!silent_mode)
      send_to_char(ch, "What?!!?\r\n");
    return NULL;
  }

  if (IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].weaponFlags, WEAPON_FLAG_RANGED))
    return wielded;

  if (!silent_mode)
    send_to_char(ch, "You are not wielding a ranged weapon!\r\n");
  return NULL;
}

/* is this ranged weapon the type that needs reloading? */
bool is_reloading_weapon(struct char_data *ch, struct obj_data *wielded, bool silent_mode)
{
  /* value 0 = weapon define value */
  switch (GET_OBJ_VAL(wielded, 0))
  {
  case WEAPON_TYPE_HEAVY_CROSSBOW:
  case WEAPON_TYPE_HEAVY_REP_XBOW:
  case WEAPON_TYPE_LIGHT_REP_XBOW:
  case WEAPON_TYPE_LIGHT_CROSSBOW:
  case WEAPON_TYPE_SLING:
  case WEAPON_TYPE_HAND_CROSSBOW:
    return TRUE;
  }
  if (!silent_mode)
    send_to_char(ch, "This is not a ranged weapon that needs reloading!\r\n");
  return FALSE;
}

/* more weapon flags we have to deal with */
/* can throw weapon, such as shuriken, throwing axes, etc */
//#define WEAPON_FLAG_THROWN      (1 << 4)
/* Reach: You use a reach weapon to strike opponents 10 feet away, but you can't
 * use it against an adjacent foe. */
//#define WEAPON_FLAG_REACH       (1 << 5)
//#define WEAPON_FLAG_ENTANGLE    (1 << 6)
/* Trip*: You can use a trip weapon to make trip attacks. If you are tripped
 * during your own trip attempt, you can drop the weapon to avoid being tripped
 * (*see FAQ/Errata.) */
//#define WEAPON_FLAG_TRIP        (1 << 7)
/* Disarm: When you use a disarm weapon, you get a +2 bonus on Combat Maneuver
 * Checks to disarm an enemy. */
//#define WEAPON_FLAG_DISARM      (1 << 9)
/* Nonlethal: These weapons deal nonlethal damage (see Combat). */
//#define WEAPON_FLAG_NONLETHAL   (1 << 10)
//#define WEAPON_FLAG_SLOW_RELOAD (1 << 11)
//#define WEAPON_FLAG_BALANCED    (1 << 12)
//#define WEAPON_FLAG_CHARGE      (1 << 13)
//#define WEAPON_FLAG_REPEATING   (1 << 14)
//#define WEAPON_FLAG_TWO_HANDED  (1 << 15)
/* Blocking: When you use this weapon to fight defensively, you gain a +1 shield
 * bonus to AC. Source: Ultimate Combat. */
//#define WEAPON_FLAG_BLOCKING    (1 << 17)
/* Brace: If you use a readied action to set a brace weapon against a charge,
 * you deal double damage on a successful hit against a charging creature
 * (see Combat). */
//#define WEAPON_FLAG_BRACING     (1 << 18)
/* Deadly: When you use this weapon to deliver a coup de grace, it gains a +4
 * bonus to damage when calculating the DC of the Fortitude saving throw to see
 * whether the target of the coup de grace dies from the attack. The bonus is
 * not added to the actual damage of the coup de grace attack.
 * Source: Ultimate Combat. */
//#define WEAPON_FLAG_DEADLY      (1 << 19)
/* Distracting: You gain a +2 bonus on Bluff skill checks to feint in combat
 * while wielding this weapon. Source: Ultimate Combat. */
//#define WEAPON_FLAG_DISTRACTING (1 << 20)
/* Fragile: Weapons and armor with the fragile quality cannot take the beating
 * that sturdier weapons can. A fragile weapon gains the broken condition if the
 * wielder rolls a natural 1 on an attack roll with the weapon. If a fragile
 * weapon is already broken, the roll of a natural 1 destroys it instead.
 * Masterwork and magical fragile weapons and armor lack these flaws unless
 * otherwise noted in the item description or the special material description.
 * If a weapon gains the broken condition in this way, that weapon is considered
 * to have taken damage equal to half its hit points +1. This damage is repaired
 * either by something that addresses the effect that granted the weapon the
 * broken condition (like quick clear in the case of firearm misfires or the
 * Field Repair feat) or by the repair methods described in the broken condition.
 * When an effect that grants the broken condition is removed, the weapon
 * regains the hit points it lost when the broken condition was applied. Damage
 * done by an attack against a weapon (such as from a sunder combat maneuver)
 * cannot be repaired by an effect that removes the broken condition.
 * Source: Ultimate Combat.*/
//#define WEAPON_FLAG_FRAGILE     (1 << 21)
/* Grapple: On a successful critical hit with a weapon of this type, you can
 * grapple the target of the attack. The wielder can then attempt a combat
 * maneuver check to grapple his opponent as a free action. This grapple attempt
 * does not provoke an attack of opportunity from the creature you are
 * attempting to grapple if that creature is not threatening you. While you
 * grapple the target with a grappling weapon, you can only move or damage the
 * creature on your turn. You are still considered grappled, though you do not
 * have to be adjacent to the creature to continue the grapple. If you move far
 * enough away to be out of the weapon’s reach, you end the grapple with that
 * action. Source: Ultimate Combat. */
//#define WEAPON_FLAG_GRAPPLING   (1 << 22)
/* Performance: When wielding this weapon, if an attack or combat maneuver made
 * with this weapon prompts a combat performance check, you gain a +2 bonus on
 * that check. See Gladiator Weapons below for more information. */
//#define WEAPON_FLAG_PERFORMANCE (1 << 23)
/* ***Strength (#): This feature is usually only applied to ranged weapons (such
 * as composite bows). Some weapons function better in the hands of stronger
 * users. All such weapons are made with a particular Strength rating (that is,
 * each requires a minimum Strength modifier to use with proficiency and this
 * number is included in parenthesis). If your Strength bonus is less than the
 * strength rating of the weapon, you can't effectively use it, so you take a –2
 * penalty on attacks with it. For example, the default (lowest form of)
 * composite longbow requires a Strength modifier of +0 or higher to use with
 * proficiency. A weapon with the Strength feature allows you to add your
 * Strength bonus to damage, up to the maximum bonus indicated for the bow. Each
 * point of Strength bonus granted by the bow adds 100 gp to its cost. If you
 * have a penalty for low Strength, apply it to damage rolls when you use a
 * composite longbow. Editor's Note: The "Strength" weapon feature was 'created'
 * by d20pfsrd.com as a shorthand note to the composite bow mechanics. This is
 * not "Paizo" or "official" content. */
//#define WEAPON_FLAG_STRENGTH    (1 << 24)
/* Sunder: When you use a sunder weapon, you get a +2 bonus on Combat Maneuver
 * Checks to sunder attempts. */
//#define WEAPON_FLAG_SUNDER      (1 << 25)

/* light weapons - necessary for some feats such as weapon finesse */
bool is_using_light_weapon(struct char_data *ch, struct obj_data *wielded)
{

  if (!wielded) /* fists are light?  i need to check this */
    return TRUE;

  if (GET_EQ(ch, WEAR_WIELD_OFFHAND) == wielded && HAS_FEAT(ch, FEAT_OVERSIZED_TWO_WEAPON_FIGHTING))
    return TRUE;

  if (GET_OBJ_SIZE(wielded) > GET_SIZE(ch))
    return FALSE;

  if (IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].weaponFlags, WEAPON_FLAG_LIGHT))
    return TRUE;
  if (IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].weaponFlags, WEAPON_FLAG_BALANCED))
    return TRUE;

  /* if you are a size classes bigger than the weapon, then it is light */
  if (GET_SIZE(ch) - GET_OBJ_SIZE(wielded) >= 1)
    return TRUE;

  /* not a light weapon! */
  return FALSE;
}

/* Double: You can use a double weapon to fight as if fighting with two weapons,
 * but if you do, you incur all the normal attack penalties associated with
 * fighting with two weapons, just as if you were using a one-handed weapon and
 * a light weapon. You can choose to wield one end of a double weapon two-handed,
 * but it cannot be used as a double weapon when wielded in this way—only one
 * end of the weapon can be used in any given round. */
bool is_using_double_weapon(struct char_data *ch)
{
  struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD_2H);

  /* we are going to say it is not enough that the weapon just be flagged
     double, we also need the weapon to be a size-class larger than the player */
  if (!wielded)
    return FALSE;
  if (GET_OBJ_SIZE(wielded) <= GET_SIZE(ch))
    return FALSE;

  if (IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].weaponFlags, WEAPON_FLAG_DOUBLE))
    return TRUE;

  return FALSE;
}

/* end utility, start base set/load/init functions for weapons/armor */

static void setweapon(int type, const char *name, int numDice, int diceSize, int critRange, int critMult,
                      int weaponFlags, int cost, int damageTypes, int weight, int range, int weaponFamily, int size,
                      int material, int handle_type, int head_type, const char *description)
{
  weapon_type[type] = strdup(name);
  weapon_list[type].name = name;
  weapon_list[type].numDice = numDice;
  weapon_list[type].diceSize = diceSize;
  weapon_list[type].critRange = critRange;
  if (critMult == 2)
    weapon_list[type].critMult = CRIT_X2;
  else if (critMult == 3)
    weapon_list[type].critMult = CRIT_X3;
  else if (critMult == 4)
    weapon_list[type].critMult = CRIT_X4;
  else if (critMult == 5)
    weapon_list[type].critMult = CRIT_X5;
  else if (critMult == 6)
    weapon_list[type].critMult = CRIT_X6;
  weapon_list[type].weaponFlags = weaponFlags;
  weapon_list[type].cost = cost;
  weapon_list[type].damageTypes = damageTypes;
  weapon_list[type].weight = weight;
  weapon_list[type].range = range;
  weapon_list[type].weaponFamily = weaponFamily;
  weapon_list[type].size = size;
  weapon_list[type].material = material;
  weapon_list[type].handle_type = handle_type;
  weapon_list[type].head_type = head_type;
  weapon_list[type].description = description;
}

void initialize_weapons(int type)
{
  weapon_list[type].name = "unused weapon";
  weapon_list[type].description = "unused weapon";
  weapon_list[type].numDice = 1;
  weapon_list[type].diceSize = 1;
  weapon_list[type].critRange = 0;
  weapon_list[type].critMult = 1;
  weapon_list[type].weaponFlags = 0;
  weapon_list[type].cost = 0;
  weapon_list[type].damageTypes = 0;
  weapon_list[type].weight = 0;
  weapon_list[type].range = 0;
  weapon_list[type].weaponFamily = 0;
  weapon_list[type].size = 0;
  weapon_list[type].material = 0;
  weapon_list[type].handle_type = 0;
  weapon_list[type].head_type = 0;
}

void load_weapons(void)
{
  int i = 0;

  for (i = 0; i < NUM_WEAPON_TYPES; i++)
    initialize_weapons(i);

  /*	(weapon num, name, numDamDice, sizeDamDice, critRange, critMult, weapon flags, cost,
   * damageType, weight, range, weaponFamily, Size, material,
   * handle, head) */
  setweapon(WEAPON_TYPE_UNARMED, "knuckle", 1, 3, 0, 2, WEAPON_FLAG_SIMPLE, 2,
            DAMAGE_TYPE_BLUDGEONING, 1, 0, WEAPON_FAMILY_MONK, SIZE_SMALL, MATERIAL_ORGANIC,
            HANDLE_TYPE_GLOVE, HEAD_TYPE_FIST,
            "A knuckle can be any type of unarmed weapon, such as a spiked gauntlet, brass knuckles, hand wraps, cesti, and so forth.");
  setweapon(WEAPON_TYPE_DAGGER, "dagger", 1, 4, 1, 2, WEAPON_FLAG_THROWN | WEAPON_FLAG_SIMPLE, 2, DAMAGE_TYPE_PIERCING, 1, 10, WEAPON_FAMILY_SMALL_BLADE, SIZE_TINY,
            MATERIAL_STEEL, HANDLE_TYPE_HILT, HEAD_TYPE_BLADE,
            "A dagger has a blade that is about 1 foot in length.");
  setweapon(WEAPON_TYPE_KNIFE, "knife", 1, 3, 1, 2, WEAPON_FLAG_THROWN | WEAPON_FLAG_SIMPLE, 2, DAMAGE_TYPE_PIERCING, 1, 10, WEAPON_FAMILY_SMALL_BLADE, SIZE_DIMINUTIVE,
            MATERIAL_STEEL, HANDLE_TYPE_HILT, HEAD_TYPE_BLADE,
            "A dagger has a blade that is about 1 foot in length.");
  setweapon(WEAPON_TYPE_LIGHT_MACE, "light mace", 1, 6, 0, 2, WEAPON_FLAG_SIMPLE, 5,
            DAMAGE_TYPE_BLUDGEONING, 4, 0, WEAPON_FAMILY_CLUB, SIZE_SMALL, MATERIAL_STEEL,
            HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD,
            "A light mace is made up of an ornate metal head attached to a simple wooden or metal shaft.");
  setweapon(WEAPON_TYPE_SICKLE, "sickle", 1, 6, 0, 2, WEAPON_FLAG_SIMPLE | WEAPON_FLAG_TRIP, 6,
            DAMAGE_TYPE_SLASHING, 2, 0, WEAPON_FAMILY_SMALL_BLADE, SIZE_SMALL, MATERIAL_STEEL,
            HANDLE_TYPE_HANDLE, HEAD_TYPE_BLADE,
            "A sickle is usually used for harvesting crops, but can also be used as a weapon.  It has a curving C-shaped blade.");
  setweapon(WEAPON_TYPE_CLUB, "club", 1, 6, 0, 2, WEAPON_FLAG_SIMPLE, 1,
            DAMAGE_TYPE_BLUDGEONING, 3, 0, WEAPON_FAMILY_CLUB, SIZE_SMALL, MATERIAL_WOOD,
            HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD,
            "This weapon is usually just a shaped piece of wood, sometimes with a few nails or studs embedded in it.");
  setweapon(WEAPON_TYPE_HEAVY_MACE, "heavy mace", 1, 8, 0, 2, WEAPON_FLAG_SIMPLE, 12,
            DAMAGE_TYPE_BLUDGEONING, 8, 0, WEAPON_FAMILY_CLUB, SIZE_MEDIUM, MATERIAL_STEEL,
            HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD,
            "A heavy mace has a larger head and a longer handle than a normal (light) mace.");
  setweapon(WEAPON_TYPE_MORNINGSTAR, "morningstar", 1, 8, 0, 2, WEAPON_FLAG_SIMPLE, 8,
            DAMAGE_TYPE_BLUDGEONING | DAMAGE_TYPE_PIERCING, 6, 0, WEAPON_FAMILY_FLAIL, SIZE_MEDIUM,
            MATERIAL_STEEL, HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD,
            "A morningstar is a spiked metal ball, affixed to the top of a long handle.");
  setweapon(WEAPON_TYPE_SHORTSPEAR, "shortspear", 1, 6, 0, 2, WEAPON_FLAG_SIMPLE | WEAPON_FLAG_THROWN, 1, DAMAGE_TYPE_PIERCING, 3, 20, WEAPON_FAMILY_SPEAR, SIZE_MEDIUM,
            MATERIAL_WOOD, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT,
            "A shortspear is about 3 feet in length, making it a suitable thrown weapon.");
  setweapon(WEAPON_TYPE_LONGSPEAR, "longspear", 1, 8, 0, 3, WEAPON_FLAG_SIMPLE | WEAPON_FLAG_REACH, 5, DAMAGE_TYPE_PIERCING, 9, 0, WEAPON_FAMILY_SPEAR, SIZE_LARGE,
            MATERIAL_WOOD, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT,
            "A longspear is about 8 feet in length.");
  setweapon(WEAPON_TYPE_QUARTERSTAFF, "quarterstaff", 1, 6, 0, 2, WEAPON_FLAG_SIMPLE,
            1, DAMAGE_TYPE_BLUDGEONING, 4, 0, WEAPON_FAMILY_MONK, SIZE_LARGE,
            MATERIAL_WOOD, HANDLE_TYPE_SHAFT, HEAD_TYPE_HEAD,
            "A quarterstaff is a simple piece of wood, about 5 feet in length.");
  setweapon(WEAPON_TYPE_SPEAR, "spear", 1, 8, 0, 3, WEAPON_FLAG_SIMPLE | WEAPON_FLAG_THROWN | WEAPON_FLAG_REACH, 2, DAMAGE_TYPE_PIERCING, 6, 20, WEAPON_FAMILY_SPEAR, SIZE_LARGE,
            MATERIAL_WOOD, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT,
            "A spear is 5 feet in length and can be thrown.");
  setweapon(WEAPON_TYPE_HEAVY_CROSSBOW, "heavy crossbow", 1, 10, 1, 2, WEAPON_FLAG_SIMPLE | WEAPON_FLAG_SLOW_RELOAD | WEAPON_FLAG_RANGED, 50, DAMAGE_TYPE_PIERCING, 8, 120,
            WEAPON_FAMILY_CROSSBOW, SIZE_MEDIUM, MATERIAL_WOOD, HANDLE_TYPE_HANDLE, HEAD_TYPE_BOW,
            "A heavy crossbow is, as its namesake, a larger version than a regular (light) crossbow. It fires with greater force but takes a -4 penalty to hit if fired with one hand.");
  setweapon(WEAPON_TYPE_LIGHT_CROSSBOW, "light crossbow", 1, 8, 1, 2, WEAPON_FLAG_SIMPLE | WEAPON_FLAG_SLOW_RELOAD | WEAPON_FLAG_RANGED, 35, DAMAGE_TYPE_PIERCING, 4, 80,
            WEAPON_FAMILY_CROSSBOW, SIZE_MEDIUM, MATERIAL_WOOD, HANDLE_TYPE_HANDLE, HEAD_TYPE_BOW,
            "A light crossbow is a lighter version of a normal crossbow. It fires with less force than a heavy crossbow, and takes a -2 penalty to hit if fired with one hand.");
  /*	(weapon num, name, numDamDice, sizeDamDice, critRange, critMult, weapon flags, cost, damageType, weight, range, weaponFamily, Size, material, handle, head) */
  setweapon(WEAPON_TYPE_DART, "dart", 1, 4, 0, 2, WEAPON_FLAG_SIMPLE | WEAPON_FLAG_THROWN | WEAPON_FLAG_RANGED, 1, DAMAGE_TYPE_PIERCING, 1, 20, WEAPON_FAMILY_THROWN, SIZE_TINY,
            MATERIAL_WOOD, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT,
            "Darts are missile weapons, designed to fly such that a sharp, often weighted point will strike first. They can be distinguished from javelins by fletching (i.e., feathers on the tail) and a shaft that is shorter and/or more flexible, and from arrows by the fact that they are not of the right length to use with a normal bow.");
  setweapon(WEAPON_TYPE_JAVELIN, "javelin", 1, 6, 0, 2, WEAPON_FLAG_SIMPLE | WEAPON_FLAG_THROWN | WEAPON_FLAG_RANGED, 1, DAMAGE_TYPE_PIERCING, 2, 30,
            WEAPON_FAMILY_SPEAR, SIZE_MEDIUM, MATERIAL_WOOD, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT,
            "A javelin is a thin throwing spear.");
  setweapon(WEAPON_TYPE_SLING, "sling", 1, 4, 0, 2, WEAPON_FLAG_SIMPLE | WEAPON_FLAG_RANGED, 1, DAMAGE_TYPE_BLUDGEONING, 1, 50, WEAPON_FAMILY_THROWN, SIZE_SMALL,
            MATERIAL_LEATHER, HANDLE_TYPE_STRAP, HEAD_TYPE_POUCH,
            "A sling is little more than a leather cup attached to a pair of strings.");
  setweapon(WEAPON_TYPE_THROWING_AXE, "throwing axe", 1, 6, 0, 2, WEAPON_FLAG_MARTIAL | WEAPON_FLAG_THROWN, 8, DAMAGE_TYPE_SLASHING, 2, 10, WEAPON_FAMILY_AXE, SIZE_SMALL,
            MATERIAL_STEEL, HANDLE_TYPE_HANDLE, HEAD_TYPE_BLADE,
            "This is a small axe balanced for throwing.");
  setweapon(WEAPON_TYPE_LIGHT_HAMMER, "light hammer", 1, 4, 0, 2, WEAPON_FLAG_MARTIAL | WEAPON_FLAG_THROWN, 1, DAMAGE_TYPE_BLUDGEONING, 2, 20, WEAPON_FAMILY_HAMMER, SIZE_SMALL,
            MATERIAL_STEEL, HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD,
            "A lighter version of a warhammer, this weapon usually has a sleek metal head with one striking surface.");
  setweapon(WEAPON_TYPE_HAND_AXE, "hand axe", 1, 6, 0, 3, WEAPON_FLAG_MARTIAL, 6,
            DAMAGE_TYPE_SLASHING, 3, 0, WEAPON_FAMILY_AXE, SIZE_SMALL, MATERIAL_STEEL, HANDLE_TYPE_HANDLE,
            HEAD_TYPE_BLADE,
            "This one-handed axe is short (roughly 1 foot long) and designed for use with one hand. Unlike throwing axes, it is not well balanced for a graceful tumbling motion, and is instead heavier at its head. Tomahawks, war hatchets, and other such names usually refer to hand axes.");
  setweapon(WEAPON_TYPE_KUKRI, "kukri", 1, 4, 2, 2, WEAPON_FLAG_MARTIAL, 8,
            DAMAGE_TYPE_SLASHING, 2, 0, WEAPON_FAMILY_SMALL_BLADE, SIZE_SMALL, MATERIAL_STEEL,
            HANDLE_TYPE_HILT, HEAD_TYPE_BLADE,
            "A kukri is a curved blade, about 1 foot in length.");
  setweapon(WEAPON_TYPE_LIGHT_PICK, "light pick", 1, 4, 0, 4, WEAPON_FLAG_MARTIAL, 4,
            DAMAGE_TYPE_PIERCING, 3, 0, WEAPON_FAMILY_PICK, SIZE_SMALL, MATERIAL_STEEL,
            HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD,
            "This weapon, adapted from the pickaxe tool, has a head with a slightly curved, armorpiercing spike and a hammerlike counterweight.");
  setweapon(WEAPON_TYPE_SAP, "sap", 1, 6, 0, 2, WEAPON_FLAG_MARTIAL | WEAPON_FLAG_NONLETHAL, 1, DAMAGE_TYPE_BLUDGEONING | DAMAGE_TYPE_NONLETHAL, 2, 0,
            WEAPON_FAMILY_CLUB, SIZE_SMALL, MATERIAL_LEATHER, HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD,
            "This weapon consists of a soft wrapping around a hard, dense core, typically a leather sheath around a lead rod. The head is wider than the handle and designed to spread out the force of the blow, making it less likely to draw blood or break bones.");
  setweapon(WEAPON_TYPE_SHORT_SWORD, "short sword", 1, 6, 1, 2, WEAPON_FLAG_MARTIAL,
            10, DAMAGE_TYPE_PIERCING, 2, 0, WEAPON_FAMILY_SMALL_BLADE, SIZE_SMALL, MATERIAL_STEEL,
            HANDLE_TYPE_HILT, HEAD_TYPE_BLADE,
            "Short swords are some of the most common weapons found in any martial society, and thus designs are extremely varied, depending on the region and creator. Most are around 2 feet in length. Their blades can be curved or straight, single- or double-edged, and wide or narrow. Hilts may be ornate or simple, with crossguards, basket hilts, or no guard at all. Such weapons are often used on their own, but can also be paired as a matched set, or used in conjunction with a dagger or longer sword.");
  setweapon(WEAPON_TYPE_BATTLE_AXE, "battle axe", 1, 8, 0, 3, WEAPON_FLAG_MARTIAL, 10,
            DAMAGE_TYPE_SLASHING, 6, 0, WEAPON_FAMILY_AXE, SIZE_MEDIUM, MATERIAL_STEEL,
            HANDLE_TYPE_HANDLE, HEAD_TYPE_BLADE,
            "The handle of this axe is long enough that you can wield it one-handed or two-handed. The head may have one blade or two, with blade shapes ranging from half-moons to squared edges like narrower versions of woodcutting axes. The wooden haft may be protected and strengthened with metal bands called langets.");
  /*	(weapon num, name, numDamDice, sizeDamDice, critRange, critMult, weapon flags, cost, damageType, weight, range, weaponFamily, Size, material, handle, head) */
  setweapon(WEAPON_TYPE_FLAIL, "flail", 1, 8, 0, 2, WEAPON_FLAG_MARTIAL, 8,
            DAMAGE_TYPE_BLUDGEONING, 5, 0, WEAPON_FAMILY_FLAIL, SIZE_MEDIUM, MATERIAL_STEEL,
            HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD,
            "A flail consists of a weighted striking end connected to a handle by a sturdy chain. Though often imagined as a ball, sometimes spiked like the head of a morningstar, the head of a light flail can actually take many different shapes, such as short bars. Military flails are sturdier evolutions of agricultural flails, which are used for threshing – beating stacks of grains to separate the useful grains from their husks.");
  setweapon(WEAPON_TYPE_LONG_SWORD, "long sword", 1, 8, 1, 2, WEAPON_FLAG_MARTIAL, 15,
            DAMAGE_TYPE_SLASHING, 4, 0, WEAPON_FAMILY_MEDIUM_BLADE, SIZE_MEDIUM, MATERIAL_STEEL,
            HANDLE_TYPE_HILT, HEAD_TYPE_BLADE,
            "This sword is about 3 1/2 feet in length.");
  setweapon(WEAPON_TYPE_HEAVY_PICK, "heavy pick", 1, 6, 0, 4, WEAPON_FLAG_MARTIAL, 8,
            DAMAGE_TYPE_PIERCING, 6, 0, WEAPON_FAMILY_PICK, SIZE_MEDIUM, MATERIAL_STEEL,
            HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD,
            "This variant of the light pick has a longer handle and can be used with one or two hands. It is a common, inexpensive weapon for mounted soldiers since it can be used effectively from horseback.");
  setweapon(WEAPON_TYPE_RAPIER, "rapier", 1, 6, 2, 2, WEAPON_FLAG_MARTIAL | WEAPON_FLAG_BALANCED, 20, DAMAGE_TYPE_PIERCING, 2, 0, WEAPON_FAMILY_SMALL_BLADE,
            SIZE_SMALL, MATERIAL_STEEL, HANDLE_TYPE_HILT, HEAD_TYPE_BLADE,
            "A thin, light sharp-pointed sword used for thrusting.");
  setweapon(WEAPON_TYPE_SCIMITAR, "scimitar", 1, 6, 2, 2, WEAPON_FLAG_MARTIAL, 15,
            DAMAGE_TYPE_SLASHING, 4, 0, WEAPON_FAMILY_MEDIUM_BLADE, SIZE_MEDIUM, MATERIAL_STEEL,
            HANDLE_TYPE_HILT, HEAD_TYPE_BLADE,
            "This curved sword is shorter than a longsword and longer than a shortsword. Only the outer edge is sharp, and the back is flat, giving the blade a triangular cross-section.");
  setweapon(WEAPON_TYPE_TRIDENT, "trident", 1, 8, 0, 2, WEAPON_FLAG_MARTIAL | WEAPON_FLAG_THROWN, 15, DAMAGE_TYPE_PIERCING, 4, 0, WEAPON_FAMILY_SPEAR, SIZE_MEDIUM,
            MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT,
            "A trident has three metal prongs at the end of a 4-foot-long shaft.");
  setweapon(WEAPON_TYPE_WARHAMMER, "warhammer", 1, 8, 0, 3, WEAPON_FLAG_MARTIAL, 12,
            DAMAGE_TYPE_BLUDGEONING, 5, 0, WEAPON_FAMILY_HAMMER, SIZE_MEDIUM, MATERIAL_STEEL,
            HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD,
            "This weapon consists of a wooden haft and a heavy, metal head. The head may be single (like a carpenter’s hammer) or double (like a sledgehammer). The haft is long enough that you may wield it one- or two-handed. Though heavy and relatively slow to wield, warhammers are capable of delivering immense blows, crushing armor and flesh alike.");
  setweapon(WEAPON_TYPE_FALCHION, "falchion", 2, 4, 2, 2, WEAPON_FLAG_MARTIAL, 75,
            DAMAGE_TYPE_SLASHING, 8, 0, WEAPON_FAMILY_LARGE_BLADE, SIZE_LARGE, MATERIAL_STEEL,
            HANDLE_TYPE_HILT, HEAD_TYPE_BLADE,
            "This sword has one curved, sharp edge like a scimitar, with the back edge unsharpened and either flat or slightly curved. Its weight is greater toward the end, making it better for chopping rather than stabbing.");
  setweapon(WEAPON_TYPE_GLAIVE, "glaive", 1, 10, 0, 3, WEAPON_FLAG_MARTIAL | WEAPON_FLAG_REACH, 8, DAMAGE_TYPE_SLASHING, 10, 0, WEAPON_FAMILY_POLEARM, SIZE_LARGE,
            MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_BLADE,
            "A glaive is composed of a simple blade mounted on the end of a pole about 7 feet in length.");
  setweapon(WEAPON_TYPE_GREAT_AXE, "great axe", 1, 12, 0, 3, WEAPON_FLAG_MARTIAL, 20,
            DAMAGE_TYPE_SLASHING, 12, 0, WEAPON_FAMILY_AXE, SIZE_LARGE, MATERIAL_STEEL,
            HANDLE_TYPE_HANDLE, HEAD_TYPE_BLADE,
            "This two-handed battle axe is heavy enough that you can’t wield it with one hand. The head may have one blade or two, and may be “bearded” (meaning hooked or trailing at the bottom) to increase cleaving power and help pull down enemy shields. The haft is usually 3 to 4 feet long.");
  setweapon(WEAPON_TYPE_GREAT_CLUB, "great club", 1, 10, 0, 2, WEAPON_FLAG_MARTIAL, 5,
            DAMAGE_TYPE_BLUDGEONING, 8, 0, WEAPON_FAMILY_CLUB, SIZE_LARGE, MATERIAL_WOOD,
            HANDLE_TYPE_SHAFT, HEAD_TYPE_HEAD,
            "This larger, bulkier version of the common club is heavy enough that you can’t wield it with one hand. It may be ornate and carved, reinforced with metal, or a simple branch from a tree. Like simple clubs, greatclubs have many names, such as cudgels, bludgeons, shillelaghs, and more.");
  /*	(weapon num, name, numDamDice, sizeDamDice, critRange, critMult, weapon flags, cost, damageType, weight, range, weaponFamily, Size, material, handle, head) */
  setweapon(WEAPON_TYPE_HEAVY_FLAIL, "heavy flail", 1, 10, 1, 2, WEAPON_FLAG_MARTIAL,
            15, DAMAGE_TYPE_BLUDGEONING, 10, 0, WEAPON_FAMILY_FLAIL, SIZE_LARGE, MATERIAL_STEEL,
            HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD,
            "Similar to a light flail, a heavy flail has a larger metal ball and a longer handle.");
  setweapon(WEAPON_TYPE_GREAT_SWORD, "great sword", 2, 6, 1, 2, WEAPON_FLAG_MARTIAL,
            50, DAMAGE_TYPE_SLASHING, 8, 0, WEAPON_FAMILY_LARGE_BLADE, SIZE_LARGE, MATERIAL_STEEL,
            HANDLE_TYPE_HILT, HEAD_TYPE_BLADE,
            "This immense two-handed sword is about 5 feet in length. A greatsword may have a dulled lower blade that can be gripped.");
  setweapon(WEAPON_TYPE_GUISARME, "guisarme", 2, 4, 0, 3, WEAPON_FLAG_MARTIAL | WEAPON_FLAG_REACH, 9, DAMAGE_TYPE_SLASHING, 12, 0, WEAPON_FAMILY_POLEARM, SIZE_LARGE,
            MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_BLADE,
            "A guisarme is an 8-foot-long shaft with a blade and a hook mounted at the tip.");
  setweapon(WEAPON_TYPE_HALBERD, "halberd", 1, 10, 0, 3, WEAPON_FLAG_MARTIAL | WEAPON_FLAG_REACH, 10, DAMAGE_TYPE_SLASHING | DAMAGE_TYPE_PIERCING, 12, 0,
            WEAPON_FAMILY_POLEARM, SIZE_LARGE, MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_BLADE,
            "A halberd is similar to a 5-foot-long spear, but it also has a small, axe-like head mounted near the tip.");
  setweapon(WEAPON_TYPE_LANCE, "lance", 1, 8, 0, 3, WEAPON_FLAG_MARTIAL | WEAPON_FLAG_REACH | WEAPON_FLAG_CHARGE, 10, DAMAGE_TYPE_PIERCING, 10, 0,
            WEAPON_FAMILY_POLEARM, SIZE_LARGE, MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT,
            "A steel-tipped spear carried by mounted knights or light cavalry.");
  setweapon(WEAPON_TYPE_FOOTMANS_LANCE, "footmans lance", 1, 6, 0, 3, 
            WEAPON_FLAG_MARTIAL | WEAPON_FLAG_REACH | WEAPON_FLAG_CHARGE, 6, DAMAGE_TYPE_PIERCING, 10, 0,
            WEAPON_FAMILY_POLEARM, SIZE_MEDIUM, MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT,
            "A steel-tipped spear carried by mounted knights or light cavalry.");
  setweapon(WEAPON_TYPE_RANSEUR, "ranseur", 2, 4, 0, 3, WEAPON_FLAG_MARTIAL | WEAPON_FLAG_REACH, 10, DAMAGE_TYPE_PIERCING, 10, 0, WEAPON_FAMILY_POLEARM, SIZE_LARGE,
            MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT,
            "Similar in appearance to a trident, a ranseur has a single spear at its tip, f lanked by a pair of short, curving blades.");
  setweapon(WEAPON_TYPE_SCYTHE, "scythe", 2, 4, 0, 4, WEAPON_FLAG_MARTIAL, 18,
            DAMAGE_TYPE_SLASHING, 10, 0, WEAPON_FAMILY_POLEARM, SIZE_LARGE,
            MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_BLADE,
            "This weapon consists of a long wooden shaft with protruding handles and a sharp curved blade set at a right angle. Derived from a farm tool used to mow down crops, a scythe requires two hands to use, and is unwieldy but capable of inflicting grievous wounds. Its connotations as a symbol of death make it an intimidating weapon.");
  setweapon(WEAPON_TYPE_LONG_BOW, "long bow", 1, 8, 0, 3, WEAPON_FLAG_MARTIAL | WEAPON_FLAG_RANGED, 75, DAMAGE_TYPE_PIERCING, 3, 100, WEAPON_FAMILY_BOW, SIZE_MEDIUM,
            MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW,
            "At almost 5 feet in height, a longbow is made up of one solid piece of carefully curved wood.");

  setweapon(WEAPON_TYPE_COMPOSITE_LONGBOW, "composite long bow", 1, 8, 0, 3,
            WEAPON_FLAG_MARTIAL | WEAPON_FLAG_RANGED, 100, DAMAGE_TYPE_PIERCING, 3, 110,
            WEAPON_FAMILY_BOW, SIZE_MEDIUM, MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW,
            "A composite bow is a traditional bow made from horn, wood, and sinew laminated together, a form of laminated bow. The horn is on the belly, facing the archer, and sinew on the outer side of a wooden core.");
  setweapon(WEAPON_TYPE_COMPOSITE_LONGBOW_2, "composite long bow (2)", 1, 8, 0, 3,
            WEAPON_FLAG_MARTIAL | WEAPON_FLAG_RANGED, 200, DAMAGE_TYPE_PIERCING, 3, 110,
            WEAPON_FAMILY_BOW, SIZE_MEDIUM, MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW,
            "A composite bow is a traditional bow made from horn, wood, and sinew laminated together, a form of laminated bow. The horn is on the belly, facing the archer, and sinew on the outer side of a wooden core.");
  setweapon(WEAPON_TYPE_COMPOSITE_LONGBOW_3, "composite long bow (3)", 1, 8, 0, 3,
            WEAPON_FLAG_MARTIAL | WEAPON_FLAG_RANGED, 300, DAMAGE_TYPE_PIERCING, 3, 110,
            WEAPON_FAMILY_BOW, SIZE_MEDIUM, MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW,
            "A composite bow is a traditional bow made from horn, wood, and sinew laminated together, a form of laminated bow. The horn is on the belly, facing the archer, and sinew on the outer side of a wooden core.");
  setweapon(WEAPON_TYPE_COMPOSITE_LONGBOW_4, "composite long bow (4)", 1, 8, 0, 3,
            WEAPON_FLAG_MARTIAL | WEAPON_FLAG_RANGED, 400, DAMAGE_TYPE_PIERCING, 3, 110,
            WEAPON_FAMILY_BOW, SIZE_MEDIUM, MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW,
            "A composite bow is a traditional bow made from horn, wood, and sinew laminated together, a form of laminated bow. The horn is on the belly, facing the archer, and sinew on the outer side of a wooden core.");
  setweapon(WEAPON_TYPE_COMPOSITE_LONGBOW_5, "composite long bow (5)", 1, 8, 0, 3,
            WEAPON_FLAG_MARTIAL | WEAPON_FLAG_RANGED, 500, DAMAGE_TYPE_PIERCING, 3, 110,
            WEAPON_FAMILY_BOW, SIZE_MEDIUM, MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW,
            "A composite bow is a traditional bow made from horn, wood, and sinew laminated together, a form of laminated bow. The horn is on the belly, facing the archer, and sinew on the outer side of a wooden core.");

  setweapon(WEAPON_TYPE_SHORT_BOW, "short bow", 1, 6, 0, 3, WEAPON_FLAG_MARTIAL | WEAPON_FLAG_RANGED, 30, DAMAGE_TYPE_PIERCING, 2, 60, WEAPON_FAMILY_BOW, SIZE_MEDIUM,
            MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW,
            "A shortbow is made up of one piece of wood, about 3 feet in length.");

  setweapon(WEAPON_TYPE_COMPOSITE_SHORTBOW, "composite short bow", 1, 6, 0, 3,
            WEAPON_FLAG_MARTIAL | WEAPON_FLAG_RANGED, 75, DAMAGE_TYPE_PIERCING, 2, 70,
            WEAPON_FAMILY_BOW, SIZE_SMALL, MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW,
            "A composite bow is a traditional bow made from horn, wood, and sinew laminated together, a form of laminated bow. The horn is on the belly, facing the archer, and sinew on the outer side of a wooden core.");
  setweapon(WEAPON_TYPE_COMPOSITE_SHORTBOW_2, "composite short bow (2)", 1, 6, 0, 3,
            WEAPON_FLAG_MARTIAL | WEAPON_FLAG_RANGED, 175, DAMAGE_TYPE_PIERCING, 2, 70,
            WEAPON_FAMILY_BOW, SIZE_SMALL, MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW,
            "A composite bow is a traditional bow made from horn, wood, and sinew laminated together, a form of laminated bow. The horn is on the belly, facing the archer, and sinew on the outer side of a wooden core.");
  setweapon(WEAPON_TYPE_COMPOSITE_SHORTBOW_3, "composite short bow (3)", 1, 6, 0, 3,
            WEAPON_FLAG_MARTIAL | WEAPON_FLAG_RANGED, 275, DAMAGE_TYPE_PIERCING, 2, 70,
            WEAPON_FAMILY_BOW, SIZE_SMALL, MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW,
            "A composite bow is a traditional bow made from horn, wood, and sinew laminated together, a form of laminated bow. The horn is on the belly, facing the archer, and sinew on the outer side of a wooden core.");
  setweapon(WEAPON_TYPE_COMPOSITE_SHORTBOW_4, "composite short bow (4)", 1, 6, 0, 3,
            WEAPON_FLAG_MARTIAL | WEAPON_FLAG_RANGED, 375, DAMAGE_TYPE_PIERCING, 2, 70,
            WEAPON_FAMILY_BOW, SIZE_SMALL, MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW,
            "A composite bow is a traditional bow made from horn, wood, and sinew laminated together, a form of laminated bow. The horn is on the belly, facing the archer, and sinew on the outer side of a wooden core.");
  setweapon(WEAPON_TYPE_COMPOSITE_SHORTBOW_5, "composite short bow (5)", 1, 6, 0, 3,
            WEAPON_FLAG_MARTIAL | WEAPON_FLAG_RANGED, 475, DAMAGE_TYPE_PIERCING, 2, 70,
            WEAPON_FAMILY_BOW, SIZE_SMALL, MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW,
            "A composite bow is a traditional bow made from horn, wood, and sinew laminated together, a form of laminated bow. The horn is on the belly, facing the archer, and sinew on the outer side of a wooden core.");

  /*	(weapon num, name, numDamDice, sizeDamDice, critRange, critMult, weapon flags, cost, damageType, weight, range, weaponFamily, Size, material, handle, head) */
  setweapon(WEAPON_TYPE_KAMA, "kama", 1, 6, 0, 2, WEAPON_FLAG_EXOTIC, 2,
            DAMAGE_TYPE_SLASHING, 2, 0, WEAPON_FAMILY_MONK, SIZE_SMALL, MATERIAL_STEEL,
            HANDLE_TYPE_HANDLE, HEAD_TYPE_BLADE,
            "Similar to a sickle-and in some regions still used to reap grain-a kama is a short, curved blade attached to a simple handle, usually made of wood. It is sometimes also referred to as a kai, and is frequently used in pairs by martial artists.");
  setweapon(WEAPON_TYPE_NUNCHAKU, "nunchaku", 1, 6, 1, 2, WEAPON_FLAG_EXOTIC, 2,
            DAMAGE_TYPE_BLUDGEONING, 2, 0, WEAPON_FAMILY_MONK, SIZE_SMALL, MATERIAL_WOOD,
            HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD,
            "A nunchaku is made up of two wooden or metal bars connected by a small length of rope or chain.");
  setweapon(WEAPON_TYPE_SAI, "sai", 1, 4, 1, 2, WEAPON_FLAG_EXOTIC | WEAPON_FLAG_THROWN,
            1, DAMAGE_TYPE_BLUDGEONING, 1, 10, WEAPON_FAMILY_MONK, SIZE_SMALL, MATERIAL_STEEL,
            HANDLE_TYPE_HANDLE, HEAD_TYPE_POINT,
            "A sai is a metal spike flanked by a pair of prongs used to trap an enemy’s weapon. Though pointed, a sai is not usually used for stabbing. Instead, it is used primarily to bludgeon foes, punching with the hilt, or else to catch and disarm weapons between its tines. Sais are often wielded in pairs.");
  setweapon(WEAPON_TYPE_SIANGHAM, "siangham", 1, 6, 1, 2, WEAPON_FLAG_EXOTIC, 3,
            DAMAGE_TYPE_PIERCING, 1, 0, WEAPON_FAMILY_MONK, SIZE_SMALL, MATERIAL_STEEL,
            HANDLE_TYPE_HANDLE, HEAD_TYPE_POINT,
            "This weapon is a handheld shaft fitted with a pointed tip for stabbing foes. It resembles a (much sturdier) arrow with a grip designed for melee combat.");
  setweapon(WEAPON_TYPE_BASTARD_SWORD, "bastard sword", 1, 10, 1, 2, WEAPON_FLAG_EXOTIC,
            35, DAMAGE_TYPE_SLASHING, 6, 0, WEAPON_FAMILY_MEDIUM_BLADE, SIZE_MEDIUM, MATERIAL_STEEL,
            HANDLE_TYPE_HILT, HEAD_TYPE_BLADE,
            "A bastard sword is about 4 feet in length, making it too large to use in one hand without special training");
  setweapon(WEAPON_TYPE_KHOPESH, "khopesh", 1, 8, 2, 2, WEAPON_FLAG_EXOTIC,
            35, DAMAGE_TYPE_SLASHING, 6, 0, WEAPON_FAMILY_MEDIUM_BLADE, SIZE_MEDIUM, MATERIAL_STEEL,
            HANDLE_TYPE_HILT, HEAD_TYPE_BLADE,
            "This heavy blade has a convex curve near the end, making its overall shape similar to that of a battleaxe. A typical khopesh is 20 to 24 inches in length. Its curved shape allows the wielder to hook around defenses and trip foes. The elegant shape of a khopesh leads some artisans to cover them in ornate decorations.");
  setweapon(WEAPON_TYPE_DWARVEN_WAR_AXE, "dwarven war axe", 1, 10, 0, 3,
            WEAPON_FLAG_EXOTIC, 30, DAMAGE_TYPE_SLASHING, 8, 0, WEAPON_FAMILY_AXE, SIZE_MEDIUM,
            MATERIAL_STEEL, HANDLE_TYPE_HANDLE, HEAD_TYPE_BLADE,
            "A dwarven waraxe has a large, ornate head mounted to a thick handle, making it too large to use in one hand without special training.");
  setweapon(WEAPON_TYPE_WHIP, "whip", 1, 3, 0, 2, WEAPON_FLAG_EXOTIC | WEAPON_FLAG_REACH | WEAPON_FLAG_DISARM | WEAPON_FLAG_TRIP | WEAPON_FLAG_BALANCED, 1, DAMAGE_TYPE_SLASHING, 2, 0, WEAPON_FAMILY_WHIP,
            SIZE_MEDIUM, MATERIAL_LEATHER, HANDLE_TYPE_HANDLE, HEAD_TYPE_CORD,
            "An instrument consisting usually of a handle and lash forming a flexible rod that is used for whipping");
  setweapon(WEAPON_TYPE_SPIKED_CHAIN, "spiked chain", 2, 4, 0, 2, WEAPON_FLAG_EXOTIC | WEAPON_FLAG_REACH | WEAPON_FLAG_DISARM | WEAPON_FLAG_TRIP | WEAPON_FLAG_BALANCED, 25, DAMAGE_TYPE_PIERCING, 10, 0,
            WEAPON_FAMILY_WHIP, SIZE_LARGE, MATERIAL_STEEL, HANDLE_TYPE_GRIP, HEAD_TYPE_CHAIN,
            "A spiked chain is about 4 feet in length, covered in wicked barbs.");
  setweapon(WEAPON_TYPE_DOUBLE_AXE, "double-headed axe", 1, 8, 0, 3, WEAPON_FLAG_EXOTIC | WEAPON_FLAG_DOUBLE, 65, DAMAGE_TYPE_SLASHING, 15, 0, WEAPON_FAMILY_DOUBLE, SIZE_LARGE,
            MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_BLADE,
            "This polearm has a reinforced wooden haft with a double-bladed battleaxe head at each tip.");
  setweapon(WEAPON_TYPE_DIRE_FLAIL, "dire flail", 1, 8, 0, 2, WEAPON_FLAG_EXOTIC | WEAPON_FLAG_DOUBLE, 90, DAMAGE_TYPE_BLUDGEONING, 10, 0, WEAPON_FAMILY_DOUBLE, SIZE_LARGE,
            MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_HEAD,
            "A dire flail consists of two spheres of spiked iron dangling from chains at opposite ends of a long haft. This weapon excels at short but powerful strikes, and is typically swung in a constant churning motion. The wielder of a dire flail must have great strength, both to use the weapon effectively and to keep from tiring out.");
  setweapon(WEAPON_TYPE_HOOKED_HAMMER, "hooked hammer", 1, 6, 0, 4, WEAPON_FLAG_EXOTIC | WEAPON_FLAG_DOUBLE, 20, DAMAGE_TYPE_PIERCING | DAMAGE_TYPE_BLUDGEONING, 6, 0,
            WEAPON_FAMILY_DOUBLE, SIZE_LARGE, MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_HEAD,
            "A gnome hooked hammer is a double weapon-an ingenious tool with a hammer head at one end of its haft and a long, curved pick at the other.");
  /*	(weapon num, name, numDamDice, sizeDamDice, critRange, critMult, weapon flags, cost, damageType, weight, range, weaponFamily, Size, material, handle, head) */
  setweapon(WEAPON_TYPE_2_BLADED_SWORD, "two-bladed sword", 1, 8, 1, 2,
            WEAPON_FLAG_EXOTIC | WEAPON_FLAG_DOUBLE, 100, DAMAGE_TYPE_SLASHING, 10, 0,
            WEAPON_FAMILY_DOUBLE, SIZE_LARGE, MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_BLADE,
            "A two-bladed sword is a double weapon-twin blades extend from either side of a central, short haft, allowing the wielder to attack with graceful but deadly flourishes.");
  setweapon(WEAPON_TYPE_DWARVEN_URGOSH, "dwarven urgosh", 1, 7, 0, 3, WEAPON_FLAG_EXOTIC | WEAPON_FLAG_DOUBLE, 50, DAMAGE_TYPE_PIERCING | DAMAGE_TYPE_SLASHING, 12, 0,
            WEAPON_FAMILY_DOUBLE, SIZE_LARGE, MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_BLADE,
            "A dwarven urgrosh is a double weapon-an axe head and a spear point on opposite ends of a long haft.");
  setweapon(WEAPON_TYPE_HAND_CROSSBOW, "hand crossbow", 1, 4, 1, 2, WEAPON_FLAG_EXOTIC | WEAPON_FLAG_RANGED, 100, DAMAGE_TYPE_PIERCING, 2, 30, WEAPON_FAMILY_CROSSBOW, SIZE_SMALL,
            MATERIAL_WOOD, HANDLE_TYPE_HANDLE, HEAD_TYPE_BOW,
            "A hand crossbow is a miniature crossbow that can be wielded and reloaded with one hand.");
  setweapon(WEAPON_TYPE_HEAVY_REP_XBOW, "heavy repeating crossbow", 1, 10, 1, 2,
            WEAPON_FLAG_EXOTIC | WEAPON_FLAG_RANGED | WEAPON_FLAG_REPEATING, 400, DAMAGE_TYPE_PIERCING, 12, 120,
            WEAPON_FAMILY_CROSSBOW, SIZE_MEDIUM, MATERIAL_WOOD, HANDLE_TYPE_HANDLE, HEAD_TYPE_BOW,
            "The repeating heavy crossbow holds 5 crossbow bolts. As long as it holds bolts, you can reload it by pulling the reloading lever (a free action).");
  setweapon(WEAPON_TYPE_LIGHT_REP_XBOW, "light repeating crossbow", 1, 8, 1, 2,
            WEAPON_FLAG_EXOTIC | WEAPON_FLAG_RANGED, 250, DAMAGE_TYPE_PIERCING, 6, 80,
            WEAPON_FAMILY_CROSSBOW, SIZE_MEDIUM, MATERIAL_WOOD, HANDLE_TYPE_HANDLE, HEAD_TYPE_BOW,
            "The repeating heavy crossbow holds 5 crossbow bolts. As long as it holds bolts, you can reload it by pulling the reloading lever (a free action).");
  setweapon(WEAPON_TYPE_BOLA, "bola", 1, 4, 0, 2, WEAPON_FLAG_EXOTIC | WEAPON_FLAG_THROWN | WEAPON_FLAG_TRIP, 5, DAMAGE_TYPE_BLUDGEONING, 2, 10, WEAPON_FAMILY_THROWN, SIZE_MEDIUM,
            MATERIAL_LEATHER, HANDLE_TYPE_GRIP, HEAD_TYPE_CORD,
            "A bolas is a pair of wooden, stone, or metal weights connected by a thin rope or cord.");
  setweapon(WEAPON_TYPE_NET, "net", 1, 1, 0, 1, WEAPON_FLAG_EXOTIC | WEAPON_FLAG_THROWN | WEAPON_FLAG_ENTANGLE, 20, DAMAGE_TYPE_BLUDGEONING, 6, 10, WEAPON_FAMILY_THROWN, SIZE_LARGE,
            MATERIAL_LEATHER, HANDLE_TYPE_GRIP, HEAD_TYPE_MESH,
            "Nets can be fitted with weighted edges making them useful for entangling animals and humanoids the same size as the net or smaller.");
  setweapon(WEAPON_TYPE_SHURIKEN, "shuriken", 1, 2, 0, 2, WEAPON_FLAG_EXOTIC | WEAPON_FLAG_THROWN, 1, DAMAGE_TYPE_PIERCING, 1, 10, WEAPON_FAMILY_MONK, SIZE_SMALL,
            MATERIAL_STEEL, HANDLE_TYPE_GRIP, HEAD_TYPE_BLADE,
            "A shuriken is a small piece of metal with sharpened edges, designed for throwing.");
  setweapon(WEAPON_TYPE_WARMAUL, "war maul", 2, 6, 0, 3, WEAPON_FLAG_EXOTIC, 50,
            DAMAGE_TYPE_BLUDGEONING, 10, 0, WEAPON_FAMILY_HAMMER, SIZE_LARGE, MATERIAL_STEEL,
            HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD,
            "A warmaul has one damaging end with two heads; one axe and one hammer.");
  setweapon(WEAPON_TYPE_HOOPAK, "hoopak", 1, 6, 0, 2, WEAPON_FLAG_EXOTIC,
            1, DAMAGE_TYPE_PIERCING, 4, 0, WEAPON_FAMILY_RANGED, SIZE_MEDIUM,
            MATERIAL_WOOD, HANDLE_TYPE_SHAFT, HEAD_TYPE_HEAD,
            "A hoopak is a sturdy stick with a sling at one end and a pointed tip at the other.");
}

/************** ------- ARMOR ----------************************************/

/* some utility functions necessary for our piecemail armor system, everything
 * is up for changes since this is highly experimental system */

/* Armor types */ /*
#define ARMOR_TYPE_NONE     0
#define ARMOR_TYPE_LIGHT    1
#define ARMOR_TYPE_MEDIUM   2
#define ARMOR_TYPE_HEAVY    3
#define ARMOR_TYPE_SHIELD   4
#define ARMOR_TYPE_TOWER_SHIELD   5
#define NUM_ARMOR_TYPES     6 */

/* we have to be strict here, some classes such as monk require armor_type
   check, we are going to return the lowest armortype-value that the given
   ch is wearing */
int compute_gear_armor_type(struct char_data *ch)
{
  int armor_type = ARMOR_TYPE_NONE, armor_compare = ARMOR_TYPE_NONE, i;
  struct obj_data *obj = NULL;

  for (i = 0; i < NUM_WEARS; i++)
  {
    obj = GET_EQ(ch, i);
    if (obj && GET_OBJ_TYPE(obj) == ITEM_ARMOR)
    {
      /* ok we have an armor piece... */
      armor_compare = armor_list[GET_OBJ_VAL(obj, 1)].armorType;
      if (armor_compare == ARMOR_TYPE_HEAVY && HAS_FEAT(ch, FEAT_ARMORED_MOBILITY))
        armor_compare = ARMOR_TYPE_MEDIUM;
      if (armor_compare < ARMOR_TYPE_SHIELD && armor_compare > armor_type)
      {
        armor_type = armor_compare;
      }
    }
  }

  return armor_type;
}

int compute_gear_shield_type(struct char_data *ch)
{
  int shield_type = ARMOR_TYPE_NONE;
  struct obj_data *obj = GET_EQ(ch, WEAR_SHIELD);

  if (obj)
  {
    shield_type = armor_list[GET_OBJ_VAL(obj, 1)].armorType;
    if (shield_type != ARMOR_TYPE_SHIELD && shield_type != ARMOR_TYPE_TOWER_SHIELD)
    {
      shield_type = ARMOR_TYPE_NONE;
    }
  }

  return shield_type;
}

/* enhancement bonus + material bonus */
int compute_gear_enhancement_bonus(struct char_data *ch)
{
  struct obj_data *obj = NULL;
  int enhancement_bonus = 0;
  float counter = 0.0;
  float num_pieces = 0.0;

  /* we're going to check slot-by-slot */

  /* SPECIAL HANDLING FOR SHIELD */
  /* shield - gets full enchantment bonus, so _do not_ increment num_pieces */
  obj = GET_EQ(ch, WEAR_SHIELD);
  if (obj && GET_OBJ_TYPE(obj) == ITEM_ARMOR)
  {
    switch (GET_OBJ_MATERIAL(obj))
    {
    case MATERIAL_ADAMANTINE:
    case MATERIAL_MITHRIL:
    case MATERIAL_DRAGONHIDE:
    case MATERIAL_DRAGONSCALE:
    case MATERIAL_DRAGONBONE:
    case MATERIAL_DIAMOND:
    case MATERIAL_DARKWOOD:
      counter += 1.1;
      break;
    }
    counter += (float)GET_OBJ_VAL(obj, 4) * 1.01;
    /* DON'T increment num_pieces, should get full bang for buck on shields */
  }

  // this spell doubles armor enhancement bonus
  if (affected_by_spell(ch, SPELL_LITANY_OF_DEFENSE))
    counter *= 2;

  enhancement_bonus += counter;
  counter = 0.1; /* reset the counter for all other slots */
  /* end SPECIAL HANDLING FOR SHIELD */

  /* body */
  obj = GET_EQ(ch, WEAR_BODY);
  num_pieces += 0.99;
  if (obj && GET_OBJ_TYPE(obj) == ITEM_ARMOR)
  {
    switch (GET_OBJ_MATERIAL(obj))
    {
    case MATERIAL_ADAMANTINE:
    case MATERIAL_MITHRIL:
    case MATERIAL_DRAGONHIDE:
    case MATERIAL_DRAGONSCALE:
    case MATERIAL_DRAGONBONE:
    case MATERIAL_DIAMOND:
    case MATERIAL_DARKWOOD:
      counter += 1.1;
      break;
    }
    counter += (float)GET_OBJ_VAL(obj, 4) * 1.01;
  }

  /* head */
  obj = GET_EQ(ch, WEAR_HEAD);
  num_pieces += 0.99;
  if (obj && GET_OBJ_TYPE(obj) == ITEM_ARMOR)
  {
    switch (GET_OBJ_MATERIAL(obj))
    {
    case MATERIAL_ADAMANTINE:
    case MATERIAL_MITHRIL:
    case MATERIAL_DRAGONHIDE:
    case MATERIAL_DRAGONSCALE:
    case MATERIAL_DRAGONBONE:
    case MATERIAL_DIAMOND:
    case MATERIAL_DARKWOOD:
      counter += 1.1;
      break;
    }
    counter += (float)GET_OBJ_VAL(obj, 4) * 1.01;
  }

  /* legs */
  obj = GET_EQ(ch, WEAR_LEGS);
  num_pieces += 0.99;
  if (obj && GET_OBJ_TYPE(obj) == ITEM_ARMOR)
  {
    switch (GET_OBJ_MATERIAL(obj))
    {
    case MATERIAL_ADAMANTINE:
    case MATERIAL_MITHRIL:
    case MATERIAL_DRAGONHIDE:
    case MATERIAL_DRAGONSCALE:
    case MATERIAL_DRAGONBONE:
    case MATERIAL_DIAMOND:
    case MATERIAL_DARKWOOD:
      counter += 1.1;
      break;
    }
    counter += (float)GET_OBJ_VAL(obj, 4) * 1.01;
  }

  /* arms */
  obj = GET_EQ(ch, WEAR_ARMS);
  num_pieces += 0.99;
  if (obj && GET_OBJ_TYPE(obj) == ITEM_ARMOR)
  {
    switch (GET_OBJ_MATERIAL(obj))
    {
    case MATERIAL_ADAMANTINE:
    case MATERIAL_MITHRIL:
    case MATERIAL_DRAGONHIDE:
    case MATERIAL_DRAGONSCALE:
    case MATERIAL_DRAGONBONE:
    case MATERIAL_DIAMOND:
    case MATERIAL_DARKWOOD:
      counter += 1.1;
      break;
    }
    counter += (float)GET_OBJ_VAL(obj, 4) * 1.01;
  }

  enhancement_bonus += MAX(0, (int)(counter / num_pieces));

  return enhancement_bonus;
}

/* should return a percentage */
int compute_gear_spell_failure(struct char_data *ch)
{
  int spell_failure = 0, i, count = 0;
  struct obj_data *obj = NULL;
  bool bonearmor = false;

  for (i = 0; i < NUM_WEARS; i++)
  {
    obj = GET_EQ(ch, i);
    if (obj && GET_OBJ_TYPE(obj) == ITEM_ARMOR &&
        (i == WEAR_BODY || i == WEAR_HEAD || i == WEAR_LEGS || i == WEAR_ARMS ||
         i == WEAR_SHIELD))
    {
      // all armor pieces must be bone to benefit from bone armor necromancer ability
      bonearmor = false;
      if (GET_OBJ_MATERIAL(obj) == MATERIAL_BONE)
        bonearmor = true;
      
      if (i != WEAR_SHIELD) /* shield and armor combined increase spell failure chance */
        count++;
      /* ok we have an armor piece... */
      spell_failure += armor_list[GET_OBJ_VAL(obj, 1)].spellFail;
    }
  }

  if (count)
  {
    spell_failure = spell_failure / count;
  }

  if (HAS_FEAT(ch, FEAT_ARCANE_ARMOR_MASTERY))
    spell_failure -= 20;
  else if (HAS_FEAT(ch, FEAT_ARCANE_ARMOR_TRAINING))
    spell_failure -= 10;
  if (bonearmor && HAS_REAL_FEAT(ch, FEAT_BONE_ARMOR))
    spell_failure -= (10 * HAS_REAL_FEAT(ch, FEAT_BONE_ARMOR));

  if (affected_by_spell(ch, PSIONIC_OAK_BODY))
    spell_failure += 25;
  if (affected_by_spell(ch, PSIONIC_BODY_OF_IRON))
    spell_failure += 35;

  /* 5% improvement in spell success with this feat */
  spell_failure -= HAS_FEAT(ch, FEAT_ARMORED_SPELLCASTING) * 5;
  // Spellsword ignore spell fail
  if (HAS_FEAT(ch, FEAT_IGNORE_SPELL_FAILURE))
    spell_failure -= (HAS_FEAT(ch, FEAT_IGNORE_SPELL_FAILURE) * 10);

  if (spell_failure < 0)
    spell_failure = 0;
  if (spell_failure > 100)
    spell_failure = 100;

  return spell_failure;
}

/* for doing (usually) dexterity based tasks */
int compute_gear_armor_penalty(struct char_data *ch)
{
  int armor_penalty = 0, i, count = 0;
  int masterwork_bonus = 0;

  struct obj_data *obj = NULL;

  for (i = 0; i < NUM_WEARS; i++)
  {
    obj = GET_EQ(ch, i);
    if (obj && GET_OBJ_TYPE(obj) == ITEM_ARMOR &&
        (i == WEAR_BODY || i == WEAR_HEAD || i == WEAR_LEGS || i == WEAR_ARMS ||
         i == WEAR_SHIELD))
    {
      count++;
      /* ok we have an armor piece... */
      armor_penalty += armor_list[GET_OBJ_VAL(obj, 1)].armorCheck;
      // masterwork armor reduces penalty, but all pieces need to be masterwork, except shields
      if (OBJ_FLAGGED(obj, ITEM_MASTERWORK))
      {
        if (i == WEAR_SHIELD)
        {
          armor_penalty++;
        }
        else
        {
          masterwork_bonus++;
        }
      }
    }
  }

  if (affected_by_spell(ch, PSIONIC_OAK_BODY))
    armor_penalty -= 4;
  if (affected_by_spell(ch, PSIONIC_BODY_OF_IRON))
    armor_penalty -= 6;

  if (HAS_REAL_FEAT(ch, FEAT_SHIELD_DWARF_ARMOR_TRAINING))
    armor_penalty++;

  // for masterwork armor, all 4 pieces need to be masterwork to get the benefit
  if ((masterwork_bonus / 4) >= 1)
    armor_penalty++;

  if (HAS_FEAT(ch, FEAT_ARMORED_MOBILITY))
    armor_penalty += 2;

  if (count)
  {
    armor_penalty = armor_penalty / count;
    armor_penalty += HAS_FEAT(ch, FEAT_ARMOR_TRAINING);
  }

  if (affected_by_spell(ch, SPELL_EFFORTLESS_ARMOR))
    armor_penalty += get_char_affect_modifier(ch, SPELL_EFFORTLESS_ARMOR, APPLY_SPECIAL);

  if (armor_penalty > 0)
    armor_penalty = 0;
  if (armor_penalty < -10)
    armor_penalty = -10;

  return armor_penalty;
}

/* maximum dexterity bonus, returns 99 for no limit */
int compute_gear_max_dex(struct char_data *ch)
{
  int dexterity_cap = 0;
  int armor_max_dexterity = 0, i, count = 0;

  if (IS_WILDSHAPED(ch) || IS_MORPHED(ch)) /* not wearing armor, no limit to dexterity */
    return 99;

  struct obj_data *obj = NULL;

  for (i = 0; i < NUM_WEARS; i++)
  {
    obj = GET_EQ(ch, i);
    if (obj && GET_OBJ_TYPE(obj) == ITEM_ARMOR &&
        (i == WEAR_BODY || i == WEAR_HEAD || i == WEAR_LEGS || i == WEAR_ARMS ||
         i == WEAR_SHIELD))
    {
      /* ok we have an armor piece... */
      armor_max_dexterity = armor_list[GET_OBJ_VAL(obj, 1)].dexBonus;
      if (armor_max_dexterity == 99) /* no limit to dexterity */
        count--;
      else
        dexterity_cap += armor_max_dexterity;
      count++;
    }
  }

  if (count > 0)
  {
    dexterity_cap = dexterity_cap / count;
    
    dexterity_cap += HAS_FEAT(ch, FEAT_ARMOR_TRAINING);
    
    if (HAS_FEAT(ch, FEAT_ARMORED_MOBILITY))
      dexterity_cap += 2;
  }
  else /* not wearing armor */
    dexterity_cap = 99;

  if (dexterity_cap < 0)
    dexterity_cap = 0;

  return dexterity_cap;
}

int is_proficient_with_shield(struct char_data *ch)
{
  struct obj_data *shield = GET_EQ(ch, WEAR_SHIELD);

  if (IS_NPC(ch))
    return FALSE;

  if (!shield)
    return TRUE;

  switch (GET_ARMOR_TYPE_PROF(shield))
  {
  case ARMOR_TYPE_SHIELD:
    if (HAS_FEAT(ch, FEAT_ARMOR_PROFICIENCY_SHIELD))
      return TRUE;
    break;
  case ARMOR_TYPE_TOWER_SHIELD:
    if (HAS_FEAT(ch, FEAT_ARMOR_PROFICIENCY_TOWER_SHIELD))
      return TRUE;
    break;
  }

  return FALSE;
}

int is_proficient_with_body_armor(struct char_data *ch)
{
  struct obj_data *body_armor = GET_EQ(ch, WEAR_BODY);

  if (IS_NPC(ch))
    return FALSE;

  if (!body_armor)
    return TRUE;

  switch (GET_ARMOR_TYPE_PROF(body_armor))
  {
  case ARMOR_TYPE_NONE:
    return TRUE;
    break;
  case ARMOR_TYPE_LIGHT:
    if (HAS_FEAT(ch, FEAT_ARMOR_PROFICIENCY_LIGHT))
      return TRUE;
    break;
  case ARMOR_TYPE_MEDIUM:
    if (HAS_FEAT(ch, FEAT_ARMOR_PROFICIENCY_MEDIUM))
      return TRUE;
    break;
  case ARMOR_TYPE_HEAVY:
    if (HAS_FEAT(ch, FEAT_ARMOR_PROFICIENCY_HEAVY))
      return TRUE;
    break;
  default:
    break;
  }

  return FALSE;
}

int is_proficient_with_helm(struct char_data *ch)
{
  struct obj_data *head_armor = GET_EQ(ch, WEAR_HEAD);

  if (IS_NPC(ch))
    return FALSE;

  if (!head_armor)
    return TRUE;

  switch (GET_ARMOR_TYPE_PROF(head_armor))
  {
  case ARMOR_TYPE_NONE:
    return TRUE;
    break;
  case ARMOR_TYPE_LIGHT:
    if (HAS_FEAT(ch, FEAT_ARMOR_PROFICIENCY_LIGHT))
      return TRUE;
    break;
  case ARMOR_TYPE_MEDIUM:
    if (HAS_FEAT(ch, FEAT_ARMOR_PROFICIENCY_MEDIUM))
      return TRUE;
    break;
  case ARMOR_TYPE_HEAVY:
    if (HAS_FEAT(ch, FEAT_ARMOR_PROFICIENCY_HEAVY))
      return TRUE;
    break;
  default:
    break;
  }

  return FALSE;
}

int is_proficient_with_sleeves(struct char_data *ch)
{
  struct obj_data *arm_armor = GET_EQ(ch, WEAR_ARMS);

  if (IS_NPC(ch))
    return FALSE;

  if (!arm_armor)
    return TRUE;

  switch (GET_ARMOR_TYPE_PROF(arm_armor))
  {
  case ARMOR_TYPE_NONE:
    return TRUE;
    break;
  case ARMOR_TYPE_LIGHT:
    if (HAS_FEAT(ch, FEAT_ARMOR_PROFICIENCY_LIGHT))
      return TRUE;
    break;
  case ARMOR_TYPE_MEDIUM:
    if (HAS_FEAT(ch, FEAT_ARMOR_PROFICIENCY_MEDIUM))
      return TRUE;
    break;
  case ARMOR_TYPE_HEAVY:
    if (HAS_FEAT(ch, FEAT_ARMOR_PROFICIENCY_HEAVY))
      return TRUE;
    break;
  default:
    break;
  }

  return FALSE;
}

int is_proficient_with_leggings(struct char_data *ch)
{
  struct obj_data *leg_armor = GET_EQ(ch, WEAR_LEGS);

  if (IS_NPC(ch))
    return FALSE;

  if (!leg_armor)
    return TRUE;

  switch (GET_ARMOR_TYPE_PROF(leg_armor))
  {
  case ARMOR_TYPE_NONE:
    return TRUE;
    break;
  case ARMOR_TYPE_LIGHT:
    if (HAS_FEAT(ch, FEAT_ARMOR_PROFICIENCY_LIGHT))
      return TRUE;
    break;
  case ARMOR_TYPE_MEDIUM:
    if (HAS_FEAT(ch, FEAT_ARMOR_PROFICIENCY_MEDIUM))
      return TRUE;
    break;
  case ARMOR_TYPE_HEAVY:
    if (HAS_FEAT(ch, FEAT_ARMOR_PROFICIENCY_HEAVY))
      return TRUE;
    break;
  default:
    break;
  }

  return FALSE;
}

/* simply checks if ch has proficiency with given armor_type */
int is_proficient_with_armor(struct char_data *ch)
{
  if (
      is_proficient_with_leggings(ch) &&
      is_proficient_with_sleeves(ch) &&
      is_proficient_with_helm(ch) &&
      is_proficient_with_body_armor(ch) &&
      is_proficient_with_shield(ch))
    return TRUE;

  return FALSE;
}

static void setarmor(int type, const char *name, int armorType, int cost, int armorBonus,
                     int dexBonus, int armorCheck, int spellFail, int thirtyFoot,
                     int twentyFoot, int weight, int material, int wear, const char *description)
{
  armor_list[type].name = name;
  armor_list[type].armorType = armorType;
  armor_list[type].cost = cost;
  armor_list[type].armorBonus = armorBonus;
  armor_list[type].dexBonus = dexBonus;
  armor_list[type].armorCheck = armorCheck;
  armor_list[type].spellFail = spellFail;
  armor_list[type].thirtyFoot = thirtyFoot;
  armor_list[type].twentyFoot = twentyFoot;
  armor_list[type].weight = weight;
  armor_list[type].material = material;
  armor_list[type].wear = wear;
  armor_list[type].description = description;
}

void initialize_armor(int type)
{
  armor_list[type].name = "unused armor";
  armor_list[type].description = "unused armor";
  armor_list[type].armorType = 0;
  armor_list[type].cost = 0;
  armor_list[type].armorBonus = 0;
  armor_list[type].dexBonus = 0;
  armor_list[type].armorCheck = 0;
  armor_list[type].spellFail = 0;
  armor_list[type].thirtyFoot = 0;
  armor_list[type].twentyFoot = 0;
  armor_list[type].weight = 0;
  armor_list[type].material = 0;
  armor_list[type].wear = ITEM_WEAR_TAKE;
}

void load_armor(void)
{
  int i = 0;

  for (i = 0; i < NUM_SPEC_ARMOR_TYPES; i++)
    initialize_armor(i);

  /* (armor, name, type,
   *    cost, AC, dexBonusCap, armorCheckPenalty, spellFailChance, (move)30ft, (move)20ft,
   *    weight, material, wear) */
  /* UNARMORED */
  setarmor(SPEC_ARMOR_TYPE_CLOTHING, "robe", ARMOR_TYPE_NONE,
           10, 0, 99, 0, 0, 30, 20,
           1, MATERIAL_COTTON, ITEM_WEAR_BODY,
           "This can pertain to any type of unarmored clothing.");
  setarmor(SPEC_ARMOR_TYPE_CLOTHING_HEAD, "hood", ARMOR_TYPE_NONE,
           10, 0, 99, 0, 0, 30, 20,
           1, MATERIAL_COTTON, ITEM_WEAR_HEAD, "<ask staff to fill out>");
  setarmor(SPEC_ARMOR_TYPE_CLOTHING_ARMS, "sleeves", ARMOR_TYPE_NONE,
           10, 0, 99, 0, 0, 30, 20,
           1, MATERIAL_COTTON, ITEM_WEAR_ARMS, "<ask staff to fill out>");
  setarmor(SPEC_ARMOR_TYPE_CLOTHING_LEGS, "pants", ARMOR_TYPE_NONE,
           10, 0, 99, 0, 0, 30, 20,
           1, MATERIAL_COTTON, ITEM_WEAR_LEGS, "<ask staff to fill out>");

  /* LIGHT ARMOR ********************/
  setarmor(SPEC_ARMOR_TYPE_PADDED, "padded body armor", ARMOR_TYPE_LIGHT,
           50, 9, 14, 0, 5, 30, 20,
           7, MATERIAL_COTTON, ITEM_WEAR_BODY, "More than simple clothing, padded armor combines heavy, quilted cloth and layers of densely packed stuffing to create a cheap and basic protection. It is typically worn by those not intending to face lethal combat or those who wish their maneuverability to be impacted as little as possible.");
  setarmor(SPEC_ARMOR_TYPE_PADDED_HEAD, "padded armor helm", ARMOR_TYPE_LIGHT,
           50, 1, 14, 0, 5, 30, 20,
           1, MATERIAL_COTTON, ITEM_WEAR_HEAD, "<ask staff to fill out>");
  setarmor(SPEC_ARMOR_TYPE_PADDED_ARMS, "padded armor sleeves", ARMOR_TYPE_LIGHT,
           50, 1, 14, 0, 5, 30, 20,
           1, MATERIAL_COTTON, ITEM_WEAR_ARMS, "<ask staff to fill out>");
  setarmor(SPEC_ARMOR_TYPE_PADDED_LEGS, "padded armor leggings", ARMOR_TYPE_LIGHT,
           50, 1, 14, 0, 5, 30, 20,
           1, MATERIAL_COTTON, ITEM_WEAR_LEGS, "<ask staff to fill out>");

  setarmor(SPEC_ARMOR_TYPE_LEATHER, "leather armor", ARMOR_TYPE_LIGHT,
           100, 14, 13, 0, 10, 30, 20,
           9, MATERIAL_LEATHER, ITEM_WEAR_BODY, "Leather armor is made up of multiple overlapping pieces of leather, boiled to increase their natural toughness and then deliberately stitched together. Although not as sturdy as metal armor, the flexibility it allows wearers makes it among the most widely used types of armor.");
  setarmor(SPEC_ARMOR_TYPE_LEATHER_HEAD, "leather helm", ARMOR_TYPE_LIGHT,
           100, 4, 13, 0, 10, 30, 20,
           2, MATERIAL_LEATHER, ITEM_WEAR_HEAD, "<ask staff to fill out>");
  setarmor(SPEC_ARMOR_TYPE_LEATHER_ARMS, "leather sleeves", ARMOR_TYPE_LIGHT,
           100, 4, 13, 0, 10, 30, 20,
           2, MATERIAL_LEATHER, ITEM_WEAR_ARMS, "<ask staff to fill out>");
  setarmor(SPEC_ARMOR_TYPE_LEATHER_LEGS, "leather leggings", ARMOR_TYPE_LIGHT,
           100, 4, 13, 0, 10, 30, 20,
           2, MATERIAL_LEATHER, ITEM_WEAR_LEGS, "<ask staff to fill out>");

  /* (armor, name, type,
   *    cost, AC, dexBonusCap, armorCheckPenalty, spellFailChance, (move)30ft, (move)20ft,
   *    weight, material, wear) */
  setarmor(SPEC_ARMOR_TYPE_STUDDED_LEATHER, "studded leather armor", ARMOR_TYPE_LIGHT,
           250, 20, 12, -1, 15, 30, 20,
           11, MATERIAL_LEATHER, ITEM_WEAR_BODY, "An improved form of leather armor, studded leather armor is covered with dozens of metal protuberances. While these rounded studs offer little defense individually, in the numbers they are arrayed in upon such armor, they help catch lethal edges and channel them away from vital spots. The rigidity caused by the additional metal does, however, result in less mobility than is afforded by a suit of normal leather armor.");
  setarmor(SPEC_ARMOR_TYPE_STUDDED_LEATHER_HEAD, "studded leather helm", ARMOR_TYPE_LIGHT,
           250, 6, 12, -1, 15, 30, 20,
           3, MATERIAL_LEATHER, ITEM_WEAR_HEAD, "<ask staff to fill out>");
  setarmor(SPEC_ARMOR_TYPE_STUDDED_LEATHER_ARMS, "studded leather sleeves", ARMOR_TYPE_LIGHT,
           250, 6, 12, -1, 15, 30, 20,
           3, MATERIAL_LEATHER, ITEM_WEAR_ARMS, "<ask staff to fill out>");
  setarmor(SPEC_ARMOR_TYPE_STUDDED_LEATHER_LEGS, "studded leather leggings", ARMOR_TYPE_LIGHT,
           250, 6, 12, -1, 15, 30, 20,
           3, MATERIAL_LEATHER, ITEM_WEAR_LEGS, "<ask staff to fill out>");

  setarmor(SPEC_ARMOR_TYPE_LIGHT_CHAIN, "light chainmail armor", ARMOR_TYPE_LIGHT,
           1000, 24, 11, -2, 20, 30, 20,
           13, MATERIAL_STEEL, ITEM_WEAR_BODY, "This armor is made up of thousands of interlocking metal rings, laid on top of leather armor.");
  setarmor(SPEC_ARMOR_TYPE_LIGHT_CHAIN_HEAD, "light chainmail helm", ARMOR_TYPE_LIGHT,
           1000, 9, 11, -2, 20, 30, 20,
           4, MATERIAL_STEEL, ITEM_WEAR_HEAD, "<ask staff to fill out>");
  setarmor(SPEC_ARMOR_TYPE_LIGHT_CHAIN_ARMS, "light chainmail sleeves", ARMOR_TYPE_LIGHT,
           1000, 9, 11, -2, 20, 30, 20,
           4, MATERIAL_STEEL, ITEM_WEAR_ARMS, "<ask staff to fill out>");
  setarmor(SPEC_ARMOR_TYPE_LIGHT_CHAIN_LEGS, "light chainmail leggings", ARMOR_TYPE_LIGHT,
           1000, 9, 11, -2, 20, 30, 20,
           4, MATERIAL_STEEL, ITEM_WEAR_LEGS, "<ask staff to fill out>");

  /******************* MEDIUM ARMOR *******************************************/

  setarmor(SPEC_ARMOR_TYPE_HIDE, "hide armor", ARMOR_TYPE_MEDIUM,
           150, 26, 10, -3, 20, 20, 15,
           13, MATERIAL_LEATHER, ITEM_WEAR_BODY, "Hide armor is made from the tanned skin of particularly thick-hided beasts, stitched with either multiple overlapping layers of crude leather or exterior pieces of leather stuffed with padding or fur. Damage to the armor is typically repaired by restitching gashes or adding new pieces of hide, giving the most heavily used suits a distinctively patchwork quality.");
  setarmor(SPEC_ARMOR_TYPE_HIDE_HEAD, "hide helm", ARMOR_TYPE_MEDIUM,
           150, 10, 10, -3, 20, 20, 15,
           4, MATERIAL_LEATHER, ITEM_WEAR_HEAD, "<ask staff to fill out>");
  setarmor(SPEC_ARMOR_TYPE_HIDE_ARMS, "hide sleeves", ARMOR_TYPE_MEDIUM,
           150, 10, 10, -3, 20, 20, 15,
           4, MATERIAL_LEATHER, ITEM_WEAR_ARMS, "<ask staff to fill out>");
  setarmor(SPEC_ARMOR_TYPE_HIDE_LEGS, "hide leggings", ARMOR_TYPE_MEDIUM,
           150, 10, 10, -3, 20, 20, 15,
           4, MATERIAL_LEATHER, ITEM_WEAR_LEGS, "<ask staff to fill out>");

  /* (armor, name, type,
   *    cost, AC, dexBonusCap, armorCheckPenalty, spellFailChance, (move)30ft, (move)20ft,
   *    weight, material, wear) */
  setarmor(SPEC_ARMOR_TYPE_SCALE, "scale armor", ARMOR_TYPE_MEDIUM,
           500, 32, 9, -4, 25, 20, 15,
           15, MATERIAL_STEEL, ITEM_WEAR_BODY, "Scale mail is made up of dozens of small, overlapping metal plates. Similar to both splint mail and banded mail, scalemail has a flexible arrangement of scales in an attempt to avoid hindering the wearer’s mobility, but at the expense of omitting additional protective layers of armor. A suit of scale mail includes gauntlets.");
  setarmor(SPEC_ARMOR_TYPE_SCALE_HEAD, "scale helm", ARMOR_TYPE_MEDIUM,
           500, 12, 9, -4, 25, 20, 15,
           5, MATERIAL_STEEL, ITEM_WEAR_HEAD, "<ask staff to fill this out>");
  setarmor(SPEC_ARMOR_TYPE_SCALE_ARMS, "scale sleeves", ARMOR_TYPE_MEDIUM,
           500, 12, 9, -4, 25, 20, 15,
           5, MATERIAL_STEEL, ITEM_WEAR_ARMS, "<ask staff to fill this out>");
  setarmor(SPEC_ARMOR_TYPE_SCALE_LEGS, "scale leggings", ARMOR_TYPE_MEDIUM,
           500, 12, 9, -4, 25, 20, 15,
           5, MATERIAL_STEEL, ITEM_WEAR_LEGS, "<ask staff to fill this out>");

  setarmor(SPEC_ARMOR_TYPE_CHAINMAIL, "chainmail armor", ARMOR_TYPE_MEDIUM,
           1500, 37, 8, -5, 30, 20, 15,
           27, MATERIAL_STEEL, ITEM_WEAR_BODY, "Chainmail protects the wearer with a complete mesh of chain links that cover the torso and arms, and extends below the waist. Multiple interconnected pieces offer additional protection over vital areas. The suit includes gauntlets. It uses thicker rings and heavier leather than light chainmail.");
  setarmor(SPEC_ARMOR_TYPE_CHAINMAIL_HEAD, "chainmail helm", ARMOR_TYPE_MEDIUM,
           1500, 15, 8, -5, 30, 20, 15,
           11, MATERIAL_STEEL, ITEM_WEAR_HEAD, "<ask staff to fill this out>");
  /* duplicate item */
  setarmor(SPEC_ARMOR_TYPE_CHAIN_HEAD, "chainmail helm", ARMOR_TYPE_MEDIUM,
           1500, 15, 8, -5, 30, 20, 15,
           11, MATERIAL_STEEL, ITEM_WEAR_HEAD, "<ask staff to fill this out>");
  setarmor(SPEC_ARMOR_TYPE_CHAINMAIL_ARMS, "chainmail sleeves", ARMOR_TYPE_MEDIUM,
           1500, 15, 8, -5, 30, 20, 15,
           11, MATERIAL_STEEL, ITEM_WEAR_ARMS, "<ask staff to fill this out>");
  setarmor(SPEC_ARMOR_TYPE_CHAINMAIL_LEGS, "chainmail leggings", ARMOR_TYPE_MEDIUM,
           1500, 15, 8, -5, 30, 20, 15,
           11, MATERIAL_STEEL, ITEM_WEAR_LEGS, "<ask staff to fill this out>");

  setarmor(SPEC_ARMOR_TYPE_PIECEMEAL, "piecemeal armor", ARMOR_TYPE_MEDIUM,
           2000, 35, 9, -4, 25, 20, 15,
           19, MATERIAL_STEEL, ITEM_WEAR_BODY, "This type of armor typically consists of a metal breastplate and mismatched pieces of other different armor types patched together. Though certainly not fashionable, it is nonetheless fairly functional.");
  setarmor(SPEC_ARMOR_TYPE_PIECEMEAL_HEAD, "piecemeal helm", ARMOR_TYPE_MEDIUM,
           2000, 14, 9, -4, 25, 20, 15,
           7, MATERIAL_STEEL, ITEM_WEAR_HEAD, "<ask staff to fill this out>");
  setarmor(SPEC_ARMOR_TYPE_PIECEMEAL_ARMS, "piecemeal sleeves", ARMOR_TYPE_MEDIUM,
           2000, 14, 9, -4, 25, 20, 15,
           7, MATERIAL_STEEL, ITEM_WEAR_ARMS, "<ask staff to fill this out>");
  setarmor(SPEC_ARMOR_TYPE_PIECEMEAL_LEGS, "piecemeal leggings", ARMOR_TYPE_MEDIUM,
           2000, 14, 9, -4, 25, 20, 15,
           7, MATERIAL_STEEL, ITEM_WEAR_LEGS, "<ask staff to fill this out>");

  /******************* HEAVY ARMOR *******************************************/

  /* (armor, name, type,
   *    cost, AC, dexBonusCap, armorCheckPenalty, spellFailChance, (move)30ft, (move)20ft,
   *    weight, material, wear) */
  setarmor(SPEC_ARMOR_TYPE_SPLINT, "splint mail armor", ARMOR_TYPE_HEAVY,
           2000, 46, 5, -7, 40, 20, 15,
           21, MATERIAL_STEEL, ITEM_WEAR_BODY, "Splint mail is made up of overlapping layers of metal strips attached to a backing of leather or sturdy fabric. These splints are of greater size and durability than those that compose a suit of scale mail, improving the protection they afford the wearer, but at the cost of flexibility. A suit of splint mail includes gauntlets.");
  setarmor(SPEC_ARMOR_TYPE_SPLINT_HEAD, "splint mail helm", ARMOR_TYPE_HEAVY,
           2000, 19, 5, -7, 40, 20, 15,
           8, MATERIAL_STEEL, ITEM_WEAR_HEAD, "<ask staff to fill this out>");
  setarmor(SPEC_ARMOR_TYPE_SPLINT_ARMS, "splint mail sleeves", ARMOR_TYPE_HEAVY,
           2000, 19, 5, -7, 40, 20, 15,
           8, MATERIAL_STEEL, ITEM_WEAR_ARMS, "<ask staff to fill this out>");
  setarmor(SPEC_ARMOR_TYPE_SPLINT_LEGS, "splint mail leggings", ARMOR_TYPE_HEAVY,
           2000, 19, 5, -7, 40, 20, 15,
           8, MATERIAL_STEEL, ITEM_WEAR_LEGS, "<ask staff to fill this out>");

  setarmor(SPEC_ARMOR_TYPE_BANDED, "banded mail armor", ARMOR_TYPE_HEAVY,
           2500, 47, 7, -6, 35, 20, 15,
           17, MATERIAL_STEEL, ITEM_WEAR_BODY, "Banded mail is made up of overlapping strips of metal, fastened to a sturdy backing of leather and chain. The size of the metal plates, interconnected metal bands, and layers of underlying armor make it a more significant defense than similar armors, like scale mail or splint mail.");
  setarmor(SPEC_ARMOR_TYPE_BANDED_HEAD, "banded mail helm", ARMOR_TYPE_HEAVY,
           2500, 20, 7, -6, 35, 20, 15,
           6, MATERIAL_STEEL, ITEM_WEAR_HEAD, "<ask staff to fill this out>");
  setarmor(SPEC_ARMOR_TYPE_BANDED_ARMS, "banded mail sleeves", ARMOR_TYPE_HEAVY,
           2500, 20, 7, -6, 35, 20, 15,
           6, MATERIAL_STEEL, ITEM_WEAR_ARMS, "<ask staff to fill this out>");
  setarmor(SPEC_ARMOR_TYPE_BANDED_LEGS, "banded mail leggings", ARMOR_TYPE_HEAVY,
           2500, 20, 7, -6, 35, 20, 15,
           6, MATERIAL_STEEL, ITEM_WEAR_LEGS, "<ask staff to fill this out>");

  setarmor(SPEC_ARMOR_TYPE_HALF_PLATE, "half plate armor", ARMOR_TYPE_HEAVY,
           6000, 52, 7, -6, 40, 20, 15,
           23, MATERIAL_STEEL, ITEM_WEAR_BODY, "Half-plate armor combines elements of full plate and chainmail, incorporating several sizable plates of sculpted metal with an underlying mesh of chain links. While this suit protects vital areas with several layers of armor, it is not sculpted to a single individual’s frame, reducing its wearer’s mobility even more than a suit of full plate. Half-plate armor includes gauntlets and a helm.");
  setarmor(SPEC_ARMOR_TYPE_HALF_PLATE_HEAD, "half plate helm", ARMOR_TYPE_HEAVY,
           6000, 22, 7, -6, 40, 20, 15,
           9, MATERIAL_STEEL, ITEM_WEAR_HEAD, "<ask staff to fill this out>");
  setarmor(SPEC_ARMOR_TYPE_HALF_PLATE_ARMS, "half plate sleeves", ARMOR_TYPE_HEAVY,
           6000, 22, 7, -6, 40, 20, 15,
           9, MATERIAL_STEEL, ITEM_WEAR_ARMS, "<ask staff to fill this out>");
  setarmor(SPEC_ARMOR_TYPE_HALF_PLATE_LEGS, "half plate leggings", ARMOR_TYPE_HEAVY,
           6000, 22, 7, -6, 40, 20, 15,
           9, MATERIAL_STEEL, ITEM_WEAR_LEGS, "<ask staff to fill this out>");

  /* (armor, name, type,
   *    cost, AC, dexBonusCap, armorCheckPenalty, spellFailChance, (move)30ft, (move)20ft,
   *    weight, material, wear) */
  setarmor(SPEC_ARMOR_TYPE_FULL_PLATE, "full plate armor", ARMOR_TYPE_HEAVY,
           15000, 60, 7, -6, 35, 20, 15,
           23, MATERIAL_STEEL, ITEM_WEAR_BODY, "This metal suit comprises multiple pieces of interconnected and overlaying metal plates, incorporating the benefits of numerous types of lesser armor. A complete suit of full plate (or platemail, as it is often called) includes gauntlets, heavy leather boots, a visored helmet, and a thick layer of padding that is worn underneath the armor.");
  setarmor(SPEC_ARMOR_TYPE_FULL_PLATE_HEAD, "full plate helm", ARMOR_TYPE_HEAVY,
           15000, 25, 7, -6, 35, 20, 15,
           9, MATERIAL_STEEL, ITEM_WEAR_HEAD, "<ask staff to fill this out>");
  setarmor(SPEC_ARMOR_TYPE_FULL_PLATE_ARMS, "full plate sleeves", ARMOR_TYPE_HEAVY,
           15000, 25, 7, -6, 35, 20, 15,
           9, MATERIAL_STEEL, ITEM_WEAR_ARMS, "<ask staff to fill this out>");
  setarmor(SPEC_ARMOR_TYPE_FULL_PLATE_LEGS, "full plate leggings", ARMOR_TYPE_HEAVY,
           15000, 25, 7, -6, 35, 20, 15,
           9, MATERIAL_STEEL, ITEM_WEAR_LEGS, "<ask staff to fill this out>");

  /* (armor, name, type,
   *    cost, AC, dexBonusCap, armorCheckPenalty, spellFailChance, (move)30ft, (move)20ft,
   *    weight, material, wear) */
  setarmor(SPEC_ARMOR_TYPE_BUCKLER, "buckler shield", ARMOR_TYPE_SHIELD,
           150, 10, 99, -1, 5, 999, 999,
           5, MATERIAL_WOOD, ITEM_WEAR_SHIELD, "Bucklers are small round shields worn on the forearm.");
  setarmor(SPEC_ARMOR_TYPE_SMALL_SHIELD, "light shield", ARMOR_TYPE_SHIELD,
           90, 15, 99, -1, 5, 999, 999,
           6, MATERIAL_WOOD, ITEM_WEAR_SHIELD, "A light shield is strapped to one's forearm and gripped by hand and is relatively light.");
  setarmor(SPEC_ARMOR_TYPE_LARGE_SHIELD, "heavy shield", ARMOR_TYPE_SHIELD,
           200, 20, 99, -2, 15, 999, 999,
           13, MATERIAL_WOOD, ITEM_WEAR_SHIELD, "A heavy shield is strapped to one's forearm and gripped by hand and is relatively heavy.");
  setarmor(SPEC_ARMOR_TYPE_TOWER_SHIELD, "tower shield", ARMOR_TYPE_TOWER_SHIELD,
           300, 40, 7, -10, 50, 999, 999,
           45, MATERIAL_WOOD, ITEM_WEAR_SHIELD, "A tower shield is strapped to one's forearm and gripped by hand and is relatively very heavy.  It often resembles a wall and protects most of the body.");
}

/******* special mixed checks (such as monk) */

bool is_bare_handed(struct char_data *ch)
{
  if (GET_EQ(ch, WEAR_HOLD_1))
    return FALSE;
  if (GET_EQ(ch, WEAR_HOLD_2))
    return FALSE;
  if (GET_EQ(ch, WEAR_SHIELD))
    return FALSE;
  if (GET_EQ(ch, WEAR_WIELD_1))
    return FALSE;
  if (GET_EQ(ch, WEAR_WIELD_OFFHAND))
    return FALSE;
  if (GET_EQ(ch, WEAR_WIELD_2H))
    return FALSE;
  /* made it */
  return TRUE;
}

/* our simple little function to make sure our monk
   is following his martial-arts requirements for gear */
bool monk_gear_ok(struct char_data *ch)
{
  struct obj_data *obj = NULL;

  /* hands have to be free, or wielding monk family weapon */
  if (GET_EQ(ch, WEAR_HOLD_1))
    return FALSE;

  if (GET_EQ(ch, WEAR_HOLD_2))
    return FALSE;

  if (GET_EQ(ch, WEAR_SHIELD))
    return FALSE;

  obj = GET_EQ(ch, WEAR_WIELD_1);
  if (obj &&
      (weapon_list[GET_WEAPON_TYPE(obj)].weaponFamily != WEAPON_FAMILY_MONK))
    return FALSE;

  obj = GET_EQ(ch, WEAR_WIELD_OFFHAND);
  if (obj &&
      (weapon_list[GET_WEAPON_TYPE(obj)].weaponFamily != WEAPON_FAMILY_MONK))
    return FALSE;

  obj = GET_EQ(ch, WEAR_WIELD_2H);
  if (obj &&
      (weapon_list[GET_WEAPON_TYPE(obj)].weaponFamily != WEAPON_FAMILY_MONK))
    return FALSE;

  /* now check to make sure he isn't wearing invalid armor */
  if (compute_gear_armor_type(ch) != ARMOR_TYPE_NONE)
    return FALSE;

  /* monk gear is ok */
  return TRUE;
}

/* ACMD */

/* list all the weapon defines in-game */
ACMD(do_weaponlist_old)
{
  int type = 0;
  char buf[MAX_STRING_LENGTH] = {'\0'};
  char buf2[100];
  char buf3[100];
  size_t len = 0;
  int crit_multi = 0;

  for (type = 1; type < NUM_WEAPON_TYPES; type++)
  {

    /* have to do some calculations beforehand */
    switch (weapon_list[type].critMult)
    {
    case CRIT_X2:
      crit_multi = 2;
      break;
    case CRIT_X3:
      crit_multi = 3;
      break;
    case CRIT_X4:
      crit_multi = 4;
      break;
    case CRIT_X5:
      crit_multi = 5;
      break;
    case CRIT_X6:
      crit_multi = 6;
      break;
    }
    sprintbit(weapon_list[type].weaponFlags, weapon_flags, buf2, sizeof(buf2));
    sprintbit(weapon_list[type].damageTypes, weapon_damage_types, buf3, sizeof(buf3));

    len += snprintf(buf + len, sizeof(buf) - len,
                    "\tW%s\tn, Dam: %dd%d, Threat: %d, Crit-Multi: %d, Flags: %s, Cost: %d, "
                    "Dam-Types: %s, Weight: %d, Range: %d, Family: %s, Size: %s, Material: %s, "
                    "Handle: %s, Head: %s.\r\n",
                    weapon_list[type].name, weapon_list[type].numDice, weapon_list[type].diceSize,
                    (20 - weapon_list[type].critRange), crit_multi, buf2, weapon_list[type].cost,
                    buf3, weapon_list[type].weight, weapon_list[type].range,
                    weapon_family[weapon_list[type].weaponFamily],
                    sizes[weapon_list[type].size], material_name[weapon_list[type].material],
                    weapon_handle_types[weapon_list[type].handle_type],
                    weapon_head_types[weapon_list[type].head_type]);
  }
  page_string(ch->desc, buf, 1);
}

/* list all the weapon defines in-game */
ACMD(do_armorlist_old)
{
  int i = 0;
  char buf[MAX_STRING_LENGTH] = {'\0'};
  size_t len = 0;

  for (i = 1; i < NUM_SPEC_ARMOR_TYPES; i++)
  {
    len += snprintf(buf + len, sizeof(buf) - len, "\tW%s\tn, Type: %s, Cost: %d, "
                                                  "AC: %.1f, Max Dex: %d, Armor Penalty: %d, Spell Fail: %d, Weight: %d, "
                                                  "Material: %s\r\n",
                    armor_list[i].name, armor_type[armor_list[i].armorType],
                    armor_list[i].cost, (float)armor_list[i].armorBonus / 10.0,
                    armor_list[i].dexBonus, armor_list[i].armorCheck, armor_list[i].spellFail,
                    armor_list[i].weight, material_name[armor_list[i].material]);
  }
  page_string(ch->desc, buf, 1);
}

/* end ACMD */

/*********** deprecated functions *****************/

/* Proficiency Related Functions */

/*
#define ITEM_PROF_NONE		0	// no proficiency required
#define ITEM_PROF_MINIMAL	1	//  "Minimal Weapon Proficiency"
#define ITEM_PROF_BASIC		2	//  "Basic Weapon Proficiency"
#define ITEM_PROF_ADVANCED	3	//  "Advanced Weapon Proficiency"
#define ITEM_PROF_MASTER 	4	//  "Master Weapon Proficiency"
#define ITEM_PROF_EXOTIC 	5	//  "Exotic Weapon Proficiency"
#define ITEM_PROF_LIGHT_A	6	// light armor prof
#define ITEM_PROF_MEDIUM_A	7	// medium armor prof
#define ITEM_PROF_HEAVY_A	8	// heavy armor prof
#define ITEM_PROF_SHIELDS	9	// shield prof
#define ITEM_PROF_T_SHIELDS	10	// tower shield prof
 */

/* a function to check the -highest- level of proficiency of gear
   worn on a character
 * in:  requires character (pc only), type is either weapon/armor/shield
 * out:  value of highest proficiency worn
 *  */

/* deprecated */
/*
int proficiency_worn(struct char_data *ch, int type) {
  int prof = ITEM_PROF_NONE, i = 0;

  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i)) {
      if (type == WEAPON_PROFICIENCY && (
              i == WEAR_WIELD_1 ||
              i == WEAR_WIELD_OFFHAND ||
              i == WEAR_WIELD_2H
              )) {
        if (GET_OBJ_PROF(GET_EQ(ch, i)) > prof) {
          prof = GET_OBJ_PROF(GET_EQ(ch, i));
        }
      } else if (type == SHIELD_PROFICIENCY && (
              i == WEAR_SHIELD
              )) {
        if (GET_OBJ_PROF(GET_EQ(ch, i)) > prof) {
          prof = GET_OBJ_PROF(GET_EQ(ch, i));
        }
      } else if (type == ARMOR_PROFICIENCY && (
              i != WEAR_WIELD_1 &&
              i != WEAR_WIELD_OFFHAND &&
              i != WEAR_WIELD_2H &&
              i != WEAR_SHIELD
              )) {
        if (GET_OBJ_PROF(GET_EQ(ch, i)) > prof) {
          prof = GET_OBJ_PROF(GET_EQ(ch, i));
        }
      }
    }
  }

  switch (type) {
    case WEAPON_PROFICIENCY:
      if (prof < 0 || prof >= NUM_WEAPON_PROFS)
        return ITEM_PROF_NONE;
      break;
    case ARMOR_PROFICIENCY:
      if (prof < NUM_WEAPON_PROFS || prof >= NUM_ARMOR_PROFS)
        return ITEM_PROF_NONE;
      break;
    case SHIELD_PROFICIENCY:
      if (prof < NUM_ARMOR_PROFS || prof >= NUM_SHIELD_PROFS)
        return ITEM_PROF_NONE;
      break;
  }

  return prof;
}
 */

/* deprecated */
/*
int determine_gear_weight(struct char_data *ch, int type) {
  int i = 0, weight = 0;

  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i)) {
      if (type == WEAPON_PROFICIENCY && (
              i == WEAR_WIELD_1 ||
              i == WEAR_WIELD_OFFHAND ||
              i == WEAR_WIELD_2H
              )) {
        weight += GET_OBJ_WEIGHT(GET_EQ(ch, i));
      } else if (type == SHIELD_PROFICIENCY && (
              i == WEAR_SHIELD
              )) {
        weight += GET_OBJ_WEIGHT(GET_EQ(ch, i));
      } else if (type == ARMOR_PROFICIENCY && (
              i == WEAR_HEAD ||
              i == WEAR_BODY ||
              i == WEAR_ARMS ||
              i == WEAR_LEGS
              )) {
        weight += GET_OBJ_WEIGHT(GET_EQ(ch, i));
      }
    }
  }

  return weight;
}
 */

/* this function will determine the penalty (or lack of) created
 by the gear the character is wearing - this penalty is mostly in
 regards to rogue-like skills such as sneak/hide */
/* deprecated */
/*
int compute_gear_penalty_check(struct char_data *ch) {
  int factor = determine_gear_weight(ch, ARMOR_PROFICIENCY);
  factor += determine_gear_weight(ch, SHIELD_PROFICIENCY);

  if (factor > 51)
    return -8;
  if (factor >= 45)
    return -6;
  if (factor >= 40)
    return -5;
  if (factor >= 35)
    return -4;
  if (factor >= 30)
    return -3;
  if (factor >= 25)
    return -2;
  if (factor >= 20)
    return -1;

  return 0; //should be less than 10
}
 */

/* this function will determine the % penalty created by the
   gear the char is wearing - this penalty is unique to
   arcane casting only (sorc, wizard, bard, etc) */
/* deprecated */
/*
int compute_gear_arcane_fail(struct char_data *ch) {
  int factor = determine_gear_weight(ch, ARMOR_PROFICIENCY);
  factor += determine_gear_weight(ch, SHIELD_PROFICIENCY);

  factor -= HAS_FEAT(ch, FEAT_ARMORED_SPELLCASTING) * 5;

  if (factor > 51)
    return 50;
  if (factor >= 45)
    return 40;
  if (factor >= 40)
    return 35;
  if (factor >= 35)
    return 30;
  if (factor >= 30)
    return 25;
  if (factor >= 25)
    return 20;
  if (factor >= 20)
    return 15;
  if (factor >= 15)
    return 10;
  if (factor >= 10)
    return 5;

  return 0; //should be less than 10

}
 */

/* this function will determine the dam-reduc created by the
   gear the char is wearing  */
/* deprecated */
/*
int compute_gear_dam_reduc(struct char_data *ch) {
  int factor = determine_gear_weight(ch, ARMOR_PROFICIENCY);
  int shields = determine_gear_weight(ch, SHIELD_PROFICIENCY);

  if (shields > factor)
    factor = shields;

  if (factor > 51)
    return 6;
  if (factor >= 45)
    return 4;
  if (factor >= 40)
    return 4;
  if (factor >= 35)
    return 3;
  if (factor >= 30)
    return 3;
  if (factor >= 25)
    return 2;
  if (factor >= 20)
    return 2;
  if (factor >= 15)
    return 1;
  if (factor >= 10)
    return 1;

  return 0; //should be less than 10
}
 */

/* this function will determine the max-dex created by the
   gear the char is wearing  */
/* deprecated */
/*
int compute_gear_max_dex(struct char_data *ch) {
  int factor = determine_gear_weight(ch, ARMOR_PROFICIENCY);
  int shields = determine_gear_weight(ch, SHIELD_PROFICIENCY);

  if (shields > factor)
    factor = shields;

  if (factor > 51)
    return 0;
  if (factor >= 45)
    return 1;
  if (factor >= 40)
    return 2;
  if (factor >= 35)
    return 3;
  if (factor >= 30)
    return 4;
  if (factor >= 25)
    return 5;
  if (factor >= 20)
    return 7;
  if (factor >= 15)
    return 9;
  if (factor >= 10)
    return 11;

  if (factor >= 1)
    return 13;
  else
    return 99; // wearing no weight!
}
 */

bool is_two_handed_ranged_weapon(struct obj_data *obj)
{

  if (!obj)
    return FALSE;

  if (GET_OBJ_TYPE(obj) != ITEM_WEAPON)
    return FALSE;

  int type = GET_OBJ_VAL(obj, 0);

  if (type < 0 || type > NUM_WEAPON_TYPES)
    return FALSE;

  if (weapon_list[type].weaponFamily != WEAPON_FAMILY_RANGED)
    return FALSE;

  if (type == WEAPON_TYPE_DART || type == WEAPON_TYPE_SLING || type == WEAPON_TYPE_HAND_CROSSBOW || type == WEAPON_TYPE_BOLA)
    return FALSE;

  return TRUE;
}

int get_defending_weapon_bonus(struct char_data *ch, bool weapon)
{
  struct obj_data *obj = NULL;
  int bonus = weapon ? 0 : 1;

  obj = GET_EQ(ch, WEAR_WIELD_1);
  if (obj && OBJ_FLAGGED(obj, ITEM_DEFENDING))
    bonus += GET_OBJ_VAL(obj, 1) / 2;

  obj = GET_EQ(ch, WEAR_WIELD_2H);
  if (obj && OBJ_FLAGGED(obj, ITEM_DEFENDING))
    bonus += GET_OBJ_VAL(obj, 1) / 2;

  obj = GET_EQ(ch, WEAR_WIELD_OFFHAND);
  if (obj && OBJ_FLAGGED(obj, ITEM_DEFENDING))
    bonus += GET_OBJ_VAL(obj, 1) / 2;

  return bonus;
}

bool has_speed_weapon(struct char_data *ch)
{
  struct obj_data *obj = NULL;

  obj = GET_EQ(ch, WEAR_WIELD_1);
  if (obj && obj_has_special_ability(obj, WEAPON_SPECAB_SPEED))
    return true;

  obj = GET_EQ(ch, WEAR_WIELD_2H);
  if (obj && obj_has_special_ability(obj, WEAPON_SPECAB_SPEED))
    return true;

  obj = GET_EQ(ch, WEAR_WIELD_OFFHAND);
  if (obj && obj_has_special_ability(obj, WEAPON_SPECAB_SPEED))
    return true;

  if (FIENDISH_BOON_ACTIVE(ch, FIENDISH_BOON_SPEED))
    return true;

  return false;
}

bool is_using_ghost_touch_weapon(struct char_data *ch)
{
  struct obj_data *obj = NULL;

  obj = GET_EQ(ch, WEAR_WIELD_1);
  if (obj && obj_has_special_ability(obj, WEAPON_SPECAB_GHOST_TOUCH))
    return true;

  obj = GET_EQ(ch, WEAR_WIELD_2H);
  if (obj && obj_has_special_ability(obj, WEAPON_SPECAB_GHOST_TOUCH))
    return true;

  obj = GET_EQ(ch, WEAR_WIELD_OFFHAND);
  if (obj && obj_has_special_ability(obj, WEAPON_SPECAB_GHOST_TOUCH))
    return true;

  return false;
}

bool is_using_keen_weapon(struct char_data *ch)
{
  struct obj_data *obj = NULL;

  obj = GET_EQ(ch, WEAR_WIELD_1);
  if (obj && obj_has_special_ability(obj, WEAPON_SPECAB_KEEN))
    return true;
  if (obj && affected_by_spell(ch, PSIONIC_SHARPENED_EDGE) && IS_WEAPON_SHARP(obj))
    return true;
  if (obj && (IS_SET(weapon_list[GET_WEAPON_TYPE(obj)].damageTypes, DAMAGE_TYPE_PIERCING) || IS_SET(weapon_list[GET_WEAPON_TYPE(obj)].damageTypes, DAMAGE_TYPE_SLASHING)) &&
      affected_by_spell(ch, SPELL_KEEN_EDGE))
    return true;
  if ((!obj || (obj && (IS_SET(weapon_list[GET_WEAPON_TYPE(obj)].damageTypes, DAMAGE_TYPE_BLUDGEONING)))) && affected_by_spell(ch, SPELL_WEAPON_OF_IMPACT))
    return true;

  obj = GET_EQ(ch, WEAR_WIELD_2H);
  if (obj && obj_has_special_ability(obj, WEAPON_SPECAB_KEEN))
    return true;
  if (obj && affected_by_spell(ch, PSIONIC_SHARPENED_EDGE) && IS_WEAPON_SHARP(obj))
    return true;
  if (obj && (IS_SET(weapon_list[GET_WEAPON_TYPE(obj)].damageTypes, DAMAGE_TYPE_PIERCING) || IS_SET(weapon_list[GET_WEAPON_TYPE(obj)].damageTypes, DAMAGE_TYPE_SLASHING)) &&
      affected_by_spell(ch, SPELL_KEEN_EDGE))
    return true;
  if ((!obj || (obj && (IS_SET(weapon_list[GET_WEAPON_TYPE(obj)].damageTypes, DAMAGE_TYPE_BLUDGEONING)))) && affected_by_spell(ch, SPELL_WEAPON_OF_IMPACT))
    return true;

  obj = GET_EQ(ch, WEAR_WIELD_OFFHAND);
  if (obj && obj_has_special_ability(obj, WEAPON_SPECAB_KEEN))
    return true;
  if (obj && affected_by_spell(ch, PSIONIC_SHARPENED_EDGE) && IS_WEAPON_SHARP(obj))
    return true;
  if (obj && (IS_SET(weapon_list[GET_WEAPON_TYPE(obj)].damageTypes, DAMAGE_TYPE_PIERCING) || IS_SET(weapon_list[GET_WEAPON_TYPE(obj)].damageTypes, DAMAGE_TYPE_SLASHING)) &&
      affected_by_spell(ch, SPELL_KEEN_EDGE))
    return true;
  if ((!obj || (obj && (IS_SET(weapon_list[GET_WEAPON_TYPE(obj)].damageTypes, DAMAGE_TYPE_BLUDGEONING)))) && affected_by_spell(ch, SPELL_WEAPON_OF_IMPACT))
    return true;

  if (FIENDISH_BOON_ACTIVE(ch, FIENDISH_BOON_KEEN))
    return true;

  return false;
}

int get_lucky_weapon_bonus(struct char_data *ch)
{
  if (!ch)
    return 0;

  struct obj_data *obj = NULL;
  int bonus = 0;

  obj = GET_EQ(ch, WEAR_WIELD_1);
  if (obj && obj_has_special_ability(obj, WEAPON_SPECAB_LUCKY))
    bonus = GET_OBJ_VAL(obj, 4);

  obj = GET_EQ(ch, WEAR_WIELD_2H);
  if (obj && obj_has_special_ability(obj, WEAPON_SPECAB_LUCKY))
    bonus = MAX(bonus, GET_OBJ_VAL(obj, 4));

  obj = GET_EQ(ch, WEAR_WIELD_OFFHAND);
  if (obj && obj_has_special_ability(obj, WEAPON_SPECAB_LUCKY))
    bonus = MAX(bonus, GET_OBJ_VAL(obj, 4));

  return bonus;
}

int get_agile_weapon_dex_bonus(struct char_data *ch)
{
  struct obj_data *obj = NULL;
  int bonus = 0;
  int dex_bonus = MAX(0, GET_DEX_BONUS(ch));

  obj = GET_EQ(ch, WEAR_WIELD_1);
  if (obj && obj_has_special_ability(obj, WEAPON_SPECAB_AGILE))
    bonus = GET_OBJ_VAL(obj, 4);

  obj = GET_EQ(ch, WEAR_WIELD_2H);
  if (obj && obj_has_special_ability(obj, WEAPON_SPECAB_AGILE))
    bonus = MAX(bonus, GET_OBJ_VAL(obj, 4));

  obj = GET_EQ(ch, WEAR_WIELD_OFFHAND);
  if (obj && obj_has_special_ability(obj, WEAPON_SPECAB_AGILE))
    bonus = MAX(bonus, GET_OBJ_VAL(obj, 4));

  bonus = MIN(bonus, dex_bonus);

  return bonus;
}