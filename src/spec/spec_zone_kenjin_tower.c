/**************************************************************************
 *  File: spec/spec_zone_kenjin_tower.c                 Part of LuminariMUD *
 *  Usage: Tower of Kenjin zone procedures.                                *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "movement/door_state.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "magic/spells.h"
#include "act.h"
#include "spec_zone_kenjin_tower.h"

/* kenjin proc */
SPECIAL(kt_kenjin)
{
  struct affected_type af;
  struct char_data *vict = 0;
  struct char_data *tch = 0;
  int val = 0;

  if (cmd)
    return FALSE;
  if (!FIGHTING(ch))
    return FALSE;

  if (GET_POS(ch) < POS_FIGHTING)
    return FALSE;

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    if (!IS_NPC(tch) || IS_PET(tch))
    {
      if (!vict || !rand_number(0, 2))
      {
        vict = tch;
      }
    }
  }

  val = dice(1, 3);

  // turns you into stone
  if (val == 1)
  {
    act("$n\tc whips out a \tWbone-white\tn wand, and waves it in the air.\tn\r\n"
        "\tcSuddenly $e points it at \tn$N\tc, who slowly turns to stone.",
        FALSE, ch, 0, vict, TO_ROOM);
    new_affect(&af);
    af.spell = SPELL_HOLD_PERSON;
    SET_BIT_AR(af.bitvector, AFF_PARALYZED);
    af.duration = rand_number(2, 3);
    affect_join(vict, &af, TRUE, FALSE, FALSE, FALSE);
    return TRUE;
  }

  // teleports you to the bottom of the shaft.
  if (val == 2)
  {
    act("$n\tc whips out a \trfiery red\tn wand, and waves it in the air.\tn\r\n"
        "\tcSuddenly $e points it at \tn$N\tc, who fades away suddenly.",
        FALSE, ch, 0, vict, TO_ROOM);
    char_from_room(vict);
    char_to_room(vict, real_room(132908));
    look_at_room(vict, 0);
    return TRUE;
  }

  // loads a new mob in 132919 :)
  if (val == 3)
  {
    act("$n\tc whips out a \tYgolden\tn wand, and waves it in the air.\tn\r\n"
        "\tcSuddenly $e taps in the air, and you see \tLshadow\tc coalesce behind you.",
        FALSE, ch, 0, vict, TO_ROOM);
    tch = read_mobile(132902, VIRTUAL);
    if (!tch)
      return FALSE;
    char_to_room(tch, real_room(132919));
    return TRUE;
  }

  return FALSE;
}

/* from homeland */
SPECIAL(kt_twister)
{
  struct char_data *mob;
  char l_name[256];
  char s_name[256];
  struct door_state_operation operations[4] = {0};
  int direction;
  int temp;

  if (cmd)
    return FALSE;

  if (IS_NPC(ch) && !IS_PET(ch))
    return FALSE;

  for (direction = 0; direction < 4; direction++)
    door_state_begin(&operations[direction], real_room(32901), direction, false, DOMAIN_DOOR_EDIT);
  temp = world[real_room(132901)].dir_option[0]->to_room;
  world[real_room(32901)].dir_option[0]->to_room =

      world[real_room(132901)].dir_option[1]->to_room;
  world[real_room(32901)].dir_option[1]->to_room =

      world[real_room(132901)].dir_option[2]->to_room;
  world[real_room(32901)].dir_option[2]->to_room =

      world[real_room(132901)].dir_option[3]->to_room;
  world[real_room(32901)].dir_option[3]->to_room = temp;

  send_to_room(real_room(132901), "\tCThe world seems to turn.\tn\r\n");

  mob = read_mobile(132901, VIRTUAL);

  if (!mob)
  {
    for (direction = 0; direction < 4; direction++)
      door_state_finish(&operations[direction]);
    return FALSE;
  }

  char_to_room(mob, real_room(132906));

  snprintf(l_name, sizeof(l_name), "\tLThe shadow of \tw%s\tL stands here.\tn  ", GET_NAME(ch));
  snprintf(s_name, sizeof(s_name), "\tLa shadow of \tw%s\tn", GET_NAME(ch));

  mob->player.short_descr = strdup(s_name);
  mob->player.name = strdup("shadow");
  mob->player.long_descr = strdup(l_name);

  GET_LEVEL(mob) = GET_LEVEL(ch);
  GET_MAX_HIT(mob) = 1000 + GET_LEVEL(mob) * 50;
  GET_HIT(mob) = GET_MAX_HIT(mob);
  GET_CLASS(mob) = GET_CLASS(ch);

  send_to_char(ch, "You somehow feel \tWsplit\tn in half.\r\n");
  for (direction = 0; direction < 4; direction++)
    door_state_finish(&operations[direction]);
  return TRUE;
}
