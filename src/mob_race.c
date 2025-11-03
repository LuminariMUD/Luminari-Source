/**************************************************************************
 *  File: mob_race.c                                  Part of LuminariMUD *
 *  Usage: Mobile racial behavior functions                               *
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
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "act.h"
#include "fight.h"
#include "mob_utils.h"
#include "mob_race.h"

/* racial behaviour function */
void npc_racial_behave(struct char_data *ch)
{
  struct char_data *vict = NULL;
  int num_targets = 0;

  if (!can_continue(ch, TRUE))
    return;

  /* retrieve random valid target and number of targets */
  if (!(vict = npc_find_target(ch, &num_targets)))
    return;

  if (AFF_FLAGGED(ch, AFF_FEAR_AURA) && dice(1, 2) == 1)
  {
    send_to_char(ch, "You're trying to perform your fear aura.\r\n");
    do_fear_aura(ch, NULL, 0, 0);
  }

  // first figure out which race we are dealing with
  switch (GET_RACE(ch))
  {
  case RACE_TYPE_ANIMAL:
    switch (rand_number(1, 2))
    {
    case 1:
      do_rage(ch, 0, 0, 0);
    default:
      break;
    }
    break;
  case RACE_TYPE_DRAGON:

    switch (rand_number(1, 4))
    {
    case 1:
      do_tailsweep(ch, 0, 0, 0);
      break;
    case 2:
      do_breathe(ch, 0, 0, 0);
      break;
    case 3:
      do_frightful(ch, 0, 0, 0);
      break;
    default:
      break;
    }
    break;
  default:
    switch (GET_LEVEL(ch))
    {
    default:
      break;
    }
    break;
  }
}