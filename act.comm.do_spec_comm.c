#include "act.h"     /* for SCMD_WHISPER */
#include "comm.h"    /* for send_to_char */
#include "handler.h" /* for get_char_vis */
#include "hlquest.h" /* for quest_ask */
#include "utils.h"   /* for ACMD */

ACMD(do_spec_comm)
{
  char buf[MAX_INPUT_LENGTH] = {'\0'}, buf2[MAX_INPUT_LENGTH] = {'\0'}, *buf3 = NULL;
  struct char_data *vict;
  const char *action_sing, *action_plur, *action_others;
  const char *punctuation;
  int len = 0;

  switch (subcmd)
  {
  case SCMD_WHISPER:
    action_sing = "whisper to";
    action_plur = "whispers to";
    action_others = "$n whispers something to $N.";
    punctuation = ".";
    break;

  case SCMD_ASK:
    action_sing = "ask";
    action_plur = "asks";
    action_others = "$n asks $N a question.";
    punctuation = "?";
    break;

  default:
    action_sing = "oops";
    action_plur = "oopses";
    action_others = "$n is tongue-tied trying to speak with $N.";
    punctuation = ".";
    break;
  }

  half_chop_c(argument, buf, sizeof(buf), buf2, sizeof(buf2));

  if (!*buf || !*buf2)
    send_to_char(ch, "Whom do you want to %s.. and what??\r\n", action_sing);
  else if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (AFF_FLAGGED(ch, AFF_SILENCED))
    send_to_char(ch, "You can't seem to make a sound.\r\n");
  else if (vict == ch)
    send_to_char(ch, "You can't get your mouth close enough to your ear...\r\n");
  else if (AFF_FLAGGED(vict, AFF_DEAF))
    send_to_char(ch, "Your target seems to be deaf!\r\n");
  else
  {
    char buf1[MAX_STRING_LENGTH] = {'\0'};

    /* homeland-port copying string before parsing */
    buf3 = strdup(buf2);

    if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
      parse_at(buf2);
    sentence_case(buf2);

    if (subcmd == SCMD_ASK)
    {
      len = strlen(buf2);
      // remove trailing punctuation from ask
      while (len >= 0 && (buf2[len - 1] == '.' || buf2[len - 1] == '!' || *buf2 == '\n'))
      {
        *(buf2 + len - 1) = '\0';
        len--;
      }
    }
    // append period if it's not already there
    if (buf2[strlen(buf2) - 1] != '.' && buf2[strlen(buf2) - 1] != '!' && buf2[strlen(buf2) - 1] != '?')
      strlcat(buf2, punctuation, sizeof(buf2));

    snprintf(buf1, sizeof(buf1), "$n %s you, '%s'", action_plur, buf2);
    act(buf1, FALSE, ch, 0, vict, TO_VICT);

    if ((!IS_NPC(ch)) && (PRF_FLAGGED(ch, PRF_NOREPEAT)))
      send_to_char(ch, "%s", CONFIG_OK);
    else
      send_to_char(ch, "You %s %s, '%s'\r\n", action_sing, GET_NAME(vict), buf2);
    act(action_others, FALSE, ch, 0, vict, TO_NOTVICT);

    if (subcmd == SCMD_ASK)
      quest_ask(ch, vict, buf3);
  }
}
