/* ************************************************************************
 *      File:   vessels_upgrades.c                    Part of LuminariMUD  *
 *   Purpose:   Upgrades, upkeep, and insurance (Phase 06, Sessions 04-05).*
 *              Upgrades raise a hull's ceilings; wear grinds them down    *
 *              under way; insurance softens a total loss.                 *
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
#include "comms/new_mail.h"

extern MYSQL *conn;
extern bool mysql_available;
extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/* Insurance premium is a fraction of the insured value */
#define INSURANCE_PREMIUM_DIVISOR 5

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
 * Convert an insured loss into one durable claim and one system mail.
 *
 * Clearing ship_interiors.insured_for and inserting the claim share a
 * transaction. A reboot after the commit therefore cannot queue the same loss
 * again even if the sinking ship has not yet been purged.
 */
static bool vessel_queue_insurance_claim(struct greyhawk_ship_data *ship)
{
  char escaped_owner[sizeof(ship->owner) * 2 + 1];
  char escaped_name[sizeof(ship->name) * 2 + 1];
  char query[MAX_STRING_LENGTH];
  char subject[256];
  char message[1024];
  unsigned long long claim_id;
  int amount;

  if (ship == NULL || ship->insured_for <= 0 || ship->owner[0] == '\0' ||
      !vessel_insurance_ensure_schema())
  {
    return FALSE;
  }

  amount = ship->insured_for;
  mysql_real_escape_string(conn, escaped_owner, ship->owner, strlen(ship->owner));
  mysql_real_escape_string(conn, escaped_name, ship->name, strlen(ship->name));

  if (mysql_query(conn, "START TRANSACTION"))
  {
    log("SYSERR: Could not begin insurance claim transaction: %s", mysql_error(conn));
    return FALSE;
  }

  snprintf(query, sizeof(query),
           "UPDATE ship_interiors SET insured_for = 0 "
           "WHERE ship_id = %d AND insured_for > 0",
           ship->shipnum);
  if (mysql_query(conn, query) || mysql_affected_rows(conn) != 1)
  {
    log("SYSERR: Could not consume insurance policy for ship %d: %s", ship->shipnum,
        mysql_error(conn));
    mysql_query(conn, "ROLLBACK");
    return FALSE;
  }

  snprintf(query, sizeof(query),
           "INSERT INTO vessel_insurance_claims "
           "(ship_id, owner, ship_name, amount) "
           "VALUES (%d, '%s', '%s', %d)",
           ship->shipnum, escaped_owner, escaped_name, amount);
  if (mysql_query(conn, query))
  {
    log("SYSERR: Could not queue insurance for ship %d: %s", ship->shipnum, mysql_error(conn));
    mysql_query(conn, "ROLLBACK");
    return FALSE;
  }
  claim_id = mysql_insert_id(conn);

  snprintf(subject, sizeof(subject), "Insurance settlement for %s", ship->name);
  snprintf(message, sizeof(message),
           "The underwriters confirm the total loss of %s. Claim #%llu is approved "
           "for %d gold. The settlement is delivered automatically when you enter "
           "the game; this letter is your receipt.",
           ship->name, claim_id, amount);
  if (!new_mail_send_system(ship->owner, subject, message))
  {
    mysql_query(conn, "ROLLBACK");
    return FALSE;
  }

  if (mysql_query(conn, "COMMIT"))
  {
    log("SYSERR: Could not commit insurance claim for ship %d: %s", ship->shipnum,
        mysql_error(conn));
    mysql_query(conn, "ROLLBACK");
    return FALSE;
  }

  ship->insured_for = 0;
  log("Info: Queued insurance claim %llu for %s: ship %d '%s', %d gold", claim_id, ship->owner,
      ship->shipnum, ship->name, amount);
  return TRUE;
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
    return "+50% armor on all sides";
  case 1:
    return "+5 maximum speed";
  case 2:
    return "+25% cargo capacity";
  case 3:
    return "+50% hull structure";
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
 * Installation cost, scaled to hull class so a warship refit costs more
 * than a rowboat's.
 */
int vessel_upgrade_cost(int index, enum vessel_class vessel_type)
{
  int base;

  if (index < 0 || index >= NUM_SHIP_UPGRADES)
  {
    return 0;
  }

  base = vessel_prototype_price((int)vessel_type, 10, 10) / 4;
  if (base < 100)
  {
    base = 100;
  }

  return base;
}

/**
 * Persist upgrades, insurance, and wear counters.
 */
void vessel_db_save_extras(struct greyhawk_ship_data *ship)
{
  char query[MAX_STRING_LENGTH];

  if (!mysql_available || conn == NULL || ship == NULL)
  {
    return;
  }

  snprintf(query, sizeof(query),
           "UPDATE ship_interiors SET upgrades = %d, insured_for = %d, wages_owed = %d "
           "WHERE ship_id = %d",
           ship->upgrades, ship->insured_for, ship->wages_owed, ship->shipnum);

  if (mysql_query(conn, query))
  {
    log("SYSERR: vessel_db_save_extras failed for ship %d: %s", ship->shipnum, mysql_error(conn));
  }
}

/**
 * Load upgrades, insurance, and wage debt.
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

  snprintf(query, sizeof(query),
           "SELECT upgrades, insured_for, wages_owed FROM ship_interiors WHERE ship_id = %d",
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
    ship->upgrades = row[0] ? atoi(row[0]) : 0;
    ship->insured_for = row[1] ? atoi(row[1]) : 0;
    ship->wages_owed = row[2] ? atoi(row[2]) : 0;
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
      total += atoll(row[1]);
      credited++;
    }
  }
  mysql_free_result(result);

  if (pending == 0)
  {
    return 0;
  }
  if (total < 0 || total > INT_MAX - GET_GOLD(ch))
  {
    log("SYSERR: Insurance settlements overflow gold for %s", GET_NAME(ch));
    return 0;
  }

  old_gold = GET_GOLD(ch);
  GET_GOLD(ch) += (int)total;
  GET_VESSEL_INSURANCE_CLAIM(ch) = highest_claim_id;
  if (credited > 0 && !save_char_checked(ch, 0))
  {
    GET_GOLD(ch) = old_gold;
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
                 "The vessel underwriters deliver %lld gold from %d settled "
                 "insurance claim%s. Check your mail for the receipt%s.\r\n",
                 total, credited, credited == 1 ? "" : "s", credited == 1 ? "" : "s");
    log("Info: Delivered %lld insurance gold to %s from %d claim%s", total, GET_NAME(ch), credited,
        credited == 1 ? "" : "s");
  }
  return credited;
}

/**
 * Settle insurance when a ship is lost.
 *
 * Both online and offline owners use the same durable queue. Online owners
 * receive it immediately; offline owners receive it automatically on login.
 */
void vessel_pay_insurance(struct greyhawk_ship_data *ship)
{
  struct descriptor_data *d;

  if (!vessel_queue_insurance_claim(ship))
  {
    if (ship != NULL && ship->insured_for > 0)
    {
      log("SYSERR: Insurance for lost ship %d '%s' could not be queued", ship->shipnum, ship->name);
    }
    return;
  }

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
 * Upkeep tick: hulls working under way accumulate wear on armor and
 * subsystems. Wear never sinks a ship by itself - it stops at 1 structure
 * per section - but it makes a neglected hull fragile in a fight.
 */
void vessel_upkeep_tick(void)
{
  struct greyhawk_ship_data *ship;
  int i;

  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
  {
    ship = &greyhawk_ships[i];
    if (!is_valid_ship(ship) || ship->speed <= 0)
    {
      continue; /* Moored ships do not wear */
    }

    ship->wear_ticks++;
    if (ship->wear_ticks < SHIP_WEAR_INTERVAL)
    {
      continue;
    }
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

    VSSL_DEBUG("Ship %d wear tick: armor %d/%d/%d/%d sail %d rudder %d", i, ship->farmor,
               ship->rarmor, ship->parmor, ship->sarmor, ship->mainsail, ship->turnrate);
  }
}

/**
 * Owner gate for refit commands: must own the ship and be moored at a dock.
 */
static struct greyhawk_ship_data *refit_command_ship(struct char_data *ch)
{
  struct greyhawk_ship_data *ship;

  ship = get_ship_from_room(IN_ROOM(ch));
  if (ship == NULL)
  {
    send_to_char(ch, "You must be aboard a ship.\r\n");
    return NULL;
  }

  if (ship->owner[0] == '\0' || (str_cmp(ship->owner, GET_NAME(ch)) && GET_LEVEL(ch) < LVL_IMMORT))
  {
    send_to_char(ch, "Only the owner may arrange a refit.\r\n");
    return NULL;
  }

  if (!vessel_ship_is_in_port(ship))
  {
    send_to_char(ch, "Refits happen at a shipyard - moor at a dock first.\r\n");
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

  ship = refit_command_ship(ch);
  if (ship == NULL)
  {
    return;
  }

  one_argument_u((char *)argument, arg);

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

  GET_GOLD(ch) -= cost;
  SET_BIT(ship->upgrades, bit);

  /* Raise the relevant ceilings once, at install time */
  switch (index)
  {
  case 0: /* plating */
    ship->maxfarmor += ship->maxfarmor / 2;
    ship->maxrarmor += ship->maxrarmor / 2;
    ship->maxparmor += ship->maxparmor / 2;
    ship->maxsarmor += ship->maxsarmor / 2;
    ship->farmor = ship->maxfarmor;
    ship->rarmor = ship->maxrarmor;
    ship->parmor = ship->maxparmor;
    ship->sarmor = ship->maxsarmor;
    break;
  case 1: /* rigging */
    ship->maxspeed += 5;
    ship->maxmainsail += 5;
    ship->mainsail = ship->maxmainsail;
    break;
  case 2: /* hold - read by vessel_effective_cargo_capacity */
    break;
  case 3: /* reinforcement */
    ship->maxfinternal += ship->maxfinternal / 2;
    ship->maxrinternal += ship->maxrinternal / 2;
    ship->maxpinternal += ship->maxpinternal / 2;
    ship->maxsinternal += ship->maxsinternal / 2;
    ship->finternal = ship->maxfinternal;
    ship->rinternal = ship->maxrinternal;
    ship->pinternal = ship->maxpinternal;
    ship->sinternal = ship->maxsinternal;
    break;
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

/**
 * shipinsure [<value>] - buy or review sinking insurance.
 */
ACMD(do_shipinsure)
{
  struct greyhawk_ship_data *ship;
  char arg[MAX_INPUT_LENGTH];
  int value;
  int premium;

  ship = refit_command_ship(ch);
  if (ship == NULL)
  {
    return;
  }

  one_argument_u((char *)argument, arg);

  if (!*arg)
  {
    if (ship->insured_for > 0)
    {
      send_to_char(ch, "%s is insured for %d gold.\r\n", ship->name, ship->insured_for);
    }
    else
    {
      send_to_char(ch, "%s carries no insurance.\r\n", ship->name);
    }
    send_to_char(ch, "Usage: shipinsure <payout value> - premium is one fifth of the payout.\r\n");
    return;
  }

  value = atoi(arg);
  if (value <= 0)
  {
    send_to_char(ch, "Insure her for how much?\r\n");
    return;
  }

  /* Cap the payout at the hull's market value so insurance is a hedge, not
   * a business model. */
  premium = vessel_prototype_price((int)ship->vessel_type, ship->maxspeed, ship->maxfarmor);
  if (value > premium)
  {
    send_to_char(ch, "The underwriters will not insure %s above her value of %d gold.\r\n",
                 ship->name, premium);
    return;
  }

  premium = value / INSURANCE_PREMIUM_DIVISOR;
  if (premium < 1)
  {
    premium = 1;
  }

  if (GET_GOLD(ch) < premium)
  {
    send_to_char(ch, "The premium is %d gold; you have %d.\r\n", premium, GET_GOLD(ch));
    return;
  }

  GET_GOLD(ch) -= premium;
  ship->insured_for = value;
  vessel_db_save_extras(ship);

  send_to_char(ch, "You pay %d gold. %s is insured for %d gold against total loss.\r\n", premium,
               ship->name, value);
  log("Info: %s insured ship %d for %d gold (premium %d)", GET_NAME(ch), ship->shipnum, value,
      premium);
}
