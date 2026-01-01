/**
 * @file introduce.c
 * @author Zusuk
 * @brief Introduction system - allows players to introduce themselves to each other
 *
 * This file contains the implementation of the introduction system, which allows
 * players to introduce themselves to other players. When CONFIG_USE_INTRO_SYSTEM
 * is enabled, players will see short descriptions instead of names until they've
 * been introduced.
 *
 * Part of the LuminariMUD project.
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"
#include "constants.h"
#include "act.h"
#include "mud_event.h"

/**
 * Check if one character knows another through introduction
 *
 * @param ch The character checking
 * @param vict The character being checked
 * @return TRUE if ch knows vict, FALSE otherwise
 */
bool knows_character(struct char_data *ch, struct char_data *vict)
{
  int i;

  /* NPCs and immortals always know everyone */
  if (IS_NPC(ch) || IS_NPC(vict))
    return TRUE;

  if (GET_LEVEL(ch) >= LVL_IMMORT || GET_LEVEL(vict) >= LVL_IMMORT)
    return TRUE;

  /* If intro system is disabled, everyone knows everyone */
  if (!CONFIG_USE_INTRO_SYSTEM)
    return TRUE;

  /* You always know yourself */
  if (ch == vict)
    return TRUE;

  /* Check if victim's name is in ch's introduction list */
  for (i = 0; i < MAX_INTROS; i++)
  {
    if (ch->player_specials->saved.intro_list[i] == NULL)
      break; /* End of list */

    if (!str_cmp(ch->player_specials->saved.intro_list[i], GET_NAME(vict)))
      return TRUE;
  }

  return FALSE;
}

/**
 * Add a character to another's introduction list
 *
 * @param ch The character who is learning the name
 * @param vict The character being learned
 * @return TRUE if successfully added, FALSE if list is full or already known
 */
bool add_introduction(struct char_data *ch, struct char_data *vict)
{
  int i;

  /* Don't add NPCs or immortals to intro lists */
  if (IS_NPC(ch) || IS_NPC(vict))
    return FALSE;

  /* You always know yourself */
  if (ch == vict)
    return FALSE;

  /* Check if already in list */
  if (knows_character(ch, vict))
    return FALSE;

  /* Find first empty slot and add */
  for (i = 0; i < MAX_INTROS; i++)
  {
    if (ch->player_specials->saved.intro_list[i] == NULL)
    {
      ch->player_specials->saved.intro_list[i] = strdup(GET_NAME(vict));
      return TRUE;
    }
  }

  /* List is full */
  return FALSE;
}

/**
 * The introduce command
 *
 * Usage: introduce <person>  - Introduce yourself to someone
 *        introduce list      - List people who have introduced themselves to you
 */
ACMD(do_introduce)
{
  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int i, j, count = 0;
  bool found = FALSE;
  const char *known_names[MAX_INTROS];
  char name_buf[MAX_NAME_LENGTH + 1];

  /* Check if introduction system is enabled */
  if (!CONFIG_USE_INTRO_SYSTEM)
  {
    send_to_char(ch, "The introduction system is not enabled on this MUD.\r\n");
    return;
  }

  two_arguments(argument, arg, sizeof(arg), arg2, sizeof(arg2));

  if (!*arg)
  {
    send_to_char(ch, "Introduce yourself to whom?\r\n"
                     "Usage: introduce <person>\r\n"
                     "       introduce list\r\n"
                     "       introduce forget <person>\r\n");
    return;
  }

  /* Check if they want to forget someone */
  if (is_abbrev(arg, "forget"))
  {
    if (!*arg2)
    {
      send_to_char(ch, "Forget whom?\r\n"
                       "Usage: introduce forget <person>\r\n");
      return;
    }

    /* Search for the name in the introduction list */
    for (i = 0; i < MAX_INTROS; i++)
    {
      if (ch->player_specials->saved.intro_list[i] == NULL)
        break; /* End of list */

      if (!str_cmp(ch->player_specials->saved.intro_list[i], arg2))
      {
        /* Found the name - remove it */
        found = TRUE;

        /* Free the memory for this name */
        free(ch->player_specials->saved.intro_list[i]);

        /* Shift all subsequent names down one slot */
        for (j = i; j < MAX_INTROS - 1; j++)
        {
          ch->player_specials->saved.intro_list[j] = ch->player_specials->saved.intro_list[j + 1];
          if (ch->player_specials->saved.intro_list[j] == NULL)
            break;
        }

        /* Ensure the last slot is NULL */
        ch->player_specials->saved.intro_list[MAX_INTROS - 1] = NULL;

        send_to_char(ch, "You have forgotten %s.\r\n", CAP(arg2));
        return;
      }
    }

    if (!found)
    {
      send_to_char(ch, "You don't know anyone by that name.\r\n");
    }
    return;
  }

  /* Check if they want to list known people */
  if (is_abbrev(arg, "list"))
  {
    send_to_char(ch, "\tYPeople who have introduced themselves to you:\tn\r\n");
    send_to_char(ch, "\tc----------------------------------------\tn\r\n");

    /* Build array of known names */
    for (i = 0; i < MAX_INTROS; i++)
    {
      if (ch->player_specials->saved.intro_list[i] == NULL)
        break; /* End of list */

      /* Copy and capitalize the name */
      strncpy(name_buf, ch->player_specials->saved.intro_list[i], sizeof(name_buf) - 1);
      name_buf[sizeof(name_buf) - 1] = '\0';
      CAP(name_buf);
      known_names[count++] = strdup(name_buf);
    }

    if (count == 0)
    {
      send_to_char(ch, "  No one has introduced themselves to you yet.\r\n");
    }
    else
    {
      /* Display in 3 columns */
      column_list(ch, 3, known_names, count, FALSE);

      /* Free the duplicated strings */
      for (i = 0; i < count; i++)
        free((void *)known_names[i]);
    }
    return;
  }

  /* Find the target */
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
  {
    send_to_char(ch, "%s", CONFIG_NOPERSON);
    return;
  }

  /* Can't introduce to yourself */
  if (vict == ch)
  {
    send_to_char(ch, "You already know who you are.\r\n");
    return;
  }

  /* Can't introduce to NPCs */
  if (IS_NPC(vict))
  {
    send_to_char(ch, "There's no need to introduce yourself to %s.\r\n", show_pers(vict, ch));
    return;
  }

  /* Check if already introduced */
  if (knows_character(vict, ch))
  {
    send_to_char(ch, "%s already knows who you are.\r\n", show_pers(vict, ch));
    return;
  }

  /* Check if ch can act */
  if (char_has_mud_event(ch, eSTUNNED))
  {
    send_to_char(ch, "You are stunned and cannot act!\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_PARALYZED))
  {
    send_to_char(ch, "You are paralyzed and cannot move!\r\n");
    return;
  }

  /* Perform the introduction - add ch's name to vict's list */
  if (add_introduction(vict, ch))
  {
    send_to_char(ch, "You introduce yourself to %s.\r\n", show_pers(vict, ch));
    send_to_char(vict, "%s introduces %sself to you.\r\n", GET_NAME(ch), HSSH(ch));
    act("$n introduces $mself to $N.", FALSE, ch, 0, vict, TO_NOTVICT);
  }
  else
  {
    send_to_char(ch, "%s's introduction list is full!\r\n", show_pers(vict, ch));
  }
}
