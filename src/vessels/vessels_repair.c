/* ************************************************************************
 *      File:   vessels_repair.c                      Part of LuminariMUD  *
 *   Purpose:   Vessel repair (vessels-ships study S5, 3.3.6): the repair  *
 *              stock, crew repairs at sea, a character's repair, and the  *
 *              shipyard's priced dock repairs                             *
 * ********************************************************************** */

/*
 * At sea the crew mends what it can from a stock of materials equal to the
 * class hull weight, refilled whenever the hull berths. Sails, the rudder
 * and structure come back only to a cap set by the bosun, and armor never:
 * a hull is made whole only at a shipyard, for gold and the shipwrights'
 * time. Chances are DurisMUD's per-mille odds per second, halved for the
 * 0.5 s vessel tick.
 */

#include "conf.h"
#include "core/sysdep.h"
#include "core/structs.h"
#include "core/utils.h"
#include "core/comm.h"
#include "core/db.h"
#include "core/handler.h"
#include "core/interpreter.h"
#include "character/rewards.h"
#include "magic/spells.h"
#include "vessels.h"

/* Sails and rudder: anchored and whole, anchored and shot away (at battle
 * stations or not), and under way. */
#define REPAIR_RIGGING_ANCHORED 250
#define REPAIR_RIGGING_GONE 50
#define REPAIR_RIGGING_GONE_BATTLE 15
#define REPAIR_RIGGING_UNDER_WAY 15
#define REPAIR_WEAPON 100
/* Structure: anchored (at battle stations or not, holed or not) and under way. */
#define REPAIR_STRUCTURE_ANCHORED 125
#define REPAIR_STRUCTURE_HOLED 50
#define REPAIR_STRUCTURE_BATTLE 50
#define REPAIR_STRUCTURE_HOLED_BATTLE 5
#define REPAIR_STRUCTURE_UNDER_WAY 15

/* A dock repair order keeps the hull at the berth 75 s plus 1 s a point;
 * a weapon 75 s, or 150 s rebuilt. */
#define DOCK_ORDER_TICKS 150
#define DOCK_WEAPON_TICKS 150
#define DOCK_WEAPON_REBUILD_TICKS 300

enum vessel_dock_repair
{
  DOCK_REPAIR_ARMOR,
  DOCK_REPAIR_STRUCTURE,
  DOCK_REPAIR_SAILS,
  DOCK_REPAIR_RUDDER,
  DOCK_REPAIR_WEAPONS,
  NUM_DOCK_REPAIRS
};

static const char *const dock_repair_names[NUM_DOCK_REPAIRS] = {"armor", "structure", "sails",
                                                                "rudder", "weapons"};

/** Repair materials left: the class hull weight less what the crew has used. */
int vessel_repair_stock(const struct greyhawk_ship_data *ship)
{
  if (ship == NULL)
  {
    return 0;
  }
  return MAX(0, vessel_class_handling(ship->vessel_type)->hull_weight - ship->repair_used);
}

/** The bosun's repair mod: 0.15 a tier. */
double vessel_bosun_mod(const struct greyhawk_ship_data *ship)
{
  return ship != NULL ? 0.15 * ship->crew_tier[CREW_BOSUN] : 0.0;
}

/** Below the cap the crew can mend at sea: `share` of the maximum, and below 90% of it. */
static bool vessel_below_sea_cap(int current, int max, double share)
{
  return (double)current < (double)max * share && (double)current < (double)max * 0.9;
}

/**
 * One repair the crew or a character made: spend its materials and stamina,
 * and train the bosun, more at battle stations against a locked contact.
 */
static void vessel_repair_made(struct greyhawk_ship_data *ship, int stock, double stamina)
{
  ship->repair_used = (short int)(ship->repair_used + stock);
  ship->stamina_spent += stamina;
  vessel_crew_gain(ship, CREW_BOSUN,
                   vessel_at_battle_stations(ship) && ship->lock_target != 0 ? 0.1 : 0.01);
  if (vessel_repair_stock(ship) == 0)
  {
    send_to_ship(ship, "The crew has used the last of the repair stores.");
  }
}

static bool vessel_repair_roll(int permille, double scale)
{
  return rand_number(0, 999) < (int)(permille * scale);
}

/** A crew repair on the sails or rudder: one point toward the sea cap. */
static void vessel_repair_rigging(struct greyhawk_ship_data *ship, unsigned char *current, int max,
                                  double scale)
{
  int permille;

  if (vessel_repair_stock(ship) <= 0 ||
      !vessel_below_sea_cap(*current, max, vessel_bosun_mod(ship) + 0.4))
  {
    return;
  }
  if (!ship->anchored)
  {
    permille = *current > 0 ? REPAIR_RIGGING_UNDER_WAY : 0;
  }
  else if (*current > 0)
  {
    permille = REPAIR_RIGGING_ANCHORED;
  }
  else
  {
    permille = vessel_at_battle_stations(ship) ? REPAIR_RIGGING_GONE_BATTLE : REPAIR_RIGGING_GONE;
  }
  if (vessel_repair_roll(permille, scale))
  {
    (*current)++;
    vessel_repair_made(ship, 1, 3.0);
  }
}

/**
 * Crew repairs, each vessel tick (Duris ship_activity()): the sails and
 * rudder below the bosun mod plus 40% of their maximum, damaged weapons, and
 * structure below the bosun mod plus 10%, at odds scaled by the bosun and the
 * crew's fatigue. A sinking or stunned crew repairs nothing.
 */
void vessel_repair_tick_one(struct greyhawk_ship_data *ship)
{
  unsigned char *internal;
  double scale;
  int permille;
  int arc;
  int s;

  if (ship == NULL || vessel_is_sinking(ship) || vessel_crew_stunned(ship) ||
      vessel_repair_stock(ship) <= 0)
  {
    return;
  }
  scale = (1.0 + vessel_bosun_mod(ship)) * vessel_stamina_modifier(ship);

  vessel_repair_rigging(ship, &ship->mainsail, ship->maxmainsail, scale);
  vessel_repair_rigging(ship, &ship->turnrate, ship->maxturnrate, scale);

  for (s = 0; s < GREYHAWK_MAXSLOTS && vessel_repair_stock(ship) > 0; s++)
  {
    if (vessel_slot_weapon(&ship->slot[s]) == NULL || ship->slot[s].damage == 0 ||
        ship->slot[s].damage >= VESSEL_WEAPON_DESTROYED ||
        !vessel_repair_roll(REPAIR_WEAPON, scale))
    {
      continue;
    }
    ship->slot[s].damage--;
    if (ship->slot[s].damage == 0)
    {
      send_to_ship(ship, "The %s %s has been repaired!", vessel_arc_name(ship->slot[s].position),
                   vessel_slot_name(&ship->slot[s]));
    }
    vessel_repair_made(ship, rand_number(0, 4) == 0 ? 1 : 0, 1.0);
  }

  for (arc = 0; arc < VESSEL_NUM_ARCS && vessel_repair_stock(ship) > 0; arc++)
  {
    internal = vessel_arc_internal(ship, arc);
    if (!vessel_below_sea_cap(*internal, *vessel_arc_max_internal(ship, arc),
                              vessel_bosun_mod(ship) + 0.1))
    {
      continue;
    }
    if (!ship->anchored)
    {
      permille = *internal > 0 ? REPAIR_STRUCTURE_UNDER_WAY : 0;
    }
    else if (vessel_at_battle_stations(ship))
    {
      permille = *internal > 0 ? REPAIR_STRUCTURE_BATTLE : REPAIR_STRUCTURE_HOLED_BATTLE;
    }
    else
    {
      permille = *internal > 0 ? REPAIR_STRUCTURE_ANCHORED : REPAIR_STRUCTURE_HOLED;
    }
    if (vessel_repair_roll(permille, scale))
    {
      (*internal)++;
      vessel_repair_made(ship, 1, 2.0);
    }
  }
}

/**
 * A character's repair at sea: one point from the stores on the weakest
 * structure below the sea cap, else the sails, the rudder, or a damaged
 * weapon, for a Craft (woodworking) check.
 */
static void vessel_character_repair(struct char_data *ch, struct greyhawk_ship_data *ship)
{
  unsigned char *point;
  const char *what;
  double structure_cap;
  int step;
  int arc;
  int s;

  if (vessel_repair_stock(ship) <= 0)
  {
    send_to_char(ch, "The repair stores are spent; she must put into port to refill them.\r\n");
    return;
  }

  point = NULL;
  what = NULL;
  step = 1;
  structure_cap = vessel_bosun_mod(ship) + 0.1;
  for (arc = 0; arc < VESSEL_NUM_ARCS; arc++)
  {
    if (vessel_below_sea_cap(*vessel_arc_internal(ship, arc), *vessel_arc_max_internal(ship, arc),
                             structure_cap) &&
        (point == NULL || *vessel_arc_internal(ship, arc) < *point))
    {
      point = vessel_arc_internal(ship, arc);
      what = "timbers";
    }
  }
  if (point == NULL &&
      vessel_below_sea_cap(ship->mainsail, ship->maxmainsail, vessel_bosun_mod(ship) + 0.4))
  {
    point = &ship->mainsail;
    what = "rigging";
  }
  if (point == NULL &&
      vessel_below_sea_cap(ship->turnrate, ship->maxturnrate, vessel_bosun_mod(ship) + 0.4))
  {
    point = &ship->turnrate;
    what = "rudder";
  }
  for (s = 0; point == NULL && s < GREYHAWK_MAXSLOTS; s++)
  {
    if (vessel_slot_weapon(&ship->slot[s]) != NULL && ship->slot[s].damage > 0 &&
        ship->slot[s].damage < VESSEL_WEAPON_DESTROYED)
    {
      point = &ship->slot[s].damage;
      what = vessel_slot_name(&ship->slot[s]);
      step = -1;
    }
  }
  if (point == NULL)
  {
    send_to_char(ch, "Nothing aboard needs a patch the stores can make at sea; armor, and the "
                     "rest of her, are made good only at a shipyard.\r\n");
    return;
  }

  WAIT_STATE(ch, PULSE_VIOLENCE * 2);
  if (!skill_check(ch, ABILITY_CRAFT_WOODWORKING, VESSEL_REPAIR_DC))
  {
    send_to_char(ch, "You work at the %s, but the patch will not hold.\r\n", what);
    return;
  }
  *point = (unsigned char)(*point + step);
  ship->repair_used++;
  act("$n works on the ship's repairs.", TRUE, ch, 0, 0, TO_ROOM);
  send_to_char(ch, "You patch the %s. The ship is %s.\r\n", what,
               vessel_status_name(vessel_status(ship)));
}

/**
 * A dock repair order's gold and the shipwrights' vessel ticks: armor and
 * structure 2 gold a point and sails and rudder 4, each order 75 s plus a
 * second a point; a damaged weapon 2 gold a damage point (75 s), a destroyed
 * one half its price (150 s).
 *
 * @return points of work, or damaged weapons for DOCK_REPAIR_WEAPONS
 */
static int vessel_dock_repair_quote(struct greyhawk_ship_data *ship, int kind, int *cost,
                                    int *ticks)
{
  const struct vessel_weapon_type *weapon;
  int points;
  int arc;
  int s;

  points = 0;
  *cost = 0;
  *ticks = 0;
  switch (kind)
  {
  case DOCK_REPAIR_ARMOR:
  case DOCK_REPAIR_STRUCTURE:
    for (arc = 0; arc < VESSEL_NUM_ARCS; arc++)
    {
      points += kind == DOCK_REPAIR_ARMOR
                    ? *vessel_arc_max_armor(ship, arc) - *vessel_arc_armor(ship, arc)
                    : *vessel_arc_max_internal(ship, arc) - *vessel_arc_internal(ship, arc);
    }
    *cost = points * VESSEL_DOCK_REPAIR_POINT_PRICE;
    break;
  case DOCK_REPAIR_SAILS:
    points = ship->maxmainsail - ship->mainsail;
    *cost = points * VESSEL_DOCK_RIGGING_POINT_PRICE;
    break;
  case DOCK_REPAIR_RUDDER:
    points = ship->maxturnrate - ship->turnrate;
    *cost = points * VESSEL_DOCK_RIGGING_POINT_PRICE;
    break;
  default:
    for (s = 0; s < GREYHAWK_MAXSLOTS; s++)
    {
      weapon = vessel_slot_weapon(&ship->slot[s]);
      if (weapon == NULL || ship->slot[s].damage == 0)
      {
        continue;
      }
      points++;
      if (ship->slot[s].damage >= VESSEL_WEAPON_DESTROYED)
      {
        *cost += weapon->price / 2;
        *ticks += DOCK_WEAPON_REBUILD_TICKS;
      }
      else
      {
        *cost += ship->slot[s].damage * VESSEL_DOCK_REPAIR_POINT_PRICE;
        *ticks += DOCK_WEAPON_TICKS;
      }
    }
    return points;
  }
  if (points > 0)
  {
    *ticks = DOCK_ORDER_TICKS + 2 * points;
  }
  return points;
}

static void vessel_dock_repair_apply(struct greyhawk_ship_data *ship, int kind)
{
  int arc;
  int s;

  switch (kind)
  {
  case DOCK_REPAIR_ARMOR:
    for (arc = 0; arc < VESSEL_NUM_ARCS; arc++)
    {
      *vessel_arc_armor(ship, arc) = *vessel_arc_max_armor(ship, arc);
    }
    break;
  case DOCK_REPAIR_STRUCTURE:
    for (arc = 0; arc < VESSEL_NUM_ARCS; arc++)
    {
      *vessel_arc_internal(ship, arc) = *vessel_arc_max_internal(ship, arc);
    }
    break;
  case DOCK_REPAIR_SAILS:
    ship->mainsail = ship->maxmainsail;
    break;
  case DOCK_REPAIR_RUDDER:
    ship->turnrate = ship->maxturnrate;
    break;
  default:
    for (s = 0; s < GREYHAWK_MAXSLOTS; s++)
    {
      ship->slot[s].damage = 0;
    }
    break;
  }
}

/**
 * shiprepair [armor | structure | sails | rudder | weapons | all] - at a
 * shipyard, the owner buys dock repairs; at sea, anyone aboard patches one
 * point from the stores.
 */
ACMD(do_shiprepair)
{
  struct greyhawk_ship_data *ship;
  struct greyhawk_ship_data before;
  char arg[MAX_INPUT_LENGTH];
  int points[NUM_DOCK_REPAIRS];
  int costs[NUM_DOCK_REPAIRS];
  int ticks[NUM_DOCK_REPAIRS];
  int total_cost;
  int total_ticks;
  int kind;

  ship = get_ship_from_room(IN_ROOM(ch));
  if (ship == NULL)
  {
    send_to_char(ch, "You must be aboard a ship to make repairs.\r\n");
    return;
  }
  if (vessel_is_sinking(ship))
  {
    send_to_char(ch, "She is holed on two sides and going down - no patch will save her.\r\n");
    return;
  }
  if (ship->dock <= 0)
  {
    if (vessel_crew_stunned(ship))
    {
      send_to_char(ch, "The crew reels from a mental blast; nobody can hold a tool steady.\r\n");
      return;
    }
    vessel_character_repair(ch, ship);
    return;
  }

  ship = vessel_refit_ship(ch);
  if (ship == NULL)
  {
    return;
  }

  one_argument(argument, arg, sizeof(arg));
  total_cost = 0;
  total_ticks = 0;
  for (kind = 0; kind < NUM_DOCK_REPAIRS; kind++)
  {
    points[kind] = vessel_dock_repair_quote(ship, kind, &costs[kind], &ticks[kind]);
    total_cost += costs[kind];
    total_ticks += ticks[kind];
  }

  if (!*arg)
  {
    send_to_char(ch, "The shipwrights' quote for %s:\r\n", ship->name);
    for (kind = 0; kind < NUM_DOCK_REPAIRS; kind++)
    {
      send_to_char(ch, "  %-10s %4d %-7s %7d gold, %4d seconds\r\n", dock_repair_names[kind],
                   points[kind], kind == DOCK_REPAIR_WEAPONS ? "weapons" : "points", costs[kind],
                   ticks[kind] / 2);
    }
    send_to_char(ch, "  %-10s %20d gold, %4d seconds\r\n", "all", total_cost, total_ticks / 2);
    send_to_char(ch, "Usage: shiprepair <armor|structure|sails|rudder|weapons|all>\r\n");
    return;
  }

  if (is_abbrev(arg, "all"))
  {
    kind = NUM_DOCK_REPAIRS;
  }
  else
  {
    for (kind = 0; kind < NUM_DOCK_REPAIRS; kind++)
    {
      if (is_abbrev(arg, dock_repair_names[kind]))
      {
        break;
      }
    }
    if (kind == NUM_DOCK_REPAIRS)
    {
      send_to_char(ch, "Repair what? Type 'shiprepair' for the shipwrights' quote.\r\n");
      return;
    }
    total_cost = costs[kind];
    total_ticks = ticks[kind];
  }

  if (total_ticks == 0)
  {
    send_to_char(ch, "%s needs no such work.\r\n", ship->name);
    return;
  }
  if (GET_GOLD(ch) < total_cost)
  {
    send_to_char(ch, "The shipwrights want %d gold for that; you have %d.\r\n", total_cost,
                 GET_GOLD(ch));
    return;
  }

  before = *ship;
  if (!vessel_charge(ch, total_cost))
  {
    return;
  }
  if (kind == NUM_DOCK_REPAIRS)
  {
    for (kind = 0; kind < NUM_DOCK_REPAIRS; kind++)
    {
      vessel_dock_repair_apply(ship, kind);
    }
  }
  else
  {
    vessel_dock_repair_apply(ship, kind);
  }
  vessel_add_maintenance(ship, ch, total_ticks);
  if (!vessel_purchase_recorded(ch, ship, &before, total_cost))
  {
    return;
  }

  send_to_char(ch, "You pay %d gold. The shipwrights set to work on %s: %d seconds.\r\n",
               total_cost, ship->name, total_ticks / 2);
  log("Info: %s bought dock repairs for ship %d for %d gold", GET_NAME(ch), ship->shipnum,
      total_cost);
}
