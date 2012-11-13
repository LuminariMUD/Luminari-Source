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
   "Human",
   "Ghoul",
   "Badger",
   "Red Dragon",
   "Hill Giant"
};
const char *npc_race_abbrevs[] = {
   "Unknown",
   "\tWHuman\tn",
   "\tGhoul\tn",
   "\tgBadger\tn",
   "\tRRed Dragon\tn",
   "\tYHill Giant\tn"
};
const char *npc_race_menu = 
"\r\n"
"  \tbRea\tclms \tWof Lu\tcmin\tbari\tn | npc race selection\r\n"
"---------------------+\r\n"
"1)  \tWHuman\tn\r\n"
"2)  \tDGhoul\tn\r\n"
"3)  \tgBadger\tn\r\n"
"4)  \tRRed Dragon\tn\r\n"
"5)  \tYHill Giant\tn\r\n";


// pc races
const char *race_abbrevs[] = {
        "\tBHumn\tn",
        "\tYElf\tn",
        "\tgDwrf\tn",
        "\trTrol\tn",
        "\tCC\tgDwf\tn",
        "\tcHflg\tn",
        "\twH\tYElf\tn",
        "\twH\tROrc\tn",
        "\tmGnme\tn",
        "\n"
};
const char *pc_race_types[] = {
        "Human",
        "Elf",
        "Dwarf",
        "Troll",
        "Crystal Dwarf",
        "Halfling",
        "Half Elf",
        "Half Orc"
        "Gnome"
        "\n"
};
const char *race_menu = 
"\r\n"
"  \tbRea\tclms \tWof Lu\tcmin\tbari\tn | race selection\r\n"
"---------------------+\r\n"
"  a)  \tBHuman\tn\r\n"
"  b)  \tYElf\tn\r\n"
"  c)  \tgDwarf\tn\r\n"
"  d)  \trTroll\tn\r\n"
"  f)  \tcHalfling\tn\r\n"
"  g)  \twHalf \tYElf\tn\r\n"
"  h)  \twHalf \tROrc\tn\r\n"
"  i)  \tMGnome\tn\r\n";

// interpret race for interpreter.c and act.wizard.c etc
int parse_race(char arg)
{
  arg = LOWER(arg);

  switch (arg) {
  case 'a': return RACE_HUMAN;
  case 'b': return RACE_ELF;
  case 'c': return RACE_DWARF;
  case 'd': return RACE_TROLL;
  case 'f': return RACE_HALFLING;
  case 'g': return RACE_H_ELF;  
  case 'h': return RACE_H_ORC;
  case 'i': return RACE_GNOME;
  default:  return RACE_UNDEFINED;
  }
}


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
      (OBJ_FLAGGED(obj, ITEM_ANTI_TROLL)   && IS_TROLL(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_HALFLING)   && IS_HALFLING(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_H_ELF)   && IS_H_ELF(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_H_ORC)   && IS_H_ORC(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_GNOME)   && IS_GNOME(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_CRYSTAL_DWARF)   && IS_CRYSTAL_DWARF(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_DWARF) && IS_DWARF(ch)))
        return 1;
  else
        return 0;
}

