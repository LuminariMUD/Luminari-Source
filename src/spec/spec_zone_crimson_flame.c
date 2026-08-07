/**************************************************************************
 *  File: spec/spec_zone_crimson_flame.c               Part of LuminariMUD *
 *  Usage: Crimson Flame zone procedures.                                 *
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
#include "magic/spells.h"
#include "act.h"
#include "spec_zone_crimson_flame.h"
#include "combat/fight.h"
#include "graph.h"

#define CF_VNUM 1060

/* just made this to help facilitate switching of zone vnums if needed */
int cf_converter(int value)
{
  return (CF_VNUM * 100) + value;
}

/* this proc will cause the training master to sick all his minions to track
   whoever he is fighting - will fire one time and that's it */
SPECIAL(cf_trainingmaster)
{
  struct char_data *i = NULL;
  struct char_data *enemy = NULL;

  if (cmd || GET_POS(ch) == POS_DEAD)
    return 0;

  enemy = FIGHTING(ch);

  if (!enemy)
    PROC_FIRED(ch) = FALSE;

  if (FIGHTING(ch) && !ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
  {
    if (enemy->master && enemy->master->in_room == enemy->in_room)
      enemy = enemy->master;
    act("$n waves $s hand slightly.", FALSE, ch, 0, 0, TO_ROOM);
    for (i = character_list; i; i = i->next)
    {
      if (!FIGHTING(i) && IS_NPC(i) &&
          (GET_MOB_VNUM(i) == (mob_vnum)cf_converter(32) ||
           GET_MOB_VNUM(i) == (mob_vnum)cf_converter(33) ||
           GET_MOB_VNUM(i) == (mob_vnum)cf_converter(34) ||
           GET_MOB_VNUM(i) == (mob_vnum)cf_converter(35) ||
           GET_MOB_VNUM(i) == (mob_vnum)cf_converter(36) ||
           GET_MOB_VNUM(i) == (mob_vnum)cf_converter(37) ||
           GET_MOB_VNUM(i) == (mob_vnum)cf_converter(38) ||
           GET_MOB_VNUM(i) == (mob_vnum)cf_converter(39)) &&
          ch != i)
      {
        if (ch->in_room != i->in_room)
        {
          HUNTING(i) = enemy;
          hunt_victim(i);
        }
        else
          hit(ch, enemy, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      }
    } // for loop

    PROC_FIRED(ch) = TRUE;
    return 1;
  }

  return 0;
}

/* this is lord alathar's proc to summon his bodyguards to him */
SPECIAL(cf_alathar)
{
  struct char_data *mob = NULL;
  int i = 0;

  if (cmd || GET_POS(ch) == POS_DEAD)
    return 0;

  if (!FIGHTING(ch))
    return 0;

  if (PROC_FIRED(ch))
    return FALSE;

  send_to_room(IN_ROOM(ch),
               "\tDLord Alathar thrusts his hands out and makes a sweeping gesture\tn\r\n"
               "\tDwhile uttering words in an unknown tongue. The \tRcrimson colored\tn\r\n"
               "\tRflames \tDin the huge black brazier blaze even brighter than before\tn\r\n"
               "\tDproducing a great and \tRblinding red radiance \tDthroughout the\tn\r\n"
               "\tDarea. Dark shadows are summoned and swirl into view then swarm to\tn\r\n"
               "\tDLord Alathar's aid.\tn");

  if (!GROUP(ch))
    create_group(ch);

  for (i = 50; i < 57; i++)
  {
    mob = read_mobile(cf_converter(i), VIRTUAL);
    if (mob)
    {
      char_to_room(mob, ch->in_room);
      add_follower(mob, ch);
      if (!GROUP(mob))
        join_group(mob, GROUP(ch));
    }
  }

  PROC_FIRED(ch) = TRUE;

  return TRUE;
}

#undef CF_VNUM
