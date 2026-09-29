/* ************************************************************************
 *      File:   vessels_weapons.c                     Part of LuminariMUD  *
 *   Purpose:   Vessel weapon and equipment catalogue                     *
 *              (vessels-ships study S4, 3.3.4)                           *
 * ********************************************************************** */

/*
 * The twelve DurisMUD weapons and its ram and neutral colors, at 2 gold per
 * platinum, with reloads in 0.5 s vessel ticks. A slot names its catalogue
 * row; everything else about the weapon comes from here.
 */

#include "conf.h"
#include "core/sysdep.h"
#include "core/structs.h"
#include "core/utils.h"
#include "vessels.h"

#define ARC_BIT(arc) (1 << (arc))
#define ARCS_ALL                                                                                   \
  (ARC_BIT(GREYHAWK_FORE) | ARC_BIT(GREYHAWK_PORT) | ARC_BIT(GREYHAWK_REAR) |                      \
   ARC_BIT(GREYHAWK_STARBOARD))
#define ARCS_ENDS (ARC_BIT(GREYHAWK_FORE) | ARC_BIT(GREYHAWK_REAR))
#define ARCS_BEAMS (ARC_BIT(GREYHAWK_PORT) | ARC_BIT(GREYHAWK_STARBOARD))

/* Indexed by enum vessel_weapon_id; row 0 is the unconverted pre-S4 weapon. */
static const struct vessel_weapon_type weapon_types[NUM_VESSEL_WEAPONS] = {
    /* name, price, weight, ammo, range, damage, fragments, spread, sail hit,
     * hull/sail percent, pierce, reload, arcs, flags */
    {"Unknown Weapon", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {"Small Ballista", 100, 3, 60, 0, 8, 2, 4, 1, 10, 12, 100, 50, 10, 60, ARCS_ALL, 0},
    {"Medium Ballista", 200, 6, 50, 0, 10, 4, 6, 1, 10, 14, 100, 50, 10, 60, ARCS_ALL, 0},
    {"Large Ballista", 1000, 10, 30, 0, 12, 6, 9, 1, 10, 16, 100, 50, 10, 60, ARCS_ALL, 0},
    {"Small Catapult", 1000, 10, 30, 4, 15, 2, 3, 4, 160, 20, 100, 100, 2, 60, ARCS_ENDS,
     VESSEL_WEAPON_BALLISTIC},
    {"Medium Catapult", 1600, 13, 20, 5, 20, 2, 4, 5, 260, 20, 100, 100, 2, 60, ARCS_ENDS,
     VESSEL_WEAPON_BALLISTIC},
    {"Large Catapult", 2400, 17, 12, 6, 25, 2, 5, 6, 360, 20, 100, 100, 2, 60, ARCS_ENDS,
     VESSEL_WEAPON_BALLISTIC},
    {"Heavy Ballista", 2000, 15, 6, 0, 4, 15, 22, 1, 10, 0, 100, 0, 15, 60, ARCS_BEAMS, 0},
    {"Light Beamcannon", 8000, 7, 40, 0, 20, 4, 16, 1, 10, 10, 100, 30, 15, 90, ARCS_ALL,
     VESSEL_WEAPON_RANGE_DAMAGE | VESSEL_WEAPON_CAPITAL},
    {"Heavy Beamcannon", 10000, 9, 40, 0, 23, 5, 22, 1, 10, 10, 100, 30, 15, 90, ARCS_ALL,
     VESSEL_WEAPON_RANGE_DAMAGE | VESSEL_WEAPON_CAPITAL},
    {"Mind Blast Cannon", 8000, 5, 50, 0, 20, 0, 0, 1, 360, 0, 0, 0, 0, 90, ARCS_ALL,
     VESSEL_WEAPON_CREW_STUN | VESSEL_WEAPON_CAPITAL},
    {"Fragmentation Cannon", 10000, 7, 20, 0, 16, 4, 6, 5, 90, 50, 50, 100, 0, 90, ARCS_ENDS,
     VESSEL_WEAPON_CAPITAL},
    {"Long Tom Catapult", 10000, 9, 6, 12, 32, 3, 6, 8, 360, 20, 100, 100, 3, 90, ARCS_ENDS,
     VESSEL_WEAPON_BALLISTIC | VESSEL_WEAPON_CAPITAL}};

static const char *const equipment_names[NUM_VESSEL_EQUIPMENT] = {"Unknown Equipment", "Ram",
                                                                  "Neutral Colors"};

/**
 * A catalogue weapon, or NULL for anything outside the catalogue.
 */
const struct vessel_weapon_type *vessel_weapon_type(int weapon)
{
  if (weapon <= VESSEL_WEAPON_NONE || weapon >= NUM_VESSEL_WEAPONS)
  {
    return NULL;
  }
  return &weapon_types[weapon];
}

/**
 * The catalogue weapon a slot holds, or NULL when it holds none.
 */
const struct vessel_weapon_type *vessel_slot_weapon(const struct greyhawk_ship_slot *slot)
{
  if (slot == NULL || slot->type != VESSEL_SLOT_WEAPON)
  {
    return NULL;
  }
  return vessel_weapon_type(slot->item);
}

/** What a slot holds, for messages. */
const char *vessel_slot_name(const struct greyhawk_ship_slot *slot)
{
  if (slot == NULL)
  {
    return "an empty slot";
  }
  if (slot->type == VESSEL_SLOT_WEAPON)
  {
    return vessel_weapon_type(slot->item) != NULL ? weapon_types[slot->item].name
                                                  : weapon_types[VESSEL_WEAPON_NONE].name;
  }
  if (slot->type == VESSEL_SLOT_EQUIPMENT)
  {
    return slot->item < NUM_VESSEL_EQUIPMENT ? equipment_names[slot->item]
                                             : equipment_names[VESSEL_EQUIPMENT_NONE];
  }
  return "an empty slot";
}

/**
 * Weight a slot puts on the hull: the weapon's, or the ram's
 * (hull weight + 10) / 24; neutral colors weigh nothing.
 */
int vessel_slot_weight(const struct greyhawk_ship_data *ship, const struct greyhawk_ship_slot *slot)
{
  const struct vessel_weapon_type *weapon;

  if (ship == NULL || slot == NULL)
  {
    return 0;
  }
  weapon = vessel_slot_weapon(slot);
  if (weapon != NULL)
  {
    return weapon->weight;
  }
  if (slot->type == VESSEL_SLOT_EQUIPMENT && slot->item == VESSEL_EQUIPMENT_RAM)
  {
    return (vessel_class_handling(ship->vessel_type)->hull_weight + 10) / 24;
  }
  return 0;
}

/** An arc's name for weapon mounts: fore, port, rear, or starboard. */
const char *vessel_arc_name(int arc)
{
  switch (arc)
  {
  case GREYHAWK_PORT:
    return "port";
  case GREYHAWK_REAR:
    return "rear";
  case GREYHAWK_STARBOARD:
    return "starboard";
  default:
    return "fore";
  }
}

/**
 * The weapon a class carries by default (3.3.10): large ballistae on a
 * warship, a medium ballista on the other armed classes, none on a raft or
 * boat.
 */
int vessel_default_weapon(enum vessel_class vessel_type)
{
  switch (vessel_type)
  {
  case VESSEL_RAFT:
  case VESSEL_BOAT:
    return VESSEL_WEAPON_NONE;
  case VESSEL_WARSHIP:
    return VESSEL_WEAPON_LARGE_BALLISTA;
  default:
    return VESSEL_WEAPON_MEDIUM_BALLISTA;
  }
}

/** Mount a catalogue weapon on an arc, loaded, undamaged and ready. */
void vessel_set_weapon(struct greyhawk_ship_slot *slot, int weapon, int arc)
{
  if (slot == NULL)
  {
    return;
  }
  memset(slot, 0, sizeof(*slot));
  if (vessel_weapon_type(weapon) == NULL)
  {
    return;
  }
  slot->type = VESSEL_SLOT_WEAPON;
  slot->item = (unsigned char)weapon;
  slot->position = (char)arc;
  slot->ammo = (unsigned char)weapon_types[weapon].ammo;
}

/**
 * Give a new hull its class armament: a warship's large ballistae on the
 * bow and both beams, one medium ballista on the bow of the other armed
 * classes.
 */
void vessel_fit_default_weapons(struct greyhawk_ship_data *ship)
{
  int weapon;

  if (ship == NULL)
  {
    return;
  }
  memset(ship->slot, 0, sizeof(ship->slot));
  weapon = vessel_default_weapon(ship->vessel_type);
  if (weapon == VESSEL_WEAPON_NONE)
  {
    return;
  }
  vessel_set_weapon(&ship->slot[0], weapon, GREYHAWK_FORE);
  if (ship->vessel_type == VESSEL_WARSHIP)
  {
    vessel_set_weapon(&ship->slot[1], weapon, GREYHAWK_PORT);
    vessel_set_weapon(&ship->slot[2], weapon, GREYHAWK_STARBOARD);
  }
}
