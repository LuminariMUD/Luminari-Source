/**************************************************************************
 *  File: class.c                                           Part of tbaMUD *
 *  Usage: Source file for class-specific code.                            *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

/** Help buffer the global variable definitions */
#define __CLASS_C__

/* This file attempts to concentrate most of the code which must be changed
 * in order for new classes to be added.  If you're adding a new class, you
 * should go through this entire file from beginning to end and add the
 * appropriate new special cases for your new class. */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "spells.h"
#include "interpreter.h"
#include "constants.h"
#include "act.h"
#include "handler.h"
#include "comm.h"
#include "spells.h"
#include "mud_event.h"
#include "mudlim.h"
#include "feats.h"
#include "class.h"
#include "assign_wpn_armor.h"
#include "pfdefaults.h"
#include "domains_schools.h"


/* Names first */
const char *class_abbrevs[] = {
  "\tmWiz\tn",
  "\tBCle\tn",
  "\twRog\tn",
  "\tRWar\tn",
  "\tgMon\tn",
  "\tGD\tgr\tGu\tn",
  "\trB\tRe\trs\tn",
  "\tMSor\tn",
  "\tWPal\tn",
  "\tYRan\tn",
  "\tCBar\tn",
  "\tcWpM\tn",
  "\n"
};

const char *class_abbrevs_no_color[] = {
  "Wiz",
  "Cle",
  "Rog",
  "War",
  "Mon",
  "Dru",
  "Bes",
  "Sor",
  "Pal",
  "Ran",
  "Bar",
  "WpM",
  "\n"
};

const char *pc_class_types[] = {
  "Wizard",
  "Cleric",
  "Rogue",
  "Warrior",
  "Monk",
  "Druid",
  "Berserker",
  "Sorcerer",
  "Paladin",
  "Ranger",
  "Bard",
  "WeaponMaster",
  "\n"
};

/* The menu for choosing a class in interpreter.c: */
const char *class_menu =
        "\r\n"
        "  \tbRea\tclms \tWof Lu\tcmin\tbari\tn | class selection\r\n"
        "---------------------+\r\n"
        "  c)  \tBCleric\tn\r\n"
        "  t)  \tWRogue\tn\r\n"
        "  w)  \tRWarrior\tn\r\n"
        "  o)  \tgMonk\tn\r\n"
        "  d)  \tGD\tgr\tGu\tgi\tGd\tn\r\n"
        "  m)  \tmWizard\tn\r\n"
        "  b)  \trBer\tRser\trker\tn\r\n"
        "  p)  \tWPaladin\tn\r\n"
        "  s)  \tMSorcerer\tn\r\n"
        "  r)  \tYRanger\tn\r\n"
        "  a)  \tCBard\tn\r\n"
        "  e)  \tcWeaponMaster\tn\r\n";


/* homeland-port */
const char *church_types[] = {
  "Ao",
  "Akadi",
  "Chauntea",
  "Cyric",
  "Grumbar",
  "Istishia", //5
  "Kelemvor",
  "Kossuth",
  "Lathander",
  "Mystra",
  "Oghma", //10
  "Shar",
  "Silvanus",
  "\n"
};
// 14

/* The code to interpret a class letter -- used in interpreter.c when a new
 * character is selecting a class and by 'set class' in act.wizard.c. */
int parse_class(char arg) {
  arg = LOWER(arg);

  switch (arg) {
    case 'm': return CLASS_WIZARD;
    case 'c': return CLASS_CLERIC;
    case 'w': return CLASS_WARRIOR;
    case 't': return CLASS_ROGUE;
    case 'o': return CLASS_MONK;
    case 'd': return CLASS_DRUID;
    case 'b': return CLASS_BERSERKER;
    case 's': return CLASS_SORCERER;
    case 'p': return CLASS_PALADIN;
    case 'r': return CLASS_RANGER;
    case 'a': return CLASS_BARD;
    case 'e': return CLASS_WEAPON_MASTER;
    default: return CLASS_UNDEFINED;
  }
}

/* accept short descrip, return class */
int parse_class_long(char *arg) {
  int l = 0; /* string length */

  for (l = 0; *(arg + l); l++) /* convert to lower case */
    *(arg + l) = LOWER(*(arg + l));

  if (is_abbrev(arg, "wizard")) return CLASS_WIZARD;
  if (is_abbrev(arg, "cleric")) return CLASS_CLERIC;
  if (is_abbrev(arg, "warrior")) return CLASS_WARRIOR;
  if (is_abbrev(arg, "rogue")) return CLASS_ROGUE;
  if (is_abbrev(arg, "monk")) return CLASS_MONK;
  if (is_abbrev(arg, "druid")) return CLASS_DRUID;
  if (is_abbrev(arg, "berserker")) return CLASS_BERSERKER;
  if (is_abbrev(arg, "sorcerer")) return CLASS_SORCERER;
  if (is_abbrev(arg, "paladin")) return CLASS_PALADIN;
  if (is_abbrev(arg, "ranger")) return CLASS_RANGER;
  if (is_abbrev(arg, "bard")) return CLASS_BARD;
  if (is_abbrev(arg, "weaponmaster")) return CLASS_WEAPON_MASTER;
  if (is_abbrev(arg, "weapon-master")) return CLASS_WEAPON_MASTER;

  return CLASS_UNDEFINED;
}

/* bitvectors (i.e., powers of two) for each class, mainly for use in do_who
 * and do_users.  Add new classes at the end so that all classes use sequential
 * powers of two (1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5, etc.) up to
 * the limit of your bitvector_t, typically 0-31. */
bitvector_t find_class_bitvector(const char *arg) {
  size_t rpos, ret = 0;

  for (rpos = 0; rpos < strlen(arg); rpos++)
    ret |= (1 << parse_class(arg[rpos]));

  return (ret);
}

/* These are definitions which control the guildmasters for each class.
 * The  first field (top line) controls the highest percentage skill level a
 * character of the class is allowed to attain in any skill.  (After this
 * level, attempts to practice will say "You are already learned in this area."
 *
 * The second line controls the maximum percent gain in learnedness a character
 * is allowed per practice -- in other words, if the random die throw comes out
 * higher than this number, the gain will only be this number instead.
 *
 * The third line controls the minimu percent gain in learnedness a character
 * is allowed per practice -- in other words, if the random die throw comes
 * out below this number, the gain will be set up to this number.
 *
 * The fourth line simply sets whether the character knows 'spells' or 'skills'.
 * This does not affect anything except the message given to the character when
 * trying to practice (i.e. "You know of the following spells" vs. "You know of
 * the following skills" */

#define SP	0
#define SK	1

/* #define LEARNED_LEVEL	0  % known which is considered "learned" */
/* #define MAX_PER_PRAC		1  max percent gain in skill per practice */
/* #define MIN_PER_PRAC		2  min percent gain in skill per practice */
/* #define PRAC_TYPE		3  should it say 'spell' or 'skill'?	*/

int prac_params[4][NUM_CLASSES] = {
  /* MG  CL  TH WR  MN  DR  BK  SR  PL  RA  BA  WM */
  { 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75}, /* learned level */
  { 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75}, /* max per practice */
  { 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75}, /* min per practice */
  { SK, SK, SK, SK, SK, SK, SK, SK, SK, SK, SK, SK}, /* prac name */
};
#undef SP
#undef SK
/* The appropriate rooms for each guildmaster/guildguard; controls which types
 * of people the various guildguards let through.  i.e., the first line shows
 * that from room 3017, only WIZARDS are allowed to go south. Don't forget
 * to visit spec_assign.c if you create any new mobiles that should be a guild
 * master or guard so they can act appropriately. If you "recycle" the
 * existing mobs that are used in other guilds for your new guild, then you
 * don't have to change that file, only here. Guildguards are now implemented
 * via triggers. This code remains as an example. */
struct guild_info_type guild_info[] = {

  /* Midgaard */
  { CLASS_WIZARD, 3017, SOUTH},
  { CLASS_CLERIC, 3004, NORTH},
  { CLASS_DRUID, 3004, NORTH},
  { CLASS_MONK, 3004, NORTH},
  { CLASS_ROGUE, 3027, EAST},
  { CLASS_BARD, 3027, EAST},
  { CLASS_WARRIOR, 3021, EAST},
  { CLASS_WEAPON_MASTER, 3021, EAST},
  { CLASS_RANGER, 3021, EAST},
  { CLASS_PALADIN, 3021, EAST},
  { CLASS_BERSERKER, 3021, EAST},
  { CLASS_SORCERER, 3017, SOUTH},

  /* Brass Dragon */
  { -999 /* all */, 5065, WEST},

  /* this must go last -- add new guards above! */
  { -1, NOWHERE, -1}
};

/* Maximum ranks that may be taken in each class.
 * -1 indicates no limit to the number of levels in this
 *  class according to epic rules. */
int class_max_ranks[NUM_CLASSES] = {
  /* Wizard       */ -1,
  /* Cleric       */ -1,
  /* Rogue        */ -1,
  /* Warrior      */ -1,
  /* Monk         */ -1,
  /* Druid        */ -1,
  /* Berserker    */ -1,
  /* Sorcerer     */ -1,
  /* Paladin      */ -1,
  /* Ranger       */ -1,
  /* Bard         */ -1,
  /* WeaponMaster */ 10
};

/* This array determines whether an ability is cross-class or a class-ability
 * based on class of the character */
#define		NA	0	//not available
#define		CC	1	//cross class
#define		CA	2	//class ability
int class_ability[NUM_ABILITIES][NUM_CLASSES] = {
//  MU  CL  TH  WA  MO  DR  BZ  SR  PL  RA  BA  WM
  { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, //0 - reserved

  { CC, CC, CA, CC, CA, CC, CC, CC, CC, CC, CA, CC}, //1 - Acrobatics
  { CC, CC, CA, CC, CA, CC, CC, CC, CC, CA, CA, CC}, //2 - hide
  { CC, CC, CA, CC, CA, CC, CC, CC, CC, CA, CA, CC}, //3 move silently
  { CC, CC, CA, CC, CA, CC, CC, CC, CC, CA, CC, CC}, //4 spot
  { CC, CC, CA, CC, CA, CC, CA, CC, CC, CA, CA, CC}, //5 listen
  { CA, CA, CA, CA, CA, CA, CA, CA, CA, CA, CA, CA}, //6 heal
  { CC, CC, CC, CC, CC, CC, CA, CC, CA, CC, CA, CA}, //7 intimidate
  { CA, CA, CC, CA, CA, CA, CC, CA, CA, CA, CA, CA}, //8 concentration
  { CA, CA, CC, CC, CC, CA, CC, CA, CC, CC, CA, CC}, //9 spellcraft
  { CA, CC, CA, CC, CC, CC, CC, CC, CC, CC, CA, CC}, //10 appraise
  { CC, CC, CC, CA, CC, CC, CA, CC, CA, CA, CA, CA}, //11 discipline
  { CC, CA, CA, CA, CA, CA, CA, CC, CA, CA, CA, CA}, //12 total defense
  { CA, CA, CA, CA, CA, CA, CA, CA, CA, CA, CA, CA}, //13 lore
  { CA, CA, CA, CA, CA, CA, CA, CA, CA, CA, CA, CA}, //14 ride
  { CC, CC, CA, CC, CA, CC, CC, CC, CC, CC, CC, CC}, //15 balance
  { CC, CC, CA, CA, CA, CC, CA, CC, CC, CA, CA, CC}, //16 climb
  { CC, CC, CA, CC, CC, CC, CC, CC, CC, CC, CC, CC}, //17 open lock
  { CC, CC, CA, CC, CC, CC, CC, CC, CC, CC, CA, CC}, //18 sleight of hand
  { CA, CC, CA, CC, CC, CC, CC, CC, CC, CA, CC, CC}, //19 search
  { CC, CC, CA, CC, CC, CC, CC, CA, CC, CC, CA, CA}, //20 bluff
  { CA, CC, CA, CC, CC, CC, CC, CC, CC, CC, CA, CC}, //21 decipher script
  { CC, CA, CA, CC, CA, CA, CC, CC, CA, CC, CA, CC}, //22 diplomacy
  { CC, CC, CA, CC, CC, CC, CC, CC, CC, CC, CC, CC}, //23 disable device
  { CC, CC, CA, CC, CC, CC, CC, CC, CC, CC, CA, CC}, //24 disguise
  { CC, CC, CA, CC, CA, CC, CC, CC, CC, CC, CA, CC}, //25 escape artist
  { CC, CC, CC, CA, CC, CA, CA, CC, CA, CA, CC, CC}, //26 handle animal
  { CC, CC, CA, CA, CA, CC, CA, CC, CC, CA, CA, CC}, //27 jump
  { CC, CC, CA, CC, CA, CC, CC, CC, CA, CC, CA, CA}, //28 sense motive
  { CC, CC, CC, CC, CC, CA, CA, CC, CC, CA, CC, CC}, //29 survival
  { CC, CC, CA, CA, CA, CA, CA, CC, CC, CA, CA, CA}, //30 swim
  { CA, CC, CA, CC, CC, CC, CC, CC, CC, CC, CA, CC}, //31 use magic device
  { CC, CC, CA, CC, CC, CC, CC, CC, CC, CA, CC, CC}, //32 use rope
  { CC, CC, CC, CC, CC, CC, CC, CC, CC, CC, CA, CC}, //33 perform
  { CA, CA, CA, CA, CA, CA, CA, CA, CA, CA, CA, CA}, //34 Craft (woodworking)
  { CA, CA, CA, CA, CA, CA, CA, CA, CA, CA, CA, CA}, //35 Craft (weaving)
  { CA, CA, CA, CA, CA, CA, CA, CA, CA, CA, CA, CA}, //36 Craft (alchemy)
  { CA, CA, CA, CA, CA, CA, CA, CA, CA, CA, CA, CA}, //37 Craft (armorsmithing)
  { CA, CA, CA, CA, CA, CA, CA, CA, CA, CA, CA, CA}, //38 Craft (weaponsmithing)
  { CA, CA, CA, CA, CA, CA, CA, CA, CA, CA, CA, CA}, //39 Craft (bowmaking)
  { CA, CA, CA, CA, CA, CA, CA, CA, CA, CA, CA, CA}, //40 Craft (gemcutting)
  { CA, CA, CA, CA, CA, CA, CA, CA, CA, CA, CA, CA}, //41 Craft (leatherworking)
  { CA, CA, CA, CA, CA, CA, CA, CA, CA, CA, CA, CA}, //42 Craft (trapmaking)
  { CA, CA, CA, CA, CA, CA, CA, CA, CA, CA, CA, CA}, //43 Craft (poisonmaking)
  { CA, CA, CA, CA, CA, CA, CA, CA, CA, CA, CA, CA}, //44 Craft (metalworking)
  { CA, CA, CC, CC, CA, CC, CC, CA, CC, CC, CA, CC}, //46 Knowledge (arcana)
  { CA, CC, CC, CC, CC, CC, CC, CC, CC, CC, CA, CC}, //47 Knowledge (engineering)
  { CA, CC, CC, CC, CC, CC, CC, CC, CC, CA, CA, CC}, //48 Knowledge (dungeoneering)
  { CA, CC, CC, CC, CC, CC, CC, CC, CC, CA, CA, CC}, //49 Knowledge (geography)
  { CA, CA, CC, CC, CC, CC, CC, CC, CC, CC, CA, CC}, //50 Knowledge (history)
  { CA, CC, CA, CC, CC, CC, CC, CC, CC, CC, CA, CC}, //51 Knowledge (local)
  { CA, CC, CC, CC, CC, CA, CC, CC, CC, CA, CA, CC}, //52 Knowledge (nature)
  { CA, CC, CC, CC, CC, CC, CC, CC, CA, CC, CA, CC}, //53 Knowledge (nobility)
  { CA, CA, CC, CC, CA, CC, CC, CC, CA, CC, CA, CC}, //54 Knowledge (religion)
  { CA, CA, CC, CC, CC, CC, CC, CC, CC, CC, CA, CC}, //55 Knowledge (the planes)
};
int modify_class_ability(struct char_data *ch, int ability, int class) {
  int ability_value = class_ability[ability][class];

  if (HAS_FEAT(ch, FEAT_DECEPTION)) {
    if (ability == ABILITY_DISGUISE || ability == ABILITY_STEALTH)
      ability_value = CA;
  }

  return ability_value;
}
#undef NA
#undef CC
#undef CA

// Saving Throw System
#define		H	1	//high
#define		L	0	//low
int preferred_save[5][NUM_CLASSES] = {
  //MU CL TH WA MO DR BK SR PL RA BA WM
  /*fort */
  { L, H, L, H, H, H, H, L, H, H, L, L},
  /*refl */
  { L, L, H, L, H, L, L, L, L, L, H, H},
  /*will */
  { H, H, L, L, H, H, L, H, L, L, H, L},
  /*psn  */
  { L, L, L, L, L, L, L, L, L, L, L, L},
  /*death*/
  { L, L, L, L, L, L, L, L, L, L, L, L},
};
// fortitude / reflex / will / ( poison / death )

int free_start_feats_wizard[] = {
  FEAT_WEAPON_PROFICIENCY_WIZARD,
  FEAT_SCRIBE_SCROLL,
  FEAT_SUMMON_FAMILIAR,
  0
};
int free_start_feats_cleric[] = {
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  FEAT_ARMOR_PROFICIENCY_HEAVY,
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_ARMOR_PROFICIENCY_MEDIUM,
  FEAT_ARMOR_PROFICIENCY_SHIELD,
  0
};
int free_start_feats_rogue[] = {
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  FEAT_WEAPON_PROFICIENCY_ROGUE,
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_WEAPON_FINESSE,
  0
};
int free_start_feats_warrior[] = {
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  FEAT_MARTIAL_WEAPON_PROFICIENCY,
  FEAT_ARMOR_PROFICIENCY_HEAVY,
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_ARMOR_PROFICIENCY_MEDIUM,
  FEAT_ARMOR_PROFICIENCY_SHIELD,
  FEAT_ARMOR_PROFICIENCY_TOWER_SHIELD,
  0
};
int free_start_feats_monk[] = {
  FEAT_WEAPON_PROFICIENCY_MONK,
  FEAT_UNARMED_STRIKE,
  FEAT_IMPROVED_UNARMED_STRIKE,
  FEAT_FLURRY_OF_BLOWS,
  FEAT_STUNNING_FIST,
  0
};
int free_start_feats_paladin[] = {
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  FEAT_ARMOR_PROFICIENCY_HEAVY,
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_ARMOR_PROFICIENCY_MEDIUM,
  FEAT_ARMOR_PROFICIENCY_SHIELD,
  FEAT_MARTIAL_WEAPON_PROFICIENCY,
  FEAT_AURA_OF_GOOD,
  FEAT_DETECT_EVIL,
  0
};
int free_start_feats_berserker[] = {
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_ARMOR_PROFICIENCY_MEDIUM,
  FEAT_ARMOR_PROFICIENCY_SHIELD,
  FEAT_MARTIAL_WEAPON_PROFICIENCY,
  FEAT_FAST_MOVEMENT,
  0
};
int free_start_feats_druid[] = {
  FEAT_WEAPON_PROFICIENCY_DRUID,
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_ARMOR_PROFICIENCY_MEDIUM,
  FEAT_ARMOR_PROFICIENCY_SHIELD,
  FEAT_ANIMAL_COMPANION,
  FEAT_NATURE_SENSE,
  0
};
int free_start_feats_bard[] = {
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  FEAT_WEAPON_PROFICIENCY_BARD,
  FEAT_WEAPON_PROFICIENCY_ROGUE,
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_ARMOR_PROFICIENCY_SHIELD,
  0
};
int free_start_feats_sorcerer[] = {
  FEAT_WEAPON_PROFICIENCY_WIZARD,
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  FEAT_SUMMON_FAMILIAR,
  0
};
int free_start_feats_ranger[] = {
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_ARMOR_PROFICIENCY_MEDIUM,
  FEAT_ARMOR_PROFICIENCY_SHIELD,
  FEAT_MARTIAL_WEAPON_PROFICIENCY,
  0
};
int free_start_feats_weaponmaster[] = {
  FEAT_WEAPON_OF_CHOICE,
  0
};
int free_start_feats_none[] = {
  0
};
int *free_start_feats[] = {
  /* CLASS_WIZARD        */ free_start_feats_wizard,
  /* CLASS_CLERIC        */ free_start_feats_cleric,
  /* CLASS_ROGUE         */ free_start_feats_rogue,
  /* CLASS_WARRIOR       */ free_start_feats_warrior,
  /* CLASS_MONK          */ free_start_feats_monk,
  /* CLASS_DRUID         */ free_start_feats_druid,
  /* CLASS_BERSERKER     */ free_start_feats_berserker,
  /* CLASS_SORC          */ free_start_feats_sorcerer,
  /* CLASS_PALADIN       */ free_start_feats_paladin,
  /* CLASS_RANGER        */ free_start_feats_ranger,
  /* CLASS_BARD          */ free_start_feats_bard,
  /* CLASS_WEAPON_MASTER */ free_start_feats_weaponmaster
};

/* Information required for character leveling in regards to free feats
   1) required class
   2) required race
   3) stacks?
   4) level received
   5) feat name */
int level_feats[][LEVEL_FEATS] = {

  /* class, race, stacks?, level, feat_ name */
  /* wizard */
  {CLASS_WIZARD, RACE_UNDEFINED, FALSE, 6, FEAT_SCRIBE_SCROLL},

  /* cleric */
  {CLASS_CLERIC, RACE_UNDEFINED, FALSE, 1, FEAT_TURN_UNDEAD},

  /* warrior */
  /* bonus: they select from a master list of combat feats every 2 levels */
  {CLASS_WARRIOR, RACE_UNDEFINED, TRUE, 3, FEAT_ARMOR_TRAINING},
  {CLASS_WARRIOR, RACE_UNDEFINED, TRUE, 5, FEAT_WEAPON_TRAINING},
  {CLASS_WARRIOR, RACE_UNDEFINED, TRUE, 7, FEAT_ARMOR_TRAINING},
  {CLASS_WARRIOR, RACE_UNDEFINED, TRUE, 9, FEAT_WEAPON_TRAINING},
  {CLASS_WARRIOR, RACE_UNDEFINED, TRUE, 11, FEAT_ARMOR_TRAINING},
  {CLASS_WARRIOR, RACE_UNDEFINED, TRUE, 13, FEAT_WEAPON_TRAINING},
  {CLASS_WARRIOR, RACE_UNDEFINED, TRUE, 15, FEAT_ARMOR_TRAINING},
  {CLASS_WARRIOR, RACE_UNDEFINED, TRUE, 17, FEAT_WEAPON_TRAINING},
  {CLASS_WARRIOR, RACE_UNDEFINED, FALSE, 19, FEAT_STALWART_WARRIOR},
  /* epic */
  {CLASS_WARRIOR, RACE_UNDEFINED, FALSE, 20, FEAT_ARMOR_MASTERY},
  {CLASS_WARRIOR, RACE_UNDEFINED, FALSE, 23, FEAT_WEAPON_MASTERY},
  {CLASS_WARRIOR, RACE_UNDEFINED, FALSE, 26, FEAT_ARMOR_MASTERY_2},
  {CLASS_WARRIOR, RACE_UNDEFINED, FALSE, 29, FEAT_WEAPON_MASTERY_2},


  /* paladin */
  {CLASS_PALADIN, RACE_UNDEFINED, TRUE, 1, FEAT_SMITE_EVIL},
  {CLASS_PALADIN, RACE_UNDEFINED, FALSE, 2, FEAT_DIVINE_GRACE},
  {CLASS_PALADIN, RACE_UNDEFINED, FALSE, 3, FEAT_LAYHANDS},
  {CLASS_PALADIN, RACE_UNDEFINED, FALSE, 3, FEAT_TURN_UNDEAD},
  {CLASS_PALADIN, RACE_UNDEFINED, FALSE, 4, FEAT_AURA_OF_COURAGE},
  {CLASS_PALADIN, RACE_UNDEFINED, FALSE, 5, FEAT_DIVINE_HEALTH},
  /* bonus feat - mounted combat 5 */
  {CLASS_PALADIN, RACE_UNDEFINED, FALSE, 5, FEAT_MOUNTED_COMBAT},
  {CLASS_PALADIN, RACE_UNDEFINED, TRUE, 5, FEAT_SMITE_EVIL},
  /* bonus feat - ride by attack 6 */
  {CLASS_PALADIN, RACE_UNDEFINED, FALSE, 6, FEAT_RIDE_BY_ATTACK},
  {CLASS_PALADIN, RACE_UNDEFINED, TRUE, 6, FEAT_REMOVE_DISEASE},
  {CLASS_PALADIN, RACE_UNDEFINED, FALSE, 7, FEAT_CALL_MOUNT},
  {CLASS_PALADIN, RACE_UNDEFINED, FALSE, 8, FEAT_DIVINE_BOND},
  /* bonus feat - spirited charge 9 */
  {CLASS_PALADIN, RACE_UNDEFINED, TRUE, 9, FEAT_SPIRITED_CHARGE},
  {CLASS_PALADIN, RACE_UNDEFINED, TRUE, 9, FEAT_REMOVE_DISEASE},
  {CLASS_PALADIN, RACE_UNDEFINED, TRUE, 10, FEAT_SMITE_EVIL},
  {CLASS_PALADIN, RACE_UNDEFINED, TRUE, 12, FEAT_REMOVE_DISEASE},
  /* bonus feat - mounted archery 13 */
  {CLASS_PALADIN, RACE_UNDEFINED, FALSE, 13, FEAT_MOUNTED_ARCHERY},
  {CLASS_PALADIN, RACE_UNDEFINED, TRUE, 14, FEAT_REMOVE_DISEASE},
  {CLASS_PALADIN, RACE_UNDEFINED, TRUE, 15, FEAT_SMITE_EVIL},
  {CLASS_PALADIN, RACE_UNDEFINED, TRUE, 18, FEAT_REMOVE_DISEASE},
  /* bonus feat - glorious rider 19 */
  {CLASS_PALADIN, RACE_UNDEFINED, FALSE, 19, FEAT_GLORIOUS_RIDER},
  /* epic */
  {CLASS_PALADIN, RACE_UNDEFINED, TRUE, 20, FEAT_SMITE_EVIL},
  /* bonus epic feat - legendary rider 21 */
  {CLASS_PALADIN, RACE_UNDEFINED, FALSE, 21, FEAT_LEGENDARY_RIDER},
  {CLASS_PALADIN, RACE_UNDEFINED, TRUE, 22, FEAT_REMOVE_DISEASE},
  {CLASS_PALADIN, RACE_UNDEFINED, TRUE, 25, FEAT_SMITE_EVIL},
  {CLASS_PALADIN, RACE_UNDEFINED, TRUE, 26, FEAT_REMOVE_DISEASE},
  /* bonus epic feat - epic mount 27 */
  {CLASS_PALADIN, RACE_UNDEFINED, FALSE, 27, FEAT_EPIC_MOUNT},
  {CLASS_PALADIN, RACE_UNDEFINED, TRUE, 30, FEAT_REMOVE_DISEASE},
  {CLASS_PALADIN, RACE_UNDEFINED, TRUE, 30, FEAT_SMITE_EVIL},

  /* rogue */
  /* if we use pathfinder type rules, rogues should get special
   selections of feats from a talent list every two levels...
   as a temporary (?) solution, every 3 levels we are giving rogues
   hand selected talents */
  /* class, race, stacks?, level, feat-name */
  {CLASS_ROGUE, RACE_UNDEFINED, TRUE, 1, FEAT_SNEAK_ATTACK},
  {CLASS_ROGUE, RACE_UNDEFINED, FALSE, 1, FEAT_TRAPFINDING},
  {CLASS_ROGUE, RACE_UNDEFINED, FALSE, 2, FEAT_EVASION},
  {CLASS_ROGUE, RACE_UNDEFINED, TRUE, 3, FEAT_SNEAK_ATTACK},
  {CLASS_ROGUE, RACE_UNDEFINED, TRUE, 3, FEAT_TRAP_SENSE},
  /* talent lvl 3, slippery mind*/
  {CLASS_ROGUE, RACE_UNDEFINED, FALSE, 3, FEAT_SLIPPERY_MIND},
  {CLASS_ROGUE, RACE_UNDEFINED, FALSE, 4, FEAT_UNCANNY_DODGE},
  {CLASS_ROGUE, RACE_UNDEFINED, TRUE, 5, FEAT_SNEAK_ATTACK},
  {CLASS_ROGUE, RACE_UNDEFINED, TRUE, 6, FEAT_TRAP_SENSE},
  /* talent lvl 6, crippling strike*/
  {CLASS_ROGUE, RACE_UNDEFINED, FALSE, 6, FEAT_CRIPPLING_STRIKE},
  {CLASS_ROGUE, RACE_UNDEFINED, TRUE, 7, FEAT_SNEAK_ATTACK},
  {CLASS_ROGUE, RACE_UNDEFINED, FALSE, 8, FEAT_IMPROVED_UNCANNY_DODGE},
  {CLASS_ROGUE, RACE_UNDEFINED, TRUE, 9, FEAT_SNEAK_ATTACK},
  {CLASS_ROGUE, RACE_UNDEFINED, TRUE, 9, FEAT_TRAP_SENSE},
  /* talent lvl 9, improved evasion*/
  {CLASS_ROGUE, RACE_UNDEFINED, FALSE, 9, FEAT_IMPROVED_EVASION},
  {CLASS_ROGUE, RACE_UNDEFINED, TRUE, 11, FEAT_SNEAK_ATTACK},
  {CLASS_ROGUE, RACE_UNDEFINED, TRUE, 12, FEAT_TRAP_SENSE},
  /* talent lvl 12, apply poison */
  {CLASS_ROGUE, RACE_UNDEFINED, FALSE, 12, FEAT_APPLY_POISON},
  {CLASS_ROGUE, RACE_UNDEFINED, TRUE, 13, FEAT_SNEAK_ATTACK},
  {CLASS_ROGUE, RACE_UNDEFINED, TRUE, 15, FEAT_SNEAK_ATTACK},
  {CLASS_ROGUE, RACE_UNDEFINED, TRUE, 15, FEAT_TRAP_SENSE},
  /* advanced talent lvl 15, defensive roll */
  {CLASS_ROGUE, RACE_UNDEFINED, FALSE, 15, FEAT_DEFENSIVE_ROLL},
  {CLASS_ROGUE, RACE_UNDEFINED, TRUE, 17, FEAT_SNEAK_ATTACK},
  {CLASS_ROGUE, RACE_UNDEFINED, TRUE, 18, FEAT_TRAP_SENSE},
  /* talent lvl 18, dirtkick */
  {CLASS_ROGUE, RACE_UNDEFINED, FALSE, 18, FEAT_DIRT_KICK},
  {CLASS_ROGUE, RACE_UNDEFINED, TRUE, 19, FEAT_SNEAK_ATTACK},
  /* epic */
  {CLASS_ROGUE, RACE_UNDEFINED, TRUE, 21, FEAT_SNEAK_ATTACK},
  /* talent lvl 21, backstab */
  {CLASS_ROGUE, RACE_UNDEFINED, FALSE, 21, FEAT_BACKSTAB},
  {CLASS_ROGUE, RACE_UNDEFINED, TRUE, 22, FEAT_TRAP_SENSE},
  {CLASS_ROGUE, RACE_UNDEFINED, TRUE, 23, FEAT_SNEAK_ATTACK},
  /* talent lvl 24, sap */
  {CLASS_ROGUE, RACE_UNDEFINED, FALSE, 24, FEAT_SAP},
  {CLASS_ROGUE, RACE_UNDEFINED, TRUE, 25, FEAT_SNEAK_ATTACK},
  {CLASS_ROGUE, RACE_UNDEFINED, TRUE, 26, FEAT_TRAP_SENSE},
  {CLASS_ROGUE, RACE_UNDEFINED, TRUE, 27, FEAT_SNEAK_ATTACK},
  /* talent lvl 27, vanish */
  {CLASS_ROGUE, RACE_UNDEFINED, FALSE, 27, FEAT_VANISH},
  {CLASS_ROGUE, RACE_UNDEFINED, TRUE, 29, FEAT_SNEAK_ATTACK},
  {CLASS_ROGUE, RACE_UNDEFINED, TRUE, 30, FEAT_TRAP_SENSE},
  /* talent lvl 30, improved vanish */
  {CLASS_ROGUE, RACE_UNDEFINED, FALSE, 30, FEAT_IMPROVED_VANISH},

  /* monk */
  /* class, race, stacks?, level, feat_ name */
  {CLASS_MONK, RACE_UNDEFINED, FALSE, 2, FEAT_EVASION},
  {CLASS_MONK, RACE_UNDEFINED, FALSE, 3, FEAT_STILL_MIND},
  {CLASS_MONK, RACE_UNDEFINED, TRUE, 4, FEAT_KI_STRIKE},
  {CLASS_MONK, RACE_UNDEFINED, TRUE, 4, FEAT_SLOW_FALL},
  {CLASS_MONK, RACE_UNDEFINED, TRUE, 5, FEAT_SLOW_FALL},
  {CLASS_MONK, RACE_UNDEFINED, FALSE, 5, FEAT_PURITY_OF_BODY},
  {CLASS_MONK, RACE_UNDEFINED, TRUE, 6, FEAT_SLOW_FALL},
  {CLASS_MONK, RACE_UNDEFINED, FALSE, 7, FEAT_WHOLENESS_OF_BODY},
  {CLASS_MONK, RACE_UNDEFINED, TRUE, 8, FEAT_SLOW_FALL},
  {CLASS_MONK, RACE_UNDEFINED, FALSE, 9, FEAT_IMPROVED_EVASION},
  {CLASS_MONK, RACE_UNDEFINED, TRUE, 10, FEAT_KI_STRIKE},
  {CLASS_MONK, RACE_UNDEFINED, TRUE, 10, FEAT_SLOW_FALL},
  {CLASS_MONK, RACE_UNDEFINED, FALSE, 11, FEAT_DIAMOND_BODY},
  {CLASS_MONK, RACE_UNDEFINED, FALSE, 11, FEAT_GREATER_FLURRY},
  {CLASS_MONK, RACE_UNDEFINED, FALSE, 12, FEAT_ABUNDANT_STEP},
  {CLASS_MONK, RACE_UNDEFINED, TRUE, 12, FEAT_SLOW_FALL},
  {CLASS_MONK, RACE_UNDEFINED, FALSE, 13, FEAT_DIAMOND_SOUL},
  {CLASS_MONK, RACE_UNDEFINED, TRUE, 14, FEAT_SLOW_FALL},
  {CLASS_MONK, RACE_UNDEFINED, FALSE, 15, FEAT_QUIVERING_PALM},
  {CLASS_MONK, RACE_UNDEFINED, TRUE, 15, FEAT_KI_STRIKE},
  {CLASS_MONK, RACE_UNDEFINED, FALSE, 16, FEAT_TIMELESS_BODY},
  /* note this feat does nothing currently */
  {CLASS_MONK, RACE_UNDEFINED, FALSE, 17, FEAT_TONGUE_OF_THE_SUN_AND_MOON},
  {CLASS_MONK, RACE_UNDEFINED, TRUE, 18, FEAT_SLOW_FALL},
  {CLASS_MONK, RACE_UNDEFINED, FALSE, 19, FEAT_EMPTY_BODY},
  {CLASS_MONK, RACE_UNDEFINED, FALSE, 20, FEAT_PERFECT_SELF},
  {CLASS_MONK, RACE_UNDEFINED, TRUE, 20, FEAT_SLOW_FALL},
  {CLASS_MONK, RACE_UNDEFINED, TRUE, 22, FEAT_SLOW_FALL},
  /* 23 bonus free epic feat */
  {CLASS_MONK, RACE_UNDEFINED, FALSE, 23, FEAT_KEEN_STRIKE},
  {CLASS_MONK, RACE_UNDEFINED, TRUE, 24, FEAT_SLOW_FALL},
  {CLASS_MONK, RACE_UNDEFINED, TRUE, 26, FEAT_SLOW_FALL},
  /* 26 bonus free epic feat */
  {CLASS_MONK, RACE_UNDEFINED, FALSE, 26, FEAT_BLINDING_SPEED},
  {CLASS_MONK, RACE_UNDEFINED, TRUE, 28, FEAT_SLOW_FALL},
  /* 29 bonus free epic feat */
  {CLASS_MONK, RACE_UNDEFINED, FALSE, 29, FEAT_OUTSIDER},
  {CLASS_MONK, RACE_UNDEFINED, TRUE, 30, FEAT_SLOW_FALL},

  /* berserker */
  /* if we use pathfinder type rules, berserkers should get special
   selections of feats from a rage-power list every two levels...
   as a temporary solution, every 3 levels we are giving berserkers
   hand selected rage-powers */
  /* class, race, stacks?, level, feat_ name */
  {CLASS_BERSERKER, RACE_UNDEFINED, TRUE, 1, FEAT_RAGE},
  {CLASS_BERSERKER, RACE_UNDEFINED, FALSE, 2, FEAT_UNCANNY_DODGE},
  {CLASS_BERSERKER, RACE_UNDEFINED, TRUE, 3, FEAT_TRAP_SENSE},
  /* rage power (level 3) */
  {CLASS_BERSERKER, RACE_UNDEFINED, FALSE, 3, FEAT_RP_SUPRISE_ACCURACY},
  {CLASS_BERSERKER, RACE_UNDEFINED, TRUE, 4, FEAT_RAGE},
  {CLASS_BERSERKER, RACE_UNDEFINED, TRUE, 4, FEAT_SHRUG_DAMAGE},
  {CLASS_BERSERKER, RACE_UNDEFINED, FALSE, 5, FEAT_IMPROVED_UNCANNY_DODGE},
  {CLASS_BERSERKER, RACE_UNDEFINED, TRUE, 6, FEAT_TRAP_SENSE},
  /* rage power (level 6) */
  {CLASS_BERSERKER, RACE_UNDEFINED, FALSE, 6, FEAT_RP_POWERFUL_BLOW},
  {CLASS_BERSERKER, RACE_UNDEFINED, TRUE, 7, FEAT_SHRUG_DAMAGE},
  {CLASS_BERSERKER, RACE_UNDEFINED, TRUE, 8, FEAT_RAGE},
  {CLASS_BERSERKER, RACE_UNDEFINED, TRUE, 9, FEAT_TRAP_SENSE},
  /* rage power (level 9) */
  {CLASS_BERSERKER, RACE_UNDEFINED, FALSE, 9, FEAT_RP_RENEWED_VIGOR},
  {CLASS_BERSERKER, RACE_UNDEFINED, TRUE, 10, FEAT_SHRUG_DAMAGE},
  {CLASS_BERSERKER, RACE_UNDEFINED, FALSE, 11, FEAT_GREATER_RAGE},
  {CLASS_BERSERKER, RACE_UNDEFINED, TRUE, 11, FEAT_RAGE},
  {CLASS_BERSERKER, RACE_UNDEFINED, TRUE, 12, FEAT_TRAP_SENSE},
  /* rage power (level 12) */
  {CLASS_BERSERKER, RACE_UNDEFINED, FALSE, 12, FEAT_RP_HEAVY_SHRUG},
  {CLASS_BERSERKER, RACE_UNDEFINED, TRUE, 13, FEAT_SHRUG_DAMAGE},
  {CLASS_BERSERKER, RACE_UNDEFINED, FALSE, 14, FEAT_INDOMITABLE_WILL},
  {CLASS_BERSERKER, RACE_UNDEFINED, TRUE, 15, FEAT_TRAP_SENSE},
  {CLASS_BERSERKER, RACE_UNDEFINED, TRUE, 15, FEAT_RAGE},
  /* rage power (level 15) */
  {CLASS_BERSERKER, RACE_UNDEFINED, FALSE, 15, FEAT_RP_FEARLESS_RAGE},
  {CLASS_BERSERKER, RACE_UNDEFINED, TRUE, 16, FEAT_SHRUG_DAMAGE},
  {CLASS_BERSERKER, RACE_UNDEFINED, FALSE, 17, FEAT_TIRELESS_RAGE},
  {CLASS_BERSERKER, RACE_UNDEFINED, TRUE, 18, FEAT_TRAP_SENSE},
  /* rage power (level 18) */
  {CLASS_BERSERKER, RACE_UNDEFINED, FALSE, 18, FEAT_RP_COME_AND_GET_ME},
  {CLASS_BERSERKER, RACE_UNDEFINED, TRUE, 19, FEAT_SHRUG_DAMAGE},
  {CLASS_BERSERKER, RACE_UNDEFINED, TRUE, 20, FEAT_RAGE},
  {CLASS_BERSERKER, RACE_UNDEFINED, FALSE, 20, FEAT_MIGHTY_RAGE},
  /* epic */
  /* this ended up being wayyyyy too powerful */
  //{CLASS_BERSERKER, RACE_UNDEFINED, TRUE, 21, FEAT_RAGING_CRITICAL},
  /* rage power lvl 22*/
  {CLASS_BERSERKER, RACE_UNDEFINED, FALSE, 24, FEAT_EATER_OF_MAGIC},
  {CLASS_BERSERKER, RACE_UNDEFINED, TRUE, 22, FEAT_SHRUG_DAMAGE},
  {CLASS_BERSERKER, RACE_UNDEFINED, TRUE, 24, FEAT_RAGE},
  {CLASS_BERSERKER, RACE_UNDEFINED, TRUE, 25, FEAT_SHRUG_DAMAGE},
  /* rage power lvl 26*/
  {CLASS_BERSERKER, RACE_UNDEFINED, FALSE, 26, FEAT_RAGE_RESISTANCE},
  {CLASS_BERSERKER, RACE_UNDEFINED, FALSE, 27, FEAT_INDOMITABLE_RAGE},
  {CLASS_BERSERKER, RACE_UNDEFINED, TRUE, 28, FEAT_SHRUG_DAMAGE},
  {CLASS_BERSERKER, RACE_UNDEFINED, TRUE, 29, FEAT_RAGE},
  /* rage power lvl 30*/
  {CLASS_BERSERKER, RACE_UNDEFINED, FALSE, 30, FEAT_DEATHLESS_FRENZY},


  /* ranger */ /* CM = combat matery substitute */
  /* class, race, stacks?, level, feat_ name */
  {CLASS_RANGER, RACE_UNDEFINED, TRUE,  1,  FEAT_FAVORED_ENEMY_AVAILABLE},
  {CLASS_RANGER, RACE_UNDEFINED, FALSE, 2,  FEAT_WILD_EMPATHY},
  {CLASS_RANGER, RACE_UNDEFINED, FALSE, 3,  FEAT_DUAL_WEAPON_FIGHTING},/*CM*/
  {CLASS_RANGER, RACE_UNDEFINED, FALSE, 4,  FEAT_POINT_BLANK_SHOT},/*CM*/
  {CLASS_RANGER, RACE_UNDEFINED, FALSE, 4,  FEAT_ENDURANCE},
  {CLASS_RANGER, RACE_UNDEFINED, FALSE, 4,  FEAT_ANIMAL_COMPANION},
  {CLASS_RANGER, RACE_UNDEFINED, TRUE,  5,  FEAT_FAVORED_ENEMY_AVAILABLE},
  {CLASS_RANGER, RACE_UNDEFINED, FALSE, 6,  FEAT_IMPROVED_DUAL_WEAPON_FIGHTING},/*CM*/
  {CLASS_RANGER, RACE_UNDEFINED, FALSE, 7,  FEAT_RAPID_SHOT},/*CM*/
  {CLASS_RANGER, RACE_UNDEFINED, FALSE, 8,  FEAT_WOODLAND_STRIDE},
  {CLASS_RANGER, RACE_UNDEFINED, FALSE, 9,  FEAT_SWIFT_TRACKER},
  {CLASS_RANGER, RACE_UNDEFINED, FALSE, 10, FEAT_EVASION},
  {CLASS_RANGER, RACE_UNDEFINED, TRUE,  10, FEAT_FAVORED_ENEMY_AVAILABLE},
  {CLASS_RANGER, RACE_UNDEFINED, FALSE, 11, FEAT_GREATER_DUAL_WEAPON_FIGHTING},/*CM*/
  {CLASS_RANGER, RACE_UNDEFINED, FALSE, 12, FEAT_MANYSHOT},/*CM*/
  {CLASS_RANGER, RACE_UNDEFINED, FALSE, 13, FEAT_CAMOUFLAGE},
  {CLASS_RANGER, RACE_UNDEFINED, FALSE, 14, FEAT_TRACK},
  {CLASS_RANGER, RACE_UNDEFINED, TRUE,  15, FEAT_FAVORED_ENEMY_AVAILABLE},
  {CLASS_RANGER, RACE_UNDEFINED, FALSE, 17, FEAT_HIDE_IN_PLAIN_SIGHT},
  /* Epic */
  {CLASS_RANGER, RACE_UNDEFINED, TRUE,  20, FEAT_FAVORED_ENEMY_AVAILABLE},
  {CLASS_RANGER, RACE_UNDEFINED, FALSE, 21, FEAT_PERFECT_DUAL_WEAPON_FIGHTING},/*CM*/
  {CLASS_RANGER, RACE_UNDEFINED, FALSE, 22, FEAT_EPIC_MANYSHOT},/*CM*/
  /* bonus feat - improved evasion 23 */
  {CLASS_RANGER, RACE_UNDEFINED, FALSE, 23, FEAT_IMPROVED_EVASION},
  {CLASS_RANGER, RACE_UNDEFINED, TRUE,  25, FEAT_FAVORED_ENEMY_AVAILABLE},
  /* bonus feat - bane of enemies 26 */
  {CLASS_RANGER, RACE_UNDEFINED, FALSE, 26, FEAT_BANE_OF_ENEMIES},
  /* bonus feat - epic favored enemy 29 */
  {CLASS_RANGER, RACE_UNDEFINED, FALSE, 29, FEAT_EPIC_FAVORED_ENEMY},
  {CLASS_RANGER, RACE_UNDEFINED, TRUE,  30, FEAT_FAVORED_ENEMY_AVAILABLE},

  /* druid */
  /* class, race, stacks?, level, feat_ name */
  {CLASS_DRUID, RACE_UNDEFINED, FALSE, 2, FEAT_WILD_EMPATHY},
  {CLASS_DRUID, RACE_UNDEFINED, FALSE, 2, FEAT_WOODLAND_STRIDE},
  {CLASS_DRUID, RACE_UNDEFINED, FALSE, 3, FEAT_TRACKLESS_STEP},
  {CLASS_DRUID, RACE_UNDEFINED, FALSE, 4, FEAT_RESIST_NATURES_LURE},
  /* FEAT_WILD_SHAPE is the first level of wildshape forms AND cooldown */
  {CLASS_DRUID, RACE_UNDEFINED, TRUE, 4, FEAT_WILD_SHAPE},
  {CLASS_DRUID, RACE_UNDEFINED, TRUE, 6, FEAT_WILD_SHAPE},
  /* FEAT_WILD_SHAPE_x is the xth level of wildshape forms, does not affect cooldown */
  {CLASS_DRUID, RACE_UNDEFINED, FALSE, 6, FEAT_WILD_SHAPE_2},
  {CLASS_DRUID, RACE_UNDEFINED, TRUE, 8, FEAT_WILD_SHAPE},
  {CLASS_DRUID, RACE_UNDEFINED, FALSE, 8, FEAT_WILD_SHAPE_3},
  {CLASS_DRUID, RACE_UNDEFINED, FALSE, 9, FEAT_VENOM_IMMUNITY},
  {CLASS_DRUID, RACE_UNDEFINED, TRUE, 10, FEAT_WILD_SHAPE},
  {CLASS_DRUID, RACE_UNDEFINED, FALSE, 10, FEAT_WILD_SHAPE_4},
  {CLASS_DRUID, RACE_UNDEFINED, TRUE, 12, FEAT_WILD_SHAPE},
  {CLASS_DRUID, RACE_UNDEFINED, FALSE, 12, FEAT_WILD_SHAPE_5},
  {CLASS_DRUID, RACE_UNDEFINED, FALSE, 13, FEAT_THOUSAND_FACES},
  {CLASS_DRUID, RACE_UNDEFINED, TRUE, 14, FEAT_WILD_SHAPE},
  {CLASS_DRUID, RACE_UNDEFINED, FALSE, 15, FEAT_TIMELESS_BODY},
  {CLASS_DRUID, RACE_UNDEFINED, TRUE, 16, FEAT_WILD_SHAPE},
  {CLASS_DRUID, RACE_UNDEFINED, TRUE, 18, FEAT_WILD_SHAPE},
  {CLASS_DRUID, RACE_UNDEFINED, TRUE, 20, FEAT_WILD_SHAPE},
  {CLASS_DRUID, RACE_UNDEFINED, TRUE, 22, FEAT_WILD_SHAPE},
  {CLASS_DRUID, RACE_UNDEFINED, TRUE, 24, FEAT_WILD_SHAPE},
  {CLASS_DRUID, RACE_UNDEFINED, TRUE, 26, FEAT_WILD_SHAPE},
  {CLASS_DRUID, RACE_UNDEFINED, TRUE, 28, FEAT_WILD_SHAPE},
  {CLASS_DRUID, RACE_UNDEFINED, TRUE, 30, FEAT_WILD_SHAPE},

  /* bard */
  /* class, race, stacks?, level, feat_ name */
  {CLASS_BARD, RACE_UNDEFINED, FALSE, 1, FEAT_BARDIC_MUSIC},
  {CLASS_BARD, RACE_UNDEFINED, FALSE, 1, FEAT_BARDIC_KNOWLEDGE},
  {CLASS_BARD, RACE_UNDEFINED, FALSE, 1, FEAT_COUNTERSONG},
  {CLASS_BARD, RACE_UNDEFINED, FALSE, 1, FEAT_FASCINATE},
  {CLASS_BARD, RACE_UNDEFINED, TRUE, 1, FEAT_INSPIRE_COURAGE},
  {CLASS_BARD, RACE_UNDEFINED, FALSE, 3, FEAT_INSPIRE_COMPETENCE},
  {CLASS_BARD, RACE_UNDEFINED, FALSE, 6, FEAT_SUGGESTION},
  {CLASS_BARD, RACE_UNDEFINED, TRUE, 8, FEAT_INSPIRE_COURAGE},
  {CLASS_BARD, RACE_UNDEFINED, FALSE, 9, FEAT_INSPIRE_GREATNESS},
  {CLASS_BARD, RACE_UNDEFINED, FALSE, 12, FEAT_SONG_OF_FREEDOM},
  {CLASS_BARD, RACE_UNDEFINED, TRUE, 14, FEAT_INSPIRE_COURAGE},
  {CLASS_BARD, RACE_UNDEFINED, FALSE, 15, FEAT_INSPIRE_HEROICS},
  {CLASS_BARD, RACE_UNDEFINED, FALSE, 18, FEAT_MASS_SUGGESTION},
  {CLASS_BARD, RACE_UNDEFINED, TRUE, 20, FEAT_INSPIRE_COURAGE},

  /* weapon master */
  /* class, race, stacks?, level, feat_ name */
  /* lvl 1 - weapon of choice, assigned during "free feats" */
  {CLASS_WEAPON_MASTER, RACE_UNDEFINED, FALSE, 2,  FEAT_SUPERIOR_WEAPON_FOCUS},
  {CLASS_WEAPON_MASTER, RACE_UNDEFINED, TRUE,  4,  FEAT_CRITICAL_SPECIALIST},
  {CLASS_WEAPON_MASTER, RACE_UNDEFINED, FALSE, 6,  FEAT_UNSTOPPABLE_STRIKE},
  {CLASS_WEAPON_MASTER, RACE_UNDEFINED, TRUE,  8,  FEAT_CRITICAL_SPECIALIST},
  {CLASS_WEAPON_MASTER, RACE_UNDEFINED, FALSE, 10, FEAT_INCREASED_MULTIPLIER},

  /* Racial feats */
  /* class, race, stacks?, level, feat_ name */

  /* elf */
  {CLASS_UNDEFINED, RACE_ELF, FALSE, 1, FEAT_LOW_LIGHT_VISION},
  {CLASS_UNDEFINED, RACE_ELF, FALSE, 1, FEAT_WEAPON_PROFICIENCY_ELF},

  /* crystal dwarf */
  {CLASS_UNDEFINED, RACE_CRYSTAL_DWARF, FALSE, 1, FEAT_CRYSTAL_BODY},
  {CLASS_UNDEFINED, RACE_CRYSTAL_DWARF, FALSE, 1, FEAT_CRYSTAL_FIST},

  /*****************************************/
  /* This is always the last array element */
  /*****************************************/
  {CLASS_UNDEFINED, RACE_UNDEFINED, FALSE, 1, FEAT_UNDEFINED}
};

/* this is not currently used */
int epic_level_feats[][7] = {

  { CLASS_ROGUE, 0, 2, 1, TRUE, FEAT_SNEAK_ATTACK, 1},
  { CLASS_ROGUE, 0, 4, 0, TRUE, FEAT_TRAP_SENSE, 1},
  { CLASS_BERSERKER, 0, 3, 0, TRUE, FEAT_TRAP_SENSE, 1},
  { CLASS_BERSERKER, 0, 3, 1, TRUE, FEAT_SHRUG_DAMAGE, 1},
  { CLASS_BERSERKER, 0, 4, 0, FALSE, FEAT_RAGE, 1},
  { CLASS_DRUID, -2, 4, 0, TRUE, FEAT_WILD_SHAPE, 1},
  { CLASS_PALADIN, 0, 5, 0, TRUE, FEAT_SMITE_EVIL, 1},
  { CLASS_PALADIN, 0, 3, 0, TRUE, FEAT_REMOVE_DISEASE, 1},
  { CLASS_RANGER, 0, 5, 0, TRUE, FEAT_FAVORED_ENEMY_AVAILABLE, 1},

  // This is always the last one
  { CLASS_UNDEFINED, 0, 0, 0, TRUE, FEAT_UNDEFINED, 0}
};


/* CLASS FEATS ---------------------------------------------------- */
const int class_feats_wizard[] = {
  FEAT_SPELL_PENETRATION,
  FEAT_GREATER_SPELL_PENETRATION,
  FEAT_ARMORED_SPELLCASTING,
  FEAT_FASTER_MEMORIZATION,
  FEAT_SPELL_FOCUS,
  FEAT_GREATER_SPELL_FOCUS,
  FEAT_IMPROVED_FAMILIAR,
  FEAT_QUICK_CHANT,
  FEAT_AUGMENT_SUMMONING,
  FEAT_ENHANCED_SPELL_DAMAGE,

  /* epic class */
  FEAT_DRAGON_KNIGHT,
  FEAT_HELLBALL,
  FEAT_EPIC_MAGE_ARMOR,
  FEAT_EPIC_WARDING,
  FEAT_GREAT_INTELLIGENCE,

  /*end*/
  FEAT_UNDEFINED
};
/* should pretty much just match wizard, but sorcs don't actually get any bonus feats right?*/
const int class_feats_sorcerer[] = {
  FEAT_SPELL_PENETRATION,
  FEAT_GREATER_SPELL_PENETRATION,
  FEAT_ARMORED_SPELLCASTING,
  FEAT_FASTER_MEMORIZATION,
  FEAT_SPELL_FOCUS,
  FEAT_GREATER_SPELL_FOCUS,
  FEAT_IMPROVED_FAMILIAR,
  FEAT_QUICK_CHANT,
  FEAT_AUGMENT_SUMMONING,
  FEAT_ENHANCED_SPELL_DAMAGE,

  /* epic class */
  FEAT_DRAGON_KNIGHT,
  FEAT_HELLBALL,
  FEAT_EPIC_MAGE_ARMOR,
  FEAT_EPIC_WARDING,
  FEAT_GREAT_CHARISMA,

  /*end*/
  FEAT_UNDEFINED
};
const int class_feats_cleric[] = {
  FEAT_SPELL_PENETRATION,
  FEAT_GREATER_SPELL_PENETRATION,
  FEAT_FASTER_MEMORIZATION,
  FEAT_QUICK_CHANT,
  FEAT_AUGMENT_SUMMONING,
  FEAT_ENHANCED_SPELL_DAMAGE,

  /* epic */
  FEAT_GREAT_WISDOM,
  FEAT_MUMMY_DUST,
  FEAT_GREATER_RUIN,

  /*end*/
  FEAT_UNDEFINED
};
const int class_feats_druid[] = {
  FEAT_SPELL_PENETRATION,
  FEAT_GREATER_SPELL_PENETRATION,
  FEAT_FASTER_MEMORIZATION,
  FEAT_QUICK_CHANT,
  FEAT_FAST_HEALING,
  FEAT_AUGMENT_SUMMONING,
  FEAT_ENHANCED_SPELL_DAMAGE,

  /* epic */
  FEAT_GREAT_WISDOM,
  FEAT_MUMMY_DUST,
  FEAT_GREATER_RUIN,

  /*end*/
  FEAT_UNDEFINED
};
/*
 * Rogues follow opposite logic - they can take any feat in place of these,
 * all of these are abilities that are not normally able to be taken as
 * feats. Most classes can ONLY take from these lists for their class
 * feats.
 */
const int class_feats_rogue[] = {
  FEAT_APPLY_POISON,
  FEAT_BLEEDING_ATTACK,
  FEAT_BLIND_FIGHT,
  FEAT_CLEAVE,
  FEAT_COMBAT_EXPERTISE,
  FEAT_COMBAT_REFLEXES,
  FEAT_CRIPPLING_STRIKE,
  FEAT_DEFENSIVE_ROLL,
  FEAT_DEFLECT_ARROWS,
  FEAT_DIRT_KICK,
  FEAT_DODGE,
  FEAT_EXOTIC_WEAPON_PROFICIENCY,
  FEAT_FAR_SHOT,
  FEAT_GREAT_CLEAVE,
  FEAT_GREATER_TWO_WEAPON_FIGHTING,
  FEAT_GREATER_WEAPON_FOCUS,
  FEAT_IMPROVED_BULL_RUSH,
  FEAT_IMPROVED_CRITICAL,
  FEAT_IMPROVED_DISARM,
  FEAT_IMPROVED_EVASION,
  FEAT_IMPROVED_FEINT,
  FEAT_IMPROVED_GRAPPLE,
  FEAT_IMPROVED_INITIATIVE,
  FEAT_IMPROVED_OVERRUN,
  FEAT_IMPROVED_PRECISE_SHOT,
  FEAT_IMPROVED_SHIELD_PUNCH,
  FEAT_IMPROVED_SNEAK_ATTACK,
  FEAT_IMPROVED_SUNDER,
  FEAT_IMPROVED_TRIP,
  FEAT_IMPROVED_TWO_WEAPON_FIGHTING,
  FEAT_IMPROVED_UNARMED_STRIKE,
  FEAT_MANYSHOT,
  FEAT_MOBILITY,
  FEAT_MOUNTED_ARCHERY,
  FEAT_MOUNTED_COMBAT,
  FEAT_OPPORTUNIST,
  FEAT_POINT_BLANK_SHOT,
  FEAT_POWER_ATTACK,
  FEAT_POWERFUL_SNEAK,
  FEAT_PRECISE_SHOT,
  FEAT_QUICK_DRAW,
  FEAT_RAPID_RELOAD,
  FEAT_RAPID_SHOT,
  FEAT_RIDE_BY_ATTACK,
  FEAT_SHOT_ON_THE_RUN,
  FEAT_SKILL_MASTERY,
  FEAT_SLIPPERY_MIND,
  FEAT_SNATCH_ARROWS,
  FEAT_SNEAK_ATTACK,
  FEAT_SNEAK_ATTACK_OF_OPPORTUNITY,
  FEAT_SPIRITED_CHARGE,
  FEAT_SPRING_ATTACK,
  FEAT_STUNNING_FIST,
  FEAT_TRAMPLE,
  FEAT_TWO_WEAPON_DEFENSE,
  FEAT_TWO_WEAPON_FIGHTING,
  FEAT_WEAPON_FINESSE,
  FEAT_WEAPON_FOCUS,
  FEAT_WEAPON_PROFICIENCY_BASTARD_SWORD,
  FEAT_WHIRLWIND_ATTACK,

  /* epic class */
  FEAT_EPIC_DODGE,
  FEAT_EPIC_SKILL_FOCUS,
  FEAT_GREAT_DEXTERITY,
  FEAT_PERFECT_TWO_WEAPON_FIGHTING,
  FEAT_SELF_CONCEALMENT,

  /* end */
  FEAT_UNDEFINED
};
const int class_feats_fighter[] = {
  FEAT_EPIC_PROWESS,
  FEAT_SWARM_OF_ARROWS,
  FEAT_BLIND_FIGHT,
  FEAT_CLEAVE,
  FEAT_COMBAT_EXPERTISE,
  FEAT_COMBAT_REFLEXES,
  FEAT_DEFLECT_ARROWS,
  FEAT_DODGE,
  FEAT_WEAPON_PROFICIENCY_BASTARD_SWORD,
  FEAT_EXOTIC_WEAPON_PROFICIENCY,
  FEAT_FAR_SHOT,
  FEAT_GREAT_CLEAVE,
  FEAT_GREATER_TWO_WEAPON_FIGHTING,
  FEAT_GREATER_WEAPON_FOCUS,
  FEAT_GREATER_WEAPON_SPECIALIZATION,
  FEAT_IMPROVED_BULL_RUSH,
  FEAT_IMPROVED_CRITICAL,
  FEAT_IMPROVED_DISARM,
  FEAT_IMPROVED_FEINT,
  FEAT_IMPROVED_GRAPPLE,
  FEAT_IMPROVED_INITIATIVE,
  FEAT_IMPROVED_OVERRUN,
  FEAT_IMPROVED_PRECISE_SHOT,
  FEAT_IMPROVED_SHIELD_PUNCH,
  FEAT_SHIELD_CHARGE,
  FEAT_SHIELD_SLAM,
  FEAT_IMPROVED_SUNDER,
  FEAT_IMPROVED_TRIP,
  FEAT_IMPROVED_TWO_WEAPON_FIGHTING,
  FEAT_IMPROVED_UNARMED_STRIKE,
  FEAT_MANYSHOT,
  FEAT_MOBILITY,
  FEAT_MOUNTED_ARCHERY,
  FEAT_MOUNTED_COMBAT,
  FEAT_POINT_BLANK_SHOT,
  FEAT_POWER_ATTACK,
  FEAT_PRECISE_SHOT,
  FEAT_QUICK_DRAW,
  FEAT_RAPID_RELOAD,
  FEAT_RAPID_SHOT,
  FEAT_RIDE_BY_ATTACK,
  FEAT_SHOT_ON_THE_RUN,
  FEAT_SNATCH_ARROWS,
  FEAT_SPIRITED_CHARGE,
  FEAT_SPRING_ATTACK,
  FEAT_STUNNING_FIST,
  FEAT_TRAMPLE,
  FEAT_TWO_WEAPON_DEFENSE,
  FEAT_TWO_WEAPON_FIGHTING,
  FEAT_WEAPON_FINESSE,
  FEAT_WEAPON_FOCUS,
  FEAT_WEAPON_SPECIALIZATION,
  FEAT_WHIRLWIND_ATTACK,
  FEAT_DAMAGE_REDUCTION,
  FEAT_FAST_HEALING,
  FEAT_ARMOR_SKIN,
  FEAT_ARMOR_SPECIALIZATION_LIGHT,
  FEAT_ARMOR_SPECIALIZATION_MEDIUM,
  FEAT_ARMOR_SPECIALIZATION_HEAVY,
  FEAT_WEAPON_MASTERY,
  FEAT_WEAPON_FLURRY,
  FEAT_WEAPON_SUPREMACY,
  FEAT_ROBILARS_GAMBIT,
  FEAT_KNOCKDOWN,

  /* epic */
  FEAT_GREAT_STRENGTH,
  FEAT_GREAT_DEXTERITY,
  FEAT_GREAT_CONSTITUTION,
  FEAT_EPIC_TOUGHNESS,
  FEAT_EPIC_WEAPON_SPECIALIZATION,

  /*end*/
  FEAT_UNDEFINED
};
const int class_feats_paladin[] = {
  FEAT_DAMAGE_REDUCTION,
  FEAT_EPIC_PROWESS,
  FEAT_ARMOR_SKIN,
  FEAT_GREAT_SMITING,

  /* epic */
  FEAT_GREAT_CHARISMA,
  FEAT_EPIC_TOUGHNESS,

  /*end*/
  FEAT_UNDEFINED
};
const int class_feats_monk[] = {
  FEAT_STUNNING_FIST,
  FEAT_EPIC_PROWESS,
  FEAT_SELF_CONCEALMENT,
  FEAT_IMPROVED_TRIP,
  FEAT_DEFLECT_ARROWS,
  FEAT_COMBAT_REFLEXES,
  FEAT_FAST_HEALING,
  FEAT_DAMAGE_REDUCTION,

  /* epic */
  FEAT_GREAT_WISDOM,
  FEAT_EPIC_TOUGHNESS,

  /*end*/
  FEAT_UNDEFINED
};
const int class_feats_berserker[] = {
  FEAT_FAST_HEALING,
  FEAT_EPIC_PROWESS,
  FEAT_DAMAGE_REDUCTION,
  FEAT_EPIC_TOUGHNESS,
  FEAT_RP_SUPRISE_ACCURACY,
  FEAT_RP_POWERFUL_BLOW,
  FEAT_RP_RENEWED_VIGOR,
  FEAT_RP_HEAVY_SHRUG,
  FEAT_RP_FEARLESS_RAGE,
  FEAT_RP_COME_AND_GET_ME,

  /* epic */
  FEAT_GREAT_CONSTITUTION,
  FEAT_GREAT_STRENGTH,

  /*end*/
  FEAT_UNDEFINED
};
const int class_feats_ranger[] = {
  FEAT_FAST_HEALING,
  FEAT_EPIC_PROWESS,
  FEAT_SWARM_OF_ARROWS,

  /*epic*/
  FEAT_EPIC_TOUGHNESS,
  FEAT_GREAT_DEXTERITY,

  /*end*/
  FEAT_UNDEFINED
};
const int class_feats_weaponmaster[] = {
  FEAT_BLIND_FIGHT,
  FEAT_CLEAVE,
  FEAT_COMBAT_EXPERTISE,
  FEAT_COMBAT_REFLEXES,
  FEAT_DEFLECT_ARROWS,
  FEAT_EXOTIC_WEAPON_PROFICIENCY,
  FEAT_GREAT_CLEAVE,
  FEAT_GREATER_TWO_WEAPON_FIGHTING,
  FEAT_IMPROVED_CRITICAL,
  FEAT_IMPROVED_SUNDER,
  FEAT_IMPROVED_TWO_WEAPON_FIGHTING,
  FEAT_IMPROVED_UNARMED_STRIKE,
  FEAT_POWER_ATTACK,
  FEAT_RAPID_RELOAD,
  FEAT_RAPID_SHOT,
  FEAT_SHOT_ON_THE_RUN,
  FEAT_TWO_WEAPON_DEFENSE,
  FEAT_TWO_WEAPON_FIGHTING,
  FEAT_WEAPON_FINESSE,
  FEAT_WEAPON_FOCUS,

  /* epic */
  FEAT_EPIC_PROWESS,
  FEAT_EPIC_TOUGHNESS,
  FEAT_GREAT_STRENGTH,
  FEAT_GREAT_DEXTERITY,

  /*end*/
  FEAT_UNDEFINED
};
const int class_feats_bard[] = {

  /* epic */
  FEAT_GREAT_CHARISMA,

  /*end*/
  FEAT_UNDEFINED
};
const int no_class_feats[] = {
  /*end*/
  FEAT_UNDEFINED
};

/* This array collects all of the information about class feats
 * and is used during level gain to show the allowed feats.
 * SEE NOTE FOR ROGUE FEATS */
const int *class_bonus_feats[NUM_CLASSES] = {
  /* Wizard       */ class_feats_wizard,
  /* Cleric       */ class_feats_cleric,
  /* Rogue        */ class_feats_rogue,
  /* Warrior      */ class_feats_fighter,
  /* Monk         */ class_feats_monk,
  /* Druid        */ class_feats_druid,
  /* Berserker    */ class_feats_berserker,
  /* Sorcerer     */ class_feats_sorcerer,
  /* Paladin      */ class_feats_paladin,
  /* Ranger       */ class_feats_ranger,
  /* Bard         */ class_feats_bard,
  /* WeaponMaster */ class_feats_weaponmaster
};

byte saving_throws(struct char_data *ch, int type) {
  int i, save = 1;

  if (IS_NPC(ch)) {
    if (preferred_save[type][GET_CLASS(ch)])
      return (GET_LEVEL(ch) / 2 + 1);
    else
      return (GET_LEVEL(ch) / 4 + 1);
  }

  for (i = 0; i < MAX_CLASSES; i++) {
    if (CLASS_LEVEL(ch, i)) { // found class and level
      if (preferred_save[type][i])
        save += CLASS_LEVEL(ch, i) / 2;
      else
        save += CLASS_LEVEL(ch, i) / 4;
    }
  }

  return save;
}
#undef H
#undef L


// base attack bonus, replacement for THAC0 system

int BAB(struct char_data *ch) {
  int i, bab = 0, level;

  /* gnarly huh? */
  if (IS_AFFECTED(ch, AFF_TFORM))
    return (GET_LEVEL(ch));

  if (IS_NPC(ch)) {
    switch (GET_CLASS(ch)) {
      case CLASS_ROGUE:
      case CLASS_CLERIC:
      case CLASS_DRUID:
      case CLASS_BARD:
      case CLASS_MONK:
        return ( (int) (GET_LEVEL(ch) * 3 / 4));
      case CLASS_WARRIOR:
      case CLASS_WEAPON_MASTER:
      case CLASS_RANGER:
      case CLASS_PALADIN:
      case CLASS_BERSERKER:
        return ( (int) (GET_LEVEL(ch)));
      case CLASS_WIZARD:
      case CLASS_SORCERER:
      default:
        return ( (int) (GET_LEVEL(ch) / 2));
    }
  }

  /* loop through all the possible classes the char could be */
  for (i = 0; i < MAX_CLASSES; i++) {
    level = CLASS_LEVEL(ch, i);
    if (level) {
      switch (i) {
        case CLASS_WIZARD:
        case CLASS_SORCERER:
          bab += level / 2;
          break;
        case CLASS_ROGUE:
        case CLASS_CLERIC:
        case CLASS_DRUID:
        case CLASS_BARD:
        case CLASS_MONK:
          bab += level * 3 / 4;
          break;
        case CLASS_WARRIOR:
        case CLASS_WEAPON_MASTER:
        case CLASS_RANGER:
        case CLASS_PALADIN:
        case CLASS_BERSERKER:
          bab += level;
          break;
      }
    }
  }

  if (bab == -1)
    log("ERROR:  BAB catching -1");

  if (char_has_mud_event(ch, eSPELLBATTLE) && SPELLBATTLE(ch) > 0) {
    bab += MAX(1, (SPELLBATTLE(ch) * 2 / 3));
  }

  if (!IS_NPC(ch)) /* cap pc bab at 30 */
    return (MIN(bab, 30));

  return bab;
}


// old random roll system abandoned for base stats + point distribution
void roll_real_abils(struct char_data *ch) {
  GET_REAL_INT(ch) = 8;
  GET_REAL_WIS(ch) = 8;
  GET_REAL_CHA(ch) = 8;
  GET_REAL_STR(ch) = 8;
  GET_REAL_DEX(ch) = 8;
  GET_REAL_CON(ch) = 8;
  ch->aff_abils = ch->real_abils;
}

//   give newbie's some eq to start with
#define NOOB_TELEPORTER    82
#define NOOB_TORCH         858
#define NOOB_RATIONS       804
#define NOOB_WATERSKIN     803
#define NOOB_BP            857
#define NOOB_CRAFTING_KIT  3118
#define NOOB_BOW           814
#define NOOB_QUIVER        816
#define NOOB_ARROW         815
#define NUM_NOOB_ARROWS    40
#define NOOB_WIZ_NOTE      850
void newbieEquipment(struct char_data *ch) {
  int objNums[] = {
    NOOB_TELEPORTER,
    NOOB_TORCH,
    NOOB_TORCH,
    NOOB_RATIONS,
    NOOB_RATIONS,
    NOOB_RATIONS,
    NOOB_RATIONS,
    NOOB_WATERSKIN,
    NOOB_BP,
    NOOB_CRAFTING_KIT,
    NOOB_BOW,
    -1 //had to end with -1
  };
  int x;
  struct obj_data *obj = NULL, *quiver = NULL;

  send_to_char(ch, "\tMYou are given a set of starting equipment...\tn\r\n");

  // give everyone torch, rations, skin, backpack
  for (x = 0; objNums[x] != -1; x++) {
    obj = read_object(objNums[x], VIRTUAL);
    if (obj) {
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch);
    }
  }

  quiver = read_object(NOOB_QUIVER, VIRTUAL);
  if (quiver)
    obj_to_char(quiver, ch);

  for (x = 0; x < NUM_NOOB_ARROWS; x++) {
    obj = read_object(NOOB_ARROW, VIRTUAL);
    if (quiver && obj)
      obj_to_obj(obj, quiver);
  }


  switch (GET_CLASS(ch)) {
    case CLASS_PALADIN:
    case CLASS_CLERIC:
      // holy symbol

      obj = read_object(854, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch); // leather sleeves

      obj = read_object(855, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch); // leather leggings

      obj = read_object(861, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch); // slender iron mace

      obj = read_object(863, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch); // shield

      obj = read_object(807, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch); // scale mail

      break;

    case CLASS_BERSERKER:
    case CLASS_WARRIOR:
    case CLASS_RANGER:

      obj = read_object(854, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch); // leather sleeves

      obj = read_object(855, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch); // leather pants

      if (GET_RACE(ch) == RACE_DWARF) {
        obj = read_object(806, VIRTUAL);
        GET_OBJ_SIZE(obj) = GET_SIZE(ch);
        obj_to_char(obj, ch); // dwarven waraxe
      } else {
        obj = read_object(808, VIRTUAL);
        GET_OBJ_SIZE(obj) = GET_SIZE(ch);
        obj_to_char(obj, ch); // bastard sword
      }
      obj = read_object(863, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch); // shield

      obj = read_object(807, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch); // scale mail

      break;

    case CLASS_MONK:
      obj = read_object(809, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch); // cloth robes

      break;

    case CLASS_BARD:
    case CLASS_ROGUE:
      obj = read_object(854, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch); // leather sleeves

      obj = read_object(855, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch); // leather pants

      obj = read_object(851, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch); // studded leather

      obj = read_object(852, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch); // dagger

      obj = read_object(852, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch); // dagger

      break;

    case CLASS_WIZARD:
      obj_to_char(read_object(NOOB_WIZ_NOTE, VIRTUAL), ch); //wizard note
      obj_to_char(read_object(812, VIRTUAL), ch); //spellbook
      /* switch fallthrough */
    case CLASS_SORCERER:
      obj = read_object(854, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch); // leather sleeves

      obj = read_object(855, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch); // leather pants

      obj = read_object(852, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch); // dagger

      obj = read_object(809, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch); // cloth robes

      break;
    default:
      log("Invalid class sent to newbieEquipment!");
      break;
  }
}
#undef NOOB_TELEPORTER
#undef NOOB_TORCH
#undef NOOB_RATIONS
#undef NOOB_WATERSKIN
#undef NOOB_BP
#undef NOOB_CRAFTING_KIT
#undef NOOB_BOW
#undef NOOB_QUIVER
#undef NOOB_ARROW

/* init spells for a class as they level up
 * i.e free skills  ;  make sure to set in spec_procs too
 * Note:  this is not currently used */
void berserker_skills(struct char_data *ch, int level) {}
void bard_skills(struct char_data *ch, int level) {}
void ranger_skills(struct char_data *ch, int level) {}
#define MOB_PALADIN_MOUNT 70
void paladin_skills(struct char_data *ch, int level) {}
#undef MOB_PALADIN_MOUNT
void sorc_skills(struct char_data *ch, int level) {}
void wizard_skills(struct char_data *ch, int level) {
  IS_WIZ_LEARNED(ch) = 0;
  send_to_char(ch,
          "\tnType \tDstudy wizard\tn to adjust your wizard skills.\r\n");
}
void cleric_skills(struct char_data *ch, int level) {}
void warrior_skills(struct char_data *ch, int level) {}
void druid_skills(struct char_data *ch, int level) {
  IS_DRUID_LEARNED(ch) = 0;
}
void rogue_skills(struct char_data *ch, int level) {}
void monk_skills(struct char_data *ch, int level) {}
void weaponmaster_skills(struct char_data *ch, int level) {}

/* this is used to assign all the spells and also starting feats */
void init_class(struct char_data *ch, int class, int level) {
  int i, j;

  /* Init Feats - Each class gets a set of free starting feats. */
  for (i = 0; (j = free_start_feats[class][i]); i++) {
    if (!HAS_REAL_FEAT(ch, j)) {
      send_to_char(ch, "You have learned the %s feat!\r\n", feat_list[j].name);
      SET_FEAT(ch, j, 1);
    }
  }

  switch (class) {

    case CLASS_WIZARD:
      /* SWITCH FALL THROUGH */
    case CLASS_SORCERER:
      //spell init
      //1st circle
      SET_SKILL(ch, SPELL_HORIZIKAULS_BOOM, 99);
      SET_SKILL(ch, SPELL_MAGIC_MISSILE, 99);
      SET_SKILL(ch, SPELL_BURNING_HANDS, 99);
      SET_SKILL(ch, SPELL_ICE_DAGGER, 99);
      SET_SKILL(ch, SPELL_MAGE_ARMOR, 99);
      SET_SKILL(ch, SPELL_SUMMON_CREATURE_1, 99);
      SET_SKILL(ch, SPELL_CHILL_TOUCH, 99);
      SET_SKILL(ch, SPELL_NEGATIVE_ENERGY_RAY, 99);
      SET_SKILL(ch, SPELL_RAY_OF_ENFEEBLEMENT, 99);
      SET_SKILL(ch, SPELL_CHARM, 99);
      SET_SKILL(ch, SPELL_ENCHANT_WEAPON, 99);
      SET_SKILL(ch, SPELL_SLEEP, 99);
      SET_SKILL(ch, SPELL_COLOR_SPRAY, 99);
      SET_SKILL(ch, SPELL_SCARE, 99);
      SET_SKILL(ch, SPELL_TRUE_STRIKE, 99);
      SET_SKILL(ch, SPELL_IDENTIFY, 99);
      SET_SKILL(ch, SPELL_SHELGARNS_BLADE, 99);
      SET_SKILL(ch, SPELL_GREASE, 99);
      SET_SKILL(ch, SPELL_ENDURE_ELEMENTS, 99);
      SET_SKILL(ch, SPELL_PROT_FROM_EVIL, 99);
      SET_SKILL(ch, SPELL_PROT_FROM_GOOD, 99);
      SET_SKILL(ch, SPELL_EXPEDITIOUS_RETREAT, 99);
      SET_SKILL(ch, SPELL_IRON_GUTS, 99);
      SET_SKILL(ch, SPELL_SHIELD, 99);

      //2nd circle
      SET_SKILL(ch, SPELL_SHOCKING_GRASP, 99);
      SET_SKILL(ch, SPELL_SCORCHING_RAY, 99);
      SET_SKILL(ch, SPELL_CONTINUAL_FLAME, 99);
      SET_SKILL(ch, SPELL_SUMMON_CREATURE_2, 99);
      SET_SKILL(ch, SPELL_WEB, 99);
      SET_SKILL(ch, SPELL_ACID_ARROW, 99);
      SET_SKILL(ch, SPELL_BLINDNESS, 99);
      SET_SKILL(ch, SPELL_DEAFNESS, 99);
      SET_SKILL(ch, SPELL_FALSE_LIFE, 99);
      SET_SKILL(ch, SPELL_DAZE_MONSTER, 99);
      SET_SKILL(ch, SPELL_HIDEOUS_LAUGHTER, 99);
      SET_SKILL(ch, SPELL_TOUCH_OF_IDIOCY, 99);
      SET_SKILL(ch, SPELL_BLUR, 99);
      SET_SKILL(ch, SPELL_MIRROR_IMAGE, 99);
      SET_SKILL(ch, SPELL_INVISIBLE, 99);
      SET_SKILL(ch, SPELL_DETECT_INVIS, 99);
      SET_SKILL(ch, SPELL_DETECT_MAGIC, 99);
      SET_SKILL(ch, SPELL_DARKNESS, 99);
      SET_SKILL(ch, SPELL_I_DARKNESS, 99);
      SET_SKILL(ch, SPELL_RESIST_ENERGY, 99);
      SET_SKILL(ch, SPELL_ENERGY_SPHERE, 99);
      SET_SKILL(ch, SPELL_ENDURANCE, 99);
      SET_SKILL(ch, SPELL_STRENGTH, 99);
      SET_SKILL(ch, SPELL_GRACE, 99);

      //3rd circle
      SET_SKILL(ch, SPELL_LIGHTNING_BOLT, 99);
      SET_SKILL(ch, SPELL_FIREBALL, 99);
      SET_SKILL(ch, SPELL_WATER_BREATHE, 99);
      SET_SKILL(ch, SPELL_SUMMON_CREATURE_3, 99);
      SET_SKILL(ch, SPELL_PHANTOM_STEED, 99);
      SET_SKILL(ch, SPELL_STINKING_CLOUD, 99);
      SET_SKILL(ch, SPELL_HALT_UNDEAD, 99);
      SET_SKILL(ch, SPELL_VAMPIRIC_TOUCH, 99);
      SET_SKILL(ch, SPELL_HEROISM, 99);
      SET_SKILL(ch, SPELL_FLY, 99);
      SET_SKILL(ch, SPELL_HOLD_PERSON, 99);
      SET_SKILL(ch, SPELL_DEEP_SLUMBER, 99);
      SET_SKILL(ch, SPELL_WALL_OF_FOG, 99);
      SET_SKILL(ch, SPELL_INVISIBILITY_SPHERE, 99);
      SET_SKILL(ch, SPELL_DAYLIGHT, 99);
      SET_SKILL(ch, SPELL_CLAIRVOYANCE, 99);
      SET_SKILL(ch, SPELL_NON_DETECTION, 99);
      SET_SKILL(ch, SPELL_DISPEL_MAGIC, 99);
      SET_SKILL(ch, SPELL_HASTE, 99);
      SET_SKILL(ch, SPELL_SLOW, 99);
      SET_SKILL(ch, SPELL_CIRCLE_A_EVIL, 99);
      SET_SKILL(ch, SPELL_CIRCLE_A_GOOD, 99);
      SET_SKILL(ch, SPELL_CUNNING, 99);
      SET_SKILL(ch, SPELL_WISDOM, 99);
      SET_SKILL(ch, SPELL_CHARISMA, 99);

      //4th circle
      SET_SKILL(ch, SPELL_FIRE_SHIELD, 99);
      SET_SKILL(ch, SPELL_COLD_SHIELD, 99);
      SET_SKILL(ch, SPELL_ICE_STORM, 99);
      SET_SKILL(ch, SPELL_BILLOWING_CLOUD, 99);
      SET_SKILL(ch, SPELL_SUMMON_CREATURE_4, 99);
      SET_SKILL(ch, SPELL_ANIMATE_DEAD, 99);
      SET_SKILL(ch, SPELL_CURSE, 99);
      SET_SKILL(ch, SPELL_INFRAVISION, 99); //shared
      SET_SKILL(ch, SPELL_POISON, 99); //shared
      SET_SKILL(ch, SPELL_GREATER_INVIS, 99);
      SET_SKILL(ch, SPELL_RAINBOW_PATTERN, 99);
      SET_SKILL(ch, SPELL_WIZARD_EYE, 99);
      SET_SKILL(ch, SPELL_LOCATE_CREATURE, 99);
      SET_SKILL(ch, SPELL_MINOR_GLOBE, 99);
      SET_SKILL(ch, SPELL_REMOVE_CURSE, 99); //shared
      SET_SKILL(ch, SPELL_STONESKIN, 99);
      SET_SKILL(ch, SPELL_ENLARGE_PERSON, 99);
      SET_SKILL(ch, SPELL_SHRINK_PERSON, 99);

      //5th circle
      SET_SKILL(ch, SPELL_INTERPOSING_HAND, 99);
      SET_SKILL(ch, SPELL_WALL_OF_FORCE, 99);
      SET_SKILL(ch, SPELL_BALL_OF_LIGHTNING, 99);
      SET_SKILL(ch, SPELL_CLOUDKILL, 99);
      SET_SKILL(ch, SPELL_SUMMON_CREATURE_5, 99);
      SET_SKILL(ch, SPELL_WAVES_OF_FATIGUE, 99);
      SET_SKILL(ch, SPELL_SYMBOL_OF_PAIN, 99);
      SET_SKILL(ch, SPELL_DOMINATE_PERSON, 99);
      SET_SKILL(ch, SPELL_FEEBLEMIND, 99);
      SET_SKILL(ch, SPELL_NIGHTMARE, 99);
      SET_SKILL(ch, SPELL_MIND_FOG, 99);
      SET_SKILL(ch, SPELL_ACID_SHEATH, 99);
      SET_SKILL(ch, SPELL_FAITHFUL_HOUND, 99);
      SET_SKILL(ch, SPELL_DISMISSAL, 99);
      SET_SKILL(ch, SPELL_CONE_OF_COLD, 99);
      SET_SKILL(ch, SPELL_TELEKINESIS, 99);
      SET_SKILL(ch, SPELL_FIREBRAND, 99);

      //6th circle
      SET_SKILL(ch, SPELL_FREEZING_SPHERE, 99);
      SET_SKILL(ch, SPELL_ACID_FOG, 99);
      SET_SKILL(ch, SPELL_SUMMON_CREATURE_6, 99);
      SET_SKILL(ch, SPELL_TRANSFORMATION, 99);
      SET_SKILL(ch, SPELL_EYEBITE, 99);
      SET_SKILL(ch, SPELL_MASS_HASTE, 99);
      SET_SKILL(ch, SPELL_GREATER_HEROISM, 99);
      SET_SKILL(ch, SPELL_ANTI_MAGIC_FIELD, 99);
      SET_SKILL(ch, SPELL_GREATER_MIRROR_IMAGE, 99);
      SET_SKILL(ch, SPELL_LOCATE_OBJECT, 99);
      SET_SKILL(ch, SPELL_TRUE_SEEING, 99);
      SET_SKILL(ch, SPELL_GLOBE_OF_INVULN, 99);
      SET_SKILL(ch, SPELL_GREATER_DISPELLING, 99);
      SET_SKILL(ch, SPELL_CLONE, 99);
      SET_SKILL(ch, SPELL_WATERWALK, 99);

      //7th circle
      SET_SKILL(ch, SPELL_MISSILE_STORM, 99);
      SET_SKILL(ch, SPELL_GRASPING_HAND, 99);
      SET_SKILL(ch, SPELL_SUMMON_CREATURE_7, 99);
      SET_SKILL(ch, SPELL_CONTROL_WEATHER, 99);
      SET_SKILL(ch, SPELL_POWER_WORD_BLIND, 99);
      SET_SKILL(ch, SPELL_WAVES_OF_EXHAUSTION, 99);
      SET_SKILL(ch, SPELL_MASS_HOLD_PERSON, 99);
      SET_SKILL(ch, SPELL_MASS_FLY, 99);
      SET_SKILL(ch, SPELL_DISPLACEMENT, 99);
      SET_SKILL(ch, SPELL_PRISMATIC_SPRAY, 99);
      SET_SKILL(ch, SPELL_DETECT_POISON, 99); //shared
      SET_SKILL(ch, SPELL_POWER_WORD_STUN, 99);
      SET_SKILL(ch, SPELL_PROTECT_FROM_SPELLS, 99);
      SET_SKILL(ch, SPELL_THUNDERCLAP, 99);
      SET_SKILL(ch, SPELL_SPELL_MANTLE, 99);
      SET_SKILL(ch, SPELL_TELEPORT, 99);
      SET_SKILL(ch, SPELL_MASS_WISDOM, 99); //shared
      SET_SKILL(ch, SPELL_MASS_CHARISMA, 99); //shared
      SET_SKILL(ch, SPELL_MASS_CUNNING, 99); //shared

      //8th circle
      SET_SKILL(ch, SPELL_CLENCHED_FIST, 99);
      SET_SKILL(ch, SPELL_CHAIN_LIGHTNING, 99);
      SET_SKILL(ch, SPELL_INCENDIARY_CLOUD, 99);
      SET_SKILL(ch, SPELL_SUMMON_CREATURE_8, 99);
      SET_SKILL(ch, SPELL_HORRID_WILTING, 99);
      SET_SKILL(ch, SPELL_GREATER_ANIMATION, 99);
      SET_SKILL(ch, SPELL_IRRESISTIBLE_DANCE, 99);
      SET_SKILL(ch, SPELL_MASS_DOMINATION, 99);
      SET_SKILL(ch, SPELL_SCINT_PATTERN, 99);
      SET_SKILL(ch, SPELL_REFUGE, 99);
      SET_SKILL(ch, SPELL_BANISH, 99);
      SET_SKILL(ch, SPELL_SUNBURST, 99);
      SET_SKILL(ch, SPELL_SPELL_TURNING, 99);
      SET_SKILL(ch, SPELL_MIND_BLANK, 99);
      SET_SKILL(ch, SPELL_IRONSKIN, 99);
      SET_SKILL(ch, SPELL_PORTAL, 99);

      //9th circle
      SET_SKILL(ch, SPELL_METEOR_SWARM, 99);
      SET_SKILL(ch, SPELL_BLADE_OF_DISASTER, 99);
      SET_SKILL(ch, SPELL_SUMMON_CREATURE_9, 99);
      SET_SKILL(ch, SPELL_GATE, 99);
      SET_SKILL(ch, SPELL_ENERGY_DRAIN, 99);
      SET_SKILL(ch, SPELL_WAIL_OF_THE_BANSHEE, 99);
      SET_SKILL(ch, SPELL_POWER_WORD_KILL, 99);
      SET_SKILL(ch, SPELL_ENFEEBLEMENT, 99);
      SET_SKILL(ch, SPELL_WEIRD, 99);
      SET_SKILL(ch, SPELL_SHADOW_SHIELD, 99);
      SET_SKILL(ch, SPELL_PRISMATIC_SPHERE, 99);
      SET_SKILL(ch, SPELL_IMPLODE, 99);
      SET_SKILL(ch, SPELL_TIMESTOP, 99);
      SET_SKILL(ch, SPELL_GREATER_SPELL_MANTLE, 99);
      SET_SKILL(ch, SPELL_POLYMORPH, 99);
      SET_SKILL(ch, SPELL_MASS_ENHANCE, 99);

      // wizard/sorc innate cantrips
      /*
      SET_SKILL(ch, SPELL_ACID_SPLASH, 99);
      SET_SKILL(ch, SPELL_RAY_OF_FROST, 99);
       */

      send_to_char(ch, "Wizard / Sorcerer Done.\tn\r\n");
      break;

    case CLASS_BARD:
      //spell init
      //1st circle
      SET_SKILL(ch, SPELL_HORIZIKAULS_BOOM, 99);
      SET_SKILL(ch, SPELL_SHIELD, 99);
      SET_SKILL(ch, SPELL_SUMMON_CREATURE_1, 99);
      SET_SKILL(ch, SPELL_CHARM, 99);
      SET_SKILL(ch, SPELL_ENDURE_ELEMENTS, 99);
      SET_SKILL(ch, SPELL_PROT_FROM_EVIL, 99);
      SET_SKILL(ch, SPELL_PROT_FROM_GOOD, 99);
      SET_SKILL(ch, SPELL_MAGIC_MISSILE, 99);
      SET_SKILL(ch, SPELL_CURE_LIGHT, 99);

      //2nd circle
      SET_SKILL(ch, SPELL_SUMMON_CREATURE_2, 99);
      SET_SKILL(ch, SPELL_DEAFNESS, 99);
      SET_SKILL(ch, SPELL_HIDEOUS_LAUGHTER, 99);
      SET_SKILL(ch, SPELL_MIRROR_IMAGE, 99);
      SET_SKILL(ch, SPELL_DETECT_INVIS, 99);
      SET_SKILL(ch, SPELL_DETECT_MAGIC, 99);
      SET_SKILL(ch, SPELL_INVISIBLE, 99);
      SET_SKILL(ch, SPELL_ENDURANCE, 99);
      SET_SKILL(ch, SPELL_STRENGTH, 99);
      SET_SKILL(ch, SPELL_GRACE, 99);
      SET_SKILL(ch, SPELL_CURE_MODERATE, 99);

      //3rd circle
      SET_SKILL(ch, SPELL_SUMMON_CREATURE_3, 99);
      SET_SKILL(ch, SPELL_LIGHTNING_BOLT, 99);
      SET_SKILL(ch, SPELL_DEEP_SLUMBER, 99);
      SET_SKILL(ch, SPELL_HASTE, 99);
      SET_SKILL(ch, SPELL_CIRCLE_A_EVIL, 99);
      SET_SKILL(ch, SPELL_CIRCLE_A_GOOD, 99);
      SET_SKILL(ch, SPELL_CUNNING, 99);
      SET_SKILL(ch, SPELL_WISDOM, 99);
      SET_SKILL(ch, SPELL_CHARISMA, 99);
      SET_SKILL(ch, SPELL_CURE_SERIOUS, 99);

      //4th circle
      SET_SKILL(ch, SPELL_SUMMON_CREATURE_4, 99);
      SET_SKILL(ch, SPELL_GREATER_INVIS, 99);
      SET_SKILL(ch, SPELL_RAINBOW_PATTERN, 99);
      SET_SKILL(ch, SPELL_REMOVE_CURSE, 99); //shared
      SET_SKILL(ch, SPELL_ICE_STORM, 99);
      SET_SKILL(ch, SPELL_CURE_CRITIC, 99);

      //5th circle
      SET_SKILL(ch, SPELL_SUMMON_CREATURE_5, 99);
      SET_SKILL(ch, SPELL_ACID_SHEATH, 99);
      SET_SKILL(ch, SPELL_CONE_OF_COLD, 99);
      SET_SKILL(ch, SPELL_NIGHTMARE, 99);
      SET_SKILL(ch, SPELL_MIND_FOG, 99);
      SET_SKILL(ch, SPELL_MASS_CURE_LIGHT, 99);

      //6th circle
      SET_SKILL(ch, SPELL_SUMMON_CREATURE_7, 99);
      SET_SKILL(ch, SPELL_FREEZING_SPHERE, 99);
      SET_SKILL(ch, SPELL_GREATER_HEROISM, 99);
      SET_SKILL(ch, SPELL_MASS_CURE_MODERATE, 99);
      SET_SKILL(ch, SPELL_STONESKIN, 99);

      // bard innate cantrips
      /*
      SET_SKILL(ch, SPELL_ACID_SPLASH, 99);
      SET_SKILL(ch, SPELL_RAY_OF_FROST, 99);
       */

      send_to_char(ch, "Bard Done.\tn\r\n");
      break;

    case CLASS_CLERIC:
      /* we also have to add this to study where we set our domains */
      assign_domain_spells(ch);

      //spell init
      //1st circle
      SET_SKILL(ch, SPELL_ARMOR, 99);
      SET_SKILL(ch, SPELL_CURE_LIGHT, 99);
      SET_SKILL(ch, SPELL_ENDURANCE, 99);
      SET_SKILL(ch, SPELL_CAUSE_LIGHT_WOUNDS, 99);
      SET_SKILL(ch, SPELL_NEGATIVE_ENERGY_RAY, 99);
      SET_SKILL(ch, SPELL_ENDURE_ELEMENTS, 99);
      SET_SKILL(ch, SPELL_PROT_FROM_GOOD, 99);
      SET_SKILL(ch, SPELL_PROT_FROM_EVIL, 99);
      SET_SKILL(ch, SPELL_SUMMON_CREATURE_1, 99);
      SET_SKILL(ch, SPELL_STRENGTH, 99);
      SET_SKILL(ch, SPELL_GRACE, 99);
      SET_SKILL(ch, SPELL_REMOVE_FEAR, 99);
      //2nd circle
      SET_SKILL(ch, SPELL_CREATE_FOOD, 99);
      SET_SKILL(ch, SPELL_CREATE_WATER, 99);
      SET_SKILL(ch, SPELL_DETECT_POISON, 99);
      SET_SKILL(ch, SPELL_CAUSE_MODERATE_WOUNDS, 99);
      SET_SKILL(ch, SPELL_CURE_MODERATE, 99);
      SET_SKILL(ch, SPELL_SCARE, 99);
      SET_SKILL(ch, SPELL_SUMMON_CREATURE_2, 99);
      SET_SKILL(ch, SPELL_DETECT_MAGIC, 99);
      SET_SKILL(ch, SPELL_DARKNESS, 99);
      SET_SKILL(ch, SPELL_RESIST_ENERGY, 99);
      SET_SKILL(ch, SPELL_WISDOM, 99);
      SET_SKILL(ch, SPELL_CHARISMA, 99);
      //3rd circle
      SET_SKILL(ch, SPELL_BLESS, 99);
      SET_SKILL(ch, SPELL_CURE_BLIND, 99);
      SET_SKILL(ch, SPELL_DETECT_ALIGN, 99);
      SET_SKILL(ch, SPELL_CAUSE_SERIOUS_WOUNDS, 99);
      SET_SKILL(ch, SPELL_CURE_SERIOUS, 99);
      SET_SKILL(ch, SPELL_SUMMON_CREATURE_3, 99);
      SET_SKILL(ch, SPELL_BLINDNESS, 99);
      SET_SKILL(ch, SPELL_DEAFNESS, 99);
      SET_SKILL(ch, SPELL_CURE_DEAFNESS, 99);
      SET_SKILL(ch, SPELL_CUNNING, 99);
      SET_SKILL(ch, SPELL_DISPEL_MAGIC, 99);
      SET_SKILL(ch, SPELL_ANIMATE_DEAD, 99);
      SET_SKILL(ch, SPELL_FAERIE_FOG, 99);
      //4th circle
      SET_SKILL(ch, SPELL_CURE_CRITIC, 99);
      SET_SKILL(ch, SPELL_REMOVE_CURSE, 99);
      SET_SKILL(ch, SPELL_INFRAVISION, 99);
      SET_SKILL(ch, SPELL_CAUSE_CRITICAL_WOUNDS, 99);
      SET_SKILL(ch, SPELL_SUMMON_CREATURE_4, 99);
      SET_SKILL(ch, SPELL_CIRCLE_A_EVIL, 99);
      SET_SKILL(ch, SPELL_CIRCLE_A_GOOD, 99);
      SET_SKILL(ch, SPELL_CURSE, 99);
      SET_SKILL(ch, SPELL_DAYLIGHT, 99);
      SET_SKILL(ch, SPELL_MASS_CURE_LIGHT, 99);
      SET_SKILL(ch, SPELL_AID, 99);
      SET_SKILL(ch, SPELL_BRAVERY, 99);
      //5th circle
      SET_SKILL(ch, SPELL_POISON, 99);
      SET_SKILL(ch, SPELL_REMOVE_POISON, 99);
      SET_SKILL(ch, SPELL_PROT_FROM_EVIL, 99);
      SET_SKILL(ch, SPELL_GROUP_ARMOR, 99);
      SET_SKILL(ch, SPELL_FLAME_STRIKE, 99);
      SET_SKILL(ch, SPELL_PROT_FROM_GOOD, 99);
      SET_SKILL(ch, SPELL_MASS_CURE_MODERATE, 99);
      SET_SKILL(ch, SPELL_SUMMON_CREATURE_5, 99);
      SET_SKILL(ch, SPELL_WATER_BREATHE, 99);
      SET_SKILL(ch, SPELL_WATERWALK, 99);
      SET_SKILL(ch, SPELL_REGENERATION, 99);
      SET_SKILL(ch, SPELL_FREE_MOVEMENT, 99);
      SET_SKILL(ch, SPELL_STRENGTHEN_BONE, 99);
      //6th circle
      SET_SKILL(ch, SPELL_DISPEL_EVIL, 99);
      SET_SKILL(ch, SPELL_HARM, 99);
      SET_SKILL(ch, SPELL_HEAL, 99);
      SET_SKILL(ch, SPELL_DISPEL_GOOD, 99);
      SET_SKILL(ch, SPELL_SUMMON_CREATURE_6, 99);
      SET_SKILL(ch, SPELL_MASS_CURE_SERIOUS, 99);
      SET_SKILL(ch, SPELL_EYEBITE, 99);
      SET_SKILL(ch, SPELL_PRAYER, 99);
      SET_SKILL(ch, SPELL_MASS_WISDOM, 99);
      SET_SKILL(ch, SPELL_MASS_CHARISMA, 99);
      SET_SKILL(ch, SPELL_MASS_CUNNING, 99);
      SET_SKILL(ch, SPELL_REMOVE_DISEASE, 99);
      //7th circle
      SET_SKILL(ch, SPELL_CALL_LIGHTNING, 99);
      //SET_SKILL(ch, SPELL_CONTROL_WEATHER, 99);
      SET_SKILL(ch, SPELL_SUMMON, 99);
      SET_SKILL(ch, SPELL_WORD_OF_RECALL, 99);
      SET_SKILL(ch, SPELL_SUMMON_CREATURE_7, 99);
      SET_SKILL(ch, SPELL_MASS_CURE_CRIT, 99);
      SET_SKILL(ch, SPELL_GREATER_DISPELLING, 99);
      SET_SKILL(ch, SPELL_MASS_ENHANCE, 99);
      SET_SKILL(ch, SPELL_BLADE_BARRIER, 99);
      SET_SKILL(ch, SPELL_BATTLETIDE, 99);
      SET_SKILL(ch, SPELL_SPELL_RESISTANCE, 99);
      SET_SKILL(ch, SPELL_SENSE_LIFE, 99);
      //8th circle
      //SET_SKILL(ch, SPELL_SANCTUARY, 99);
      SET_SKILL(ch, SPELL_DESTRUCTION, 99);
      SET_SKILL(ch, SPELL_SUMMON_CREATURE_8, 99);
      SET_SKILL(ch, SPELL_SPELL_MANTLE, 99);
      SET_SKILL(ch, SPELL_TRUE_SEEING, 99);
      SET_SKILL(ch, SPELL_WORD_OF_FAITH, 99);
      SET_SKILL(ch, SPELL_GREATER_ANIMATION, 99);
      SET_SKILL(ch, SPELL_EARTHQUAKE, 99);
      SET_SKILL(ch, SPELL_ANTI_MAGIC_FIELD, 99);
      SET_SKILL(ch, SPELL_DIMENSIONAL_LOCK, 99);
      SET_SKILL(ch, SPELL_SALVATION, 99);
      SET_SKILL(ch, SPELL_SPRING_OF_LIFE, 99);
      //9th circle
      SET_SKILL(ch, SPELL_SUNBURST, 99);
      SET_SKILL(ch, SPELL_ENERGY_DRAIN, 99);
      SET_SKILL(ch, SPELL_GROUP_HEAL, 99);
      SET_SKILL(ch, SPELL_SUMMON_CREATURE_9, 99);
      SET_SKILL(ch, SPELL_PLANE_SHIFT, 99);
      SET_SKILL(ch, SPELL_STORM_OF_VENGEANCE, 99);
      SET_SKILL(ch, SPELL_IMPLODE, 99);
      //death shield
      //command
      //air walker
      SET_SKILL(ch, SPELL_REFUGE, 99);
      SET_SKILL(ch, SPELL_GROUP_SUMMON, 99);


      send_to_char(ch, "Cleric Done.\tn\r\n");
      break;


    case CLASS_DRUID:
      //spell init
      //1st circle
      SET_SKILL(ch, SPELL_CHARM_ANIMAL, 99);
      SET_SKILL(ch, SPELL_CURE_LIGHT, 99);
      SET_SKILL(ch, SPELL_FAERIE_FIRE, 99);
      SET_SKILL(ch, SPELL_GOODBERRY, 99);
      SET_SKILL(ch, SPELL_JUMP, 99);
      SET_SKILL(ch, SPELL_MAGIC_FANG, 99);
      SET_SKILL(ch, SPELL_MAGIC_STONE, 99);
      SET_SKILL(ch, SPELL_OBSCURING_MIST, 99);
      SET_SKILL(ch, SPELL_PRODUCE_FLAME, 99);
      SET_SKILL(ch, SPELL_SUMMON_NATURES_ALLY_1, 99);

      //2nd circle
      SET_SKILL(ch, SPELL_BARKSKIN, 99);
      SET_SKILL(ch, SPELL_ENDURANCE, 99);
      SET_SKILL(ch, SPELL_STRENGTH, 99);
      SET_SKILL(ch, SPELL_GRACE, 99);
      SET_SKILL(ch, SPELL_FLAME_BLADE, 99);
      SET_SKILL(ch, SPELL_FLAMING_SPHERE, 99);
      SET_SKILL(ch, SPELL_HOLD_ANIMAL, 99);
      SET_SKILL(ch, SPELL_SUMMON_NATURES_ALLY_2, 99);
      SET_SKILL(ch, SPELL_SUMMON_SWARM, 99);
      SET_SKILL(ch, SPELL_WISDOM, 99);

      //3rd circle
      SET_SKILL(ch, SPELL_CALL_LIGHTNING, 99);
      SET_SKILL(ch, SPELL_CURE_MODERATE, 99);
      SET_SKILL(ch, SPELL_CONTAGION, 99);
      SET_SKILL(ch, SPELL_GREATER_MAGIC_FANG, 99);
      SET_SKILL(ch, SPELL_POISON, 99);
      SET_SKILL(ch, SPELL_REMOVE_DISEASE, 99);
      SET_SKILL(ch, SPELL_REMOVE_POISON, 99);
      SET_SKILL(ch, SPELL_SPIKE_GROWTH, 99);
      SET_SKILL(ch, SPELL_SUMMON_NATURES_ALLY_3, 99);

      //4th circle
      SET_SKILL(ch, SPELL_BLIGHT, 99);
      SET_SKILL(ch, SPELL_CURE_SERIOUS, 99);
      SET_SKILL(ch, SPELL_DISPEL_MAGIC, 99);
      SET_SKILL(ch, SPELL_FLAME_STRIKE, 99);
      SET_SKILL(ch, SPELL_FREE_MOVEMENT, 99);
      SET_SKILL(ch, SPELL_ICE_STORM, 99);
      SET_SKILL(ch, SPELL_LOCATE_CREATURE, 99);
      // reincarnate? SET_SKILL(ch, SPELL_REINCARNATE, 99);
      SET_SKILL(ch, SPELL_SPIKE_STONES, 99);
      SET_SKILL(ch, SPELL_SUMMON_NATURES_ALLY_4, 99);

      //5th circle
      // baleful polymorph SET_SKILL(ch, SPELL_BALEFUL_POLYMORPH, 99);
      SET_SKILL(ch, SPELL_CALL_LIGHTNING_STORM, 99);
      SET_SKILL(ch, SPELL_CURE_CRITIC, 99);
      SET_SKILL(ch, SPELL_DEATH_WARD, 99);
      SET_SKILL(ch, SPELL_HALLOW, 99);
      SET_SKILL(ch, SPELL_INSECT_PLAGUE, 99);
      SET_SKILL(ch, SPELL_STONESKIN, 99);
      SET_SKILL(ch, SPELL_SUMMON_NATURES_ALLY_5, 99);
      SET_SKILL(ch, SPELL_UNHALLOW, 99);
      SET_SKILL(ch, SPELL_WALL_OF_FIRE, 99);
      SET_SKILL(ch, SPELL_WALL_OF_THORNS, 99);

      //6th circle
      SET_SKILL(ch, SPELL_FIRE_SEEDS, 99);
      SET_SKILL(ch, SPELL_GREATER_DISPELLING, 99);
      SET_SKILL(ch, SPELL_MASS_ENDURANCE, 99);
      SET_SKILL(ch, SPELL_MASS_STRENGTH, 99);
      SET_SKILL(ch, SPELL_MASS_GRACE, 99);
      SET_SKILL(ch, SPELL_MASS_WISDOM, 99);
      SET_SKILL(ch, SPELL_SPELLSTAFF, 99);
      SET_SKILL(ch, SPELL_SUMMON_NATURES_ALLY_6, 99);
      SET_SKILL(ch, SPELL_TRANSPORT_VIA_PLANTS, 99);

      //7th circle
      SET_SKILL(ch, SPELL_CONTROL_WEATHER, 99);
      SET_SKILL(ch, SPELL_CREEPING_DOOM, 99);
      SET_SKILL(ch, SPELL_FIRE_STORM, 99);
      // greater scrying SET_SKILL(ch, SPELL_GREATER_SCRYING, 99);
      SET_SKILL(ch, SPELL_HEAL, 99);
      SET_SKILL(ch, SPELL_MASS_CURE_MODERATE, 99);
      SET_SKILL(ch, SPELL_SUMMON_NATURES_ALLY_7, 99);
      SET_SKILL(ch, SPELL_SUNBEAM, 99);

      //8th circle
      SET_SKILL(ch, SPELL_ANIMAL_SHAPES, 99);
      SET_SKILL(ch, SPELL_CONTROL_PLANTS, 99);
      SET_SKILL(ch, SPELL_EARTHQUAKE, 99);
      SET_SKILL(ch, SPELL_FINGER_OF_DEATH, 99);
      SET_SKILL(ch, SPELL_MASS_CURE_SERIOUS, 99);
      SET_SKILL(ch, SPELL_SUMMON_NATURES_ALLY_8, 99);
      SET_SKILL(ch, SPELL_SUNBURST, 99);
      SET_SKILL(ch, SPELL_WHIRLWIND, 99);
      SET_SKILL(ch, SPELL_WORD_OF_RECALL, 99);

      //9th circle
      SET_SKILL(ch, SPELL_ELEMENTAL_SWARM, 99);
      SET_SKILL(ch, SPELL_REGENERATION, 99);
      SET_SKILL(ch, SPELL_MASS_CURE_CRIT, 99);
      SET_SKILL(ch, SPELL_SHAMBLER, 99);
      SET_SKILL(ch, SPELL_POLYMORPH, 99); // should be SHAPECHANGE
      SET_SKILL(ch, SPELL_STORM_OF_VENGEANCE, 99);
      SET_SKILL(ch, SPELL_SUMMON_NATURES_ALLY_9, 99);

      send_to_char(ch, "Druid Done.\tn\r\n");
      break;

    case CLASS_PALADIN:
      //spell init
      //1st circle
      SET_SKILL(ch, SPELL_CURE_LIGHT, 99);
      SET_SKILL(ch, SPELL_ENDURANCE, 99);
      SET_SKILL(ch, SPELL_ARMOR, 99);
      //2nd circle
      SET_SKILL(ch, SPELL_CREATE_FOOD, 99);
      SET_SKILL(ch, SPELL_CREATE_WATER, 99);
      SET_SKILL(ch, SPELL_DETECT_POISON, 99);
      SET_SKILL(ch, SPELL_CURE_MODERATE, 99);
      //3rd circle
      SET_SKILL(ch, SPELL_DETECT_ALIGN, 99);
      SET_SKILL(ch, SPELL_CURE_BLIND, 99);
      SET_SKILL(ch, SPELL_BLESS, 99);
      SET_SKILL(ch, SPELL_CURE_SERIOUS, 99);
      //4th circle
      SET_SKILL(ch, SPELL_AID, 99);
      SET_SKILL(ch, SPELL_INFRAVISION, 99);
      SET_SKILL(ch, SPELL_REMOVE_CURSE, 99);
      SET_SKILL(ch, SPELL_CURE_CRITIC, 99);
      SET_SKILL(ch, SPELL_HOLY_SWORD, 99);

      send_to_char(ch, "Paladin Done.\tn\r\n");
      break;

    case CLASS_ROGUE:
      send_to_char(ch, "Rogue Done.\tn\r\n");
      break;

    case CLASS_WARRIOR:
      send_to_char(ch, "Warrior Done.\tn\r\n");
      break;

    case CLASS_RANGER:
      //spell init
      //1st circle
      SET_SKILL(ch, SPELL_CURE_LIGHT, 99);
      SET_SKILL(ch, SPELL_CHARM_ANIMAL, 99);
      SET_SKILL(ch, SPELL_FAERIE_FIRE, 99);
      SET_SKILL(ch, SPELL_JUMP, 99);
      SET_SKILL(ch, SPELL_MAGIC_FANG, 99);
      SET_SKILL(ch, SPELL_SUMMON_NATURES_ALLY_1, 99);
      //2nd circle
      SET_SKILL(ch, SPELL_ENDURANCE, 99);
      SET_SKILL(ch, SPELL_BARKSKIN, 99);
      SET_SKILL(ch, SPELL_GRACE, 99);
      SET_SKILL(ch, SPELL_HOLD_ANIMAL, 99);
      SET_SKILL(ch, SPELL_WISDOM, 99);
      SET_SKILL(ch, SPELL_STRENGTH, 99);
      SET_SKILL(ch, SPELL_SUMMON_NATURES_ALLY_2, 99);
      //3rd circle
      SET_SKILL(ch, SPELL_SPIKE_GROWTH, 99);
      SET_SKILL(ch, SPELL_GREATER_MAGIC_FANG, 99);
      SET_SKILL(ch, SPELL_CONTAGION, 99);
      SET_SKILL(ch, SPELL_CURE_MODERATE, 99);
      SET_SKILL(ch, SPELL_SUMMON_NATURES_ALLY_3, 99);
      //4th circle
      SET_SKILL(ch, SPELL_SUMMON_NATURES_ALLY_4, 99);
      SET_SKILL(ch, SPELL_FREE_MOVEMENT, 99);
      SET_SKILL(ch, SPELL_DISPEL_MAGIC, 99);
      SET_SKILL(ch, SPELL_CURE_SERIOUS, 99);

      send_to_char(ch, "Ranger Done.\tn\r\n");
      break;

    case CLASS_BERSERKER:
      send_to_char(ch, "Berserker Done.\tn\r\n");
      break;

    case CLASS_MONK:
      send_to_char(ch, "Monk Done.\tn\r\n");
      break;

    case CLASS_WEAPON_MASTER:
      send_to_char(ch, "WeaponMaster Done.\tn\r\n");
      break;

    default:
      send_to_char(ch, "None needed.\tn\r\n");
      break;
  }
}

/* not to be confused with init_char, this is exclusive right now for do_start */
void init_start_char(struct char_data *ch) {
  int trains = 0, i = 0, j = 0;

  /* clear polymorph, affections cleared below */
  SUBRACE(ch) = 0;
  IS_MORPHED(ch) = 0;
  GET_DISGUISE_RACE(ch) = 0;
  cleanup_disguise(ch);

  /* clear immortal flags */
  if (PRF_FLAGGED(ch, PRF_HOLYLIGHT))
    i = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
  if (PRF_FLAGGED(ch, PRF_NOHASSLE))
    i = PRF_TOG_CHK(ch, PRF_NOHASSLE);
  if (PRF_FLAGGED(ch, PRF_SHOWVNUMS))
    i = PRF_TOG_CHK(ch, PRF_SHOWVNUMS);
  if (PRF_FLAGGED(ch, PRF_BUILDWALK))
    i = PRF_TOG_CHK(ch, PRF_BUILDWALK);

  /* clear gear for clean start */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i))
      perform_remove(ch, i, TRUE);

  /* clear affects for clean start */
  if (ch->affected || AFF_FLAGS(ch)) {
    while (ch->affected)
      affect_remove(ch, ch->affected);
    for (i = 0; i < AF_ARRAY_MAX; i++)
      AFF_FLAGS(ch)[i] = 0;
  }

  /* initialize all levels and spec_abil array */
  for (i = 0; i < MAX_CLASSES; i++) {
    CLASS_LEVEL(ch, i) = 0;
    GET_SPEC_ABIL(ch, i) = 0;
  }
  for (i = 0; i < MAX_ENEMIES; i++)
    GET_FAVORED_ENEMY(ch, i) = 0;

  /* a bit silly, but go ahead make sure no stone-skin like spells */
  for (i = 0; i < MAX_WARDING; i++)
    GET_WARDING(ch, i) = 0;

  /* start at level 1 */
  GET_LEVEL(ch) = 1;
  CLASS_LEVEL(ch, GET_CLASS(ch)) = 1;
  GET_EXP(ch) = 1;

  /* reset title */
  set_title(ch, NULL);

  /* reset stats */
  roll_real_abils(ch);
  GET_REAL_AC(ch) = 100; /* base AC of 10 */
  GET_REAL_HITROLL(ch) = 0;
  GET_REAL_DAMROLL(ch) = 0;
  GET_REAL_MAX_HIT(ch) = 20;
  GET_REAL_MAX_MANA(ch) = 100;
  GET_REAL_MAX_MOVE(ch) = 82;
  GET_PRACTICES(ch) = 0;
  GET_TRAINS(ch) = 0;
  GET_BOOSTS(ch) = 0;
  GET_FEAT_POINTS(ch) = 0;
  GET_EPIC_FEAT_POINTS(ch) = 0;

  for (i = 0; i < NUM_CLASSES; i++) {
    GET_CLASS_FEATS(ch, i) = 0;
    GET_EPIC_CLASS_FEATS(ch, i) = 0;
  }

  GET_REAL_SPELL_RES(ch) = 0;

  /* reset skills/abilities */
  /* we don't want players to lose their hard-earned crafting skills */
  for (i = 1; i <= NUM_SKILLS; i++)
    if (spell_info[i].schoolOfMagic != CRAFTING_SKILL)
      SET_SKILL(ch, i, 0);
  for (i = 1; i <= NUM_ABILITIES; i++)
    SET_ABILITY(ch, i, 0);
  for (i = 1; i < NUM_FEATS; i++)
    SET_FEAT(ch, i, 0);
  for (i = 0; i < NUM_CFEATS; i++)
    for (j = 0; j < FT_ARRAY_MAX; j++)
      (ch)->char_specials.saved.combat_feats[(i)][j] = 0;
  for (i = 0; i < NUM_SFEATS; i++)
    (ch)->char_specials.saved.school_feats[(i)] = 0;
  for (i = 0; i < NUM_SKFEATS; i++)
    for (j = 0; j > NUM_ABILITIES; j++)
      (ch)->player_specials->saved.skill_focus[(i)][j] = 0;


  /* initialize mem data, allow adjustment of spells known */
  init_spell_slots(ch);
  IS_SORC_LEARNED(ch) = 0;
  IS_BARD_LEARNED(ch) = 0;
  IS_RANG_LEARNED(ch) = 0;
  IS_WIZ_LEARNED(ch) = 0;
  IS_DRUID_LEARNED(ch) = 0;

  /* hunger and thirst are off */
  GET_COND(ch, HUNGER) = -1;
  GET_COND(ch, THIRST) = -1;

  /* universal skills */
  switch (GET_RACE(ch)) {
    case RACE_HUMAN:
      GET_REAL_SIZE(ch) = SIZE_MEDIUM;
      GET_FEAT_POINTS(ch)++;
      trains += 3;
      break;
    case RACE_ELF:
      GET_REAL_SIZE(ch) = SIZE_MEDIUM;
      GET_REAL_DEX(ch) += 2;
      GET_REAL_CON(ch) -= 2;
      break;
    case RACE_DWARF:
      GET_REAL_SIZE(ch) = SIZE_MEDIUM;
      GET_REAL_CON(ch) += 2;
      GET_REAL_CHA(ch) -= 2;
      break;
    case RACE_HALFLING:
      GET_REAL_SIZE(ch) = SIZE_SMALL;
      GET_REAL_STR(ch) -= 2;
      GET_REAL_DEX(ch) += 2;
      break;
    case RACE_H_ELF:
      GET_REAL_SIZE(ch) = SIZE_MEDIUM;
      break;
    case RACE_H_ORC:
      GET_REAL_SIZE(ch) = SIZE_MEDIUM;
      GET_REAL_INT(ch) -= 2;
      GET_REAL_CHA(ch) -= 2;
      GET_REAL_STR(ch) += 2;
      break;
    case RACE_GNOME:
      GET_REAL_SIZE(ch) = SIZE_SMALL;
      GET_REAL_CON(ch) += 2;
      GET_REAL_STR(ch) -= 2;
      break;
    case RACE_HALF_TROLL:
      GET_REAL_SIZE(ch) = SIZE_LARGE;
      GET_REAL_CON(ch) += 2;
      GET_REAL_STR(ch) += 2;
      GET_REAL_DEX(ch) += 2;
      GET_REAL_INT(ch) -= 4;
      GET_REAL_WIS(ch) -= 4;
      GET_REAL_CHA(ch) -= 4;
      break;
    case RACE_CRYSTAL_DWARF:
      GET_REAL_SIZE(ch) = SIZE_MEDIUM;
      GET_REAL_CON(ch) += 8;
      GET_REAL_STR(ch) += 2;
      GET_REAL_CHA(ch) += 2;
      GET_REAL_WIS(ch) += 2;
      GET_MAX_HIT(ch) += 10;
      break;
    case RACE_TRELUX:
      GET_REAL_SIZE(ch) = SIZE_SMALL;
      GET_REAL_STR(ch) += 2;
      GET_REAL_DEX(ch) += 8;
      GET_REAL_CON(ch) += 4;
      GET_MAX_HIT(ch) += 10;
      break;
    case RACE_ARCANA_GOLEM:
      GET_REAL_SIZE(ch) = SIZE_MEDIUM;
      GET_REAL_CON(ch) -= 2;
      GET_REAL_STR(ch) -= 2;
      GET_REAL_INT(ch) += 2;
      GET_REAL_WIS(ch) += 2;
      GET_REAL_CHA(ch) += 2;
      break;
    default:
      GET_REAL_SIZE(ch) = SIZE_MEDIUM;
      break;
  }

  /* this is a hack - you can set your stats at level 1, which means the very
   first starting level you are going to get stats based on starting stats of 8,
   so as compensation, we're treating all starting characters as having 12 int */
  //class-related inits
  int int_bonus = GET_INT_BONUS(ch); /* this is the way it should be */
  int_bonus = 1; /* this we're forcing because of set-stats at level 1 */
  switch (GET_CLASS(ch)) {
    case CLASS_WARRIOR:
      GET_CLASS_FEATS(ch, CLASS_WARRIOR)++; /* Bonus Feat */
      /* fallthrough */
    case CLASS_WEAPON_MASTER:
    case CLASS_WIZARD:
    case CLASS_CLERIC:
      trains += MAX(1, (2 + (int) (int_bonus)) * 3);
      break;
    case CLASS_DRUID:
    case CLASS_RANGER:
    case CLASS_BERSERKER:
    case CLASS_MONK:
      trains += MAX(1, (4 + (int) (int_bonus)) * 3);
      break;
    case CLASS_BARD:
      trains += MAX(1, (6 + (int) (int_bonus)) * 3);
      break;
    case CLASS_ROGUE:
      trains += MAX(1, (8 + (int) (int_bonus)) * 3);
      break;
  }

  /* finalize */
  GET_FEAT_POINTS(ch)++; /* 1st level feat. */
  send_to_char(ch, "%d \tMFeat points gained.\tn\r\n", GET_FEAT_POINTS(ch));
  send_to_char(ch, "%d \tMClass Feat points gained.\tn\r\n", GET_CLASS_FEATS(ch, GET_CLASS(ch)));
  GET_TRAINS(ch) += trains;
  send_to_char(ch, "%d \tMTraining sessions gained.\tn\r\n", trains);
}

/* Some initializations for characters, including initial skills */
void do_start(struct char_data *ch) {
  init_start_char(ch);

  //from level 0 -> level 1
  advance_level(ch, GET_CLASS(ch));
  GET_HIT(ch) = GET_MAX_HIT(ch);
  GET_MANA(ch) = GET_MAX_MANA(ch);
  GET_MOVE(ch) = GET_MAX_MOVE(ch);
  GET_COND(ch, DRUNK) = 0;
  if (CONFIG_SITEOK_ALL)
    SET_BIT_AR(PLR_FLAGS(ch), PLR_SITEOK);
}

void process_level_feats(struct char_data *ch, int class) {
  char featbuf[MAX_STRING_LENGTH];
  int i = 0;
  //int j = 0;

  sprintf(featbuf, "\tM");

  /* increment through the list, FEAT_UNDEFINED is our terminator */
  while (level_feats[i][LF_FEAT] != FEAT_UNDEFINED) {

    /* feat i matches our class, and we meet the min-level */
    if (level_feats[i][LF_CLASS] == class &&
            level_feats[i][LF_RACE] == RACE_UNDEFINED &&
            CLASS_LEVEL(ch, level_feats[i][LF_CLASS]) >= level_feats[i][LF_MIN_LVL]) {

      /* skip this feat, we have it already? */
      if (!(
              (!HAS_REAL_FEAT(ch, level_feats[i][LF_FEAT]) &&
              CLASS_LEVEL(ch, level_feats[i][LF_CLASS]) > level_feats[i][LF_MIN_LVL] &&
              CLASS_LEVEL(ch, level_feats[i][LF_CLASS]) > 0) ||
              CLASS_LEVEL(ch, level_feats[i][LF_CLASS]) == level_feats[i][LF_MIN_LVL]
              )
              ) {
        i++;
        continue;
      }

      if (level_feats[i][LF_FEAT] == FEAT_SNEAK_ATTACK)
        sprintf(featbuf, "%s\tMYour sneak attack has increased to +%dd6!\tn\r\n", featbuf, HAS_FEAT(ch, FEAT_SNEAK_ATTACK) + 1);

      if (level_feats[i][LF_FEAT] == FEAT_SHRUG_DAMAGE) {
        struct damage_reduction_type *dr, *temp, *ptr;

        for (dr = GET_DR(ch); dr != NULL; dr = dr->next) {
          if (dr->feat == FEAT_SHRUG_DAMAGE) {
            REMOVE_FROM_LIST(dr, GET_DR(ch), next);
          }
        }

        CREATE(ptr, struct damage_reduction_type, 1);

        ptr->spell = 0;
        ptr->feat = FEAT_SHRUG_DAMAGE;
        ptr->amount = HAS_FEAT(ch, FEAT_SHRUG_DAMAGE) + 1;
        ptr->max_damage = -1;

        ptr->bypass_cat[0] = DR_BYPASS_CAT_NONE;
        ptr->bypass_val[0] = 0;
        ptr->bypass_cat[1] = DR_BYPASS_CAT_UNUSED;
        ptr->bypass_val[1] = 0; /* Unused. */
        ptr->bypass_cat[2] = DR_BYPASS_CAT_UNUSED;
        ptr->bypass_val[2] = 0; /* Unused. */

        ptr->next = GET_DR(ch);
        GET_DR(ch) = ptr;

        sprintf(featbuf, "%s\tMYou can now shrug off %d damage!\tn\r\n", featbuf, HAS_FEAT(ch, FEAT_SHRUG_DAMAGE) + 1);
      }

      if (level_feats[i][LF_FEAT] == FEAT_STRENGTH_BOOST) {
        ch->real_abils.str += 2;
        sprintf(featbuf, "%s\tMYour natural strength has increased by +2!\r\n", featbuf);
      } else if (level_feats[i][LF_FEAT] == FEAT_CHARISMA_BOOST) {
        ch->real_abils.cha += 2;
        sprintf(featbuf, "%s\tMYour natural charisma has increased by +2!\r\n", featbuf);
      } else if (level_feats[i][LF_FEAT] == FEAT_CONSTITUTION_BOOST) {
        ch->real_abils.con += 2;
        sprintf(featbuf, "%s\tMYour natural constitution has increased by +2!\r\n", featbuf);
      } else if (level_feats[i][LF_FEAT] == FEAT_INTELLIGENCE_BOOST) {
        ch->real_abils.intel += 2;
        sprintf(featbuf, "%s\tMYour natural intelligence has increased by +2!\r\n", featbuf);
      } else {
        if (HAS_FEAT(ch, level_feats[i][LF_FEAT]))
          sprintf(featbuf, "%s\tMYou have improved your %s class ability!\tn\r\n", featbuf, feat_list[level_feats[i][LF_FEAT]].name);
        else
          sprintf(featbuf, "%s\tMYou have gained the %s class ability!\tn\r\n", featbuf, feat_list[level_feats[i][LF_FEAT]].name);
      }
      SET_FEAT(ch, level_feats[i][LF_FEAT], HAS_REAL_FEAT(ch, level_feats[i][LF_FEAT]) + 1);
    }

    /* feat i doesnt matches our class or we don't meet the min-level (from if above) */
    /* non-class, racial feat and don't have it yet */
    else if (level_feats[i][LF_CLASS] == CLASS_UNDEFINED &&
            level_feats[i][LF_RACE] == GET_RACE(ch) &&
            !HAS_FEAT(ch, level_feats[i][LF_FEAT])) {
      /*
      if (level_feats[i][LF_STACK] == TRUE) {
        if (i == FEAT_TWO_WEAPON_FIGHTING && GET_CLASS(ch) == CLASS_RANGER)
          //if (!HAS_FEAT(ch, FEAT_RANGER_TWO_WEAPON_STYLE))
          continue;
      }
      */
      if (HAS_FEAT(ch, level_feats[i][LF_FEAT]))
        sprintf(featbuf, "%s\tMYou have improved your %s class ability!\tn\r\n", featbuf, feat_list[level_feats[i][LF_FEAT]].name);
      else
        sprintf(featbuf, "%s\tMYou have gained the %s class ability!\tn\r\n", featbuf, feat_list[level_feats[i][LF_FEAT]].name);
      SET_FEAT(ch, level_feats[i][LF_FEAT], HAS_REAL_FEAT(ch, level_feats[i][LF_FEAT]) + 1);
    }

    /* feat doesn't match our class or we don't meet the min-level (from if above) */
    /* matches class or doesn't match race or has feat (from if above) */
    /* class matches and race matches and meet min level */
    else if (GET_CLASS(ch) == level_feats[i][LF_CLASS] &&
            level_feats[i][LF_RACE] == GET_RACE(ch) &&
            CLASS_LEVEL(ch, level_feats[i][LF_CLASS]) == level_feats[i][LF_MIN_LVL]) {
      if (HAS_FEAT(ch, level_feats[i][LF_FEAT]))
        sprintf(featbuf, "%s\tMYou have improved your %s class ability!\tn\r\n", featbuf, feat_list[level_feats[i][LF_FEAT]].name);
      else
        sprintf(featbuf, "%s\tMYou have gained the %s class ability!\tn\r\n", featbuf, feat_list[level_feats[i][LF_FEAT]].name);
      SET_FEAT(ch, level_feats[i][LF_FEAT], HAS_REAL_FEAT(ch, level_feats[i][LF_FEAT]) + 1);
    }
    i++;
  }

  send_to_char(ch, "%s", featbuf);
}

void advance_level(struct char_data *ch, int class) {
  int add_hp = 0, at_armor = 100,
          add_mana = 0, add_move = 0, k, trains = 0;
  int feats = 0, class_feats = 0, epic_feats = 0, epic_class_feats = 0;
  int i = 0;

  /**because con items / spells are affecting based on level, we have to
  unaffect before we level up -zusuk */
  at_armor = affect_total_sub(ch); /* at_armor stores ac */
  /* done unaffecting */

  add_hp = GET_CON_BONUS(ch);

  /* first level in a class?  might have some inits to do! */
  if (CLASS_LEVEL(ch, class) == 1) {
    send_to_char(ch, "\tMInitializing class:  \r\n");
    init_class(ch, class, CLASS_LEVEL(ch, class));
    send_to_char(ch, "\r\n");
  }

  /* start our primary advancement block */
  send_to_char(ch, "\tMGAINS:\tn\r\n");

  /* class bonuses */
  switch (class) {
    case CLASS_SORCERER:
      sorc_skills(ch, CLASS_LEVEL(ch, CLASS_SORCERER));
      add_hp += rand_number(2, 4);
      add_mana = 0;
      add_move = rand_number(0, 2);

      trains += MAX(1, (2 + (GET_REAL_INT_BONUS(ch))));

      //epic
      if (!(CLASS_LEVEL(ch, class) % 3) && GET_LEVEL(ch) >= 20) {
        epic_class_feats++;
      }
      break;
    case CLASS_WIZARD:
      wizard_skills(ch, CLASS_LEVEL(ch, CLASS_WIZARD));
      add_hp += rand_number(2, 4);
      add_mana = 0;
      add_move = rand_number(0, 2);

      trains += MAX(1, (2 + (GET_REAL_INT_BONUS(ch))));

      if (!(CLASS_LEVEL(ch, class) % 5) && GET_LEVEL(ch) < 20) {
        class_feats++;
      }
      //epic
      if (!(CLASS_LEVEL(ch, class) % 3) && GET_LEVEL(ch) >= 20) {
        epic_class_feats++;
      }

      break;
    case CLASS_CLERIC:
      cleric_skills(ch, CLASS_LEVEL(ch, CLASS_CLERIC));
      add_hp += rand_number(4, 8);
      add_mana = 0;
      add_move = rand_number(0, 2);

      trains += MAX(1, (2 + (GET_REAL_INT_BONUS(ch))));

      //epic
      if (!(CLASS_LEVEL(ch, class) % 3) && GET_LEVEL(ch) >= 20) {
        epic_class_feats++;
      }
      break;
    case CLASS_ROGUE:
      rogue_skills(ch, CLASS_LEVEL(ch, CLASS_ROGUE));
      add_hp += rand_number(3, 6);
      add_mana = 0;
      add_move = rand_number(0, 2);

      trains += MAX(1, (8 + (GET_REAL_INT_BONUS(ch))));

      //epic
      if (!(CLASS_LEVEL(ch, class) % 4) && GET_LEVEL(ch) >= 20) {
        epic_class_feats++;
      }

      break;
    case CLASS_BARD:
      bard_skills(ch, CLASS_LEVEL(ch, CLASS_BARD));
      add_hp += rand_number(3, 6);
      add_mana = 0;
      add_move = rand_number(0, 2);

      trains += MAX(1, (6 + (GET_REAL_INT_BONUS(ch))));

      //epic
      if (!(CLASS_LEVEL(ch, class) % 3) && GET_LEVEL(ch) >= 20) {
        epic_class_feats++;
      }
      break;
    case CLASS_MONK:
      monk_skills(ch, CLASS_LEVEL(ch, CLASS_MONK));
      add_hp += rand_number(4, 8);
      add_mana = 0;
      add_move = rand_number(0, 2);

      trains += MAX(1, (4 + (GET_REAL_INT_BONUS(ch))));

      //epic
      if (!(CLASS_LEVEL(ch, class) % 5) && GET_LEVEL(ch) >= 20) {
        epic_class_feats++;
      }

      break;
    case CLASS_BERSERKER:
      berserker_skills(ch, CLASS_LEVEL(ch, CLASS_BERSERKER));
      add_hp += rand_number(6, 12);
      add_mana = 0;
      add_move = rand_number(0, 2);

      trains += MAX(1, (4 + (GET_REAL_INT_BONUS(ch))));

      //epic
      if (!(CLASS_LEVEL(ch, class) % 4) && GET_LEVEL(ch) >= 20) {
        epic_class_feats++;
      }

      break;
    case CLASS_DRUID:
      druid_skills(ch, CLASS_LEVEL(ch, CLASS_SORCERER));
      add_hp += rand_number(4, 8);
      add_mana = 0;
      add_move = rand_number(0, 2);

      trains += MAX(1, (4 + (GET_REAL_INT_BONUS(ch))));

      //epic
      if (!(CLASS_LEVEL(ch, class) % 4) && GET_LEVEL(ch) >= 20) {
        epic_class_feats++;
      }

      break;
    case CLASS_RANGER:
      ranger_skills(ch, CLASS_LEVEL(ch, CLASS_RANGER));
      add_hp += rand_number(5, 10);
      add_mana = 0;
      add_move = rand_number(0, 2);

      trains += MAX(1, (4 + (GET_REAL_INT_BONUS(ch))));

      //epic
      if (!(CLASS_LEVEL(ch, class) % 3) && GET_LEVEL(ch) >= 20) {
        epic_class_feats++;
      }

      break;
    case CLASS_PALADIN:
      paladin_skills(ch, CLASS_LEVEL(ch, CLASS_PALADIN));
      add_hp += rand_number(5, 10);
      add_mana = 0;
      add_move = rand_number(0, 2);

      trains += MAX(1, (2 + (GET_REAL_INT_BONUS(ch))));

      //epic
      if (!(CLASS_LEVEL(ch, class) % 3) && GET_LEVEL(ch) >= 20) {
        epic_class_feats++;
      }

      break;
    case CLASS_WARRIOR:
      warrior_skills(ch, CLASS_LEVEL(ch, CLASS_WARRIOR));
      add_hp += rand_number(5, 10);
      add_mana = 0;
      add_move = rand_number(0, 2);

      trains += MAX(1, (2 + (GET_REAL_INT_BONUS(ch))));
      if (!(CLASS_LEVEL(ch, class) % 2) && !IS_EPIC(ch)) {
        class_feats++;
      }
      if (!(CLASS_LEVEL(ch, class) % 2) && IS_EPIC(ch)) {
        epic_class_feats++;
      }

      break;
    case CLASS_WEAPON_MASTER:
      weaponmaster_skills(ch, CLASS_LEVEL(ch, CLASS_WEAPON_MASTER));
      add_hp += rand_number(5, 10);
      add_mana = 0;
      add_move = rand_number(0, 2);

      trains += MAX(1, (2 + (GET_REAL_INT_BONUS(ch))));

      //epic
      if (!(CLASS_LEVEL(ch, class) % 3) && GET_LEVEL(ch) >= 20) {
        epic_class_feats++;
      }

      break;
  }

  /* further movement modifications */
  if (HAS_FEAT(ch, FEAT_ENDURANCE)) {
    add_move += rand_number(1, 2);
  }
  if (HAS_FEAT(ch, FEAT_FAST_MOVEMENT)) {
    add_move += rand_number(1, 2);
  }

  process_level_feats(ch, class);

  //Racial Bonuses
  switch (GET_RACE(ch)) {
    case RACE_HUMAN:
      trains++;
      break;
    case RACE_CRYSTAL_DWARF:
      add_hp += 4;
      break;
    case RACE_TRELUX:
      add_hp += 4;
      break;
    default:
      break;
  }

  //base practice / boost improvement
  if (!(GET_LEVEL(ch) % 3) && !IS_EPIC(ch)) {
    feats++;
  }
  if (!(GET_LEVEL(ch) % 3) && IS_EPIC(ch)) {
    epic_feats++;
  }
  if (!(GET_LEVEL(ch) % 4)) {
    GET_BOOSTS(ch)++;
    send_to_char(ch, "\tMYou gain a boost (to stats) point!\tn\r\n");
  }

  /* miscellaneous level-based bonuses */
  if (HAS_FEAT(ch, FEAT_TOUGHNESS)) {
    /* SRD has this as +3 hp.  More fun as +1 per level. */
    for (i = HAS_FEAT(ch, FEAT_TOUGHNESS); i > 0; i--)
      add_hp++;
  }
  if (HAS_FEAT(ch, FEAT_EPIC_TOUGHNESS)) {
    /* SRD has this listed as +30 hp.  More fun to do it by level perhaps. */
    for (i = HAS_FEAT(ch, FEAT_EPIC_TOUGHNESS); i > 0; i--)
      add_hp += 1;
  }

  /* adjust final and report changes! */
  GET_REAL_MAX_HIT(ch) += MAX(1, add_hp);
  send_to_char(ch, "\tMTotal HP:\tn %d\r\n", MAX(1, add_hp));
  GET_REAL_MAX_MOVE(ch) += MAX(1, add_move);
  send_to_char(ch, "\tMTotal Move:\tn %d\r\n", MAX(1, add_move));
  if (GET_LEVEL(ch) > 1) {
    GET_REAL_MAX_MANA(ch) += add_mana;
    send_to_char(ch, "\tMTotal Mana:\tn %d\r\n", add_mana);
  }
  GET_FEAT_POINTS(ch) += feats;
  GET_CLASS_FEATS(ch, GET_CLASS(ch)) += class_feats;
  GET_EPIC_FEAT_POINTS(ch) += epic_feats;
  GET_EPIC_CLASS_FEATS(ch, GET_CLASS(ch)) += epic_class_feats;
  if (feats)
    send_to_char(ch, "%d \tMFeat points gained.\tn\r\n", feats);
  if (class_feats)
    send_to_char(ch, "%d \tMClass feat points gained.\tn\r\n", class_feats);
  if (epic_feats)
    send_to_char(ch, "%d \tMEpic feat points gained.\tn\r\n", epic_feats);
  if (epic_class_feats)
    send_to_char(ch, "%d \tMEpic class feat points gained.\tn\r\n", epic_class_feats);
  GET_TRAINS(ch) += trains;
  send_to_char(ch, "%d \tMTraining sessions gained.\tn\r\n", trains);
  /*******/
  /* end advancement block */
  /*******/

  /**** reaffect ****/
  affect_total_plus(ch, at_armor);
  /* end reaffecting */

  /* give immorts some goodies */
  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    for (k = 0; k < 3; k++)
      GET_COND(ch, k) = (char) - 1;
    SET_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
    send_to_char(ch, "Setting \tRHOLYLIGHT\tn on.\r\n");
    SET_BIT_AR(PRF_FLAGS(ch), PRF_NOHASSLE);
    send_to_char(ch, "Setting \tRNOHASSLE\tn on.\r\n");
    SET_BIT_AR(PRF_FLAGS(ch), PRF_SHOWVNUMS);
    send_to_char(ch, "Setting \tRSHOWVNUMS\tn on.\r\n");
  }

  /* make sure you aren't snooping someone you shouldn't with new level */
  snoop_check(ch);
  save_char(ch, 0);
}

int backstab_mult(struct char_data *ch) {

  if (HAS_FEAT(ch, FEAT_BACKSTAB))
    return 2;

  return 1;
}


// used by handler.c

int invalid_class(struct char_data *ch, struct obj_data *obj) {
  /* this is all deprecated by the proficiency system */
  /*
  if (OBJ_FLAGGED(obj, ITEM_ANTI_WIZARD) && IS_WIZARD(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_SORCERER) && IS_SORCERER(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_CLERIC) && IS_CLERIC(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_RANGER) && IS_RANGER(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_PALADIN) && IS_PALADIN(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_WARRIOR) && IS_WARRIOR(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_WEAPON_MASTER) && IS_WEAPONMASTER(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_MONK) && IS_MONK(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_BERSERKER) && IS_BERSERKER(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_DRUID) && IS_DRUID(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_BARD) && IS_BARD(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_ROGUE) && IS_ROGUE(ch))
    return TRUE;
   */

  return FALSE;
}


// vital min level info!

void init_spell_levels(void) {
  int i = 0, j = 0;

  // simple loop to init all the SKILL_x to all classes
  for (i = (MAX_SPELLS + 1); i < NUM_SKILLS; i++) {
    for (j = 0; j < NUM_CLASSES; j++) {
      spell_level(i, j, 1);
    }
  }

  // wizard innate cantrips
  /*
  spell_level(SPELL_ACID_SPLASH, CLASS_WIZARD, 1);
  spell_level(SPELL_RAY_OF_FROST, CLASS_WIZARD, 1);
   */

  // wizard, increment spells by spell-level
  //1st circle
  spell_level(SPELL_MAGIC_MISSILE, CLASS_WIZARD, 1);
  spell_level(SPELL_HORIZIKAULS_BOOM, CLASS_WIZARD, 1);
  spell_level(SPELL_BURNING_HANDS, CLASS_WIZARD, 1);
  spell_level(SPELL_ICE_DAGGER, CLASS_WIZARD, 1);
  spell_level(SPELL_MAGE_ARMOR, CLASS_WIZARD, 1);
  spell_level(SPELL_SUMMON_CREATURE_1, CLASS_WIZARD, 1);
  spell_level(SPELL_CHILL_TOUCH, CLASS_WIZARD, 1);
  spell_level(SPELL_NEGATIVE_ENERGY_RAY, CLASS_WIZARD, 1);
  spell_level(SPELL_RAY_OF_ENFEEBLEMENT, CLASS_WIZARD, 1);
  spell_level(SPELL_CHARM, CLASS_WIZARD, 1);
  spell_level(SPELL_ENCHANT_WEAPON, CLASS_WIZARD, 1);
  spell_level(SPELL_SLEEP, CLASS_WIZARD, 1);
  spell_level(SPELL_COLOR_SPRAY, CLASS_WIZARD, 1);
  spell_level(SPELL_SCARE, CLASS_WIZARD, 1);
  spell_level(SPELL_TRUE_STRIKE, CLASS_WIZARD, 1);
  spell_level(SPELL_IDENTIFY, CLASS_WIZARD, 1);
  spell_level(SPELL_SHELGARNS_BLADE, CLASS_WIZARD, 1);
  spell_level(SPELL_GREASE, CLASS_WIZARD, 1);
  spell_level(SPELL_ENDURE_ELEMENTS, CLASS_WIZARD, 1);
  spell_level(SPELL_PROT_FROM_EVIL, CLASS_WIZARD, 1);
  spell_level(SPELL_PROT_FROM_GOOD, CLASS_WIZARD, 1);
  spell_level(SPELL_EXPEDITIOUS_RETREAT, CLASS_WIZARD, 1);
  spell_level(SPELL_IRON_GUTS, CLASS_WIZARD, 1);
  spell_level(SPELL_SHIELD, CLASS_WIZARD, 1);

  //2nd circle
  spell_level(SPELL_SHOCKING_GRASP, CLASS_WIZARD, 3);
  spell_level(SPELL_SCORCHING_RAY, CLASS_WIZARD, 3);
  spell_level(SPELL_CONTINUAL_FLAME, CLASS_WIZARD, 3);
  spell_level(SPELL_SUMMON_CREATURE_2, CLASS_WIZARD, 3);
  spell_level(SPELL_WEB, CLASS_WIZARD, 3);
  spell_level(SPELL_ACID_ARROW, CLASS_WIZARD, 3);
  spell_level(SPELL_BLINDNESS, CLASS_WIZARD, 3);
  spell_level(SPELL_DEAFNESS, CLASS_WIZARD, 3);
  spell_level(SPELL_FALSE_LIFE, CLASS_WIZARD, 3);
  spell_level(SPELL_DAZE_MONSTER, CLASS_WIZARD, 3);
  spell_level(SPELL_HIDEOUS_LAUGHTER, CLASS_WIZARD, 3);
  spell_level(SPELL_TOUCH_OF_IDIOCY, CLASS_WIZARD, 3);
  spell_level(SPELL_BLUR, CLASS_WIZARD, 3);
  spell_level(SPELL_INVISIBLE, CLASS_WIZARD, 3);
  spell_level(SPELL_MIRROR_IMAGE, CLASS_WIZARD, 3);
  spell_level(SPELL_DETECT_INVIS, CLASS_WIZARD, 3);
  spell_level(SPELL_DETECT_MAGIC, CLASS_WIZARD, 3);
  spell_level(SPELL_DARKNESS, CLASS_WIZARD, 3);
  spell_level(SPELL_RESIST_ENERGY, CLASS_WIZARD, 3);
  spell_level(SPELL_ENERGY_SPHERE, CLASS_WIZARD, 3);
  spell_level(SPELL_ENDURANCE, CLASS_WIZARD, 3); //shared
  spell_level(SPELL_STRENGTH, CLASS_WIZARD, 3);
  spell_level(SPELL_GRACE, CLASS_WIZARD, 3);

  //3rd circle
  spell_level(SPELL_LIGHTNING_BOLT, CLASS_WIZARD, 5);
  spell_level(SPELL_FIREBALL, CLASS_WIZARD, 5);
  spell_level(SPELL_WATER_BREATHE, CLASS_WIZARD, 5);
  spell_level(SPELL_NON_DETECTION, CLASS_WIZARD, 5);
  spell_level(SPELL_CLAIRVOYANCE, CLASS_WIZARD, 5);
  spell_level(SPELL_DAYLIGHT, CLASS_WIZARD, 5);
  spell_level(SPELL_INVISIBILITY_SPHERE, CLASS_WIZARD, 5);
  spell_level(SPELL_WALL_OF_FOG, CLASS_WIZARD, 5);
  spell_level(SPELL_DEEP_SLUMBER, CLASS_WIZARD, 5);
  spell_level(SPELL_HOLD_PERSON, CLASS_WIZARD, 5);
  spell_level(SPELL_FLY, CLASS_WIZARD, 5);
  spell_level(SPELL_HEROISM, CLASS_WIZARD, 5);
  spell_level(SPELL_VAMPIRIC_TOUCH, CLASS_WIZARD, 5);
  spell_level(SPELL_HALT_UNDEAD, CLASS_WIZARD, 5);
  spell_level(SPELL_STINKING_CLOUD, CLASS_WIZARD, 5);
  spell_level(SPELL_PHANTOM_STEED, CLASS_WIZARD, 5);
  spell_level(SPELL_SUMMON_CREATURE_3, CLASS_WIZARD, 5);
  spell_level(SPELL_DISPEL_MAGIC, CLASS_WIZARD, 5);
  spell_level(SPELL_HASTE, CLASS_WIZARD, 5);
  spell_level(SPELL_SLOW, CLASS_WIZARD, 5);
  spell_level(SPELL_CIRCLE_A_EVIL, CLASS_WIZARD, 5);
  spell_level(SPELL_CIRCLE_A_GOOD, CLASS_WIZARD, 5);
  spell_level(SPELL_CUNNING, CLASS_WIZARD, 5);
  spell_level(SPELL_WISDOM, CLASS_WIZARD, 5);
  spell_level(SPELL_CHARISMA, CLASS_WIZARD, 5);

  //4th circle
  spell_level(SPELL_FIRE_SHIELD, CLASS_WIZARD, 7);
  spell_level(SPELL_COLD_SHIELD, CLASS_WIZARD, 7);
  spell_level(SPELL_ICE_STORM, CLASS_WIZARD, 7);
  spell_level(SPELL_BILLOWING_CLOUD, CLASS_WIZARD, 7);
  spell_level(SPELL_SUMMON_CREATURE_4, CLASS_WIZARD, 7);
  spell_level(SPELL_ANIMATE_DEAD, CLASS_WIZARD, 7); //shared
  spell_level(SPELL_CURSE, CLASS_WIZARD, 7); //shared
  spell_level(SPELL_INFRAVISION, CLASS_WIZARD, 7); //shared
  spell_level(SPELL_POISON, CLASS_WIZARD, 7); //shared
  spell_level(SPELL_GREATER_INVIS, CLASS_WIZARD, 7);
  spell_level(SPELL_RAINBOW_PATTERN, CLASS_WIZARD, 7);
  spell_level(SPELL_WIZARD_EYE, CLASS_WIZARD, 7);
  spell_level(SPELL_LOCATE_CREATURE, CLASS_WIZARD, 7);
  spell_level(SPELL_MINOR_GLOBE, CLASS_WIZARD, 7);
  spell_level(SPELL_REMOVE_CURSE, CLASS_WIZARD, 7);
  spell_level(SPELL_STONESKIN, CLASS_WIZARD, 7);
  spell_level(SPELL_ENLARGE_PERSON, CLASS_WIZARD, 7);
  spell_level(SPELL_SHRINK_PERSON, CLASS_WIZARD, 7);

  //5th circle
  spell_level(SPELL_INTERPOSING_HAND, CLASS_WIZARD, 9);
  spell_level(SPELL_WALL_OF_FORCE, CLASS_WIZARD, 9);
  spell_level(SPELL_BALL_OF_LIGHTNING, CLASS_WIZARD, 9);
  spell_level(SPELL_CLOUDKILL, CLASS_WIZARD, 9);
  spell_level(SPELL_SUMMON_CREATURE_5, CLASS_WIZARD, 9);
  spell_level(SPELL_WAVES_OF_FATIGUE, CLASS_WIZARD, 9);
  spell_level(SPELL_SYMBOL_OF_PAIN, CLASS_WIZARD, 9);
  spell_level(SPELL_DOMINATE_PERSON, CLASS_WIZARD, 9);
  spell_level(SPELL_FEEBLEMIND, CLASS_WIZARD, 9);
  spell_level(SPELL_NIGHTMARE, CLASS_WIZARD, 9);
  spell_level(SPELL_MIND_FOG, CLASS_WIZARD, 9);
  spell_level(SPELL_ACID_SHEATH, CLASS_WIZARD, 9);
  spell_level(SPELL_FAITHFUL_HOUND, CLASS_WIZARD, 9);
  spell_level(SPELL_DISMISSAL, CLASS_WIZARD, 9);
  spell_level(SPELL_CONE_OF_COLD, CLASS_WIZARD, 9);
  spell_level(SPELL_TELEKINESIS, CLASS_WIZARD, 9);
  spell_level(SPELL_FIREBRAND, CLASS_WIZARD, 9);

  //6th circle
  spell_level(SPELL_FREEZING_SPHERE, CLASS_WIZARD, 11);
  spell_level(SPELL_ACID_FOG, CLASS_WIZARD, 11);
  spell_level(SPELL_SUMMON_CREATURE_6, CLASS_WIZARD, 11);
  spell_level(SPELL_TRANSFORMATION, CLASS_WIZARD, 11);
  spell_level(SPELL_EYEBITE, CLASS_WIZARD, 11);
  spell_level(SPELL_MASS_HASTE, CLASS_WIZARD, 11);
  spell_level(SPELL_GREATER_HEROISM, CLASS_WIZARD, 11);
  spell_level(SPELL_ANTI_MAGIC_FIELD, CLASS_WIZARD, 11);
  spell_level(SPELL_GREATER_MIRROR_IMAGE, CLASS_WIZARD, 11);
  spell_level(SPELL_LOCATE_OBJECT, CLASS_WIZARD, 11);
  spell_level(SPELL_TRUE_SEEING, CLASS_WIZARD, 11);
  spell_level(SPELL_GLOBE_OF_INVULN, CLASS_WIZARD, 11);
  spell_level(SPELL_GREATER_DISPELLING, CLASS_WIZARD, 11);
  spell_level(SPELL_CLONE, CLASS_WIZARD, 11);
  spell_level(SPELL_WATERWALK, CLASS_WIZARD, 11);

  //7th circle
  spell_level(SPELL_MISSILE_STORM, CLASS_WIZARD, 13);
  spell_level(SPELL_GRASPING_HAND, CLASS_WIZARD, 13);
  spell_level(SPELL_SUMMON_CREATURE_7, CLASS_WIZARD, 13);
  spell_level(SPELL_CONTROL_WEATHER, CLASS_WIZARD, 13);
  spell_level(SPELL_POWER_WORD_BLIND, CLASS_WIZARD, 13);
  spell_level(SPELL_WAVES_OF_EXHAUSTION, CLASS_WIZARD, 13);
  spell_level(SPELL_MASS_HOLD_PERSON, CLASS_WIZARD, 13);
  spell_level(SPELL_MASS_FLY, CLASS_WIZARD, 13);
  spell_level(SPELL_DISPLACEMENT, CLASS_WIZARD, 13);
  spell_level(SPELL_PRISMATIC_SPRAY, CLASS_WIZARD, 13);
  spell_level(SPELL_DETECT_POISON, CLASS_WIZARD, 13); //shared
  spell_level(SPELL_POWER_WORD_STUN, CLASS_WIZARD, 13);
  spell_level(SPELL_PROTECT_FROM_SPELLS, CLASS_WIZARD, 13);
  spell_level(SPELL_THUNDERCLAP, CLASS_WIZARD, 13);
  spell_level(SPELL_SPELL_MANTLE, CLASS_WIZARD, 13);
  spell_level(SPELL_TELEPORT, CLASS_WIZARD, 13);
  spell_level(SPELL_MASS_WISDOM, CLASS_WIZARD, 13); //shared
  spell_level(SPELL_MASS_CHARISMA, CLASS_WIZARD, 13); //shared
  spell_level(SPELL_MASS_CUNNING, CLASS_WIZARD, 13); //shared

  //8th circle
  spell_level(SPELL_CLENCHED_FIST, CLASS_WIZARD, 15);
  spell_level(SPELL_CHAIN_LIGHTNING, CLASS_WIZARD, 15);
  spell_level(SPELL_INCENDIARY_CLOUD, CLASS_WIZARD, 15);
  spell_level(SPELL_SUMMON_CREATURE_8, CLASS_WIZARD, 15);
  spell_level(SPELL_HORRID_WILTING, CLASS_WIZARD, 15);
  spell_level(SPELL_GREATER_ANIMATION, CLASS_WIZARD, 15);
  spell_level(SPELL_IRRESISTIBLE_DANCE, CLASS_WIZARD, 15);
  spell_level(SPELL_MASS_DOMINATION, CLASS_WIZARD, 15);
  spell_level(SPELL_SCINT_PATTERN, CLASS_WIZARD, 15);
  spell_level(SPELL_REFUGE, CLASS_WIZARD, 15);
  spell_level(SPELL_BANISH, CLASS_WIZARD, 15);
  spell_level(SPELL_SUNBURST, CLASS_WIZARD, 15);
  spell_level(SPELL_SPELL_TURNING, CLASS_WIZARD, 15);
  spell_level(SPELL_MIND_BLANK, CLASS_WIZARD, 15);
  spell_level(SPELL_IRONSKIN, CLASS_WIZARD, 15);
  spell_level(SPELL_PORTAL, CLASS_WIZARD, 15);

  //9th circle
  spell_level(SPELL_METEOR_SWARM, CLASS_WIZARD, 17);
  spell_level(SPELL_BLADE_OF_DISASTER, CLASS_WIZARD, 17);
  spell_level(SPELL_SUMMON_CREATURE_9, CLASS_WIZARD, 17);
  spell_level(SPELL_GATE, CLASS_WIZARD, 17);
  spell_level(SPELL_ENERGY_DRAIN, CLASS_WIZARD, 17); //shared
  spell_level(SPELL_WAIL_OF_THE_BANSHEE, CLASS_WIZARD, 17);
  spell_level(SPELL_POWER_WORD_KILL, CLASS_WIZARD, 17);
  spell_level(SPELL_ENFEEBLEMENT, CLASS_WIZARD, 17);
  spell_level(SPELL_WEIRD, CLASS_WIZARD, 17);
  spell_level(SPELL_SHADOW_SHIELD, CLASS_WIZARD, 17);
  spell_level(SPELL_PRISMATIC_SPHERE, CLASS_WIZARD, 17);
  spell_level(SPELL_IMPLODE, CLASS_WIZARD, 17);
  spell_level(SPELL_TIMESTOP, CLASS_WIZARD, 17);
  spell_level(SPELL_GREATER_SPELL_MANTLE, CLASS_WIZARD, 17);
  spell_level(SPELL_POLYMORPH, CLASS_WIZARD, 17);
  spell_level(SPELL_MASS_ENHANCE, CLASS_WIZARD, 17);

  //epic wizard
  spell_level(SPELL_MUMMY_DUST, CLASS_WIZARD, 20); //shared
  spell_level(SPELL_DRAGON_KNIGHT, CLASS_WIZARD, 20); //shared
  spell_level(SPELL_GREATER_RUIN, CLASS_WIZARD, 20); //shared
  spell_level(SPELL_HELLBALL, CLASS_WIZARD, 20); //shared
  spell_level(SPELL_EPIC_MAGE_ARMOR, CLASS_WIZARD, 20);
  spell_level(SPELL_EPIC_WARDING, CLASS_WIZARD, 20);
  //end wizard spells


  // sorcerer, increment spells by spell-level
  //1st circle
  spell_level(SPELL_MAGIC_MISSILE, CLASS_SORCERER, 1);
  spell_level(SPELL_HORIZIKAULS_BOOM, CLASS_SORCERER, 1);
  spell_level(SPELL_BURNING_HANDS, CLASS_SORCERER, 1);
  spell_level(SPELL_ICE_DAGGER, CLASS_SORCERER, 1);
  spell_level(SPELL_MAGE_ARMOR, CLASS_SORCERER, 1);
  spell_level(SPELL_SUMMON_CREATURE_1, CLASS_SORCERER, 1);
  spell_level(SPELL_CHILL_TOUCH, CLASS_SORCERER, 1);
  spell_level(SPELL_NEGATIVE_ENERGY_RAY, CLASS_SORCERER, 1);
  spell_level(SPELL_RAY_OF_ENFEEBLEMENT, CLASS_SORCERER, 1);
  spell_level(SPELL_CHARM, CLASS_SORCERER, 1);
  spell_level(SPELL_ENCHANT_WEAPON, CLASS_SORCERER, 1);
  spell_level(SPELL_SLEEP, CLASS_SORCERER, 1);
  spell_level(SPELL_COLOR_SPRAY, CLASS_SORCERER, 1);
  spell_level(SPELL_SCARE, CLASS_SORCERER, 1);
  spell_level(SPELL_TRUE_STRIKE, CLASS_SORCERER, 1);
  spell_level(SPELL_IDENTIFY, CLASS_SORCERER, 1);
  spell_level(SPELL_SHELGARNS_BLADE, CLASS_SORCERER, 1);
  spell_level(SPELL_GREASE, CLASS_SORCERER, 1);
  spell_level(SPELL_ENDURE_ELEMENTS, CLASS_SORCERER, 1);
  spell_level(SPELL_PROT_FROM_EVIL, CLASS_SORCERER, 1);
  spell_level(SPELL_PROT_FROM_GOOD, CLASS_SORCERER, 1);
  spell_level(SPELL_EXPEDITIOUS_RETREAT, CLASS_SORCERER, 1);
  spell_level(SPELL_IRON_GUTS, CLASS_SORCERER, 1);
  spell_level(SPELL_SHIELD, CLASS_SORCERER, 1);

  //2nd circle
  spell_level(SPELL_SHOCKING_GRASP, CLASS_SORCERER, 4);
  spell_level(SPELL_SCORCHING_RAY, CLASS_SORCERER, 4);
  spell_level(SPELL_CONTINUAL_FLAME, CLASS_SORCERER, 4);
  spell_level(SPELL_SUMMON_CREATURE_2, CLASS_SORCERER, 4);
  spell_level(SPELL_WEB, CLASS_SORCERER, 4);
  spell_level(SPELL_ACID_ARROW, CLASS_SORCERER, 4);
  spell_level(SPELL_BLINDNESS, CLASS_SORCERER, 4);
  spell_level(SPELL_DEAFNESS, CLASS_SORCERER, 4);
  spell_level(SPELL_FALSE_LIFE, CLASS_SORCERER, 4);
  spell_level(SPELL_DAZE_MONSTER, CLASS_SORCERER, 4);
  spell_level(SPELL_HIDEOUS_LAUGHTER, CLASS_SORCERER, 4);
  spell_level(SPELL_TOUCH_OF_IDIOCY, CLASS_SORCERER, 4);
  spell_level(SPELL_BLUR, CLASS_SORCERER, 4);
  spell_level(SPELL_INVISIBLE, CLASS_SORCERER, 4);
  spell_level(SPELL_MIRROR_IMAGE, CLASS_SORCERER, 4);
  spell_level(SPELL_DETECT_INVIS, CLASS_SORCERER, 4);
  spell_level(SPELL_DETECT_MAGIC, CLASS_SORCERER, 4);
  spell_level(SPELL_DARKNESS, CLASS_SORCERER, 4);
  spell_level(SPELL_RESIST_ENERGY, CLASS_SORCERER, 4);
  spell_level(SPELL_ENERGY_SPHERE, CLASS_SORCERER, 4);
  spell_level(SPELL_ENDURANCE, CLASS_SORCERER, 4); //shared
  spell_level(SPELL_STRENGTH, CLASS_SORCERER, 4);
  spell_level(SPELL_GRACE, CLASS_SORCERER, 4);

  //3rd circle
  spell_level(SPELL_LIGHTNING_BOLT, CLASS_SORCERER, 6);
  spell_level(SPELL_FIREBALL, CLASS_SORCERER, 6);
  spell_level(SPELL_WATER_BREATHE, CLASS_SORCERER, 6);
  spell_level(SPELL_NON_DETECTION, CLASS_SORCERER, 6);
  spell_level(SPELL_CLAIRVOYANCE, CLASS_SORCERER, 6);
  spell_level(SPELL_DAYLIGHT, CLASS_SORCERER, 6);
  spell_level(SPELL_INVISIBILITY_SPHERE, CLASS_SORCERER, 6);
  spell_level(SPELL_WALL_OF_FOG, CLASS_SORCERER, 6);
  spell_level(SPELL_DEEP_SLUMBER, CLASS_SORCERER, 6);
  spell_level(SPELL_HOLD_PERSON, CLASS_SORCERER, 6);
  spell_level(SPELL_FLY, CLASS_SORCERER, 6);
  spell_level(SPELL_HEROISM, CLASS_SORCERER, 6);
  spell_level(SPELL_VAMPIRIC_TOUCH, CLASS_SORCERER, 6);
  spell_level(SPELL_HALT_UNDEAD, CLASS_SORCERER, 6);
  spell_level(SPELL_STINKING_CLOUD, CLASS_SORCERER, 6);
  spell_level(SPELL_PHANTOM_STEED, CLASS_SORCERER, 6);
  spell_level(SPELL_SUMMON_CREATURE_3, CLASS_SORCERER, 6);
  spell_level(SPELL_DISPEL_MAGIC, CLASS_SORCERER, 6);
  spell_level(SPELL_HASTE, CLASS_SORCERER, 6);
  spell_level(SPELL_SLOW, CLASS_SORCERER, 6);
  spell_level(SPELL_CIRCLE_A_EVIL, CLASS_SORCERER, 6);
  spell_level(SPELL_CIRCLE_A_GOOD, CLASS_SORCERER, 6);
  spell_level(SPELL_CUNNING, CLASS_SORCERER, 6);
  spell_level(SPELL_WISDOM, CLASS_SORCERER, 6);
  spell_level(SPELL_CHARISMA, CLASS_SORCERER, 6);

  //4th circle
  spell_level(SPELL_FIRE_SHIELD, CLASS_SORCERER, 8);
  spell_level(SPELL_COLD_SHIELD, CLASS_SORCERER, 8);
  spell_level(SPELL_ICE_STORM, CLASS_SORCERER, 8);
  spell_level(SPELL_BILLOWING_CLOUD, CLASS_SORCERER, 8);
  spell_level(SPELL_SUMMON_CREATURE_4, CLASS_SORCERER, 8);
  spell_level(SPELL_ANIMATE_DEAD, CLASS_SORCERER, 8); //shared
  spell_level(SPELL_CURSE, CLASS_SORCERER, 8); //shared
  spell_level(SPELL_INFRAVISION, CLASS_SORCERER, 8); //shared
  spell_level(SPELL_POISON, CLASS_SORCERER, 8); //shared
  spell_level(SPELL_GREATER_INVIS, CLASS_SORCERER, 8);
  spell_level(SPELL_RAINBOW_PATTERN, CLASS_SORCERER, 8);
  spell_level(SPELL_WIZARD_EYE, CLASS_SORCERER, 8);
  spell_level(SPELL_LOCATE_CREATURE, CLASS_SORCERER, 8);
  spell_level(SPELL_MINOR_GLOBE, CLASS_SORCERER, 8);
  spell_level(SPELL_REMOVE_CURSE, CLASS_SORCERER, 8);
  spell_level(SPELL_STONESKIN, CLASS_SORCERER, 8);
  spell_level(SPELL_ENLARGE_PERSON, CLASS_SORCERER, 8);
  spell_level(SPELL_SHRINK_PERSON, CLASS_SORCERER, 8);

  //5th circle
  spell_level(SPELL_INTERPOSING_HAND, CLASS_SORCERER, 10);
  spell_level(SPELL_WALL_OF_FORCE, CLASS_SORCERER, 10);
  spell_level(SPELL_BALL_OF_LIGHTNING, CLASS_SORCERER, 10);
  spell_level(SPELL_CLOUDKILL, CLASS_SORCERER, 10);
  spell_level(SPELL_SUMMON_CREATURE_5, CLASS_SORCERER, 10);
  spell_level(SPELL_WAVES_OF_FATIGUE, CLASS_SORCERER, 10);
  spell_level(SPELL_SYMBOL_OF_PAIN, CLASS_SORCERER, 10);
  spell_level(SPELL_DOMINATE_PERSON, CLASS_SORCERER, 10);
  spell_level(SPELL_FEEBLEMIND, CLASS_SORCERER, 10);
  spell_level(SPELL_NIGHTMARE, CLASS_SORCERER, 10);
  spell_level(SPELL_MIND_FOG, CLASS_SORCERER, 10);
  spell_level(SPELL_ACID_SHEATH, CLASS_SORCERER, 10);
  spell_level(SPELL_FAITHFUL_HOUND, CLASS_SORCERER, 10);
  spell_level(SPELL_DISMISSAL, CLASS_SORCERER, 10);
  spell_level(SPELL_CONE_OF_COLD, CLASS_SORCERER, 10);
  spell_level(SPELL_TELEKINESIS, CLASS_SORCERER, 10);
  spell_level(SPELL_FIREBRAND, CLASS_SORCERER, 10);

  //6th circle
  spell_level(SPELL_FREEZING_SPHERE, CLASS_SORCERER, 12);
  spell_level(SPELL_ACID_FOG, CLASS_SORCERER, 12);
  spell_level(SPELL_SUMMON_CREATURE_6, CLASS_SORCERER, 12);
  spell_level(SPELL_TRANSFORMATION, CLASS_SORCERER, 12);
  spell_level(SPELL_EYEBITE, CLASS_SORCERER, 12);
  spell_level(SPELL_MASS_HASTE, CLASS_SORCERER, 12);
  spell_level(SPELL_GREATER_HEROISM, CLASS_SORCERER, 12);
  spell_level(SPELL_ANTI_MAGIC_FIELD, CLASS_SORCERER, 12);
  spell_level(SPELL_GREATER_MIRROR_IMAGE, CLASS_SORCERER, 12);
  spell_level(SPELL_LOCATE_OBJECT, CLASS_SORCERER, 12);
  spell_level(SPELL_TRUE_SEEING, CLASS_SORCERER, 12);
  spell_level(SPELL_GLOBE_OF_INVULN, CLASS_SORCERER, 12);
  spell_level(SPELL_GREATER_DISPELLING, CLASS_SORCERER, 12);
  spell_level(SPELL_CLONE, CLASS_SORCERER, 12);
  spell_level(SPELL_WATERWALK, CLASS_SORCERER, 12);

  //7th circle
  spell_level(SPELL_MISSILE_STORM, CLASS_SORCERER, 14);
  spell_level(SPELL_GRASPING_HAND, CLASS_SORCERER, 14);
  spell_level(SPELL_SUMMON_CREATURE_7, CLASS_SORCERER, 14);
  spell_level(SPELL_CONTROL_WEATHER, CLASS_SORCERER, 14);
  spell_level(SPELL_POWER_WORD_BLIND, CLASS_SORCERER, 14);
  spell_level(SPELL_WAVES_OF_EXHAUSTION, CLASS_SORCERER, 14);
  spell_level(SPELL_MASS_HOLD_PERSON, CLASS_SORCERER, 14);
  spell_level(SPELL_MASS_FLY, CLASS_SORCERER, 14);
  spell_level(SPELL_DISPLACEMENT, CLASS_SORCERER, 14);
  spell_level(SPELL_PRISMATIC_SPRAY, CLASS_SORCERER, 14);
  spell_level(SPELL_DETECT_POISON, CLASS_SORCERER, 14); //shared
  spell_level(SPELL_POWER_WORD_STUN, CLASS_SORCERER, 14);
  spell_level(SPELL_PROTECT_FROM_SPELLS, CLASS_SORCERER, 14);
  spell_level(SPELL_THUNDERCLAP, CLASS_SORCERER, 14);
  spell_level(SPELL_SPELL_MANTLE, CLASS_SORCERER, 14);
  spell_level(SPELL_TELEPORT, CLASS_SORCERER, 14);
  spell_level(SPELL_MASS_WISDOM, CLASS_SORCERER, 14); //shared
  spell_level(SPELL_MASS_CHARISMA, CLASS_SORCERER, 14); //shared
  spell_level(SPELL_MASS_CUNNING, CLASS_SORCERER, 14); //shared

  //8th circle
  spell_level(SPELL_CLENCHED_FIST, CLASS_SORCERER, 16);
  spell_level(SPELL_CHAIN_LIGHTNING, CLASS_SORCERER, 16);
  spell_level(SPELL_INCENDIARY_CLOUD, CLASS_SORCERER, 16);
  spell_level(SPELL_SUMMON_CREATURE_8, CLASS_SORCERER, 16);
  spell_level(SPELL_HORRID_WILTING, CLASS_SORCERER, 16);
  spell_level(SPELL_GREATER_ANIMATION, CLASS_SORCERER, 16);
  spell_level(SPELL_IRRESISTIBLE_DANCE, CLASS_SORCERER, 16);
  spell_level(SPELL_MASS_DOMINATION, CLASS_SORCERER, 16);
  spell_level(SPELL_SCINT_PATTERN, CLASS_SORCERER, 16);
  spell_level(SPELL_REFUGE, CLASS_SORCERER, 16);
  spell_level(SPELL_BANISH, CLASS_SORCERER, 16);
  spell_level(SPELL_SUNBURST, CLASS_SORCERER, 16);
  spell_level(SPELL_SPELL_TURNING, CLASS_SORCERER, 16);
  spell_level(SPELL_MIND_BLANK, CLASS_SORCERER, 16);
  spell_level(SPELL_IRONSKIN, CLASS_SORCERER, 16);
  spell_level(SPELL_PORTAL, CLASS_SORCERER, 16);

  //9th circle
  spell_level(SPELL_METEOR_SWARM, CLASS_SORCERER, 18);
  spell_level(SPELL_BLADE_OF_DISASTER, CLASS_SORCERER, 18);
  spell_level(SPELL_SUMMON_CREATURE_9, CLASS_SORCERER, 18);
  spell_level(SPELL_GATE, CLASS_SORCERER, 18);
  spell_level(SPELL_ENERGY_DRAIN, CLASS_SORCERER, 18); //shared
  spell_level(SPELL_WAIL_OF_THE_BANSHEE, CLASS_SORCERER, 18);
  spell_level(SPELL_POWER_WORD_KILL, CLASS_SORCERER, 18);
  spell_level(SPELL_ENFEEBLEMENT, CLASS_SORCERER, 18);
  spell_level(SPELL_WEIRD, CLASS_SORCERER, 18);
  spell_level(SPELL_SHADOW_SHIELD, CLASS_SORCERER, 18);
  spell_level(SPELL_PRISMATIC_SPHERE, CLASS_SORCERER, 18);
  spell_level(SPELL_IMPLODE, CLASS_SORCERER, 18);
  spell_level(SPELL_TIMESTOP, CLASS_SORCERER, 18);
  spell_level(SPELL_GREATER_SPELL_MANTLE, CLASS_SORCERER, 18);
  spell_level(SPELL_POLYMORPH, CLASS_SORCERER, 18);
  spell_level(SPELL_MASS_ENHANCE, CLASS_SORCERER, 18);

  //epic sorcerer
  spell_level(SPELL_MUMMY_DUST, CLASS_SORCERER, 20); //shared
  spell_level(SPELL_DRAGON_KNIGHT, CLASS_SORCERER, 20); //shared
  spell_level(SPELL_GREATER_RUIN, CLASS_SORCERER, 20); //shared
  spell_level(SPELL_HELLBALL, CLASS_SORCERER, 20); //shared
  spell_level(SPELL_EPIC_MAGE_ARMOR, CLASS_SORCERER, 20);
  spell_level(SPELL_EPIC_WARDING, CLASS_SORCERER, 20);
  //end sorcerer spells


  // bard, increment spells by spell-level
  //1st circle
  spell_level(SPELL_HORIZIKAULS_BOOM, CLASS_BARD, 3);
  spell_level(SPELL_SHIELD, CLASS_BARD, 3);
  spell_level(SPELL_SUMMON_CREATURE_1, CLASS_BARD, 3);
  spell_level(SPELL_CHARM, CLASS_BARD, 3);
  spell_level(SPELL_ENDURE_ELEMENTS, CLASS_BARD, 3);
  spell_level(SPELL_PROT_FROM_EVIL, CLASS_BARD, 3);
  spell_level(SPELL_PROT_FROM_GOOD, CLASS_BARD, 3);
  spell_level(SPELL_MAGIC_MISSILE, CLASS_BARD, 3);

  spell_level(SPELL_CURE_LIGHT, CLASS_BARD, 3);


  //2nd circle
  spell_level(SPELL_SUMMON_CREATURE_2, CLASS_BARD, 5);
  spell_level(SPELL_DEAFNESS, CLASS_BARD, 5);
  spell_level(SPELL_HIDEOUS_LAUGHTER, CLASS_BARD, 5);
  spell_level(SPELL_MIRROR_IMAGE, CLASS_BARD, 5);
  spell_level(SPELL_DETECT_INVIS, CLASS_BARD, 5);
  spell_level(SPELL_DETECT_MAGIC, CLASS_BARD, 5);
  spell_level(SPELL_INVISIBLE, CLASS_BARD, 5);
  spell_level(SPELL_ENDURANCE, CLASS_BARD, 5); //shared
  spell_level(SPELL_STRENGTH, CLASS_BARD, 5);
  spell_level(SPELL_GRACE, CLASS_BARD, 5);

  spell_level(SPELL_CURE_MODERATE, CLASS_BARD, 5);


  //3rd circle
  spell_level(SPELL_SUMMON_CREATURE_3, CLASS_BARD, 8);
  spell_level(SPELL_LIGHTNING_BOLT, CLASS_BARD, 8);
  spell_level(SPELL_DEEP_SLUMBER, CLASS_BARD, 8);
  spell_level(SPELL_HASTE, CLASS_BARD, 8);
  spell_level(SPELL_CIRCLE_A_EVIL, CLASS_BARD, 8);
  spell_level(SPELL_CIRCLE_A_GOOD, CLASS_BARD, 8);
  spell_level(SPELL_CUNNING, CLASS_BARD, 8);
  spell_level(SPELL_WISDOM, CLASS_BARD, 8);
  spell_level(SPELL_CHARISMA, CLASS_BARD, 8);

  spell_level(SPELL_CURE_SERIOUS, CLASS_BARD, 8);


  //4th circle
  spell_level(SPELL_SUMMON_CREATURE_4, CLASS_BARD, 11);
  spell_level(SPELL_GREATER_INVIS, CLASS_BARD, 11);
  spell_level(SPELL_RAINBOW_PATTERN, CLASS_BARD, 11);
  spell_level(SPELL_REMOVE_CURSE, CLASS_BARD, 11);
  spell_level(SPELL_ICE_STORM, CLASS_BARD, 11);

  spell_level(SPELL_CURE_CRITIC, CLASS_BARD, 11);


  //5th circle
  spell_level(SPELL_SUMMON_CREATURE_5, CLASS_BARD, 14);
  spell_level(SPELL_ACID_SHEATH, CLASS_BARD, 14);
  spell_level(SPELL_CONE_OF_COLD, CLASS_BARD, 14);
  spell_level(SPELL_NIGHTMARE, CLASS_BARD, 14);
  spell_level(SPELL_MIND_FOG, CLASS_BARD, 14);

  spell_level(SPELL_MASS_CURE_LIGHT, CLASS_BARD, 14);


  //6th circle
  spell_level(SPELL_SUMMON_CREATURE_7, CLASS_BARD, 17);
  spell_level(SPELL_FREEZING_SPHERE, CLASS_BARD, 17);
  spell_level(SPELL_GREATER_HEROISM, CLASS_BARD, 17);
  spell_level(SPELL_STONESKIN, CLASS_BARD, 11);

  spell_level(SPELL_MASS_CURE_MODERATE, CLASS_BARD, 17);


  //epic bard
  spell_level(SPELL_MUMMY_DUST, CLASS_BARD, 20); //shared
  spell_level(SPELL_GREATER_RUIN, CLASS_BARD, 20); //shared
  //end bard spells


  // paladins
  //1st circle
  spell_level(SPELL_CURE_LIGHT, CLASS_PALADIN, 6);
  spell_level(SPELL_ENDURANCE, CLASS_PALADIN, 6); //shared
  spell_level(SPELL_ARMOR, CLASS_PALADIN, 6);
  spell_level(SPELL_CAUSE_LIGHT_WOUNDS, CLASS_PALADIN, 6);
  //2nd circle
  spell_level(SPELL_CREATE_FOOD, CLASS_PALADIN, 10);
  spell_level(SPELL_CREATE_WATER, CLASS_PALADIN, 10);
  spell_level(SPELL_DETECT_POISON, CLASS_PALADIN, 10); //shared
  spell_level(SPELL_CAUSE_MODERATE_WOUNDS, CLASS_PALADIN, 10);
  //3rd circle
  spell_level(SPELL_DETECT_ALIGN, CLASS_PALADIN, 12);
  spell_level(SPELL_CURE_BLIND, CLASS_PALADIN, 12);
  spell_level(SPELL_BLESS, CLASS_PALADIN, 12);
  spell_level(SPELL_CAUSE_SERIOUS_WOUNDS, CLASS_PALADIN, 12);
  //4th circle
  spell_level(SPELL_INFRAVISION, CLASS_PALADIN, 15); //shared
  spell_level(SPELL_REMOVE_CURSE, CLASS_PALADIN, 15); //shared
  spell_level(SPELL_CAUSE_CRITICAL_WOUNDS, CLASS_PALADIN, 15);
  spell_level(SPELL_CURE_CRITIC, CLASS_PALADIN, 15);
  spell_level(SPELL_HOLY_SWORD, CLASS_PALADIN, 15); // unique to paladin


  // rangers
  //1st circle
  spell_level(SPELL_CHARM_ANIMAL, CLASS_RANGER, 6);
  spell_level(SPELL_CURE_LIGHT, CLASS_RANGER, 6);
  spell_level(SPELL_FAERIE_FIRE, CLASS_RANGER, 6);
  spell_level(SPELL_JUMP, CLASS_RANGER, 6);
  spell_level(SPELL_MAGIC_FANG, CLASS_RANGER, 6);
  spell_level(SPELL_SUMMON_NATURES_ALLY_1, CLASS_RANGER, 6);

  //2nd circle
  spell_level(SPELL_BARKSKIN, CLASS_RANGER, 10);
  spell_level(SPELL_ENDURANCE, CLASS_RANGER, 10);
  spell_level(SPELL_GRACE, CLASS_RANGER, 10);
  spell_level(SPELL_HOLD_ANIMAL, CLASS_RANGER, 10);
  spell_level(SPELL_WISDOM, CLASS_RANGER, 10);
  spell_level(SPELL_STRENGTH, CLASS_RANGER, 10);
  spell_level(SPELL_SUMMON_NATURES_ALLY_2, CLASS_RANGER, 10);

  //3rd circle
  spell_level(SPELL_CURE_MODERATE, CLASS_RANGER, 12);
  spell_level(SPELL_CONTAGION, CLASS_RANGER, 12);
  spell_level(SPELL_GREATER_MAGIC_FANG, CLASS_RANGER, 12);
  spell_level(SPELL_SPIKE_GROWTH, CLASS_RANGER, 12);
  spell_level(SPELL_SUMMON_NATURES_ALLY_3, CLASS_RANGER, 12);

  //4th circle
  spell_level(SPELL_CURE_SERIOUS, CLASS_RANGER, 15);
  spell_level(SPELL_DISPEL_MAGIC, CLASS_RANGER, 15);
  spell_level(SPELL_FREE_MOVEMENT, CLASS_RANGER, 15);
  spell_level(SPELL_SUMMON_NATURES_ALLY_4, CLASS_RANGER, 15);


  // clerics
  //1st circle
  spell_level(SPELL_ARMOR, CLASS_CLERIC, 1);
  spell_level(SPELL_CURE_LIGHT, CLASS_CLERIC, 1);
  spell_level(SPELL_ENDURANCE, CLASS_CLERIC, 1); //shared
  spell_level(SPELL_CAUSE_LIGHT_WOUNDS, CLASS_CLERIC, 1);
  spell_level(SPELL_NEGATIVE_ENERGY_RAY, CLASS_CLERIC, 1);
  spell_level(SPELL_ENDURE_ELEMENTS, CLASS_CLERIC, 1);
  spell_level(SPELL_PROT_FROM_EVIL, CLASS_CLERIC, 1);
  spell_level(SPELL_PROT_FROM_GOOD, CLASS_CLERIC, 1);
  spell_level(SPELL_SUMMON_CREATURE_1, CLASS_CLERIC, 1);
  spell_level(SPELL_STRENGTH, CLASS_CLERIC, 1);
  spell_level(SPELL_GRACE, CLASS_CLERIC, 1);
  spell_level(SPELL_REMOVE_FEAR, CLASS_CLERIC, 1);
  //2nd circle
  spell_level(SPELL_CREATE_FOOD, CLASS_CLERIC, 3);
  spell_level(SPELL_CREATE_WATER, CLASS_CLERIC, 3);
  spell_level(SPELL_DETECT_POISON, CLASS_CLERIC, 3); //shared
  spell_level(SPELL_CAUSE_MODERATE_WOUNDS, CLASS_CLERIC, 3);
  spell_level(SPELL_CURE_MODERATE, CLASS_CLERIC, 3);
  spell_level(SPELL_SCARE, CLASS_CLERIC, 3);
  spell_level(SPELL_SUMMON_CREATURE_2, CLASS_CLERIC, 3);
  spell_level(SPELL_DETECT_MAGIC, CLASS_CLERIC, 3);
  spell_level(SPELL_DARKNESS, CLASS_CLERIC, 3);
  spell_level(SPELL_RESIST_ENERGY, CLASS_CLERIC, 3);
  spell_level(SPELL_WISDOM, CLASS_CLERIC, 3);
  spell_level(SPELL_CHARISMA, CLASS_CLERIC, 3);
  //3rd circle
  spell_level(SPELL_BLESS, CLASS_CLERIC, 5);
  spell_level(SPELL_CURE_BLIND, CLASS_CLERIC, 5);
  spell_level(SPELL_DETECT_ALIGN, CLASS_CLERIC, 5);
  spell_level(SPELL_CAUSE_SERIOUS_WOUNDS, CLASS_CLERIC, 5);
  spell_level(SPELL_CURE_SERIOUS, CLASS_CLERIC, 5);
  spell_level(SPELL_SUMMON_CREATURE_3, CLASS_CLERIC, 5);
  spell_level(SPELL_BLINDNESS, CLASS_CLERIC, 5);
  spell_level(SPELL_DEAFNESS, CLASS_CLERIC, 5);
  spell_level(SPELL_CURE_DEAFNESS, CLASS_CLERIC, 5);
  spell_level(SPELL_CUNNING, CLASS_CLERIC, 5);
  spell_level(SPELL_DISPEL_MAGIC, CLASS_CLERIC, 5);
  spell_level(SPELL_ANIMATE_DEAD, CLASS_CLERIC, 5);
  spell_level(SPELL_FAERIE_FOG, CLASS_CLERIC, 5);
  //4th circle
  spell_level(SPELL_CURE_CRITIC, CLASS_CLERIC, 7);
  spell_level(SPELL_REMOVE_CURSE, CLASS_CLERIC, 7); //shared
  spell_level(SPELL_INFRAVISION, CLASS_CLERIC, 7); //shared
  spell_level(SPELL_CAUSE_CRITICAL_WOUNDS, CLASS_CLERIC, 7);
  spell_level(SPELL_SUMMON_CREATURE_4, CLASS_CLERIC, 7);
  spell_level(SPELL_CIRCLE_A_EVIL, CLASS_CLERIC, 7);
  spell_level(SPELL_CIRCLE_A_GOOD, CLASS_CLERIC, 7);
  spell_level(SPELL_CURSE, CLASS_CLERIC, 7);
  spell_level(SPELL_DAYLIGHT, CLASS_CLERIC, 7);
  spell_level(SPELL_MASS_CURE_LIGHT, CLASS_CLERIC, 7);
  spell_level(SPELL_AID, CLASS_CLERIC, 7);
  spell_level(SPELL_BRAVERY, CLASS_CLERIC, 7);
  //5th circle
  spell_level(SPELL_POISON, CLASS_CLERIC, 9); //shared
  spell_level(SPELL_REMOVE_POISON, CLASS_CLERIC, 9);
  spell_level(SPELL_PROT_FROM_EVIL, CLASS_CLERIC, 9);
  spell_level(SPELL_GROUP_ARMOR, CLASS_CLERIC, 9);
  spell_level(SPELL_FLAME_STRIKE, CLASS_CLERIC, 9);
  spell_level(SPELL_PROT_FROM_GOOD, CLASS_CLERIC, 9);
  spell_level(SPELL_MASS_CURE_MODERATE, CLASS_CLERIC, 9);
  spell_level(SPELL_SUMMON_CREATURE_5, CLASS_CLERIC, 9);
  spell_level(SPELL_WATER_BREATHE, CLASS_CLERIC, 9);
  spell_level(SPELL_WATERWALK, CLASS_CLERIC, 9);
  spell_level(SPELL_REGENERATION, CLASS_CLERIC, 9);
  spell_level(SPELL_FREE_MOVEMENT, CLASS_CLERIC, 9);
  spell_level(SPELL_STRENGTHEN_BONE, CLASS_CLERIC, 9);
  //6th circle
  spell_level(SPELL_DISPEL_EVIL, CLASS_CLERIC, 11);
  spell_level(SPELL_HARM, CLASS_CLERIC, 11);
  spell_level(SPELL_HEAL, CLASS_CLERIC, 11);
  spell_level(SPELL_DISPEL_GOOD, CLASS_CLERIC, 11);
  spell_level(SPELL_SUMMON_CREATURE_6, CLASS_CLERIC, 11);
  spell_level(SPELL_MASS_CURE_SERIOUS, CLASS_CLERIC, 11);
  spell_level(SPELL_EYEBITE, CLASS_CLERIC, 11);
  spell_level(SPELL_PRAYER, CLASS_CLERIC, 11);
  spell_level(SPELL_MASS_WISDOM, CLASS_CLERIC, 11);
  spell_level(SPELL_MASS_CHARISMA, CLASS_CLERIC, 11);
  spell_level(SPELL_MASS_CUNNING, CLASS_CLERIC, 11);
  spell_level(SPELL_REMOVE_DISEASE, CLASS_CLERIC, 11);
  //7th circle
  spell_level(SPELL_CALL_LIGHTNING, CLASS_CLERIC, 13);
  //spell_level(SPELL_CONTROL_WEATHER, CLASS_CLERIC, 13);
  spell_level(SPELL_SUMMON, CLASS_CLERIC, 13);
  spell_level(SPELL_WORD_OF_RECALL, CLASS_CLERIC, 13);
  spell_level(SPELL_SUMMON_CREATURE_7, CLASS_CLERIC, 13);
  spell_level(SPELL_MASS_CURE_CRIT, CLASS_CLERIC, 13);
  spell_level(SPELL_GREATER_DISPELLING, CLASS_CLERIC, 13);
  spell_level(SPELL_MASS_ENHANCE, CLASS_CLERIC, 13);
  spell_level(SPELL_BLADE_BARRIER, CLASS_CLERIC, 13);
  spell_level(SPELL_BATTLETIDE, CLASS_CLERIC, 13);
  spell_level(SPELL_SPELL_RESISTANCE, CLASS_CLERIC, 13);
  spell_level(SPELL_SENSE_LIFE, CLASS_CLERIC, 13);
  //8th circle
  //spell_level(SPELL_SANCTUARY, CLASS_CLERIC, 15);
  spell_level(SPELL_DESTRUCTION, CLASS_CLERIC, 15);
  spell_level(SPELL_SUMMON_CREATURE_8, CLASS_CLERIC, 15);
  spell_level(SPELL_SPELL_MANTLE, CLASS_CLERIC, 15);
  spell_level(SPELL_TRUE_SEEING, CLASS_CLERIC, 15);
  spell_level(SPELL_WORD_OF_FAITH, CLASS_CLERIC, 15);
  spell_level(SPELL_GREATER_ANIMATION, CLASS_CLERIC, 15);
  spell_level(SPELL_EARTHQUAKE, CLASS_CLERIC, 15);
  spell_level(SPELL_ANTI_MAGIC_FIELD, CLASS_CLERIC, 15);
  spell_level(SPELL_DIMENSIONAL_LOCK, CLASS_CLERIC, 15);
  spell_level(SPELL_SALVATION, CLASS_CLERIC, 15);
  spell_level(SPELL_SPRING_OF_LIFE, CLASS_CLERIC, 15);
  //9th circle
  spell_level(SPELL_SUNBURST, CLASS_CLERIC, 17);
  spell_level(SPELL_ENERGY_DRAIN, CLASS_CLERIC, 17); //shared
  spell_level(SPELL_GROUP_HEAL, CLASS_CLERIC, 17);
  spell_level(SPELL_SUMMON_CREATURE_9, CLASS_CLERIC, 17);
  spell_level(SPELL_PLANE_SHIFT, CLASS_CLERIC, 17);
  spell_level(SPELL_STORM_OF_VENGEANCE, CLASS_CLERIC, 17);
  spell_level(SPELL_IMPLODE, CLASS_CLERIC, 17);
  // death shield
  // command
  // air walker
  spell_level(SPELL_REFUGE, CLASS_CLERIC, 17);
  spell_level(SPELL_GROUP_SUMMON, CLASS_CLERIC, 17);
  //epic spells
  spell_level(SPELL_MUMMY_DUST, CLASS_CLERIC, 20);
  spell_level(SPELL_DRAGON_KNIGHT, CLASS_CLERIC, 20);
  spell_level(SPELL_GREATER_RUIN, CLASS_CLERIC, 20);
  spell_level(SPELL_HELLBALL, CLASS_CLERIC, 20);
  //end cleric spells


  // druid
  //1st circle
  spell_level(SPELL_CHARM_ANIMAL, CLASS_DRUID, 1);
  spell_level(SPELL_CURE_LIGHT, CLASS_DRUID, 1);
  spell_level(SPELL_FAERIE_FIRE, CLASS_DRUID, 1);
  spell_level(SPELL_GOODBERRY, CLASS_DRUID, 1);
  spell_level(SPELL_JUMP, CLASS_DRUID, 1);
  spell_level(SPELL_MAGIC_FANG, CLASS_DRUID, 1);
  spell_level(SPELL_MAGIC_STONE, CLASS_DRUID, 1);
  spell_level(SPELL_OBSCURING_MIST, CLASS_DRUID, 1);
  spell_level(SPELL_PRODUCE_FLAME, CLASS_DRUID, 1);
  spell_level(SPELL_SUMMON_NATURES_ALLY_1, CLASS_DRUID, 1);

  //2nd circle
  spell_level(SPELL_BARKSKIN, CLASS_DRUID, 3);
  spell_level(SPELL_ENDURANCE, CLASS_DRUID, 3);
  spell_level(SPELL_FLAME_BLADE, CLASS_DRUID, 3);
  spell_level(SPELL_FLAMING_SPHERE, CLASS_DRUID, 3);
  spell_level(SPELL_GRACE, CLASS_DRUID, 3);
  spell_level(SPELL_HOLD_ANIMAL, CLASS_DRUID, 3);
  spell_level(SPELL_WISDOM, CLASS_DRUID, 3);
  spell_level(SPELL_STRENGTH, CLASS_DRUID, 3);
  spell_level(SPELL_SUMMON_NATURES_ALLY_2, CLASS_DRUID, 3);
  spell_level(SPELL_SUMMON_SWARM, CLASS_DRUID, 3);

  //3rd circle
  spell_level(SPELL_CALL_LIGHTNING, CLASS_DRUID, 5);
  spell_level(SPELL_CURE_MODERATE, CLASS_DRUID, 5);
  spell_level(SPELL_CONTAGION, CLASS_DRUID, 5);
  spell_level(SPELL_GREATER_MAGIC_FANG, CLASS_DRUID, 5);
  spell_level(SPELL_POISON, CLASS_DRUID, 5);
  spell_level(SPELL_REMOVE_DISEASE, CLASS_DRUID, 5);
  spell_level(SPELL_REMOVE_POISON, CLASS_DRUID, 5);
  spell_level(SPELL_SPIKE_GROWTH, CLASS_DRUID, 5);
  spell_level(SPELL_SUMMON_NATURES_ALLY_3, CLASS_DRUID, 5);

  //4th circle
  spell_level(SPELL_BLIGHT, CLASS_DRUID, 7);
  spell_level(SPELL_CURE_SERIOUS, CLASS_DRUID, 7);
  spell_level(SPELL_DISPEL_MAGIC, CLASS_DRUID, 7);
  spell_level(SPELL_FLAME_STRIKE, CLASS_DRUID, 7);
  spell_level(SPELL_FREE_MOVEMENT, CLASS_DRUID, 7);
  spell_level(SPELL_ICE_STORM, CLASS_DRUID, 7);
  spell_level(SPELL_LOCATE_CREATURE, CLASS_DRUID, 7);
  // reincarnate
  spell_level(SPELL_SPIKE_STONES, CLASS_DRUID, 7);
  spell_level(SPELL_SUMMON_NATURES_ALLY_4, CLASS_DRUID, 7);

  //5th circle
  // baleful polymorph (holding off on this one for now)
  spell_level(SPELL_CALL_LIGHTNING_STORM, CLASS_DRUID, 9);
  spell_level(SPELL_CURE_CRITIC, CLASS_DRUID, 9);
  spell_level(SPELL_DEATH_WARD, CLASS_DRUID, 9);
  spell_level(SPELL_HALLOW, CLASS_DRUID, 9);
  spell_level(SPELL_INSECT_PLAGUE, CLASS_DRUID, 9);
  spell_level(SPELL_STONESKIN, CLASS_DRUID, 9);
  spell_level(SPELL_SUMMON_NATURES_ALLY_5, CLASS_DRUID, 9);
  spell_level(SPELL_UNHALLOW, CLASS_DRUID, 9);
  spell_level(SPELL_WALL_OF_FIRE, CLASS_DRUID, 9);
  spell_level(SPELL_WALL_OF_THORNS, CLASS_DRUID, 9);

  //6th circle
  spell_level(SPELL_FIRE_SEEDS, CLASS_DRUID, 11);
  spell_level(SPELL_GREATER_DISPELLING, CLASS_DRUID, 11);
  spell_level(SPELL_MASS_CURE_LIGHT, CLASS_DRUID, 11);
  spell_level(SPELL_MASS_ENDURANCE, CLASS_DRUID, 11);
  spell_level(SPELL_MASS_STRENGTH, CLASS_DRUID, 11);
  spell_level(SPELL_MASS_GRACE, CLASS_DRUID, 11);
  spell_level(SPELL_MASS_WISDOM, CLASS_DRUID, 11);
  spell_level(SPELL_SPELLSTAFF, CLASS_DRUID, 11);
  spell_level(SPELL_SUMMON_NATURES_ALLY_6, CLASS_DRUID, 11);
  spell_level(SPELL_TRANSPORT_VIA_PLANTS, CLASS_DRUID, 11);

  //7th circle
  spell_level(SPELL_CONTROL_WEATHER, CLASS_DRUID, 13);
  spell_level(SPELL_CREEPING_DOOM, CLASS_DRUID, 13);
  spell_level(SPELL_FIRE_STORM, CLASS_DRUID, 13);
  // greater scrying (not going to implement this yet)
  spell_level(SPELL_HEAL, CLASS_DRUID, 13);
  spell_level(SPELL_MASS_CURE_MODERATE, CLASS_DRUID, 13);
  spell_level(SPELL_SUMMON_NATURES_ALLY_7, CLASS_DRUID, 13);
  spell_level(SPELL_SUNBEAM, CLASS_DRUID, 13);

  //8th circle
  spell_level(SPELL_ANIMAL_SHAPES, CLASS_DRUID, 15);
  spell_level(SPELL_CONTROL_PLANTS, CLASS_DRUID, 15);
  spell_level(SPELL_EARTHQUAKE, CLASS_DRUID, 15);
  spell_level(SPELL_FINGER_OF_DEATH, CLASS_DRUID, 15);
  spell_level(SPELL_MASS_CURE_SERIOUS, CLASS_DRUID, 15);
  spell_level(SPELL_SUMMON_NATURES_ALLY_8, CLASS_DRUID, 15);
  spell_level(SPELL_SUNBURST, CLASS_DRUID, 15);
  spell_level(SPELL_WHIRLWIND, CLASS_DRUID, 15);
  spell_level(SPELL_WORD_OF_RECALL, CLASS_DRUID, 15);

  //9th circle
  spell_level(SPELL_ELEMENTAL_SWARM, CLASS_DRUID, 17);
  spell_level(SPELL_REGENERATION, CLASS_DRUID, 17);
  spell_level(SPELL_MASS_CURE_CRIT, CLASS_DRUID, 17);
  spell_level(SPELL_SHAMBLER, CLASS_DRUID, 17);
  spell_level(SPELL_POLYMORPH, CLASS_DRUID, 17); // should be shapechange
  spell_level(SPELL_STORM_OF_VENGEANCE, CLASS_DRUID, 17);
  spell_level(SPELL_SUMMON_NATURES_ALLY_9, CLASS_DRUID, 17);

  //epic spells
  spell_level(SPELL_MUMMY_DUST, CLASS_DRUID, 20);
  spell_level(SPELL_DRAGON_KNIGHT, CLASS_DRUID, 20);
  spell_level(SPELL_GREATER_RUIN, CLASS_DRUID, 20);
  spell_level(SPELL_HELLBALL, CLASS_DRUID, 20);
  //end druid spells

}


// level_exp ran with level+1 will give xp to next level
// level_exp+1 - level_exp = exp to next level
#define EXP_MAX  2100000000

int level_exp(struct char_data *ch, int level) {
  int chclass = GET_CLASS(ch);
  int exp = 0, factor = 0;

  if (level > (LVL_IMPL + 1) || level < 0) {
    log("SYSERR: Requesting exp for invalid level %d!", level);
    return 0;
  }

  /* Gods have exp close to EXP_MAX.  This statement should never have to
   * changed, regardless of how many mortal or immortal levels exist. */
  if (level > LVL_IMMORT) {
    return EXP_MAX - ((LVL_IMPL - level) * 1000);
  }

  factor = 2000 + (level - 2) * 750;

  /* Exp required for normal mortals is below */
  switch (chclass) {
    case CLASS_WIZARD:
    case CLASS_SORCERER:
    case CLASS_PALADIN:
    case CLASS_MONK:
    case CLASS_DRUID:
    case CLASS_RANGER:
    case CLASS_WARRIOR:
    case CLASS_WEAPON_MASTER:
    case CLASS_ROGUE:
    case CLASS_BARD:
    case CLASS_BERSERKER:
    case CLASS_CLERIC:
      level--;
      if (level < 0)
        level = 0;
      exp += (level * level * factor);
      break;

    default:
      log("SYSERR: Reached invalid class in class.c level()!");
      return 123456;
  }

  //can add other exp penalty/bonuses here
  switch (GET_RACE(ch)) {
      //advanced races
    case RACE_HALF_TROLL:
      exp *= 2;
      break;
    case RACE_ARCANA_GOLEM:
      exp *= 2;
      break;
      //epic races
    case RACE_CRYSTAL_DWARF:
      exp *= 15;
      break;
    case RACE_TRELUX:
      exp *= 15;
      break;
    default:
      break;
  }

  return exp;
}

/* Default titles system, simplified from stock -zusuk */
const char *titles(int chclass, int level) {
  if (level <= 0 || level > LVL_IMPL)
    return "the Being";
  if (level == LVL_IMPL)
    return "the Implementor";

  switch (chclass) {

    case CLASS_WIZARD:
      switch (level) {
        case 1:
        case 2:
        case 3:
        case 4: return "";
        case 5:
        case 6:
        case 7:
        case 8:
        case 9: return "the Reader of Arcane Texts";
        case 10:
        case 11:
        case 12:
        case 13:
        case 14: return "the Ever-Learning";
        case 15:
        case 16:
        case 17:
        case 18:
        case 19: return "the Advanced Student";
        case 20:
        case 21:
        case 22:
        case 23:
        case 24: return "the Channel of Power";
        case 25:
        case 26:
        case 27:
        case 28:
        case 29: return "the Delver of Mysteries";
        case 30: return "the Knower of Hidden Things";
        case LVL_IMMORT: return "the Immortal Warlock";
        case LVL_STAFF: return "the Avatar of Magic";
        case LVL_GRSTAFF: return "the God of Magic";
        default: return "the Wizard";
      }
      break;


    case CLASS_RANGER:
      switch (level) {
        case 1:
        case 2:
        case 3:
        case 4: return "";
        case 5:
        case 6:
        case 7:
        case 8:
        case 9: return "the Dirt-watcher";
        case 10:
        case 11:
        case 12:
        case 13:
        case 14: return "the Hunter";
        case 15:
        case 16:
        case 17:
        case 18:
        case 19: return "the Tracker";
        case 20:
        case 21:
        case 22:
        case 23:
        case 24: return "the Finder of Prey";
        case 25:
        case 26:
        case 27:
        case 28:
        case 29: return "the Hidden Stalker";
        case 30: return "the Great Seeker";
        case LVL_IMMORT: return "the Avatar of the Wild";
        case LVL_STAFF: return "the Wrath of the Wild";
        case LVL_GRSTAFF: return "the Cyclone of Nature";
        default: return "the Ranger";
      }
      break;


    case CLASS_DRUID:
      switch (level) {
        case 1:
        case 2:
        case 3:
        case 4: return "";
        case 5:
        case 6:
        case 7:
        case 8:
        case 9: return "the Walker on Loam";
        case 10:
        case 11:
        case 12:
        case 13:
        case 14: return "the Speaker for Beasts";
        case 15:
        case 16:
        case 17:
        case 18:
        case 19: return "the Watcher from Shade";
        case 20:
        case 21:
        case 22:
        case 23:
        case 24: return "the Whispering Winds";
        case 25:
        case 26:
        case 27:
        case 28:
        case 29: return "the Balancer";
        case 30: return "the Still Waters";
        case LVL_IMMORT: return "the Avatar of Nature";
        case LVL_STAFF: return "the Wrath of Nature";
        case LVL_GRSTAFF: return "the Storm of Earth's Voice";
        default: return "the Druid";
      }
      break;


    case CLASS_SORCERER:
      switch (level) {
        case 1:
        case 2:
        case 3:
        case 4: return "";
        case 5:
        case 6:
        case 7:
        case 8:
        case 9: return "the Awakened";
        case 10:
        case 11:
        case 12:
        case 13:
        case 14: return "the Torch";
        case 15:
        case 16:
        case 17:
        case 18:
        case 19: return "the Firebrand";
        case 20:
        case 21:
        case 22:
        case 23:
        case 24: return "the Destroyer";
        case 25:
        case 26:
        case 27:
        case 28:
        case 29: return "the Crux of Power";
        case 30: return "the Near-Divine";
        case LVL_IMMORT: return "the Immortal Magic Weaver";
        case LVL_STAFF: return "the Avatar of the Flow";
        case LVL_GRSTAFF: return "the Hand of Mystical Might";
        default: return "the Sorcerer";
      }
      break;


    case CLASS_BARD:
      switch (level) {
        case 1:
        case 2:
        case 3:
        case 4: return "";
        case 5:
        case 6:
        case 7:
        case 8:
        case 9: return "the Melodious";
        case 10:
        case 11:
        case 12:
        case 13:
        case 14: return "the Hummer of Harmonies";
        case 15:
        case 16:
        case 17:
        case 18:
        case 19: return "Weaver of Song";
        case 20:
        case 21:
        case 22:
        case 23:
        case 24: return "Keeper of Chords";
        case 25:
        case 26:
        case 27:
        case 28:
        case 29: return "the Composer";
        case 30: return "the Maestro";
        case LVL_IMMORT: return "the Immortal Songweaver";
        case LVL_STAFF: return "the Master of Sound";
        case LVL_GRSTAFF: return "the Lord of Dance";
        default: return "the Bard";
      }
      break;


    case CLASS_CLERIC:
      switch (level) {
        case 1:
        case 2:
        case 3:
        case 4: return "";
        case 5:
        case 6:
        case 7:
        case 8:
        case 9: return "the Devotee";
        case 10:
        case 11:
        case 12:
        case 13:
        case 14: return "the Example";
        case 15:
        case 16:
        case 17:
        case 18:
        case 19: return "the Truly Pious";
        case 20:
        case 21:
        case 22:
        case 23:
        case 24: return "the Mighty in Faith";
        case 25:
        case 26:
        case 27:
        case 28:
        case 29: return "the God-Favored";
        case 30: return "the One Who Moves Mountains";
        case LVL_IMMORT: return "the Immortal Cardinal";
        case LVL_STAFF: return "the Inquisitor";
        case LVL_GRSTAFF: return "the God of Good and Evil";
        default: return "the Cleric";
      }
      break;

    case CLASS_PALADIN:
      switch (level) {
        case 1:
        case 2:
        case 3:
        case 4: return "";
        case 5:
        case 6:
        case 7:
        case 8:
        case 9: return "the Initiated";
        case 10:
        case 11:
        case 12:
        case 13:
        case 14: return "the Accepted";
        case 15:
        case 16:
        case 17:
        case 18:
        case 19: return "the Hand of Mercy";
        case 20:
        case 21:
        case 22:
        case 23:
        case 24: return "the Sword of Justice";
        case 25:
        case 26:
        case 27:
        case 28:
        case 29: return "who Walks in the Light";
        case 30: return "the Defender of the Faith";
        case LVL_IMMORT: return "the Immortal Justicar";
        case LVL_STAFF: return "the Immortal Sword of Light";
        case LVL_GRSTAFF: return "the Immortal Hammer of Justic";
        default: return "the Paladin";
      }
      break;

    case CLASS_MONK:
      switch (level) {
        case 1:
        case 2:
        case 3:
        case 4: return "";
        case 5:
        case 6:
        case 7:
        case 8:
        case 9: return "of the Crushing Fist";
        case 10:
        case 11:
        case 12:
        case 13:
        case 14: return "of the Stomping Foot";
        case 15:
        case 16:
        case 17:
        case 18:
        case 19: return "of the Directed Motions";
        case 20:
        case 21:
        case 22:
        case 23:
        case 24: return "of the Disciplined Body";
        case 25:
        case 26:
        case 27:
        case 28:
        case 29: return "of the Disciplined Mind";
        case 30: return "of the Mastered Self";
        case LVL_IMMORT: return "the Immortal Monk";
        case LVL_STAFF: return "the Inquisitor Monk";
        case LVL_GRSTAFF: return "the God of the Fist";
        default: return "the Monk";
      }
      break;

    case CLASS_ROGUE:
      switch (level) {
        case 1:
        case 2:
        case 3:
        case 4: return "";
        case 5:
        case 6:
        case 7:
        case 8:
        case 9: return "the Rover";
        case 10:
        case 11:
        case 12:
        case 13:
        case 14: return "the Multifarious";
        case 15:
        case 16:
        case 17:
        case 18:
        case 19: return "the Illusive";
        case 20:
        case 21:
        case 22:
        case 23:
        case 24: return "the Swindler";
        case 25:
        case 26:
        case 27:
        case 28:
        case 29: return "the Marauder";
        case 30: return "the Volatile";
        case LVL_IMMORT: return "the Immortal Assassin";
        case LVL_STAFF: return "the Demi God of Thieves";
        case LVL_GRSTAFF: return "the God of Thieves and Tradesmen";
        default: return "the Rogue";
      }
      break;

    case CLASS_WARRIOR:
      switch (level) {
        case 1:
        case 2:
        case 3:
        case 4: return "";
        case 5:
        case 6:
        case 7:
        case 8:
        case 9: return "the Mostly Harmless";
        case 10:
        case 11:
        case 12:
        case 13:
        case 14: return "the Useful in Bar-Fights";
        case 15:
        case 16:
        case 17:
        case 18:
        case 19: return "the Friend to Violence";
        case 20:
        case 21:
        case 22:
        case 23:
        case 24: return "the Strong";
        case 25:
        case 26:
        case 27:
        case 28:
        case 29: return "the Bane of All Enemies";
        case 30: return "the Exceptionally Dangerous";
        case LVL_IMMORT: return "the Immortal Warlord";
        case LVL_STAFF: return "the Extirpator";
        case LVL_GRSTAFF: return "the God of War";
        default: return "the Warrior";
      }
      break;

    case CLASS_WEAPON_MASTER:
      switch (level) {
        case 1:
        case 2:
        case 3:
        case 4: return "";
        case 5:
        case 6:
        case 7:
        case 8:
        case 9: return "the Inexperienced Weapon";
        case 10:
        case 11:
        case 12:
        case 13:
        case 14: return "the Weapon";
        case 15:
        case 16:
        case 17:
        case 18:
        case 19: return "the Skilled Weapon";
        case 20:
        case 21:
        case 22:
        case 23:
        case 24: return "the Master of Weapons";
        case 25:
        case 26:
        case 27:
        case 28:
        case 29: return "the Master of All Weapons";
        case 30: return "the Unmatched Weapon";
        case LVL_IMMORT: return "the Immortal WeaponMaster";
        case LVL_STAFF: return "the Relentless Weapon";
        case LVL_GRSTAFF: return "the God of Weapons";
        default: return "the WeaponMaster";
      }
      break;

    case CLASS_BERSERKER:
      switch (level) {
        case 1:
        case 2:
        case 3:
        case 4: return "";
        case 5:
        case 6:
        case 7:
        case 8:
        case 9: return "the Ripper of Flesh";
        case 10:
        case 11:
        case 12:
        case 13:
        case 14: return "the Shatterer of Bone";
        case 15:
        case 16:
        case 17:
        case 18:
        case 19: return "the Cleaver of Organs";
        case 20:
        case 21:
        case 22:
        case 23:
        case 24: return "the Wrecker of Hope";
        case 25:
        case 26:
        case 27:
        case 28:
        case 29: return "the Effulgence of Rage";
        case 30: return "the Foe-Hewer";
        case LVL_IMMORT: return "the Immortal Warlord";
        case LVL_STAFF: return "the Extirpator";
        case LVL_GRSTAFF: return "the God of Rage";
        default: return "the Berserker";
      }
      break;

  }

  /* Default title for classes which do not have titles defined */
  return "the Classless";
}


