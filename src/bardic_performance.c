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
#include "magic/domains_schools.h"
#include "bardic_performance.h"
#include "combat/fight.h"
#include "spec_procs.h"
#include "actions.h"
#include "lists.h"
#include "character/feats.h"
#include "character/perks.h"
#include "magic/spell_prep.h"

/* defines */
#define DEBUG_MODE FALSE

static long next_bardic_performance_source_id = 1;
extern int performance_info[MAX_PERFORMANCES][PERFORMANCE_INFO_FIELDS];

static int process_bardic_performance_slot_internal(struct char_data *ch, int slot,
                                                    bool check_stutter);

static long get_bardic_performance_source_id(struct char_data *ch)
{
  if (ch->char_specials.performance_source_id == 0)
  {
    ch->char_specials.performance_source_id = next_bardic_performance_source_id;
    if (next_bardic_performance_source_id == LONG_MAX)
      next_bardic_performance_source_id = 1;
    else
      next_bardic_performance_source_id++;
  }

  return ch->char_specials.performance_source_id;
}

static int get_bardic_performer_level(struct char_data *ch)
{
  if (IS_NPC(ch))
    return MAX(1, GET_LEVEL(ch));

  return CLASS_LEVEL(ch, CLASS_BARD);
}

static int find_performance_by_spell(int spellnum)
{
  int i;

  for (i = 0; i < MAX_PERFORMANCES; i++)
    if (performance_info[i][PERFORMANCE_SKILLNUM] == spellnum)
      return i;

  return PERFORMANCE_NONE;
}

struct obj_data *get_equipped_bardic_instrument(struct char_data *ch)
{
  static const int instrument_slots[] = {WEAR_INSTRUMENT, WEAR_HOLD_1, WEAR_HOLD_2, WEAR_HOLD_2H};
  struct obj_data *instrument;
  size_t i;

  if (ch == NULL)
    return NULL;

  for (i = 0; i < sizeof(instrument_slots) / sizeof(instrument_slots[0]); i++)
  {
    instrument = GET_EQ(ch, instrument_slots[i]);
    if (instrument != NULL && GET_OBJ_TYPE(instrument) == ITEM_INSTRUMENT)
      return instrument;
  }

  return NULL;
}

static bool bardic_performance_requires_hearing(int spellnum)
{
  int performance_num;
  int performance_type;

  performance_num = find_performance_by_spell(spellnum);
  if (performance_num == PERFORMANCE_NONE)
    return FALSE;

  performance_type = performance_info[performance_num][PERFORMANCE_TYPE];
  return performance_type != PERFORMANCE_TYPE_ACT && performance_type != PERFORMANCE_TYPE_DANCE;
}

static int bardic_performance_save_level(struct char_data *ch, int effectiveness)
{
  int save_level;

  save_level = get_bardic_performer_level(ch) / 2;
  save_level += GET_CHA_BONUS(ch);
  save_level += effectiveness / 10;

  return MAX(1, save_level);
}

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
   skillnum, ideal instrument, difficulty
 *   performance-type, area of affect, associated feat */
/* NOTE: dont' forget to update MAX_PERFORMANCES in bardic_performance.h */
/* NOTE: dont' forget to add associated feat */
int performance_info[MAX_PERFORMANCES][PERFORMANCE_INFO_FIELDS] = {
    /* 0*/
    {SKILL_SONG_OF_HEALING, INSTRUMENT_LYRE, 4, PERFORMANCE_TYPE_SING, PERFORM_AOE_GROUP,
     FEAT_SONG_OF_HEALING},
    /* 1*/
    {SKILL_DANCE_OF_PROTECTION, INSTRUMENT_DRUM, 5, PERFORMANCE_TYPE_DANCE, PERFORM_AOE_GROUP,
     FEAT_DANCE_OF_PROTECTION},
    /* 2*/
    {SKILL_SONG_OF_FOCUSED_MIND, INSTRUMENT_HARP, 6, PERFORMANCE_TYPE_SING, PERFORM_AOE_GROUP,
     FEAT_SONG_OF_FOCUSED_MIND},
    /* 3*/
    {SKILL_SONG_OF_HEROISM, INSTRUMENT_DRUM, 8, PERFORMANCE_TYPE_SING, PERFORM_AOE_GROUP,
     FEAT_SONG_OF_HEROISM},
    /* 4*/
    {SKILL_ORATORY_OF_REJUVENATION, INSTRUMENT_LYRE, 10, PERFORMANCE_TYPE_ORATORY,
     PERFORM_AOE_GROUP, FEAT_ORATORY_OF_REJUVENATION},
    /* 5*/
    {SKILL_SONG_OF_FLIGHT, INSTRUMENT_HORN, 12, PERFORMANCE_TYPE_SING, PERFORM_AOE_GROUP,
     FEAT_SONG_OF_FLIGHT},
    /* 6*/
    {SKILL_SONG_OF_REVELATION, INSTRUMENT_FLUTE, 14, PERFORMANCE_TYPE_SING, PERFORM_AOE_GROUP,
     FEAT_SONG_OF_REVELATION},
    /* 7*/
    {SKILL_SONG_OF_FEAR, INSTRUMENT_HARP, 16, PERFORMANCE_TYPE_SING, PERFORM_AOE_FOES,
     FEAT_SONG_OF_FEAR},
    /* 8*/
    {SKILL_ACT_OF_FORGETFULNESS, INSTRUMENT_FLUTE, 18, PERFORMANCE_TYPE_ACT, PERFORM_AOE_FOES,
     FEAT_ACT_OF_FORGETFULNESS},
    /* 9*/
    {SKILL_SONG_OF_ROOTING, INSTRUMENT_MANDOLIN, 20, PERFORMANCE_TYPE_SING, PERFORM_AOE_FOES,
     FEAT_SONG_OF_ROOTING},
    /*10*/
    {SKILL_SONG_OF_DRAGONS, INSTRUMENT_HORN, 24, PERFORMANCE_TYPE_SING, PERFORM_AOE_GROUP,
     FEAT_SONG_OF_DRAGONS},
    /*11*/
    {SKILL_SONG_OF_THE_MAGI, INSTRUMENT_MANDOLIN, 29, PERFORMANCE_TYPE_SING, PERFORM_AOE_FOES,
     FEAT_SONG_OF_THE_MAGI},
    /*12*/
    {SKILL_DEAFENING_SONG, INSTRUMENT_DRUM, 20, PERFORMANCE_TYPE_SING, PERFORM_AOE_FOES,
     FEAT_DEAFENING_SONG},
    /*MAX_PERFORMANCES: 13*/
};

static void reset_performance_crescendo(struct char_data *ch)
{
  GET_CRESCENDO_USED(ch) = 0;
  GET_CRESCENDO_DICE(ch) = 0;
  GET_CRESCENDO_DC(ch) = 0;
}

int get_active_bardic_resonant_voice_bonus(struct char_data *ch)
{
  struct affected_type *af;
  int bonus;

  if (ch == NULL)
    return 0;

  bonus = 0;
  for (af = ch->affected; af; af = af->next)
  {
    if (af->source_id != 0 && af->location == APPLY_NONE &&
        af->specific == BARDIC_RESONANT_VOICE_MARKER)
      bonus = MAX(bonus, af->modifier);
  }

  return bonus;
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
  ch->char_specials.performance_source_id = 0;
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

void stop_descriptor_bardic_performances(struct descriptor_data *d)
{
  if (d == NULL)
    return;

  if (d->character != NULL && IS_PERFORMING(d->character))
    stop_bardic_performance(d->character, FALSE);
  if (d->original != NULL && d->original != d->character && IS_PERFORMING(d->original))
    stop_bardic_performance(d->original, FALSE);
}

void handle_bardic_spell_performance(struct char_data *ch)
{
  if (ch == NULL || IS_NPC(ch) || GET_CASTING_CLASS(ch) != CLASS_BARD || !IS_PERFORMING(ch))
    return;

  if (has_bard_harmonic_casting(ch))
  {
    send_to_char(ch, "\tCYour harmonious casting sustains your performance.\tn\r\n");
    return;
  }

  stop_bardic_performance(ch, FALSE);
  send_to_char(ch, "\tRYour spellcasting interrupts your performance!\tn\r\n");
  act("$n's performance falters as $e casts a spell.", TRUE, ch, 0, 0, TO_ROOM);
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

  /* A new performance begins with a verse instead of waiting for the global pulse phase. */
  process_bardic_performance_slot_internal(ch, transition, FALSE);
}

static bool bardic_performance_target_is_eligible(struct char_data *ch, struct char_data *tch,
                                                  int spellnum, long source_id)
{
  if (bardic_performance_requires_hearing(spellnum) && AFF_FLAGGED(tch, AFF_DEAF) &&
      !(spellnum == SKILL_DEAFENING_SONG && affected_by_spell_source(tch, spellnum, source_id)))
    return FALSE;

  if ((spellnum == SKILL_SONG_OF_HEALING || spellnum == SKILL_ORATORY_OF_REJUVENATION) &&
      (IS_GOLEM(tch) || IS_CONSTRUCT(tch)))
  {
    if (ch != tch)
      act("$N's construct form rejects your magical healing.", FALSE, ch, 0, tch, TO_CHAR);
    act("Your construct form rejects the performance's magical healing.", FALSE, ch, 0, tch,
        TO_VICT);
    return FALSE;
  }

  return TRUE;
}

static void engage_bardic_performance_foe(struct char_data *ch, struct char_data *tch)
{
  if (ch == tch)
    return;

  if (GET_POS(ch) > POS_STUNNED && FIGHTING(ch) == NULL)
    set_fighting(ch, tch);
  if (GET_POS(tch) > POS_STUNNED && FIGHTING(tch) == NULL)
    set_fighting(tch, ch);
}

static bool bardic_performance_target_resists(struct char_data *ch, struct char_data *tch,
                                              int spellnum, int effectiveness)
{
  int save_level;
  int save_type;
  int school;

  save_level = bardic_performance_save_level(ch, effectiveness);
  save_type = -1;
  school = NOSCHOOL;

  switch (spellnum)
  {
  case SKILL_SONG_OF_FEAR:
    if (is_immune_fear(ch, tch, TRUE) || is_immune_mind_affecting(ch, tch, TRUE))
      return TRUE;
    save_type = SAVING_WILL;
    school = ENCHANTMENT;
    break;
  case SKILL_ACT_OF_FORGETFULNESS:
    if (is_immune_mind_affecting(ch, tch, TRUE))
      return TRUE;
    save_type = SAVING_WILL;
    school = ENCHANTMENT;
    break;
  case SKILL_SONG_OF_ROOTING:
    save_type = SAVING_REFL;
    school = TRANSMUTATION;
    break;
  case SKILL_DEAFENING_SONG:
    if (!can_deafen(tch))
    {
      act("$N cannot be deafened.", FALSE, ch, 0, tch, TO_CHAR);
      act("You are immune to the deafening performance.", FALSE, ch, 0, tch, TO_VICT);
      return TRUE;
    }
    save_type = SAVING_FORT;
    school = EVOCATION;
    break;
  case SKILL_SONG_OF_THE_MAGI:
    save_type = SAVING_WILL;
    school = ENCHANTMENT;
    break;
  default:
    return FALSE;
  }

  if (!savingthrow(ch, tch, save_type, 0, CAST_INNATE, save_level, school))
    return FALSE;

  act("$N resists your performance.", FALSE, ch, 0, tch, TO_CHAR);
  act("You resist $n's performance.", FALSE, ch, 0, tch, TO_VICT);
  return TRUE;
}

static bool send_bardic_performer_verse_message(struct char_data *ch, int spellnum)
{
  const char *message;

  switch (spellnum)
  {
  case SKILL_SONG_OF_HEALING:
    message = "You sing a verse that soothes wounds.";
    break;
  case SKILL_DANCE_OF_PROTECTION:
    message = "You dance a verse of supernatural protection.";
    break;
  case SKILL_SONG_OF_FLIGHT:
    message = "You sing a verse that lifts the spirit and body.";
    break;
  case SKILL_SONG_OF_HEROISM:
    message = "You sing a verse of swelling heroism.";
    break;
  case SKILL_ORATORY_OF_REJUVENATION:
    message = "You deliver a rejuvenating verse.";
    break;
  case SKILL_ACT_OF_FORGETFULNESS:
    message = "You perform a skit of bewildering forgetfulness.";
    break;
  case SKILL_SONG_OF_REVELATION:
    message = "You sing a verse that reveals the hidden.";
    break;
  case SKILL_SONG_OF_DRAGONS:
    message = "You sing a verse that defies the mightiest dragons.";
    break;
  case SKILL_SONG_OF_FOCUSED_MIND:
    message = "You sing a verse of intense mental focus.";
    break;
  case SKILL_SONG_OF_FEAR:
    message = "You sing a verse meant to strike fear into your foes.";
    break;
  case SKILL_SONG_OF_ROOTING:
    message = "You sing a verse that calls grasping roots.";
    break;
  case SKILL_DEAFENING_SONG:
    message = "You unleash a deafening verse.";
    break;
  case SKILL_SONG_OF_THE_MAGI:
    message = "You sing a verse that unravels magical defenses.";
    break;
  default:
    return FALSE;
  }

  act(message, FALSE, ch, 0, 0, TO_CHAR);
  return TRUE;
}

static void send_bardic_target_verse_message(struct char_data *ch, struct char_data *tch,
                                             int spellnum)
{
  const char *message;

  if (ch == tch)
    return;

  switch (spellnum)
  {
  case SKILL_SONG_OF_HEALING:
    message = "$n's song soothes your wounds.";
    break;
  case SKILL_DANCE_OF_PROTECTION:
    message = "$n's dance envelops you in protection.";
    break;
  case SKILL_SONG_OF_FLIGHT:
    message = "$n's song lifts you into the air.";
    break;
  case SKILL_SONG_OF_HEROISM:
    message = "$n's song fills you with heroism.";
    break;
  case SKILL_ORATORY_OF_REJUVENATION:
    message = "$n's oratory restores your vigor.";
    break;
  case SKILL_ACT_OF_FORGETFULNESS:
    message = "$n's skit drives your purpose from your mind.";
    break;
  case SKILL_SONG_OF_REVELATION:
    message = "$n's song sharpens your senses.";
    break;
  case SKILL_SONG_OF_DRAGONS:
    message = "$n's song steels you against draconic might.";
    break;
  case SKILL_SONG_OF_FOCUSED_MIND:
    message = "$n's song focuses your mind.";
    break;
  case SKILL_SONG_OF_FEAR:
    message = "$n's song fills you with fear.";
    break;
  case SKILL_SONG_OF_ROOTING:
    message = "Roots answer $n's song and coil around you.";
    break;
  case SKILL_DEAFENING_SONG:
    message = "$n's song overwhelms your hearing.";
    break;
  case SKILL_SONG_OF_THE_MAGI:
    message = "$n's song leaves your magical defenses exposed.";
    break;
  default:
    return;
  }

  act(message, FALSE, ch, 0, tch, TO_VICT);
}

/* function for processing individual effects for the performance */
int performance_effects(struct char_data *ch, struct char_data *tch, int spellnum,
                        int effectiveness, int aoe)
{
  int return_val = 1, i = 0;
  int songweaver_bonus;
  int resonant_bonus;
  int healing_amount;
  unsigned int active_affects = 0;
  bool nomessage = FALSE;
  bool removed_existing = FALSE;
  long source_id;
  struct affected_type af[BARD_AFFECTS];
  struct affected_type resonant_af;
  bool resonant_active;

  if (!ch || !tch || find_performance_by_spell(spellnum) == PERFORMANCE_NONE)
    return 0;

  source_id = get_bardic_performance_source_id(ch);

  if (!bardic_performance_target_is_eligible(ch, tch, spellnum, source_id))
    return 0;

  songweaver_bonus = IS_NPC(ch) ? 0 : get_bard_songweaver_level_bonus(ch);
  effectiveness = MIN(MAX_PRFM_EFFECT, effectiveness + songweaver_bonus);

  if (aoe == PERFORM_AOE_FOES)
    engage_bardic_performance_foe(ch, tch);

  if (bardic_performance_target_resists(ch, tch, spellnum, effectiveness))
    return 0;

  if (DEBUG_MODE)
  {
    send_to_char(ch, "performance_effects(): tch: %s, SNum: %d, Effect %d, AoE %d.\r\n",
                 GET_NAME(tch), spellnum, effectiveness, aoe);
  }

  resonant_bonus = 0;
  resonant_active = FALSE;
  new_affect(&resonant_af);

  /* init affect array */
  for (i = 0; i < BARD_AFFECTS; i++)
  {
    new_affect(&(af[i]));

    af[i].spell = spellnum;
    af[i].duration = BARDIC_BASE_AFFECT_ROUNDS + songweaver_bonus;
    af[i].bonus_type = BONUS_TYPE_INHERENT;
    af[i].modifier = 1;
    af[i].location = APPLY_NONE;
  }

  affect_batch_begin(tch);
  if (affected_by_spell_source(tch, spellnum, source_id))
  {
    nomessage = TRUE;
    removed_existing = TRUE;
    /* purpose: refresh song duration */
    affect_from_char_source(tch, spellnum, source_id);
  }

  switch (spellnum)
  {
  case SKILL_SONG_OF_HEALING:
    healing_amount = rand_number(effectiveness, effectiveness * 2 + 10);
    process_healing(ch, tch, SKILL_SONG_OF_HEALING, healing_amount, 0, 0);
    break;

  case SKILL_DANCE_OF_PROTECTION:
    af[0].location = APPLY_AC_NEW;
    af[0].modifier = (effectiveness + 1) / 7;

    af[1].location = APPLY_SAVING_WILL;
    af[1].modifier = effectiveness / 6;

    af[2].location = APPLY_DR;
    af[2].modifier = effectiveness / 13;
    active_affects = (1U << 3) - 1;

    break;

  case SKILL_SONG_OF_HEROISM:
    af[0].location = APPLY_HITROLL;
    af[0].modifier = 1 + effectiveness / 10;

    af[1].location = APPLY_DAMROLL;
    af[1].modifier = effectiveness / 10 + get_bard_battle_hymn_damage_bonus(ch) +
                     get_bard_battle_hymn_ii_damage_bonus(ch);

    af[2].location = APPLY_STR;
    af[2].modifier = effectiveness / 10;

    af[3].location = APPLY_DEX;
    af[3].modifier = effectiveness / 10;

    af[4].location = APPLY_CON;
    af[4].modifier = effectiveness / 10;

    if (get_bardic_performer_level(ch) >= 10)
    {
      if (!nomessage && !AFF_FLAGGED(tch, AFF_HASTE))
        act("You feel the world slow down around you.", FALSE, tch, 0, 0, TO_CHAR);
      SET_BIT_AR(af[1].bitvector, AFF_HASTE);
    }
    active_affects = (1U << 5) - 1;
    if (has_bard_warchanters_dominance(ch))
    {
      af[0].modifier += get_bard_warchanters_dominance_tohit_bonus(ch);
      af[5].location = APPLY_AC_NEW;
      af[5].modifier = get_bard_warchanters_dominance_ac_bonus(ch);
      active_affects |= 1U << 5;
    }
    break;

  case SKILL_ORATORY_OF_REJUVENATION:
    if (GET_HIT(tch) < GET_MAX_HIT(tch))
    {
      healing_amount = rand_number(effectiveness / 3, effectiveness / 2);
      process_healing(ch, tch, SKILL_ORATORY_OF_REJUVENATION, healing_amount, 0, 0);
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
    af[0].location = APPLY_HITROLL;
    af[0].modifier = 0;
    SET_BIT_AR(af[0].bitvector, AFF_DETECT_INVIS);
    active_affects |= 1U << 0;
    if (get_bardic_performer_level(ch) >= 5)
    {
      af[1].location = APPLY_DAMROLL;
      af[1].modifier = 0;
      SET_BIT_AR(af[1].bitvector, AFF_DETECT_ALIGN);
      active_affects |= 1U << 1;
    }
    if (get_bardic_performer_level(ch) >= 10)
    {
      af[2].location = APPLY_AC;
      af[2].modifier = 0;
      SET_BIT_AR(af[2].bitvector, AFF_DETECT_MAGIC);
      active_affects |= 1U << 2;
    }
    if (get_bardic_performer_level(ch) >= 15)
    {
      af[3].location = APPLY_DEX;
      af[3].modifier = 0;
      SET_BIT_AR(af[3].bitvector, AFF_SENSE_LIFE);
      active_affects |= 1U << 3;
    }
    if (get_bardic_performer_level(ch) >= 20)
    {
      af[4].location = APPLY_AGE;
      af[4].modifier = 0;
      SET_BIT_AR(af[4].bitvector, AFF_FARSEE);
      active_affects |= 1U << 4;
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
    active_affects = (1U << BARD_AFFECTS) - 1;

    break;

  case SKILL_ACT_OF_FORGETFULNESS:
    if (IS_NPC(tch))
      clearMemory(tch);

    if (FIGHTING(tch))
      stop_fighting(tch);
    if (FIGHTING(ch) == tch)
      stop_fighting(ch);
    break;

  case SKILL_SONG_OF_FLIGHT:
    af[0].location = APPLY_SPECIAL;
    SET_BIT_AR(af[0].bitvector, AFF_FLYING);
    active_affects |= 1U << 0;
    if (!nomessage && !AFF_FLAGGED(tch, AFF_FLYING))
      act("You fly through the air, free as a bird!", FALSE, tch, 0, 0, TO_CHAR);
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
    active_affects = (1U << 3) - 1;

    /* using affected_by_spell() for memorization bonus */

    break;

  /* enemy fight less effective / flee */
  case SKILL_SONG_OF_FEAR:
    SET_BIT_AR(af[0].bitvector, AFF_FEAR);
    af[0].location = APPLY_HITROLL;
    af[0].modifier = -(1 + effectiveness / 10);
    active_affects |= 1U << 0;
    break;

  /* enemy fight less effective / entangled */
  case SKILL_SONG_OF_ROOTING:
    SET_BIT_AR(af[0].bitvector, AFF_ENTANGLED);
    af[0].location = APPLY_DAMROLL;
    af[0].modifier = -effectiveness / 5;

    SET_BIT_AR(af[1].bitvector, AFF_SLOW);
    af[1].location = APPLY_AC_NEW;
    af[1].modifier = -effectiveness / 9;
    active_affects |= (1U << 2) - 1;
    break;

  case SKILL_DEAFENING_SONG:
    SET_BIT_AR(af[0].bitvector, AFF_DEAF);
    af[0].location = APPLY_AC_NEW;
    af[0].modifier = -effectiveness / 5;
    active_affects |= 1U << 0;
    break;

  /* enemy spell resistance / saves reduced */
  case SKILL_SONG_OF_THE_MAGI:
    af[0].location = APPLY_SAVING_WILL;
    af[0].modifier = -(1 + effectiveness / 4);

    af[1].location = APPLY_SPELL_RES;
    af[1].modifier = -(2 + effectiveness / 10);

    af[2].location = APPLY_INT;
    af[2].modifier = -(1 + effectiveness / 7);

    af[3].location = APPLY_WIS;
    af[3].modifier = -(1 + effectiveness / 7);

    af[4].location = APPLY_CHA;
    af[4].modifier = -(1 + effectiveness / 7);
    active_affects |= (1U << 5) - 1;
    break;

  /* UH OH! */
  default:
    log("SYSERR: performance_effects reached default case! "
        "(spellnum: %d)",
        spellnum);
    return_val = 0;
    break;

  } /* end switch */

  /* Resonant Voice adds its own source-owned competence affect. */
  if (!IS_NPC(ch) && aoe == PERFORM_AOE_GROUP)
  {
    resonant_bonus = get_bard_resonant_voice_save_bonus(ch);
    if (resonant_bonus > 0)
    {
      resonant_af.spell = spellnum;
      resonant_af.duration = BARDIC_BASE_AFFECT_ROUNDS + songweaver_bonus;
      resonant_af.location = APPLY_NONE;
      resonant_af.modifier = resonant_bonus;
      resonant_af.bonus_type = BONUS_TYPE_COMPETENCE;
      resonant_af.specific = BARDIC_RESONANT_VOICE_MARKER;
      resonant_active = TRUE;
    }
  }

  /* Apply only meaningful affect records and preserve effects from other performers. */
  for (i = 0; i < BARD_AFFECTS; i++)
  {
    if (!(active_affects & (1U << i)))
      continue;

    /* lingering song bonus */
    if (HAS_FEAT(ch, FEAT_LINGERING_SONG))
      af[i].duration += BARDIC_LINGERING_AFFECT_ROUNDS;

    /* attach the affections! */
    affect_join_source(tch, af + i, source_id, FALSE, FALSE, FALSE, FALSE);
  }
  if (resonant_active)
  {
    if (HAS_FEAT(ch, FEAT_LINGERING_SONG))
      resonant_af.duration += BARDIC_LINGERING_AFFECT_ROUNDS;
    affect_join_source(tch, &resonant_af, source_id, FALSE, FALSE, FALSE, FALSE);
  }
  affect_batch_end(tch);
  if (removed_existing)
    update_pos(tch);

  if (return_val)
    send_bardic_target_verse_message(ch, tch, spellnum);

  return return_val;
}

/* main function for performance effects / message / targets / etc */
int process_performance(struct char_data *ch, int performance_num, int effectiveness, int aoe)
{
  struct char_data *tch = NULL, *tch_next = NULL;
  struct iterator_data iterator;
  bool hit_self = FALSE, hit_leader = FALSE;

  if (ch == NULL || IN_ROOM(ch) == NOWHERE)
    return 0;

  if (DEBUG_MODE)
  {
    send_to_char(ch, "process_performance(): PNum: %d, Effect %d, AoE %d.\r\n", performance_num,
                 effectiveness, aoe);
  }

  if (!send_bardic_performer_verse_message(ch, performance_num))
  {
    log("SYSERR: messages in process_performance reached default case! "
        "(performance_num: %d)",
        performance_num);
    return 0;
  }

  /* Apply the verse only to recipients selected by its area contract. */
  switch (aoe)
  {
  case PERFORM_AOE_GROUP:
    if (GROUP(ch) && GROUP(ch)->members && GROUP(ch)->members->iSize)
    {
      for (tch = (struct char_data *)merge_iterator(&iterator, GROUP(ch)->members); tch != NULL;
           tch = (struct char_data *)next_in_list(&iterator))
      {
        if (IN_ROOM(tch) != IN_ROOM(ch))
          continue;

        if (tch == ch)
          hit_self = TRUE;
        if (GROUP(ch)->leader == tch)
          hit_leader = TRUE;

        performance_effects(ch, tch, performance_num, effectiveness, aoe);
      }
      remove_iterator(&iterator);
    }

    if (!hit_self)
    {
      performance_effects(ch, ch, performance_num, effectiveness, aoe);
      if (GROUP(ch) && ch == GROUP(ch)->leader)
        hit_leader = TRUE;
    }

    if (GROUP(ch) && !hit_leader && GROUP(ch)->leader && IN_ROOM(GROUP(ch)->leader) == IN_ROOM(ch))
      performance_effects(ch, GROUP(ch)->leader, performance_num, effectiveness, aoe);
    break;

  case PERFORM_AOE_FOES:
    for (tch = world[ch->in_room].people; tch; tch = tch_next)
    {
      tch_next = tch->next_in_room;
      if (aoeOK(ch, tch, performance_num))
        performance_effects(ch, tch, performance_num, effectiveness, aoe);
    }
    break;

  case PERFORM_AOE_ROOM:
    for (tch = world[ch->in_room].people; tch; tch = tch_next)
    {
      tch_next = tch->next_in_room;
      performance_effects(ch, tch, performance_num, effectiveness, aoe);
    }
    break;

  default:
    log("SYSERR: aoe-switch in process_performance reached default case! "
        "(performance_num: %d)",
        performance_num);
    return 0;
  }

  return 1;
}

/* This is the primary engine for one active performance slot. */
static int process_bardic_performance_slot_internal(struct char_data *ch, int slot,
                                                    bool check_stutter)
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
    log("SYSERR: process_bardic_performance_slot received invalid slot %d", slot);
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

  instrument = get_equipped_bardic_instrument(ch);

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
  if (check_stutter && !rand_number(0, 1) && rand_number(1, 101) < difficulty)
  {
    send_to_char(ch, "Uh oh.. how did the performance go, anyway?\r\n");
    act("$n stutters in the performance!", TRUE, ch, 0, 0, TO_ROOM);
    stop_bardic_performance_slot(ch, slot, FALSE);
    return 0;
  }

  /* success, the next verse arrives on the global performance pulse */
  return 1;
}

int process_bardic_performance_slot(struct char_data *ch, int slot)
{
  return process_bardic_performance_slot_internal(ch, slot, TRUE);
}

static void pulse_bard_winters_war_march(struct char_data *ch)
{
  struct char_data *tch;
  struct char_data *next_tch;
  struct affected_type af;
  int damage_dice;
  int save_level;

  if (ch == NULL || IN_ROOM(ch) == NOWHERE || !has_bard_winters_war_march(ch))
    return;

  damage_dice = get_bard_winters_war_march_damage(ch);
  save_level = MAX(1, CLASS_LEVEL(ch, CLASS_BARD) / 2 + GET_CHA_BONUS(ch));
  if (damage_dice <= 0)
    return;

  send_to_char(ch, "\tCYour winter war march surges through the room!\tn\r\n");
  for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch)
  {
    int cold_damage;
    int slow_duration;
    bool saved;

    next_tch = tch->next_in_room;
    if (tch == ch || !aoeOK(ch, tch, -1))
      continue;

    saved = savingthrow_full(ch, tch, SAVING_FORT, 0, CAST_INNATE, save_level, EVOCATION,
                             AFFECT_BARD_WINTERS_WAR_MARCH);
    cold_damage = dice(damage_dice, 6);
    slow_duration = saved ? 1 : 3;
    if (saved)
      cold_damage /= 2;

    new_affect(&af);
    af.spell = AFFECT_BARD_WINTERS_WAR_MARCH;
    af.duration = slow_duration;
    SET_BIT_AR(af.bitvector, AFF_SLOW);
    affect_join(tch, &af, FALSE, FALSE, FALSE, FALSE);

    if (saved)
      act("You partially resist $n's frigid march, but its rhythm still slows you.", FALSE, ch, 0,
          tch, TO_VICT);
    else
      act("$n's frigid march freezes and slows you!", FALSE, ch, 0, tch, TO_VICT);
    damage(ch, tch, cold_damage, AFFECT_BARD_WINTERS_WAR_MARCH, DAM_COLD, FALSE);
  }
}

static void pulse_bard_symphonic_resonance(struct char_data *ch)
{
  int temp_hp_dice;
  int temp_hp;
  int available;

  if (ch == NULL || IS_NPC(ch) || !IS_PERFORMING(ch) || !has_bard_symphonic_resonance(ch))
    return;

  temp_hp_dice = get_bard_symphonic_resonance_temp_hp(ch);
  available = GET_MAX_HIT(ch) + BARDIC_SYMPHONIC_TEMP_HP_CAP - GET_HIT(ch);
  temp_hp = MIN(MAX(0, available), dice(temp_hp_dice, 6));
  if (temp_hp > 0)
  {
    GET_HIT(ch) += temp_hp;
    send_to_char(ch, "\tCYour symphonic resonance grants you %d temporary HP.\tn\r\n", temp_hp);
  }
}

static void pulse_bard_endless_refrain(struct char_data *ch)
{
  int slot_regen;
  int recovered;
  int i;

  if (ch == NULL || IS_NPC(ch) || !IS_PERFORMING(ch) || !has_bard_endless_refrain(ch))
    return;

  slot_regen = get_bard_endless_refrain_slot_regen(ch);
  recovered = 0;
  for (i = 0; i < slot_regen; i++)
  {
    if (sustain_melody_recover_one_slot(ch, CLASS_BARD))
      recovered++;
  }
  if (recovered > 0)
    send_to_char(ch, "\tCYour endless refrain recovers %d Bard spell slot%s.\tn\r\n", recovered,
                 recovered == 1 ? "" : "s");
}

#ifdef LUMINARI_CUTEST
void test_pulse_bard_winters_war_march(struct char_data *ch)
{
  pulse_bard_winters_war_march(ch);
}

void test_pulse_bard_symphonic_resonance(struct char_data *ch)
{
  pulse_bard_symphonic_resonance(ch);
}

void test_pulse_bard_endless_refrain(struct char_data *ch)
{
  pulse_bard_endless_refrain(ch);
}

int test_process_bardic_performance_slot_without_stutter(struct char_data *ch, int slot)
{
  return process_bardic_performance_slot_internal(ch, slot, FALSE);
}
#endif

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

    process_bardic_performance_slot(ch, PERFORMANCE_VAR_PRIMARY);

    if (IS_PERFORMING(ch) && GET_SECONDARY_PERFORMING(ch) != PERFORMANCE_NONE)
      process_bardic_performance_slot(ch, PERFORMANCE_VAR_SECONDARY);

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

    /* Tier 4 Warchanter: room-wide cold damage and slow once per verse. */
    if (!IS_NPC(ch) && IS_PERFORMING(ch))
      pulse_bard_winters_war_march(ch);

    /* Tier 4 Spellsinger: Symphonic Resonance - grant temporary HP each verse. */
    pulse_bard_symphonic_resonance(ch);

    /* Tier 4 Spellsinger: Endless Refrain - recover one Bard slot each verse. */
    pulse_bard_endless_refrain(ch);
  }

  return;
}

/* EOF */
