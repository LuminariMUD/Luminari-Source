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

/**
 * Raise every arc's armor ceiling, or its structure ceiling, by a fifth for
 * the plating or reinforcement refit (3.3.1). The new plates or frames add
 * the same points to the arc, so damage stays damage (L10).
 */
void vessel_refit_arcs(struct greyhawk_ship_data *ship, bool structure)
{
  unsigned char *ceiling;
  unsigned char *current;
  int added;
  int arc;

  if (ship == NULL)
  {
    return;
  }

  for (arc = 0; arc < VESSEL_NUM_ARCS; arc++)
  {
    ceiling = structure ? vessel_arc_max_internal(ship, arc) : vessel_arc_max_armor(ship, arc);
    current = structure ? vessel_arc_internal(ship, arc) : vessel_arc_armor(ship, arc);
    added = MIN(255, *ceiling + *ceiling / 5) - *ceiling;
    *ceiling = (unsigned char)(*ceiling + added);
    *current = (unsigned char)(*current + added);
  }
}

/**
 * A pre-S3 prototype armor on the S3 scale: the class beam armor over the
 * class's old default armor (3.3.10). Mirrors the one-time rescale in
 * vessel_prototype_ensure_schema() and vessels_phase19_schema.sql.
 */
int vessel_rescale_legacy_armor(int vclass, int armor)
{
  /* The old `vedit new` defaults, in enum vessel_class order */
  static const int legacy_default_armor[NUM_VESSEL_TYPES] = {2, 5, 20, 40, 15, 25, 20, 20};
  int legacy;

  if (vclass < 0 || vclass >= NUM_VESSEL_TYPES)
  {
    vclass = VESSEL_SHIP;
  }
  legacy = legacy_default_armor[vclass];
  armor = MAX(0, armor);
  return MIN(VESSEL_MAX_PROTOTYPE_ARMOR,
             (armor * vessel_class_condition((enum vessel_class)vclass)->beam_armor + legacy / 2) /
                 legacy);
}

/** `current` of `old_max` carried to `new_max`, rounded. A 0 maximum was whole. */
static int vessel_carry_fraction(int current, int old_max, int new_max)
{
  if (old_max <= 0)
  {
    return new_max;
  }
  current = MAX(0, MIN(old_max, current));
  return (new_max * current + old_max / 2) / old_max;
}

/**
 * Convert a hull saved under the pre-S3 condition model (3.3.10): the class
 * profile at its rescaled armor, refits recomputed at the S3 sizes, and each
 * arc, the sails, and the rudder keeping their damage fraction.
 *
 * The old model gave every arc the prototype armor (half again with
 * plating), structure of half that plus 10 (half again with reinforcement),
 * 20 sail and 20 rudder, and rigging added 5 speed. It had no holes, so a
 * converted arc keeps at least 1 point of structure.
 */
void vessel_convert_legacy_condition(struct greyhawk_ship_data *ship, int upgrades)
{
  int old_armor[VESSEL_NUM_ARCS];
  int old_max_armor[VESSEL_NUM_ARCS];
  int old_internal[VESSEL_NUM_ARCS];
  int old_max_internal[VESSEL_NUM_ARCS];
  int old_sail;
  int old_max_sail;
  int old_rudder;
  int old_max_rudder;
  int base_armor;
  int arc;

  if (ship == NULL)
  {
    return;
  }

  for (arc = 0; arc < VESSEL_NUM_ARCS; arc++)
  {
    old_armor[arc] = *vessel_arc_armor(ship, arc);
    old_max_armor[arc] = *vessel_arc_max_armor(ship, arc);
    old_internal[arc] = *vessel_arc_internal(ship, arc);
    old_max_internal[arc] = *vessel_arc_max_internal(ship, arc);
  }
  old_sail = ship->mainsail;
  old_max_sail = ship->maxmainsail;
  old_rudder = ship->turnrate;
  old_max_rudder = ship->maxturnrate;

  /* Undo the old plating's +50% (x + x / 2) to find the prototype armor. */
  base_armor = ship->maxfarmor;
  if (IS_SET(upgrades, SHIP_UPGRADE_PLATING))
  {
    base_armor = (base_armor * 2 + 2) / 3;
  }
  vessel_initialize_condition(ship, vessel_rescale_legacy_armor(ship->vessel_type, base_armor));
  if (IS_SET(upgrades, SHIP_UPGRADE_PLATING))
  {
    vessel_refit_arcs(ship, FALSE);
  }
  if (IS_SET(upgrades, SHIP_UPGRADE_REINFORCED))
  {
    vessel_refit_arcs(ship, TRUE);
  }
  if (IS_SET(upgrades, SHIP_UPGRADE_RIGGING))
  {
    ship->maxspeed = vessel_rigged_speed(MAX(1, ship->maxspeed - 5));
  }

  for (arc = 0; arc < VESSEL_NUM_ARCS; arc++)
  {
    *vessel_arc_armor(ship, arc) = (unsigned char)vessel_carry_fraction(
        old_armor[arc], old_max_armor[arc], *vessel_arc_max_armor(ship, arc));
    *vessel_arc_internal(ship, arc) =
        (unsigned char)MAX(1, vessel_carry_fraction(old_internal[arc], old_max_internal[arc],
                                                    *vessel_arc_max_internal(ship, arc)));
  }
  ship->mainsail = (unsigned char)vessel_carry_fraction(old_sail, old_max_sail, ship->maxmainsail);
  ship->turnrate =
      (unsigned char)vessel_carry_fraction(old_rudder, old_max_rudder, ship->maxturnrate);
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

/** A mounted weapon that is undamaged, reloaded, and has a round left. */
bool vessel_weapon_ready(const struct greyhawk_ship_slot *slot)
{
  return vessel_slot_weapon(slot) != NULL && slot->damage == 0 && slot->timer <= 0 &&
         slot->ammo > 0;
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
    if (target->slot[i].type == VESSEL_SLOT_WEAPON && target->slot[i].position == arc &&
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
    send_to_ship(target, "The %s %s has been destroyed!", vessel_arc_name(arc),
                 vessel_slot_name(weapon));
    if (attacker != NULL)
    {
      send_to_ship(attacker, "You destroy the %s %s aboard %s!", vessel_arc_name(arc),
                   vessel_slot_name(weapon), target->name);
    }
  }
  else
  {
    send_to_ship(target, "The %s %s has been damaged!", vessel_arc_name(arc),
                 vessel_slot_name(weapon));
    if (attacker != NULL)
    {
      send_to_ship(attacker, "You damage the %s %s aboard %s!", vessel_arc_name(arc),
                   vessel_slot_name(weapon), target->name);
    }
  }
}

/**
 * Knock everyone aboard off their feet unless they make the save (DC 15):
 * Reflex against a hull hit's blast, Will against a mental blast. The fallen
 * are prone for two combat rounds. Staff are exempt.
 */
void vessel_knockdown_aboard(struct greyhawk_ship_data *ship, int save)
{
  struct char_data *ch;
  room_rnum room;
  bool mental;
  int i;

  if (ship == NULL)
  {
    return;
  }

  mental = save == SAVING_WILL;
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
      if (d20(ch) + compute_mag_saves(ch, save, 0) >= VESSEL_KNOCKDOWN_DC)
      {
        send_to_char(ch, mental ? "You shake off the blast.\r\n" : "You keep your footing.\r\n");
        continue;
      }
      send_to_char(ch, mental
                           ? "Your mind reels from the blast; you huddle on the deck in pain!\r\n"
                           : "The blast knocks you off your feet!\r\n");
      if (GET_POS(ch) > POS_RECLINING)
      {
        change_position(ch, POS_RECLINING);
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
  int breaches;
  int dealt;
  int other;

  if (target == NULL)
  {
    return 0;
  }
  damage = MAX(1, damage);
  dealt = damage;
  breaches = vessel_breached_arcs(target);

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
  if (*vessel_arc_armor(target, arc) == 0 && *internal == 0 &&
      vessel_breached_arcs(target) > breaches)
  {
    send_to_ship(target, "The %s is holed through!", vessel_arc_side_name(arc));
    if (breaches == 0)
    {
      send_to_ship(target, target->z > 0.0 ? "She can make only half speed aloft!"
                                           : "She is too damaged to move!");
    }
    if (attacker != NULL)
    {
      send_to_ship(attacker, "You hole %s's %s!", target->name, vessel_arc_side_name(arc));
    }
  }

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
    send_to_ship(target, "The blast shakes the whole hull!");
    vessel_knockdown_aboard(target, SAVING_REFL);
  }
  return dealt;
}

/**
 * Resolve a weapon's hit on target (Duris volley_hit_event()): each fragment
 * strikes the sails or the arc facing the shooter, scattered across the
 * weapon's spread. A beam weapon's damage falls from its maximum at minimum
 * range to its minimum at maximum range; a crew-stun weapon does none.
 *
 * @param attacker Firing hull (the bearing's origin)
 * @param range Range the shot was fired at, in rooms
 * @param critical A confirmed critical for the shot
 * @return total damage dealt
 */
int vessel_resolve_hit(struct greyhawk_ship_data *attacker, struct greyhawk_ship_data *target,
                       const struct greyhawk_ship_slot *weapon, double range, bool critical)
{
  const struct vessel_weapon_type *type;
  double closeness;
  int bearing;
  int damage;
  int total;
  int arc;
  int i;

  type = vessel_slot_weapon(weapon);
  if (attacker == NULL || target == NULL || type == NULL || type->max_damage <= 0)
  {
    return 0;
  }

  closeness = type->max_range > type->min_range
                  ? ((double)type->max_range - range) / (double)(type->max_range - type->min_range)
                  : 1.0;
  closeness = fmax(0.0, fmin(1.0, closeness));
  bearing = greyhawk_bearing(target->x, target->y, attacker->x, attacker->y);
  total = 0;
  for (i = 0; i < type->fragments; i++)
  {
    if (IS_SET(type->flags, VESSEL_WEAPON_RANGE_DAMAGE))
    {
      damage = type->min_damage + (int)((type->max_damage - type->min_damage) * closeness);
    }
    else
    {
      damage = rand_number(type->min_damage, type->max_damage);
    }
    if (target->mainsail > 0 && rand_number(0, 99) < type->sail_hit)
    {
      total += vessel_damage_sail(attacker, target, damage * type->sail_percent / 100);
      continue;
    }
    arc = vessel_arc_for_relative_bearing(bearing +
                                          rand_number(-(type->spread / 2), type->spread / 2) -
                                          vessel_display_heading(target->heading));
    total += vessel_damage_hull(attacker, target, damage * type->hull_percent / 100, arc, critical);
  }
  vessel_update_condition(target, attacker);
  return total;
}

/** Arcs with neither armor nor structure left (study 3.3.3). */
int vessel_breached_arcs(const struct greyhawk_ship_data *ship)
{
  if (ship == NULL)
  {
    return 0;
  }
  return (ship->farmor == 0 && ship->finternal == 0) + (ship->parmor == 0 && ship->pinternal == 0) +
         (ship->rarmor == 0 && ship->rinternal == 0) + (ship->sarmor == 0 && ship->sinternal == 0);
}

bool vessel_is_sinking(const struct greyhawk_ship_data *ship)
{
  return ship != NULL && ship->sink_ticks > 0;
}

/**
 * Start the sink timer: 75-150 s for a player-owned hull, 1000-1500 s for a
 * public or NPC hull so it can be boarded and looted. The hull stops, drops
 * her autopilot, and can neither move nor fire until she goes down.
 */
void vessel_begin_sinking(struct greyhawk_ship_data *ship)
{
  if (!is_valid_ship(ship) || vessel_is_sinking(ship))
  {
    return;
  }

  ship->sink_ticks =
      (short int)(ship->owner[0] != '\0'
                      ? rand_number(VESSEL_SINK_TICKS_OWNED_MIN, VESSEL_SINK_TICKS_OWNED_MAX)
                      : rand_number(VESSEL_SINK_TICKS_UNOWNED_MIN, VESSEL_SINK_TICKS_UNOWNED_MAX));
  ship->setspeed = 0;
  ship->anchored = FALSE;
  ship->colors_struck_ticks = 0;
  if (ship->autopilot != NULL && (ship->autopilot->state == AUTOPILOT_TRAVELING ||
                                  ship->autopilot->state == AUTOPILOT_WAITING))
  {
    autopilot_pause(ship);
  }
  send_to_ship(ship, "%s is holed on two sides - she is SINKING! Abandon ship!", ship->name);
  log("Info: Ship %d '%s' started sinking at (%d,%d); %d ticks", ship->shipnum, ship->name,
      (int)ship->x, (int)ship->y, ship->sink_ticks);
}

/**
 * Reconcile a hull's state after damage (Duris update_ship_status()): one
 * breached arc immobilizes her (vessel_max_speed()), two start her sinking.
 * Either stops her dead at once; a hull aloft with one hole keeps half speed.
 */
void vessel_update_condition(struct greyhawk_ship_data *ship, struct greyhawk_ship_data *attacker)
{
  (void)attacker;
  if (!is_valid_ship(ship))
  {
    return;
  }
  if (!vessel_is_sinking(ship) && vessel_breached_arcs(ship) >= 2)
  {
    vessel_begin_sinking(ship);
  }
  if (vessel_is_sinking(ship) || (vessel_breached_arcs(ship) > 0 && ship->z <= 0.0))
  {
    ship->speed = 0.0;
  }
}

/**
 * Float half of each bulk cargo lot off as salvage crates in `room` (study
 * 3.3.3); the other half goes down with the hull. A crate floats for
 * VESSEL_SALVAGE_CRATE_HOURS and cannot be carried off by hand.
 *
 * @return crates set afloat
 */
int vessel_spill_cargo(struct greyhawk_ship_data *ship, room_rnum room)
{
  struct obj_data *crate;
  char buf[MAX_STRING_LENGTH];
  const char *goods;
  int crates;
  int units;
  int i;

  if (ship == NULL || room == NOWHERE)
  {
    return 0;
  }

  crates = 0;
  for (i = 0; i < MAX_CARGO_LOTS; i++)
  {
    units = ship->cargo[i].quantity / 2;
    if (ship->cargo[i].commodity_id <= 0 || units <= 0)
    {
      continue;
    }

    goods = vessel_commodity_name(ship->cargo[i].commodity_id);
    crate = create_obj();
    GET_OBJ_TYPE(crate) = ITEM_OTHER;
    GET_OBJ_VAL(crate, 0) = ship->cargo[i].commodity_id;
    GET_OBJ_VAL(crate, 1) = units;
    GET_OBJ_VAL(crate, 2) = VESSEL_SALVAGE_CRATE_MARK;
    GET_OBJ_TIMER(crate) = VESSEL_SALVAGE_CRATE_HOURS;
    GET_OBJ_COST(crate) = 0;
    GET_OBJ_RENT(crate) = 0;
    GET_OBJ_WEIGHT(crate) = 0;
    snprintf(buf, sizeof(buf), "crate salvage floating %s", goods);
    crate->name = strdup(buf);
    snprintf(buf, sizeof(buf), "a floating crate of %s", goods);
    crate->short_description = strdup(buf);
    snprintf(buf, sizeof(buf), "A salvage crate of %s (%d unit%s) bobs among the waves here.",
             goods, units, units == 1 ? "" : "s");
    crate->description = strdup(buf);
    SET_BIT_AR(GET_OBJ_EXTRA(crate), ITEM_DECAY);
    SET_BIT_AR(GET_OBJ_EXTRA(crate), ITEM_NORENT);
    SET_BIT_AR(GET_OBJ_EXTRA(crate), ITEM_NOSELL);
    obj_to_room(crate, room);
    crates++;
  }
  return crates;
}

/** A floating salvage crate made by vessel_spill_cargo(). */
bool vessel_is_salvage_crate(const struct obj_data *obj)
{
  return obj != NULL && GET_OBJ_TYPE(obj) == ITEM_OTHER &&
         GET_OBJ_VAL(obj, 2) == VESSEL_SALVAGE_CRATE_MARK && GET_OBJ_VAL(obj, 0) > 0 &&
         GET_OBJ_VAL(obj, 1) > 0;
}

/**
 * Haul the salvage crates floating in `room` into ship's hold, as much as it
 * will carry; a crate that does not fit keeps the rest.
 *
 * @return units hauled aboard
 */
int vessel_salvage_crates(struct greyhawk_ship_data *ship, room_rnum room)
{
  struct obj_data *obj;
  struct obj_data *next_obj;
  int hauled;
  int stowed;

  if (ship == NULL || room == NOWHERE)
  {
    return 0;
  }

  hauled = 0;
  for (obj = world[room].contents; obj != NULL; obj = next_obj)
  {
    next_obj = obj->next_content;
    if (!vessel_is_salvage_crate(obj))
    {
      continue;
    }
    stowed = vessel_stow_cargo(ship, GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1));
    hauled += stowed;
    GET_OBJ_VAL(obj, 1) -= stowed;
    if (GET_OBJ_VAL(obj, 1) <= 0)
    {
      extract_obj(obj);
    }
  }
  if (hauled > 0)
  {
    vessel_db_save_cargo(ship);
  }
  return hauled;
}

/** The sinking hull goes down: her cargo spills, then vessel_sink(). */
static void vessel_finish_sinking(struct greyhawk_ship_data *ship)
{
  room_rnum water;

  water = ship->shipobj != NULL ? IN_ROOM(ship->shipobj) : NOWHERE;
  if (vessel_spill_cargo(ship, water) > 0)
  {
    send_to_room(water, "Crates of cargo burst from the hold and bob to the surface.\r\n");
  }
  vessel_sink(ship->shipnum);
}

/**
 * Damage-model tick: count a sinking hull down to her end, and haul struck
 * colors back up when she moves or their time runs out.
 */
void vessel_damage_tick_one(struct greyhawk_ship_data *ship)
{
  if (!is_valid_ship(ship))
  {
    return;
  }

  if (ship->colors_struck_ticks > 0)
  {
    ship->colors_struck_ticks--;
    if (ship->colors_struck_ticks == 0 || ship->speed > 0.0)
    {
      ship->colors_struck_ticks = 0;
      send_to_ship(ship, "%s's colors fly again.", ship->name);
    }
  }

  if (ship->sink_ticks > 0)
  {
    ship->sink_ticks--;
    if (ship->sink_ticks == 0)
    {
      vessel_finish_sinking(ship);
    }
  }
}

/**
 * shipsalvage - haul floating salvage crates into the hold from the helm of
 * a stopped hull.
 */
ACMD(do_shipsalvage)
{
  struct greyhawk_ship_data *ship;
  room_rnum water;
  int hauled;

  ship = get_ship_from_room(IN_ROOM(ch));
  if (!is_valid_ship(ship))
  {
    send_to_char(ch, "You must be aboard a vessel to haul in salvage.\r\n");
    return;
  }
  if (!is_pilot(ch, ship))
  {
    send_to_char(ch, "You must be at an authorized helm to order salvage.\r\n");
    return;
  }
  if (vessel_is_sinking(ship))
  {
    send_to_char(ch, "She is going down - there is no time for salvage.\r\n");
    return;
  }
  if (ship->speed > 0.0)
  {
    send_to_char(ch, "Bring her to a stop before hauling in salvage.\r\n");
    return;
  }

  water = ship->shipobj != NULL ? IN_ROOM(ship->shipobj) : NOWHERE;
  hauled = vessel_salvage_crates(ship, water);
  if (hauled <= 0)
  {
    send_to_char(ch, "There is no salvage alongside that the hold can take.\r\n");
    return;
  }
  send_to_ship(ship, "The crew hauls %d unit%s of floating salvage into the hold.", hauled,
               hauled == 1 ? "" : "s");
  WAIT_STATE(ch, PULSE_VIOLENCE);
}

bool vessel_colors_struck(const struct greyhawk_ship_data *ship)
{
  return ship != NULL && ship->colors_struck_ticks > 0;
}

/**
 * At sea with no conscious character aboard but `except` (a claimant). Hired
 * crew positions are abstract and defend nothing.
 */
bool vessel_abandoned_at_sea(struct greyhawk_ship_data *ship, const struct char_data *except)
{
  struct char_data *ch;
  room_rnum room;
  int i;

  if (!is_valid_ship(ship) || vessel_ship_is_in_port(ship))
  {
    return FALSE;
  }

  for (i = 0; i < ship->num_rooms && i < MAX_SHIP_ROOMS; i++)
  {
    room = real_room(ship->room_vnums[i]);
    if (room == NOWHERE)
    {
      continue;
    }
    for (ch = world[room].people; ch != NULL; ch = ch->next_in_room)
    {
      if (ch != except && AWAKE(ch))
      {
        return FALSE;
      }
    }
  }
  return TRUE;
}

/**
 * A beaten prize (decision D6): holed, immobile, colors struck, or abandoned
 * at sea. Only such a hull can be captured or plundered, or boarded at speed.
 */
bool vessel_prize_disabled(struct greyhawk_ship_data *ship, const struct char_data *except)
{
  return is_valid_ship(ship) &&
         (vessel_breached_arcs(ship) > 0 || vessel_max_speed(ship) <= 0.0 ||
          vessel_colors_struck(ship) || vessel_abandoned_at_sea(ship, except));
}

/**
 * strikecolors - yield: the owner or a permit holder strikes the colors of a
 * stopped hull, making her a prize until she moves or ten minutes pass.
 */
ACMD(do_strikecolors)
{
  struct greyhawk_ship_data *ship;

  ship = get_ship_from_room(IN_ROOM(ch));
  if (!is_valid_ship(ship))
  {
    send_to_char(ch, "You must be aboard a vessel to strike her colors.\r\n");
    return;
  }
  if (ship->owner[0] == '\0' || !vessel_helm_permitted(ch, ship))
  {
    send_to_char(ch, "Only her owner or a helm permit holder can strike %s's colors.\r\n",
                 ship->name);
    return;
  }
  if (vessel_colors_struck(ship))
  {
    send_to_char(ch, "%s's colors are already struck.\r\n", ship->name);
    return;
  }
  if (ship->speed > 0.0)
  {
    send_to_char(ch, "Bring her to a stop before striking her colors.\r\n");
    return;
  }

  ship->colors_struck_ticks = VESSEL_COLORS_STRUCK_TICKS;
  send_to_ship(ship,
               "%s strikes %s's colors: she yields. They fly again when she gets under way "
               "or in ten minutes.",
               GET_NAME(ch), ship->name);
  if (ship->shipobj != NULL && IN_ROOM(ship->shipobj) != NOWHERE)
  {
    send_to_room(IN_ROOM(ship->shipobj), "%s strikes her colors.\r\n", ship->name);
  }
  log("Info: %s struck the colors of ship %d '%s'", GET_NAME(ch), ship->shipnum, ship->name);
}

/**
 * The damage-model part of `shipstatus`: structure, sails, rudder, crew
 * stamina, repair stores, holes, the sink timer, struck colors, and each
 * weapon's state.
 */
void vessel_show_condition(struct char_data *ch, struct greyhawk_ship_data *ship)
{
  /* The order of the armor lines above: bow, port, starboard, stern */
  static const int display_arc[VESSEL_NUM_ARCS] = {GREYHAWK_FORE, GREYHAWK_PORT, GREYHAWK_STARBOARD,
                                                   GREYHAWK_REAR};
  static const char *const structure_name[VESSEL_NUM_ARCS] = {"bow", "port", "starboard", "stern"};
  const struct greyhawk_ship_slot *slot;
  const struct vessel_weapon_type *weapon;
  char holes[128];
  int breaches;
  int holed;
  int arc;
  int i;
  bool armed;

  if (ch == NULL || ship == NULL)
  {
    return;
  }

  send_to_char(ch, "Structure:");
  for (i = 0; i < VESSEL_NUM_ARCS; i++)
  {
    arc = display_arc[i];
    send_to_char(ch, "%s %s %d/%d", i > 0 ? "," : "", structure_name[i],
                 *vessel_arc_internal(ship, arc), *vessel_arc_max_internal(ship, arc));
  }
  send_to_char(ch, "\r\nSails: %d/%d\r\nRudder: %d/%d\r\n", ship->mainsail, ship->maxmainsail,
               ship->turnrate, ship->maxturnrate);
  send_to_char(ch, "Crew stamina: %d/%d%s\r\nRepair stores: %d/%d\r\n",
               vessel_stamina_max(ship) - (int)ceil(ship->stamina_spent), vessel_stamina_max(ship),
               vessel_stamina_modifier(ship) < 1.0 ? " (exhausted)" : "", vessel_repair_stock(ship),
               vessel_class_handling(ship->vessel_type)->hull_weight);

  breaches = vessel_breached_arcs(ship);
  holes[0] = '\0';
  holed = 0;
  for (i = 0; i < VESSEL_NUM_ARCS; i++)
  {
    arc = display_arc[i];
    if (*vessel_arc_armor(ship, arc) == 0 && *vessel_arc_internal(ship, arc) == 0)
    {
      holed++;
      if (holed > 1)
      {
        strlcat(holes, holed == breaches ? " and " : ", ", sizeof(holes));
      }
      strlcat(holes, vessel_arc_side_name(arc), sizeof(holes));
    }
  }
  if (breaches > 0)
  {
    send_to_char(ch, "Holed: %s. ", holes);
  }
  if (vessel_is_sinking(ship))
  {
    send_to_char(ch, "SINKING: she goes down in about %d seconds.\r\n", (ship->sink_ticks + 1) / 2);
  }
  else if (breaches > 0)
  {
    send_to_char(ch, ship->z > 0 ? "She makes half speed aloft.\r\n" : "She cannot move.\r\n");
  }
  if (vessel_colors_struck(ship))
  {
    send_to_char(ch, "Colors: struck, for about %d more seconds.\r\n",
                 (ship->colors_struck_ticks + 1) / 2);
  }

  send_to_char(ch, "\r\n== Weapons ==\r\n");
  armed = FALSE;
  for (i = 0; i < GREYHAWK_MAXSLOTS; i++)
  {
    slot = &ship->slot[i];
    weapon = vessel_slot_weapon(slot);
    if (slot->type == VESSEL_SLOT_EMPTY)
    {
      continue;
    }
    armed = TRUE;
    if (weapon == NULL)
    {
      send_to_char(ch, "Slot %d: %s\r\n", i, vessel_slot_name(slot));
      continue;
    }
    send_to_char(ch, "Slot %d: %s (%s), %d/%d rounds: ", i, weapon->name,
                 vessel_arc_name(slot->position), slot->ammo, weapon->ammo);
    if (slot->damage >= VESSEL_WEAPON_DESTROYED)
    {
      send_to_char(ch, "destroyed\r\n");
    }
    else if (slot->damage > 0)
    {
      send_to_char(ch, "disabled, %d%% damaged\r\n", slot->damage);
    }
    else if (slot->ammo == 0)
    {
      send_to_char(ch, "out of ammunition\r\n");
    }
    else if (slot->timer > 0)
    {
      send_to_char(ch, "reloading\r\n");
    }
    else
    {
      send_to_char(ch, "ready\r\n");
    }
  }
  if (!armed)
  {
    send_to_char(ch, "None mounted.\r\n");
  }
}
