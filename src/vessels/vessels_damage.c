/* ************************************************************************
 *      File:   vessels_damage.c                      Part of LuminariMUD  *
 *   Purpose:   Vessel damage model: class condition profiles, arcs,      *
 *              hull and sail damage, breaches, sinking, and salvage      *
 *              (vessels-ships study S3, 3.3.1 and 3.3.3)                 *
 * ********************************************************************** */

/*
 * Each class takes its DurisMUD analog's per-arc armor and internal
 * structure, stated at the class beam armor; a prototype's armor (its beam
 * armor) scales all eight numbers in proportion. The beams are the big
 * targets and the stern is the weak spot.
 *
 * Arcs are relative to the heading, as in DurisMUD: fore 320-40 degrees,
 * starboard 40-140, rear 140-220, port 220-320.
 */

#include "conf.h"
#include "core/sysdep.h"
#include <math.h>
#include "core/structs.h"
#include "core/utils.h"
#include "core/comm.h"
#include "core/db.h"
#include "core/handler.h"
#include "magic/spells.h"
#include "movement/movement_position.h"
#include "vessels.h"

/* Class order follows enum vessel_class; arcs follow GREYHAWK_FORE (0),
 * GREYHAWK_PORT (1), GREYHAWK_REAR (2), GREYHAWK_STARBOARD (3). */
static const struct vessel_class_condition class_condition[NUM_VESSEL_TYPES] = {
    /* beam  armor F/P/R/S         internal F/P/R/S   sail  price */
    {3, {2, 3, 1, 3}, {1, 1, 1, 1}, 20, 200},                 /* RAFT: Duris sloop */
    {8, {6, 8, 4, 8}, {3, 4, 2, 4}, 40, 600},                 /* BOAT: yacht */
    {66, {53, 66, 33, 66}, {26, 33, 16, 33}, 110, 8000},      /* SHIP: caravel */
    {109, {87, 109, 65, 109}, {38, 47, 23, 47}, 140, 44000},  /* WARSHIP: frigate */
    {63, {50, 63, 37, 63}, {22, 27, 13, 27}, 120, 72000},     /* AIRSHIP: corvette */
    {84, {67, 84, 50, 84}, {29, 36, 18, 36}, 130, 60000},     /* SUBMARINE: destroyer */
    {110, {88, 110, 55, 110}, {44, 55, 27, 55}, 130, 24000},  /* TRANSPORT: galleon */
    {153, {122, 153, 91, 153}, {53, 66, 33, 66}, 160, 144000} /* MAGICAL: cruiser */
};

const struct vessel_class_condition *vessel_class_condition(enum vessel_class vessel_type)
{
  if (vessel_type < 0 || vessel_type >= NUM_VESSEL_TYPES)
  {
    vessel_type = VESSEL_SHIP;
  }
  return &class_condition[vessel_type];
}

/** A class profile value scaled from the class beam armor to `armor`. */
static int vessel_scale_profile(int value, int armor, int beam)
{
  if (beam <= 0)
  {
    return value;
  }
  return MIN(255, (value * armor + beam / 2) / beam);
}

/**
 * Set a hull's condition from its class profile at the prototype's armor:
 * per-arc armor and internal structure (at least 1 per arc), the class
 * sail, and a whole rudder.
 */
void vessel_initialize_condition(struct greyhawk_ship_data *ship, int armor)
{
  const struct vessel_class_condition *profile;
  int arc;

  if (ship == NULL)
  {
    return;
  }

  profile = vessel_class_condition(ship->vessel_type);
  armor = MAX(0, MIN(VESSEL_MAX_PROTOTYPE_ARMOR, armor));
  for (arc = 0; arc < VESSEL_NUM_ARCS; arc++)
  {
    *vessel_arc_max_armor(ship, arc) = *vessel_arc_armor(ship, arc) =
        (unsigned char)vessel_scale_profile(profile->armor[arc], armor, profile->beam_armor);
    *vessel_arc_max_internal(ship, arc) = *vessel_arc_internal(ship, arc) = (unsigned char)MAX(
        1, vessel_scale_profile(profile->internal[arc], armor, profile->beam_armor));
  }
  ship->maxmainsail = ship->mainsail = (unsigned char)profile->sail;
  ship->maxturnrate = ship->turnrate = VESSEL_RUDDER_MAX;
}

/* Arc field accessors, indexed GREYHAWK_FORE..GREYHAWK_STARBOARD. */
unsigned char *vessel_arc_armor(struct greyhawk_ship_data *ship, int arc)
{
  switch (arc)
  {
  case GREYHAWK_PORT:
    return &ship->parmor;
  case GREYHAWK_REAR:
    return &ship->rarmor;
  case GREYHAWK_STARBOARD:
    return &ship->sarmor;
  default:
    return &ship->farmor;
  }
}

unsigned char *vessel_arc_max_armor(struct greyhawk_ship_data *ship, int arc)
{
  switch (arc)
  {
  case GREYHAWK_PORT:
    return &ship->maxparmor;
  case GREYHAWK_REAR:
    return &ship->maxrarmor;
  case GREYHAWK_STARBOARD:
    return &ship->maxsarmor;
  default:
    return &ship->maxfarmor;
  }
}

unsigned char *vessel_arc_internal(struct greyhawk_ship_data *ship, int arc)
{
  switch (arc)
  {
  case GREYHAWK_PORT:
    return &ship->pinternal;
  case GREYHAWK_REAR:
    return &ship->rinternal;
  case GREYHAWK_STARBOARD:
    return &ship->sinternal;
  default:
    return &ship->finternal;
  }
}

unsigned char *vessel_arc_max_internal(struct greyhawk_ship_data *ship, int arc)
{
  switch (arc)
  {
  case GREYHAWK_PORT:
    return &ship->maxpinternal;
  case GREYHAWK_REAR:
    return &ship->maxrinternal;
  case GREYHAWK_STARBOARD:
    return &ship->maxsinternal;
  default:
    return &ship->maxfinternal;
  }
}

/** The arc a relative bearing (degrees off the bow) falls in. */
int vessel_arc_for_relative_bearing(int relative)
{
  relative %= 360;
  if (relative < 0)
  {
    relative += 360;
  }

  if (relative >= 320 || relative < 40)
  {
    return GREYHAWK_FORE;
  }
  if (relative < 140)
  {
    return GREYHAWK_STARBOARD;
  }
  if (relative < 220)
  {
    return GREYHAWK_REAR;
  }
  return GREYHAWK_PORT;
}

/* The Duris ballista family (1.3): one bolt, a 10-degree scatter, a 14% sail
 * hit at half damage, and 10% armor pierce. */
static const struct vessel_weapon_profile ballista_profile = {1, 10, 14, 100, 50, 10};

const struct vessel_weapon_profile *vessel_weapon_profile(const struct greyhawk_ship_slot *slot)
{
  (void)slot;
  return &ballista_profile;
}

/**
 * Lowest natural d20 that threatens a critical for a weapon's armor pierce:
 * 20 for 2-3%, 19-20 for 10%, 18-20 for 15%, never for 0% (study 3.3.4).
 */
int vessel_critical_threat(int pierce)
{
  if (pierce <= 0)
  {
    return 21;
  }
  if (pierce <= 3)
  {
    return 20;
  }
  if (pierce <= 10)
  {
    return 19;
  }
  return 18;
}

/** A mounted weapon that is neither damaged nor reloading. */
bool vessel_weapon_ready(const struct greyhawk_ship_slot *slot)
{
  return slot != NULL && slot->type == 1 && slot->damage == 0 && slot->timer <= 0;
}

static const char *vessel_arc_side_name(int arc)
{
  switch (arc)
  {
  case GREYHAWK_PORT:
    return "port side";
  case GREYHAWK_REAR:
    return "stern";
  case GREYHAWK_STARBOARD:
    return "starboard side";
  default:
    return "bow";
  }
}

/**
 * Put one fragment's sail damage on target's sails. A warship's rigging takes
 * 85% (Duris warship.sails.damage.reduction); at least 1 point lands.
 *
 * @return damage dealt
 */
int vessel_damage_sail(struct greyhawk_ship_data *attacker, struct greyhawk_ship_data *target,
                       int damage)
{
  if (target == NULL)
  {
    return 0;
  }
  if (target->vessel_type == VESSEL_WARSHIP)
  {
    damage = damage * 85 / 100;
  }
  damage = MAX(1, damage);

  if (attacker != NULL)
  {
    send_to_ship(attacker, "You hit [%s] %s for %d point%s on the sails!", target->id, target->name,
                 damage, damage == 1 ? "" : "s");
  }
  send_to_ship(target, "The sails are hit for %d point%s!", damage, damage == 1 ? "" : "s");

  target->mainsail = (unsigned char)MAX(0, (int)target->mainsail - damage);
  if (target->mainsail == 0)
  {
    send_to_ship(target, "The rigging collapses! The ship is dead in the water.");
  }
  return damage;
}

/**
 * Damage one of the struck arc's surviving weapons, chosen at random.
 * Damage accumulates: 1 or more disables the weapon, VESSEL_WEAPON_DESTROYED
 * destroys it.
 */
void vessel_damage_weapon(struct greyhawk_ship_data *attacker, struct greyhawk_ship_data *target,
                          int arc, int damage)
{
  struct greyhawk_ship_slot *weapon;
  int candidates[GREYHAWK_MAXSLOTS];
  int count;
  int i;

  if (target == NULL || damage <= 0)
  {
    return;
  }

  count = 0;
  for (i = 0; i < GREYHAWK_MAXSLOTS; i++)
  {
    if (target->slot[i].type == 1 && target->slot[i].position == arc &&
        target->slot[i].damage < VESSEL_WEAPON_DESTROYED)
    {
      candidates[count++] = i;
    }
  }
  if (count == 0)
  {
    return;
  }

  weapon = &target->slot[candidates[rand_number(0, count - 1)]];
  weapon->damage = (unsigned char)MIN(VESSEL_WEAPON_DESTROYED, weapon->damage + damage);
  if (weapon->damage >= VESSEL_WEAPON_DESTROYED)
  {
    send_to_ship(target, "%s has been destroyed!", weapon->desc[0] ? weapon->desc : "A weapon");
    if (attacker != NULL)
    {
      send_to_ship(attacker, "You destroy %s aboard %s!",
                   weapon->desc[0] ? weapon->desc : "a weapon", target->name);
    }
  }
  else
  {
    send_to_ship(target, "%s has been damaged!", weapon->desc[0] ? weapon->desc : "A weapon");
    if (attacker != NULL)
    {
      send_to_ship(attacker, "You damage %s aboard %s!",
                   weapon->desc[0] ? weapon->desc : "a weapon", target->name);
    }
  }
}

/**
 * A hull hit knocks everyone aboard off their feet unless they make a Reflex
 * save (DC 15); the fallen are prone for two combat rounds. Staff are exempt.
 */
void vessel_knockdown_aboard(struct greyhawk_ship_data *ship)
{
  struct char_data *ch;
  room_rnum room;
  int i;

  if (ship == NULL)
  {
    return;
  }

  send_to_ship(ship, "The blast shakes the whole hull!");
  for (i = 0; i < ship->num_rooms && i < MAX_SHIP_ROOMS; i++)
  {
    room = real_room(ship->room_vnums[i]);
    if (room == NOWHERE)
    {
      continue;
    }
    for (ch = world[room].people; ch != NULL; ch = ch->next_in_room)
    {
      if ((!IS_NPC(ch) && GET_LEVEL(ch) >= LVL_IMMORT) || GET_POS(ch) <= POS_SLEEPING)
      {
        continue;
      }
      if (d20(ch) + compute_mag_saves(ch, SAVING_REFL, 0) >= VESSEL_KNOCKDOWN_DC)
      {
        send_to_char(ch, "You keep your footing.\r\n");
        continue;
      }
      send_to_char(ch, "The blast knocks you off your feet!\r\n");
      if (GET_POS(ch) > POS_SITTING)
      {
        change_position(ch, POS_SITTING);
      }
      WAIT_STATE(ch, PULSE_VIOLENCE * 2);
    }
  }
}

/**
 * Put one fragment's hull damage on the target's arc (Duris damage_hull()).
 *
 * Armor absorbs first. A hit it holds stops there unless it is a confirmed
 * critical, which carries half the damage into the structure with a 50%
 * chance to damage a weapon; overkill spills into the structure with a 15%
 * chance. On a gutted arc one hit in three deflects into another arc that
 * still has structure, and every hit there damages a weapon. Stern hits foul
 * the rudder (LuminariMUD-only), and one structural hit in nine knocks the
 * crew down.
 *
 * @return damage dealt to armor and structure
 */
int vessel_damage_hull(struct greyhawk_ship_data *attacker, struct greyhawk_ship_data *target,
                       int damage, int arc, bool critical)
{
  unsigned char *armor;
  unsigned char *internal;
  int weapon_chance;
  int dealt;
  int other;

  if (target == NULL)
  {
    return 0;
  }
  damage = MAX(1, damage);
  dealt = damage;

  if (attacker != NULL)
  {
    send_to_ship(attacker, "You hit [%s] %s for %d point%s on the %s!", target->id, target->name,
                 damage, damage == 1 ? "" : "s", vessel_arc_side_name(arc));
  }
  send_to_ship_throttled(target, VESSEL_MESSAGE_COMBAT_DAMAGE, VESSEL_COMBAT_MESSAGE_COOLDOWN,
                         "The %s is hit for %d point%s!", vessel_arc_side_name(arc), damage,
                         damage == 1 ? "" : "s");

  weapon_chance = 15;
  armor = vessel_arc_armor(target, arc);
  if (*armor > 0)
  {
    if (damage > *armor)
    {
      damage -= *armor;
      *armor = 0;
    }
    else
    {
      *armor = (unsigned char)(*armor - damage);
      if (!critical)
      {
        return dealt;
      }
      send_to_ship(target, "CRITICAL HIT!");
      if (attacker != NULL)
      {
        send_to_ship(attacker, "CRITICAL HIT!");
      }
      damage /= 2;
      weapon_chance = 50;
    }
  }

  internal = vessel_arc_internal(target, arc);
  if (*internal < 1)
  {
    other = rand_number(1, 9);
    if (other < 4 && *vessel_arc_internal(target, (arc + other) % VESSEL_NUM_ARCS) > 0)
    {
      arc = (arc + other) % VESSEL_NUM_ARCS;
      internal = vessel_arc_internal(target, arc);
    }
  }
  if (*internal < 1)
  {
    weapon_chance = 100;
  }
  *internal = (unsigned char)MAX(0, (int)*internal - damage);

  if (arc == GREYHAWK_REAR && target->turnrate > 0)
  {
    target->turnrate = (unsigned char)MAX(0, (int)target->turnrate - damage);
    if (target->turnrate == 0)
    {
      send_to_ship(target, "The rudder is smashed! The helm no longer answers.");
    }
  }

  if (rand_number(0, 99) < weapon_chance)
  {
    vessel_damage_weapon(attacker, target, arc, damage * 5);
  }
  if (rand_number(1, 9) == 9)
  {
    vessel_knockdown_aboard(target);
  }
  return dealt;
}

/**
 * Resolve a weapon's hit on target (Duris volley_hit_event()): each fragment
 * strikes the sails or the arc facing the shooter, scattered across the
 * weapon's spread.
 *
 * @param attacker Firing hull (the bearing's origin)
 * @param critical A confirmed critical for the shot
 * @return total damage dealt
 */
int vessel_resolve_hit(struct greyhawk_ship_data *attacker, struct greyhawk_ship_data *target,
                       const struct greyhawk_ship_slot *weapon, bool critical)
{
  const struct vessel_weapon_profile *profile;
  int bearing;
  int damage;
  int total;
  int arc;
  int i;

  if (attacker == NULL || target == NULL || weapon == NULL)
  {
    return 0;
  }

  profile = vessel_weapon_profile(weapon);
  bearing = greyhawk_bearing(target->x, target->y, attacker->x, attacker->y);
  total = 0;
  for (i = 0; i < profile->fragments; i++)
  {
    damage = (weapon->val2 > 0 && weapon->val3 > 0) ? dice(weapon->val2, weapon->val3) : dice(2, 6);
    if (target->mainsail > 0 && rand_number(0, 99) < profile->sail_hit)
    {
      total += vessel_damage_sail(attacker, target, damage * profile->sail_percent / 100);
      continue;
    }
    arc = vessel_arc_for_relative_bearing(bearing +
                                          rand_number(-(profile->spread / 2), profile->spread / 2) -
                                          vessel_display_heading(target->heading));
    total +=
        vessel_damage_hull(attacker, target, damage * profile->hull_percent / 100, arc, critical);
  }
  vessel_update_condition(target, attacker);
  return total;
}

/**
 * Reconcile a hull's state after damage: a hull with no structure left sinks.
 */
void vessel_update_condition(struct greyhawk_ship_data *ship, struct greyhawk_ship_data *attacker)
{
  (void)attacker;
  if (is_valid_ship(ship) && vessel_total_internal(ship) <= 0)
  {
    vessel_sink(ship->shipnum);
  }
}
