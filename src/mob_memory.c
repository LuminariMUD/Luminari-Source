/**************************************************************************
 *  File: mob_memory.c                                Part of LuminariMUD *
 *  Usage: Mobile memory management functions                             *
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
#include "handler.h"
#include "shop.h"
#include "mob_memory.h"

/* Mob Memory Routines */

/* checks if vict is in memory of ch */
bool is_in_memory(struct char_data *ch, struct char_data *vict)
{
  memory_rec *names = NULL;

  if (!IS_NPC(ch))
    return FALSE;

  if (!MOB_FLAGGED(ch, MOB_MEMORY))
    return FALSE;

  if (!MEMORY(ch))
    return FALSE;

  if (IS_NPC(vict) || !CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE))
    return FALSE;

  /* at this stage vict should be a valid target to check, now loop
   through ch's memory to cross-reference it */
  for (names = MEMORY(ch); names; names = names->next)
  {
    if (names->id != GET_IDNUM(vict))
      continue;
    else
      return TRUE; // bingo, found a match
  }

  return FALSE; // nothing
}

/* make ch remember victim */
void remember(struct char_data *ch, struct char_data *victim)
{
  memory_rec *tmp = NULL;
  bool present = FALSE;

  if (!IS_NPC(ch) || IS_NPC(victim) || PRF_FLAGGED(victim, PRF_NOHASSLE))
    return;

  if (MOB_FLAGGED(ch, MOB_NOKILL))
  {
    send_to_char(ch, "You are a protected mob, it doesn't make sense for you to remember your victim!\r\n");
    return;
  }

  if (victim && !ok_damage_shopkeeper(victim, ch))
  {
    send_to_char(ch, "You are a shopkeeper (that can't be damaged), it doesn't make sense for you to remember that target!\r\n");
    return;
  }

  /*
  if (victim && !is_mission_mob(victim, ch))
  {
    send_to_char(ch, "You are a mission mob, it doesn't make sense for you to remember that target!\r\n");
    return;
  }
  */

  for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
    if (tmp->id == GET_IDNUM(victim))
      present = TRUE;

  if (!present)
  {
    CREATE(tmp, memory_rec, 1);
    tmp->next = MEMORY(ch);
    tmp->id = GET_IDNUM(victim);
    MEMORY(ch) = tmp;
  }
}

/* make ch forget victim */
void forget(struct char_data *ch, struct char_data *victim)
{
  memory_rec *curr = NULL, *prev = NULL;

  if (!(curr = MEMORY(ch)))
    return;

  while (curr && curr->id != GET_IDNUM(victim))
  {
    prev = curr;
    curr = curr->next;
  }

  if (!curr)
    return; /* person wasn't there at all. */

  if (curr == MEMORY(ch))
    MEMORY(ch) = curr->next;
  else
    prev->next = curr->next;

  free(curr);
}

/* erase ch's memory completely, also freeing memory */
void clearMemory(struct char_data *ch)
{
  memory_rec *curr = NULL, *next = NULL;

  curr = MEMORY(ch);

  while (curr)
  {
    next = curr->next;
    free(curr);
    curr = next;
  }

  MEMORY(ch) = NULL;
}

/* end memory routines */