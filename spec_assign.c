/**************************************************************************
*  File: spec_assign.c                                     Part of tbaMUD *
*  Usage: Functions to assign function pointers to objs/mobs/rooms        *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "interpreter.h"
#include "spec_procs.h"
#include "ban.h" /* for SPECIAL(gen_board) */
#include "boards.h"
#include "mail.h"

SPECIAL(questmaster);
SPECIAL(shop_keeper);

/* local (file scope only) functions */
static void ASSIGNROOM(room_vnum room, SPECIAL(fname));
static void ASSIGNMOB(mob_vnum mob, SPECIAL(fname));
static void ASSIGNOBJ(obj_vnum obj, SPECIAL(fname));

/* functions to perform assignments */
static void ASSIGNMOB(mob_vnum mob, SPECIAL(fname))
{
  mob_rnum rnum;

  if ((rnum = real_mobile(mob)) != NOBODY)
    mob_index[rnum].func = fname;
  else if (!mini_mud)
    log("SYSERR: Attempt to assign spec to non-existant mob #%d", mob);
}

static void ASSIGNOBJ(obj_vnum obj, SPECIAL(fname))
{
  obj_rnum rnum;

  if ((rnum = real_object(obj)) != NOTHING)
    obj_index[rnum].func = fname;
  else if (!mini_mud)
    log("SYSERR: Attempt to assign spec to non-existant obj #%d", obj);
}

static void ASSIGNROOM(room_vnum room, SPECIAL(fname))
{
  room_rnum rnum;

  if ((rnum = real_room(room)) != NOWHERE)
    world[rnum].func = fname;
  else if (!mini_mud)
    log("SYSERR: Attempt to assign spec to non-existant room #%d", room);
}

/* Assignments */
/* assign special procedures to mobiles. Guildguards, snake, thief, wizard,
 * puff, fido, janitor, and cityguards are now implemented via triggers. */
void assign_mobiles(void)
{
  assign_kings_castle();

  /* cryogenicist */
  ASSIGNMOB(3095, cryogenicist);

  /* guildmasters */
  ASSIGNMOB(120, guild);
  ASSIGNMOB(121, guild);
  ASSIGNMOB(122, guild);
  ASSIGNMOB(123, guild);
  ASSIGNMOB(196, guild);  /* female newbie trainer sanctus */
  ASSIGNMOB(2556, guild);
  ASSIGNMOB(2559, guild);
  ASSIGNMOB(2562, guild);
  ASSIGNMOB(2564, guild);
  ASSIGNMOB(2800, guild);
  ASSIGNMOB(3013, guild);
  ASSIGNMOB(3020, guild);
  ASSIGNMOB(3021, guild);
  ASSIGNMOB(3022, guild);
  ASSIGNMOB(3023, guild);
  ASSIGNMOB(5400, guild);
  ASSIGNMOB(5401, guild);
  ASSIGNMOB(5402, guild);
  ASSIGNMOB(5403, guild);
  ASSIGNMOB(11518, guild);
  ASSIGNMOB(14105, guild);
  ASSIGNMOB(23411, guild);  /* female newbie trainer newbie school */
  ASSIGNMOB(25720, guild);
  ASSIGNMOB(25721, guild);
  ASSIGNMOB(25722, guild);
  ASSIGNMOB(25723, guild);
  ASSIGNMOB(25726, guild);
  ASSIGNMOB(25732, guild);
  ASSIGNMOB(27572, guild);
  ASSIGNMOB(27573, guild);
  ASSIGNMOB(27574, guild);
  ASSIGNMOB(27575, guild);
  ASSIGNMOB(27721, guild);
  ASSIGNMOB(29204, guild);
  ASSIGNMOB(29227, guild);
  ASSIGNMOB(31601, guild);
  ASSIGNMOB(31603, guild);
  ASSIGNMOB(31605, guild);
  ASSIGNMOB(31607, guild);
  ASSIGNMOB(31609, guild);
  ASSIGNMOB(31611, guild);
  ASSIGNMOB(31639, guild);
  ASSIGNMOB(31641, guild);
  ASSIGNMOB(145333, guild);  /* female newbie trainer mosswood village */

  /* mayors */
  ASSIGNMOB(3105, mayor);

  /* postmasters */
  ASSIGNMOB(110, postmaster);
  ASSIGNMOB(1201, postmaster);
  ASSIGNMOB(3010, postmaster);
  ASSIGNMOB(10412, postmaster);
  ASSIGNMOB(10719, postmaster);
  ASSIGNMOB(23496, postmaster);
  ASSIGNMOB(25710, postmaster);
  ASSIGNMOB(27164, postmaster);
  ASSIGNMOB(30128, postmaster);
  ASSIGNMOB(31510, postmaster);
  ASSIGNMOB(103010, postmaster);

  /* receptionists */
  ASSIGNMOB(1200, receptionist);
  ASSIGNMOB(3005, receptionist);
  ASSIGNMOB(5404, receptionist);
  ASSIGNMOB(27713, receptionist);
  ASSIGNMOB(27730, receptionist);

  /* walls */
  ASSIGNMOB(47, wall);
  ASSIGNMOB(90, wall);

  /* hounds */
  ASSIGNMOB(49, hound);

  /* abyss randomizer */
  ASSIGNMOB(142300, abyss_randomizer);

  /* crimson flame zone mob specs */
  ASSIGNMOB(106040, cf_trainingmaster);  // training master
  ASSIGNMOB(106000, cf_alathar);  // lord alathar

  /* Jotunheim */
  ASSIGNMOB(196027, thrym);
  ASSIGNMOB(196077, planetar);
  ASSIGNMOB(196070, ymir);
  ASSIGNMOB(196033, gatehouse_guard);
  ASSIGNMOB(196032, gatehouse_guard);
  ASSIGNMOB(196200, jot_invasion_loader);  // this will load invasion
}

/* assign special procedures to objects */
void assign_objects(void)
{
  ASSIGNOBJ(1226, gen_board);   /* builder's board */
  ASSIGNOBJ(1227, gen_board);   /* staff board */
  ASSIGNOBJ(1228, gen_board);   /* advertising board */
  ASSIGNOBJ(3096, gen_board);	/* social board */
  ASSIGNOBJ(3097, gen_board);	/* freeze board */
  ASSIGNOBJ(3098, gen_board);	/* immortal board */
  ASSIGNOBJ(3099, gen_board);	/* mortal board */
  ASSIGNOBJ(100400, gen_board);   /* quest board */
  ASSIGNOBJ(103093, gen_board);   /* ashenport market board */
  ASSIGNOBJ(103094, gen_board);   /* forger board */
  ASSIGNOBJ(103095, gen_board);   /* areas board */
  ASSIGNOBJ(103096, gen_board);	/* social board */
  ASSIGNOBJ(103097, gen_board);	/* freeze board */
  ASSIGNOBJ(103098, gen_board);	/* immortal board */
  ASSIGNOBJ(103099, gen_board);	/* mortal board */

  ASSIGNOBJ(115, bank);
  ASSIGNOBJ(334, bank);	        /* atm */
  ASSIGNOBJ(336, bank);	        /* cashcard */
  ASSIGNOBJ(3034, bank);        /* atm */
  ASSIGNOBJ(3036, bank);        /* cashcard */
  ASSIGNOBJ(3907, bank);
  ASSIGNOBJ(10640, bank);
  ASSIGNOBJ(10751, bank);
  ASSIGNOBJ(25758, bank);

  /* homeland - need to be converted to objects */
  /*
  ASSIGNOBJ(102536, bank);
  ASSIGNOBJ(103007, bank);
  ASSIGNOBJ(105039, bank);
  ASSIGNOBJ(105807, bank);
  ASSIGNOBJ(108181, bank);
  ASSIGNOBJ(108198, bank);
  ASSIGNOBJ(110421, bank);
  ASSIGNOBJ(111374, bank);
  ASSIGNOBJ(113010, bank);
  ASSIGNOBJ(119102, bank);
  ASSIGNOBJ(121475, bank);
  ASSIGNOBJ(121825, bank);
  ASSIGNOBJ(122685, bank);
  ASSIGNOBJ(125088, bank);
  ASSIGNOBJ(125984, bank);
  ASSIGNOBJ(126906, bank);
  ASSIGNOBJ(127618, bank);
  ASSIGNOBJ(135051, bank);
  ASSIGNOBJ(138809, bank);
  */

  ASSIGNOBJ(3118, crafting_kit);

  ASSIGNOBJ(104072, chionthar_ferry);  //transport

  ASSIGNOBJ(128106, ches);  //weapon

  ASSIGNOBJ(128150, spikeshield);  //shield

  ASSIGNOBJ(136100, air_sphere);  //weapon (lightning)

  /* JOTUNHEIM EQ */
  ASSIGNOBJ(196012, mistweave);
  ASSIGNOBJ(196000, frostbite);
  ASSIGNOBJ(196059, ymir_cloak);
  ASSIGNOBJ(196062, vaprak_claws);
  ASSIGNOBJ(196056, valkyrie_sword);
  ASSIGNOBJ(196081, twilight);
  ASSIGNOBJ(196090, fake_twilight);
  ASSIGNOBJ(196066, giantslayer);
  ASSIGNOBJ(196073, planetar_sword);
}

/* assign special procedures to rooms */
void assign_rooms(void)
{
  room_rnum i;

  ASSIGNROOM(370, crafting_quest);

  ASSIGNROOM(3031, pet_shops);
  ASSIGNROOM(10738, pet_shops);
  ASSIGNROOM(23281, pet_shops);
  ASSIGNROOM(25722, pet_shops);
  ASSIGNROOM(27155, pet_shops);
  ASSIGNROOM(27616, pet_shops);
  ASSIGNROOM(31523, pet_shops);

  ASSIGNROOM(145287, pet_shops); /* mosswood petshop */

  if (CONFIG_DTS_ARE_DUMPS)
    for (i = 0; i <= top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_DEATH))
	world[i].func = dump;
}

struct spec_func_data {
   char *name;
   SPECIAL(*func);
};

struct spec_func_data spec_func_list[] = {
  {"Mayor",          mayor },
  {"Snake",          snake },
  {"Thief",          thief },
  {"wizard",         wizard },
  {"Puff",           puff },
  {"Fido",           fido },
  {"Janitor",        janitor },
  {"Cityguard",      cityguard },
  {"Postmaster",     postmaster },
  {"Receptionist",   receptionist },
  {"Cryogenicist",   cryogenicist},
  {"Bulletin Board", gen_board },
  {"Bank",           bank },
  {"Pet Shop",       pet_shops },
  {"Dump",           dump },
  {"Guildmaster",    guild },
  {"Guild Guard",    guild_guard },
  {"Questmaster",    questmaster },
  {"Shopkeeper",     shop_keeper },
  /* end stock specs */
  {"Magical Wall",   wall },
  {"Faithful Hound", hound },
  {"Mistweave",      mistweave },
  {"Frostbite",      frostbite },
  {"Vaprak Claws",   vaprak_claws },
  {"Valkyrie Sword", valkyrie_sword },
  {"Twilight",       twilight },
  {"Fake Twilight",  fake_twilight },
  {"Giantslayer",    giantslayer },
  {"Planetar Sword", planetar_sword },
  {"Crafting Kit",   crafting_kit },
  {"Chionthar_Ferry",chionthar_ferry },
  {"Ches",           ches },
  {"SpikeShield",    spikeshield },
  {"Air Sphere",     air_sphere },
  {"Crafting Quest",    crafting_quest },
  {"Abyss Randomizer",  abyss_randomizer },
  {"Trainingmaster",    cf_trainingmaster },
  {"Alathar",           cf_alathar },
  {"Thrym",             thrym },
  {"Planetar",          planetar },
  {"Ymir",              ymir },
  {"Gatehouse Guard",   gatehouse_guard },
  {"Invasion",          jot_invasion_loader },

  {"\n", NULL}
};

const char *get_spec_func_name(SPECIAL(*func))
{
  int i;
  for (i=0; *(spec_func_list[i].name) != '\n'; i++) {
    if (func == spec_func_list[i].func) return (spec_func_list[i].name);
  }
  return NULL;
}

