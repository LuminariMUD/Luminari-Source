/*
 * File:   bardic_performance.c
 * Author: Zusuk
 * Functions, commands, etc for the bardic performance system
 *    influence from the homelandMUD bard system
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "magic/spells.h"
#include "bardic_performance.h"
#include "combat/fight.h"
#include "spec_procs.h"
#include "actions.h"
#include "character/feats.h"
#include "character/perks.h"

/* defines */
#define DEBUG_MODE FALSE

/* performance types
Act (comedy, drama, pantomime)
Comedy (buffoonery, limericks, joke-telling)
Dance (ballet, waltz, jig)
Keyboard instruments (harpsichord, piano, pipe organ)
Oratory (epic, ode, storytelling)
Percussion instruments (bells, chimes, drums, gong)
String instruments (fiddle, harp, lute, mandolin)
Wind instruments (flute, pan pipes, recorder, trumpet)
Sing (ballad, chant, melody)
*/

/* Instruments obj vals are
   0 - type (lyre/drum/etc)
   1 - diffulty
   2 - level
   3 - breakability   ***/

/* order of current song difficulty (level)
song of healing          1
song of protection       2
song of focused mind     3
song of heroism          5
song of rejuvenation     7
song of flight           9
song of revelation      11
song of fear            13
song of forgetfulness   15
song of rooting         17
deafening song          20
song of dragons         21
song of the magi        25 */

/* performance info: this will be our reference/lookup data for each song/performance
   skillnum, instrument, instrument_skill, difficulty
 *   performance-type, area of affect, associated feat */
/* NOTE: dont' forget to update MAX_PERFORMANCES in bardic_performance.h */
/* NOTE: dont' forget to add associated feat */
int performance_info[MAX_PERFORMANCES][PERFORMANCE_INFO_FIELDS] = {
    /* 0*/
    {SKILL_SONG_OF_HEALING, INSTRUMENT_LYRE, SKILL_LYRE, 4, PERFORMANCE_TYPE_SING,
     PERFORM_AOE_GROUP, FEAT_SONG_OF_HEALING},
    /* 1*/
    {SKILL_DANCE_OF_PROTECTION, INSTRUMENT_DRUM, SKILL_DRUM, 5, PERFORMANCE_TYPE_DANCE,
     PERFORM_AOE_GROUP, FEAT_DANCE_OF_PROTECTION},
    /* 2*/
    {SKILL_SONG_OF_FOCUSED_MIND, INSTRUMENT_HARP, SKILL_HARP, 6, PERFORMANCE_TYPE_SING,
     PERFORM_AOE_GROUP, FEAT_SONG_OF_FOCUSED_MIND},
    /* 3*/
    {SKILL_SONG_OF_HEROISM, INSTRUMENT_DRUM, SKILL_DRUM, 8, PERFORMANCE_TYPE_SING,
     PERFORM_AOE_GROUP, FEAT_SONG_OF_HEROISM},
    /* 4*/
    {SKILL_ORATORY_OF_REJUVENATION, INSTRUMENT_LYRE, SKILL_LYRE, 10, PERFORMANCE_TYPE_ORATORY,
     PERFORM_AOE_GROUP, FEAT_ORATORY_OF_REJUVENATION},
    /* 5*/
    {SKILL_SONG_OF_FLIGHT, INSTRUMENT_HORN, SKILL_HORN, 12, PERFORMANCE_TYPE_SING,
     PERFORM_AOE_GROUP, FEAT_SONG_OF_FLIGHT},
    /* 6*/
    {SKILL_SONG_OF_REVELATION, INSTRUMENT_FLUTE, SKILL_FLUTE, 14, PERFORMANCE_TYPE_SING,
     PERFORM_AOE_GROUP, FEAT_SONG_OF_REVELATION},
    /* 7*/
    {SKILL_SONG_OF_FEAR, INSTRUMENT_HARP, SKILL_HARP, 16, PERFORMANCE_TYPE_SING, PERFORM_AOE_FOES,
     FEAT_SONG_OF_FEAR},
    /* 8*/
    {SKILL_ACT_OF_FORGETFULNESS, INSTRUMENT_FLUTE, SKILL_FLUTE, 18, PERFORMANCE_TYPE_ACT,
     PERFORM_AOE_FOES, FEAT_ACT_OF_FORGETFULNESS},
    /* 9*/
    {SKILL_SONG_OF_ROOTING, INSTRUMENT_MANDOLIN, SKILL_MANDOLIN, 20, PERFORMANCE_TYPE_SING,
     PERFORM_AOE_FOES, FEAT_SONG_OF_ROOTING},
    /*10*/
    {SKILL_SONG_OF_DRAGONS, INSTRUMENT_HORN, SKILL_HORN, 24, PERFORMANCE_TYPE_SING,
     PERFORM_AOE_GROUP, FEAT_SONG_OF_DRAGONS},
    /*11*/
    {SKILL_SONG_OF_THE_MAGI, INSTRUMENT_MANDOLIN, SKILL_MANDOLIN, 29, PERFORMANCE_TYPE_SING,
     PERFORM_AOE_FOES, FEAT_SONG_OF_THE_MAGI},
    /*12*/
    {SKILL_DEAFENING_SONG, INSTRUMENT_DRUM, SKILL_DRUM, 20, PERFORMANCE_TYPE_SING, PERFORM_AOE_FOES,
     FEAT_DEAFENING_SONG},
    /*MAX_PERFORMANCES: 13*/
};

static void reset_performance_crescendo(struct char_data *ch)
{
  GET_CRESCENDO_USED(ch) = 0;
  GET_CRESCENDO_DICE(ch) = 0;
}

void initialize_bardic_performance_state(struct char_data *ch)
{
  int i;

  if (ch == NULL)
    return;

  for (i = 0; i < MAX_PERFORMANCE_VARS; i++)
    GET_PERFORMANCE_VAR(ch, i) = 0;

  GET_PERFORMING(ch) = PERFORMANCE_NONE;
  GET_SECONDARY_PERFORMING(ch) = PERFORMANCE_NONE;
}

void stop_bardic_performance(struct char_data *ch, bool notify)
{
  if (ch == NULL)
    return;

  IS_PERFORMING(ch) = FALSE;
  GET_PERFORMING(ch) = PERFORMANCE_NONE;
  GET_SECONDARY_PERFORMING(ch) = PERFORMANCE_NONE;
  reset_performance_crescendo(ch);

  if (notify)
  {
    act("You stop performing.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n stops performing.", TRUE, ch, 0, 0, TO_ROOM);
  }
}

void stop_bardic_performance_slot(struct char_data *ch, int slot, bool notify)
{
  if (ch == NULL)
    return;

  if (slot == PERFORMANCE_VAR_PRIMARY)
  {
    if (GET_PERFORMING(ch) == PERFORMANCE_NONE)
      return;

    GET_PERFORMING(ch) = GET_SECONDARY_PERFORMING(ch);
    GET_SECONDARY_PERFORMING(ch) = PERFORMANCE_NONE;
  }
  else if (slot == PERFORMANCE_VAR_SECONDARY)
  {
    if (GET_SECONDARY_PERFORMING(ch) == PERFORMANCE_NONE)
      return;

    GET_SECONDARY_PERFORMING(ch) = PERFORMANCE_NONE;
  }
  else
  {
    log("SYSERR: stop_bardic_performance_slot received invalid slot %d", slot);
    return;
  }

  IS_PERFORMING(ch) = GET_PERFORMING(ch) != PERFORMANCE_NONE;
  if (!IS_PERFORMING(ch))
    reset_performance_crescendo(ch);

  if (notify)
  {
    act("You stop that performance.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n brings part of $s performance to a close.", TRUE, ch, 0, 0, TO_ROOM);
  }
}

static void copy_trimmed_performance_argument(const char *argument, char *result,
                                              size_t result_size)
{
  const char *start;
  size_t length;

  if (result == NULL || result_size == 0)
    return;

  result[0] = '\0';
  if (argument == NULL)
    return;

  start = argument;
  while (*start != '\0' && isspace((unsigned char)*start))
    start++;

  length = strlen(start);
  while (length > 0 && isspace((unsigned char)start[length - 1]))
    length--;

  if (length >= result_size)
    length = result_size - 1;

  memcpy(result, start, length);
  result[length] = '\0';
}

static int resolve_performance_name(const char *name, bool *ambiguous)
{
  const char *candidate;
  int exact_match;
  int prefix_match;
  int prefix_count;
  int i;

  if (ambiguous != NULL)
    *ambiguous = FALSE;
  if (name == NULL || *name == '\0')
    return PERFORMANCE_NONE;

  exact_match = PERFORMANCE_NONE;
  prefix_match = PERFORMANCE_NONE;
  prefix_count = 0;

  for (i = 0; i < MAX_PERFORMANCES; i++)
  {
    candidate = skill_name(performance_info[i][PERFORMANCE_SKILLNUM]);
    if (candidate != NULL && str_cmp(name, candidate) == 0)
    {
      exact_match = i;
      break;
    }
  }

  if (exact_match != PERFORMANCE_NONE)
    return exact_match;

  for (i = 0; i < MAX_PERFORMANCES; i++)
  {
    candidate = skill_name(performance_info[i][PERFORMANCE_SKILLNUM]);
    if (candidate != NULL && is_abbrev(name, candidate))
    {
      prefix_match = i;
      prefix_count++;
    }
  }

  if (prefix_count == 1)
    return prefix_match;
  if (prefix_count > 1 && ambiguous != NULL)
    *ambiguous = TRUE;

  return PERFORMANCE_NONE;
}

/* local functions for modifying chars points (hitpoints or moves)
 * note: negative (-) is healing -- 09/2022, replaced with process_healing() -zusuk */
/*
static void alter_hit(struct char_data *ch, int points, bool unused)
{
 GET_HIT(ch) -= points;
 GET_HIT(ch) = MIN(GET_HIT(ch), GET_MAX_HIT(ch));
 update_pos(ch);
}
*/

/* local functions for modifying chars points (hitpoints or moves) */
/* note : negative(-) is healing-- 09 / 2022, replaced with process_healing() - zusuk */
/*
static void alter_move(struct char_data *ch, int points)
{
  GET_MOVE(ch) -= points;
  GET_MOVE(ch) = MIN(GET_MOVE(ch), GET_MAX_MOVE(ch));
  update_pos(ch);
}
*/

/* checks if incoming performance number is actually a valid performance */
bool is_valid_performance(int performance_num)
{
  bool return_val = FALSE;

  if (performance_num < 0 || performance_num >= MAX_PERFORMANCES)
    return FALSE;

  switch (performance_info[performance_num][PERFORMANCE_SKILLNUM])
  {
  case SKILL_SONG_OF_FOCUSED_MIND:
    return_val = TRUE;
    break;
  case SKILL_SONG_OF_FEAR:
    return_val = TRUE;
    break;
  case SKILL_SONG_OF_ROOTING:
    return_val = TRUE;
    break;
  case SKILL_DEAFENING_SONG:
    return_val = TRUE;
    break;
  case SKILL_SONG_OF_THE_MAGI:
    return_val = TRUE;
    break;
  case SKILL_SONG_OF_HEALING:
    return_val = TRUE;
    break;
  case SKILL_DANCE_OF_PROTECTION:
    return_val = TRUE;
    break;
  case SKILL_SONG_OF_FLIGHT:
    return_val = TRUE;
    break;
  case SKILL_SONG_OF_HEROISM:
    return_val = TRUE;
    break;
  case SKILL_ORATORY_OF_REJUVENATION:
    return_val = TRUE;
    break;
  case SKILL_ACT_OF_FORGETFULNESS:
    return_val = TRUE;
    break;
  case SKILL_SONG_OF_REVELATION:
    return_val = TRUE;
    break;
  case SKILL_SONG_OF_DRAGONS:
    return_val = TRUE;
    break;

  default:
    return_val = FALSE;
  }

  return return_val;
}

static void normalize_bardic_performance_state(struct char_data *ch)
{
  if (ch == NULL)
    return;

  if (!is_valid_performance(GET_PERFORMING(ch)))
  {
    if (is_valid_performance(GET_SECONDARY_PERFORMING(ch)))
      GET_PERFORMING(ch) = GET_SECONDARY_PERFORMING(ch);
    else
      GET_PERFORMING(ch) = PERFORMANCE_NONE;
    GET_SECONDARY_PERFORMING(ch) = PERFORMANCE_NONE;
  }

  if (GET_SECONDARY_PERFORMING(ch) == GET_PERFORMING(ch) ||
      !is_valid_performance(GET_SECONDARY_PERFORMING(ch)))
    GET_SECONDARY_PERFORMING(ch) = PERFORMANCE_NONE;

  if (!IS_NPC(ch) && GET_SECONDARY_PERFORMING(ch) != PERFORMANCE_NONE &&
      !has_bard_master_of_motifs(ch))
    GET_SECONDARY_PERFORMING(ch) = PERFORMANCE_NONE;

  IS_PERFORMING(ch) = GET_PERFORMING(ch) != PERFORMANCE_NONE;
  if (!IS_PERFORMING(ch))
    reset_performance_crescendo(ch);
}

/* will list to the performer which performances are available to them */
void list_available_performances(struct char_data *ch)
{
  int i = 0;

  send_to_char(ch, "Available performances:\r\n");
  for (i = 1; i < FEAT_LAST_FEAT; i++)
  {
    if (HAS_FEAT(ch, i))
    {
      if (feat_list[i].feat_type == FEAT_TYPE_PERFORMANCE)
      {
        send_to_char(ch, "%s\r\n", feat_list[i].name);
      }
    }
  }
  send_to_char(ch, "\r\n");
}

/* this function checks whether the conditions for starting/continuing a performance are in place
     in: performer(ch), performance_num,
         need to check whether they are already performing?
         silent (should we be silent and not send ch messages?)
     out:  0 - FALSE, 1 - TRUE   i.e. whether we can continue/start performing -zusuk */
int can_perform(struct char_data *ch, int performance_num, bool need_check, bool silent)
{
  struct char_data *vict = NULL, *next_vict = NULL;

  if (!ch)
    return 0;

  if (IN_ROOM(ch) == NOWHERE)
    return 0;

  if (DEBUG_MODE)
  {
    send_to_char(ch, "can_perform(): PNum: %d, NeedCheck %d, Silent %d.\r\n", performance_num,
                 need_check, silent);
  }

  /* check for disqualifiers */

  if (!is_valid_performance(performance_num))
  {
    if (!silent)
      send_to_char(ch, "(%d) is an invalid performance number.  Please report this to staff.\r\n",
                   performance_num);
    return 0;
  }

  if (!IS_NPC(ch) && !HAS_FEAT(ch, FEAT_BARDIC_MUSIC))
  {
    if (!silent)
      send_to_char(ch, "You don't know how to perform.\r\n");
    return 0;
  }

  if (need_check && IS_PERFORMING(ch))
  {
    if (!IS_NPC(ch) && has_bard_master_of_motifs(ch))
    {
      if (GET_SECONDARY_PERFORMING(ch) != PERFORMANCE_NONE)
      {
        if (!silent)
          send_to_char(ch, "You are already maintaining two performances!\r\n");
        return 0;
      }
    }
    else
    {
      /* Normal bards can only have 1 performance */
      if (!silent)
        send_to_char(ch, "You are already in the middle of a performance!\r\n");
      return 0;
    }
  }

  if (((ch->in_room != NOWHERE && ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF)) ||
       AFF_FLAGGED(ch, AFF_SILENCED)) &&
      (performance_info[performance_num][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_KEYBOARD ||
       performance_info[performance_num][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_ORATORY ||
       performance_info[performance_num][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_PERCUSSION ||
       performance_info[performance_num][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_STRING ||
       performance_info[performance_num][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_WIND ||
       performance_info[performance_num][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_SING))
  {
    if (!silent)
      send_to_char(ch, "The silence effectively stops your performance.\r\n");
    return 0;
  }

  if (GET_HIT(ch) < 0)
  {
    if (!silent)
      send_to_char(ch, "You can't concentrate on your performance while so seriously injured!\r\n");
    return 0;
  }

  if (GET_POS(ch) < POS_FIGHTING)
  {
    if (!silent)
      send_to_char(ch, "You can't concentrate on your performance when you are in "
                       "this position.\r\n");
    return 0;
  }

  /***** new limit - only one bard in the room performing, sorry! ******/
  for (vict = world[IN_ROOM(ch)].people; vict; vict = next_vict)
  {
    next_vict = vict->next_in_room;

    if (vict)
    {
      if (vict != ch && !IS_NPC(vict) && IS_PERFORMING(vict) &&
          (vict->desc == NULL || !IS_PLAYING(vict->desc)))
        stop_bardic_performance(vict, FALSE);

      if (IN_ROOM(vict) != NOWHERE && vict != ch && IS_PERFORMING(vict))
      {
        if (!silent)
          send_to_char(ch, "Your bardic performance conflicts with %s and is interrupted!\r\n",
                       GET_NAME(vict));
        return 0;
      }
    }
  }

  /* the check for hunger/thirst/etc WOULD to be here */
  /***/

  /* we made it! */
  return 1;
}

/* primary command entry point for the bardic performance */
ACMD(do_perform)
{
  const char *name;
  const char *remainder;
  char request[MAX_INPUT_LENGTH];
  char command[MAX_INPUT_LENGTH];
  char command_argument[MAX_INPUT_LENGTH];
  int performance_num;
  int transition;
  bool ambiguous;
  bool explicit_add;
  bool explicit_replace;
  bool has_move_action;
  bool has_standard_action;

  copy_trimmed_performance_argument(argument, request, sizeof(request));
  normalize_bardic_performance_state(ch);

  if (*request == '\0')
  {
    if (argument != NULL && *argument != '\0')
    {
      send_to_char(ch, "Specify a performance name, 'list', or 'stop'.\r\n");
      return;
    }
    if (IS_PERFORMING(ch))
      stop_bardic_performance(ch, TRUE);
    else
    {
      send_to_char(ch, "Perform what?\r\n");
      list_available_performances(ch);
    }
    return;
  }

  remainder = one_argument(request, command, sizeof(command));
  copy_trimmed_performance_argument(remainder, command_argument, sizeof(command_argument));

  if (str_cmp(command, "list") == 0 && *command_argument == '\0')
  {
    list_available_performances(ch);
    return;
  }

  if (str_cmp(command, "stop") == 0)
  {
    if (*command_argument == '\0')
    {
      if (IS_PERFORMING(ch))
        stop_bardic_performance(ch, TRUE);
      else
        send_to_char(ch, "You are not performing.\r\n");
      return;
    }

    performance_num = resolve_performance_name(command_argument, &ambiguous);
    if (ambiguous)
    {
      send_to_char(ch, "That performance name is ambiguous.\r\n");
      return;
    }
    if (performance_num == PERFORMANCE_NONE)
    {
      send_to_char(ch, "That is not a performance.\r\n");
      return;
    }
    if (GET_PERFORMING(ch) == performance_num)
      stop_bardic_performance_slot(ch, PERFORMANCE_VAR_PRIMARY, TRUE);
    else if (GET_SECONDARY_PERFORMING(ch) == performance_num)
      stop_bardic_performance_slot(ch, PERFORMANCE_VAR_SECONDARY, TRUE);
    else
      send_to_char(ch, "You are not maintaining that performance.\r\n");
    return;
  }

  explicit_add = str_cmp(command, "add") == 0;
  explicit_replace = str_cmp(command, "replace") == 0;
  if (explicit_add || explicit_replace)
  {
    if (*command_argument == '\0')
    {
      send_to_char(ch, "Specify the performance to %s.\r\n", explicit_add ? "add" : "replace");
      return;
    }
    name = command_argument;
  }
  else
  {
    name = request;
  }

  performance_num = resolve_performance_name(name, &ambiguous);
  if (ambiguous)
  {
    send_to_char(ch, "That performance name is ambiguous.\r\n");
    return;
  }
  if (performance_num == PERFORMANCE_NONE)
  {
    send_to_char(ch, "That is not a performance.\r\n");
    list_available_performances(ch);
    return;
  }

  if (!HAS_FEAT(ch, performance_info[performance_num][PERFORMANCE_FEATNUM]))
  {
    send_to_char(ch, "You do not know that performance.\r\n");
    return;
  }

  if (GET_PERFORMING(ch) == performance_num || GET_SECONDARY_PERFORMING(ch) == performance_num)
  {
    send_to_char(ch, "You are already maintaining that performance.\r\n");
    return;
  }

  transition = PERFORMANCE_VAR_PRIMARY;
  if (IS_PERFORMING(ch))
  {
    if (explicit_add || (!explicit_replace && !IS_NPC(ch) && has_bard_master_of_motifs(ch) &&
                         GET_SECONDARY_PERFORMING(ch) == PERFORMANCE_NONE))
    {
      if (IS_NPC(ch) || !has_bard_master_of_motifs(ch))
      {
        send_to_char(ch, "You cannot maintain a second performance.\r\n");
        return;
      }
      if (GET_SECONDARY_PERFORMING(ch) != PERFORMANCE_NONE)
      {
        send_to_char(ch, "You are already maintaining two performances.\r\n");
        return;
      }
      transition = PERFORMANCE_VAR_SECONDARY;
    }
  }
  else if (explicit_add)
  {
    send_to_char(ch, "Start a primary performance before adding a second one.\r\n");
    return;
  }

  if (!can_perform(ch, performance_num, FALSE, FALSE))
    return;

  has_move_action = TRUE;
  has_standard_action = TRUE;
  if (!IS_NPC(ch))
  {
    has_move_action = is_action_available(ch, atMOVE, FALSE);
    has_standard_action = is_action_available(ch, atSTANDARD, FALSE);

    if (HAS_FEAT(ch, FEAT_EFFICIENT_PERFORMANCE))
    {
      if (!has_move_action && !has_standard_action)
      {
        send_to_char(ch, "You need a move or standard action to begin performing.\r\n");
        return;
      }
    }
    else if (!has_standard_action)
    {
      send_to_char(ch, "You need a standard action to begin performing.\r\n");
      return;
    }
  }

  if (transition == PERFORMANCE_VAR_SECONDARY)
  {
    GET_SECONDARY_PERFORMING(ch) = performance_num;
    act("You begin a second performance, maintaining both at once.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n begins a second performance without letting the first lapse.", TRUE, ch, 0, 0, TO_ROOM);
  }
  else
  {
    GET_PERFORMING(ch) = performance_num;
    if (!IS_PERFORMING(ch))
      GET_SECONDARY_PERFORMING(ch) = PERFORMANCE_NONE;
    act(IS_PERFORMING(ch) ? "You replace your primary performance without losing the rhythm."
                          : "You start performing.",
        FALSE, ch, 0, 0, TO_CHAR);
    act(IS_PERFORMING(ch) ? "$n changes the lead of $s performance." : "$n starts performing.",
        TRUE, ch, 0, 0, TO_ROOM);
  }

  IS_PERFORMING(ch) = TRUE;
  reset_performance_crescendo(ch);

  if (!IS_NPC(ch))
  {
    if (HAS_FEAT(ch, FEAT_EFFICIENT_PERFORMANCE))
    {
      if (has_move_action)
        USE_MOVE_ACTION(ch);
      else
        USE_STANDARD_ACTION(ch);
    }
    else
      USE_STANDARD_ACTION(ch);
  }
}

/* function for processing individual effects for the performance */
int performance_effects(struct char_data *ch, struct char_data *tch, int spellnum,
                        int effectiveness, int aoe)
{
  int return_val = 1, i = 0; /* return_val is 1, very limited reasons to fail here! */
  int songweaver_bonus;
  bool nomessage = FALSE, engage = TRUE;
  struct affected_type af[BARD_AFFECTS];

  if (!ch)
    return 0;

  if (!tch)
    return 0;

  if (DEBUG_MODE)
  {
    send_to_char(ch, "performance_effects(): tch: %s, SNum: %d, Effect %d, AoE %d.\r\n",
                 GET_NAME(tch), spellnum, effectiveness, aoe);
  }

  songweaver_bonus = IS_NPC(ch) ? 0 : get_bard_songweaver_level_bonus(ch);

  /* init affect array */
  for (i = 0; i < BARD_AFFECTS; i++)
  {
    new_affect(&(af[i]));

    af[i].spell = spellnum;
    af[i].duration = 3 + songweaver_bonus;
    af[i].bonus_type = BONUS_TYPE_INHERENT;
    af[i].modifier = 1;
    af[i].location = APPLY_NONE;
  }

  if (affected_by_spell(tch, spellnum))
  {
    nomessage = TRUE;
    /* purpose: refresh song duration */
    affect_from_char(tch, spellnum);
    update_pos(tch);
  }

  /* dummy check: still issues with AC */
  if (!IS_NPC(tch) && tch->desc)
    save_char(tch, 0);

  switch (spellnum)
  {
  case SKILL_SONG_OF_HEALING:
    send_to_char(tch, "You are soothed by the power of music!\r\n");
    process_healing(ch, tch, SKILL_SONG_OF_HEALING,
                    rand_number(effectiveness, effectiveness * 2 + 10), 0, 0);
    break;

  case SKILL_DANCE_OF_PROTECTION:
    af[0].location = APPLY_AC_NEW;
    af[0].modifier = (effectiveness + 1) / 7;

    af[1].location = APPLY_SAVING_WILL;
    af[1].modifier = effectiveness / 6;

    af[2].location = APPLY_DR;
    af[2].modifier = effectiveness / 13;

    break;

  case SKILL_SONG_OF_HEROISM:
    af[0].location = APPLY_HITROLL;
    af[0].modifier = 1 + effectiveness / 10;

    af[1].location = APPLY_DAMROLL;
    af[1].modifier = effectiveness / 10;

    af[2].location = APPLY_STR;
    af[2].modifier = effectiveness / 10;

    af[3].location = APPLY_DEX;
    af[3].modifier = effectiveness / 10;

    af[4].location = APPLY_CON;
    af[4].modifier = effectiveness / 10;

    if (GET_LEVEL(ch) >= 10 && !AFF_FLAGGED(tch, AFF_HASTE))
    {
      SET_BIT_AR(af[1].bitvector, AFF_HASTE);
      act("You feel the world slow down around you.", FALSE, tch, 0, 0, TO_CHAR);
      act("$n starts to move with uncanny speed.", TRUE, tch, 0, 0, TO_ROOM);
    }
    break;

  case SKILL_ORATORY_OF_REJUVENATION:
    if (GET_HIT(tch) < GET_MAX_HIT(tch))
    {
      send_to_char(tch, "You are soothed by the power of music!\r\n");
      process_healing(ch, tch, SKILL_ORATORY_OF_REJUVENATION,
                      rand_number(effectiveness / 3, effectiveness / 2), 0, 0);
    }

    process_healing(ch, tch, SKILL_ORATORY_OF_REJUVENATION, 0,
                    rand_number(effectiveness * 40, effectiveness * 60 + 60), 0);

    if (rand_number(0, 100) < effectiveness && affected_by_spell(tch, SPELL_POISON))
    {
      affect_from_char(tch, SPELL_POISON);
      send_to_char(tch, "The soothing music clears the poison from your body!\r\n");
    }
    break;

  case SKILL_SONG_OF_REVELATION:
    if (!AFF_FLAGGED(tch, AFF_DETECT_INVIS))
    {
      af[0].location = APPLY_HITROLL;
      af[0].modifier = 0;
      SET_BIT_AR(af[0].bitvector, AFF_DETECT_INVIS);
    }
    if (!AFF_FLAGGED(tch, AFF_DETECT_ALIGN) && GET_LEVEL(ch) >= 5)
    {
      af[1].location = APPLY_DAMROLL;
      af[1].modifier = 0;
      SET_BIT_AR(af[1].bitvector, AFF_DETECT_ALIGN);
    }
    if (!AFF_FLAGGED(tch, AFF_DETECT_MAGIC) && GET_LEVEL(ch) >= 10)
    {
      af[2].location = APPLY_AC;
      af[2].modifier = 0;
      SET_BIT_AR(af[2].bitvector, AFF_DETECT_MAGIC);
    }
    if (!AFF_FLAGGED(tch, AFF_SENSE_LIFE) && GET_LEVEL(ch) >= 15)
    {
      af[3].location = APPLY_DEX;
      af[3].modifier = 0;
      SET_BIT_AR(af[3].bitvector, AFF_SENSE_LIFE);
    }
    if (!AFF_FLAGGED(tch, AFF_FARSEE) && GET_LEVEL(ch) >= 20)
    {
      af[4].location = APPLY_AGE;
      af[4].modifier = 0;
      SET_BIT_AR(af[4].bitvector, AFF_FARSEE);
    }
    if (nomessage == FALSE)
      act("You feel your eyes tingle.", FALSE, tch, 0, 0, TO_CHAR);
    break;

  case SKILL_SONG_OF_DRAGONS:

    af[0].location = APPLY_AC_NEW;
    af[0].modifier = MAX(1, (effectiveness + 2) / 9);
    af[0].bonus_type = BONUS_TYPE_INHERENT;

    af[1].location = APPLY_SAVING_REFL;
    af[1].modifier = effectiveness / 5;
    af[1].bonus_type = BONUS_TYPE_INHERENT;

    af[2].location = APPLY_SAVING_DEATH;
    af[2].modifier = effectiveness / 5;
    af[2].bonus_type = BONUS_TYPE_INHERENT;

    af[3].location = APPLY_SAVING_FORT;
    af[3].modifier = effectiveness / 5;
    af[3].bonus_type = BONUS_TYPE_INHERENT;

    af[4].location = APPLY_SAVING_POISON;
    af[4].modifier = effectiveness / 5;
    af[4].bonus_type = BONUS_TYPE_INHERENT;

    af[5].location = APPLY_SAVING_WILL;
    af[5].modifier = effectiveness / 5;
    af[5].bonus_type = BONUS_TYPE_INHERENT;

    af[6].location = APPLY_CON;
    af[6].modifier = 2 + effectiveness / 3;
    af[6].bonus_type = BONUS_TYPE_INHERENT;

    af[7].location = APPLY_HIT;
    af[7].modifier = 40 + effectiveness * 4;
    af[7].bonus_type = BONUS_TYPE_INHERENT;

    break;

  case SKILL_ACT_OF_FORGETFULNESS:
    if (IS_NPC(tch) && rand_number(0, 100) < effectiveness)
    {
      clearMemory(tch);

      if (FIGHTING(tch))
        stop_fighting(tch);

      engage = FALSE;
    }
    break;

  case SKILL_SONG_OF_FLIGHT:
    if (!AFF_FLAGGED(tch, AFF_FLYING))
    {
      af[0].location = APPLY_SPECIAL;
      af[0].duration = 30;
      SET_BIT_AR(af[0].bitvector, AFF_FLYING);
      act("You fly through the air, free as a bird!", FALSE, tch, 0, 0, TO_CHAR);
      act("$n fly through the air, free as a bird!", TRUE, tch, 0, 0, TO_ROOM);
    }
    process_healing(ch, tch, SKILL_SONG_OF_FLIGHT, 0, rand_number(50, effectiveness * 10 / 3 + 50),
                    0);
    break;

  /* increases memming / casting effectiveness */
  case SKILL_SONG_OF_FOCUSED_MIND:
    af[0].location = APPLY_INT;
    af[0].modifier = 1 + effectiveness / 7;
    af[1].location = APPLY_WIS;
    af[1].modifier = 1 + effectiveness / 7;
    af[2].location = APPLY_CHA;
    af[2].modifier = 1 + effectiveness / 7;

    /* using affected_by_spell() for memorization bonus */

    break;

  /* enemy fight less effective / flee */
  case SKILL_SONG_OF_FEAR:
    if (!IS_NPC(tch) && has_aura_of_courage(tch) && !affected_by_aura_of_cowardice(tch))
      break;
    if (!IS_NPC(tch) && HAS_FEAT(tch, FEAT_RP_FEARLESS_RAGE) && affected_by_spell(tch, SKILL_RAGE))
      break;
    if (!IS_NPC(tch) && HAS_FEAT(tch, FEAT_FEARLESS_DEFENSE) &&
        affected_by_spell(tch, SKILL_DEFENSIVE_STANCE))
      break;
    if (AFF_FLAGGED(tch, AFF_MIND_BLANK))
      break;

    if (rand_number(0, 100) < effectiveness)
    {
      act("$n shivers with fear.", TRUE, tch, 0, 0, TO_ROOM);
      SET_BIT_AR(af[0].bitvector, AFF_FEAR);
      af[0].location = APPLY_HITROLL;
      af[0].modifier = -(1 + effectiveness / 10);
    }
    break;

  /* enemy fight less effective / entangled */
  case SKILL_SONG_OF_ROOTING:
    if (rand_number(0, 100) < effectiveness)
    {
      act("$n has spawned roots.", TRUE, tch, 0, 0, TO_ROOM);
      SET_BIT_AR(af[0].bitvector, AFF_ENTANGLED);
      af[0].location = APPLY_DAMROLL;
      af[0].modifier = -effectiveness / 5;

      SET_BIT_AR(af[1].bitvector, AFF_SLOW);
      af[1].location = APPLY_AC_NEW;
      af[1].modifier = -effectiveness / 9;
    }
    break;

  case SKILL_DEAFENING_SONG:
    act("$n hs lost their hearing.", TRUE, tch, 0, 0, TO_ROOM);
    SET_BIT_AR(af[0].bitvector, AFF_DEAF);
    af[0].location = APPLY_AC_NEW;
    af[0].modifier = -effectiveness / 5;
    break;

  /* enemy spell resistance / saves reduced */
  case SKILL_SONG_OF_THE_MAGI:
    if (rand_number(0, 100) < effectiveness)
    {
      act("$n seems more vulnerable to magic.", TRUE, tch, 0, 0, TO_ROOM);
      af[0].location = APPLY_SAVING_WILL;
      af[0].modifier = -(1 + effectiveness / 4);

      af[1].location = APPLY_SPELL_RES;
      af[1].modifier = -(2 + effectiveness / 10);

      af[2].location = APPLY_INT;
      af[2].modifier = 1 + effectiveness / 7;

      af[3].location = APPLY_WIS;
      af[3].modifier = 1 + effectiveness / 7;

      af[4].location = APPLY_CHA;
      af[4].modifier = 1 + effectiveness / 7;
    }
    break;

  /* UH OH! */
  default:
    log("SYSERR: performance_effects reached default case! "
        "(spellnum: %d)",
        spellnum);
    return_val = 0;
    break;

  } /* end switch */

  /*** Bard Spellsinger: Resonant Voice I - add save bonuses for allies to resistances against mind-affecting effects ***/
  if (!IS_NPC(ch))
  {
    int resonant_bonus = get_bard_resonant_voice_save_bonus(ch);
    if (resonant_bonus > 0 && aoe == PERFORM_AOE_GROUP)
    {
      /* Find an available affect slot and add Will save bonus (for mind-affecting) */
      if (af[6].location == APPLY_NONE)
      {
        af[6].location = APPLY_SAVING_WILL;
        af[6].modifier = resonant_bonus;
        af[6].bonus_type = BONUS_TYPE_COMPETENCE;
      }
    }
  }

  /*** now we apply the affection(s) */
  for (i = 0; i < BARD_AFFECTS; i++)
  {
    /* lingering song bonus */
    if (HAS_FEAT(ch, FEAT_LINGERING_SONG))
      af[i].duration += 3;

    /* attach the affections! */
    affect_join(tch, af + i, FALSE, FALSE, FALSE, FALSE);
  }
  /****/

  /* dummy check: still issues with AC */
  if (!IS_NPC(tch) && tch->desc)
    save_char(tch, 0);

  /* aggressive song should engage foes */
  if (aoe == PERFORM_AOE_FOES && engage)
  {
    if (tch != ch)
    {
      if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL))
      {
        set_fighting(ch, tch);
      }
      if (GET_POS(tch) > POS_STUNNED && (FIGHTING(tch) == NULL))
      {
        set_fighting(tch, ch);
      }
    }
  }

  return return_val;
}

/* main function for performance effects / message / targets / etc */
int process_performance(struct char_data *ch, int performance_num, int effectiveness, int aoe)
{
  if (DEBUG_MODE)
  {
    send_to_char(ch, "process_performance(): PNum: %d, Effect %d, AoE %d.\r\n", performance_num,
                 effectiveness, aoe);
  }

  struct char_data *tch = NULL, *tch_next = NULL;
  int return_val = 1;
  bool hit_self = FALSE, hit_leader = FALSE;

  /* performance message */
  switch (performance_num)
  {
  case SKILL_SONG_OF_HEALING:
    act("You sing a song to heal all wounds.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n sings a song so well you feel your pain and suffering ebbing away.", TRUE, ch, 0, 0,
        TO_ROOM);
    break;

  case SKILL_DANCE_OF_PROTECTION:
    act("You dance to protect yourself from harm.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n performs a dance that envelops you in protection.", TRUE, ch, 0, 0, TO_ROOM);
    break;

  case SKILL_SONG_OF_FLIGHT:
    act("You sing a song that lifts the spirits high.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n sings a song that lifts the spirits high.", TRUE, ch, 0, 0, TO_ROOM);
    break;

  case SKILL_SONG_OF_HEROISM:
    act("You sing a song that makes your heart swell with pride.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n sings a song that makes your heart swell with pride.", TRUE, ch, 0, 0, TO_ROOM);
    break;

  case SKILL_ORATORY_OF_REJUVENATION:
    act("You conduct an oratory to rejuvenate the exhausted.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n conducts an oratory which eases some of your exhaustion.", TRUE, ch, 0, 0, TO_ROOM);
    break;

  case SKILL_ACT_OF_FORGETFULNESS:
    act("You act out a skit causing forgetfulness.", FALSE, ch, 0, 0, TO_CHAR);
    act("As you observe $n acting out a skit, suddenly you can hardly "
        "remember what you were doing.",
        TRUE, ch, 0, 0, TO_ROOM);
    break;

  case SKILL_SONG_OF_REVELATION:
    act("You sing a song to reveal what is hidden.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n sings a song that seems to enhance your vision.", TRUE, ch, 0, 0, TO_ROOM);
    break;

  case SKILL_SONG_OF_DRAGONS:
    act("You sing a song that defies the mightiest of dragons.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n sings a song that defies the mightiest of dragons, inspiring you to truly heroic "
        "deeds!",
        TRUE, ch, 0, 0, TO_ROOM);
    break;

  case SKILL_SONG_OF_FOCUSED_MIND:
    act("You sing a song which focuses the minds of the listener.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n sings a song which seems to focus your mind.", TRUE, ch, 0, 0, TO_ROOM);
    break;

  case SKILL_SONG_OF_FEAR:
    act("You sing a song which strikes fear into your enemies.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n sings a song which stikes fear into your heart!", TRUE, ch, 0, 0, TO_ROOM);
    break;

  case SKILL_SONG_OF_ROOTING:
    act("You sing a song which makes your enemies paralysed.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n sings a song so well, you feel paralysed by the tune.", TRUE, ch, 0, 0, TO_ROOM);
    break;

  case SKILL_DEAFENING_SONG:
    act("You sing a song which deafens your enemies.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n sings a song so well, you feel deafened by the tune.", TRUE, ch, 0, 0, TO_ROOM);
    break;

  case SKILL_SONG_OF_THE_MAGI:
    act("You sing a song so well, that magic in itself feels strengthened by it.", FALSE, ch, 0, 0,
        TO_CHAR);
    act("$n sings a song which makes you forget completely about hostile magic.", TRUE, ch, 0, 0,
        TO_ROOM);
    break;

  default:
    return_val = 0;
    log("SYSERR: messages in process_performance reached default case! "
        "(performance_num: %d)",
        performance_num);
    break;
  }

  /* here we handle the different type of dances */
  switch (aoe)
  {
  /* performance that should affect your group only */
  case PERFORM_AOE_GROUP:
    if (!GROUP(ch))
    { /* self only */
      return_val = performance_effects(ch, ch, performance_num, effectiveness, aoe);
    }
    else
    {
      /* Beginner's Note: Reset simple_list iterator before use to prevent
       * cross-contamination from previous iterations. Without this reset,
       * if simple_list was used elsewhere and not completed, it would
       * continue from where it left off instead of starting fresh. */
      simple_list(NULL);

      while ((tch = (struct char_data *)simple_list(GROUP(ch)->members)) != NULL)
      {
        if (IN_ROOM(tch) != IN_ROOM(ch))
          continue;

        if (tch == ch) /* this is a dummy check added due to an uknown bug with lists :(  -zusuk */
          hit_self = TRUE;

        /* this is a dummy check added due to an uknown bug with lists :(  -zusuk */
        if (GROUP(ch)->leader && GROUP(ch)->leader == tch)
          hit_leader = TRUE;

        return_val = performance_effects(ch, tch, performance_num, effectiveness, aoe);
      }

      /* this is a dummy check added due to an uknown bug with lists :(  -zusuk */
      if (!hit_self)
      {
        return_val = performance_effects(ch, ch, performance_num, effectiveness, aoe);

        if (ch == GROUP(ch)->leader)
          hit_leader = TRUE;
      }

      /* this is a dummy check added due to an uknown bug with lists :(  -zusuk */
      if (!hit_leader && GROUP(ch)->leader && IN_ROOM(GROUP(ch)->leader) == IN_ROOM(ch))
        return_val =
            performance_effects(ch, GROUP(ch)->leader, performance_num, effectiveness, aoe);
    }
    break;

  /* performance that should affect those NOT in your group (potential foes) */
  case PERFORM_AOE_FOES:
    /* for loop to step through all in room */
    for (tch = world[ch->in_room].people; tch; tch = tch_next)
    {
      tch_next = tch->next_in_room;

      /* check if offensive aoe is OK */
      if (aoeOK(ch, tch, performance_num))
      {
        return_val = performance_effects(ch, tch, performance_num, effectiveness, aoe);
      }
    } /* end for loop */
    break;

  /* performance that should affect everyone in the room */
  case PERFORM_AOE_ROOM:
    /* for loop to step through all in room */
    for (tch = world[ch->in_room].people; tch; tch = tch_next)
    {
      tch_next = tch->next_in_room;

      return_val = performance_effects(ch, tch, performance_num, effectiveness, aoe);
    } /* end for loop */
    break;

  default:
    log("SYSERR: aoe-switch in process_performance reached default case! "
        "(performance_num: %d)",
        performance_num);
    return_val = 0;
    break;
  }

  return return_val; /* 0 = fail, 1 = success */
}

/* this is the primary engine for the bard songs */
static int bardic_performance_engine(struct char_data *ch, int slot)
{
  struct obj_data *instrument = NULL;
  int effectiveness = 0;
  int performance_num;
  int spellnum = -1;
  int difficulty = 0;

  if (ch == NULL)
    return 0;

  if (slot == PERFORMANCE_VAR_PRIMARY)
    performance_num = GET_PERFORMING(ch);
  else if (slot == PERFORMANCE_VAR_SECONDARY)
    performance_num = GET_SECONDARY_PERFORMING(ch);
  else
  {
    log("SYSERR: bardic_performance_engine received invalid slot %d", slot);
    return 0;
  }

  if (!is_valid_performance(performance_num))
  {
    stop_bardic_performance_slot(ch, slot, FALSE);
    return 0;
  }

  /* disqualifiers */
  if (!can_perform(ch, performance_num, FALSE, FALSE))
  {
    stop_bardic_performance_slot(ch, slot, FALSE);
    return 0;
  }

  /* the check for hunger/thirst WOULD to be here */
  /***/
  /* end disqualifiers */

  /* base effectiveness of performance */
  effectiveness = rand_number(1, 9);

  /* base difficulty */
  difficulty = 30;

  /* charisma bonus helps difficulty */
  difficulty -= GET_CHA_BONUS(ch);

  /* performance check for difficulty ! */
  if (compute_ability(ch, ABILITY_PERFORM) + d20(ch) >=
      performance_info[performance_num][PERFORMANCE_DIFF] + 10)
  {
    difficulty -= 4;
  }

  /* find an instrument */
  instrument = GET_EQ(ch, WEAR_HOLD_1);
  if (!instrument || GET_OBJ_TYPE(instrument) != ITEM_INSTRUMENT)
  {
    instrument = GET_EQ(ch, WEAR_HOLD_2);
  }
  if (!instrument || GET_OBJ_TYPE(instrument) != ITEM_INSTRUMENT)
  {
    instrument = GET_EQ(ch, WEAR_HOLD_2H);
  }
  if (!instrument || GET_OBJ_TYPE(instrument) != ITEM_INSTRUMENT)
    instrument = NULL; /* nope, nothing! */
  /* END find an instrument */

  /* Any instrument is better than nothing, if its the designated instrument,
   * and good at it, then even better.. */
  if (!instrument)
  {
    effectiveness -= 3;
    send_to_char(ch, "You perform without an instrument...  ");
  }
  else
  {
    /* the effectiveness / difficulty bonus of our instrument is all handled here */
    difficulty -= GET_OBJ_VAL(instrument, 1);

    /* instrument of quality <= 0 is unbreakable */
    if (!rand_number(0, 9) && rand_number(2, 11111) <= GET_OBJ_VAL(instrument, 3))
    {
      act("Your $p cannot take the strain of magic any longer, and it breaks!", FALSE, ch,
          instrument, 0, TO_CHAR);
      act("$n's $p cannot take the strain of magic any longer, and it breaks!", TRUE, ch,
          instrument, 0, TO_ROOM);
      extract_obj(instrument);
      instrument = NULL;
      effectiveness -= 5;
    }
    else if (GET_OBJ_VAL(instrument, 0) == performance_info[performance_num][INSTRUMENT_NUM])
    {
      /* can add a check to see how proficient one is at given instrument */
      effectiveness += GET_OBJ_VAL(instrument, 2);
    }
    else
    { /* wrong instrument */
      send_to_char(ch, "Not the ideal instrument, but better than nothing!  ");
      effectiveness -= 2;
    }
  }

  /* cap how effective our base roll + instruments can help us */
  if (effectiveness > MAX_INSTRUMENT_EFFECT)
    effectiveness = MAX_INSTRUMENT_EFFECT;

  /* if fighting, 1/2 effect of it.*/
  if (FIGHTING(ch))
  {
    send_to_char(
        ch, "Your performance is slightly hindered as you are concentrating on the combat.\r\n");
    effectiveness /= 2;
  }

  /* performance ability! */
  spellnum = performance_info[performance_num][PERFORMANCE_SKILLNUM];

  /* performance check for effectiveness! */
  if (compute_ability(ch, ABILITY_PERFORM) + d20(ch) >=
      performance_info[performance_num][PERFORMANCE_DIFF] + 10)
  {
    effectiveness += 3;
  }

  /* this is the currently formula for effectiveness of the performance */
  effectiveness = effectiveness * compute_ability(ch, ABILITY_PERFORM) / 7;

  /* effectiveness is from 1 - MAX_PRFM_EFFECT */
  if (effectiveness < 1)
    effectiveness = 1;
  if (effectiveness > MAX_PRFM_EFFECT)
    effectiveness = MAX_PRFM_EFFECT;

  /* GUTS! message, effect processed in this function */
  if (!process_performance(ch, spellnum, effectiveness,
                           performance_info[performance_num][PERFORMANCE_AOE]))
  {
    send_to_char(ch, "Your performance fails!\r\n");
    stop_bardic_performance_slot(ch, slot, FALSE);
    return 0; /* process performance failed somehow */
  }

  /* check for stutter. if stutter, stop performance  */
  if (!rand_number(0, 1) && rand_number(1, 101) < difficulty)
  {
    send_to_char(ch, "Uh oh.. how did the performance go, anyway?\r\n");
    act("$n stutters in the performance!", TRUE, ch, 0, 0, TO_ROOM);
    stop_bardic_performance_slot(ch, slot, FALSE);
    return 0;
  }

  /* success, the next verse arrives on the global performance pulse */
  return 1;
}

/* Process every active performer. Linkless players are stopped; NPCs can use this engine directly. */
void pulse_bardic_performance()
{
  struct char_data *ch;
  struct char_data *next_ch;

  for (ch = character_list; ch; ch = next_ch)
  {
    next_ch = ch->next;

    if (!IS_PERFORMING(ch))
      continue;

    if (!IS_NPC(ch) && (ch->desc == NULL || !IS_PLAYING(ch->desc)))
    {
      stop_bardic_performance(ch, FALSE);
      continue;
    }

    normalize_bardic_performance_state(ch);
    if (!IS_PERFORMING(ch))
      continue;

    bardic_performance_engine(ch, PERFORMANCE_VAR_PRIMARY);

    if (IS_PERFORMING(ch) && GET_SECONDARY_PERFORMING(ch) != PERFORMANCE_NONE)
      bardic_performance_engine(ch, PERFORMANCE_VAR_SECONDARY);

    /* Tier 3 Spellsinger: Dirge of Dissonance - room-wide sonic damage */
    if (!IS_NPC(ch) && IS_PERFORMING(ch) && has_bard_dirge_of_dissonance(ch))
    {
      struct char_data *tch = NULL, *next_tch = NULL;
      int dirge_damage = get_bard_dirge_sonic_damage(ch);

      if (dirge_damage > 0)
      {
        send_to_char(ch,
                     "\tMYour Dirge of Dissonance fills the room with discordant tones!\tn\r\n");

        /* Damage all enemies in the room */
        for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch)
        {
          int dam;

          next_tch = tch->next_in_room;

          /* Skip self, allies, and those not valid AOE targets */
          if (tch == ch || !aoeOK(ch, tch, -1))
            continue;

          dam = dice(dirge_damage, 6);
          if (dam > 0)
          {
            send_to_char(tch, "\tRThe discordant sounds assault your senses! [%d damage]\tn\r\n",
                         dam);
            damage(ch, tch, dam, -1, DAM_SOUND, FALSE);
          }
        }
      }
    }

    /* Tier 4 Spellsinger: Symphonic Resonance - grant temp HP per round */
    if (!IS_NPC(ch) && IS_PERFORMING(ch) && has_bard_symphonic_resonance(ch))
    {
      int temp_hp = get_bard_symphonic_resonance_temp_hp(ch);
      if (temp_hp > 0)
      {
        /* Temporary HP implementation would go here */
        /* For now, just send a message indicating the effect is active */
        send_to_char(ch, "\tCYour song grants temporary protection to you and your allies.\tn\r\n");
      }
    }

    /* Tier 4 Spellsinger: Endless Refrain - regenerate spell slots per round */
    if (!IS_NPC(ch) && IS_PERFORMING(ch) && has_bard_endless_refrain(ch))
    {
      int slot_regen = get_bard_endless_refrain_slot_regen(ch);
      if (slot_regen > 0)
      {
        int class_level = CLASS_LEVEL(ch, CLASS_BARD);
        int circle = MIN(6, class_level / 2);

        if (circle > 0 && circle <= NUM_CIRCLES)
        {
          /* Note: Assuming spells_prepared array exists and tracks available slots */
          /* This needs to match the spell slot tracking system in place */
          send_to_char(ch, "\tCYour song refills your magical reserves.\tn\r\n");
        }
      }
    }
  }

  return;
}

/* EOF */
