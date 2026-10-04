/* ************************************************************************
 *      File:   vessels_settlement.c                  Part of LuminariMUD  *
 *   Purpose:   Two-phase vessel settlements (study step S14).             *
 *              A cargo trade, a freight acceptance and a dock-fee payment *
 *              write the ship's side to MariaDB and the captain's gold to *
 *              the player file. A settlement row ties the two: it is      *
 *              written with the ship's side, the file is saved with its   *
 *              id, and then it is deleted. A row left behind is judged    *
 *              by that file: deleted when the file names it, undone       *
 *              otherwise.                                                 *
 * ********************************************************************** */

#include "conf.h"
#include "core/sysdep.h"
#include "core/structs.h"
#include "core/utils.h"
#include "core/comm.h"
#include "core/db.h"
#include "character/rewards.h"
#include "vessels.h"
#include "database/mysql.h"

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/* A ship and a player have one open settlement each (the table's unique
 * keys), so a reconcile reads two rows at most. */
#define SETTLEMENT_ROWS 2

/**
 * Create the settlement table.
 * Mirrored by sql/components/vessels_phase25_schema.sql.
 */
void vessel_settlement_ensure_schema(void)
{
  if (!mysql_available || conn == NULL)
  {
    return;
  }

  if (mysql_query(conn, "CREATE TABLE IF NOT EXISTS vessel_settlements ("
                        "  settlement_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,"
                        "  player_id INT UNSIGNED NOT NULL,"
                        "  ship_id INT NOT NULL,"
                        "  port_vnum INT NOT NULL DEFAULT 0,"
                        "  commodity_id INT NOT NULL DEFAULT 0,"
                        "  supply_delta INT NOT NULL DEFAULT 0,"
                        "  cargo_delta INT NOT NULL DEFAULT 0,"
                        "  contract_id INT NOT NULL DEFAULT 0,"
                        "  fee_amount INT NOT NULL DEFAULT 0,"
                        "  fee_port INT NOT NULL DEFAULT 0,"
                        "  fee_clan INT NOT NULL DEFAULT 0,"
                        "  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                        "  UNIQUE KEY uk_vessel_settlement_ship (ship_id),"
                        "  UNIQUE KEY uk_vessel_settlement_player (player_id)"
                        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"))
  {
    log("SYSERR: vessel_settlements create failed: %s", mysql_error(conn));
  }
}

/** The hull in a fleet slot, in play or stowed, or NULL. */
static struct greyhawk_ship_data *settlement_ship(int ship_id)
{
  struct greyhawk_ship_data *ship;

  if (ship_id < 0 || ship_id >= GREYHAWK_MAXSHIPS)
  {
    return NULL;
  }
  ship = &greyhawk_ships[ship_id];
  return is_valid_ship(ship) || ship->stowed ? ship : NULL;
}

/** What a settlement was, for the player and the log. */
static const char *settlement_account(const struct vessel_settlement *settlement)
{
  if (settlement->contract_id > 0)
  {
    return "freight contract";
  }
  return settlement->fee_amount > 0 ? "dock-fee payment" : "cargo trade";
}

/**
 * Is the settlement's row in the database? The read locks the row, so after
 * a COMMIT that got no reply it waits until the lost session's transaction
 * has ended one way or the other.
 *
 * @return 1 if it is, 0 if it is not, -1 if it could not be read
 */
static int settlement_recorded(unsigned long long id)
{
  PREPARED_STMT *statement;
  int recorded = -1;

  statement = mysql_stmt_create(conn);
  if (statement != NULL &&
      mysql_stmt_prepare_query(statement, "SELECT COUNT(*) FROM vessel_settlements "
                                          "WHERE settlement_id = ? LOCK IN SHARE MODE") &&
      mysql_stmt_bind_param_long(statement, 0, (long)id) &&
      mysql_stmt_execute_prepared(statement) && mysql_stmt_fetch_row(statement))
  {
    recorded = mysql_stmt_get_int(statement, 0) > 0 ? 1 : 0;
  }
  mysql_stmt_cleanup(statement);
  return recorded;
}

/** Delete a settlement's row, in the caller's transaction if one is open. */
static bool settlement_delete(unsigned long long id)
{
  PREPARED_STMT *statement;
  bool deleted;

  statement = mysql_stmt_create(conn);
  deleted = statement != NULL &&
            mysql_stmt_prepare_query(statement,
                                     "DELETE FROM vessel_settlements WHERE settlement_id = ?") &&
            mysql_stmt_bind_param_long(statement, 0, (long)id) &&
            mysql_stmt_execute_prepared(statement);
  mysql_stmt_cleanup(statement);
  return deleted;
}

/**
 * Delete a purged hull's settlement with her other rows, in the caller's
 * transaction: her fleet slot will name another hull.
 */
bool vessel_settlement_forget_ship(int shipnum)
{
  PREPARED_STMT *statement;
  bool deleted;

  statement = mysql_stmt_create(conn);
  deleted =
      statement != NULL &&
      mysql_stmt_prepare_query(statement, "DELETE FROM vessel_settlements WHERE ship_id = ?") &&
      mysql_stmt_bind_param_int(statement, 0, shipnum) && mysql_stmt_execute_prepared(statement);
  mysql_stmt_cleanup(statement);
  return deleted;
}

/**
 * Record the settlement's row and commit the caller's open transaction, which
 * holds the ship's side. If the COMMIT goes unanswered, the row says whether
 * it took effect. If the row cannot be read either, the settlement counts as
 * not recorded and the hull remembers it (settlement_unresolved): the caller
 * takes it out of memory, and the next reconcile undoes it in the database if
 * it was recorded after all.
 *
 * @return FALSE when the settlement is not recorded, or may not be; the
 *         transaction is then over
 */
bool vessel_settlement_commit(struct char_data *ch, struct greyhawk_ship_data *ship,
                              struct vessel_settlement *settlement)
{
  PREPARED_STMT *statement;
  enum mysql_commit_result result;
  bool inserted;
  int recorded;

  settlement->id = 0;
  settlement->player_id = IS_NPC(ch) ? 0 : GET_IDNUM(ch);
  settlement->ship_id = ship->shipnum;

  statement = mysql_stmt_create(conn);
  inserted =
      statement != NULL &&
      mysql_stmt_prepare_query(statement, "INSERT INTO vessel_settlements (player_id, ship_id, "
                                          "port_vnum, commodity_id, supply_delta, cargo_delta, "
                                          "contract_id, fee_amount, fee_port, fee_clan) "
                                          "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)") &&
      mysql_stmt_bind_param_long(statement, 0, settlement->player_id) &&
      mysql_stmt_bind_param_int(statement, 1, settlement->ship_id) &&
      mysql_stmt_bind_param_int(statement, 2, settlement->port_vnum) &&
      mysql_stmt_bind_param_int(statement, 3, settlement->commodity_id) &&
      mysql_stmt_bind_param_int(statement, 4, settlement->supply_delta) &&
      mysql_stmt_bind_param_int(statement, 5, settlement->cargo_delta) &&
      mysql_stmt_bind_param_int(statement, 6, settlement->contract_id) &&
      mysql_stmt_bind_param_int(statement, 7, settlement->fee_amount) &&
      mysql_stmt_bind_param_int(statement, 8, settlement->fee_port) &&
      mysql_stmt_bind_param_int(statement, 9, settlement->fee_clan) &&
      mysql_stmt_execute_prepared(statement);
  if (inserted)
  {
    settlement->id = mysql_stmt_insert_id(statement->stmt);
  }
  mysql_stmt_cleanup(statement);
  if (!inserted)
  {
    log("SYSERR: Could not record a %s settlement for ship %d", settlement_account(settlement),
        ship->shipnum);
    mysql_query(conn, "ROLLBACK");
    return FALSE;
  }

  result = mysql_commit_transaction(conn);
  if (result == MYSQL_COMMIT_UNANSWERED)
  {
    recorded = settlement_recorded(settlement->id);
    if (recorded > 0)
    {
      result = MYSQL_COMMIT_DONE;
    }
    else if (recorded < 0)
    {
      ship->settlement_unresolved = settlement->id;
      log("SYSERR: Settlement %llu of ship %d got no reply to its COMMIT and cannot be read "
          "back; her accounts are held until it can",
          settlement->id, ship->shipnum);
    }
  }
  return result == MYSQL_COMMIT_DONE;
}

/** Change the hold's lot of the settlement's goods by its cargo delta. */
static void settlement_restow(struct greyhawk_ship_data *ship,
                              const struct vessel_settlement *settlement)
{
  int lot;

  lot = vessel_cargo_lot(ship, settlement->commodity_id, settlement->cargo_delta > 0);
  if (lot < 0)
  {
    if (settlement->cargo_delta > 0)
    {
      log("SYSERR: Ship %d has no free bay for the %d units of commodity %d that settlement "
          "%llu returns; they are left ashore",
          ship->shipnum, settlement->cargo_delta, settlement->commodity_id, settlement->id);
    }
    return;
  }

  ship->cargo[lot].commodity_id = settlement->commodity_id;
  ship->cargo[lot].quantity = MAX(0, ship->cargo[lot].quantity + settlement->cargo_delta);
  if (ship->cargo[lot].quantity == 0)
  {
    ship->cargo[lot].commodity_id = 0;
  }
}

/** The undo's writes, inside its transaction. */
static bool settlement_undo_write(struct greyhawk_ship_data *ship,
                                  const struct vessel_settlement *settlement)
{
  PREPARED_STMT *statement;
  bool written = TRUE;

  if (settlement->supply_delta != 0)
  {
    statement = mysql_stmt_create(conn);
    written = statement != NULL &&
              mysql_stmt_prepare_query(statement, "UPDATE port_commodities "
                                                  "SET supply = LEAST(?, GREATEST(?, supply + ?)) "
                                                  "WHERE port_vnum = ? AND commodity_id = ?") &&
              mysql_stmt_bind_param_int(statement, 0, TRADE_SUPPLY_MAX) &&
              mysql_stmt_bind_param_int(statement, 1, TRADE_SUPPLY_MIN) &&
              mysql_stmt_bind_param_int(statement, 2, settlement->supply_delta) &&
              mysql_stmt_bind_param_int(statement, 3, settlement->port_vnum) &&
              mysql_stmt_bind_param_int(statement, 4, settlement->commodity_id) &&
              mysql_stmt_execute_prepared(statement);
    mysql_stmt_cleanup(statement);
  }
  if (written && settlement->contract_id > 0)
  {
    /* Only the captain of this settlement can hold the contract: delivering
     * and abandoning it wait for the settlement. */
    statement = mysql_stmt_create(conn);
    written = statement != NULL &&
              mysql_stmt_prepare_query(statement,
                                       "UPDATE freight_contracts SET status = ?, taken_by = '' "
                                       "WHERE contract_id = ? AND status = ?") &&
              mysql_stmt_bind_param_int(statement, 0, CONTRACT_STATUS_OPEN) &&
              mysql_stmt_bind_param_int(statement, 1, settlement->contract_id) &&
              mysql_stmt_bind_param_int(statement, 2, CONTRACT_STATUS_TAKEN) &&
              mysql_stmt_execute_prepared(statement);
    mysql_stmt_cleanup(statement);
  }
  if (written && settlement->cargo_delta != 0)
  {
    written = vessel_db_save_cargo(ship);
  }
  if (written && settlement->fee_amount > 0)
  {
    written = vessel_db_save_runtime(ship);
  }
  return written && settlement_delete(settlement->id);
}

/**
 * Undo a settlement's ship side, in memory and in one transaction: the
 * port's supply moves back by the row's delta, the freight contract is
 * reopened, the hold's lot changes by the row's delta, the dock fee is owed
 * again, and the row is deleted. No gold moves: a settlement is undone only
 * when no player file holds its gold.
 *
 * The hold changes in memory unless the hull remembers this settlement as
 * one already taken out of it. A hull that already owes a fee keeps that
 * one: a runtime save rewrites the balance from memory at any time, so the
 * fee is restored, not added.
 *
 * If the undo cannot be recorded, or its COMMIT goes unanswered and the row
 * cannot be read back, memory stays undone and the hull remembers the
 * settlement for the next reconcile.
 *
 * A hull that is not in memory (boot could not rebuild her and left her rows)
 * keeps her settlement: her manifest and fee are not undone without her. It
 * is undone once she is loaded, or deleted with her rows when she is purged.
 *
 * @return TRUE once the undo is committed
 */
static bool settlement_undo(const struct vessel_settlement *settlement)
{
  struct greyhawk_ship_data *ship;
  enum mysql_commit_result result;

  ship = settlement_ship(settlement->ship_id);
  if (ship == NULL)
  {
    log("SYSERR: The %s settlement %llu waits for ship %d, which is not in memory",
        settlement_account(settlement), settlement->id, settlement->ship_id);
    return FALSE;
  }
  if (settlement->cargo_delta != 0 && ship->settlement_unresolved != settlement->id)
  {
    settlement_restow(ship, settlement);
  }
  if (settlement->fee_amount > 0 && ship->dock_fee_balance == 0)
  {
    ship->dock_fee_balance = settlement->fee_amount;
    ship->dock_fee_port = settlement->fee_port;
    ship->dock_fee_clan = settlement->fee_clan;
  }
  /* The manifest is written from memory below. */
  ship->settlement_unresolved = 0;

  result = MYSQL_COMMIT_REFUSED;
  if (mysql_query(conn, "START TRANSACTION") == 0)
  {
    if (settlement_undo_write(ship, settlement))
    {
      result = mysql_commit_transaction(conn);
    }
    else
    {
      mysql_query(conn, "ROLLBACK");
    }
  }
  if (result == MYSQL_COMMIT_DONE ||
      (result == MYSQL_COMMIT_UNANSWERED && settlement_recorded(settlement->id) == 0))
  {
    return TRUE;
  }

  ship->settlement_unresolved = settlement->id;
  log("SYSERR: Could not undo the %s settlement %llu of ship %d; her accounts are held until "
      "it is undone",
      settlement_account(settlement), settlement->id, settlement->ship_id);
  return FALSE;
}

/**
 * Pay a recorded settlement: move the captain's gold and save it with the
 * settlement's id, then delete the row. A row whose delete fails is deleted
 * by the next reconcile, which finds its id in the file.
 *
 * If the save fails, the gold and the id go back and the settlement is
 * undone. The captain is told it is undone only once the undo is committed.
 *
 * @param gold The captain's gold, signed
 * @param account What is settled, for the captain ("trade")
 * @return FALSE when the settlement was not paid
 */
bool vessel_settlement_pay(struct char_data *ch, const struct vessel_settlement *settlement,
                           int gold, const char *account)
{
  unsigned long long old_id;
  int old_gold;

  if (!IS_NPC(ch))
  {
    old_gold = GET_GOLD(ch);
    old_id = GET_VESSEL_SETTLEMENT(ch);
    award_gold(ch, gold);
    GET_VESSEL_SETTLEMENT(ch) = settlement->id;
    if (save_char_checked(ch, 0))
    {
      if (!settlement_delete(settlement->id))
      {
        log("SYSERR: Could not close the paid settlement %llu of ship %d; the next reconcile "
            "closes it",
            settlement->id, settlement->ship_id);
      }
      return TRUE;
    }
    award_set_points(ch, AWARD_GOLD, old_gold);
    GET_VESSEL_SETTLEMENT(ch) = old_id;
  }

  if (settlement_undo(settlement))
  {
    send_to_char(ch, "Your gold could not be recorded, so the %s is undone.\r\n", account);
  }
  else
  {
    send_to_char(ch,
                 "Your gold could not be recorded, and the %s could not be undone yet. No gold "
                 "changed hands; the harbor office holds this ship's accounts until it is "
                 "undone.\r\n",
                 account);
  }
  return FALSE;
}

/**
 * Read the open settlements of a ship or a player.
 *
 * @return How many there are (rows holds the first SETTLEMENT_ROWS), or -1
 *         if they could not be read
 */
static int settlement_rows(int ship_id, long player_id, struct vessel_settlement *rows)
{
  PREPARED_STMT *statement;
  int count = -1;

  statement = mysql_stmt_create(conn);
  if (statement != NULL &&
      mysql_stmt_prepare_query(statement,
                               "SELECT settlement_id, player_id, ship_id, port_vnum, "
                               "commodity_id, supply_delta, cargo_delta, contract_id, "
                               "fee_amount, fee_port, fee_clan FROM vessel_settlements "
                               "WHERE ship_id = ? OR player_id = ? ORDER BY settlement_id") &&
      mysql_stmt_bind_param_int(statement, 0, ship_id) &&
      mysql_stmt_bind_param_long(statement, 1, player_id) && mysql_stmt_execute_prepared(statement))
  {
    count = 0;
    while (mysql_stmt_fetch_row(statement))
    {
      if (count < SETTLEMENT_ROWS)
      {
        rows[count].id = mysql_stmt_get_ulong(statement, 0);
        rows[count].player_id = (long)mysql_stmt_get_long(statement, 1);
        rows[count].ship_id = mysql_stmt_get_int(statement, 2);
        rows[count].port_vnum = mysql_stmt_get_int(statement, 3);
        rows[count].commodity_id = mysql_stmt_get_int(statement, 4);
        rows[count].supply_delta = mysql_stmt_get_int(statement, 5);
        rows[count].cargo_delta = mysql_stmt_get_int(statement, 6);
        rows[count].contract_id = mysql_stmt_get_int(statement, 7);
        rows[count].fee_amount = mysql_stmt_get_int(statement, 8);
        rows[count].fee_port = mysql_stmt_get_int(statement, 9);
        rows[count].fee_clan = mysql_stmt_get_int(statement, 10);
      }
      count++;
    }
  }
  mysql_stmt_cleanup(statement);
  return count;
}

/**
 * Does the player file of the settlement's captain hold its gold: is its id
 * the one the file was saved with? Outside vessel_settlement_pay() a
 * character in the game carries the id that is in the file, so the acting
 * character is read from memory and anyone else from the file. A mob and a
 * removed player have no file.
 *
 * @return 1 if it does, 0 if it does not, -1 if the file could not be read
 */
static int settlement_covered(struct char_data *ch, const struct vessel_settlement *settlement)
{
  struct char_data *captain;
  const char *name;
  int covered;

  if (settlement->player_id == 0)
  {
    return 0;
  }
  if (ch != NULL && !IS_NPC(ch) && GET_IDNUM(ch) == settlement->player_id)
  {
    return GET_VESSEL_SETTLEMENT(ch) == settlement->id ? 1 : 0;
  }

  name = get_name_by_id(settlement->player_id);
  if (name == NULL)
  {
    return 0;
  }
  captain = new_char();
  if (load_char(name, captain) < 0)
  {
    log("SYSERR: Could not read %s's player file to settle settlement %llu of ship %d", name,
        settlement->id, settlement->ship_id);
    free_char(captain);
    return -1;
  }
  covered = GET_VESSEL_SETTLEMENT(captain) == settlement->id ? 1 : 0;
  free_char(captain);
  return covered;
}

/**
 * Settle every open settlement of this ship and of this player (either may
 * be NULL): one whose gold is in its captain's player file is deleted, and
 * any other is undone. A player whose own settlement is undone is told.
 *
 * @return TRUE when none is left open
 */
bool vessel_settlements_reconcile(struct char_data *ch, struct greyhawk_ship_data *ship)
{
  struct vessel_settlement rows[SETTLEMENT_ROWS];
  bool settled = TRUE;
  long player_id;
  int covered;
  int count;
  int i;

  if (!mysql_available || conn == NULL)
  {
    return ship == NULL || ship->settlement_unresolved == 0;
  }

  player_id = ch != NULL && !IS_NPC(ch) ? GET_IDNUM(ch) : 0;
  count = settlement_rows(ship != NULL ? ship->shipnum : -1, player_id, rows);
  if (count < 0)
  {
    return FALSE;
  }

  for (i = 0; i < count && i < SETTLEMENT_ROWS; i++)
  {
    covered = settlement_covered(ch, &rows[i]);
    if (covered > 0)
    {
      settled = settlement_delete(rows[i].id) && settled;
      continue;
    }
    if (covered < 0 || !settlement_undo(&rows[i]))
    {
      settled = FALSE;
      continue;
    }

    if (ch != NULL && player_id != 0 && rows[i].player_id == player_id)
    {
      send_to_char(ch,
                   "The harbor office never recorded your gold for a %s aboard %s, so it has "
                   "been undone.\r\n",
                   settlement_account(&rows[i]), greyhawk_ships[rows[i].ship_id].name);
    }
    log("Info: Undid the unpaid %s settlement %llu of player %ld aboard ship %d",
        settlement_account(&rows[i]), rows[i].id, rows[i].player_id, rows[i].ship_id);
  }

  /* Every row of the ship was read. One she remembers and that is not among
   * them was never recorded, unless its COMMIT is still on its way: the
   * locking read waits for that. */
  settled = settled && count <= SETTLEMENT_ROWS;
  if (ship != NULL && settled && ship->settlement_unresolved != 0)
  {
    settled = settlement_recorded(ship->settlement_unresolved) == 0;
    if (settled)
    {
      ship->settlement_unresolved = 0;
    }
  }
  return settled;
}

/**
 * The gate of a command that opens an account for this ship or this player:
 * their earlier settlements are settled first.
 *
 * @return FALSE, with the player told, while one stays open
 */
bool vessel_settlement_gate(struct char_data *ch, struct greyhawk_ship_data *ship)
{
  if (vessel_settlements_reconcile(ch, ship))
  {
    return TRUE;
  }
  send_to_char(ch, "The harbor office is still settling an earlier account. Try again "
                   "shortly.\r\n");
  return FALSE;
}
