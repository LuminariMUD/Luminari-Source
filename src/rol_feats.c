/**************************************************************************
 *  File: rol_feats.c                                  Part of LuminariMUD *
 *  Usage: Feats converted from Realms of Luminari player skills.          *
 *                                                                         *
 *  Realms of Luminari carried five player skills with no LuminariMUD      *
 *  counterpart: shadow, calm, establish camp, garrote and accompany.      *
 *  They are re-expressed here as feats driven by d20 checks, ability      *
 *  scores and the feat/daily-use machinery rather than by RoL's           *
 *  percentage skills.                                                     *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"
#include "constants.h"
#include "act.h"
#include "combat/fight.h"
#include "mud_event.h"
#include "actions.h"
#include "mudlim.h"
#include "magic/spells.h"
#include "character/abilities.h"
#include "character/class.h"
#include "character/feats.h"
#include "character/evolutions.h"
#include "magic/domains_schools.h"
#include "movement/movement.h"
#include "character_periodic.h"
#include "bardic_performance.h"
#include "rol_feats.h"
#include "activity_manager.h"
#include "affected_owners.h"
#include "domain_event_world.h"

/* the contested check both ends of a tail make */
#define SHADOW_STEALTH_ROLL(ch) (compute_ability((ch), ABILITY_STEALTH) + d20(ch))
#define SHADOW_NOTICE_ROLL(ch) (compute_ability((ch), ABILITY_PERCEPTION) + d20(ch))

/* how long a camp keeps sheltering its occupants, in combat rounds */
#define CAMP_DURATION 300
/* how long a garroted throat stays closed, in combat rounds */
#define GARROTE_CHOKE_DURATION 2
/* how long a calmed combatant stays settled, in combat rounds */
#define CALM_DURATION 3
/* ceiling on how much accompanists can lift a lead performance */
#define MAX_ACCOMPANIMENT_BONUS 12

/*****************************************************************************
 * shadow - covertly tail a target from room to room
 *
 * RoL contested the tailer's shadow skill against the mark's own.  Here the
 * tail is a contested stealth check against perception, re-rolled on every
 * room the mark leaves.
 ****************************************************************************/

void stop_shadowing(struct char_data *ch, bool notify)
{
  struct char_data *mark = NULL;

  if (ch == NULL || SHADOWING(ch) == NULL)
    return;

  mark = SHADOWING(ch);
  SHADOWING(ch) = NULL;

  if (notify)
    act("You stop shadowing $N.", FALSE, ch, 0, mark, TO_CHAR);
}

/* drop every tail this character is part of, in either direction */
void clear_shadow_links(struct char_data *ch)
{
  struct char_data *tch = NULL;

  if (ch == NULL)
    return;

  SHADOWING(ch) = NULL;

  for (tch = character_list; tch; tch = tch->next)
    if (SHADOWING(tch) == ch)
      SHADOWING(tch) = NULL;
}

/* the mark spotted the tail, or the tail simply lost it */
static void shadow_tail_broken(struct char_data *tail, struct char_data *mark, bool spotted)
{
  SHADOWING(tail) = NULL;

  if (spotted)
  {
    act("$N glances back and catches you following $M!", FALSE, tail, 0, mark, TO_CHAR);
    act("You catch $n trailing along behind you!", FALSE, tail, 0, mark, TO_VICT);
  }
  else
  {
    act("You lose $N's trail.", FALSE, tail, 0, mark, TO_CHAR);
  }
}

/* A tail moving independently loses a mark that is no longer in the room. */
void shadow_movement_complete(struct char_data *ch)
{
  if (ch == NULL || SHADOWING(ch) == NULL)
    return;

  if (IN_ROOM(ch) == NOWHERE || IN_ROOM(SHADOWING(ch)) != IN_ROOM(ch))
    shadow_tail_broken(ch, SHADOWING(ch), FALSE);
}

/* called from perform_move_full() once the mark has changed rooms */
void shadowers_follow(struct char_data *ch, room_rnum was_in, int dir)
{
  struct char_data *tail = NULL, *next_tail = NULL;

  if (ch == NULL || was_in == NOWHERE || !VALID_ROOM_RNUM(was_in))
    return;

  for (tail = world[was_in].people; tail; tail = next_tail)
  {
    next_tail = tail->next_in_room;

    if (tail == ch || SHADOWING(tail) != ch)
      continue;

    if (GET_POS(tail) < POS_STANDING || FIGHTING(tail))
    {
      shadow_tail_broken(tail, ch, FALSE);
      continue;
    }

    /* the tail is only covert while moving silently */
    if (!AFF_FLAGGED(tail, AFF_SNEAK))
    {
      act("You cannot keep after $N without moving silently.", FALSE, tail, 0, ch, TO_CHAR);
      shadow_tail_broken(tail, ch, FALSE);
      continue;
    }

    if (SHADOW_STEALTH_ROLL(tail) < SHADOW_NOTICE_ROLL(ch))
    {
      shadow_tail_broken(tail, ch, TRUE);
      continue;
    }

    act("You slip after $N, keeping to cover.", FALSE, tail, 0, ch, TO_CHAR);
    perform_move(tail, dir, 1);

    /* blocked door, no movement points, a closed gate - the trail is cold */
    if (IN_ROOM(tail) != IN_ROOM(ch))
      shadow_tail_broken(tail, ch, FALSE);
  }
}

ACMDCHECK(can_shadow)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_SHADOW, "You do not know how to tail anyone.\r\n");
  ACMDCHECK_TEMPFAIL_IF(!AFF_FLAGGED(ch, AFF_SNEAK),
                        "You have to be sneaking before you can shadow anyone.\r\n");
  return CAN_CMD;
}

ACMD(do_shadow)
{
  struct char_data *vict = NULL;
  char arg[MAX_INPUT_LENGTH] = {'\0'};

  if (IS_NPC(ch))
    return;

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    if (SHADOWING(ch))
      stop_shadowing(ch, TRUE);
    else
      send_to_char(ch, "Shadow whom?\r\n");
    return;
  }

  PREREQ_CHECK(can_shadow);

  if (!(vict = get_char_room_vis(ch, arg, NULL)))
  {
    send_to_char(ch, "There is nobody here by that name.\r\n");
    return;
  }

  if (vict == ch)
  {
    send_to_char(ch, "You cannot tail yourself.\r\n");
    return;
  }

  if (SHADOWING(ch) == vict)
  {
    act("You are already shadowing $N.", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }

  if (FIGHTING(ch))
  {
    send_to_char(ch, "You are far too busy fighting to tail anyone.\r\n");
    return;
  }

  /* opening contest: are you unnoticed while you take up the trail? */
  if (SHADOW_STEALTH_ROLL(ch) < SHADOW_NOTICE_ROLL(vict))
  {
    act("$N notices you studying $S movements.", FALSE, ch, 0, vict, TO_CHAR);
    act("You notice $n studying your movements.", FALSE, ch, 0, vict, TO_VICT);
    SHADOWING(ch) = NULL;
    USE_MOVE_ACTION(ch);
    return;
  }

  if (SHADOWING(ch))
    stop_shadowing(ch, FALSE);

  SHADOWING(ch) = vict;
  act("You settle in behind $N, matching $S pace from cover.", FALSE, ch, 0, vict, TO_CHAR);
  USE_MOVE_ACTION(ch);
}

/*****************************************************************************
 * calm - a chant that tries to end every fight in the room
 *
 * RoL ran this off 'chant calm'.  Here it is a limited-use feat resisted by
 * a will save, and creatures immune to mind-affecting effects shrug it off.
 ****************************************************************************/

ACMDCHECK(can_calm)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_CALM, "You do not know how to calm anyone.\r\n");
  return CAN_CMD;
}

ACMD(do_calm)
{
  struct char_data *tch = NULL, *next_tch = NULL;
  struct affected_type af;
  int save_level = 0, calmed = 0, resisted = 0;

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_calm);
  PREREQ_HAS_USES(FEAT_CALM, "You must recover before you can calm anyone again.\r\n");

  if (AFF_FLAGGED(ch, AFF_SILENCED))
  {
    send_to_char(ch, "You cannot make a sound.\r\n");
    return;
  }

  /* the chant's difficulty scales with level and force of personality */
  save_level = GET_LEVEL(ch) / 2 + GET_CHA_BONUS(ch);

  act("You raise a slow, settling chant.", FALSE, ch, 0, 0, TO_CHAR);
  act("$n raises a slow, settling chant.", FALSE, ch, 0, 0, TO_ROOM);

  for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch)
  {
    next_tch = tch->next_in_room;

    if (tch == ch || !FIGHTING(tch))
      continue;

    if (is_immune_mind_affecting(ch, tch, FALSE) || affected_by_spell(tch, SKILL_CALM))
    {
      resisted++;
      continue;
    }

    if (savingthrow(ch, tch, SAVING_WILL, 0, CAST_INNATE, save_level, NOSCHOOL))
    {
      resisted++;
      continue;
    }

    /* mark the target so a single chant cannot lock a fight down forever */
    new_affect(&af);
    af.spell = SKILL_CALM;
    af.duration = CALM_DURATION;
    affect_join(tch, &af, FALSE, FALSE, FALSE, FALSE);

    if (FIGHTING(ch) == tch)
      stop_fighting(ch);
    if (FIGHTING(tch))
      stop_fighting(tch);
    if (IS_NPC(tch))
      clearMemory(tch);

    act("The chant settles over you, and your anger drains away.", FALSE, ch, 0, tch, TO_VICT);
    act("$N lowers $S guard, the fight gone out of $M.", FALSE, ch, 0, tch, TO_NOTVICT);
    calmed++;
  }

  if (!calmed && !resisted)
    send_to_char(ch, "Nobody here is fighting.\r\n");
  else
    send_to_char(ch, "Your chant calms %d combatant%s%s.\r\n", calmed, calmed == 1 ? "" : "s",
                 resisted ? " - others shrug it off" : "");

  start_daily_use_cooldown(ch, FEAT_CALM);
  USE_STANDARD_ACTION(ch);
}

/*****************************************************************************
 * establish camp - pitch a camp that shelters anyone in its room
 *
 * RoL let a camp act as a wilderness rent point.  LuminariMUD already saves
 * on quit, so a camp here speeds recovery in its room and becomes the
 * campers' return point.
 ****************************************************************************/

/* difficulty of pitching a camp in this room */
static int camp_difficulty(struct char_data *ch)
{
  int dc = 15;

  switch (SECT(IN_ROOM(ch)))
  {
  case SECT_FIELD:
  case SECT_FOREST:
  case SECT_HILLS:
  case SECT_ROAD_NS:
  case SECT_ROAD_EW:
  case SECT_ROAD_INT:
  case SECT_CITY:
    break;
  case SECT_DESERT:
  case SECT_MARSHLAND:
  case SECT_MOUNTAIN:
  case SECT_UD_WILD:
    dc += 5;
    break;
  case SECT_HIGH_MOUNTAIN:
    dc += 8;
    break;
  default:
    dc += 3;
    break;
  }

  if (OUTSIDE(ch))
  {
    if (weather_info.sky == SKY_RAINING)
      dc += 4;
    else if (weather_info.sky == SKY_LIGHTNING)
      dc += 8;
  }

  return dc;
}

static bool camp_location_allowed(struct char_data *ch)
{
  if (ch == NULL || !VALID_ROOM_RNUM(IN_ROOM(ch)) || ROOM_FLAGGED(IN_ROOM(ch), ROOM_INDOORS) ||
      AFF_FLAGGED(ch, AFF_FLYING))
    return false;

  switch (SECT(IN_ROOM(ch)))
  {
  case SECT_WATER_SWIM:
  case SECT_WATER_NOSWIM:
  case SECT_UNDERWATER:
  case SECT_OCEAN:
  case SECT_UD_WATER:
  case SECT_UD_NOSWIM:
  case SECT_FLYING:
    return false;
  default:
    return true;
  }
}

/* extra hitpoint and movement recovery for anyone resting in a camp room */
int camp_recovery_bonus(struct char_data *ch, int gain)
{
  if (ch == NULL || !VALID_ROOM_RNUM(IN_ROOM(ch)))
    return 0;

  if (GET_POS(ch) < POS_SLEEPING || GET_POS(ch) > POS_SITTING)
    return 0;

  return ROOM_AFFECTED(IN_ROOM(ch), RAFF_CAMP) ? gain / 2 : 0;
}

static void camp_create_site(struct char_data *ch)
{
  struct raff_node *raff = NULL;
  room_rnum room;

  if (ch == NULL || !VALID_ROOM_RNUM(IN_ROOM(ch)) || ROOM_AFFECTED(IN_ROOM(ch), RAFF_CAMP))
    return;

  room = IN_ROOM(ch);
  CREATE(raff, struct raff_node, 1);
  raff->room = room;
  raff->timer = CAMP_DURATION;
  raff->affection = RAFF_CAMP;
  raff->spell = SKILL_CAMP;
  raff->next = raff_list;
  raff_list = raff;
  affected_room_owner_add(raff);
  SET_BIT(ROOM_AFFECTIONS(room), RAFF_CAMP);
}

static void camp_set_return_point(struct char_data *ch, struct char_data *tch)
{
  if (tch != NULL && !IS_NPC(tch))
    GET_LOADROOM(tch) = GET_ROOM_VNUM(IN_ROOM(ch));
}

#ifdef LUMINARI_CUTEST
void test_camp_create_site(struct char_data *ch)
{
  if (ch != NULL && VALID_ROOM_RNUM(IN_ROOM(ch)))
    camp_create_site(ch);
}
#endif

ACMDCHECK(can_camp)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_ESTABLISH_CAMP, "You do not know how to make camp.\r\n");
  ACMDCHECK_TEMPFAIL_IF(!camp_location_allowed(ch), "You can't camp here!\r\n");
  return CAN_CMD;
}

struct camp_activity_context
{
  bool successful_check;
};

static bool camp_site_is_usable(struct char_data *ch)
{
  return camp_location_allowed(ch) && FIGHTING(ch) == NULL &&
         !ROOM_AFFECTED(IN_ROOM(ch), RAFF_CAMP);
}

static void camp_apply_result(struct char_data *ch, bool successful_check)
{
  struct char_data *tch = NULL;

  if (!successful_check)
  {
    act("You work at a camp, but the site defeats you.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n tries to make camp, without much success.", FALSE, ch, 0, 0, TO_ROOM);
    return;
  }
  act("You clear a site, set your gear and get a fire going.", FALSE, ch, 0, 0, TO_CHAR);
  act("$n clears a site, sets $s gear and gets a fire going.", FALSE, ch, 0, 0, TO_ROOM);
  send_to_char(ch, "Anyone resting here will recover far more quickly.\r\n");
  camp_create_site(ch);
  camp_set_return_point(ch, ch);
  if (GROUP(ch))
  {
    simple_list(NULL);
    while ((tch = (struct char_data *)simple_list(GROUP(ch)->members)) != NULL)
    {
      if (tch == ch || IN_ROOM(tch) != IN_ROOM(ch))
        continue;
      camp_set_return_point(ch, tch);
      act("$n's camp is ready for you to settle into.", FALSE, ch, 0, tch, TO_VICT);
    }
    simple_list(NULL);
  }
}

static bool camp_activity_recheck(struct char_data *ch, void *target, void *context)
{
  (void)context;
  return camp_site_is_usable(ch) && target == &world[IN_ROOM(ch)];
}

static void camp_activity_progress(struct char_data *ch, void *target, uint32_t completed_steps,
                                   uint32_t total_steps, void *context)
{
  (void)target;
  (void)total_steps;
  (void)context;
  if (completed_steps == 1U)
    act("You clear the ground and gather material for the camp.", FALSE, ch, 0, 0, TO_CHAR);
  else
    act("You arrange the shelter and prepare a safe fire.", FALSE, ch, 0, 0, TO_CHAR);
}

static void camp_activity_complete(struct char_data *ch, void *target, void *context)
{
  struct camp_activity_context *camp = context;

  (void)target;
  camp_apply_result(ch, camp != NULL && camp->successful_check);
}

static void camp_activity_context_cleanup(void *context)
{
  free(context);
}

ACMD(do_camp)
{
  struct primary_activity_definition definition;
  struct camp_activity_context *context;
  bool successful_check;
  int dc = 0, check = 0;

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_camp);

  if (FIGHTING(ch))
  {
    send_to_char(ch, "Not while you are fighting!\r\n");
    return;
  }

  if (ROOM_AFFECTED(IN_ROOM(ch), RAFF_CAMP))
  {
    send_to_char(ch, "There is already a camp here.\r\n");
    return;
  }

  dc = camp_difficulty(ch);
  check = compute_ability(ch, ABILITY_SURVIVAL) + d20(ch);
  if (!primary_activity_feature_enabled(PRIMARY_ACTIVITY_CAMP))
  {
    camp_apply_result(ch, check >= dc);
    USE_STANDARD_ACTION(ch);
    if (check >= dc)
      USE_MOVE_ACTION(ch);
    return;
  }
  context = calloc(1U, sizeof(*context));
  if (context == NULL)
  {
    send_to_char(ch, "You cannot begin making camp right now.\r\n");
    return;
  }
  successful_check = check >= dc;
  context->successful_check = successful_check;
  memset(&definition, 0, sizeof(definition));
  definition.type = PRIMARY_ACTIVITY_CAMP;
  definition.display_name = "building camp";
  definition.capabilities = PRIMARY_ACTIVITY_CAP_MOVEMENT | PRIMARY_ACTIVITY_CAP_HANDS |
                            PRIMARY_ACTIVITY_CAP_ATTENTION | PRIMARY_ACTIVITY_CAP_STANDARD |
                            PRIMARY_ACTIVITY_CAP_MOVE;
  definition.traits = PRIMARY_ACTIVITY_TRAIT_STATIONARY | PRIMARY_ACTIVITY_TRAIT_DISTRACTED |
                      PRIMARY_ACTIVITY_TRAIT_HANDS_OCCUPIED | PRIMARY_ACTIVITY_TRAIT_OBVIOUS;
  definition.progress_model = PRIMARY_ACTIVITY_PROGRESS_PROGRESSIVE;
  definition.progress_owner = PRIMARY_ACTIVITY_PROGRESS_CHARACTER;
  definition.total_steps = 3U;
  definition.step_interval = 2 RL_SEC;
  definition.combat_actions_required = ACTION_STANDARD | ACTION_MOVE;
  definition.movement_response = PRIMARY_ACTIVITY_RESPONSE_CANCEL;
  definition.damage_response = PRIMARY_ACTIVITY_RESPONSE_DELAY;
  definition.combat_response = PRIMARY_ACTIVITY_RESPONSE_PAUSE;
  definition.target_loss_response = PRIMARY_ACTIVITY_RESPONSE_CANCEL;
  definition.command_response = PRIMARY_ACTIVITY_RESPONSE_REJECT;
  definition.delay_pulses = 2 RL_SEC;
  definition.recheck = camp_activity_recheck;
  definition.progress = camp_activity_progress;
  definition.complete = camp_activity_complete;
  definition.cleanup_context = camp_activity_context_cleanup;
  definition.context = context;
  if (!primary_activity_start(ch, domain_event_room_handle(IN_ROOM(ch)), &definition))
  {
    free(context);
    send_to_char(ch, "You are already occupied or cannot begin making camp right now.\r\n");
    return;
  }
  act("You begin clearing a site and laying out your camp.", FALSE, ch, 0, 0, TO_CHAR);
  act("$n begins clearing a site and laying out a camp.", FALSE, ch, 0, 0, TO_ROOM);
  USE_STANDARD_ACTION(ch);
  if (successful_check)
    USE_MOVE_ACTION(ch);
}

/*****************************************************************************
 * garrote - a strangling attack made from concealment
 *
 * RoL required a garrote weapon.  Here the requirement is a free hand plus
 * the same hide-then-sneak posture that sap already asks for.
 ****************************************************************************/

/* creatures that do not breathe cannot be strangled */
static bool can_be_garroted(struct char_data *ch, struct char_data *vict)
{
  if (IS_UNDEAD(vict) || IS_CONSTRUCT(vict) || IS_GOLEM(vict) || IS_ELEMENTAL(vict))
  {
    act("$N has no breath to cut off.", FALSE, ch, 0, vict, TO_CHAR);
    return FALSE;
  }

  if (IS_INCORPOREAL(vict))
  {
    act("There is nothing solid there to strangle.", FALSE, ch, 0, vict, TO_CHAR);
    return FALSE;
  }

  return TRUE;
}

ACMDCHECK(can_garrote)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_GARROTE, "You do not know how to use a garrote.\r\n");
  ACMDCHECK_TEMPFAIL_IF(!AFF_FLAGGED(ch, AFF_HIDE) || !AFF_FLAGGED(ch, AFF_SNEAK),
                        "You have to be sneaking and hiding (in that order) before you can "
                        "garrote anyone.\r\n");
  ACMDCHECK_TEMPFAIL_IF(hands_available(ch) < 1, "You need a free hand to loop the cord.\r\n");
  return CAN_CMD;
}

ACMD(do_garrote)
{
  struct char_data *vict = NULL;
  struct affected_type af;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  int prob = 0, dam = 0, save_level = 0;

  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_garrote);

  one_argument(argument, arg, sizeof(arg));

  PREREQ_NOT_PEACEFUL_ROOM();

  if (FIGHTING(ch))
  {
    send_to_char(ch, "You are too busy fighting to do this!\r\n");
    return;
  }

  if (!(vict = get_char_room_vis(ch, arg, NULL)))
  {
    send_to_char(ch, "Garrote whom?\r\n");
    return;
  }

  if (vict == ch)
  {
    send_to_char(ch, "That would be a poor idea.\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) && ch->next_in_room != vict &&
      vict->next_in_room != ch)
  {
    send_to_char(ch, "You simply can't reach that far.\r\n");
    return;
  }

  if (CAN_SEE(vict, ch))
  {
    act("$N is watching you far too closely.", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }

  if (GET_SIZE(ch) < GET_SIZE(vict))
  {
    send_to_char(ch, "You can't reach that high.\r\n");
    return;
  }

  if (GET_SIZE(ch) - GET_SIZE(vict) >= 2)
  {
    send_to_char(ch, "The target is too small.\r\n");
    return;
  }

  if (!can_be_garroted(ch, vict))
    return;

  /* stepping in unseen is the whole point of the technique */
  prob = 4;

  if (HAS_EVOLUTION(vict, EVOLUTION_UNDEAD_APPEARANCE))
    prob -= get_evolution_appearance_save_bonus(vict);

  if (attack_roll(ch, vict, ATTACK_TYPE_UNARMED, FALSE, prob) > 0)
  {
    dam = dice(2, 6) + GET_LEVEL(ch) / 2 + GET_STR_BONUS(ch);
    act("You whip a cord around $N's throat and haul back!", FALSE, ch, 0, vict, TO_CHAR);
    act("$n whips a cord around $N's throat from behind!", FALSE, ch, 0, vict, TO_NOTVICT);
    act("A cord bites into your throat from behind!", FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
    if (damage(ch, vict, dam, SKILL_GARROTE, DAM_FORCE, FALSE) < 0)
    {
      /* the cord finished the target off */
      USE_STANDARD_ACTION(ch);
      USE_MOVE_ACTION(ch);
      return;
    }

    save_level = GET_LEVEL(ch) / 2 + GET_DEX_BONUS(ch);

    if (!savingthrow(ch, vict, SAVING_FORT, 0, CAST_INNATE, save_level, NOSCHOOL))
    {
      new_affect(&af);
      af.spell = SKILL_GARROTE;
      af.duration = GARROTE_CHOKE_DURATION;
      SET_BIT_AR(af.bitvector, AFF_SILENCED);
      SET_BIT_AR(af.bitvector, AFF_STAGGERED);
      affect_join(vict, &af, TRUE, FALSE, FALSE, FALSE);

      act("$N chokes, unable to draw breath or make a sound!", FALSE, ch, 0, vict, TO_CHAR);
      act("You choke, unable to draw breath or make a sound!", FALSE, ch, 0, vict,
          TO_VICT | TO_SLEEP);
      act("$N chokes helplessly against the cord!", FALSE, ch, 0, vict, TO_NOTVICT);
    }

    damage_shield_check(ch, vict, ATTACK_TYPE_UNARMED, TRUE, DAM_FORCE);
  }
  else
  {
    act("Your cord slips off $N's throat!", FALSE, ch, 0, vict, TO_CHAR);
    damage(ch, vict, 0, SKILL_GARROTE, DAM_FORCE, FALSE);
  }

  USE_STANDARD_ACTION(ch);
  USE_MOVE_ACTION(ch);
}

/*****************************************************************************
 * accompany - back a grouped performer instead of leading your own song
 *
 * RoL let a second bard add skill to the lead's song and take it over on a
 * failure.  Both halves are kept here, expressed through the perform
 * ability and the existing performance slots.
 ****************************************************************************/

void stop_accompanying(struct char_data *ch, bool notify)
{
  struct char_data *lead = NULL;

  if (ch == NULL || ACCOMPANYING(ch) == NULL)
    return;

  lead = ACCOMPANYING(ch);
  ACCOMPANYING(ch) = NULL;

  if (notify)
  {
    act("You let $N's performance carry on without you.", FALSE, ch, 0, lead, TO_CHAR);
    act("$n stops accompanying you.", FALSE, ch, 0, lead, TO_VICT);
  }
}

/* drop every accompaniment this character is part of, in either direction */
void clear_accompany_links(struct char_data *ch)
{
  struct char_data *tch = NULL;

  if (ch == NULL)
    return;

  ACCOMPANYING(ch) = NULL;

  for (tch = character_list; tch; tch = tch->next)
    if (ACCOMPANYING(tch) == ch)
      ACCOMPANYING(tch) = NULL;
}

/* is this accompanist still able to back the lead? */
static bool accompanist_is_able(struct char_data *tch, struct char_data *lead)
{
  if (tch == NULL || tch == lead || ACCOMPANYING(tch) != lead)
    return FALSE;

  if (IN_ROOM(tch) != IN_ROOM(lead) || GET_POS(tch) <= POS_STUNNED)
    return FALSE;

  if (GROUP(tch) == NULL || GROUP(tch) != GROUP(lead))
    return FALSE;

  if (!HAS_FEAT(tch, FEAT_ACCOMPANY) || IS_PERFORMING(tch))
    return FALSE;

  if (AFF_FLAGGED(tch, AFF_SILENCED))
    return FALSE;

  return TRUE;
}

/* how much the accompanists raise the lead performance's effectiveness */
int accompaniment_bonus(struct char_data *ch)
{
  struct char_data *tch = NULL;
  int bonus = 0;

  if (ch == NULL || IN_ROOM(ch) == NOWHERE)
    return 0;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (!accompanist_is_able(tch, ch))
      continue;

    bonus += MAX(1, compute_ability(tch, ABILITY_PERFORM) / 4);

    if (get_equipped_bardic_instrument(tch) != NULL)
      bonus += 1;
  }

  return MIN(MAX_ACCOMPANIMENT_BONUS, bonus);
}

/* the lead faltered - hand the song to an accompanist rather than lose it */
bool accompany_takeover(struct char_data *ch, int performance_num)
{
  struct char_data *tch = NULL;

  if (ch == NULL || IN_ROOM(ch) == NOWHERE || !is_valid_performance(performance_num))
    return FALSE;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (!accompanist_is_able(tch, ch))
      continue;

    if (!HAS_FEAT(tch, performance_info[performance_num][PERFORMANCE_FEATNUM]))
      continue;

    if (!can_perform(tch, performance_num, FALSE, TRUE))
      continue;

    ACCOMPANYING(tch) = NULL;
    GET_PERFORMING(tch) = performance_num;
    GET_SECONDARY_PERFORMING(tch) = PERFORMANCE_NONE;
    IS_PERFORMING(tch) = TRUE;
    character_periodic_sync(tch);

    act("$N picks the song up where you dropped it.", FALSE, ch, 0, tch, TO_CHAR);
    act("You pick the song up where $n dropped it.", FALSE, ch, 0, tch, TO_VICT);
    act("$N takes the lead of the performance from $n.", FALSE, ch, 0, tch, TO_NOTVICT);

    return TRUE;
  }

  return FALSE;
}

ACMDCHECK(can_accompany)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_ACCOMPANY, "You do not know how to accompany anyone.\r\n");
  return CAN_CMD;
}

ACMD(do_accompany)
{
  struct char_data *vict = NULL;
  char arg[MAX_INPUT_LENGTH] = {'\0'};

  if (IS_NPC(ch))
    return;

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    if (ACCOMPANYING(ch))
      stop_accompanying(ch, TRUE);
    else
      send_to_char(ch, "Accompany whom?\r\n");
    return;
  }

  PREREQ_CHECK(can_accompany);

  if (!(vict = get_char_room_vis(ch, arg, NULL)))
  {
    send_to_char(ch, "There is nobody here by that name.\r\n");
    return;
  }

  if (vict == ch)
  {
    send_to_char(ch, "You cannot accompany yourself.\r\n");
    return;
  }

  if (!GROUP(ch) || GROUP(ch) != GROUP(vict))
  {
    act("You would have to be grouped with $N to follow $S lead.", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }

  if (!IS_PERFORMING(vict))
  {
    act("$N is not performing.", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }

  if (IS_PERFORMING(ch))
  {
    send_to_char(ch, "Stop your own performance first.\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_SILENCED))
  {
    send_to_char(ch, "You cannot make a sound.\r\n");
    return;
  }

  if (ACCOMPANYING(ch) == vict)
  {
    act("You are already accompanying $N.", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }

  if (ACCOMPANYING(ch))
    stop_accompanying(ch, FALSE);

  ACCOMPANYING(ch) = vict;

  act("You find $N's rhythm and fall in behind it.", FALSE, ch, 0, vict, TO_CHAR);
  act("$n falls in behind your performance, backing it.", FALSE, ch, 0, vict, TO_VICT);
  act("$n begins accompanying $N's performance.", TRUE, ch, 0, vict, TO_NOTVICT);

  USE_MOVE_ACTION(ch);
}

#undef SHADOW_STEALTH_ROLL
#undef SHADOW_NOTICE_ROLL
#undef CAMP_DURATION
#undef GARROTE_CHOKE_DURATION
#undef CALM_DURATION
#undef MAX_ACCOMPANIMENT_BONUS
