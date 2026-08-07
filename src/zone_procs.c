/**************************************************************************
 *  File: zone_procs.c                                 Part of LuminariMUD *
 *  Usage: Special procedures for zones                                    *
 *  Author:  Zusuk                                                         *
 *                                                                         *
 *  Header File:  spec_procs.h                                             *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "constants.h"
#include "magic/spells.h"
#include "act.h"        /* for act related stuff, like act.offensive fuctions */
#include "spec_procs.h" /**< zone_procs.c is part of the spec_procs module */
#include "combat/fight.h"
#include "graph.h"
#include "mud_event.h"
#include "actions.h"
#include "magic/domains_schools.h"
#include "combat/spec_abilities.h"
#include "obj/treasure.h"
#include "mob/mob_utils.h"       /* for npc_find_target() */
#include "dgscript/dg_scripts.h" /* for load_mtrigger() */
#include "quest/staff_events.h"  /* for staff events!  prisoner treasury! */
#include "character/evolutions.h"
#include "spec/spec_effective_binding.h"
#include "spec/spec_registry.h"

/**********************/
/* Fire Giant Zone(s) */
/**********************/

/*** fg invasion - zusuk threw this together for some quick end-game content ****/

/* limits */
#define MAX_JARL 100    /* jarl hunters */
#define MAX_EFREETI 40  /* efreeti mercs */
#define MAX_FG_GUARDS 6 /* fire giant guards by the king, this is per room */
/* end limits */

/* here is the treasure from the invasion */
/* in treasure room */
#define ETHER_LEGGINGS 34548
#define FLAMEKISS_LYRE 34549
#define FLAMEKISSED_TRANSFORM_HIT_COST 20
/* following 4 items are on the elite squads, 1 per squad */
#define SLAADI_BELT 34550
#define BASTION 34551
#define DIVINE_SPARK 34552
#define BALORSKIN_LEGGINGS 34553
#define ADAMANTINE_LEGGINGS 34554
/* end treasure */

/* this is the list of load rooms */
#define TREASURE_ROOM 34655      /* this will load 2 treasure items */
#define NEAR_KING 34514          /* location for an elite squad */
#define NEAR_QUEEN 34506         /* location for an elite squad */
#define WITH_GRUGNAR 34587       /* location for an elite squad */
#define COMMUNITY_QUARTERS 34644 /* location for an elite squad */
#define DISTRIBUTION_1 34656     /* distribution room, has 10 exits to randomly drop mobs */
#define GUARDROOM_1 34516        /* extra guards load */
#define GUARDROOM_2 34518        /* extra guards load */
#define THRONE_ROOM 34517        /* extra guards load */
#define THE_SHAFT 106700         /* load room for the valkyrie */
/* end list of the load rooms */

/* this is the mobiles we are loading for the invasion */
#define JARL 34543                /* random distribution */
#define EFREETI_MERCS 34544       /* random distribution */
#define THRONE_GUARDS 34545       /* extra king guards */
#define FROST_GIANT_GENERAL 34546 /* elite squad member/leader */
#define FROST_GIANT_MAGE 34547    /* elite squad member */
#define FROST_GIANT_PRIEST 34548  /* elite squad member */
#define THE_VALKYRIE 34549        /* harbringer of the invasion */
/* end mobile list */

/* spec proc for loading the fire giant invasion -zusuk */
SPECIAL(fg_invasion_loader)
{
  struct char_data *leader = NULL, *mob = NULL;
  int i = 0;
  int where = -1;
  struct obj_data *obj = NULL;
  obj_rnum objrnum = NOTHING;
  room_rnum roomrnum = NOWHERE;
  mob_rnum mobrnum = NOWHERE;

  /* if a command is sent to this function, exit...  if we already ran this function, exit */
  if (cmd || PROC_FIRED(ch) == TRUE)
    return 0;

  /* we are loading 3 items from this invasion into the treasure room behind the king */
  /* ether leggings to treasure room */
  if ((objrnum = real_object(ETHER_LEGGINGS)) != NOWHERE)
  {
    if ((obj = read_object(objrnum, REAL)) != NULL)
    {
      if ((roomrnum = real_room(TREASURE_ROOM)) != NOWHERE)
      {
        obj_to_room(obj, roomrnum);
      }
    }
  }
  /* flamekiss lyre to treasure room */
  if ((objrnum = real_object(FLAMEKISS_LYRE)) != NOWHERE)
  {
    if ((obj = read_object(objrnum, REAL)) != NULL)
    {
      if ((roomrnum = real_room(TREASURE_ROOM)) != NOWHERE)
      {
        obj_to_room(obj, roomrnum);
      }
    }
  }
  /* adamantine leggings to treasure room */
  if ((objrnum = real_object(ADAMANTINE_LEGGINGS)) != NOWHERE)
  {
    if ((obj = read_object(objrnum, REAL)) != NULL)
    {
      if ((roomrnum = real_room(TREASURE_ROOM)) != NOWHERE)
      {
        obj_to_room(obj, roomrnum);
      }
    }
  }
  /* end treasure room code */

  /* extra jarls to deal with, these will distribute to one of 10 rooms, check out the distribution room to see the exits */
  for (i = 0; i < MAX_JARL; i++)
  {
    if ((mobrnum = real_mobile(JARL)) != NOBODY)
    {
      if ((mob = read_mobile(mobrnum, REAL)) != NULL)
      {
        if ((roomrnum = real_room(DISTRIBUTION_1)) != NOWHERE)
        {
          char_to_room(mob, roomrnum);
        }
      }
    }
  }
  /* extra efreeti mercs to deal with, these will distribute to one of 10 rooms, check out the distribution room to see the exits */
  for (i = 0; i < MAX_EFREETI; i++)
  {
    if ((mobrnum = real_mobile(EFREETI_MERCS)) != NOBODY)
    {
      if ((mob = read_mobile(mobrnum, REAL)) != NULL)
      {
        if ((roomrnum = real_room(DISTRIBUTION_1)) != NOWHERE)
        {
          char_to_room(mob, roomrnum);
        }
      }
    }
  }
  /* end distribution room distributing */

  /* extra throne guards around the king */
  for (i = 0; i < MAX_FG_GUARDS; i++)
  {
    if ((mobrnum = real_mobile(THRONE_GUARDS)) != NOBODY)
    {
      if ((mob = read_mobile(mobrnum, REAL)) != NULL)
      {
        if ((roomrnum = real_room(GUARDROOM_1)) != NOWHERE)
        {
          char_to_room(mob, roomrnum);
        }
      }
    }
  }
  for (i = 0; i < MAX_FG_GUARDS; i++)
  {
    if ((mobrnum = real_mobile(THRONE_GUARDS)) != NOBODY)
    {
      if ((mob = read_mobile(mobrnum, REAL)) != NULL)
      {
        if ((roomrnum = real_room(GUARDROOM_2)) != NOWHERE)
        {
          char_to_room(mob, roomrnum);
        }
      }
    }
  }
  for (i = 0; i < MAX_FG_GUARDS; i++)
  {
    if ((mobrnum = real_mobile(THRONE_GUARDS)) != NOBODY)
    {
      if ((mob = read_mobile(mobrnum, REAL)) != NULL)
      {
        if ((roomrnum = real_room(THRONE_ROOM)) != NOWHERE)
        {
          char_to_room(mob, roomrnum);
        }
      }
    }
  }
  /* end throne guards */

  /**************************************************/

  /* we have 4 elite hit-groups composed of
       - 4 generals
       - 3 wizards
       - 3 priests
    they are laid out here to obtain the 4 items */

  /* near the king */
  /* assign the leader + equip him with the special item */
  if ((mobrnum = real_mobile(FROST_GIANT_GENERAL)) != NOBODY)
  {
    if ((leader = read_mobile(mobrnum, REAL)) != NULL)
    {
      if ((roomrnum = real_room(NEAR_KING)) != NOWHERE)
      {
        char_to_room(leader, roomrnum);
        /* create our leader */
        if (!GROUP(leader))
          create_group(leader);

        /* equip him/her */
        obj = read_object(SLAADI_BELT, VIRTUAL);
        obj_to_char(obj, leader);
        where = find_eq_pos(leader, obj, 0);
        perform_wear(leader, obj, where);
      }
    }
  }
  /* 3 more generals */
  for (i = 0; i < 3; i++)
  {
    if ((mobrnum = real_mobile(FROST_GIANT_GENERAL)) != NOBODY)
    {
      if ((mob = read_mobile(mobrnum, REAL)) != NULL)
      {
        if ((roomrnum = real_room(NEAR_KING)) != NOWHERE)
        {
          char_to_room(mob, roomrnum);
          if (leader)
          {
            add_follower(mob, leader);
            if (!GROUP(mob))
              join_group(mob, GROUP(leader));
          }
        }
      }
    }
  }
  /* 3 mages */
  for (i = 0; i < 3; i++)
  {
    if ((mobrnum = real_mobile(FROST_GIANT_MAGE)) != NOBODY)
    {
      if ((mob = read_mobile(mobrnum, REAL)) != NULL)
      {
        if ((roomrnum = real_room(NEAR_KING)) != NOWHERE)
        {
          char_to_room(mob, roomrnum);
          if (leader)
          {
            add_follower(mob, leader);
            if (!GROUP(mob))
              join_group(mob, GROUP(leader));
          }
        }
      }
    }
  }
  /* 3 priests */
  for (i = 0; i < 3; i++)
  {
    if ((mobrnum = real_mobile(FROST_GIANT_PRIEST)) != NOBODY)
    {
      if ((mob = read_mobile(mobrnum, REAL)) != NULL)
      {
        if ((roomrnum = real_room(NEAR_KING)) != NOWHERE)
        {
          char_to_room(mob, roomrnum);
          if (leader)
          {
            add_follower(mob, leader);
            if (!GROUP(mob))
              join_group(mob, GROUP(leader));
          }
        }
      }
    }
  }
  /* END near the king */

  /* near the queen */
  /* assign the leader + equip him with the special item */
  if ((mobrnum = real_mobile(FROST_GIANT_GENERAL)) != NOBODY)
  {
    if ((leader = read_mobile(mobrnum, REAL)) != NULL)
    {
      if ((roomrnum = real_room(NEAR_QUEEN)) != NOWHERE)
      {
        char_to_room(leader, roomrnum);
        /* create our leader */
        if (!GROUP(leader))
          create_group(leader);

        /* equip him/her */
        obj = read_object(BASTION, VIRTUAL);
        obj_to_char(obj, leader);
        where = find_eq_pos(leader, obj, 0);
        perform_wear(leader, obj, where);
      }
    }
  }
  /* 3 more generals */
  for (i = 0; i < 3; i++)
  {
    if ((mobrnum = real_mobile(FROST_GIANT_GENERAL)) != NOBODY)
    {
      if ((mob = read_mobile(mobrnum, REAL)) != NULL)
      {
        if ((roomrnum = real_room(NEAR_QUEEN)) != NOWHERE)
        {
          char_to_room(mob, roomrnum);
          if (leader)
          {
            add_follower(mob, leader);
            if (!GROUP(mob))
              join_group(mob, GROUP(leader));
          }
        }
      }
    }
  }
  /* 3 mages */
  for (i = 0; i < 3; i++)
  {
    if ((mobrnum = real_mobile(FROST_GIANT_MAGE)) != NOBODY)
    {
      if ((mob = read_mobile(mobrnum, REAL)) != NULL)
      {
        if ((roomrnum = real_room(NEAR_QUEEN)) != NOWHERE)
        {
          char_to_room(mob, roomrnum);
          if (leader)
          {
            add_follower(mob, leader);
            if (!GROUP(mob))
              join_group(mob, GROUP(leader));
          }
        }
      }
    }
  }
  /* 3 priests */
  for (i = 0; i < 3; i++)
  {
    if ((mobrnum = real_mobile(FROST_GIANT_PRIEST)) != NOBODY)
    {
      if ((mob = read_mobile(mobrnum, REAL)) != NULL)
      {
        if ((roomrnum = real_room(NEAR_QUEEN)) != NOWHERE)
        {
          char_to_room(mob, roomrnum);
          if (leader)
          {
            add_follower(mob, leader);
            if (!GROUP(mob))
              join_group(mob, GROUP(leader));
          }
        }
      }
    }
  }
  /* END near the queen */

  /* with grungnar */
  /* assign the leader + equip him with the special item */
  if ((mobrnum = real_mobile(FROST_GIANT_GENERAL)) != NOBODY)
  {
    if ((leader = read_mobile(mobrnum, REAL)) != NULL)
    {
      if ((roomrnum = real_room(WITH_GRUGNAR)) != NOWHERE)
      {
        char_to_room(leader, roomrnum);
        /* create our leader */
        if (!GROUP(leader))
          create_group(leader);

        /* equip him/her */
        obj = read_object(DIVINE_SPARK, VIRTUAL);
        obj_to_char(obj, leader);
        where = find_eq_pos(leader, obj, 0);
        perform_wear(leader, obj, where);
      }
    }
  }
  /* 3 more generals */
  for (i = 0; i < 3; i++)
  {
    if ((mobrnum = real_mobile(FROST_GIANT_GENERAL)) != NOBODY)
    {
      if ((mob = read_mobile(mobrnum, REAL)) != NULL)
      {
        if ((roomrnum = real_room(WITH_GRUGNAR)) != NOWHERE)
        {
          char_to_room(mob, roomrnum);
          if (leader)
          {
            add_follower(mob, leader);
            if (!GROUP(mob))
              join_group(mob, GROUP(leader));
          }
        }
      }
    }
  }
  /* 3 mages */
  for (i = 0; i < 3; i++)
  {
    if ((mobrnum = real_mobile(FROST_GIANT_MAGE)) != NOBODY)
    {
      if ((mob = read_mobile(mobrnum, REAL)) != NULL)
      {
        if ((roomrnum = real_room(WITH_GRUGNAR)) != NOWHERE)
        {
          char_to_room(mob, roomrnum);
          if (leader)
          {
            add_follower(mob, leader);
            if (!GROUP(mob))
              join_group(mob, GROUP(leader));
          }
        }
      }
    }
  }
  /* 3 priests */
  for (i = 0; i < 3; i++)
  {
    if ((mobrnum = real_mobile(FROST_GIANT_PRIEST)) != NOBODY)
    {
      if ((mob = read_mobile(mobrnum, REAL)) != NULL)
      {
        if ((roomrnum = real_room(WITH_GRUGNAR)) != NOWHERE)
        {
          char_to_room(mob, roomrnum);
          if (leader)
          {
            add_follower(mob, leader);
            if (!GROUP(mob))
              join_group(mob, GROUP(leader));
          }
        }
      }
    }
  }
  /* END with grugnar */

  /* community quarter */
  /* assign the leader + equip him with the special item */
  if ((mobrnum = real_mobile(FROST_GIANT_GENERAL)) != NOBODY)
  {
    if ((leader = read_mobile(mobrnum, REAL)) != NULL)
    {
      if ((roomrnum = real_room(COMMUNITY_QUARTERS)) != NOWHERE)
      {
        char_to_room(leader, roomrnum);
        /* create our leader */
        if (!GROUP(leader))
          create_group(leader);

        /* equip him/her */
        obj = read_object(BALORSKIN_LEGGINGS, VIRTUAL);
        obj_to_char(obj, leader);
        where = find_eq_pos(leader, obj, 0);
        perform_wear(leader, obj, where);
      }
    }
  }
  /* 3 more generals */
  for (i = 0; i < 3; i++)
  {
    if ((mobrnum = real_mobile(FROST_GIANT_GENERAL)) != NOBODY)
    {
      if ((mob = read_mobile(mobrnum, REAL)) != NULL)
      {
        if ((roomrnum = real_room(COMMUNITY_QUARTERS)) != NOWHERE)
        {
          char_to_room(mob, roomrnum);
          if (leader)
          {
            add_follower(mob, leader);
            if (!GROUP(mob))
              join_group(mob, GROUP(leader));
          }
        }
      }
    }
  }
  /* 3 mages */
  for (i = 0; i < 3; i++)
  {
    if ((mobrnum = real_mobile(FROST_GIANT_MAGE)) != NOBODY)
    {
      if ((mob = read_mobile(mobrnum, REAL)) != NULL)
      {
        if ((roomrnum = real_room(COMMUNITY_QUARTERS)) != NOWHERE)
        {
          char_to_room(mob, roomrnum);
          if (leader)
          {
            add_follower(mob, leader);
            if (!GROUP(mob))
              join_group(mob, GROUP(leader));
          }
        }
      }
    }
  }
  /* 3 priests */
  for (i = 0; i < 3; i++)
  {
    if ((mobrnum = real_mobile(FROST_GIANT_PRIEST)) != NOBODY)
    {
      if ((mob = read_mobile(mobrnum, REAL)) != NULL)
      {
        if ((roomrnum = real_room(COMMUNITY_QUARTERS)) != NOWHERE)
        {
          char_to_room(mob, roomrnum);
          if (leader)
          {
            add_follower(mob, leader);
            if (!GROUP(mob))
              join_group(mob, GROUP(leader));
          }
        }
      }
    }
  }
  /* END with grugnar */

  /* Valkyrie - the harbringer of the invasion */
  if ((roomrnum = real_room(THE_SHAFT)) != NOWHERE)
  {
    if ((mob = read_mobile(THE_VALKYRIE, VIRTUAL)) != NULL)
    {
      char_to_room(mob, roomrnum);
    }
  }

  /* signal that this is all done */
  PROC_FIRED(ch) = TRUE;

  /* exit */
  return 1;
}

static int flamekissed_instrument_subtype(const char *argument)
{
  int subtype;

  if (argument == NULL)
    return -1;

  for (subtype = 0; subtype < MAX_INSTRUMENTS; subtype++)
  {
    if (!str_cmp(argument, instrument_subtype_name(subtype)))
      return subtype;
  }

  return -1;
}

/* This special instrument transforms to any subtype for a nonlethal hit-point cost. */
SPECIAL(flamekissed_instrument)
{
  struct obj_data *obj;
  int subtype;
  char char_message[MAX_STRING_LENGTH];
  char room_message[MAX_STRING_LENGTH];

  if (ch == NULL || argument == NULL)
    return 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch,
                 "Say an instrument subtype while wearing this instrument to transform it. "
                 "The transformation costs %d hit points and cannot reduce you below 1.\r\n",
                 FLAMEKISSED_TRANSFORM_HIT_COST);
    return 1;
  }

  obj = (struct obj_data *)me;

  if (obj == NULL || !cmd || !CMD_IS("say") || obj->worn_by != ch)
    return 0;

  skip_spaces(&argument);
  subtype = flamekissed_instrument_subtype(argument);

  if (!is_valid_instrument_subtype(subtype))
    return 0;

  if (GET_HIT(ch) <= FLAMEKISSED_TRANSFORM_HIT_COST)
  {
    send_to_char(ch,
                 "The flames refuse to answer. You need more than %d hit points to transform "
                 "the instrument.\r\n",
                 FLAMEKISSED_TRANSFORM_HIT_COST);
    return 1;
  }

  snprintf(char_message, sizeof(char_message),
           "\tyAs you say, '\tW%s\ty' to $p\ty, it rises from your hand. \tRFlame engulfs "
           "it and you\ty as it transforms, then returns to your hands.\tn",
           instrument_subtype_name(subtype));
  snprintf(room_message, sizeof(room_message),
           "\tyAs $n\ty says, '\tW%s\ty' to $p\ty, it rises from $s hand. \tRFlame engulfs "
           "it and $m\ty as it transforms, then returns to $s hands.\tn",
           instrument_subtype_name(subtype));
  act(char_message, FALSE, ch, obj, NULL, TO_CHAR);
  act(room_message, FALSE, ch, obj, NULL, TO_ROOM);

  GET_OBJ_VAL(obj, INSTRUMENT_VALUE_TYPE) = subtype;
  GET_HIT(ch) -= FLAMEKISSED_TRANSFORM_HIT_COST;
  USE_MOVE_ACTION(ch);

  return 1;
}

/* Undefines! */
#undef MAX_JARL
#undef MAX_EFREETI
#undef MAX_FG_GUARDS
#undef ETHER_LEGGINGS
#undef FLAMEKISS_LYRE
#undef FLAMEKISSED_TRANSFORM_HIT_COST
#undef SLAADI_BELT
#undef BASTION
#undef DIVINE_SPARK
#undef BALORSKIN_LEGGINGS
#undef TREASURE_ROOM
#undef NEAR_KING
#undef NEAR_QUEEN
#undef WITH_GRUGNAR
#undef COMMUNITY_QUARTERS
#undef DISTRIBUTION_1
#undef GUARDROOM_1
#undef GUARDROOM_2
#undef THRONE_ROOM
#undef THE_SHAFT
#undef JARL
#undef EFREETI_MERCS
#undef THRONE_GUARDS
#undef FROST_GIANT_GENERAL
#undef FROST_GIANT_MAGE
#undef FROST_GIANT_PRIEST
#undef THE_VALKYRIE
/* end of undefines */

/**********************/
/*   End Fire Giant   */
/**********************/

/*****************/
/* Jot           */
/*****************/

#define JOT_VNUM 1960

#define MAX_FG 60    // fire giants
#define MAX_SB 20    // smoking beard batallion
#define MAX_EM 20    // efreeti mercenaries
#define MAX_FROST 65 // frost giants

bool jot_inv_check = false;

ACMD_DECL(do_say);

/* just made this to help facilitate switching of zone vnums if needed */
int jot_converter(int value)
{
  return (JOT_VNUM * 100) + value;
}

/* currently unused */
void jot_invasion()
{
  if (jot_inv_check)
    return;

  jot_inv_check = true;

  if (rand_number(0, 99) <= 2)
    return;
}

/* load rooms for fire giants */
int fg_pos[MAX_FG] = {295, 295, 295, 295, 295, 295, 295, 295, 295, 295, 295, 295, 295, 295, 295,
                      295, 295, 295, 295, 295, 295, 295, 295, 295, 295, 295, 295, 295, 295, 295,
                      295, 295, 295, 295, 295, 295, 295, 295, 295, 295, 215, 215, 215, 215, 215,
                      212, 218, 222, 207, 188, 204, 204, 204, 204, 196, 204, 204, 204, 204, 196};

/* load rooms for smoking beard batallion */
int sb_pos[MAX_SB] = {295, 295, 295, 295, 295, 295, 295, 295, 295, 295,
                      295, 295, 295, 215, 215, 188, 188, 217, 206, 206};

/* load rooms for frost giants */
int frost_pos[MAX_FROST] = {286, 286, 282, 283, 284, 285, 285, 285, 286, 286, 273, 273, 270,
                            270, 269, 273, 273, 270, 270, 269, 266, 266, 267, 264, 264, 266,
                            266, 267, 264, 264, 265, 272, 272, 271, 271, 228, 240, 240, 233,
                            233, 233, 235, 235, 235, 251, 251, 252, 252, 253, 253, 251, 252,
                            252, 253, 253, 244, 244, 255, 255, 254, 254, 256, 256, 243, 243};

/* spec proc for loading the jot invasion */
SPECIAL(jot_invasion_loader)
{
  struct char_data *tch = NULL, *chmove = NULL, *glammad = NULL, *leader = NULL, *mob = NULL;
  int i = 0;
  int where = -1;
  struct obj_data *obj = NULL, *obj2 = NULL;
  obj_rnum objrnum = NOTHING;
  room_rnum roomrnum = NOWHERE;
  mob_rnum mobrnum = NOWHERE;

  if (cmd || PROC_FIRED(ch) == TRUE)
    return 0;

  /* moving these special mobiles from their storage room to jot */
  for (tch = world[ch->in_room].people; tch; tch = chmove)
  {
    chmove = tch->next_in_room;
    /* glammad */
    if (GET_MOB_VNUM(tch) == (mob_vnum)jot_converter(80))
    {
      if ((roomrnum = real_room(jot_converter(204))) != NOWHERE)
      {
        glammad = tch; /* going to use this to form a group */
        char_from_room(glammad);
        char_to_room(glammad, roomrnum);
        if (!GROUP(glammad))
          create_group(glammad);
      }
    }
    /* fire giant captain(s) */
    if (GET_MOB_VNUM(tch) == (mob_vnum)jot_converter(81))
    {
      if ((roomrnum = real_room(jot_converter(204))) != NOWHERE)
      {
        char_from_room(tch);
        char_to_room(tch, roomrnum);
      }
    }
    /* sirthon quilen */
    if (GET_MOB_VNUM(tch) == (mob_vnum)jot_converter(83))
    {
      if ((roomrnum = real_room(jot_converter(115))) != NOWHERE)
      {
        char_from_room(tch);
        char_to_room(tch, roomrnum);
      }
    }
  }

  /* soldiers to glammad */
  for (i = 0; i < 2; i++)
  {
    mob = read_mobile(jot_converter(78), VIRTUAL);
    obj = read_object(jot_converter(17), VIRTUAL);
    if (obj && mob)
    {
      obj_to_char(obj, mob);
      perform_wield(mob, obj, TRUE);
      if ((roomrnum = real_room(jot_converter(204))) != NOWHERE)
      {
        char_to_room(mob, roomrnum);
        SET_BIT_AR(MOB_FLAGS(mob), MOB_SENTINEL);
        REMOVE_BIT_AR(MOB_FLAGS(mob), MOB_LISTEN);
        if (glammad)
        {
          add_follower(mob, glammad);
          if (!GROUP(mob))
            join_group(mob, GROUP(glammad));
        }
      }
    }
  }

  /* twilight to treasure room */
  if ((objrnum = real_object(jot_converter(90))) != NOWHERE)
  {
    if ((obj = read_object(objrnum, REAL)) != NULL)
    {
      if ((roomrnum = real_room(jot_converter(296))) != NOWHERE)
      {
        obj_to_room(obj, roomrnum);
      }
    }
  }
  /* fire giant crown to treasure room */
  if ((objrnum = real_object(jot_converter(82))) != NOWHERE)
  {
    if ((obj = read_object(objrnum, REAL)) != NULL)
    {
      if ((roomrnum = real_room(jot_converter(296))) != NOWHERE)
      {
        obj_to_room(obj, roomrnum);
      }
    }
  }

  /* extra jarls to deal with */
  for (i = 0; i < 2; i++)
  { /* treasure room */
    if ((mobrnum = real_mobile(jot_converter(39))) != NOBODY)
    {
      if ((mob = read_mobile(mobrnum, REAL)) != NULL)
      {
        if ((roomrnum = real_room(jot_converter(296))) != NOWHERE)
        {
          char_to_room(mob, roomrnum);
        }
      }
    }
  }
  for (i = 0; i < 3; i++)
  { /* uthgard loki throne room */
    if ((mobrnum = real_mobile(jot_converter(39))) != NOBODY)
    {
      if ((mob = read_mobile(mobrnum, REAL)) != NULL)
      {
        if ((roomrnum = real_room(jot_converter(287))) != NOWHERE)
        {
          char_to_room(mob, roomrnum);
        }
      }
    }
  }

  /* heavily guarded gatehouse, frost giant mage is leading this group */
  if ((mobrnum = real_mobile(jot_converter(90))) != NOBODY)
  {
    if ((leader = read_mobile(mobrnum, REAL)) != NULL)
    {
      if ((roomrnum = real_room(jot_converter(266))) != NOWHERE)
      {
        char_to_room(leader, roomrnum);
        if (!GROUP(leader))
          create_group(leader);
      }
    }
  }
  /* 2nd mage in group */
  if ((mobrnum = real_mobile(jot_converter(90))) != NOBODY)
  {
    if ((mob = read_mobile(mobrnum, REAL)) != NULL)
    {
      if ((roomrnum = real_room(jot_converter(266))) != NOWHERE)
      {
        char_to_room(mob, roomrnum);
        if (leader)
        {
          add_follower(mob, leader);
          if (!GROUP(mob))
            join_group(mob, GROUP(leader));
        }
      }
    }
  }
  /* citadel guards join the group */
  if (roomrnum != NOWHERE)
  {
    for (i = 0; i < 8; i++)
    {
      mob = read_mobile(jot_converter(33), VIRTUAL);
      obj = read_object(jot_converter(28), VIRTUAL);
      if (mob && obj)
      {
        obj_to_char(obj, mob);
        perform_wield(mob, obj, TRUE);
      }
      if ((obj2 = read_object(jot_converter(41), VIRTUAL)) != NULL)
      {
        obj_to_char(obj2, mob);
        where = find_eq_pos(mob, obj2, 0);
        perform_wear(mob, obj2, where);
      }
      char_to_room(mob, roomrnum);
      if (leader)
      {
        add_follower(mob, leader);
        if (!GROUP(mob))
          join_group(mob, GROUP(leader));
      }
    }
  }

  /* large gatehouse group, led by a mage again */
  if ((mobrnum = real_mobile(jot_converter(90))) != NOBODY)
  {
    if ((leader = read_mobile(mobrnum, REAL)) != NULL)
    {
      if ((roomrnum = real_room(jot_converter(252))) != NOWHERE)
      {
        char_to_room(leader, roomrnum);
        if (!GROUP(leader))
          create_group(leader);
      }
    }
  }
  /* 2nd mage in group */
  if ((mobrnum = real_mobile(jot_converter(90))) != NOBODY)
  {
    if ((mob = read_mobile(mobrnum, REAL)) != NULL)
    {
      if ((roomrnum = real_room(jot_converter(252))) != NOWHERE)
      {
        char_to_room(mob, roomrnum);
        if (leader)
        {
          add_follower(mob, leader);
          if (!GROUP(mob))
            join_group(mob, GROUP(leader));
        }
      }
    }
  }
  /* citadel guards join the group */
  if (roomrnum != NOWHERE)
  {
    for (i = 0; i < 5; i++)
    {
      mob = read_mobile(jot_converter(33), VIRTUAL);
      obj = read_object(jot_converter(28), VIRTUAL);
      if (mob && obj)
      {
        obj_to_char(obj, mob);
        perform_wield(mob, obj, TRUE);
      }
      if ((obj2 = read_object(jot_converter(40), VIRTUAL)) != NULL)
      {
        obj_to_char(obj2, mob);
        where = find_eq_pos(mob, obj2, 0);
        perform_wear(mob, obj2, where);
      }
      char_to_room(mob, roomrnum);
      if (leader)
      {
        add_follower(mob, leader);
        if (!GROUP(mob))
          join_group(mob, GROUP(leader));
      }
    }
  }

  /* load up some firegiants, then equip them */
  for (i = 0; i < MAX_FG; i++)
  {
    if ((roomrnum = real_room(jot_converter(fg_pos[i]))) != NOWHERE)
    {
      if ((mob = read_mobile(jot_converter(78), VIRTUAL)) != NULL)
      {
        char_to_room(mob, roomrnum);
        if ((obj = read_object(jot_converter(17), VIRTUAL)) != NULL)
        {
          obj_to_char(obj, mob);
          perform_wield(mob, obj, TRUE);
        }
      }
    }
  }

  /* load up smoking beard batallion */
  for (i = 0; i < MAX_SB; i++)
  {
    if ((roomrnum = real_room(jot_converter(sb_pos[i]))) != NOWHERE)
    {
      if ((mob = read_mobile(jot_converter(79), VIRTUAL)) != NULL)
      {
        char_to_room(mob, roomrnum);
      }
    }
  }

  /* efreeti mercenary */
  for (i = 0; i < MAX_EM; i++)
  {
    if ((roomrnum = real_room(jot_converter(295))) != NOWHERE)
    {
      if ((mob = read_mobile(jot_converter(84), VIRTUAL)) != NULL)
      {
        char_to_room(mob, roomrnum);
      }
    }
  }

  /* Extra frost giants */
  for (i = 0; i < MAX_FROST; i++)
  {
    if ((roomrnum = real_room(jot_converter(frost_pos[i]))) != NOWHERE)
    {
      if ((mob = read_mobile(jot_converter(85), VIRTUAL)) != NULL)
      {
        char_to_room(mob, roomrnum);
        if ((obj = read_object(jot_converter(28), VIRTUAL)) != NULL)
        {
          obj_to_char(obj, mob);
          perform_wield(mob, obj, TRUE);
        }
      }
    }
  }

  /* Valkyrie */
  if ((roomrnum = real_room(jot_converter(4))) != NOWHERE)
  {
    if ((mob = read_mobile(jot_converter(82), VIRTUAL)) != NULL)
    {
      char_to_room(mob, roomrnum);
    }
  }

  /* Remove Brunnhilde */
  for (mob = character_list; mob; mob = mob->next)
    if (GET_MOB_VNUM(mob) == (mob_vnum)jot_converter(68))
      extract_char(mob);

  PROC_FIRED(ch) = TRUE;
  return 1;
}

/* thrym jot fight spec */
SPECIAL(thrym)
{
  if (!ch)
    return 0;

  struct char_data *vict = FIGHTING(ch);
  struct affected_type af;
  int bonus = 0;

  if (cmd || !vict || rand_number(0, 8))
    return 0;

  if (paralysis_immunity(vict))
  {
    send_to_char(ch, "Your target is unfazed.\r\n");
    return 1;
  }

  if (HAS_EVOLUTION(vict, EVOLUTION_UNDEAD_APPEARANCE))
    bonus += get_evolution_appearance_save_bonus(vict);

  // no save, unless have special feat
  if (HAS_FEAT(vict, FEAT_PARALYSIS_RESIST) ||
      savingthrow(ch, vict, SAVING_FORT, 4 + bonus, CAST_INNATE, 30, ENCHANTMENT))
  {
    send_to_char(ch, "Your target is unfazed.\r\n");
    return 1;
  }

  act("\tCThrym touches you with a chilling hand, freezing you in place.\tn", FALSE, vict, 0, ch,
      TO_CHAR);
  act("\tCThrym touches $n\tC, freezing $m in place.\tn", FALSE, vict, 0, ch, TO_ROOM);

  new_affect(&af);
  af.spell = SPELL_HOLD_PERSON;
  SET_BIT_AR(af.bitvector, AFF_PARALYZED);
  af.duration = 8;
  affect_join(vict, &af, TRUE, FALSE, FALSE, FALSE);

  return 1;
}

SPECIAL(ymir)
{
  if (!ch || cmd)
    return 0;

  if (FIGHTING(ch) && !rand_number(0, 4))
  {
    call_magic(ch, FIGHTING(ch), 0, SPELL_FROST_BREATHE, 0, GET_LEVEL(ch), CAST_INNATE);
    return 1;
  }

  return 0;
}

SPECIAL(planetar)
{
  if (!ch || cmd)
    return 0;

  if (FIGHTING(ch) && !rand_number(0, 5))
  {
    call_magic(ch, FIGHTING(ch), 0, SPELL_LIGHTNING_BREATHE, 0, GET_LEVEL(ch), CAST_INNATE);
    return 1;
  }

  return 0;
}

SPECIAL(gatehouse_guard)
{
  struct char_data *mob = (struct char_data *)me;

  if (!IS_MOVE(cmd) || AFF_FLAGGED(mob, AFF_BLIND) || AFF_FLAGGED(mob, AFF_SLEEP) ||
      AFF_FLAGGED(mob, AFF_PARALYZED) || AFF_FLAGGED(mob, AFF_GRAPPLED) ||
      AFF_FLAGGED(mob, AFF_ENTANGLED) || HAS_WAIT(mob))
    return FALSE;

  if (cmd == SCMD_EAST && (!IS_NPC(ch) || IS_PET(ch)) && GET_LEVEL(ch) < 31)
  {
    act("$N \twblocks your way!\tn\r\n", FALSE, ch, 0, mob, TO_CHAR);
    act("$N \twblocks $n's\tw way!\tn\r\n", FALSE, ch, 0, mob, TO_ROOM);
    return TRUE;
  }

  return 0;
}

/************************************************/
/* end mobile specs, start object specs for jot */
/************************************************/

/* special cloak object proc */
SPECIAL(ymir_cloak)
{
  if (!ch)
    return 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Invoke ice storm by saying 'icicle storm'.\r\nOnce per day.\r\n");
    return 1;
  }

  struct obj_data *obj = (struct obj_data *)me;

  if (cmd && argument && CMD_IS("say"))
  {
    if (!is_wearing(ch, 196059))
      return 0;

    skip_spaces(&argument);

    if (!strcmp(argument, "icicle storm"))
    {
      if (GET_OBJ_SPECTIMER(obj, 0) > 0)
      {
        send_to_char(ch, "\tcAs you say '\tCicicle storm\tc' to your \tWa cloak of glittering "
                         "icicles\tc, nothing happens.\tn\r\n");
        return 1;
      }

      weapons_spells("\tBAs you say '\twicicle storm\tB' to $p \tBit flashes bright blue and sends "
                     "forth a storm of razor sharp icicles in all directions.\tn",
                     "\tBAs $n \tBmutters something under his breath  to $p \tBit flashes bright "
                     "blue and sends forth a storm of razor sharp icicles in all directions.\tn",
                     "\tBAs $n \tBmutters something under his breath  to $p \tBit flashes bright "
                     "blue and sends forth a storm of razor sharp icicles in all directions.\tn",
                     ch, 0, (struct obj_data *)me, SPELL_ICE_STORM);
      GET_OBJ_SPECTIMER(obj, 0) = 6;
      return 1;
    }
  }
  return 0;
}

/* mistweave mace object proc */
SPECIAL(mistweave)
{
  if (!ch)
    return 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Invoke blindness by saying 'mistweave'. Once per day.\r\n");
    return 1;
  }

  struct obj_data *obj = (struct obj_data *)me;
  struct char_data *vict = FIGHTING(ch);

  if (cmd && argument && CMD_IS("say"))
  {
    if (!is_wearing(ch, 196012))
      return 0;

    skip_spaces(&argument);

    if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room) && !strcmp(argument, "mistweave"))
    {
      if (GET_OBJ_SPECTIMER(obj, 0) > 0)
      {
        send_to_char(ch, "\tpAs you say '\twmistweave\tp' to your a huge adamantium mace "
                         "enshrouded with \tWmist\tp, nothing happens.\tn\r\n");
        return 1;
      }
      act("\tLAs you say, '\tnmistweave\tL', "
          "\tLa thick vapor issues forth from $p\tL, "
          "\tLenshrouding the eyes of $N\tL.\tn",
          FALSE, ch, obj, vict, TO_CHAR);
      act("\tLAs $n \tLmutters something under his breath, "
          "\tLa thick vapor issues forth from $p\tL, "
          "\tLenshrouding the eyes of $N.",
          FALSE, ch, obj, vict, TO_ROOM);

      call_magic(ch, FIGHTING(ch), 0, SPELL_BLINDNESS, 0, 30, CAST_WEAPON_SPELL);
      GET_OBJ_SPECTIMER(obj, 0) = 24;
      return 1;
    }
    else
      return 0;
  }
  return 0;
}

/* frostbite axe proc */
SPECIAL(frostbite)
{
  if (!ch)
    return 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Invoke cone of cold  by saying 'frostbite'. Once per day.\r\n");
    return 1;
  }

  struct obj_data *obj = (struct obj_data *)me;
  struct char_data *vict = FIGHTING(ch);
  int pct;
  struct affected_type af;

  if (cmd && argument && CMD_IS("say"))
  {
    if (!is_wearing(ch, 196000))
      return 0;

    skip_spaces(&argument);

    if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room) && !strcmp(argument, "frostbite"))
    {
      if (GET_OBJ_SPECTIMER(obj, 0) > 0)
      {
        send_to_char(ch, "\tcAs you say '\twfrostbite\tc' to your a \tLa great iron axe \tCrimmed "
                         "\tLwith \tWfrost\tc, nothing happens.\tn\r\n");
        return 1;
      }
      act("\tCAs you say, '\twfrostbite\tC',\n\r"
          "\tCa swirling gale of pounding ice emanates forth from\n\r"
          "$p \tCpelting your foes.\tn",
          FALSE, ch, obj, 0, TO_CHAR);
      act("\tCAs $n \tCmutters something under his breath,\n\r"
          "\tCa swirling gale of pounding ice emanates forth from\n\r"
          "$p \tCpelting $n's \tCfoes.\tn",
          FALSE, ch, obj, 0, TO_ROOM);

      pct = rand_number(0, 99);
      if (pct < 55)
        call_magic(ch, vict, 0, SPELL_CONE_OF_COLD, 0, 20, CAST_WEAPON_SPELL);
      else if (pct < 85)
        call_magic(ch, vict, 0, SPELL_CONE_OF_COLD, 0, 30, CAST_WEAPON_SPELL);
      else
      {
        call_magic(ch, vict, 0, SPELL_CONE_OF_COLD, 0, 30, CAST_WEAPON_SPELL);
        if (!paralysis_immunity(vict))
        {
          new_affect(&af);
          af.spell = SPELL_HOLD_PERSON;
          SET_BIT_AR(af.bitvector, AFF_PARALYZED);
          af.duration = dice(2, 4);
          affect_join(vict, &af, TRUE, FALSE, FALSE, FALSE);
        }
      }

      GET_OBJ_SPECTIMER(obj, 0) = 24;
      return 1;
    }
    else
      return 0;
  }
  return 0;
}

/* special claws gear with proc */
#define VAP_AFFECTS 3

SPECIAL(vaprak_claws)
{
  struct affected_type af[VAP_AFFECTS];
  int duration = 0, i = 0;
  struct obj_data *obj = (struct obj_data *)me;

  skip_spaces(&argument);

  if (!ch)
    return 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Invoke Fury of Vaprak by saying 'vaprak'. Once per day.\r\nWorks only for "
                     "Trolls and Ogres.\r\n");
    return 1;
  }

  if (!argument)
    return 0;

  /*
  if (GET_RACE(ch) != RACE_OGRE && GET_RACE(ch) != RACE_HALF_TROLL)
    return 0;
   */
  if (GET_RACE(ch) != RACE_HALF_TROLL)
    return 0;

  if (!is_wearing(ch, 196062))
    return 0;

  skip_spaces(&argument);

  if (!strcmp(argument, "vaprak") && CMD_IS("say"))
  {
    // if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room)) {
    if (GET_OBJ_SPECTIMER(obj, 0) > 0)
    {
      send_to_char(ch, "\trAs you say '\twvaprak\tr' to your claws \tLof the destroyer\tr, nothing "
                       "happens.\tn\r\n");
      return 1;
    }

    if (affected_by_spell(ch, SKILL_RAGE) || affected_by_spell(ch, SKILL_DEFENSIVE_STANCE))
    {
      send_to_char(ch, "You are already raging or in a defensive stance!\r\n");
      return 1;
    }

    weapons_spells("\tLAs you say '\twvaprak\tL' to $p\tL, an evil warmth fills your body.\tn", 0,
                   "\tr$n \trmutters something under his breath.\tn", ch, ch, (struct obj_data *)me,
                   0);

    duration = GET_LEVEL(ch);
    /* init affect array */
    for (i = 0; i < VAP_AFFECTS; i++)
    {
      new_affect(&(af[i]));
      af[i].spell = SKILL_RAGE;
      af[i].duration = duration;
    }

    af[0].location = APPLY_HITROLL;
    af[0].modifier = 3;

    af[1].location = APPLY_DAMROLL;
    af[1].modifier = 3;

    af[2].location = APPLY_SAVING_WILL;
    af[2].modifier = 3;
    SET_BIT_AR(af[2].bitvector, AFF_HASTE);

    for (i = 0; i < VAP_AFFECTS; i++)
      affect_join(ch, af + i, FALSE, FALSE, FALSE, FALSE);
    GET_OBJ_SPECTIMER(obj, 0) = 24;
    return 1; /* success! */
  }

  return 0;
}
#undef VAP_AFFECTS

/* a fake twilight proc (large sword) */
#define TWI_AFFECTS 2

SPECIAL(fake_twilight)
{
  struct affected_type af[TWI_AFFECTS];
  struct char_data *vict;
  int duration = 0, i = 0;

  if (!ch)
    return 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Twilight Rage.\r\n");
    return 1;
  }

  vict = FIGHTING(ch);

  if (affected_by_spell(ch, SPELL_BATTLETIDE))
  {
    return 0;
  }

  if (cmd || !vict || rand_number(0, 16))
    return 0;

  weapons_spells("\tLA glimmer of insanity crosses your face as your\r\n"
                 "\tLblade starts glowing with a strong \tpmagenta\tL sheen.\tn",
                 "\tLA glimmer of insanity crosses $n\tL's face as $s\r\n"
                 "\tLblade starts glowing with a strong \tpmagenta\tL sheen.\tn",
                 "\tLA glimmer of insanity crosses $n\tL's face as $s\r\n"
                 "\tLblade starts glowing with a strong \tpmagenta\tL sheen.\tn",
                 ch, vict, (struct obj_data *)me, 0);

  duration = GET_LEVEL(ch) / 5;
  /* init affect array */
  for (i = 0; i < TWI_AFFECTS; i++)
  {
    new_affect(&(af[i]));
    af[i].spell = SPELL_BATTLETIDE;
    af[i].duration = duration;
  }

  af[0].location = APPLY_HITROLL;
  af[0].modifier = GET_STR_BONUS(ch);

  af[1].location = APPLY_DAMROLL;
  af[1].modifier = GET_STR_BONUS(ch);

  for (i = 0; i < TWI_AFFECTS; i++)
    affect_join(ch, af + i, FALSE, FALSE, FALSE, FALSE);

  return 1;
}

/* a twilight proc (large sword) */
SPECIAL(twilight)
{
  struct affected_type af[TWI_AFFECTS];
  struct char_data *vict;
  int duration = 0, i = 0;

  if (!ch)
    return 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Twilight Rage!\r\n");
    return 1;
  }

  vict = FIGHTING(ch);

  if (affected_by_spell(ch, SPELL_BATTLETIDE))
  {
    return 0;
  }

  if (cmd || !vict || rand_number(0, 12))
    return 0;

  weapons_spells("\tLA glimmer of insanity crosses your face as your\r\n"
                 "\tLblade starts glowing with a strong \tpmagenta\tL sheen.\tn",
                 "\tLA glimmer of insanity crosses $n\tL's face as $s\r\n"
                 "\tLblade starts glowing with a strong \tpmagenta\tL sheen.\tn",
                 "\tLA glimmer of insanity crosses $n\tL's face as $s\r\n"
                 "\tLblade starts glowing with a strong \tpmagenta\tL sheen.\tn",
                 ch, vict, (struct obj_data *)me, 0);

  duration = GET_LEVEL(ch) / 5 + 1;
  /* init affect array */
  for (i = 0; i < TWI_AFFECTS; i++)
  {
    new_affect(&(af[i]));
    af[i].spell = SPELL_BATTLETIDE;
    af[i].duration = duration;
  }

  af[0].location = APPLY_HITROLL;
  af[0].modifier = GET_STR_BONUS(ch);
  af[0].bonus_type = BONUS_TYPE_CIRCUMSTANCE;

  af[1].location = APPLY_DAMROLL;
  af[1].modifier = GET_STR_BONUS(ch);
  af[1].bonus_type = BONUS_TYPE_CIRCUMSTANCE;

  for (i = 0; i < TWI_AFFECTS; i++)
    affect_join(ch, af + i, FALSE, FALSE, FALSE, FALSE);

  return 1;
}
#undef TWI_AFFECTS

SPECIAL(valkyrie_sword)
{
  if (!ch || cmd)
    return 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Female Only - Proc Burning Hands\r\n");
    return 1;
  }

  if (GET_SEX(ch) != SEX_FEMALE && !IS_NPC(ch))
  {
    damage(ch, ch, dice(5, 4), -1, DAM_HOLY, FALSE);
    send_to_char(ch, "\twYou are \tYburned \twby holy light.\tn\r\n");
    act("\tw$n is \tYburned \twby holy light.\tn", FALSE, ch, 0, ch, TO_ROOM);
    return 1;
  }

  struct char_data *vict = FIGHTING(ch);

  if (!is_wearing(ch, 196056) || !vict || rand_number(0, 20))
    return 0;

  weapons_spells("\tYStreaks of flames issue forth from $p\n\r"
                 "\tYengulfing your foe.\tn",
                 "\tYYou are engulfed by the flames issuing forth from $p.",
                 "\tYStreaks of flames issue forth from $p\n\r"
                 "\tYengulfing $n's \tYfoe.",
                 ch, vict, (struct obj_data *)me, 0);

  call_magic(ch, vict, 0, SPELL_BURNING_HANDS, 0, 30, CAST_WEAPON_SPELL);

  return 1;
}

SPECIAL(planetar_sword)
{
  if (!ch)
    return 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc Cure Critical and Dispel Evil\r\n");
    return 1;
  }

  struct char_data *vict = FIGHTING(ch);

  if (cmd || !vict || rand_number(0, 27))
    return 0;

  switch (rand_number(0, 1))
  {
  case 1:
    weapons_spells("\tWA nimbus of holy light surrounds your sword, bathing you in its radiance\tn",
                   0,
                   "\tWA nimbus of holy light surrounds $n's\tW sword, bathing $m in its radiance.",
                   ch, ch, (struct obj_data *)me, SPELL_CURE_CRITIC);
    call_magic(ch, ch, 0, SPELL_CURE_CRITIC, 0, GET_LEVEL(ch), CAST_WEAPON_SPELL);
    return 1;
  case 2:
    weapons_spells(
        "\tWA glowing nimbus of light emanates forth blasting the foul evil in its presence.\tn",
        "\tWA glowing nimbus of light emanates forth from $n, blasting the foul evil in its "
        "presence.\tn",
        "\tWA glowing nimbus of light emanates forth from $n, blasting the foul evil in its "
        "presence.\tn",
        ch, vict, (struct obj_data *)me, SPELL_DISPEL_EVIL);
    return 1;
  default:
    return 0;
  }

  return 1;
}

SPECIAL(giantslayer)
{
  if (!ch)
    return 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Invoke giant hamstring attack by saying 'hamstring'. Once per day.\r\nWorks "
                     "only for Dwarves.\r\n");
    return 1;
  }

  switch (GET_RACE(ch))
  {
  case RACE_DWARF:
    break;

  case RACE_DUERGAR:
    break;

  default:
    return 0;
    break;
  }

  struct obj_data *obj = (struct obj_data *)me;
  struct char_data *vict = FIGHTING(ch);

  if (!vict)
    return 0;

  skip_spaces(&argument);
  if (!is_wearing(ch, 196066))
    return 0;
  if (!strcmp(argument, "hamstring"))
  {
    if (IS_NPC(vict) && GET_RACE(vict) == RACE_TYPE_GIANT && (vict->in_room == ch->in_room))
    {
      if (GET_OBJ_SPECTIMER(obj, 0) > 0)
      {
        send_to_char(ch, "\tYAs you say '\twhamstring\tY' to your \tLa double-bladed dwarvish axe "
                         "of \tYgiantslaying, nothing happens.\tn\r\n");
        return 1;
      }

      act("\tyAs you say, '\tLhamstring\ty' to $p\ty,\n\r"
          "\tyit twirls forth from your hand, arcing through the air to "
          "hamstring\n\r$N \tybefore returning to your grasp.\tn",
          FALSE, ch, obj, vict, TO_CHAR);
      act("\tyAs $n \tymutters something under his breath to $p\ty,\n\r"
          "\tyit twirls forth from $s hand, arcing through the air to "
          "hamstring\n\r$N \tybefore returning to your grasp.\tn",
          FALSE, ch, obj, vict, TO_ROOM);
      // We hamstring the foe
      act("$N falls to $S knees before you!", FALSE, ch, obj, vict, TO_CHAR);
      act("$N falls to $S knees before $n!", FALSE, ch, obj, vict, TO_NOTVICT);
      act("You fall to your knees in agony!", FALSE, ch, obj, vict, TO_VICT);
      USE_MOVE_ACTION(vict);
      change_position(vict, POS_SITTING);
      GET_HIT(vict) -= 100;

      GET_OBJ_SPECTIMER(obj, 0) = 24;
      return 1; // end for
    }
    else
    {
      send_to_char(ch, "\tYAs you say '\twhamstring\tY' to your \tLa double-bladed dwarvish axe of "
                       "\tYgiantslaying, nothing happens.\tn\r\n");
      return 1;
    }
    return 0;
  }
  return 0;
}

#undef JOT_VNUM
#undef MAX_FG    // fire giants
#undef MAX_SB    // smoking beard batallion
#undef MAX_EM    // efreeti mercenaries
#undef MAX_FROST // frost giants

/*****************/
/* End Jot       */
/*****************/

/*****************/
/* Mad Drow */
/*****************/

bool open_msg = FALSE;
bool close_msg = FALSE;

struct slider_row
{
  int room;
  int door;
};

struct slider_row row_1_a_n_s[] = {{155521, EAST}, {155522, WEST}, {155530, EAST}, {155529, WEST},
                                   {155531, EAST}, {155532, WEST}, {155540, EAST}, {155539, WEST},
                                   {155541, EAST}, {155542, WEST}, {-1, -1}};
struct slider_row row_1_b_n_s[] = {{155546, EAST}, {155547, WEST}, {155555, EAST}, {155554, WEST},
                                   {155556, EAST}, {155557, WEST}, {155565, EAST}, {155564, WEST},
                                   {155566, EAST}, {155567, WEST}, {-1, -1}};
struct slider_row row_1_c_n_s[] = {{155571, EAST}, {155572, WEST}, {155580, EAST}, {155579, WEST},
                                   {155581, EAST}, {155582, WEST}, {155590, EAST}, {155589, WEST},
                                   {155591, EAST}, {155592, WEST}, {-1, -1}};
struct slider_row row_1_d_n_s[] = {{155596, EAST}, {155597, WEST}, {155605, EAST}, {155604, WEST},
                                   {155606, EAST}, {155607, WEST}, {155615, EAST}, {155614, WEST},
                                   {155616, EAST}, {155617, WEST}, {-1, -1}};
struct slider_row row_1_e_n_s[] = {{155625, EAST}, {155624, WEST}, {155626, EAST}, {155627, WEST},
                                   {155635, EAST}, {155634, WEST}, {155636, EAST}, {155637, WEST},
                                   {155645, EAST}, {155644, WEST}, {-1, -1}};
struct slider_row row_2_a_n_s[] = {{155522, EAST}, {155523, WEST}, {155529, EAST}, {155528, WEST},
                                   {155532, EAST}, {155533, WEST}, {155539, EAST}, {155538, WEST},
                                   {155542, EAST}, {155543, WEST}, {-1, -1}};
struct slider_row row_2_b_n_s[] = {{155547, EAST}, {155548, WEST}, {155554, EAST}, {155553, WEST},
                                   {155557, EAST}, {155558, WEST}, {155564, EAST}, {155563, WEST},
                                   {155567, EAST}, {155568, WEST}, {-1, -1}};
struct slider_row row_2_c_n_s[] = {{155572, EAST}, {155573, WEST}, {155579, WEST}, {155578, WEST},
                                   {155582, EAST}, {155583, WEST}, {155589, EAST}, {155588, WEST},
                                   {155592, EAST}, {155593, WEST}, {-1, -1}};
struct slider_row row_2_d_n_s[] = {{155597, EAST}, {155598, WEST}, {155604, EAST}, {155603, WEST},
                                   {155607, EAST}, {155608, WEST}, {155614, EAST}, {155613, WEST},
                                   {155617, EAST}, {155618, WEST}, {-1, -1}};
struct slider_row row_2_e_n_s[] = {{155624, EAST}, {155623, WEST}, {155627, EAST}, {155628, WEST},
                                   {155634, EAST}, {155633, WEST}, {155637, EAST}, {155638, WEST},
                                   {155644, EAST}, {155643, WEST}, {-1, -1}};
struct slider_row row_3_a_n_s[] = {{155523, EAST}, {155534, WEST}, {155528, EAST}, {155527, WEST},
                                   {155533, EAST}, {155534, WEST}, {155538, EAST}, {155537, WEST},
                                   {155543, EAST}, {155544, WEST}, {-1, -1}};
struct slider_row row_3_b_n_s[] = {{155548, EAST}, {155549, WEST}, {155553, EAST}, {155552, WEST},
                                   {155559, EAST}, {155559, WEST}, {155563, EAST}, {155562, WEST},
                                   {155568, EAST}, {155569, WEST}, {-1, -1}};
struct slider_row row_3_c_n_s[] = {{155573, EAST}, {155574, WEST}, {155578, EAST}, {155577, WEST},
                                   {155583, EAST}, {155584, WEST}, {155588, EAST}, {155587, WEST},
                                   {155593, EAST}, {155594, WEST}, {-1, -1}};
struct slider_row row_3_d_n_s[] = {{155598, EAST}, {155599, WEST}, {155603, EAST}, {155602, WEST},
                                   {155608, EAST}, {155609, WEST}, {155613, EAST}, {155612, WEST},
                                   {155618, EAST}, {155619, WEST}, {-1, -1}};
struct slider_row row_3_e_n_s[] = {{155623, EAST}, {155622, WEST}, {155628, EAST}, {155629, WEST},
                                   {155633, EAST}, {155632, WEST}, {155638, EAST}, {155639, WEST},
                                   {155643, EAST}, {155642, WEST}, {-1, -1}};
struct slider_row row_4_a_n_s[] = {{155524, EAST}, {155525, WEST}, {155527, EAST}, {155526, WEST},
                                   {155534, EAST}, {155535, WEST}, {155537, EAST}, {155536, WEST},
                                   {155544, EAST}, {155545, WEST}, {-1, -1}};
struct slider_row row_4_b_n_s[] = {{155549, EAST}, {155550, WEST}, {155552, EAST}, {155551, WEST},
                                   {155559, EAST}, {155560, WEST}, {155562, EAST}, {155561, WEST},
                                   {155569, EAST}, {155570, WEST}, {-1, -1}};
struct slider_row row_4_c_n_s[] = {{155574, EAST}, {155575, WEST}, {155577, EAST}, {155576, WEST},
                                   {155584, EAST}, {155585, WEST}, {155587, EAST}, {155586, WEST},
                                   {155594, EAST}, {155595, WEST}, {-1, -1}};
struct slider_row row_4_d_n_s[] = {{155599, EAST}, {155600, WEST}, {155602, EAST}, {155601, WEST},
                                   {155609, EAST}, {155610, WEST}, {155612, EAST}, {155611, WEST},
                                   {155619, EAST}, {155620, WEST}, {-1, -1}};
struct slider_row row_4_e_n_s[] = {{155622, EAST}, {155621, WEST}, {155629, EAST}, {155630, WEST},
                                   {155632, EAST}, {155631, WEST}, {155632, EAST}, {155631, WEST},
                                   {155639, EAST}, {155640, WEST}, {155642, EAST}, {155641, WEST},
                                   {-1, -1}};
struct slider_row row_1_a_e_w[] = {{155521, SOUTH}, {155530, NORTH}, {155522, SOUTH},
                                   {155523, SOUTH}, {155524, SOUTH}, {155525, SOUTH},
                                   {155529, NORTH}, {155528, NORTH}, {155527, NORTH},
                                   {155526, NORTH}, {-1, -1}};
struct slider_row row_1_b_e_w[] = {{155546, SOUTH}, {155547, SOUTH}, {155548, SOUTH},
                                   {155549, SOUTH}, {155550, SOUTH}, {155555, NORTH},
                                   {155554, NORTH}, {155553, NORTH}, {155552, NORTH},
                                   {155551, NORTH}, {-1, -1}};
struct slider_row row_1_c_e_w[] = {{155571, SOUTH}, {155572, SOUTH}, {155573, SOUTH},
                                   {155574, SOUTH}, {155575, SOUTH}, {155580, NORTH},
                                   {155579, NORTH}, {155578, NORTH}, {155577, NORTH},
                                   {155576, NORTH}, {-1, -1}};
struct slider_row row_1_d_e_w[] = {{155596, SOUTH}, {155597, SOUTH}, {155598, SOUTH},
                                   {155599, SOUTH}, {155600, SOUTH}, {155605, NORTH},
                                   {155604, NORTH}, {155603, NORTH}, {155602, NORTH},
                                   {155601, NORTH}, {-1, -1}};
struct slider_row row_1_e_e_w[] = {{155625, SOUTH}, {155624, SOUTH}, {155623, SOUTH},
                                   {155622, SOUTH}, {155621, SOUTH}, {155626, NORTH},
                                   {155627, NORTH}, {155628, NORTH}, {155629, NORTH},
                                   {155630, NORTH}, {-1, -1}};
struct slider_row row_2_a_e_w[] = {
    {155530, SOUTH}, {155531, NORTH}, {155529, SOUTH}, {155527, SOUTH}, {155526, SOUTH},
    {155532, NORTH}, {155533, NORTH}, {155534, NORTH}, {155535, NORTH}, {-1, -1}};
struct slider_row row_2_b_e_w[] = {{155555, SOUTH}, {155554, SOUTH}, {155553, SOUTH},
                                   {155552, SOUTH}, {155551, SOUTH}, {155556, NORTH},
                                   {155557, NORTH}, {155558, NORTH}, {155559, NORTH},
                                   {155560, NORTH}, {-1, -1}};
struct slider_row row_2_c_e_w[] = {{155580, SOUTH}, {155579, SOUTH}, {155578, SOUTH},
                                   {155577, SOUTH}, {155576, SOUTH}, {155581, NORTH},
                                   {155582, NORTH}, {155583, NORTH}, {155584, NORTH},
                                   {155585, NORTH}, {-1, -1}};
struct slider_row row_2_d_e_w[] = {{155605, SOUTH}, {155604, SOUTH}, {155603, SOUTH},
                                   {155602, SOUTH}, {155601, SOUTH}, {155606, NORTH},
                                   {155607, NORTH}, {155608, NORTH}, {155609, NORTH},
                                   {155610, NORTH}, {-1, -1}};
struct slider_row row_2_e_e_w[] = {{155626, SOUTH}, {155627, SOUTH}, {155628, SOUTH},
                                   {155629, SOUTH}, {155630, SOUTH}, {155635, NORTH},
                                   {155634, NORTH}, {155633, NORTH}, {155632, NORTH},
                                   {155631, NORTH}, {-1, -1}};
struct slider_row row_3_a_e_w[] = {{155531, SOUTH}, {155540, NORTH}, {155532, SOUTH},
                                   {155533, SOUTH}, {155534, SOUTH}, {155535, SOUTH},
                                   {155539, NORTH}, {155538, NORTH}, {155537, NORTH},
                                   {155536, NORTH}, {-1, -1}};
struct slider_row row_3_b_e_w[] = {{155556, SOUTH}, {155557, SOUTH}, {155558, SOUTH},
                                   {155559, SOUTH}, {155560, SOUTH}, {155565, NORTH},
                                   {155564, NORTH}, {155563, NORTH}, {155562, NORTH},
                                   {155561, NORTH}, {-1, -1}};
struct slider_row row_3_c_e_w[] = {{155581, SOUTH}, {155582, SOUTH}, {155583, SOUTH},
                                   {155584, SOUTH}, {155585, SOUTH}, {155590, NORTH},
                                   {155589, NORTH}, {155588, NORTH}, {155587, NORTH},
                                   {155586, NORTH}, {-1, -1}};
struct slider_row row_3_d_e_w[] = {{155606, SOUTH}, {155607, SOUTH}, {155608, SOUTH},
                                   {155609, SOUTH}, {155610, SOUTH}, {155615, NORTH},
                                   {155614, NORTH}, {155613, NORTH}, {155612, NORTH},
                                   {155611, NORTH}, {-1, -1}};
struct slider_row row_3_e_e_w[] = {{155635, SOUTH}, {155634, SOUTH}, {155633, SOUTH},
                                   {155632, SOUTH}, {155631, SOUTH}, {155636, NORTH},
                                   {155637, NORTH}, {155638, NORTH}, {155639, NORTH},
                                   {155640, NORTH}, {-1, -1}};
struct slider_row row_4_a_e_w[] = {{155540, SOUTH}, {155541, NORTH}, {155539, SOUTH},
                                   {155538, SOUTH}, {155537, SOUTH}, {155536, SOUTH},
                                   {155542, NORTH}, {155543, NORTH}, {155544, NORTH},
                                   {155545, NORTH}, {-1, -1}};
struct slider_row row_4_b_e_w[] = {{155565, SOUTH}, {155564, SOUTH}, {155563, SOUTH},
                                   {155562, SOUTH}, {155561, SOUTH}, {155566, NORTH},
                                   {155567, NORTH}, {155568, NORTH}, {155569, NORTH},
                                   {155570, NORTH}, {-1, -1}};
struct slider_row row_4_c_e_w[] = {{155590, SOUTH}, {155589, SOUTH}, {155588, SOUTH},
                                   {155587, SOUTH}, {155586, SOUTH}, {155591, NORTH},
                                   {155592, NORTH}, {155593, NORTH}, {155594, NORTH},
                                   {155595, NORTH}, {-1, -1}};
struct slider_row row_4_d_e_w[] = {{155615, SOUTH}, {155614, SOUTH}, {155613, SOUTH},
                                   {155612, SOUTH}, {155611, SOUTH}, {155616, NORTH},
                                   {155617, NORTH}, {155618, NORTH}, {155619, NORTH},
                                   {155620, NORTH}, {-1, -1}};
struct slider_row row_4_e_e_w[] = {{155636, SOUTH}, {155637, SOUTH}, {155638, SOUTH},
                                   {155639, SOUTH}, {155640, SOUTH}, {155645, NORTH},
                                   {155644, NORTH}, {155643, NORTH}, {155642, NORTH},
                                   {155641, NORTH}, {-1, -1}};
struct slider_row row_1_a_u_d[] = {{155521, DOWN}, {155546, UP}, {155530, DOWN}, {155555, UP},
                                   {155531, DOWN}, {155556, UP}, {155540, DOWN}, {155565, UP},
                                   {155541, DOWN}, {155566, UP}, {-1, -1}};
struct slider_row row_1_b_u_d[] = {{155522, DOWN}, {155547, UP}, {155529, DOWN}, {155554, UP},
                                   {155532, DOWN}, {155557, UP}, {155539, DOWN}, {155564, UP},
                                   {155542, DOWN}, {155567, UP}, {-1, -1}};
struct slider_row row_1_c_u_d[] = {{155523, DOWN}, {155528, DOWN}, {155533, DOWN}, {155538, DOWN},
                                   {155543, DOWN}, {155548, UP},   {155553, UP},   {155558, UP},
                                   {155563, UP},   {155568, UP},   {-1, -1}};
struct slider_row row_1_d_u_d[] = {{155524, DOWN}, {155527, DOWN}, {155534, DOWN}, {155537, DOWN},
                                   {155544, DOWN}, {155549, UP},   {155552, UP},   {155559, UP},
                                   {155562, UP},   {155569, UP},   {-1, -1}};
struct slider_row row_1_e_u_d[] = {{155525, DOWN}, {155526, DOWN}, {155535, DOWN}, {155536, DOWN},
                                   {155545, DOWN}, {155550, UP},   {155551, UP},   {155560, UP},
                                   {155561, UP},   {155570, UP},   {-1, -1}};
struct slider_row row_2_a_u_d[] = {{155546, DOWN}, {155571, UP},   {155555, DOWN}, {155556, DOWN},
                                   {155565, DOWN}, {155566, DOWN}, {155580, UP},   {155581, UP},
                                   {155590, UP},   {155591, UP},   {-1, -1}};
struct slider_row row_2_b_u_d[] = {{155547, DOWN}, {155554, DOWN}, {155557, DOWN}, {155564, DOWN},
                                   {155567, DOWN}, {155572, UP},   {155569, UP},   {155582, UP},
                                   {155589, UP},   {155592, UP},   {-1, -1}};
struct slider_row row_2_c_u_d[] = {{155548, DOWN}, {155553, DOWN}, {155558, DOWN}, {155563, DOWN},
                                   {155568, DOWN}, {155573, UP},   {155578, UP},   {155583, UP},
                                   {155588, UP},   {155593, UP},   {-1, -1}};
struct slider_row row_2_d_u_d[] = {{155549, DOWN}, {155552, DOWN}, {155559, DOWN}, {155562, DOWN},
                                   {155569, DOWN}, {155574, UP},   {155577, UP},   {155584, UP},
                                   {155587, UP},   {155594, UP},   {-1, -1}};
struct slider_row row_2_e_u_d[] = {{155550, DOWN}, {155551, DOWN}, {155560, DOWN}, {155561, DOWN},
                                   {155570, DOWN}, {155575, UP},   {155576, UP},   {155585, UP},
                                   {155586, UP},   {155595, UP},   {-1, -1}};
struct slider_row row_3_a_u_d[] = {{155571, DOWN}, {155596, UP},   {155580, DOWN}, {155581, DOWN},
                                   {155590, DOWN}, {155591, DOWN}, {155596, UP},   {155605, UP},
                                   {155606, UP},   {155615, UP},   {155616, UP},   {-1, -1}};
struct slider_row row_3_b_u_d[] = {{155572, DOWN}, {155579, DOWN}, {155582, DOWN}, {155589, DOWN},
                                   {155592, DOWN}, {155597, UP},   {155604, UP},   {155607, UP},
                                   {155614, UP},   {155617, UP},   {-1, -1}};
struct slider_row row_3_c_u_d[] = {{155573, DOWN}, {155578, DOWN}, {155583, DOWN}, {155588, DOWN},
                                   {155593, DOWN}, {155598, UP},   {155603, UP},   {155608, UP},
                                   {155613, UP},   {155618, UP},   {-1, -1}};
struct slider_row row_3_d_u_d[] = {{155574, DOWN}, {155577, DOWN}, {155584, DOWN}, {155587, DOWN},
                                   {155594, DOWN}, {155599, UP},   {155602, UP},   {155609, UP},
                                   {155612, UP},   {155619, UP},   {-1, -1}};
struct slider_row row_3_e_u_d[] = {{155575, DOWN}, {155576, DOWN}, {155585, DOWN}, {155586, DOWN},
                                   {155595, DOWN}, {155600, UP},   {155601, UP},   {155610, UP},
                                   {155611, UP},   {155620, UP},   {-1, -1}};
struct slider_row row_4_a_u_d[] = {{155596, DOWN}, {155625, UP},   {155605, DOWN}, {155606, DOWN},
                                   {155615, DOWN}, {155616, DOWN}, {155626, UP},   {155635, UP},
                                   {155636, UP},   {155645, UP},   {-1, -1}};
struct slider_row row_4_b_u_d[] = {{155597, DOWN}, {155604, DOWN}, {155607, DOWN}, {155614, DOWN},
                                   {155617, DOWN}, {155624, UP},   {155627, UP},   {155634, UP},
                                   {155637, UP},   {155644, UP},   {-1, -1}};
struct slider_row row_4_c_u_d[] = {{155598, DOWN}, {155603, DOWN}, {155608, DOWN}, {155613, DOWN},
                                   {155618, DOWN}, {155623, UP},   {155628, UP},   {155633, UP},
                                   {155638, UP},   {155643, UP},   {-1, -1}};
struct slider_row row_4_d_u_d[] = {{155599, DOWN}, {155602, DOWN}, {155609, DOWN}, {155612, DOWN},
                                   {155619, DOWN}, {155622, UP},   {155629, UP},   {155632, UP},
                                   {155639, UP},   {155642, UP},   {-1, -1}};
struct slider_row row_4_e_u_d[] = {{155600, DOWN}, {155601, DOWN}, {155610, DOWN}, {155611, DOWN},
                                   {155620, DOWN}, {155621, UP},   {155630, UP},   {155631, UP},
                                   {155640, UP},   {155641, UP},   {-1, -1}};

void open_exit(struct slider_row row)
{
  REMOVE_BIT(EXITN(row.room, row.door)->exit_info, EX_CLOSED);
  REMOVE_BIT(EXITN(row.room, row.door)->exit_info, EX_LOCKED);
  // REMOVE_BIT(EXITN(row.room, row.door)->exit_info, EX_HIDDEN3);
  SET_BIT(EXITN(row.room, row.door)->exit_info, EX_PICKPROOF);
}

void close_exit(struct slider_row row)
{
  SET_BIT(EXITN(row.room, row.door)->exit_info, EX_CLOSED);
  SET_BIT(EXITN(row.room, row.door)->exit_info, EX_LOCKED);
  // SET_BIT(EXITN(row.room, row.door)->exit_info, EX_HIDDEN3);
  SET_BIT(EXITN(row.room, row.door)->exit_info, EX_PICKPROOF);
}

static void send_to_cube(const char *echo)
{
  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next)
  {
    if (!d->character)
      continue;
    if (GET_ROOM_VNUM(d->character->in_room) < 155521)
      continue;
    if (GET_ROOM_VNUM(d->character->in_room) > 155641)
      continue;

    if (!AWAKE(d->character))
      continue;
    send_to_char(d->character, "%s", echo);
  }
}

void open_row(struct slider_row *row)
{
  open_msg = TRUE;
  while (row->room != -1)
    open_exit(*row++);
}

void close_row(struct slider_row *row)
{
  close_msg = TRUE;
  while (row->room != -1)
    close_exit(*row++);
}

void toggle_row(struct slider_row *row)
{
  if (IS_CLOSED(row->room, row->door))
    open_row(row);
  else
    close_row(row);
}

// How long average probability between polls..
#define TOG_DELAY 20
// Probability for a wall to toggle..  close to 1000 less likely.
#define TOG_FREQ 975

SPECIAL(cube_slider)
{
  if (cmd)
    return 0;

  if (!rand_number(0, TOG_DELAY))
    return 0;

  open_msg = FALSE;
  close_msg = FALSE;

  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_a_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_b_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_c_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_d_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_e_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_a_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_b_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_c_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_d_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_e_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_a_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_b_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_c_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_d_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_e_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_a_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_b_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_c_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_d_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_e_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_a_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_b_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_c_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_d_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_e_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_a_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_b_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_c_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_d_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_e_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_a_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_b_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_c_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_d_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_e_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_a_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_b_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_c_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_d_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_e_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_a_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_b_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_c_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_d_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_e_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_a_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_b_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_c_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_d_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_e_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_a_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_b_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_c_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_d_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_e_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_a_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_b_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_c_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_d_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_e_u_d);

  if (open_msg)
    send_to_cube("\tD\tLThe whole area rumbles loudly as a dividing wall slams shut.\tn\r\n");
  if (close_msg)
    send_to_cube("\tDEverything begins to shake and rumble as a dividing wall opens.\tn\r\n");

  return 1;
}

/*****************/
/* End Mad Drow */
/*****************/

/*****************/
/* Temple of Twisted Flesh (TTF) */

/*****************/

SPECIAL(ttf_monstrosity)
{
  struct char_data *vict;
  struct char_data *next_vict;
  int percent, prob;

  if (cmd || GET_POS(ch) == POS_DEAD)
    return 0;

  if (!FIGHTING(ch))
    return 0;

  if (dice(1, 10) > 2)
    return 0;

  act("\tLThe tentacled monstrosity rises up in the air and sends its full mass crashing into the "
      "floor!\tn",
      FALSE, ch, 0, 0, TO_ROOM);

  for (vict = world[ch->in_room].people; vict; vict = next_vict)
  {
    next_vict = vict->next_in_room;

    if (!aoeOK(ch, vict, -1))
      continue;

    percent = rand_number(1, 101); /* 101% is a complete failure */
    prob = GET_LEVEL(ch) / 5;
    if (percent < prob)
    {
      change_position(vict, POS_SITTING);
      WAIT_STATE(vict, 1 * PULSE_VIOLENCE);
      act("\trThe shockwave sends you crashing to the ground!\tn", FALSE, vict, 0, 0, TO_CHAR);
      act("\trThe shockwave sends \tn$n\tr crashing to the ground!\tn", FALSE, vict, 0, 0, TO_ROOM);
    }
  }
  return TRUE;
}

SPECIAL(ttf_abomination)
{
  struct char_data *vict;
  struct char_data *next_vict;
  int percent, prob;

  if (cmd || GET_POS(ch) == POS_DEAD)
    return 0;

  if (!FIGHTING(ch))
    return 0;

  if (dice(1, 16) > 2)
    return 0;

  act("\tLA gargantuan four-armed battle abomination lunges forward and swings one of his\r\n"
      "\tLenormous arms straight into your group!\tn",
      FALSE, ch, 0, 0, TO_ROOM);

  for (vict = world[ch->in_room].people; vict; vict = next_vict)
  {
    next_vict = vict->next_in_room;

    if (!aoeOK(ch, vict, -1))
      continue;

    percent = rand_number(1, 101); /* 101% is a complete failure */
    prob = GET_LEVEL(ch) / 5;
    if (percent < prob)
    {
      change_position(vict, POS_SITTING);
      WAIT_STATE(vict, 1 * PULSE_VIOLENCE);
      act("\trYou are unable to dodge the blow, and its force sends you crashing to the ground!\tn",
          FALSE, vict, 0, 0, TO_CHAR);
      act("$n \tris unable to dodge the blow, and its force sends $m crashing to the ground!\tn",
          FALSE, vict, 0, 0, TO_ROOM);
    }
  }
  return TRUE;
}

SPECIAL(ttf_rotbringer)
{
  int hp;
  struct char_data *mob;

  if (cmd || GET_POS(ch) == POS_DEAD)
    return 0;

  if (!FIGHTING(ch))
    PROC_FIRED(ch) = FALSE;
  if (!FIGHTING(ch))
    return 0;

  if (PROC_FIRED(ch))
    return FALSE;

  hp = GET_HIT(ch) * 100;
  hp /= GET_MAX_HIT(ch);
  if (hp < 40)
  {
    send_to_room(
        ch->in_room,
        "\tRThe Rot Bringer realizes the tide of the battle is turning against him, and he\tn\r\n"
        "\tRtakes a step towards the bloody basin. His face contorted in rage, he whispers\tn\r\n"
        "\tRsomething while clawing at the air over the floating bodies. Instantly, the red\tn\r\n"
        "\tRliquid starts swirling as the cadavers join together, forming a massive mound of\tn\r\n"
        "\tRmeat! A massive ball of flesh rises out of the basin, and follows its new "
        "master!\tn\r\n");

    mob = read_mobile(145193, VIRTUAL);
    char_to_room(mob, ch->in_room);
    add_follower(mob, ch);
    PROC_FIRED(ch) = TRUE;

    return TRUE;
  }
  return FALSE;
}

int ttf_path[] = {145185, 145184, 145183, 145184, 145186, 145187, 145186, 145188,
                  145189, 145188, 145186, 145187, 145186, 145184, 145185, -1};

SPECIAL(ttf_patrol)
{
  int dir = -1;
  // int next = 0;

  if (!ch)
    return 0;
  if (FIGHTING(ch))
    return 0;

  if (cmd)
    return 0;

  if (PATH_INDEX(ch) > 16 || PATH_INDEX(ch) < 0)
    PATH_INDEX(ch) = 0;

  // 8 second delay...
  if (PATH_DELAY(ch) > 0)
  {
    PATH_DELAY(ch)
    --;
    return 0;
  }
  PATH_DELAY(ch) = 8;

  PATH_INDEX(ch)
  ++;

  if (ttf_path[PATH_INDEX(ch)] == -1)
    PATH_INDEX(ch) = 0;

  dir = find_first_step(ch->in_room, real_room(ttf_path[PATH_INDEX(ch)]));
  if (dir >= 0)
    perform_move(ch, dir, 1);
  return 1;
}

/*****************/
/* End Temple of Twisted Flesh (TTF) */
/*****************/

/* put new zone procs here */
