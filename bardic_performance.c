/*
 * File:   bardic_performance.h
 * Author: Zusuk
 * Functions, commands, etc for the bardic performance system.
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
   skillnum, instrument, instrument_skill, difficulty */
int performance_info[MAX_PERFORMANCES][PERFORMANCE_INFO_FIELDS] = {
  {SKILL_SONG_OF_HEALING,        INSTRUMENT_LYRE,       SKILL_LYRE,      4,   PERFORMANCE_TYPE_SING},
  {SKILL_SONG_OF_PROTECTION,     INSTRUMENT_DRUM,       SKILL_DRUM,      5,   PERFORMANCE_TYPE_SING},
  {SKILL_SONG_OF_FOCUSED_MIND,   INSTRUMENT_HARP,       SKILL_HARP,      6,   PERFORMANCE_TYPE_SING},
  {SKILL_SONG_OF_HEROISM,        INSTRUMENT_DRUM,       SKILL_DRUM,      8,   PERFORMANCE_TYPE_SING},
  {SKILL_SONG_OF_REJUVENATION,   INSTRUMENT_LYRE,       SKILL_LYRE,      10,  PERFORMANCE_TYPE_SING},
  {SKILL_SONG_OF_FLIGHT,         INSTRUMENT_HORN,       SKILL_HORN,      12,  PERFORMANCE_TYPE_SING},
  {SKILL_SONG_OF_REVELATION,     INSTRUMENT_FLUTE,      SKILL_FLUTE,     14,  PERFORMANCE_TYPE_SING},
  {SKILL_SONG_OF_FEAR,           INSTRUMENT_HARP,       SKILL_HARP,      16,  PERFORMANCE_TYPE_SING},
  {SKILL_SONG_OF_FORGETFULNESS,  INSTRUMENT_FLUTE,      SKILL_FLUTE,     18,  PERFORMANCE_TYPE_SING},
  {SKILL_SONG_OF_ROOTING,        INSTRUMENT_MANDOLIN,   SKILL_MANDOLIN,  20,  PERFORMANCE_TYPE_SING},
  {SKILL_SONG_OF_DRAGONS,        INSTRUMENT_HORN,       SKILL_HORN,      24,  PERFORMANCE_TYPE_SING},
  {SKILL_SONG_OF_THE_MAGI,       INSTRUMENT_MANDOLIN,   SKILL_MANDOLIN,  29,  PERFORMANCE_TYPE_SING},
};

/* local function for modifying chars points (hitpoints)
 * note: negative (-) is healing */
void alter_hit(struct char_data *ch, int points, bool unused) {
  GET_HIT(ch) -= points;
  GET_HIT(ch) = MIN(GET_HIT(ch), GET_MAX_HIT(ch));  
  update_pos(ch);
}

void alter_move(struct char_data *ch, int points) {
  GET_MOVE(ch) -= points;
  GET_MOVE(ch) = MIN(GET_MOVE(ch), GET_MAX_MOVE(ch));  
  update_pos(ch);
}

/* primary command entry point for the play skill */
ACMD(do_play) {
  int i;
  int len = 0;

  if (!argument || (len = strlen(argument)) == 0) {
    if (char_has_mud_event(ch, eBARDIC_PERFORMANCE)) {
      event_cancel_specific(ch, eBARDIC_PERFORMANCE);
      act("You stopped your performance.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n stops performing.", FALSE, ch, 0, 0, TO_ROOM);
      return;
    } else {
      send_to_char(ch, "Play what performance?\r\n");
      return;
    }
  }
  
  if (char_has_mud_event(ch, eBARDIC_PERFORMANCE)) {
    act("You stopped your current performance.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n stops performing...", FALSE, ch, 0, 0, TO_ROOM);
    event_cancel_specific(ch, eBARDIC_PERFORMANCE);
  }
  
  skip_spaces(&argument);
  len = strlen(argument);

  for (i = 0; i < MAX_PERFORMANCES; i++) {
    if (!strncmp(argument, skill_name(performance_info[i][PERFORMANCE_SKILLNUM]), len)) {
      if (!GET_SKILL(ch, performance_info[i][PERFORMANCE_SKILLNUM])) {
        send_to_char(ch, "But you do not know that performance!\r\n");
        return;
      } else {
        /* check for disqualifiers */
        if (!IS_NPC(ch) && !HAS_FEAT(ch, FEAT_BARDIC_MUSIC)) {
          send_to_char(ch, "You don't know how to perform.\r\n");
          return;
        }
        if (char_has_mud_event(ch, ePERFORM)) {
          send_to_char(ch, "You are already performing!\r\n");
          return;
        }
        if (char_has_mud_event(ch, eBARDIC_PERFORMANCE)) {
          send_to_char(ch, "You are already in the middle of a performance!\r\n");
          return;
        }
        if (compute_ability(ch, ABILITY_PERFORM) <= performance_info[i][PERFORMANCE_DIFF]) {
          send_to_char(ch, "You are not trained enough for this performance! "
                  "(need: %d performance ability)\r\n",
                  performance_info[i][PERFORMANCE_DIFF]);
          return;
        }
        if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF) && (
                performance_info[i][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_KEYBOARD ||
                performance_info[i][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_ORATORY ||
                performance_info[i][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_PERCUSSION ||
                performance_info[i][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_STRING ||
                performance_info[i][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_WIND ||
                performance_info[i][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_SING
                )) {
          send_to_char(ch, "The silence effectively stops your performance.\r\n");
          return;
        }
        if (GET_POS(ch) < POS_FIGHTING) {
          send_to_char(ch, "You can't concentrate on your performance when you are in "
                  "this position.\r\n");
          return;
        }
        /* the check for hunger/thirst WOULD to be here */
        /***/

        /* SUCCESS! */
        act("You start performing.", FALSE, ch, 0, 0, TO_CHAR);
        act("$n starts performing.", FALSE, ch, 0, 0, TO_ROOM);
        char buf[128];
        sprintf(buf, "%d", i); /* Build the effect string */
        NEW_EVENT(eBARDIC_PERFORMANCE, ch, strdup(buf), 4 * PASSES_PER_SEC);
        return;
      }
    }
  }
  
  send_to_char(ch, "But that is not a performance!\r\n");
  return;
}

/* main function for performance effects / message / etc */
int process_performance(struct char_data *ch, int spellnum, int effectiveness) {
  struct affected_type af;
  struct char_data *tch = NULL, *tch_next = NULL;
  bool nomessage = FALSE;
  int return_val = 1;
  
  /* init affection / default values */
  new_affect(&af);
  SET_BIT_AR(af.bitvector, 0);
  af.modifier = 0;
  af.duration = 2;
  af.location = APPLY_NONE;
  af.spell = spellnum;
  
  /* performance message */
  switch (spellnum) {
    case SKILL_SONG_OF_HEALING:
      act("You sing a song to heal all wounds.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n sings a song so well you feel your pain and suffering ebbing away.", FALSE,
              ch, 0, 0, TO_ROOM);
      break;
    case SKILL_SONG_OF_PROTECTION:
      act("You sing a song to protect you from harm.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n sings a song that envelops you in a musical protection.", FALSE, ch, 0, 0,
              TO_ROOM);
      break;
    case SKILL_SONG_OF_FLIGHT:
      act("You sing a song that lifts the spirits high", FALSE, ch, 0, 0, TO_CHAR);
      act("$n sings a song that lifts the spirits high.", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case SKILL_SONG_OF_HEROISM:
      act("You sing a song that makes your heart swell with pride.", FALSE, ch, 0, 0,
              TO_CHAR);
      act("$n sings a song that makes your heart swell with pride.", FALSE, ch, 0, 0,
              TO_ROOM);
      break;
    case SKILL_SONG_OF_REJUVENATION:
      act("You sing a song to ease all suffering.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n sings a song which ease all your suffering and gentle your soul.", FALSE, ch,
              0, 0, TO_ROOM);
      break;
    case SKILL_SONG_OF_FORGETFULNESS:
      act("You sing a really DULL song.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n sings a song that is so dull, so you can hardly remember what you were doing.",
              FALSE, ch, 0, 0, TO_ROOM); 
      break;
    case SKILL_SONG_OF_REVELATION:
      act("You sing a song to reveal what is hidden.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n sings a song that seems to enhance your vision.", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case SKILL_SONG_OF_DRAGONS:
      act("You sing a song that defies the mightiest of dragons.", FALSE, ch, 0, 0,
              TO_CHAR);
      act("$n sings a song, inspiring you to truly heroic deeds!", FALSE, ch, 0, 0,
              TO_ROOM);
      break;
    default:
      return_val = 0;
      log("SYSERR: messages in process_performance reached default case! "
                "(spellnum: %d)", spellnum);
      break;
  }

  /* actually guts, currently we affect all in room with performance */
  for (tch = world[ch->in_room].people; tch; tch = tch_next) {
    tch_next = tch->next_in_room;

    if (affected_by_spell(tch, spellnum))
            nomessage = TRUE;

    switch (spellnum) {
      case SKILL_SONG_OF_HEALING:
        if (GET_HIT(tch) < GET_MAX_HIT(tch)) {
          send_to_char(tch, "You are soothed by the power of music!\r\n");
          alter_hit(tch, -rand_number(effectiveness / 2, effectiveness * 2), FALSE);
        }
        break;

      case SKILL_SONG_OF_PROTECTION:
        af.location = APPLY_AC_NEW;
        af.modifier = (effectiveness+1) / 10;
        affect_join(tch, &af, FALSE, TRUE, FALSE, FALSE);
        af.location = APPLY_SAVING_WILL;
        af.modifier = effectiveness / 6;
        affect_join(tch, &af, FALSE, TRUE, FALSE, FALSE);
        break;

      case SKILL_SONG_OF_FLIGHT:
        if (!AFF_FLAGGED(tch, AFF_FLYING)) {
          af.duration = 30;
          SET_BIT_AR(af.bitvector, AFF_FLYING);
          affect_join(tch, &af, FALSE, 1, FALSE, FALSE);
          act("You fly through the air, free as a bird!", FALSE, tch, 0, 0,
                    TO_CHAR);
          act("$n fly through the air, free as a bird!", FALSE, tch, 0, 0,
                    TO_ROOM);
        }
        alter_move(tch, -rand_number(3, effectiveness / 3));
        break;

      case SKILL_SONG_OF_HEROISM:
        af.location = APPLY_HITROLL;
        af.modifier = 1 + effectiveness / 10;
        affect_join(tch, &af, FALSE, TRUE, FALSE, FALSE);
        af.location = APPLY_DAMROLL;
        af.modifier = effectiveness / 13;
        if (GET_LEVEL(ch) >= 20 && !AFF_FLAGGED(tch, AFF_HASTE)) {
          SET_BIT_AR(af.bitvector, AFF_HASTE);
          act("You feel the world slow down around you.", FALSE, tch, 0, 0,
                    TO_CHAR);
          act("$n starts to move with uncanny speed.", FALSE, tch, 0, 0,
                    TO_ROOM);
        }
        affect_join(tch, &af, FALSE, TRUE, FALSE, FALSE);
        break;
  
      case SKILL_SONG_OF_REJUVENATION:
        if (GET_HIT(tch) < GET_MAX_HIT(tch)) {
          send_to_char(tch, "You are soothed by the power of music!\r\n");
          alter_hit(tch, -rand_number(effectiveness / 3, effectiveness / 2), FALSE);
        }
        alter_move(tch, -rand_number(effectiveness / 3, effectiveness / 2));
        if (rand_number(0, 100) < effectiveness && affected_by_spell(tch, SPELL_POISON)) {
          affect_from_char(tch, SPELL_POISON);
          send_to_char(tch, "The soothing music clears the poison from your body!\r\n");
        }
        break;

      case SKILL_SONG_OF_FORGETFULNESS:
        if (IS_NPC(tch) && rand_number(0, 100) < effectiveness)
          clearMemory(tch);
        break;

      case SKILL_SONG_OF_REVELATION:
        if (!AFF_FLAGGED(tch, AFF_DETECT_INVIS)) {
          af.location = APPLY_HITROLL;
          af.modifier = 0;
          SET_BIT_AR(af.bitvector, AFF_DETECT_INVIS);
          affect_join(tch, &af, FALSE, TRUE, FALSE, FALSE);
        }
        if (!AFF_FLAGGED(tch, AFF_DETECT_ALIGN) && GET_LEVEL(ch) >= 5) {
          af.location = APPLY_DAMROLL;
          af.modifier = 0;
          SET_BIT_AR(af.bitvector, AFF_DETECT_ALIGN);
          affect_join(tch, &af, FALSE, TRUE, FALSE, FALSE);
        }
        if (!AFF_FLAGGED(tch, AFF_DETECT_MAGIC) && GET_LEVEL(ch) >= 10) {
          af.location = APPLY_AC;
          af.modifier = 0;
          SET_BIT_AR(af.bitvector, AFF_DETECT_MAGIC);
          affect_join(tch, &af, FALSE, TRUE, FALSE, FALSE);
        }
        if (!AFF_FLAGGED(tch, AFF_SENSE_LIFE) && GET_LEVEL(ch) >= 15) {
          af.location = APPLY_DEX;
          af.modifier = 0;
          SET_BIT_AR(af.bitvector, AFF_SENSE_LIFE);
          affect_join(tch, &af, FALSE, TRUE, FALSE, FALSE);
        }
        if (!AFF_FLAGGED(tch, AFF_FARSEE) && GET_LEVEL(ch) >= 20) {
          af.location = APPLY_AGE;
          af.modifier = 0;
          SET_BIT_AR(af.bitvector, AFF_FARSEE);
          affect_join(tch, &af, FALSE, TRUE, FALSE, FALSE);
        }
        if (nomessage == FALSE)
          act("You feel your eyes tingle.", FALSE, tch, 0, 0, TO_CHAR);
        break;

      case SKILL_SONG_OF_DRAGONS:
        if (!IS_NPC(tch)) {
          if (GET_HIT(tch) < GET_MAX_HIT(tch)) {
            send_to_char(tch, "You are soothed by the power of music!\r\n");
            alter_hit(tch, -rand_number(effectiveness / 4, effectiveness / 2), FALSE);
          }
          af.location = APPLY_AC_NEW;
          af.modifier = MAX(1, (effectiveness+2) / 19);
          affect_join(tch, &af, FALSE, TRUE, FALSE, FALSE);
          af.location = APPLY_SAVING_REFL;
          af.modifier = effectiveness / 5;
          affect_join(tch, &af, FALSE, TRUE, FALSE, FALSE);
        }
        break;
          
      default:
        log("SYSERR: room-loop in process_performance reached default case! "
                "(spellnum: %d)", spellnum);
        return_val = 0;
        break;
    } /* end switch */
  } /* end for loop */
  
  return return_val; /* 0 = fail, 1 = success */
}

EVENTFUNC(event_bardic_performance) {
  struct mud_event_data *pMudEvent = NULL;
  struct char_data *ch = NULL;
  struct obj_data *instrument = NULL;
  int effectiveness;
  int spellnum;
  int performance_num;
  int difficulty;

  /* start handling the event data */
  pMudEvent = (struct mud_event_data *) event_obj;
  if (!pMudEvent) {
    log("SYSERR: event_bardic_performance missing pMudEvent!");
    return 0;
  }
  if (!pMudEvent->iId) {
    log("SYSERR: event_bardic_performance missing pMudEvent->iId!");
    return 0;
  }
  ch = (struct char_data *) pMudEvent->pStruct;
  if (!ch) {
    log("SYSERR: event_bardic_performance missing pMudEvent->pStruct!");
    return 0;
  }
  
  /* extract the variable(s) */
  if (pMudEvent->sVariables == NULL) {
    /* This is odd - This field should always be populated! */
    log("SYSERR: sVariables field is NULL for event_bardic_performance: %d",
            pMudEvent->iId);
    return 0;
  } else {
    performance_num = atoi((char *) pMudEvent->sVariables);
  }  
  /* finished handling event data */  
  
  /* disqualifiers */
  if (!IS_NPC(ch) && !HAS_FEAT(ch, FEAT_BARDIC_MUSIC)) {
    send_to_char(ch, "You don't know how to perform.\r\n");
    return 0;
  }
  if (char_has_mud_event(ch, ePERFORM)) {
    send_to_char(ch, "You are already performing!\r\n");
    return 0;
  }
  if (compute_ability(ch, ABILITY_PERFORM) <= performance_info[performance_num][PERFORMANCE_DIFF]) {
    send_to_char(ch, "You are not trained enough for this performance! "
            "(need: %d performance ability)\r\n",
            performance_info[performance_num][PERFORMANCE_DIFF]);
    return 0;
  }
  if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF) && (
      performance_info[performance_num][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_KEYBOARD ||
      performance_info[performance_num][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_ORATORY ||
      performance_info[performance_num][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_PERCUSSION ||
      performance_info[performance_num][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_STRING ||
      performance_info[performance_num][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_WIND ||
      performance_info[performance_num][PERFORMANCE_TYPE] == PERFORMANCE_TYPE_SING
      ) ) {
    send_to_char(ch, "The silence effectively stops your performance.\r\n");
    return 0;
  }
  if (GET_POS(ch) < POS_FIGHTING) {
    send_to_char(ch, "You can't concentrate on your performance when you are in "
            "this position.\r\n");
    return 0;
  }
  /* the check for hunger/thirst WOULD to be here */
  /***/
  /* end disqualifiers */
  
  /* base effectiveness of performance */
  effectiveness = 10;
  /* base difficulty */
  difficulty = 39;

  /* find an instrument */
  instrument = GET_EQ(ch, WEAR_HOLD_1);
  if (!instrument || GET_OBJ_TYPE(instrument) != ITEM_INSTRUMENT) {
    instrument = GET_EQ(ch, WEAR_HOLD_2);
  }
  if (!instrument || GET_OBJ_TYPE(instrument) != ITEM_INSTRUMENT) {
    instrument = GET_EQ(ch, WEAR_HOLD_2H);
  }  
  if (!instrument || GET_OBJ_TYPE(instrument) != ITEM_INSTRUMENT)
    instrument = NULL; /* nope, nothing! */
  /* END find an instrument */
  
  /* Any instrument is better than nothing, if its the designated instrument, 
   * and good at it, then even better.. */
  if (!instrument) {
    effectiveness -= 3;
    send_to_char(ch, "You sing this verse a-capella...  ");
  } else {
    difficulty -= GET_OBJ_VAL(instrument, 1);

    /* instrument of quality > 2000 is unbreakable */
    if (rand_number(0, 2000) < GET_OBJ_VAL(instrument, 3)) {
      act("Your $p cannot take the strain of magic any longer, and it breaks.", FALSE, ch, instrument, 0, TO_CHAR);
      act("$n's $p cannot take the strain of magic any longer, and it breaks.", FALSE, ch, instrument, 0, TO_ROOM);
      extract_obj(instrument);
      instrument = NULL;
      effectiveness -= 5;
    } else if (GET_OBJ_VAL(instrument, 0) == performance_info[performance_num][INSTRUMENT_NUM]) {
      effectiveness += GET_OBJ_VAL(instrument, 2);
    } else
      effectiveness -= 2;
  }

  if (effectiveness > 20)
    effectiveness = 20;

  /* if fighting, 1/2 effect of it.*/
  if (FIGHTING(ch)) {
    send_to_char(ch, "You sing slightly off-key as you are concentrating on the combat.\r\n");
    effectiveness /= 2;
  }

  spellnum = performance_info[performance_num][PERFORMANCE_SKILLNUM];

  /* check for stutter. if stutter, stop performance  */
  if (!rand_number(0, difficulty)) {
    send_to_char(ch, "Uh oh.. how did the performance go, anyway?\r\n");
    act("$n stutters in his performance, and falls silent.", FALSE, ch, 0, 0, TO_ROOM);
    return 0;
  }

  /* this is suppose to be a skill check for how good you are at a particular performance */
  effectiveness += 3;

  //effectiveness is from 1-60
  effectiveness = effectiveness * compute_ability(ch, ABILITY_PERFORM) * 3;
  effectiveness /= 20;
  if (effectiveness < 1)
    effectiveness = 1;
  if (effectiveness > 60)
    effectiveness = 60;

  /* GUTS! message, effect processed in this function */
  if (!process_performance(ch, spellnum, effectiveness)) {
    send_to_char(ch, "Your performance fails!\r\n");
    return 0; /* process performance failed somehow */
  }

  /* success, we're coming back in VERSE_INTERVAL */
  return VERSE_INTERVAL;
}

/* EOF */
