/* ************************************************************************
 *      File:   vessels_contracts.c                   Part of LuminariMUD  *
 *   Purpose:   Freight contracts (Phase 07, Session 03).                  *
 *              A port's board offers paid deliveries to other ports. The  *
 *              offers are generated from live commodity prices and the    *
 *              real distance between ports, so pay tracks actual risk.    *
 * ********************************************************************** */

#include <math.h>

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
extern struct room_data *world;

/* Freight pays this many gold per unit per 10 coordinate units of run,
 * on top of the goods' own value. */
#define FREIGHT_RATE_PER_DISTANCE 2

/* Contract offers stay on a board this long (real seconds) before the
 * board is regenerated. */
#define CONTRACT_BOARD_TTL 3600

/**
 * Create the freight contract table.
 * Mirrored by sql/components/vessels_phase7_schema.sql.
 */
void vessel_contracts_ensure_schema(void)
{
  if (!mysql_available || conn == NULL)
  {
    return;
  }

  if (mysql_query(conn, "CREATE TABLE IF NOT EXISTS freight_contracts ("
                        "  contract_id INT AUTO_INCREMENT PRIMARY KEY,"
                        "  origin_vnum INT NOT NULL,"
                        "  destination_vnum INT NOT NULL,"
                        "  commodity_id INT NOT NULL,"
                        "  quantity INT NOT NULL,"
                        "  payout INT NOT NULL,"
                        "  status INT NOT NULL DEFAULT 0,"
                        "  taken_by VARCHAR(64) NOT NULL DEFAULT '',"
                        "  offered_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
                        "  INDEX idx_origin (origin_vnum),"
                        "  INDEX idx_status (status),"
                        "  INDEX idx_taken (taken_by)"
                        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"))
  {
    log("SYSERR: freight_contracts create failed: %s", mysql_error(conn));
  }
}

/**
 * Find the wilderness coordinates of a dock room.
 *
 * @return TRUE if the room exists and yielded coordinates
 */
static bool port_coords(int port_vnum, int *x, int *y)
{
  room_rnum rnum = real_room(port_vnum);

  if (rnum == NOWHERE)
  {
    return FALSE;
  }

  *x = world[rnum].coords[0];
  *y = world[rnum].coords[1];
  return TRUE;
}

/**
 * Straight-line distance between two ports in coordinate units.
 */
static int port_distance(int from_vnum, int to_vnum)
{
  int x1, y1, x2, y2;
  int dx, dy;

  if (!port_coords(from_vnum, &x1, &y1) || !port_coords(to_vnum, &x2, &y2))
  {
    return 0;
  }

  dx = x2 - x1;
  dy = y2 - y1;
  return (int)sqrt((double)(dx * dx + dy * dy));
}

/**
 * Regenerate a port's contract board when the current offers are stale.
 *
 * Destinations are drawn from other ports already known to the trade
 * system (they have port_commodities rows), so the board only ever offers
 * runs to places that actually exist and trade.
 */
void vessel_contracts_refresh_port(int port_vnum)
{
  char query[MAX_STRING_LENGTH];
  MYSQL_RES *result;
  MYSQL_ROW row;
  int destinations[MAX_CONTRACT_OFFERS];
  int num_destinations = 0;
  int commodity_ids[MAX_CONTRACT_OFFERS];
  int base_prices[MAX_CONTRACT_OFFERS];
  int num_commodities = 0;
  int fresh = 0;
  int i;

  if (!mysql_available || conn == NULL)
  {
    return;
  }

  /* Leave the board alone while its offers are still fresh */
  snprintf(query, sizeof(query),
           "SELECT COUNT(*) FROM freight_contracts WHERE origin_vnum = %d AND status = %d "
           "AND offered_at > (NOW() - INTERVAL %d SECOND)",
           port_vnum, CONTRACT_STATUS_OPEN, CONTRACT_BOARD_TTL);
  if (mysql_query(conn, query))
  {
    return;
  }
  result = mysql_store_result(conn);
  if (result != NULL)
  {
    row = mysql_fetch_row(result);
    if (row != NULL && row[0] != NULL)
    {
      fresh = atoi(row[0]);
    }
    mysql_free_result(result);
  }
  if (fresh > 0)
  {
    return;
  }

  /* Clear stale open offers (accepted contracts are never touched) */
  snprintf(query, sizeof(query),
           "DELETE FROM freight_contracts WHERE origin_vnum = %d AND status = %d", port_vnum,
           CONTRACT_STATUS_OPEN);
  if (mysql_query(conn, query))
  {
    log("SYSERR: contract board clear failed: %s", mysql_error(conn));
    return;
  }

  /* Candidate destinations: other known trading ports */
  snprintf(query, sizeof(query),
           "SELECT DISTINCT port_vnum FROM port_commodities WHERE port_vnum <> %d LIMIT %d",
           port_vnum, MAX_CONTRACT_OFFERS);
  if (mysql_query(conn, query))
  {
    return;
  }
  result = mysql_store_result(conn);
  if (result != NULL)
  {
    while ((row = mysql_fetch_row(result)) != NULL && num_destinations < MAX_CONTRACT_OFFERS)
    {
      if (row[0] != NULL)
      {
        destinations[num_destinations++] = atoi(row[0]);
      }
    }
    mysql_free_result(result);
  }

  if (num_destinations == 0)
  {
    return; /* Nowhere to ship to yet */
  }

  /* Goods to ship */
  snprintf(query, sizeof(query),
           "SELECT commodity_id, base_price FROM trade_commodities ORDER BY commodity_id LIMIT %d",
           MAX_CONTRACT_OFFERS);
  if (mysql_query(conn, query))
  {
    return;
  }
  result = mysql_store_result(conn);
  if (result != NULL)
  {
    while ((row = mysql_fetch_row(result)) != NULL && num_commodities < MAX_CONTRACT_OFFERS)
    {
      if (row[0] != NULL)
      {
        commodity_ids[num_commodities] = atoi(row[0]);
        base_prices[num_commodities] = row[1] ? atoi(row[1]) : 10;
        num_commodities++;
      }
    }
    mysql_free_result(result);
  }

  if (num_commodities == 0)
  {
    return;
  }

  /* One offer per destination: quantity and payout scale with the run */
  for (i = 0; i < num_destinations; i++)
  {
    int commodity_index = i % num_commodities;
    int distance = port_distance(port_vnum, destinations[i]);
    int quantity = 10 + (distance / 20) * 5;
    int payout;

    if (quantity > 100)
    {
      quantity = 100;
    }

    /* Pay the goods' worth plus a distance premium, so long hauls of
     * valuable cargo are the good jobs. */
    payout = base_prices[commodity_index] * quantity +
             quantity * FREIGHT_RATE_PER_DISTANCE * MAX(1, distance / 10);

    snprintf(query, sizeof(query),
             "INSERT INTO freight_contracts (origin_vnum, destination_vnum, commodity_id, "
             "quantity, payout, status) VALUES (%d, %d, %d, %d, %d, %d)",
             port_vnum, destinations[i], commodity_ids[commodity_index], quantity, payout,
             CONTRACT_STATUS_OPEN);
    if (mysql_query(conn, query))
    {
      log("SYSERR: contract offer insert failed: %s", mysql_error(conn));
    }
  }

  log("Info: Refreshed freight board at port %d (%d offers)", port_vnum, num_destinations);
}

/**
 * Resolve the trading context: the player's ship, moored at a dock.
 *
 * @param port_vnum Out: the dock room's vnum
 * @return The ship, or NULL with a message sent
 */
static struct greyhawk_ship_data *contract_context(struct char_data *ch, int *port_vnum)
{
  struct greyhawk_ship_data *ship;

  if (!mysql_available || conn == NULL)
  {
    send_to_char(ch, "The freight office is closed.\r\n");
    return NULL;
  }

  ship = get_ship_from_room(IN_ROOM(ch));
  if (ship == NULL)
  {
    send_to_char(ch, "You must be aboard a ship to take freight work.\r\n");
    return NULL;
  }

  if (!vessel_ship_is_in_port(ship))
  {
    send_to_char(ch, "You must be moored at a port.\r\n");
    return NULL;
  }

  if (vessel_port_refuses(ch))
  {
    return NULL;
  }

  *port_vnum = world[IN_ROOM(ship->shipobj)].number;
  return ship;
}

/**
 * Name of a destination port room, for display.
 */
static const char *port_name(int port_vnum)
{
  room_rnum rnum = real_room(port_vnum);

  return (rnum == NOWHERE) ? "an unknown port" : world[rnum].name;
}

/**
 * contracts - show this port's freight board plus your active jobs.
 */
ACMD(do_contracts)
{
  struct greyhawk_ship_data *ship;
  char query[MAX_STRING_LENGTH];
  char escaped[130];
  MYSQL_RES *result;
  MYSQL_ROW row;
  int port_vnum;
  int listed = 0;

  ship = contract_context(ch, &port_vnum);
  if (ship == NULL)
  {
    return;
  }

  vessel_contracts_refresh_port(port_vnum);

  send_to_char(ch, "Freight board at %s:\r\n", world[IN_ROOM(ship->shipobj)].name);
  send_to_char(ch, "ID     Cargo            Qty  Payout  Destination\r\n");
  send_to_char(ch, "------ ---------------- ---- ------- ---------------------------\r\n");

  snprintf(query, sizeof(query),
           "SELECT fc.contract_id, tc.name, fc.quantity, fc.payout, fc.destination_vnum "
           "FROM freight_contracts fc JOIN trade_commodities tc "
           "ON tc.commodity_id = fc.commodity_id "
           "WHERE fc.origin_vnum = %d AND fc.status = %d ORDER BY fc.payout DESC",
           port_vnum, CONTRACT_STATUS_OPEN);
  if (mysql_query(conn, query))
  {
    send_to_char(ch, "The freight clerk cannot find the ledger.\r\n");
    return;
  }
  result = mysql_store_result(conn);
  if (result != NULL)
  {
    while ((row = mysql_fetch_row(result)) != NULL)
    {
      send_to_char(ch, "%-6s %-16s %4s %7s  %s\r\n", row[0], row[1], row[2], row[3],
                   port_name(atoi(row[4])));
      listed++;
    }
    mysql_free_result(result);
  }

  if (listed == 0)
  {
    send_to_char(ch, "  (no work offered here right now)\r\n");
  }

  /* The player's accepted contracts, wherever they were taken */
  mysql_real_escape_string(conn, escaped, GET_NAME(ch), strlen(GET_NAME(ch)));
  snprintf(query, sizeof(query),
           "SELECT fc.contract_id, tc.name, fc.quantity, fc.payout, fc.destination_vnum "
           "FROM freight_contracts fc JOIN trade_commodities tc "
           "ON tc.commodity_id = fc.commodity_id "
           "WHERE fc.taken_by = '%s' AND fc.status = %d",
           escaped, CONTRACT_STATUS_TAKEN);
  if (mysql_query(conn, query))
  {
    return;
  }
  result = mysql_store_result(conn);
  if (result != NULL)
  {
    listed = 0;
    while ((row = mysql_fetch_row(result)) != NULL)
    {
      if (listed == 0)
      {
        send_to_char(ch, "\r\nYour active contracts:\r\n");
      }
      send_to_char(ch, "%-6s %-16s %4s %7s  deliver to %s\r\n", row[0], row[1], row[2], row[3],
                   port_name(atoi(row[4])));
      listed++;
    }
    mysql_free_result(result);
  }

  send_to_char(ch, "\r\nUse 'contractaccept <id>' to take work, 'contractdeliver <id>' on "
                   "arrival.\r\n");
}

/**
 * Fetch one contract's fields.
 *
 * @return TRUE if found; out params filled
 */
static bool contract_fetch(int contract_id, int *commodity_id, int *quantity, int *payout,
                           int *destination, int *status, char *taken_by, size_t taken_size)
{
  char query[MAX_STRING_LENGTH];
  MYSQL_RES *result;
  MYSQL_ROW row;
  bool found = FALSE;

  snprintf(query, sizeof(query),
           "SELECT commodity_id, quantity, payout, destination_vnum, status, taken_by "
           "FROM freight_contracts WHERE contract_id = %d",
           contract_id);
  if (mysql_query(conn, query))
  {
    return FALSE;
  }

  result = mysql_store_result(conn);
  if (result == NULL)
  {
    return FALSE;
  }

  row = mysql_fetch_row(result);
  if (row != NULL)
  {
    *commodity_id = row[0] ? atoi(row[0]) : 0;
    *quantity = row[1] ? atoi(row[1]) : 0;
    *payout = row[2] ? atoi(row[2]) : 0;
    *destination = row[3] ? atoi(row[3]) : 0;
    *status = row[4] ? atoi(row[4]) : CONTRACT_STATUS_OPEN;
    strlcpy(taken_by, row[5] ? row[5] : "", taken_size);
    found = TRUE;
  }
  mysql_free_result(result);

  return found;
}

/**
 * contractaccept <id> - take a job and receive the cargo aboard.
 */
ACMD(do_contractaccept)
{
  struct greyhawk_ship_data *ship;
  char arg[MAX_INPUT_LENGTH];
  char query[MAX_STRING_LENGTH];
  char escaped[130];
  char taken_by[64];
  int port_vnum;
  int contract_id;
  int commodity_id, quantity, payout, destination, status;
  int lot = -1;
  int i;
  int empty = -1;

  ship = contract_context(ch, &port_vnum);
  if (ship == NULL)
  {
    return;
  }

  if (!vessel_helm_permitted(ch, ship))
  {
    send_to_char(ch, "You are not cleared to commit this ship to a contract.\r\n");
    return;
  }

  one_argument_u((char *)argument, arg);
  contract_id = atoi(arg);
  if (contract_id <= 0)
  {
    send_to_char(ch, "Accept which contract? See 'contracts'.\r\n");
    return;
  }

  if (!contract_fetch(contract_id, &commodity_id, &quantity, &payout, &destination, &status,
                      taken_by, sizeof(taken_by)))
  {
    send_to_char(ch, "No such contract.\r\n");
    return;
  }

  if (status != CONTRACT_STATUS_OPEN)
  {
    send_to_char(ch, "That contract has already been taken.\r\n");
    return;
  }

  /* Find or claim a hold bay for the freight */
  for (i = 0; i < MAX_CARGO_LOTS; i++)
  {
    if (ship->cargo[i].commodity_id == commodity_id)
    {
      lot = i;
      break;
    }
    if (empty < 0 && ship->cargo[i].commodity_id == 0)
    {
      empty = i;
    }
  }
  if (lot < 0)
  {
    lot = empty;
  }
  if (lot < 0)
  {
    send_to_char(ch, "The hold has no free bay for this freight.\r\n");
    return;
  }

  /* Weight check against effective capacity */
  {
    int capacity = vessel_effective_cargo_capacity(ship);
    int current = vessel_cargo_weight(ship);
    /* Unit weight comes from the commodity table via the trade module's
     * cache; approximate with the manifest's own accounting by loading and
     * re-measuring. */
    ship->cargo[lot].commodity_id = commodity_id;
    ship->cargo[lot].quantity += quantity;
    if (vessel_cargo_weight(ship) > capacity)
    {
      ship->cargo[lot].quantity -= quantity;
      if (ship->cargo[lot].quantity <= 0)
      {
        ship->cargo[lot].commodity_id = 0;
      }
      send_to_char(ch,
                   "That freight will not fit: the hold holds %d lbs and %d lbs are "
                   "already stowed.\r\n",
                   capacity, current);
      return;
    }
  }

  mysql_real_escape_string(conn, escaped, GET_NAME(ch), strlen(GET_NAME(ch)));
  snprintf(query, sizeof(query),
           "UPDATE freight_contracts SET status = %d, taken_by = '%s' "
           "WHERE contract_id = %d AND status = %d",
           CONTRACT_STATUS_TAKEN, escaped, contract_id, CONTRACT_STATUS_OPEN);
  if (mysql_query(conn, query) || mysql_affected_rows(conn) == 0)
  {
    /* Someone else took it between our read and write - roll the cargo back */
    ship->cargo[lot].quantity -= quantity;
    if (ship->cargo[lot].quantity <= 0)
    {
      ship->cargo[lot].commodity_id = 0;
    }
    send_to_char(ch, "Another captain just took that contract.\r\n");
    return;
  }

  vessel_db_save_cargo(ship);
  send_to_char(ch, "Contract %d accepted: %d units loaded, %d gold on delivery to %s.\r\n",
               contract_id, quantity, payout, port_name(destination));
  send_to_ship(ship, "Dockhands load %d units of freight aboard %s.", quantity, ship->name);
  log("Info: %s accepted freight contract %d (%d units to port %d)", GET_NAME(ch), contract_id,
      quantity, destination);
}

/**
 * contractdeliver <id> - deliver at the destination and collect.
 */
ACMD(do_contractdeliver)
{
  struct greyhawk_ship_data *ship;
  char arg[MAX_INPUT_LENGTH];
  char query[MAX_STRING_LENGTH];
  char taken_by[64];
  int port_vnum;
  int contract_id;
  int commodity_id, quantity, payout, destination, status;
  int lot = -1;
  int i;

  ship = contract_context(ch, &port_vnum);
  if (ship == NULL)
  {
    return;
  }

  one_argument_u((char *)argument, arg);
  contract_id = atoi(arg);
  if (contract_id <= 0)
  {
    send_to_char(ch, "Deliver which contract? See 'contracts'.\r\n");
    return;
  }

  if (!contract_fetch(contract_id, &commodity_id, &quantity, &payout, &destination, &status,
                      taken_by, sizeof(taken_by)))
  {
    send_to_char(ch, "No such contract.\r\n");
    return;
  }

  if (status != CONTRACT_STATUS_TAKEN || str_cmp(taken_by, GET_NAME(ch)))
  {
    send_to_char(ch, "That contract is not yours to deliver.\r\n");
    return;
  }

  if (destination != port_vnum)
  {
    send_to_char(ch, "This freight is bound for %s, not here.\r\n", port_name(destination));
    return;
  }

  for (i = 0; i < MAX_CARGO_LOTS; i++)
  {
    if (ship->cargo[i].commodity_id == commodity_id && ship->cargo[i].quantity >= quantity)
    {
      lot = i;
      break;
    }
  }
  if (lot < 0)
  {
    send_to_char(ch, "The freight is not aboard - you cannot deliver what you lost.\r\n");
    return;
  }

  ship->cargo[lot].quantity -= quantity;
  if (ship->cargo[lot].quantity <= 0)
  {
    ship->cargo[lot].commodity_id = 0;
  }
  vessel_db_save_cargo(ship);

  snprintf(query, sizeof(query), "UPDATE freight_contracts SET status = %d WHERE contract_id = %d",
           CONTRACT_STATUS_DONE, contract_id);
  if (mysql_query(conn, query))
  {
    log("SYSERR: contract completion update failed: %s", mysql_error(conn));
  }

  GET_GOLD(ch) += payout;
  send_to_char(ch, "Freight delivered. The consignee pays %d gold.\r\n", payout);
  send_to_ship(ship, "Dockhands unload %d units of freight from %s.", quantity, ship->name);
  log("Info: %s delivered freight contract %d for %d gold", GET_NAME(ch), contract_id, payout);
}

/**
 * contractabandon <id> - drop a contract, forfeiting the cargo.
 */
ACMD(do_contractabandon)
{
  char arg[MAX_INPUT_LENGTH];
  char query[MAX_STRING_LENGTH];
  char taken_by[64];
  int contract_id;
  int commodity_id, quantity, payout, destination, status;

  if (!mysql_available || conn == NULL)
  {
    send_to_char(ch, "The freight office is closed.\r\n");
    return;
  }

  one_argument_u((char *)argument, arg);
  contract_id = atoi(arg);
  if (contract_id <= 0)
  {
    send_to_char(ch, "Abandon which contract?\r\n");
    return;
  }

  if (!contract_fetch(contract_id, &commodity_id, &quantity, &payout, &destination, &status,
                      taken_by, sizeof(taken_by)))
  {
    send_to_char(ch, "No such contract.\r\n");
    return;
  }

  if (status != CONTRACT_STATUS_TAKEN || str_cmp(taken_by, GET_NAME(ch)))
  {
    send_to_char(ch, "That contract is not yours.\r\n");
    return;
  }

  /* Abandoning returns the job to the board; the freight stays aboard as
   * ordinary cargo the captain may sell (at a loss to their reputation
   * once Phase 07's piracy/faction work lands). */
  snprintf(query, sizeof(query),
           "UPDATE freight_contracts SET status = %d, taken_by = '' WHERE contract_id = %d",
           CONTRACT_STATUS_OPEN, contract_id);
  if (mysql_query(conn, query))
  {
    log("SYSERR: contract abandon update failed: %s", mysql_error(conn));
    send_to_char(ch, "The clerk cannot amend the ledger.\r\n");
    return;
  }

  send_to_char(ch, "You abandon contract %d. The cargo remains in your hold.\r\n", contract_id);
  log("Info: %s abandoned freight contract %d", GET_NAME(ch), contract_id);
}
