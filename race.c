/**************************************************************************
*  File: race.c                                           Part of tbaMUD *
*  Usage: Source file for class-specific code.                            *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

/** Help buffer the global variable definitions */
#define __RACE_C__

/* This file attempts to concentrate most of the code which must be changed
 * in order for new race to be added.  If you're adding a new class, you
 * should go through this entire file from beginning to end and add the
 * appropriate new special cases for your new race. */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "spells.h"
#include "interpreter.h"
#include "constants.h"
#include "act.h"
#include "comm.h"
#include "race.h"


// npc races
const char *npc_race_types[] = {
   "Unknown",
   "Humanoid",
   "Undead",
   "Animal",
   "Dragon",
   "Giant",
   "Aberration",
   "Construct",
   "Elemental",
   "Fey",
   "Magical Beast",
   "Monstrous Humanoid",
   "Ooze",
   "Outsider",
   "Plant",
   "Vermin"
};


// npc races, short form
const char *npc_race_short[] = {
   "???",
   "Hmn",
   "Und",
   "Anm",
   "Drg",
   "Gnt",
   "Abr",
   "Con",
   "Ele",
   "Fey",
   "Bst",
   "MoH",
   "Oze",
   "Out",
   "Plt",
   "Ver"
};

// colored npc race abbreviations
// for now full name for effect
const char *npc_race_abbrevs[] = {
   "Unknown",
   "\tWHmnd\tn",
   "\tDUndd\tn",
   "\tgAnml\tn",
   "\trDrgn\tn",
   "\tYGnt\tn",
   "\tRAbrt\tn",
   "\tcCnst\tn",
   "\tRElem\tn",
   "\tCFey\tn",
   "\tmM\tgBst\tn",
   "\tBM\tWHmn\tn",
   "\tMOoze\tn",
   "\tDOut\tws\tn",
   "\tGPlnt\tn",
   "\tyVrmn\tn"
};
/*
const char *npc_race_abbrevs[] = {
   "Unknown",
   "\tWHumanoid\tn",
   "\tDUndead\tn",
   "\tgAnimal\tn",
   "\trDragon\tn",
   "\tYGiant\tn",
   "\tRAberration\tn",
   "\tcConstruct\tn",
   "\tRElemental\tn",
   "\tCFey\tn",
   "\tmMagical \tgBeast\tn",
   "\tBMonstrous \tWHumanoid\tn",
   "\tMOoze\tn",
   "\tDOut\twsider\tn",
   "\tGPlant\tn",
   "\tyVermin\tn"
};
*/

// npc subrace
const char *npc_subrace_types[] = {
   "Unknown",/**/
   "Air",/**/
   "Angelic",/**/
   "Aquatic",/**/
   "Archon",/**/
   "Augmented",/**/
   "Chaotic",/**/
   "Cold",/**/
   "Earth",/**/
   "Evil",/**/
   "Extraplanar",/**/
   "Fire",/**/
   "Goblinoid",/**/
   "Good",/**/
   "Incorporeal",/**/
   "Lawful",/**/
   "Native",/**/
   "Reptilian",/**/
   "Shapechanger",/**/
   "Swarm",/**/
   "Water"/**/
};


// colored npc subrace abbreviations
// for now full name for effect
const char *npc_subrace_abbrevs[] = {
   "Unknown",
   "\tCAir\tn",
   "\tWAngelic\tn",
   "\tBAquatic\tn",
   "\trArch\tRon\tn",
   "\tYAugmented\tn",
   "\tDChaotic\tn",
   "\tbCold\tn",
   "\tGEarth\tn",
   "\trEvil\tn",
   "\tmExtraplanar\tn",
   "\tRFire\tn",
   "\tgGoblinoid\tn",
   "\tWGood\tn",
   "\tGIncorporeal\tn",
   "\twLawful\tn",
   "\tyNative\tn",
   "\tyReptilian\tn",
   "\tMShapechanger\tn",
   "\tySwarm\tn",
   "\tBWater\tn"
};


// made this for shapechange, a tad tacky -zusuk
const char *npc_race_menu =
"\r\n"
"  \tbRea\tclms \tWof Lu\tcmin\tbari\tn | npc race selection\r\n"
"---------------------+\r\n"
   "1)  \tWHumanoid\tn\r\n"
   "2)  \tDUndead\tn\r\n"
   "3)  \tgAnimal\tn\r\n"
   "4)  \trDragon\tn\r\n"
   "5)  \tYGiant\tn\r\n"
   "6)  \tRAberration\tn\r\n"
   "7)  \tcConstruct\tn\r\n"
   "8)  \tRElemental\tn\r\n"
   "9)  \tCFey\tn\r\n"
   "10) \tmMagical \tgBeast\tn\r\n"
   "11) \tBMonstrous \tWHumanoid\tn\r\n"
   "12) \tMOoze\tn\r\n"
   "13) \tDOut\twsider\tn\r\n"
   "14) \tGPlant\tn\r\n"
   "15) \tyVermin\tn\r\n";


/* druid shape change race options */
const char *shape_types[] = {
   "Unknown",
   "badger",
   "panther",
   "bear",
   "crocodile"
};
//5 (number of types)


/* druid shape change messages, to room */
const char *shape_to_room[] = {
   "Unknown",
   /* badger */
   "$n shrinks and suddenly grows spiky brown fur all over $s body, $s nose lengthens"
     " into a dirty snout as $s face contorts into an expression of primal"
     " rage.",
   /* panther */
   "$n's back arches into a feline form and $s teeth grow long and sharp.  "
     "Knifelike claws extend from $s newly formed paws and $s body becomes "
     "covered in sleek, dark fur.",
   /* bear */
   "$n's form swells with muscle as $s shoulders expand into a great girth.  "
     "Suddenly $s nose transforms "
     "into a short perceptive snout and $s ears become larger and rounder on the "
     "top of $s head.  Then $s teeth become sharper as claws extend from $s meaty paws.",
   /* crocodile, giant */
   "$n involuntarily drops to the ground on all fours as $s legs shorten to "
     "small stumps and a large tail extends from $s body.  Hard dark scales cover "
     "$s whole body as $s nose and mouth extend into a large tooth-filled maw."
};

/* druid shape change messages, to char */
const char *shape_to_char[] = {
   "Unknown",
   /* badger */
   "You shrink and suddenly grows spiky brown fur all over your body, your nose lengthens"
     " into a dirty snout as his face contorts into an expression of primal"
     " rage.",
   /* panther */
   "Your back arches into a feline form and your teeth grow long and sharp.  "
     "Knifelike claws extend from your newly formed paws and your body becomes "
     "covered in sleek, dark fur.",
   /* bear */
   "Your form swells with muscle as your shoulders expand into a great girth.  "
     "Suddenly you seem more aware of scents in the air as your nose transforms "
     "into a short perceptive snout.  Your ears become larger and rounder on the "
     "top of your head and your teeth become sharper as claws extend from your meaty paws.",
   /* crocodile, giant */
   "You involuntarily drop to the ground on all fours as your legs shorten to "
     "small stumps and a large tail extends from your body.  Hard dark scales cover "
     "your whole body as your nose and mouth extend into a large tooth-filled maw."
};

// shapechange morph messages to_room, original system
const char *morph_to_room[] = {
  /* unknown */
" ",
  /* Humanoid */
" ",
  /* Undead */
"$n's flesh decays visibly, $s features becoming shallow and sunken as $e"
        "turns to the \tDundead\tn.",
  /* Animal */
" ",
  /* Dragon */
"$n's features lengthen, $s skin peeling back to reveal a thick, "
"scaly hide.  Leathery wings sprout from $s shoulders and $s "
"fingers become long, razor-sharp talons.",
  /* Giant */
" ",
  /* Aberration */
" ",
  /* Construct */
" ",
  /* Elemental */
"$n bursts into elemental energy, then becomes that element as $s form shifts to that of a "
"\tRelemental\tn.",
  /* Fey */
" ",
  /* Magical Beast */
" ",
  /* Monstrous Humanoid */
" ",
  /* Ooze */
"$n's bones dissolve and $s flesh becomes translucent as $e changes form "
"into an ooze!",
  /* Outsider */
" ",
  /* Plant */
"Thin vines and shoots curl away from $n's body as $s skin changes to a "
"\tGmottled green plant\tn.",
  /* Vermin */
" "
};

// shapechange morph messages to_char
const char *morph_to_char[] = {
  /* unknown */
" ",
  /* Humanoid */
" ",
  /* Undead */
"Your flesh decays visibly, and your features becoming shallow and sunken as"
" you turn to the \tDundead\tn.",
  /* Animal */
" ",
  /* Dragon */
"Your features lengthen, your skin peeling back to reveal a thick, "
"scaly hide.  Leathery wings sprout from your shoulders and your "
"fingers become long, razor sharp talons.",
  /* Giant */
" ",
  /* Aberration */
" ",
  /* Construct */
" ",
  /* Elemental */
"You burst into fire, then become living flame as your form shifts to that "
"of a \tRfire elemental\tn.",
  /* Fey */
" ",
  /* Magical Beast */
" ",
  /* Monstrous Humanoid */
" ",
  /* Ooze */
"Your bones dissolve and your flesh becomes translucent as you change form "
"into an \tGooze\tn!",
  /* Outsider */
" ",
  /* Plant */
"Thin vines and shoots curl away from your body as your skin changes to a "
"\tGmottled green plant\tn.",
  /* Vermin */
" "
};

// pc race abbreviations, with color
const char *race_abbrevs[] = {
        "\tBHumn\tn",
        "\tYElf \tn",
        "\tgDwrf\tn",
        "\trHTrl\tn",
        "\tCC\tgDwf\tn",
        "\tcHflg\tn",
        "\twH\tYElf\tn",
        "\twH\tROrc\tn",
        "\tmGnme\tn",
        "\tGTr\tYlx\tn",
        "\tRAr\tcGo\tn",
        "\n"
};


// pc race types, full name no color
const char *pc_race_types[] = {
        "Human",
        "Elf",
        "Dwarf",
        "Half-Troll",
        "Crystal-Dwarf",
        "Halfling",
        "Half-Elf",
        "Half-Orc",
        "Gnome",
        "Trelux",
        "Arcana-Golem",
        "\n"
};


// pc character creation menu
// notice, epic races are not manually or in-game settable at this stage
const char *race_menu =
"\r\n"
"  \tbRea\tclms \tWof Lu\tcmin\tbari\tn | race selection\r\n"
"---------------------+\r\n"
"  a)  \tBHuman\tn\r\n"
"  b)  \tYElf\tn\r\n"
"  c)  \tgDwarf\tn\r\n"
"  d)  \trHalf Troll\tn\r\n"
"  f)  \tcHalfling\tn\r\n"
"  g)  \twHalf \tYElf\tn\r\n"
"  h)  \twHalf \tROrc\tn\r\n"
"  i)  \tMGnome\tn\r\n"
"  j)  \tRArcana \tcGolem\tn\r\n"
;


// interpret race for interpreter.c and act.wizard.c etc
// notice, epic races are not manually or in-game settable at this stage
int parse_race(char arg)
{
  arg = LOWER(arg);

  switch (arg) {
  case 'a': return RACE_HUMAN;
  case 'b': return RACE_ELF;
  case 'c': return RACE_DWARF;
  case 'd': return RACE_HALF_TROLL;
  case 'f': return RACE_HALFLING;
  case 'g': return RACE_H_ELF;
  case 'h': return RACE_H_ORC;
  case 'i': return RACE_GNOME;
  case 'j': return RACE_ARCANA_GOLEM;
  default:  return RACE_UNDEFINED;
  }
}

/* accept short descrip, return race */
int parse_race_long(char *arg) {
  int l = 0; /* string length */

  for (l = 0; *(arg + l); l++) /* convert to lower case */
    *(arg + l) = LOWER(*(arg + l));

  if (is_abbrev(arg, "human")) return RACE_HUMAN;
  if (is_abbrev(arg, "elf")) return RACE_ELF;
  if (is_abbrev(arg, "dwarf")) return RACE_DWARF;
  if (is_abbrev(arg, "half-troll")) return RACE_HALF_TROLL;
  if (is_abbrev(arg, "halftroll")) return RACE_HALF_TROLL;
  if (is_abbrev(arg, "halfling")) return RACE_HALFLING;
  if (is_abbrev(arg, "halfelf")) return RACE_H_ELF;
  if (is_abbrev(arg, "half-elf")) return RACE_H_ELF;
  if (is_abbrev(arg, "halforc")) return RACE_H_ORC;
  if (is_abbrev(arg, "half-orc")) return RACE_H_ORC;
  if (is_abbrev(arg, "gnome")) return RACE_GNOME;
  if (is_abbrev(arg, "arcanagolem")) return RACE_ARCANA_GOLEM;
  if (is_abbrev(arg, "arcana-golem")) return RACE_ARCANA_GOLEM;
  if (is_abbrev(arg, "trelux")) return RACE_TRELUX;
  if (is_abbrev(arg, "crystaldwarf")) return RACE_CRYSTAL_DWARF;
  if (is_abbrev(arg, "crystal-dwarf")) return RACE_CRYSTAL_DWARF;

  return RACE_UNDEFINED;
}


// returns the proper integer for the race, given a character
bitvector_t find_race_bitvector(const char *arg)
{
  size_t rpos, ret = 0;

  for (rpos = 0; rpos < strlen(arg); rpos++)
    ret |= (1 << parse_race(arg[rpos]));

  return (ret);
}


/* Invalid wear flags */
int invalid_race(struct char_data *ch, struct obj_data *obj) {
  if ((OBJ_FLAGGED(obj, ITEM_ANTI_HUMAN) && IS_HUMAN(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_ELF)   && IS_ELF(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_HALF_TROLL)   && IS_HALF_TROLL(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_HALFLING)   && IS_HALFLING(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_H_ELF)   && IS_H_ELF(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_H_ORC)   && IS_H_ORC(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_GNOME)   && IS_GNOME(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_CRYSTAL_DWARF)   && IS_CRYSTAL_DWARF(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_TRELUX)   && IS_TRELUX(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_ARCANA_GOLEM)   && IS_ARCANA_GOLEM(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_DWARF) && IS_DWARF(ch)))
        return 1;
  else
        return 0;
}

