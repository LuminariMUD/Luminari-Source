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
#define SINFO spell_info[spellnum]
#define SPELLUP_SPELLS 54
#define OFFENSIVE_SPELLS 58
#define OFFENSIVE_AOE_SPELLS 16

/* list of spells mobiles will use for spellups */
int valid_spellup_spell[SPELLUP_SPELLS] = {
  SPELL_ARMOR, //0
  SPELL_BLESS,
  SPELL_DETECT_ALIGN,
  SPELL_DETECT_INVIS,
  SPELL_DETECT_MAGIC,
  SPELL_DETECT_POISON, //5
  SPELL_INVISIBLE,
  SPELL_PROT_FROM_EVIL,
  SPELL_SANCTUARY,
  SPELL_STRENGTH,
  SPELL_SENSE_LIFE, //10
  SPELL_INFRAVISION,
  SPELL_WATERWALK,
  SPELL_FLY,
  SPELL_BLUR,
  SPELL_MIRROR_IMAGE, //15
  SPELL_STONESKIN,
  SPELL_ENDURANCE,
  SPELL_PROT_FROM_GOOD,
  SPELL_ENDURE_ELEMENTS,
  SPELL_EXPEDITIOUS_RETREAT, //20
  SPELL_IRON_GUTS,
  SPELL_MAGE_ARMOR,
  SPELL_SHIELD,
  SPELL_TRUE_STRIKE,
  SPELL_FALSE_LIFE, //25
  SPELL_GRACE,
  SPELL_RESIST_ENERGY,
  SPELL_WATER_BREATHE,
  SPELL_HEROISM,
  SPELL_NON_DETECTION, //30
  SPELL_HASTE,
  SPELL_CUNNING,
  SPELL_WISDOM,
  SPELL_CHARISMA,
  SPELL_FIRE_SHIELD, //35
  SPELL_COLD_SHIELD,
  SPELL_GREATER_INVIS,
  SPELL_MINOR_GLOBE,
  SPELL_GREATER_HEROISM,
  SPELL_TRUE_SEEING, //40
  SPELL_GLOBE_OF_INVULN,
  SPELL_GREATER_MIRROR_IMAGE,
  SPELL_DISPLACEMENT,
  SPELL_PROTECT_FROM_SPELLS,
  SPELL_SPELL_MANTLE, //45
  SPELL_IRONSKIN,
  SPELL_MIND_BLANK,
  SPELL_SHADOW_SHIELD,
  SPELL_GREATER_SPELL_MANTLE,
  SPELL_REGENERATION, //50
  SPELL_DEATH_SHIELD,
  SPELL_BARKSKIN,
  SPELL_SPELL_RESISTANCE
};

/* list of spells mobiles will use for offense (aoe) */
int valid_aoe_spell[OFFENSIVE_AOE_SPELLS] = {
  /* aoe */
  SPELL_EARTHQUAKE, //0
  SPELL_ICE_STORM,
  SPELL_METEOR_SWARM,
  SPELL_CHAIN_LIGHTNING,
  SPELL_SYMBOL_OF_PAIN,
  SPELL_MASS_HOLD_PERSON, //5
  SPELL_PRISMATIC_SPRAY,
  SPELL_THUNDERCLAP,
  SPELL_INCENDIARY_CLOUD,
  SPELL_HORRID_WILTING,
  SPELL_WAIL_OF_THE_BANSHEE, //10
  SPELL_STORM_OF_VENGEANCE,
  SPELL_CALL_LIGHTNING_STORM,
  SPELL_CREEPING_DOOM,
  SPELL_FIRE_STORM,
  SPELL_SUNBEAM //15
};

/* list of spells mobiles will use for offense */
int valid_offensive_spell[OFFENSIVE_SPELLS] = {
  /* single target */
  SPELL_BLINDNESS, //0
  SPELL_BURNING_HANDS,
  SPELL_CALL_LIGHTNING,
  SPELL_CHILL_TOUCH,
  SPELL_COLOR_SPRAY,
  SPELL_CURSE, //5
  SPELL_ENERGY_DRAIN,
  SPELL_FIREBALL,
  SPELL_HARM,
  SPELL_LIGHTNING_BOLT,
  SPELL_MAGIC_MISSILE, //10
  SPELL_POISON,
  SPELL_SHOCKING_GRASP,
  SPELL_CAUSE_LIGHT_WOUNDS,
  SPELL_CAUSE_MODERATE_WOUNDS,
  SPELL_CAUSE_SERIOUS_WOUNDS, //15
  SPELL_CAUSE_CRITICAL_WOUNDS,
  SPELL_FLAME_STRIKE,
  SPELL_DESTRUCTION,
  SPELL_BALL_OF_LIGHTNING,
  SPELL_MISSILE_STORM, //20
  SPELL_HORIZIKAULS_BOOM,
  SPELL_ICE_DAGGER,
  SPELL_NEGATIVE_ENERGY_RAY,
  SPELL_RAY_OF_ENFEEBLEMENT,
  SPELL_SCARE, //25
  SPELL_ACID_ARROW,
  SPELL_DAZE_MONSTER,
  SPELL_HIDEOUS_LAUGHTER,
  SPELL_TOUCH_OF_IDIOCY,
  SPELL_SCORCHING_RAY, //30
  SPELL_DEAFNESS,
  SPELL_ENERGY_SPHERE,
  SPELL_VAMPIRIC_TOUCH,
  SPELL_HOLD_PERSON,
  SPELL_SLOW, //35
  SPELL_FEEBLEMIND,
  SPELL_NIGHTMARE,
  SPELL_MIND_FOG,
  SPELL_CONE_OF_COLD,
  SPELL_TELEKINESIS, //40
  SPELL_FIREBRAND,
  SPELL_FREEZING_SPHERE,
  SPELL_EYEBITE,
  SPELL_GRASPING_HAND,
  SPELL_POWER_WORD_BLIND, //45
  SPELL_POWER_WORD_STUN,
  SPELL_CLENCHED_FIST,
  SPELL_IRRESISTIBLE_DANCE,
  SPELL_SCINT_PATTERN,
  SPELL_SUNBURST, //50
  SPELL_WEIRD,
  SPELL_IMPLODE,
  SPELL_FAERIE_FIRE,
  SPELL_FLAMING_SPHERE,
  SPELL_BLIGHT, //55
  SPELL_FINGER_OF_DEATH,
  SPELL_WHIRLWIND
};

/* end local */


/*** UTILITY FUNCTIONS ***/
/* this function will attempt to rescue
   1)  master
   2)  group member 
 */
/* how many times will you loop group list to find rescue target cap */
#define RESCUE_LOOP  20
void npc_rescue(struct char_data *ch) {
  struct char_data *victim = NULL;
  int loop_counter = 0;

  // going to prioritize rescuing master (if it has one)
  if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && !rand_number(0, 1) &&
          (GET_MAX_HIT(ch) / GET_HIT(ch)) <= 2) {
    if (FIGHTING(ch->master)) {
      do_npc_rescue(ch, ch->master);
      SET_WAIT(ch, PULSE_VIOLENCE * 2);
      return;
    }
  }

  /* determine victim (someone in group, including self) */
  if (GROUP(ch) && GROUP(ch)->members->iSize && !rand_number(0, 1) &&
          (GET_MAX_HIT(ch) / GET_HIT(ch)) <= 2) {
    do {
      victim = (struct char_data *) random_from_list(GROUP(ch)->members);
      loop_counter++;
      if (loop_counter >= RESCUE_LOOP)
        break;
    } while (!victim || victim == ch);

    if (loop_counter < RESCUE_LOOP && FIGHTING(victim)) {
      do_npc_rescue(ch, victim);
      SET_WAIT(ch, PULSE_VIOLENCE * 2);
      return;
    }
  }
}

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

/* end memory routines */

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
int can_continue(struct char_data *ch, bool fighting) {
  //dummy checks
  if (fighting && (!FIGHTING(ch) || IN_ROOM(ch) != IN_ROOM(FIGHTING(ch)))) {
    stop_fighting(ch);
    return 0;
  }
  if (GET_MOB_WAIT(ch) > 0)
    return 0;
  if (HAS_WAIT(ch))
    return 0;
  if (GET_POS(ch) <= POS_SITTING)
    return 0;
  if (IS_CASTING(ch))
    return 0;
  if (GET_HIT(ch) <= 0)
    return 0;

  return 1;
}

/*** END UTILITY FUNCTIONS ***/

/*** RACE TYPES ***/

/* racial behaviour function */
void npc_racial_behave(struct char_data *ch) {
  struct char_data *AoE = NULL, *vict = NULL;
  int engaged = 0;

  if (!can_continue(ch, TRUE))
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

/*** MELEE CLASSES ***/

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
// bard behaviour, behave based on level
void npc_bard_behave(struct char_data *ch, struct char_data *vict,
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

  /* first rescue friends/master */
  npc_rescue(ch);

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

  /* first rescue friends/master */
  npc_rescue(ch);

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

  /* first rescue friends/master */
  npc_rescue(ch);

  switch (rand_number(5, level)) {
    case 5: // level 1-4 mobs won't act
      break;
    default:
      break;
  }
}
// berserk behaviour, behave based on level
void npc_berserker_behave(struct char_data *ch, struct char_data *vict,
        int level, int engaged) {

  /* first rescue friends/master */
  npc_rescue(ch);

  switch (rand_number(5, level)) {
    case 5: // level 1-4 mobs won't act
      break;
    default:
      break;
  }
}

/* this is our non-caster's entry point in combat AI
 all semi-casters such as ranger/paladin will go through here */
void npc_class_behave(struct char_data *ch) {
  struct char_data *AoE = NULL, *vict = NULL;
  int engaged = 0;

  if (MOB_FLAGGED(ch, MOB_NOCLASS))
    return;
  if (!can_continue(ch, TRUE))
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
    case CLASS_BARD:
      npc_bard_behave(ch, vict, GET_LEVEL(ch), engaged);
      break;
    case CLASS_BERSERKER:
      npc_berserker_behave(ch, vict, GET_LEVEL(ch), engaged);
      break;
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
    case CLASS_MONK:
      npc_monk_behave(ch, vict, GET_LEVEL(ch), engaged);
      break;
    default:
      break;
  }
}

/* this defines maximum amount of times the function will check the
 spellup array for a valid spell 
 note:  npc_offensive_spells() uses this define as well */
#define MAX_LOOPS 10

/* generic function for spelling up as a caster */
void npc_spellup(struct char_data *ch) {
  struct obj_data *obj = NULL;
  struct char_data *victim = ch;
  int level, spellnum = -1, loop_counter = 0;
  /* our priorities are going to be in this order:
   1)  get a charmee
   2)  heal (heal group?), condition issues
   3)  spellup (spellup group?)
   */
  if (!ch)
    return;
  if (MOB_FLAGGED(ch, MOB_NOCLASS))
    return;
  if (!can_continue(ch, FALSE))
    return;

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    level = LVL_IMMORT - 1;
  else
    level = GET_LEVEL(ch);

  /* try animate undead first */
  if (!HAS_PET_UNDEAD(ch) && !rand_number(0, 1)) {
    for (obj = world[ch->in_room].contents; obj; obj = obj->next_content) {
      if (!IS_CORPSE(obj))
        continue;
      if (level >= spell_info[SPELL_GREATER_ANIMATION].min_level[GET_CLASS(ch)]) {
        if (!GROUP(ch))
          create_group(ch);
        cast_spell(ch, NULL, obj, SPELL_GREATER_ANIMATION);
        return;
      }
    }
  }

  /* try for an elemental */
  if (!HAS_PET_ELEMENTAL(ch) && !rand_number(0, 6)) {
    if (level >= spell_info[SPELL_SUMMON_CREATURE_9].min_level[GET_CLASS(ch)]) {
      if (!GROUP(ch))
        create_group(ch);
      cast_spell(ch, NULL, NULL, SPELL_SUMMON_CREATURE_9);
      return;
    }
    else if (level >= spell_info[SPELL_SUMMON_CREATURE_8].min_level[GET_CLASS(ch)]) {
      if (!GROUP(ch))
        create_group(ch);
      cast_spell(ch, NULL, NULL, SPELL_SUMMON_CREATURE_8);
      return;
    }
    else if (level >= spell_info[SPELL_SUMMON_CREATURE_7].min_level[GET_CLASS(ch)]) {
      if (!GROUP(ch))
        create_group(ch);
      cast_spell(ch, NULL, NULL, SPELL_SUMMON_CREATURE_7);
      return;
    }
  }

  /* determine victim (someone in group, including self) */
  if (GROUP(ch) && GROUP(ch)->members->iSize) {
    victim = (struct char_data *) random_from_list(GROUP(ch)->members);
    if (!victim)
      victim = ch;
  }

  /* try healing */
  if ((GET_MAX_HIT(victim) / GET_HIT(victim)) >= 2) {
    if (level >= spell_info[SPELL_HEAL].min_level[GET_CLASS(ch)]) {
      cast_spell(ch, victim, NULL, SPELL_HEAL);
      return;
    }
    else if (level >= spell_info[SPELL_CURE_CRITIC].min_level[GET_CLASS(ch)]) {
      cast_spell(ch, victim, NULL, SPELL_CURE_CRITIC);
      return;
    }
  }

  /* try to fix condition issues (blindness, etc) */
  /* TODO */

  /* random buffs */
  do {
    spellnum = valid_spellup_spell[rand_number(0, SPELLUP_SPELLS - 1)];
    loop_counter++;
    if (loop_counter >= (MAX_LOOPS))
      break;
  } while (level < spell_info[spellnum].min_level[GET_CLASS(ch)] ||
          affected_by_spell(victim, spellnum));

  if (loop_counter < (MAX_LOOPS))
    // found a spell, cast it
    cast_spell(ch, victim, NULL, spellnum);

  return;
}

/* note MAX_LOOPS used here too */

/* generic function for spelling up as a caster */
void npc_offensive_spells(struct char_data *ch) {
  struct char_data *tch = NULL;
  int level, spellnum = -1, loop_counter = 0;
  struct list_data *room_list = NULL;
  bool use_aoe = FALSE;

  if (!ch)
    return;
  if (MOB_FLAGGED(ch, MOB_NOCLASS))
    return;

  /* 25% of spellup instead of offensive spell */
  if (!rand_number(0, 3)) {
    npc_spellup(ch);
    return;
  }

  /* our semi-casters will rarely use this function */
  switch (GET_CLASS(ch)) {
    case CLASS_RANGER:
    case CLASS_PALADIN: // 10 out of 11 times will not cast
      if (rand_number(0, 10)) {
        npc_class_behave(ch);
        return;
      }
      break;
    case CLASS_BARD: // bards 33% will not cast
      if (!rand_number(0, 2)) {
        npc_class_behave(ch);
        return;
      }
      break;
  }

  if (!IN_ROOM(ch)) // dummy check since room info used in building list
    return;

  if (FIGHTING(ch))
    tch = FIGHTING(ch);

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    level = LVL_IMMORT - 1;
  else
    level = GET_LEVEL(ch);

  /* determine victim (fighting multiple opponents?) */
  room_list = create_list(); //allocate memory for list

  /* build list of opponents */
  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
    if (tch && FIGHTING(tch) && FIGHTING(tch) == ch)
      add_to_list(tch, room_list);
  }

  /* if list empty, free it */
  if (room_list->iSize == 0)
    free_list(room_list);
  else {
    /* ok we have a list, lets pick a random out of it */
    tch = random_from_list(room_list);
  }

  /* just a dummy check, after this tch should have a valid target */
  if (!tch && FIGHTING(ch))
    tch = FIGHTING(ch);

  /* should we use Aoe?  if 2 or more opponents, lets do it */
  if (room_list->iSize >= 2)
    use_aoe = TRUE;

  /* random offensive spell */
  if (use_aoe) {
    do {
      spellnum = valid_aoe_spell[rand_number(0, OFFENSIVE_AOE_SPELLS - 1)];
      loop_counter++;
      if (loop_counter >= (MAX_LOOPS / 2))
        break;
    } while (level < spell_info[spellnum].min_level[GET_CLASS(ch)]);

    if (loop_counter < (MAX_LOOPS / 2)) {
      // found a spell, cast it
      cast_spell(ch, tch, NULL, spellnum);
      return;
    }
  }

  /* we intentionally fall through here,
   a lot of mobiles will not have aoe spells */
  loop_counter = 0;

  do {
    spellnum = valid_offensive_spell[rand_number(0, OFFENSIVE_SPELLS - 1)];
    loop_counter++;
    if (loop_counter >= (MAX_LOOPS / 2))
      break;
  } while (level < spell_info[spellnum].min_level[GET_CLASS(ch)] ||
          affected_by_spell(tch, spellnum));

  if (loop_counter < (MAX_LOOPS / 2))
    // found a spell, cast it
    cast_spell(ch, tch, NULL, spellnum);

  return;
}

/*** MOBILE ACTIVITY ***/

/* the primary engine for mobile activity */
void mobile_activity(void) {
  struct char_data *ch = NULL, *next_ch = NULL, *vict = NULL, *tmp_char = NULL;
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
      else if (IS_NPC_CASTER(ch))
        npc_offensive_spells(ch);
      else
        npc_class_behave(ch);
      continue;
    } else if (!rand_number(0, 6) && IS_NPC_CASTER(ch)) {
      /* not in combat */
      npc_spellup(ch);
    }

    /* return mobile to preferred (default) position if necessary */
    if (GET_POS(ch) != GET_DEFAULT_POS(ch)) {
      if (GET_DEFAULT_POS(ch) == POS_SITTING) {
        do_sit(ch, NULL, 0, 0);
      } else if (GET_DEFAULT_POS(ch) == POS_RESTING) {
        do_rest(ch, NULL, 0, 0);
      } else if (GET_DEFAULT_POS(ch) == POS_STANDING) {
        do_stand(ch, NULL, 0, 0);
      } else if (GET_DEFAULT_POS(ch) == POS_SLEEPING) {
        bool go_to_sleep = FALSE;
        do_rest(ch, NULL, 0, 0);

        // only go back to sleep if no PCs in the room, and percentage
        if (rand_number(1, 100) <= 10) {
          go_to_sleep = TRUE;
          for (tmp_char = world[ch->in_room].people; tmp_char; tmp_char = tmp_char->next_in_room)
            if (!IS_NPC(tmp_char) && CAN_SEE(ch, tmp_char)) {
              // don't go to sleep
              go_to_sleep = FALSE;
              break;
            }
        }

        if (go_to_sleep)
          do_sleep(ch, NULL, 0, 0);
      }
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

/* must be at end of file */
#undef SINFO
#undef MOB_SPELLS
#undef RESCUE_LOOP

/**************************/

