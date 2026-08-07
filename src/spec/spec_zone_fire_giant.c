/**************************************************************************
 *  File: spec/spec_zone_fire_giant.c                  Part of LuminariMUD *
 *  Usage: Fire Giant invasion procedures.                                *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
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
#include "act.h"
#include "spec_zone_fire_giant.h"
#include "combat/fight.h"
#include "actions.h"

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
