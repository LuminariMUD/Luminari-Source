/* *************************************************************************
 *   File: spec_abilities.c                            Part of LuminariMUD *
 *  Usage: Code file for special abilities for weapons, armor and          *
 *         shields.                                                        *
 * Author: Ornir                                                           *
 ***************************************************************************
 *                                                                         *
 * In d20/Dungeons and Dragons, special abilities are what make magic      *
 * items -magical-.  These abilities, being wreathed in fire, exploding    *
 * with frost on a critical hit etc. are part of what defineds D&D.        *
 *                                                                         *
 * In order to implement these thing in LuminariMUD, some additions to the *
 * stock object model have been made (in structs.h).  These changes allow  *
 * the addition of any number of the defined special abilities to be added *
 * to the weapon, armor or shield in addition to any APPLY_ values that    *
 * the object has.  Additionally, an activation method must be defined.    *
 *                                                                         *
 * The code is defined similarly to the spells and commands in stock code, *
 * in that macros and an array of structures are used to define new        *
 * special abilities.                                                      *
 ***************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "fight.h"
#include "comm.h"
#include "structs.h"
#include "dg_event.h"
#include "spells.h"
#include "spec_abilities.h"

struct special_ability_info_type weapon_special_ability_info[NUM_WEAPON_SPECABS];
struct special_ability_info_type armor_special_ability_info[NUM_ARMOR_SPECABS];

const char *unused_specabname = "!UNUSED!"; /* So we can get &unused_specabname */

const char *activation_methods[NUM_ACTIVATION_METHODS + 1] = { "None", 
                                                               "On Wear", 
                                                               "On Use", 
                                                               "Command Word",
                                                               "On Hit", 
                                                               "On Crit", 
                                                               "\n" };

/* Procedures for loading and managing the special abilities on boot. */
static void add_weapon_special_ability(int specab, const char *name, int level, int minpos, int targets, int violent, int time, int school, int cost, SPECAB_PROC_DEF(specab_proc)) {
  weapon_special_ability_info[specab].level = level;
  weapon_special_ability_info[specab].min_position = minpos;
  weapon_special_ability_info[specab].targets = targets;
  weapon_special_ability_info[specab].violent = violent;
  weapon_special_ability_info[specab].name = name;
  weapon_special_ability_info[specab].time = time;
  weapon_special_ability_info[specab].school = school;
  weapon_special_ability_info[specab].cost = cost;
  weapon_special_ability_info[specab].special_ability_proc = specab_proc;
}

static void add_armor_special_ability(int specab, const char *name, int level, int minpos, int targets, int violent, int time, int school, int cost, SPECAB_PROC_DEF(specab_proc)) {
  armor_special_ability_info[specab].level = level;
  armor_special_ability_info[specab].min_position = minpos;
  armor_special_ability_info[specab].targets = targets;
  armor_special_ability_info[specab].violent = violent;
  armor_special_ability_info[specab].name = name;
  armor_special_ability_info[specab].time = time;
  armor_special_ability_info[specab].school = school;
  armor_special_ability_info[specab].cost = cost;
  armor_special_ability_info[specab].special_ability_proc = specab_proc;
  
}

static void add_unused_weapon_special_ability(int specab) {
  weapon_special_ability_info[specab].level = 0;
  weapon_special_ability_info[specab].min_position = POS_DEAD;
  weapon_special_ability_info[specab].targets = 0;
  weapon_special_ability_info[specab].violent = 0;
  weapon_special_ability_info[specab].name = unused_specabname;
  weapon_special_ability_info[specab].time = 0;
  weapon_special_ability_info[specab].school = NOSCHOOL;
  weapon_special_ability_info[specab].cost = 0;
  weapon_special_ability_info[specab].special_ability_proc = NULL;
}

static void add_unused_armor_special_ability(int specab) {
  armor_special_ability_info[specab].level = 0; 
  armor_special_ability_info[specab].min_position = POS_DEAD;
  armor_special_ability_info[specab].targets = 0;
  armor_special_ability_info[specab].violent = 0;
  armor_special_ability_info[specab].name = unused_specabname;
  armor_special_ability_info[specab].time = 0;
  armor_special_ability_info[specab].school = NOSCHOOL; 
  armor_special_ability_info[specab].cost = 0;
  armor_special_ability_info[specab].special_ability_proc = NULL;

}

/**  (Targeting re-used from spells.h)
 **  TAR_IGNORE    : IGNORE TARGET.
 **  TAR_CHAR_ROOM : PC/NPC in room.
 **  TAR_CHAR_WORLD: PC/NPC in world.
 **  TAR_FIGHT_SELF: If fighting, and no argument, select tar_char as self.
 **  TAR_FIGHT_VICT: If fighting, and no argument, select tar_char as victim (fighting).
 **  TAR_SELF_ONLY : If no argument, select self, if argument check that it IS self.
 **  TAR_NOT_SELF  : Target is anyone else besides self.
 **  TAR_OBJ_INV   : Object in inventory.
 **  TAR_OBJ_ROOM  : Object in room.
 **  TAR_OBJ_WORLD : Object in world.
 **  TAR_OBJ_EQUIP : Object held.
 **/


void initialize_special_abilities(void) {
  int i;

  /* Initialize all abilities to UNUSED. */
  /* Do not change the loops below. */
  for (i = 0; i < NUM_WEAPON_SPECABS; i++)
    add_unused_weapon_special_ability(i);
  for (i = 0; i < NUM_ARMOR_SPECABS; i++)
    add_unused_armor_special_ability(i);
  /* Do not change the loops above. */

  add_weapon_special_ability( WEAPON_SPECAB_ANARCHIC, "Anarchic", 7, POS_RECLINING,
    TAR_IGNORE, FALSE, 0, EVOCATION, 2, NULL);

  add_weapon_special_ability( WEAPON_SPECAB_AXIOMATIC, "Axiomatic", 7, POS_RECLINING,
    TAR_IGNORE, FALSE, 0, EVOCATION, 2, NULL);

  add_weapon_special_ability( WEAPON_SPECAB_BANE, "Bane", 8, POS_FIGHTING, 
    TAR_FIGHT_VICT, FALSE, 0, CONJURATION, 1, NULL);

  add_weapon_special_ability( WEAPON_SPECAB_BRILLIANT_ENERGY, "Brilliant Energy", 16, POS_RECLINING,
    TAR_IGNORE, FALSE, 0, TRANSMUTATION, 1, NULL);

  add_weapon_special_ability( WEAPON_SPECAB_DANCING, "Dancing", 15, POS_RECLINING,
    TAR_IGNORE, FALSE, 0, TRANSMUTATION, 1, NULL);

  add_weapon_special_ability( WEAPON_SPECAB_DEFENDING, "Defending", 8, POS_RECLINING,
    TAR_IGNORE, FALSE, 0, ABJURATION, 1, NULL);

  add_weapon_special_ability( WEAPON_SPECAB_DISRUPTION, "Disruption", 14,  POS_RECLINING,
    TAR_IGNORE, FALSE, 0, CONJURATION, 1, NULL);

  add_weapon_special_ability( WEAPON_SPECAB_DISTANCE, "Distance", 6, POS_RECLINING,
    TAR_IGNORE, FALSE, 0, DIVINATION, 1, NULL);

  add_weapon_special_ability( WEAPON_SPECAB_FLAMING, "Flaming", 10, POS_RECLINING,
    TAR_IGNORE, FALSE, 0, EVOCATION, 1, weapon_specab_flaming);

  add_weapon_special_ability( WEAPON_SPECAB_FLAMING_BURST, "Flaming Burst", 12, POS_RECLINING,
    TAR_IGNORE, FALSE, 0, EVOCATION, 1, weapon_specab_flaming_burst);

  add_weapon_special_ability( WEAPON_SPECAB_FROST, "Frost", 8, POS_RECLINING,
    TAR_IGNORE, FALSE, 0, EVOCATION, 1, NULL);

  add_weapon_special_ability( WEAPON_SPECAB_GHOST_TOUCH, "Ghost Touch", 9, POS_RECLINING,
    TAR_IGNORE, FALSE, 0, CONJURATION, 1, NULL);

}

/* Returns the number of activated abilites. */
int  process_weapon_abilities(struct obj_data  *weapon, /* The weapon to check for special abilities. */
                              struct char_data *ch,     /* The wielder of the weapon. */
                              struct char_data *victim, /* The target of the ability (either fighting or 
							 * specified explicitly. */ 
                              int    actmtd,            /* Activation method */
                              char   *cmdword)          /* Command word (optional, NULL if none. */

{
  int activated_abilities = 0;
  struct obj_special_ability *specab; /* struct for iterating through the object's abilities. */
  /* Run the 'callbacks' for each of the special abilities on weapon that match the activation method. */
  for( specab = weapon->special_abilities; specab != NULL; specab = specab->next) {
    /* So we have an ability, check the activation method. */
    if(IS_SET(specab->activation_method, actmtd)) { /* Match! */
      if(actmtd == ACTMTD_COMMAND_WORD) { /* check the command word */
        if(strcmp(specab->command_word, cmdword)) /* No Match */
          continue; /* Skip this ability, no match. */
      }
      if(weapon_special_ability_info[specab->ability].special_ability_proc == NULL) {
        log("SYSERR: PROCESS_WEAPON_ABILITIES: ability '%s' has no callback function!", weapon_special_ability_info[specab->ability].name);
        continue;
      }
      activated_abilities++;
      (*weapon_special_ability_info[specab->ability].special_ability_proc) (specab, weapon, ch, victim, actmtd);
      
    }
  }  
  
  return activated_abilities;
}

WEAPON_SPECIAL_ABILITY(weapon_specab_flaming) {
  /* 
   * level
   * weapon
   * ch
   * victim
   * obj
   */
  switch(actmtd) {
    case ACTMTD_COMMAND_WORD: /* User UTTERs the command word. */
    case ACTMTD_USE:          /* User USEs the item. */ 
      /* Activate the flaming ability.
       *  - Set the FLAMING bit on the weapon (this affects the display, 
       *    and is used to toggle the effect.)
       */
      if(OBJ_FLAGGED(weapon, ITEM_FLAMING)) {
        /* Flaming is on, turn it off. */
        send_to_char(ch,"The magical flames wreathing your weapon vanish.\r\n");
        act("The magical flames wreathing $n's $o vanish.", FALSE, ch, weapon, NULL, TO_ROOM);
  
        REMOVE_OBJ_FLAG(weapon, ITEM_FLAMING);
      } else {
        /* FLAME ON! */
        send_to_char(ch, "Magical flames spread down the length of your weapon!\r\n");
        act("Magical flames spread down the length of $n's $o!", FALSE, ch, weapon, NULL, TO_ROOM);
    
        SET_OBJ_FLAG(weapon, ITEM_FLAMING);
      }
      break;
    case ACTMTD_ON_HIT: /* Called whenever a weapon hits an enemy. */
      if(OBJ_FLAGGED(weapon, ITEM_FLAMING))  /* Burn 'em. */
        if (victim) {
          damage(ch, victim, dice(1, 6), TYPE_SPECAB_FLAMING, DAM_FIRE, FALSE);
        }
      break;
    case ACTMTD_ON_CRIT: /* Called whenever a weapon hits critically. */
    case ACTMTD_WEAR: /* Called whenever the item is worn. */
    default:
      /* Do nothing. */
      break;
  }
}

/* A weapon with Flaming burst functions as a flaming weapon, except on critical hits it
 * performs a flame burst for 1d10 extra damage. */
WEAPON_SPECIAL_ABILITY(weapon_specab_flaming_burst) {
  /* 
   * level
   * weapon
   * ch
   * victim
   * obj
   */
  switch(actmtd) {
    case ACTMTD_COMMAND_WORD: /* User UTTERs the command word. */
    case ACTMTD_USE:          /* User USEs the item. */
      /* Activate the flaming ability.
       *  - Set the FLAMING bit on the weapon (this affects the display, 
       *    and is used to toggle the effect.)
       */
      if(OBJ_FLAGGED(weapon, ITEM_FLAMING)) {
        /* Flaming is on, turn it off. */
        send_to_char(ch,"The magical flames wreathing your weapon vanish.\r\n");
        act("The magical flames wreathing $n's $o vanish.", FALSE, ch, weapon, NULL, TO_ROOM);

        REMOVE_OBJ_FLAG(weapon, ITEM_FLAMING);
      } else {
        /* FLAME ON! */
        send_to_char(ch, "Magical flames spread down the length of your weapon!\r\n");
        act("Magical flames spread down the length of $n's $o!", FALSE, ch, weapon, NULL, TO_ROOM);

        SET_OBJ_FLAG(weapon, ITEM_FLAMING);
      }
      break;
    case ACTMTD_ON_HIT: /* Called whenever a weapon hits an enemy. */
      if(OBJ_FLAGGED(weapon, ITEM_FLAMING))  /* Burn 'em. */
        if (victim) {
          /*send_to_char(ch, "\tr[spcab]\tn");*/
          damage(ch, victim, dice(1, 6), TYPE_SPECAB_FLAMING, DAM_FIRE, FALSE);
        }
      break;
    case ACTMTD_ON_CRIT: /* Called whenever a weapon hits critically. */
      /* We don't care if the flaming property is active, it bursts anyway! */
      /*
      act("\tRMagical fire \tYexplodes\tR from your $o, searing $N!", FALSE, ch, weapon, victim, TO_CHAR);
      act("\tRMagical fire \tYexplodes\tR from $n's $o, searing you!", FALSE, ch, weapon, victim, TO_VICT);
      act("\tRMagical fire \tYexplodes\tR from $n's $o, searing $N!", FALSE, ch, weapon, victim, TO_NOTVICT);
      */
      if(victim) {
        /* send_to_char(ch,"\tr[burst]\tn");*/
        damage(ch, victim, dice(1, 10), TYPE_SPECAB_FLAMING_BURST, DAM_FIRE, FALSE);
      }
      break;
    case ACTMTD_WEAR: /* Called whenever the item is worn. */
    default:
      /* Do nothing. */
      break;
  }
}

