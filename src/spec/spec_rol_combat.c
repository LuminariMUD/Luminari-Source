/**
 * @file spec/spec_rol_combat.c
 * Identity-profiled combat adapters for converted Realms of Luminari mobiles.
 */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"

#include "combat/fight.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "magic/domains_schools.h"
#include "magic/spells.h"
#include "mud_event.h"
#include "spec/spec_combat.h"
#include "spec/spec_context.h"
#include "spec/spec_dispatch.h"
#include "spec/spec_rol_conversion.h"

enum rol_monster_combat_effect
{
  ROL_MONSTER_PLANT_POISON = 0,
  ROL_MONSTER_LYCAN_TIGER,
  ROL_MONSTER_LYCAN_FOX,
  ROL_MONSTER_SPIDER_VENOM,
  ROL_MONSTER_ASHENTORIS,
  ROL_MONSTER_BANSHEE_WAIL,
  ROL_MONSTER_FOUR_ARMS,
  ROL_MONSTER_TENTACLE_SLAM,
  ROL_MONSTER_ROT_BRINGER,
  ROL_MONSTER_WINGED_DEVA,
  ROL_MONSTER_SMALL_PRISMATIC,
  ROL_MONSTER_CRITICAL_PRISMATIC,
  ROL_MONSTER_UBER_PRISMATIC,
  ROL_MONSTER_FIRE_BOSS,
  ROL_MONSTER_EARTH_BOSS,
  ROL_MONSTER_AIR_BOSS,
  ROL_MONSTER_WATER_BOSS,
  ROL_MONSTER_PIT_FIEND_BITE
};

struct rol_monster_combat_profile
{
  int mobile_vnum;
  enum rol_monster_combat_effect effect;
  int proc_denominator;
  const char *description;
};

/* Keep this table sorted by converted mobile VNUM for binary lookup. */
static const struct rol_monster_combat_profile rol_monster_combat_profiles[] = {
    {150772, ROL_MONSTER_PLANT_POISON, 3, "Barbed-thorn poison volley."},
    {2000325, ROL_MONSTER_LYCAN_TIGER, 11, "Were-tiger tearing attack."},
    {2000326, ROL_MONSTER_LYCAN_FOX, 6, "Were-fox slashing attack."},
    {2000327, ROL_MONSTER_LYCAN_TIGER, 11, "Were-tiger tearing attack."},
    {2000328, ROL_MONSTER_LYCAN_TIGER, 11, "Were-tiger tearing attack."},
    {2005023, ROL_MONSTER_SPIDER_VENOM, 15, "Random-player venom bite."},
    {2014601, ROL_MONSTER_PLANT_POISON, 3, "Barbed-thorn poison volley."},
    {2020378, ROL_MONSTER_ASHENTORIS, 11, "Life drain and lava storm."},
    {2034833, ROL_MONSTER_BANSHEE_WAIL, 3, "Room-wide sonic wail."},
    {2045116, ROL_MONSTER_FOUR_ARMS, 1, "Extra swing and crushing shockwave."},
    {2045146, ROL_MONSTER_TENTACLE_SLAM, 11, "Room-wide tentacle shockwave."},
    {2045182, ROL_MONSTER_ROT_BRINGER, 1, "One-time flesh helper below forty percent health."},
    {2051246, ROL_MONSTER_WINGED_DEVA, 11, "Healing lightning burst and earthquake."},
    {2053264, ROL_MONSTER_SMALL_PRISMATIC, 11, "Bound helper prismatic spray."},
    {2053265, ROL_MONSTER_CRITICAL_PRISMATIC, 20,
     "Prismatic burst adapted from a source critical event."},
    {2053266, ROL_MONSTER_UBER_PRISMATIC, 3, "Frequent prismatic spray."},
    {2062401, ROL_MONSTER_FIRE_BOSS, 2, "Room-wide elemental fire storm."},
    {2062402, ROL_MONSTER_EARTH_BOSS, 2, "Room-wide falling-rock assault."},
    {2062405, ROL_MONSTER_AIR_BOSS, 2, "Whirlwind strike and forced movement."},
    {2062406, ROL_MONSTER_WATER_BOSS, 2, "Room-wide tidal assault and silence."},
    {2081706, ROL_MONSTER_PIT_FIEND_BITE, 16, "Venomous pit-fiend bite."},
    {2081746, ROL_MONSTER_PIT_FIEND_BITE, 16, "Venomous pit-fiend bite."},
    {2081747, ROL_MONSTER_PIT_FIEND_BITE, 16, "Venomous pit-fiend bite."},
    {2083224, ROL_MONSTER_PIT_FIEND_BITE, 16, "Venomous pit-fiend bite."},
};

static const struct rol_monster_combat_profile *rol_monster_combat_profile_for(int mobile_vnum)
{
  size_t high = sizeof(rol_monster_combat_profiles) / sizeof(rol_monster_combat_profiles[0]);
  size_t low = 0;
  size_t middle;

  while (low < high)
  {
    middle = low + (high - low) / 2;
    if (rol_monster_combat_profiles[middle].mobile_vnum < mobile_vnum)
      low = middle + 1;
    else
      high = middle;
  }
  if (low < sizeof(rol_monster_combat_profiles) / sizeof(rol_monster_combat_profiles[0]) &&
      rol_monster_combat_profiles[low].mobile_vnum == mobile_vnum)
    return &rol_monster_combat_profiles[low];

  return NULL;
}

size_t rol_monster_combat_profile_count(void)
{
  return sizeof(rol_monster_combat_profiles) / sizeof(rol_monster_combat_profiles[0]);
}

bool rol_monster_combat_profile(int mobile_vnum, int *proc_denominator, const char **description)
{
  const struct rol_monster_combat_profile *profile = rol_monster_combat_profile_for(mobile_vnum);

  if (profile == NULL)
    return false;
  if (proc_denominator != NULL)
    *proc_denominator = profile->proc_denominator;
  if (description != NULL)
    *description = profile->description;
  return true;
}

static bool rol_monster_fires(const struct rol_monster_combat_profile *profile)
{
  return profile->proc_denominator <= 1 || rand_number(1, profile->proc_denominator) == 1;
}

static bool rol_monster_room_target(const struct char_data *ch, const struct char_data *victim)
{
  if (ch == NULL || victim == NULL || victim == ch || IN_ROOM(ch) == NOWHERE ||
      IN_ROOM(victim) != IN_ROOM(ch) || GET_POS(victim) <= POS_DEAD ||
      GET_LEVEL(victim) >= LVL_IMMORT)
    return false;

  return !IS_NPC(victim) || IS_PET(victim);
}

static void rol_monster_stun(struct char_data *victim, int rounds)
{
  if (victim == NULL || rounds <= 0)
    return;

  if (GET_POS(victim) > POS_SITTING)
    GET_POS(victim) = POS_SITTING;
  resetCastingData(victim);
  if (can_stun(victim) && char_has_mud_event(victim, eSTUNNED) == NULL)
    attach_mud_event(new_mud_event(eSTUNNED, victim, NULL), PULSE_VIOLENCE * rounds);
}

static struct char_data *rol_monster_random_player(struct char_data *ch)
{
  struct char_data *candidate;
  struct char_data *selected = NULL;
  int eligible = 0;

  for (candidate = world[IN_ROOM(ch)].people; candidate != NULL;
       candidate = candidate->next_in_room)
  {
    if (!rol_monster_room_target(ch, candidate) || IS_NPC(candidate))
      continue;
    eligible++;
    if (rand_number(1, eligible) == 1)
      selected = candidate;
  }
  return selected;
}

static void rol_monster_prismatic(struct char_data *ch, int level)
{
  if (FIGHTING(ch) == NULL)
    return;

  act("$n opens $s hands and releases a prismatic spray!", FALSE, ch, NULL, NULL, TO_ROOM);
  call_magic(ch, FIGHTING(ch), NULL, SPELL_PRISMATIC_SPRAY, 0, level, CAST_INNATE);
}

static void rol_monster_lycan(struct char_data *ch, struct char_data *victim, bool tiger)
{
  struct spec_damage_result result;

  if (tiger)
  {
    act("$n tears into you with massive claws and teeth!", FALSE, ch, NULL, victim, TO_VICT);
    act("$n tears into $N with massive claws and teeth!", FALSE, ch, NULL, victim, TO_NOTVICT);
    result = spec_damage_current_target(ch, victim, dice(35, 10), -1, DAM_SLASHING, FALSE);
  }
  else
  {
    act("$n leaps onto you in a flurry of claws and teeth!", FALSE, ch, NULL, victim, TO_VICT);
    act("$n leaps onto $N in a flurry of claws and teeth!", FALSE, ch, NULL, victim, TO_NOTVICT);
    result = spec_damage_current_target(ch, victim, dice(20, 7), -1, DAM_SLASHING, FALSE);
  }
  (void)result;
}

static void rol_monster_pit_fiend_bite(struct char_data *ch, struct char_data *victim)
{
  struct spec_damage_result result;

  act("$n savagely bites down on your arm!", FALSE, ch, NULL, victim, TO_VICT);
  act("$n savagely bites down on $N's arm!", FALSE, ch, NULL, victim, TO_NOTVICT);
  result = spec_damage_current_target(ch, victim, dice(2, 6), -1, DAM_PUNCTURE, FALSE);
  if (result.status != SPEC_DAMAGE_TARGET_INVALIDATED)
    call_magic(ch, victim, NULL, SPELL_POISON, 0, 6, CAST_INNATE);
}

static void rol_monster_spider_venom(struct char_data *ch)
{
  struct char_data *victim = rol_monster_random_player(ch);

  if (victim == NULL)
    return;
  act("$n rears up and sinks $s fangs deep into you!", FALSE, ch, NULL, victim, TO_VICT);
  act("$n rears up and sinks $s fangs deep into $N!", FALSE, ch, NULL, victim, TO_NOTVICT);
  call_magic(ch, victim, NULL, SPELL_POISON, 0, 4, CAST_INNATE);
}

static void rol_monster_plant_poison(struct char_data *ch)
{
  struct char_data *victim;
  struct char_data *next;
  bool announced = false;

  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = next)
  {
    next = victim->next_in_room;
    if (!rol_monster_room_target(ch, victim) || IS_NPC(victim) || rand_number(1, 3) != 1 ||
        savingthrow(ch, victim, SAVING_FORT, 0, CAST_INNATE, GET_LEVEL(ch), NOSCHOOL))
      continue;
    if (!announced)
    {
      act("$n launches a volley of barbed red thorns!", TRUE, ch, NULL, NULL, TO_ROOM);
      announced = true;
    }
    act("One of $n's thorns pierces your skin!", FALSE, ch, NULL, victim, TO_VICT);
    call_magic(ch, victim, NULL, SPELL_POISON, 0, GET_LEVEL(ch), CAST_INNATE);
  }
}

static void rol_monster_banshee_wail(struct char_data *ch)
{
  struct char_data *victim;
  struct char_data *next;
  int amount;

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF) || AFF_FLAGGED(ch, AFF_SILENCED))
    return;
  act("$n's wail chills you to the bone!", FALSE, ch, NULL, NULL, TO_ROOM);
  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = next)
  {
    next = victim->next_in_room;
    if (!rol_monster_room_target(ch, victim) || IS_NPC(victim))
      continue;
    amount = rand_number(190, 210);
    if (savingthrow(ch, victim, SAVING_WILL, 0, CAST_INNATE, GET_LEVEL(ch), NECROMANCY))
      amount /= 2;
    (void)damage(ch, victim, amount, -1, DAM_SOUND, FALSE);
  }
}

static void rol_monster_shockwave(struct char_data *ch, int save_type)
{
  struct char_data *victim;
  struct char_data *next;

  act("$n crashes forward and sends a crushing shockwave through the room!", TRUE, ch, NULL, NULL,
      TO_ROOM);
  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = next)
  {
    next = victim->next_in_room;
    if (!rol_monster_room_target(ch, victim) ||
        savingthrow(ch, victim, save_type, 0, CAST_INNATE, GET_LEVEL(ch), NOSCHOOL))
      continue;
    act("The shockwave sends you crashing to the ground!", FALSE, ch, NULL, victim, TO_VICT);
    act("The shockwave sends $N crashing to the ground!", FALSE, ch, NULL, victim, TO_NOTVICT);
    rol_monster_stun(victim, rand_number(1, 2));
  }
}

static void rol_monster_four_arms(struct char_data *ch, struct char_data *victim)
{
  if (rand_number(1, 16) == 1)
    rol_monster_shockwave(ch, SAVING_FORT);
  if (FIGHTING(ch) == victim && GET_POS(victim) > POS_DEAD)
    (void)hit(ch, victim, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, ATTACK_TYPE_PRIMARY);
}

static void rol_monster_rot_bringer(struct char_data *ch, struct char_data *victim)
{
  struct char_data *helper;

  if (PROC_FIRED(ch) || GET_HIT(ch) * 100 >= GET_MAX_HIT(ch) * 40)
    return;
  if ((helper = read_mobile(2045193, VIRTUAL)) == NULL)
  {
    log("SYSERR: RoL Rot Bringer helper 2045193 is unavailable");
    return;
  }

  PROC_FIRED(ch) = TRUE;
  act("$n claws at a bloody basin as a massive ball of flesh rises to defend $m!", FALSE, ch, NULL,
      NULL, TO_ROOM);
  char_to_room(helper, IN_ROOM(ch));
  GET_MOB_LOADROOM(helper) = IN_ROOM(ch);
  add_follower(helper, ch);
  (void)set_fighting(helper, victim);
}

static void rol_monster_ashentoris(struct char_data *ch, struct char_data *victim)
{
  struct char_data *target;
  struct char_data *next;
  struct spec_damage_result result;

  act("A blazing beam of black energy drains $N as lava erupts through the room!", TRUE, ch, NULL,
      victim, TO_ROOM);
  result = spec_damage_current_target(ch, victim, 200, -1, DAM_NEGATIVE, FALSE);
  GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + 900);

  for (target = world[IN_ROOM(ch)].people; target != NULL; target = next)
  {
    next = target->next_in_room;
    if (!rol_monster_room_target(ch, target))
      continue;
    (void)damage(ch, target, 400, -1, DAM_FIRE, FALSE);
  }
  (void)result;
}

static void rol_monster_winged_deva(struct char_data *ch)
{
  struct char_data *victim;
  struct char_data *next;
  int result;

  act("A blazing thunderclap strikes the room as $n calls to the heavens!", TRUE, ch, NULL, NULL,
      TO_ROOM);
  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = next)
  {
    next = victim->next_in_room;
    if (!rol_monster_room_target(ch, victim))
      continue;
    result = damage(ch, victim, 300, -1, DAM_ELECTRIC, FALSE);
    if (result >= 0)
      GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + 300);
  }
  if (rand_number(1, 4) == 1)
    call_magic(ch, NULL, NULL, SPELL_EARTHQUAKE, 0, 60, CAST_INNATE);
}

static void rol_monster_area_damage(struct char_data *ch, enum rol_monster_combat_effect effect)
{
  struct char_data *victim;
  struct char_data *next;
  struct char_data *silence_target = NULL;
  int amount;
  int damage_type;
  bool saved;

  if (IS_CASTING(ch))
    return;
  if (effect == ROL_MONSTER_FIRE_BOSS)
    act("A storm of fire erupts from $n and showers the room!", TRUE, ch, NULL, NULL, TO_ROOM);
  else if (effect == ROL_MONSTER_EARTH_BOSS)
    act("$n calls down an avalanche of crushing rocks!", TRUE, ch, NULL, NULL, TO_ROOM);
  else if (effect == ROL_MONSTER_WATER_BOSS)
    act("A giant tidal wave surges out from $n!", TRUE, ch, NULL, NULL, TO_ROOM);

  if (effect == ROL_MONSTER_WATER_BOSS)
    silence_target = rol_monster_random_player(ch);

  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = next)
  {
    next = victim->next_in_room;
    if (!rol_monster_room_target(ch, victim))
      continue;

    if (effect == ROL_MONSTER_FIRE_BOSS)
    {
      amount = rand_number(150, 350);
      damage_type = DAM_FIRE;
      saved = savingthrow(ch, victim, SAVING_REFL, 0, CAST_INNATE, GET_LEVEL(ch), EVOCATION);
    }
    else if (effect == ROL_MONSTER_EARTH_BOSS)
    {
      amount = rand_number(50, 200);
      damage_type = DAM_EARTH;
      saved = savingthrow(ch, victim, SAVING_FORT, 0, CAST_INNATE, GET_LEVEL(ch), CONJURATION);
    }
    else
    {
      amount = rand_number(100, 300);
      damage_type = DAM_WATER;
      saved = savingthrow(ch, victim, SAVING_REFL, 0, CAST_INNATE, GET_LEVEL(ch), EVOCATION);
    }
    if (saved)
      amount /= 2;
    if (damage(ch, victim, amount, -1, damage_type, FALSE) < 0)
      continue;

    if (effect == ROL_MONSTER_EARTH_BOSS && !saved)
      rol_monster_stun(victim, 2);
    else if (effect == ROL_MONSTER_WATER_BOSS && victim == silence_target)
      call_magic(ch, victim, NULL, SPELL_SILENCE, 0, 2, CAST_INNATE);
  }
}

static void rol_monster_stop_combat(struct char_data *victim)
{
  struct char_data *fighter;
  struct char_data *next;

  if (FIGHTING(victim) != NULL)
    stop_fighting(victim);
  for (fighter = combat_list; fighter != NULL; fighter = next)
  {
    next = fighter->next_fighting;
    if (FIGHTING(fighter) == victim)
      stop_fighting(fighter);
  }
}

static void rol_monster_air_boss(struct char_data *ch)
{
  struct char_data *victim = rol_monster_random_player(ch);
  room_rnum destination = NOWHERE;
  int amount;
  int checked;
  int direction;
  bool saved;

  if (victim == NULL || IS_CASTING(ch))
    return;
  amount = rand_number(100, 450);
  saved = savingthrow(ch, victim, SAVING_REFL, 0, CAST_INNATE, GET_LEVEL(ch), EVOCATION);
  if (saved)
    amount /= 2;

  act("A roaring whirlwind slams directly into you!", FALSE, ch, NULL, victim, TO_VICT);
  act("A roaring whirlwind slams directly into $N!", FALSE, ch, NULL, victim, TO_NOTVICT);
  if (damage(ch, victim, amount, -1, DAM_AIR, FALSE) < 0 || saved)
    return;

  direction = rand_number(0, NUM_OF_DIRS - 1);
  for (checked = 0; checked < NUM_OF_DIRS; checked++)
  {
    if (direction >= NUM_OF_DIRS)
      direction = 0;
    if (CAN_GO(victim, direction) &&
        valid_mortal_tele_dest(victim, EXIT(victim, direction)->to_room, false))
    {
      destination = EXIT(victim, direction)->to_room;
      break;
    }
    direction++;
  }
  if (destination == NOWHERE)
  {
    rol_monster_stun(victim, 2);
    return;
  }

  rol_monster_stop_combat(victim);
  act("The whirlwind hurls $n from the room!", TRUE, victim, NULL, NULL, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, destination);
  act("$n tumbles in on $s back!", FALSE, victim, NULL, NULL, TO_ROOM);
  rol_monster_stun(victim, 2);
}

static int rol_monster_small_prismatic_activity(struct spec_event_context *context,
                                                struct char_data *ch)
{
  struct char_data *master = ch->master;

  if (master == NULL || IN_ROOM(master) != IN_ROOM(ch) ||
      (FIGHTING(ch) == NULL && FIGHTING(master) == NULL))
  {
    act("$n vanishes in a swirl of color.", FALSE, ch, NULL, NULL, TO_ROOM);
    extract_char(ch);
    context->invalidation |= SPEC_INVALIDATE_OWNER | SPEC_INVALIDATE_ACTOR;
    return TRUE;
  }
  if (FIGHTING(ch) == NULL && FIGHTING(master) != NULL)
    (void)set_fighting(ch, FIGHTING(master));
  return FALSE;
}

int rol_monster_combat(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(ch);
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);

  return FALSE;
}

int rol_monster_combat_typed(struct spec_event_context *context)
{
  const struct rol_monster_combat_profile *profile;
  struct char_data *ch;
  struct char_data *victim;

  if (context == NULL || context->owner_type != SPEC_OWNER_MOBILE || context->owner == NULL)
    return FALSE;
  ch = context->owner;
  if (!IS_NPC(ch) || !VALID_ROOM_RNUM(IN_ROOM(ch)) ||
      (profile = rol_monster_combat_profile_for(GET_MOB_VNUM(ch))) == NULL)
    return FALSE;

  if (context->event == SPEC_EVENT_MOBILE_ACTIVITY)
  {
    if (profile->effect == ROL_MONSTER_SMALL_PRISMATIC)
      return rol_monster_small_prismatic_activity(context, ch);
    return FALSE;
  }
  if (context->event != SPEC_EVENT_MOBILE_COMBAT_TURN || (victim = FIGHTING(ch)) == NULL ||
      spec_context_validate_combat_target(ch, victim, true) != SPEC_CONTEXT_VALID)
    return FALSE;

  if (profile->effect == ROL_MONSTER_FIRE_BOSS || profile->effect == ROL_MONSTER_EARTH_BOSS ||
      profile->effect == ROL_MONSTER_AIR_BOSS || profile->effect == ROL_MONSTER_WATER_BOSS)
    (void)rol_alert_caller(ch, ch, 0, "");

  if (profile->effect != ROL_MONSTER_PLANT_POISON && profile->effect != ROL_MONSTER_FOUR_ARMS &&
      profile->effect != ROL_MONSTER_ROT_BRINGER && !rol_monster_fires(profile))
    return FALSE;

  switch (profile->effect)
  {
  case ROL_MONSTER_PLANT_POISON:
    rol_monster_plant_poison(ch);
    break;
  case ROL_MONSTER_LYCAN_TIGER:
    rol_monster_lycan(ch, victim, true);
    break;
  case ROL_MONSTER_LYCAN_FOX:
    rol_monster_lycan(ch, victim, false);
    break;
  case ROL_MONSTER_SPIDER_VENOM:
    rol_monster_spider_venom(ch);
    break;
  case ROL_MONSTER_ASHENTORIS:
    rol_monster_ashentoris(ch, victim);
    break;
  case ROL_MONSTER_BANSHEE_WAIL:
    rol_monster_banshee_wail(ch);
    break;
  case ROL_MONSTER_FOUR_ARMS:
    rol_monster_four_arms(ch, victim);
    break;
  case ROL_MONSTER_TENTACLE_SLAM:
    rol_monster_shockwave(ch, SAVING_REFL);
    break;
  case ROL_MONSTER_ROT_BRINGER:
    rol_monster_rot_bringer(ch, victim);
    break;
  case ROL_MONSTER_WINGED_DEVA:
    rol_monster_winged_deva(ch);
    break;
  case ROL_MONSTER_SMALL_PRISMATIC:
    rol_monster_prismatic(ch, 15);
    break;
  case ROL_MONSTER_CRITICAL_PRISMATIC:
    rol_monster_prismatic(ch, 45);
    break;
  case ROL_MONSTER_UBER_PRISMATIC:
    rol_monster_prismatic(ch, 51);
    break;
  case ROL_MONSTER_FIRE_BOSS:
  case ROL_MONSTER_EARTH_BOSS:
  case ROL_MONSTER_WATER_BOSS:
    rol_monster_area_damage(ch, profile->effect);
    break;
  case ROL_MONSTER_AIR_BOSS:
    rol_monster_air_boss(ch);
    break;
  case ROL_MONSTER_PIT_FIEND_BITE:
    rol_monster_pit_fiend_bite(ch, victim);
    break;
  }
  return FALSE;
}
