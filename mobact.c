/**************************************************************************
 *  File: mobact.c                                          Part of tbaMUD *
 *  Usage: Functions for generating intelligent (?) behavior in mobiles.   *
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
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "spells.h"
#include "constants.h"
#include "act.h"
#include "graph.h"
#include "fight.h"
#include "spec_procs.h"
#include "mud_event.h" /* for eSTUNNED */

/* local file scope only function prototypes, defines, externs, etc */

/* end local */






/*** UTILITY FUNCTIONS ***/

/* function to move a mobile along a specified path (patrols) */
bool move_on_path(struct char_data *ch) {
  int dir = -1;
  int next = 0;

  if (!ch)
    return FALSE;

  if (FIGHTING(ch))
    return FALSE;

  /* finished path */
  if (PATH_SIZE(ch) < 1)
    return FALSE;

  /* stuck in a spot for a moment */
  if (PATH_DELAY(ch) > 0) {
    PATH_DELAY(ch)--;
    return FALSE;
  }

  PATH_DELAY(ch) = PATH_RESET(ch);
  PATH_INDEX(ch)++;

  if (PATH_INDEX(ch) >= PATH_SIZE(ch) || PATH_INDEX(ch) < 0)
    PATH_INDEX(ch) = 0;

  next = GET_PATH(ch, PATH_INDEX(ch));
  dir = find_first_step(ch->in_room, real_room(next));

  if (dir >= 0)
    perform_move(ch, dir, 1);

  return TRUE;
}

/* mobile echo system, from homeland ported by nashak */
void mobile_echos(struct char_data *ch) {
  char *echo;
  struct descriptor_data *d;

  if (!ECHO_COUNT(ch))
    return;

  if (rand_number(1, 75) > (ECHO_FREQ(ch) / 2))
    return;

  if (ECHO_SEQUENTIAL(ch)) {
    echo = ECHO_ENTRIES(ch)[CURRENT_ECHO(ch)++];
    if (CURRENT_ECHO(ch) >= ECHO_COUNT(ch))
      CURRENT_ECHO(ch) = 0;
  } else {
    echo = ECHO_ENTRIES(ch)[rand_number(0, ECHO_COUNT(ch) - 1)];
  }

  if (!echo)
    return;

  if (ECHO_IS_ZONE(ch)) {
    for (d = descriptor_list; d; d = d->next) {
      if (!d->character)
        continue;
      if (d->character->in_room == NOWHERE || ch->in_room == NOWHERE)
        continue;
      if (world[d->character->in_room].zone != world[ch->in_room].zone)
        continue;
      if (!AWAKE(d->character))
        continue;

      send_to_char(d->character, "%s\r\n", echo);
    }
  } else {
    act(echo, FALSE, ch, 0, 0, TO_ROOM);
  }
}

/* Mob Memory Routines */

/* make ch remember victim */
void remember(struct char_data *ch, struct char_data *victim) {
  memory_rec *tmp = NULL;
  bool present = FALSE;

  if (!IS_NPC(ch) || IS_NPC(victim) || PRF_FLAGGED(victim, PRF_NOHASSLE))
    return;

  for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
    if (tmp->id == GET_IDNUM(victim))
      present = TRUE;

  if (!present) {
    CREATE(tmp, memory_rec, 1);
    tmp->next = MEMORY(ch);
    tmp->id = GET_IDNUM(victim);
    MEMORY(ch) = tmp;
  }
}

/* make ch forget victim */
void forget(struct char_data *ch, struct char_data *victim) {
  memory_rec *curr = NULL, *prev = NULL;

  if (!(curr = MEMORY(ch)))
    return;

  while (curr && curr->id != GET_IDNUM(victim)) {
    prev = curr;
    curr = curr->next;
  }

  if (!curr)
    return; /* person wasn't there at all. */

  if (curr == MEMORY(ch))
    MEMORY(ch) = curr->next;
  else
    prev->next = curr->next;

  free(curr);
}

/* erase ch's memory */
void clearMemory(struct char_data *ch) {
  memory_rec *curr = NULL, *next = NULL;

  curr = MEMORY(ch);

  while (curr) {
    next = curr->next;
    free(curr);
    curr = next;
  }

  MEMORY(ch) = NULL;
}

/* An aggressive mobile wants to attack something.  If they're under the 
 * influence of mind altering PC, then see if their master can talk them out 
 * of it, eye them down, or otherwise intimidate the slave. */
static bool aggressive_mob_on_a_leash(struct char_data *slave, struct char_data *master, struct char_data *attack) {
  static int snarl_cmd = 0, sneer_cmd = 0;
  int dieroll = 0;

  if (!slave)
    return FALSE;

  if (!master || !AFF_FLAGGED(slave, AFF_CHARM))
    return (FALSE);

  if (!snarl_cmd)
    snarl_cmd = find_command("snarl");
  if (!sneer_cmd)
    sneer_cmd = find_command("sneer");

  /* Sit. Down boy! HEEEEeeeel! */
  dieroll = rand_number(1, 20);
  if (dieroll != 1 && (dieroll == 20 || dieroll > 10 -
          GET_CHA_BONUS(master) + GET_WIS_BONUS(slave))) {
    if (snarl_cmd > 0 && attack && !rand_number(0, 3)) {
      char victbuf[MAX_NAME_LENGTH + 1];

      strncpy(victbuf, GET_NAME(attack), sizeof (victbuf)); /* strncpy: OK */
      victbuf[sizeof (victbuf) - 1] = '\0';

      do_action(slave, victbuf, snarl_cmd, 0);
    }

    /* Success! But for how long? Hehe. */
    return (TRUE);
  }

  /* indicator that he/she isn't happy! */
  if (snarl_cmd > 0 && attack) {
    char victbuf[MAX_NAME_LENGTH + 1];

    strncpy(victbuf, GET_NAME(attack), sizeof (victbuf)); /* strncpy: OK */
    victbuf[sizeof (victbuf) - 1] = '\0';

    do_action(slave, victbuf, sneer_cmd, 0);
  }

  return (FALSE);
}

/* function encapsulating conditions that will stop the mobile from
 acting */
int canContinue(struct char_data *ch) {
  //dummy checks
  if (FIGHTING(ch) == NULL || IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))) {
    stop_fighting(ch);
    return 0;
  }
  if (GET_MOB_WAIT(ch) > 0)
    return 0;
  if (GET_POS(ch) <= POS_SITTING)
    return 0;
  if (IS_CASTING(ch))
    return 0;
  if (GET_HIT(ch) <= 1)
    return 0;

  // this combined with PULSE_MOBILE will control how often they proc
  //  if (!rand_number(0,3))
  //    return 0;

  return 1;
}

/*** END UTILITY FUNCTIONS ***/

/*** RACE TYPES ***/

/* racial behaviour function */
void npc_racial_behave(struct char_data *ch) {
  struct char_data *AoE = NULL, *vict = NULL;
  int engaged = 0;

  if (!canContinue(ch))
    return;

  //semi randomly choose vict, determine if can AoE
  for (AoE = world[IN_ROOM(ch)].people; AoE; AoE = AoE->next_in_room)
    if (FIGHTING(AoE) == ch) {
      engaged++;
      if (!rand_number(0, 4)) {
        vict = AoE;
      }
    }
  if (vict == NULL && IN_ROOM(FIGHTING(ch)) == IN_ROOM(ch))
    vict = FIGHTING(ch);
  if (vict == NULL)
    return;
  if (!CAN_SEE(ch, vict))
    return;

  //first figure out which race we are dealing with
  switch (GET_RACE(ch)) {
    case NPCRACE_ANIMAL:
      switch (rand_number(1, 2)) {
        case 1:
          do_rage(ch, 0, 0, 0);
        default:
          break;
      }
      break;
    case NPCRACE_DRAGON:
      if (!HUNTING(ch))
        HUNTING(ch) = vict;

      switch (rand_number(1, 4)) {
        case 1:
          do_tailsweep(ch, 0, 0, 0);
          break;
        case 2:
          do_breathe(ch, 0, 0, 0);
          break;
        case 3:
          do_frightful(ch, 0, 0, 0);
          break;
        default:
          break;
      }
      break;
    default:
      switch (GET_LEVEL(ch)) {
        default:
          break;
      }
      break;

  }

}

/*** CLASSES ***/

// monk behaviour, behave based on level

void npc_monk_behave(struct char_data *ch, struct char_data *vict,
        int level, int engaged) {

  switch (rand_number(5, level)) {
    case 5: // level 1-4 mobs won't act
      break;
    default:
      break;
  }
}

// rogue behaviour, behave based on level

void npc_rogue_behave(struct char_data *ch, struct char_data *vict,
        int level, int engaged) {

  switch (rand_number(5, level)) {
    case 5: // level 1-4 mobs won't act
      break;
    default:
      break;
  }
}


// warrior behaviour, behave based on circle

void npc_warrior_behave(struct char_data *ch, struct char_data *vict,
        int level, int engaged) {

  // going to prioritize rescuing master (if he has one)
  if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
    if (FIGHTING(ch->master)) {
      do_npc_rescue(ch, ch->master);
      return;
    }
  }

  switch (rand_number(5, level)) {
    case 5: // level 1-4 mobs won't act
      break;
    default:
      break;
  }
}


// ranger behaviour, behave based on level

void npc_ranger_behave(struct char_data *ch, struct char_data *vict,
        int level, int engaged) {

  // going to prioritize rescuing master (if he has one)
  if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
    if (FIGHTING(ch->master)) {
      do_npc_rescue(ch, ch->master);
      return;
    }
  }

  switch (rand_number(5, level)) {
    case 5: // level 1-4 mobs won't act
      break;
    default:
      break;
  }
}


// paladin behaviour, behave based on level

void npc_paladin_behave(struct char_data *ch, struct char_data *vict,
        int level, int engaged) {

  // going to prioritize rescuing master (if he has one)
  if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
    if (FIGHTING(ch->master)) {
      do_npc_rescue(ch, ch->master);
      return;
    }
  }

  switch (rand_number(5, level)) {
    case 5: // level 1-4 mobs won't act
      break;
    default:
      break;
  }
}


// cleric behaviour, behave based on circle

void npc_cleric_behave(struct char_data *ch, struct char_data *vict,
        int circle, int engaged) {

  switch (rand_number(3, circle)) {
    case 3: // level 1-4 mobs won't cast
      switch (rand_number(1, 3)) {
        case 1:
          if (!affected_by_spell(ch, SPELL_ARMOR))
            cast_spell(ch, ch, NULL, SPELL_ARMOR);
          else
            cast_spell(ch, vict, NULL, SPELL_CAUSE_LIGHT_WOUNDS);
          break;
        case 2:
          if (!affected_by_spell(ch, SPELL_ENDURANCE))
            cast_spell(ch, ch, NULL, SPELL_ENDURANCE);
          else
            cast_spell(ch, vict, NULL, SPELL_CAUSE_MODERATE_WOUNDS);
          break;
        case 3:
          if (GET_HIT(ch) < GET_MAX_HIT(ch))
            if (circle >= 7)
              cast_spell(ch, ch, NULL, SPELL_HEAL);
            else if (circle >= 5)
              cast_spell(ch, ch, NULL, SPELL_CURE_CRITIC);
            else
              cast_spell(ch, ch, NULL, SPELL_CURE_LIGHT);
          else
            cast_spell(ch, vict, NULL, SPELL_CAUSE_LIGHT_WOUNDS);
      }
      break;
    case 4:
      switch (rand_number(1, 2)) {
        case 1:
          if (!affected_by_spell(ch, SPELL_BLESS))
            cast_spell(ch, ch, NULL, SPELL_BLESS);
          else
            cast_spell(ch, vict, NULL, SPELL_CAUSE_SERIOUS_WOUNDS);
          break;
        case 2:
          if (AFF_FLAGGED(ch, AFF_BLIND))
            cast_spell(ch, ch, NULL, SPELL_CURE_BLIND);
          else
            cast_spell(ch, vict, NULL, SPELL_CAUSE_MODERATE_WOUNDS);
          break;
      }
      break;
    case 5:
      switch (rand_number(1, 3)) {
        case 1:
          if (!affected_by_spell(ch, SPELL_INFRAVISION))
            cast_spell(ch, ch, NULL, SPELL_INFRAVISION);
          else
            cast_spell(ch, vict, NULL, SPELL_CAUSE_CRITICAL_WOUNDS);
          break;
        case 2:
          if (AFF_FLAGGED(ch, AFF_CURSE))
            cast_spell(ch, ch, NULL, SPELL_REMOVE_CURSE);
          else
            cast_spell(ch, vict, NULL, SPELL_CAUSE_CRITICAL_WOUNDS);
          break;
        case 3:
          if (GET_HIT(ch) < GET_MAX_HIT(ch))
            if (circle >= 7)
              cast_spell(ch, ch, NULL, SPELL_HEAL);
            else
              cast_spell(ch, ch, NULL, SPELL_CURE_CRITIC);
          else
            cast_spell(ch, vict, NULL, SPELL_CAUSE_CRITICAL_WOUNDS);
          break;
      }
      break;
    case 6:
      switch (rand_number(1, 3)) {
        case 1:
          if (!AFF_FLAGGED(vict, AFF_BLIND))
            cast_spell(ch, vict, NULL, SPELL_BLINDNESS);
          else
            cast_spell(ch, vict, NULL, SPELL_FLAME_STRIKE);
          break;
        case 2:
          if (!AFF_FLAGGED(vict, AFF_POISON))
            cast_spell(ch, vict, NULL, SPELL_POISON);
          else
            cast_spell(ch, vict, NULL, SPELL_FLAME_STRIKE);
          break;
        case 3:
          if (GET_ALIGNMENT(ch) < 0 && !AFF_FLAGGED(ch, AFF_PROTECT_GOOD))
            cast_spell(ch, ch, NULL, SPELL_PROT_FROM_GOOD);
          else if (GET_ALIGNMENT(ch) >= 0 && !AFF_FLAGGED(ch, AFF_PROTECT_EVIL))
            cast_spell(ch, ch, NULL, SPELL_PROT_FROM_EVIL);
          else
            cast_spell(ch, vict, NULL, SPELL_FLAME_STRIKE);
          break;
      }
      break;
    case 7:
      switch (rand_number(1, 3)) {
        case 1:
          if (AFF_FLAGGED(ch, AFF_POISON))
            cast_spell(ch, ch, NULL, SPELL_REMOVE_POISON);
          else
            cast_spell(ch, vict, NULL, SPELL_HARM);
          break;
        case 2:
          if (GET_MAX_HIT(ch) - GET_HIT(ch) > 75)
            cast_spell(ch, ch, NULL, SPELL_HEAL);
          else
            cast_spell(ch, vict, NULL, SPELL_HARM);
          break;
        case 3:
          if (GET_ALIGNMENT(ch) < 0 && GET_ALIGNMENT(vict) > 333)
            cast_spell(ch, vict, NULL, SPELL_DISPEL_GOOD);
          if (GET_ALIGNMENT(ch) >= 0 && GET_ALIGNMENT(vict) < -333)
            cast_spell(ch, vict, NULL, SPELL_DISPEL_EVIL);
          else
            cast_spell(ch, vict, NULL, SPELL_HARM);
          break;
      }
      break;
    case 8:
      switch (rand_number(1, 2)) {
        case 1:
          if (!AFF_FLAGGED(ch, AFF_SANCTUARY))
            cast_spell(ch, ch, NULL, SPELL_SANCTUARY);
          else
            cast_spell(ch, vict, NULL, SPELL_DESTRUCTION);
          break;
        case 2:
          if (!AFF_FLAGGED(ch, AFF_SENSE_LIFE))
            cast_spell(ch, ch, NULL, SPELL_SENSE_LIFE);
          else
            cast_spell(ch, vict, NULL, SPELL_CALL_LIGHTNING);
          break;
      }
      break;
    case 9:
      if (engaged >= 2)
        cast_spell(ch, vict, NULL, SPELL_EARTHQUAKE);
      else
        cast_spell(ch, vict, NULL, SPELL_DESTRUCTION);
      break;
    default:
      log("ERR:  Reached invalid circle in npc_cleric_behave.");
      break;
  }
}


// wizard behaviour, behave based on circle

void npc_wizard_behave(struct char_data *ch, struct char_data *vict,
        int circle, int engaged) {
  int num = -1;

  if (circle < 3)
    return;
  num = rand_number(3, circle);

  switch (num) {
    case 3: // level 1-4 mobs won't cast
      switch (rand_number(1, 2)) {
        case 1:
          if (!affected_by_spell(ch, SPELL_BLUR))
            cast_spell(ch, ch, NULL, SPELL_BLUR);
          else
            cast_spell(ch, vict, NULL, SPELL_CHILL_TOUCH);
          break;
        case 2:
          if (!affected_by_spell(ch, SPELL_DETECT_INVIS))
            cast_spell(ch, ch, NULL, SPELL_DETECT_INVIS);
          else
            cast_spell(ch, vict, NULL, SPELL_BURNING_HANDS);
          break;
      }
      break;
    case 4:
      switch (rand_number(1, 2)) {
        case 1:
          if (!affected_by_spell(ch, SPELL_INFRAVISION))
            cast_spell(ch, ch, NULL, SPELL_INFRAVISION);
          else
            cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP);
          break;
        case 2:
          if (!affected_by_spell(ch, SPELL_MIRROR_IMAGE))
            cast_spell(ch, ch, NULL, SPELL_MIRROR_IMAGE);
          else
            cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT);
          break;
      }
      break;
    case 5:
      switch (rand_number(1, 2)) {
        case 1:
          if (!affected_by_spell(ch, SPELL_ARMOR))
            cast_spell(ch, ch, NULL, SPELL_ARMOR);
          else
            cast_spell(ch, vict, NULL, SPELL_COLOR_SPRAY);
          break;
        case 2:
          if (!affected_by_spell(ch, SPELL_STONESKIN))
            cast_spell(ch, ch, NULL, SPELL_STONESKIN);
          else
            cast_spell(ch, vict, NULL, SPELL_FIREBALL);
          break;
      }
      break;
    case 6:
      if (engaged >= 2)
        cast_spell(ch, vict, NULL, SPELL_ICE_STORM);
      else
        cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT);
      break;
    case 7:
      switch (rand_number(1, 2)) {
        case 1:
          if (!affected_by_spell(ch, SPELL_STRENGTH))
            cast_spell(ch, ch, NULL, SPELL_STRENGTH);
          else
            cast_spell(ch, vict, NULL, SPELL_BALL_OF_LIGHTNING);
          break;
        case 2:
          if (!affected_by_spell(vict, SPELL_BLINDNESS))
            cast_spell(ch, vict, NULL, SPELL_BLINDNESS);
          else
            cast_spell(ch, vict, NULL, SPELL_BALL_OF_LIGHTNING);
          break;
      }
      break;
    case 8:
      switch (rand_number(1, 2)) {
        case 1:
          if (!affected_by_spell(vict, SPELL_CURSE))
            cast_spell(ch, vict, NULL, SPELL_CURSE);
          else
            cast_spell(ch, vict, NULL, SPELL_BALL_OF_LIGHTNING);
          break;
        case 2:
          if (engaged >= 2)
            cast_spell(ch, vict, NULL, SPELL_CHAIN_LIGHTNING);
          else
            cast_spell(ch, vict, NULL, SPELL_MISSILE_STORM);
          break;
      }
      break;
    case 9:
      switch (rand_number(1, 2)) {
        case 1:
          if (!affected_by_spell(vict, SPELL_POISON))
            cast_spell(ch, vict, NULL, SPELL_POISON);
          else
            cast_spell(ch, vict, NULL, SPELL_BALL_OF_LIGHTNING);
          break;
        case 2:
          if (engaged >= 2)
            cast_spell(ch, vict, NULL, SPELL_METEOR_SWARM);
          else
            cast_spell(ch, vict, NULL, SPELL_MISSILE_STORM);
          break;
      }
      break;
    default:
      log("ERR:  Reached invalid circle in npc_wizard_behave.");
      break;
  }

}

void npc_class_behave(struct char_data *ch) {
  struct char_data *AoE = NULL, *vict = NULL;
  int engaged = 0;

  if (MOB_FLAGGED(ch, MOB_NOCLASS))
    return;
  if (!canContinue(ch))
    return;
  if (GET_LEVEL(ch) < 5)
    return;

  //semi randomly choose vict, determine if can AoE
  for (AoE = world[IN_ROOM(ch)].people; AoE; AoE = AoE->next_in_room)
    if (FIGHTING(AoE) == ch) {
      engaged++;
      if (!rand_number(0, 4)) {
        vict = AoE;
      }
    }
  if (vict == NULL && IN_ROOM(FIGHTING(ch)) == IN_ROOM(ch))
    vict = FIGHTING(ch);
  if (vict == NULL)
    return;
  if (!CAN_SEE(ch, vict))
    return;

  switch (GET_CLASS(ch)) {
    case CLASS_SORCERER:
    case CLASS_WIZARD:
      npc_wizard_behave(ch, vict, getCircle(ch, CLASS_WIZARD), engaged);
      break;
    case CLASS_BARD:
      npc_wizard_behave(ch, vict, getCircle(ch, CLASS_BARD), engaged);
      break;
    case CLASS_BERSERKER:
    case CLASS_WARRIOR:
      npc_warrior_behave(ch, vict, GET_LEVEL(ch), engaged);
      break;
    case CLASS_PALADIN:
      npc_paladin_behave(ch, vict, GET_LEVEL(ch), engaged);
      break;
    case CLASS_RANGER:
      npc_ranger_behave(ch, vict, GET_LEVEL(ch), engaged);
      break;
    case CLASS_ROGUE:
      npc_rogue_behave(ch, vict, GET_LEVEL(ch), engaged);
      break;
    case CLASS_DRUID:
    case CLASS_CLERIC:
      npc_cleric_behave(ch, vict, getCircle(ch, CLASS_CLERIC), engaged);
      break;
    case CLASS_MONK:
      npc_monk_behave(ch, vict, GET_LEVEL(ch), engaged);
      break;
    default:
      log("ERR:  Reached invalid class in npc_class_behave.");
      break;
  }
}

/* generic function for spelling up as a caster */
void npc_spellup(struct char_data *ch) {
  struct obj_data *obj = NULL;
  int level;
  /* our priorities are going to be in this order:
   1)  get a charmee
   2)  heal (heal group)
   3)  spellup (spellup group)
   */
  if (!ch)
    return;

  /* create a group in case we need it */
  if (!GROUP(ch))
    create_group(ch);
  
  if (GET_LEVEL(ch) >= LVL_IMMORT)
    level = LVL_IMMORT - 1;
  else
    level = GET_LEVEL(ch);

  /* try animate undead first */
  if (!HAS_PET_UNDEAD(ch)) {
    for (obj = world[ch->in_room].contents; obj; obj = obj->next_content) {
      if (!IS_CORPSE(obj))
        continue;
      if (level >= spell_info[SPELL_GREATER_ANIMATION].min_level[GET_CLASS(ch)]) {
        cast_spell(ch, NULL, obj, SPELL_GREATER_ANIMATION);
        return;
      }
    }
  }
  
  /* try for an elemental */
  if (!HAS_PET_ELEMENTAL(ch)) {
    if (level >= spell_info[SPELL_SUMMON_CREATURE_9].min_level[GET_CLASS(ch)]) {
      cast_spell(ch, NULL, NULL, SPELL_SUMMON_CREATURE_9);
      return;
    }    
    else if (level >= spell_info[SPELL_SUMMON_CREATURE_8].min_level[GET_CLASS(ch)]) {
      cast_spell(ch, NULL, NULL, SPELL_SUMMON_CREATURE_8);
      return;
    }    
    else if (level >= spell_info[SPELL_SUMMON_CREATURE_7].min_level[GET_CLASS(ch)]) {
      cast_spell(ch, NULL, NULL, SPELL_SUMMON_CREATURE_7);
      return;
    }    
  }
  
  /* try healing */
  
  
  return;
}

/*** MOBILE ACTIVITY ***/

/* the primary engine for mobile activity */
void mobile_activity(void) {
  struct char_data *ch = NULL, *next_ch = NULL, *vict = NULL;
  struct obj_data *obj = NULL, *best_obj = NULL;
  int door = 0, found = 0, max = 0, where = -1;
  memory_rec *names = NULL;

  for (ch = character_list; ch; ch = next_ch) {
    next_ch = ch->next;

    if (!IS_MOB(ch))
      continue;

    if (AFF_FLAGGED(ch, AFF_STUN) || AFF_FLAGGED(ch, AFF_PARALYZED) ||
            char_has_mud_event(ch, eSTUNNED) || AFF_FLAGGED(ch, AFF_NAUSEATED)) {
      send_to_char(ch, "You are unable to move!\r\n");
      continue;
    }

    /* Examine call for special procedure */
    /* not the AWAKE() type of checks are inside the spec_procs */
    if (MOB_FLAGGED(ch, MOB_SPEC) && !no_specials) {
      if (mob_index[GET_MOB_RNUM(ch)].func == NULL) {
        log("SYSERR: %s (#%d): Attempting to call non-existing mob function.",
                GET_NAME(ch), GET_MOB_VNUM(ch));
        REMOVE_BIT_AR(MOB_FLAGS(ch), MOB_SPEC);
      } else {
        char actbuf[MAX_INPUT_LENGTH] = "";
        if ((mob_index[GET_MOB_RNUM(ch)].func) (ch, ch, 0, actbuf))
          continue; /* go to next char */
      }
    }

    /* can't do any of the following if not at least AWAKE() and not casting */
    if (!AWAKE(ch) || IS_CASTING(ch))
      continue;

    /* If the mob has no specproc, do the default actions */

    /* follow set path for mobile (like patrols) */
    if (move_on_path(ch))
      continue;

    // entry point for npc race and class behaviour in combat -zusuk
    if (FIGHTING(ch)) {
      // 50% chance will react off of class, 50% chance will react off of race
      if (rand_number(0, 1))
        npc_racial_behave(ch);
      else
        npc_class_behave(ch);
      continue;
    } else if (IS_NPC_CASTER(ch) && rand_number(0, 1)) {
      /* not in combat */
      npc_spellup(ch);
    }

    /* send out mobile echos to room or zone */
    mobile_echos(ch);

    /* hunt a victim, if applicable */
    hunt_victim(ch);

    /* Scavenger (picking up objects) */
    if (MOB_FLAGGED(ch, MOB_SCAVENGER))
      if (world[IN_ROOM(ch)].contents && !rand_number(0, 10)) {
        max = 1;
        best_obj = NULL;
        for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj->next_content)
          if (CAN_GET_OBJ(ch, obj) && GET_OBJ_COST(obj) > max) {
            best_obj = obj;
            max = GET_OBJ_COST(obj);
          }
        if (best_obj != NULL) {
          obj_from_room(best_obj);
          obj_to_char(best_obj, ch);
          act("$n gets $p.", FALSE, ch, best_obj, 0, TO_ROOM);
          
          if ((where = find_eq_pos(ch, best_obj, 0)) > 0)
            perform_wear(ch, best_obj, where);

          continue;
        }
      }

    /* Mob Movement */
    if (rand_number(0, 1)) //customize frequency
      if (!MOB_FLAGGED(ch, MOB_SENTINEL) && (GET_POS(ch) == POS_STANDING) &&
              ((door = rand_number(0, 18)) < DIR_COUNT) && CAN_GO(ch, door) &&
              !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_NOMOB) &&
              !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_DEATH) &&
              (!MOB_FLAGGED(ch, MOB_STAY_ZONE) ||
              (world[EXIT(ch, door)->to_room].zone == world[IN_ROOM(ch)].zone))) {
        /* If the mob is charmed, do not move the mob. */
        if (ch->master == NULL)
          perform_move(ch, door, 1);
      }

    /* Aggressive Mobs */
    if (!MOB_FLAGGED(ch, MOB_HELPER) && (!AFF_FLAGGED(ch, AFF_BLIND) || 
            !AFF_FLAGGED(ch, AFF_CHARM))) {
      found = FALSE;
      for (vict = world[IN_ROOM(ch)].people; vict && !found; 
              vict = vict->next_in_room) {
        
        if (IS_NPC(vict) && !IS_PET(vict))
          continue;
        
        if (IS_PET(vict) && IS_NPC(vict->master))
          continue;

        if (!CAN_SEE(ch, vict) || (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_NOHASSLE)))
          continue;

        if (MOB_FLAGGED(ch, MOB_WIMPY) && AWAKE(vict))
          continue;

        if (MOB_FLAGGED(ch, MOB_AGGRESSIVE) ||
                (MOB_FLAGGED(ch, MOB_AGGR_EVIL) && IS_EVIL(vict)) ||
                (MOB_FLAGGED(ch, MOB_AGGR_NEUTRAL) && IS_NEUTRAL(vict)) ||
                (MOB_FLAGGED(ch, MOB_AGGR_GOOD) && IS_GOOD(vict))) {
          hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
          found = TRUE;
        }
      }
    }

    /* Mob Memory */
    if (MOB_FLAGGED(ch, MOB_MEMORY) && MEMORY(ch)) {
      found = FALSE;
      for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
        if (IS_NPC(vict) || !CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE))
          continue;

        for (names = MEMORY(ch); names && !found; names = names->next) {
          if (names->id != GET_IDNUM(vict))
            continue;

          found = TRUE;
          act("'!!!', exclaims $n.", FALSE, ch, 0, 0, TO_ROOM);
          hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
        }
      }
    }

    /* Charmed Mob Rebellion: In order to rebel, there need to be more charmed 
     * monsters than the person can feasibly control at a time.  Then the
     * mobiles have a chance based on the charisma of their leader.
     * 1-4 = 0, 5-7 = 1, 8-10 = 2, 11-13 = 3, 14-16 = 4, 17-19 = 5, etc. */
    if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master &&
            num_followers_charmed(ch->master) > MAX(1, GET_CHA_BONUS(ch->master))) {
      if (!aggressive_mob_on_a_leash(ch, ch->master, ch->master)) {
        if (CAN_SEE(ch, ch->master) && !PRF_FLAGGED(ch->master, PRF_NOHASSLE))
          hit(ch, ch->master, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
        stop_follower(ch);
      }
    }

    /* Helper Mobs */
    if (MOB_FLAGGED(ch, MOB_HELPER) && (!AFF_FLAGGED(ch, AFF_BLIND) ||
            !AFF_FLAGGED(ch, AFF_CHARM))) {
      found = FALSE;
      for (vict = world[IN_ROOM(ch)].people; vict && !found;
              vict = vict->next_in_room) {
        if (ch == vict || !IS_NPC(vict) || !FIGHTING(vict))
          continue;
        if (GROUP(vict) && GROUP(vict) == GROUP(ch))
          continue;
        if (IS_NPC(FIGHTING(vict)) || ch == FIGHTING(vict))
          continue;

        act("$n jumps to the aid of $N!", FALSE, ch, 0, vict, TO_ROOM);
        hit(ch, FIGHTING(vict), TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
        found = TRUE;
      }
      if (found)
        continue;
    }

    /* helping group members use to be here, now its in 
     * perform_violence() in fight.c */

    /* a function to move mobile back to its loadroom (if sentinel) */
    if (!HUNTING(ch) && !MEMORY(ch) && !ch->master &&
            MOB_FLAGGED(ch, MOB_SENTINEL) && !IS_PET(ch) &&
            GET_MOB_LOADROOM(ch) != ch->in_room)
      hunt_loadroom(ch);

    /* pets return to their master */
    if (GET_POS(ch) == POS_STANDING && IS_PET(ch) && ch->master->in_room !=
            ch->in_room && !HUNTING(ch)) {
      HUNTING(ch) = ch->master;
      hunt_victim(ch);
    }
    /* Add new mobile actions here */

  } /* end for() */
}
