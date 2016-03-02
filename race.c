/**************************************************************************
*  File: race.c                                               LuminariMUD *
*  Usage: Source file for race-specific code.                             *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

/** Help buffer the global variable definitions */
#define __RACE_C__

/* This file attempts to concentrate most of the code which must be changed
 * in order for new race to be added.  If you're adding a new race, you
 * should go through this entire file from beginning to end and add the
 * appropriate new special cases for your new race. */

/* Zusuk, 02/2016:  Start notes here!
 * RACE_ these are specific race defines, eventually should be a massive list
 *       of every race in our world
 * SUBRACE_ these are subraces for NPC's, currently set to maximum 3, some
 *          mechanics such as resistances are built into these
 * PC_SUBRACE_ these are subraces for PC's, only used for animal shapes spell
 *             currently, use to be part of the wildshape system
 * RACE_TYPE_ this is like the family the race belongs to, like an iron golem
 *            would be RACE_TYPE_CONSTRUCT
 */

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
#include "handler.h"

/* defines */
#define Y   TRUE
#define N   FALSE

struct race_data race_list[NUM_EXTENDED_RACES];
char *race_names[NUM_EXTENDED_RACES];

void set_race_genders(int race, int neuter, int male, int female) {
  race_list[race].genders[0] = neuter;
  race_list[race].genders[1] = male;
  race_list[race].genders[2] = female;
}

void set_race_abilities(int race, int str_mod, int con_mod, int int_mod, int wis_mod, int dex_mod, int cha_mod) {
  race_list[race].ability_mods[0] = str_mod;
  race_list[race].ability_mods[1] = con_mod;
  race_list[race].ability_mods[2] = int_mod;
  race_list[race].ability_mods[3] = wis_mod;
  race_list[race].ability_mods[4] = dex_mod;
  race_list[race].ability_mods[5] = cha_mod;
}

void set_race_alignments(int race, int lg, int ng, int cg, int ln, int tn, int cn, int le, int ne, int ce) {
  race_list[race].alignments[0] = lg;
  race_list[race].alignments[1] = ng;
  race_list[race].alignments[2] = cg;
  race_list[race].alignments[3] = ln;
  race_list[race].alignments[4] = tn;
  race_list[race].alignments[5] = cn;
  race_list[race].alignments[6] = le;
  race_list[race].alignments[7] = ne;
  race_list[race].alignments[8] = ce;
}

void initialize_races(void) {
  int i = 0;

  for (i = 0; i < NUM_EXTENDED_RACES; i++) {
    race_list[i].name = NULL;
    race_names[i] = NULL;
    race_list[i].abbrev = NULL;
    race_list[i].type = NULL;
    race_list[i].family = RACE_TYPE_UNDEFINED;
    race_list[i].menu_display = NULL;
    race_list[i].size = SIZE_MEDIUM;
    race_list[i].is_pc = FALSE;
    race_list[i].favored_class[0] = CLASS_UNDEFINED;
    race_list[i].favored_class[1] = CLASS_UNDEFINED;
    race_list[i].favored_class[2] = CLASS_UNDEFINED;
    race_list[i].language = 0;
    race_list[i].level_adjustment = 0;
    
    set_race_genders(i, N, N, N);
    set_race_abilities(i, 0, 0, 0, 0, 0, 0);
    set_race_alignments(i, N, N, N, N, N, N, N, N, N);
  }
}

/* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod,
 * height-n,height-m,height-f,weight-n,weight-m,weight-f, lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pic, fav-class, lang, lvl-adjust*/
void add_race(int race, char *name, char *abbrev, char *capName, int family, int neuter, int male, int female,
        int str_mod, int con_mod, int int_mod, int wis_mod, int dex_mod, int cha_mod, int lg, int ln, int le,
        int ng, int tn, int ne, int cg, int cn, int ce, int size, int is_pc, int favored_class, int language, int level_adjustment) {
  race_list[race].name = strdup(name);
  race_names[race] = strdup(name);
  race_list[race].abbrev = strdup(abbrev);
  race_list[race].type = strdup(capName);
  race_list[race].family = family;
  set_race_genders(race, neuter, male, female);
  set_race_abilities(race, str_mod, con_mod, int_mod, wis_mod, dex_mod, cha_mod);
  race_list[race].size = size;
  set_race_alignments(race, lg, ng, cg, ln, tn, cn, le, ne, ce);
  race_list[race].is_pc = is_pc;
  race_list[race].favored_class[0] = favored_class;
  race_list[race].favored_class[1] = favored_class;
  race_list[race].favored_class[2] = favored_class;
  race_list[race].language = language;
  race_list[race].level_adjustment = level_adjustment;
}

void favored_class_female(int race, int favored_class) {
  race_list[race].favored_class[2] = favored_class;
}

void assign_races(void) {
  /* initialization */
  initialize_races();

  /* begin listing */

  /* humanoid */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  add_race(RACE_HUMAN, "human", "Human", "Human", RACE_TYPE_HUMANOID, N, Y, Y, 0, 0, 0, 0, 0, 0,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_UNDEFINED, SKILL_LANG_COMMON, 0);
  add_race(RACE_DROW_ELF, "drow elf", "DrowElf", "Drow Elf", RACE_TYPE_HUMANOID, N, Y, Y, 0, -2, 2, 0, 2, 2,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, FALSE, CLASS_WIZARD, SKILL_LANG_UNDERCOMMON, 2);
  favored_class_female(RACE_DROW_ELF, CLASS_CLERIC);
  add_race(RACE_HALF_ELF, "half elf", "HalfElf", "Half Elf", RACE_TYPE_HUMANOID, N, Y, Y, 0, 0, 0, 0, 0, 0,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WIZARD, SKILL_LANG_ELVEN, 0);
  add_race(RACE_ELF, "elf", "Elf", "Elf", RACE_TYPE_HUMANOID, N, Y, Y, 0, 0, 0, 0, 0, 0,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WIZARD, SKILL_LANG_ELVEN, 0);
  add_race(RACE_CRYSTAL_DWARF, "crystal dwarf", "CrystalDwarf", "Crystal Dwarf", RACE_TYPE_HUMANOID, N, Y, Y, 0, 2, 0, 0, 0, -4,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WARRIOR, SKILL_LANG_UNDERCOMMON, 1);
  add_race(RACE_DWARF, "dwarf", "Dwarf", "Dwarf", RACE_TYPE_HUMANOID, N, Y, Y, 0, 2, 0, 0, 0, -4,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WARRIOR, SKILL_LANG_DWARVEN, 1);
  add_race(RACE_HALFLING, "halfling", "Halfling", "Halfling", RACE_TYPE_HUMANOID, N, Y, Y, 0, 0, 0, 0, 0, 0,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_SMALL, TRUE, CLASS_UNDEFINED, SKILL_LANG_COMMON, 0);
  add_race(RACE_ROCK_GNOME, "rock gnome", "RkGnome", "Rock Gnome", RACE_TYPE_HUMANOID, N, Y, Y, -2, 0, 2, 0, 0, 0,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_SMALL, FALSE, CLASS_WIZARD, SKILL_LANG_GNOME, 0);
  add_race(RACE_DEEP_GNOME, "svirfneblin", "Svfnbln", "Svirfneblin", RACE_TYPE_HUMANOID, N, Y, Y, -2, 0, 0, 2, 2, -4,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_SMALL, FALSE, CLASS_ROGUE, SKILL_LANG_GNOME, 3);
  add_race(RACE_GNOME, "gnome", "Gnome", "Gnome", RACE_TYPE_HUMANOID, N, Y, Y, 0, 0, 2, -2, 2, 0,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_SMALL, TRUE, CLASS_UNDEFINED, SKILL_LANG_GNOME, 0);
  add_race(RACE_HALF_ORC, "half orc", "HalfOrc", "Half Orc", RACE_TYPE_HUMANOID, N, Y, Y, 2, 0, -2, 0, 0, -2,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_BERSERKER, SKILL_LANG_ORCISH, 0);
  add_race(RACE_ORC, "orc", "Orc", "Orc", RACE_TYPE_HUMANOID, N, Y, Y, 2, 2, -2, -2, 0, -2,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, FALSE, CLASS_BERSERKER, SKILL_LANG_ORCISH, 0);

  /* monstrous humanoid */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  add_race(RACE_HALF_TROLL, "half troll", "HalfTroll", "Half Troll", RACE_TYPE_MONSTROUS_HUMANOID, N, Y, Y, 2, 4, 0, 0, 0, -2,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_LARGE, TRUE, CLASS_WARRIOR, SKILL_LANG_GOBLIN, 0);

  /* giant */
  add_race(RACE_HALF_OGRE, "half ogre", "HlfOgre", "Half Ogre", RACE_TYPE_GIANT, N, Y, Y, 6, 4, -2, 0, 2, 0,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_LARGE, FALSE, CLASS_BERSERKER, SKILL_LANG_GIANT, 2);

  /* undead */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  add_race(RACE_SKELETON, "skeleton", "Skeletn", "Skeleton", RACE_TYPE_UNDEAD, Y, N, N, 0, 0, 0, 0, 0, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_ZOMBIE, "zombie", "Zombie", "Zombie", RACE_TYPE_UNDEAD, N, Y, Y, 0, 0, 0, 0, 0, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_GHOUL, "ghoul", "Ghoul", "Ghoul", RACE_TYPE_UNDEAD, N, Y, Y, 0, 0, 0, 0, 0, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_GHAST, "ghast", "Ghast", "Ghast", RACE_TYPE_UNDEAD, N, Y, Y, 0, 0, 0, 0, 0, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_MUMMY, "mummy", "Mummy", "Mummy", RACE_TYPE_UNDEAD, N, Y, Y, 14, 0, 0, 0, 0, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_MOHRG, "mohrg", "Mohrg", "Mohrg", RACE_TYPE_UNDEAD, N, Y, Y, 11, 0, 0, 0, 9, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);

  /* animal */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  add_race(RACE_EAGLE, "eagle", "Eagle", "Eagle", RACE_TYPE_ANIMAL, N, Y, Y, 0, 2, 0, 0, 5, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_SMALL, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_RAT, "rat", "Rat", "Rat", RACE_TYPE_ANIMAL, N, Y, Y, -8, 0, -2, 0, 4, -2,
          N, N, N, N, Y, N, N, N, N, SIZE_TINY, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_WOLF, "wolf", "Wolf", "Wolf", RACE_TYPE_ANIMAL, N, Y, Y, 2, 4, 0, 0, 4, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_GREAT_CAT, "great cat", "Grt Cat", "Great Cat", RACE_TYPE_ANIMAL, N, Y, Y, 4, 2, 0, 0, 2, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_HORSE, "horse", "Horse", "Horse", RACE_TYPE_ANIMAL, N, Y, Y, 6, 4, 0, 0, 2, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_DINOSAUR, "dinosaur", "Dino", "Dinosaur", RACE_TYPE_ANIMAL, N, Y, Y, 10, 0, 0, 0, 0, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_HUGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_LION, "lion", "Lion", "Lion", RACE_TYPE_ANIMAL, N, Y, Y, 10, 4, 0, 0, 6, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_BLACK_BEAR, "black bear", "BlkBear", "Black Bear", RACE_TYPE_ANIMAL, N, Y, Y, 8, 4, 0, 0, 2, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_BROWN_BEAR, "brown bear", "BrnBear", "Brown Bear", RACE_TYPE_ANIMAL, N, Y, Y, 16, 8, 0, 0, 2, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_POLAR_BEAR, "polar bear", "PlrBear", "Polar Bear", RACE_TYPE_ANIMAL, N, Y, Y, 16, 8, 0, 0, 2, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_TIGER, "tiger", "Tiger", "Tiger", RACE_TYPE_ANIMAL, N, Y, Y, 12, 6, 0, 0, 4, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_CONSTRICTOR_SNAKE, "constrictor snake", "CnsSnak", "Constrictor Snake", RACE_TYPE_ANIMAL, N, Y, Y, 6, 2, 0, 0, 6, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_GIANT_CONSTRICTOR_SNAKE, "giant constrictor snake", "GCnSnak", "Giant Constrictor Snake", RACE_TYPE_ANIMAL, N, Y, Y, 14, 2, 0, 0, 6, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_HUGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_MEDIUM_VIPER, "medium viper", "MdViper", "Medium Viper", RACE_TYPE_ANIMAL, N, Y, Y, -2, 0, 0, 0, 6, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_LARGE_VIPER, "large viper", "LgViper", "Large Viper", RACE_TYPE_ANIMAL, N, Y, Y, 0, 0, 0, 0, 6, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_HUGE_VIPER, "huge viper", "HgViper", "Huge Viper", RACE_TYPE_ANIMAL, N, Y, Y, 6, 2, 0, 0, 4, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_HUGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_WOLVERINE, "wolverine", "Wlvrine", "Wolverine", RACE_TYPE_ANIMAL, N, Y, Y, 4, 8, 0, 0, 4, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_RHINOCEROS, "rhinoceros", "Rhino", "Rhinoceros", RACE_TYPE_ANIMAL, N, Y, Y, 16, 10, 0, 0, 0, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_LEOPARD, "leopard", "Leopard", "Leopard", RACE_TYPE_ANIMAL, N, Y, Y, 6, 4, 0, 0, 8, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_HYENA, "hyena", "Hyena", "Hyena", RACE_TYPE_ANIMAL, N, Y, Y, 4, 4, 0, 0, 4, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_CROCODILE, "crocodile", "Crocodl", "Crocodile", RACE_TYPE_ANIMAL, N, Y, Y, 8, 6, 0, 0, 2, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_GIANT_CROCODILE, "giant crocodile", "GCrocdl", "Giant Crocodile", RACE_TYPE_ANIMAL, N, Y, Y, 16, 8, 0, 0, 2, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_HUGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_CHEETAH, "cheetah", "Cheetah", "Cheetah", RACE_TYPE_ANIMAL, N, Y, Y, 6, 4, 0, 0, 8, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_BOAR, "boar", "Boar", "Boar", RACE_TYPE_ANIMAL, N, Y, Y, 4, 10, 0, 0, 0, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_APE, "ape", "Ape", "Ape", RACE_TYPE_ANIMAL, N, Y, Y, 10, 6, 0, 0, 6, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_ELEPHANT, "elephant", "Elephnt", "Elephant", RACE_TYPE_ANIMAL, N, Y, Y, 20, 10, 0, 0, 0, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_HUGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);

  /* plant */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  add_race(RACE_MANDRAGORA, "mandragora", "Mndrgra", "Mandragora", RACE_TYPE_PLANT, Y, Y, Y, 0, 0, 0, 0, 0, 0,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_SMALL, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_MYCANOID, "mycanoid", "Mycanid", "Mycanoid", RACE_TYPE_PLANT, Y, Y, Y, 0, 0, 0, 0, 0, 0,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_SHAMBLING_MOUND, "shambling mound", "Shmbler", "Shambling Mound", RACE_TYPE_PLANT, Y, Y, Y, 0, 0, 0, 0, 0, 0,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_TREANT, "treant", "Treant", "Treant", RACE_TYPE_PLANT, Y, Y, Y, 0, 0, 0, 0, 0, 0,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_HUGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);

  /* ooze */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/

  /* elemental */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  add_race(RACE_SMALL_FIRE_ELEMENTAL, "small fire elemental", "SFirElm", "Small Fire Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 4, 6, 0, 0, 10, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_SMALL, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_MEDIUM_FIRE_ELEMENTAL, "medium fire elemental", "MFirElm", "Medium Fire Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 4, 6, 0, 0, 10, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_LARGE_FIRE_ELEMENTAL, "large fire elemental", "LFirElm", "Large Fire Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 4, 6, 0, 0, 10, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_HUGE_FIRE_ELEMENTAL, "huge fire elemental", "HFirElm", "Huge Fire Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 8, 8, 0, 0, 14, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_HUGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);

  add_race(RACE_SMALL_EARTH_ELEMENTAL, "small earth elemental", "SErtElm", "Small Earth Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 14, 8, 0, 0, -2, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_SMALL, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_MEDIUM_EARTH_ELEMENTAL, "medium earth elemental", "MErtElm", "Medium Earth Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 14, 8, 0, 0, -2, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_LARGE_EARTH_ELEMENTAL, "large earth elemental", "LErtElm", "Large Earth Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 14, 8, 0, 0, -2, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_HUGE_EARTH_ELEMENTAL, "huge earth elemental", "HErtElm", "Huge Earth Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 18, 10, 0, 0, -2, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_HUGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);

  add_race(RACE_SMALL_AIR_ELEMENTAL, "small air elemental", "SAirElm", "Small Air Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 4, 6, 0, 0, 14, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_SMALL, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_MEDIUM_AIR_ELEMENTAL, "medium air elemental", "MAirElm", "Medium Air Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 4, 6, 0, 0, 14, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_LARGE_AIR_ELEMENTAL, "large air elemental", "LAirElm", "Large Air Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 4, 6, 0, 0, 14, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_HUGE_AIR_ELEMENTAL, "huge air elemental", "HAirElm", "Huge Air Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 8, 8, 0, 0, 18, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_HUGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);

  add_race(RACE_SMALL_WATER_ELEMENTAL, "small water elemental", "SWatElm", "Small Water Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 10, 8, 0, 0, 4, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_SMALL, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_MEDIUM_WATER_ELEMENTAL, "medium water elemental", "MWatElm", "Medium Water Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 10, 8, 0, 0, 4, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_LARGE_WATER_ELEMENTAL, "large water elemental", "LWatElm", "Large Water Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 10, 8, 0, 0, 4, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_HUGE_WATER_ELEMENTAL, "huge water elemental", "HWatElm", "Huge Water Elemental", RACE_TYPE_ELEMENTAL, N, Y, Y, 14, 10, 0, 0, 8, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_HUGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);

  /* magical beast */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  add_race(RACE_BLINK_DOG, "blink dog", "BlnkDog", "Blink Dog", RACE_TYPE_MAGICAL_BEAST, N, Y, Y, 0, 0, 0, 0, 0, 0,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);

  /* fey */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  add_race(RACE_PIXIE, "pixie", "Pixie", "Pixie", RACE_TYPE_FEY, N, Y, Y, -4, 0, 4, 0, 4, 4,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_TINY, FALSE, CLASS_DRUID, SKILL_LANG_ELVEN, 3);
  
  /* construct */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  add_race(RACE_IRON_GOLEM, "iron golem", "IronGolem", "Iron Golem", RACE_TYPE_CONSTRUCT, Y, N, N, 0, 0, 0, 0, 0, 0,
          N, N, N, N, Y, N, Y, Y, Y, SIZE_LARGE, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_ARCANA_GOLEM, "arcana golem", "ArcanaGolem", "Arcana Golem", RACE_TYPE_CONSTRUCT, Y, N, N, 0, 0, 0, 0, 0, 0,
          N, N, N, N, Y, N, Y, Y, Y, SIZE_MEDIUM, TRUE, CLASS_WIZARD, SKILL_LANG_COMMON, 0);

  /* outsiders */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  add_race(RACE_AEON_THELETOS, "aeon theletos", "AeonThel", "Theletos Aeon", RACE_TYPE_OUTSIDER, N, Y, Y, 0, 0, 0, 0, 0, 0,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);

  /* dragon */
  /* race-num, abbrev, cap-name, family, neuter,male,female, str-mod,con-mod,int-mod,wis-mod,dex-mod,cha-mod,
   * lg,ln,le,ng,tn,ne,cg,cn,ce, size, is-pc, fav-class, lang, lvl-adjust*/
  add_race(RACE_DRAGON_CLOUD, "dragon cloud", "DrgCloud", "Cloud Dragon", RACE_TYPE_DRAGON, N, Y, Y, 0, 0, 0, 0, 0, 0,
          N, N, N, N, Y, N, Y, Y, Y, SIZE_HUGE, FALSE, CLASS_WARRIOR, SKILL_LANG_DRACONIC, 0);
  
  /* aberration */
  add_race(RACE_TRELUX, "trelux", "Trelux", "Trelux", RACE_TYPE_ABERRATION, N, Y, Y, 0, 0, 0, 0, 0, 0,
          N, N, N, N, Y, N, Y, Y, Y, SIZE_SMALL, TRUE, CLASS_WARRIOR, SKILL_LANG_ABERRATION, 0);
  
  /* end listing */
}

void racial_ability_modifiers(struct char_data *ch) {
  int chrace = 0;
  if (GET_RACE(ch) >= NUM_EXTENDED_RACES || GET_RACE(ch) < 0) {
    log("SYSERR: Unknown race %d in racial_ability_modifiers", GET_RACE(ch));
  } else {
    chrace = GET_RACE(ch);
  }

  ch->real_abils.str += race_list[chrace].ability_mods[0];
  ch->real_abils.con += race_list[chrace].ability_mods[1];
  ch->real_abils.intel += race_list[chrace].ability_mods[2];
  ch->real_abils.wis += race_list[chrace].ability_mods[3];
  ch->real_abils.dex += race_list[chrace].ability_mods[4];
  ch->real_abils.cha += race_list[chrace].ability_mods[5];
}

// npc races
const char *npc_race_types[] = {
   "Unknown", //0
   "Humanoid",
   "Undead",
   "Animal",
   "Dragon",
   "Giant",  //5
   "Aberration",
   "Construct",
   "Elemental",
   "Fey",
   "Magical Beast",  //10
   "Monstrous Humanoid",
   "Ooze",
   "Outsider",
   "Plant",
   "Vermin",  //15
   "Elf",
   "Dwarf",
   "Halfling",
   "Centaur",
   "Gnome",  //20
   "Minotaur",
   "Half Elf",
   "Orc",
   "Goblinoid",  //24
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

/*
int get_size(struct char_data *ch) {
  int racenum;

  if (ch == NULL)
    return SIZE_MEDIUM;

  racenum = GET_RACE(ch);

  if (racenum < 0 || racenum >= NUM_EXTENDED_RACES)
    return SIZE_MEDIUM;

  return (GET_SIZE(ch) = ((affected_by_spell(ch, SPELL_ENLARGE_PERSON) ? 1 : 0) + race_list[racenum].size));
}
 */


/* clear up local defines */
#undef Y
#undef N

/*EOF*/
