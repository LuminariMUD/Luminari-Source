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
#include "comm.h"
#include "structs.h"
#include "dg_event.h"
#include "spells.h"
#include "spec_abilities.h"

struct special_ability_info_type weapon_special_ability_info[NUM_WEAPON_SPECABS];
struct special_ability_info_type armor_special_ability_info[NUM_ARMOR_SPECABS];

const char *unused_specabname = "!UNUSED!"; /* So we can get &unused_specabname */


/* Procedures for loading and managing the special abilities on boot. */
static void add_weapon_special_ability(int specab, const char *name, int level, int minpos, int targets, int violent, int time, int school, int cost) {
  weapon_special_ability_info[specab].level = level;
  weapon_special_ability_info[specab].min_position = minpos;
  weapon_special_ability_info[specab].targets = targets;
  weapon_special_ability_info[specab].violent = violent;
  weapon_special_ability_info[specab].name = name;
  weapon_special_ability_info[specab].time = time;
  weapon_special_ability_info[specab].school = school;
  weapon_special_ability_info[specab].cost = cost;
}

static void add_armor_special_ability(int specab, const char *name, int level, int minpos, int targets, int violent, int time, int school, int cost) {
  armor_special_ability_info[specab].level = level;
  armor_special_ability_info[specab].min_position = minpos;
  armor_special_ability_info[specab].targets = targets;
  armor_special_ability_info[specab].violent = violent;
  armor_special_ability_info[specab].name = name;
  armor_special_ability_info[specab].time = time;
  armor_special_ability_info[specab].school = school;
  armor_special_ability_info[specab].cost = cost;
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
}

static void add_unused_armor_special_ability(int specab) {
  armor_special_ability_info[specab].level = 0; 
  armor_special_ability_info[specab].min_position = POS_DEAD;
  armor_special_ability_info[specab].targets = 0;
  armor_special_ability_info[specab].violent = 0;
  armor_special_ability_info[specab].name = unused_specabname;
  armor_special_ability_info[specab].time = 0;
  armor_special_ability_info[specab].school = NOSCHOOL; 
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
    TAR_IGNORE, FALSE, 0, EVOCATION, 2);

  add_weapon_special_ability( WEAPON_SPECAB_FLAMING, "Flaming", 10, POS_RECLINING,
    TAR_IGNORE, FALSE, 0, EVOCATION, 1);

}

WEAPON_SPECIAL_ABILITY(weapon_specab_flaming) {
  /* 
   * level
   * weapon
   * ch
   * victim
   * obj
   */

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
}


