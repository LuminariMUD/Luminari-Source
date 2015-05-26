
/*****************************************************************************
 ** FEATS.C                                                                  **
 ** Source code for the Gates of Krynn Feats System.                         **
 ** Initial code by Paladine (Stephen Squires), Ported by Ornir to Luminari  **
 *****************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "constants.h"
#include "interpreter.h"
#include "dg_scripts.h"
#include "modify.h"
#include "feats.h"
#include "class.h"
#include "mud_event.h"
#include "assign_wpn_armor.h"
#include "domains_schools.h"

/* Local Functions */
/* Prerequisite definition procedures */
/* Global Variables and Structures */
int feat_sort_info[MAX_FEATS];
struct feat_info feat_list[NUM_FEATS];
/* END */

/* START */
void free_feats(void) {} /* Nothing to do right now.  What, for shutdown maybe? */

/* Helper function for t sort_feats function - not very robust and should not be reused.
 * SCARY pointer stuff! */
int compare_feats(const void *x, const void *y) {
  int a = *(const int *) x,
          b = *(const int *) y;

  return strcmp(feat_list[a].name, feat_list[b].name);
}
/* sort feats called at boot up */
void sort_feats(void) {
  int a;

  /* initialize array, avoiding reserved. */
  for (a = 1; a < NUM_FEATS; a++)
    feat_sort_info[a] = a;

  qsort(&feat_sort_info[1], NUM_FEATS, sizeof (int), compare_feats);
}

/* checks if the char has the feat either saved to file or in the process
 of acquiring it in study */
int has_feat(struct char_data *ch, int featnum) {
  if (ch->desc && LEVELUP(ch)) { /* check if he's in study mode */
    return (HAS_FEAT(ch, featnum) + LEVELUP(ch)->feats[featnum]);
  }
  /*
   * // this is for the option of allowing feats on items
    struct obj_data *obj;
    int i = 0, j = 0;

    for (j = 0; j < NUM_WEARS; j++) {
      if ((obj = GET_EQ(ch, j)) == NULL)
        continue;
      for (i = 0; i < 6; i++) {
        if (obj->affected[i].location == APPLY_FEAT && obj->affected[i].specific == featnum)
          return (has_feat(ch, featnum) + obj->affected[i].modifier);
      }
    }
   */
  return HAS_FEAT(ch, featnum);
}

/* checks if ch has feat (compare) as one of his/her combat feats (cfeat) */
bool has_combat_feat(struct char_data *ch, int cfeat, int compare) {
  if (ch->desc && LEVELUP(ch)) {
    if ((IS_SET_AR(LEVELUP(ch)->combat_feats[(cfeat)], (compare))))
      return TRUE;
  }

  if ((IS_SET_AR((ch)->char_specials.saved.combat_feats[(cfeat)], (compare))))
    return TRUE;

  return FALSE;
}

/* checks if ch has feat (compare) as one of his/her spells (school) feats (sfeat) */
/*
bool has_spell_feat(struct char_data *ch, int sfeat, int school) {
  if (ch->desc && LEVELUP(ch)) {
    if ((IS_SET_AR(LEVELUP(ch)->combat_feats[(sfeat)], (school))))
      return TRUE;
  }

  if ((IS_SET_AR((ch)->char_specials.saved.spell_feats[(sfeat)], (compare))))
    return TRUE;

  return FALSE;
}
*/

/* create/allocate memory for a pre-req struct, then assign the prereqs */
struct feat_prerequisite* create_prerequisite(int prereq_type, int val1, int val2, int val3) {
  struct feat_prerequisite *prereq = NULL;

  CREATE(prereq, struct feat_prerequisite, 1);
  prereq->prerequisite_type = prereq_type;
  prereq->values[0] = val1;
  prereq->values[1] = val2;
  prereq->values[2] = val3;

  return prereq;
}
/*  The following procedures are used to define feat prerequisites.
 *  These prerequisites are automatically checked, if they exist.
 *  Dynamically assigning prerequisites also allows us to create
 *  dynamic 'help' and easier to read presentations of feat lists. */
void feat_prereq_attribute(int featnum, int attribute, int value) {
  struct feat_prerequisite *prereq = NULL;
  char buf[80];

  const char* attribute_abbr[7] = {
    "None",
    "Str",
    "Dex",
    "Int",
    "Wis",
    "Con",
    "Cha"
  };

  prereq = create_prerequisite(FEAT_PREREQ_ATTRIBUTE, attribute, value, 0);

  /* Generate the description. */
  sprintf(buf, "%s : %d", attribute_abbr[attribute], value);
  prereq->description = strdup(buf);

  /*  Link it up. */
  prereq->next = feat_list[featnum].prerequisite_list;
  feat_list[featnum].prerequisite_list = prereq;
}
void feat_prereq_class_level(int featnum, int cl, int level) {
  struct feat_prerequisite *prereq = NULL;
  char buf[80];

  prereq = create_prerequisite(FEAT_PREREQ_CLASS_LEVEL, cl, level, 0);

  /* Generate the description. */
  sprintf(buf, "%s level %d", pc_class_types[cl], level);
  prereq->description = strdup(buf);

  /*   Link it up. */
  prereq->next = feat_list[featnum].prerequisite_list;
  feat_list[featnum].prerequisite_list = prereq;
}
void feat_prereq_feat(int featnum, int feat, int ranks) {
  struct feat_prerequisite *prereq = NULL;
  char buf[80];

  prereq = create_prerequisite(FEAT_PREREQ_FEAT, feat, ranks, 0);

  /* Generate the description. */
  if (ranks > 1)
    sprintf(buf, "%s (%d ranks)", feat_list[feat].name, ranks);
  else
    sprintf(buf, "%s", feat_list[feat].name);

  prereq->description = strdup(buf);

  /*   Link it up. */
  prereq->next = feat_list[featnum].prerequisite_list;
  feat_list[featnum].prerequisite_list = prereq;
}
void feat_prereq_cfeat(int featnum, int feat) {
  struct feat_prerequisite *prereq = NULL;
  char buf[80];

  prereq = create_prerequisite(FEAT_PREREQ_CFEAT, feat, 0, 0);

  sprintf(buf, "%s (in same weapon)", feat_list[feat].name);
  prereq->description = strdup(buf);

  /*   Link it up. */
  prereq->next = feat_list[featnum].prerequisite_list;
  feat_list[featnum].prerequisite_list = prereq;
}
void feat_prereq_ability(int featnum, int ability, int ranks) {
  struct feat_prerequisite *prereq = NULL;
  char buf[80];

  prereq = create_prerequisite(FEAT_PREREQ_ABILITY, ability, ranks, 0);

  sprintf(buf, "%d ranks in %s", ranks, ability_names[ability]);
  prereq->description = strdup(buf);

  /*   Link it up. */
  prereq->next = feat_list[featnum].prerequisite_list;
  feat_list[featnum].prerequisite_list = prereq;
}
void feat_prereq_spellcasting(int featnum, int casting_type, int prep_type, int circle) {
  struct feat_prerequisite *prereq = NULL;
  char buf[80];

  const char *casting_types[4] = {
    "None",
    "Arcane",
    "Divine",
    "Any"
  };

  const char *spell_preparation_types[4] = {
    "None",
    "Prepared",
    "Spontaneous",
    "Any"
  };

  prereq = create_prerequisite(FEAT_PREREQ_SPELLCASTING, casting_type, prep_type, circle);

  sprintf(buf, "Ability to cast %s %s spells", casting_types[casting_type], spell_preparation_types[prep_type]);
  prereq->description = strdup(buf);

  /*   Link it up. */
  prereq->next = feat_list[featnum].prerequisite_list;
  feat_list[featnum].prerequisite_list = prereq;
}
void feat_prereq_race(int featnum, int race) {
  struct feat_prerequisite *prereq = NULL;
  char buf[80];

  prereq = create_prerequisite(FEAT_PREREQ_RACE, race, 0, 0);

  sprintf(buf, "Race : %s", pc_race_types[race]);
  prereq->description = strdup(buf);

  /*   Link it up. */
  prereq->next = feat_list[featnum].prerequisite_list;
  feat_list[featnum].prerequisite_list = prereq;
}
void feat_prereq_bab(int featnum, int bab) {
  struct feat_prerequisite *prereq = NULL;
  char buf[80];

  prereq = create_prerequisite(FEAT_PREREQ_BAB, bab, 0, 0);

  sprintf(buf, "BAB +%d", bab);
  prereq->description = strdup(buf);

  /* Link it up */
  prereq->next = feat_list[featnum].prerequisite_list;
  feat_list[featnum].prerequisite_list = prereq;
}
void feat_prereq_weapon_proficiency(int featnum) {
  struct feat_prerequisite *prereq = NULL;
  char buf[80];

  prereq = create_prerequisite(FEAT_PREREQ_WEAPON_PROFICIENCY, 0, 0, 0);

  sprintf(buf, "Proficiency in same weapon");
  prereq->description = strdup(buf);

  /*  Link it up */
  prereq->next = feat_list[featnum].prerequisite_list;
  feat_list[featnum].prerequisite_list = prereq;
}

  /* ASSIGNING FEATS - Below are the various feat initializations :
   *   1) feat number, defined in structs.h
   *   2) displayed name of the feat
   *   3) in the game or not, and thus can be learned and displayed
   *   4) learned through a trainer (study menu) or whether it is a feat given automatically
   *      to certain classes or races.
   *   5) feat can stack with itself.
   *   6) feat type, for organization in the selection menu
   *   7) short description of the feat.
   *   8) long description of the feat.   */
/* utility functions for assigning "specials" to individual feats */
void epicfeat(int featnum) {
  feat_list[featnum].epic = TRUE;
}
void combatfeat(int featnum) {
  feat_list[featnum].combat_feat = TRUE;
}
void dailyfeat(int featnum, event_id event) {
  feat_list[featnum].event = event;
}

/* function to assign basic attributes to feat */
void feato(int featnum, char *name, int in_game, int can_learn, int can_stack, int feat_type, char *short_description, char *description) {
  feat_list[featnum].name = name;
  feat_list[featnum].in_game = in_game;
  feat_list[featnum].can_learn = can_learn;
  feat_list[featnum].can_stack = can_stack;
  feat_list[featnum].feat_type = feat_type;
  feat_list[featnum].short_description = short_description;
  feat_list[featnum].description = description;
  feat_list[featnum].prerequisite_list = NULL;
}
void initialize_feat_list(void) {
  int i;

/* initialize the list of feats */
  for (i = 0; i < NUM_FEATS; i++) {
    feat_list[i].name = "Unused Feat";
    feat_list[i].in_game = FALSE;
    feat_list[i].can_learn = FALSE;
    feat_list[i].can_stack = FALSE;
    feat_list[i].feat_type = FEAT_TYPE_NONE;
    feat_list[i].short_description = "ask staff";
    feat_list[i].description = "ask staff";
    feat_list[i].epic = FALSE;
    feat_list[i].combat_feat = FALSE;
    feat_list[i].prerequisite_list = NULL;
    feat_list[i].event = eNULL; /* Set all feats to eNULL event as default. */
  }
}
/* primary function for assigning feats */
void assign_feats(void) {

  initialize_feat_list();

  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */

  /***/
  /* Combat feats */
  /***/

  /* combat modes */
  feato(FEAT_POWER_ATTACK, "power attack", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "subtract a number from hit and add to dam.  If 2H weapon add 2x dam instead",
    "subtract a number from hit and add to dam.  If 2H weapon add 2x dam instead");
    feat_prereq_attribute(FEAT_POWER_ATTACK, AB_STR, 13);
  feato(FEAT_COMBAT_EXPERTISE, "combat expertise", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "When active, take -5 penalty to attack roll and gain a +5 dodge bonus to your AC",
    "When active, take -5 penalty to attack roll and gain a +5 dodge bonus to your AC");
    feat_prereq_attribute(FEAT_COMBAT_EXPERTISE, AB_INT, 13);
    /* required for whirlwind */

  /* weapon focus feats */
  feato(FEAT_WEAPON_FOCUS, "weapon focus", TRUE, TRUE, TRUE, FEAT_TYPE_COMBAT,
    "+1 to hit rolls for selected weapon",
    "+1 to hit rolls for selected weapon");
    feat_prereq_bab(FEAT_WEAPON_FOCUS, 1);
    feat_prereq_weapon_proficiency(FEAT_WEAPON_FOCUS);
  feato(FEAT_GREATER_WEAPON_FOCUS, "greater weapon focus", TRUE, TRUE, TRUE, FEAT_TYPE_COMBAT,
    "+1 to hit rolls with weapon",
    "+1 to hit rolls with weapon");
    feat_prereq_cfeat(FEAT_GREATER_WEAPON_FOCUS, FEAT_WEAPON_FOCUS);
    feat_prereq_weapon_proficiency(FEAT_GREATER_WEAPON_FOCUS);
    feat_prereq_class_level(FEAT_GREATER_WEAPON_FOCUS, CLASS_WARRIOR, 8);
  feato(FEAT_WEAPON_SPECIALIZATION, "weapon specialization", TRUE, TRUE, TRUE, FEAT_TYPE_COMBAT,
    "+2 to dam rolls with chosen weapon",
    "Choose one type of weapon, such as greataxe, for which you have already "
      "selected the Weapon Focus feat. You can also choose unarmed strike as "
      "your weapon for purposes of this feat. You gain a +2 bonus on damage "
      "using the selected weapon.");
    feat_prereq_weapon_proficiency(FEAT_WEAPON_SPECIALIZATION);
    feat_prereq_cfeat(FEAT_WEAPON_SPECIALIZATION, FEAT_WEAPON_FOCUS);
    feat_prereq_class_level(FEAT_WEAPON_SPECIALIZATION, CLASS_WARRIOR, 4);
  feato(FEAT_GREATER_WEAPON_SPECIALIZATION, "greater weapon specialization", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "+4 damage with weapon",
    "Choose one type of weapon, such as halberd, for which you have already "
      "selected the Weapon Focus feat. You can also choose unarmed strike as "
      "your weapon for purposes of this feat. You gain a +4 bonus on damage "
      "using the selected weapon (stacks).");
    feat_prereq_weapon_proficiency(FEAT_GREATER_WEAPON_SPECIALIZATION);
    feat_prereq_cfeat(FEAT_GREATER_WEAPON_SPECIALIZATION, FEAT_WEAPON_FOCUS);
    feat_prereq_cfeat(FEAT_GREATER_WEAPON_SPECIALIZATION, FEAT_GREATER_WEAPON_FOCUS);
    feat_prereq_cfeat(FEAT_GREATER_WEAPON_SPECIALIZATION, FEAT_WEAPON_SPECIALIZATION);
    feat_prereq_class_level(FEAT_GREATER_WEAPON_SPECIALIZATION, CLASS_WARRIOR, 12);

  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */

  /* mobility feats */
  feato(FEAT_DODGE, "dodge", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "+1 dodge bonus to ac",
    "+1 dodge bonus to ac");
    feat_prereq_attribute(FEAT_DODGE, AB_DEX, 13);
  feato(FEAT_MOBILITY, "mobility", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "+4 dodge ac bonus against attacks of opportunity",
    "+4 dodge ac bonus against attacks of opportunity");
    feat_prereq_attribute(FEAT_MOBILITY, AB_DEX, 13);
    feat_prereq_feat(FEAT_MOBILITY, FEAT_DODGE, 1);
  feato(FEAT_SPRING_ATTACK, "spring attack", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "you can choose the direction you flee and gain springleap",
    "you can choose the direction you flee and gain springleap");
    feat_prereq_bab(FEAT_SPRING_ATTACK, 4);
    feat_prereq_attribute(FEAT_SPRING_ATTACK, AB_DEX, 13);
    feat_prereq_feat(FEAT_SPRING_ATTACK, FEAT_DODGE, 1);
    feat_prereq_feat(FEAT_SPRING_ATTACK, FEAT_MOBILITY, 1);
  feato(FEAT_WHIRLWIND_ATTACK, "whirlwind attack", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "grants whirlwind and hitall attacks [under construction]",
    "grants whirlwind and hitall attacks [under construction]");
    feat_prereq_attribute(FEAT_WHIRLWIND_ATTACK, AB_DEX, 13);
    feat_prereq_attribute(FEAT_WHIRLWIND_ATTACK, AB_INT, 13);
    feat_prereq_feat(FEAT_WHIRLWIND_ATTACK, FEAT_COMBAT_EXPERTISE, 1);
    feat_prereq_feat(FEAT_WHIRLWIND_ATTACK, FEAT_DODGE, 1);
    feat_prereq_feat(FEAT_WHIRLWIND_ATTACK, FEAT_MOBILITY, 1);
    feat_prereq_feat(FEAT_WHIRLWIND_ATTACK, FEAT_SPRING_ATTACK, 1);
    feat_prereq_bab(FEAT_WHIRLWIND_ATTACK, 4);

  /* critical feats */
  feato(FEAT_POWER_CRITICAL, "power critical", TRUE, TRUE, TRUE, FEAT_TYPE_COMBAT,
    "+4 to rolls to confirm critical hits.",
    "+4 to rolls to confirm critical hits.");
    feat_prereq_weapon_proficiency(FEAT_POWER_CRITICAL);
    feat_prereq_bab(FEAT_POWER_CRITICAL, 4);
  feato(FEAT_IMPROVED_CRITICAL, "improved critical", TRUE, TRUE, TRUE, FEAT_TYPE_COMBAT,
    "doubled critical threat rating for weapon chosen",
    "doubled critical threat rating for weapon chosen");
    feat_prereq_weapon_proficiency(FEAT_IMPROVED_CRITICAL);
    feat_prereq_bab(FEAT_IMPROVED_CRITICAL, 8);

  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  /* ranged attack feats */
  feato(FEAT_POINT_BLANK_SHOT, "point blank shot", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "+1 to hit and dam rolls with ranged weapons in the same room",
    "+1 to hit and dam rolls with ranged weapons in the same room, can fight "
      "in close quarters with ranged weapon");
  feato(FEAT_FAR_SHOT, "far shot", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "allows firing outside room",
    "When the 'far shot' feat is taken you are able to fire in any direction at "
      "a target in the next room.");
    feat_prereq_attribute(FEAT_FAR_SHOT, AB_DEX, 15);
    feat_prereq_feat(FEAT_FAR_SHOT, FEAT_POINT_BLANK_SHOT, 1);
  feato(FEAT_RAPID_SHOT, "rapid shot", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "extra attack ranged weapon at -2 to all attacks",
    "can make extra attack per round with ranged weapon at -2 to all attacks");
    feat_prereq_attribute(FEAT_RAPID_SHOT, AB_DEX, 13);
    feat_prereq_feat(FEAT_RAPID_SHOT, FEAT_POINT_BLANK_SHOT, 1);
  feato(FEAT_MANYSHOT, "manyshot", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "extra ranged attack when rapid shot turned on",
    "extra ranged attack when rapid shot turned on");
    feat_prereq_attribute(FEAT_MANYSHOT, AB_DEX, 15);
    feat_prereq_feat(FEAT_MANYSHOT, FEAT_RAPID_SHOT, 1);
  feato(FEAT_PRECISE_SHOT, "precise shot", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "You may shoot in melee without the standard -4 to hit penalty",
    "You may shoot in melee without the standard -4 to hit penalty");
    feat_prereq_attribute(FEAT_PRECISE_SHOT, AB_DEX, 13);
  feato(FEAT_IMPROVED_PRECISE_SHOT, "improved precise shot", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "+4 to hit on all close ranged attacks",
    "+4 to hit on all close ranged attacks");
    feat_prereq_feat(FEAT_IMPROVED_PRECISE_SHOT, FEAT_PRECISE_SHOT, 1);
    feat_prereq_bab(FEAT_IMPROVED_PRECISE_SHOT, 12);
  feato(FEAT_RAPID_RELOAD, "rapid reload", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "can load crossbows and slings rapidly",
    "The time required for you to reload your chosen type of weapon is reduced "
      "to a free action (for a hand or light crossbow), a move action (for heavy "
      "crossbow or one-handed firearm). If you have selected this feat for a hand "
      "crossbow or light crossbow, you may fire that weapon as many times in a "
      "full-attack action as you could attack if you were using a bow.");

  /* here is our mounted combat feats */
  feato(FEAT_MOUNTED_COMBAT, "mounted combat", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "once per round rider may negate a hit against him with a successful ride vs attack roll check",
    "once per round rider may negate a hit against him with a successful ride vs attack roll check");
    feat_prereq_ability(FEAT_MOUNTED_COMBAT, ABILITY_RIDE, 4);
  feato(FEAT_RIDE_BY_ATTACK, "ride by attack", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "normally use full round action on charge, now use move action",
    "normally use full round action on charge, now use move action");
    feat_prereq_feat(FEAT_RIDE_BY_ATTACK, FEAT_MOUNTED_COMBAT, 1);
    feat_prereq_ability(FEAT_RIDE_BY_ATTACK, ABILITY_RIDE, 6);
  feato(FEAT_SPIRITED_CHARGE, "spirited charge", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "When mounted and using charge, you deal double damage with a melee weapon (or triple damage with a lance).",
    "When mounted and using charge, you deal double damage with a melee weapon (or triple damage with a lance).");
    feat_prereq_ability(FEAT_SPIRITED_CHARGE, ABILITY_RIDE, 8);
    feat_prereq_feat(FEAT_SPIRITED_CHARGE, FEAT_MOUNTED_COMBAT, 1);
    feat_prereq_feat(FEAT_SPIRITED_CHARGE, FEAT_RIDE_BY_ATTACK, 1);
  /* end mounted combat feats */

  /* ranged attack + mounted combat feats */
  feato(FEAT_MOUNTED_ARCHERY, "mounted archery", FALSE, FALSE, FALSE, FEAT_TYPE_COMBAT,
    "no penalty for mounted archery attacks",
    "normally mounted archery combat imposes a -4 penalty to attacks, with this "
      "feat you have no penalty to your attacks");
    feat_prereq_feat(FEAT_MOUNTED_ARCHERY, FEAT_MOUNTED_COMBAT, 1);
    feat_prereq_ability(FEAT_MOUNTED_ARCHERY, ABILITY_RIDE, 6);

  /* shield feats */
  feato(FEAT_IMPROVED_SHIELD_PUNCH, "improved shield punch", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "retain your shield's AC bonus when you shield punch",
    "retain your shield's AC bonus when you shield punch");
    feat_prereq_feat(FEAT_IMPROVED_SHIELD_PUNCH, FEAT_ARMOR_PROFICIENCY_SHIELD, 1);
  feato(FEAT_SHIELD_CHARGE, "shield charge", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "make a knockdown attack when you bash with your shield",
    "make a knockdown attack when you bash with your shield");
    feat_prereq_bab(FEAT_SHIELD_CHARGE, 3);
    feat_prereq_feat(FEAT_SHIELD_CHARGE, FEAT_IMPROVED_SHIELD_PUNCH, 1);
  feato(FEAT_SHIELD_SLAM, "shield slam", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "Daze an opponent of any size by slamming them with your shield.",
    "Daze an opponent of any size by slamming them with your shield.");
    feat_prereq_bab(FEAT_SHIELD_SLAM, 6);
    feat_prereq_feat(FEAT_SHIELD_SLAM, FEAT_SHIELD_CHARGE, 1);
    feat_prereq_feat(FEAT_SHIELD_SLAM, FEAT_IMPROVED_SHIELD_PUNCH, 1);

  /* two weapon fighting feats */
  feato(FEAT_TWO_WEAPON_FIGHTING, "two weapon fighting", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "reduces penalty for two weapon fighting",
    "reduces penalty for two weapon fighting");
    feat_prereq_attribute(FEAT_TWO_WEAPON_FIGHTING, AB_DEX, 15);
  feato(FEAT_IMPROVED_TWO_WEAPON_FIGHTING, "improved two weapon fighting", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "extra attack with offhand weapon at -5 penalty",
    "extra attack with offhand weapon at -5 penalty");
    feat_prereq_cfeat(FEAT_IMPROVED_TWO_WEAPON_FIGHTING, FEAT_TWO_WEAPON_FIGHTING);
    feat_prereq_attribute(FEAT_IMPROVED_TWO_WEAPON_FIGHTING, AB_DEX, 17);
  feato(FEAT_GREATER_TWO_WEAPON_FIGHTING, "greater two weapon fighting", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "gives an additional offhand weapon attack at -10 penalty",
    "gives an additional offhand weapon attack at -10 penalty");
    feat_prereq_cfeat(FEAT_GREATER_TWO_WEAPON_FIGHTING, FEAT_IMPROVED_TWO_WEAPON_FIGHTING);
    feat_prereq_attribute(FEAT_GREATER_TWO_WEAPON_FIGHTING, AB_DEX, 19);
  feato(FEAT_TWO_WEAPON_DEFENSE, "two weapon defense", FALSE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "when wielding two weapons receive +1 shield ac bonus",
    "When dual-wielding, or using a double-weapon, you automatically get a +1 "
      "bonus to deflection AC");
  feat_prereq_cfeat(FEAT_TWO_WEAPON_DEFENSE, FEAT_TWO_WEAPON_FIGHTING);

  /* uncategorized combat feats */
  feato(FEAT_BLIND_FIGHT, "blind fighting", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "when fighting blind, retain dex bonus to AC and deny enemy +2 attack bonus for invisibility or other concealment.",
    "when fighting blind, retain dex bonus to AC and deny enemy +2 attack bonus for invisibility or other concealment.");

  feato(FEAT_COMBAT_REFLEXES, "combat reflexes", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "can make a number of attacks of opportunity equal to dex bonus",
    "can make a number of attacks of opportunity equal to dex bonus");

  feato(FEAT_IMPROVED_INITIATIVE, "improved initiative", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "+4 to initiative checks to see who attacks first each round",
    "+4 to initiative checks to see who attacks first each round");

  feato(FEAT_IMPROVED_TRIP, "improved trip", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "no AoO when tripping, +4 to trip, attack immediately",
    "no attack of opportunity when tripping, +4 to trip check, attack immediately "
      "on successful trip.");
    feat_prereq_attribute(FEAT_IMPROVED_TRIP, AB_INT, 13);
    feat_prereq_feat(FEAT_IMPROVED_TRIP, FEAT_COMBAT_EXPERTISE, 1);

  feato(FEAT_IMPROVED_DISARM, "improved disarm", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "opponent doesn't receive AoO",
    "You do not provoke an attack of opportunity when performing a disarm combat "
      "maneuver. In addition, you receive a +2 bonus on checks made to disarm "
      "a foe. You also receive a +2 bonus to your Combat Maneuver Defense "
      "whenever an opponent tries to disarm you.");
    feat_prereq_attribute(FEAT_IMPROVED_DISARM, AB_INT, 13);
    feat_prereq_feat(FEAT_IMPROVED_DISARM, FEAT_COMBAT_EXPERTISE, 1);
  feato(FEAT_GREATER_DISARM, "greater disarm", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "knock weapon to ground on successful disarm",
    "On a successful disarm, normally the weapon is knocked to the opponent's "
          "inventory, but with greater disarm, the weapon gets knocked into "
          "the room.");
    feat_prereq_attribute(FEAT_GREATER_DISARM, AB_INT, 13);
    feat_prereq_feat(FEAT_GREATER_DISARM, FEAT_COMBAT_EXPERTISE, 1);
    feat_prereq_feat(FEAT_GREATER_DISARM, FEAT_IMPROVED_DISARM, 1);

  feato(FEAT_IMPROVED_FEINT, "improved feint", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "can feint as a move action",
    "You can make a Bluff check to feint in combat as a move action as opposed "
      "to standard action.");
    feat_prereq_attribute(FEAT_IMPROVED_FEINT, AB_INT, 13);
    feat_prereq_feat(FEAT_IMPROVED_FEINT, FEAT_COMBAT_EXPERTISE, 1);

  feato(FEAT_IMPROVED_TAUNTING, "improved taunting", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "can taunt as a move action",
    "You can make a diplomacy check to taunt in combat as a move action as opposed "
      "to standard action.");
    feat_prereq_ability(FEAT_IMPROVED_TAUNTING, ABILITY_DIPLOMACY, 10);

  feato(FEAT_IMPROVED_INTIMIDATION, "improved intimidation", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "can intimidate as a move action",
    "You can make a intimidate check to taunt in combat as a move action as opposed "
      "to standard action.");
    feat_prereq_ability(FEAT_IMPROVED_INTIMIDATION, ABILITY_INTIMIDATE, 10);

    /* note: monks get this for free */
  feato(FEAT_STUNNING_FIST, "stunning fist", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "may make unarmed attack to stun opponent for one round",
    "may make unarmed attack to stun opponent for one round");
    feat_prereq_attribute(FEAT_STUNNING_FIST, AB_DEX, 13);
    feat_prereq_attribute(FEAT_STUNNING_FIST, AB_WIS, 13);
    feat_prereq_feat(FEAT_STUNNING_FIST, FEAT_IMPROVED_UNARMED_STRIKE, 1);
    feat_prereq_bab(FEAT_STUNNING_FIST, 8);

    /* rogues get this feat for free */
  feato(FEAT_WEAPON_FINESSE, "weapon finesse", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "use dex for hit roll of weapons",
    "Use dexterity bonus for hit roll of weapons (if better than strength bonus), "
      "there is no benefit to this feat for archery.  Only works for 'light' classed "
      "weapons, or weapons that are a smaller size class than the character.");
    feat_prereq_bab(FEAT_WEAPON_FINESSE, 1);

  /* epic */
  feato(FEAT_EPIC_PROWESS, "epic prowess", TRUE, TRUE, TRUE, FEAT_TYPE_COMBAT,
    "+1 to all attacks per rank",
    "+1 to all attacks per rank");
  /* two weapon fighting feats, epic */
  feato(FEAT_PERFECT_TWO_WEAPON_FIGHTING, "perfect two weapon fighting", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "Extra attack with offhand weapon",
    "Extra attack with offhand weapon with no penalty");
    feat_prereq_cfeat(FEAT_PERFECT_TWO_WEAPON_FIGHTING, FEAT_GREATER_TWO_WEAPON_FIGHTING);
    feat_prereq_attribute(FEAT_PERFECT_TWO_WEAPON_FIGHTING, AB_DEX, 21);
  /* archery epic feats */
  feato(FEAT_EPIC_MANYSHOT, "epic manyshot", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "extra ranged attack when rapid shot turned on",
    "extra ranged attack when rapid shot turned on");
    feat_prereq_attribute(FEAT_EPIC_MANYSHOT, AB_DEX, 19);
    feat_prereq_feat(FEAT_EPIC_MANYSHOT, FEAT_MANYSHOT, 1);
  feato(FEAT_BLINDING_SPEED, "blinding speed", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "get an extra attack, as if hasted",
    "You get an extra attack, as if hasted.");
    feat_prereq_attribute(FEAT_BLINDING_SPEED, AB_DEX, 25);
  feato(FEAT_EPIC_WEAPON_SPECIALIZATION, "epic weapon specialization", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
    "+3 hit/dam with weapon",
    "Choose one type of weapon, such as halberd, for which you have already "
      "selected the Weapon Focus feat. You can also choose unarmed strike as "
      "your weapon for purposes of this feat. You gain a +3 bonus on damage "
      "and attack bonus using the selected weapon (stacks).");
    feat_prereq_cfeat(FEAT_EPIC_WEAPON_SPECIALIZATION, FEAT_GREATER_WEAPON_SPECIALIZATION);
    feat_prereq_class_level(FEAT_EPIC_WEAPON_SPECIALIZATION, CLASS_WARRIOR, 20);

  /*****************/
  /* General feats */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */

  /* weapon / armor proficiency */
  feato(FEAT_ARMOR_PROFICIENCY_LIGHT, "light armor proficiency", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "allows unpenalized use of light armor ",
    "allows unpenalized use of light armor ");
  feato(FEAT_ARMOR_PROFICIENCY_MEDIUM, "medium armor proficiency", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "allows unpenalized use of medium armor ",
    "allows unpenalized use of medium armor ");
    feat_prereq_feat(FEAT_ARMOR_PROFICIENCY_MEDIUM, FEAT_ARMOR_PROFICIENCY_LIGHT, 1);
  feato(FEAT_ARMOR_PROFICIENCY_HEAVY, "heavy armor proficiency", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "allows unpenalized use of heavy armor ",
    "allows unpenalized use of heavy armor ");
    feat_prereq_feat(FEAT_ARMOR_PROFICIENCY_HEAVY, FEAT_ARMOR_PROFICIENCY_LIGHT, 1);
    feat_prereq_feat(FEAT_ARMOR_PROFICIENCY_HEAVY, FEAT_ARMOR_PROFICIENCY_MEDIUM, 1);
  feato(FEAT_ARMOR_PROFICIENCY_SHIELD, "shield armor proficiency", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "able to use bucklers, shields without penalty ",
    "able to use bucklers, light, medium and heavy shields without penalty, does not include tower shields");
  feato(FEAT_ARMOR_PROFICIENCY_TOWER_SHIELD, "tower shield proficiency", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "can use tower shields without penalties",
    "can use tower shields without penalties");
    feat_prereq_feat(FEAT_ARMOR_PROFICIENCY_TOWER_SHIELD, FEAT_ARMOR_PROFICIENCY_SHIELD, 1);
  feato(FEAT_SIMPLE_WEAPON_PROFICIENCY, "simple weapon proficiency", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "may use all simple weapons",
    "may use all simple weapons");
  feato(FEAT_MARTIAL_WEAPON_PROFICIENCY, "martial weapon proficiency", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "able to use all martial weapons",
    "able to use all martial weapons");
    feat_prereq_feat(FEAT_MARTIAL_WEAPON_PROFICIENCY, FEAT_SIMPLE_WEAPON_PROFICIENCY, 1);
  feato(FEAT_EXOTIC_WEAPON_PROFICIENCY, "exotic weapon proficiency", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "can use exotic weapons without penalties",
    "can use exotic weapons without penalties");
    feat_prereq_bab(FEAT_EXOTIC_WEAPON_PROFICIENCY, 2);
    feat_prereq_feat(FEAT_EXOTIC_WEAPON_PROFICIENCY, FEAT_MARTIAL_WEAPON_PROFICIENCY, 1);
  /********/

  feato(FEAT_ABLE_LEARNER, "able learner", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "+1 to all skills",
    "+1 to all skills");
  feato(FEAT_ACROBATIC, "acrobatic", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "+3 to acrobatics skill checks",
    "+3 to acrobatics skill checks");
  feato(FEAT_AGILE, "agile", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "+2 to acrobatics and escape artist skill checks",
    "+2 to acrobatics and escape artist skill checks");
  feato(FEAT_ALERTNESS, "alertness", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "+2 to spot and listen skill checks ",
    "+2 to spot and listen skill checks ");
  feato(FEAT_ANIMAL_AFFINITY, "animal affinity", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "+2 to handle animal and ride skill checks",
    "+2 to handle animal and ride skill checks");

  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_ATHLETIC, "athletic", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "+2 to swim and climb skill checks",
    "+2 to swim and climb skill checks");
  feato(FEAT_DECEITFUL, "deceitful", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "+2 to disguise and forgery skill checks",
    "+2 to disguise and forgery skill checks");
  feato(FEAT_DEFT_HANDS, "deft hands", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "+3 to sleight of hand checks",
    "+3 to sleight of hand skill checks");
  feato(FEAT_DILIGENT, "diligent", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "+2 bonus to appraise and use magical device skill checks",
    "+2 bonus to appraise and use magical device skill checks");
  feato(FEAT_ENDURANCE, "endurance", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "larger movement gain, and recovery",
    "One with endurance will gain extra movement points per level and in addition "
      "recover movement points quicker.");

  /* extra cleric feat */
  feato(FEAT_EXTRA_TURNING, "extra turning", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "2 extra turn attempts per day",
    "2 extra turn attempts per day");
  feat_prereq_feat(FEAT_EXTRA_TURNING, FEAT_TURN_UNDEAD, 1);

  feato(FEAT_GREAT_FORTITUDE, "great fortitude", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "+2 to all fortitude saving throw checks",
    "+2 to all fortitude saving throw checks");

  feato(FEAT_INVESTIGATOR, "investigator", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "+2 to lore and perception checks",
    "+2 to lore and perception checks");
  feato(FEAT_IRON_WILL, "iron will", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "+2 to all willpower saving throw checks",
    "+2 to all willpower saving throw checks");
  feato(FEAT_LIGHTNING_REFLEXES, "lightning reflexes", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "+2 to all reflex saving throw checks",
    "+2 to all reflex saving throw checks");
  feato(FEAT_MAGICAL_APTITUDE, "magical aptitude", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "+2 to spellcraft and use magical device skill checks",
    "+2 to spellcraft and use magical device skill checks");
  feato(FEAT_NEGOTIATOR, "negotiator", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "+2 to diplomacy and sense motive skills",
    "+2 to diplomacy and sense motive skills");
  feato(FEAT_NIMBLE_FINGERS, "nimble fingers", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "+2 to disable device skill checks",
    "+2 to disable device skill checks");
  feato(FEAT_PERSUASIVE, "persuasive", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "+2 to bluff and intimidate skill checks",
    "+2 to bluff and intimidate skill checks");
  feato(FEAT_SELF_SUFFICIENT, "self sufficient", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "+2 to heal and survival skill checks",
    "+2 to heal and survival skill checks");
  feato(FEAT_STEALTHY, "stealthy", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "+2 to hide and move silently skill checks",
    "+2 to hide and move silently skill checks");
  feato(FEAT_TOUGHNESS, "toughness", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
    "+1 hp per level, +(level) hp upon taking",
    "+1 hp per level, +(level) hp upon taking");
  /* rangers get this for free */
  feato(FEAT_TRACK, "track", TRUE, FALSE, FALSE, FEAT_TYPE_GENERAL,
    "use survival skill to track others",
    "use survival skill to track others");
    feat_prereq_ability(FEAT_TRACK, ABILITY_SURVIVAL, 19);


  /* Epic */
  feato(FEAT_EPIC_TOUGHNESS, "epic toughness", TRUE, TRUE, TRUE, FEAT_TYPE_GENERAL,
    "gain 30 hps",
    "Gain 30 more maximum hit points");
  feato(FEAT_ARMOR_SKIN, "armor skin", TRUE, TRUE, TRUE, FEAT_TYPE_GENERAL,
    "Increases natural armor by 1",
    "Increases natural armor by 1");
  /* monk */ feato(FEAT_IMPROVED_SPELL_RESISTANCE, "improved spell resistance", TRUE, TRUE, TRUE, FEAT_TYPE_GENERAL,
    "+2 to spell resistance",
    "+2 to spell resistance");
  feat_prereq_feat(FEAT_IMPROVED_SPELL_RESISTANCE, FEAT_DIAMOND_SOUL, 1);
  feato(FEAT_GREAT_CONSTITUTION, "great constitution", TRUE, TRUE, TRUE, FEAT_TYPE_GENERAL,
    "Increases Constitution by 1",
    "Increases Constitution by 1");
  feato(FEAT_GREAT_DEXTERITY, "great dexterity", TRUE, TRUE, TRUE, FEAT_TYPE_GENERAL,
    "Increases Dexterity by 1",
    "Increases Dexterity by 1");
  feato(FEAT_GREAT_INTELLIGENCE, "great intelligence", TRUE, TRUE, TRUE, FEAT_TYPE_GENERAL,
    "Increases Intelligence by 1",
    "Increases Intelligence by 1");
  feato(FEAT_GREAT_STRENGTH, "great strength", TRUE, TRUE, TRUE, FEAT_TYPE_GENERAL,
    "Increases Strength by 1",
    "Increases Strength by 1");
  feato(FEAT_GREAT_WISDOM, "great wisdom", TRUE, TRUE, TRUE, FEAT_TYPE_GENERAL,
    "Increases Wisdom by 1",
    "Increases Wisdom by 1");
  feato(FEAT_GREAT_CHARISMA, "great charisma", TRUE, TRUE, TRUE, FEAT_TYPE_GENERAL,
    "Increases Charisma by 1",
    "Increases Charisma by 1");

  /* Spellcasting feats */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_SPELL_PENETRATION, "spell penetration", TRUE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING,
    "+2 bonus on caster level checks to defeat spell resistance",
    "+2 bonus on caster level checks to defeat spell resistance");
  feato(FEAT_GREATER_SPELL_PENETRATION, "greater spell penetration", TRUE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING,
    "+2 to caster level checks to defeat spell resistance",
    "+2 to caster level checks to defeat spell resistance");
  feat_prereq_feat(FEAT_GREATER_SPELL_PENETRATION, FEAT_SPELL_PENETRATION, 1);

  feato(FEAT_ARMORED_SPELLCASTING, "armored spellcasting", TRUE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING,
    "reduce penalty for casting arcane spells while armored",
    "reduces the arcane armor weight penalty by 5");

  feato(FEAT_FASTER_MEMORIZATION, "faster memorization", TRUE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING,
    "decreases spell memorization time",
    "decreases spell memorization time");

  feato(FEAT_SPELL_FOCUS, "spell focus", TRUE, TRUE, TRUE, FEAT_TYPE_SPELLCASTING,
    "+1 to all spell dcs for all spells in school/domain",
    "+1 to all spell dcs for all spells in school/domain");
  feato(FEAT_GREATER_SPELL_FOCUS, "greater spell focus", TRUE, TRUE, TRUE, FEAT_TYPE_SPELLCASTING,
    "+2 to all spell dcs for all spells in school/domain",
    "+2 to all spell dcs for all spells in school/domain");

  feato(FEAT_IMPROVED_FAMILIAR, "improved familiar", TRUE, TRUE, TRUE, FEAT_TYPE_SPELLCASTING,
    "your familiar gets more powerful",
    "Each rank in this feat will give your familiar: 1 AC, 10 Hit-points, +1 to "
          "strength, dexterity and constitution.");

  feato(FEAT_QUICK_CHANT, "quick chant", TRUE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING,
    "you can cast spells faster",
    "You can cast spells about 50 percent faster than normal with this feat.");

  feato(FEAT_AUGMENT_SUMMONING, "augment summoning", TRUE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING,
    "enhance summoned creatures",
    "Gives all creatures you have from summoning spells +4 to strength and "
      "constitution.  Note: this will not augment your familiar, called companions, "
      "or charmed/dominated victims.");

  feato(FEAT_ENHANCED_SPELL_DAMAGE, "enhanced spell damage", TRUE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING,
    "+1 spell damage per die rolled",
    "You gain +1 spell damage per die rolled, example:  if you are level 10 and "
      "normally create a 10d6 damage fireball, with this feat your fireball would "
      "do 10d6+10.");

  feato(FEAT_COMBAT_CASTING, "combat casting", TRUE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING,
    "+4 to concentration checks made in combat or when grappled ",
    "+4 to concentration checks made in combat or when grappled ");

  /* epic type spellcasting feats */
  feato(FEAT_MUMMY_DUST, "mummy dust", TRUE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING,
    "gain access to epic spell - mummy dust",
    "Once per game day, you can cast a spell that will conjure a powerful Mummy "
          "Lord to assist you in combat.");
    feat_prereq_ability(FEAT_MUMMY_DUST, ABILITY_SPELLCRAFT, 23);
  feato(FEAT_DRAGON_KNIGHT, "dragon knight", TRUE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING,
    "gain access to epic spell - dragon knight",
    "Once per game day, you can cast a spell that will conjure a small red dragon"
          " to assist you in combat.");
    feat_prereq_ability(FEAT_DRAGON_KNIGHT, ABILITY_SPELLCRAFT, 25);
  feato(FEAT_GREATER_RUIN, "greater ruin", TRUE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING,
    "gain access to epic spell - greater ruin",
    "Once per game day, you can cast a spell that will cause serious damage to "
          "a selected target.");
    feat_prereq_ability(FEAT_GREATER_RUIN, ABILITY_SPELLCRAFT, 27);
  feato(FEAT_HELLBALL, "hellball", TRUE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING,
    "gain access to epic spell - greater ruin",
    "Once per game day, you can cast a spell that will cause serious damage to "
          "all the targets in a room.");
    feat_prereq_ability(FEAT_HELLBALL, ABILITY_SPELLCRAFT, 29);
  feato(FEAT_EPIC_MAGE_ARMOR, "epic mage armor", TRUE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING,
    "gain access to epic spell - epic mage armor",
    "Once per game day, you can cast a spell that will give a massive AC bonus to "
          "the caster.");
    feat_prereq_ability(FEAT_EPIC_MAGE_ARMOR, ABILITY_SPELLCRAFT, 31);
  feato(FEAT_EPIC_WARDING, "epic warding", TRUE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING,
    "gain access to epic spell - epic warding",
    "Once per game day, you can cast a spell that will absorb a massive amount "
          "of damage.");
    feat_prereq_ability(FEAT_EPIC_WARDING, ABILITY_SPELLCRAFT, 32);
  /* zusuk marker */


  /* Crafting feats */

  feato(FEAT_DRACONIC_CRAFTING, "draconic crafting", TRUE, FALSE, FALSE, FEAT_TYPE_CRAFT,
    "All magical items created gain higher bonuses w/o increasing level",
    "All magical items created gain higher bonuses w/o increasing level");
  feato(FEAT_DWARVEN_CRAFTING, "dwarven crafting", TRUE, FALSE, FALSE, FEAT_TYPE_CRAFT,
    "All weapons and armor made have higher bonuses",
    "All weapons and armor made have higher bonuses");
  feato(FEAT_ELVEN_CRAFTING, "elven crafting", TRUE, FALSE, FALSE, FEAT_TYPE_CRAFT,
    "All equipment made is 50 percent weight and uses 50 percent materials",
    "All equipment made is 50 percent weight and uses 50 percent materials");
  feato(FEAT_FAST_CRAFTER, "fast crafter", TRUE, FALSE, FALSE, FEAT_TYPE_CRAFT,
    "Reduces crafting time",
    "Reduces crafting time");

  /* Cleric Domain (ability) Feats */
  feato(FEAT_LIGHTNING_ARC, "lightning arc", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "you can unleash an arc of electricity",
    "As a standard action, you can unleash an arc of electricity. This arc of "
          "electricity deals 1d6+10 points of electricity damage + 1 point for "
          "every two cleric levels you possess. You can use this ability a number "
          "of times per day equal to 3 + your Wisdom modifier.");
  feato(FEAT_ACID_DART, "acid dart", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "you can unleash a dart of acid",
    "As a standard action, you can unleash a dart of acid. This dart of "
          "acid deals 1d6+10 points of acid damage + 1 point for "
          "every two cleric levels you possess. You can use this ability a number "
          "of times per day equal to 3 + your Wisdom modifier.");
  feato(FEAT_FIRE_BOLT, "fire bolt", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "you can unleash a bolt of fire",
    "As a standard action, you can unleash a bolt of fire. This bolt of "
          "fire deals 1d6+10 points of acid damage + 1 point for "
          "every two cleric levels you possess. You can use this ability a number "
          "of times per day equal to 3 + your Wisdom modifier.");
  feato(FEAT_ICICLE, "icicle", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "you can unleash an icicle",
    "As a standard action, you can unleash an icicle. This icicle "
          "deals 1d6+10 points of cold damage + 1 point for "
          "every two cleric levels you possess. You can use this ability a number "
          "of times per day equal to 3 + your Wisdom modifier.");
  feato(FEAT_DOMAIN_ELECTRIC_RESIST, "domain electric resistance", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "gain electricity resistance",
    "At 6th cleric levels, you gain resist electricity 10. This resistance increases "
          "to 20 at 12th level and to 50 at 20th level.");
  feato(FEAT_DOMAIN_ACID_RESIST, "domain acid resistance", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "gain acid resistance",
    "At 6th cleric levels, you gain resist acid 10. This resistance increases "
          "to 20 at 12th level and to 50 at 20th level.");
  feato(FEAT_DOMAIN_FIRE_RESIST, "domain fire resistance", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "gain fire resistance",
    "At 6th cleric levels, you gain resist fire 10. This resistance increases "
          "to 20 at 12th level and to 50 at 20th level.");
  feato(FEAT_DOMAIN_COLD_RESIST, "domain cold resistance", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "gain cold resistance",
    "At 6th cleric levels, you gain resist cold 10. This resistance increases "
          "to 20 at 12th level and to 50 at 20th level.");
  feato(FEAT_CURSE_TOUCH, "curse touch", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "you can unleash a curse",
    "As a standard action, you can unleash a curse (like the spell).  You can use "
      "this ability a number of times per day equal to 3 + your Wisdom modifier.");
  feato(FEAT_CHAOTIC_WEAPON, "chaotic weapon", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "your weapon becomes chaotic",
    "Any weapon you wield behaves as if it is chaotic, and will do additional 2d6 "
      "damage against lawful-raced and lawful-aligned opponents.");
  feato(FEAT_DESTRUCTIVE_SMITE, "destructive smite", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "add 1/2 cleric level to damage",
    "You gain the destructive smite power: the supernatural ability to make a "
      "single melee attack with a morale bonus on damage rolls equal to 1/2 your "
      "cleric level (minimum 1). You must declare the destructive smite before "
      "making the attack. You can use this ability a number of times per day "
      "equal to 3 + your Wisdom modifier.");
  feato(FEAT_DESTRUCTIVE_AURA, "destructive aura", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "add 1/2 cleric level to group's damage",
    "As a standard action, you can unleash a destructive aura which will give "
      "all your group companions cleric-level/2 bonus damage for 1 round.  You can use "
      "this ability a number of times per day equal to your Wisdom modifier.");
  feato(FEAT_EVIL_TOUCH, "evil touch", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "inflict disease with touch attack",
    "You can cause a creature to become sickened as a melee touch attack.  This "
          "ability lasts for a number of rounds equal to 1/2 "
          "your cleric level (minimum 1). You can use this ability a number of times "
          "per day equal to 3 + your Wisdom modifier.");
  feato(FEAT_EVIL_SCYTHE, "evil scythe", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "empower your weapon with unholy damage",
    "At 8th level, you can give a weapon touched the unholy special weapon quality "
          "for a number of rounds equal to 1/2 your cleric level. You can use "
          "this ability once per day at 8th level, and an additional time per "
          "day for every four levels beyond 8th.");
  feato(FEAT_GOOD_TOUCH, "good touch", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "cure disease and poison with touch",
    "You can touch a creature as a standard action, removing one poison affliction "
          "and one disease per usage. You can use this ability "
          "a number of times per day equal to 3 + your Wisdom modifier.");
  feato(FEAT_GOOD_LANCE, "good lance", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "empower your weapon with holy damage",
    "At 8th level, you can give a weapon you touch the holy special weapon quality "
          "for a number of rounds equal to 1/2 your cleric level. You can use this "
          "ability once per day at 8th level, and an additional time per day for "
          "every four levels beyond 8th.");
  feato(FEAT_HEALING_TOUCH, "healing touch", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "you can heal with your touch",
    "You can touch a living creature as a standard action, healing it for 20 + 1d4 points "
          "of damage plus 1 for every two cleric levels you possess. You can only "
          "use this ability on a creature that is below half their total hit points. You can use "
          "this ability a number of times per day equal to 3 + your Wisdom modifier.");
  feato(FEAT_EMPOWERED_HEALING, "empowered healing", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "heal spells empowered",
    "At 6th level, all of your cure spells are treated as if they were empowered, "
          "increasing the amount of damage healed by half (+50%). This does not "
          "apply to damage dealt to undead with a cure spell. This does not stack "
          "with the Empower Spell metamagic feat.");
  feato(FEAT_KNOWLEDGE, "knowledge", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "your lore ability is enhanced",
    "You get +4 to your lore checks and you can apply your wisdom bonus to your "
          "lore checks.");
  feato(FEAT_EYE_OF_KNOWLEDGE, "eye of knowledge", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "summon a wizard eye",
    "Like the 'wizard eye' spell available to arcane casters, you can summon a "
          "wizard eye which you can control to scout for you.  You can use this "
          "ability a number of times per day equal to 3 + your Wisdom modifier.");
  feato(FEAT_BLESSED_TOUCH, "blessed touch", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "can bless with your touch",
    "You can touch a willing creature as a standard action, infusing it with the "
          "power of divine order and allowing it to treat all attack rolls, skill "
          "checks, ability checks, and saving throws for 1 round as if the natural "
          "d20 roll resulted in an 11. You can use this ability a number of times "
          "per day equal to 3 + your Wisdom modifier.");
  feato(FEAT_LAWFUL_WEAPON, "lawful weapon", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "your weapons becomes lawful",
    "Any weapon you wield behaves as if it is axiomatic, and will do additional 2d6 "
      "damage against chaotic-raced and chaotic-aligned opponents.");
  feato(FEAT_DECEPTION, "deception", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "gain stealth and disguise as class abilities",
    "Both stealth and disguise are now class abilities to you, making them twice "
          "as effective at training.");
  feato(FEAT_COPYCAT, "copycat", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "can innate create mirror images",
    "You can use 'mirror image', like the arcane spell.  You can use this ability "
          "a number of times per day equal to your Wisdom modifier.");
  feato(FEAT_MASS_INVIS, "mass invis", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "can use mass invisibility",
    "You can use 'mass invisibility', like the arcane spell.  You can use this ability "
          "a number of times per day equal to your Wisdom modifier.");
  feato(FEAT_RESISTANCE, "resistance", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "gain bonus to all resistances",
    "You get +1 to all your resistances per 6 cleric levels.");
  feato(FEAT_SAVES, "saves", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "gain bonus to all saves",
    "You get +1 to all your saves per 6 cleric levels.");
  feato(FEAT_AURA_OF_PROTECTION, "aura of protection", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "grant AC/sves bonus to group",
    "You can grant +1 to all saves and AC per 6 cleric levels to all your "
          "group members for 4 rounds.  You can use this ability a number of times "
          "per day equal to 3 + your Wisdom modifier.");
  feato(FEAT_ETH_SHIFT, "eth shift", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "shift between ethereal/prime planes at will",
    "You gain the ability to shift group members or yourself, at will, to the "
          "ethereal plane and back to the prime material plane.");
  feato(FEAT_BATTLE_RAGE, "battle rage", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "bonus to hitroll/damage",
    "You add your cleric level / 4 to both your hitroll and damroll for "
          "5 rounds.  You can use this ability a number of times per day equal "
          "to your Wisdom modifier.");
  feato(FEAT_WEAPON_EXPERT, "weapon expert", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
    "+1 to attack rolls, can use all martial weapons",
    "This feat behaves like the martial weapon proficiency, granting you proficiency "
          "in all weapons except for exotic ones.  In addition you get a +1 to all "
          "attack rolls when wielding a weapon.");

  /*****/
  /* Class ability feats */

  /* warrior */
  feato(FEAT_ARMOR_TRAINING, "armor training", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "reduced armor penalty, increased maxdex",
    "The penalty caused by wearing armor is reduced by 1 per rank of this feat.  "
      "In addition your maximum dexterity for your armoring increased 1 per rank.");
  feato(FEAT_WEAPON_TRAINING, "weapon training", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "+2 confirm crit, +1 min diceroll",
    "You get to reroll your attack diceroll if you roll 1, per rank of this feat "
      "(example: 4 ranks means you cannot roll lower than 5).  In addition, you "
      "get +2 to confirming critical hits per rank.");
  feato(FEAT_STALWART_WARRIOR, "stalwart warrior", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "apply class level to AC",
    "The warrior gets to apply all warrior-levels divided by 4 to AC.");
  feato(FEAT_ARMOR_MASTERY, "armor mastery i", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "gains Damage Reduction 5/— when armored",
    "Gain Damage Reduction 5/— whenever wearing armor or using a shield.");
  feato(FEAT_WEAPON_MASTERY, "weapon mastery i", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "cannot be disarmed, +2 ac when using any weapon",
    "Gain immunity to disarm attempts, in addition, while wielding any type of "
      "weapon, gain a bonus 2 to deflection AC.");
  feato(FEAT_ARMOR_MASTERY_2, "armor mastery ii", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "become extremely skilled with shields",
    "You gain 2 bonus AC while using a shield, in addition you gain 25 magic "
      "resistance while using a shield.");
  feato(FEAT_WEAPON_MASTERY_2, "weapon mastery ii", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "+6 to CMD and CMB",
    "While wielding a weapon, gain a +6 bonus to all combat maneuver attempts and +6 bonus to defending "
      "against any combat maneuver.");

  /* Cleric */
  /* turn undead is below, shared with paladin */

  /* Cleric / Paladin */
  feato(FEAT_TURN_UNDEAD, "turn undead", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "can cause fear in or destroy undead based on class level and charisma bonus",
    "can cause fear in or destroy undead based on class level and charisma bonus");

  /* Paladin / Champion of Torm */
  feato(FEAT_DIVINE_GRACE, "divine grace", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "charisma bonus added to all saving throw checks",
    "charisma bonus added to all saving throw checks");

  /* Paladin */
  /* divine grace is above, shared with champion of torm */
  /* turn undead is above, shared with cleric */
  feato(FEAT_AURA_OF_COURAGE, "aura of courage", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Immunity to fear attacks, +4 bonus to fear saves for group members",
    "Immunity to fear attacks, +4 bonus to fear saves for group members");
  feato(FEAT_SMITE_EVIL, "smite evil", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "add level to hit roll and charisma bonus to damage",
    "add level to hit roll and charisma bonus to damage");
  feato(FEAT_DETECT_EVIL, "detect evil", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "able to detect evil alignments",
    "able to detect evil alignments");
  feato(FEAT_AURA_OF_GOOD, "aura of good", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "able to detect good alignments",
    "able to detect good alignments");
  feato(FEAT_DIVINE_HEALTH, "divine health", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "immune to disease",
    "immune to disease");
  feato(FEAT_LAYHANDS, "lay on hands", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Powerful divine healing ability usable a limited number of times a day",
    "Powerful divine healing ability usable a limited number of times a day");
  feato(FEAT_REMOVE_DISEASE, "remove disease", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "can cure diseases",
    "can cure diseases (purify)");
  feato(FEAT_CALL_MOUNT, "call mount", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Allows you to call a paladin mount",
    "Allows you to call a paladin mount");
  feato(FEAT_DIVINE_BOND, "divine bond", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "bonuses to attack and damage rolls",
    "Hitroll and Damage bonus of 1 + paladin-level/3 for levels above 5, the "
      "bonus caps at 6");
  feato(FEAT_GLORIOUS_RIDER, "glorious rider", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "use cha instead of dex for ride checks",
    "Normally ride checks for performing skills while mounted, uses your dexterity "
      "bonus to compute your chance.  With this feat you can use your charisma bonus instead.");
  feato(FEAT_LEGENDARY_RIDER, "legendary rider", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "no armor penalty to ride check, can block extra attack",
    "You can now block up to two attacks a round with successful ride checks "
      "against the incoming attacks.  In addition, you no longer suffer any "
      "penalty because of armor while mounted.");
  feato(FEAT_EPIC_MOUNT, "epic mount", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "you get a more powerful mount",
    "You get an epic level mount with some special abilities.");

  /* Rogue */
  /* trap sense below (shared with berserker) */
  /* uncanny dodge below (shared with berserker) */
  /* improved uncanny dodge below (shared with berserker) */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  /*talent*/feato(FEAT_CRIPPLING_STRIKE, "crippling strike", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Chance to do strength damage with a sneak attack.",
    "Chance to do strength damage with a sneak attack.");
  /*talent*/feato(FEAT_IMPROVED_EVASION, "improved evasion", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "as evasion but half damage of failed save",
    "as evasion but half damage of failed save");
  feato(FEAT_EVASION, "evasion", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "on successful reflex save no damage from spells and effects",
    "on successful reflex save no damage from spells and effects");
  feato(FEAT_TRAPFINDING, "trapfinding", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "+4 to detecting traps",
    "+4 to detecting traps (detecttrap)");
  /*adv talent*/feato(FEAT_DEFENSIVE_ROLL, "defensive roll", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "can survive a potentially fatal blow",
    "can survive a potentially fatal blow, has long cooldown before usable "
      "again (automatic usage)");
  /*epic talent*/feato(FEAT_BACKSTAB, "backstab", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "bonus to attack and double damage",
    "You get +4 to attack (and another +1 if sneaking, +1 if hiding).  Do double "
      "damage as well.  In addition, backstab is a move-action instead of full "
      "round action.  Backstab requires a piercing weapon to be a success.");
  /*epic talent*/feato(FEAT_SAP, "sap", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "chance to knock opponent out",
    "When both sneaking and hiding (in that order) and wielding a bludgeon weapon, you can perform a special sap attack "
      "which on success, will knock an opponent down and if they fail a fortitude save "
      "versus rogue-level / 2 + dex bonus they will be paralyzed for 2 rounds.  This type of "
      "attack will get a -6 penalty to success, but can be negated by wielding a "
      "2-handed bludgeoning weapon (+4 bonus) and if your opponent can't see you (+4 bonus).");
  /*epic talent*/feato(FEAT_VANISH, "vanish", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "escape combat!",
    "Once per day, you can use vanish as a free action to completely escape combat "
      "effectively disengaging and entering a sneak/hidden mode.  This also heals "
      "10 hitpoints and gives 25 percent concealment for 2 rounds.");
  /*epic talent*/feato(FEAT_IMPROVED_VANISH, "improved vanish", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "improves vanish feat",
    "Vanish now heals 20 hitpoints and gives 100 percent concealment for 2 rounds.");
  /*talent*/feato(FEAT_SLIPPERY_MIND, "slippery mind", TRUE, TRUE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "extra chance for will saves vs mind affecting spells",
    "extra chance for will saves vs mind affecting spells");
  /*talent*/feato(FEAT_APPLY_POISON, "apply poison", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "can apply poison to weapons",
    "can apply poison to weapons (applypoison)");
  /*talent*/feato(FEAT_DIRT_KICK, "dirt kick", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "can kick dirt into opponents face",
    "can kick dirt into opponents face, causing blindness (dirtkick)");
  feato(FEAT_SNEAK_ATTACK, "sneak attack", TRUE, FALSE, TRUE, FEAT_TYPE_COMBAT,
    "+1d6 to damage when flanking",
    "+1d6/rank to damage when flanking, opponent is flat-footed, or opponent is without dexterity bonus");
  feat_prereq_class_level(FEAT_SNEAK_ATTACK, CLASS_ROGUE, 2);
  feato(FEAT_WEAPON_PROFICIENCY_ROGUE, "weapon proficiency - rogues", TRUE, FALSE, FALSE, FEAT_TYPE_GENERAL,
  "proficiency in rogue weapons",
  "You are proficient in the usage of hand-crossbows, rapiers, sap, short-sword "
          "and short-bow.");


  /* Rogue / Berserker */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_TRAP_SENSE, "trap sense", TRUE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY,
    "auto-sense traps",
    "Normally to find traps you have to actively try to detect them.  With "
      "this feat you will make a perception check to detect traps automatically. "
      "For every point you have in this feat your bonus to your check is increase by 1.");
  feato(FEAT_UNCANNY_DODGE, "uncanny dodge", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "retains dex bonus when flat footed",
    "retains dexterity bonus when flat footed");
  feato(FEAT_IMPROVED_UNCANNY_DODGE, "improved uncanny dodge", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "cannot be flanked",
    "cannot be flanked, unless opponents berserker/rogue levels are 4 or more");

  /* Ranger */
  /* the favored enemy mechanic is already built into the study system, not utilizing these two feats */
  /* unfinished */ feato(FEAT_FAVORED_ENEMY_AVAILABLE, "favored enemy available", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "can choose an enemy type as a favored enemy",
    "can choose an enemy type as a favored enemy");
  /* unfinished */ feato(FEAT_FAVORED_ENEMY, "favored enemy", TRUE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY,
    "Gain bonuses when fighting against a particular type of enemy",
    "Gain bonuses when fighting against a particular type of enemy");
  /* modified from original */
  feato(FEAT_CAMOUFLAGE, "camouflage", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "gain stealth bonus in nature",
    "Gains +6 bonus to sneak/hide in nature");
  feato(FEAT_HIDE_IN_PLAIN_SIGHT, "hide in plain sight", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "can hide in battle",
    "This feat grants the ability to perform the stealth maneuver: hide, even while in combat.  This check is made with a -8 penalty");
  /* unfinished */ feato(FEAT_SWIFT_TRACKER, "swift tracker", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "No penalty while autotracking.",
    "No penalty while autotracking.");
  /* combat mastery should be either an archer set of feats or dual wielding set of feats, for now we give them both */
  /* we have to make separate feats for dual weapon fighting (two weapon fighting) */
  feato(FEAT_DUAL_WEAPON_FIGHTING, "dual weapon fighting", TRUE, FALSE, FALSE, FEAT_TYPE_COMBAT,
    "reduces penalty for two weapon fighting",
    "reduces penalty for two weapon fighting while wearing light or lighter armor");
  feato(FEAT_IMPROVED_DUAL_WEAPON_FIGHTING, "improved dual weapon fighting", TRUE, FALSE, FALSE, FEAT_TYPE_COMBAT,
    "extra attack with offhand weapon at -5 penalty",
    "extra attack with offhand weapon at -5 penalty while wearing light or lighter armor");
  feato(FEAT_GREATER_DUAL_WEAPON_FIGHTING, "greater dual weapon fighting", TRUE, FALSE, FALSE, FEAT_TYPE_COMBAT,
    "gives an additional offhand weapon attack at -10 penalty",
    "gives an additional offhand weapon attack at -10 penalty while wearing light or lighter armor");
    /* point blank shot */
    /* rapid shot */
    /* manyshot */
  /* epic */
    /* epic manyshot */
  feato(FEAT_PERFECT_DUAL_WEAPON_FIGHTING, "perfect dual weapon fighting", TRUE, FALSE, FALSE, FEAT_TYPE_COMBAT,
    "Extra attack with offhand weapon",
    "Extra attack with offhand weapon while wearing light or lighter armor");
  feato(FEAT_BANE_OF_ENEMIES, "bane of enemies", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "weapon acts as bane",
    "Any weapon you wield that strikes an opponent that is a favored enemy will "
      "act as a bane weapon and do an additional 2d6 damage.");
  feato(FEAT_EPIC_FAVORED_ENEMY, "epic favored enemy", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "gain extra bonus to dam against fav enemy",
    "You will gain an extra +4 to attack bonus and damage against any of your "
      "favored enemies.");

  /* Ranger / Druid */
  feato(FEAT_ANIMAL_COMPANION, "animal companion", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Can call a loyal companion animal that accompanies the adventurer.",
    "Can call a loyal companion animal that accompanies the adventurer.");
  /* unfinished */ feato(FEAT_WILD_EMPATHY, "wild empathy", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "The adventurer can improve the attitude of an animal.",
    "The adventurer can improve the attitude of an animal.");
  /* unfinished */ feato(FEAT_WOODLAND_STRIDE, "woodland stride", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Reduced movement penalty when moving through woodland areas.",
    "Reduced movement penalty when moving through woodland areas.");

  /* Druid */
  feato(FEAT_VENOM_IMMUNITY, "venom immunity", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "grants immunity to poison",
    "grants immunity to poison");
  /* unfinished */ feato(FEAT_NATURE_SENSE, "nature sense", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "+2 to lore and survival skills",
    "+2 to lore and survival skills");
  /* unfinished */ feato(FEAT_RESIST_NATURES_LURE, "resist nature's lure", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "+4 to resist spells and spell like abilities from fey creatures",
    "+4 to resist spells and spell like abilities from fey creatures");
  /* unfinished */ feato(FEAT_THOUSAND_FACES, "a thousand faces", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Can alter one's physical appearance, giving +10 to disguise checks.",
    "Can alter one's physical appearance, giving +10 to disguise checks.");
  /* modified from original */
  feato(FEAT_TRACKLESS_STEP, "trackless step", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "bonus to hide/sneak in nature",
    "+4 bonus to hide/sneak in nature");
  feato(FEAT_WILD_SHAPE, "wild shape", TRUE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY,
    "Gain the ability to shapechange",
    "Gains the ability to turn into any small or medium animal and back again "
          "once per day. Options for new forms include all creatures with the "
          "animal type.  Changing form (to animal or back) is a standard action "
          "and doesn't provoke an attack of opportunity.");
  feato(FEAT_WILD_SHAPE_2, "wild shape ii", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Advance the ability to shapechange",
    "Can use wildshape to change into a Large or Tiny animal or a Small elemental.");
  feato(FEAT_WILD_SHAPE_3, "wild shape iii", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Advance the ability to shapechange",
    "Can use wildshape to change into a Huge or Diminutive animal, a Medium "
          "elemental, or a Small or Medium plant creature.");
  feato(FEAT_WILD_SHAPE_4, "wild shape iv", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Advance the ability to shapechange",
    "Can use wildshape to change into a Large elemental or a Large plant creature.");
  feato(FEAT_WILD_SHAPE_5, "wild shape v", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Advance the ability to shapechange",
    "Can use wildshape to change into a Huge elemental or a Huge plant creature.");
  feato(FEAT_WEAPON_PROFICIENCY_DRUID, "weapon proficiency - druids", TRUE, FALSE, FALSE, FEAT_TYPE_GENERAL,
    "proficiency in druid weapons",
    "You are proficient in the usage of clubs, daggers, quarterstaff, darts, sickle, scimitar, shortspear, spear and slings.");

  /* Druid / Monk */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_TIMELESS_BODY, "timeless body", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
  "immune to negative aging effects (unfinished)",
  "immune to negative aging effects (unfinished) - currently gives a flat 25 percent "
    "reduction to all incoming negative damage");

  /* Monk */
  feato(FEAT_UNARMED_STRIKE, "unarmed strike", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Unarmed attacks are considered to be weapons.",
    "Unarmed attacks are considered to be weapons.");
  feato(FEAT_IMPROVED_UNARMED_STRIKE, "improved unarmed strike", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Unarmed attacks are considered to be weapons.",
    "Unarmed attacks are considered to be weapons.  You can disarm foes without "
          "a penalty when fighting unarmed.  Also you get access to the headbutt combat maneuver.");
  /*unfinished*/feato(FEAT_KI_STRIKE, "ki strike", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "unarmed attack considered a magical weapon (unfinished)",
    "unarmed attack considered a magical weapon [note: until fixed this feat "
      "just gives a +1 to hitroll/damroll]");
  feato(FEAT_STILL_MIND, "still mind", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "+2 bonus on saving throws vs. Enchantments",
    "+2 bonus on saving throws vs. Enchantments");
  feato(FEAT_WHOLENESS_OF_BODY, "wholeness of body", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "can heal class level*2 + 20 hp to self",
    "can heal class level*2 + 20 hp to self");
  feato(FEAT_SLOW_FALL, "slow fall", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "no damage for falling 1 room/feat rank",
    "no damage for falling 1 room/feat rank");
  feato(FEAT_ABUNDANT_STEP, "abundant step", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "magically move between tight spaces, as the spell dimension door",
    "Magically move between tight spaces, as the spell dimension door.  You "
      "can even go through doors.  To use, you must give directions from your "
      "current location.  Example: abundantstep w w n n e 2w n n e");
  feato(FEAT_DIAMOND_BODY, "diamond body", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "immune to disease",
    "immune to disease");
  feato(FEAT_DIAMOND_SOUL, "diamond soul", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "spell resistance equal to class level + 10",
    "spell resistance equal to class level + 10");
  feato(FEAT_EMPTY_BODY, "empty body", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "50 percent concealment for 1 round/monk level per day",
    "50 percent concealment for 1 round/monk level per day");
  feato(FEAT_FLURRY_OF_BLOWS, "flurry of blows", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "extra attack when fighting unarmed at -2 to all attacks",
    "Flurry of Blows is a special mode available to monks (type flurryofblows).  You get an extra attack "
      "at full BAB, but at a penalty to -2 to all your attacks.  At level 5 the "
      "penalty drops to -1, and at level 9 the penalty disappears completely. ");
  feato(FEAT_GREATER_FLURRY, "greater flurry", TRUE, FALSE, FALSE, FEAT_TYPE_COMBAT,
    "extra unarmed attack when using flurry of blows",
    "extra unarmed attack when using flurry of blows, at level 15 you get yet "
      "another bonus attack at full BAB");
  /*unfinished*/ feato(FEAT_PERFECT_SELF, "perfect self", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Gain 10/magic damage reduction (unfinished)",
    "Gain 10/magic damage reduction [note: until our damage reduction system is "
      "changed, this feat will give a flat 3 damage reduction against ALL incoming attacks");
  feato(FEAT_PURITY_OF_BODY, "purity of body", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "immune to poison",
    "immune to poison");
  feato(FEAT_QUIVERING_PALM, "quivering palm", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "chance to kill on strike with unarmed attack",
    "You will do your wisdom bonus as bonus damage to your next unarmed strike, "
      "in addition, to opponents that are your level or lower, they have to make "
      "a fortitude save vs DC: your monk level + your wisdom bonus + 10 in order "
      "to survive your quivering palm attack");
  feato(FEAT_WEAPON_PROFICIENCY_MONK, "weapon proficiency - monks", TRUE, FALSE, FALSE, FEAT_TYPE_GENERAL,
    "proficiency in monk weapons",
    "You are proficient in the usage of quarterstaff, kama, siangham, and shuriken.");
  /* not imped */feato(FEAT_TONGUE_OF_THE_SUN_AND_MOON, "tongue of the sun and moon [not impd]", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "[not implemented] can speak any language", "[not implemented] can speak any language");
  /*epic*//* free blinding speed */
  /*epic*/feato(FEAT_KEEN_STRIKE, "keen strike", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "critical more unarmed, better stunning fist",
    "Your threat range with unarmed attacks increase by 1, in addition you get "
      "a +4 bonus to your stunning fist DC.");
  /*epic*/feato(FEAT_OUTSIDER, "outsider", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "you become an outsider",
    "You gain the ability to shift at will to the ethereal plane and back "
      "additionally you gain 15 percent concealment.");

  /* Bard */
  /* unfinished */ feato(FEAT_BARDIC_KNOWLEDGE, "bardic knowledge", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "+Int modifier bonus on knowledge checks.",
    "+Int modifier bonus on knowledge checks.");
  /* unfinished */ feato(FEAT_BARDIC_MUSIC, "bardic music", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Use Perform skill to create various magical effects.",
    "Use Perform skill to create various magical effects.");
  /* unfinished */ feato(FEAT_COUNTERSONG, "countersong", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Boost group members' resistance to sonic attacks.",
    "Boost group members' resistance to sonic attacks.");
  /* unfinished */ feato(FEAT_FASCINATE, "fascinate", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Fascinate one opponent plus one additional  for every three bard levels beyond first.",
    "Fascinate one opponent plus one additional  for every three bard levels beyond first.");
  /* unfinished */ feato(FEAT_INSPIRE_COMPETENCE, "inspire competence", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Group members gain a +2 competence bonus on skills.",
    "Group members gain a +2 competence bonus on skills.");
  /* unfinished */ feato(FEAT_INSPIRE_GREATNESS, "inspire greatness", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Grant allies better fighting capability.",
    "Grant allies better fighting capability.");
  /* unfinished */ feato(FEAT_INSPIRE_HEROICS, "inspire heroics", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Grant allies a +4 morale bonus on saving throws and a +4 dodge bonus to AC.",
    "Grant allies a +4 morale bonus on saving throws and a +4 dodge bonus to AC.");
  /* unfinished */ feato(FEAT_MASS_SUGGESTION, "mass suggestion", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Compel a group of opponents to perform an action.",
    "Compel a group of opponents to perform an action.");
  /* unfinished */ feato(FEAT_SONG_OF_FREEDOM, "song of freedom", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Break an enchantment on a single target other than yourself.",
    "Break an enchantment on a single target other than yourself.");
  /* unfinished */ feato(FEAT_SUGGESTION, "suggestion", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Compel a single opponent to perform an action.",
    "Compel a single opponent to perform an action.");
  /* unfinished */ feato(FEAT_INSPIRE_COURAGE, "inspire courage", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Bolster group members against fear attacks and improve their combat ability.",
    "Bolster group members against fear attacks and improve their combat ability.");
  feato(FEAT_WEAPON_PROFICIENCY_BARD, "weapon proficiency - bards", TRUE, FALSE, FALSE, FEAT_TYPE_GENERAL,
    "proficiency in bard weapons",
    "You are proficient in the usage of long swords, rapiers, sap, short swords, short bows and whips.");

  /* Berserker */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  /* uncanny dodge above (shared with rogue) */
  /* improved uncanny dodge above (shared with rogue) */
  /* trap sense above (shared with rogue) */
  feato(FEAT_INDOMITABLE_WILL, "indomitable will", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "bonus to will save while raging",
    "While in rage, gain a +4 bonus on Will saves");
  feato(FEAT_SHRUG_DAMAGE, "shrug damage", TRUE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY,
    "Shrug off damage, grants damage reduction",
    "Your extensive training and violent lifestyle allow you to shrug off a "
      "portion of incoming damage.  This ability grants you DR 1/- for every 3"
      "berserker levels, starting at level 4.");
  feato(FEAT_RAGE, "rage", TRUE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY,
    "+4 bonus to con, str, and will for several rounds",
    "+4 bonus to constitution, strength and will-saves, but 2 penalty to AC, for "
      "(2 * constitution-bonus + 6) rounds");
  feato(FEAT_GREATER_RAGE, "greater rage", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "+6 to str, con, and will when raging",
    "+6 to strength, constitution, and will-saves when raging");
  feato(FEAT_MIGHTY_RAGE, "mighty rage", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "+9 to str, con and will when raging",
    "+9 to strength, constitution, and will-saves when raging");
  feato(FEAT_INDOMITABLE_RAGE, "indomitable rage", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "+12 to str, con and will when raging",
    "+12 to strength, constitution, and will-saves when raging");
  feato(FEAT_TIRELESS_RAGE, "tireless rage", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "no fatigue after raging",
    "no fatigue after raging");
  /*temporary mechanic*/feato(FEAT_FAST_MOVEMENT, "fast movement", TRUE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY,
    "reduces movement usage, and increases movement regen",
    "Reduces movement usage, and increases movement regeneration.  This is a temporary mechanic.");
  /*rage power*/feato(FEAT_RP_SUPRISE_ACCURACY, "suprise accuracy", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "bonus hitroll once/rage",
    "Gain a +1 morale bonus on one attack roll.  This bonus increases by +1 for "
      "every 4 berserker levels attained. This power is used as a swift action.  This power "
      "can only be used once per rage.");
  /*rage power*/feato(FEAT_RP_POWERFUL_BLOW, "powerful blow", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "bonus damage once/rage",
    "Gain a +1 bonus on a single damage roll. This bonus increases by +1 for "
      "every 4 berserker levels attained. This power is used as a swift action.  "
      "This power can only be used once per rage.");
  /*rage power*/feato(FEAT_RP_RENEWED_VIGOR, "renewed vigor", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "able to heal while raging",
    "As a standard action, the berserker heals 3d8 points of damage + her "
      "Constitution modifier. For every four levels the berserker has attained "
      "above 4th, this amount of damage healed increases by 1d8.  This power can "
      "be used only once per day and only while raging.");
  /*rage power*/feato(FEAT_RP_HEAVY_SHRUG, "heavy shrug", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "increased DR while raging",
    "The berserker's damage reduction increases by 3/-. This increase is always "
      "active while the berserker is raging.");
  /*rage power*/feato(FEAT_RP_FEARLESS_RAGE, "fearless rage", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "fearless while raging",
    "While raging, the berserker is immune to the shaken and frightened conditions. ");
  /*rage power*/feato(FEAT_RP_COME_AND_GET_ME, "come and get me", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "take it, but dish it out heavy",
    "While raging, as a free action the berserker may leave herself open to "
      "attack while preparing devastating counterattacks. Enemies gain a +4 bonus "
      "on attack and damage rolls against the berserker until the beginning of "
      "her next turn, but every attack against the berserker provokes an attack "
      "of opportunity from her, which is resolved prior to resolving each enemy attack. ");
  /*epic*/feato(FEAT_RAGING_CRITICAL, "raging critical", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "your criticals knock over opponents while raging",
    "While raging, on successful criticals, against opponents that are standing, and not too "
      "much larger or smaller than you, your powerful critical attacks will knock "
      "them down.");
  /*epic*/feato(FEAT_EATER_OF_MAGIC, "eater of magic", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "successful saving throws will heal rager",
    "While raging, if the rager makes a successful saving throw, they recover "
      "their level in hit points.");
  /*epic*/feato(FEAT_RAGE_RESISTANCE, "rage resistance", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "gain boost in resistances while raging",
    "While raging, you will gain a 10 bonus to all your resistances.");
  /*epic*/feato(FEAT_DEATHLESS_FRENZY, "deathless frenzy", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "the rager is nearly unstoppable",
    "While raging, you have to be brought to -51 or lower to be stopped.");

  /* Sorcerer/Wizard */
  feato(FEAT_SUMMON_FAMILIAR, "summon familiar", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "summon a magical pet",
    "summon a magical pet");
  feato(FEAT_WEAPON_PROFICIENCY_WIZARD, "weapon proficiency - wizards", TRUE, FALSE, FALSE, FEAT_TYPE_GENERAL,
    "proficiency in wizard weapons",
    "You are proficient in the usage of daggers, quarterstaff, club, heavy and light crossbows.");

  /* weapon master */
  /*lvl 1*/feato(FEAT_WEAPON_OF_CHOICE, "weapons of choice", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "All weapons with weapon focus gain special abilities",
    "All weapons with weapon focus gain special abilities");
  /*lvl 2*/feato(FEAT_SUPERIOR_WEAPON_FOCUS, "superior weapon focus", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Weapons of choice have +1 to hit",
    "Weapons of choice have +1 to hit");
  /*lvl 4*/feato(FEAT_CRITICAL_SPECIALIST, "critical specialist", TRUE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY,
    "Weapons of choice have +1 to threat range per rank",
    "Weapons of choice have +1 to threat range per rank");
  /*lvl 6*/feato(FEAT_UNSTOPPABLE_STRIKE, "unstoppable strike", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Weapons of Choice have 5 percent chance to deal max damage",
    "Weapons of Choice have 5 percent chance to deal max damage");
  /*lvl 8 - 2nd rank of critical specialist */
  /*lvl 10*/feato(FEAT_INCREASED_MULTIPLIER, "increased multiplier", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Weapons of choice have +1 to their critical multiplier",
    "Weapons of choice have +1 to their critical multiplier");


  /* class feats that are implemented on classes that are not yet in the game */

  /* Duelist */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_ENHANCED_MOBILITY, "enhanced mobility", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "+4 dodge bonus vs AOO",
    "gain an additional +4 dodge bonus to AC against "
          "attacks of opportunity provoked by movement. This bonus stacks with "
          "that granted by the Mobility feat.");
  feato(FEAT_GRACE, "grace", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "+2 reflex saves",
    "gain a +2 bonus on Reflex saves");

  /* Blackguard */   /* knight of the skull (dragonlance) */
  feato(FEAT_SMITE_GOOD, "smite good", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "add level to hit roll and charisma bonus to damage",
    "add level to hit roll and charisma bonus to damage");

  /* Pale/Death Master */
  feato(FEAT_ANIMATE_DEAD, "animate dead", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "allows innate use of animate dead spell",
    "Allows innate use of animate dead spell once per day.  You get one use per ");

  /**************************/
  /* Disabled/Unimplemented */
  /**************************/

  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */

  /* not class feats */
  /* probably don't want in game at this stage */feato(FEAT_LEADERSHIP_BONUS, "improved leadership", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");
  feato(FEAT_IMPROVED_GRAPPLE, "improved grapple", FALSE, FALSE, FALSE, FEAT_TYPE_COMBAT, "ask staff", "ask staff");
  feato(FEAT_IMPROVED_OVERRUN, "improved overrun", FALSE, FALSE, FALSE, FEAT_TYPE_COMBAT, "ask staff", "ask staff");
  feato(FEAT_QUICK_DRAW, "quick draw", FALSE, FALSE, FALSE, FEAT_TYPE_COMBAT, "ask staff", "ask staff");
  feato(FEAT_SHOT_ON_THE_RUN, "shot on the run", FALSE, FALSE, FALSE, FEAT_TYPE_COMBAT, "ask staff", "ask staff");
  feato(FEAT_COMBAT_CHALLENGE, "combat challenge", FALSE, TRUE, FALSE, FEAT_TYPE_COMBAT, "allows you to make a mob focus their attention on you", "allows you to make a mob focus their attention on you");
  feato(FEAT_GREATER_COMBAT_CHALLENGE, "greater combat challenge", FALSE, TRUE, FALSE, FEAT_TYPE_COMBAT, "as improved combat challenge, but regular challenge is a minor action & challenge all is a move action", "as improved combat challenge, but regular challenge is a minor action & challenge all is a move action");
  feato(FEAT_IMPROVED_COMBAT_CHALLENGE, "improved combat challenge", FALSE, TRUE, FALSE, FEAT_TYPE_COMBAT, "allows you to make all mobs focus their attention on you", "allows you to make all mobs focus their attention on you");
  feato(FEAT_IMPROVED_NATURAL_WEAPON, "improved natural weapons", FALSE, TRUE, FALSE, FEAT_TYPE_COMBAT, "increase damage dice by one category for natural weapons", "increase damage dice by one category for natural weapons");
  feato(FEAT_IMPROVED_WEAPON_FINESSE, "improved weapon finesse", FALSE, TRUE, TRUE, FEAT_TYPE_COMBAT, "add dex bonus to damage instead of str for light weapons", "add dex bonus to damage instead of str for light weapons");
  feato(FEAT_KNOCKDOWN, "knockdown", FALSE, TRUE, FALSE, FEAT_TYPE_COMBAT, "when active, any melee attack that deals 10 damage or more invokes a free automatic trip attempt against your target", "when active, any melee attack that deals 10 damage or more invokes a free automatic trip attempt against your target");
  feato(FEAT_HEROIC_INITIATIVE, "heroic initiative", FALSE, FALSE, FALSE, FEAT_TYPE_GENERAL, "bonus to initiative checks", "bonus to initiative checks");
  feato(FEAT_IMPROVED_BULL_RUSH, "improved bull rush", FALSE, FALSE, FALSE, FEAT_TYPE_GENERAL, "ask staff", "ask staff");
  feato(FEAT_IMPROVED_REACTION, "improved reaction", FALSE, FALSE, FALSE, FEAT_TYPE_GENERAL, "+2 bonus to initiative checks (+4 at 8th class level)", "+2 bonus to initiative checks (+4 at 8th class level)");
  feato(FEAT_IMPROVED_SUNDER, "improved sunder", FALSE, FALSE, FALSE, FEAT_TYPE_GENERAL, "ask staff", "ask staff");
  feato(FEAT_SUNDER, "sunder", FALSE, FALSE, FALSE, FEAT_TYPE_GENERAL, "ask staff", "ask staff");
  feato(FEAT_MONKEY_GRIP, "monkey grip", FALSE, TRUE, TRUE, FEAT_TYPE_GENERAL, "can wield weapons one size larger than wielder in one hand with -2 to attacks.", "can wield weapons one size larger than wielder in one hand with -2 to attacks.");
  feato(FEAT_IMPROVED_INSTIGATION, "improved instigation", FALSE, TRUE, FALSE, FEAT_TYPE_GENERAL, "ask staff", "ask staff");
  feato(FEAT_IMPROVED_INTIMIDATION, "improved intimidation", FALSE, TRUE, FALSE, FEAT_TYPE_GENERAL, "ask staff", "ask staff");
  feato(FEAT_DIEHARD, "diehard", FALSE, TRUE, FALSE, FEAT_TYPE_GENERAL, "will stay alive and conscious until -10 hp or lower", "will stay alive and conscious until -10 hp or lower");
  feato(FEAT_RUN, "run", FALSE, TRUE, FALSE, FEAT_TYPE_GENERAL, "ask staff", "ask staff");
  feato(FEAT_SKILL_FOCUS, "skill focus", FALSE, TRUE, TRUE, FEAT_TYPE_GENERAL, "+3 in chosen skill", "+3 in chosen skill");
  feato(FEAT_ENERGY_RESISTANCE, "energy resistance", FALSE, TRUE, TRUE, FEAT_TYPE_GENERAL, "reduces all energy related damage by 3 per rank", "reduces all energy related damage by 3 per rank");
  feato(FEAT_FAST_HEALER, "fast healer", FALSE, TRUE, FALSE, FEAT_TYPE_GENERAL, "+2 hp healed per round", "+2 hp healed per round");
  feato(FEAT_LEADERSHIP, "leadership", FALSE, TRUE, FALSE, FEAT_TYPE_GENERAL, "can have more and higher level followers, group members get extra exp on kills and hit/ac bonuses", "can have more and higher level followers, group members get extra exp on kills and hit/ac bonuses");
  feato(FEAT_HONORBOUND, "honorbound", FALSE, TRUE, FALSE, FEAT_TYPE_GENERAL, "+2 to saving throws against fear or compulsion effects, +2 to sense motive checks", "+2 to saving throws against fear or compulsion effects, +2 to sense motive checks");
  feato(FEAT_STEADFAST_DETERMINATION, "steadfast determination", FALSE, TRUE, FALSE, FEAT_TYPE_GENERAL, "allows you to use your con bonus instead of your wis bonus for will saves", "allows you to use your con bonus instead of your wis bonus for will saves");
  feato(FEAT_WEAPON_PROFICIENCY_BASTARD_SWORD, "weapon proficiency - bastard sword", FALSE, TRUE, FALSE, FEAT_TYPE_GENERAL, "ask staff", "ask staff");

  /* difficult to implement, but a basic feat that has dependencies */
  feato(FEAT_CLEAVE, "cleave", FALSE, TRUE, FALSE, FEAT_TYPE_COMBAT, "extra initial attack against opponent after killing another opponent in same room", "extra initial attack against opponent after killing another opponent in same room");
  feat_prereq_attribute(FEAT_CLEAVE, AB_STR, 13);
  feat_prereq_feat(FEAT_CLEAVE, FEAT_POWER_ATTACK, 1);
  feato(FEAT_GREAT_CLEAVE, "great cleave", FALSE, TRUE, FALSE, FEAT_TYPE_COMBAT, "ask staff", "ask staff");
  feat_prereq_feat(FEAT_GREAT_CLEAVE, FEAT_CLEAVE, 1);
  feat_prereq_feat(FEAT_GREAT_CLEAVE, FEAT_POWER_ATTACK, 1);
  feat_prereq_attribute(FEAT_GREAT_CLEAVE, AB_STR, 13);
  feat_prereq_bab(FEAT_GREAT_CLEAVE, 4);

  /* artisan */
  feato(FEAT_LEARNED_CRAFTER, "learned crafter", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "Artisan gains exp for crafting items and harvesting", "Artisan gains exp for crafting items and harvesting");
  feato(FEAT_PROFICIENT_CRAFTER, "proficient crafter", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "Increases all crafting skills", "Increases all crafting skills");
  feato(FEAT_PROFICIENT_HARVESTER, "proficient harvester", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "Increases all harvesting skills", "Increases all harvesting skills");
  feato(FEAT_SCAVENGE, "scavenge", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "Can find materials on corpses", "Can find materials on corpses");
  feato(FEAT_BRANDING, "branding", FALSE, FALSE, FALSE, FEAT_TYPE_CRAFT, "All items made carry the artisan's brand", "All items made carry the artisan's brand");
  feato(FEAT_CRAFT_MAGICAL_ARMS_AND_ARMOR, "craft magical arms and armor", FALSE, FALSE, FALSE, FEAT_TYPE_CRAFT, "can create magical weapons and armor ", "can create magical weapons and armor ");
  feato(FEAT_CRAFT_ROD, "craft rod", FALSE, FALSE, FALSE, FEAT_TYPE_CRAFT, "can crate magical rods", "can crate magical rods");
  feato(FEAT_CRAFT_STAFF, "craft staff", FALSE, FALSE, FALSE, FEAT_TYPE_CRAFT, "can create magical staves ", "can create magical staves ");
  feato(FEAT_CRAFT_WAND, "craft wand", FALSE, FALSE, FALSE, FEAT_TYPE_CRAFT, "can create magical wands ", "can create magical wands ");
  feato(FEAT_CRAFT_WONDEROUS_ITEM, "craft wonderous item", FALSE, FALSE, FALSE, FEAT_TYPE_CRAFT, "can crate miscellaneous magical items ", "can crate miscellaneous magical items ");
  feato(FEAT_FORGE_RING, "forge ring", FALSE, FALSE, FALSE, FEAT_TYPE_CRAFT, "ask staff ", "ask staff ");
  feato(FEAT_MASTERWORK_CRAFTING, "masterwork crafting", FALSE, FALSE, FALSE, FEAT_TYPE_CRAFT, "All equipment made is masterwork", "All equipment made is masterwork");

  /* epic */
  feato(FEAT_EPIC_COMBAT_CHALLENGE, "epic combat challenge", FALSE, TRUE, FALSE, FEAT_TYPE_COMBAT, "as improved combat challenge, but both regular challenges and challenge all are minor actions", "as improved combat challenge, but both regular challenges and challenge all are minor actions");
  feato(FEAT_EPIC_DODGE, "epic dodge", FALSE, TRUE, FALSE, FEAT_TYPE_COMBAT, "automatically dodge first attack against you each round", "automatically dodge first attack against you each round");
  feato(FEAT_EPIC_SKILL_FOCUS, "epic skill focus", FALSE, TRUE, TRUE, FEAT_TYPE_GENERAL, "+10 in chosen skill", "+10 in chosen skill");
  feato(FEAT_DAMAGE_REDUCTION, "damage reduction", FALSE, TRUE, TRUE, FEAT_TYPE_GENERAL, "1/- damage reduction per rank of feat, 3/- for epic", "1/- damage reduction per rank of feat, 3/- for epic");
  feato(FEAT_FAST_HEALING, "fast healing", FALSE, TRUE, TRUE, FEAT_TYPE_GENERAL, "Heals 3 hp per rank each combat round if fighting otherwise every 6 seconds", "Heals 3 hp per rank each combat round if fighting otherwise every 6 seconds");

  /****/
  /* class feats */
  /*****/
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */

  /* ranger */
  /* in this form, these ranger feats are not in the game, they are just given both sets of feats for free now */
  /* unfinished */ feato(FEAT_COMBAT_STYLE, "combat style", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Either Rapid Shot or Two Weapon Fighting, depending on the chosen combat style.",
    "Either Rapid Shot or Two Weapon Fighting, depending on the chosen combat style.");
  /* unfinished */ feato(FEAT_IMPROVED_COMBAT_STYLE, "improved combat style", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Either Manyshot or Improved Two Weapon Fighting, depending on the chosen combat style.",
    "Either Manyshot or Improved Two Weapon Fighting, depending on the chosen combat style.");
  /* unfinished */ feato(FEAT_COMBAT_STYLE_MASTERY, "combat style master", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
    "Either Improved Precise Shot or Greater Two Weapon Fighting, depending on the chosen combat style.",
    "Either Improved Precise Shot or Greater Two Weapon Fighting, depending on the chosen combat style.");

  /* Bard */
  feato(FEAT_LINGERING_SONG, "lingering song", FALSE, TRUE, FALSE, FEAT_TYPE_GENERAL, "5 extra rounds for bard songs", "5 extra rounds for bard songs");
  feato(FEAT_EXTRA_MUSIC, "extra music", FALSE, TRUE, FALSE, FEAT_TYPE_GENERAL, "4 extra bard music uses per day", "4 extra bard music uses per day");

  /* paladin / cleric [shared] */
  feato(FEAT_DIVINE_MIGHT, "divine might", FALSE, TRUE, FALSE, FEAT_TYPE_DIVINE, "Add cha bonus to damage for number of rounds equal to cha bonus", "Add cha bonus to damage for number of rounds equal to cha bonus");
  feato(FEAT_DIVINE_SHIELD, "divine shield", FALSE, TRUE, FALSE, FEAT_TYPE_DIVINE, "Add cha bonus to armor class for number of rounds equal to cha bonus", "Add cha bonus to armor class for number of rounds equal to cha bonus");
  feato(FEAT_DIVINE_VENGEANCE, "divine vengeance", FALSE, TRUE, FALSE, FEAT_TYPE_DIVINE, "Add 2d6 damage against undead for number of rounds equal to cha bonus", "Add 2d6 damage against undead for number of rounds equal to cha bonus");
  feato(FEAT_EXCEPTIONAL_TURNING, "exceptional turning", FALSE, FALSE, FALSE, FEAT_TYPE_GENERAL, "+1d10 hit dice of undead turned", "+1d10 hit dice of undead turned");
  feato(FEAT_IMPROVED_TURNING, "improved turning", FALSE, FALSE, FALSE, FEAT_TYPE_GENERAL, "ask staff", "ask staff");

  /* paladin / champion of torm [shared] */
  /* epic */
  feato(FEAT_GREAT_SMITING, "great smiting", FALSE, TRUE, TRUE, FEAT_TYPE_COMBAT, "For each rank in this feat you add your level in damage to all smite attacks", "For each rank in this feat you add your level in damage to all smite attacks");

  /* berserker */
  feato(FEAT_EXTEND_RAGE, "extend rage", FALSE, TRUE, FALSE, FEAT_TYPE_COMBAT, "Each of the uses of your rage or frenzy ability lasts an additional 5 rounds beyond its normal duration.", "Each of the uses of your rage or frenzy ability lasts an additional 5 rounds beyond its normal duration.");
  feato(FEAT_EXTRA_RAGE, "extra rage", FALSE, TRUE, FALSE, FEAT_TYPE_GENERAL, "ask staff", "ask staff");

  /* fighter */
  feato(FEAT_WEAPON_FLURRY, "weapon flurry", FALSE, TRUE, TRUE, FEAT_TYPE_COMBAT, "2nd attack at -5 to hit with standard action or extra attack at full bonus with full round action", "2nd attack at -5 to hit with standard action or extra attack at full bonus with full round action");
  feato(FEAT_WEAPON_SUPREMACY, "weapon supremacy", FALSE, TRUE, TRUE, FEAT_TYPE_COMBAT, "+4 to resist disarm, ignore grapples, add +5 to hit roll when miss by 5 or less, can take 10 on attack rolls, +1 bonus to AC when wielding weapon", "+4 to resist disarm, ignore grapples, add +5 to hit roll when miss by 5 or less, can take 10 on attack rolls, +1 bonus to AC when wielding weapon");
  feato(FEAT_ARMOR_SPECIALIZATION_HEAVY, "armor specialization (heavy)", FALSE, TRUE, FALSE, FEAT_TYPE_GENERAL, "DR 2/- when wearing heavy armor", "DR 2/- when wearing heavy armor");
  feato(FEAT_ARMOR_SPECIALIZATION_LIGHT, "armor specialization (light)", FALSE, TRUE, FALSE, FEAT_TYPE_GENERAL, "DR 2/- when wearing light armor", "DR 2/- when wearing light armor");
  feato(FEAT_ARMOR_SPECIALIZATION_MEDIUM, "armor specialization (medium)", FALSE, TRUE, FALSE, FEAT_TYPE_GENERAL, "DR 2/- when wearing medium armor", "DR 2/- when wearing medium armor");

  /* rogue (make talent or advanced talent?) */
  feato(FEAT_BLEEDING_ATTACK, "bleeding attack", FALSE, TRUE, FALSE, FEAT_TYPE_CLASS_ABILITY, "causes bleed damage on living targets who are hit by sneak attack.", "causes bleed damage on living targets who are hit by sneak attack.");
  feato(FEAT_OPPORTUNIST, "opportunist", FALSE, TRUE, FALSE, FEAT_TYPE_CLASS_ABILITY, "once per round the rogue may make an attack of opportunity against a foe an ally just struck", "once per round the rogue may make an attack of opportunity against a foe an ally just struck");
  feato(FEAT_IMPROVED_SNEAK_ATTACK, "improved sneak attack", FALSE, TRUE, TRUE, FEAT_TYPE_COMBAT, "each rank gives +5 percent chance per attack, per rank to be a sneak attack.", "each rank gives +5 percent chance per attack, per rank to be a sneak attack.");
  feato(FEAT_ROBILARS_GAMBIT, "robilars gambit", FALSE, TRUE, FALSE, FEAT_TYPE_COMBAT, "when active enemies gain +4 to hit and damage against you, but all melee attacks invoke an attack of opportunity from you.", "when active enemies gain +4 to hit and damage against you, but all melee attacks invoke an attack of opportunity from you.");
  feato(FEAT_POWERFUL_SNEAK, "powerful sneak", FALSE, TRUE, FALSE, FEAT_TYPE_GENERAL, "opt to take -2 to attacks and treat all sneak attack dice rolls of 1 as a 2", "opt to take -2 to attacks and treat all sneak attack dice rolls of 1 as a 2");
  /* epic */
  feato(FEAT_SNEAK_ATTACK_OF_OPPORTUNITY, "sneak attack of opportunity", FALSE, TRUE, FALSE, FEAT_TYPE_COMBAT, "makes all opportunity attacks sneak attacks", "makes all opportunity attacks sneak attacks");
  feato(FEAT_SELF_CONCEALMENT, "self concealment", FALSE, TRUE, TRUE, FEAT_TYPE_COMBAT, "10 percent miss chance for attacks against you per rank", "10 percent miss chance for attacks against you per rank");

  /* knight of the rose (dragonlance) */
  feato(FEAT_FINAL_STAND, "final stand", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");
  feato(FEAT_KNIGHTHOODS_FLOWER, "knighthood's flower", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");
  feato(FEAT_RALLYING_CRY, "rallying cry", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");
  feato(FEAT_WISDOM_OF_THE_MEASURE, "wisdom of the measure", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");

  /* knight of the sword */
  feato(FEAT_SOUL_OF_KNIGHTHOOD, "soul of knighthood", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");

  /* knight of the crown (dragonlance) */
  feato(FEAT_HONORABLE_WILL, "honorable will", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");
  feato(FEAT_CROWN_OF_KNIGHTHOOD, "crown of knighthood", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");
  feato(FEAT_KNIGHTLY_COURAGE, "knightly courage", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "bonus to fear checks", "bonus to fear checks");
  feato(FEAT_MIGHT_OF_HONOR, "might of honor", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");
  feato(FEAT_STRENGTH_OF_HONOR, "strength of honor", FALSE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY, "+4 to strength for several rounds", "+4 to strength for several rounds");

  /* knight of the crown / knight of the lily [SHARED] (dragonlance) */
  feato(FEAT_ARMORED_MOBILITY, "armored mobility", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "heavy armor is treated as medium armor", "heavy armor is treated as medium armor");

  /* knight of the lily (dragonlance) */
  feato(FEAT_DEMORALIZE, "demoralize", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");
  feato(FEAT_ONE_THOUGHT, "one thought", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");
  feato(FEAT_UNBREAKABLE_WILL, "unbreakable will", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");

  /* knight of the thorn (dragonlance) */
  feato(FEAT_COSMIC_UNDERSTANDING, "cosmic understanding", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");
  feato(FEAT_AURA_OF_TERROR, "aura of terror", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");
  feato(FEAT_DIVINER, "diviner", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");
  feato(FEAT_READ_OMENS, "read omens", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");
  feato(FEAT_READ_PORTENTS, "read portents", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");
  feato(FEAT_WEAPON_TOUCH, "weapon touch", FALSE, FALSE, FALSE, FEAT_TYPE_COMBAT, "ask staff", "ask staff");

  /* knight of the skull (dragonlance) */
  feato(FEAT_DARK_BLESSING, "dark blessing", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");
  feato(FEAT_AURA_OF_EVIL, "aura of evil", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");
  feato(FEAT_DETECT_GOOD, "detect good", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");
  feato(FEAT_DISCERN_LIES, "discern lies", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");
  feato(FEAT_FAVOR_OF_DARKNESS, "favor of darkness", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");

  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */

  /* Pale/Death Master */
  feato(FEAT_BONE_ARMOR, "bone armor", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "allows creation of bone armor and 10 percent arcane spell failure reduction in bone armor per rank.", "allows creation of bone armor and 10 percent arcane spell failure reduction in bone armor per rank.");
  feato(FEAT_ESSENCE_OF_UNDEATH, "essence of undeath", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "gives immunity to poison, disease, sneak attack and critical hits", "gives immunity to poison, disease, sneak attack and critical hits");
  feato(FEAT_SUMMON_GREATER_UNDEAD, "summon greater undead", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "allows innate use of summon greater undead spell 3x per day", "allows innate use of summon greater undead spell 3x per day");
  feato(FEAT_SUMMON_UNDEAD, "summon undead", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "allows innate use of summon undead spell 3x per day", "allows innate use of summon undead spell 3x per day");
  feato(FEAT_TOUCH_OF_UNDEATH, "touch of undeath", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "allows for paralytic or instant death touch", "allows for paralytic or instant death touch");
  feato(FEAT_UNDEAD_FAMILIAR, "undead familiar", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "allows for undead familiars", "allows for undead familiars");

  /* Duelist */
  feato(FEAT_CRIPPLING_CRITICAL, "crippling critical", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "allows your criticals to have random additional effects", "allows your criticals to have random additional effects");
  feato(FEAT_NO_RETREAT, "no retreat", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "allows you to gain an attack of opportunity against retreating opponents", "allows you to gain an attack of opportunity against retreating opponents");
  feato(FEAT_PARRY, "parry", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "allows you to parry incoming attacks", "allows you to parry incoming attacks");
  feato(FEAT_RIPOSTE, "riposte", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "allows you to gain an attack of opportunity after a successful parry", "allows you to gain an attack of opportunity after a successful parry");
  /* suppose to be acrobatic charge, but had naming conflict */
  feato(FEAT_ACROBATIC_CHARGE, "dextrous charge", FALSE, FALSE, FALSE, FEAT_TYPE_COMBAT, "can charge in situations when others cannot", "can charge in situations when others cannot");
  feato(FEAT_CANNY_DEFENSE, "canny defense", FALSE, FALSE, FALSE, FEAT_TYPE_COMBAT, "add int bonus (max class level) to ac when useing one light weapon and no shield", "add int bonus (max class level) to ac when useing one light weapon and no shield");
  feato(FEAT_ELABORATE_PARRY, "elaborate parry", FALSE, FALSE, FALSE, FEAT_TYPE_COMBAT, "when fighting defensively or total defense, gains +1 dodge ac per class level", "when fighting defensively or total defense, gains +1 dodge ac per class level");
  feato(FEAT_PRECISE_STRIKE, "precise strike", FALSE, FALSE, FALSE, FEAT_TYPE_COMBAT, "+1d6 damage when using only one weapon and no shield", "+1d6 damage when using only one weapon and no shield");
  feato(FEAT_DEFLECT_ARROWS, "deflect arrows", FALSE, TRUE, FALSE, FEAT_TYPE_COMBAT, "can deflect one ranged attack per round ", "can deflect one ranged attack per round ");

  /* Assassin */
  feato(FEAT_DEATH_ATTACK, "death attack", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "Chance to kill a target with sneak attack or Paralysis after 3 rounds of hidden study.", "Chance to kill a target with sneak attack or Paralysis after 3 rounds of hidden study.");
  feato(FEAT_POISON_SAVE_BONUS, "poison save bonus", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "Assassin level 2", "Bonus to all saves against poison.");
  feato(FEAT_POISON_USE, "poison use", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "Trained use in poisons without risk of poisoning self.", "Trained use in poisons without risk of poisoning self.");

  /* dwarven defender */
  feato(FEAT_DEFENSIVE_STANCE, "defensive stance", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "Allows you to fight defensively with bonuses to ac and stats.", "Allows you to fight defensively with bonuses to ac and stats.");
  feato(FEAT_MOBILE_DEFENSE, "mobile defense", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "Allows one to move while in defensive stance", "Allows one to move while in defensive stance");

  /* favored soul */
  feato(FEAT_DEITY_WEAPON_PROFICIENCY, "deity's weapon proficiency", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "allows you to use the weapon of your deity", "allows you to use the weapon of your deity");
  feato(FEAT_HASTE, "haste", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "can cast haste 3x per day", "can cast haste 3x per day");

  /* dragon disciple */
  feato(FEAT_DRAGON_APOTHEOSIS, "dragon apotheosis", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");
  feato(FEAT_ELEMENTAL_IMMUNITY, "elemental immunity", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");
  feato(FEAT_BREATH_WEAPON, "breath weapon", FALSE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY, "ask staff", "ask staff");
  feato(FEAT_CHARISMA_BOOST, "charisma boost", FALSE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY, "ask staff", "ask staff");
  feato(FEAT_CLAWS_AND_BITE, "claws and bite", FALSE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY, "ask staff", "ask staff");
  feato(FEAT_CONSTITUTION_BOOST, "constitution boost", FALSE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY, "ask staff", "ask staff");
  feato(FEAT_INTELLIGENCE_BOOST, "intelligence boost", FALSE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY, "ask staff", "ask staff");
  feato(FEAT_SLEEP_PARALYSIS_IMMUNITY, "sleep & paralysis immunity", FALSE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY, "ask staff", "ask staff");
  feato(FEAT_STRENGTH_BOOST, "strength boost", FALSE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY, "ask staff", "ask staff");
  feato(FEAT_TRAMPLE, "trample", FALSE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY, "ask staff", "ask staff");
  feato(FEAT_WINGS, "wings", FALSE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY, "ask staff", "ask staff");
  feato(FEAT_NATURAL_ARMOR_INCREASE, "natural armor increase", FALSE, FALSE, FALSE, FEAT_TYPE_GENERAL, "ask staff", "ask staff");
  feato(FEAT_BLINDSENSE, "blindsense", FALSE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY, "ask staff", "ask staff");

  /* dragon rider */
  feato(FEAT_DRAGON_MOUNT_BOOST, "dragon mount boost", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "gives +18 hp, +1 ac, +1 hit and +1 damage per rank in the feat", "gives +18 hp, +10 ac, +1 hit and +1 damage per rank in the feat");
  feato(FEAT_DRAGON_MOUNT_BREATH, "dragon mount breath", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "allows you to use your dragon mount's breath weapon once per rank, per 10 minutes.", "allows you to use your dragon mount's breath weapon once per rank, per 10 minutes.");

  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */

  /* arcane archer */
  feato(FEAT_ENHANCE_ARROW_ALIGNED, "enhance arrow (aligned)", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "+1d6 holy/unholy damage with bows against different aligned creatures.", "+1d6 holy/unholy damage with bows against different aligned creatures.");
  feato(FEAT_ENHANCE_ARROW_DISTANCE, "enhance arrow (distance)", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "doubles range increment on weapon.", "doubles range increment on weapon.");
  feato(FEAT_ENHANCE_ARROW_ELEMENTAL, "enhance arrow (elemental)", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "+1d6 elemental damage with bows", "+1d6 elemental damage with bows");
  feato(FEAT_ENHANCE_ARROW_ELEMENTAL_BURST, "enhance arrow (elemental burst)", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "+2d10 on critical hits with bows", "+2d10 on critical hits with bows");
  feato(FEAT_ENHANCE_ARROW_MAGIC, "enhance arrow (magic)", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "+1 to hit and damage with bows per rank", "+1 to hit and damage with bows per rank");
  /* epic */
  feato(FEAT_SWARM_OF_ARROWS, "swarm of arrows", FALSE, TRUE, FALSE, FEAT_TYPE_COMBAT, "allows you to make a single ranged attack against everyone in range.", "allows you to make a single ranged attack against everyone in range.");

  /* wizard / sorc */
  /*craft*/
  feato(FEAT_BREW_POTION, "brew potion", FALSE, FALSE, FALSE, FEAT_TYPE_CRAFT, "can create magical potions ", "can create magical potions ");
  feato(FEAT_SCRIBE_SCROLL, "scribe scroll", FALSE, FALSE, FALSE, FEAT_TYPE_CRAFT, "can scribe spells from memory onto scrolls", "can scribe spells from memory onto scrolls");
  /*metamagic*/
  feato(FEAT_ENLARGE_SPELL, "enlarge spell", FALSE, FALSE, FALSE, FEAT_TYPE_METAMAGIC, "ask staff ", "ask staff ");
  feato(FEAT_HEIGHTEN_SPELL, "heighten spell", FALSE, FALSE, FALSE, FEAT_TYPE_METAMAGIC, "ask staff ", "ask staff ");
  feato(FEAT_SILENT_SPELL, "silent spell", FALSE, FALSE, FALSE, FEAT_TYPE_METAMAGIC, "ask staff", "ask staff");
  feato(FEAT_STILL_SPELL, "still spell", FALSE, FALSE, FALSE, FEAT_TYPE_METAMAGIC, "ask staff", "ask staff");
  feato(FEAT_WIDEN_SPELL, "widen spell", FALSE, FALSE, FALSE, FEAT_TYPE_METAMAGIC, "ask staff", "ask staff");
  feato(FEAT_EMPOWER_SPELL, "empower spell", FALSE, TRUE, FALSE, FEAT_TYPE_METAMAGIC, "all variable numerical effects of a spell are increased by one half ", "all variable numerical effects of a spell are increased by one half ");
  feato(FEAT_EMPOWERED_MAGIC, "empowered magic", FALSE, TRUE, FALSE, FEAT_TYPE_METAMAGIC, "+1 to all spell dcs", "+1 to all spell dcs");
  feato(FEAT_EXTEND_SPELL, "extend spell", FALSE, TRUE, FALSE, FEAT_TYPE_METAMAGIC, "durations of spells are 50 percent longer when enabled ", "durations of spells are 50 percent longer when enabled ");
  feato(FEAT_MAXIMIZE_SPELL, "maximize spell", FALSE, TRUE, FALSE, FEAT_TYPE_METAMAGIC, "all spells cast while maximised enabled do maximum effect.", "all spells cast while maximised enabled do maximum effect.");
  feato(FEAT_QUICKEN_SPELL, "quicken spell", FALSE, TRUE, FALSE, FEAT_TYPE_METAMAGIC, "allows you to cast spell as a move action instead of standard action", "allows you to cast spell as a move action instead of standard action");
  /*spellcasting*/
  feato(FEAT_ESCHEW_MATERIALS, "eschew materials", FALSE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING, "ask staff", "ask staff");
  feato(FEAT_IMPROVED_COUNTERSPELL, "improved counterspell", FALSE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING, "ask staff", "ask staff");
  feato(FEAT_SPELL_MASTERY, "spell mastery", FALSE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING, "ask staff", "ask staff");
  /* epic */
  /*spellcasting*/
  feato(FEAT_EPIC_SPELLCASTING, "epic spellcasting", FALSE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING, "allows you to cast epic spells", "allows you to cast epic spells");
  /*metamagic*/
  feato(FEAT_INTENSIFY_SPELL, "intensify spell", FALSE, TRUE, FALSE, FEAT_TYPE_METAMAGIC, "maximizes damage/healing and then doubles it.", "maximizes damage/healing and then doubles it.");
  feato(FEAT_ENHANCE_SPELL, "increase spell damage", FALSE, TRUE, FALSE, FEAT_TYPE_METAMAGIC, "increase max number of damage dice for certain damage based spell by 5", "increase max number of damage dice for certain damage based spell by 5");
  feato(FEAT_AUTOMATIC_QUICKEN_SPELL, "automatic quicken spell", FALSE, TRUE, TRUE, FEAT_TYPE_METAMAGIC, "You can cast level 0, 1, 2 & 3 spells automatically as if quickened.  Every addition rank increases the max spell level by 3.", "You can cast level 0, 1, 2 & 3 spells automatically as if quickened.  Every addition rank increases the max spell level by 3.");

  /* sacred fist */
  feato(FEAT_SACRED_FLAMES, "sacred flames", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "allows you to use innate 'flame weapon' 3 times per 10 minutes", "allows you to use innate 'flame weapon' 3 times per 10 minutes");

  /* druid */
  feato(FEAT_NATURAL_SPELL, "natural spell", FALSE, TRUE, FALSE, FEAT_TYPE_WILD, "allows casting of spells while wild shaped.", "allows casting of spells while wild shaped.");

  /* monk */

  /* End Class ability Feats */

  /******/
  /* Racial ability feats */

  /* Elf */
  feato(FEAT_WEAPON_PROFICIENCY_ELF, "weapon proficiency - elves", FALSE, FALSE, FALSE, FEAT_TYPE_GENERAL, "ask staff", "ask staff");

  /* Crystal Dwarf */
  feato(FEAT_CRYSTAL_BODY, "crystal body", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY, "Allows you to harden your crystal-like body for a short time. (Damage reduction 3/-)", "Allows you to harden your crystal-like body for a short time. (Damage reduction 3/-)");
  feato(FEAT_CRYSTAL_FIST, "crystal fist", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY, "Allows you to innately grow jagged and sharp crystals on your arms and legs to enhance damage in melee. (+3 damage)", "Allows you to innately grow jagged and sharp crystals on your arms and legs to enhance damage in melee. (+3 damage)");

  /* Arcana Golem */
  feato(FEAT_SPELLBATTLE, "spellbattle", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY, "Strengthen your body with Arcane power.", "By channeling their inner magic, Arcana Golems can use it to provide a huge surge to their physical attributes in the rare cases in which they must resort to physical violence. While the eldritch energies cloud their mind and finesse, the bonus to durability and power can provide that edge when need.  \r\nSpell Battle is a mode. When this mode is activated, you receive a penalty to attack rolls equal to the argument and caster levels equal to half. In return, you gain a bonus to AC and saving throws equal to the penalty taken, a bonus to BAB equal to 2/3rds of this penalty (partially but not completely negating the attack penalty), and a bonus to maximum hit points equal to 10 * penalty taken. Arcana Golems take an additional -2 penalty to Intelligence, * Wisdom, and Charisma while in Spell Battle.\r\nSpell battle without any arguments will cancel the mode, but an Arcana Golem can only cancel Spell Battle after spending 6 minutes in it. The surge of energy is not easily turned off.\r\nYou cannot use Spell-Battle at the same time you use Power Attack, Combat Expertise, or similar effects");

  /* Shared - Various */
  feato(FEAT_DARKVISION, "darkvision", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY, "ask staff", "ask staff");
  feato(FEAT_LOW_LIGHT_VISION, "low light vision", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY, "can see in the dark outside only", "can see in the dark outside only");

  /* End Racial ability feats */

  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */

  /* self explanatory */
  feato(FEAT_LAST_FEAT, "do not take me", FALSE, FALSE, FALSE, FEAT_TYPE_NONE, "placeholder feat", "placeholder feat");

  /****************************************************************************/
  /* some more assigning */

  /* Combat Feats */
  combatfeat(FEAT_IMPROVED_CRITICAL);
  combatfeat(FEAT_WEAPON_FINESSE);
  combatfeat(FEAT_WEAPON_FOCUS);
  combatfeat(FEAT_WEAPON_SPECIALIZATION);
  combatfeat(FEAT_GREATER_WEAPON_FOCUS);
  combatfeat(FEAT_GREATER_WEAPON_SPECIALIZATION);
  combatfeat(FEAT_IMPROVED_WEAPON_FINESSE);
  combatfeat(FEAT_MONKEY_GRIP);
  combatfeat(FEAT_POWER_CRITICAL);
  combatfeat(FEAT_WEAPON_FLURRY);
  combatfeat(FEAT_WEAPON_SUPREMACY);
  /*epic combat feat*/combatfeat(FEAT_EPIC_WEAPON_SPECIALIZATION);

  /* Epic Feats */
  epicfeat(FEAT_EPIC_WEAPON_SPECIALIZATION);
  epicfeat(FEAT_EPIC_PROWESS);
  epicfeat(FEAT_SWARM_OF_ARROWS);
  epicfeat(FEAT_SELF_CONCEALMENT);
  epicfeat(FEAT_INTENSIFY_SPELL);
  epicfeat(FEAT_EPIC_SPELLCASTING);
  epicfeat(FEAT_EPIC_SKILL_FOCUS);
  epicfeat(FEAT_EPIC_DODGE);
  epicfeat(FEAT_EPIC_COMBAT_CHALLENGE);
  epicfeat(FEAT_EPIC_TOUGHNESS);
  epicfeat(FEAT_GREAT_STRENGTH);
  epicfeat(FEAT_GREAT_CONSTITUTION);
  epicfeat(FEAT_GREAT_DEXTERITY);
  epicfeat(FEAT_GREAT_WISDOM);
  epicfeat(FEAT_GREAT_INTELLIGENCE);
  epicfeat(FEAT_GREAT_CHARISMA);
  epicfeat(FEAT_DAMAGE_REDUCTION);
  epicfeat(FEAT_ARMOR_SKIN);
  epicfeat(FEAT_FAST_HEALING);
  epicfeat(FEAT_ENHANCE_SPELL);
  epicfeat(FEAT_GREAT_SMITING);
  epicfeat(FEAT_PERFECT_TWO_WEAPON_FIGHTING);
  epicfeat(FEAT_SNEAK_ATTACK);
  epicfeat(FEAT_SNEAK_ATTACK_OF_OPPORTUNITY);
  epicfeat(FEAT_AUTOMATIC_QUICKEN_SPELL);
  epicfeat(FEAT_IMPROVED_SPELL_RESISTANCE);
  epicfeat(FEAT_BLINDING_SPEED);
  /* epic spell feats */
  epicfeat(FEAT_MUMMY_DUST);
  epicfeat(FEAT_DRAGON_KNIGHT);
  epicfeat(FEAT_GREATER_RUIN);
  epicfeat(FEAT_HELLBALL);
  epicfeat(FEAT_EPIC_MAGE_ARMOR);
  epicfeat(FEAT_EPIC_WARDING);

  epicfeat(FEAT_LAST_FEAT);

  /* Feats with "Daily Use" Mechanic, make sure to add to
   * EVENTFUNC(event_daily_use_cooldown) and mud_event.c */
  dailyfeat(FEAT_QUIVERING_PALM, eQUIVERINGPALM);
  dailyfeat(FEAT_STUNNING_FIST, eSTUNNINGFIST);
  dailyfeat(FEAT_ANIMATE_DEAD, eANIMATEDEAD);
  dailyfeat(FEAT_WILD_SHAPE, eWILD_SHAPE);
  dailyfeat(FEAT_CRYSTAL_BODY, eCRYSTALBODY);
  dailyfeat(FEAT_CRYSTAL_FIST, eCRYSTALFIST);
  dailyfeat(FEAT_LAYHANDS, eLAYONHANDS);
  dailyfeat(FEAT_REMOVE_DISEASE, ePURIFY);
  dailyfeat(FEAT_RAGE, eRAGE);
  dailyfeat(FEAT_VANISH, eVANISHED);
  dailyfeat(FEAT_SMITE_EVIL, eSMITE_EVIL);
  dailyfeat(FEAT_SMITE_GOOD, eSMITE_GOOD);
  dailyfeat(FEAT_DESTRUCTIVE_SMITE, eSMITE_DESTRUCTION);
  dailyfeat(FEAT_TURN_UNDEAD, eTURN_UNDEAD);
  dailyfeat(FEAT_LIGHTNING_ARC, eLIGHTNING_ARC);
  dailyfeat(FEAT_ACID_DART, eACID_DART);
  dailyfeat(FEAT_FIRE_BOLT, eFIRE_BOLT);
  dailyfeat(FEAT_ICICLE, eICICLE);
  dailyfeat(FEAT_CURSE_TOUCH, eCURSE_TOUCH);
  dailyfeat(FEAT_DESTRUCTIVE_AURA, eDESTRUCTIVE_AURA);
  dailyfeat(FEAT_EVIL_TOUCH, eEVIL_TOUCH);
  dailyfeat(FEAT_GOOD_TOUCH, eGOOD_TOUCH);
  dailyfeat(FEAT_HEALING_TOUCH, eHEALING_TOUCH);
  dailyfeat(FEAT_EYE_OF_KNOWLEDGE, eEYE_OF_KNOWLEDGE);
  dailyfeat(FEAT_BLESSED_TOUCH, eBLESSED_TOUCH);
  dailyfeat(FEAT_COPYCAT, eCOPYCAT);
  dailyfeat(FEAT_MASS_INVIS, eMASS_INVIS);
  dailyfeat(FEAT_AURA_OF_PROTECTION, eAURA_OF_PROTECTION);
  dailyfeat(FEAT_BATTLE_RAGE, eBATTLE_RAGE);

  /** END **/
}

/* Check to see if ch meets the provided feat prerequisite.
   iarg is for external comparison */
bool meets_prerequisite(struct char_data *ch, struct feat_prerequisite *prereq, int iarg) {
  switch (prereq->prerequisite_type) {
    case FEAT_PREREQ_NONE:
      /* This is a NON-prereq. */
      break;
    case FEAT_PREREQ_ATTRIBUTE:
      switch (prereq->values[0]) {
        case AB_STR:
          if (GET_REAL_STR(ch) < prereq->values[1])
            return FALSE;
          break;
        case AB_DEX:
          if (GET_REAL_DEX(ch) < prereq->values[1])
            return FALSE;
          break;
        case AB_CON:
          if (GET_REAL_CON(ch) < prereq->values[1])
            return FALSE;
          break;
        case AB_WIS:
          if (GET_REAL_WIS(ch) < prereq->values[1])
            return FALSE;
          break;
        case AB_INT:
          if (GET_REAL_INT(ch) < prereq->values[1])
            return FALSE;
          break;
        case AB_CHA:
          if (GET_REAL_CHA(ch) < prereq->values[1])
            return FALSE;
          break;
        default:
          log("SYSERR: meets_prerequisite() - Bad Attribute prerequisite %d", prereq->values[0]);
          return FALSE;
      }
      break;
    case FEAT_PREREQ_CLASS_LEVEL:
      if (CLASS_LEVEL(ch, prereq->values[0]) < prereq->values[1])
        return FALSE;
      break;
    case FEAT_PREREQ_FEAT:
      if (has_feat(ch, prereq->values[0]) < prereq->values[1])
        return FALSE;
      break;
    case FEAT_PREREQ_ABILITY:
      if (GET_ABILITY(ch, prereq->values[0]) < prereq->values[1])
        return FALSE;
      break;
    case FEAT_PREREQ_SPELLCASTING:
      switch (prereq->values[0]) {
        case CASTING_TYPE_NONE:
          if (IS_SPELLCASTER(ch))
            return FALSE;
          break;
        case CASTING_TYPE_ARCANE:
          if (!(IS_WIZARD(ch) ||
                  IS_SORCERER(ch) ||
                  IS_BARD(ch)))
            return FALSE;
          /* This stuff is all messed up - fix. */
          if (prereq->values[2] > 0) {
            if (!(comp_slots(ch, CLASS_WIZARD, prereq->values[2]) == 0 ||
                    comp_slots(ch, CLASS_SORCERER, prereq->values[2]) == 0 ||
                    comp_slots(ch, CLASS_BARD, prereq->values[2]) == 0))
              return FALSE;
          }
          break;
        case CASTING_TYPE_DIVINE:
          if (!(IS_CLERIC(ch) ||
                  IS_DRUID(ch) ||
                  IS_PALADIN(ch) ||
                  IS_RANGER(ch)))
            return FALSE;
          if (prereq->values[2] > 0) {
            if (!(comp_slots(ch, CLASS_CLERIC, prereq->values[2]) == 0 ||
                    comp_slots(ch, CLASS_PALADIN, prereq->values[2]) == 0 ||
                    comp_slots(ch, CLASS_DRUID, prereq->values[2]) == 0 ||
                    comp_slots(ch, CLASS_RANGER, prereq->values[2]) == 0))
              return FALSE;
          }
          break;
        case CASTING_TYPE_ANY:
          if (!IS_SPELLCASTER(ch))
            return FALSE;
          if (prereq->values[2] > 0) {
            if (!(comp_slots(ch, CLASS_WIZARD, prereq->values[2]) == 0 ||
                    comp_slots(ch, CLASS_SORCERER, prereq->values[2]) == 0 ||
                    comp_slots(ch, CLASS_BARD, prereq->values[2]) == 0 ||
                    comp_slots(ch, CLASS_CLERIC, prereq->values[2]) == 0 ||
                    comp_slots(ch, CLASS_PALADIN, prereq->values[2]) == 0 ||
                    comp_slots(ch, CLASS_DRUID, prereq->values[2]) == 0 ||
                    comp_slots(ch, CLASS_RANGER, prereq->values[2]) == 0))
              return FALSE;
          }
          break;
        default:
          log("SYSERR: meets_prerequisite() - Bad Casting Type prerequisite %d", prereq->values[0]);
          return FALSE;
      }

      switch (prereq->values[1]) {
        case PREP_TYPE_NONE:
          return FALSE;
        case PREP_TYPE_PREPARED:
          if (!IS_MEM_BASED_CASTER(ch))
            return FALSE;
          break;
        case PREP_TYPE_SPONTANEOUS:
          if (IS_MEM_BASED_CASTER(ch))
            return FALSE;
          break;
        case PREP_TYPE_ANY:
          break;
        default:
          log("SYSERR: meets_prerequisite() - Bad Preparation type prerequisite %d", prereq->values[1]);
          return FALSE;
      }
      break;
    case FEAT_PREREQ_RACE:
      if (!IS_NPC(ch) && GET_RACE(ch) != prereq->values[0])
        return FALSE;
      break;
    case FEAT_PREREQ_BAB:
      if (BAB(ch) < prereq->values[0])
        return FALSE;
      break;
    case FEAT_PREREQ_CFEAT:
      /*  SPECIAL CASE - You must have a feat, and it must be the cfeat for the chosen weapon. */
      if (iarg && !has_combat_feat(ch, feat_to_cfeat(prereq->values[0]), iarg))
        return FALSE;
      break;
    case FEAT_PREREQ_WEAPON_PROFICIENCY:
      if (iarg && !is_proficient_with_weapon(ch, iarg))
        return FALSE;
      break;
    default:
      log("SYSERR: meets_prerequisite() - Bad prerequisite type %d", prereq->prerequisite_type);
      return FALSE;
  }

  return TRUE;
}

/* The follwing function is used to check if the character satisfies the various prerequisite(s) (if any)
   of a feat in order to learn it. */
int feat_is_available(struct char_data *ch, int featnum, int iarg, char *sarg) {
  struct feat_prerequisite *prereq = NULL;

  if (featnum > NUM_FEATS) /* even valid featnum? */
    return FALSE;

  if (feat_list[featnum].epic == TRUE && !IS_EPIC(ch)) /* epic only feat */
    return FALSE;

  if (has_feat(ch, featnum) && !feat_list[featnum].can_stack) /* stackable? */
    return FALSE;

  if (feat_list[featnum].in_game == FALSE) /* feat in the game at all? */
    return FALSE;

  if (feat_list[featnum].prerequisite_list != NULL) {
    /*  This feat has prerequisites. Traverse the list and check. */
    for (prereq = feat_list[featnum].prerequisite_list; prereq != NULL; prereq = prereq->next) {
      if (meets_prerequisite(ch, prereq, iarg) == FALSE)
        return FALSE;
    }
  } else { /* no pre-requisites */
    switch (featnum) {
      case FEAT_AUTOMATIC_QUICKEN_SPELL:
        if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) < 30)
          return FALSE;
        if (comp_slots(ch, CLASS_SORCERER, 9) > 0)
          return TRUE;
        if (comp_slots(ch, CLASS_WIZARD, 9) > 0)
          return TRUE;
        if (comp_slots(ch, CLASS_CLERIC, 9) > 0)
          return TRUE;
        if (comp_slots(ch, CLASS_DRUID, 9) > 0)
          return TRUE;
        return FALSE;

      case FEAT_INTENSIFY_SPELL:
        if (!has_feat(ch, FEAT_MAXIMIZE_SPELL))
          return FALSE;
        if (!has_feat(ch, FEAT_EMPOWER_SPELL))
          return FALSE;
        if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) < 30)
          return FALSE;
        if (comp_slots(ch, CLASS_SORCERER, 9) > 0)
          return TRUE;
        if (comp_slots(ch, CLASS_WIZARD, 9) > 0)
          return TRUE;
        if (comp_slots(ch, CLASS_CLERIC, 9) > 0)
          return TRUE;
        if (comp_slots(ch, CLASS_DRUID, 9) > 0)
          return TRUE;
        return FALSE;

      case FEAT_SWARM_OF_ARROWS:
        if (ch->real_abils.dex < 23)
          return FALSE;
        if (!has_feat(ch, FEAT_POINT_BLANK_SHOT))
          return FALSE;
        if (!has_feat(ch, FEAT_RAPID_SHOT))
          return FALSE;
        if (has_feat(ch, FEAT_WEAPON_FOCUS)) /* Need to check for BOW... */
          return FALSE;
        return TRUE;

      case FEAT_FAST_HEALING:
        if (ch->real_abils.con < 25)
          return FALSE;
        return TRUE;

      case FEAT_SELF_CONCEALMENT:
        if (GET_ABILITY(ch, ABILITY_STEALTH) < 30)
          return FALSE;
        if (GET_ABILITY(ch, ABILITY_ACROBATICS) < 30)
          return FALSE;
        if (ch->real_abils.dex < 30)
          return FALSE;
        return TRUE;

      case FEAT_ARMOR_SKIN:
        return TRUE;

      case FEAT_ANIMATE_DEAD:
        return TRUE;

      case FEAT_COMBAT_CHALLENGE:
        if (GET_ABILITY(ch, ABILITY_DIPLOMACY) < 5 &&
                GET_ABILITY(ch, ABILITY_INTIMIDATE) < 5 &&
                GET_ABILITY(ch, ABILITY_BLUFF) < 5)
          return false;
        return true;

      case FEAT_BLEEDING_ATTACK:
      case FEAT_POWERFUL_SNEAK:
        if (CLASS_LEVEL(ch, CLASS_ROGUE) > 1)
          return TRUE;
        return FALSE;

      case FEAT_IMPROVED_COMBAT_CHALLENGE:
        if (GET_ABILITY(ch, ABILITY_DIPLOMACY) < 10 &&
                GET_ABILITY(ch, ABILITY_INTIMIDATE) < 10 &&
                GET_ABILITY(ch, ABILITY_BLUFF) < 10)
          return false;
        if (!has_feat(ch, FEAT_COMBAT_CHALLENGE))
          return false;
        return true;

      case FEAT_GREATER_COMBAT_CHALLENGE:
        if (GET_ABILITY(ch, ABILITY_DIPLOMACY) < 15 &&
                GET_ABILITY(ch, ABILITY_INTIMIDATE) < 15 &&
                GET_ABILITY(ch, ABILITY_BLUFF) < 15)
          return false;
        if (!has_feat(ch, FEAT_IMPROVED_COMBAT_CHALLENGE))
          return false;
        return true;

      case FEAT_EPIC_PROWESS:
        if (has_feat(ch, FEAT_EPIC_PROWESS) >= 5)
          return FALSE;
        return TRUE;

      case FEAT_EPIC_COMBAT_CHALLENGE:
        if (GET_ABILITY(ch, ABILITY_DIPLOMACY) < 20 &&
                GET_ABILITY(ch, ABILITY_INTIMIDATE) < 20 &&
                GET_ABILITY(ch, ABILITY_BLUFF) < 20)
          return false;
        if (!has_feat(ch, FEAT_GREATER_COMBAT_CHALLENGE))
          return false;
        return true;

      case FEAT_NATURAL_SPELL:
        if (ch->real_abils.wis < 13)
          return false;
        if (!has_feat(ch, FEAT_WILD_SHAPE))
          return false;
        return true;

      case FEAT_EPIC_DODGE:
        if (ch->real_abils.dex >= 25 && has_feat(ch, FEAT_DODGE) && has_feat(ch, FEAT_DEFENSIVE_ROLL) && GET_ABILITY(ch, ABILITY_ACROBATICS) >= 30)
          return TRUE;
        return FALSE;

      case FEAT_IMPROVED_SNEAK_ATTACK:
        if (has_feat(ch, FEAT_SNEAK_ATTACK) >= 8)
          return TRUE;
        return FALSE;

      case FEAT_SNEAK_ATTACK:
        if (has_feat(ch, FEAT_SNEAK_ATTACK) < 8)
          return FALSE;
        return TRUE;

      case FEAT_SNEAK_ATTACK_OF_OPPORTUNITY:
        if (has_feat(ch, FEAT_SNEAK_ATTACK) < 8)
          return FALSE;
        if (!has_feat(ch, FEAT_OPPORTUNIST))
          return FALSE;
        return TRUE;

      case FEAT_STEADFAST_DETERMINATION:
        if (!has_feat(ch, FEAT_ENDURANCE))
          return FALSE;
        return TRUE;

      case FEAT_GREAT_SMITING:
        if (ch->real_abils.cha >= 25 && has_feat(ch, FEAT_SMITE_EVIL))
          return TRUE;
        return FALSE;

      case FEAT_DIVINE_MIGHT:
      case FEAT_DIVINE_SHIELD:
        if (has_feat(ch, FEAT_TURN_UNDEAD) && has_feat(ch, FEAT_POWER_ATTACK) &&
                ch->real_abils.cha >= 13 && ch->real_abils.str >= 13)
          return TRUE;
        return FALSE;

      case FEAT_DIVINE_VENGEANCE:
        if (has_feat(ch, FEAT_TURN_UNDEAD) && has_feat(ch, FEAT_EXTRA_TURNING))
          return TRUE;
        return FALSE;

      case FEAT_IMPROVED_EVASION:
      case FEAT_CRIPPLING_STRIKE:
      case FEAT_DEFENSIVE_ROLL:
      case FEAT_OPPORTUNIST:
        if (CLASS_LEVEL(ch, CLASS_ROGUE) < 10)
          return FALSE;
        return TRUE;

      case FEAT_EMPOWERED_MAGIC:
      case FEAT_ENHANCED_SPELL_DAMAGE:
        if (IS_SPELLCASTER(ch))
          return TRUE;
        return FALSE;

      case FEAT_AUGMENT_SUMMONING:
        if (has_feat(ch, FEAT_SPELL_FOCUS) && HAS_SCHOOL_FEAT(ch, feat_to_sfeat(FEAT_SPELL_FOCUS), CONJURATION))
          return TRUE;
        return FALSE;

      case FEAT_FASTER_MEMORIZATION:
        if (IS_CASTER(ch))
          return TRUE;
        return FALSE;

      case FEAT_DAMAGE_REDUCTION:
        /*    if (ch->real_abils.con < 21)
              return FALSE;
            return TRUE;
         */
        return false;

      case FEAT_MONKEY_GRIP:
        /*    if (!iarg)
              return TRUE;
            if (!is_proficient_with_weapon(ch, iarg))
              return FALSE;
            return TRUE;
         */
        return false;

      case FEAT_LAST_FEAT:
        return FALSE;

      case FEAT_SLIPPERY_MIND:
        if (CLASS_LEVEL(ch, CLASS_ROGUE) >= 11)
          return TRUE;
        return FALSE;

      case FEAT_LINGERING_SONG:
      case FEAT_EXTRA_MUSIC:
        if (CLASS_LEVEL(ch, CLASS_BARD) > 0)
          return TRUE;
        return FALSE;

      case FEAT_EXTEND_RAGE:
      case FEAT_EXTRA_RAGE:
        if (has_feat(ch, FEAT_RAGE))
          return TRUE;
        return FALSE;

      case FEAT_ABLE_LEARNER:
        return TRUE;

      case FEAT_FAVORED_ENEMY:
        if (has_feat(ch, FEAT_FAVORED_ENEMY_AVAILABLE))
          return TRUE;
        return FALSE;

      case FEAT_IMPROVED_INTIMIDATION:
        if (GET_ABILITY(ch, ABILITY_INTIMIDATE) < 10)
          return FALSE;
        return TRUE;

      case FEAT_IMPROVED_INSTIGATION:
        if (GET_ABILITY(ch, ABILITY_DIPLOMACY) < 10)
          return FALSE;
        return TRUE;

      case FEAT_IMPROVED_TAUNTING:
        if (GET_ABILITY(ch, ABILITY_DIPLOMACY) < 10)
          return FALSE;
        return TRUE;

      case FEAT_TWO_WEAPON_DEFENSE:
        if (!has_feat(ch, FEAT_TWO_WEAPON_FIGHTING))
          return FALSE;
        if (ch->real_abils.dex < 15)
          return FALSE;
        return TRUE;

      case FEAT_IMPROVED_FEINT:
        if (!has_feat(ch, FEAT_COMBAT_EXPERTISE))
          return false;
        return true;

      case FEAT_AURA_OF_GOOD:
        if (CLASS_LEVEL(ch, CLASS_PALADIN))
          return true;
        return false;

      case FEAT_DETECT_EVIL:
        if (CLASS_LEVEL(ch, CLASS_PALADIN))
          return true;
        return false;

      case FEAT_SMITE_EVIL:
        if (CLASS_LEVEL(ch, CLASS_PALADIN))
          return true;
        return false;

      case FEAT_DIVINE_GRACE:
        if (CLASS_LEVEL(ch, CLASS_PALADIN) > 1)
          return true;
        return false;

      case FEAT_LAYHANDS:
        if (CLASS_LEVEL(ch, CLASS_PALADIN) > 1)
          return true;
        return false;

      case FEAT_AURA_OF_COURAGE:
        if (CLASS_LEVEL(ch, CLASS_PALADIN) > 2)
          return true;
        return false;

      case FEAT_DIVINE_HEALTH:
        if (CLASS_LEVEL(ch, CLASS_PALADIN) > 2)
          return true;
        return false;

      case FEAT_TURN_UNDEAD:
        if (CLASS_LEVEL(ch, CLASS_PALADIN) > 3 || CLASS_LEVEL(ch, CLASS_CLERIC))
          return true;
        return false;

      case FEAT_REMOVE_DISEASE:
        if (CLASS_LEVEL(ch, CLASS_PALADIN) > 5)
          return true;
        return false;

      case FEAT_ARMOR_PROFICIENCY_HEAVY:
        if (has_feat(ch, FEAT_ARMOR_PROFICIENCY_MEDIUM))
          return TRUE;
        return FALSE;

      case FEAT_ARMOR_PROFICIENCY_MEDIUM:
        if (has_feat(ch, FEAT_ARMOR_PROFICIENCY_LIGHT))
          return TRUE;
        return FALSE;

      case FEAT_DODGE:
        if (ch->real_abils.dex >= 13)
          return TRUE;
        return FALSE;

      case FEAT_MOBILITY:
        if (has_feat(ch, FEAT_DODGE))
          return TRUE;
        return FALSE;

      case FEAT_WEAPON_PROFICIENCY_BASTARD_SWORD:
        if (BAB(ch) >= 1)
          return TRUE;
        return FALSE;

      case FEAT_IMPROVED_DISARM:
        if (has_feat(ch, FEAT_COMBAT_EXPERTISE) &&
            ch->real_abils.intel >= 13)
          return TRUE;
        return FALSE;

      case FEAT_GREATER_DISARM:
        if (has_feat(ch, FEAT_IMPROVED_DISARM) &&
            has_feat(ch, FEAT_COMBAT_EXPERTISE) &&
            ch->real_abils.intel >= 13)
          return TRUE;
        return FALSE;

      case FEAT_IMPROVED_TRIP:
        if (has_feat(ch, FEAT_COMBAT_EXPERTISE))
          return TRUE;
        return FALSE;

      case FEAT_WHIRLWIND_ATTACK:
        if (!has_feat(ch, FEAT_DODGE))
          return FALSE;
        if (!has_feat(ch, FEAT_MOBILITY))
          return FALSE;
        if (!has_feat(ch, FEAT_SPRING_ATTACK))
          return FALSE;
        if (ch->real_abils.intel < 13)
          return FALSE;
        if (ch->real_abils.dex < 13)
          return FALSE;
        if (BAB(ch) < 4)
          return FALSE;
        return TRUE;

      case FEAT_STUNNING_FIST:
        if (has_feat(ch, FEAT_IMPROVED_UNARMED_STRIKE) && ch->real_abils.wis >= 13 && ch->real_abils.dex >= 13 && BAB(ch) >= 8)
          return TRUE;
        if (CLASS_LEVEL(ch, CLASS_MONK) > 0)
          return TRUE;
        return FALSE;

      case FEAT_POWER_ATTACK:
        if (ch->real_abils.str >= 13)
          return TRUE;
        return FALSE;

      case FEAT_CLEAVE:
        if (has_feat(ch, FEAT_POWER_ATTACK))
          return TRUE;
        return FALSE;

      case FEAT_GREAT_CLEAVE:
        if (has_feat(ch, FEAT_POWER_ATTACK) &&
                has_feat(ch, FEAT_CLEAVE) &&
                (BAB(ch) >= 4) &&
                (ch->real_abils.str >= 13))
          return TRUE;
        else return FALSE;

      case FEAT_SUNDER:
        if (has_feat(ch, FEAT_POWER_ATTACK))
          return TRUE;
        return FALSE;

      case FEAT_TWO_WEAPON_FIGHTING:
        if (ch->real_abils.dex >= 15)
          return TRUE;
        return FALSE;

      case FEAT_IMPROVED_TWO_WEAPON_FIGHTING:
        if (ch->real_abils.dex >= 17 && has_feat(ch, FEAT_TWO_WEAPON_FIGHTING) && BAB(ch) >= 6)
          return TRUE;
        return FALSE;

      case FEAT_GREATER_TWO_WEAPON_FIGHTING:
        if (ch->real_abils.dex >= 19 && has_feat(ch, FEAT_TWO_WEAPON_FIGHTING) &&
            has_feat(ch, FEAT_IMPROVED_TWO_WEAPON_FIGHTING) && BAB(ch) >= 11)
          return TRUE;
        return FALSE;

      case FEAT_PERFECT_TWO_WEAPON_FIGHTING:
        if (ch->real_abils.dex >= 21 && has_feat(ch, FEAT_GREATER_TWO_WEAPON_FIGHTING))
          return TRUE;
        return FALSE;

      case FEAT_BLINDING_SPEED:
        if (ch->real_abils.dex >= 25)
          return TRUE;
        return FALSE;

      case FEAT_IMPROVED_CRITICAL:
        if (BAB(ch) < 8)
          return FALSE;
        if (!iarg || is_proficient_with_weapon(ch, iarg))
          return TRUE;
        return FALSE;

      case FEAT_POWER_CRITICAL:
        if (BAB(ch) < 4)
          return FALSE;
        if (!iarg || has_combat_feat(ch, CFEAT_WEAPON_FOCUS, iarg))
          return TRUE;
        return FALSE;
/*
          case FEAT_WEAPON_FLURRY:
            if (BAB(ch) < 14)
              return FALSE;
            if (!iarg)
              return TRUE;
            if (!is_proficient_with_weapon(ch, iarg))
              return FALSE;
            if (!has_combat_feat(ch, CFEAT_WEAPON_FOCUS, iarg))
              return FALSE;
            if (!has_combat_feat(ch, CFEAT_WEAPON_SPECIALIZATION, iarg))
              return FALSE;
            if (!has_combat_feat(ch, CFEAT_WEAPON_MASTERY, iarg))
              return FALSE;
            return TRUE;

          case FEAT_WEAPON_SUPREMACY:
            if (CLASS_LEVEL(ch, CLASS_WARRIOR) < 17)
              return FALSE;
            if (!iarg)
              return TRUE;
            if (!is_proficient_with_weapon(ch, iarg))
              return FALSE;
            if (!has_combat_feat(ch, CFEAT_WEAPON_FOCUS, iarg))
              return FALSE;
            if (!has_combat_feat(ch, CFEAT_WEAPON_SPECIALIZATION, iarg))
              return FALSE;
            if (!has_combat_feat(ch, CFEAT_GREATER_WEAPON_FOCUS, iarg))
              return FALSE;
            if (!has_combat_feat(ch, CFEAT_GREATER_WEAPON_SPECIALIZATION, iarg))
              return FALSE;
            if (!has_combat_feat(ch, CFEAT_WEAPON_MASTERY, iarg))
              return FALSE;
            return TRUE;

          case FEAT_ROBILARS_GAMBIT:
            if (!HAS_REAL_FEAT(ch, FEAT_COMBAT_REFLEXES))
              return FALSE;
            if (BAB(ch) < 12)
              return FALSE;
            return TRUE;

          case FEAT_KNOCKDOWN:
            if (!HAS_REAL_FEAT(ch, FEAT_IMPROVED_TRIP))
              return FALSE;
            if (BAB(ch) < 4)
              return FALSE;
            return TRUE;
         */
      case FEAT_ARMOR_SPECIALIZATION_LIGHT:
        if (!has_feat(ch, FEAT_ARMOR_PROFICIENCY_LIGHT))
          return FALSE;
        if (BAB(ch) < 12)
          return FALSE;
        return TRUE;

      case FEAT_ARMOR_SPECIALIZATION_MEDIUM:
        if (!has_feat(ch, FEAT_ARMOR_PROFICIENCY_MEDIUM))
          return FALSE;
        if (BAB(ch) < 12)
          return FALSE;
        return TRUE;

      case FEAT_ARMOR_SPECIALIZATION_HEAVY:
        if (!has_feat(ch, FEAT_ARMOR_PROFICIENCY_HEAVY))
          return FALSE;
        if (BAB(ch) < 12)
          return FALSE;
        return TRUE;

      case FEAT_WEAPON_FINESSE:
        if (BAB(ch) < 1)
          return FALSE;
        return TRUE;
        /*
          case FEAT_WEAPON_FOCUS:
            if (BAB(ch) < 1)
              return FALSE;
            if (!iarg || is_proficient_with_weapon(ch, iarg))
              return TRUE;
            return FALSE;
          case FEAT_GREATER_WEAPON_FOCUS:
            if (CLASS_LEVEL(ch, CLASS_WARRIOR) < 8)
              return FALSE;
            if (!iarg)
              return TRUE;
            if (is_proficient_with_weapon(ch, iarg) && has_combat_feat(ch, CFEAT_WEAPON_FOCUS, iarg))
              return TRUE;
            return FALSE;

          case FEAT_EPIC_SKILL_FOCUS:
            if (!iarg)
              return TRUE;
            if (GET_ABILITY(ch, iarg) >= 20)
              return TRUE;
            return FALSE;

          case  FEAT_IMPROVED_WEAPON_FINESSE:
             if (!has_feat(ch, FEAT_WEAPON_FINESSE))
               return FALSE;
             if (BAB(ch) < 4)
                  return FALSE;
                if (!iarg)
                  return TRUE;
             if (!has_combat_feat(ch, CFEAT_WEAPON_FOCUS, iarg))
                  return FALSE;
                if (weapon_list[iarg].size >= get_size(ch))
                  return FALSE;

             return TRUE;
         */
      case FEAT_WEAPON_SPECIALIZATION:
        if (BAB(ch) < 4 || CLASS_LEVEL(ch, CLASS_WARRIOR) < 4)
          return FALSE;
        if (!iarg || is_proficient_with_weapon(ch, iarg))
          return TRUE;
        return FALSE;
      case FEAT_GREATER_WEAPON_SPECIALIZATION:
        if (CLASS_LEVEL(ch, CLASS_WARRIOR) < 12)
          return FALSE;
        if (!iarg)
          return TRUE;
        if (is_proficient_with_weapon(ch, iarg) &&
                has_combat_feat(ch, CFEAT_GREATER_WEAPON_FOCUS, iarg) &&
                has_combat_feat(ch, CFEAT_WEAPON_SPECIALIZATION, iarg) &&
                has_combat_feat(ch, CFEAT_WEAPON_FOCUS, iarg))
          return TRUE;
        return FALSE;
      case FEAT_EPIC_WEAPON_SPECIALIZATION:
        if (CLASS_LEVEL(ch, CLASS_WARRIOR) < 20)
          return FALSE;
        if (!iarg)
          return TRUE;
        if (is_proficient_with_weapon(ch, iarg) &&
                has_combat_feat(ch, FEAT_GREATER_WEAPON_SPECIALIZATION, iarg) )
          return TRUE;
        return FALSE;

      case FEAT_SPELL_FOCUS:
        if (CLASS_LEVEL(ch, CLASS_WIZARD))
          return TRUE;
        return FALSE;
      case FEAT_GREATER_SPELL_FOCUS:
        if (CLASS_LEVEL(ch, CLASS_WIZARD) && has_feat(ch, FEAT_SPELL_FOCUS))
          return TRUE;
        return FALSE;

      case FEAT_MUMMY_DUST:
        if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) >= 23 && CASTER_LEVEL(ch) >= 20)
          return TRUE;
        return FALSE;
      case FEAT_DRAGON_KNIGHT:
        if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) >= 25 && CASTER_LEVEL(ch) >= 21 &&
            (CLASS_LEVEL(ch, CLASS_WIZARD) > 17 ||
              CLASS_LEVEL(ch, CLASS_SORCERER) > 19))
          return TRUE;
        return FALSE;
      case FEAT_GREATER_RUIN:
        if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) >= 27 && CASTER_LEVEL(ch) >= 22)
          return TRUE;
        return FALSE;
      case FEAT_HELLBALL:
        if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) >= 29 && CASTER_LEVEL(ch) >= 23 &&
              (CLASS_LEVEL(ch, CLASS_WIZARD) > 16 ||
              CLASS_LEVEL(ch, CLASS_SORCERER) > 18))
          return TRUE;
        return FALSE;
      case FEAT_EPIC_MAGE_ARMOR:
        if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) >= 31 && CASTER_LEVEL(ch) >= 24 &&
            (CLASS_LEVEL(ch, CLASS_WIZARD) > 13 ||
             CLASS_LEVEL(ch, CLASS_SORCERER) > 13))
          return TRUE;
        return FALSE;
      case FEAT_EPIC_WARDING:
        if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) >= 32 && CASTER_LEVEL(ch) >= 25 &&
            (CLASS_LEVEL(ch, CLASS_WIZARD) > 15 ||
             CLASS_LEVEL(ch, CLASS_SORCERER) > 15))
          return TRUE;
        return FALSE;

      case FEAT_IMPROVED_FAMILIAR:
        if (CLASS_LEVEL(ch, CLASS_WIZARD) || CLASS_LEVEL(ch, CLASS_SORCERER))
          return TRUE;
        return FALSE;

      case FEAT_SPELL_PENETRATION:
        if (CASTER_LEVEL(ch))
          return TRUE;
        return FALSE;

      case FEAT_BREW_POTION:
        if (CASTER_LEVEL(ch) >= 3)
          return TRUE;
        return FALSE;

      case FEAT_CRAFT_MAGICAL_ARMS_AND_ARMOR:
        if (GET_LEVEL(ch) >= 5)
          return TRUE;
        return FALSE;

      case FEAT_CRAFT_ROD:
        if (CASTER_LEVEL(ch) >= 9)
          return TRUE;
        return FALSE;

      case FEAT_CRAFT_STAFF:
        if (CASTER_LEVEL(ch) >= 12)
          return TRUE;
        return FALSE;

      case FEAT_CRAFT_WAND:
        if (CASTER_LEVEL(ch) >= 5)
          return TRUE;
        return FALSE;

      case FEAT_FORGE_RING:
        if (GET_LEVEL(ch) >= 5)
          return TRUE;
        return FALSE;

      case FEAT_SCRIBE_SCROLL:
        if (CASTER_LEVEL(ch) >= 1)
          return TRUE;
        return FALSE;

      case FEAT_EXTEND_SPELL:
        if (IS_SPELLCASTER(ch))
          return TRUE;
        return FALSE;

      case FEAT_HEIGHTEN_SPELL:
        if (CLASS_LEVEL(ch, CLASS_WIZARD))
          return TRUE;
        return FALSE;

      case FEAT_MAXIMIZE_SPELL:
      case FEAT_EMPOWER_SPELL:
        if (IS_SPELLCASTER(ch))
          return TRUE;
        return FALSE;

      case FEAT_QUICKEN_SPELL:
        if (CLASS_LEVEL(ch, CLASS_WIZARD))
          return TRUE;
        return FALSE;

      case FEAT_SILENT_SPELL:
        if (CLASS_LEVEL(ch, CLASS_WIZARD))
          return TRUE;
        return FALSE;

      case FEAT_STILL_SPELL:
        if (CLASS_LEVEL(ch, CLASS_WIZARD))
          return TRUE;
        return FALSE;

      case FEAT_EXTRA_TURNING:
        if (CLASS_LEVEL(ch, CLASS_CLERIC))
          return TRUE;
        return FALSE;

      case FEAT_SPELL_MASTERY:
        if (CLASS_LEVEL(ch, CLASS_WIZARD))
          return TRUE;
        return FALSE;

      case FEAT_IMPROVED_SPELL_RESISTANCE:
        if (has_feat(ch, FEAT_DIAMOND_SOUL))
          return TRUE;
        return FALSE;

      case FEAT_IMPROVED_SHIELD_PUNCH:
        if (has_feat(ch, FEAT_ARMOR_PROFICIENCY_SHIELD))
          return TRUE;
        return FALSE;

      case FEAT_SHIELD_CHARGE:
        if (!has_feat(ch, FEAT_IMPROVED_SHIELD_PUNCH) ||
                (BAB(ch) < 3))
          return FALSE;
        return TRUE;

      case FEAT_SHIELD_SLAM:
        if (!has_feat(ch, FEAT_SHIELD_CHARGE) ||
                !has_feat(ch, FEAT_IMPROVED_SHIELD_PUNCH) ||
                (BAB(ch) < 6))
          return FALSE;
        return TRUE;

        /* we're going to assume at this stage that this feat truly has
         no prerequisites */
      default:
        return TRUE;
    }
  }
  return TRUE;
}

/*
 *  --------------------------------Known Feats-------------------------------------
 *  Heavy Armor Proficiency       : You are proficient with heavy armor.
 *  Light Armor Proficiency       : You are proficient with light armor.
 *  Medium Armor Proficiency      : You are proficient with medium armor.
 *  Simple Weapon Proficiency     : You are proficient with simple weapons.
 *  Martial Weapon Proficiency    : You are proficient with martial weapons.
 *  Shield Proficiency            : You are proficient with shields and bucklers.
 *  Tower Shield Proficiency      : You are proficient with tower shields.
 *  Power Attack                  : You can make exceptionally powerful attacks.
 *  Cleave                        : You can follow through with powerful blows.
 *  Great Cleave                  : You can slay multiple enemies with each strike.
 *  Tongue of the Sun and the Moon: You can speak any language.
 *  Stacking Feat                 : This feat stacks.
 *  Stacking Feat                 : This feat stacks.
 *  --------------------------------------------------------------------------------
 *
 *  The short description of the feats must not be longer than 47 characters.
 *
 *  --------------------------------Known Feats-------------------------------------
 *  Heavy Armor Proficiency             Simple Weapon Proficiency
 *  Light Armor Proficiency             Martial Weapon Proficiency
 *  Medium Armor Proficiency            Shield Proficiency
 *  Shield Proficiency                  Stackable Feat
 *  Stackable Feat                      Weapon Focus (Greatsword)
 *  --------------------------------------------------------------------------------
 */
void list_feats(struct char_data *ch, char *arg, int list_type, struct char_data *viewer) {
  int i, sortpos, j;
  int none_shown = TRUE;
  int mode = 0;
  char buf [MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH], buf3[150];
  int count = 0;
  int subfeat;
  int line_length = 80; /* Width of the display. */

  if (*arg && is_abbrev(arg, "descriptions")) {
    mode = 1;
  }

  /* Header bar */
  if (list_type == LIST_FEATS_KNOWN)
    sprintf(buf + strlen(buf), "\tC%s\tn", text_line_string("\tYKnown Feats\tC", line_length, '-', '-'));
  if (list_type == LIST_FEATS_AVAILABLE)
    sprintf(buf + strlen(buf), "\tC%s\tn", text_line_string("\tYAvailable Feats\tC", line_length, '-', '-'));
  if (list_type == LIST_FEATS_ALL)
    sprintf(buf + strlen(buf), "\tC%s\tn", text_line_string("\tYAll Feats\tC", line_length, '-', '-'));

  strcpy(buf2, buf);

  for (sortpos = 1; sortpos < NUM_FEATS; sortpos++) {

    if (strlen(buf2) > MAX_STRING_LENGTH - 32)
      break;

    i = feat_sort_info[sortpos];
    /*  Print the feat, depending on the type of list. */
    if (feat_list[i].in_game && (list_type == LIST_FEATS_KNOWN && (has_feat(ch, i)))) {
      if ((subfeat = feat_to_sfeat(i)) != -1) {
        /* This is a 'school feat' */
        for (j = 1; j < NUM_SCHOOLS; j++) {
          if (HAS_SCHOOL_FEAT(ch, subfeat, j)) {
            if (mode == 1) { /* description mode */
              sprintf(buf3, "%s (%s)", feat_list[i].name, spell_schools[j]);
              sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
            } else {
              sprintf(buf3, "%s (%s)", feat_list[i].name, spell_schools[j]);
              sprintf(buf, "%-40s ", buf3);
            }
            strcat(buf2, buf);
            none_shown = FALSE;
          }
        }
      } else if ((subfeat = feat_to_cfeat(i)) != -1) {
        /* This is a 'combat feat' */
        for (j = 1; j < NUM_WEAPON_TYPES; j++) {
          if (HAS_COMBAT_FEAT(ch, subfeat, j)) {
            if (mode == 1) {
              sprintf(buf3, "%s (%s)", feat_list[i].name, weapon_list[j].name);
              sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
            } else {
              sprintf(buf3, "%s (%s)", feat_list[i].name, weapon_list[j].name);
              sprintf(buf, "%-40s ", buf3);
            }
            strcat(buf2, buf);
            none_shown = FALSE;
          }
        }
      } else if ((subfeat = feat_to_skfeat(i)) != -1) {
        /* This is a 'skill' feat */
        for (j = 1; j < NUM_ABILITIES; j++) {
          if (ch->player_specials->saved.skill_focus[i][j] > 0) {
            if (mode == 1) {
              sprintf(buf3, "%s (%s)", feat_list[i].name, ability_names[j]);
              sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
            } else {
              sprintf(buf3, "%s (%s) ", feat_list[i].name, ability_names[j]);
              sprintf(buf, "%-40s ", buf3);
            }
            strcat(buf2, buf);
            none_shown = FALSE;
          }
        }
      } else if (i == FEAT_FAST_HEALING) {
        if (mode == 1) {
          sprintf(buf3, "%s (+%d hp/round)", feat_list[i].name, has_feat(ch, FEAT_FAST_HEALING) * 3);
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (+%d hp/round)", feat_list[i].name, has_feat(ch, FEAT_FAST_HEALING) * 3);
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_DAMAGE_REDUCTION) {
        if (mode == 1) {
          sprintf(buf3, "%s (%d/-)", feat_list[i].name, has_feat(ch, FEAT_DAMAGE_REDUCTION));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (%d/-)", feat_list[i].name, has_feat(ch, FEAT_DAMAGE_REDUCTION));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_SHRUG_DAMAGE) {
        if (mode == 1) {
          sprintf(buf3, "%s (%d/-)", feat_list[i].name, has_feat(ch, FEAT_SHRUG_DAMAGE));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (%d/-)", feat_list[i].name, has_feat(ch, FEAT_SHRUG_DAMAGE));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_ARMOR_SKIN) {
        if (mode == 1) {
          sprintf(buf3, "%s (+%d ac)", feat_list[i].name, has_feat(ch, FEAT_ARMOR_SKIN));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (+%d ac)", feat_list[i].name, has_feat(ch, FEAT_ARMOR_SKIN));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_ARMOR_TRAINING) {
        if (mode == 1) {
          sprintf(buf3, "%s (+%d ranks)", feat_list[i].name, has_feat(ch, FEAT_ARMOR_TRAINING));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (+%d ranks)", feat_list[i].name, has_feat(ch, FEAT_ARMOR_TRAINING));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_WEAPON_TRAINING) {
        if (mode == 1) {
          sprintf(buf3, "%s (+%d ranks)", feat_list[i].name, has_feat(ch, FEAT_WEAPON_TRAINING));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (+%d ranks)", feat_list[i].name, has_feat(ch, FEAT_WEAPON_TRAINING));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_CRITICAL_SPECIALIST) {
        if (mode == 1) {
          sprintf(buf3, "%s (-%d threat)", feat_list[i].name, has_feat(ch, FEAT_CRITICAL_SPECIALIST));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (-%d threat)", feat_list[i].name, has_feat(ch, FEAT_CRITICAL_SPECIALIST));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_SLOW_FALL) {
        if (mode == 1) {
          sprintf(buf3, "%s (+%d feet)", feat_list[i].name, 10 * has_feat(ch, FEAT_SLOW_FALL));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (+%d feet)", feat_list[i].name, 10 * has_feat(ch, FEAT_SLOW_FALL));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_EPIC_PROWESS) {
        if (mode == 1) {
          sprintf(buf3, "%s (+%d attack bonus)", feat_list[i].name, has_feat(ch, FEAT_EPIC_PROWESS));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (+%d attack bonus)", feat_list[i].name, has_feat(ch, FEAT_EPIC_PROWESS));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_EPIC_TOUGHNESS) {
        if (mode == 1) {
          sprintf(buf3, "%s (+%d hp)", feat_list[i].name, (has_feat(ch, FEAT_EPIC_TOUGHNESS) * 30));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (+%d hp)", feat_list[i].name, (has_feat(ch, FEAT_EPIC_TOUGHNESS) * 30));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_ENERGY_RESISTANCE) {
        if (mode == 1) {
          sprintf(buf3, "%s (%d/-)", feat_list[i].name, has_feat(ch, FEAT_ENERGY_RESISTANCE) * 3);
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (%d/-)", feat_list[i].name, has_feat(ch, FEAT_ENERGY_RESISTANCE) * 3);
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_HASTE) {
        if (mode == 1) {
          sprintf(buf3, "%s (3x/day)", feat_list[i].name);
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (3x/day)", feat_list[i].name);
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_SACRED_FLAMES) {
        if (mode == 1) {
          sprintf(buf3, "%s (3x/day)", feat_list[i].name);
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (3x/day)", feat_list[i].name);
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_DRAGON_MOUNT_BREATH) {
        if (mode == 1) {
          sprintf(buf3, "%s (%dx/day)", feat_list[i].name, has_feat(ch, FEAT_DRAGON_MOUNT_BREATH));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (%dx/day)", feat_list[i].name, has_feat(ch, FEAT_DRAGON_MOUNT_BREATH));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_DRAGON_MOUNT_BOOST) {
        if (mode == 1) {
          sprintf(buf3, "%s (+%d)", feat_list[i].name, has_feat(ch, FEAT_DRAGON_MOUNT_BOOST));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (+%d)", feat_list[i].name, has_feat(ch, FEAT_DRAGON_MOUNT_BOOST));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_BREATH_WEAPON) {
        if (mode == 1) {
          sprintf(buf3, "%s (%dd8 dmg|%dx/day)", feat_list[i].name, has_feat(ch, FEAT_BREATH_WEAPON), HAS_FEAT(ch, FEAT_BREATH_WEAPON));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (%dd8 dmg|%dx/day)", feat_list[i].name, has_feat(ch, FEAT_BREATH_WEAPON), HAS_FEAT(ch, FEAT_BREATH_WEAPON));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_LEADERSHIP) {
        if (mode == 1) {
          sprintf(buf3, "%s (+%d%% group exp)", feat_list[i].name, 5 * (1 + has_feat(ch, FEAT_LEADERSHIP)));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (+%d%% group exp)", feat_list[i].name, 5 * (1 + has_feat(ch, FEAT_LEADERSHIP)));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_RAGE) {
        if (mode == 1) {
          sprintf(buf3, "%s (%d / day)", feat_list[i].name, has_feat(ch, FEAT_RAGE));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (%d / day)", feat_list[i].name, has_feat(ch, FEAT_RAGE));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_DEFENSIVE_STANCE) {
        if (mode == 1) {
          sprintf(buf3, "%s (%d / day)", feat_list[i].name, has_feat(ch, FEAT_DEFENSIVE_STANCE));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (%d / day)", feat_list[i].name, has_feat(ch, FEAT_DEFENSIVE_STANCE));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_ENHANCED_SPELL_DAMAGE) {
        if (mode == 1) {
          sprintf(buf3, "%s (+%d dam / die)", feat_list[i].name, has_feat(ch, FEAT_ENHANCED_SPELL_DAMAGE));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (+%d dam / die)", feat_list[i].name, has_feat(ch, FEAT_ENHANCED_SPELL_DAMAGE));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_EMPOWERED_MAGIC) {
        if (mode == 1) {
          sprintf(buf3, "%s (+%d to dcs)", feat_list[i].name, has_feat(ch, FEAT_EMPOWERED_MAGIC));
          sprintf(buf, "\tW%-30s\tC:\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (+%d to dcs)", feat_list[i].name, has_feat(ch, FEAT_EMPOWERED_MAGIC));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_ENHANCE_SPELL) {
        if (mode == 1) {
          sprintf(buf3, "%s (+%d dam dice)", feat_list[i].name, has_feat(ch, FEAT_ENHANCE_SPELL) * 5);
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (+%d dam dice)", feat_list[i].name, has_feat(ch, FEAT_ENHANCE_SPELL) * 5);
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_NATURAL_ARMOR_INCREASE) {
        if (mode == 1) {
          sprintf(buf3, "%s (+%d ac)", feat_list[i].name, has_feat(ch, FEAT_NATURAL_ARMOR_INCREASE));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (+%d ac)", feat_list[i].name, has_feat(ch, FEAT_NATURAL_ARMOR_INCREASE));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_GREAT_STRENGTH) {
        if (mode == 1) {
          sprintf(buf3, "%s (+%d)", feat_list[i].name, has_feat(ch, FEAT_GREAT_STRENGTH));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (+%d)", feat_list[i].name, has_feat(ch, FEAT_GREAT_STRENGTH));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_GREAT_DEXTERITY) {
        if (mode == 1) {
          sprintf(buf3, "%s (+%d)", feat_list[i].name, has_feat(ch, FEAT_GREAT_DEXTERITY));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (+%d)", feat_list[i].name, has_feat(ch, FEAT_GREAT_DEXTERITY));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_GREAT_CONSTITUTION) {
        if (mode == 1) {
          sprintf(buf3, "%s (+%d)", feat_list[i].name, has_feat(ch, FEAT_GREAT_CONSTITUTION));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (+%d)", feat_list[i].name, has_feat(ch, FEAT_GREAT_CONSTITUTION));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_GREAT_INTELLIGENCE) {
        if (mode == 1) {
          sprintf(buf3, "%s (+%d)", feat_list[i].name, has_feat(ch, FEAT_GREAT_INTELLIGENCE));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (+%d)", feat_list[i].name, has_feat(ch, FEAT_GREAT_INTELLIGENCE));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_GREAT_WISDOM) {
        if (mode == 1) {
          sprintf(buf3, "%s (+%d)", feat_list[i].name, has_feat(ch, FEAT_GREAT_WISDOM));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (+%d)", feat_list[i].name, has_feat(ch, FEAT_GREAT_WISDOM));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_GREAT_CHARISMA) {
        if (mode == 1) {
          sprintf(buf3, "%s (+%d)", feat_list[i].name, has_feat(ch, FEAT_GREAT_CHARISMA));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (+%d)", feat_list[i].name, has_feat(ch, FEAT_GREAT_CHARISMA));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_POISON_SAVE_BONUS) {
        if (mode == 1) {
          sprintf(buf3, "%s (+%d)", feat_list[i].name, has_feat(ch, FEAT_POISON_SAVE_BONUS));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (+%d)", feat_list[i].name, has_feat(ch, FEAT_POISON_SAVE_BONUS));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_SNEAK_ATTACK) {
        if (mode == 1) {
          sprintf(buf3, "%s (+%dd6)", feat_list[i].name, has_feat(ch, FEAT_SNEAK_ATTACK));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (+%dd6)", feat_list[i].name, has_feat(ch, FEAT_SNEAK_ATTACK));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_ANIMATE_DEAD) {
        if (mode == 1) {
          sprintf(buf3, "%s (+%d)", feat_list[i].name, has_feat(ch, FEAT_ANIMATE_DEAD));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (+%d)", feat_list[i].name, has_feat(ch, FEAT_ANIMATE_DEAD));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_ARMORED_SPELLCASTING) {
        if (mode == 1) {
          sprintf(buf3, "%s (+%d)", feat_list[i].name, has_feat(ch, FEAT_ARMORED_SPELLCASTING));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (+%d)", feat_list[i].name, has_feat(ch, FEAT_ARMORED_SPELLCASTING));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_TRAP_SENSE) {
        if (mode == 1) {
          sprintf(buf3, "%s (+%d)", feat_list[i].name, has_feat(ch, FEAT_TRAP_SENSE));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (+%d)", feat_list[i].name, has_feat(ch, FEAT_TRAP_SENSE));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_SELF_CONCEALMENT) {
        if (mode == 1) {
          sprintf(buf3, "%s (%d%% miss)", feat_list[i].name, has_feat(ch, FEAT_SELF_CONCEALMENT) * 10);
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (%d%% miss)", feat_list[i].name, has_feat(ch, FEAT_SELF_CONCEALMENT) * 10);
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_ENHANCE_ARROW_MAGIC) {
        if (mode == 1) {
          sprintf(buf3, "%s (+%d)", feat_list[i].name, has_feat(ch, FEAT_ENHANCE_ARROW_MAGIC));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (+%d)", feat_list[i].name, has_feat(ch, FEAT_ENHANCE_ARROW_MAGIC));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_FAST_CRAFTER) {
        if (mode == 1) {
          sprintf(buf3, "%s (%d%% less time)", feat_list[i].name, has_feat(ch, FEAT_FAST_CRAFTER) * 10);
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (-%d seconds)", feat_list[i].name, has_feat(ch, FEAT_FAST_CRAFTER) * 10);
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_PROFICIENT_CRAFTER) {
        if (mode == 1) {
          sprintf(buf3, "%s (+%d to checks)", feat_list[i].name, has_feat(ch, FEAT_PROFICIENT_CRAFTER));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%s (+%d to checks)", feat_list[i].name, has_feat(ch, FEAT_PROFICIENT_CRAFTER));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else if (i == FEAT_PROFICIENT_HARVESTER) {
        if (mode == 1) {
          sprintf(buf3, "%s (+%d to checks)", feat_list[i].name, has_feat(ch, FEAT_PROFICIENT_HARVESTER));
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf3, "%-20s (+%d to checks)", feat_list[i].name, has_feat(ch, FEAT_PROFICIENT_HARVESTER));
          sprintf(buf, "%-40s ", buf3);
        }
        strcat(buf2, buf);
        none_shown = FALSE;
      } else {
        if (mode == 1) {
          sprintf(buf3, "%s", feat_list[i].name);
          sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        } else {
          sprintf(buf, "%-40s ", feat_list[i].name);
        }
        strcat(buf2, buf); /* The above, ^ should always be safe to do. */
        none_shown = FALSE;
      }
      /*  If we are not in description mode, split the output up in columns. */
      if (!mode) {
        count++;
        if (count % 2 == 0)
          strcat(buf2, "\r\n");
      }
    } else if (feat_list[i].in_game &&
            ((list_type == LIST_FEATS_ALL) ||
            (list_type == LIST_FEATS_AVAILABLE && (feat_is_available(ch, i, 0, NULL) && feat_list[i].can_learn)))) {

      /* Display a simple list of all feats. */
      if (mode == 1) {
        sprintf(buf3, "%s", feat_list[i].name);
        sprintf(buf, "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
      } else {
        sprintf(buf, "%-40s ", feat_list[i].name);
      }

      strcat(buf2, buf); /* The above, ^ should always be safe to do. */
      none_shown = FALSE;

      /*  If we are not in description mode, split the output up in columns. */
      if (!mode) {
        count++;
        if (count % 2 == 0)
          strcat(buf2, "\r\n");
      }
    }
  }

  if (none_shown) {
    sprintf(buf, "You do not know any feats at this time.\r\n");
    strcat(buf2, buf);
  }

  if (count % 2 == 1) /* Only one feat on last row */
    strcat(buf2, "\r\n");

  strcat(buf2, "\tC");
  strcat(buf2, line_string(line_length, '-', '-'));
  strcat(buf2, "\tDSyntax: feats <known|available|all <description>>\tn\r\n");

  if (!viewer)
    viewer = ch;

  page_string(viewer->desc, buf2, 1);
}

int is_class_feat(int featnum, int class) {
  int i = 0;
  int marker = class_bonus_feats[class][i];

  while (marker != FEAT_UNDEFINED) {
    if (marker == featnum)
      return TRUE;
    marker = class_bonus_feats[class][++i];
  }

  return FALSE;
}

int is_daily_feat(int featnum) {
  return (feat_list[featnum].event != eNULL);
};

int find_feat_num(char *name) {
  int index, ok;
  char *temp, *temp2;
  char first[256], first2[256];

  for (index = 1; index < NUM_FEATS; index++) {
    if (is_abbrev(name, feat_list[index].name))
      return (index);

    ok = TRUE;
    /* It won't be changed, but other uses of this function elsewhere may. */
    temp = any_one_arg((char *) feat_list[index].name, first);
    temp2 = any_one_arg(name, first2);
    while (*first && *first2 && ok) {
      if (!is_abbrev(first2, first))
        ok = FALSE;
      temp = any_one_arg(temp, first);
      temp2 = any_one_arg(temp2, first2);
    }
    if (ok && !*first2)
      return (index);
  }

  return (-1);
}

/* display_feat_info()
 *
 * Show information about a particular feat, dynamically
 * generated to tailor the output to a particular player.
 *
 * Example feat info :
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * Feat    : Cleave
 * Type    : Combat
 * ---------------------------------------------------
 * Prerequisites : Power Attack, Str:13
 * Required for  : Greater Cleave
 * ---------------------------------------------------
 * Benefit : If you deal a creature enough damage to
 * make it drop (typically by dropping it to below 0
 * hit points or killing it), you get an immediate,
 * extra melee attack against another engaged
 * creature.  The extra attack is with the
 * same weapon and at the same bonus as the attack
 * that dropped the previous creature.  You can use
 * this ability once per round.
 *
 * Normal : (none)
 *
 * Special : A fighter may select Cleave as one of his
 * fighter bonus feats.
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 *
 * (NOTE: The headers of the sections above will be colored
 * differently, making them stand out.) */
bool display_feat_info(struct char_data *ch, char *featname) {
  int feat = -1;
  char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];

  //  static int line_length = 57;
  static int line_length = 80;

  skip_spaces(&featname);
  feat = find_feat_num(featname);

  if (feat == -1 || feat_list[feat].in_game == FALSE) {
    /* Not found - Maybe put in a soundex list here? */
    //send_to_char(ch, "Could not find that feat.\r\n");
    return FALSE;
  }

  /* We found the feat, and the feat number is stored in 'feat'. */
  /* Display the feat info, formatted. */
  send_to_char(ch, "\tC\r\n");
  //text_line(ch, "Feat Information", line_length, '-', '-');
  draw_line(ch, line_length, '-', '-');
  send_to_char(ch, "\tcFeat    : \tn%s\r\n"
          "\tcType    : \tn%s\r\n",
          //                   "\tcCommand : \tn%s\r\n",
          feat_list[feat].name,
          feat_types[feat_list[feat].feat_type]
          );
  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');

  /*  Here display the prerequisites */
  if (feat_list[feat].prerequisite_list == NULL) {
    sprintf(buf, "\tCPrerequisites : \tnnone\r\n");
  } else {
    bool first = TRUE;
    struct feat_prerequisite *prereq;

    for (prereq = feat_list[feat].prerequisite_list; prereq != NULL; prereq = prereq->next) {
      if (first) {
        first = FALSE;
        sprintf(buf, "\tcPrerequisites : %s%s%s",
                (meets_prerequisite(ch, prereq, -1) ? "\tn" : "\tr"), prereq->description, "\tn");
      } else {
        sprintf(buf2, ", %s%s%s",
                (meets_prerequisite(ch, prereq, -1) ? "\tn" : "\tr"), prereq->description, "\tn");
        strcat(buf, buf2);
      }
    }
  }
  send_to_char(ch, "%s", strfrmt(buf, line_length, 1, FALSE, FALSE, FALSE));

  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');

  /* This we will need to buffer and wrap so that it will fit in the space provided. */
  sprintf(buf, "\tcDescription : \tn%s\r\n",
          feat_list[feat].description
          );
  send_to_char(ch, strfrmt(buf, line_length, 1, FALSE, FALSE, FALSE));
  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');
  send_to_char(ch, "\tn\r\n");

  return TRUE;
}

/*  do_feats
 *  Overarching command for getting information out of the Luminari Feat system.
 *
 *  feats | feats known  - List all known (learnable) feats
 *  feat info <featname> - Show detailed information about a partcular feat.
 *  feat all             - List all feats.
 *  feat available       - List all feats for which you qualify
 *  feat category <category name> - List all feats in a specific category
 *
 *  Sample output :
 *
 *  feats known
 *  --------------------------------Known Feats-------------------------------------
 *  Heavy Armor Proficiency       : You are proficient with heavy armor.
 *  Light Armor Proficiency       : You are proficient with light armor.
 *  Medium Armor Proficiency      : You are proficient with medium armor.
 *  Simple Weapon Proficiency     : You are proficient with simple weapons.
 *  Martial Weapon Proficiency    : You are proficient with martial weapons.
 *  Shield Proficiency            : You are proficient with shields and bucklers.
 *  Tower Shield Proficiency      : You are proficient with tower shields.
 *  Power Attack                  : You can make exceptionally powerful attacks.
 *  Cleave                        : You can follow through with powerful blows.
 *  Great Cleave                  : You can slay multiple enemies with each strike.
 *  Tongue of the Sun and the Moon: You can speak any language.
 *  --------------------------------------------------------------------------------
 *
 *  --------------------------------Known Feats-------------------------------------
 *  Heavy Armor Proficiency             Simple Weapon Proficiency
 *  Light Armor Proficiency             Martial Weapon Proficiency
 *  Medium Armor Proficiency            Shield Proficiency
 *  Shield Proficiency                  Stackable Feat
 *  Stackable Feat
 *  --------------------------------------------------------------------------------
 *
 *  Use the same format for the other listings, other than info.
 *  */
ACMD(do_feats) {
  char arg[80];
  char arg2[80];
  char *featname;

  /*  Have to process arguments like this
   *  because of the syntax - feat info <featname> */
  featname = one_argument(argument, arg);
  one_argument(featname, arg2);

  if (is_abbrev(arg, "known") || !*arg) {
    list_feats(ch, arg2, LIST_FEATS_KNOWN, ch);
  } else if (is_abbrev(arg, "info")) {

    if (!strcmp(featname, "")) {
      send_to_char(ch, "You must provide the name of a feat.\r\n");
    } else if(!display_feat_info(ch, featname)) {
      send_to_char(ch, "Could not find that feat.\r\n");
    }
  } else if (is_abbrev(arg, "available")) {
    list_feats(ch, arg2, LIST_FEATS_AVAILABLE, ch);
  } else if (is_abbrev(arg, "all")) {
    list_feats(ch, arg2, LIST_FEATS_ALL, ch);
  }
}

int feat_to_cfeat(int feat) {
  switch (feat) {
    case FEAT_IMPROVED_CRITICAL:
      return CFEAT_IMPROVED_CRITICAL;
    case FEAT_POWER_CRITICAL:
      return CFEAT_POWER_CRITICAL;
      //  case FEAT_WEAPON_FINESSE:
      //    return CFEAT_WEAPON_FINESSE;
    case FEAT_WEAPON_FOCUS:
      return CFEAT_WEAPON_FOCUS;
    case FEAT_WEAPON_SPECIALIZATION:
      return CFEAT_WEAPON_SPECIALIZATION;
    case FEAT_GREATER_WEAPON_FOCUS:
      return CFEAT_GREATER_WEAPON_FOCUS;
    case FEAT_GREATER_WEAPON_SPECIALIZATION:
      return CFEAT_GREATER_WEAPON_SPECIALIZATION;
    case FEAT_EPIC_WEAPON_SPECIALIZATION:
      return CFEAT_EPIC_WEAPON_SPECIALIZATION;
    case FEAT_IMPROVED_WEAPON_FINESSE:
      return CFEAT_IMPROVED_WEAPON_FINESSE;
   // case FEAT_EXOTIC_WEAPON_PROFICIENCY:
   //   return CFEAT_EXOTIC_WEAPON_PROFICIENCY;
    case FEAT_MONKEY_GRIP:
      return CFEAT_MONKEY_GRIP;
    case FEAT_WEAPON_FLURRY:
      return CFEAT_WEAPON_FLURRY;
    case FEAT_WEAPON_SUPREMACY:
      return CFEAT_WEAPON_SUPREMACY;
    default:
      return -1;
  }
}

int feat_to_sfeat(int feat) {
  switch (feat) {
    case FEAT_SPELL_FOCUS:
      return SFEAT_SPELL_FOCUS;
    case FEAT_GREATER_SPELL_FOCUS:
      return SFEAT_GREATER_SPELL_FOCUS;
    default:
      return -1;
  }
}

int feat_to_skfeat(int feat) {
  switch (feat) {
    case FEAT_SKILL_FOCUS:
      return SKFEAT_SKILL_FOCUS;
    case FEAT_EPIC_SKILL_FOCUS:
      return SKFEAT_EPIC_SKILL_FOCUS;
    default:
      return -1;
  }
}

/* EOF */
