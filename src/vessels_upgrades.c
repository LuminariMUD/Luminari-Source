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

extern MYSQL *conn;
extern bool mysql_available;
extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/* Insurance premium is a fraction of the insured value */
#define INSURANCE_PREMIUM_DIVISOR 5

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
           "WHERE ship_id = '%s'",
           ship->upgrades, ship->insured_for, ship->wages_owed, ship->id);

  if (mysql_query(conn, query))
  {
    log("SYSERR: vessel_db_save_extras failed for ship %s: %s", ship->id, mysql_error(conn));
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
           "SELECT upgrades, insured_for, wages_owed FROM ship_interiors WHERE ship_id = '%s'",
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
 * Pay out insurance to the owner when a ship is lost.
 *
 * Called from vessel_sink() before the fleet slot is cleared. The payout
 * goes to the owner if they are logged in; otherwise the loss is logged
 * for staff reconciliation (mail-based payout is future work).
 */
void vessel_pay_insurance(struct greyhawk_ship_data *ship)
{
  struct descriptor_data *d;

  if (ship == NULL || ship->insured_for <= 0 || ship->owner[0] == '\0')
  {
    return;
  }

  for (d = descriptor_list; d; d = d->next)
  {
    if (STATE(d) != CON_PLAYING || d->character == NULL)
    {
      continue;
    }
    if (!str_cmp(GET_NAME(d->character), ship->owner))
    {
      GET_GOLD(d->character) += ship->insured_for;
      send_to_char(d->character,
                   "Word reaches the underwriters: %s is lost. They pay out %d gold.\r\n",
                   ship->name, ship->insured_for);
      log("Info: Insurance paid %d gold to %s for lost ship %d '%s'", ship->insured_for,
          ship->owner, ship->shipnum, ship->name);
      return;
    }
  }

  log("Info: Ship %d '%s' sank insured for %d gold; owner %s offline - payout pending staff "
      "reconciliation",
      ship->shipnum, ship->name, ship->insured_for, ship->owner);
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
    if (ship->name[0] == '\0' || ship->speed <= 0)
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

  if (ship->shipobj == NULL || IN_ROOM(ship->shipobj) == NOWHERE ||
      !ROOM_FLAGGED(IN_ROOM(ship->shipobj), ROOM_DOCKABLE))
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
