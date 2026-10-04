/* ************************************************************************
 *      File:   vessels_crew.c                        Part of LuminariMUD  *
 *   Purpose:   Hired crew (Phase 06, Session 03; vessels-ships S5).       *
 *              Crew fill four positions at three quality tiers for a      *
 *              one-time hire price, learn from their work, and earn       *
 *              promotion; a lost hull costs them experience.              *
 * ********************************************************************** */

#include "conf.h"
#include "core/sysdep.h"
#include <math.h> /* before utils.h, which defines log() as a macro */
#include "core/structs.h"
#include "core/utils.h"
#include "core/comm.h"
#include "core/db.h"
#include "core/handler.h"
#include "character/rewards.h"
#include "core/interpreter.h"
#include "vessels.h"
#include "database/mysql.h"

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/* Crew rows live in ship_crew_roster with npc_vnum encoding the position so
 * they never collide with the pilot record or the helm permits
 * (npc_vnum -1). */
#define CREW_ROW_VNUM_BASE (-100)

/**
 * Display name for a hireable position.
 */
const char *vessel_crew_position_name(int position)
{
  switch (position)
  {
  case CREW_SAILMASTER:
    return "sailmaster";
  case CREW_GUNNER:
    return "gunner";
  case CREW_BOSUN:
    return "bosun";
  case CREW_QUARTERMASTER:
    return "quartermaster";
  default:
    return "unknown";
  }
}

/**
 * Display name for a quality tier.
 */
const char *vessel_crew_tier_name(int tier)
{
  switch (tier)
  {
  case CREW_TIER_GREEN:
    return "green";
  case CREW_TIER_ABLE:
    return "able";
  case CREW_TIER_VETERAN:
    return "veteran";
  default:
    return "unfilled";
  }
}

/**
 * Map a position name to its index.
 *
 * @return Position index, or -1 if unrecognized
 */
static int vessel_crew_position_by_name(const char *name)
{
  int i;

  if (name == NULL || !*name)
  {
    return -1;
  }

  for (i = 0; i < NUM_CREW_POSITIONS; i++)
  {
    if (is_abbrev(name, vessel_crew_position_name(i)))
    {
      return i;
    }
  }

  return -1;
}

/**
 * Map a tier name to its index.
 *
 * @return Tier index, or -1 if unrecognized
 */
static int vessel_crew_tier_by_name(const char *name)
{
  int i;

  if (name == NULL || !*name)
  {
    return -1;
  }

  for (i = CREW_TIER_GREEN; i <= CREW_TIER_VETERAN; i++)
  {
    if (is_abbrev(name, vessel_crew_tier_name(i)))
    {
      return i;
    }
  }

  return -1;
}

/**
 * One-time price to hire a crew member; crew draw no wages.
 *
 * DurisMUD chief prices at 2 gold per platinum (vessels-ships study 3.3.5).
 */
int vessel_crew_hire_cost(int position, int tier)
{
  static const int hire_cost[NUM_CREW_POSITIONS][CREW_TIER_VETERAN] = {
      {1600, 6000, 15000}, /* sailmaster */
      {2400, 8000, 18000}, /* gunner */
      {2000, 7000, 16000}, /* bosun */
      {1200, 4500, 11000}  /* quartermaster */
  };

  if (position < 0 || position >= NUM_CREW_POSITIONS || tier < CREW_TIER_GREEN ||
      tier > CREW_TIER_VETERAN)
  {
    return 0;
  }

  return hire_cost[position][tier - CREW_TIER_GREEN];
}

/**
 * Renown a hull needs before a hand of this tier, of a known position, signs
 * on (study 3.3.5); green hands need none.
 */
int vessel_crew_hire_renown(int position, int tier)
{
  static const int hire_renown[NUM_CREW_POSITIONS][CREW_TIER_VETERAN - CREW_TIER_GREEN] = {
      {540, 1350}, /* sailmaster */
      {700, 1640}, /* gunner */
      {640, 1480}, /* bosun */
      {540, 1350}  /* quartermaster */
  };

  return tier > CREW_TIER_GREEN ? hire_renown[position][tier - CREW_TIER_ABLE] : 0;
}

/**
 * Experience at which a position reaches a tier, in Duris skill points
 * (study 3.3.5); a hand starts at the floor of the tier hired.
 */
double vessel_crew_floor(int position, int tier)
{
  static const int floor_points[NUM_CREW_POSITIONS][CREW_TIER_VETERAN] = {
      {200, 800, 2000},  /* sailmaster */
      {250, 1000, 2500}, /* gunner */
      {220, 900, 2200},  /* bosun */
      {200, 800, 2000}   /* quartermaster */
  };

  if (position < 0 || position >= NUM_CREW_POSITIONS || tier < CREW_TIER_GREEN ||
      tier > CREW_TIER_VETERAN)
  {
    return 0.0;
  }

  return (double)floor_points[position][tier - CREW_TIER_GREEN];
}

/**
 * Train a filled position. Reaching the next tier's floor promotes it; a
 * promotion pays no hiring gate.
 */
void vessel_crew_gain(struct greyhawk_ship_data *ship, int position, double amount)
{
  int tier;

  if (ship == NULL || position < 0 || position >= NUM_CREW_POSITIONS || amount <= 0.0 ||
      ship->crew_tier[position] == CREW_TIER_NONE)
  {
    return;
  }

  ship->crew_xp[position] += amount;
  tier = ship->crew_tier[position];
  while (tier < CREW_TIER_VETERAN &&
         ship->crew_xp[position] >= vessel_crew_floor(position, tier + 1))
  {
    tier++;
  }
  if (tier == ship->crew_tier[position])
  {
    return;
  }

  ship->crew_tier[position] = tier;
  vessel_apply_crew_bonuses(ship);
  vessel_db_save_crew(ship);
  send_to_ship(ship, "The %s has earned promotion to %s.", vessel_crew_position_name(position),
               vessel_crew_tier_name(tier));
}

/**
 * Every hand aboard the hull that sank target learns from it: the target's
 * hull weight for a player's hull, a tenth of that for an NPC hull. Such
 * gains are saved at once; the small ones of sailing and gunnery wait for the
 * next save.
 */
void vessel_crew_credit_kill(struct greyhawk_ship_data *victor,
                             const struct greyhawk_ship_data *target)
{
  double gain;
  int i;

  if (victor == NULL || target == NULL)
  {
    return;
  }

  gain = (double)vessel_class_handling(target->vessel_type)->hull_weight;
  if (target->owner[0] == '\0')
  {
    gain /= 10.0;
  }
  for (i = 0; i < NUM_CREW_POSITIONS; i++)
  {
    vessel_crew_gain(victor, i, gain);
  }
  vessel_db_save_crew(victor);
}

/**
 * Selling cargo trains the sailmaster and quartermaster 1.5 points and the
 * bosun 0.5 for every 2,000 gold of revenue.
 */
void vessel_crew_sale_gain(struct greyhawk_ship_data *ship, long long revenue)
{
  double lots = (double)revenue / 2000.0;

  vessel_crew_gain(ship, CREW_SAILMASTER, 1.5 * lots);
  vessel_crew_gain(ship, CREW_BOSUN, 0.5 * lots);
  vessel_crew_gain(ship, CREW_QUARTERMASTER, 1.5 * lots);
  vessel_db_save_crew(ship);
}

/**
 * A lost hull's crew gives up `percent` of its experience. A hand who falls
 * below the floor of their tier drops a tier, but never below green.
 */
void vessel_crew_casualties(struct greyhawk_ship_data *ship, double percent)
{
  int i;

  if (ship == NULL)
  {
    return;
  }

  for (i = 0; i < NUM_CREW_POSITIONS; i++)
  {
    if (ship->crew_tier[i] == CREW_TIER_NONE)
    {
      continue;
    }
    ship->crew_xp[i] =
        fmax(vessel_crew_floor(i, CREW_TIER_GREEN), ship->crew_xp[i] * (1.0 - percent / 100.0));
    if (ship->crew_tier[i] > CREW_TIER_GREEN &&
        ship->crew_xp[i] < vessel_crew_floor(i, ship->crew_tier[i]))
    {
      ship->crew_tier[i]--;
    }
  }
  vessel_apply_crew_bonuses(ship);
}

/** The crew's stamina when rested: 500, and 100 for each tier aboard (study 3.3.5). */
int vessel_stamina_max(const struct greyhawk_ship_data *ship)
{
  int tiers;
  int i;

  tiers = 0;
  for (i = 0; ship != NULL && i < NUM_CREW_POSITIONS; i++)
  {
    tiers += ship->crew_tier[i];
  }
  return 500 + 100 * tiers;
}

/**
 * Fatigue: 1 while the crew has stamina left, then 1 / (1 + deficit / max /
 * 3). It scales accel, turn, reload, repair odds, and the hit chance.
 */
double vessel_stamina_modifier(const struct greyhawk_ship_data *ship)
{
  double max;
  double deficit;

  if (ship == NULL)
  {
    return 1.0;
  }
  max = (double)vessel_stamina_max(ship);
  deficit = ship->stamina_spent - max;
  return deficit <= 0.0 ? 1.0 : 1.0 / (1.0 + deficit / max / 3.0);
}

/**
 * Duris's work divisor for a hull: the square root of her class hull weight
 * over 10. A heavier hull's gun crews tire less per shot and her helm more
 * per maneuver.
 */
double vessel_hull_effort(const struct greyhawk_ship_data *ship)
{
  return sqrt((double)vessel_class_handling(ship->vessel_type)->hull_weight) / 10.0;
}

/**
 * Crew tick: stamina returns 1.5 a tick, four times as fast berthed or at
 * anchor.
 */
void vessel_crew_tick_one(struct greyhawk_ship_data *ship)
{
  if (ship == NULL || ship->stamina_spent <= 0.0)
  {
    return;
  }
  ship->stamina_spent =
      fmax(0.0, ship->stamina_spent - (ship->dock > 0 || ship->anchored ? 6.0 : 1.5));
}

/**
 * Recompute the ship's crew effect fields from hired tiers.
 *
 * Gunnery reads the legacy guncrew field; movement and repair read the
 * sailmaster and bosun tiers directly.
 */
void vessel_apply_crew_bonuses(struct greyhawk_ship_data *ship)
{
  if (ship == NULL)
  {
    return;
  }

  ship->guncrew.gunadjust = (char)(ship->crew_tier[CREW_GUNNER] * 2);

  snprintf(ship->sailcrew.crewname, sizeof(ship->sailcrew.crewname), "%s deck crew",
           vessel_crew_tier_name(ship->crew_tier[CREW_SAILMASTER]));
  snprintf(ship->guncrew.crewname, sizeof(ship->guncrew.crewname), "%s gun crew",
           vessel_crew_tier_name(ship->crew_tier[CREW_GUNNER]));
}

/**
 * Persist hired crew (delete-and-reinsert, idempotent).
 *
 * @return FALSE when a write failed, which may leave the roster cleared
 */
bool vessel_db_save_crew(struct greyhawk_ship_data *ship)
{
  char query[MAX_STRING_LENGTH];
  bool has_crew;
  int i;
  int length;

  if (!mysql_available || conn == NULL || ship == NULL)
  {
    return FALSE;
  }

  snprintf(query, sizeof(query),
           "DELETE FROM ship_crew_roster WHERE ship_id = %d AND npc_vnum <= %d", ship->shipnum,
           CREW_ROW_VNUM_BASE);
  if (mysql_query(conn, query))
  {
    log("SYSERR: vessel_db_save_crew (clear) failed for ship %d: %s", ship->shipnum,
        mysql_error(conn));
    return FALSE;
  }

  length = snprintf(query, sizeof(query),
                    "INSERT INTO ship_crew_roster "
                    "(ship_id, npc_vnum, npc_name, crew_role, loyalty_rating, experience) VALUES ");
  if (length < 0 || length >= (int)sizeof(query))
  {
    log("SYSERR: vessel_db_save_crew could not build insert for ship %d", ship->shipnum);
    return FALSE;
  }

  has_crew = FALSE;
  for (i = 0; i < NUM_CREW_POSITIONS; i++)
  {
    if (ship->crew_tier[i] == CREW_TIER_NONE)
    {
      continue;
    }
    /* loyalty_rating carries the tier; npc_name carries the position so the
     * roster stays human-readable in the database. */
    length = snprintf_append(query, sizeof(query), length, "%s(%d, %d, '%s', 'crew', %d, %.4f)",
                             has_crew ? ", " : "", ship->shipnum, CREW_ROW_VNUM_BASE - i,
                             vessel_crew_position_name(i), ship->crew_tier[i], ship->crew_xp[i]);
    has_crew = TRUE;
  }

  if (length >= (int)sizeof(query) - 1)
  {
    log("SYSERR: vessel_db_save_crew insert overflow for ship %d", ship->shipnum);
    return FALSE;
  }

  if (has_crew && mysql_query(conn, query))
  {
    log("SYSERR: vessel_db_save_crew (insert) failed for ship %d: %s", ship->shipnum,
        mysql_error(conn));
    return FALSE;
  }
  return TRUE;
}

/**
 * Load hired crew and reapply their bonuses. A hand saved before S5 has no
 * experience and starts at the floor of their tier.
 */
void vessel_db_load_crew(struct greyhawk_ship_data *ship)
{
  char query[MAX_STRING_LENGTH];
  MYSQL_RES *result;
  MYSQL_ROW row;
  int position;

  if (!mysql_available || conn == NULL || ship == NULL)
  {
    return;
  }

  snprintf(query, sizeof(query),
           "SELECT npc_vnum, loyalty_rating, experience FROM ship_crew_roster "
           "WHERE ship_id = %d AND npc_vnum <= %d",
           ship->shipnum, CREW_ROW_VNUM_BASE);
  if (mysql_query(conn, query))
  {
    return;
  }

  result = mysql_store_result(conn);
  if (result == NULL)
  {
    return;
  }

  while ((row = mysql_fetch_row(result)) != NULL)
  {
    if (row[0] == NULL || row[1] == NULL)
    {
      continue;
    }
    position = CREW_ROW_VNUM_BASE - parse_int(row[0]);
    if (position >= 0 && position < NUM_CREW_POSITIONS)
    {
      ship->crew_tier[position] = parse_int(row[1]);
      ship->crew_xp[position] = fmax(row[2] != NULL ? strtod(row[2], NULL) : 0.0,
                                     vessel_crew_floor(position, ship->crew_tier[position]));
    }
  }
  mysql_free_result(result);

  vessel_apply_crew_bonuses(ship);
}

/**
 * Owner gate shared by the crew commands.
 *
 * @return The ship if ch owns it and is aboard, else NULL
 */
static struct greyhawk_ship_data *crew_command_ship(struct char_data *ch)
{
  struct greyhawk_ship_data *ship;

  ship = get_ship_from_room(IN_ROOM(ch));
  if (ship == NULL)
  {
    send_to_char(ch, "You must be aboard a ship.\r\n");
    return NULL;
  }

  if (ship->owner[0] == '\0')
  {
    send_to_char(ch, "This ship has no owner - claim her first.\r\n");
    return NULL;
  }

  if (str_cmp(ship->owner, GET_NAME(ch)) != 0 && GET_LEVEL(ch) < LVL_IMMORT)
  {
    send_to_char(ch, "Only %s's owner (%s) hires the crew.\r\n", ship->name, ship->owner);
    return NULL;
  }

  return ship;
}

/**
 * shiphire <position> <tier> - hire crew while docked.
 */
ACMD(do_shiphire)
{
  struct greyhawk_ship_data *ship;
  struct greyhawk_ship_data before;
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  int position;
  int tier;
  int cost;
  int i;

  ship = crew_command_ship(ch);
  if (ship == NULL)
  {
    return;
  }

  /* Hiring happens ashore - the hiring hall is at the dock */
  if (!vessel_ship_is_in_port(ship))
  {
    send_to_char(ch, "You can only take on crew while moored at a dock.\r\n");
    return;
  }

  if (vessel_port_refuses(ch))
  {
    return;
  }

  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
  if (!*arg1 || !*arg2)
  {
    send_to_char(ch, "Usage: shiphire <position> <tier>\r\n");
    send_to_char(ch, "One-time hire prices (crew draw no wages), and the renown a hull needs:\r\n");
    for (i = 0; i < NUM_CREW_POSITIONS; i++)
    {
      send_to_char(ch, "  %-14s green %6d  able %6d (%4d)  veteran %6d (%4d)\r\n",
                   vessel_crew_position_name(i), vessel_crew_hire_cost(i, CREW_TIER_GREEN),
                   vessel_crew_hire_cost(i, CREW_TIER_ABLE),
                   vessel_crew_hire_renown(i, CREW_TIER_ABLE),
                   vessel_crew_hire_cost(i, CREW_TIER_VETERAN),
                   vessel_crew_hire_renown(i, CREW_TIER_VETERAN));
    }
    send_to_char(ch, "%s has %d renown; green hands earn promotion at sea (see 'shipcrew').\r\n",
                 ship->name, ship->renown);
    return;
  }

  position = vessel_crew_position_by_name(arg1);
  if (position < 0)
  {
    send_to_char(ch, "No such position. Try: sailmaster, gunner, bosun, quartermaster.\r\n");
    return;
  }

  tier = vessel_crew_tier_by_name(arg2);
  if (tier < 0)
  {
    send_to_char(ch, "Quality runs green, able, or veteran.\r\n");
    return;
  }

  if (ship->renown < vessel_crew_hire_renown(position, tier) && GET_LEVEL(ch) < LVL_IMMORT)
  {
    send_to_char(ch,
                 "No %s %s will sign on with a hull of less than %d renown, and %s has %d. Hire "
                 "a green hand and let the sea promote them.\r\n",
                 vessel_crew_tier_name(tier), vessel_crew_position_name(position),
                 vessel_crew_hire_renown(position, tier), ship->name, ship->renown);
    return;
  }

  if (ship->crew_tier[position] != CREW_TIER_NONE)
  {
    send_to_char(ch, "A %s %s already serves aboard - dismiss them first.\r\n",
                 vessel_crew_tier_name(ship->crew_tier[position]),
                 vessel_crew_position_name(position));
    return;
  }

  cost = vessel_crew_hire_cost(position, tier);
  if (GET_GOLD(ch) < cost)
  {
    send_to_char(ch, "A %s %s wants %d gold to sign on; you have %d.\r\n",
                 vessel_crew_tier_name(tier), vessel_crew_position_name(position), cost,
                 GET_GOLD(ch));
    return;
  }

  before = *ship;
  if (!vessel_charge(ch, cost))
  {
    return;
  }
  ship->crew_tier[position] = tier;
  ship->crew_xp[position] = vessel_crew_floor(position, tier);
  vessel_apply_crew_bonuses(ship);
  if (!vessel_purchase_recorded(ch, ship, &before, cost))
  {
    return;
  }

  send_to_char(ch, "You sign on a %s %s for %d gold.\r\n", vessel_crew_tier_name(tier),
               vessel_crew_position_name(position), cost);
  send_to_ship(ship, "A %s %s reports aboard %s.", vessel_crew_tier_name(tier),
               vessel_crew_position_name(position), ship->name);
  log("Info: %s hired a %s %s for ship %d (%d gold)", GET_NAME(ch), vessel_crew_tier_name(tier),
      vessel_crew_position_name(position), ship->shipnum, cost);
}

/**
 * shipdismiss <position> - let a crew member go.
 */
ACMD(do_shipdismiss)
{
  struct greyhawk_ship_data *ship;
  char arg[MAX_INPUT_LENGTH];
  int position;

  ship = crew_command_ship(ch);
  if (ship == NULL)
  {
    return;
  }

  one_argument(argument, arg, sizeof(arg));
  position = vessel_crew_position_by_name(arg);
  if (position < 0)
  {
    send_to_char(ch, "Dismiss which position? (sailmaster, gunner, bosun, quartermaster)\r\n");
    return;
  }

  if (ship->crew_tier[position] == CREW_TIER_NONE)
  {
    send_to_char(ch, "No %s serves aboard.\r\n", vessel_crew_position_name(position));
    return;
  }

  send_to_ship(ship, "The %s gathers their kit and goes ashore.",
               vessel_crew_position_name(position));
  ship->crew_tier[position] = CREW_TIER_NONE;
  ship->crew_xp[position] = 0.0;
  vessel_apply_crew_bonuses(ship);
  vessel_db_save_crew(ship);
  send_to_char(ch, "You dismiss the %s.\r\n", vessel_crew_position_name(position));
}
