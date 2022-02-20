/**************************************************************************
 *  File: mobact.c                                     Part of LuminariMUD *
 *  Usage: Functions for generating intelligent (?) behavior in mobiles.   *
 *  Rewritten by Zusuk                                                     *
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
#include "modify.h"
#include "mobact.h"
#include "shop.h"
#include "quest.h"      /* so you can identify questmaster mobiles */
#include "dg_scripts.h" /* so you can identify script mobiles */

/***********/

void npc_offensive_spells(struct char_data *ch);
void npc_racial_behave(struct char_data *ch);

/* local file scope only function prototypes, defines, externs, etc */
#define SINFO spell_info[spellnum]
#define SPELLUP_SPELLS 55
#define OFFENSIVE_SPELLS 58
#define OFFENSIVE_AOE_SPELLS 16

/* list of spells mobiles will use for spellups */
int valid_spellup_spell[SPELLUP_SPELLS] = {
    SPELL_ARMOR, // 0
    SPELL_BLESS,
    SPELL_DETECT_ALIGN,
    SPELL_DETECT_INVIS,
    SPELL_DETECT_MAGIC,
    SPELL_DETECT_POISON, // 5
    SPELL_INVISIBLE,
    SPELL_PROT_FROM_EVIL,
    SPELL_SANCTUARY,
    SPELL_STRENGTH,
    SPELL_SENSE_LIFE, // 10
    SPELL_INFRAVISION,
    SPELL_WATERWALK,
    SPELL_FLY,
    SPELL_BLUR,
    SPELL_MIRROR_IMAGE, // 15
    SPELL_STONESKIN,
    SPELL_ENDURANCE,
    SPELL_PROT_FROM_GOOD,
    SPELL_ENDURE_ELEMENTS,
    SPELL_EXPEDITIOUS_RETREAT, // 20
    SPELL_IRON_GUTS,
    SPELL_MAGE_ARMOR,
    SPELL_SHIELD,
    SPELL_TRUE_STRIKE,
    SPELL_FALSE_LIFE, // 25
    SPELL_GRACE,
    SPELL_RESIST_ENERGY,
    SPELL_WATER_BREATHE,
    SPELL_HEROISM,
    SPELL_NON_DETECTION, // 30
    SPELL_HASTE,
    SPELL_CUNNING,
    SPELL_WISDOM,
    SPELL_CHARISMA,
    SPELL_FIRE_SHIELD, // 35
    SPELL_COLD_SHIELD,
    SPELL_GREATER_INVIS,
    SPELL_MINOR_GLOBE,
    SPELL_GREATER_HEROISM,
    SPELL_TRUE_SEEING, // 40
    SPELL_GLOBE_OF_INVULN,
    SPELL_GREATER_MIRROR_IMAGE,
    SPELL_DISPLACEMENT,
    SPELL_PROTECT_FROM_SPELLS,
    SPELL_SPELL_MANTLE, // 45
    SPELL_IRONSKIN,
    SPELL_MIND_BLANK,
    SPELL_SHADOW_SHIELD,
    SPELL_GREATER_SPELL_MANTLE,
    SPELL_REGENERATION, // 50
    SPELL_DEATH_SHIELD,
    SPELL_BARKSKIN,
    SPELL_SPELL_RESISTANCE,
    SPELL_WATERWALK};

/* list of spells mobiles will use for offense (aoe) */
int valid_aoe_spell[OFFENSIVE_AOE_SPELLS] = {
    /* aoe */
    SPELL_EARTHQUAKE, // 0
    SPELL_ICE_STORM,
    SPELL_METEOR_SWARM,
    SPELL_CHAIN_LIGHTNING,
    SPELL_SYMBOL_OF_PAIN,
    SPELL_MASS_HOLD_PERSON, // 5
    SPELL_PRISMATIC_SPRAY,
    SPELL_THUNDERCLAP,
    SPELL_INCENDIARY_CLOUD,
    SPELL_HORRID_WILTING,
    SPELL_WAIL_OF_THE_BANSHEE, // 10
    SPELL_STORM_OF_VENGEANCE,
    SPELL_CALL_LIGHTNING_STORM,
    SPELL_CREEPING_DOOM,
    SPELL_FIRE_STORM,
    SPELL_SUNBEAM // 15
};

/* list of spells mobiles will use for offense */
int valid_offensive_spell[OFFENSIVE_SPELLS] = {
    /* single target */
    SPELL_BLINDNESS, // 0
    SPELL_BURNING_HANDS,
    SPELL_CALL_LIGHTNING,
    SPELL_CHILL_TOUCH,
    SPELL_COLOR_SPRAY,
    SPELL_CURSE, // 5
    SPELL_ENERGY_DRAIN,
    SPELL_FIREBALL,
    SPELL_HARM,
    SPELL_LIGHTNING_BOLT,
    SPELL_MAGIC_MISSILE, // 10
    SPELL_POISON,
    SPELL_SHOCKING_GRASP,
    SPELL_CAUSE_LIGHT_WOUNDS,
    SPELL_CAUSE_MODERATE_WOUNDS,
    SPELL_CAUSE_SERIOUS_WOUNDS, // 15
    SPELL_CAUSE_CRITICAL_WOUNDS,
    SPELL_FLAME_STRIKE,
    SPELL_DESTRUCTION,
    SPELL_BALL_OF_LIGHTNING,
    SPELL_MISSILE_STORM, // 20
    SPELL_HORIZIKAULS_BOOM,
    SPELL_ICE_DAGGER,
    SPELL_NEGATIVE_ENERGY_RAY,
    SPELL_RAY_OF_ENFEEBLEMENT,
    SPELL_SCARE, // 25
    SPELL_ACID_ARROW,
    SPELL_DAZE_MONSTER,
    SPELL_HIDEOUS_LAUGHTER,
    SPELL_TOUCH_OF_IDIOCY,
    SPELL_SCORCHING_RAY, // 30
    SPELL_DEAFNESS,
    SPELL_ENERGY_SPHERE,
    SPELL_VAMPIRIC_TOUCH,
    SPELL_HOLD_PERSON,
    SPELL_SLOW, // 35
    SPELL_FEEBLEMIND,
    SPELL_NIGHTMARE,
    SPELL_MIND_FOG,
    SPELL_CONE_OF_COLD,
    SPELL_TELEKINESIS, // 40
    SPELL_FIREBRAND,
    SPELL_FREEZING_SPHERE,
    SPELL_EYEBITE,
    SPELL_GRASPING_HAND,
    SPELL_POWER_WORD_BLIND, // 45
    SPELL_POWER_WORD_STUN,
    SPELL_CLENCHED_FIST,
    SPELL_IRRESISTIBLE_DANCE,
    SPELL_SCINT_PATTERN,
    SPELL_SUNBURST, // 50
    SPELL_WEIRD,
    SPELL_IMPLODE,
    SPELL_FAERIE_FIRE,
    SPELL_FLAMING_SPHERE,
    SPELL_BLIGHT, // 55
    SPELL_FINGER_OF_DEATH,
    SPELL_WHIRLWIND};

/* end local */

/*** UTILITY FUNCTIONS ***/

/* function to return possible targets for mobile, used for
 * npc that is fighting  */
struct char_data *npc_find_target(struct char_data *ch, int *num_targets)
{
  struct list_data *target_list = NULL;
  struct char_data *tch = NULL;

  if (!ch || IN_ROOM(ch) == NOWHERE || !FIGHTING(ch))
    return NULL;

  /* dynamic memory allocation required */
  target_list = create_list();

  /* loop through chars in room to find possible targets to build list */
  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (tch == ch)
      continue;

    if (!CAN_SEE(ch, tch))
      continue;

    if (!IS_NPC(tch) && PRF_FLAGGED(tch, PRF_NOHASSLE))
      continue;

    /* in mobile memory? */
    if (is_in_memory(ch, tch))
      add_to_list(tch, target_list);

    /* hunting target? */
    if (HUNTING(ch) == tch)
      add_to_list(tch, target_list);

    /* fighting me? */
    if (FIGHTING(tch) == ch)
      add_to_list(tch, target_list);

    /* me fighting? */
    if (FIGHTING(ch) == tch)
      add_to_list(tch, target_list);
  }

  /* did we snag anything? */
  if (target_list->iSize == 0)
  {
    free_list(target_list);
    return NULL;
  }

  /* ok should be golden, go ahead snag a random and free list */
  /* always can just return fighting target */
  tch = random_from_list(target_list);
  *num_targets = target_list->iSize; // yay pointers!

  if (target_list)
    free_list(target_list);

  if (tch) // dummy check
    return tch;
  else // backup plan
    return NULL;
}

/* a very simplified switch opponents engine */
bool npc_switch_opponents(struct char_data *ch, struct char_data *vict)
{
  // mudlog(NRM, LVL_IMMORT, TRUE, "%s trying to switch opponents!", ch->player.short_descr);
  if (!ch || !vict)
    return FALSE;

  if (!CAN_SEE(ch, vict))
    return FALSE;

  if (GET_POS(ch) <= POS_SITTING)
  {
    send_to_char(ch, "You are in no position to switch opponents!\r\n");
    return FALSE;
  }

  if (FIGHTING(ch) == vict)
  {
    send_to_char(ch, "You can't switch opponents to an opponent you are "
                     "already fighting!\r\n");
    return FALSE;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE))
  {
    if (ch->next_in_room != vict && vict->next_in_room != ch)
    {
      send_to_char(ch, "You can't reach to switch opponents!\r\n");
      return FALSE;
    }
  }

  /* should be a valid opponent */
  if (FIGHTING(ch))
    stop_fighting(ch);
  send_to_char(ch, "You switch opponents!\r\n");
  act("$n switches opponents to $N!", FALSE, ch, 0, vict, TO_ROOM);
  hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);

  return TRUE;
}

/* this function will attempt to rescue
   1)  master
   2)  group member
 * returns TRUE if rescue attempt is made
 */
/* how many times will you loop group list to find rescue target cap */
#define RESCUE_LOOP 20
bool npc_rescue(struct char_data *ch)
{

  if (ch->master && PRF_FLAGGED(ch->master, PRF_NO_CHARMIE_RESCUE))
    return false;

  struct char_data *victim = NULL;
  int loop_counter = 0;

  // going to prioritize rescuing master (if it has one)
  if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && !rand_number(0, 1) &&
      (GET_MAX_HIT(ch) / GET_HIT(ch)) <= 2)
  {
    if (FIGHTING(ch->master) && ((GET_MAX_HIT(ch->master) / GET_HIT(ch->master)) <= 3))
    {
      perform_rescue(ch, ch->master);
      return TRUE;
    }
  }

  /* determine victim (someone in group, including self) */
  if (GROUP(ch) && GROUP(ch)->members->iSize && !rand_number(0, 1) &&
      (GET_MAX_HIT(ch) / GET_HIT(ch)) <= 2)
  {
    do
    {
      victim = (struct char_data *)random_from_list(GROUP(ch)->members);
      loop_counter++;
      if (loop_counter >= RESCUE_LOOP)
        break;
    } while (!victim || victim == ch || !FIGHTING(victim) ||
             ((GET_MAX_HIT(victim) / GET_HIT(victim)) > 3));

    if (loop_counter < RESCUE_LOOP && FIGHTING(victim))
    {
      perform_rescue(ch, victim);
      return TRUE;
    }
  }

  return FALSE;
}
#undef RESCUE_LOOP

/* function to move a mobile along a specified path (patrols) */
bool move_on_path(struct char_data *ch)
{
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
  if (PATH_DELAY(ch) > 0)
  {
    PATH_DELAY(ch)
    --;
    return FALSE;
  }

  send_to_char(ch, "OK, I am in room %d (%d)...  ", GET_ROOM_VNUM(IN_ROOM(ch)),
               IN_ROOM(ch));

  PATH_DELAY(ch) = PATH_RESET(ch);

  if (PATH_INDEX(ch) >= PATH_SIZE(ch) || PATH_INDEX(ch) < 0)
    PATH_INDEX(ch) = 0;

  next = GET_PATH(ch, PATH_INDEX(ch));

  send_to_char(ch, "PATH:  Path-Index:  %d, Next (get-path vnum):  %d (%d).\r\n",
               PATH_INDEX(ch), next, real_room(next));

  dir = find_first_step(IN_ROOM(ch), real_room(next));

  if (EXIT(ch, dir)->to_room != real_room(next))
  {
    send_to_char(ch, "Hrm, it appears I am off-path...\r\n");
    send_to_char(ch, "I want to go %s, which is room %d, but I need to get to"
                     " room %d..\r\n",
                 dirs[dir], GET_ROOM_VNUM(EXIT(ch, dir)->to_room),
                 next);
  }
  else
  {
    send_to_char(ch, "... it looks like my path is perfect so far!\r\n");
  }

  switch (dir)
  {
  case BFS_ERROR:
    send_to_char(ch, "Hmm.. something seems to be seriously wrong.\r\n");
    log("PATH ERROR: Mob %s, in room %d, trying to get to %d", GET_NAME(ch), world[IN_ROOM(ch)].number, next);
    break;
  case BFS_ALREADY_THERE:
    send_to_char(ch, "I seem to be in the right room already!\r\n");
    break;
  case BFS_NO_PATH:
    send_to_char(ch, "I can't sense a trail to %d (%d) from here.\r\n",
                 next, real_room(next));
    // log("NO PATH: Mob %s, in room %d, trying to get to %d", GET_NAME(ch), world[IN_ROOM(ch)].number, next);
    break;
  default: /* Success! */
    send_to_char(ch, "I sense a trail %s from here!\r\n", dirs[dir]);
    perform_move(ch, dir, 1);
    break;
  }

  PATH_INDEX(ch)
  ++;

  return TRUE;
}

/* mobile echo system, from homeland ported by nashak */
void mobile_echos(struct char_data *ch)
{
  char *echo = NULL;
  struct descriptor_data *d = NULL;

  if (!ECHO_COUNT(ch))
    return;
  if (!ECHO_ENTRIES(ch))
    return;

  if (rand_number(1, 75) > (ECHO_FREQ(ch) / 4))
    return;

  if (CURRENT_ECHO(ch) > ECHO_COUNT(ch)) /* dummy check */
    CURRENT_ECHO(ch) = 0;

  if (ECHO_SEQUENTIAL(ch))
  {
    echo = ECHO_ENTRIES(ch)[CURRENT_ECHO(ch)++];
    if (CURRENT_ECHO(ch) >= ECHO_COUNT(ch))
      CURRENT_ECHO(ch) = 0;
  }
  else
  {
    echo = ECHO_ENTRIES(ch)[rand_number(0, ECHO_COUNT(ch) - 1)];
  }

  if (!echo)
    return;

  parse_at(echo);

  if (ECHO_IS_ZONE(ch))
  {
    for (d = descriptor_list; d; d = d->next)
    {
      if (!d->character)
        continue;
      if (d->character->in_room == NOWHERE || ch->in_room == NOWHERE)
        continue;
      if (world[d->character->in_room].zone != world[ch->in_room].zone)
        continue;
      if (!AWAKE(d->character))
        continue;

      if (!PLR_FLAGGED(d->character, PLR_WRITING))
        send_to_char(d->character, "%s\r\n", echo);
    }
  }
  else
  {
    act(echo, FALSE, ch, 0, 0, TO_ROOM);
  }
}

/* Mob Memory Routines */

/* checks if vict is in memory of ch */
bool is_in_memory(struct char_data *ch, struct char_data *vict)
{
  memory_rec *names = NULL;

  if (!IS_NPC(ch))
    return FALSE;

  if (!MOB_FLAGGED(ch, MOB_MEMORY))
    return FALSE;

  if (!MEMORY(ch))
    return FALSE;

  if (IS_NPC(vict) || !CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE))
    return FALSE;

  /* at this stage vict should be a valid target to check, now loop
   through ch's memory to cross-reference it */
  for (names = MEMORY(ch); names; names = names->next)
  {
    if (names->id != GET_IDNUM(vict))
      continue;
    else
      return TRUE; // bingo, found a match
  }

  return FALSE; // nothing
}

/* make ch remember victim */
void remember(struct char_data *ch, struct char_data *victim)
{
  memory_rec *tmp = NULL;
  bool present = FALSE;

  if (!IS_NPC(ch) || IS_NPC(victim) || PRF_FLAGGED(victim, PRF_NOHASSLE))
    return;

  if (MOB_FLAGGED(ch, MOB_NOKILL))
  {
    send_to_char(ch, "You are a protected mob, it doesn't make sense for you to remember your victim!\r\n");
    return;
  }

  if (victim && !ok_damage_shopkeeper(victim, ch))
  {
    send_to_char(ch, "You are a shopkeeper (that can't be damaged), it doesn't make sense for you to remember that target!\r\n");
    return;
  }

  /*
  if (victim && !is_mission_mob(victim, ch))
  {
    send_to_char(ch, "You are a mission mob, it doesn't make sense for you to remember that target!\r\n");
    return;
  }
  */

  for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
    if (tmp->id == GET_IDNUM(victim))
      present = TRUE;

  if (!present)
  {
    CREATE(tmp, memory_rec, 1);
    tmp->next = MEMORY(ch);
    tmp->id = GET_IDNUM(victim);
    MEMORY(ch) = tmp;
  }
}

/* make ch forget victim */
void forget(struct char_data *ch, struct char_data *victim)
{
  memory_rec *curr = NULL, *prev = NULL;

  if (!(curr = MEMORY(ch)))
    return;

  while (curr && curr->id != GET_IDNUM(victim))
  {
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

/* erase ch's memory completely, also freeing memory */
void clearMemory(struct char_data *ch)
{
  memory_rec *curr = NULL, *next = NULL;

  curr = MEMORY(ch);

  while (curr)
  {
    next = curr->next;
    free(curr);
    curr = next;
  }

  MEMORY(ch) = NULL;
}

/* end memory routines */

/* function encapsulating conditions that will stop the mobile from
 acting */
int can_continue(struct char_data *ch, bool fighting)
{
  // dummy checks
  if (fighting && (!FIGHTING(ch) || IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))))
  {
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
  /* almost finished victims, they will stop using these skills -zusuk */
  if (FIGHTING(ch) && GET_HIT(FIGHTING(ch)) <= 5)
    return 0;
  return 1;
}

void npc_ability_behave(struct char_data *ch)
{
  if (dice(1, 2) == 1)
    npc_racial_behave(ch);
  else
    npc_offensive_spells(ch);
  return;
  // we need to code the abilities before we go ahead with this
  struct char_data *vict = NULL;
  int num_targets = 0;

  if (!can_continue(ch, TRUE))
    return;

  /* retrieve random valid target and number of targets */
  if (!(vict = npc_find_target(ch, &num_targets)))
    return;
}

/*** END UTILITY FUNCTIONS ***/

/*** RACE TYPES ***/

/* racial behaviour function */
void npc_racial_behave(struct char_data *ch)
{
  struct char_data *vict = NULL;
  int num_targets = 0;

  if (!can_continue(ch, TRUE))
    return;

  /* retrieve random valid target and number of targets */
  if (!(vict = npc_find_target(ch, &num_targets)))
    return;

  // first figure out which race we are dealing with
  switch (GET_RACE(ch))
  {
  case RACE_TYPE_ANIMAL:
    switch (rand_number(1, 2))
    {
    case 1:
      do_rage(ch, 0, 0, 0);
    default:
      break;
    }
    break;
  case RACE_TYPE_DRAGON:

    switch (rand_number(1, 4))
    {
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
    switch (GET_LEVEL(ch))
    {
    default:
      break;
    }
    break;
  }
}

/*** MELEE CLASSES ***/

// monk behaviour, behave based on level

void npc_monk_behave(struct char_data *ch, struct char_data *vict,
                     int engaged)
{

  /* list of skills to use:
   1) switch opponents
   2) springleap
   3) stunning fist
   4) quivering palm
   */

  if (!can_continue(ch, TRUE))
    return;

  /* switch opponents attempt */
  if (!rand_number(0, 2) && npc_switch_opponents(ch, vict))
    return;

  switch (rand_number(1, 6))
  {
  case 1:
    perform_stunningfist(ch);
    break;
  case 2:
  case 3:
  case 4:
  case 5:
  case 6:
    perform_springleap(ch, vict);
    break;
  case 7: /* hahah just kidding */
    perform_quiveringpalm(ch);
    break;
  default:
    break;
  }
}
// rogue behaviour, behave based on level

void npc_rogue_behave(struct char_data *ch, struct char_data *vict,
                      int engaged)
{

  /* almost finished victims, they will stop using these skills -zusuk */
  if (GET_HIT(vict) <= 5)
    return;

  /* list of skills to use:
   1) trip
   2) dirt kick
   3) sap  //todo
   4) backstab / circle
   */
  if (GET_LEVEL(ch) >= 2 && !HAS_FEAT(ch, FEAT_SNEAK_ATTACK))
  {
    MOB_SET_FEAT(ch, FEAT_SNEAK_ATTACK, (GET_LEVEL(ch)) / 2);
  }

  if (!can_continue(ch, TRUE))
    return;

  switch (rand_number(1, 2))
  {
  case 1:
    if (perform_knockdown(ch, vict, SKILL_TRIP))
      break;
    /* fallthrough */
  case 2:
    if (perform_dirtkick(ch, vict))
    {
      send_to_char(ch, "Succeeded dirtkick\r\n");
      break;
    }
    else
      send_to_char(ch, "Failed dirtkick\r\n");
    /* fallthrough */
  default:
    if (perform_backstab(ch, vict))
      break;
    break;
  }
}
// bard behaviour, behave based on level

void npc_bard_behave(struct char_data *ch, struct char_data *vict,
                     int engaged)
{

  /* list of skills to use:
   1) trip
   2) dirt kick
   3) perform
   4) kick
   */
  /* try to throw up song */
  perform_perform(ch);

  if (!can_continue(ch, TRUE))
    return;

  switch (rand_number(1, 3))
  {
  case 1:
    perform_knockdown(ch, vict, SKILL_TRIP);
    break;
  case 2:
    perform_dirtkick(ch, vict);
    break;
  case 3:
    perform_kick(ch, vict);
    break;
  default:
    break;
  }
}
// warrior behaviour, behave based on circle

void npc_warrior_behave(struct char_data *ch, struct char_data *vict,
                        int engaged)
{

  /* list of skills to use:
   1) rescue
   2) bash
   3) shieldpunch
   4) switch opponents
   */

  /* first rescue friends/master */
  if (npc_rescue(ch))
    return;

  if (!can_continue(ch, TRUE))
    return;

  /* switch opponents attempt */
  if (!rand_number(0, 2) && npc_switch_opponents(ch, vict))
    return;

  switch (rand_number(1, 2))
  {
  case 1:
    if (perform_knockdown(ch, vict, SKILL_BASH))
      break;
  case 2:
    if (perform_shieldpunch(ch, vict))
      break;
  default:
    break;
  }
}
// ranger behaviour, behave based on level

void npc_ranger_behave(struct char_data *ch, struct char_data *vict,
                       int engaged)
{

  /* list of skills to use:
   1) rescue
   2) switch opponents
   3) call companion
   4) kick
   */

  /* attempt to call companion */
  perform_call(ch, MOB_C_ANIMAL, GET_LEVEL(ch));

  /* next rescue friends/master */
  if (npc_rescue(ch))
    return;

  if (!can_continue(ch, TRUE))
    return;

  /* switch opponents attempt */
  if (!rand_number(0, 2) && npc_switch_opponents(ch, vict))
    return;

  perform_kick(ch, vict);
}

// paladin behaviour, behave based on level

void npc_paladin_behave(struct char_data *ch, struct char_data *vict,
                        int engaged)
{
  float percent = ((float)GET_HIT(ch) / (float)GET_MAX_HIT(ch)) * 100.0;

  /* list of skills to use:
   1) rescue
   2) lay on hands
   3) smite evil
   4) switch opponents
   */

  /* first rescue friends/master */
  if (npc_rescue(ch))
    return;

  if (!can_continue(ch, TRUE))
    return;

  /* switch opponents attempt */
  if (!rand_number(0, 2) && npc_switch_opponents(ch, vict))
    return;

  if (IS_EVIL(vict))
    perform_smite(ch, SMITE_TYPE_EVIL);

  if (percent <= 25.0)
    perform_layonhands(ch, ch);
}

// berserk behaviour, behave based on level

void npc_berserker_behave(struct char_data *ch, struct char_data *vict,
                          int engaged)
{

  /* list of skills to use:
   1) rescue
   2) berserk
   3) headbutt
   4) switch opponents
   */

  /* first rescue friends/master */
  if (npc_rescue(ch))
    return;

  if (!can_continue(ch, TRUE))
    return;

  /* switch opponents attempt */
  if (!rand_number(0, 2) && npc_switch_opponents(ch, vict))
    return;

  perform_rage(ch);

  perform_headbutt(ch, vict);
}

/* this is our non-caster's entry point in combat AI
 all semi-casters such as ranger/paladin will go through here */
void npc_class_behave(struct char_data *ch)
{
  struct char_data *vict = NULL;
  int num_targets = 0;

  if (MOB_FLAGGED(ch, MOB_NOCLASS))
    return;

  /* retrieve random valid target and number of targets */
  if (!(vict = npc_find_target(ch, &num_targets)))
    return;

  switch (GET_CLASS(ch))
  {
  case CLASS_BARD:
    npc_bard_behave(ch, vict, num_targets);
    break;
  case CLASS_BERSERKER:
    npc_berserker_behave(ch, vict, num_targets);
    break;
  case CLASS_PALADIN:
    npc_paladin_behave(ch, vict, num_targets);
    break;
  case CLASS_RANGER:
    npc_ranger_behave(ch, vict, num_targets);
    break;
  case CLASS_ROGUE:
    npc_rogue_behave(ch, vict, num_targets);
    break;
  case CLASS_MONK:
    npc_monk_behave(ch, vict, num_targets);
    break;
  case CLASS_WARRIOR:
  case CLASS_WEAPON_MASTER: /*todo!*/
  default:
    npc_warrior_behave(ch, vict, num_targets);
    break;
  }
}

/* this defines maximum amount of times the function will check the
 spellup array for a valid spell
 note:  npc_offensive_spells() uses this define as well */
#define MAX_LOOPS 40

/* generic function for spelling up as a caster */
void npc_spellup(struct char_data *ch)
{
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

  /* we're checking spell min-levels so this is a necessity */
  if (GET_LEVEL(ch) >= LVL_IMMORT)
    level = LVL_IMMORT - 1;
  else
    level = GET_LEVEL(ch);

  /* try animate undead first */
  /* UPDATE: plans to add a mob flag for this, for now restrict to mobs
   over level 30 -zusuk */
  if (GET_LEVEL(ch) > 30 && !HAS_PET_UNDEAD(ch) && !rand_number(0, 1) && !ch->master)
  {
    for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
    {
      if (!IS_CORPSE(obj))
        continue;
      if (level >= spell_info[SPELL_GREATER_ANIMATION].min_level[GET_CLASS(ch)])
      {
        if (!GROUP(ch))
          create_group(ch);
        cast_spell(ch, NULL, obj, SPELL_GREATER_ANIMATION, 0);
        return;
      }
    }
  }

  /* try for an elemental */
  /* UPDATE: plans to add a mob flag for this, for now restrict to mobs
   over level 30 -zusuk */
  if (GET_LEVEL(ch) > 30 && !HAS_PET_ELEMENTAL(ch) && !rand_number(0, 6) && !ch->master)
  {
    if (level >= spell_info[SPELL_SUMMON_CREATURE_9].min_level[GET_CLASS(ch)])
    {
      if (!GROUP(ch))
        create_group(ch);
      cast_spell(ch, NULL, NULL, SPELL_SUMMON_CREATURE_9, 0);
      return;
    }
    else if (level >= spell_info[SPELL_SUMMON_CREATURE_8].min_level[GET_CLASS(ch)])
    {
      if (!GROUP(ch))
        create_group(ch);
      cast_spell(ch, NULL, NULL, SPELL_SUMMON_CREATURE_8, 0);
      return;
    }
    else if (level >= spell_info[SPELL_SUMMON_CREATURE_7].min_level[GET_CLASS(ch)])
    {
      if (!GROUP(ch))
        create_group(ch);
      cast_spell(ch, NULL, NULL, SPELL_SUMMON_CREATURE_7, 0);
      return;
    }
  }

  /* determine victim (someone in group, including self) */
  if (GROUP(ch) && GROUP(ch)->members->iSize)
  {
    victim = (struct char_data *)random_from_list(GROUP(ch)->members);
    if (!victim)
      victim = ch;
  }

  /* try healing */
  if (GET_HIT(victim) && (GET_MAX_HIT(victim) / GET_HIT(victim)) >= 2)
  {
    if (level >= spell_info[SPELL_HEAL].min_level[GET_CLASS(ch)])
    {
      cast_spell(ch, victim, NULL, SPELL_HEAL, 0);
      return;
    }
    else if (level >= spell_info[SPELL_CURE_CRITIC].min_level[GET_CLASS(ch)])
    {
      cast_spell(ch, victim, NULL, SPELL_CURE_CRITIC, 0);
      return;
    }
  }

  /* try to fix condition issues (blindness, etc) */
  /* TODO */

  /* random buffs */
  do
  {
    spellnum = valid_spellup_spell[rand_number(0, SPELLUP_SPELLS - 1)];
    loop_counter++;
    if (loop_counter >= (MAX_LOOPS))
      break;
  } while (level < spell_info[spellnum].min_level[GET_CLASS(ch)] ||
           affected_by_spell(victim, spellnum));

  /* we're putting some special restrictions here */
  if (IS_MOB(ch) && mob_index[GET_MOB_RNUM(ch)].func == shop_keeper &&
      (spellnum == SPELL_GREATER_INVIS ||
       spellnum == SPELL_INVISIBLE))
  {
    /* shopkeepers invising themselves is silly :p  -zusuk */
    return;
  }

  /* mobs with triggers / autoquest / quest will no longer invis */
  if (IS_MOB(ch) && ((SCRIPT(ch) && TRIGGERS(SCRIPT(ch))) || (ch->mob_specials.quest) || (GET_MOB_SPEC(ch) == questmaster)) &&
      (spellnum == SPELL_GREATER_INVIS ||
       spellnum == SPELL_INVISIBLE))
  {
    /* these type of mobs casting invis is problematic */
    return;
  }

  /* casters that serve as mounts no longer cast invis */
  if (IS_MOB(ch) && MOB_FLAGGED(ch, MOB_C_MOUNT) &&
      AFF_FLAGGED(ch, AFF_CHARM) &&
      (spellnum == SPELL_GREATER_INVIS ||
       spellnum == SPELL_INVISIBLE))
  {
    /* these type of mobs casting invis is problematic */
    return;
  }

  // charmies should not cast invis
  if (IS_MOB(ch) && AFF_FLAGGED(ch, AFF_CHARM) &&
      (spellnum == SPELL_GREATER_INVIS ||
       spellnum == SPELL_INVISIBLE))
  {
    return;
  }

  /* end special restrictions */

  if (loop_counter < (MAX_LOOPS))
    // found a spell, cast it
    cast_spell(ch, victim, NULL, spellnum, 0);

  return;
}

/* note MAX_LOOPS used here too */

/* generic function for spelling up as a caster */
void npc_offensive_spells(struct char_data *ch)
{
  struct char_data *tch = NULL;
  int level, spellnum = -1, loop_counter = 0;
  int use_aoe = 0;

  if (!ch)
    return;

  if (MOB_FLAGGED(ch, MOB_NOCLASS))
    return;

  /* capping */
  if (GET_LEVEL(ch) >= LVL_IMMORT)
    level = LVL_IMMORT - 1;
  else
    level = GET_LEVEL(ch);

  /* 25% of spellup instead of offensive spell */
  if (!rand_number(0, 3))
  {
    npc_spellup(ch);
    return;
  }

  /* our semi-casters will rarely use this function */
  switch (GET_CLASS(ch))
  {
  case CLASS_RANGER:
  case CLASS_PALADIN: // 10 out of 11 times will not cast
    if (rand_number(0, 10))
    {
      npc_class_behave(ch);
      return;
    }
    break;
  case CLASS_BARD: // bards 33% will not cast
    if (!rand_number(0, 2))
    {
      npc_class_behave(ch);
      return;
    }
    break;
    /* our 'healing' types will do another check for spellup */
  case CLASS_DRUID:
  case CLASS_CLERIC:
    /* additional 25% of spellup instead of offensive spell */
    if (!rand_number(0, 3))
    {
      npc_spellup(ch);
      return;
    }
    break;
  }

  /* find random target, and num targets */
  if (!(tch = npc_find_target(ch, &use_aoe)))
    return;

  /* random offensive spell */
  if (use_aoe >= 2)
  {
    do
    {
      spellnum = valid_aoe_spell[rand_number(0, OFFENSIVE_AOE_SPELLS - 1)];
      loop_counter++;
      if (loop_counter >= (MAX_LOOPS / 2))
        break;
    } while (level < spell_info[spellnum].min_level[GET_CLASS(ch)]);

    if (loop_counter < (MAX_LOOPS / 2) && spellnum != -1)
    {
      // found a spell, cast it
      cast_spell(ch, tch, NULL, spellnum, 0);
      return;
    }
  }

  /* we intentionally fall through here,
   some (a lot?) of mobiles will not have aoe spells */
  loop_counter = 0;

  do
  {
    spellnum = valid_offensive_spell[rand_number(0, OFFENSIVE_SPELLS - 1)];
    loop_counter++;
    if (loop_counter >= (MAX_LOOPS / 2))
      break;
  } while (level < spell_info[spellnum].min_level[GET_CLASS(ch)] ||
           affected_by_spell(tch, spellnum));

  if (loop_counter < (MAX_LOOPS / 2) && spellnum != -1)
    // found a spell, cast it
    cast_spell(ch, tch, NULL, spellnum, 0);

  return;
}

/*** MOBILE ACTIVITY ***/

/* the primary engine for mobile activity */
void mobile_activity(void)
{
  struct char_data *ch = NULL, *next_ch = NULL, *vict = NULL, *tmp_char = NULL;
  struct obj_data *obj = NULL, *best_obj = NULL;
  int door = 0, found = FALSE, max = 0, where = -1;

  for (ch = character_list; ch; ch = next_ch)
  {
    next_ch = ch->next;

    if (IN_ROOM(ch) > top_of_world)
      continue;

    if (!IS_MOB(ch))
      continue;

    if (MOB_FLAGGED(ch, MOB_NO_AI))
      continue;

    if (AFF_FLAGGED(ch, AFF_STUN) || AFF_FLAGGED(ch, AFF_PARALYZED) || AFF_FLAGGED(ch, AFF_DAZED) ||
        char_has_mud_event(ch, eSTUNNED) || AFF_FLAGGED(ch, AFF_NAUSEATED))
    {
      send_to_char(ch, "You are unable to move!\r\n");
      continue;
    }

    /* Examine call for special procedure */
    /* not the AWAKE() type of checks are inside the spec_procs */
    if (MOB_FLAGGED(ch, MOB_SPEC) && !no_specials)
    {
      if (mob_index[GET_MOB_RNUM(ch)].func == NULL)
      {
        log("SYSERR: %s (#%d): Attempting to call non-existing mob function.",
            GET_NAME(ch), GET_MOB_VNUM(ch));
        REMOVE_BIT_AR(MOB_FLAGS(ch), MOB_SPEC);
      }
      else
      {
        char actbuf[MAX_INPUT_LENGTH] = "";
        if ((mob_index[GET_MOB_RNUM(ch)].func)(ch, ch, 0, actbuf))
          continue; /* go to next char */
      }
    }

    /* can't do any of the following if not at least AWAKE() and not casting */
    if (!AWAKE(ch) || IS_CASTING(ch))
      continue;

    /* If the mob has no specproc, do the default actions */

    // entry point for npc race and class behaviour in combat -zusuk
    if (GET_LEVEL(ch) > NEWBIE_LEVEL)
    {
      if (FIGHTING(ch))
      {
        // 50% chance will react off of class, 50% chance will react off of race
        if (dice(1, 3) == 1)
          npc_racial_behave(ch);
        else if (dice(1, 3) == 2)
          npc_ability_behave(ch);
        else if (IS_NPC_CASTER(ch))
          npc_offensive_spells(ch);
        else
          npc_class_behave(ch);
        continue;
      }
      else if (!rand_number(0, 8) && IS_NPC_CASTER(ch))
      {
        /* not in combat */
        npc_spellup(ch);
      }
      else if (!rand_number(0, 8) && !IS_NPC_CASTER(ch))
      {
        /* not in combat, non-caster */
        ; // this is where we'd put mob AI to use hide skill, etc
      }
    }

    /* send out mobile echos to room or zone */
    mobile_echos(ch);

    /* Scavenger (picking up objects) */
    if (MOB_FLAGGED(ch, MOB_SCAVENGER))
      if (world[IN_ROOM(ch)].contents && !rand_number(0, 10))
      {
        max = 1;
        best_obj = NULL;
        for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj->next_content)
          if (CAN_GET_OBJ(ch, obj) && GET_OBJ_COST(obj) > max)
          {
            best_obj = obj;
            max = GET_OBJ_COST(obj);
          }
        if (best_obj != NULL)
        {
          obj_from_room(best_obj);
          obj_to_char(best_obj, ch);
          act("$n gets $p.", FALSE, ch, best_obj, 0, TO_ROOM);

          if ((where = find_eq_pos(ch, best_obj, 0)) > 0)
            perform_wear(ch, best_obj, where);

          continue;
        }
      }

    /* Aggressive Mobs */
    if (!MOB_FLAGGED(ch, MOB_HELPER) && (!AFF_FLAGGED(ch, AFF_BLIND) ||
                                         !AFF_FLAGGED(ch, AFF_CHARM)))
    {
      found = FALSE;
      for (vict = world[IN_ROOM(ch)].people; vict && !found;
           vict = vict->next_in_room)
      {

        if (IS_NPC(vict) && !IS_PET(vict))
          continue;

        if (IS_PET(vict) && IS_NPC(vict->master))
          continue;

        if (!CAN_SEE(ch, vict) || (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_NOHASSLE)))
          continue;

        if (MOB_FLAGGED(ch, MOB_WIMPY) && AWAKE(vict))
          continue;

        if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
            (ch->next_in_room != vict && vict->next_in_room != ch))
          continue;

        if (MOB_FLAGGED(ch, MOB_AGGRESSIVE)
            /* we want to replace this with factions system */
            /*||
                (MOB_FLAGGED(ch, MOB_AGGR_EVIL) && IS_EVIL(vict)) ||
                (MOB_FLAGGED(ch, MOB_AGGR_NEUTRAL) && IS_NEUTRAL(vict)) ||
                (MOB_FLAGGED(ch, MOB_AGGR_GOOD) && IS_GOOD(vict)) */
        )
        {
          if (IS_ANIMAL(ch) && HAS_FEAT(vict, FEAT_SOUL_OF_THE_FEY))
          {
            continue;
          }
          if (IS_UNDEAD(ch) && HAS_FEAT(vict, FEAT_ONE_OF_US))
          {
            continue;
          }
          if (MOB_FLAGGED(ch, MOB_ENCOUNTER) && ((GET_LEVEL(ch) - GET_LEVEL(vict)) < 2))
          {
            // We don't want abandoned random encounters killing people they weren't meant for
            hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
            found = TRUE;
          }
          else if (!MOB_FLAGGED(ch, MOB_ENCOUNTER))
          {
            // all other aggro mobs
            hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
            found = TRUE;
          }
        }
      }
    }

    /* Mob Memory */
    found = FALSE;
    /* loop through room, check if each person is in memory */
    for (vict = world[IN_ROOM(ch)].people; vict && !found;
         vict = vict->next_in_room)
    {

      /* this function cross-references memory-list with vict */
      if (!is_in_memory(ch, vict))
        continue;

      /* bingo! */
      found = TRUE;
      if (!FIGHTING(ch))
      {
        act("'!!', exclaims $n.", FALSE, ch, 0, 0, TO_ROOM);
        hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      }
    }

    /* NOTE old charmee rebellion - Deprecated by current system
     * use to be here */

    /* Helper Mobs */
    if ((MOB_FLAGGED(ch, MOB_HELPER) || MOB_FLAGGED(ch, MOB_GUARD)) && (!AFF_FLAGGED(ch, AFF_BLIND) ||
                                                                        !AFF_FLAGGED(ch, AFF_CHARM)))
    {
      found = FALSE;
      for (vict = world[IN_ROOM(ch)].people; vict && !found;
           vict = vict->next_in_room)
      {
        if (ch == vict || !IS_NPC(vict) || !FIGHTING(vict))
          continue;
        if (GROUP(vict) && GROUP(vict) == GROUP(ch))
          continue;
        if (IS_NPC(FIGHTING(vict)) || ch == FIGHTING(vict))
          continue;
        if (MOB_FLAGGED(ch, MOB_GUARD) && !MOB_FLAGGED(ch, MOB_HELPER) && !MOB_FLAGGED(vict, MOB_CITIZEN))
          continue;
        if (MOB_FLAGGED(ch, MOB_GUARD) && (MOB_FLAGGED(ch, MOB_HELPER) || MOB_FLAGGED(vict, MOB_CITIZEN)))
          if (ch->mission_owner && vict->mission_owner && ch->mission_owner != vict->mission_owner)
            continue;
        if (MOB_FLAGGED(ch, MOB_HELPER) && IS_ANIMAL(vict))
          continue;

        act("$n jumps to the aid of $N!", FALSE, ch, 0, vict, TO_ROOM);
        hit(ch, FIGHTING(vict), TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
        found = TRUE;
      }
      if (found)
        continue;
    }

    /* Mob Movement */

    /* follow set path for mobile (like patrols) */
    if (move_on_path(ch))
      continue;

    /* hunt a victim, if applicable */
    hunt_victim(ch);

    /* (mob-listen) is mob interested in fights nearby*/
    if (MOB_FLAGGED(ch, MOB_LISTEN) && !ch->master)
    {
      for (door = 0; door < DIR_COUNT; door++)
      {
        if (!CAN_GO(ch, door))
          continue;
        for (vict = world[EXIT(ch, door)->to_room].people; vict; vict = vict->next_in_room)
        {
          if (FIGHTING(vict) && !rand_number(0, 3) && !ROOM_FLAGGED(vict->in_room, ROOM_NOTRACK))
          {
            perform_move(ch, door, 1);
            return;
          }
        }
      }
    }

    /* random movement */
    if (!rand_number(0, 2)) // customize frequency
      if (!MOB_FLAGGED(ch, MOB_SENTINEL) && (GET_POS(ch) == POS_STANDING) &&
          ((door = rand_number(0, 18)) < DIR_COUNT) && CAN_GO(ch, door) &&
          !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_NOMOB) &&
          !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_DEATH) &&
          (!MOB_FLAGGED(ch, MOB_STAY_ZONE) ||
           (world[EXIT(ch, door)->to_room].zone == world[IN_ROOM(ch)].zone)))
      {
        /* If the mob is charmed, do not move the mob. */
        if (ch->master == NULL)
          perform_move(ch, door, 1);
      }

    /* helping group members use to be here, now its in
     * perform_violence() in fight.c */

    /* a function to move mobile back to its loadroom (if sentinel) */
    /*    if (!HUNTING(ch) && !MEMORY(ch) && !ch->master &&
            MOB_FLAGGED(ch, MOB_SENTINEL) && !IS_PET(ch) &&
            GET_MOB_LOADROOM(ch) != IN_ROOM(ch))
      hunt_loadroom(ch);
*/
    /* pets return to their master */
    if (GET_POS(ch) == POS_STANDING && IS_PET(ch) && IN_ROOM(ch->master) != IN_ROOM(ch) && !HUNTING(ch))
    {
      HUNTING(ch) = ch->master;
      hunt_victim(ch);
    }

    /* return mobile to preferred (default) position if necessary */
    if (GET_POS(ch) != GET_DEFAULT_POS(ch) && MOB_FLAGGED(ch, MOB_SENTINEL) &&
        GET_MOB_LOADROOM(ch) == IN_ROOM(ch))
    {
      if (GET_DEFAULT_POS(ch) == POS_SITTING)
      {
        do_sit(ch, NULL, 0, 0);
      }
      else if (GET_DEFAULT_POS(ch) == POS_RECLINING)
      {
        do_recline(ch, NULL, 0, 0);
      }
      else if (GET_DEFAULT_POS(ch) == POS_RESTING)
      {
        do_rest(ch, NULL, 0, 0);
      }
      else if (GET_DEFAULT_POS(ch) == POS_STANDING)
      {
        do_stand(ch, NULL, 0, 0);
      }
      else if (GET_DEFAULT_POS(ch) == POS_SLEEPING)
      {
        int go_to_sleep = FALSE;
        do_rest(ch, NULL, 0, 0);

        // only go back to sleep if no PCs in the room, and percentage
        if (rand_number(1, 100) <= 10)
        {
          go_to_sleep = TRUE;
          for (tmp_char = world[ch->in_room].people; tmp_char; tmp_char = tmp_char->next_in_room)
          {
            if (!IS_NPC(tmp_char) && CAN_SEE(ch, tmp_char))
            {
              // don't go to sleep
              go_to_sleep = FALSE;
              break;
            }
          }
        }

        if (go_to_sleep == TRUE)
          do_sleep(ch, NULL, 0, 0);
      }
    }

    /* Add new mobile actions here */

  } /* end for() */
}

/* must be at end of file */
#undef SINFO
#undef MOB_SPELLS
#undef RESCUE_LOOP

/**************************/
