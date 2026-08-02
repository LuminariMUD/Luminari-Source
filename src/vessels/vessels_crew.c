/* ************************************************************************
 *      File:   vessels_crew.c                        Part of LuminariMUD  *
 *   Purpose:   Hired crew and wages (Phase 06, Session 03).               *
 *              Crew fill four positions at three quality tiers; their     *
 *              bonuses feed the existing sailcrew/guncrew fields that     *
 *              movement, gunnery, and repair already read.                *
 * ********************************************************************** */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "vessels.h"
#include "mysql.h"

extern MYSQL *conn;
extern bool mysql_available;
extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/* Crew rows live in ship_crew_roster with npc_vnum encoding the position so
 * they never collide with the pilot record or the helm permits
 * (npc_vnum -1). */
#define CREW_ROW_VNUM_BASE (-100)
#define CREW_WAGE_MAX_BATCH_SHIPS                                                        \
  ((GREYHAWK_ACTIVE_SHIP_CAPACITY + CREW_WAGE_BATCH_COUNT - 1) /                         \
   CREW_WAGE_BATCH_COUNT)

static int crew_wage_batch_cursor = 0;

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
 * Signing bonus to hire a crew member.
 */
int vessel_crew_hire_cost(int position, int tier)
{
  static const int base_cost[NUM_CREW_POSITIONS] = {400, 600, 350, 300};

  if (position < 0 || position >= NUM_CREW_POSITIONS || tier < CREW_TIER_GREEN ||
      tier > CREW_TIER_VETERAN)
  {
    return 0;
  }

  return base_cost[position] * tier;
}

/**
 * Per-payday wage for a crew member.
 */
int vessel_crew_wage(int position, int tier)
{
  if (position < 0 || position >= NUM_CREW_POSITIONS || tier < CREW_TIER_GREEN ||
      tier > CREW_TIER_VETERAN)
  {
    return 0;
  }

  return vessel_crew_hire_cost(position, tier) / 10;
}

/**
 * Recompute the ship's crew effect fields from hired tiers.
 *
 * Bonuses land in the legacy sailcrew/guncrew structures so movement,
 * gunnery (vessels_combat.c), and repair read them without special cases.
 */
void vessel_apply_crew_bonuses(struct greyhawk_ship_data *ship)
{
  if (ship == NULL)
  {
    return;
  }

  ship->sailcrew.speedadjust = (char)(ship->crew_tier[CREW_SAILMASTER] * 2);
  ship->guncrew.gunadjust = (char)(ship->crew_tier[CREW_GUNNER] * 2);
  ship->sailcrew.repairspeed = (char)(ship->crew_tier[CREW_BOSUN] * 2);

  snprintf(ship->sailcrew.crewname, sizeof(ship->sailcrew.crewname), "%s deck crew",
           vessel_crew_tier_name(ship->crew_tier[CREW_SAILMASTER]));
  snprintf(ship->guncrew.crewname, sizeof(ship->guncrew.crewname), "%s gun crew",
           vessel_crew_tier_name(ship->crew_tier[CREW_GUNNER]));
}

/**
 * Persist hired crew (delete-and-reinsert, idempotent).
 */
void vessel_db_save_crew(struct greyhawk_ship_data *ship)
{
  char query[MAX_STRING_LENGTH];
  bool has_crew;
  int i;
  int length;

  if (!mysql_available || conn == NULL || ship == NULL)
  {
    return;
  }

  snprintf(query, sizeof(query),
           "DELETE FROM ship_crew_roster WHERE ship_id = %d AND npc_vnum <= %d", ship->shipnum,
           CREW_ROW_VNUM_BASE);
  if (mysql_query(conn, query))
  {
    log("SYSERR: vessel_db_save_crew (clear) failed for ship %d: %s", ship->shipnum,
        mysql_error(conn));
    return;
  }

  length = snprintf(query, sizeof(query),
                    "INSERT INTO ship_crew_roster "
                    "(ship_id, npc_vnum, npc_name, crew_role, loyalty_rating) VALUES ");
  if (length < 0 || length >= (int)sizeof(query))
  {
    log("SYSERR: vessel_db_save_crew could not build insert for ship %d", ship->shipnum);
    return;
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
    length = snprintf_append(query, sizeof(query), length, "%s(%d, %d, '%s', 'crew', %d)",
                             has_crew ? ", " : "", ship->shipnum, CREW_ROW_VNUM_BASE - i,
                             vessel_crew_position_name(i), ship->crew_tier[i]);
    has_crew = TRUE;
  }

  if (length >= (int)sizeof(query) - 1)
  {
    log("SYSERR: vessel_db_save_crew insert overflow for ship %d", ship->shipnum);
    return;
  }

  if (has_crew && mysql_query(conn, query))
  {
    log("SYSERR: vessel_db_save_crew (insert) failed for ship %d: %s", ship->shipnum,
        mysql_error(conn));
  }
}

/**
 * Load hired crew and reapply their bonuses.
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
           "SELECT npc_vnum, loyalty_rating FROM ship_crew_roster "
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
    position = CREW_ROW_VNUM_BASE - atoi(row[0]);
    if (position >= 0 && position < NUM_CREW_POSITIONS)
    {
      ship->crew_tier[position] = atoi(row[1]);
    }
  }
  mysql_free_result(result);

  vessel_apply_crew_bonuses(ship);
}

/**
 * Build one atomic delete for crew members who leave during a payroll batch.
 */
int vessel_crew_departure_delete_query(char *query, size_t query_size,
                                       const int *ship_slots,
                                       const int *positions, int count)
{
  int length;
  int i;

  if (query == NULL || query_size == 0 || ship_slots == NULL ||
      positions == NULL || count <= 0 || count > CREW_WAGE_MAX_BATCH_SHIPS)
  {
    return -1;
  }

  length = snprintf(query, query_size, "DELETE FROM ship_crew_roster WHERE ");
  if (length < 0 || (size_t)length >= query_size)
  {
    return -1;
  }

  for (i = 0; i < count; i++)
  {
    if (ship_slots[i] <= 0 || ship_slots[i] >= GREYHAWK_MAXSHIPS ||
        positions[i] < 0 || positions[i] >= NUM_CREW_POSITIONS)
    {
      return -1;
    }
    length = snprintf_append(
        query, query_size, length,
        "%s(ship_id = %d AND npc_vnum = %d)", i == 0 ? "" : " OR ",
        ship_slots[i], CREW_ROW_VNUM_BASE - positions[i]);
    if (length < 0 || (size_t)length >= query_size - 1)
    {
      return -1;
    }
  }

  return length;
}

static void vessel_db_delete_departed_crew(const int *ship_slots,
                                            const int *positions, int count)
{
  char query[MAX_STRING_LENGTH];

  if (!mysql_available || conn == NULL || count <= 0)
  {
    return;
  }
  if (vessel_crew_departure_delete_query(query, sizeof(query), ship_slots,
                                         positions, count) < 0)
  {
    log("SYSERR: Could not build the crew-departure payroll delete");
    return;
  }
  if (mysql_query(conn, query))
  {
    log("SYSERR: Could not persist %d crew payroll departure%s: %s", count,
        count == 1 ? "" : "s", mysql_error(conn));
  }
}

/**
 * Total wage bill per payday for a ship.
 */
static int vessel_crew_payroll(struct greyhawk_ship_data *ship)
{
  int total = 0;
  int i;

  for (i = 0; i < NUM_CREW_POSITIONS; i++)
  {
    total += vessel_crew_wage(i, ship->crew_tier[i]);
  }

  return total;
}

/**
 * Assign one active fleet slot to a stable payroll batch.
 *
 * Slot zero is reserved. The 500 player-facing slots divide evenly across
 * the 100 batches, limiting a synchronized payday to five ships per tick.
 */
int vessel_crew_wage_batch_for_slot(int ship_slot)
{
  if (ship_slot <= 0 || ship_slot >= GREYHAWK_MAXSHIPS)
  {
    return -1;
  }

  return (ship_slot - 1) % CREW_WAGE_BATCH_COUNT;
}

/**
 * Wage accrual tick. Runs on the vessel combat/autopilot cadence; every
 * CREW_WAGE_INTERVAL ticks the payroll comes due. Crew whose wages go
 * badly unpaid walk off, taking their bonuses with them.
 */
void vessel_crew_wage_tick(void)
{
  struct greyhawk_ship_data *ship;
  int departed_ships[CREW_WAGE_MAX_BATCH_SHIPS];
  int departed_positions[CREW_WAGE_MAX_BATCH_SHIPS];
  int departed_count;
  int current_batch;
  int payroll;
  int i;
  int pos;

  departed_count = 0;
  current_batch = crew_wage_batch_cursor;
  crew_wage_batch_cursor = (crew_wage_batch_cursor + 1) % CREW_WAGE_BATCH_COUNT;

  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
  {
    ship = &greyhawk_ships[i];
    if (!is_valid_ship(ship))
    {
      continue;
    }

    payroll = vessel_crew_payroll(ship);
    if (payroll <= 0)
    {
      continue;
    }

    ship->wage_ticks++;
    if (ship->wage_ticks < CREW_WAGE_INTERVAL)
    {
      continue;
    }

    /* Hold a due payroll at the threshold until its bounded batch runs. */
    ship->wage_ticks = CREW_WAGE_INTERVAL;
    if (vessel_crew_wage_batch_for_slot(i) != current_batch)
    {
      continue;
    }

    ship->wage_ticks = 0;
    ship->wages_owed += payroll;
    send_to_ship(ship, "The crew's wages come due: %d gold owed (use 'shipwages').",
                 ship->wages_owed);

    /* Three unpaid paydays and the best-paid hand walks at the next port */
    if (ship->wages_owed > payroll * 3)
    {
      for (pos = NUM_CREW_POSITIONS - 1; pos >= 0; pos--)
      {
        if (ship->crew_tier[pos] != CREW_TIER_NONE)
        {
          send_to_ship(ship, "The %s has had enough of empty promises and walks off!",
                       vessel_crew_position_name(pos));
          log("Info: Ship %d '%s' lost %s to unpaid wages (%d owed)", ship->shipnum, ship->name,
              vessel_crew_position_name(pos), ship->wages_owed);
          ship->crew_tier[pos] = CREW_TIER_NONE;
          ship->wages_owed -= payroll; /* one less mouth on the books */
          vessel_apply_crew_bonuses(ship);
          if (departed_count < CREW_WAGE_MAX_BATCH_SHIPS)
          {
            departed_ships[departed_count] = ship->shipnum;
            departed_positions[departed_count] = pos;
            departed_count++;
          }
          break;
        }
      }
    }
  }

  vessel_db_delete_departed_crew(departed_ships, departed_positions,
                                  departed_count);
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

  if (str_cmp(ship->owner, GET_NAME(ch)) && GET_LEVEL(ch) < LVL_IMMORT)
  {
    send_to_char(ch, "Only %s's owner (%s) hires and pays the crew.\r\n", ship->name, ship->owner);
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

  two_arguments_u((char *)argument, arg1, arg2);
  if (!*arg1 || !*arg2)
  {
    send_to_char(ch, "Usage: shiphire <position> <tier>\r\n");
    send_to_char(ch, "Positions and rates (hire / per payday):\r\n");
    for (i = 0; i < NUM_CREW_POSITIONS; i++)
    {
      send_to_char(ch, "  %-14s green %d/%d  able %d/%d  veteran %d/%d\r\n",
                   vessel_crew_position_name(i), vessel_crew_hire_cost(i, CREW_TIER_GREEN),
                   vessel_crew_wage(i, CREW_TIER_GREEN), vessel_crew_hire_cost(i, CREW_TIER_ABLE),
                   vessel_crew_wage(i, CREW_TIER_ABLE), vessel_crew_hire_cost(i, CREW_TIER_VETERAN),
                   vessel_crew_wage(i, CREW_TIER_VETERAN));
    }
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

  GET_GOLD(ch) -= cost;
  ship->crew_tier[position] = tier;
  vessel_apply_crew_bonuses(ship);
  vessel_db_save_crew(ship);

  send_to_char(ch, "You sign on a %s %s for %d gold (%d per payday).\r\n",
               vessel_crew_tier_name(tier), vessel_crew_position_name(position), cost,
               vessel_crew_wage(position, tier));
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

  one_argument_u((char *)argument, arg);
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
  vessel_apply_crew_bonuses(ship);
  vessel_db_save_crew(ship);
  send_to_char(ch, "You dismiss the %s.\r\n", vessel_crew_position_name(position));
}

/**
 * shipwages - settle the accrued payroll.
 */
ACMD(do_shipwages)
{
  struct greyhawk_ship_data *ship;
  int payroll;

  ship = crew_command_ship(ch);
  if (ship == NULL)
  {
    return;
  }

  payroll = vessel_crew_payroll(ship);
  send_to_char(ch, "%s's payroll: %d gold per payday.\r\n", ship->name, payroll);

  if (ship->wages_owed <= 0)
  {
    send_to_char(ch, "The crew is paid up.\r\n");
    return;
  }

  if (GET_GOLD(ch) < ship->wages_owed)
  {
    send_to_char(ch, "You owe %d gold in back wages but carry only %d.\r\n", ship->wages_owed,
                 GET_GOLD(ch));
    return;
  }

  GET_GOLD(ch) -= ship->wages_owed;
  send_to_char(ch, "You pay out %d gold in wages.\r\n", ship->wages_owed);
  send_to_ship(ship, "Wages paid - the crew's mood improves considerably.");
  ship->wages_owed = 0;
}
