/* ************************************************************************
 *      File:   vessels_balance.c                     Part of LuminariMUD  *
 *   Purpose:   Read-only mechanical vessel balance diagnostics.          *
 * ********************************************************************** */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "vessels.h"
#include "mysql.h"

extern MYSQL *conn;
extern bool mysql_available;

#define VESSEL_BALANCE_DUEL_ARMOR 40
#define VESSEL_BALANCE_DUEL_SPEED 20
#define VESSEL_BALANCE_DUEL_GUN_BONUS 9
#define VESSEL_BALANCE_DUEL_DAMAGE_DICE 2
#define VESSEL_BALANCE_DUEL_DAMAGE_SIDES 8
#define VESSEL_BALANCE_DUEL_MAX_TICKS 2000

#define VESSEL_BALANCE_MEDIAN_MIN_TENTHS 450
#define VESSEL_BALANCE_MEDIAN_MAX_TENTHS 1200
#define VESSEL_BALANCE_P95_MAX_TENTHS 1800
#define VESSEL_BALANCE_MINIMUM_TENTHS 150

struct vessel_balance_observed_data
{
  long long owned_hulls;
  long long wages_owed;
  long long insured_value;
  long long dock_fees;
  long long completed_freight;
  long long freight_payout;
  long long showcase_entries;
};

/**
 * Advance the diagnostic's private deterministic random stream.
 */
static unsigned int vessel_balance_random(unsigned int *state, unsigned int limit)
{
  *state = *state * 1664525U + 1013904223U;
  return *state % limit + 1U;
}

/**
 * Roll damage without consuming the live game's random stream.
 */
static int vessel_balance_dice(unsigned int *state, int count, int sides)
{
  int total;
  int i;

  total = 0;
  for (i = 0; i < count; i++)
  {
    total += (int)vessel_balance_random(state, (unsigned int)sides);
  }
  return total;
}

/**
 * Apply one same-arc hit using the production armor-before-structure rule.
 */
static void vessel_balance_damage(int *armor, int *structure, int damage)
{
  int spill;

  spill = damage - *armor;
  if (spill <= 0)
  {
    *armor -= damage;
    return;
  }

  *armor = 0;
  *structure -= spill;
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
 * Run equal representative warships through the production hit, damage, and
 * reload formulas without creating hulls or touching the live random stream.
 */
bool vessel_balance_run_duels(int duel_count, struct vessel_balance_duel_result *result)
{
  int durations[VESSEL_BALANCE_MAX_DUELS];
  unsigned int random_state;
  int structure;
  int first_armor;
  int first_structure;
  int first_timer;
  int second_armor;
  int second_structure;
  int second_timer;
  int duel;
  int tick;
  int p95_index;

  if (result == NULL || duel_count < 1 || duel_count > VESSEL_BALANCE_MAX_DUELS)
  {
    return FALSE;
  }

  memset(result, 0, sizeof(*result));
  result->requested_duels = duel_count;
  random_state = 0x5eed1234U;
  structure = 4 * MAX(10, VESSEL_BALANCE_DUEL_ARMOR / 2 + 10);

  for (duel = 0; duel < duel_count; duel++)
  {
    first_armor = VESSEL_BALANCE_DUEL_ARMOR;
    first_structure = structure;
    first_timer = 0;
    second_armor = VESSEL_BALANCE_DUEL_ARMOR;
    second_structure = structure;
    second_timer = 0;

    for (tick = 1; tick <= VESSEL_BALANCE_DUEL_MAX_TICKS; tick++)
    {
      if (first_timer > 0)
      {
        first_timer--;
      }
      if (second_timer > 0)
      {
        second_timer--;
      }

      if (first_timer == 0)
      {
        first_timer = VESSEL_WEAPON_RELOAD_TICKS;
        if ((int)vessel_balance_random(&random_state, 20U) + VESSEL_BALANCE_DUEL_GUN_BONUS >=
            10 + VESSEL_BALANCE_DUEL_SPEED / 5)
        {
          vessel_balance_damage(&second_armor, &second_structure,
                                vessel_balance_dice(&random_state, VESSEL_BALANCE_DUEL_DAMAGE_DICE,
                                                    VESSEL_BALANCE_DUEL_DAMAGE_SIDES));
        }
        if (second_structure <= 0)
        {
          result->first_wins++;
          break;
        }
      }

      if (second_timer == 0)
      {
        second_timer = VESSEL_WEAPON_RELOAD_TICKS;
        if ((int)vessel_balance_random(&random_state, 20U) + VESSEL_BALANCE_DUEL_GUN_BONUS >=
            10 + VESSEL_BALANCE_DUEL_SPEED / 5)
        {
          vessel_balance_damage(&first_armor, &first_structure,
                                vessel_balance_dice(&random_state, VESSEL_BALANCE_DUEL_DAMAGE_DICE,
                                                    VESSEL_BALANCE_DUEL_DAMAGE_SIDES));
        }
        if (first_structure <= 0)
        {
          result->second_wins++;
          break;
        }
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

  return result->completed_duels == duel_count && result->unresolved_duels == 0;
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
  long long *fields[7];
  int i;

  if (data == NULL || !mysql_available || conn == NULL)
  {
    return FALSE;
  }

  memset(data, 0, sizeof(*data));
  query = "SELECT "
          "(SELECT COUNT(*) FROM ship_interiors WHERE owner <> ''), "
          "(SELECT COALESCE(SUM(wages_owed), 0) FROM ship_interiors WHERE owner <> ''), "
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
  fields[1] = &data->wages_owed;
  fields[2] = &data->insured_value;
  fields[3] = &data->dock_fees;
  fields[4] = &data->completed_freight;
  fields[5] = &data->freight_payout;
  fields[6] = &data->showcase_entries;
  for (i = 0; i < 7; i++)
  {
    *fields[i] = row[i] == NULL ? 0 : atoll(row[i]);
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
  int crew_wages[3];
  int payday_seconds;
  int payday_deferral_seconds;
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
                    minimum_tenths >= VESSEL_BALANCE_MINIMUM_TENTHS;

  memset(crew_wages, 0, sizeof(crew_wages));
  for (tier = CREW_TIER_GREEN; tier <= CREW_TIER_VETERAN; tier++)
  {
    for (position = 0; position < NUM_CREW_POSITIONS; position++)
    {
      crew_wages[tier - CREW_TIER_GREEN] += vessel_crew_wage(position, tier);
    }
  }
  payday_seconds = CREW_WAGE_INTERVAL * AUTOPILOT_TICK_INTERVAL / PASSES_PER_SEC;
  payday_deferral_seconds = CREW_WAGE_BATCH_COUNT * AUTOPILOT_TICK_INTERVAL / PASSES_PER_SEC;

  send_to_char(ch, "Vessel mechanical balance diagnostic: %s\r\n",
               mechanical_pass ? "PASS" : "FAIL");
  send_to_char(ch, "Equal-warship duels: %d/%d resolved; first/second wins %d/%d.\r\n",
               duel.completed_duels, duel.requested_duels, duel.first_wins, duel.second_wins);
  send_to_char(ch,
               "  TTK min/median/p95/max: %d.%d/%d.%d/%d.%d/%d.%d seconds "
               "(target median 45-120, p95 <= 180).\r\n",
               minimum_tenths / 10, minimum_tenths % 10, median_tenths / 10, median_tenths % 10,
               p95_tenths / 10, p95_tenths % 10, maximum_tenths / 10, maximum_tenths % 10);
  send_to_char(ch, "  Model: armor 40, speed 20, able gunner, one bearing 2d8 "
                   "battery, six-tick reload.\r\n");
  send_to_char(ch,
               "Economy: %d/%d trades, route %d trips/%lld gold, reversal "
               "%lld gold, restock %d/%d.\r\n",
               trade.completed_trades, trade.requested_trades, trade.profitable_routes,
               trade.finite_route_profit, trade.adversarial_profit, trade.restocked_source_supply,
               trade.restocked_destination_supply);
  send_to_char(ch,
               "Full-roster wages per payday: green %d, able %d, veteran %d "
               "gold; cadence %d seconds plus <= %d batch deferral.\r\n",
               crew_wages[0], crew_wages[1], crew_wages[2], payday_seconds,
               payday_deferral_seconds);
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
                 "Persisted sample: %lld owned hulls, %lld wages owed, %lld "
                 "insured value, %lld dock fees.\r\n",
                 observed.owned_hulls, observed.wages_owed, observed.insured_value,
                 observed.dock_fees);
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
