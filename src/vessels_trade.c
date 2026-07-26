/* ************************************************************************
 *      File:   vessels_trade.c                       Part of LuminariMUD  *
 *   Purpose:   Bulk cargo and port trading (Phase 07).                    *
 *              Commodities and per-port supply live in the database so    *
 *              builders tune the economy without a recompile. Prices move *
 *              with supply inside hard bounds, so arbitrage is a living   *
 *              gradient rather than an exploit.                           *
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
extern struct room_data *world;

/* A port sells at the listed price and buys at this percentage of it, so
 * round-tripping in one port always loses money. */
#define TRADE_SELL_PERCENT 85

/* Cached commodity definition */
struct commodity_def
{
  int id;
  char name[64];
  int base_price;
  int unit_weight;
};

#define MAX_COMMODITIES 32
static struct commodity_def commodity_cache[MAX_COMMODITIES];
static int num_commodities = 0;
static int restock_ticks = 0;

/**
 * Create the trade tables and seed a default commodity set.
 *
 * Mirrored by sql/components/vessels_phase7_schema.sql. Seeded rows are
 * builder-editable; the seed only runs when the table is empty.
 */
void vessel_trade_ensure_schema(void)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  bool empty = TRUE;

  if (!mysql_available || conn == NULL)
  {
    return;
  }

  if (mysql_query(conn, "CREATE TABLE IF NOT EXISTS trade_commodities ("
                        "  commodity_id INT AUTO_INCREMENT PRIMARY KEY,"
                        "  name VARCHAR(63) NOT NULL UNIQUE,"
                        "  base_price INT NOT NULL DEFAULT 10,"
                        "  unit_weight INT NOT NULL DEFAULT 10"
                        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"))
  {
    log("SYSERR: trade_commodities create failed: %s", mysql_error(conn));
    return;
  }

  if (mysql_query(conn, "CREATE TABLE IF NOT EXISTS port_commodities ("
                        "  port_vnum INT NOT NULL,"
                        "  commodity_id INT NOT NULL,"
                        "  supply INT NOT NULL DEFAULT 100,"
                        "  PRIMARY KEY (port_vnum, commodity_id),"
                        "  INDEX idx_port (port_vnum)"
                        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"))
  {
    log("SYSERR: port_commodities create failed: %s", mysql_error(conn));
    return;
  }

  /* Seed default goods only when the table is empty */
  if (mysql_query(conn, "SELECT COUNT(*) FROM trade_commodities"))
  {
    return;
  }
  result = mysql_store_result(conn);
  if (result != NULL)
  {
    row = mysql_fetch_row(result);
    if (row != NULL && row[0] != NULL && atoi(row[0]) > 0)
    {
      empty = FALSE;
    }
    mysql_free_result(result);
  }

  if (empty)
  {
    if (mysql_query(conn, "INSERT INTO trade_commodities (name, base_price, unit_weight) VALUES "
                          "('grain', 8, 20),"
                          "('salt', 14, 15),"
                          "('timber', 6, 40),"
                          "('iron', 30, 50),"
                          "('cloth', 25, 8),"
                          "('wine', 35, 25),"
                          "('spice', 90, 4),"
                          "('silk', 120, 3),"
                          "('gemstones', 250, 2)"))
    {
      log("SYSERR: trade_commodities seed failed: %s", mysql_error(conn));
    }
    else
    {
      log("Info: Seeded 9 default trade commodities");
    }
  }

  /* Cache the commodity list for the run */
  num_commodities = 0;
  if (mysql_query(conn, "SELECT commodity_id, name, base_price, unit_weight "
                        "FROM trade_commodities ORDER BY base_price"))
  {
    return;
  }
  result = mysql_store_result(conn);
  if (result == NULL)
  {
    return;
  }
  while ((row = mysql_fetch_row(result)) != NULL && num_commodities < MAX_COMMODITIES)
  {
    if (row[0] == NULL || row[1] == NULL)
    {
      continue;
    }
    commodity_cache[num_commodities].id = atoi(row[0]);
    strlcpy(commodity_cache[num_commodities].name, row[1],
            sizeof(commodity_cache[num_commodities].name));
    commodity_cache[num_commodities].base_price = row[2] ? atoi(row[2]) : 10;
    commodity_cache[num_commodities].unit_weight = row[3] ? atoi(row[3]) : 10;
    num_commodities++;
  }
  mysql_free_result(result);
  log("Info: Loaded %d trade commodities", num_commodities);
}

/**
 * Look up a cached commodity by id.
 */
static struct commodity_def *commodity_by_id(int id)
{
  int i;

  for (i = 0; i < num_commodities; i++)
  {
    if (commodity_cache[i].id == id)
    {
      return &commodity_cache[i];
    }
  }

  return NULL;
}

/**
 * Look up a cached commodity by (abbreviated) name.
 */
static struct commodity_def *commodity_by_name(const char *name)
{
  int i;

  if (name == NULL || !*name)
  {
    return NULL;
  }

  for (i = 0; i < num_commodities; i++)
  {
    if (is_abbrev(name, commodity_cache[i].name))
    {
      return &commodity_cache[i];
    }
  }

  return NULL;
}

/**
 * Price a commodity at a given supply level.
 *
 * Scarcity raises the price, glut lowers it, and the swing is clamped to
 * +/- TRADE_MAX_DRIFT percent so no route can ever pay unboundedly.
 *
 * @param base_price The commodity's base price
 * @param supply Port supply level (TRADE_SUPPLY_MIN..TRADE_SUPPLY_MAX)
 * @return Unit price in gold, at least 1
 */
int vessel_commodity_price(int base_price, int supply)
{
  int drift;
  int price;

  if (base_price < 1)
  {
    base_price = 1;
  }
  if (supply < TRADE_SUPPLY_MIN)
  {
    supply = TRADE_SUPPLY_MIN;
  }
  if (supply > TRADE_SUPPLY_MAX)
  {
    supply = TRADE_SUPPLY_MAX;
  }

  /* Positive drift when supply is below baseline (scarce), negative above */
  drift = ((TRADE_BASELINE_SUPPLY - supply) * TRADE_MAX_DRIFT) / TRADE_BASELINE_SUPPLY;
  if (drift > TRADE_MAX_DRIFT)
  {
    drift = TRADE_MAX_DRIFT;
  }
  if (drift < -TRADE_MAX_DRIFT)
  {
    drift = -TRADE_MAX_DRIFT;
  }

  price = base_price + (base_price * drift) / 100;
  return MAX(1, price);
}

/**
 * Read a port's supply for a commodity, creating the row on first contact.
 */
static int port_supply(int port_vnum, int commodity_id)
{
  char query[MAX_STRING_LENGTH];
  MYSQL_RES *result;
  MYSQL_ROW row;
  int supply = TRADE_BASELINE_SUPPLY;

  if (!mysql_available || conn == NULL)
  {
    return TRADE_BASELINE_SUPPLY;
  }

  snprintf(query, sizeof(query),
           "SELECT supply FROM port_commodities WHERE port_vnum = %d AND commodity_id = %d",
           port_vnum, commodity_id);
  if (mysql_query(conn, query))
  {
    return TRADE_BASELINE_SUPPLY;
  }

  result = mysql_store_result(conn);
  if (result == NULL)
  {
    return TRADE_BASELINE_SUPPLY;
  }

  row = mysql_fetch_row(result);
  if (row != NULL && row[0] != NULL)
  {
    supply = atoi(row[0]);
    mysql_free_result(result);
    return supply;
  }
  mysql_free_result(result);

  /* First visit: seed this port/commodity pair with a deterministic
   * variation from the vnum so ports differ without random drift. */
  supply = TRADE_BASELINE_SUPPLY + ((port_vnum * 7 + commodity_id * 31) % 121) - 60;
  if (supply < TRADE_SUPPLY_MIN)
  {
    supply = TRADE_SUPPLY_MIN;
  }
  snprintf(query, sizeof(query),
           "INSERT IGNORE INTO port_commodities (port_vnum, commodity_id, supply) "
           "VALUES (%d, %d, %d)",
           port_vnum, commodity_id, supply);
  if (mysql_query(conn, query))
  {
    log("SYSERR: port_supply seed failed: %s", mysql_error(conn));
  }

  return supply;
}

/**
 * Shift a port's supply after a trade, clamped to the configured band.
 */
static void port_adjust_supply(int port_vnum, int commodity_id, int delta)
{
  char query[MAX_STRING_LENGTH];
  int supply;

  if (!mysql_available || conn == NULL || delta == 0)
  {
    return;
  }

  supply = port_supply(port_vnum, commodity_id) + delta;
  if (supply < TRADE_SUPPLY_MIN)
  {
    supply = TRADE_SUPPLY_MIN;
  }
  if (supply > TRADE_SUPPLY_MAX)
  {
    supply = TRADE_SUPPLY_MAX;
  }

  snprintf(query, sizeof(query),
           "UPDATE port_commodities SET supply = %d WHERE port_vnum = %d AND commodity_id = %d",
           supply, port_vnum, commodity_id);
  if (mysql_query(conn, query))
  {
    log("SYSERR: port_adjust_supply failed: %s", mysql_error(conn));
  }
}

/**
 * Total weight of bulk cargo aboard.
 */
int vessel_cargo_weight(const struct greyhawk_ship_data *ship)
{
  struct commodity_def *def;
  int total = 0;
  int i;

  if (ship == NULL)
  {
    return 0;
  }

  for (i = 0; i < MAX_CARGO_LOTS; i++)
  {
    if (ship->cargo[i].commodity_id == 0)
    {
      continue;
    }
    def = commodity_by_id(ship->cargo[i].commodity_id);
    if (def != NULL)
    {
      total += def->unit_weight * ship->cargo[i].quantity;
    }
  }

  return total;
}

/**
 * Find the ship's lot for a commodity, or the first empty lot.
 *
 * @param create TRUE to claim an empty lot when none exists
 * @return Lot index, or -1 if the hold has no room for a new commodity
 */
static int vessel_cargo_lot(struct greyhawk_ship_data *ship, int commodity_id, bool create)
{
  int i;
  int empty = -1;

  for (i = 0; i < MAX_CARGO_LOTS; i++)
  {
    if (ship->cargo[i].commodity_id == commodity_id)
    {
      return i;
    }
    if (empty < 0 && ship->cargo[i].commodity_id == 0)
    {
      empty = i;
    }
  }

  return create ? empty : -1;
}

/**
 * Persist the cargo manifest (delete-and-reinsert, idempotent).
 *
 * Bulk lots ride in ship_cargo_manifest with item_vnum = commodity id and
 * cargo_room = 0, distinguishing them from crated object cargo.
 */
void vessel_db_save_cargo(struct greyhawk_ship_data *ship)
{
  char query[MAX_STRING_LENGTH];
  char escaped[130];
  struct commodity_def *def;
  int i;

  if (!mysql_available || conn == NULL || ship == NULL)
  {
    return;
  }

  snprintf(query, sizeof(query),
           "DELETE FROM ship_cargo_manifest WHERE ship_id = '%s' AND cargo_room = 0", ship->id);
  if (mysql_query(conn, query))
  {
    log("SYSERR: vessel_db_save_cargo (clear) failed for ship %s: %s", ship->id, mysql_error(conn));
    return;
  }

  for (i = 0; i < MAX_CARGO_LOTS; i++)
  {
    if (ship->cargo[i].commodity_id == 0 || ship->cargo[i].quantity <= 0)
    {
      continue;
    }
    def = commodity_by_id(ship->cargo[i].commodity_id);
    mysql_real_escape_string(conn, escaped, def ? def->name : "cargo",
                             strlen(def ? def->name : "cargo"));
    snprintf(query, sizeof(query),
             "INSERT INTO ship_cargo_manifest (ship_id, cargo_room, item_vnum, item_name, "
             "item_count, item_weight) VALUES ('%s', 0, %d, '%s', %d, %d)",
             ship->id, ship->cargo[i].commodity_id, escaped, ship->cargo[i].quantity,
             def ? def->unit_weight * ship->cargo[i].quantity : 0);
    if (mysql_query(conn, query))
    {
      log("SYSERR: vessel_db_save_cargo (insert) failed for ship %s: %s", ship->id,
          mysql_error(conn));
    }
  }
}

/**
 * Load the bulk cargo manifest.
 */
void vessel_db_load_cargo(struct greyhawk_ship_data *ship)
{
  char query[MAX_STRING_LENGTH];
  MYSQL_RES *result;
  MYSQL_ROW row;
  int lot = 0;

  if (!mysql_available || conn == NULL || ship == NULL)
  {
    return;
  }

  snprintf(query, sizeof(query),
           "SELECT item_vnum, item_count FROM ship_cargo_manifest "
           "WHERE ship_id = '%s' AND cargo_room = 0",
           ship->id);
  if (mysql_query(conn, query))
  {
    return;
  }

  result = mysql_store_result(conn);
  if (result == NULL)
  {
    return;
  }

  while ((row = mysql_fetch_row(result)) != NULL && lot < MAX_CARGO_LOTS)
  {
    if (row[0] == NULL || row[1] == NULL)
    {
      continue;
    }
    ship->cargo[lot].commodity_id = atoi(row[0]);
    ship->cargo[lot].quantity = atoi(row[1]);
    lot++;
  }
  ship->num_cargo_lots = lot;
  mysql_free_result(result);
}

/**
 * Supply drift tick: every port's stock creeps back toward baseline, so a
 * route milked flat recovers and a route left alone normalizes.
 */
void vessel_trade_restock_tick(void)
{
  if (!mysql_available || conn == NULL)
  {
    return;
  }

  restock_ticks++;
  if (restock_ticks < TRADE_RESTOCK_INTERVAL)
  {
    return;
  }
  restock_ticks = 0;

  if (mysql_query(conn, "UPDATE port_commodities SET supply = supply + "
                        "GREATEST(-5, LEAST(5, 100 - supply))"))
  {
    log("SYSERR: vessel_trade_restock_tick failed: %s", mysql_error(conn));
    return;
  }

  VSSL_DEBUG("Trade restock tick applied");
}

/**
 * Resolve the trading context: the player's ship, moored at a dock.
 *
 * @param port_vnum Out: the dock room's vnum
 * @return The ship, or NULL with a message sent
 */
static struct greyhawk_ship_data *trade_context(struct char_data *ch, int *port_vnum)
{
  struct greyhawk_ship_data *ship;

  ship = get_ship_from_room(IN_ROOM(ch));
  if (ship == NULL)
  {
    send_to_char(ch, "You must be aboard a ship to trade cargo.\r\n");
    return NULL;
  }

  if (ship->shipobj == NULL || IN_ROOM(ship->shipobj) == NOWHERE ||
      !ROOM_FLAGGED(IN_ROOM(ship->shipobj), ROOM_DOCKABLE))
  {
    send_to_char(ch, "You must be moored at a port to trade.\r\n");
    return NULL;
  }

  if (num_commodities == 0)
  {
    send_to_char(ch, "No goods are traded in these waters.\r\n");
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
 * market - list this port's commodity prices.
 */
ACMD(do_market)
{
  struct greyhawk_ship_data *ship;
  int port_vnum;
  int supply;
  int price;
  int i;

  ship = trade_context(ch, &port_vnum);
  if (ship == NULL)
  {
    return;
  }

  send_to_char(ch, "Market at %s:\r\n", world[IN_ROOM(ship->shipobj)].name);
  send_to_char(ch, "Commodity        Wt/unit    Buy   Sell  Local supply\r\n");
  send_to_char(ch, "---------------- ------- ------ ------ ------------\r\n");
  for (i = 0; i < num_commodities; i++)
  {
    supply = port_supply(port_vnum, commodity_cache[i].id);
    price = vessel_commodity_price(commodity_cache[i].base_price, supply);
    send_to_char(ch, "%-16s %7d %6d %6d  %s\r\n", commodity_cache[i].name,
                 commodity_cache[i].unit_weight, price, price * TRADE_SELL_PERCENT / 100,
                 supply < 60 ? "scarce" : (supply > 160 ? "glutted" : "steady"));
  }
  send_to_char(ch, "Hold: %d of %d lbs used.\r\n", vessel_cargo_weight(ship),
               vessel_effective_cargo_capacity(ship));
}

/**
 * cargobuy <commodity> <quantity> - load bulk cargo.
 */
ACMD(do_cargobuy)
{
  struct greyhawk_ship_data *ship;
  struct commodity_def *def;
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  int port_vnum;
  int quantity;
  int price;
  int cost;
  int lot;
  int capacity;

  ship = trade_context(ch, &port_vnum);
  if (ship == NULL)
  {
    return;
  }

  if (!vessel_helm_permitted(ch, ship))
  {
    send_to_char(ch, "You are not cleared to trade on this ship's account.\r\n");
    return;
  }

  two_arguments_u((char *)argument, arg1, arg2);
  if (!*arg1 || !*arg2)
  {
    send_to_char(ch, "Usage: cargobuy <commodity> <quantity>\r\n");
    return;
  }

  def = commodity_by_name(arg1);
  if (def == NULL)
  {
    send_to_char(ch, "No such commodity here. Check 'market'.\r\n");
    return;
  }

  quantity = atoi(arg2);
  if (quantity < 1)
  {
    send_to_char(ch, "Buy how many units?\r\n");
    return;
  }

  capacity = vessel_effective_cargo_capacity(ship);
  if (vessel_cargo_weight(ship) + def->unit_weight * quantity > capacity)
  {
    send_to_char(ch, "That won't fit: %d units weigh %d lbs and the hold has %d lbs free.\r\n",
                 quantity, def->unit_weight * quantity, capacity - vessel_cargo_weight(ship));
    return;
  }

  price = vessel_commodity_price(def->base_price, port_supply(port_vnum, def->id));
  cost = price * quantity;
  if (GET_GOLD(ch) < cost)
  {
    send_to_char(ch, "%d units of %s cost %d gold; you have %d.\r\n", quantity, def->name, cost,
                 GET_GOLD(ch));
    return;
  }

  lot = vessel_cargo_lot(ship, def->id, TRUE);
  if (lot < 0)
  {
    send_to_char(ch, "The hold has no free bay for another kind of goods.\r\n");
    return;
  }

  GET_GOLD(ch) -= cost;
  ship->cargo[lot].commodity_id = def->id;
  ship->cargo[lot].quantity += quantity;

  /* Buying drains the port's stock, nudging its price up */
  port_adjust_supply(port_vnum, def->id, -quantity);
  vessel_db_save_cargo(ship);

  send_to_char(ch, "You load %d units of %s for %d gold (%d each).\r\n", quantity, def->name, cost,
               price);
  log("Info: %s bought %d %s at port %d for %d gold", GET_NAME(ch), quantity, def->name, port_vnum,
      cost);
}

/**
 * cargosell <commodity> <quantity> - unload bulk cargo for gold.
 */
ACMD(do_cargosell)
{
  struct greyhawk_ship_data *ship;
  struct commodity_def *def;
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  int port_vnum;
  int quantity;
  int price;
  int revenue;
  int lot;

  ship = trade_context(ch, &port_vnum);
  if (ship == NULL)
  {
    return;
  }

  if (!vessel_helm_permitted(ch, ship))
  {
    send_to_char(ch, "You are not cleared to trade on this ship's account.\r\n");
    return;
  }

  two_arguments_u((char *)argument, arg1, arg2);
  if (!*arg1)
  {
    send_to_char(ch, "Usage: cargosell <commodity> <quantity|all>\r\n");
    return;
  }

  def = commodity_by_name(arg1);
  if (def == NULL)
  {
    send_to_char(ch, "No such commodity. Check 'cargomanifest'.\r\n");
    return;
  }

  lot = vessel_cargo_lot(ship, def->id, FALSE);
  if (lot < 0 || ship->cargo[lot].quantity <= 0)
  {
    send_to_char(ch, "You carry no %s.\r\n", def->name);
    return;
  }

  if (!*arg2 || !str_cmp(arg2, "all"))
  {
    quantity = ship->cargo[lot].quantity;
  }
  else
  {
    quantity = atoi(arg2);
  }

  if (quantity < 1)
  {
    send_to_char(ch, "Sell how many units?\r\n");
    return;
  }
  if (quantity > ship->cargo[lot].quantity)
  {
    send_to_char(ch, "You only carry %d units of %s.\r\n", ship->cargo[lot].quantity, def->name);
    return;
  }

  price = vessel_commodity_price(def->base_price, port_supply(port_vnum, def->id)) *
          TRADE_SELL_PERCENT / 100;
  price = MAX(1, price);
  revenue = price * quantity;

  ship->cargo[lot].quantity -= quantity;
  if (ship->cargo[lot].quantity == 0)
  {
    ship->cargo[lot].commodity_id = 0;
  }
  GET_GOLD(ch) += revenue;

  /* Selling floods the local market, nudging its price down */
  port_adjust_supply(port_vnum, def->id, quantity);
  vessel_db_save_cargo(ship);

  send_to_char(ch, "You sell %d units of %s for %d gold (%d each).\r\n", quantity, def->name,
               revenue, price);
  log("Info: %s sold %d %s at port %d for %d gold", GET_NAME(ch), quantity, def->name, port_vnum,
      revenue);
}

/**
 * manifest - show the ship's bulk cargo.
 */
ACMD(do_cargomanifest)
{
  struct greyhawk_ship_data *ship;
  struct commodity_def *def;
  int i;
  int carried = 0;

  ship = get_ship_from_room(IN_ROOM(ch));
  if (ship == NULL)
  {
    send_to_char(ch, "You must be aboard a ship.\r\n");
    return;
  }

  send_to_char(ch, "Cargo manifest for %s:\r\n", ship->name);
  for (i = 0; i < MAX_CARGO_LOTS; i++)
  {
    if (ship->cargo[i].commodity_id == 0 || ship->cargo[i].quantity <= 0)
    {
      continue;
    }
    def = commodity_by_id(ship->cargo[i].commodity_id);
    send_to_char(ch, "  %-16s %5d units  %6d lbs\r\n", def ? def->name : "unknown goods",
                 ship->cargo[i].quantity, def ? def->unit_weight * ship->cargo[i].quantity : 0);
    carried++;
  }

  if (carried == 0)
  {
    send_to_char(ch, "  (empty)\r\n");
  }

  send_to_char(ch, "Hold: %d of %d lbs used.\r\n", vessel_cargo_weight(ship),
               vessel_effective_cargo_capacity(ship));
}
