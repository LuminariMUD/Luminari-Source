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
#include "mud_event.h"
#include "db.h"
#include "spells.h"
#include "bardic_performance.h"
#include "fight.h"
#include "spec_procs.h"
#include "actions.h"
#include "feats.h"

/* defines */
#define DEBUG_MODE FALSE
/* this will determine whether the system is ran through events or the tick system */
/* #define EVENT_RAN */

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
    {SKILL_SONG_OF_HEALING, INSTRUMENT_LYRE, SKILL_LYRE, 4,
     PERFORMANCE_TYPE_SING, PERFORM_AOE_GROUP, FEAT_SONG_OF_HEALING},
    /* 1*/
    {SKILL_DANCE_OF_PROTECTION, INSTRUMENT_DRUM, SKILL_DRUM, 5,
     PERFORMANCE_TYPE_DANCE, PERFORM_AOE_GROUP, FEAT_DANCE_OF_PROTECTION},
    /* 2*/
    {SKILL_SONG_OF_FOCUSED_MIND, INSTRUMENT_HARP, SKILL_HARP, 6,
     PERFORMANCE_TYPE_SING, PERFORM_AOE_GROUP, FEAT_SONG_OF_FOCUSED_MIND},
    /* 3*/
    {SKILL_SONG_OF_HEROISM, INSTRUMENT_DRUM, SKILL_DRUM, 8,
     PERFORMANCE_TYPE_SING, PERFORM_AOE_GROUP, FEAT_SONG_OF_HEROISM},
    /* 4*/
    {SKILL_ORATORY_OF_REJUVENATION, INSTRUMENT_LYRE, SKILL_LYRE, 10,
     PERFORMANCE_TYPE_ORATORY, PERFORM_AOE_GROUP, FEAT_ORATORY_OF_REJUVENATION},
    /* 5*/
    {SKILL_SONG_OF_FLIGHT, INSTRUMENT_HORN, SKILL_HORN, 12,
     PERFORMANCE_TYPE_SING, PERFORM_AOE_GROUP, FEAT_SONG_OF_FLIGHT},
    /* 6*/
    {SKILL_SONG_OF_REVELATION, INSTRUMENT_FLUTE, SKILL_FLUTE, 14,
     PERFORMANCE_TYPE_SING, PERFORM_AOE_GROUP, FEAT_SONG_OF_REVELATION},
    /* 7*/
    {SKILL_SONG_OF_FEAR, INSTRUMENT_HARP, SKILL_HARP, 16,
     PERFORMANCE_TYPE_SING, PERFORM_AOE_FOES, FEAT_SONG_OF_FEAR},
    /* 8*/
    {SKILL_ACT_OF_FORGETFULNESS, INSTRUMENT_FLUTE, SKILL_FLUTE, 18,
     PERFORMANCE_TYPE_ACT, PERFORM_AOE_FOES, FEAT_ACT_OF_FORGETFULNESS},
    /* 9*/
    {SKILL_SONG_OF_ROOTING, INSTRUMENT_MANDOLIN, SKILL_MANDOLIN, 20,
     PERFORMANCE_TYPE_SING, PERFORM_AOE_FOES, FEAT_SONG_OF_ROOTING},
    /*10*/
    {SKILL_SONG_OF_DRAGONS, INSTRUMENT_HORN, SKILL_HORN, 24,
     PERFORMANCE_TYPE_SING, PERFORM_AOE_GROUP, FEAT_SONG_OF_DRAGONS},
    /*11*/
    {SKILL_SONG_OF_THE_MAGI, INSTRUMENT_MANDOLIN, SKILL_MANDOLIN, 29,
     PERFORMANCE_TYPE_SING, PERFORM_AOE_FOES, FEAT_SONG_OF_THE_MAGI},
    /*12*/
    {SKILL_DEAFENING_SONG, INSTRUMENT_DRUM, SKILL_DRUM, 20,
     PERFORMANCE_TYPE_SING, PERFORM_AOE_FOES, FEAT_DEAFENING_SONG},
    /*MAX_PERFORMANCES: 13*/
};

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

/* will list to the performer which performances are available to them */
void list_available_performances(struct char_data *ch)
{
  int i = 0;

  send_to_char(ch, "Available performances:\r\n");
  for (i = 0; i < NUM_FEATS; i++)
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
    send_to_char(ch, "can_perform(): PNum: %d, NeedCheck %d, Silent %d.\r\n", performance_num, need_check, silent);
  }

  /* check for disqualifiers */

  if (!is_valid_performance(performance_num))
  {
    if (!silent)
      send_to_char(ch, "(%d) is an invalid performance number.  Please report this to staff.\r\n", performance_num);
    return 0;
  }

  if (!IS_NPC(ch) && !HAS_FEAT(ch, FEAT_BARDIC_MUSIC))
  {
    if (!silent)
      send_to_char(ch, "You don't know how to perform.\r\n");
    return 0;
  }

  if (char_has_mud_event(ch, ePERFORM)) /* OLD perform, this is a dummy check -zusuk */
  {
    if (!silent)
      send_to_char(ch, "You are already performing!\r\n");
    return 0;
  }

#ifdef EVENT_RAN
  if (need_check && char_has_mud_event(ch, eBARDIC_PERFORMANCE))
  {
    if (!silent)
      send_to_char(ch, "You are already in the middle of a performance!\r\n");
    return 0;
  }
#else
  if (need_check && IS_PERFORMING(ch))
  {
    if (!silent)
      send_to_char(ch, "You are already in the middle of a performance!\r\n");
    return 0;
  }
#endif

  if (((ch->in_room != NOWHERE && ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF)) || AFF_FLAGGED(ch, AFF_SILENCED)) &&
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
#ifdef EVENT_RAN
      if (IN_ROOM(vict) != NOWHERE && vict != ch && (char_has_mud_event(vict, ePERFORM) || char_has_mud_event(vict, eBARDIC_PERFORMANCE)))
#else
      if (IN_ROOM(vict) != NOWHERE && vict != ch && (char_has_mud_event(vict, ePERFORM) || IS_PERFORMING(vict)))
#endif
      {
        if (!silent)
          send_to_char(ch, "Your bardic performance conflicts with %s and is abrupted!\r\n", GET_NAME(vict));
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
  int performance_num = -1;
  int len = 0;

  if (!argument || (len = strlen(argument)) == 0)
  {

#ifdef EVENT_RAN
    if (char_has_mud_event(ch, eBARDIC_PERFORMANCE))
    {
      event_cancel_specific(ch, eBARDIC_PERFORMANCE);
      act("You stopped your performance.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n stops performing.", TRUE, ch, 0, 0, TO_ROOM);
      return;
    }
#else
    if (IS_PERFORMING(ch))
    {
      IS_PERFORMING(ch) = FALSE;
      act("You stopped your performance.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n stops performing.", TRUE, ch, 0, 0, TO_ROOM);
      return;
    }
#endif
    else
    {
      send_to_char(ch, "Play what performance?\r\n");
      list_available_performances(ch);
      return;
    }
  }

#ifdef EVENT_RAN
  if (char_has_mud_event(ch, eBARDIC_PERFORMANCE))
  {
    act("You stopped your current performance.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n stops performing...", TRUE, ch, 0, 0, TO_ROOM);
    event_cancel_specific(ch, eBARDIC_PERFORMANCE);
  }
#else
  if (IS_PERFORMING(ch))
  {
    IS_PERFORMING(ch) = FALSE;
    act("You stop your current performance.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n stops performing...", TRUE, ch, 0, 0, TO_ROOM);
  }
#endif

  skip_spaces_c(&argument);
  len = strlen(argument);

  for (performance_num = 0; performance_num < MAX_PERFORMANCES; performance_num++)
  {
    if (!strncmp(argument, skill_name(performance_info[performance_num][PERFORMANCE_SKILLNUM]), len))
    {
      if (!HAS_FEAT(ch, performance_info[performance_num][PERFORMANCE_FEATNUM]))
      {
        send_to_char(ch, "But you do not know that performance!\r\n");
        return;
      }
      else
      {
        /* check for disqualifiers */
        if (!can_perform(ch, performance_num, TRUE, FALSE))
        {
          /* we DO check if they have a bardic_performanc event here represented by the first TRUE */
          /* the messages were sent via the last FALSE in can_perform()! */
          return;
        }

        /* SUCCESS! */
        act("You start performing.", FALSE, ch, 0, 0, TO_CHAR);
        act("$n starts performing.", TRUE, ch, 0, 0, TO_ROOM);

#ifdef EVENT_RAN
        char buf[128];
        snprintf(buf, sizeof(buf), "%d", i); /* Build the effect string */

        NEW_EVENT(eBARDIC_PERFORMANCE, ch, buf, 4 * PASSES_PER_SEC);
#else
        IS_PERFORMING(ch) = TRUE;
        GET_PERFORMING(ch) = performance_num;
#endif

        if (HAS_FEAT(ch, FEAT_EFFICIENT_PERFORMANCE))
          USE_MOVE_ACTION(ch);
        else
          USE_STANDARD_ACTION(ch);

        return;
      }
    }
  }

  send_to_char(ch, "But that is not a performance!\r\n");
  list_available_performances(ch);
  return;
}

/* function for processing individual effects for the performance */
int performance_effects(struct char_data *ch, struct char_data *tch, int spellnum, int effectiveness, int aoe)
{
  int return_val = 1, i = 0; /* return_val is 1, very limited reasons to fail here! */
  bool nomessage = FALSE, engage = TRUE;
  struct affected_type af[BARD_AFFECTS];

  if (!ch)
    return 0;

  if (!tch)
    return 0;

  if (DEBUG_MODE)
  {
    send_to_char(ch, "performance_effects(): tch: %s, SNum: %d, Effect %d, AoE %d.\r\n", GET_NAME(tch), spellnum, effectiveness, aoe);
  }

  /* init affect array */
  for (i = 0; i < BARD_AFFECTS; i++)
  {
    new_affect(&(af[i]));

    af[i].spell = spellnum;
    af[i].duration = 3;
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
    process_healing(ch, tch, SKILL_SONG_OF_HEALING, rand_number(effectiveness, effectiveness * 2 + 10), 0, 0);
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
      act("You feel the world slow down around you.", FALSE, tch, 0, 0,
          TO_CHAR);
      act("$n starts to move with uncanny speed.", TRUE, tch, 0, 0,
          TO_ROOM);
    }
    break;

  case SKILL_ORATORY_OF_REJUVENATION:
    if (GET_HIT(tch) < GET_MAX_HIT(tch))
    {
      send_to_char(tch, "You are soothed by the power of music!\r\n");
      process_healing(ch, tch, SKILL_ORATORY_OF_REJUVENATION, rand_number(effectiveness / 3, effectiveness / 2), 0, 0);
    }

    process_healing(ch, tch, SKILL_ORATORY_OF_REJUVENATION, 0, rand_number(effectiveness * 40, effectiveness * 60 + 60), 0);

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
    process_healing(ch, tch, SKILL_SONG_OF_FLIGHT, 0, rand_number(50, effectiveness * 10 / 3 + 50), 0);
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
    if (!IS_NPC(tch) && HAS_FEAT(tch, FEAT_RP_FEARLESS_RAGE) &&
        affected_by_spell(tch, SKILL_RAGE))
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
    send_to_char(ch, "process_performance(): PNum: %d, Effect %d, AoE %d.\r\n", performance_num, effectiveness, aoe);
  }

  struct char_data *tch = NULL, *tch_next = NULL;
  int return_val = 1;
  bool hit_self = FALSE, hit_leader = FALSE;

  /* performance message */
  switch (performance_num)
  {

  case SKILL_SONG_OF_HEALING:
    act("You sing a song to heal all wounds.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n sings a song so well you feel your pain and suffering ebbing away.", TRUE,
        ch, 0, 0, TO_ROOM);
    break;

  case SKILL_DANCE_OF_PROTECTION:
    act("You dance to protect yourself from harm.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n performs a dance that envelops you in protection.", TRUE, ch, 0, 0,
        TO_ROOM);
    break;

  case SKILL_SONG_OF_FLIGHT:
    act("You sing a song that lifts the spirits high.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n sings a song that lifts the spirits high.", TRUE, ch, 0, 0, TO_ROOM);
    break;

  case SKILL_SONG_OF_HEROISM:
    act("You sing a song that makes your heart swell with pride.", FALSE, ch, 0, 0,
        TO_CHAR);
    act("$n sings a song that makes your heart swell with pride.", TRUE, ch, 0, 0,
        TO_ROOM);
    break;

  case SKILL_ORATORY_OF_REJUVENATION:
    act("You conduct an oratory to rejuvenate the exhausted.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n conducts an oratory which eases some of your exhaustion.", TRUE, ch,
        0, 0, TO_ROOM);
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
    act("You sing a song that defies the mightiest of dragons.", FALSE, ch, 0, 0,
        TO_CHAR);
    act("$n sings a song that defies the mightiest of dragons, inspiring you to truly heroic deeds!", TRUE, ch, 0, 0,
        TO_ROOM);
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
    act("You sing a song so well, that magic in itself feels strengthened by it.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n sings a song which makes you forget completely about hostile magic.", TRUE, ch, 0, 0, TO_ROOM);
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

      while ((tch = (struct char_data *)simple_list(GROUP(ch)->members)) !=
             NULL)
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
        return_val = performance_effects(ch, GROUP(ch)->leader, performance_num, effectiveness, aoe);
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
int bardic_performance_engine(struct char_data *ch, int performance_num)
{
  struct obj_data *instrument = NULL;
  int effectiveness = 0;
  int spellnum = -1;
  int difficulty = 0;

  /* disqualifiers */
  if (!can_perform(ch, performance_num, FALSE, FALSE))
  {
    /* we don't check if they have a bardic_performanc event here represented by the first FALSE */
    /* the messages were sent via the last FALSE in can_perform()! */
    GET_PERFORMING(ch) = -1;
    IS_PERFORMING(ch) = FALSE;
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
      act("Your $p cannot take the strain of magic any longer, and it breaks!", FALSE, ch, instrument, 0, TO_CHAR);
      act("$n's $p cannot take the strain of magic any longer, and it breaks!", TRUE, ch, instrument, 0, TO_ROOM);
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
    send_to_char(ch, "Your performance is slightly hindered as you are concentrating on the combat.\r\n");
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
    GET_PERFORMING(ch) = -1;
    IS_PERFORMING(ch) = FALSE;
    return 0; /* process performance failed somehow */
  }

  /* check for stutter. if stutter, stop performance  */
  if (!rand_number(0, 1) && rand_number(1, 101) < difficulty)
  {
    send_to_char(ch, "Uh oh.. how did the performance go, anyway?\r\n");
    act("$n stutters in the performance!", TRUE, ch, 0, 0, TO_ROOM);
    GET_PERFORMING(ch) = -1;
    IS_PERFORMING(ch) = FALSE;
    return 0;
  }

  /* success, we're coming back in VERSE_INTERVAL */
  return 1;
}

/* this is the event called every verse-interval that carries the char_data and performance_num */
EVENTFUNC(event_bardic_performance)
{
#ifdef EVENT_RAN
  struct mud_event_data *pMudEvent = NULL;
  struct char_data *ch = NULL;
  int performance_num = -1;

  /* start handling the event data */
  pMudEvent = (struct mud_event_data *)event_obj;

  if (!pMudEvent)
  {
    log("SYSERR: event_bardic_performance missing pMudEvent!");
    return 0;
  }

  if (!pMudEvent->iId)
  {
    log("SYSERR: event_bardic_performance missing pMudEvent->iId!");
    return 0;
  }

  /* extracted the character */
  ch = (struct char_data *)pMudEvent->pStruct;

  if (!ch)
  {
    log("SYSERR: event_bardic_performance missing pMudEvent->pStruct!");
    return 0;
  }

  /* extract the variable(s) */
  if (pMudEvent->sVariables == NULL)
  {
    /* This is odd - This field should always be populated! */
    log("SYSERR: sVariables field is NULL for event_bardic_performance: %d",
        pMudEvent->iId);
    return 0;
  }
  else
  {
    performance_num = atoi((char *)pMudEvent->sVariables);
  }
  /* finished handling event data */

  /* this is the main engine */
  if (bardic_performance_engine(ch, performance_num))
    return VERSE_INTERVAL;

#else
  /* we didn't survive the journey through the code! */
  return 0;
}

/* this is a very basic function to go through connected players to see if anyone is performing */
void pulse_bardic_performance()
{
  struct descriptor_data *pt = NULL;

  /* we are going to cycle through the online players to find performers */
  for (pt = descriptor_list; pt; pt = pt->next)
  {
    if (IS_PLAYING(pt) && pt->character &&
        IS_PERFORMING(pt->character) && (GET_PERFORMING(pt->character) >= 0)) /* GET_PERFORMING: -1 is no perform/fail */
    {
      /* this is the main engine */
      (bardic_performance_engine(pt->character, GET_PERFORMING(pt->character)));
    }
  }

  return;
}
#endif

/* this will determine whether the system is ran through events or the tick system */
#undef EVENT_RAN

  /* EOF */
