/* ************************************************************************
 *      File:   vessels_balance.c                     Part of LuminariMUD  *
 *   Purpose:   Read-only mechanical vessel balance diagnostics.          *
 * ********************************************************************** */

#include "conf.h"
#include "core/sysdep.h"
#include <math.h>
#include "core/structs.h"
#include "core/utils.h"
#include "core/comm.h"
#include "vessels.h"
#include "database/mysql.h"


/* The duel harness (study D2, 3.3.4): equal warships with Duris's frigate
 * combat fit fight through the production movement, gunnery, damage, and
 * reload code. */
#define VESSEL_BALANCE_DUEL_RANGE 8        /* Rooms apart at the start */
#define VESSEL_BALANCE_DUEL_HOLD 7.5       /* Range held abeam */
#define VESSEL_BALANCE_DUEL_TURN 20.0      /* Degrees toward her per room beyond it */
#define VESSEL_BALANCE_DUEL_MAX_TICKS 7200 /* An hour */
#define VESSEL_BALANCE_DUEL_SEED 0x5eed1234UL

/* D2: a 3-8 minute median, a 12 minute p95, nothing under 90 s. A duel
 * with no kill in an hour is a draw: a hull whose rudder is smashed cannot
 * come about, and one can limp off or both lie still with nothing bearing. */
#define VESSEL_BALANCE_MEDIAN_MIN_TENTHS 1800
#define VESSEL_BALANCE_MEDIAN_MAX_TENTHS 4800
#define VESSEL_BALANCE_P95_MAX_TENTHS 7200
#define VESSEL_BALANCE_MINIMUM_TENTHS 900
#define VESSEL_BALANCE_MAX_DRAW_PERCENT 2

struct vessel_balance_observed_data
{
  long long owned_hulls;
  long long insured_value;
  long long dock_fees;
  long long completed_freight;
  long long freight_payout;
  long long showcase_entries;
};

/**
 * A default warship with Duris's frigate combat fit (study 1.13d): three
 * large ballistae on each beam and a heavy beamcannon on the bow, sailing
 * east at her best speed. Fleet slot 0 is reserved, so she is never in port.
 */
static void vessel_balance_warship(struct greyhawk_ship_data *ship, double y)
{
  int i;

  memset(ship, 0, sizeof(*ship));
  ship->vessel_type = VESSEL_WARSHIP;
  ship->maxspeed = (short int)vessel_class_handling(VESSEL_WARSHIP)->speed;
  ship->position_speed_percent = 100;
  ship->docked_to_ship = -1;
  vessel_initialize_condition(ship, vessel_class_condition(VESSEL_WARSHIP)->beam_armor);
  vessel_set_weapon(&ship->slot[0], VESSEL_WEAPON_HEAVY_BEAMCANNON, GREYHAWK_FORE);
  for (i = 1; i <= 3; i++)
  {
    vessel_set_weapon(&ship->slot[i], VESSEL_WEAPON_LARGE_BALLISTA, GREYHAWK_PORT);
    vessel_set_weapon(&ship->slot[i + 3], VESSEL_WEAPON_LARGE_BALLISTA, GREYHAWK_STARBOARD);
  }
  ship->y = y;
  ship->heading = 90.0;
  ship->setheading = 90;
  ship->setspeed = ship->maxspeed;
  ship->speed = vessel_max_speed(ship);
}

/** A side's remaining armor and structure. */
static int vessel_balance_side(struct greyhawk_ship_data *ship, int arc)
{
  return *vessel_arc_armor(ship, arc) + *vessel_arc_internal(ship, arc);
}

/**
 * One tick of a duelling captain: bring the healthier beam to bear, turning
 * toward the enemy 20 degrees for each room beyond 7.5 (bow on by 12) or
 * away inside it (at most 45), fire everything that bears, and sail on.
 */
static void vessel_balance_captain(struct greyhawk_ship_data *ship,
                                   struct greyhawk_ship_data *enemy)
{
  double toward;
  double relative;
  int s;

  toward = fmax(-45.0, fmin(90.0, (vessel_range_between(ship, enemy) - VESSEL_BALANCE_DUEL_HOLD) *
                                      VESSEL_BALANCE_DUEL_TURN));
  if (vessel_balance_side(ship, GREYHAWK_PORT) >= vessel_balance_side(ship, GREYHAWK_STARBOARD))
  {
    relative = 270.0 + toward;
  }
  else
  {
    relative = 90.0 - toward;
  }
  ship->setheading =
      (short int)vessel_display_heading(vessel_bearing_between(ship, enemy) - relative);

  for (s = 0; s < GREYHAWK_MAXSLOTS; s++)
  {
    if (ship->slot[s].timer > 0)
    {
      ship->slot[s].timer--;
    }
    if (vessel_weapon_fire_problem(ship, s, enemy, TRUE) == NULL)
    {
      vessel_fire_weapon(ship, s, enemy, NULL);
    }
  }
  vessel_sail_tick(ship, vessel_max_speed(ship), vessel_open_water, NULL, NULL);
}

static int vessel_balance_compare_ints(const void *left, const void *right)
{
  const int *left_value;
  const int *right_value;

  left_value = left;
  right_value = right;
  if (*left_value < *right_value)
  {
    return -1;
  }
  if (*left_value > *right_value)
  {
    return 1;
  }
  return 0;
}

/**
 * Convert combat ticks to tenths of a real second at the production cadence.
 */
static int vessel_balance_tick_tenths(int ticks)
{
  return ticks * AUTOPILOT_TICK_INTERVAL * 10 / PASSES_PER_SEC;
}

/**
 * Duel equal warships through the production rules until one is holed on a
 * second side and would sink, or an hour passes (a draw). The duels draw on
 * the live random stream from a fixed seed, so the report repeats, and the
 * live stream resumes where it was.
 *
 * @return FALSE for a bad request or when no duel was decided
 */
bool vessel_balance_run_duels(int duel_count, struct vessel_balance_duel_result *result)
{
  struct greyhawk_ship_data first;
  struct greyhawk_ship_data second;
  int durations[VESSEL_BALANCE_MAX_DUELS];
  unsigned long live_seed;
  int duel;
  int tick;
  int p95_index;

  if (result == NULL || duel_count < 1 || duel_count > VESSEL_BALANCE_MAX_DUELS)
  {
    return FALSE;
  }

  memset(result, 0, sizeof(*result));
  result->requested_duels = duel_count;
  live_seed = circle_random();
  circle_srandom(VESSEL_BALANCE_DUEL_SEED);

  for (duel = 0; duel < duel_count; duel++)
  {
    /* Parallel courses, the first with the second off her port beam. */
    vessel_balance_warship(&first, 0.0);
    vessel_balance_warship(&second, (double)VESSEL_BALANCE_DUEL_RANGE);
    for (tick = 1; tick <= VESSEL_BALANCE_DUEL_MAX_TICKS; tick++)
    {
      vessel_balance_captain(&first, &second);
      vessel_balance_captain(&second, &first);
      if (vessel_breached_arcs(&second) >= 2)
      {
        result->first_wins++;
        break;
      }
      if (vessel_breached_arcs(&first) >= 2)
      {
        result->second_wins++;
        break;
      }
    }

    if (tick <= VESSEL_BALANCE_DUEL_MAX_TICKS)
    {
      durations[result->completed_duels++] = tick;
    }
    else
    {
      result->unresolved_duels++;
    }
  }
  circle_srandom(live_seed);

  if (result->completed_duels == 0)
  {
    return FALSE;
  }

  qsort(durations, (size_t)result->completed_duels, sizeof(durations[0]),
        vessel_balance_compare_ints);
  p95_index = (result->completed_duels * 95 + 99) / 100 - 1;
  result->minimum_ticks = durations[0];
  result->median_ticks = durations[result->completed_duels / 2];
  result->p95_ticks = durations[p95_index];
  result->maximum_ticks = durations[result->completed_duels - 1];

  return TRUE;
}

/**
 * Read anonymized persisted usage totals. Failure leaves the mechanical
 * report usable while making the missing player-data evidence explicit.
 */
static bool vessel_balance_load_observed(struct vessel_balance_observed_data *data)
{
  const char *query;
  MYSQL_RES *query_result;
  MYSQL_ROW row;
  long long *fields[6];
  int i;

  if (data == NULL || !mysql_available || conn == NULL)
  {
    return FALSE;
  }

  memset(data, 0, sizeof(*data));
  query = "SELECT "
          "(SELECT COUNT(*) FROM ship_interiors WHERE owner <> ''), "
          "(SELECT COALESCE(SUM(insured_for), 0) FROM ship_interiors WHERE owner <> ''), "
          "(SELECT COALESCE(SUM(r.dock_fee_balance), 0) "
          "FROM ship_runtime_state r JOIN ship_interiors i ON i.ship_id = r.ship_id "
          "WHERE i.owner <> ''), "
          "(SELECT COUNT(*) FROM freight_contracts WHERE status = 2), "
          "(SELECT COALESCE(SUM(payout), 0) FROM freight_contracts WHERE status = 2), "
          "(SELECT COALESCE(SUM(entries), 0) FROM vessel_event_leaderboards)";
  if (mysql_query(conn, query))
  {
    log("SYSERR: Vessel balance usage query failed: %s", mysql_error(conn));
    return FALSE;
  }

  query_result = mysql_store_result(conn);
  if (query_result == NULL)
  {
    return FALSE;
  }
  row = mysql_fetch_row(query_result);
  if (row == NULL)
  {
    mysql_free_result(query_result);
    return FALSE;
  }

  fields[0] = &data->owned_hulls;
  fields[1] = &data->insured_value;
  fields[2] = &data->dock_fees;
  fields[3] = &data->completed_freight;
  fields[4] = &data->freight_payout;
  fields[5] = &data->showcase_entries;
  for (i = 0; i < 6; i++)
  {
    *fields[i] = row[i] == NULL ? 0 : parse_llong(row[i]);
  }
  mysql_free_result(query_result);
  return TRUE;
}

/**
 * Render the mechanical release diagnostic to one actual staff character.
 */
bool vessel_balance_report(struct char_data *ch, int duel_count)
{
  struct vessel_balance_duel_result duel;
  struct vessel_trade_simulation_result trade;
  struct vessel_balance_observed_data observed;
  bool duel_ok;
  bool trade_ok;
  bool observed_ok;
  bool mechanical_pass;
  int median_tenths;
  int p95_tenths;
  int minimum_tenths;
  int maximum_tenths;
  int crew_hires[3];
  int price;
  int refit;
  int tier;
  int position;
  int vessel_type;

  if (ch == NULL || duel_count < 1 || duel_count > VESSEL_BALANCE_MAX_DUELS)
  {
    return FALSE;
  }

  duel_ok = vessel_balance_run_duels(duel_count, &duel);
  trade_ok = vessel_trade_run_simulation(1000, &trade);
  observed_ok = vessel_balance_load_observed(&observed);
  median_tenths = vessel_balance_tick_tenths(duel.median_ticks);
  p95_tenths = vessel_balance_tick_tenths(duel.p95_ticks);
  minimum_tenths = vessel_balance_tick_tenths(duel.minimum_ticks);
  maximum_tenths = vessel_balance_tick_tenths(duel.maximum_ticks);
  mechanical_pass = duel_ok && trade_ok && median_tenths >= VESSEL_BALANCE_MEDIAN_MIN_TENTHS &&
                    median_tenths <= VESSEL_BALANCE_MEDIAN_MAX_TENTHS &&
                    p95_tenths <= VESSEL_BALANCE_P95_MAX_TENTHS &&
                    minimum_tenths >= VESSEL_BALANCE_MINIMUM_TENTHS &&
                    duel.unresolved_duels * 100 <= duel_count * VESSEL_BALANCE_MAX_DRAW_PERCENT;

  memset(crew_hires, 0, sizeof(crew_hires));
  for (tier = CREW_TIER_GREEN; tier <= CREW_TIER_VETERAN; tier++)
  {
    for (position = 0; position < NUM_CREW_POSITIONS; position++)
    {
      crew_hires[tier - CREW_TIER_GREEN] += vessel_crew_hire_cost(position, tier);
    }
  }

  send_to_char(ch, "Vessel mechanical balance diagnostic: %s\r\n",
               mechanical_pass ? "PASS" : "FAIL");
  send_to_char(ch,
               "Equal-warship duels: %d/%d decided, %d drawn (at most %d%%); first/second wins "
               "%d/%d.\r\n",
               duel.completed_duels, duel.requested_duels, duel.unresolved_duels,
               VESSEL_BALANCE_MAX_DRAW_PERCENT, duel.first_wins, duel.second_wins);
  send_to_char(ch,
               "  TTK min/median/p95/max: %d.%d/%d.%d/%d.%d/%d.%d seconds "
               "(target median 180-480, p95 <= 720, minimum >= 90).\r\n",
               minimum_tenths / 10, minimum_tenths % 10, median_tenths / 10, median_tenths % 10,
               p95_tenths / 10, p95_tenths % 10, maximum_tenths / 10, maximum_tenths % 10);
  send_to_char(ch, "  Model: default warships with three large ballistae a beam and a heavy "
                   "beamcannon, NPC-crew gunnery (+5), holding the healthier beam at 6-9 "
                   "rooms.\r\n");
  send_to_char(ch,
               "Economy: %d/%d trades, route %d trips/%lld gold, reversal "
               "%lld gold, restock %d/%d.\r\n",
               trade.completed_trades, trade.requested_trades, trade.profitable_routes,
               trade.finite_route_profit, trade.adversarial_profit, trade.restocked_source_supply,
               trade.restocked_destination_supply);
  send_to_char(ch,
               "Full-roster one-time hire: green %d, able %d, veteran %d gold; no "
               "wages.\r\n",
               crew_hires[0], crew_hires[1], crew_hires[2]);
  send_to_char(ch, "Class cost anchors (speed 10, armor 10): hull / one refit / "
                   "20%% insurance premium / dock.\r\n");
  for (vessel_type = 0; vessel_type < NUM_VESSEL_TYPES; vessel_type++)
  {
    price = vessel_prototype_price(vessel_type, 10, 10);
    refit = vessel_upgrade_cost(0, (enum vessel_class)vessel_type);
    send_to_char(ch, "  %-16s %7d / %6d / %6d / %3d gold\r\n",
                 get_vessel_type_name((enum vessel_class)vessel_type), price, refit,
                 MAX(1, price / 5), vessel_dock_fee_for_class((enum vessel_class)vessel_type));
  }

  if (observed_ok)
  {
    send_to_char(ch,
                 "Persisted sample: %lld owned hulls, %lld insured value, %lld dock "
                 "fees.\r\n",
                 observed.owned_hulls, observed.insured_value, observed.dock_fees);
    send_to_char(ch,
                 "  Completed freight: %lld contracts / %lld gold; showcase "
                 "entries: %lld.\r\n",
                 observed.completed_freight, observed.freight_payout, observed.showcase_entries);
  }
  else
  {
    send_to_char(ch, "Persisted sample: unavailable; player-data evidence is missing.\r\n");
  }
  send_to_char(ch, "Human beta and player fun sign-off: REQUIRED; this diagnostic "
                   "does not supply feedback.\r\n");

  log("Info: %s ran %d vessel balance duels: mechanical %s, median %d.%d seconds", GET_NAME(ch),
      duel_count, mechanical_pass ? "PASS" : "FAIL", median_tenths / 10, median_tenths % 10);
  return mechanical_pass;
}
