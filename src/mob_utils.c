/**************************************************************************
 *  File: mob_utils.c                                 Part of LuminariMUD *
 *  Usage: Mobile utility functions                                       *
 *                                                                         *
 *  All rights reserved.  See license for complete information.           *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "spells.h"
#include "act.h"
#include "graph.h"
#include "fight.h"
#include "mud_event.h"
#include "modify.h"
#include "mob_memory.h"
#include "mob_utils.h"

/* External declarations */
extern const char *dirs[];

/*** UTILITY FUNCTIONS ***/

/* function to return possible targets for mobile, used for
 * npc that is fighting  */
struct char_data *npc_find_target(struct char_data *ch, int *num_targets)
{
  struct list_data *target_list = NULL;
  struct char_data *tch = NULL;

  if (!ch || IN_ROOM(ch) == NOWHERE || !FIGHTING(ch))
    return NULL;

  /* dynamic memory allocation required */
  target_list = create_list();

  /* loop through chars in room to find possible targets to build list */
  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (tch == ch)
      continue;

    if (!CAN_SEE(ch, tch))
      continue;

    if (!IS_NPC(tch) && PRF_FLAGGED(tch, PRF_NOHASSLE))
      continue;

    /* in mobile memory? */
    if (is_in_memory(ch, tch))
      add_to_list(tch, target_list);

    /* hunting target? */
    if (HUNTING(ch) == tch)
      add_to_list(tch, target_list);

    /* fighting me? */
    if (FIGHTING(tch) == ch)
      add_to_list(tch, target_list);

    /* me fighting? */
    if (FIGHTING(ch) == tch)
      add_to_list(tch, target_list);
  }

  /* did we snag anything? */
  if (target_list->iSize == 0)
  {
    free_list(target_list);
    return NULL;
  }

  /* ok should be golden, go ahead snag a random and free list */
  /* always can just return fighting target */
  tch = random_from_list(target_list);
  *num_targets = target_list->iSize; // yay pointers!

  if (target_list)
    free_list(target_list);

  if (tch) // dummy check
    return tch;
  else // backup plan
    return NULL;
}

/* a very simplified switch opponents engine */
bool npc_switch_opponents(struct char_data *ch, struct char_data *vict)
{
  // mudlog(NRM, LVL_IMMORT, TRUE, "%s trying to switch opponents!", ch->player.short_descr);
  if (!ch || !vict)
    return FALSE;

  if (!CAN_SEE(ch, vict))
    return FALSE;

  if (GET_POS(ch) <= POS_SITTING)
  {
    send_to_char(ch, "You are in no position to switch opponents!\r\n");
    return FALSE;
  }

  if (FIGHTING(ch) == vict)
  {
    send_to_char(ch, "You can't switch opponents to an opponent you are already fighting!\r\n");
    return FALSE;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE))
  {
    if (ch->next_in_room != vict && vict->next_in_room != ch)
    {
      send_to_char(ch, "You can't reach to switch opponents!\r\n");
      return FALSE;
    }
  }

  /* should be a valid opponent */
  if (FIGHTING(ch))
    stop_fighting(ch);
  send_to_char(ch, "You switch opponents!\r\n");
  act("$n switches opponents to $N!", FALSE, ch, 0, vict, TO_ROOM);
  hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);

  return TRUE;
}

/* this function will attempt to rescue
   1)  master
   2)  group member
 * returns TRUE if rescue attempt is made
 */
/* how many times will you loop group list to find rescue target cap */
#define RESCUE_LOOP 20
bool npc_rescue(struct char_data *ch)
{

  if (!ch)
    return false;

  if (ch->master && !IS_NPC(ch->master) && PRF_FLAGGED(ch->master, PRF_NO_CHARMIE_RESCUE))
    return false;

  struct char_data *victim = NULL;
  int loop_counter = 0;

  if (GET_HIT(ch) <= 1)
    return FALSE; /* too weak */

  // going to prioritize rescuing master (if it has one)
  if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && !rand_number(0, 1) &&
      (GET_MAX_HIT(ch) / GET_HIT(ch)) <= 2)
  {
    if (FIGHTING(ch->master) && ((GET_MAX_HIT(ch->master) / GET_HIT(ch->master)) <= 3))
    {
      perform_rescue(ch, ch->master);
      return TRUE;
    }
  }

  /* determine victim (someone in group, including self) */
  if (GROUP(ch) && GROUP(ch)->members->iSize && !rand_number(0, 1) &&
      (GET_MAX_HIT(ch) / GET_HIT(ch)) <= 2)
  {
    do
    {
      victim = (struct char_data *)random_from_list(GROUP(ch)->members);

      if (!victim || GET_HIT(victim) <= 1)
        continue;

      loop_counter++;

      if (loop_counter >= RESCUE_LOOP)
        break;

    } while (!victim || victim == ch || !FIGHTING(victim) ||
             ((GET_MAX_HIT(victim) / MAX(1, GET_HIT(victim))) > 3));

    if (loop_counter < RESCUE_LOOP && FIGHTING(victim))
    {
      perform_rescue(ch, victim);
      return TRUE;
    }
  }

  return FALSE;
}
#undef RESCUE_LOOP

/* function to move a mobile along a specified path (patrols) */
bool move_on_path(struct char_data *ch)
{
  int dir = -1;
  int next = 0;

  if (!ch)
    return FALSE;

  if (FIGHTING(ch))
    return FALSE;

  /* finished path */
  if (PATH_SIZE(ch) < 1)
    return FALSE;

  /* stuck in a spot for a moment */
  if (PATH_DELAY(ch) > 0)
  {
    PATH_DELAY(ch)
    --;
    return FALSE;
  }

  send_to_char(ch, "OK, I am in room %d (%d)...  ", GET_ROOM_VNUM(IN_ROOM(ch)),
               IN_ROOM(ch));

  PATH_DELAY(ch) = PATH_RESET(ch);

  if (PATH_INDEX(ch) >= PATH_SIZE(ch) || PATH_INDEX(ch) < 0)
    PATH_INDEX(ch) = 0;

  next = GET_PATH(ch, PATH_INDEX(ch));

  send_to_char(ch, "PATH:  Path-Index:  %d, Next (get-path vnum):  %d (%d).\r\n",
               PATH_INDEX(ch), next, real_room(next));

  dir = find_first_step(IN_ROOM(ch), real_room(next));

  if (EXIT(ch, dir)->to_room != real_room(next))
  {
    send_to_char(ch, "Hrm, it appears I am off-path...\r\n");
    send_to_char(ch, "I want to go %s, which is room %d, but I need to get to"
                     " room %d..\r\n",
                 dirs[dir], GET_ROOM_VNUM(EXIT(ch, dir)->to_room),
                 next);
  }
  else
  {
    send_to_char(ch, "... it looks like my path is perfect so far!\r\n");
  }

  switch (dir)
  {
  case BFS_ERROR:
    send_to_char(ch, "Hmm.. something seems to be seriously wrong.\r\n");
    log("PATH ERROR: Mob %s, in room %d, trying to get to %d", GET_NAME(ch), world[IN_ROOM(ch)].number, next);
    break;
  case BFS_ALREADY_THERE:
    send_to_char(ch, "I seem to be in the right room already!\r\n");
    break;
  case BFS_NO_PATH:
    send_to_char(ch, "I can't sense a trail to %d (%d) from here.\r\n",
                 next, real_room(next));
    // log("NO PATH: Mob %s, in room %d, trying to get to %d", GET_NAME(ch), world[IN_ROOM(ch)].number, next);
    break;
  default: /* Success! */
    send_to_char(ch, "I sense a trail %s from here!\r\n", dirs[dir]);
    perform_move(ch, dir, 1);
    break;
  }

  PATH_INDEX(ch)
  ++;

  return TRUE;
}

/* mobile echo system, from homeland ported by nashak */
void mobile_echos(struct char_data *ch)
{
  char *echo = NULL;
  struct descriptor_data *d = NULL;

  if (!ECHO_COUNT(ch))
    return;
  if (!ECHO_ENTRIES(ch))
    return;

  if (rand_number(1, 75) > (ECHO_FREQ(ch) / 4))
    return;

  if (CURRENT_ECHO(ch) > ECHO_COUNT(ch)) /* dummy check */
    CURRENT_ECHO(ch) = 0;

  if (ECHO_SEQUENTIAL(ch))
  {
    echo = ECHO_ENTRIES(ch)[CURRENT_ECHO(ch)++];
    if (CURRENT_ECHO(ch) >= ECHO_COUNT(ch))
      CURRENT_ECHO(ch) = 0;
  }
  else
  {
    echo = ECHO_ENTRIES(ch)[rand_number(0, ECHO_COUNT(ch) - 1)];
  }

  if (!echo)
    return;

  parse_at(echo);

  if (ECHO_IS_ZONE(ch))
  {
    for (d = descriptor_list; d; d = d->next)
    {
      if (!d->character)
        continue;
      if (d->character->in_room == NOWHERE || ch->in_room == NOWHERE)
        continue;
      if (world[d->character->in_room].zone != world[ch->in_room].zone)
        continue;
      if (!AWAKE(d->character))
        continue;

      if (!PLR_FLAGGED(d->character, PLR_WRITING))
        send_to_char(d->character, "%s\r\n", echo);
    }
  }
  else
  {
    act(echo, FALSE, ch, 0, 0, TO_ROOM);
  }
}

/* function encapsulating conditions that will stop the mobile from
 acting */
int can_continue(struct char_data *ch, bool fighting)
{
  // dummy checks
  if (fighting && (!FIGHTING(ch) || IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))))
  {
    stop_fighting(ch);
    return 0;
  }
  if (GET_MOB_WAIT(ch) > 0)
    return 0;
  if (HAS_WAIT(ch))
    return 0;
  if (GET_POS(ch) <= POS_SITTING)
    return 0;
  if (IS_CASTING(ch))
    return 0;
  if (GET_HIT(ch) <= 0)
    return 0;
  /* almost finished victims, they will stop using these skills -zusuk */
  if (FIGHTING(ch) && GET_HIT(FIGHTING(ch)) <= 5)
    return 0;
  return 1;
}