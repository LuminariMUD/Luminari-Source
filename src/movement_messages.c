/**************************************************************************
 *  File: movement_messages.c                          Part of LuminariMUD *
 *  Usage: Movement message display functions.                            *
 *                                                                         *
 *  All rights reserved.  See license for complete information.           *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
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
#include "house.h"
#include "constants.h"
#include "dg_scripts.h"
#include "act.h"
#include "fight.h"
#include "spec_procs.h"
#include "mud_event.h"
#include "hlquest.h"
#include "mudlim.h"
#include "actions.h"
#include "spell_prep.h"
#include "class.h"
#include "transport.h"

/* Include movement system header */
#include "movement.h"
#include "movement_messages.h"

/* External functions */
bool can_hear_sneaking(struct char_data *observer, struct char_data *sneaker);

/**
 * Display leave messages when a character exits a room
 * Handles various scenarios: walking, sneaking, mounted, etc.
 * 
 * @param ch Character who is leaving
 * @param dir Direction they are leaving
 * @param riding TRUE if character is mounted
 * @param ridden_by TRUE if character is being ridden
 */
void display_leave_messages(struct char_data *ch, int dir, int riding, int ridden_by)
{
  struct char_data *tch = NULL, *next_tch = NULL;
  char buf2[MAX_STRING_LENGTH] = {'\0'};

  /* scenario: mounted char */
  if (riding)
  {
    /* riding, mount is -not- attempting to sneak */
    if (!IS_AFFECTED(RIDING(ch), AFF_SNEAK))
    {
      /* character is attempting to sneak (mount is not) */
      if (IS_AFFECTED(ch, AFF_SNEAK))
      {
        /* we know the player is trying to sneak, we have to do the
           sneak-check with all the observers in the room */
        for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch)
        {
          next_tch = tch->next_in_room;

          /* skip self and mount of course */
          if (tch == ch || tch == RIDING(ch))
            continue;

          /* sneak versus listen check */
          if (!can_hear_sneaking(tch, ch))
          {
            /* message: mount not sneaking, rider is sneaking */
            snprintf(buf2, sizeof(buf2), "$n leaves %s.", dirs[dir]);
            act(buf2, TRUE, RIDING(ch), 0, tch, TO_VICT);
          }
          else
          {
            /* rider detected ! */
            snprintf(buf2, sizeof(buf2), "$n rides %s %s.",
                     GET_NAME(RIDING(ch)), dirs[dir]);
            act(buf2, TRUE, ch, 0, tch, TO_VICT);
          }
        }

        /* character is -not- attempting to sneak (mount not too) */
      }
      else
      {
        snprintf(buf2, sizeof(buf2), "$n rides $N %s.", dirs[dir]);
        act(buf2, TRUE, ch, 0, RIDING(ch), TO_NOTVICT);
      }
    } /* riding, mount -is- attempting to sneak */
    else
    {
      if (!IS_AFFECTED(ch, AFF_SNEAK))
      {
        /* we know the mount (and not ch) is trying to sneak */
        for (tch = world[IN_ROOM(RIDING(ch))].people; tch; tch = next_tch)
        {
          next_tch = tch->next_in_room;

          /* skip self (mount) of course and ch */
          if (tch == RIDING(ch) || tch == ch)
            continue;

          /* sneak versus listen check */
          if (can_hear_sneaking(tch, RIDING(ch)))
          {
            /* mount detected! */
            snprintf(buf2, sizeof(buf2), "$n rides %s %s.",
                     GET_NAME(RIDING(ch)), dirs[dir]);
            act(buf2, TRUE, ch, 0, tch, TO_VICT);
          } /* if we pass this check, the rider/mount are both sneaking */
        }
      } /* ch is still trying to sneak (mount too) */
      else
      {
        /* we know the mount (and ch) is trying to sneak */
        for (tch = world[IN_ROOM(RIDING(ch))].people; tch; tch = next_tch)
        {
          next_tch = tch->next_in_room;

          /* skip self (mount) of course, skipping ch too */
          if (tch == RIDING(ch) || tch == ch)
            continue;

          /* sneak versus listen check */
          if (!can_hear_sneaking(tch, RIDING(ch)))
          {
            /* mount success! "package" is sneaking */
          }
          else if (!can_hear_sneaking(tch, ch))
          {
            /* mount failed, player succeeded */
            /* message: mount not sneaking, rider is sneaking */
            snprintf(buf2, sizeof(buf2), "$n leaves %s.", dirs[dir]);
            act(buf2, TRUE, RIDING(ch), 0, tch, TO_VICT);
          }
          else
          {
            /* mount failed, player failed */
            snprintf(buf2, sizeof(buf2), "$n rides %s %s.",
                     GET_NAME(RIDING(ch)), dirs[dir]);
            act(buf2, TRUE, ch, 0, tch, TO_VICT);
          }
        }
      }
    }
    /* message to self */
    send_to_char(ch, "You ride %s %s.\r\n", GET_NAME(RIDING(ch)), dirs[dir]);
    /* message to mount */
    send_to_char(RIDING(ch), "You carry %s %s.\r\n",
                 GET_NAME(ch), dirs[dir]);
  } /* end: mounted char */

  /* scenario: char is mount */
  else if (ridden_by)
  {
    /* ridden and mount-char is -not- attempting to sneak */
    if (!IS_AFFECTED(ch, AFF_SNEAK))
    {
      /* char's rider is attempting to sneak (mount-char is not) */
      if (IS_AFFECTED(RIDDEN_BY(ch), AFF_SNEAK))
      {
        /* we know the rider is trying to sneak */
        for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch)
        {
          next_tch = tch->next_in_room;

          /* skip self and rider of course */
          if (tch == ch || tch == RIDDEN_BY(ch))
            continue;

          /* sneak versus listen check */
          if (!can_hear_sneaking(tch, RIDDEN_BY(ch)))
          {
            /* message: mount not sneaking, rider is sneaking */
            snprintf(buf2, sizeof(buf2), "$n leaves %s.", dirs[dir]);
            act(buf2, TRUE, ch, 0, tch, TO_VICT);
          }
          else
          {
            /* rider detected ! */
            snprintf(buf2, sizeof(buf2), "$n rides %s %s.",
                     GET_NAME(ch), dirs[dir]);
            act(buf2, TRUE, RIDDEN_BY(ch), 0, tch, TO_VICT);
          }
        }

        /* rider is -not- attempting to sneak (mount-char not too) */
      }
      else
      {
        snprintf(buf2, sizeof(buf2), "$n rides $N %s.", dirs[dir]);
        act(buf2, TRUE, RIDDEN_BY(ch), 0, ch, TO_NOTVICT);
      }
    } /* ridden and mount-char -is- attempting to sneak */
    else
    {
      /* both are attempt to sneak */
      if (IS_AFFECTED(RIDDEN_BY(ch), AFF_SNEAK))
      {
        /* we know the mount and rider is trying to sneak */
        for (tch = world[IN_ROOM(RIDDEN_BY(ch))].people; tch; tch = next_tch)
        {
          next_tch = tch->next_in_room;

          /* skip rider of course, skipping mount-ch too */
          if (tch == RIDDEN_BY(ch) || tch == ch)
            continue;

          /* sneak versus listen check */
          if (!can_hear_sneaking(tch, ch))
          {
            /* mount success! "package" is sneaking */
          }
          else if (!can_hear_sneaking(tch, RIDDEN_BY(ch)))
          {
            /* mount failed, rider succeeded */
            /* message: mount not sneaking, rider is sneaking */
            snprintf(buf2, sizeof(buf2), "$n leaves %s.", dirs[dir]);
            act(buf2, TRUE, RIDDEN_BY(ch), 0, tch, TO_VICT);
          }
          else
          {
            /* mount failed, rider failed */
            /* 3.23.18 Ornir Bugfix. */
            snprintf(buf2, sizeof(buf2), "$n rides %s %s.",
                     GET_NAME(ch), dirs[dir]);
            act(buf2, TRUE, RIDDEN_BY(ch), 0, tch, TO_VICT);
          }
        }

        /* ridden and mount-char -is- attempt to sneak, rider -not- */
      }
      else
      {
        /* we know the mount (rider no) is trying to sneak */
        for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch)
        {
          next_tch = tch->next_in_room;

          /* skip self (mount) and rider */
          if (tch == RIDDEN_BY(ch) || tch == ch)
            continue;

          /* sneak versus listen check */
          if (can_hear_sneaking(tch, ch))
          {
            /* mount detected! */
            snprintf(buf2, sizeof(buf2), "$n rides %s %s.",
                     GET_NAME(ch), dirs[dir]);
            act(buf2, TRUE, ch, 0, tch, TO_VICT);
          } /* if we pass this check, the rider/mount are both sneaking */
        }
      }
    }
    /* message to self */
    send_to_char(ch, "You carry %s %s.\r\n",
                 GET_NAME(RIDDEN_BY(ch)), dirs[dir]);
    /* message to rider */
    send_to_char(RIDDEN_BY(ch), "You are carried %s by %s.\r\n",
                 dirs[dir], GET_NAME(ch));
  } /* end char is mounted */

  /* ch is on foot */
  else if (IS_AFFECTED(ch, AFF_SNEAK))
  {
    /* sneak attempt vs the room content */
    for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch)
    {
      next_tch = tch->next_in_room;

      /* skip self */
      if (tch == ch)
        continue;

      /* sneak versus listen check */
      if (can_hear_sneaking(tch, ch))
      {
        /* detected! */
        if (IS_NPC(ch) && (ch->player.walkout != NULL))
        {
          /* if they have a walk-out message, display that instead */
          snprintf(buf2, sizeof(buf2), "%s %s.", ch->player.walkout, dirs[dir]);
          act(buf2, TRUE, ch, 0, tch, TO_VICT);
        }
        else
        {
          snprintf(buf2, sizeof(buf2), "$n leaves %s.", dirs[dir]);
          act(buf2, TRUE, ch, 0, tch, TO_VICT);
        }
      } /* if we pass this check, we are sneaking */
    }
    /* message to self */
    send_to_char(ch, "You sneak %s.\r\n", dirs[dir]);
  } /* not attempting to sneak */
  else if (!IS_AFFECTED(ch, AFF_SNEAK))
  {
    if (IS_NPC(ch) && (ch->player.walkout != NULL))
    {
      /* if they have a walk-out message, display that instead */
      snprintf(buf2, sizeof(buf2), "%s %s.", ch->player.walkout, dirs[dir]);
      act(buf2, TRUE, ch, 0, 0, TO_ROOM);
    }
    else
    {
      snprintf(buf2, sizeof(buf2), "$n leaves %s.", dirs[dir]);
      act(buf2, TRUE, ch, 0, 0, TO_ROOM);
    }
    /* message to self */
    send_to_char(ch, "You leave %s.\r\n", dirs[dir]);
  }
}

/**
 * Display enter messages when a character arrives in a room
 * Handles various scenarios: walking, sneaking, mounted, etc.
 * 
 * @param ch Character who is entering
 * @param dir Direction they came from
 * @param riding TRUE if character is mounted
 * @param ridden_by TRUE if character is being ridden
 */
void display_enter_messages(struct char_data *ch, int dir, int riding, int ridden_by)
{
  struct char_data *tch = NULL, *next_tch = NULL;
  char buf2[MAX_STRING_LENGTH] = {'\0'};

  if (!riding && !ridden_by)
  {
    /* simplest case, not riding or being ridden-by */
    for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch)
    {
      next_tch = tch->next_in_room;

      /* skip self */
      if (tch == ch)
        continue;

      /* sneak versus listen check */
      if (can_hear_sneaking(tch, ch))
      {
        /* failed sneak attempt (if valid) */
        if (IS_NPC(ch) && ch->player.walkin)
        {
          snprintf(buf2, sizeof(buf2), "%s %s%s.", ch->player.walkin,
                   ((dir == UP || dir == DOWN) ? "" : "the "),
                   (dir == UP ? "below" : dir == DOWN ? "above"
                                                      : dirs[rev_dir[dir]]));
        }
        else
        {
          snprintf(buf2, sizeof(buf2), "$n arrives from %s%s.",
                   ((dir == UP || dir == DOWN) ? "" : "the "),
                   (dir == UP ? "below" : dir == DOWN ? "above"
                                                      : dirs[rev_dir[dir]]));
        }
        act(buf2, TRUE, ch, 0, tch, TO_VICT);
      }
    }
  }
  else if (riding)
  {
    for (tch = world[IN_ROOM(RIDING(ch))].people; tch; tch = next_tch)
    {
      next_tch = tch->next_in_room;

      /* skip rider of course, and mount */
      if (tch == RIDING(ch) || tch == ch)
        continue;

      /* sneak versus listen check */
      if (!can_hear_sneaking(tch, RIDING(ch)))
      {
        /* mount success! "package" is sneaking */
      }
      else if (!can_hear_sneaking(tch, ch))
      {
        /* mount failed, rider succeeded */
        /* message: mount not sneaking, rider is sneaking */
        snprintf(buf2, sizeof(buf2), "$n arrives from %s%s.",
                 ((dir == UP || dir == DOWN) ? "" : "the "),
                 (dir == UP ? "below" : dir == DOWN ? "above"
                                                    : dirs[rev_dir[dir]]));
        act(buf2, TRUE, RIDING(ch), 0, tch, TO_VICT);
      }
      else
      {
        /* mount failed, rider failed */
        snprintf(buf2, sizeof(buf2), "$n arrives from %s%s, riding %s.",
                 ((dir == UP || dir == DOWN) ? "" : "the "),
                 (dir == UP ? "below" : dir == DOWN ? "above"
                                                    : dirs[rev_dir[dir]]),
                 GET_NAME(RIDING(ch)));
        act(buf2, TRUE, ch, 0, tch, TO_VICT);
      }
    }
  }
  else if (ridden_by)
  {
    for (tch = world[IN_ROOM(RIDDEN_BY(ch))].people; tch; tch = next_tch)
    {
      next_tch = tch->next_in_room;

      /* skip rider of course, and mount */
      if (tch == RIDDEN_BY(ch) || tch == ch)
        continue;

      /* sneak versus listen check, remember ch = mount right now */
      if (!can_hear_sneaking(tch, ch))
      {
        /* mount success! "package" is sneaking */
      }
      else if (!can_hear_sneaking(tch, RIDDEN_BY(ch)))
      {
        /* mount failed, rider succeeded */
        /* message: mount not sneaking, rider is sneaking */
        snprintf(buf2, sizeof(buf2), "$n arrives from %s%s.",
                 ((dir == UP || dir == DOWN) ? "" : "the "),
                 (dir == UP ? "below" : dir == DOWN ? "above"
                                                    : dirs[rev_dir[dir]]));
        act(buf2, TRUE, ch, 0, tch, TO_VICT);
      }
      else
      {
        /* mount failed, rider failed */
        snprintf(buf2, sizeof(buf2), "$n arrives from %s%s, ridden by %s.",
                 ((dir == UP || dir == DOWN) ? "" : "the "),
                 (dir == UP ? "below" : dir == DOWN ? "above"
                                                    : dirs[rev_dir[dir]]),
                 GET_NAME(RIDDEN_BY(ch)));
        act(buf2, TRUE, RIDDEN_BY(ch), 0, tch, TO_VICT);
      }
    }
  }
}