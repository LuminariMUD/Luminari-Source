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

/* Instruments obj vals are
   0 - type (lyre/drum/etc)
   1 - diffulty
   2 - level
   3 - breakability   ***/

/* these are defines just made for fillers in the following lookup data
 since they are currently unused in our feat system */
#define SKILL_LYRE     1
#define SKILL_DRUM     2
#define SKILL_HORN     3
#define SKILL_FLUTE    4
#define SKILL_HARP     5
#define SKILL_MANDOLIN 6

/* song info: this will be our reference/lookup data for each song/performance
   skillnum, instrument, instrument_skill */
int song_info[MAX_PERFORMANCES][3] = {
  {SKILL_SONG_OF_HEALING,        INSTRUMENT_LYRE,       SKILL_LYRE},
  {SKILL_SONG_OF_PROTECTION,     INSTRUMENT_DRUM,       SKILL_DRUM},
  {SKILL_SONG_OF_FLIGHT,         INSTRUMENT_HORN,       SKILL_HORN},
  {SKILL_SONG_OF_HEROISM,        INSTRUMENT_DRUM,       SKILL_DRUM},
  {SKILL_SONG_OF_REJUVENATION,   INSTRUMENT_LYRE,       SKILL_LYRE},
  {SKILL_SONG_OF_FORGETFULNESS,  INSTRUMENT_FLUTE,      SKILL_FLUTE},
  {SKILL_SONG_OF_REVELATION,     INSTRUMENT_FLUTE,      SKILL_FLUTE},
  {SKILL_SONG_OF_DRAGONS,        INSTRUMENT_HORN,       SKILL_HORN},
  {SKILL_SONG_OF_FOCUSED_MIND,   INSTRUMENT_HARP,       SKILL_HARP},
  {SKILL_SONG_OF_FEAR,           INSTRUMENT_HARP,       SKILL_HARP},
  {SKILL_SONG_OF_ROOTING,        INSTRUMENT_MANDOLIN,   SKILL_MANDOLIN},
  {SKILL_SONG_OF_THE_MAGI,       INSTRUMENT_MANDOLIN,   SKILL_MANDOLIN},
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

EVENTFUNC(event_bardic_performance) {
  struct mud_event_data *pMudEvent = NULL;
  struct char_data *ch;
  struct obj_data *instrument = NULL;
  struct obj_data *tobj = NULL;
  struct char_data *tch, *tch_next;
  int effect;
  int spellnum;
  int song_num;
  struct affected_type af;
  int difficulty;
  int hold;
  bool nomessage = FALSE;

  /* start handling the event data */
  pMudEvent = (struct mud_event_data *) event_obj;
  if (!pMudEvent)
    return 0;
  if (!pMudEvent->iId)
    return 0;
  ch = (struct char_data *) pMudEvent->pStruct;
  if (!ch)
    return 0;
  
  //extract the variables
  if (pMudEvent->sVariables == NULL) {
    // This is odd - This field should always be populated
    log("SYSERR: sVariables field is NULL for event_song: %d", pMudEvent->iId);
    return 0;
  } else {
    song_num = atoi((char *) pMudEvent->sVariables);
  }  
  /* finished handling event data */  
  
  effect = 10;
  difficulty = 40;

  /* the check for hunger/thirst USE to be here */
  /***/

  tobj = GET_EQ(ch, WEAR_HOLD_1);
  hold = WEAR_HOLD_1;
  
  if (!tobj || GET_OBJ_TYPE(tobj) != ITEM_INSTRUMENT) {
    tobj = GET_EQ(ch, WEAR_HOLD_2);
    hold = WEAR_HOLD_2;
  }
  
  /* this was just go get rid of a compiler warning */
  if (hold) ;
  /**/

  if (tobj && GET_OBJ_TYPE(tobj) == ITEM_INSTRUMENT)
    instrument = tobj;
  tobj = GET_EQ(ch, WEAR_HOLD_1);

  if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF)) {
    send_to_char(ch, "The silence effectively stops your song.\r\n");
    return 0;
  }

  if (GET_POS(ch) < POS_FIGHTING) {
    send_to_char(ch, "You can't concentrate on your songs when you are in this position.\r\n");
    return 0;
  }
  
  /* Any instrument is better than nothing, if its the designated instrument, 
   * and good at it, then even better.. */
  if (!instrument) {
    effect -= 3;
    send_to_char(ch, "You sing this verse a-capella...  ");
  } else {
    difficulty = 40 - GET_OBJ_VAL(instrument, 1);

    if (rand_number(0, 2000) < GET_OBJ_VAL(instrument, 3)) {
      act("Your $p cannot take the strain of magic any longer, and it breaks.", FALSE, ch, instrument, 0, TO_CHAR);
      act("$n's $p cannot take the strain of magic any longer, and it breaks.", FALSE, ch, instrument, 0, TO_ROOM);
      extract_obj(instrument);
      instrument = 0;
      effect -= 5;
    } else if (GET_OBJ_VAL(instrument, 0) == song_info[song_num][INSTRUMENT_NUM]) {
      effect += GET_OBJ_VAL(instrument, 2);
    } else
      effect -= 2;
  }

  if (effect > 20)
    effect = 20;

  /* if fighting, 1/2 effect of it.*/
  if (FIGHTING(ch)) {
    send_to_char(ch, "You sing slightly off-key as you are concentrating on the combat.\r\n");
    effect /= 2;
  }

  for (tch = world[ch->in_room].people; tch; tch = tch_next) {
    tch_next = tch->next_in_room;
    if (FIGHTING(tch) == ch) {
      send_to_char(ch, "You are too busy defending yourself to sing a song.\r\n");
      return 0;
    }
  }

  spellnum = song_info[song_num][PERFORMANCE_SKILLNUM];

  /* check for stutter. if stutter, stop song. :) */
  if (!rand_number(0, difficulty) && GET_SKILL(ch, spellnum) < 90) {
    send_to_char(ch, "Uh oh.. how did the song go, anyway?\r\n");
    act("$n stutters in his song, and falls silent.", FALSE, ch, 0, 0, TO_ROOM);
    return 0;
  }

  /* this is suppose to be a skill check for how good you are at a particular song */
  effect += 3;

  SET_BIT_AR(af.bitvector, 0);
  af.modifier = 0;
  af.duration = 8;
  af.location = APPLY_NONE;

  //effect is from 1-60
  effect = effect * GET_SKILL(ch, spellnum);
  effect /= 20;
  if (effect < 1)
    effect = 1;

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
      act("$n sings a song that seems to enchance your vision.", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case SKILL_SONG_OF_DRAGONS:
      act("You sing a song that defies the mightiest of dragons.", FALSE, ch, 0, 0,
              TO_CHAR);
      act("$n sings a song, inspiring you to truly heroic deeds!", FALSE, ch, 0, 0,
              TO_ROOM);
      break;
  }

  // affect all in room with spell.
  for (tch = world[ch->in_room].people; tch; tch = tch_next) {
    tch_next = tch->next_in_room;

    if (affected_by_spell(tch, spellnum))
            nomessage = TRUE;

    switch (spellnum) {
      case SKILL_SONG_OF_HEALING:
        if (GET_HIT(tch) < GET_MAX_HIT(tch)) {
          send_to_char(tch, "You are soothed by the power of music!\r\n");
          alter_hit(tch, -rand_number(effect / 2, effect * 2), FALSE);
        }
        break;

      case SKILL_SONG_OF_PROTECTION:
        af.location = APPLY_AC;
        af.modifier = -effect;
        affect_join(tch, &af, FALSE, TRUE, FALSE, FALSE);
        af.location = APPLY_SAVING_WILL;
        af.modifier = -effect / 6;
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
        alter_move(tch, -rand_number(3, effect / 3));
        break;

      case SKILL_SONG_OF_HEROISM:
        af.location = APPLY_HITROLL;
        af.modifier = 1 + effect / 10;
        affect_join(tch, &af, FALSE, TRUE, FALSE, FALSE);
        af.location = APPLY_DAMROLL;
        af.modifier = effect / 13;
        if (GET_LEVEL(ch) > 39 && !AFF_FLAGGED(tch, AFF_HASTE)) {
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
          alter_hit(tch, -rand_number(effect / 3, effect / 2), FALSE);
        }
        alter_move(tch, -rand_number(effect / 3, effect / 2));
        if (rand_number(0, 100) < effect) {
          affect_from_char(tch, SPELL_POISON);
        }
        break;

      case SKILL_SONG_OF_FORGETFULNESS:
        if (IS_NPC(tch) && rand_number(0, 100) < effect)
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
              alter_hit(tch, -rand_number(effect / 4, effect / 2), FALSE);
            }
            af.location = APPLY_AC;
            af.modifier = -effect / 2;
            affect_join(tch, &af, FALSE, TRUE, FALSE, FALSE);
            af.location = APPLY_SAVING_REFL;
            af.modifier = -effect / 5;
            affect_join(tch, &af, FALSE, TRUE, FALSE, FALSE);
          }
          break;
          
      default:break;
    } /* end switch */
  } /* end for loop */

  return VERSE_INTERVAL;
}

/* primary command entry point for the play skill */
ACMD(do_play) {
  int i;
  int len = 0;

  if (!argument || (len = strlen(argument)) == 0) {
    if (char_has_mud_event(ch, eBARDIC_PERFORMANCE)) {
      event_cancel_specific(ch, eBARDIC_PERFORMANCE);
      act("You stopped your song.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n stops singing.", FALSE, ch, 0, 0, TO_ROOM);
      return;
    } else {
      send_to_char(ch, "Play what song?\r\n");
      return;
    }
  }
  
  if (char_has_mud_event(ch, eBARDIC_PERFORMANCE))
    event_cancel_specific(ch, eBARDIC_PERFORMANCE);
  
  skip_spaces(&argument);
  len = strlen(argument);

  for (i = 0; i < MAX_PERFORMANCES; i++) {
    if (!strncmp(argument, spells[song_info[i][PERFORMANCE_SKILLNUM]], len)) {
      if (!GET_SKILL(ch, song_info[i][PERFORMANCE_SKILLNUM])) {
        send_to_char(ch, "But you do not know that song!\r\n");
        return;
      } else {
        if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF)) {
          send_to_char(ch, "Your lips move, but no sound is heard!\r\n");
          return;
        }

        act("You start singing.", FALSE, ch, 0, 0, TO_CHAR);
        act("$n starts singing.", FALSE, ch, 0, 0, TO_ROOM);

        char buf[128];
        /* Build the effect string */
        sprintf(buf, "%d", i);
        NEW_EVENT(eTRAPTRIGGERED, ch, strdup(buf), 1);
        return;
      }
    }
  }
  
  send_to_char(ch, "But that is not a song!\r\n");
  return;
}

/* EOF */
