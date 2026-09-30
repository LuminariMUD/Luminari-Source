/* ************************************************************************
 *      File:   vessels_upgrades.c                    Part of LuminariMUD  *
 *   Purpose:   Upgrades, upkeep, and insurance (Phase 06, Sessions 04-05).*
 *              Upgrades raise a hull's ceilings; wear grinds them down    *
 *              under way; insurance, automatic since S5, softens a loss.  *
 * ********************************************************************** */

#include "conf.h"
#include "core/sysdep.h"
#include "core/structs.h"
#include "core/utils.h"
#include "core/comm.h"
#include "core/db.h"
#include "core/handler.h"
#include "character/rewards.h"
#include "core/interpreter.h"
#include "vessels.h"
#include "database/mysql.h"
#include "comms/new_mail.h"

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/**
 * Ensure the durable insurance claim queue exists.
 */
static bool vessel_insurance_ensure_schema(void)
{
  const char *query = "CREATE TABLE IF NOT EXISTS vessel_insurance_claims ("
                      "claim_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY, "
                      "ship_id INT NOT NULL, "
                      "owner VARCHAR(64) NOT NULL, "
                      "ship_name VARCHAR(128) NOT NULL, "
                      "amount INT NOT NULL, "
                      "status VARCHAR(16) NOT NULL DEFAULT 'pending', "
                      "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                      "paid_at TIMESTAMP NULL DEFAULT NULL, "
                      "INDEX idx_vessel_claim_owner_status (owner, status), "
                      "INDEX idx_vessel_claim_ship (ship_id)"
                      ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";

  if (!mysql_available || conn == NULL)
  {
    return FALSE;
  }
  if (mysql_query(conn, query))
  {
    log("SYSERR: Could not create vessel_insurance_claims: %s", mysql_error(conn));
    return FALSE;
  }
  return TRUE;
}

/**
 * Queue one durable settlement for a hull's owner with its mail receipt, in
 * one transaction. A lost hull's claim also marks her loss paid.
 */
static bool vessel_queue_claim(struct greyhawk_ship_data *ship, int amount, const char *subject,
                               const char *letter, bool loss)
{
  char escaped_owner[sizeof(ship->owner) * 2 + 1];
  char escaped_name[sizeof(ship->name) * 2 + 1];
  char query[MAX_STRING_LENGTH];
  char message[2048];
  unsigned long long claim_id;
  PREPARED_STMT *statement;
  bool marked;

  if (ship == NULL || amount <= 0 || ship->owner[0] == '\0' || !vessel_insurance_ensure_schema())
  {
    return FALSE;
  }

  mysql_real_escape_string(conn, escaped_owner, ship->owner, strlen(ship->owner));
  mysql_real_escape_string(conn, escaped_name, ship->name, strlen(ship->name));

  if (mysql_query(conn, "START TRANSACTION"))
  {
    log("SYSERR: Could not begin vessel settlement transaction: %s", mysql_error(conn));
    return FALSE;
  }

  snprintf(query, sizeof(query),
           "INSERT INTO vessel_insurance_claims "
           "(ship_id, owner, ship_name, amount) "
           "VALUES (%d, '%s', '%s', %d)",
           ship->shipnum, escaped_owner, escaped_name, amount);
  if (mysql_query(conn, query))
  {
    log("SYSERR: Could not queue a settlement for ship %d: %s", ship->shipnum, mysql_error(conn));
    mysql_query(conn, "ROLLBACK");
    return FALSE;
  }
  claim_id = mysql_insert_id(conn);

  /* Mark the loss paid with the claim: a hull saved while sinking and
   * restored before her wreck was saved goes down again without paying. */
  if (loss)
  {
    statement = mysql_stmt_create(conn);
    marked = statement != NULL &&
             mysql_stmt_prepare_query(
                 statement, "UPDATE ship_runtime_state SET wreck_hull = 1 WHERE ship_id = ?") &&
             mysql_stmt_bind_param_int(statement, 0, ship->shipnum) &&
             mysql_stmt_execute_prepared(statement);
    mysql_stmt_cleanup(statement);
    if (!marked)
    {
      log("SYSERR: Could not mark the loss of ship %d paid", ship->shipnum);
      mysql_query(conn, "ROLLBACK");
      return FALSE;
    }
  }

  snprintf(message, sizeof(message),
           "%s Settlement #%llu, %d gold, is delivered automatically when you enter the game; "
           "this letter is your receipt.",
           letter, claim_id, amount);
  if (!new_mail_send_system(ship->owner, subject, message))
  {
    mysql_query(conn, "ROLLBACK");
    return FALSE;
  }

  if (mysql_query(conn, "COMMIT"))
  {
    log("SYSERR: Could not commit the settlement for ship %d: %s", ship->shipnum,
        mysql_error(conn));
    mysql_query(conn, "ROLLBACK");
    return FALSE;
  }

  log("Info: Queued settlement %llu for %s: ship %d '%s', %d gold (%s)", claim_id, ship->owner,
      ship->shipnum, ship->name, amount, subject);
  return TRUE;
}

/**
 * Insurance became automatic in S5 (study 3.3.10): refund the premium of
 * every policy bought before, a fifth of its value and at least 1 gold, as a
 * claim the settlement path delivers. Clearing the policies in the same
 * transaction makes the refund run once.
 */
void vessel_refund_insurance_premiums(void)
{
  if (!mysql_available || conn == NULL)
  {
    return;
  }
  if (mysql_query(conn, "START TRANSACTION"))
  {
    log("SYSERR: Could not begin the insurance premium refund: %s", mysql_error(conn));
    return;
  }
  if (mysql_query(conn, "INSERT INTO vessel_insurance_claims (ship_id, owner, ship_name, amount) "
                        "SELECT ship_id, owner, vessel_name, GREATEST(1, insured_for DIV 5) "
                        "FROM ship_interiors WHERE insured_for > 0 AND owner <> ''") ||
      mysql_query(conn, "UPDATE ship_interiors SET insured_for = 0 WHERE insured_for > 0") ||
      mysql_query(conn, "COMMIT"))
  {
    log("SYSERR: Could not refund vessel insurance premiums: %s", mysql_error(conn));
    mysql_query(conn, "ROLLBACK");
  }
}

/**
 * Display name for an upgrade index.
 */
const char *vessel_upgrade_name(int index)
{
  switch (index)
  {
  case 0:
    return "plating";
  case 1:
    return "rigging";
  case 2:
    return "hold";
  case 3:
    return "reinforcement";
  default:
    return "unknown";
  }
}

/**
 * Short description of what an upgrade does.
 */
static const char *vessel_upgrade_effect(int index)
{
  switch (index)
  {
  case 0:
    return "+20% armor on all sides";
  case 1:
    return "+10% maximum speed";
  case 2:
    return "+25% cargo capacity";
  case 3:
    return "+20% hull structure";
  default:
    return "no effect";
  }
}

/**
 * Bitfield value for an upgrade index.
 */
int vessel_upgrade_bit(int index)
{
  switch (index)
  {
  case 0:
    return SHIP_UPGRADE_PLATING;
  case 1:
    return SHIP_UPGRADE_RIGGING;
  case 2:
    return SHIP_UPGRADE_HOLD;
  case 3:
    return SHIP_UPGRADE_REINFORCED;
  default:
    return 0;
  }
}

/**
 * Map an upgrade name to its index.
 *
 * @return Upgrade index, or -1 if unrecognized
 */
static int vessel_upgrade_by_name(const char *name)
{
  int i;

  if (name == NULL || !*name)
  {
    return -1;
  }

  for (i = 0; i < NUM_SHIP_UPGRADES; i++)
  {
    if (is_abbrev(name, vessel_upgrade_name(i)))
    {
      return i;
    }
  }

  return -1;
}

/**
 * Installation cost: a fifth of the class price (study 3.3.1).
 */
int vessel_upgrade_cost(int index, enum vessel_class vessel_type)
{
  if (index < 0 || index >= NUM_SHIP_UPGRADES)
  {
    return 0;
  }

  return vessel_class_condition(vessel_type)->price / 5;
}

/** Design speed with the rigging refit: +10%, at least 1, at most the speed limit. */
short int vessel_rigged_speed(int design_speed)
{
  return (short int)MIN(VESSEL_SPEED_LIMIT, design_speed + MAX(1, (design_speed + 5) / 10));
}

/**
 * Persist upgrades.
 */
void vessel_db_save_extras(struct greyhawk_ship_data *ship)
{
  char query[MAX_STRING_LENGTH];

  if (!mysql_available || conn == NULL || ship == NULL)
  {
    return;
  }

  snprintf(query, sizeof(query), "UPDATE ship_interiors SET upgrades = %d WHERE ship_id = %d",
           ship->upgrades, ship->shipnum);

  if (mysql_query(conn, query))
  {
    log("SYSERR: vessel_db_save_extras failed for ship %d: %s", ship->shipnum, mysql_error(conn));
  }
}

/**
 * Load upgrades.
 */
void vessel_db_load_extras(struct greyhawk_ship_data *ship)
{
  char query[MAX_STRING_LENGTH];
  MYSQL_RES *result;
  MYSQL_ROW row;

  if (!mysql_available || conn == NULL || ship == NULL)
  {
    return;
  }

  snprintf(query, sizeof(query), "SELECT upgrades FROM ship_interiors WHERE ship_id = %d",
           ship->shipnum);
  if (mysql_query(conn, query))
  {
    return;
  }

  result = mysql_store_result(conn);
  if (result == NULL)
  {
    return;
  }

  row = mysql_fetch_row(result);
  if (row != NULL)
  {
    ship->upgrades = row[0] ? parse_int(row[0]) : 0;
  }
  mysql_free_result(result);
}

/**
 * Apply all pending vessel settlements to a loaded player exactly once.
 *
 * The highest applied claim ID is saved in the player file before database
 * rows are marked paid. If the process stops between those operations, the
 * next login recognizes the saved high-water mark and closes the rows without
 * crediting the gold twice.
 *
 * @return Number of newly credited claims
 */
int vessel_deliver_pending_insurance(struct char_data *ch)
{
  char escaped_owner[MAX_NAME_LENGTH * 2 + 1];
  char query[MAX_STRING_LENGTH];
  MYSQL_RES *result;
  MYSQL_ROW row;
  unsigned long long claim_id;
  unsigned long long previous_claim_id;
  unsigned long long highest_claim_id;
  long long total;
  int old_gold;
  int credited;
  int pending;

  if (ch == NULL || IS_NPC(ch) || GET_NAME(ch) == NULL || !vessel_insurance_ensure_schema())
  {
    return 0;
  }

  mysql_real_escape_string(conn, escaped_owner, GET_NAME(ch), strlen(GET_NAME(ch)));
  snprintf(query, sizeof(query),
           "SELECT claim_id, amount FROM vessel_insurance_claims "
           "WHERE owner = '%s' AND status = 'pending' ORDER BY claim_id",
           escaped_owner);
  if (mysql_query(conn, query))
  {
    log("SYSERR: Could not load insurance claims for %s: %s", GET_NAME(ch), mysql_error(conn));
    return 0;
  }

  result = mysql_store_result(conn);
  if (result == NULL)
  {
    return 0;
  }

  previous_claim_id = GET_VESSEL_INSURANCE_CLAIM(ch);
  highest_claim_id = previous_claim_id;
  total = 0;
  credited = 0;
  pending = 0;
  while ((row = mysql_fetch_row(result)) != NULL)
  {
    pending++;
    claim_id = row[0] ? strtoull(row[0], NULL, 10) : 0;
    if (claim_id > highest_claim_id)
    {
      highest_claim_id = claim_id;
    }
    if (claim_id > previous_claim_id && row[1] != NULL)
    {
      total += parse_llong(row[1]);
      credited++;
    }
  }
  mysql_free_result(result);

  if (pending == 0)
  {
    return 0;
  }
  if (total < 0)
  {
    log("SYSERR: Insurance settlements for %s total %lld gold", GET_NAME(ch), total);
    return 0;
  }
  /* Claims stay pending until the whole settlement fits in the purse. */
  if (total > award_capacity(ch, AWARD_GOLD))
  {
    send_to_char(ch,
                 "The harbor office holds %lld gold in vessel settlements for you, "
                 "more than you can carry. Bank some gold to collect it.\r\n",
                 total);
    return 0;
  }

  old_gold = GET_GOLD(ch);
  award_gold(ch, (int)total);
  GET_VESSEL_INSURANCE_CLAIM(ch) = highest_claim_id;
  if (credited > 0 && !save_char_checked(ch, 0))
  {
    award_set_points(ch, AWARD_GOLD, old_gold);
    GET_VESSEL_INSURANCE_CLAIM(ch) = previous_claim_id;
    log("SYSERR: Could not save insurance settlement for %s", GET_NAME(ch));
    return 0;
  }

  snprintf(query, sizeof(query),
           "UPDATE vessel_insurance_claims SET status = 'paid', paid_at = NOW() "
           "WHERE owner = '%s' AND status = 'pending' AND claim_id <= %llu",
           escaped_owner, highest_claim_id);
  if (mysql_query(conn, query))
  {
    log("SYSERR: Could not close insurance claims for %s: %s", GET_NAME(ch), mysql_error(conn));
  }

  if (credited > 0)
  {
    send_to_char(ch,
                 "The harbor office delivers %lld gold from %d vessel "
                 "settlement%s. Check your mail for the receipt%s.\r\n",
                 total, credited, credited == 1 ? "" : "s", credited == 1 ? "" : "s");
    log("Info: Delivered %lld insurance gold to %s from %d claim%s", total, GET_NAME(ch), credited,
        credited == 1 ? "" : "s");
  }
  return credited;
}

/** Deliver a hull owner's settlements now, if the owner is in the game. */
static void vessel_deliver_to_online_owner(const struct greyhawk_ship_data *ship)
{
  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next)
  {
    if (STATE(d) == CON_PLAYING && d->character != NULL &&
        !str_cmp(GET_NAME(d->character), ship->owner))
    {
      vessel_deliver_pending_insurance(d->character);
      return;
    }
  }
}

/**
 * Settle insurance when a ship is lost.
 *
 * Both online and offline owners use the same durable queue. Online owners
 * receive it immediately; offline owners receive it automatically on login.
 */
void vessel_pay_insurance(struct greyhawk_ship_data *ship, int amount)
{
  char subject[256];

  if (amount <= 0)
  {
    return;
  }
  snprintf(subject, sizeof(subject), "Insurance settlement for %s", ship->name);
  if (!vessel_queue_claim(ship, amount, subject,
                          "The underwriters confirm the loss of your hull. What could be saved of "
                          "her waits in the wreck registry: summon her at any shipyard with "
                          "SHIPSUMMON.",
                          TRUE))
  {
    log("SYSERR: Insurance for lost ship %d could not be queued", ship->shipnum);
    return;
  }
  vessel_deliver_to_online_owner(ship);
}

/**
 * Pay a hull's owner her share of a sinking (study 3.3.7) through the same
 * durable queue as insurance.
 */
void vessel_pay_prize(struct greyhawk_ship_data *ship, int amount, const char *letter)
{
  char subject[256];

  if (amount <= 0)
  {
    return;
  }
  snprintf(subject, sizeof(subject), "Prize money for %s", ship->name);
  if (!vessel_queue_claim(ship, amount, subject, letter, FALSE))
  {
    log("SYSERR: Prize money for ship %d could not be queued", ship->shipnum);
    return;
  }
  vessel_deliver_to_online_owner(ship);
}

/**
 * Upkeep tick: hulls working under way accumulate wear on armor and
 * subsystems. Wear never sinks a ship by itself - it stops at 1 structure
 * per section - but it makes a neglected hull fragile in a fight.
 */
void vessel_upkeep_tick_one(struct greyhawk_ship_data *ship)
{
  if (!is_valid_ship(ship) || ship->speed <= 0)
    return;

  ship->wear_ticks++;
  if (ship->wear_ticks < SHIP_WEAR_INTERVAL)
    return;
  ship->wear_ticks = 0;

  if (ship->farmor > 0)
    ship->farmor--;
  if (ship->rarmor > 0)
    ship->rarmor--;
  if (ship->parmor > 0)
    ship->parmor--;
  if (ship->sarmor > 0)
    ship->sarmor--;
  if (ship->mainsail > 1)
    ship->mainsail--;
  if (ship->turnrate > 1)
    ship->turnrate--;

  VSSL_DEBUG("Ship %d wear tick: armor %d/%d/%d/%d sail %d rudder %d", ship->shipnum, ship->farmor,
             ship->rarmor, ship->parmor, ship->sarmor, ship->mainsail, ship->turnrate);
}

void vessel_upkeep_tick(void)
{
  int i;

  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
    vessel_upkeep_tick_one(&greyhawk_ships[i]);
}

/**
 * Owner gate for shipyard commands: must own the ship and be berthed at a
 * dock, with no departure under way, so work the shipwrights start keeps her
 * at the berth until it is done.
 */
struct greyhawk_ship_data *vessel_refit_ship(struct char_data *ch)
{
  struct greyhawk_ship_data *ship;

  ship = get_ship_from_room(IN_ROOM(ch));
  if (ship == NULL)
  {
    send_to_char(ch, "You must be aboard a ship.\r\n");
    return NULL;
  }

  if (ship->owner[0] == '\0' ||
      (str_cmp(ship->owner, GET_NAME(ch)) != 0 && GET_LEVEL(ch) < LVL_IMMORT))
  {
    send_to_char(ch, "Only the owner may arrange a refit.\r\n");
    return NULL;
  }

  if (!vessel_ship_is_in_port(ship) || ship->dock <= 0)
  {
    send_to_char(ch, "Refits happen at a shipyard - moor at a dock first.\r\n");
    return NULL;
  }

  if (ship->departure_ticks > 0)
  {
    send_to_char(ch, "The crew is casting off; the shipwrights cannot work on her now.\r\n");
    return NULL;
  }

  if (vessel_port_refuses(ch))
  {
    return NULL;
  }

  return ship;
}

/**
 * shipupgrade [<upgrade>] - list or install upgrades at a dock.
 */
ACMD(do_shipupgrade)
{
  struct greyhawk_ship_data *ship;
  char arg[MAX_INPUT_LENGTH];
  int index;
  int cost;
  int bit;
  int i;

  ship = vessel_refit_ship(ch);
  if (ship == NULL)
  {
    return;
  }

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    send_to_char(ch, "Available refits for %s:\r\n", ship->name);
    for (i = 0; i < NUM_SHIP_UPGRADES; i++)
    {
      send_to_char(ch, "  %-14s %-24s %8d gold %s\r\n", vessel_upgrade_name(i),
                   vessel_upgrade_effect(i), vessel_upgrade_cost(i, ship->vessel_type),
                   IS_SET(ship->upgrades, vessel_upgrade_bit(i)) ? "[installed]" : "");
    }
    return;
  }

  index = vessel_upgrade_by_name(arg);
  if (index < 0)
  {
    send_to_char(ch, "No such refit. Type 'shipupgrade' for the list.\r\n");
    return;
  }

  bit = vessel_upgrade_bit(index);
  if (IS_SET(ship->upgrades, bit))
  {
    send_to_char(ch, "%s already carries that refit.\r\n", ship->name);
    return;
  }

  cost = vessel_upgrade_cost(index, ship->vessel_type);
  if (GET_GOLD(ch) < cost)
  {
    send_to_char(ch, "That refit costs %d gold; you have %d.\r\n", cost, GET_GOLD(ch));
    return;
  }

  award_gold(ch, -cost);
  SET_BIT(ship->upgrades, bit);

  /* Raise the relevant ceilings once, at install time (study 3.3.1) */
  switch (index)
  {
  case 0: /* plating */
  case 3: /* reinforcement */
    vessel_refit_arcs(ship, index == 3);
    break;
  case 1: /* rigging */
    ship->maxspeed = vessel_rigged_speed(ship->maxspeed);
    break;
  case 2: /* hold - read by vessel_effective_cargo_capacity */
  default:
    break;
  }

  vessel_db_save_extras(ship);
  save_ship_interior(ship);

  send_to_char(ch, "The shipwrights fit %s to %s for %d gold.\r\n", vessel_upgrade_name(index),
               ship->name, cost);
  send_to_ship(ship, "%s has been refitted: %s.", ship->name, vessel_upgrade_effect(index));
  log("Info: %s installed %s on ship %d for %d gold", GET_NAME(ch), vessel_upgrade_name(index),
      ship->shipnum, cost);
}
