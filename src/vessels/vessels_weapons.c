/* ************************************************************************
 *      File:   vessels_weapons.c                     Part of LuminariMUD  *
 *   Purpose:   Vessel weapon and equipment catalogue, class fitting,     *
 *              and the shipyard (vessels-ships study S4, 3.3.1, 3.3.4)   *
 * ********************************************************************** */

/*
 * The twelve DurisMUD weapons and its ram and neutral colors, at 2 gold per
 * platinum, with reloads in 0.5 s vessel ticks. A slot names its catalogue
 * row; everything else about the weapon comes from here. Each class mounts
 * what its Duris analog may (ship_allowed_weapons[]), within the analog's
 * mounts and weight cap per arc and its weight budget.
 */

#include "conf.h"
#include "core/sysdep.h"
#include "core/structs.h"
#include "core/utils.h"
#include "core/comm.h"
#include "core/interpreter.h"
#include "core/helpers.h"
#include "character/rewards.h"
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
     * hull/sail percent, pierce, reload, arcs, flags, capital renown */
    {"Unknown Weapon", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {"Small Ballista", 100, 3, 60, 0, 8, 2, 4, 1, 10, 12, 100, 50, 10, 34, ARCS_ALL, 0, 0},
    {"Medium Ballista", 200, 6, 50, 0, 10, 4, 6, 1, 10, 14, 100, 50, 10, 34, ARCS_ALL, 0, 0},
    {"Large Ballista", 1000, 10, 30, 0, 12, 6, 9, 1, 10, 16, 100, 50, 10, 34, ARCS_ALL, 0, 0},
    {"Small Catapult", 1000, 10, 30, 4, 15, 2, 3, 4, 160, 20, 100, 100, 2, 34, ARCS_ENDS,
     VESSEL_WEAPON_BALLISTIC, 0},
    {"Medium Catapult", 1600, 13, 20, 5, 20, 2, 4, 5, 260, 20, 100, 100, 2, 34, ARCS_ENDS,
     VESSEL_WEAPON_BALLISTIC, 0},
    {"Large Catapult", 2400, 17, 12, 6, 25, 2, 5, 6, 360, 20, 100, 100, 2, 34, ARCS_ENDS,
     VESSEL_WEAPON_BALLISTIC, 0},
    {"Heavy Ballista", 2000, 15, 6, 0, 4, 15, 22, 1, 10, 0, 100, 0, 15, 34, ARCS_BEAMS, 0, 0},
    {"Light Beamcannon", 8000, 7, 40, 0, 20, 4, 16, 1, 10, 10, 100, 30, 15, 51, ARCS_ALL,
     VESSEL_WEAPON_RANGE_DAMAGE | VESSEL_WEAPON_CAPITAL, 1600},
    {"Heavy Beamcannon", 10000, 9, 40, 0, 23, 5, 22, 1, 10, 10, 100, 30, 15, 51, ARCS_ALL,
     VESSEL_WEAPON_RANGE_DAMAGE | VESSEL_WEAPON_CAPITAL, 1800},
    {"Mind Blast Cannon", 8000, 5, 50, 0, 20, 0, 0, 1, 360, 0, 0, 0, 0, 51, ARCS_ALL,
     VESSEL_WEAPON_CREW_STUN | VESSEL_WEAPON_CAPITAL, 1700},
    {"Fragmentation Cannon", 10000, 7, 20, 0, 16, 4, 6, 5, 90, 50, 50, 100, 0, 51, ARCS_ENDS,
     VESSEL_WEAPON_CAPITAL, 1900},
    {"Long Tom Catapult", 10000, 9, 6, 12, 32, 3, 6, 8, 360, 20, 100, 100, 3, 51, ARCS_ENDS,
     VESSEL_WEAPON_BALLISTIC | VESSEL_WEAPON_CAPITAL, 2000}};

static const char *const equipment_names[NUM_VESSEL_EQUIPMENT] = {"Unknown Equipment", "Ram",
                                                                  "Neutral Colors"};

#define WEAPON_BIT(weapon) (1 << (weapon))
#define WEAPONS_ALL ((1 << NUM_VESSEL_WEAPONS) - 2)
#define WEAPONS_LIGHT                                                                              \
  (WEAPON_BIT(VESSEL_WEAPON_SMALL_BALLISTA) | WEAPON_BIT(VESSEL_WEAPON_MEDIUM_BALLISTA) |          \
   WEAPON_BIT(VESSEL_WEAPON_LARGE_BALLISTA) | WEAPON_BIT(VESSEL_WEAPON_SMALL_CATAPULT) |           \
   WEAPON_BIT(VESSEL_WEAPON_MEDIUM_CATAPULT) | WEAPON_BIT(VESSEL_WEAPON_LIGHT_BEAMCANNON) |        \
   WEAPON_BIT(VESSEL_WEAPON_MIND_BLAST) | WEAPON_BIT(VESSEL_WEAPON_FRAGMENTATION))

/* A class's armament (3.3.1): its Duris analog's mounts and weapon weight cap
 * per arc (GREYHAWK_FORE order) and the weapons it may mount. */
struct vessel_class_fitting
{
  int mounts[VESSEL_NUM_ARCS];
  int arc_weight[VESSEL_NUM_ARCS];
  int weapons; /* WEAPON_BIT() of each weapon the class may mount */
};

/* Class order follows enum vessel_class. */
static const struct vessel_class_fitting class_fitting[NUM_VESSEL_TYPES] = {
    {{0, 0, 0, 0}, {0, 0, 0, 0}, 0},                                        /* RAFT: sloop */
    {{1, 1, 1, 1}, {3, 5, 3, 5}, WEAPON_BIT(VESSEL_WEAPON_SMALL_BALLISTA)}, /* BOAT: yacht */
    {{1, 3, 1, 3},
     {17, 26, 17, 26},
     WEAPONS_ALL & ~(WEAPON_BIT(VESSEL_WEAPON_HEAVY_BEAMCANNON) |
                     WEAPON_BIT(VESSEL_WEAPON_LONG_TOM))}, /* SHIP: caravel */
    {{2, 3, 2, 3}, {31, 44, 31, 44}, WEAPONS_ALL},         /* WARSHIP: frigate */
    {{1, 3, 1, 3}, {13, 32, 13, 32}, WEAPONS_LIGHT},       /* AIRSHIP: corvette */
    {{2, 0, 1, 0},
     {27, 0, 27, 0},
     WEAPONS_ALL & ~WEAPON_BIT(VESSEL_WEAPON_LONG_TOM)}, /* SUBMARINE: destroyer */
    {{2, 3, 1, 3}, {26, 35, 26, 35}, WEAPONS_ALL},       /* TRANSPORT: galleon */
    {{2, 4, 2, 4}, {35, 50, 35, 50}, WEAPONS_ALL}        /* MAGICAL: cruiser */
};

static const struct vessel_class_fitting *vessel_class_fitting(enum vessel_class vessel_type)
{
  if (vessel_type < 0 || vessel_type >= NUM_VESSEL_TYPES)
  {
    vessel_type = VESSEL_SHIP;
  }
  return &class_fitting[vessel_type];
}

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
  slot->position = (unsigned char)arc;
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

/** Gold price of fitting equipment to a hull: a ram costs 2 gold per hull weight. */
int vessel_equipment_price(int equipment, enum vessel_class vessel_type)
{
  if (equipment == VESSEL_EQUIPMENT_RAM)
  {
    return 2 * vessel_class_handling(vessel_type)->hull_weight;
  }
  return 0;
}

/**
 * Why a hull's fit-out is illegal, or NULL when it is legal: every weapon
 * allowed on the class and on its arc, no arc beyond its mounts or weight
 * cap, at most one capital weapon, each equipment allowed and fitted once,
 * and the whole fit-out within the class weight budget.
 */
const char *vessel_fitout_problem(const struct greyhawk_ship_data *ship)
{
  static char problem[MAX_STRING_LENGTH];
  const struct vessel_class_fitting *fitting;
  const struct vessel_weapon_type *weapon;
  const struct greyhawk_ship_slot *slot;
  int count[VESSEL_NUM_ARCS];
  int weight[VESSEL_NUM_ARCS];
  int equipment[NUM_VESSEL_EQUIPMENT];
  int capitals;
  int total;
  int arc;
  int i;

  if (ship == NULL)
  {
    return NULL;
  }

  fitting = vessel_class_fitting(ship->vessel_type);
  memset(count, 0, sizeof(count));
  memset(weight, 0, sizeof(weight));
  memset(equipment, 0, sizeof(equipment));
  capitals = 0;
  total = 0;
  for (i = 0; i < GREYHAWK_MAXSLOTS; i++)
  {
    slot = &ship->slot[i];
    total += vessel_slot_weight(ship, slot);
    if (slot->type == VESSEL_SLOT_EQUIPMENT)
    {
      if (slot->item <= VESSEL_EQUIPMENT_NONE || slot->item >= NUM_VESSEL_EQUIPMENT)
      {
        snprintf(problem, sizeof(problem), "Slot %d holds equipment the shipyard does not know.",
                 i);
        return problem;
      }
      if (slot->item == VESSEL_EQUIPMENT_RAM &&
          (ship->vessel_type == VESSEL_RAFT || ship->vessel_type == VESSEL_BOAT))
      {
        snprintf(problem, sizeof(problem), "%s cannot carry a ram.", ship->name);
        return problem;
      }
      if (++equipment[slot->item] > 1)
      {
        snprintf(problem, sizeof(problem), "She may carry only one %s.", vessel_slot_name(slot));
        return problem;
      }
      continue;
    }
    weapon = vessel_slot_weapon(slot);
    if (weapon == NULL)
    {
      continue;
    }
    arc = slot->position;
    if (!IS_SET(fitting->weapons, WEAPON_BIT(slot->item)))
    {
      snprintf(problem, sizeof(problem), "%s cannot mount a %s.", ship->name, weapon->name);
      return problem;
    }
    if (arc < 0 || arc >= VESSEL_NUM_ARCS || !IS_SET(weapon->arcs, ARC_BIT(arc)))
    {
      snprintf(problem, sizeof(problem), "A %s cannot be mounted on the %s arc.", weapon->name,
               vessel_arc_name(arc));
      return problem;
    }
    count[arc]++;
    weight[arc] += weapon->weight;
    if (IS_SET(weapon->flags, VESSEL_WEAPON_CAPITAL))
    {
      capitals++;
    }
  }

  for (arc = 0; arc < VESSEL_NUM_ARCS; arc++)
  {
    if (count[arc] > fitting->mounts[arc])
    {
      snprintf(problem, sizeof(problem), "%s mounts at most %d weapon%s on the %s arc.", ship->name,
               fitting->mounts[arc], fitting->mounts[arc] == 1 ? "" : "s", vessel_arc_name(arc));
      return problem;
    }
    if (weight[arc] > fitting->arc_weight[arc])
    {
      snprintf(problem, sizeof(problem),
               "%s carries at most %d weight of weapons on the %s arc; this fit puts %d there.",
               ship->name, fitting->arc_weight[arc], vessel_arc_name(arc), weight[arc]);
      return problem;
    }
  }
  if (capitals > 1)
  {
    snprintf(problem, sizeof(problem), "She may carry only one capital weapon.");
    return problem;
  }
  if (total > vessel_class_handling(ship->vessel_type)->max_load)
  {
    snprintf(problem, sizeof(problem), "The fit-out would weigh %d, more than the %d %s can carry.",
             total, vessel_class_handling(ship->vessel_type)->max_load, ship->name);
    return problem;
  }
  return NULL;
}

/**
 * Keep the hull at the shipyard for the shipwrights' work; immortals skip
 * it, as Duris's trusted characters do.
 */
void vessel_add_maintenance(struct greyhawk_ship_data *ship, struct char_data *ch, int ticks)
{
  if (ship == NULL || ticks <= 0 || (ch != NULL && !IS_NPC(ch) && GET_LEVEL(ch) >= LVL_IMMORT))
  {
    return;
  }
  ship->maintenance_ticks = (short int)MIN(SHRT_MAX, ship->maintenance_ticks + ticks);
}

/** The first empty slot, or -1. */
int vessel_free_slot(const struct greyhawk_ship_data *ship)
{
  int i;

  for (i = 0; i < GREYHAWK_MAXSLOTS; i++)
  {
    if (ship->slot[i].type == VESSEL_SLOT_EMPTY)
    {
      return i;
    }
  }
  return -1;
}

/** A catalogue weapon by number (1-12) or name prefix, or VESSEL_WEAPON_NONE. */
static int vessel_weapon_by_name(const char *arg)
{
  int weapon;

  if (arg == NULL || !*arg)
  {
    return VESSEL_WEAPON_NONE;
  }
  if (isdigit((unsigned char)*arg))
  {
    weapon = parse_int(arg);
    return vessel_weapon_type(weapon) != NULL ? weapon : VESSEL_WEAPON_NONE;
  }
  for (weapon = VESSEL_WEAPON_NONE + 1; weapon < NUM_VESSEL_WEAPONS; weapon++)
  {
    if (is_abbrev(arg, weapon_types[weapon].name))
    {
      return weapon;
    }
  }
  return VESSEL_WEAPON_NONE;
}

/** An arc by name (fore, port, rear, starboard, or a prefix), or -1. */
int vessel_arc_by_name(const char *arg)
{
  int arc;

  if (arg == NULL || !*arg)
  {
    return -1;
  }
  for (arc = 0; arc < VESSEL_NUM_ARCS; arc++)
  {
    if (is_abbrev(arg, vessel_arc_name(arc)))
    {
      return arc;
    }
  }
  return -1;
}

/** A slot number argument, or -1 when it names no slot. */
static int vessel_slot_argument(const char *arg)
{
  int slot;

  if (arg == NULL || !isdigit((unsigned char)*arg))
  {
    return -1;
  }
  slot = parse_int(arg);
  return slot >= 0 && slot < GREYHAWK_MAXSLOTS ? slot : -1;
}

static void vessel_show_weapon_catalogue(struct char_data *ch, struct greyhawk_ship_data *ship)
{
  const struct vessel_class_fitting *fitting;
  const struct vessel_weapon_type *weapon;
  char damage[32];
  char capital[32];
  int count[VESSEL_NUM_ARCS];
  int weight[VESSEL_NUM_ARCS];
  int total;
  int arc;
  int i;

  fitting = vessel_class_fitting(ship->vessel_type);
  send_to_char(ch, "Weapons the shipwrights can mount on %s (%s):\r\n", ship->name,
               get_vessel_type_name(ship->vessel_type));
  send_to_char(ch, " #  %-20s %6s %3s %4s %5s %-8s %6s  %s\r\n", "Weapon", "Gold", "Wt", "Ammo",
               "Range", "Damage", "Reload", "Arcs");
  for (i = VESSEL_WEAPON_NONE + 1; i < NUM_VESSEL_WEAPONS; i++)
  {
    weapon = &weapon_types[i];
    if (IS_SET(weapon->flags, VESSEL_WEAPON_CREW_STUN))
    {
      snprintf(damage, sizeof(damage), "stun");
    }
    else if (IS_SET(weapon->flags, VESSEL_WEAPON_RANGE_DAMAGE))
    {
      snprintf(damage, sizeof(damage), "%d to %d", weapon->max_damage, weapon->min_damage);
    }
    else if (weapon->fragments > 1)
    {
      snprintf(damage, sizeof(damage), "%dx %d-%d", weapon->fragments, weapon->min_damage,
               weapon->max_damage);
    }
    else
    {
      snprintf(damage, sizeof(damage), "%d-%d", weapon->min_damage, weapon->max_damage);
    }
    *capital = '\0';
    if (IS_SET(weapon->flags, VESSEL_WEAPON_CAPITAL))
    {
      snprintf(capital, sizeof(capital), ", capital %d", weapon->renown);
    }
    send_to_char(ch, "%2d  %-20s %6d %3d %4d %2d-%-2d %-8s %5ds  %s%s%s\r\n", i, weapon->name,
                 weapon->price, weapon->weight, weapon->ammo, weapon->min_range, weapon->max_range,
                 damage, weapon->reload / 2,
                 weapon->arcs == ARCS_ALL ? "all" : (weapon->arcs == ARCS_ENDS ? "ends" : "beams"),
                 capital, IS_SET(fitting->weapons, WEAPON_BIT(i)) ? "" : " (not on this hull)");
  }

  memset(count, 0, sizeof(count));
  memset(weight, 0, sizeof(weight));
  total = 0;
  for (i = 0; i < GREYHAWK_MAXSLOTS; i++)
  {
    total += vessel_slot_weight(ship, &ship->slot[i]);
    weapon = vessel_slot_weapon(&ship->slot[i]);
    arc = ship->slot[i].position;
    if (weapon != NULL && arc >= 0 && arc < VESSEL_NUM_ARCS)
    {
      count[arc]++;
      weight[arc] += weapon->weight;
    }
  }
  send_to_char(ch, "Arcs (weapons/mounts, weight/cap):");
  for (arc = 0; arc < VESSEL_NUM_ARCS; arc++)
  {
    send_to_char(ch, "%s %s %d/%d, %d/%d", arc > 0 ? ";" : "", vessel_arc_name(arc), count[arc],
                 fitting->mounts[arc], weight[arc], fitting->arc_weight[arc]);
  }
  send_to_char(ch,
               "\r\nFit-out weight %d of %d. Capital weapons: one per hull, with the renown "
               "shown (%s has %d) or a veteran gunner.\r\n",
               total, vessel_class_handling(ship->vessel_type)->max_load, ship->name, ship->renown);
}

static void vessel_buy_weapon(struct char_data *ch, struct greyhawk_ship_data *ship,
                              const char *argument)
{
  const struct vessel_weapon_type *weapon;
  const char *problem;
  char name[MAX_INPUT_LENGTH];
  char capital[128];
  char *arc_word;
  int weapon_id;
  int slot;
  int arc;
  int i;

  /* The last word is the arc; everything before it names the weapon. */
  skip_spaces_c(&argument);
  strlcpy(name, argument, sizeof(name));
  for (i = (int)strlen(name); i > 0 && isspace((unsigned char)name[i - 1]); i--)
  {
    name[i - 1] = '\0';
  }
  arc_word = strrchr(name, ' ');
  if (arc_word == NULL)
  {
    send_to_char(ch,
                 "Usage: shipweapon buy <weapon number or name> <fore|port|rear|starboard>\r\n");
    return;
  }
  *arc_word++ = '\0';
  weapon_id = vessel_weapon_by_name(name);
  arc = vessel_arc_by_name(arc_word);
  weapon = vessel_weapon_type(weapon_id);
  if (weapon == NULL || arc < 0)
  {
    send_to_char(ch,
                 "Usage: shipweapon buy <weapon number or name> <fore|port|rear|starboard>\r\n");
    return;
  }

  slot = vessel_free_slot(ship);
  if (slot < 0)
  {
    send_to_char(ch, "Every slot aboard %s is taken.\r\n", ship->name);
    return;
  }
  vessel_set_weapon(&ship->slot[slot], weapon_id, arc);
  problem = vessel_fitout_problem(ship);
  if (problem == NULL && IS_SET(weapon->flags, VESSEL_WEAPON_CAPITAL) &&
      ship->renown < weapon->renown && ship->crew_tier[CREW_GUNNER] < CREW_TIER_VETERAN)
  {
    snprintf(capital, sizeof(capital),
             "A %s is mounted only on a hull of %d renown or with a veteran gunner.", weapon->name,
             weapon->renown);
    problem = capital;
  }
  if (problem == NULL && GET_GOLD(ch) < weapon->price)
  {
    send_to_char(ch, "A %s costs %d gold; you have %d.\r\n", weapon->name, weapon->price,
                 GET_GOLD(ch));
    memset(&ship->slot[slot], 0, sizeof(ship->slot[slot]));
    return;
  }
  if (problem != NULL)
  {
    send_to_char(ch, "%s\r\n", problem);
    memset(&ship->slot[slot], 0, sizeof(ship->slot[slot]));
    return;
  }

  award_gold(ch, -weapon->price);
  vessel_add_maintenance(ship, ch, weapon->weight * VESSEL_INSTALL_TICKS_PER_WEIGHT);
  vessel_db_save_weapons(ship);
  vessel_db_save_runtime(ship);
  send_to_char(ch,
               "The shipwrights mount a %s on the %s arc of %s in slot %d for %d gold, loaded "
               "with %d rounds.\r\n",
               weapon->name, vessel_arc_name(arc), ship->name, slot, weapon->price, weapon->ammo);
  if (ship->maintenance_ticks > 0)
  {
    send_to_char(ch, "She cannot sail for %d seconds while they work.\r\n",
                 (ship->maintenance_ticks + 1) / 2);
  }
  log("Info: %s mounted a %s on ship %d slot %d for %d gold", GET_NAME(ch), weapon->name,
      ship->shipnum, slot, weapon->price);
}

/**
 * shipweapon [list | buy <weapon> <arc> | sell <slot> | swap <slot> <slot>]
 * - the shipyard's weapons (3.3.4), for the owner of a hull in port.
 */
ACMD(do_shipweapon)
{
  struct greyhawk_ship_data *ship;
  struct greyhawk_ship_slot swap;
  const struct vessel_weapon_type *weapon;
  char command[MAX_INPUT_LENGTH];
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  int slot;
  int other;
  int value;

  ship = vessel_refit_ship(ch);
  if (ship == NULL)
  {
    return;
  }

  argument = one_argument(argument, command, sizeof(command));
  if (!*command || is_abbrev(command, "list"))
  {
    vessel_show_weapon_catalogue(ch, ship);
    return;
  }
  if (is_abbrev(command, "buy"))
  {
    vessel_buy_weapon(ch, ship, argument);
    return;
  }

  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
  slot = vessel_slot_argument(arg1);
  if (is_abbrev(command, "sell"))
  {
    weapon = slot >= 0 ? vessel_slot_weapon(&ship->slot[slot]) : NULL;
    if (weapon == NULL)
    {
      send_to_char(ch, "Usage: shipweapon sell <slot holding a weapon>\r\n");
      return;
    }
    value = vessel_slot_sale_value(&ship->slot[slot], ship->vessel_type);
    award_gold(ch, value);
    memset(&ship->slot[slot], 0, sizeof(ship->slot[slot]));
    vessel_db_save_weapons(ship);
    send_to_char(ch, "The shipwrights take the %s out of slot %d and pay you %d gold.\r\n",
                 weapon->name, slot, value);
    log("Info: %s sold the %s from ship %d slot %d for %d gold", GET_NAME(ch), weapon->name,
        ship->shipnum, slot, value);
    return;
  }
  if (is_abbrev(command, "swap"))
  {
    other = vessel_slot_argument(arg2);
    if (slot < 0 || other < 0 || slot == other)
    {
      send_to_char(ch, "Usage: shipweapon swap <slot> <other slot>, slots 0-%d\r\n",
                   GREYHAWK_MAXSLOTS - 1);
      return;
    }
    swap = ship->slot[slot];
    ship->slot[slot] = ship->slot[other];
    ship->slot[other] = swap;
    vessel_db_save_weapons(ship);
    send_to_char(ch, "Slots %d and %d trade places.\r\n", slot, other);
    return;
  }
  send_to_char(ch, "Usage: shipweapon [list | buy <weapon> <arc> | sell <slot> | swap <slot> "
                   "<slot>]\r\n");
}

/** The slot holding an equipment item, or -1. */
int vessel_equipment_slot(const struct greyhawk_ship_data *ship, int equipment)
{
  int i;

  for (i = 0; i < GREYHAWK_MAXSLOTS; i++)
  {
    if (ship->slot[i].type == VESSEL_SLOT_EQUIPMENT && ship->slot[i].item == equipment)
    {
      return i;
    }
  }
  return -1;
}

/**
 * What the shipwrights pay for a slot's weapon or equipment aboard a hull of
 * this class: 90% of the price, and 10% for a damaged weapon (Duris).
 */
int vessel_slot_sale_value(const struct greyhawk_ship_slot *slot, enum vessel_class vessel_type)
{
  const struct vessel_weapon_type *weapon;

  weapon = vessel_slot_weapon(slot);
  if (weapon != NULL)
  {
    return slot->damage > 0 ? weapon->price / 10 : weapon->price * 9 / 10;
  }
  if (slot->type == VESSEL_SLOT_EQUIPMENT)
  {
    return vessel_equipment_price(slot->item, vessel_type) * 9 / 10;
  }
  return 0;
}

/**
 * Carry a traded-in hull's weapons and equipment aboard her new hull (study
 * 3.3.7): each goes into a free slot if the fit-out stays legal, and the
 * shipwrights buy the rest.
 *
 * @return gold paid for what the new hull cannot take
 */
int vessel_carry_fitout(struct greyhawk_ship_data *ship, const struct greyhawk_ship_slot *old_slots,
                        enum vessel_class old_class)
{
  int paid;
  int slot;
  int i;

  paid = 0;
  for (i = 0; i < GREYHAWK_MAXSLOTS; i++)
  {
    if (old_slots[i].type == VESSEL_SLOT_EMPTY)
    {
      continue;
    }
    slot = vessel_free_slot(ship);
    if (slot >= 0)
    {
      ship->slot[slot] = old_slots[i];
      if (vessel_fitout_problem(ship) == NULL)
      {
        continue;
      }
      memset(&ship->slot[slot], 0, sizeof(ship->slot[slot]));
    }
    paid += vessel_slot_sale_value(&old_slots[i], old_class);
  }
  return paid;
}

/** Whether any bulk cargo lot aboard holds units. */
bool vessel_has_cargo(const struct greyhawk_ship_data *ship)
{
  int i;

  for (i = 0; i < MAX_CARGO_LOTS; i++)
  {
    if (ship->cargo[i].commodity_id > 0 && ship->cargo[i].quantity > 0)
    {
      return TRUE;
    }
  }
  return FALSE;
}

/**
 * shipequip [list | buy <ram|colors> | sell <ram|colors>] - the shipyard's
 * equipment (3.3.4), one of each, for the owner of a hull in port.
 */
ACMD(do_shipequip)
{
  struct greyhawk_ship_data *ship;
  struct greyhawk_ship_slot fitted;
  const char *problem;
  char command[MAX_INPUT_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  int equipment;
  int price;
  int slot;
  int i;

  ship = vessel_refit_ship(ch);
  if (ship == NULL)
  {
    return;
  }

  two_arguments(argument, command, sizeof(command), arg, sizeof(arg));
  if (!*command || is_abbrev(command, "list"))
  {
    send_to_char(ch, "Equipment the shipwrights can fit to %s (one of each):\r\n", ship->name);
    for (i = VESSEL_EQUIPMENT_NONE + 1; i < NUM_VESSEL_EQUIPMENT; i++)
    {
      memset(&fitted, 0, sizeof(fitted));
      fitted.type = VESSEL_SLOT_EQUIPMENT;
      fitted.item = (unsigned char)i;
      send_to_char(ch, "  %-15s %6d gold, weight %d%s\r\n", equipment_names[i],
                   vessel_equipment_price(i, ship->vessel_type), vessel_slot_weight(ship, &fitted),
                   vessel_equipment_slot(ship, i) >= 0 ? " (fitted)" : "");
    }
    return;
  }

  equipment = VESSEL_EQUIPMENT_NONE;
  if (*arg && is_abbrev(arg, "ram"))
  {
    equipment = VESSEL_EQUIPMENT_RAM;
  }
  else if (*arg && (is_abbrev(arg, "colors") || is_abbrev(arg, "neutral")))
  {
    equipment = VESSEL_EQUIPMENT_COLORS;
  }
  if (equipment == VESSEL_EQUIPMENT_NONE ||
      (!is_abbrev(command, "buy") && !is_abbrev(command, "sell")))
  {
    send_to_char(ch, "Usage: shipequip [list | buy <ram|colors> | sell <ram|colors>]\r\n");
    return;
  }
  price = vessel_equipment_price(equipment, ship->vessel_type);
  slot = vessel_equipment_slot(ship, equipment);

  if (is_abbrev(command, "sell"))
  {
    if (slot < 0)
    {
      send_to_char(ch, "%s carries no %s.\r\n", ship->name, equipment_names[equipment]);
      return;
    }
    /* Neutral colors cannot come down with cargo aboard (3.3.8). */
    if (equipment == VESSEL_EQUIPMENT_COLORS && vessel_has_cargo(ship))
    {
      send_to_char(ch, "Her neutral colors stay up while she has cargo aboard.\r\n");
      return;
    }
    price = vessel_slot_sale_value(&ship->slot[slot], ship->vessel_type);
    memset(&ship->slot[slot], 0, sizeof(ship->slot[slot]));
    award_gold(ch, price);
    vessel_db_save_weapons(ship);
    send_to_char(ch, "The shipwrights remove the %s and pay you %d gold.\r\n",
                 equipment_names[equipment], price);
    return;
  }

  if (slot >= 0)
  {
    send_to_char(ch, "%s already carries %s%s.\r\n", ship->name,
                 equipment == VESSEL_EQUIPMENT_COLORS ? "" : "a ", equipment_names[equipment]);
    return;
  }
  slot = vessel_free_slot(ship);
  if (slot < 0)
  {
    send_to_char(ch, "Every slot aboard %s is taken.\r\n", ship->name);
    return;
  }
  ship->slot[slot].type = VESSEL_SLOT_EQUIPMENT;
  ship->slot[slot].item = (unsigned char)equipment;
  problem = vessel_fitout_problem(ship);
  if (problem != NULL || GET_GOLD(ch) < price)
  {
    if (problem != NULL)
    {
      send_to_char(ch, "%s\r\n", problem);
    }
    else
    {
      send_to_char(ch, "A %s costs %d gold; you have %d.\r\n", equipment_names[equipment], price,
                   GET_GOLD(ch));
    }
    memset(&ship->slot[slot], 0, sizeof(ship->slot[slot]));
    return;
  }

  award_gold(ch, -price);
  vessel_add_maintenance(
      ship, ch, vessel_slot_weight(ship, &ship->slot[slot]) * VESSEL_INSTALL_TICKS_PER_WEIGHT);
  vessel_db_save_weapons(ship);
  vessel_db_save_runtime(ship);
  send_to_char(ch, "The shipwrights fit %s with %s%s for %d gold.\r\n", ship->name,
               equipment == VESSEL_EQUIPMENT_COLORS ? "" : "a ", equipment_names[equipment], price);
  if (ship->maintenance_ticks > 0)
  {
    send_to_char(ch, "She cannot sail for %d seconds while they work.\r\n",
                 (ship->maintenance_ticks + 1) / 2);
  }
}

/**
 * shiprearm [slot | all] - refill weapons at 2 gold a round, 75 s of the
 * shipwrights' time per weapon (3.3.4). A destroyed weapon is not rearmed.
 */
ACMD(do_shiprearm)
{
  struct greyhawk_ship_data *ship;
  const struct vessel_weapon_type *weapon;
  char arg[MAX_INPUT_LENGTH];
  int full[GREYHAWK_MAXSLOTS]; /* rounds each chosen weapon is refilled to; 0 if not chosen */
  int weapons;
  int cost;
  int slot;
  int i;

  ship = vessel_refit_ship(ch);
  if (ship == NULL)
  {
    return;
  }

  one_argument(argument, arg, sizeof(arg));
  slot = vessel_slot_argument(arg);
  if (*arg && slot < 0 && !is_abbrev(arg, "all"))
  {
    send_to_char(ch, "Usage: shiprearm [slot | all]\r\n");
    return;
  }

  weapons = 0;
  cost = 0;
  for (i = 0; i < GREYHAWK_MAXSLOTS; i++)
  {
    weapon = vessel_slot_weapon(&ship->slot[i]);
    full[i] = 0;
    if (weapon != NULL && (slot < 0 || slot == i) && ship->slot[i].ammo < weapon->ammo &&
        ship->slot[i].damage < VESSEL_WEAPON_DESTROYED)
    {
      full[i] = weapon->ammo;
      weapons++;
      cost += (weapon->ammo - ship->slot[i].ammo) * VESSEL_ROUND_PRICE;
    }
  }
  if (weapons == 0)
  {
    send_to_char(ch, "There is nothing to rearm.\r\n");
    return;
  }
  if (GET_GOLD(ch) < cost)
  {
    send_to_char(ch, "Rearming costs %d gold; you have %d.\r\n", cost, GET_GOLD(ch));
    return;
  }

  award_gold(ch, -cost);
  for (i = 0; i < GREYHAWK_MAXSLOTS; i++)
  {
    if (full[i] > 0)
    {
      ship->slot[i].ammo = (unsigned char)full[i];
    }
  }
  vessel_add_maintenance(ship, ch, weapons * VESSEL_REARM_TICKS);
  vessel_db_save_weapons(ship);
  vessel_db_save_runtime(ship);
  send_to_char(ch, "The shipwrights rearm %d weapon%s for %d gold.\r\n", weapons,
               weapons == 1 ? "" : "s", cost);
  if (ship->maintenance_ticks > 0)
  {
    send_to_char(ch, "She cannot sail for %d seconds while they work.\r\n",
                 (ship->maintenance_ticks + 1) / 2);
  }
}
