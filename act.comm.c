/* \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
\  Part of LuminariMUD
/
\  @File:     act.comm.c
/  @Header:   act.h
\  @Created:  Unknown
/  @Usage:    Player-level communication commands.
\  @Author:   CircleMUD
/  @Credits:  All rights reserved.  See license for complete information.
\  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
/  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
\ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"
#include "improved-edit.h"
#include "dg_scripts.h"
#include "act.h"
#include "modify.h"
#include "hlquest.h"
#include "constants.h"

ACMDU(do_rsay)
{
  char type[20];
  char *arg2 = NULL;

  if (IS_ANIMAL(ch) && !IS_WILDSHAPED(ch))
  {
    send_to_char(ch, "You can't speak!\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_SILENCED))
  {
    send_to_char(ch, "You can't seem to make a sound.\r\n");
    return;
  }

  skip_spaces(&argument);

  if (!*argument)
    send_to_char(ch, "Yes, but WHAT do you want to say?\r\n");
  else
  {
    char buf[MAX_INPUT_LENGTH + 14];
    const char *msg = NULL;
    arg2 = strdup(argument); // make a copy to send to triggers b4 parse
    struct char_data *vict;

    /* TODO (Nashak):
     *   - Parse and remove any smiley faces? (not for now)
     *   - Based on smileys parsed, express emotion in message?
     */
    if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
      parse_at(argument);
    sentence_case(argument);

    if (argument[strlen(argument) - 1] == '?')
    {
      // the argument ends in a question mark, it's probably a question
      strlcpy(type, "ask", sizeof(type));
    }
    else if (argument[strlen(argument) - 1] == '!')
    {
      strlcpy(type, "exclaim", sizeof(type));
    }
    else if (argument[strlen(argument) - 1] == '.' &&
             argument[strlen(argument) - 2] == '.' &&
             argument[strlen(argument) - 3] == '.')
    {
      strlcpy(type, "mutter", sizeof(type));
    }
    else
    {
      // the argument ends something else, normal tone
      // append a period if it isn't already there
      if (argument[strlen(argument) - 1] != '.')
        strcat(argument, ".");

      strlcpy(type, "say", sizeof(type));
    }

    snprintf(buf, sizeof(buf), "\tG$n %ss in %s, '%s'\tn", type, languages[SPEAKING(ch)], argument);
    // msg = act(buf, FALSE, ch, 0, 0, TO_ROOM | DG_NO_TRIG);

    // add message to history for those who heard it
    for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
      if (vict != ch && GET_POS(vict) > POS_SLEEPING && !AFF_FLAGGED(vict, AFF_DEAF))
      {
        if (CAN_SPEAK(vict, SPEAKING(ch)))
        {
          msg = act(buf, FALSE, ch, 0, vict, TO_VICT | DG_NO_TRIG);
          add_history(vict, msg, HIST_SAY);
        }
        else
        {
          msg = act("\tG$n says something in an unfamiliar tongue.\tn", FALSE, ch, 0, vict, TO_VICT | DG_NO_TRIG);
          add_history(vict, msg, HIST_SAY);
        }
      }

    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else
    {
      snprintf(buf, sizeof(buf), "\tGYou %s in %s, '%s'\tn", type, languages[SPEAKING(ch)], argument);
      msg = act(buf, FALSE, ch, 0, 0, TO_CHAR | DG_NO_TRIG);
      add_history(ch, msg, HIST_SAY);
    }
  }

  /* DEBUG */
  // send_to_char(ch, "ARG2|%s|\r\n", arg2);
  /* end DEBUG */

  /* Trigger check. */
  speech_mtrigger(ch, arg2);
  speech_wtrigger(ch, arg2);
}

ACMDU(do_say)
{
  char type[20];
  char *arg2 = NULL;

  if (IS_ANIMAL(ch) && !IS_WILDSHAPED(ch))
  {
    send_to_char(ch, "You can't speak!\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_SILENCED))
  {
    send_to_char(ch, "You can't seem to make a sound.\r\n");
    return;
  }

  skip_spaces(&argument);

  if (!*argument)
    send_to_char(ch, "Yes, but WHAT do you want to say?\r\n");
  else
  {
    char buf[MAX_INPUT_LENGTH + 14];
    const char *msg = NULL;
    arg2 = strdup(argument); // make a copy to send to triggers b4 parse
    struct char_data *vict;

    /* TODO (Nashak):
     *   - Parse and remove any smiley faces? (not for now)
     *   - Based on smileys parsed, express emotion in message?
     */
    if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
      parse_at(argument);
    sentence_case(argument);

    if (argument[strlen(argument) - 1] == '?')
    {
      // the argument ends in a question mark, it's probably a question
      strlcpy(type, "ask", sizeof(type));
    }
    else if (argument[strlen(argument) - 1] == '!')
    {
      strlcpy(type, "exclaim", sizeof(type));
    }
    else if (argument[strlen(argument) - 1] == '.' &&
             argument[strlen(argument) - 2] == '.' &&
             argument[strlen(argument) - 3] == '.')
    {
      strlcpy(type, "mutter", sizeof(type));
    }
    else
    {
      // the argument ends something else, normal tone
      // append a period if it isn't already there
      if (argument[strlen(argument) - 1] != '.')
        strcat(argument, ".");

      strlcpy(type, "say", sizeof(type));
    }

    snprintf(buf, sizeof(buf), "\tG$n %ss, '%s'\tn", type, argument);
    msg = act(buf, FALSE, ch, 0, 0, TO_ROOM | DG_NO_TRIG);

    // add message to history for those who heard it
    for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
      if (vict != ch && GET_POS(vict) > POS_SLEEPING && !AFF_FLAGGED(vict, AFF_DEAF))
        add_history(vict, msg, HIST_SAY);

    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else
    {
      snprintf(buf, sizeof(buf), "\tGYou %s, '%s'\tn", type, argument);
      msg = act(buf, FALSE, ch, 0, 0, TO_CHAR | DG_NO_TRIG);
      add_history(ch, msg, HIST_SAY);
    }
  }

  /* DEBUG */
  // send_to_char(ch, "ARG2|%s|\r\n", arg2);
  /* end DEBUG */

  /* Trigger check. */
  speech_mtrigger(ch, arg2);
  speech_wtrigger(ch, arg2);
}

ACMDU(do_osay)
{
  char type[20];
  char *arg2 = NULL;

  skip_spaces(&argument);

  if (!*argument)
    send_to_char(ch, "Yes, but WHAT do you want to say out-of-character?\r\n");
  else
  {
    char buf[MAX_INPUT_LENGTH + 14];
    const char *msg = NULL;
    arg2 = strdup(argument); // make a copy to send to triggers b4 parse
    struct char_data *vict;

    /* TODO (Nashak):
     *   - Parse and remove any smiley faces? (not for now)
     *   - Based on smileys parsed, express emotion in message?
     */
    if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
      parse_at(argument);
    sentence_case(argument);

    if (argument[strlen(argument) - 1] == '?')
    {
      // the argument ends in a question mark, it's probably a question
      strlcpy(type, "ask", sizeof(type));
    }
    else if (argument[strlen(argument) - 1] == '!')
    {
      strlcpy(type, "exclaim", sizeof(type));
    }
    else if (argument[strlen(argument) - 1] == '.' &&
             argument[strlen(argument) - 2] == '.' &&
             argument[strlen(argument) - 3] == '.')
    {
      strlcpy(type, "mutter", sizeof(type));
    }
    else
    {
      // the argument ends something else, normal tone
      // append a period if it isn't already there
      if (argument[strlen(argument) - 1] != '.')
        strcat(argument, ".");

      strlcpy(type, "say", sizeof(type));
    }

    snprintf(buf, sizeof(buf), "\tG$n %ss out-of-character, '%s'\tn", type, argument);
    msg = act(buf, FALSE, ch, 0, 0, TO_ROOM | DG_NO_TRIG);

    // add message to history for those who heard it
    for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
      if (vict != ch && GET_POS(vict) > POS_SLEEPING && !AFF_FLAGGED(vict, AFF_DEAF))
        add_history(vict, msg, HIST_OSAY);

    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else
    {
      snprintf(buf, sizeof(buf), "\tGYou %s out-of-character, '%s'\tn", type, argument);
      msg = act(buf, FALSE, ch, 0, 0, TO_CHAR | DG_NO_TRIG);
      add_history(ch, msg, HIST_OSAY);
    }
  }

  /* DEBUG */
  // send_to_char(ch, "ARG2|%s|\r\n", arg2);
  /* end DEBUG */

  /* Trigger check. */
  speech_mtrigger(ch, arg2);
  speech_wtrigger(ch, arg2);
}

ACMDU(do_speak)
{

  skip_spaces(&argument);
  int i = 0;

  if (!*argument)
  {
    send_to_char(ch, "You can select among the following languages to speak:\r\n");
    for (i = 0; i < NUM_LANGUAGES; i++)
    {
      if (CAN_SPEAK(ch, i))
      {
        send_to_char(ch, "%s%s%s ", SPEAKING(ch) == i ? "(" : "", languages[i], SPEAKING(ch) == i ? ")" : "");
      }
    }
    send_to_char(ch, "\r\n");
    return;
  }
  if (!strcmp(argument, "all"))
  {
    send_to_char(ch, "The following languages are implemented:\r\n");
    for (i = 0; i < NUM_LANGUAGES; i++)
    {
      send_to_char(ch, "%s ", languages[i]);
    }
    send_to_char(ch, "\r\n");
    return;
  }

  for (i = 0; i < NUM_LANGUAGES; i++)
  {
    if (is_abbrev(argument, languages[i]))
    {
      if (CAN_SPEAK(ch, i))
      {
        SPEAKING(ch) = i;
        send_to_char(ch, "You begin to speak '%s'.\r\n", languages[i]);
        break;
      }
      else
      {
        send_to_char(ch, "You do not know that language.\r\n");
        break;
      }
    }
  }

  if (i >= NUM_LANGUAGES)
  {
    send_to_char(ch, "That is not a valid language.  Type 'speak all' for a full list, or 'speak' by itself to see the languages you know.\r\n");
  }
}

ACMDU(do_gsay)
{
  skip_spaces(&argument);

  if (IS_ANIMAL(ch) && !IS_WILDSHAPED(ch))
  {
    send_to_char(ch, "You can't speak!\r\n");
    return;
  }

  if (!GROUP(ch))
  {
    send_to_char(ch, "But you are not a member of a group!\r\n");
    return;
  }

  if (!*argument)
    send_to_char(ch, "Yes, but WHAT do you want to group-say?\r\n");
  else
  {
    parse_at(argument);
    sentence_case(argument);

    // append period if it's not already there
    if (argument[strlen(argument) - 1] != '.' && argument[strlen(argument) - 1] != '!' && argument[strlen(argument) - 1] != '?')
      strcat(argument, ".");

    send_to_group(ch, ch->group, "%s%s%s says, '%s'%s\r\n", CCGRN(ch, C_NRM), CCGRN(ch, C_NRM),
                  GET_NAME(ch), argument, CCNRM(ch, C_NRM));

    struct descriptor_data *d = NULL;
    char buf[MAX_STRING_LENGTH] = {'\0'};
    snprintf(buf, sizeof(buf), "%s%s%s says, '%s'%s\r\n", CCGRN(ch, C_NRM), CCGRN(ch, C_NRM),
             GET_NAME(ch), argument, CCNRM(ch, C_NRM));
    for (d = descriptor_list; d; d = d->next)
    {
      if (STATE(d) != CON_PLAYING)
        continue;
      if (!d->character)
        continue;
      if (!is_player_grouped(ch, d->character))
        continue;
      add_history(d->character, buf, HIST_GSAY);
    }

    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else
    {
      send_to_char(ch, "%sYou group-say, '%s'%s\r\n", CCGRN(ch, C_NRM), argument, CCNRM(ch, C_NRM));
    }
  }
}

static void perform_tell(struct char_data *ch, struct char_data *vict, char *arg)
{
  char buf[MAX_STRING_LENGTH] = {'\0'};
  const char *msg = NULL;

  sentence_case(arg);
  // append period if it's not already there
  if (arg[strlen(arg) - 1] != '.' && arg[strlen(arg) - 1] != '!' && arg[strlen(arg) - 1] != '?')
    strcat(arg, ".");

  snprintf(buf, sizeof(buf), "%s$n tells you, '%s'%s", CBCYN(vict, C_NRM), arg, CCNRM(vict, C_NRM));
  msg = act(buf, FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
  add_history(vict, msg, HIST_TELL);

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(ch, "%s", CONFIG_OK);
  else
  {
    snprintf(buf, sizeof(buf), "%sYou tell $N, '%s'%s", CBCYN(ch, C_NRM), arg, CCNRM(ch, C_NRM));
    msg = act(buf, FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
    add_history(ch, msg, HIST_TELL);
  }

  if (!IS_NPC(vict) && !IS_NPC(ch))
    GET_LAST_TELL(vict) = GET_IDNUM(ch);
}

static int is_tell_ok(struct char_data *ch, struct char_data *vict)
{
  if (!ch)
    log("SYSERR: is_tell_ok called with no characters");
  else if (!vict)
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (ch == vict)
    send_to_char(ch, "You try to tell yourself something.\r\n");
  else if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOTELL))
    send_to_char(ch, "You can't tell other people while you have notell on.\r\n");
  else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF) && (GET_LEVEL(ch) < LVL_STAFF))
    send_to_char(ch, "The walls seem to absorb your words.\r\n");
  else if (AFF_FLAGGED(ch, AFF_SILENCED))
    send_to_char(ch, "You can't seem to make a sound..\r\n");
  else if (!IS_NPC(vict) && !vict->desc) /* linkless */
    act("$E's linkless at the moment.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (PLR_FLAGGED(vict, PLR_WRITING))
    act("$E's writing a message right now; try again later.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if ((!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_NOTELL)) || (ROOM_FLAGGED(IN_ROOM(vict), ROOM_SOUNDPROOF) && (GET_LEVEL(ch) < LVL_STAFF)))
    act("$E can't hear you.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (AFF_FLAGGED(vict, AFF_DEAF))
    act("$E seems to be deaf!", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (IS_ANIMAL(ch))
    send_to_char(ch, "You try to speak, but suddenly realize you are just an animal!\r\n");
  else
    return (TRUE);

  return (FALSE);
}

/* Yes, do_tell probably could be combined with whisper and ask, but it is
 * called frequently, and should IMHO be kept as tight as possible. */
ACMD(do_tell)
{
  int i = 0;
  struct char_data *vict = NULL;
  char buf[MAX_INPUT_LENGTH] = {'\0'}, buf2[MAX_INPUT_LENGTH] = {'\0'};
  const char *msg = NULL;

  half_chop_c(argument, buf, sizeof(buf), buf2, sizeof(buf2));

  if (!*buf || !*buf2)
    send_to_char(ch, "Who do you wish to tell what??\r\n");
  else if (!strcmp(buf, "m-w"))
  {
#ifdef CIRCLE_WINDOWS
    /* getpid() is not portable */
    send_to_char(ch, "Sorry, that is not available in the windows port.\r\n");
#else  /* all other configurations */
    // int i;
    char word[MAX_INPUT_LENGTH] = {'\0'}, *p, *q;

    if (last_webster_teller != -1L)
    {
      if (GET_IDNUM(ch) == last_webster_teller)
      {
        send_to_char(ch, "You are still waiting for a response.\r\n");
        return;
      }
      else
      {
        send_to_char(ch, "Hold on, m-w is busy. Try again in a couple of seconds.\r\n");
        return;
      }
    }

    /* Only a-z and +/- allowed. */
    for (p = buf2, q = word; *p; p++)
      if ((LOWER(*p) <= 'z' && LOWER(*p) >= 'a') || (*p == '+') || (*p == '-'))
        *q++ = *p;

    *q = '\0';

    if (!*word)
    {
      send_to_char(ch, "Sorry, only letters and +/- are allowed characters.\r\n");
      return;
    }
    snprintf(buf, sizeof(buf), "../bin/webster %s %d &", word, (int)getpid());
    i = system(buf);
    last_webster_teller = GET_IDNUM(ch);
    send_to_char(ch, "You look up '%s' in Merriam-Webster.\r\n", word);
#endif /* platform specific part */
  }
  else if (GET_LEVEL(ch) < LVL_IMMORT && !(vict = get_player_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (GET_LEVEL(ch) >= LVL_IMMORT && !(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (is_tell_ok(ch, vict))
  {
    if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
      parse_at(buf2);
    perform_tell(ch, vict, buf2);
  }
  else
  {
    /* zusuk added this for history */
    if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
      parse_at(buf2);

    char buf3[MAX_INPUT_LENGTH] = {'\0'};
    snprintf(buf3, sizeof(buf3), "%s%s told you, '%s'%s\r\n", CBCYN(vict, C_NRM), GET_NAME(ch), buf2, CCNRM(vict, C_NRM));
    // msg = act(buf3, FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
    add_history(vict, buf3, HIST_TELL);

    char buf4[MAX_INPUT_LENGTH] = {'\0'};
    snprintf(buf4, sizeof(buf4), "%sYou tell $N, '%s'%s", CBCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
    msg = act(buf4, FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
    add_history(ch, msg, HIST_TELL);
    send_to_char(ch, "Tell failed, trying to add to their history though...\r\n");
  }
}

ACMDU(do_reply)
{
  struct char_data *tch = character_list;

  if (IS_NPC(ch))
    return;

  skip_spaces(&argument);

  if (GET_LAST_TELL(ch) == NOBODY)
    send_to_char(ch, "You have nobody to reply to!\r\n");
  else if (!*argument)
    send_to_char(ch, "What is your reply?\r\n");
  else
  {
    /* Make sure the person you're replying to is still playing by searching
     * for them.  Note, now last tell is stored as player IDnum instead of
     * a pointer, which is much better because it's safer, plus will still
     * work if someone logs out and back in again. A descriptor list based
     * search would be faster although we could not find link dead people.
     * Not that they can hear tells anyway. :) -gg 2/24/98 */
    while (tch && (IS_NPC(tch) || GET_IDNUM(tch) != GET_LAST_TELL(ch)))
      tch = tch->next;

    if (!tch)
      send_to_char(ch, "They are no longer playing.\r\n");
    else if (is_tell_ok(ch, tch))
    {
      if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
        parse_at(argument);
      perform_tell(ch, tch, argument);
    }
  }
}

ACMD(do_write)
{
  struct obj_data *paper, *pen = NULL;
  char papername[MAX_STRING_LENGTH] = {'\0'}, penname[MAX_STRING_LENGTH] = {'\0'};

  two_arguments(argument, papername, sizeof(papername), penname, sizeof(penname));

  if (!ch->desc)
    return;

  if (!*papername)
  {
    /* Nothing was delivered. */
    send_to_char(ch, "Write?  With what?  ON what?  What are you trying to do?!?\r\n");
    return;
  }
  if (*penname)
  {
    /* Nothing was delivered. */
    if (!(paper = get_obj_in_list_vis(ch, papername, NULL, ch->carrying)))
    {
      send_to_char(ch, "You have no %s.\r\n", papername);
      return;
    }
    if (!(pen = get_obj_in_list_vis(ch, penname, NULL, ch->carrying)))
    {
      send_to_char(ch, "You have no %s.\r\n", penname);
      return;
    }
  }
  else
  { /* There was one arg.. let's see what we can find. */
    if (!(paper = get_obj_in_list_vis(ch, papername, NULL, ch->carrying)))
    {
      send_to_char(ch, "There is no %s in your inventory.\r\n", papername);
      return;
    }
    if (GET_OBJ_TYPE(paper) == ITEM_PEN)
    { /* Oops, a pen. */
      pen = paper;
      paper = NULL;
    }
    else if (GET_OBJ_TYPE(paper) != ITEM_NOTE)
    {
      send_to_char(ch, "That thing has nothing to do with writing.\r\n");
      return;
    }

    /* One object was found.. now for the other one. */
    if (!GET_EQ(ch, WEAR_HOLD_1))
    {
      send_to_char(ch, "You can't write with %s %s alone.\r\n",
                   AN(papername), papername);
      return;
    }
    if (!CAN_SEE_OBJ(ch, GET_EQ(ch, WEAR_HOLD_1)))
    {
      send_to_char(ch, "The stuff in your primary hand is invisible!  Yeech!!\r\n");
      return;
    }
    if (pen)
      paper = GET_EQ(ch, WEAR_HOLD_1);
    else
      pen = GET_EQ(ch, WEAR_HOLD_1);
  }

  /* Now let's see what kind of stuff we've found. */
  if (GET_OBJ_TYPE(pen) != ITEM_PEN)
    act("$p is no good for writing with.", FALSE, ch, pen, 0, TO_CHAR);
  else if (GET_OBJ_TYPE(paper) != ITEM_NOTE)
    act("You can't write on $p.", FALSE, ch, paper, 0, TO_CHAR);
  else
  {
    char *backstr = NULL;

    /* Something on it, display it as that's in input buffer. */
    if (paper->action_description)
    {
      backstr = strdup(paper->action_description);
      send_to_char(ch, "There's something written on it already:\r\n");
      send_to_char(ch, "%s", paper->action_description);
    }

    /* We can write. */
    act("$n begins to jot down a note.", TRUE, ch, 0, 0, TO_ROOM);
    send_editor_help(ch->desc);
    string_write(ch->desc, &paper->action_description, MAX_NOTE_LENGTH, 0, backstr);
  }
}

ACMD(do_page)
{
  struct descriptor_data *d;
  struct char_data *vict;
  char buf2[MAX_INPUT_LENGTH] = {'\0'}, arg[MAX_INPUT_LENGTH] = {'\0'};

  half_chop_c(argument, arg, sizeof(arg), buf2, sizeof(buf2));

  if (IS_NPC(ch))
    send_to_char(ch, "Monsters can't page.. go away.\r\n");
  else if (!*arg)
    send_to_char(ch, "Whom do you wish to page?\r\n");
  else
  {
    char buf[MAX_STRING_LENGTH] = {'\0'};

    snprintf(buf, sizeof(buf), "\007\007*$n* %s", buf2);
    if (!str_cmp(arg, "all"))
    {
      if (GET_LEVEL(ch) > LVL_STAFF)
      {
        for (d = descriptor_list; d; d = d->next)
          if (STATE(d) == CON_PLAYING && d->character)
            act(buf, FALSE, ch, 0, d->character, TO_VICT);
      }
      else
        send_to_char(ch, "You will never be godly enough to do that!\r\n");
      return;
    }
    if ((vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
    {
      act(buf, FALSE, ch, 0, vict, TO_VICT);
      if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
        send_to_char(ch, "%s", CONFIG_OK);
      else
        act(buf, FALSE, ch, 0, vict, TO_CHAR);
    }
    else
      send_to_char(ch, "There is no such person in the game!\r\n");
  }
}

/* Generalized communication function by Fred C. Merkel (Torg). */
ACMDU(do_gen_comm)
{
  struct descriptor_data *i;
  char color_on[24];
  char buf1[MAX_INPUT_LENGTH] = {'\0'}, buf2[MAX_INPUT_LENGTH] = {'\0'};
  const char *msg = NULL;
  char buf3[MAX_INPUT_LENGTH] = {'\0'};
  bool emoting = FALSE;

  /* Array of flags which must _not_ be set in order for comm to be heard. */
  int channels[] = {
      0,
      PRF_NOSHOUT,
      PRF_NOGOSS,
      PRF_NOAUCT,
      PRF_NOGRATZ,
      PRF_NOGOSS,
      0};
  int hist_type[] = {
      HIST_HOLLER,
      HIST_SHOUT,
      HIST_GOSSIP,
      HIST_AUCTION,
      HIST_GRATS,
  };
  /* com_msgs: [0] Message if you can't perform the action because of noshout
   *           [1] name of the action
   *           [2] message if you're not on the channel
   *           [3] a color string. */
  const char *const com_msgs[][4] = {
      {"You cannot holler!!\r\n",
       "holler",
       "",
       KYEL},
      {"You cannot shout!!\r\n",
       "shout",
       "Turn off your noshout flag first!\r\n",
       KYEL},
      {"You cannot gossip!!\r\n",
       "chat",
       "You aren't even on the channel!\r\n",
       KYEL},
      {"You cannot auctalk!!\r\n",
       "auctalk",
       "You aren't even on the channel!\r\n",
       KMAG},
      {"You cannot congratulate!\r\n",
       "congrat",
       "You aren't even on the channel!\r\n",
       KGRN},
      {"You cannot gossip your emotions!\r\n",
       "gossip",
       "You aren't even on the channel!\r\n",
       KYEL}};

  if (PLR_FLAGGED(ch, PLR_NOSHOUT))
  {
    send_to_char(ch, "%s", com_msgs[subcmd][0]);
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF) && (GET_LEVEL(ch) < LVL_STAFF))
  {
    send_to_char(ch, "The walls seem to absorb your words.\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_SILENCED) && subcmd != SCMD_CHAT)
  {
    send_to_char(ch, "You are unable to make a sound.\r\n");
    return;
  }

  if (subcmd == SCMD_GEMOTE)
  {
    if (!*argument)
      send_to_char(ch, "Gemote? Yes? Gemote what?\r\n");
    else
      do_gmote(ch, argument, 0, 1);
    return;
  }

  /* Level_can_shout defined in config.c. */
  if (GET_LEVEL(ch) < CONFIG_LEVEL_CAN_SHOUT)
  {
    send_to_char(ch, "You must be at least level %d before you can %s.\r\n", CONFIG_LEVEL_CAN_SHOUT, com_msgs[subcmd][1]);
    return;
  }
  /* Make sure the char is on the channel. */
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, channels[subcmd]))
  {
    send_to_char(ch, "%s", com_msgs[subcmd][2]);
    return;
  }

  /* animals can't speak */
  if (IS_ANIMAL(ch) && !IS_WILDSHAPED(ch))
  {
    send_to_char(ch, "You can't speak!\r\n");
    return;
  }

  /* skip leading spaces */
  skip_spaces(&argument);

  /* Make sure that there is something there to say! */
  if (!*argument)
  {
    send_to_char(ch, "Yes, %s, fine, %s we must, but WHAT???\r\n", com_msgs[subcmd][1], com_msgs[subcmd][1]);
    return;
  }
  if (subcmd == SCMD_HOLLER)
  {
    if (GET_MOVE(ch) < CONFIG_HOLLER_MOVE_COST)
    {
      send_to_char(ch, "You're too exhausted to holler.\r\n");
      return;
    }
    else
      GET_MOVE(ch) -= CONFIG_HOLLER_MOVE_COST;
  }
  /* Set up the color on code. */
  strlcpy(color_on, com_msgs[subcmd][3], sizeof(color_on));

  /* First, set up strings to be given to the communicator. */
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(ch, "%s", CONFIG_OK);
  else
  {
    if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
      parse_at(argument);

    snprintf(buf1, sizeof(buf1), "%sYou %s, '%s%s'%s", COLOR_LEV(ch) >= C_CMP ? color_on : "",
             com_msgs[subcmd][1], argument, COLOR_LEV(ch) >= C_CMP ? color_on : "", CCNRM(ch, C_CMP));

    msg = act(buf1, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);
    add_history(ch, msg, hist_type[subcmd]);
  }
  if (!emoting)
  {
    snprintf(buf1, sizeof(buf1), "$n %ss, '%s'", com_msgs[subcmd][1], argument);
    snprintf(buf3, sizeof(buf3), "%s %ss, '%s'\r\n", GET_NAME(ch), com_msgs[subcmd][1], argument);
  }

  /* Now send all the strings out. */
  for (i = descriptor_list; i; i = i->next)
  {

    /* completely unacceptable connection states */
    switch (STATE(i))
    {
    case CON_CLOSE:
    case CON_GET_NAME:
    case CON_NAME_CNFRM:
    case CON_PASSWORD:
    case CON_NEWPASSWD:
    case CON_CNFPASSWD:
    case CON_QSEX:
    case CON_QCLASS:
    case CON_RMOTD:
    case CON_MENU:
    case CON_PLR_DESC:
    case CON_CHPWD_GETOLD:
    case CON_CHPWD_GETNEW:
    case CON_CHPWD_VRFY:
    case CON_DELCNF1:
    case CON_DELCNF2:
    case CON_DISCONNECT:
    case CON_GET_PROTOCOL:
    case CON_QRACE:
    case CON_QCLASS_HELP:
    case CON_QALIGN:
    case CON_QRACE_HELP:
    case CON_PLR_BG:
      continue;
    }

    /* SELF or no character associated with descriptor */
    if (i == ch->desc || !i->character)
      continue;

    /* have the channel tuned out */
    if (!IS_NPC(ch) && (PRF_FLAGGED(i->character, channels[subcmd])))
      continue;

    if (IN_ROOM(ch) == NOWHERE)
      continue;
    if (IN_ROOM(i->character) == NOWHERE)
      continue;

    /* we want history for the rest of the conditions */
    add_history(i->character, buf3, hist_type[subcmd]);

    /* states we don't want to send message to, but want history to catch */
    switch (STATE(i))
    {
    case CON_OEDIT:
    case CON_IEDIT:
    case CON_REDIT:
    case CON_ZEDIT:
    case CON_MEDIT:
    case CON_SEDIT:
    case CON_TEDIT:
    case CON_CEDIT:
    case CON_AEDIT:
    case CON_TRIGEDIT:
    case CON_HEDIT:
    case CON_QEDIT:
    case CON_PREFEDIT:
    case CON_IBTEDIT:
    case CON_CLANEDIT:
    case CON_MSGEDIT:
    case CON_STUDY:
    case CON_HLQEDIT:
    case CON_QSTATS:
      continue;
    }

    /* 'writing' such as study, olc, mud-mail, etc */
    if (!IS_NPC(ch) && PLR_FLAGGED(i->character, PLR_WRITING))
      continue;

    /* soundproof room */
    if (IN_ROOM(i->character) != NOWHERE && ROOM_FLAGGED(IN_ROOM(i->character), ROOM_SOUNDPROOF) && (GET_LEVEL(ch) < LVL_STAFF))
      continue;

    /* shout only works for people that are in the same zone and awake */
    if (subcmd == SCMD_SHOUT && ((world[IN_ROOM(ch)].zone != world[IN_ROOM(i->character)].zone) ||
                                 !AWAKE(i->character)))
      continue;

    /* deaf?  just like soundproof */
    if (AFF_FLAGGED(i->character, AFF_DEAF) && (GET_LEVEL(ch) < LVL_STAFF))
      continue;

    snprintf(buf2, sizeof(buf2), "%s%s%s", (COLOR_LEV(i->character) >= C_NRM) ? color_on : "", buf1, KNRM);
    msg = act(buf2, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);
  }
}

ACMDU(do_qcomm)
{
  if (!PRF_FLAGGED(ch, PRF_QUEST))
  {
    send_to_char(ch, "You aren't even part of the quest!\r\n");
    return;
  }
  skip_spaces(&argument);

  if (!*argument)
    send_to_char(ch, "%c%s?  Yes, fine, %s we must, but WHAT??\r\n", UPPER(*CMD_NAME), CMD_NAME + 1, CMD_NAME);
  else
  {
    char buf[MAX_STRING_LENGTH] = {'\0'};
    struct descriptor_data *i;

    if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
      parse_at(argument);

    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else if (subcmd == SCMD_QSAY)
    {
      snprintf(buf, sizeof(buf), "You quest-say, '%s'", argument);
      act(buf, FALSE, ch, 0, argument, TO_CHAR);
    }
    else
      act(argument, FALSE, ch, 0, argument, TO_CHAR);

    if (subcmd == SCMD_QSAY)
      snprintf(buf, sizeof(buf), "$n quest-says, '%s'", argument);
    else
    {
      strlcpy(buf, argument, sizeof(buf));
      mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE, "(GC) %s qechoed: %s", GET_NAME(ch), argument);
    }
    for (i = descriptor_list; i; i = i->next)
      if (STATE(i) == CON_PLAYING && i != ch->desc && PRF_FLAGGED(i->character, PRF_QUEST) &&
          !AFF_FLAGGED(i->character, AFF_DEAF))
        act(buf, 0, ch, 0, i->character, TO_VICT | TO_SLEEP);
  }
}
