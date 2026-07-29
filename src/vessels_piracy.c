/* ************************************************************************
 *      File:   vessels_piracy.c                      Part of LuminariMUD  *
 *   Purpose:   Piracy, plunder, and bounty (Phase 07, Session 05).        *
 *              Taking cargo by force pays now and costs later: bounty     *
 *              closes lawful ports and eventually draws the navy. A       *
 *              letter of marque makes the same act legal.                 *
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

/* A letter of marque costs this multiple of the WANTED threshold */
#define MARQUE_COST (BOUNTY_WANTED * 4)

/* Marque duration in real seconds (one real day) */
#define MARQUE_DURATION 86400

/**
 * Create the bounty table.
 * Mirrored by sql/components/vessels_phase7_schema.sql.
 */
void vessel_piracy_ensure_schema(void)
{
  if (!mysql_available || conn == NULL)
  {
    return;
  }

  if (mysql_query(conn, "CREATE TABLE IF NOT EXISTS vessel_bounties ("
                        "  player_name VARCHAR(64) PRIMARY KEY,"
                        "  bounty INT NOT NULL DEFAULT 0,"
                        "  marque_until INT NOT NULL DEFAULT 0,"
                        "  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP "
                        "    ON UPDATE CURRENT_TIMESTAMP"
                        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"))
  {
    log("SYSERR: vessel_bounties create failed: %s", mysql_error(conn));
  }
}

/**
 * Read a player's outstanding bounty.
 *
 * @return Bounty in gold, 0 if none or on error
 */
int vessel_get_bounty(const char *player_name)
{
  char query[MAX_STRING_LENGTH];
  char escaped[130];
  MYSQL_RES *result;
  MYSQL_ROW row;
  int bounty = 0;

  if (!mysql_available || conn == NULL || player_name == NULL || !*player_name)
  {
    return 0;
  }

  mysql_real_escape_string(conn, escaped, player_name, strlen(player_name));
  snprintf(query, sizeof(query), "SELECT bounty FROM vessel_bounties WHERE player_name = '%s'",
           escaped);
  if (mysql_query(conn, query))
  {
    return 0;
  }

  result = mysql_store_result(conn);
  if (result == NULL)
  {
    return 0;
  }

  row = mysql_fetch_row(result);
  if (row != NULL && row[0] != NULL)
  {
    bounty = atoi(row[0]);
  }
  mysql_free_result(result);

  return bounty;
}

/**
 * Add to a player's bounty (creating the record if needed).
 */
void vessel_add_bounty(const char *player_name, int amount)
{
  char query[MAX_STRING_LENGTH];
  char escaped[130];

  if (!mysql_available || conn == NULL || player_name == NULL || !*player_name || amount <= 0)
  {
    return;
  }

  mysql_real_escape_string(conn, escaped, player_name, strlen(player_name));
  snprintf(query, sizeof(query),
           "INSERT INTO vessel_bounties (player_name, bounty) VALUES ('%s', %d) "
           "ON DUPLICATE KEY UPDATE bounty = bounty + %d",
           escaped, amount, amount);
  if (mysql_query(conn, query))
  {
    log("SYSERR: vessel_add_bounty failed: %s", mysql_error(conn));
    return;
  }

  log("Info: %s bounty increased by %d gold", player_name, amount);
}

/**
 * Clear a player's bounty (pardon or paid off).
 */
void vessel_clear_bounty(const char *player_name)
{
  char query[MAX_STRING_LENGTH];
  char escaped[130];

  if (!mysql_available || conn == NULL || player_name == NULL || !*player_name)
  {
    return;
  }

  mysql_real_escape_string(conn, escaped, player_name, strlen(player_name));
  snprintf(query, sizeof(query), "UPDATE vessel_bounties SET bounty = 0 WHERE player_name = '%s'",
           escaped);
  if (mysql_query(conn, query))
  {
    log("SYSERR: vessel_clear_bounty failed: %s", mysql_error(conn));
  }
}

/**
 * Does this player hold a valid letter of marque?
 */
bool vessel_has_letter_of_marque(const char *player_name)
{
  char query[MAX_STRING_LENGTH];
  char escaped[130];
  MYSQL_RES *result;
  MYSQL_ROW row;
  bool valid = FALSE;

  if (!mysql_available || conn == NULL || player_name == NULL || !*player_name)
  {
    return FALSE;
  }

  mysql_real_escape_string(conn, escaped, player_name, strlen(player_name));
  snprintf(query, sizeof(query),
           "SELECT marque_until FROM vessel_bounties WHERE player_name = '%s'", escaped);
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
  if (row != NULL && row[0] != NULL)
  {
    valid = (atoi(row[0]) > (int)time(0));
  }
  mysql_free_result(result);

  return valid;
}

/**
 * Will a lawful port refuse this character its services?
 *
 * WANTED pirates get no market, no freight work, no crew hall, and no
 * shipyard. A letter of marque restores standing. Called from every
 * port-service gate so the consequence is uniform.
 *
 * @return TRUE if refused (message already sent to ch)
 */
bool vessel_port_refuses(struct char_data *ch)
{
  int bounty;

  if (ch == NULL || IS_NPC(ch) || GET_LEVEL(ch) >= LVL_IMMORT)
  {
    return FALSE;
  }

  bounty = vessel_get_bounty(GET_NAME(ch));
  if (bounty < BOUNTY_WANTED)
  {
    return FALSE;
  }

  if (vessel_has_letter_of_marque(GET_NAME(ch)))
  {
    return FALSE;
  }

  send_to_char(ch,
               "The harbourmaster knows your face - %d gold is posted for you here. "
               "No lawful business will be done with you in this port.\r\n",
               bounty);
  return TRUE;
}

/**
 * Move cargo from a prize ship into the raider's hold.
 *
 * Takes as much as the raider can carry, lot by lot. Unlawful plunder
 * accrues bounty per unit; a letter of marque exempts the raider.
 *
 * @return Units transferred
 */
int vessel_plunder_cargo(struct char_data *ch, struct greyhawk_ship_data *prize,
                         struct greyhawk_ship_data *raider)
{
  int capacity;
  int taken = 0;
  int i, j;

  if (ch == NULL || prize == NULL || raider == NULL || prize == raider)
  {
    return 0;
  }

  capacity = vessel_effective_cargo_capacity(raider);

  for (i = 0; i < MAX_CARGO_LOTS; i++)
  {
    if (prize->cargo[i].commodity_id == 0 || prize->cargo[i].quantity <= 0)
    {
      continue;
    }

    /* Find or claim a bay in the raider's hold for this commodity */
    for (j = 0; j < MAX_CARGO_LOTS; j++)
    {
      if (raider->cargo[j].commodity_id == prize->cargo[i].commodity_id ||
          raider->cargo[j].commodity_id == 0)
      {
        break;
      }
    }
    if (j >= MAX_CARGO_LOTS)
    {
      continue; /* No room for this kind of goods */
    }

    raider->cargo[j].commodity_id = prize->cargo[i].commodity_id;

    /* Move units one at a time so the weight limit stops us exactly at
     * capacity rather than overshooting. */
    while (prize->cargo[i].quantity > 0)
    {
      raider->cargo[j].quantity++;
      if (vessel_cargo_weight(raider) > capacity)
      {
        raider->cargo[j].quantity--;
        break;
      }
      prize->cargo[i].quantity--;
      taken++;
    }

    if (prize->cargo[i].quantity <= 0)
    {
      prize->cargo[i].commodity_id = 0;
    }
    if (raider->cargo[j].quantity <= 0)
    {
      raider->cargo[j].commodity_id = 0;
    }
  }

  if (taken > 0)
  {
    vessel_db_save_cargo(prize);
    vessel_db_save_cargo(raider);
  }

  return taken;
}

/**
 * plunder - strip cargo from a ship whose bridge you hold.
 */
ACMD(do_plunder)
{
  struct greyhawk_ship_data *prize;
  struct greyhawk_ship_data *raider = NULL;
  struct char_data *tch;
  int taken;
  int bounty;
  int i;

  prize = get_ship_from_room(IN_ROOM(ch));
  if (prize == NULL)
  {
    send_to_char(ch, "You must be aboard a ship to plunder her.\r\n");
    return;
  }

  if (world[IN_ROOM(ch)].number != (room_vnum)prize->bridge_room)
  {
    send_to_char(ch, "Take the bridge first - that is where a prize is claimed.\r\n");
    return;
  }

  if (!str_cmp(prize->owner, GET_NAME(ch)))
  {
    send_to_char(ch, "You cannot plunder your own ship; use 'cargosell' in port.\r\n");
    return;
  }

  /* Taking another player's cargo is a hostile act against them, so it
   * answers to the same consent rules as attacking them directly. */
  if (!vessel_pvp_permitted(ch, prize, TRUE))
  {
    return;
  }

  /* The bridge must be uncontested */
  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (tch != ch && GET_POS(tch) > POS_STUNNED)
    {
      send_to_char(ch, "%s still holds the bridge against you.\r\n", PERS(tch, ch));
      return;
    }
  }

  /* Find the raider's own ship: the nearest vessel docked to this one */
  if (prize->docked_to_ship >= 0 && prize->docked_to_ship < GREYHAWK_MAXSHIPS)
  {
    raider = &greyhawk_ships[prize->docked_to_ship];
  }
  else
  {
    for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
    {
      if (is_valid_ship(&greyhawk_ships[i]) && &greyhawk_ships[i] != prize &&
          !str_cmp(greyhawk_ships[i].owner, GET_NAME(ch)) &&
          ships_in_docking_range(&greyhawk_ships[i], prize))
      {
        raider = &greyhawk_ships[i];
        break;
      }
    }
  }

  if (raider == NULL)
  {
    send_to_char(ch, "You need your own ship alongside to carry off the cargo.\r\n");
    return;
  }

  taken = vessel_plunder_cargo(ch, prize, raider);
  if (taken == 0)
  {
    send_to_char(ch, "There is nothing worth taking, or no room to take it.\r\n");
    return;
  }

  send_to_char(ch, "You transfer %d units of cargo from %s to %s.\r\n", taken, prize->name,
               raider->name);
  send_to_ship(prize, "Raiders strip %d units of cargo from the hold!", taken);
  send_to_ship(raider, "Plundered cargo comes aboard: %d units.", taken);

  /* Lawful privateers answer to nobody; pirates earn a price on their head */
  if (vessel_has_letter_of_marque(GET_NAME(ch)))
  {
    send_to_char(ch, "Your letter of marque makes this a lawful prize.\r\n");
  }
  else
  {
    bounty = taken * BOUNTY_PER_CARGO_UNIT;
    vessel_add_bounty(GET_NAME(ch), bounty);
    send_to_char(ch, "Word of the raid will spread - your bounty rises by %d gold.\r\n", bounty);

    bounty = vessel_get_bounty(GET_NAME(ch));
    if (bounty >= BOUNTY_HUNTED)
    {
      send_to_char(ch, "You are now HUNTED: the navy has orders to sink you on sight.\r\n");
    }
    else if (bounty >= BOUNTY_WANTED)
    {
      send_to_char(ch, "You are now WANTED: lawful ports will refuse you a berth.\r\n");
    }
  }

  log("Info: %s plundered %d units from ship %d '%s'", GET_NAME(ch), taken, prize->shipnum,
      prize->name);
}

/**
 * bounty [<player>] - check your bounty, or another's.
 */
ACMD(do_bounty)
{
  char arg[MAX_INPUT_LENGTH];
  const char *target;
  int bounty;

  if (!mysql_available || conn == NULL)
  {
    send_to_char(ch, "No bounty records are available.\r\n");
    return;
  }

  one_argument_u((char *)argument, arg);
  target = *arg ? arg : GET_NAME(ch);

  bounty = vessel_get_bounty(target);
  if (bounty <= 0)
  {
    send_to_char(ch, "%s carries no price.\r\n", *arg ? CAP(arg) : "You");
    return;
  }

  send_to_char(
      ch, "%s: %d gold on %s head%s.\r\n", *arg ? CAP(arg) : "You", bounty, *arg ? "their" : "your",
      bounty >= BOUNTY_HUNTED ? " - HUNTED by the navy"
                              : (bounty >= BOUNTY_WANTED ? " - WANTED in lawful ports" : ""));

  if (!*arg && vessel_has_letter_of_marque(GET_NAME(ch)))
  {
    send_to_char(ch, "You hold a valid letter of marque.\r\n");
  }
}

/**
 * marque - buy a letter of marque, or check the one you hold.
 */
ACMD(do_marque)
{
  char query[MAX_STRING_LENGTH];
  char escaped[130];
  int expiry;

  if (!mysql_available || conn == NULL)
  {
    send_to_char(ch, "The admiralty office is closed.\r\n");
    return;
  }

  if (!vessel_room_is_port(IN_ROOM(ch)))
  {
    send_to_char(ch, "Letters of marque are issued at a port's admiralty office.\r\n");
    return;
  }

  if (vessel_has_letter_of_marque(GET_NAME(ch)))
  {
    send_to_char(ch, "You already hold a valid letter of marque.\r\n");
    return;
  }

  if (vessel_get_bounty(GET_NAME(ch)) >= BOUNTY_WANTED)
  {
    send_to_char(ch, "The admiralty does not commission wanted pirates. Settle your bounty "
                     "first.\r\n");
    return;
  }

  if (GET_GOLD(ch) < MARQUE_COST)
  {
    send_to_char(ch, "A letter of marque costs %d gold; you have %d.\r\n", MARQUE_COST,
                 GET_GOLD(ch));
    return;
  }

  expiry = (int)time(0) + MARQUE_DURATION;
  mysql_real_escape_string(conn, escaped, GET_NAME(ch), strlen(GET_NAME(ch)));
  snprintf(query, sizeof(query),
           "INSERT INTO vessel_bounties (player_name, marque_until) VALUES ('%s', %d) "
           "ON DUPLICATE KEY UPDATE marque_until = %d",
           escaped, expiry, expiry);
  if (mysql_query(conn, query))
  {
    log("SYSERR: marque issue failed: %s", mysql_error(conn));
    send_to_char(ch, "The clerk cannot complete the commission.\r\n");
    return;
  }

  GET_GOLD(ch) -= MARQUE_COST;
  send_to_char(ch,
               "You pay %d gold. The admiralty commissions you as a privateer - prizes "
               "taken now are lawful.\r\n",
               MARQUE_COST);
  log("Info: %s bought a letter of marque for %d gold", GET_NAME(ch), MARQUE_COST);
}
