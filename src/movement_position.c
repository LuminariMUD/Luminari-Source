/**************************************************************************
 *  File: movement_position.c                         Part of LuminariMUD *
 *  Usage: Character position change functions                            *
 *                                                                         *
 *  All rights reserved.  See license complete information.               *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"
#include "act.h"
#include "fight.h"
#include "mud_event.h"
#include "actions.h"
#include "spell_prep.h"
#include "movement_position.h"
#include "config.h"

/* Functions moved from movement.c */

bool can_stand(struct char_data *ch)
{

  if (AFF_FLAGGED(ch, AFF_PINNED))
    return FALSE;

  if (AFF_FLAGGED(ch, AFF_ENTANGLED))
    return FALSE;

  if (AFF_FLAGGED(ch, AFF_GRAPPLED))
    return FALSE;

  if (AFF_FLAGGED(ch, AFF_SLEEP))
    return FALSE;

  if (AFF_FLAGGED(ch, AFF_PARALYZED))
    return FALSE;

  if (!is_action_available(ch, atMOVE, FALSE))
    return FALSE;

  switch (GET_POS(ch))
  {
  case POS_STANDING:
  case POS_SLEEPING:
  case POS_FIGHTING:
    return FALSE;
  case POS_SITTING:
  case POS_RESTING:
  case POS_RECLINING:
  default:
    return TRUE;
  }

  /* how did we get here? */
  return FALSE;
}

/* Stand - Standing costs a move action. */
ACMD(do_stand)
{
  if (AFF_FLAGGED(ch, AFF_PINNED))
  {
    send_to_char(ch, "You can't, you are pinned! (try struggle or grapple <target>).\r\n");
    return;
  }
  switch (GET_POS(ch))
  {
  case POS_STANDING:
    send_to_char(ch, "You are already standing.\r\n");
    break;
  case POS_SITTING:
    send_to_char(ch, "You stand up.\r\n");
    act("$n clambers to $s feet.", TRUE, ch, 0, 0, TO_ROOM);
    /* Were they sitting in something? */
    char_from_furniture(ch);
    /* Will be sitting after a successful bash and may still be fighting. */
    GET_POS(ch) = FIGHTING(ch) ? POS_FIGHTING : POS_STANDING;
    USE_MOVE_ACTION(ch);

    if (FIGHTING(ch))
      attacks_of_opportunity(ch, 0);

    break;
  case POS_RESTING:
    send_to_char(ch, "You stop resting, and stand up.\r\n");
    act("$n stops resting, and clambers on $s feet.", TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_STANDING);
    /* Were they sitting in something. */
    char_from_furniture(ch);
    USE_MOVE_ACTION(ch);

    if (FIGHTING(ch))
      attacks_of_opportunity(ch, 0);

    break;
  case POS_RECLINING:
    send_to_char(ch, "You hop from prone position to standing.\r\n");
    act("$n hops from prone to standing on $s feet.", TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_STANDING);
    /* Were they sitting in something. */
    char_from_furniture(ch);
    USE_MOVE_ACTION(ch);

    if (FIGHTING(ch))
      attacks_of_opportunity(ch, 0);

    break;
  case POS_SLEEPING:
    send_to_char(ch, "You have to wake up first!\r\n");
    break;
  case POS_FIGHTING:
    send_to_char(ch, "Do you not consider fighting as standing?\r\n");
    break;
  default:
    send_to_char(ch, "You stop floating around, and put your feet on the ground.\r\n");
    act("$n stops floating around, and puts $s feet on the ground.",
        TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_STANDING);
    break;
  }
}

ACMD(do_sit)
{
  char arg[MAX_STRING_LENGTH] = {'\0'};
  struct obj_data *furniture;
  struct char_data *tempch;
  int found;

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
    found = 0;
  if (!(furniture = get_obj_in_list_vis(ch, arg, NULL, world[ch->in_room].contents)))
    found = 0;
  else
    found = 1;

  switch (GET_POS(ch))
  {
  case POS_STANDING:
    if (found == 0)
    {
      send_to_char(ch, "You sit down.\r\n");
      act("$n sits down.", FALSE, ch, 0, 0, TO_ROOM);
      change_position(ch, POS_SITTING);
    }
    else
    {
      if (GET_OBJ_TYPE(furniture) != ITEM_FURNITURE)
      {
        send_to_char(ch, "You can't sit on that!\r\n");
        return;
      }
      else if (GET_OBJ_VAL(furniture, 1) > GET_OBJ_VAL(furniture, 0))
      {
        /* Val 1 is current number sitting, 0 is max in sitting. */
        act("$p looks like it's all full.", TRUE, ch, furniture, 0, TO_CHAR);
        log("SYSERR: Furniture %d holding too many people.", GET_OBJ_VNUM(furniture));
        return;
      }
      else if (GET_OBJ_VAL(furniture, 1) == GET_OBJ_VAL(furniture, 0))
      {
        act("There is no where left to sit upon $p.", TRUE, ch, furniture, 0, TO_CHAR);
        return;
      }
      else
      {
        if (OBJ_SAT_IN_BY(furniture) == NULL)
          OBJ_SAT_IN_BY(furniture) = ch;
        for (tempch = OBJ_SAT_IN_BY(furniture); tempch != ch; tempch = NEXT_SITTING(tempch))
        {
          if (NEXT_SITTING(tempch))
            continue;
          NEXT_SITTING(tempch) = ch;
        }
        act("You sit down upon $p.", TRUE, ch, furniture, 0, TO_CHAR);
        act("$n sits down upon $p.", TRUE, ch, furniture, 0, TO_ROOM);
        SITTING(ch) = furniture;
        NEXT_SITTING(ch) = NULL;
        GET_OBJ_VAL(furniture, 1) += 1;
        change_position(ch, POS_SITTING);
      }
    }
    break;
  case POS_SITTING:
    send_to_char(ch, "You're sitting already.\r\n");
    break;
  case POS_RESTING:
    send_to_char(ch, "You stop resting, and sit up.\r\n");
    act("$n stops resting.", TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_SITTING);
    break;
  case POS_RECLINING:
    send_to_char(ch, "You shift your body from prone to sitting up.\r\n");
    act("$n shifts $s body from prone to sitting up.", TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_SITTING);
    break;
  case POS_SLEEPING:
    send_to_char(ch, "You have to wake up first.\r\n");
    break;
  case POS_FIGHTING:
    send_to_char(ch, "You drop down in a low squat!\r\n");
    change_position(ch, POS_SITTING);
    break;
  default:
    send_to_char(ch, "You stop floating around, and sit down.\r\n");
    act("$n stops floating around, and sits down.", TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_SITTING);
    break;
  }
}

ACMD(do_rest)
{

  if (affected_by_spell(ch, SKILL_RAGE))
  {
    send_to_char(ch, "Rest now? No way. PRESS ON!\r\n");
    return;
  }

  switch (GET_POS(ch))
  {
  case POS_STANDING:
    send_to_char(ch, "You sit down and rest your tired bones.\r\n");
    act("$n sits down and rests.", TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_RESTING);
    break;
  case POS_SITTING:
    send_to_char(ch, "You rest your tired bones.\r\n");
    act("$n rests.", TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_RESTING);
    break;
  case POS_RESTING:
    send_to_char(ch, "You are already resting.\r\n");
    break;
  case POS_RECLINING:
    send_to_char(ch, "You sit up slowly.\r\n");
    act("$n sits up slowly.", TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_RESTING);
    break;
  case POS_SLEEPING:
    send_to_char(ch, "You have to wake up first.\r\n");
    break;
  case POS_FIGHTING:
    send_to_char(ch, "Rest while fighting?  Are you MAD?\r\n");
    break;
  default:
    send_to_char(ch, "You stop floating around, and stop to rest your tired bones.\r\n");
    act("$n stops floating around, and rests.", FALSE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_RESTING);
    break;
  }
}

ACMD(do_recline)
{
  switch (GET_POS(ch))
  {
  case POS_STANDING:
    send_to_char(ch, "You drop down to a prone position.\r\n");
    act("$n drops down to a prone position.", TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_RECLINING);
    break;
  case POS_SITTING:
    send_to_char(ch, "You shift to a prone position.\r\n");
    act("$n shifts to a prone position.", TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_RECLINING);
    break;
  case POS_RESTING:
    send_to_char(ch, "You lie down and continue resting.\r\n");
    act("$n lays down.", TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_RECLINING);
    break;
  case POS_RECLINING:
    send_to_char(ch, "You are already reclining.\r\n");
    break;
  case POS_SLEEPING:
    send_to_char(ch, "You have to wake up first.\r\n");
    break;
  case POS_FIGHTING:
    send_to_char(ch, "You drop down to your stomach!\r\n");
    change_position(ch, POS_RECLINING);
    break;
  default:
    send_to_char(ch, "You stop floating around, and drop prone to the ground.\r\n");
    act("$n stops floating around, and drops prone to the ground.", FALSE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_RECLINING);
    break;
  }
}

ACMD(do_sleep)
{

  if (affected_by_spell(ch, SKILL_RAGE))
  {
    send_to_char(ch, "You are way too hyper for that right now!\r\n");
    return;
  }

  switch (GET_POS(ch))
  {
  case POS_STANDING:
  case POS_SITTING:
  case POS_RESTING:
  case POS_RECLINING:
    send_to_char(ch, "You go to sleep.\r\n");
    act("$n lies down and falls asleep.", TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_SLEEPING);
    break;
  case POS_SLEEPING:
    send_to_char(ch, "You are already sound asleep.\r\n");
    break;
  case POS_FIGHTING:
    send_to_char(ch, "Sleep while fighting?  Are you MAD?\r\n");
    break;
  default:
    send_to_char(ch, "You stop floating around, and lie down to sleep.\r\n");
    act("$n stops floating around, and lie down to sleep.",
        TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_SLEEPING);
    break;
  }
}

ACMD(do_wake)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict;
  int self = 0;

  one_argument(argument, arg, sizeof(arg));
  if (*arg)
  {
    if (GET_POS(ch) == POS_SLEEPING)
      send_to_char(ch, "Maybe you should wake yourself up first.\r\n");
    else if ((vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)) == NULL)
      send_to_char(ch, "%s", CONFIG_NOPERSON);
    else if (vict == ch)
      self = 1;
    else if (AWAKE(vict))
      act("$E is already awake.", FALSE, ch, 0, vict, TO_CHAR);
    else if (AFF_FLAGGED(vict, AFF_SLEEP))
      act("You can't wake $M up!", FALSE, ch, 0, vict, TO_CHAR);
    else if (GET_POS(vict) < POS_SLEEPING)
      act("$E's in pretty bad shape!", FALSE, ch, 0, vict, TO_CHAR);
    else
    {
      act("You wake $M up.", FALSE, ch, 0, vict, TO_CHAR);
      act("You are awakened by $n.", FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
      change_position(vict, POS_RECLINING);
    }
    if (!self)
      return;
  }
  if (AFF_FLAGGED(ch, AFF_SLEEP))
    send_to_char(ch, "You can't wake up!\r\n");
  else if (GET_POS(ch) > POS_SLEEPING)
    send_to_char(ch, "You are already awake...\r\n");
  else
  {
    send_to_char(ch, "You awaken and are now in a prone position.\r\n");
    act("$n awakens and is now in a prone position.", TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_RECLINING);
  }
}

int change_position(struct char_data *ch, int new_position)
{
  if (!ch)
    return 0;

  int old_position = GET_POS(ch);

  /* we will put some general checks for having your position changed */

  /* casting */
  if (char_has_mud_event(ch, eCASTING) && new_position <= POS_SITTING)
  {
    act("$n's spell is interrupted!", FALSE, ch, 0, 0,
        TO_ROOM);
    send_to_char(ch, "Your spell is aborted!\r\n");
    resetCastingData(ch);
  }

  /* preparing spells */
  if (char_has_mud_event(ch, ePREPARATION) && new_position != POS_RESTING)
  {
    act("$n's preparations are aborted!", FALSE, ch, 0, 0,
        TO_ROOM);
    send_to_char(ch, "Your preparations are aborted!\r\n");
    stop_all_preparations(ch);
  }

  /* end general checks */

  /* this is really all that is going on here :P */
  GET_POS(ch) = new_position;

  /* we don't have a significant return value yet */
  if (old_position == new_position)
    return 0;

  return 1;
}