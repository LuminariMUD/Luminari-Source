/* ************************************************************************
 *      File:   vessels_ramming.c                     Part of LuminariMUD  *
 *   Purpose:   Ramming (vessels-ships study S6, 1.6): the shipram order,  *
 *              DurisMUD's impact, and its cooldowns                       *
 * ********************************************************************** */

/*
 * A braced hull rams her locked contact once it comes within a room. The
 * ram connects inside a 120-degree bow cone at the same altitude, with a
 * chance set by the hulls' weights and by how fast each sails against her
 * class speed. Both hulls take crash damage in 2-6 point hits; a fitted ram
 * strikes first and halves the crash damage its bow takes. The heavier hull
 * spins the lighter one about, both crews are knocked down, and the rammer
 * waits out a cooldown with her guns locked. Speeds are Duris's times 0.3
 * and cooldowns are in 0.5 s ticks (Duris counts seconds).
 */

#include "conf.h"
#include "core/sysdep.h"
#include <math.h>
#include "core/structs.h"
#include "core/utils.h"
#include "core/comm.h"
#include "core/interpreter.h"
#include "magic/spells.h"
#include "vessels.h"

/**
 * The percent chance a ram connects (Duris try_ram_ship()): a heavier
 * rammer and faster hulls make it harder, a stopped target ten times
 * easier, and the sailmaster helps. The rammer must be under way.
 */
int vessel_ram_chance(const struct greyhawk_ship_data *ship,
                      const struct greyhawk_ship_data *target)
{
  const struct vessel_class_handling *own;
  const struct vessel_class_handling *theirs;
  double speed_mod;
  double hull_mod;
  double sail_mod;
  double chance;

  own = vessel_class_handling(ship->vessel_type);
  theirs = vessel_class_handling(target->vessel_type);
  speed_mod = (ship->speed / own->speed + 3.0 * target->speed / theirs->speed) / 4.0;
  hull_mod = (double)own->hull_weight / theirs->hull_weight;
  if (target->speed <= 0.0)
  {
    hull_mod /= 10.0;
  }
  if (hull_mod >= 1.0)
  {
    chance = 75.0 / (hull_mod * speed_mod);
  }
  else
  {
    chance = 100.0 - 25.0 * hull_mod * speed_mod;
  }
  sail_mod = 0.1 * ship->crew_tier[CREW_SAILMASTER];
  chance = 100.0 - (100.0 - chance) / (1.0 + sail_mod * chance / 50.0);
  return (int)fmax(0.0, fmin(100.0, chance));
}

/**
 * One crash hit of 2-6 points (at most `left`) on hull, struck from
 * `from_bearing` (degrees, relative to her heading) give or take 90: one hit
 * in ten on the sails when the other hull is at least as heavy, and half on
 * a bow that carries a ram. Returns the points spent.
 */
static int vessel_ram_crash_hit(struct greyhawk_ship_data *hull, int from_bearing, int left,
                                bool other_heavier)
{
  int damage;
  int arc;

  damage = MIN(rand_number(2, 6), left);
  if (other_heavier && rand_number(0, 99) < 10)
  {
    vessel_damage_sail(NULL, hull, damage);
    return damage;
  }
  arc = vessel_arc_for_relative_bearing(from_bearing + rand_number(-90, 90));
  vessel_damage_hull(NULL, hull,
                     arc == GREYHAWK_FORE && vessel_equipment_slot(hull, VESSEL_EQUIPMENT_RAM) >= 0
                         ? MAX(1, damage / 2)
                         : damage,
                     arc, FALSE);
  return damage;
}

/**
 * Ram target (Duris try_ram_ship()). Refused outside the bow cone, across
 * altitudes, at speed 3 or less, or when the target outruns the rammer; a
 * refusal leaves the cooldowns alone. Otherwise the roll decides, and a hit
 * damages both hulls, slows or spins them, and knocks both crews down.
 *
 * @return TRUE when the ram connected
 */
bool vessel_ram(struct greyhawk_ship_data *ship, struct greyhawk_ship_data *target)
{
  const struct vessel_class_handling *own;
  const struct vessel_class_handling *theirs;
  double crew_mod;
  double angle;
  double ram_speed;
  double counter_speed;
  int ram_damage;
  int counter_damage;
  int split;
  int from_ship;
  int chance;
  int weight;
  int sarc;
  int tarc;
  int ram;

  ship->ramming = FALSE;
  if (fabs(vessel_heading_difference(ship->heading, vessel_bearing_between(ship, target))) >
      VESSEL_RAM_CONE / 2.0)
  {
    send_to_ship(ship, "[%s] %s is not before your bow to ram!", target->id, target->name);
    return FALSE;
  }
  if ((int)ship->z != (int)target->z)
  {
    send_to_ship(ship, "You try to ram but pass %s [%s] %s instead!",
                 ship->z > target->z ? "above" : "below", target->id, target->name);
    return FALSE;
  }
  if (vessel_display_speed(ship->speed) <= VESSEL_BOARDING_MAX_SPEED)
  {
    send_to_ship(ship, "She is too slow to ram!");
    return FALSE;
  }
  vessel_crew_gain(ship, CREW_SAILMASTER, rand_number(1, 3));

  /* A target sailing away takes her speed off the blow. */
  angle = fabs(vessel_heading_difference(ship->heading, target->heading)) * M_PI / 180.0;
  ram_speed = ship->speed;
  counter_speed = -target->speed * cos(angle);
  if (counter_speed < 0.0)
  {
    ram_speed += counter_speed;
    counter_speed = 0.0;
    if (ram_speed <= 0.0)
    {
      send_to_ship(ship, "[%s] %s speeds away from your attempt to ram her!", target->id,
                   target->name);
      return FALSE;
    }
  }

  chance = vessel_ram_chance(ship, target);
  send_to_ship(ship, "You attempt to ram [%s] %s! Chance to hit: %d%%", target->id, target->name,
               chance);
  send_to_ship(target, "[%s] %s attempts to ram you!", ship->id, ship->name);
  target->last_attacker = ship->shipnum;
  vessel_battle_stations(ship);
  vessel_battle_stations(target);
  crew_mod = (0.1 * ship->crew_tier[CREW_SAILMASTER] + 0.15 * ship->crew_tier[CREW_GUNNER] +
              vessel_bosun_mod(ship)) /
             3.0;
  if (rand_number(0, 99) >= chance)
  {
    send_to_ship(ship, "You miss [%s] %s!", target->id, target->name);
    send_to_ship(target, "[%s] %s misses you!", ship->id, ship->name);
    ship->ram_ticks = (short int)MAX(1, (int)(VESSEL_RAM_MISS_TICKS * (1.0 - crew_mod * 0.15)));
    return FALSE;
  }

  send_to_ship(ship, "Timbers crunch and crack as you crash into [%s] %s!", target->id,
               target->name);
  send_to_ship(target, "Timbers crunch and crack as [%s] %s crashes into your ship!", ship->id,
               ship->name);

  own = vessel_class_handling(ship->vessel_type);
  theirs = vessel_class_handling(target->vessel_type);
  sarc = vessel_arc_toward(ship, target);
  tarc = vessel_arc_toward(target, ship);
  ram_damage = (own->hull_weight + 100) / 10;
  ram_damage =
      (int)(ram_damage * (0.1 + 0.4 * ship->speed / own->speed + 0.5 * ram_speed / own->speed) *
            (sarc == GREYHAWK_FORE ? 1.2 : 1.0));
  counter_damage = (theirs->hull_weight + 100) / 10;
  counter_damage =
      (int)(counter_damage *
            (0.1 + 0.3 * ship->speed / own->speed + 0.4 * counter_speed / theirs->speed) *
            (tarc == GREYHAWK_FORE ? 1.2 : 1.0)) +
      theirs->hull_weight / own->hull_weight;

  /* A fitted ram strikes first with its own weight. */
  ram = vessel_equipment_slot(ship, VESSEL_EQUIPMENT_RAM);
  if (ram >= 0 && sarc == GREYHAWK_FORE)
  {
    weight = vessel_slot_weight(ship, &ship->slot[ram]);
    vessel_damage_hull(ship, target, rand_number(weight * 8 / 10, weight * 12 / 10), tarc, FALSE);
  }
  ram = vessel_equipment_slot(target, VESSEL_EQUIPMENT_RAM);
  if (ram >= 0 && tarc == GREYHAWK_FORE)
  {
    weight = vessel_slot_weight(target, &target->slot[ram]);
    vessel_damage_hull(NULL, ship, rand_number(weight * 6 / 10, weight), sarc, FALSE);
  }

  /* The crash, in 2-6 point hits shared in proportion. */
  split = 100 * ram_damage / MAX(1, ram_damage + counter_damage);
  from_ship = (int)lround(vessel_bearing_between(target, ship) - target->heading);
  while (ram_damage > 0 || counter_damage > 0)
  {
    if (counter_damage == 0 || (ram_damage > 0 && rand_number(0, 99) < split))
    {
      ram_damage -= vessel_ram_crash_hit(target, from_ship, ram_damage,
                                         own->hull_weight >= theirs->hull_weight);
    }
    else
    {
      counter_damage -=
          vessel_ram_crash_hit(ship, 0, counter_damage, theirs->hull_weight >= own->hull_weight);
    }
  }

  /* The heavier hull slews the lighter about and both slow to a crawl; a
   * lighter rammer is stopped dead. */
  if (own->hull_weight >= theirs->hull_weight)
  {
    target->heading = rand_number(0, 359);
    target->setheading = (short int)rand_number(0, 359);
    target->speed = fmin(target->speed, VESSEL_BOARDING_MAX_SPEED);
    ship->speed = fmin(ship->speed, VESSEL_BOARDING_MAX_SPEED);
  }
  else
  {
    ship->setspeed = 0;
  }
  vessel_knockdown_aboard(target, SAVING_REFL);
  vessel_knockdown_aboard(ship, SAVING_REFL);
  vessel_update_condition(target, ship);
  vessel_update_condition(ship, target);

  ship->ram_ticks = (short int)MAX(1, (int)(VESSEL_RAM_HIT_TICKS * (1.0 - crew_mod * 0.15)));
  ship->ram_gun_ticks = (short int)MAX(
      1, (int)(VESSEL_RAM_GUN_TICKS * (1.0 - 0.15 * ship->crew_tier[CREW_GUNNER] * 0.15)));
  return TRUE;
}

/**
 * Ram tick: count the cooldowns down, and ram the locked contact once a
 * braced hull comes within a room of her. The crew stands down when the
 * lock is lost or the hull slows to speed 3.
 */
void vessel_ram_tick_one(struct greyhawk_ship_data *ship)
{
  struct greyhawk_ship_data *target;

  if (ship->ram_ticks > 0)
  {
    ship->ram_ticks--;
  }
  if (ship->ram_gun_ticks > 0)
  {
    ship->ram_gun_ticks--;
    if (ship->ram_gun_ticks == 0)
    {
      send_to_ship(ship, "The gun crews have recovered from the ram.");
    }
  }
  if (!ship->ramming)
  {
    return;
  }

  target = vessel_locked_target(ship);
  if (target == NULL)
  {
    ship->ramming = FALSE;
    send_to_ship(ship, "The crew stands down from ramming: there is no target.");
  }
  else if (vessel_display_speed(ship->speed) <= VESSEL_BOARDING_MAX_SPEED)
  {
    ship->ramming = FALSE;
    send_to_ship(ship, "The crew stands down from ramming: she has lost way.");
  }
  else if (!vessel_crew_stunned(ship) && vessel_range_between(ship, target) < 1.0)
  {
    vessel_ram(ship, target);
  }
}

/**
 * shipram [off] - brace the crew to ram the locked contact at speed 6 or
 * more; she rams when the contact comes within a room.
 */
ACMD(do_shipram)
{
  struct greyhawk_ship_data *ship;
  struct greyhawk_ship_data *target;
  const char *problem;
  char arg[MAX_INPUT_LENGTH];

  ship = get_ship_from_room(IN_ROOM(ch));
  if (!is_valid_ship(ship))
  {
    send_to_char(ch, "You must be aboard a vessel to ram.\r\n");
    return;
  }
  if (!vessel_gunnery_permitted(ch, ship))
  {
    send_to_char(ch,
                 "%s's crew answers to her owner, the helm permit holders, and the owner's "
                 "group.\r\n",
                 ship->name);
    return;
  }

  one_argument(argument, arg, sizeof(arg));
  if (!str_cmp(arg, "off"))
  {
    if (!ship->ramming)
    {
      send_to_char(ch, "The crew is not braced to ram.\r\n");
      return;
    }
    ship->ramming = FALSE;
    send_to_ship(ship, "The crew returns to battle stations.");
    return;
  }
  if (*arg)
  {
    send_to_char(ch, "Usage: shipram [off]\r\n");
    return;
  }

  if (ship->ramming)
  {
    send_to_char(ch, "The crew is already braced to ram.\r\n");
    return;
  }
  if (ship->ram_ticks > 0)
  {
    send_to_char(ch, "She is not ready to ram again (%d seconds).\r\n", (ship->ram_ticks + 1) / 2);
    return;
  }
  problem = vessel_hull_fire_problem(ship);
  if (problem != NULL)
  {
    send_to_char(ch, "%s\r\n", problem);
    return;
  }
  target = vessel_locked_target(ship);
  if (target == NULL)
  {
    send_to_char(ch, "Lock onto a contact to ram first: shiplock <contact>.\r\n");
    return;
  }
  if (vessel_display_speed(ship->speed) < VESSEL_RAM_MIN_SPEED)
  {
    send_to_char(ch, "She is too slow to ram; she needs speed %d.\r\n", VESSEL_RAM_MIN_SPEED);
    return;
  }
  if (!vessel_fire_permitted(ch, ship, target, TRUE))
  {
    return;
  }
  ship->ramming = TRUE;
  send_to_ship(ship, "The crew braces to ram [%s] %s!", target->id, target->name);
}
