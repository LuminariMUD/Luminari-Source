/* ************************************************************************
 *      File:   vessels_gunnery.c                     Part of LuminariMUD  *
 *   Purpose:   Vessel gunnery: the geometry hit model, locks and battle   *
 *              stations, arc fire, sighting, scanning, and NPC return     *
 *              fire (vessels-ships study S4, 3.3.4)                       *
 * ********************************************************************** */

/*
 * A shot hits on d20 + gunnery bonus >= DC. The DC comes from DurisMUD's
 * weaponsight(): the chance an untrained crew hits, from the range, the
 * target's size, and how fast the firing solution is changing, projected
 * one second ahead for both hulls. That chance, after Duris's 2d50 roll,
 * becomes the DC 21 - round(20 * chance). The gunnery bonus is the gunner's
 * tier plus the firing character's Dexterity (direct fire) or Intelligence
 * (ballistic) modifier, at most +7, the Duris elite crew.
 *
 * Locking a contact or trading shots puts a crew at battle stations, which
 * last 180 s after the lock clears or the last shot.
 */

#include "conf.h"
#include "core/sysdep.h"
#include <math.h>
#include "core/structs.h"
#include "core/utils.h"
#include "core/comm.h"
#include "core/db.h"
#include "core/handler.h"
#include "core/interpreter.h"
#include "magic/spells.h"
#include "vessels.h"

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/* Duris ship units per room: a hull at speed 150 crosses a room a second. */
#define VESSEL_DURIS_UNITS_PER_ROOM 150.0

/** A cell check that admits every room: for sailing a copy of a hull. */
bool vessel_open_water(struct greyhawk_ship_data *ship, int x, int y, int z)
{
  (void)ship;
  (void)x;
  (void)y;
  (void)z;
  return TRUE;
}

/**
 * Where a hull will be in `ticks` vessel ticks: a copy sailed on her present
 * orders, turning and accelerating as she will.
 */
void vessel_project(struct greyhawk_ship_data *ship, struct greyhawk_ship_data *next,
                    struct autopilot_data *pilot, int ticks)
{
  double max_speed;
  int tick;

  max_speed = vessel_max_speed(ship);
  *next = *ship;
  if (ship->autopilot != NULL)
  {
    *pilot = *ship->autopilot;
    next->autopilot = pilot;
  }
  if (vessel_is_moored(ship) || ship->departure_ticks > 0)
  {
    return;
  }
  for (tick = 0; tick < ticks; tick++)
  {
    vessel_sail_tick(next, max_speed, vessel_open_water, NULL, NULL);
  }
}

/**
 * Chance that Duris's 2d50 roll meets 100 - sight (volley_hit_percent()):
 * two dice favour the middle, so low chances hit less often and high ones
 * more often.
 */
double vessel_volley_chance(int sight)
{
  int hits;
  int sum;

  hits = 0;
  for (sum = MAX(2, 100 - sight); sum <= 100; sum++)
  {
    hits += sum <= 51 ? sum - 1 : 101 - sum;
  }
  return (double)hits / 2500.0;
}

/**
 * Duris weaponsight() for an untrained crew: the percent chance, 1-100, of a
 * hit from the range, the target's size, the motion of the firing solution
 * over the next second, and the gun crew's fatigue. The target must lie
 * inside the weapon's band.
 */
static int vessel_weapon_sight(struct greyhawk_ship_data *ship,
                               const struct vessel_weapon_type *type,
                               struct greyhawk_ship_data *target)
{
  struct greyhawk_ship_data ship_next;
  struct greyhawk_ship_data target_next;
  struct autopilot_data ship_pilot;
  struct autopilot_data target_pilot;
  double range;
  double bearing;
  double crossing;
  double closing;
  double turning;
  double motion;
  double hit;
  double miss;
  double max_miss;
  double min_miss;
  double range_mod;

  range = vessel_range_between(ship, target);
  bearing = vessel_bearing_between(ship, target);
  crossing = fabs(sin((target->heading - bearing) * M_PI / 180.0)) * target->speed /
             VESSEL_DURIS_SPEED_SCALE;

  vessel_project(ship, &ship_next, &ship_pilot, 2);
  vessel_project(target, &target_next, &target_pilot, 2);
  closing =
      fabs(vessel_range_between(&ship_next, &target_next) - range) * VESSEL_DURIS_UNITS_PER_ROOM;
  turning = fabs(vessel_heading_difference(bearing - ship->heading,
                                           vessel_bearing_between(&ship_next, &target_next) -
                                               ship_next.heading));

  /* Lofted shots care most about the closing speed, direct fire about the
   * crossing speed and the swing of the bearing. */
  if (IS_SET(type->flags, VESSEL_WEAPON_BALLISTIC))
  {
    motion = crossing / 2.0 + turning * 3.0 + closing;
  }
  else
  {
    motion = crossing + turning * 4.0 + closing / 4.0;
  }

  /* A bigger target is easier to hit, the more so the faster the solution
   * changes; accuracy improves sharply inside three quarters of maximum
   * range; a hull aloft is half again harder to hit. */
  hit = 0.5 / (1.0 + motion / 50.0);
  hit += (sqrt((double)vessel_class_handling(target->vessel_type)->hull_weight) - 3.0) / 100.0;
  miss = 1.0 - hit;
  max_miss = miss;
  min_miss = pow(miss - 0.05, 4.0);
  range_mod = fmax(0.0, fmin(1.0, ((double)type->max_range - range) / (type->max_range * 0.75)));
  miss = max_miss - (max_miss - min_miss) * range_mod;
  if (target->z > 0.0)
  {
    miss = fmin(miss * 1.5, 0.99);
  }
  hit = fmax(0.01, fmin(1.0, 1.0 - miss) * vessel_stamina_modifier(ship));
  return (int)(hit * 100.0);
}

/** The DC a weapon's shot at target must meet: 21 - round(20 * chance). */
int vessel_gunnery_dc(struct greyhawk_ship_data *ship, const struct vessel_weapon_type *type,
                      struct greyhawk_ship_data *target)
{
  if (ship == NULL || type == NULL || target == NULL)
  {
    return 21;
  }
  return 21 - (int)lround(20.0 * vessel_volley_chance(vessel_weapon_sight(ship, type, target)));
}

/**
 * The gunnery bonus: the gunner's tier (+2/+4/+6) plus the firing
 * character's Dexterity, or Intelligence for a ballistic weapon; an NPC crew
 * fires as a trained crew. At most VESSEL_GUNNERY_BONUS_MAX.
 */
int vessel_gunnery_bonus(const struct greyhawk_ship_data *ship, struct char_data *ch,
                         const struct vessel_weapon_type *type)
{
  int bonus;

  if (ship == NULL || type == NULL)
  {
    return 0;
  }
  /* NOLINTNEXTLINE(bugprone-signed-char-misuse) -- a gunnery modifier, not a character */
  bonus = ship->guncrew.gunadjust;
  if (ch == NULL)
  {
    bonus += VESSEL_NPC_GUNNERY_BONUS;
  }
  else
  {
    bonus += IS_SET(type->flags, VESSEL_WEAPON_BALLISTIC) ? GET_INT_BONUS(ch) : GET_DEX_BONUS(ch);
  }
  return MIN(VESSEL_GUNNERY_BONUS_MAX, bonus);
}

/** Percent chance that d20 + bonus meets dc; a natural 1 misses, a 20 hits. */
int vessel_hit_percent(int dc, int bonus)
{
  return MAX(1, MIN(19, 21 - (dc - bonus))) * 5;
}

/**
 * A weapon's reload in vessel ticks: the catalogue reload less 15% of the
 * gunner mod (0.15 a tier), lengthened by the crew's fatigue.
 */
static short int vessel_reload_ticks(const struct greyhawk_ship_data *ship,
                                     const struct vessel_weapon_type *type)
{
  return (short int)MAX(1, (int)(type->reload * (1.0 - 0.15 * 0.15 * ship->crew_tier[CREW_GUNNER]) /
                                 vessel_stamina_modifier(ship)));
}

bool vessel_at_battle_stations(const struct greyhawk_ship_data *ship)
{
  return ship != NULL && ship->battle_ticks > 0;
}

/** A crew reeling from a mental blast can neither steer, fire, reload, nor repair. */
bool vessel_crew_stunned(const struct greyhawk_ship_data *ship)
{
  return ship != NULL && ship->stun_ticks > 0;
}

void vessel_battle_stations(struct greyhawk_ship_data *ship)
{
  if (ship->battle_ticks == 0)
  {
    send_to_ship(ship, "The crew scrambles to battle stations!");
  }
  ship->battle_ticks = VESSEL_BATTLE_STATIONS_TICKS;
}

/**
 * Why this hull cannot fire, or NULL: not berthed in port, anchored,
 * submerged, going down, stunned, or reeling from a ram.
 */
const char *vessel_hull_fire_problem(struct greyhawk_ship_data *ship)
{
  if (vessel_is_sinking(ship))
  {
    return "She is going down - the gun crews are abandoning ship!";
  }
  if (vessel_ship_is_in_port(ship))
  {
    return "The harbor watch forbids gunfire from a berth - put to sea first.";
  }
  if (ship->anchored)
  {
    return "She rides at anchor; weigh anchor before opening fire.";
  }
  if (ship->z < 0.0)
  {
    return "She must surface before her guns can fire.";
  }
  if (vessel_crew_stunned(ship))
  {
    return "The crew reels from a mental blast and cannot work the guns.";
  }
  if (ship->ram_gun_ticks > 0)
  {
    return "The gun crews are still picking themselves up from the ram.";
  }
  return NULL;
}

/**
 * Why this contact cannot be engaged, or NULL: a hull in port is under the
 * harbor's protection and a submerged one is beyond the guns. The buffer
 * holds the answer until the next call.
 */
const char *vessel_target_problem(struct greyhawk_ship_data *target)
{
  static char problem[MAX_STRING_LENGTH];

  if (vessel_ship_is_in_port(target))
  {
    snprintf(problem, sizeof(problem), "%s lies in harbor, under the port's protection.",
             target->name);
    return problem;
  }
  if (target->z < 0.0)
  {
    snprintf(problem, sizeof(problem), "%s is submerged, beyond your guns.", target->name);
    return problem;
  }
  return NULL;
}

/**
 * The locked contact, or NULL. A lock on a contact that has gone, come under
 * the harbor's protection, dived, or left sight is lost here, so no shot or
 * sighting uses it.
 */
struct greyhawk_ship_data *vessel_locked_target(struct greyhawk_ship_data *ship)
{
  struct greyhawk_ship_data *target;

  if (ship->lock_target == 0)
  {
    return NULL;
  }
  target = &greyhawk_ships[ship->lock_target];
  if (!is_valid_ship(target) || vessel_target_problem(target) != NULL ||
      vessel_range_between(ship, target) > (double)vessel_sight_range(ship))
  {
    ship->lock_target = 0;
    send_to_ship(ship, "The guns lose their lock.");
    return NULL;
  }
  return target;
}

/**
 * Lock the guns onto a contact named by ID or name prefix; the crew goes to
 * battle stations.
 *
 * @return The locked hull, or NULL after telling ch why not
 */
static struct greyhawk_ship_data *
vessel_lock_contact(struct char_data *ch, struct greyhawk_ship_data *ship, const char *arg)
{
  struct greyhawk_ship_data *target;
  const char *problem;
  int target_num;

  problem = vessel_hull_fire_problem(ship);
  if (problem != NULL)
  {
    send_to_char(ch, "%s\r\n", problem);
    return NULL;
  }
  target_num = vessel_find_contact(ship, arg);
  if (target_num < 0)
  {
    send_to_char(ch, "No contact in sight matches '%s'. See 'contacts'.\r\n", arg);
    return NULL;
  }
  target = &greyhawk_ships[target_num];
  problem = vessel_target_problem(target);
  if (problem != NULL)
  {
    send_to_char(ch, "%s\r\n", problem);
    return NULL;
  }
  if (ship->lock_target != target_num)
  {
    ship->lock_target = target_num;
    send_to_ship(ship, "The guns lock onto [%s] %s.", target->id, target->name);
  }
  vessel_battle_stations(ship);
  return target;
}

/**
 * A mental blast (study 3.3.4): the target's crew is stunned for 5 s at the
 * weapon's maximum range to 20 s at its minimum, and inside mid-range
 * everyone aboard makes a Will save or falls prone for two rounds.
 */
static void vessel_mental_blast(struct greyhawk_ship_data *ship, struct greyhawk_ship_data *target,
                                const struct vessel_weapon_type *type, double range)
{
  double closeness;

  closeness = fmax(0.0, fmin(1.0, ((double)type->max_range - range) /
                                      (double)(type->max_range - type->min_range)));
  target->stun_ticks = (short int)(2 * (5 + (int)(15.0 * closeness)));
  send_to_ship(ship, "You hit [%s] %s with a powerful mental blast!", target->id, target->name);
  send_to_ship(target, "A powerful mental wave hits the ship! The crew is completely disoriented.");
  if (range <= (double)(type->max_range + type->min_range) / 2.0)
  {
    vessel_knockdown_aboard(target, SAVING_WILL);
  }
}

/**
 * Fire one ready weapon at target: spend a round and the gun crew's stamina
 * (the weapon's weight over the hull effort), start the reload, put both
 * crews at battle stations, and resolve the shot against the geometry DC.
 *
 * @param ch The gunner, or NULL for the hull's NPC crew
 * @return damage dealt (0 on a miss)
 */
int vessel_fire_weapon(struct greyhawk_ship_data *ship, int slot, struct greyhawk_ship_data *target,
                       struct char_data *ch)
{
  struct greyhawk_ship_slot *weapon;
  const struct vessel_weapon_type *type;
  double range;
  int natural;
  int confirm;
  int bonus;
  int dealt;
  int dc;

  weapon = &ship->slot[slot];
  type = vessel_slot_weapon(weapon);
  range = vessel_range_between(ship, target);
  dc = vessel_gunnery_dc(ship, type, target);
  bonus = vessel_gunnery_bonus(ship, ch, type);

  weapon->ammo--;
  weapon->timer = vessel_reload_ticks(ship, type);
  ship->stamina_spent += (double)type->weight / vessel_hull_effort(ship);
  if (ship->lock_target == target->shipnum)
  {
    vessel_crew_gain(ship, CREW_GUNNER, 0.1);
  }
  target->last_attacker = ship->shipnum;
  vessel_battle_stations(ship);
  vessel_battle_stations(target);

  if (ch == NULL)
  {
    send_to_ship_throttled(ship, VESSEL_MESSAGE_COMBAT_RETURN_FIRE, VESSEL_COMBAT_MESSAGE_COOLDOWN,
                           "%s %s!",
                           ship->bounty_hunter ? "The navy crew OPENS FIRE on"
                           : ship->raider_mode ? "The raiders OPEN FIRE on"
                                               : "The crew RETURNS FIRE at",
                           target->name);
  }
  else
  {
    send_to_ship(ship, "The %s %s FIRES at %s! Chance to hit: %d%%",
                 vessel_arc_name(weapon->position), type->name, target->name,
                 vessel_hit_percent(dc, bonus));
  }
  if (weapon->ammo == 0)
  {
    send_to_ship(ship, "That was the last round for the %s %s.", vessel_arc_name(weapon->position),
                 type->name);
  }

  natural = ch != NULL ? d20(ch) : rand_number(1, 20);
  if (natural == 1 || (natural < 20 && natural + bonus < dc))
  {
    if (ch == NULL)
    {
      send_to_ship_throttled(target, VESSEL_MESSAGE_COMBAT_RETURN_FIRE_MISS,
                             VESSEL_COMBAT_MESSAGE_COOLDOWN, "%s from %s splashes wide!",
                             ship->bounty_hunter ? "Navy fire"
                             : ship->raider_mode ? "Raider fire"
                                                 : "Return fire",
                             ship->name);
    }
    else
    {
      send_to_ship(ship, "The shot goes wide, splashing harmlessly.");
      send_to_ship(target, "A projectile from %s splashes into the water nearby!", ship->name);
    }
    return 0;
  }

  if (ch != NULL)
  {
    send_to_ship(ship, "Direct hit on %s!", target->name);
  }
  confirm = ch != NULL ? d20(ch) : rand_number(1, 20);
  dealt =
      vessel_resolve_hit(ship, target, weapon, range,
                         natural >= vessel_critical_threat(type->pierce) && confirm + bonus >= dc);
  if (IS_SET(type->flags, VESSEL_WEAPON_CREW_STUN))
  {
    vessel_mental_blast(ship, target, type, range);
  }
  vessel_event_record_damage(ship->shipnum, target->shipnum, dealt);
  return dealt;
}

/**
 * Why a weapon cannot fire at target now, or NULL: it must be a sound,
 * loaded catalogue weapon whose arc faces the target inside its range band,
 * and reloaded unless check_reload is FALSE. The buffer holds the answer
 * until the next call.
 */
const char *vessel_weapon_fire_problem(struct greyhawk_ship_data *ship, int slot,
                                       struct greyhawk_ship_data *target, bool check_reload)
{
  static char problem[MAX_STRING_LENGTH];
  const struct greyhawk_ship_slot *weapon;
  const struct vessel_weapon_type *type;
  double range;

  weapon = &ship->slot[slot];
  type = vessel_slot_weapon(weapon);
  if (type == NULL)
  {
    snprintf(problem, sizeof(problem), "Slot %d holds no weapon.", slot);
  }
  else if (weapon->damage >= VESSEL_WEAPON_DESTROYED)
  {
    snprintf(problem, sizeof(problem), "The %s in slot %d has been destroyed.", type->name, slot);
  }
  else if (weapon->damage > 0)
  {
    snprintf(problem, sizeof(problem),
             "The %s in slot %d is damaged and cannot fire until it is repaired.", type->name,
             slot);
  }
  else if (weapon->ammo == 0)
  {
    snprintf(problem, sizeof(problem),
             "The %s in slot %d is out of ammunition; rearm it at a shipyard.", type->name, slot);
  }
  else if (check_reload && weapon->timer > 0)
  {
    snprintf(problem, sizeof(problem), "The %s in slot %d is still reloading (%d seconds).",
             type->name, slot, (weapon->timer + 1) / 2);
  }
  else if (weapon->position != vessel_arc_toward(ship, target))
  {
    snprintf(problem, sizeof(problem), "The %s %s cannot bear - %s lies off your %s arc.",
             vessel_arc_name(weapon->position), type->name, target->name,
             vessel_arc_name(vessel_arc_toward(ship, target)));
  }
  else
  {
    range = vessel_range_between(ship, target);
    if (range < (double)type->min_range || range > (double)type->max_range)
    {
      snprintf(problem, sizeof(problem), "%s is outside the %s's %d-%d room band (%.1f).",
               target->name, type->name, type->min_range, type->max_range, range);
    }
    else
    {
      return NULL;
    }
  }
  return problem;
}

/**
 * Auto-defense doctrine: an NPC-piloted ship returns fire at its last
 * attacker with every ready weapon that bears inside its band. A raider
 * marks her quarry as her attacker.
 */
static void vessel_npc_return_fire(struct greyhawk_ship_data *ship)
{
  struct greyhawk_ship_data *target;
  int target_num;
  int s;

  if (ship->autopilot == NULL || ship->autopilot->pilot_mob_vnum == -1 ||
      vessel_hull_fire_problem(ship) != NULL)
  {
    return; /* No NPC pilot - players fight their own battles */
  }

  target_num = ship->last_attacker;
  if (target_num <= 0 || target_num >= GREYHAWK_MAXSHIPS)
  {
    return;
  }
  target = &greyhawk_ships[target_num];
  if (!is_valid_ship(target))
  {
    ship->last_attacker = 0; /* Attacker sank or despawned */
    return;
  }
  if (vessel_target_problem(target) != NULL)
  {
    return;
  }

  for (s = 0; s < GREYHAWK_MAXSLOTS; s++)
  {
    if (vessel_weapon_fire_problem(ship, s, target, TRUE) != NULL)
    {
      continue;
    }
    vessel_fire_weapon(ship, s, target, NULL);
    VSSL_DEBUG("AI ship %d return-fired slot %d at ship %d", ship->shipnum, s, target_num);
    if (!is_valid_ship(target))
    {
      break; /* She went down */
    }
  }
}

/**
 * Count each weapon's reload down a tick, unless the crew is stunned, braced
 * to ram, or reeling from a ram (Duris). A reloading weapon tires the gun
 * crew by its weight over ten times the hull effort per Duris second, and
 * teaches the gunner while a contact is locked.
 */
void vessel_reload_tick(struct greyhawk_ship_data *ship)
{
  const struct vessel_weapon_type *type;
  int s;

  for (s = 0; s < GREYHAWK_MAXSLOTS && !vessel_crew_stunned(ship) && !ship->ramming &&
              ship->ram_gun_ticks == 0;
       s++)
  {
    if (ship->slot[s].timer <= 0)
    {
      continue;
    }
    ship->slot[s].timer--;
    type = vessel_slot_weapon(&ship->slot[s]);
    if (type != NULL)
    {
      ship->stamina_spent += (double)type->weight / vessel_hull_effort(ship) / 20.0;
    }
    if (ship->lock_target != 0)
    {
      vessel_crew_gain(ship, CREW_GUNNER, 0.0015);
    }
    if (ship->slot[s].timer == 0 && ship->slot[s].type == VESSEL_SLOT_WEAPON)
    {
      send_to_ship_throttled(ship, VESSEL_MESSAGE_COMBAT_RELOAD, VESSEL_COMBAT_MESSAGE_COOLDOWN,
                             "The %s %s is reloaded and ready.",
                             vessel_arc_name(ship->slot[s].position),
                             vessel_slot_name(&ship->slot[s]));
    }
  }
}

/**
 * Gunnery tick: recover from a mental blast, reload, keep a lock only on a
 * contact the guns may still engage, hold battle stations while locked and
 * stand down 180 s after, and run NPC return fire.
 */
void vessel_gunnery_tick_one(struct greyhawk_ship_data *ship)
{
  if (!is_valid_ship(ship))
  {
    return;
  }

  if (ship->stun_ticks > 0)
  {
    ship->stun_ticks--;
    if (ship->stun_ticks == 0)
    {
      send_to_ship(ship, "The crew recovers from the mental shock.");
    }
  }
  vessel_reload_tick(ship);

  if (ship->battle_ticks > 0)
  {
    ship->battle_ticks--;
    if (ship->battle_ticks == 0)
    {
      send_to_ship(ship, "The crew stands down from battle stations.");
    }
  }
  if (vessel_locked_target(ship) != NULL)
  {
    ship->battle_ticks = VESSEL_BATTLE_STATIONS_TICKS;
  }

  vessel_npc_return_fire(ship);
}

/**
 * shiplock [<contact> | off] - lock the guns onto a contact, putting the
 * crew at battle stations, or clear the lock.
 */
ACMD(do_shiplock)
{
  struct greyhawk_ship_data *ship;
  struct greyhawk_ship_data *target;
  char arg[MAX_INPUT_LENGTH];

  ship = get_ship_from_room(IN_ROOM(ch));
  if (!is_valid_ship(ship))
  {
    send_to_char(ch, "You must be aboard a vessel to lock her guns.\r\n");
    return;
  }
  if (!vessel_gunnery_permitted(ch, ship))
  {
    send_to_char(ch,
                 "%s's guns answer to her owner, the helm permit holders, and the owner's "
                 "group.\r\n",
                 ship->name);
    return;
  }

  one_argument(argument, arg, sizeof(arg));
  if (!*arg)
  {
    target = vessel_locked_target(ship);
    if (target == NULL)
    {
      send_to_char(ch, "The guns are not locked on anything. Usage: shiplock <contact> | off\r\n");
    }
    else
    {
      send_to_char(ch, "The guns are locked on [%s] %s, %.1f rooms off.\r\n", target->id,
                   target->name, vessel_range_between(ship, target));
    }
    return;
  }
  if (!str_cmp(arg, "off"))
  {
    if (ship->lock_target == 0)
    {
      send_to_char(ch, "The guns are not locked on anything.\r\n");
      return;
    }
    ship->lock_target = 0;
    send_to_ship(ship, "The guns come off their target; the crew stands down in %d seconds.",
                 (ship->battle_ticks + 1) / 2);
    return;
  }
  vessel_lock_contact(ch, ship, arg);
}

/**
 * shipfire <slot | fore | port | rear | starboard> [<contact>] - fire one
 * weapon, or every weapon on an arc that can, at the locked contact; naming
 * a contact locks onto it first.
 */
ACMD(do_shipfire)
{
  struct greyhawk_ship_data *ship;
  struct greyhawk_ship_data *target;
  const char *problem;
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  int slot;
  int arc;
  int s;

  ship = get_ship_from_room(IN_ROOM(ch));
  if (ship == NULL)
  {
    send_to_char(ch, "You must be aboard a ship to fire its weapons.\r\n");
    return;
  }
  if (!vessel_gunnery_permitted(ch, ship))
  {
    send_to_char(ch,
                 "%s's guns answer to her owner, the helm permit holders, and the owner's "
                 "group.\r\n",
                 ship->name);
    return;
  }

  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
  slot = -1;
  arc = -1;
  if (isdigit((unsigned char)*arg1))
  {
    slot = parse_int(arg1);
  }
  else
  {
    arc = vessel_arc_by_name(arg1);
  }
  if (!*arg1 || (arc < 0 && (slot < 0 || slot >= GREYHAWK_MAXSLOTS)))
  {
    send_to_char(ch, "Usage: shipfire <slot 0-%d | fore | port | rear | starboard> [contact]\r\n",
                 GREYHAWK_MAXSLOTS - 1);
    return;
  }

  problem = vessel_hull_fire_problem(ship);
  if (problem != NULL)
  {
    send_to_char(ch, "%s\r\n", problem);
    return;
  }
  if (*arg2)
  {
    target = vessel_lock_contact(ch, ship, arg2);
  }
  else
  {
    target = vessel_locked_target(ship);
    if (target == NULL)
    {
      send_to_char(ch, "The guns are not locked on anything: 'shiplock <contact>', or name one "
                       "after the weapon.\r\n");
    }
  }
  if (target == NULL)
  {
    return;
  }

  /* Check the shot before the consent gate, which records an engagement. */
  if (slot >= 0)
  {
    problem = vessel_weapon_fire_problem(ship, slot, target, TRUE);
    if (problem != NULL)
    {
      send_to_char(ch, "%s\r\n", problem);
      return;
    }
  }
  else
  {
    for (s = 0; s < GREYHAWK_MAXSLOTS; s++)
    {
      if (ship->slot[s].position == arc &&
          vessel_weapon_fire_problem(ship, s, target, TRUE) == NULL)
      {
        break;
      }
    }
    if (s == GREYHAWK_MAXSLOTS)
    {
      send_to_char(ch, "No weapon on the %s arc can fire at %s now. See 'shipsight'.\r\n",
                   vessel_arc_name(arc), target->name);
      return;
    }
  }
  if (!vessel_fire_permitted(ch, ship, target, TRUE))
  {
    return;
  }

  vessel_merchant_note_attacker(ch, target);
  WAIT_STATE(ch, PULSE_VIOLENCE);
  if (slot >= 0)
  {
    vessel_fire_weapon(ship, slot, target, ch);
    return;
  }
  for (s = 0; s < GREYHAWK_MAXSLOTS && is_valid_ship(target); s++)
  {
    if (ship->slot[s].position == arc && vessel_weapon_fire_problem(ship, s, target, TRUE) == NULL)
    {
      vessel_fire_weapon(ship, s, target, ch);
    }
  }
}

/**
 * shipsight [<slot>] - each weapon's shot at the locked contact: whether it
 * bears and is in its band, and the DC and chance to hit.
 */
ACMD(do_shipsight)
{
  struct greyhawk_ship_data *ship;
  struct greyhawk_ship_data *target;
  const struct vessel_weapon_type *type;
  const char *problem;
  char arg[MAX_INPUT_LENGTH];
  int only;
  int dc;
  int s;

  ship = get_ship_from_room(IN_ROOM(ch));
  if (!is_valid_ship(ship))
  {
    send_to_char(ch, "You must be aboard a vessel to sight her guns.\r\n");
    return;
  }
  target = vessel_locked_target(ship);
  if (target == NULL)
  {
    send_to_char(ch, "The guns are not locked on anything: 'shiplock <contact>' first.\r\n");
    return;
  }

  one_argument(argument, arg, sizeof(arg));
  only = *arg ? parse_int(arg) : -1;
  if (*arg && (!isdigit((unsigned char)*arg) || only < 0 || only >= GREYHAWK_MAXSLOTS))
  {
    send_to_char(ch, "Usage: shipsight [slot 0-%d]\r\n", GREYHAWK_MAXSLOTS - 1);
    return;
  }

  send_to_char(ch, "Sighting [%s] %s, %.1f rooms off your %s arc:\r\n", target->id, target->name,
               vessel_range_between(ship, target),
               vessel_arc_name(vessel_arc_toward(ship, target)));
  for (s = 0; s < GREYHAWK_MAXSLOTS; s++)
  {
    type = vessel_slot_weapon(&ship->slot[s]);
    if ((only >= 0 && s != only) || type == NULL)
    {
      continue;
    }
    problem = vessel_weapon_fire_problem(ship, s, target, FALSE);
    if (problem != NULL)
    {
      send_to_char(ch, "  %s\r\n", problem);
      continue;
    }
    dc = vessel_gunnery_dc(ship, type, target);
    send_to_char(ch, "  Slot %d, %s %s: DC %d, %d%% to hit%s\r\n", s,
                 vessel_arc_name(ship->slot[s].position), type->name, dc,
                 vessel_hit_percent(dc, vessel_gunnery_bonus(ship, ch, type)),
                 ship->slot[s].timer > 0 ? " (reloading)" : "");
  }
}

/**
 * shipscan <contact> - a close look at a hull within 20 rooms (22 with a
 * posted lookout): her armor and structure by side, weapons, condition, and
 * her owner's standing with the law.
 */
ACMD(do_shipscan)
{
  static const int display_arc[VESSEL_NUM_ARCS] = {GREYHAWK_FORE, GREYHAWK_PORT, GREYHAWK_STARBOARD,
                                                   GREYHAWK_REAR};
  struct greyhawk_ship_data *ship;
  struct greyhawk_ship_data *target;
  const struct vessel_weapon_type *type;
  char arg[MAX_INPUT_LENGTH];
  int target_num;
  int bounty;
  int arc;
  int i;
  bool armed;

  ship = get_ship_from_room(IN_ROOM(ch));
  if (!is_valid_ship(ship))
  {
    send_to_char(ch, "You must be aboard a vessel to scan another.\r\n");
    return;
  }
  one_argument(argument, arg, sizeof(arg));
  if (!*arg)
  {
    send_to_char(ch, "Usage: shipscan <contact ID or name>\r\n");
    return;
  }
  target_num = vessel_find_contact(ship, arg);
  if (target_num < 0)
  {
    send_to_char(ch, "No contact in sight matches '%s'. See 'contacts'.\r\n", arg);
    return;
  }
  target = &greyhawk_ships[target_num];
  if (vessel_range_between(ship, target) >
      (double)(VESSEL_SCAN_RANGE + (vessel_lookout_bonus(ship) > 0 ? 2 : 0)))
  {
    send_to_char(ch, "%s is too far off to make out; close within %d rooms.\r\n", target->name,
                 VESSEL_SCAN_RANGE + (vessel_lookout_bonus(ship) > 0 ? 2 : 0));
    return;
  }

  send_to_char(ch, "[%s] %s, a %s, heading %d at speed %d, %.1f rooms off.\r\n", target->id,
               target->name, get_vessel_type_name(target->vessel_type),
               vessel_display_heading(target->heading), vessel_display_speed(target->speed),
               vessel_range_between(ship, target));
  send_to_char(ch, "Armor/structure:");
  for (i = 0; i < VESSEL_NUM_ARCS; i++)
  {
    arc = display_arc[i];
    send_to_char(ch, "%s %s %d/%d %d/%d", i > 0 ? "," : "", vessel_arc_name(arc),
                 *vessel_arc_armor(target, arc), *vessel_arc_max_armor(target, arc),
                 *vessel_arc_internal(target, arc), *vessel_arc_max_internal(target, arc));
  }
  send_to_char(ch, "\r\nCondition: %s", vessel_status_name(vessel_status(target)));
  if (vessel_breached_arcs(target) > 0)
  {
    send_to_char(ch, ", holed on %d side%s", vessel_breached_arcs(target),
                 vessel_breached_arcs(target) == 1 ? "" : "s");
  }
  if (vessel_colors_struck(target))
  {
    send_to_char(ch, ", colors struck");
  }
  send_to_char(ch, "\r\nWeapons:");
  armed = FALSE;
  for (i = 0; i < GREYHAWK_MAXSLOTS; i++)
  {
    type = vessel_slot_weapon(&target->slot[i]);
    if (type == NULL)
    {
      continue;
    }
    send_to_char(ch, "%s %s %s%s", armed ? "," : "", vessel_arc_name(target->slot[i].position),
                 type->name,
                 target->slot[i].damage >= VESSEL_WEAPON_DESTROYED ? " (destroyed)" : "");
    armed = TRUE;
  }
  send_to_char(ch, "%s\r\n", armed ? "" : " none");

  if (target->owner[0] != '\0')
  {
    bounty = vessel_get_bounty(target->owner);
    send_to_char(
        ch, "Owner: %s%s%s.\r\n", target->owner,
        bounty >= BOUNTY_HUNTED ? ", HUNTED by the navy"
                                : (bounty >= BOUNTY_WANTED ? ", WANTED in lawful ports" : ""),
        vessel_has_letter_of_marque(target->owner) ? ", sailing under a letter of marque" : "");
  }
}
