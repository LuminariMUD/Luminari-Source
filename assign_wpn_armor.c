/*****************************************************************************
 * assign_wpn_armor.c                        Part of LuminariMUD
 * author: zusuk
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


/* global */
struct armor_table armor_list[NUM_SPEC_ARMOR_TYPES];
struct weapon_table weapon_list[NUM_WEAPON_TYPES];
const char *weapon_type[NUM_WEAPON_TYPES];


/* simply checks if ch has proficiency with given weapon_type */
int is_proficient_with_weapon(struct char_data *ch, int weapon) {

  /* :) */
  if (weapon == WEAPON_TYPE_UNARMED && CLASS_LEVEL(ch, CLASS_MONK))
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

  /* updated by zusuk: Druids are proficient with the following weapons: club, 
   * dagger, dart, quarterstaff, scimitar, scythe, sickle, shortspear, sling, 
   * and spear. They are also proficient with all natural attacks (claw, bite, 
   * and so forth) of any form they assume with wild shape.*/
  if (HAS_FEAT(ch, FEAT_WEAPON_PROFICIENCY_DRUID) ||
          CLASS_LEVEL(ch, CLASS_DRUID) > 0) {
    switch (weapon) {
      case WEAPON_TYPE_CLUB:
      case WEAPON_TYPE_DAGGER:
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

  if (HAS_FEAT(ch, FEAT_WEAPON_PROFICIENCY_BARD) ||
          CLASS_LEVEL(ch, CLASS_BARD) > 0) {
    switch (weapon) {
      case WEAPON_TYPE_LONG_SWORD:
      case WEAPON_TYPE_RAPIER:
      case WEAPON_TYPE_SAP:
      case WEAPON_TYPE_SHORT_SWORD:
      case WEAPON_TYPE_SHORT_BOW:
      case WEAPON_TYPE_WHIP:
        return TRUE;
    }
  }

  if (HAS_FEAT(ch, FEAT_WEAPON_PROFICIENCY_ROGUE) ||
          CLASS_LEVEL(ch, CLASS_ROGUE) > 0) {
    switch (weapon) {
      case WEAPON_TYPE_HAND_CROSSBOW:
      case WEAPON_TYPE_RAPIER:
      case WEAPON_TYPE_SAP:
      case WEAPON_TYPE_SHORT_SWORD:
      case WEAPON_TYPE_SHORT_BOW:
        return TRUE;
    }
  }

  if (HAS_FEAT(ch, FEAT_WEAPON_PROFICIENCY_WIZARD) ||
          CLASS_LEVEL(ch, CLASS_WIZARD) > 0) {
    switch (weapon) {
      case WEAPON_TYPE_DAGGER:
      case WEAPON_TYPE_QUARTERSTAFF:
      case WEAPON_TYPE_CLUB:
      case WEAPON_TYPE_HEAVY_CROSSBOW:
      case WEAPON_TYPE_LIGHT_CROSSBOW:
        return TRUE;
    }
  }

  if (HAS_FEAT(ch, FEAT_WEAPON_PROFICIENCY_DROW) ||
          IS_DROW(ch)) {
    switch (weapon) {
      case WEAPON_TYPE_HAND_CROSSBOW:
      case WEAPON_TYPE_RAPIER:
      case WEAPON_TYPE_SHORT_SWORD:
        return TRUE;
    }
  }

  if (HAS_FEAT(ch, FEAT_WEAPON_PROFICIENCY_ELF) ||
          IS_ELF(ch)) {
    switch (weapon) {
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

  if (IS_DWARF(ch) &&
          HAS_FEAT(ch, FEAT_MARTIAL_WEAPON_PROFICIENCY)) {
    switch (weapon) {
      case WEAPON_TYPE_DWARVEN_WAR_AXE:
      case WEAPON_TYPE_DWARVEN_URGOSH:
        return TRUE;
    }
  }
  
  /* cleric domain, favored weapons */
  if (domain_list[GET_1ST_DOMAIN(ch)].favored_weapon == weapon)
    return TRUE;
  if (domain_list[GET_2ND_DOMAIN(ch)].favored_weapon == weapon)
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
bool weapon_needs_reload(struct char_data *ch, struct obj_data *weapon, bool silent_mode) {
  /* object value 5 is for loaded status */
  if (GET_OBJ_VAL(weapon, 5) > 0) {
    if (!silent_mode)
      send_to_char(ch, "Your weapon is not empty yet!\r\n");
    return FALSE;
  }
  return TRUE;
}

bool ready_to_reload(struct char_data *ch, struct obj_data *wielded, bool silent_mode) {
  switch (GET_OBJ_VAL(wielded, 0)) {
    case WEAPON_TYPE_HEAVY_REP_XBOW:
    case WEAPON_TYPE_LIGHT_REP_XBOW:
    case WEAPON_TYPE_HEAVY_CROSSBOW:

      /* RAPID RELOAD! */
      if (HAS_FEAT(ch, FEAT_RAPID_RELOAD)) {
        if (is_action_available(ch, atMOVE, FALSE)) {
          if (reload_weapon(ch, wielded, silent_mode)) {
            USE_MOVE_ACTION(ch); /* success! */
          } else {
            /* failed reload */
            if (!silent_mode)
              send_to_char(ch, "You need a move action to reload!\r\n");
            return FALSE;
          }
        } else {
          /* reloading requires a move action */
          if (!silent_mode)
            send_to_char(ch, "You need a move action to reload!\r\n");
          return FALSE;
        }

        /* no rapid reload */
      } else if (is_action_available(ch, atSTANDARD, FALSE) &&
              is_action_available(ch, atMOVE, FALSE)) {
        if (reload_weapon(ch, wielded, silent_mode)) {
          USE_FULL_ROUND_ACTION(ch); /* success! */
        } else {
          if (!silent_mode)
            send_to_char(ch, "You need a full round action to reload!\r\n");
          /* failed reload */
          return FALSE;
        }

      } else {
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

      else if (is_action_available(ch, atMOVE, FALSE)) {
        if (reload_weapon(ch, wielded, silent_mode)) {
          USE_MOVE_ACTION(ch); /* success! */
        } else {
          /* failed reload */
          if (!silent_mode)
            send_to_char(ch, "You need a move action to reload!\r\n");
          return FALSE;
        }
      } else {
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
        bool silent_mode) {

  /* position check */
  if (GET_POS(ch) <= POS_STUNNED) {
    if (!silent_mode)
      send_to_char(ch, "You are in no position to do this!\r \n");
    return FALSE;
  }

  /* can't do this if stunned */
  if (AFF_FLAGGED(ch, AFF_STUN) || char_has_mud_event(ch, eSTUNNED)) {
    if (!silent_mode)
      send_to_char(ch, "You can not reload a weapon while stunned!\r\n");
    return FALSE;
  }

  /* ranged weapon? */
  if (!is_using_ranged_weapon(ch, silent_mode)) {
    return FALSE;
  }

  /* weapon that needs reloading? */
  if (!is_reloading_weapon(ch, weapon, silent_mode)) {
    return FALSE;
  }

  /* emptied out yet? */
  if (!weapon_needs_reload(ch, weapon, silent_mode)) {
    return FALSE;
  }

  /* check for actions, if available, reload */
  if (!ready_to_reload(ch, weapon, silent_mode)) {
    return FALSE;
  }

  /* success! */
  send_to_char(ch, "You reload %s.\r\n", weapon->short_description);
  if (FIGHTING(ch))
    FIRING(ch) = TRUE;
  return TRUE;
}

/* ranged-weapons, reload mechanic for slings, crossbows */
bool auto_reload_weapon(struct char_data *ch, bool silent_mode) {
  struct obj_data *wielded = is_using_ranged_weapon(ch, silent_mode);

  if (!process_load_weapon(ch, wielded, silent_mode))
    return FALSE;

  return TRUE;
}


#define MAX_AMMO_INSIDE_WEAPON 5 //unused

bool reload_weapon(struct char_data *ch, struct obj_data *wielded, bool silent_mode) {
  int load_amount = 0;

  switch (GET_OBJ_VAL(wielded, 0)) {
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
bool weapon_is_loaded(struct char_data *ch, struct obj_data *wielded, bool silent) {

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTORELOAD) && FIGHTING(ch))
    silent = TRUE; /* ornir suggested this */

  if (GET_OBJ_VAL(wielded, 5) <= 0) { /* object value 5 is for loaded status */
    if (!silent)
      send_to_char(ch, "You have to reload your weapon!\r\n");
    FIRING(ch) = FALSE;
    return FALSE;
  }

  return TRUE;
}

/* this function will check to make sure ammo is ready for firing */
bool has_ammo_in_pouch(struct char_data *ch, struct obj_data *wielded,
        bool silent) {
  struct obj_data *ammo_pouch = GET_EQ(ch, WEAR_AMMO_POUCH);

  if (!wielded) {
    if (!silent)
      send_to_char(ch, "You have no weapon!\r\n");
    FIRING(ch) = FALSE;
    return FALSE;
  }

  if (!ammo_pouch) {
    if (!silent)
      send_to_char(ch, "You have no ammo pouch!\r\n");
    FIRING(ch) = FALSE;
    return FALSE;
  }

  if (!ammo_pouch->contains) {
    if (!silent)
      send_to_char(ch, "Your ammo pouch is empty!\r\n");
    FIRING(ch) = FALSE;
    return FALSE;
  }

  if (GET_OBJ_TYPE(ammo_pouch->contains) != ITEM_MISSILE) {
    if (!silent)
      send_to_char(ch, "Your ammo pouch needs to be filled with only ammo!\r\n");
    FIRING(ch) = FALSE;
    return FALSE;
  }

  switch (GET_OBJ_VAL(ammo_pouch->contains, 0)) {

    case AMMO_TYPE_ARROW:
      switch (GET_OBJ_VAL(wielded, 0)) {
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
      switch (GET_OBJ_VAL(wielded, 0)) {
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
      switch (GET_OBJ_VAL(wielded, 0)) {
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
      switch (GET_OBJ_VAL(wielded, 0)) {
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
bool can_fire_ammo(struct char_data *ch, bool silent) {
  struct obj_data *wielded = NULL;

  if (!(wielded = is_using_ranged_weapon(ch, silent))) {
    FIRING(ch) = FALSE;
    return FALSE;
  }

  if (!has_ammo_in_pouch(ch, wielded, silent)) {
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
struct obj_data *is_using_ranged_weapon(struct char_data *ch, bool silent_mode) {
  struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD_2H);

  if (!wielded)
    wielded = GET_EQ(ch, WEAR_WIELD_1);
  if (!wielded)
    wielded = GET_EQ(ch, WEAR_WIELD_OFFHAND);

  if (!wielded) {
    if (!silent_mode)
      send_to_char(ch, "You are not wielding a ranged weapon!\r\n");
    return NULL;
  }

  if (IS_WILDSHAPED(ch) || IS_MORPHED(ch)) {
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
bool is_reloading_weapon(struct char_data *ch, struct obj_data *wielded, bool silent_mode) {
  /* value 0 = weapon define value */
  switch (GET_OBJ_VAL(wielded, 0)) {
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
bool is_using_light_weapon(struct char_data *ch, struct obj_data *wielded) {

  if (!wielded) /* fists are light?  i need to check this */
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
bool is_using_double_weapon(struct char_data *ch) {
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

void setweapon(int type, char *name, int numDice, int diceSize, int critRange, int critMult,
        int weaponFlags, int cost, int damageTypes, int weight, int range, int weaponFamily, int size,
        int material, int handle_type, int head_type) {
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
}

void initialize_weapons(int type) {
  weapon_list[type].name = "unused weapon";
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

void load_weapons(void) {
  int i = 0;

  for (i = 0; i < NUM_WEAPON_TYPES; i++)
    initialize_weapons(i);

  /*	(weapon num, name, numDamDice, sizeDamDice, critRange, critMult, weapon flags, cost,
   * damageType, weight, range, weaponFamily, Size, material,
   * handle, head) */
  setweapon(WEAPON_TYPE_UNARMED, "unarmed", 1, 3, 0, 2, WEAPON_FLAG_SIMPLE, 200,
          DAMAGE_TYPE_BLUDGEONING, 1, 0, WEAPON_FAMILY_MONK, SIZE_SMALL, MATERIAL_ORGANIC,
          HANDLE_TYPE_GLOVE, HEAD_TYPE_FIST);
  setweapon(WEAPON_TYPE_DAGGER, "dagger", 1, 4, 1, 2, WEAPON_FLAG_THROWN |
          WEAPON_FLAG_SIMPLE, 200, DAMAGE_TYPE_PIERCING, 1, 10, WEAPON_FAMILY_SMALL_BLADE, SIZE_TINY,
          MATERIAL_STEEL, HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_LIGHT_MACE, "light mace", 1, 6, 0, 2, WEAPON_FLAG_SIMPLE, 500,
          DAMAGE_TYPE_BLUDGEONING, 4, 0, WEAPON_FAMILY_CLUB, SIZE_SMALL, MATERIAL_STEEL,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
  setweapon(WEAPON_TYPE_SICKLE, "sickle", 1, 6, 0, 2, WEAPON_FLAG_SIMPLE, 600,
          DAMAGE_TYPE_SLASHING, 2, 0, WEAPON_FAMILY_SMALL_BLADE, SIZE_SMALL, MATERIAL_STEEL,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_CLUB, "club", 1, 6, 0, 2, WEAPON_FLAG_SIMPLE, 10,
          DAMAGE_TYPE_BLUDGEONING, 3, 0, WEAPON_FAMILY_CLUB, SIZE_SMALL, MATERIAL_WOOD,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
  setweapon(WEAPON_TYPE_HEAVY_MACE, "heavy mace", 1, 8, 0, 2, WEAPON_FLAG_SIMPLE, 1200,
          DAMAGE_TYPE_BLUDGEONING, 8, 0, WEAPON_FAMILY_CLUB, SIZE_MEDIUM, MATERIAL_STEEL,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
  setweapon(WEAPON_TYPE_MORNINGSTAR, "morningstar", 1, 8, 0, 2, WEAPON_FLAG_SIMPLE, 800,
          DAMAGE_TYPE_BLUDGEONING | DAMAGE_TYPE_PIERCING, 6, 0, WEAPON_FAMILY_FLAIL, SIZE_MEDIUM,
          MATERIAL_STEEL, HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
  setweapon(WEAPON_TYPE_SHORTSPEAR, "shortspear", 1, 6, 0, 2, WEAPON_FLAG_SIMPLE |
          WEAPON_FLAG_THROWN, 100, DAMAGE_TYPE_PIERCING, 3, 20, WEAPON_FAMILY_SPEAR, SIZE_MEDIUM,
          MATERIAL_WOOD, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT);
  setweapon(WEAPON_TYPE_LONGSPEAR, "longspear", 1, 8, 0, 3, WEAPON_FLAG_SIMPLE |
          WEAPON_FLAG_REACH, 500, DAMAGE_TYPE_PIERCING, 9, 0, WEAPON_FAMILY_SPEAR, SIZE_LARGE,
          MATERIAL_WOOD, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT);
  setweapon(WEAPON_TYPE_QUARTERSTAFF, "quarterstaff", 1, 6, 0, 2, WEAPON_FLAG_SIMPLE,
          10, DAMAGE_TYPE_BLUDGEONING, 4, 0, WEAPON_FAMILY_MONK, SIZE_LARGE,
          MATERIAL_WOOD, HANDLE_TYPE_SHAFT, HEAD_TYPE_HEAD);
  setweapon(WEAPON_TYPE_SPEAR, "spear", 1, 8, 0, 3, WEAPON_FLAG_SIMPLE |
          WEAPON_FLAG_THROWN | WEAPON_FLAG_REACH, 200, DAMAGE_TYPE_PIERCING, 6, 20, WEAPON_FAMILY_SPEAR, SIZE_LARGE,
          MATERIAL_WOOD, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT);
  setweapon(WEAPON_TYPE_HEAVY_CROSSBOW, "heavy crossbow", 1, 10, 1, 2, WEAPON_FLAG_SIMPLE
          | WEAPON_FLAG_SLOW_RELOAD | WEAPON_FLAG_RANGED, 5000, DAMAGE_TYPE_PIERCING, 8, 120,
          WEAPON_FAMILY_CROSSBOW, SIZE_LARGE, MATERIAL_WOOD, HANDLE_TYPE_HANDLE, HEAD_TYPE_BOW);
  setweapon(WEAPON_TYPE_LIGHT_CROSSBOW, "light crossbow", 1, 8, 1, 2, WEAPON_FLAG_SIMPLE
          | WEAPON_FLAG_SLOW_RELOAD | WEAPON_FLAG_RANGED, 3500, DAMAGE_TYPE_PIERCING, 4, 80,
          WEAPON_FAMILY_CROSSBOW, SIZE_MEDIUM, MATERIAL_WOOD, HANDLE_TYPE_HANDLE, HEAD_TYPE_BOW);
  /*	(weapon num, name, numDamDice, sizeDamDice, critRange, critMult, weapon flags, cost, damageType, weight, range, weaponFamily, Size, material, handle, head) */
  setweapon(WEAPON_TYPE_DART, "dart", 1, 4, 0, 2, WEAPON_FLAG_SIMPLE | WEAPON_FLAG_THROWN
          | WEAPON_FLAG_RANGED, 50, DAMAGE_TYPE_PIERCING, 1, 20, WEAPON_FAMILY_THROWN, SIZE_TINY,
          MATERIAL_WOOD, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT);
  setweapon(WEAPON_TYPE_JAVELIN, "javelin", 1, 6, 0, 2, WEAPON_FLAG_SIMPLE |
          WEAPON_FLAG_THROWN | WEAPON_FLAG_RANGED, 100, DAMAGE_TYPE_PIERCING, 2, 30,
          WEAPON_FAMILY_SPEAR, SIZE_MEDIUM, MATERIAL_WOOD, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT);
  setweapon(WEAPON_TYPE_SLING, "sling", 1, 4, 0, 2, WEAPON_FLAG_SIMPLE |
          WEAPON_FLAG_RANGED, 10, DAMAGE_TYPE_BLUDGEONING, 1, 50, WEAPON_FAMILY_THROWN, SIZE_SMALL,
          MATERIAL_LEATHER, HANDLE_TYPE_STRAP, HEAD_TYPE_POUCH);
  setweapon(WEAPON_TYPE_THROWING_AXE, "throwing axe", 1, 6, 0, 2, WEAPON_FLAG_MARTIAL |
          WEAPON_FLAG_THROWN, 800, DAMAGE_TYPE_SLASHING, 2, 10, WEAPON_FAMILY_AXE, SIZE_SMALL,
          MATERIAL_STEEL, HANDLE_TYPE_HANDLE, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_LIGHT_HAMMER, "light hammer", 1, 4, 0, 2, WEAPON_FLAG_MARTIAL |
          WEAPON_FLAG_THROWN, 100, DAMAGE_TYPE_BLUDGEONING, 2, 20, WEAPON_FAMILY_HAMMER, SIZE_SMALL,
          MATERIAL_STEEL, HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
  setweapon(WEAPON_TYPE_HAND_AXE, "hand axe", 1, 6, 0, 3, WEAPON_FLAG_MARTIAL, 600,
          DAMAGE_TYPE_SLASHING, 3, 0, WEAPON_FAMILY_AXE, SIZE_SMALL, MATERIAL_STEEL, HANDLE_TYPE_HANDLE,
          HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_KUKRI, "kukri", 1, 4, 2, 2, WEAPON_FLAG_MARTIAL, 800,
          DAMAGE_TYPE_SLASHING, 2, 0, WEAPON_FAMILY_SMALL_BLADE, SIZE_SMALL, MATERIAL_STEEL,
          HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_LIGHT_PICK, "light pick", 1, 4, 0, 4, WEAPON_FLAG_MARTIAL, 400,
          DAMAGE_TYPE_PIERCING, 3, 0, WEAPON_FAMILY_PICK, SIZE_SMALL, MATERIAL_STEEL,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
  setweapon(WEAPON_TYPE_SAP, "sap", 1, 6, 0, 2, WEAPON_FLAG_MARTIAL |
          WEAPON_FLAG_NONLETHAL, 100, DAMAGE_TYPE_BLUDGEONING | DAMAGE_TYPE_NONLETHAL, 2, 0,
          WEAPON_FAMILY_CLUB, SIZE_SMALL, MATERIAL_LEATHER, HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
  setweapon(WEAPON_TYPE_SHORT_SWORD, "short sword", 1, 6, 1, 2, WEAPON_FLAG_MARTIAL,
          1000, DAMAGE_TYPE_PIERCING, 2, 0, WEAPON_FAMILY_SMALL_BLADE, SIZE_SMALL, MATERIAL_STEEL,
          HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_BATTLE_AXE, "battle axe", 1, 8, 0, 3, WEAPON_FLAG_MARTIAL, 1000,
          DAMAGE_TYPE_SLASHING, 6, 0, WEAPON_FAMILY_AXE, SIZE_MEDIUM, MATERIAL_STEEL,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_BLADE);
  /*	(weapon num, name, numDamDice, sizeDamDice, critRange, critMult, weapon flags, cost, damageType, weight, range, weaponFamily, Size, material, handle, head) */
  setweapon(WEAPON_TYPE_FLAIL, "flail", 1, 8, 0, 2, WEAPON_FLAG_MARTIAL, 800,
          DAMAGE_TYPE_BLUDGEONING, 5, 0, WEAPON_FAMILY_FLAIL, SIZE_MEDIUM, MATERIAL_STEEL,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
  setweapon(WEAPON_TYPE_LONG_SWORD, "long sword", 1, 8, 1, 2, WEAPON_FLAG_MARTIAL, 1500,
          DAMAGE_TYPE_SLASHING, 4, 0, WEAPON_FAMILY_MEDIUM_BLADE, SIZE_MEDIUM, MATERIAL_STEEL,
          HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_HEAVY_PICK, "heavy pick", 1, 6, 0, 4, WEAPON_FLAG_MARTIAL, 800,
          DAMAGE_TYPE_PIERCING, 6, 0, WEAPON_FAMILY_PICK, SIZE_MEDIUM, MATERIAL_STEEL,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
  setweapon(WEAPON_TYPE_RAPIER, "rapier", 1, 6, 2, 2, WEAPON_FLAG_MARTIAL |
          WEAPON_FLAG_BALANCED, 2000, DAMAGE_TYPE_PIERCING, 2, 0, WEAPON_FAMILY_SMALL_BLADE,
          SIZE_SMALL, MATERIAL_STEEL, HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_SCIMITAR, "scimitar", 1, 6, 2, 2, WEAPON_FLAG_MARTIAL, 1500,
          DAMAGE_TYPE_SLASHING, 4, 0, WEAPON_FAMILY_MEDIUM_BLADE, SIZE_MEDIUM, MATERIAL_STEEL,
          HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_TRIDENT, "trident", 1, 8, 0, 2, WEAPON_FLAG_MARTIAL |
          WEAPON_FLAG_THROWN, 1500, DAMAGE_TYPE_PIERCING, 4, 0, WEAPON_FAMILY_SPEAR, SIZE_MEDIUM,
          MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT);
  setweapon(WEAPON_TYPE_WARHAMMER, "warhammer", 1, 8, 0, 3, WEAPON_FLAG_MARTIAL, 1200,
          DAMAGE_TYPE_BLUDGEONING, 5, 0, WEAPON_FAMILY_HAMMER, SIZE_MEDIUM, MATERIAL_STEEL,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
  setweapon(WEAPON_TYPE_FALCHION, "falchion", 2, 4, 2, 2, WEAPON_FLAG_MARTIAL, 7500,
          DAMAGE_TYPE_SLASHING, 8, 0, WEAPON_FAMILY_LARGE_BLADE, SIZE_LARGE, MATERIAL_STEEL,
          HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_GLAIVE, "glaive", 1, 10, 0, 3, WEAPON_FLAG_MARTIAL |
          WEAPON_FLAG_REACH, 800, DAMAGE_TYPE_SLASHING, 10, 0, WEAPON_FAMILY_POLEARM, SIZE_LARGE,
          MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_GREAT_AXE, "great axe", 1, 12, 0, 3, WEAPON_FLAG_MARTIAL, 2000,
          DAMAGE_TYPE_SLASHING, 12, 0, WEAPON_FAMILY_AXE, SIZE_LARGE, MATERIAL_STEEL,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_GREAT_CLUB, "great club", 1, 10, 0, 2, WEAPON_FLAG_MARTIAL, 500,
          DAMAGE_TYPE_BLUDGEONING, 8, 0, WEAPON_FAMILY_CLUB, SIZE_LARGE, MATERIAL_WOOD,
          HANDLE_TYPE_SHAFT, HEAD_TYPE_HEAD);
  /*	(weapon num, name, numDamDice, sizeDamDice, critRange, critMult, weapon flags, cost, damageType, weight, range, weaponFamily, Size, material, handle, head) */
  setweapon(WEAPON_TYPE_HEAVY_FLAIL, "heavy flail", 1, 10, 1, 2, WEAPON_FLAG_MARTIAL,
          1500, DAMAGE_TYPE_BLUDGEONING, 10, 0, WEAPON_FAMILY_FLAIL, SIZE_LARGE, MATERIAL_STEEL,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
  setweapon(WEAPON_TYPE_GREAT_SWORD, "great sword", 2, 6, 1, 2, WEAPON_FLAG_MARTIAL,
          5000, DAMAGE_TYPE_SLASHING, 8, 0, WEAPON_FAMILY_LARGE_BLADE, SIZE_LARGE, MATERIAL_STEEL,
          HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_GUISARME, "guisarme", 2, 4, 0, 3, WEAPON_FLAG_MARTIAL |
          WEAPON_FLAG_REACH, 900, DAMAGE_TYPE_SLASHING, 12, 0, WEAPON_FAMILY_POLEARM, SIZE_LARGE,
          MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_HALBERD, "halberd", 1, 10, 0, 3, WEAPON_FLAG_MARTIAL |
          WEAPON_FLAG_REACH, 1000, DAMAGE_TYPE_SLASHING | DAMAGE_TYPE_PIERCING, 12, 0,
          WEAPON_FAMILY_POLEARM, SIZE_LARGE, MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_LANCE, "lance", 1, 8, 0, 3, WEAPON_FLAG_MARTIAL |
          WEAPON_FLAG_REACH | WEAPON_FLAG_CHARGE, 1000, DAMAGE_TYPE_PIERCING, 10, 0,
          WEAPON_FAMILY_POLEARM, SIZE_LARGE, MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT);
  setweapon(WEAPON_TYPE_RANSEUR, "ranseur", 2, 4, 0, 3, WEAPON_FLAG_MARTIAL |
          WEAPON_FLAG_REACH, 1000, DAMAGE_TYPE_PIERCING, 10, 0, WEAPON_FAMILY_POLEARM, SIZE_LARGE,
          MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_POINT);
  setweapon(WEAPON_TYPE_SCYTHE, "scythe", 2, 4, 0, 4, WEAPON_FLAG_MARTIAL, 1800,
          DAMAGE_TYPE_SLASHING | DAMAGE_TYPE_PIERCING, 10, 0, WEAPON_FAMILY_POLEARM, SIZE_LARGE,
          MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_LONG_BOW, "long bow", 1, 8, 0, 3, WEAPON_FLAG_MARTIAL |
          WEAPON_FLAG_RANGED, 7500, DAMAGE_TYPE_PIERCING, 3, 100, WEAPON_FAMILY_BOW, SIZE_LARGE,
          MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW);

  setweapon(WEAPON_TYPE_COMPOSITE_LONGBOW, "composite long bow", 1, 8, 0, 3,
          WEAPON_FLAG_MARTIAL | WEAPON_FLAG_RANGED, 10000, DAMAGE_TYPE_PIERCING, 3, 110,
          WEAPON_FAMILY_BOW, SIZE_LARGE, MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW);
  setweapon(WEAPON_TYPE_COMPOSITE_LONGBOW_2, "composite long bow (2)", 1, 8, 0, 3,
          WEAPON_FLAG_MARTIAL | WEAPON_FLAG_RANGED, 20000, DAMAGE_TYPE_PIERCING, 3, 110,
          WEAPON_FAMILY_BOW, SIZE_LARGE, MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW);
  setweapon(WEAPON_TYPE_COMPOSITE_LONGBOW_3, "composite long bow (3)", 1, 8, 0, 3,
          WEAPON_FLAG_MARTIAL | WEAPON_FLAG_RANGED, 30000, DAMAGE_TYPE_PIERCING, 3, 110,
          WEAPON_FAMILY_BOW, SIZE_LARGE, MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW);
  setweapon(WEAPON_TYPE_COMPOSITE_LONGBOW_4, "composite long bow (4)", 1, 8, 0, 3,
          WEAPON_FLAG_MARTIAL | WEAPON_FLAG_RANGED, 40000, DAMAGE_TYPE_PIERCING, 3, 110,
          WEAPON_FAMILY_BOW, SIZE_LARGE, MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW);
  setweapon(WEAPON_TYPE_COMPOSITE_LONGBOW_5, "composite long bow (5)", 1, 8, 0, 3,
          WEAPON_FLAG_MARTIAL | WEAPON_FLAG_RANGED, 50000, DAMAGE_TYPE_PIERCING, 3, 110,
          WEAPON_FAMILY_BOW, SIZE_LARGE, MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW);

  setweapon(WEAPON_TYPE_SHORT_BOW, "short bow", 1, 6, 0, 3, WEAPON_FLAG_MARTIAL |
          WEAPON_FLAG_RANGED, 3000, DAMAGE_TYPE_PIERCING, 2, 60, WEAPON_FAMILY_BOW, SIZE_MEDIUM,
          MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW);

  setweapon(WEAPON_TYPE_COMPOSITE_SHORTBOW, "composite short bow", 1, 6, 0, 3,
          WEAPON_FLAG_MARTIAL | WEAPON_FLAG_RANGED, 7500, DAMAGE_TYPE_PIERCING, 2, 70,
          WEAPON_FAMILY_BOW, SIZE_SMALL, MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW);
  setweapon(WEAPON_TYPE_COMPOSITE_SHORTBOW_2, "composite short bow (2)", 1, 6, 0, 3,
          WEAPON_FLAG_MARTIAL | WEAPON_FLAG_RANGED, 17500, DAMAGE_TYPE_PIERCING, 2, 70,
          WEAPON_FAMILY_BOW, SIZE_SMALL, MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW);
  setweapon(WEAPON_TYPE_COMPOSITE_SHORTBOW_3, "composite short bow (3)", 1, 6, 0, 3,
          WEAPON_FLAG_MARTIAL | WEAPON_FLAG_RANGED, 27500, DAMAGE_TYPE_PIERCING, 2, 70,
          WEAPON_FAMILY_BOW, SIZE_SMALL, MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW);
  setweapon(WEAPON_TYPE_COMPOSITE_SHORTBOW_4, "composite short bow (4)", 1, 6, 0, 3,
          WEAPON_FLAG_MARTIAL | WEAPON_FLAG_RANGED, 37500, DAMAGE_TYPE_PIERCING, 2, 70,
          WEAPON_FAMILY_BOW, SIZE_SMALL, MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW);
  setweapon(WEAPON_TYPE_COMPOSITE_SHORTBOW_5, "composite short bow (5)", 1, 6, 0, 3,
          WEAPON_FLAG_MARTIAL | WEAPON_FLAG_RANGED, 47500, DAMAGE_TYPE_PIERCING, 2, 70,
          WEAPON_FAMILY_BOW, SIZE_SMALL, MATERIAL_WOOD, HANDLE_TYPE_STRING, HEAD_TYPE_BOW);

  /*	(weapon num, name, numDamDice, sizeDamDice, critRange, critMult, weapon flags, cost, damageType, weight, range, weaponFamily, Size, material, handle, head) */
  setweapon(WEAPON_TYPE_KAMA, "kama", 1, 6, 0, 2, WEAPON_FLAG_EXOTIC, 200,
          DAMAGE_TYPE_SLASHING, 2, 0, WEAPON_FAMILY_MONK, SIZE_SMALL, MATERIAL_STEEL,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_NUNCHAKU, "nunchaku", 1, 6, 1, 2, WEAPON_FLAG_EXOTIC, 200,
          DAMAGE_TYPE_BLUDGEONING, 2, 0, WEAPON_FAMILY_MONK, SIZE_SMALL, MATERIAL_WOOD,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_HEAD);
  setweapon(WEAPON_TYPE_SAI, "sai", 1, 4, 1, 2, WEAPON_FLAG_EXOTIC | WEAPON_FLAG_THROWN,
          100, DAMAGE_TYPE_BLUDGEONING, 1, 10, WEAPON_FAMILY_MONK, SIZE_SMALL, MATERIAL_STEEL,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_POINT);
  setweapon(WEAPON_TYPE_SIANGHAM, "siangham", 1, 6, 1, 2, WEAPON_FLAG_EXOTIC, 300,
          DAMAGE_TYPE_PIERCING, 1, 0, WEAPON_FAMILY_MONK, SIZE_SMALL, MATERIAL_STEEL,
          HANDLE_TYPE_HANDLE, HEAD_TYPE_POINT);
  setweapon(WEAPON_TYPE_BASTARD_SWORD, "bastard sword", 1, 10, 1, 2, WEAPON_FLAG_EXOTIC,
          3500, DAMAGE_TYPE_SLASHING, 6, 0, WEAPON_FAMILY_MEDIUM_BLADE, SIZE_MEDIUM, MATERIAL_STEEL,
          HANDLE_TYPE_HILT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_DWARVEN_WAR_AXE, "dwarven war axe", 1, 10, 0, 3,
          WEAPON_FLAG_EXOTIC, 3000, DAMAGE_TYPE_SLASHING, 8, 0, WEAPON_FAMILY_AXE, SIZE_MEDIUM,
          MATERIAL_STEEL, HANDLE_TYPE_HANDLE, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_WHIP, "whip", 1, 3, 0, 2, WEAPON_FLAG_EXOTIC | WEAPON_FLAG_REACH
          | WEAPON_FLAG_DISARM | WEAPON_FLAG_TRIP | WEAPON_FLAG_BALANCED, 100, DAMAGE_TYPE_SLASHING, 2, 0, WEAPON_FAMILY_WHIP,
          SIZE_MEDIUM, MATERIAL_LEATHER, HANDLE_TYPE_HANDLE, HEAD_TYPE_CORD);
  setweapon(WEAPON_TYPE_SPIKED_CHAIN, "spiked chain", 2, 4, 0, 2, WEAPON_FLAG_EXOTIC |
          WEAPON_FLAG_REACH | WEAPON_FLAG_DISARM | WEAPON_FLAG_TRIP | WEAPON_FLAG_BALANCED, 2500, DAMAGE_TYPE_PIERCING, 10, 0,
          WEAPON_FAMILY_WHIP, SIZE_LARGE, MATERIAL_STEEL, HANDLE_TYPE_GRIP, HEAD_TYPE_CHAIN);
  setweapon(WEAPON_TYPE_DOUBLE_AXE, "double-headed axe", 1, 8, 0, 3, WEAPON_FLAG_EXOTIC |
          WEAPON_FLAG_DOUBLE, 6500, DAMAGE_TYPE_SLASHING, 15, 0, WEAPON_FAMILY_DOUBLE, SIZE_LARGE,
          MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_DIRE_FLAIL, "dire flail", 1, 8, 0, 2, WEAPON_FLAG_EXOTIC |
          WEAPON_FLAG_DOUBLE, 9000, DAMAGE_TYPE_BLUDGEONING, 10, 0, WEAPON_FAMILY_DOUBLE, SIZE_LARGE,
          MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_HEAD);
  setweapon(WEAPON_TYPE_HOOKED_HAMMER, "hooked hammer", 1, 6, 0, 4, WEAPON_FLAG_EXOTIC |
          WEAPON_FLAG_DOUBLE, 2000, DAMAGE_TYPE_PIERCING | DAMAGE_TYPE_BLUDGEONING, 6, 0,
          WEAPON_FAMILY_DOUBLE, SIZE_LARGE, MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_HEAD);
  /*	(weapon num, name, numDamDice, sizeDamDice, critRange, critMult, weapon flags, cost, damageType, weight, range, weaponFamily, Size, material, handle, head) */
  setweapon(WEAPON_TYPE_2_BLADED_SWORD, "two-bladed sword", 1, 8, 1, 2,
          WEAPON_FLAG_EXOTIC | WEAPON_FLAG_DOUBLE, 10000, DAMAGE_TYPE_SLASHING, 10, 0,
          WEAPON_FAMILY_DOUBLE, SIZE_LARGE, MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_DWARVEN_URGOSH, "dwarven urgosh", 1, 7, 0, 3, WEAPON_FLAG_EXOTIC
          | WEAPON_FLAG_DOUBLE, 5000, DAMAGE_TYPE_PIERCING | DAMAGE_TYPE_SLASHING, 12, 0,
          WEAPON_FAMILY_DOUBLE, SIZE_LARGE, MATERIAL_STEEL, HANDLE_TYPE_SHAFT, HEAD_TYPE_BLADE);
  setweapon(WEAPON_TYPE_HAND_CROSSBOW, "hand crossbow", 1, 4, 1, 2, WEAPON_FLAG_EXOTIC |
          WEAPON_FLAG_RANGED, 10000, DAMAGE_TYPE_PIERCING, 2, 30, WEAPON_FAMILY_CROSSBOW, SIZE_SMALL,
          MATERIAL_WOOD, HANDLE_TYPE_HANDLE, HEAD_TYPE_BOW);
  setweapon(WEAPON_TYPE_HEAVY_REP_XBOW, "heavy repeating crossbow", 1, 10, 1, 2,
          WEAPON_FLAG_EXOTIC | WEAPON_FLAG_RANGED | WEAPON_FLAG_REPEATING, 40000, DAMAGE_TYPE_PIERCING, 12, 120,
          WEAPON_FAMILY_CROSSBOW, SIZE_LARGE, MATERIAL_WOOD, HANDLE_TYPE_HANDLE, HEAD_TYPE_BOW);
  setweapon(WEAPON_TYPE_LIGHT_REP_XBOW, "light repeating crossbow", 1, 8, 1, 2,
          WEAPON_FLAG_EXOTIC | WEAPON_FLAG_RANGED, 25000, DAMAGE_TYPE_PIERCING, 6, 80,
          WEAPON_FAMILY_CROSSBOW, SIZE_MEDIUM, MATERIAL_WOOD, HANDLE_TYPE_HANDLE, HEAD_TYPE_BOW);
  setweapon(WEAPON_TYPE_BOLA, "bola", 1, 4, 0, 2, WEAPON_FLAG_EXOTIC | WEAPON_FLAG_THROWN
          | WEAPON_FLAG_TRIP, 500, DAMAGE_TYPE_BLUDGEONING, 2, 10, WEAPON_FAMILY_THROWN, SIZE_MEDIUM,
          MATERIAL_LEATHER, HANDLE_TYPE_GRIP, HEAD_TYPE_CORD);
  setweapon(WEAPON_TYPE_NET, "net", 1, 1, 0, 1, WEAPON_FLAG_EXOTIC | WEAPON_FLAG_THROWN |
          WEAPON_FLAG_ENTANGLE, 2000, DAMAGE_TYPE_BLUDGEONING, 6, 10, WEAPON_FAMILY_THROWN, SIZE_LARGE,
          MATERIAL_LEATHER, HANDLE_TYPE_GRIP, HEAD_TYPE_MESH);
  setweapon(WEAPON_TYPE_SHURIKEN, "shuriken", 1, 2, 0, 2, WEAPON_FLAG_EXOTIC |
          WEAPON_FLAG_THROWN, 20, DAMAGE_TYPE_PIERCING, 1, 10, WEAPON_FAMILY_MONK, SIZE_SMALL,
          MATERIAL_STEEL, HANDLE_TYPE_GRIP, HEAD_TYPE_BLADE);
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
int compute_gear_armor_type(struct char_data *ch) {
  int armor_type = ARMOR_TYPE_NONE, armor_compare = ARMOR_TYPE_NONE, i;
  struct obj_data *obj = NULL;

  for (i = 0; i < NUM_WEARS; i++) {
    obj = GET_EQ(ch, i);
    if (obj && GET_OBJ_TYPE(obj) == ITEM_ARMOR) {
      /* ok we have an armor piece... */
      armor_compare = armor_list[GET_OBJ_VAL(obj, 1)].armorType;
      if (armor_compare < ARMOR_TYPE_SHIELD && armor_compare > armor_type) {
        armor_type = armor_compare;
      }
    }
  }

  return armor_type;
}

int compute_gear_shield_type(struct char_data *ch) {
  int shield_type = ARMOR_TYPE_NONE;
  struct obj_data *obj = GET_EQ(ch, WEAR_SHIELD);

  if (obj) {
    shield_type = armor_list[GET_OBJ_VAL(obj, 1)].armorType;
    if (shield_type != ARMOR_TYPE_SHIELD && shield_type != ARMOR_TYPE_TOWER_SHIELD) {
      shield_type = ARMOR_TYPE_NONE;
    }
  }

  return shield_type;
}

/* enhancement bonus + material bonus */
int compute_gear_enhancement_bonus(struct char_data *ch) {
  struct obj_data *obj = NULL;
  int enhancement_bonus = 0;
  float counter = 0.0;
  float num_pieces = 0.0;

  /* we're going to check slot-by-slot */

  /* SPECIAL HANDLING FOR SHIELD */
  /* shield - gets full enchantment bonus, so _do not_ increment num_pieces */
  obj = GET_EQ(ch, WEAR_SHIELD);
  if (obj && GET_OBJ_TYPE(obj) == ITEM_ARMOR) {
    switch (GET_OBJ_MATERIAL(obj)) {
      case MATERIAL_ADAMANTINE:
      case MATERIAL_MITHRIL:
      case MATERIAL_DRAGONHIDE:
      case MATERIAL_DIAMOND:
      case MATERIAL_DARKWOOD:
        counter += 1.1;
        break;
    }
    counter += (float) GET_OBJ_VAL(obj, 4) * 1.01;
    /* DON'T increment num_pieces, should get full bang for buck on shields */
  }
  enhancement_bonus += counter;
  counter = 0.1; /* reset the counter for all other slots */
  /* end SPECIAL HANDLING FOR SHIELD */

  /* body */
  obj = GET_EQ(ch, WEAR_BODY);
  num_pieces += 0.99;
  if (obj && GET_OBJ_TYPE(obj) == ITEM_ARMOR) {
    switch (GET_OBJ_MATERIAL(obj)) {
      case MATERIAL_ADAMANTINE:
      case MATERIAL_MITHRIL:
      case MATERIAL_DRAGONHIDE:
      case MATERIAL_DIAMOND:
      case MATERIAL_DARKWOOD:
        counter += 1.1;
        break;
    }
    counter += (float) GET_OBJ_VAL(obj, 4) * 1.01;
  }

  /* head */
  obj = GET_EQ(ch, WEAR_HEAD);
  num_pieces += 0.99;
  if (obj && GET_OBJ_TYPE(obj) == ITEM_ARMOR) {
    switch (GET_OBJ_MATERIAL(obj)) {
      case MATERIAL_ADAMANTINE:
      case MATERIAL_MITHRIL:
      case MATERIAL_DRAGONHIDE:
      case MATERIAL_DIAMOND:
      case MATERIAL_DARKWOOD:
        counter += 1.1;
        break;
    }
    counter += (float) GET_OBJ_VAL(obj, 4) * 1.01;
  }

  /* legs */
  obj = GET_EQ(ch, WEAR_LEGS);
  num_pieces += 0.99;
  if (obj && GET_OBJ_TYPE(obj) == ITEM_ARMOR) {
    switch (GET_OBJ_MATERIAL(obj)) {
      case MATERIAL_ADAMANTINE:
      case MATERIAL_MITHRIL:
      case MATERIAL_DRAGONHIDE:
      case MATERIAL_DIAMOND:
      case MATERIAL_DARKWOOD:
        counter += 1.1;
        break;
    }
    counter += (float) GET_OBJ_VAL(obj, 4) * 1.01;
  }

  /* arms */
  obj = GET_EQ(ch, WEAR_ARMS);
  num_pieces += 0.99;
  if (obj && GET_OBJ_TYPE(obj) == ITEM_ARMOR) {
    switch (GET_OBJ_MATERIAL(obj)) {
      case MATERIAL_ADAMANTINE:
      case MATERIAL_MITHRIL:
      case MATERIAL_DRAGONHIDE:
      case MATERIAL_DIAMOND:
      case MATERIAL_DARKWOOD:
        counter += 1.1;
        break;
    }
    counter += (float) GET_OBJ_VAL(obj, 4) * 1.01;
  }

  enhancement_bonus += MAX(0, (int) (counter / num_pieces));

  return enhancement_bonus;
}

/* should return a percentage */
int compute_gear_spell_failure(struct char_data *ch) {
  int spell_failure = 0, i, count = 0;
  struct obj_data *obj = NULL;

  for (i = 0; i < NUM_WEARS; i++) {
    obj = GET_EQ(ch, i);
    if (obj && GET_OBJ_TYPE(obj) == ITEM_ARMOR &&
            (i == WEAR_BODY || i == WEAR_HEAD || i == WEAR_LEGS || i == WEAR_ARMS ||
            i == WEAR_SHIELD)) {
      if (i != WEAR_SHIELD) /* shield and armor combined increase spell failure chance */
        count++;
      /* ok we have an armor piece... */
      spell_failure += armor_list[GET_OBJ_VAL(obj, 1)].spellFail;
    }
  }

  if (count) {
    spell_failure = spell_failure / count;
  }

  /* 5% improvement in spell success with this feat */
  spell_failure -= HAS_FEAT(ch, FEAT_ARMORED_SPELLCASTING) * 5;

  if (spell_failure < 0)
    spell_failure = 0;
  if (spell_failure > 100)
    spell_failure = 100;

  return spell_failure;
}

/* for doing (usually) dexterity based tasks */
int compute_gear_armor_penalty(struct char_data *ch) {
  int armor_penalty = 0, i, count = 0;

  struct obj_data *obj = NULL;

  for (i = 0; i < NUM_WEARS; i++) {
    obj = GET_EQ(ch, i);
    if (obj && GET_OBJ_TYPE(obj) == ITEM_ARMOR &&
            (i == WEAR_BODY || i == WEAR_HEAD || i == WEAR_LEGS || i == WEAR_ARMS ||
            i == WEAR_SHIELD)) {
      count++;
      /* ok we have an armor piece... */
      armor_penalty += armor_list[GET_OBJ_VAL(obj, 1)].armorCheck;
    }
  }

  if (count) {
    armor_penalty = armor_penalty / count;
    armor_penalty += HAS_FEAT(ch, FEAT_ARMOR_TRAINING);
  }

  if (armor_penalty > 0)
    armor_penalty = 0;
  if (armor_penalty < -10)
    armor_penalty = -10;

  return armor_penalty;
}

/* maximum dexterity bonus, returns 99 for no limit */
int compute_gear_max_dex(struct char_data *ch) {
  int dexterity_cap = 0;
  int armor_max_dexterity = 0, i, count = 0;

  if (IS_WILDSHAPED(ch) || IS_MORPHED(ch))/* not wearing armor, no limit to dexterity */
    return 99;

  struct obj_data *obj = NULL;

  for (i = 0; i < NUM_WEARS; i++) {
    obj = GET_EQ(ch, i);
    if (obj && GET_OBJ_TYPE(obj) == ITEM_ARMOR &&
            (i == WEAR_BODY || i == WEAR_HEAD || i == WEAR_LEGS || i == WEAR_ARMS ||
            i == WEAR_SHIELD)) {
      /* ok we have an armor piece... */
      armor_max_dexterity = armor_list[GET_OBJ_VAL(obj, 1)].dexBonus;
      if (armor_max_dexterity == 99) /* no limit to dexterity */
        count--;
      else
        dexterity_cap += armor_max_dexterity;
      count++;
    }
  }

  if (count > 0) {
    dexterity_cap = dexterity_cap / count;
    dexterity_cap += HAS_FEAT(ch, FEAT_ARMOR_TRAINING);
  } else /* not wearing armor */
    dexterity_cap = 99;


  if (dexterity_cap < 0)
    dexterity_cap = 0;

  return dexterity_cap;
}

int is_proficient_with_shield(struct char_data *ch) {
  struct obj_data *shield = GET_EQ(ch, WEAR_SHIELD);

  if (IS_NPC(ch))
    return FALSE;

  if (!shield)
    return TRUE;

  switch (GET_ARMOR_TYPE_PROF(shield)) {
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

int is_proficient_with_body_armor(struct char_data *ch) {
  struct obj_data *body_armor = GET_EQ(ch, WEAR_BODY);

  if (IS_NPC(ch))
    return FALSE;

  if (!body_armor)
    return TRUE;

  switch (GET_ARMOR_TYPE_PROF(body_armor)) {
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

int is_proficient_with_helm(struct char_data *ch) {
  struct obj_data *head_armor = GET_EQ(ch, WEAR_HEAD);

  if (IS_NPC(ch))
    return FALSE;

  if (!head_armor)
    return TRUE;

  switch (GET_ARMOR_TYPE_PROF(head_armor)) {
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

int is_proficient_with_sleeves(struct char_data *ch) {
  struct obj_data *arm_armor = GET_EQ(ch, WEAR_ARMS);

  if (IS_NPC(ch))
    return FALSE;

  if (!arm_armor)
    return TRUE;

  switch (GET_ARMOR_TYPE_PROF(arm_armor)) {
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

int is_proficient_with_leggings(struct char_data *ch) {
  struct obj_data *leg_armor = GET_EQ(ch, WEAR_LEGS);

  if (IS_NPC(ch))
    return FALSE;

  if (!leg_armor)
    return TRUE;

  switch (GET_ARMOR_TYPE_PROF(leg_armor)) {
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
int is_proficient_with_armor(struct char_data *ch) {
  if (
          is_proficient_with_leggings(ch) &&
          is_proficient_with_sleeves(ch) &&
          is_proficient_with_helm(ch) &&
          is_proficient_with_body_armor(ch) &&
          is_proficient_with_shield(ch)
          )
    return TRUE;

  return FALSE;
}

void setarmor(int type, char *name, int armorType, int cost, int armorBonus,
        int dexBonus, int armorCheck, int spellFail, int thirtyFoot,
        int twentyFoot, int weight, int material, int wear) {
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
}

void initialize_armor(int type) {
  armor_list[type].name = "unused armor";
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

void load_armor(void) {
  int i = 0;

  for (i = 0; i < NUM_SPEC_ARMOR_TYPES; i++)
    initialize_armor(i);

  /* (armor, name, type,
   *    cost, AC, dexBonusCap, armorCheckPenalty, spellFailChance, (move)30ft, (move)20ft,
   *    weight, material, wear) */
  /* UNARMORED */
  setarmor(SPEC_ARMOR_TYPE_CLOTHING, "robe", ARMOR_TYPE_NONE,
          10, 0, 99, 0, 0, 30, 20,
          1, MATERIAL_COTTON, ITEM_WEAR_BODY);
  setarmor(SPEC_ARMOR_TYPE_CLOTHING_HEAD, "hood", ARMOR_TYPE_NONE,
          10, 0, 99, 0, 0, 30, 20,
          1, MATERIAL_COTTON, ITEM_WEAR_HEAD);
  setarmor(SPEC_ARMOR_TYPE_CLOTHING_ARMS, "sleeves", ARMOR_TYPE_NONE,
          10, 0, 99, 0, 0, 30, 20,
          1, MATERIAL_COTTON, ITEM_WEAR_ARMS);
  setarmor(SPEC_ARMOR_TYPE_CLOTHING_LEGS, "pants", ARMOR_TYPE_NONE,
          10, 0, 99, 0, 0, 30, 20,
          1, MATERIAL_COTTON, ITEM_WEAR_LEGS);

  /* LIGHT ARMOR ********************/
  setarmor(SPEC_ARMOR_TYPE_PADDED, "padded body armor", ARMOR_TYPE_LIGHT,
          50, 9, 8, 0, 5, 30, 20,
          7, MATERIAL_COTTON, ITEM_WEAR_BODY);
  setarmor(SPEC_ARMOR_TYPE_PADDED_HEAD, "padded armor helm", ARMOR_TYPE_LIGHT,
          50, 1, 8, 0, 5, 30, 20,
          1, MATERIAL_COTTON, ITEM_WEAR_HEAD);
  setarmor(SPEC_ARMOR_TYPE_PADDED_ARMS, "padded armor sleeves", ARMOR_TYPE_LIGHT,
          50, 1, 8, 0, 5, 30, 20,
          1, MATERIAL_COTTON, ITEM_WEAR_ARMS);
  setarmor(SPEC_ARMOR_TYPE_PADDED_LEGS, "padded armor leggings", ARMOR_TYPE_LIGHT,
          50, 1, 8, 0, 5, 30, 20,
          1, MATERIAL_COTTON, ITEM_WEAR_LEGS);

  setarmor(SPEC_ARMOR_TYPE_LEATHER, "leather armor", ARMOR_TYPE_LIGHT,
          100, 14, 6, 0, 10, 30, 20,
          9, MATERIAL_LEATHER, ITEM_WEAR_BODY);
  setarmor(SPEC_ARMOR_TYPE_LEATHER_HEAD, "leather helm", ARMOR_TYPE_LIGHT,
          100, 4, 6, 0, 10, 30, 20,
          2, MATERIAL_LEATHER, ITEM_WEAR_HEAD);
  setarmor(SPEC_ARMOR_TYPE_LEATHER_ARMS, "leather sleeves", ARMOR_TYPE_LIGHT,
          100, 4, 6, 0, 10, 30, 20,
          2, MATERIAL_LEATHER, ITEM_WEAR_ARMS);
  setarmor(SPEC_ARMOR_TYPE_LEATHER_LEGS, "leather leggings", ARMOR_TYPE_LIGHT,
          100, 4, 6, 0, 10, 30, 20,
          2, MATERIAL_LEATHER, ITEM_WEAR_LEGS);

  /* (armor, name, type,
   *    cost, AC, dexBonusCap, armorCheckPenalty, spellFailChance, (move)30ft, (move)20ft,
   *    weight, material, wear) */
  setarmor(SPEC_ARMOR_TYPE_STUDDED_LEATHER, "studded leather armor", ARMOR_TYPE_LIGHT,
          250, 20, 5, -1, 15, 30, 20,
          11, MATERIAL_LEATHER, ITEM_WEAR_BODY);
  setarmor(SPEC_ARMOR_TYPE_STUDDED_LEATHER_HEAD, "studded leather helm", ARMOR_TYPE_LIGHT,
          250, 6, 5, -1, 15, 30, 20,
          3, MATERIAL_LEATHER, ITEM_WEAR_HEAD);
  setarmor(SPEC_ARMOR_TYPE_STUDDED_LEATHER_ARMS, "studded leather sleeves", ARMOR_TYPE_LIGHT,
          250, 6, 5, -1, 15, 30, 20,
          3, MATERIAL_LEATHER, ITEM_WEAR_ARMS);
  setarmor(SPEC_ARMOR_TYPE_STUDDED_LEATHER_LEGS, "studded leather leggings", ARMOR_TYPE_LIGHT,
          250, 6, 5, -1, 15, 30, 20,
          3, MATERIAL_LEATHER, ITEM_WEAR_LEGS);

  setarmor(SPEC_ARMOR_TYPE_LIGHT_CHAIN, "light chainmail armor", ARMOR_TYPE_LIGHT,
          1000, 24, 4, -2, 20, 30, 20,
          13, MATERIAL_STEEL, ITEM_WEAR_BODY);
  setarmor(SPEC_ARMOR_TYPE_LIGHT_CHAIN_HEAD, "light chainmail helm", ARMOR_TYPE_LIGHT,
          1000, 9, 4, -2, 20, 30, 20,
          4, MATERIAL_STEEL, ITEM_WEAR_HEAD);
  setarmor(SPEC_ARMOR_TYPE_LIGHT_CHAIN_ARMS, "light chainmail sleeves", ARMOR_TYPE_LIGHT,
          1000, 9, 4, -2, 20, 30, 20,
          4, MATERIAL_STEEL, ITEM_WEAR_ARMS);
  setarmor(SPEC_ARMOR_TYPE_LIGHT_CHAIN_LEGS, "light chainmail leggings", ARMOR_TYPE_LIGHT,
          1000, 9, 4, -2, 20, 30, 20,
          4, MATERIAL_STEEL, ITEM_WEAR_LEGS);

  /******************* MEDIUM ARMOR *******************************************/

  setarmor(SPEC_ARMOR_TYPE_HIDE, "hide armor", ARMOR_TYPE_MEDIUM,
          150, 26, 4, -3, 20, 20, 15,
          13, MATERIAL_LEATHER, ITEM_WEAR_BODY);
  setarmor(SPEC_ARMOR_TYPE_HIDE_HEAD, "hide helm", ARMOR_TYPE_MEDIUM,
          150, 10, 4, -3, 20, 20, 15,
          4, MATERIAL_LEATHER, ITEM_WEAR_HEAD);
  setarmor(SPEC_ARMOR_TYPE_HIDE_ARMS, "hide sleeves", ARMOR_TYPE_MEDIUM,
          150, 10, 4, -3, 20, 20, 15,
          4, MATERIAL_LEATHER, ITEM_WEAR_ARMS);
  setarmor(SPEC_ARMOR_TYPE_HIDE_LEGS, "hide leggings", ARMOR_TYPE_MEDIUM,
          150, 10, 4, -3, 20, 20, 15,
          4, MATERIAL_LEATHER, ITEM_WEAR_LEGS);

  /* (armor, name, type,
   *    cost, AC, dexBonusCap, armorCheckPenalty, spellFailChance, (move)30ft, (move)20ft,
   *    weight, material, wear) */
  setarmor(SPEC_ARMOR_TYPE_SCALE, "scale armor", ARMOR_TYPE_MEDIUM,
          500, 32, 3, -4, 25, 20, 15,
          15, MATERIAL_STEEL, ITEM_WEAR_BODY);
  setarmor(SPEC_ARMOR_TYPE_SCALE_HEAD, "scale helm", ARMOR_TYPE_MEDIUM,
          500, 12, 3, -4, 25, 20, 15,
          5, MATERIAL_STEEL, ITEM_WEAR_HEAD);
  setarmor(SPEC_ARMOR_TYPE_SCALE_ARMS, "scale sleeves", ARMOR_TYPE_MEDIUM,
          500, 12, 3, -4, 25, 20, 15,
          5, MATERIAL_STEEL, ITEM_WEAR_ARMS);
  setarmor(SPEC_ARMOR_TYPE_SCALE_LEGS, "scale leggings", ARMOR_TYPE_MEDIUM,
          500, 12, 3, -4, 25, 20, 15,
          5, MATERIAL_STEEL, ITEM_WEAR_LEGS);

  setarmor(SPEC_ARMOR_TYPE_CHAINMAIL, "chainmail armor", ARMOR_TYPE_MEDIUM,
          1500, 37, 2, -5, 30, 20, 15,
          27, MATERIAL_STEEL, ITEM_WEAR_BODY);
  setarmor(SPEC_ARMOR_TYPE_CHAINMAIL_HEAD, "chainmail helm", ARMOR_TYPE_MEDIUM,
          1500, 15, 2, -5, 30, 20, 15,
          11, MATERIAL_STEEL, ITEM_WEAR_HEAD);
  /* duplicate item */
  setarmor(SPEC_ARMOR_TYPE_CHAIN_HEAD, "chainmail helm", ARMOR_TYPE_MEDIUM,
          1500, 15, 2, -5, 30, 20, 15,
          11, MATERIAL_STEEL, ITEM_WEAR_HEAD);
  setarmor(SPEC_ARMOR_TYPE_CHAINMAIL_ARMS, "chainmail sleeves", ARMOR_TYPE_MEDIUM,
          1500, 15, 2, -5, 30, 20, 15,
          11, MATERIAL_STEEL, ITEM_WEAR_ARMS);
  setarmor(SPEC_ARMOR_TYPE_CHAINMAIL_LEGS, "chainmail leggings", ARMOR_TYPE_MEDIUM,
          1500, 15, 2, -5, 30, 20, 15,
          11, MATERIAL_STEEL, ITEM_WEAR_LEGS);

  setarmor(SPEC_ARMOR_TYPE_PIECEMEAL, "piecemeal armor", ARMOR_TYPE_MEDIUM,
          2000, 35, 3, -4, 25, 20, 15,
          19, MATERIAL_STEEL, ITEM_WEAR_BODY);
  setarmor(SPEC_ARMOR_TYPE_PIECEMEAL_HEAD, "piecemeal helm", ARMOR_TYPE_MEDIUM,
          2000, 14, 3, -4, 25, 20, 15,
          7, MATERIAL_STEEL, ITEM_WEAR_HEAD);
  setarmor(SPEC_ARMOR_TYPE_PIECEMEAL_ARMS, "piecemeal sleeves", ARMOR_TYPE_MEDIUM,
          2000, 14, 3, -4, 25, 20, 15,
          7, MATERIAL_STEEL, ITEM_WEAR_ARMS);
  setarmor(SPEC_ARMOR_TYPE_PIECEMEAL_LEGS, "piecemeal leggings", ARMOR_TYPE_MEDIUM,
          2000, 14, 3, -4, 25, 20, 15,
          7, MATERIAL_STEEL, ITEM_WEAR_LEGS);

  /******************* HEAVY ARMOR *******************************************/

  /* (armor, name, type,
   *    cost, AC, dexBonusCap, armorCheckPenalty, spellFailChance, (move)30ft, (move)20ft,
   *    weight, material, wear) */
  setarmor(SPEC_ARMOR_TYPE_SPLINT, "splint mail armor", ARMOR_TYPE_HEAVY,
          2000, 46, 0, -7, 40, 20, 15,
          21, MATERIAL_STEEL, ITEM_WEAR_BODY);
  setarmor(SPEC_ARMOR_TYPE_SPLINT_HEAD, "splint mail helm", ARMOR_TYPE_HEAVY,
          2000, 19, 0, -7, 40, 20, 15,
          8, MATERIAL_STEEL, ITEM_WEAR_HEAD);
  setarmor(SPEC_ARMOR_TYPE_SPLINT_ARMS, "splint mail sleeves", ARMOR_TYPE_HEAVY,
          2000, 19, 0, -7, 40, 20, 15,
          8, MATERIAL_STEEL, ITEM_WEAR_ARMS);
  setarmor(SPEC_ARMOR_TYPE_SPLINT_LEGS, "splint mail leggings", ARMOR_TYPE_HEAVY,
          2000, 19, 0, -7, 40, 20, 15,
          8, MATERIAL_STEEL, ITEM_WEAR_LEGS);

  setarmor(SPEC_ARMOR_TYPE_BANDED, "banded mail armor", ARMOR_TYPE_HEAVY,
          2500, 47, 1, -6, 35, 20, 15,
          17, MATERIAL_STEEL, ITEM_WEAR_BODY);
  setarmor(SPEC_ARMOR_TYPE_BANDED_HEAD, "banded mail helm", ARMOR_TYPE_HEAVY,
          2500, 20, 1, -6, 35, 20, 15,
          6, MATERIAL_STEEL, ITEM_WEAR_HEAD);
  setarmor(SPEC_ARMOR_TYPE_BANDED_ARMS, "banded mail sleeves", ARMOR_TYPE_HEAVY,
          2500, 20, 1, -6, 35, 20, 15,
          6, MATERIAL_STEEL, ITEM_WEAR_ARMS);
  setarmor(SPEC_ARMOR_TYPE_BANDED_LEGS, "banded mail leggings", ARMOR_TYPE_HEAVY,
          2500, 20, 1, -6, 35, 20, 15,
          6, MATERIAL_STEEL, ITEM_WEAR_LEGS);

  setarmor(SPEC_ARMOR_TYPE_HALF_PLATE, "half plate armor", ARMOR_TYPE_HEAVY,
          6000, 52, 1, -6, 40, 20, 15,
          23, MATERIAL_STEEL, ITEM_WEAR_BODY);
  setarmor(SPEC_ARMOR_TYPE_HALF_PLATE_HEAD, "half plate helm", ARMOR_TYPE_HEAVY,
          6000, 22, 1, -6, 40, 20, 15,
          9, MATERIAL_STEEL, ITEM_WEAR_HEAD);
  setarmor(SPEC_ARMOR_TYPE_HALF_PLATE_ARMS, "half plate sleeves", ARMOR_TYPE_HEAVY,
          6000, 22, 1, -6, 40, 20, 15,
          9, MATERIAL_STEEL, ITEM_WEAR_ARMS);
  setarmor(SPEC_ARMOR_TYPE_HALF_PLATE_LEGS, "half plate leggings", ARMOR_TYPE_HEAVY,
          6000, 22, 1, -6, 40, 20, 15,
          9, MATERIAL_STEEL, ITEM_WEAR_LEGS);

  /* (armor, name, type,
   *    cost, AC, dexBonusCap, armorCheckPenalty, spellFailChance, (move)30ft, (move)20ft,
   *    weight, material, wear) */
  setarmor(SPEC_ARMOR_TYPE_FULL_PLATE, "full plate armor", ARMOR_TYPE_HEAVY,
          15000, 60, 1, -6, 35, 20, 15,
          23, MATERIAL_STEEL, ITEM_WEAR_BODY);
  setarmor(SPEC_ARMOR_TYPE_FULL_PLATE_HEAD, "full plate helm", ARMOR_TYPE_HEAVY,
          15000, 25, 1, -6, 35, 20, 15,
          9, MATERIAL_STEEL, ITEM_WEAR_HEAD);
  setarmor(SPEC_ARMOR_TYPE_FULL_PLATE_ARMS, "full plate sleeves", ARMOR_TYPE_HEAVY,
          15000, 25, 1, -6, 35, 20, 15,
          9, MATERIAL_STEEL, ITEM_WEAR_ARMS);
  setarmor(SPEC_ARMOR_TYPE_FULL_PLATE_LEGS, "full plate leggings", ARMOR_TYPE_HEAVY,
          15000, 25, 1, -6, 35, 20, 15,
          9, MATERIAL_STEEL, ITEM_WEAR_LEGS);

  /* (armor, name, type,
   *    cost, AC, dexBonusCap, armorCheckPenalty, spellFailChance, (move)30ft, (move)20ft,
   *    weight, material, wear) */
  setarmor(SPEC_ARMOR_TYPE_BUCKLER, "buckler shield", ARMOR_TYPE_SHIELD,
          150, 10, 99, -1, 5, 999, 999,
          5, MATERIAL_WOOD, ITEM_WEAR_SHIELD);
  setarmor(SPEC_ARMOR_TYPE_SMALL_SHIELD, "light shield", ARMOR_TYPE_SHIELD,
          90, 15, 99, -1, 5, 999, 999,
          6, MATERIAL_WOOD, ITEM_WEAR_SHIELD);
  setarmor(SPEC_ARMOR_TYPE_LARGE_SHIELD, "heavy shield", ARMOR_TYPE_SHIELD,
          200, 20, 99, -2, 15, 999, 999,
          13, MATERIAL_WOOD, ITEM_WEAR_SHIELD);
  setarmor(SPEC_ARMOR_TYPE_TOWER_SHIELD, "tower shield", ARMOR_TYPE_TOWER_SHIELD,
          300, 40, 2, -10, 50, 999, 999,
          45, MATERIAL_WOOD, ITEM_WEAR_SHIELD);
}

/******* special mixed checks (such as monk) */

bool is_bare_handed(struct char_data *ch) {
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
bool monk_gear_ok(struct char_data *ch) {
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
ACMD(do_weaponlist) {
  int type = 0;
  char buf[MAX_STRING_LENGTH];
  char buf2[100];
  char buf3[100];
  size_t len = 0;
  int crit_multi = 0;

  for (type = 1; type < NUM_WEAPON_TYPES; type++) {

    /* have to do some calculations beforehand */
    switch (weapon_list[type].critMult) {
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
    sprintbit(weapon_list[type].weaponFlags, weapon_flags, buf2, sizeof (buf2));
    sprintbit(weapon_list[type].damageTypes, weapon_damage_types, buf3, sizeof (buf3));

    len += snprintf(buf + len, sizeof (buf) - len,
            "\tW%s\tn, Dam: %dd%d, Threat: %d, Crit-Multi: %d, Flags: %s, Cost: %d, "
            "Dam-Types: %s, Weight: %d, Range: %d, Family: %s, Size: %s, Material: %s, "
            "Handle: %s, Head: %s.\r\n",
            weapon_list[type].name, weapon_list[type].numDice, weapon_list[type].diceSize,
            (20 - weapon_list[type].critRange), crit_multi, buf2, weapon_list[type].cost,
            buf3, weapon_list[type].weight, weapon_list[type].range,
            weapon_family[weapon_list[type].weaponFamily],
            sizes[weapon_list[type].size], material_name[weapon_list[type].material],
            weapon_handle_types[weapon_list[type].handle_type],
            weapon_head_types[weapon_list[type].head_type]
            );

  }
  page_string(ch->desc, buf, 1);
}

/* list all the weapon defines in-game */
ACMD(do_armorlist) {
  int i = 0;
  char buf[MAX_STRING_LENGTH];
  size_t len = 0;

  for (i = 1; i < NUM_SPEC_ARMOR_TYPES; i++) {
    len += snprintf(buf + len, sizeof (buf) - len, "\tW%s\tn, Type: %s, Cost: %d, "
            "AC: %.1f, Max Dex: %d, Armor Penalty: %d, Spell Fail: %d, Weight: %d, "
            "Material: %s\r\n",
            armor_list[i].name, armor_type[armor_list[i].armorType],
            armor_list[i].cost, (float) armor_list[i].armorBonus / 10.0,
            armor_list[i].dexBonus, armor_list[i].armorCheck, armor_list[i].spellFail,
            armor_list[i].weight, material_name[armor_list[i].material]
            );
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