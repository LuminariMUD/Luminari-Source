/**************************************************************************
*  File: limits.c                                          Part of tbaMUD *
*  Usage: Limits & gain funcs for HMV, exp, hunger/thirst, idle time.     *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "spells.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "dg_scripts.h"
#include "class.h"
#include "fight.h"
#include "screen.h"
#include "mud_event.h"

/* local file scope function prototypes */
static int graf(int grafage, int p0, int p1, int p2, int p3, int p4, int p5, int p6);
static void check_idling(struct char_data *ch);


/* When age < 15 return the value p0
   When age is 15..29 calculate the line between p1 & p2
   When age is 30..44 calculate the line between p2 & p3
   When age is 45..59 calculate the line between p3 & p4
   When age is 60..79 calculate the line between p4 & p5
   When age >= 80 return the value p6 */
static int graf(int grafage, int p0, int p1, int p2, int p3, int p4, int p5, int p6)
{

  if (grafage < 15)
    return (p0);					/* < 15   */
  else if (grafage <= 29)
    return (p1 + (((grafage - 15) * (p2 - p1)) / 15));	/* 15..29 */
  else if (grafage <= 44)
    return (p2 + (((grafage - 30) * (p3 - p2)) / 15));	/* 30..44 */
  else if (grafage <= 59)
    return (p3 + (((grafage - 45) * (p4 - p3)) / 15));	/* 45..59 */
  else if (grafage <= 79)
    return (p4 + (((grafage - 60) * (p5 - p4)) / 20));	/* 60..79 */
  else
    return (p6);					/* >= 80 */
}


void regen_update(struct char_data *ch)
{
  struct char_data *tch = NULL;
  int hp = 1, found = 0;

  // poisoned, and dying people should suffer their damage from anyone they are
  // fighting in order that xp goes to the killer (who doesn't strike the last blow)
  // -zusuk
  if (AFF_FLAGGED(ch, AFF_POISON)) {
    if (FIGHTING(ch) || dice(1, 2) == 2) {
      for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
        if (!IS_NPC(tch) && FIGHTING(tch) == ch) {
          damage(tch, ch, dice(1, 4), SPELL_POISON, DAM_POISON, FALSE);
          found = 1;
          break;
        }
      }
      if (!found)
        damage(ch, ch, 1, SPELL_POISON, DAM_POISON, FALSE);
      update_pos(ch);
      return;
    }
  }
  found = 0; tch = NULL;
  if (GET_POS(ch) == POS_MORTALLYW) {
    for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
      if (!IS_NPC(tch) && FIGHTING(tch) == ch) {
        damage(tch, ch, 1, TYPE_SUFFERING, DAM_RESERVED_DBC, FALSE);
        found = 1;
        break;
      }
    }
    if (!found)
      damage(ch, ch, 1, TYPE_SUFFERING, DAM_RESERVED_DBC, FALSE);
    update_pos(ch);
    return;
  }
  //50% chance you'll continue dying when incapacitated
  found = 0; tch = NULL;
  if (GET_POS(ch) == POS_INCAP && dice(1,2) == 2) {
    for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
      if (!IS_NPC(tch) && FIGHTING(tch) == ch) {
        damage(tch, ch, 1, TYPE_SUFFERING, DAM_RESERVED_DBC, FALSE);
        found = 1;
        break;
      }
    }
    if (!found)
      damage(ch, ch, 1, TYPE_SUFFERING, DAM_RESERVED_DBC, FALSE);
    update_pos(ch);
    return;
  }

  //position, other bonuses
  if (GET_POS(ch) == POS_RESTING)
    hp += dice(1, 2);
  if (GET_POS(ch) == POS_SLEEPING)
    hp += dice(3, 2);

  if (ROOM_FLAGGED(ch->in_room, ROOM_REGEN))
    hp *= 2;
  if (AFF_FLAGGED(ch, AFF_REGEN))
    hp *= 2;

  // troll racial innate regeneration
  if (GET_RACE(ch) == RACE_TROLL) {
    hp *= 2;
    if (FIGHTING(ch))
      hp *= 2;
  }

  if (rand_number(0, 3) && GET_LEVEL(ch) <= LVL_IMMORT && !IS_NPC(ch) &&
	(GET_COND(ch, THIRST) == 0 || GET_COND(ch, HUNGER) == 0))
    hp = 0;

  if (IS_NPC(ch) && GET_LEVEL(ch) <= 5)
    hp = 0;

  if (GET_HIT(ch) > GET_MAX_HIT(ch)) {
    GET_HIT(ch)--;
  } else {
    GET_HIT(ch) = MIN(GET_HIT(ch) + hp, GET_MAX_HIT(ch));
  }
  if (GET_MOVE(ch) > GET_MAX_MOVE(ch)) {
    GET_MOVE(ch)--;
  } else if (!AFF_FLAGGED(ch, AFF_FATIGUED)) {
    GET_MOVE(ch) = MIN(GET_MOVE(ch) + (hp * 3), GET_MAX_MOVE(ch));
  }
  if (GET_MANA(ch) > GET_MAX_MANA(ch)) {
    GET_MANA(ch)--;
  } else {
    GET_MANA(ch) = MIN(GET_MANA(ch) + (hp * 2), GET_MAX_MANA(ch));
  }

  update_pos(ch);  
  return;
}


/* The hit_limit, mana_limit, and move_limit functions are gone.  They added an
 * unnecessary level of complexity to the internal structure, weren't
 * particularly useful, and led to some annoying bugs.  From the players' point
 * of view, the only difference the removal of these functions will make is
 * that a character's age will now only affect the HMV gain per tick, and _not_
 * the HMV maximums. */
/* manapoint gain pr. game hour */
int mana_gain(struct char_data *ch)
{
  int gain;

  if (IS_NPC(ch)) {
    /* Neat and fast */
    gain = GET_LEVEL(ch);
  } else {
    gain = graf(age(ch)->year, 4, 8, 12, 16, 12, 10, 8);

    /* Class calculations */

    /* Skill/Spell calculations */

    /* Position calculations    */
    switch (GET_POS(ch)) {
    case POS_SLEEPING:
      gain *= 2;
      break;
    case POS_RESTING:
      gain += (gain / 2);	/* Divide by 2 */
      break;
    case POS_SITTING:
      gain += (gain / 4);	/* Divide by 4 */
      break;
    }

    if (IS_MAGIC_USER(ch) || IS_CLERIC(ch) || IS_SORCERER(ch))
      gain *= 2;

    if ((GET_COND(ch, HUNGER) == 0) || (GET_COND(ch, THIRST) == 0))
      gain /= 4;
  }

  if (AFF_FLAGGED(ch, AFF_POISON))
    gain /= 4;

  return (gain);
}

/* Hitpoint gain pr. game hour */
int hit_gain(struct char_data *ch)
{
  int gain;

  if (IS_NPC(ch)) {
    /* Neat and fast */
    gain = GET_LEVEL(ch);
  } else {

    gain = graf(age(ch)->year, 8, 12, 20, 32, 16, 10, 4);

    /* Class/Level calculations */
    /* Skill/Spell calculations */
    /* Position calculations    */

    switch (GET_POS(ch)) {
    case POS_SLEEPING:
      gain += (gain / 2);	/* Divide by 2 */
      break;
    case POS_RESTING:
      gain += (gain / 4);	/* Divide by 4 */
      break;
    case POS_SITTING:
      gain += (gain / 8);	/* Divide by 8 */
      break;
    }

    if (IS_MAGIC_USER(ch) || IS_CLERIC(ch))
      gain /= 2;	/* Ouch. */

    if ((GET_COND(ch, HUNGER) == 0) || (GET_COND(ch, THIRST) == 0))
      gain /= 4;
  }

  if (AFF_FLAGGED(ch, AFF_POISON))
    gain /= 4;

  return (gain);
}

/* move gain pr. game hour */
int move_gain(struct char_data *ch)
{
  int gain;

  if (IS_NPC(ch)) {
    /* Neat and fast */
    gain = GET_LEVEL(ch);
  } else {
    gain = graf(age(ch)->year, 16, 20, 24, 20, 16, 12, 10);

    /* Class/Level calculations */
    /* Skill/Spell calculations */
    /* Position calculations    */
    switch (GET_POS(ch)) {
    case POS_SLEEPING:
      gain += (gain / 2);	/* Divide by 2 */
      break;
    case POS_RESTING:
      gain += (gain / 4);	/* Divide by 4 */
      break;
    case POS_SITTING:
      gain += (gain / 8);	/* Divide by 8 */
      break;
    }

    if ((GET_COND(ch, HUNGER) == 0) || (GET_COND(ch, THIRST) == 0))
      gain /= 4;
  }

  if (AFF_FLAGGED(ch, AFF_POISON))
    gain /= 4;

  return (gain);
}

void set_title(struct char_data *ch, char *title)
{
  if (GET_TITLE(ch) != NULL)
    free(GET_TITLE(ch));

  //why are we checking sex?  old title system -zusuk
  //OK to remove sex check!
  if (title == NULL) {
    GET_TITLE(ch) = strdup(GET_SEX(ch) == SEX_FEMALE ?
      titles(GET_CLASS(ch), GET_LEVEL(ch)) :
      titles(GET_CLASS(ch), GET_LEVEL(ch)));
  } else {
    if (strlen(title) > MAX_TITLE_LENGTH)
      title[MAX_TITLE_LENGTH] = '\0';

    GET_TITLE(ch) = strdup(title);
  }
}

void run_autowiz(void)
{
#if defined(CIRCLE_UNIX) || defined(CIRCLE_WINDOWS)
  if (CONFIG_USE_AUTOWIZ) {
    size_t res;
    char buf[1024];
    int i;

#if defined(CIRCLE_UNIX)
    res = snprintf(buf, sizeof(buf), "nice ../bin/autowiz %d %s %d %s %d &",
	CONFIG_MIN_WIZLIST_LEV, WIZLIST_FILE, LVL_IMMORT, IMMLIST_FILE, (int) getpid());
#elif defined(CIRCLE_WINDOWS)
    res = snprintf(buf, sizeof(buf), "autowiz %d %s %d %s",
	CONFIG_MIN_WIZLIST_LEV, WIZLIST_FILE, LVL_IMMORT, IMMLIST_FILE);
#endif /* CIRCLE_WINDOWS */

    /* Abusing signed -> unsigned conversion to avoid '-1' check. */
    if (res < sizeof(buf)) {
      mudlog(CMP, LVL_IMMORT, FALSE, "Initiating autowiz.");
      i = system(buf);
      reboot_wizlists();
    } else
      log("Cannot run autowiz: command-line doesn't fit in buffer.");
  }
#endif /* CIRCLE_UNIX || CIRCLE_WINDOWS */
}


/* changed to return gain */
#define NEWBIE_LEVEL	5   /* max level for newbie bonus */
#define NEWBIE_EXP       150 /* bonus in percent */
#define NUM_MOBS_10      50
#define NUM_MOBS_20      100
#define NUM_MOBS_25      200
int gain_exp(struct char_data *ch, int gain)
{
  if (!IS_NPC(ch) && ((GET_LEVEL(ch) < 1 || GET_LEVEL(ch) >= LVL_IMMORT)))
    return 0;

  if (IS_NPC(ch)) {
    GET_EXP(ch) += gain;
    return 0;
  }

  if (gain > 0) {
    
    /* happy hour bonus */
    if ((IS_HAPPYHOUR) && (IS_HAPPYEXP))
      gain += (int)((float)gain * ((float)HAPPY_EXP / (float)(100)));

    /* newbie bonus */
    if (GET_LEVEL(ch) <= NEWBIE_LEVEL)
      gain += (int)((float)gain * ((float)NEWBIE_EXP / (float)(100)));
    
    /* put an absolute cap on the max gain per kill */
    gain = MIN(CONFIG_MAX_EXP_GAIN, gain);
    
    /* new gain xp cap -zusuk */
    GET_EXP(ch) += gain;

  } else if (gain < 0) {
    
    gain = MAX(-CONFIG_MAX_EXP_LOSS, gain);	/* Cap max exp lost per death */
    GET_EXP(ch) += gain;
    if (GET_EXP(ch) < 0)
      GET_EXP(ch) = 0;

  }
  if (GET_LEVEL(ch) >= LVL_IMMORT && !PLR_FLAGGED(ch, PLR_NOWIZLIST))
    run_autowiz();

  return gain;
}


void gain_exp_regardless(struct char_data *ch, int gain)
{
  int is_altered = FALSE;
  int num_levels = 0;

  if ((IS_HAPPYHOUR) && (IS_HAPPYEXP))
    gain += (int)((float)gain * ((float)HAPPY_EXP / (float)(100)));

  GET_EXP(ch) += gain;
  if (GET_EXP(ch) < 0)
    GET_EXP(ch) = 0;

  if (!IS_NPC(ch)) {
    while (GET_LEVEL(ch) < LVL_IMPL &&
	GET_EXP(ch) >= level_exp(ch, GET_LEVEL(ch) + 1)) {
      GET_LEVEL(ch) += 1;
      CLASS_LEVEL(ch, GET_CLASS(ch))++;
      num_levels++;
      advance_level(ch, GET_CLASS(ch));
      is_altered = TRUE;
    }

    if (is_altered) {
      mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s advanced %d level%s to level %d.",
		GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s", GET_LEVEL(ch));
      if (num_levels == 1)
        send_to_char(ch, "You rise a level!\r\n");
      else
	send_to_char(ch, "You rise %d levels!\r\n", num_levels);
      set_title(ch, NULL);
    }
  }
  if (GET_LEVEL(ch) >= LVL_IMMORT && !PLR_FLAGGED(ch, PLR_NOWIZLIST))
    run_autowiz();
}

void gain_condition(struct char_data *ch, int condition, int value)
{
  bool intoxicated;

  if (IS_NPC(ch) || GET_COND(ch, condition) == -1)	/* No change */
    return;

  intoxicated = (GET_COND(ch, DRUNK) > 0);

  GET_COND(ch, condition) += value;

  GET_COND(ch, condition) = MAX(0, GET_COND(ch, condition));
  GET_COND(ch, condition) = MIN(24, GET_COND(ch, condition));

  if (GET_COND(ch, condition) || PLR_FLAGGED(ch, PLR_WRITING))
    return;

  switch (condition) {
  case HUNGER:
    send_to_char(ch, "You are hungry.\r\n");
    break;
  case THIRST:
    send_to_char(ch, "You are thirsty.\r\n");
    break;
  case DRUNK:
    if (intoxicated)
      send_to_char(ch, "You are now sober.\r\n");
    break;
  default:
    break;
  }

}

static void check_idling(struct char_data *ch)
{
  if (ch->char_specials.timer > CONFIG_IDLE_VOID) {
    if (GET_WAS_IN(ch) == NOWHERE && IN_ROOM(ch) != NOWHERE) {
      GET_WAS_IN(ch) = IN_ROOM(ch);
      if (FIGHTING(ch)) {
	stop_fighting(FIGHTING(ch));
	stop_fighting(ch);
      }
      act("$n disappears into the void.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char(ch, "You have been idle, and are pulled into a void.\r\n");
      save_char(ch);
      Crash_crashsave(ch);
      char_from_room(ch);
      char_to_room(ch, 1);
    } else if (ch->char_specials.timer > CONFIG_IDLE_RENT_TIME) {
      if (IN_ROOM(ch) != NOWHERE)
	char_from_room(ch);
      char_to_room(ch, 3);
      if (ch->desc) {
	STATE(ch->desc) = CON_DISCONNECT;
	/*
	 * For the 'if (d->character)' test in close_socket().
	 * -gg 3/1/98 (Happy anniversary.)
	 */
	ch->desc->character = NULL;
	ch->desc = NULL;
      }
      if (CONFIG_FREE_RENT)
	Crash_rentsave(ch, 0);
      else
	Crash_idlesave(ch);
      mudlog(CMP, LVL_GOD, TRUE, "%s force-rented and extracted (idle).", GET_NAME(ch));
      add_llog_entry(ch, LAST_IDLEOUT);
      extract_char(ch);
    }
  }
}

/* Update PCs, NPCs, and objects */
void point_update(void)
{
  struct char_data *i = NULL, *next_char = NULL;
  struct obj_data *j = NULL, *next_thing, *jj = NULL, *next_thing2 = NULL;

  /** general **/
  /* Take 1 from the happy-hour tick counter, and end happy-hour if zero */
  if (HAPPY_TIME > 1)
    HAPPY_TIME--;

  /* Last tick - set everything back to zero */
  else if (HAPPY_TIME == 1) {
    HAPPY_QP = 0;
    HAPPY_EXP = 0;
    HAPPY_GOLD = 0;
    HAPPY_TIME = 0;
    game_info("Happy hour has ended!");
  }
  
  
  /** characters **/
  for (i = character_list; i; i = next_char) {
    next_char = i->next;

    gain_condition(i, HUNGER, -1);
    gain_condition(i, DRUNK, -1);
    gain_condition(i, THIRST, -1);

    /* old tick regen code use to be here -zusuk */
    
    if (!IS_NPC(i)) {
      update_char_objects(i);
      (i->char_specials.timer)++;
      if (GET_LEVEL(i) < CONFIG_IDLE_MAX_LEVEL)
        check_idling(i);
    }
  }

  
  /** objects **/
  /* Make sure there is only one way to decrement each object timer */
  for (j = object_list; j; j = next_thing) {
    next_thing = j->next;	/* Next in object list */

    /** portals that fade **/
    if (IS_DECAYING_PORTAL(j)) {
      /* timer count down */
      if (GET_OBJ_TIMER(j) > 0)
        GET_OBJ_TIMER(j)--;
      
      /* the portal fades */
      if (GET_OBJ_TIMER(j) <= 0) {
        /* send message if it makes sense */
        if ((IN_ROOM(j) != NOWHERE) && (world[IN_ROOM(j)].people)) {
          act("\tnYou watch as $p \tCs\tMh\tCi\tMm\tCm\tMe\tCr\tMs\tn then "
                  "fades, then disappears.", TRUE, world[IN_ROOM(j)].people,
                  j, 0, TO_ROOM);
          act("\tnYou watch as $p \tCs\tMh\tCi\tMm\tCm\tMe\tCr\tMs\tn then "
                  "fades, then disappears.", TRUE, world[IN_ROOM(j)].people,
                  j, 0, TO_CHAR);
	   }
        extract_obj(j);
        
      } /* end portal fade */

    /** If this is a corpse **/
    }  else if (IS_CORPSE(j)) {
      /* timer count down */
      if (GET_OBJ_TIMER(j) > 0)
        GET_OBJ_TIMER(j)--;

      /* corpse decayed */
      if (GET_OBJ_TIMER(j) <= 0) {
        if (j->carried_by)
          act("$p decays in your hands.", FALSE, j->carried_by, j, 0, TO_CHAR);
        else if ((IN_ROOM(j) != NOWHERE) && (world[IN_ROOM(j)].people)) {
          act("A quivering horde of maggots consumes $p.",
                TRUE, world[IN_ROOM(j)].people, j, 0, TO_ROOM);
          act("A quivering horde of maggots consumes $p.",
                TRUE, world[IN_ROOM(j)].people, j, 0, TO_CHAR);
	   }
      
        for (jj = j->contains; jj; jj = next_thing2) {
          next_thing2 = jj->next_content;	/* Next in inventory */
          obj_from_obj(jj);

          if (j->in_obj)
            obj_to_obj(jj, j->in_obj);
          else if (j->carried_by)
            obj_to_room(jj, IN_ROOM(j->carried_by));
          else if (IN_ROOM(j) != NOWHERE)
            obj_to_room(jj, IN_ROOM(j));
          else
            core_dump();
        }
        extract_obj(j);
      }
    /** for timed object triggers **/
    } else if (GET_OBJ_TIMER(j) > 0) {
      /* If the timer is set, count it down and at 0, try the trigger
       * this should be last in point-update() */
      GET_OBJ_TIMER(j)--;
      if (GET_OBJ_TIMER(j) <= 0)
        timer_otrigger(j);
    }
  }

}


/* Note: amt may be negative */
int increase_gold(struct char_data *ch, int amt)
{
  int curr_gold = 0;

  curr_gold = GET_GOLD(ch);

  if (amt < 0) {
    GET_GOLD(ch) = MAX(0, curr_gold+amt);
    /* Validate to prevent overflow */
    if (GET_GOLD(ch) > curr_gold) GET_GOLD(ch) = 0;
  } else {
    GET_GOLD(ch) = MIN(MAX_GOLD, curr_gold+amt);
    /* Validate to prevent overflow */
    if (GET_GOLD(ch) < curr_gold) GET_GOLD(ch) = MAX_GOLD;
  }
  if (GET_GOLD(ch) == MAX_GOLD)
    send_to_char(ch, "%sYou have reached the maximum gold!\r\n%sYou must spend it or bank it before you can gain any more.\r\n", QBRED, QNRM);

  return (GET_GOLD(ch));
}

int decrease_gold(struct char_data *ch, int deduction)
{
  int amt;
  amt = (deduction * -1);
  increase_gold(ch, amt);
  return (GET_GOLD(ch));
}

int increase_bank(struct char_data *ch, int amt)
{
  int curr_bank;

  if (IS_NPC(ch)) return 0;

  curr_bank = GET_BANK_GOLD(ch);

  if (amt < 0) {
    GET_BANK_GOLD(ch) = MAX(0, curr_bank+amt);
    /* Validate to prevent overflow */
    if (GET_BANK_GOLD(ch) > curr_bank) GET_BANK_GOLD(ch) = 0;
  } else {
    GET_BANK_GOLD(ch) = MIN(MAX_BANK, curr_bank+amt);
    /* Validate to prevent overflow */
    if (GET_BANK_GOLD(ch) < curr_bank) GET_BANK_GOLD(ch) = MAX_BANK;
  }
  if (GET_BANK_GOLD(ch) == MAX_BANK)
    send_to_char(ch, "%sYou have reached the maximum bank balance!\r\n%sYou cannot put more into your account unless you withdraw some first.\r\n", QBRED, QNRM);
  return (GET_BANK_GOLD(ch));
}

int decrease_bank(struct char_data *ch, int deduction)
{
  int amt;
  amt = (deduction * -1);
  increase_bank(ch, amt);
  return (GET_BANK_GOLD(ch));
}
	
void increase_anger(struct char_data *ch, float amount)
{
  if (IS_NPC(ch) && GET_ANGER(ch) <= MAX_ANGER)
   GET_ANGER(ch) = MIN(MAX(GET_ANGER(ch) + amount, 0), MAX_ANGER);
}

