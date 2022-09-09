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
    /*MAX_PERFORMANCES: 12*/
};

/* local functions for modifying chars points (hitpoints or moves)
 * note: negative (-) is healing */
static void alter_hit(struct char_data *ch, int points, bool unused)
{
  GET_HIT(ch) -= points;
  GET_HIT(ch) = MIN(GET_HIT(ch), GET_MAX_HIT(ch));
  update_pos(ch);
}
static void alter_move(struct char_data *ch, int points)
{
  GET_MOVE(ch) -= points;
  GET_MOVE(ch) = MIN(GET_MOVE(ch), GET_MAX_MOVE(ch));
  update_pos(ch);
}

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

/* primary command entry point for the bardic performance */
ACMD(do_perform)
{
  int i;
  int len = 0;

  if (!argument || (len = strlen(argument)) == 0)
  {
    if (char_has_mud_event(ch, eBARDIC_PERFORMANCE))
    {
      event_cancel_specific(ch, eBARDIC_PERFORMANCE);
      act("You stopped your performance.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n stops performing.", FALSE, ch, 0, 0, TO_ROOM);
      return;
    }
    else
    {
      send_to_char(ch, "Play what performance?\r\n");
      list_available_performances(ch);
      return;
    }
  }

  if (char_has_mud_event(ch, eBARDIC_PERFORMANCE))
  {
    act("You stopped your current performance.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n stops performing...", FALSE, ch, 0, 0, TO_ROOM);
    event_cancel_specific(ch, eBARDIC_PERFORMANCE);
  }

  skip_spaces_c(&argument);
  len = strlen(argument);

  for (i = 0; i < MAX_PERFORMANCES; i++)
  {
    if (!strncmp(argument, skill_name(performance_info[i][PERFORMANCE_SKILLNUM]), len))
    {
      if (!HAS_FEAT(ch, performance_info[i][PERFORMANCE_FEATNUM]))
      {
        send_to_char(ch, "But you do not know that performance!\r\n");
        return;
      }
      else
      {
        /* check for disqualifiers */
        if (!IS_NPC(ch) && !HAS_FEAT(ch, FEAT_BARDIC_MUSIC))
        {
          send_to_char(ch, "You don't know how to perform.\r\n");
          return;
        }
        if (char_has_mud_event(ch, ePERFORM))
        {
          send_to_char(ch, "You are already performing!\r\n");
          return;
        }
        if (char_has_mud_event(ch, eBARDIC_PERFORMANCE))
        {
          send_to_char(ch, "You are already in the middle of a performance!\r\n");
          return;
        }
        if (compute_ability(ch, ABILITY_PERFORM) < performance_info[i][PERFORMANCE_DIFF])
        {
          send_to_char(ch, "You are not trained enough for this performance! "
                           "(need: %d 'perform' ability)\r\n",
                       performance_info[i][PERFORMANCE_DIFF]);
          return;
        }
        if (((ch->in_room != NOWHERE && ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF)) || AFF_FLAGGED(ch, AFF_SILENCED)) &&
            (performance_info[i][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_KEYBOARD ||
             performance_info[i][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_ORATORY ||
             performance_info[i][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_PERCUSSION ||
             performance_info[i][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_STRING ||
             performance_info[i][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_WIND ||
             performance_info[i][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_SING))
        {
          send_to_char(ch, "The silence effectively stops your performance.\r\n");
          return;
        }
        if (GET_POS(ch) < POS_FIGHTING)
        {
          send_to_char(ch, "You can't concentrate on your performance when you are in "
                           "this position.\r\n");
          return;
        }

        /* this isn't necessary */
        /*
        if (performance_info[i][PERFORMANCE_AOE] == PERFORM_AOE_GROUP &&
               !GROUP(ch)) {
          send_to_char(ch, "This performance requires a group.\r\n");
          return;
        }

        */
        /* the check for hunger/thirst WOULD to be here */
        /***/

        /* SUCCESS! */
        act("You start performing.", FALSE, ch, 0, 0, TO_CHAR);
        act("$n starts performing.", FALSE, ch, 0, 0, TO_ROOM);
        char buf[128];
        snprintf(buf, sizeof(buf), "%d", i); /* Build the effect string */
        NEW_EVENT(eBARDIC_PERFORMANCE, ch, strdup(buf), 4 * PASSES_PER_SEC);

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
    process_healing(ch, tch, SKILL_SONG_OF_HEALING, rand_number(effectiveness, effectiveness * 2 + 10), 0);
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
      act("$n starts to move with uncanny speed.", FALSE, tch, 0, 0,
          TO_ROOM);
    }
    break;

  case SKILL_ORATORY_OF_REJUVENATION:
    if (GET_HIT(tch) < GET_MAX_HIT(tch))
    {
      send_to_char(tch, "You are soothed by the power of music!\r\n");
      alter_hit(tch, -rand_number(effectiveness / 3, effectiveness / 2), FALSE);
    }

    alter_move(tch, -rand_number(effectiveness * 4, effectiveness * 6));

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

    if (GET_HIT(tch) < GET_MAX_HIT(tch))
    {
      send_to_char(tch, "You are soothed by the power of music!\r\n");
      process_healing(ch, tch, SKILL_SONG_OF_DRAGONS, rand_number(effectiveness / 2, effectiveness * 2), 0);
    }

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
      af[0].duration = 30;
      SET_BIT_AR(af[0].bitvector, AFF_FLYING);
      act("You fly through the air, free as a bird!", FALSE, tch, 0, 0, TO_CHAR);
      act("$n fly through the air, free as a bird!", FALSE, tch, 0, 0, TO_ROOM);
    }
    alter_move(tch, -rand_number(3, effectiveness / 3));
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
      act("$n shivers with fear.", FALSE, tch, 0, 0, TO_ROOM);
      SET_BIT_AR(af[0].bitvector, AFF_FEAR);
      af[0].location = APPLY_HITROLL;
      af[0].modifier = -(1 + effectiveness / 10);
    }
    break;

  /* enemy fight less effective / entangled */
  case SKILL_SONG_OF_ROOTING:
    if (rand_number(0, 100) < effectiveness)
    {
      act("$n has spawned roots.", FALSE, tch, 0, 0, TO_ROOM);
      SET_BIT_AR(af[0].bitvector, AFF_ENTANGLED);
      af[0].location = APPLY_DAMROLL;
      af[0].modifier = -effectiveness / 5;

      SET_BIT_AR(af[1].bitvector, AFF_SLOW);
      af[1].location = APPLY_AC_NEW;
      af[1].modifier = -effectiveness / 9;
    }
    break;

  /* enemy spell resistance / saves reduced */
  case SKILL_SONG_OF_THE_MAGI:
    if (rand_number(0, 100) < effectiveness)
    {
      act("$n seems more vulnerable to magic.", FALSE, tch, 0, 0, TO_ROOM);
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

/* main function for performance effects / message / etc */
int process_performance(struct char_data *ch, int spellnum,
                        int effectiveness, int aoe)
{
  struct char_data *tch = NULL, *tch_next = NULL;
  int return_val = 1;
  bool hit_self = FALSE, hit_leader = FALSE;

  /* performance message */
  switch (spellnum)
  {
  case SKILL_SONG_OF_HEALING:
    act("You sing a song to heal all wounds.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n sings a song so well you feel your pain and suffering ebbing away.", FALSE,
        ch, 0, 0, TO_ROOM);
    break;
  case SKILL_DANCE_OF_PROTECTION:
    act("You dance to protect yourself from harm.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n performs a dance that envelops you in protection.", FALSE, ch, 0, 0,
        TO_ROOM);
    break;
  case SKILL_SONG_OF_FLIGHT:
    act("You sing a song that lifts the spirits high.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n sings a song that lifts the spirits high.", FALSE, ch, 0, 0, TO_ROOM);
    break;
  case SKILL_SONG_OF_HEROISM:
    act("You sing a song that makes your heart swell with pride.", FALSE, ch, 0, 0,
        TO_CHAR);
    act("$n sings a song that makes your heart swell with pride.", FALSE, ch, 0, 0,
        TO_ROOM);
    break;
  case SKILL_ORATORY_OF_REJUVENATION:
    act("You conduct an oratory to rejuvenate the exhausted.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n conducts an oratory which eases some of your exhaustion.", FALSE, ch,
        0, 0, TO_ROOM);
    break;
  case SKILL_ACT_OF_FORGETFULNESS:
    act("You act out a skit causing forgetfulness.", FALSE, ch, 0, 0, TO_CHAR);
    act("As you observe $n acting out a skit, suddenly you can hardly "
        "remember what you were doing.",
        FALSE, ch, 0, 0, TO_ROOM);
    break;
  case SKILL_SONG_OF_REVELATION:
    act("You sing a song to reveal what is hidden.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n sings a song that seems to enhance your vision.", FALSE, ch, 0, 0, TO_ROOM);
    break;
  case SKILL_SONG_OF_DRAGONS:
    act("You sing a song that defies the mightiest of dragons.", FALSE, ch, 0, 0,
        TO_CHAR);
    act("$n sings a song that defies the mightiest of dragons, inspiring you to truly heroic deeds!", FALSE, ch, 0, 0,
        TO_ROOM);
    break;
  case SKILL_SONG_OF_FOCUSED_MIND:
    act("You sing a song which focuses the minds of the listener.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n sings a song which seems to focus your mind.", FALSE, ch, 0, 0, TO_ROOM);
    break;
  case SKILL_SONG_OF_FEAR:
    act("You sing a song which strikes fear into your enemies.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n sings a song which stikes fear into your heart!", FALSE, ch, 0, 0, TO_ROOM);
    break;
  case SKILL_SONG_OF_ROOTING:
    act("You sing a song which makes your enemies paralysed.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n sings a song so well, you feel paralysed by the tune.", FALSE, ch, 0, 0, TO_ROOM);
    break;
  case SKILL_SONG_OF_THE_MAGI:
    act("You sing a song so well, that magic in itself feels strengthened by it.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n sings a song which makes you forget completely about hostile magic.", FALSE, ch, 0, 0, TO_ROOM);
    break;
  default:
    return_val = 0;
    log("SYSERR: messages in process_performance reached default case! "
        "(spellnum: %d)",
        spellnum);
    break;
  }

  /* here we handle the different type of dances */
  switch (aoe)
  {

  /* performance that should affect your group only */
  case PERFORM_AOE_GROUP:
    if (!GROUP(ch))
    { /* self only */
      performance_effects(ch, ch, spellnum, effectiveness, aoe);
    }
    else
    {
      while ((tch = (struct char_data *)simple_list(GROUP(ch)->members)) !=
             NULL)
      {
        if (IN_ROOM(tch) != IN_ROOM(ch))
          continue;
        /* found a grouppie! */
        performance_effects(ch, tch, spellnum, effectiveness, aoe);
      }

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

        performance_effects(ch, tch, spellnum, effectiveness, aoe);
      }

      /* this is a dummy check added due to an uknown bug with lists :(  -zusuk */
      if (!hit_self)
      {
        performance_effects(ch, ch, spellnum, effectiveness, aoe);

        if (ch == GROUP(ch)->leader)
          hit_leader = TRUE;
      }

      /* this is a dummy check added due to an uknown bug with lists :(  -zusuk */
      if (!hit_leader && GROUP(ch)->leader && IN_ROOM(GROUP(ch)->leader) == IN_ROOM(ch))
        performance_effects(ch, GROUP(ch)->leader, spellnum, effectiveness, aoe);
    }
    break;

  /* performance that should affect those NOT in your group (potential foes) */
  case PERFORM_AOE_FOES:
    /* for loop to step through all in room */
    for (tch = world[ch->in_room].people; tch; tch = tch_next)
    {
      tch_next = tch->next_in_room;

      /* check if offensive aoe is OK */
      if (aoeOK(ch, tch, spellnum))
      {
        performance_effects(ch, tch, spellnum, effectiveness, aoe);
      }
    } /* end for loop */
    break;

  /* performance that should affect everyone in the room */
  case PERFORM_AOE_ROOM:
    /* for loop to step through all in room */
    for (tch = world[ch->in_room].people; tch; tch = tch_next)
    {
      tch_next = tch->next_in_room;

      performance_effects(ch, tch, spellnum, effectiveness, aoe);
    } /* end for loop */
    break;

  default:
    log("SYSERR: aoe-switch in process_performance reached default case! "
        "(spellnum: %d)",
        spellnum);
    return_val = 0;
    break;
  }

  return return_val; /* 0 = fail, 1 = success */
}

EVENTFUNC(event_bardic_performance)
{
  struct mud_event_data *pMudEvent = NULL;
  struct char_data *ch = NULL;
  struct obj_data *instrument = NULL;
  int effectiveness;
  int spellnum;
  int performance_num;
  int difficulty;

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

  /* disqualifiers */
  if (!IS_NPC(ch) && !HAS_FEAT(ch, FEAT_BARDIC_MUSIC))
  {
    send_to_char(ch, "You don't know how to perform.\r\n");
    return 0;
  }
  if (char_has_mud_event(ch, ePERFORM))
  {
    send_to_char(ch, "You are already performing!\r\n");
    return 0;
  }
  if (compute_ability(ch, ABILITY_PERFORM) <= performance_info[performance_num][PERFORMANCE_DIFF])
  {
    send_to_char(ch, "You are not trained enough for this performance! "
                     "(need: %d performance ability)\r\n",
                 performance_info[performance_num][PERFORMANCE_DIFF]);
    return 0;
  }
  if (((ch->in_room != NOWHERE && ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF)) || AFF_FLAGGED(ch, AFF_SILENCED)) &&
      (performance_info[performance_num][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_KEYBOARD ||
       performance_info[performance_num][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_ORATORY ||
       performance_info[performance_num][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_PERCUSSION ||
       performance_info[performance_num][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_STRING ||
       performance_info[performance_num][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_WIND ||
       performance_info[performance_num][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_SING))
  {
    send_to_char(ch, "The silence effectively stops your performance.\r\n");
    return 0;
  }
  if (GET_POS(ch) < POS_FIGHTING)
  {
    send_to_char(ch, "You can't concentrate on your performance when you are in "
                     "this position.\r\n");
    return 0;
  }

  /* not necessary */
  /*
  if (performance_info[performance_num][PERFORMANCE_AOE] == PERFORM_AOE_GROUP &&
          !GROUP(ch)) {
    send_to_char(ch, "This performance requires a group.\r\n");
    return 0;
  }
  */

  /* the check for hunger/thirst WOULD to be here */
  /***/
  /* end disqualifiers */

  /* base effectiveness of performance */
  effectiveness = rand_number(1, 9);

  /* base difficulty */
  difficulty = 30 - GET_CHA_BONUS(ch);

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
    /* the effectiveness of our instrument is all handled here */
    difficulty -= GET_OBJ_VAL(instrument, 1);

    /* instrument of quality <= 0 is unbreakable */
    if (rand_number(1, 10001) <= GET_OBJ_VAL(instrument, 3))
    {
      act("Your $p cannot take the strain of magic any longer, and it breaks!", FALSE, ch, instrument, 0, TO_CHAR);
      act("$n's $p cannot take the strain of magic any longer, and it breaks!", FALSE, ch, instrument, 0, TO_ROOM);
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

  if (effectiveness > 20)
    effectiveness = 20;

  /* if fighting, 1/2 effect of it.*/
  if (FIGHTING(ch))
  {
    send_to_char(ch, "Your performance is slightly hindered as you are concentrating on the combat.\r\n");
    effectiveness /= 2;
  }

  spellnum = performance_info[performance_num][PERFORMANCE_SKILLNUM];

  /* performance check! */
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
    return 0; /* process performance failed somehow */
  }

  /* check for stutter. if stutter, stop performance  */
  if (rand_number(1, 101) < difficulty)
  {
    send_to_char(ch, "Uh oh.. how did the performance go, anyway?\r\n");
    act("$n stutters in the performance!", FALSE, ch, 0, 0, TO_ROOM);
    return 0;
  }

  /* success, we're coming back in VERSE_INTERVAL */
  return VERSE_INTERVAL;
}

/* EOF */
