/* ************************************************************************
 *      File:   vessels_ownership.c                   Part of LuminariMUD  *
 *   Purpose:   Ship ownership and helm permissions (Phase 06).            *
 *              Owner persistence on ship_interiors, helm permits stored   *
 *              in ship_crew_roster, and the player-facing commands:       *
 *              shippermit, shiprevoke, shipcrew, shipdeed.                *
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

/* Helm permits ride in ship_crew_roster as crew_role='captain' rows with
 * npc_vnum = -1 marking them as player permits rather than NPC crew. */
#define PERMIT_ROLE "captain"
#define PERMIT_NPC_VNUM -1

/**
 * Is this character cleared to operate the ship's helm?
 *
 * Unowned ships are free for anyone (test vessels, ferries before claim).
 * Owned ships answer only to the owner, their permitted helmsmen, and
 * immortals.
 */
bool vessel_helm_permitted(struct char_data *ch, struct greyhawk_ship_data *ship)
{
  int i;

  if (ch == NULL || ship == NULL)
  {
    return FALSE;
  }

  if (ship->owner[0] == '\0')
  {
    return TRUE;
  }

  if (IS_NPC(ch))
  {
    return TRUE; /* NPC pilots run automation regardless of ownership */
  }

  if (!str_cmp(ship->owner, GET_NAME(ch)))
  {
    return TRUE;
  }

  if (GET_LEVEL(ch) >= LVL_IMMORT)
  {
    return TRUE;
  }

  for (i = 0; i < ship->num_permits && i < MAX_HELM_PERMITS; i++)
  {
    if (!str_cmp(ship->helm_permits[i], GET_NAME(ch)))
    {
      return TRUE;
    }
  }

  return FALSE;
}

/**
 * Ensure the owner column exists on ship_interiors.
 * Called once at boot; mirrored by sql/components/vessels_phase6_schema.sql.
 */
void vessel_ownership_ensure_schema(void)
{
  if (!mysql_available || conn == NULL)
  {
    return;
  }

  if (mysql_query(conn, "ALTER TABLE ship_interiors "
                        "ADD COLUMN IF NOT EXISTS owner VARCHAR(64) NOT NULL DEFAULT '', "
                        "ADD COLUMN IF NOT EXISTS upgrades INT NOT NULL DEFAULT 0, "
                        "ADD COLUMN IF NOT EXISTS insured_for INT NOT NULL DEFAULT 0, "
                        "ADD COLUMN IF NOT EXISTS wages_owed INT NOT NULL DEFAULT 0"))
  {
    log("SYSERR: vessel_ownership_ensure_schema failed: %s", mysql_error(conn));
  }
}

/**
 * Persist the ship's owner.
 */
void vessel_db_save_owner(struct greyhawk_ship_data *ship)
{
  char query[MAX_STRING_LENGTH];
  char escaped[130];

  if (!mysql_available || conn == NULL || ship == NULL)
  {
    return;
  }

  mysql_real_escape_string(conn, escaped, ship->owner, strlen(ship->owner));
  snprintf(query, sizeof(query), "UPDATE ship_interiors SET owner = '%s' WHERE ship_id = %d",
           escaped, ship->shipnum);

  if (mysql_query(conn, query))
  {
    log("SYSERR: vessel_db_save_owner failed for ship %d: %s", ship->shipnum,
        mysql_error(conn));
  }
}

/**
 * Load the ship's owner.
 */
void vessel_db_load_owner(struct greyhawk_ship_data *ship)
{
  char query[MAX_STRING_LENGTH];
  MYSQL_RES *result;
  MYSQL_ROW row;

  if (!mysql_available || conn == NULL || ship == NULL)
  {
    return;
  }

  snprintf(query, sizeof(query), "SELECT owner FROM ship_interiors WHERE ship_id = %d",
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
  if (row != NULL && row[0] != NULL)
  {
    strlcpy(ship->owner, row[0], sizeof(ship->owner));
  }
  mysql_free_result(result);
}

/**
 * Apply the permanent player-deletion policy.
 *
 * Soft-deleted characters retain their deeds so staff restoration is lossless.
 * Only remove_player() calls this hook, at the point the player files are about
 * to be destroyed. Permanent deletion makes every owned ship unclaimed,
 * removes the deleted name from helm permits, voids unpaid settlements, and
 * clears consent snapshots inherited from that owner.
 */
bool vessel_handle_player_removal(const char *player_name)
{
  char escaped_name[129];
  char query[MAX_STRING_LENGTH];
  struct greyhawk_ship_data *ship;
  int unowned;
  int permits_removed;
  int i;
  int j;
  int old_count;
  int kept;
  int write_index;

  if (player_name == NULL || !*player_name)
  {
    return FALSE;
  }
  if (!mysql_available || conn == NULL)
  {
    log("SYSERR: Permanent removal of %s deferred: vessel database unavailable",
        player_name);
    return FALSE;
  }

  vessel_persistence_ensure_schema();
  mysql_real_escape_string(conn, escaped_name, player_name, strlen(player_name));
  if (mysql_query(conn, "START TRANSACTION"))
  {
    log("SYSERR: Could not begin vessel orphan cleanup for %s: %s", player_name,
        mysql_error(conn));
    return FALSE;
  }

  snprintf(query, sizeof(query),
           "UPDATE ship_interiors SET owner = '' WHERE owner = '%s'", escaped_name);
  if (mysql_query(conn, query))
  {
    goto rollback;
  }

  snprintf(query, sizeof(query),
           "DELETE FROM ship_crew_roster "
           "WHERE npc_vnum = %d AND crew_role = '%s' AND npc_name = '%s'",
           PERMIT_NPC_VNUM, PERMIT_ROLE, escaped_name);
  if (mysql_query(conn, query))
  {
    goto rollback;
  }

  snprintf(query, sizeof(query),
           "UPDATE vessel_insurance_claims SET status = 'void' "
           "WHERE owner = '%s' AND status = 'pending'",
           escaped_name);
  if (mysql_query(conn, query))
  {
    goto rollback;
  }

  if (mysql_query(conn, "COMMIT"))
  {
    log("SYSERR: Could not commit vessel orphan cleanup for %s: %s", player_name,
        mysql_error(conn));
    mysql_query(conn, "ROLLBACK");
    return FALSE;
  }

  unowned = 0;
  permits_removed = 0;
  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
  {
    ship = &greyhawk_ships[i];
    if (!is_valid_ship(ship))
    {
      continue;
    }

    if (!str_cmp(ship->owner, player_name))
    {
      ship->owner[0] = '\0';
      vessel_clear_pvp_grace(ship);
      unowned++;
    }

    old_count = MIN(ship->num_permits, MAX_HELM_PERMITS);
    write_index = 0;
    for (j = 0; j < old_count; j++)
    {
      if (!str_cmp(ship->helm_permits[j], player_name))
      {
        permits_removed++;
        continue;
      }
      if (write_index != j)
      {
        strlcpy(ship->helm_permits[write_index], ship->helm_permits[j],
                sizeof(ship->helm_permits[write_index]));
      }
      write_index++;
    }
    kept = write_index;
    while (write_index < old_count)
    {
      ship->helm_permits[write_index][0] = '\0';
      write_index++;
    }
    ship->num_permits = kept;
  }

  log("Info: Permanent removal of %s unowned %d ship%s and removed %d helm permit%s",
      player_name, unowned, unowned == 1 ? "" : "s", permits_removed,
      permits_removed == 1 ? "" : "s");
  return TRUE;

rollback:
  log("SYSERR: Vessel orphan cleanup failed for %s: %s", player_name,
      mysql_error(conn));
  mysql_query(conn, "ROLLBACK");
  return FALSE;
}

/**
 * Persist the helm permit list (delete-and-reinsert, idempotent).
 */
void vessel_db_save_permits(struct greyhawk_ship_data *ship)
{
  char query[MAX_STRING_LENGTH];
  char escaped[64];
  int i;

  if (!mysql_available || conn == NULL || ship == NULL)
  {
    return;
  }

  snprintf(query, sizeof(query),
           "DELETE FROM ship_crew_roster WHERE ship_id = %d AND crew_role = '%s' "
           "AND npc_vnum = %d",
           ship->shipnum, PERMIT_ROLE, PERMIT_NPC_VNUM);
  if (mysql_query(conn, query))
  {
    log("SYSERR: vessel_db_save_permits (clear) failed for ship %d: %s", ship->shipnum,
        mysql_error(conn));
    return;
  }

  for (i = 0; i < ship->num_permits && i < MAX_HELM_PERMITS; i++)
  {
    mysql_real_escape_string(conn, escaped, ship->helm_permits[i], strlen(ship->helm_permits[i]));
    snprintf(query, sizeof(query),
             "INSERT INTO ship_crew_roster (ship_id, npc_vnum, npc_name, crew_role) "
             "VALUES (%d, %d, '%s', '%s')",
             ship->shipnum, PERMIT_NPC_VNUM, escaped, PERMIT_ROLE);
    if (mysql_query(conn, query))
    {
      log("SYSERR: vessel_db_save_permits (insert) failed for ship %d: %s", ship->shipnum,
          mysql_error(conn));
    }
  }
}

/**
 * Load the helm permit list.
 */
void vessel_db_load_permits(struct greyhawk_ship_data *ship)
{
  char query[MAX_STRING_LENGTH];
  MYSQL_RES *result;
  MYSQL_ROW row;

  if (!mysql_available || conn == NULL || ship == NULL)
  {
    return;
  }

  snprintf(query, sizeof(query),
           "SELECT npc_name FROM ship_crew_roster WHERE ship_id = %d AND crew_role = '%s' "
           "AND npc_vnum = %d",
           ship->shipnum, PERMIT_ROLE, PERMIT_NPC_VNUM);
  if (mysql_query(conn, query))
  {
    return;
  }

  result = mysql_store_result(conn);
  if (result == NULL)
  {
    return;
  }

  ship->num_permits = 0;
  while ((row = mysql_fetch_row(result)) != NULL && ship->num_permits < MAX_HELM_PERMITS)
  {
    if (row[0] != NULL && row[0][0] != '\0')
    {
      strlcpy(ship->helm_permits[ship->num_permits], row[0], sizeof(ship->helm_permits[0]));
      ship->num_permits++;
    }
  }
  mysql_free_result(result);
}

/**
 * Shared owner gate for the ownership commands.
 *
 * @return The ship if ch is aboard and owns it (or is immortal), else NULL
 */
static struct greyhawk_ship_data *ownership_command_ship(struct char_data *ch)
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
    send_to_char(ch, "This ship has no owner - use 'claimship' from the bridge first.\r\n");
    return NULL;
  }

  if (str_cmp(ship->owner, GET_NAME(ch)) && GET_LEVEL(ch) < LVL_IMMORT)
  {
    send_to_char(ch, "Only %s's owner (%s) may do that.\r\n", ship->name, ship->owner);
    return NULL;
  }

  return ship;
}

/**
 * shippermit <player> - clear a player to take the helm.
 */
ACMD(do_shippermit)
{
  struct greyhawk_ship_data *ship;
  char arg[MAX_INPUT_LENGTH];
  int i;

  ship = ownership_command_ship(ch);
  if (ship == NULL)
  {
    return;
  }

  one_argument_u((char *)argument, arg);
  if (!*arg)
  {
    send_to_char(ch, "Permit whom to take the helm?\r\n");
    return;
  }

  if (strlen(arg) > 20)
  {
    send_to_char(ch, "That name is too long.\r\n");
    return;
  }

  for (i = 0; i < ship->num_permits; i++)
  {
    if (!str_cmp(ship->helm_permits[i], arg))
    {
      send_to_char(ch, "%s is already cleared for the helm.\r\n", arg);
      return;
    }
  }

  if (ship->num_permits >= MAX_HELM_PERMITS)
  {
    send_to_char(ch, "The permit roster is full (%d names).\r\n", MAX_HELM_PERMITS);
    return;
  }

  strlcpy(ship->helm_permits[ship->num_permits], CAP(arg), sizeof(ship->helm_permits[0]));
  ship->num_permits++;
  vessel_db_save_permits(ship);

  send_to_char(ch, "%s is now cleared to take the helm of %s.\r\n",
               ship->helm_permits[ship->num_permits - 1], ship->name);
}

/**
 * shiprevoke <player> - revoke a helm permit.
 */
ACMD(do_shiprevoke)
{
  struct greyhawk_ship_data *ship;
  char arg[MAX_INPUT_LENGTH];
  int i, j;

  ship = ownership_command_ship(ch);
  if (ship == NULL)
  {
    return;
  }

  one_argument_u((char *)argument, arg);
  if (!*arg)
  {
    send_to_char(ch, "Revoke whose helm permit?\r\n");
    return;
  }

  for (i = 0; i < ship->num_permits; i++)
  {
    if (!str_cmp(ship->helm_permits[i], arg))
    {
      for (j = i; j < ship->num_permits - 1; j++)
      {
        strlcpy(ship->helm_permits[j], ship->helm_permits[j + 1], sizeof(ship->helm_permits[0]));
      }
      ship->num_permits--;
      vessel_db_save_permits(ship);
      send_to_char(ch, "%s is no longer cleared for the helm of %s.\r\n", CAP(arg), ship->name);
      return;
    }
  }

  send_to_char(ch, "No permit found for '%s'.\r\n", arg);
}

/**
 * shipcrew - list ownership, permits, and NPC pilot.
 */
ACMD(do_shipcrew)
{
  struct greyhawk_ship_data *ship;
  int i;

  ship = get_ship_from_room(IN_ROOM(ch));
  if (ship == NULL)
  {
    send_to_char(ch, "You must be aboard a ship.\r\n");
    return;
  }

  send_to_char(ch, "%s [%s]\r\n", ship->name, ship->id);
  send_to_char(ch, "  Owner : %s\r\n", ship->owner[0] ? ship->owner : "unclaimed");
  send_to_char(ch, "  Pilot : %s\r\n",
               (ship->autopilot != NULL && ship->autopilot->pilot_mob_vnum != -1)
                   ? "NPC pilot assigned"
                   : "none");
  if (ship->num_permits == 0)
  {
    send_to_char(ch, "  Helm permits: none\r\n");
  }
  else
  {
    send_to_char(ch, "  Helm permits:\r\n");
    for (i = 0; i < ship->num_permits; i++)
    {
      send_to_char(ch, "    %s\r\n", ship->helm_permits[i]);
    }
  }

  send_to_char(ch, "  Hired crew:\r\n");
  for (i = 0; i < NUM_CREW_POSITIONS; i++)
  {
    send_to_char(ch, "    %-14s %s\r\n", vessel_crew_position_name(i),
                 vessel_crew_tier_name(ship->crew_tier[i]));
  }
  if (ship->wages_owed > 0)
  {
    send_to_char(ch, "  Back wages owed: %d gold\r\n", ship->wages_owed);
  }
}

/**
 * shipdeed <player> - transfer ownership to another player present aboard.
 */
ACMD(do_shipdeed)
{
  struct greyhawk_ship_data *ship;
  struct char_data *target;
  char arg[MAX_INPUT_LENGTH];

  ship = ownership_command_ship(ch);
  if (ship == NULL)
  {
    return;
  }

  one_argument_u((char *)argument, arg);
  if (!*arg)
  {
    send_to_char(ch, "Deed %s to whom? (They must be here with you.)\r\n", ship->name);
    return;
  }

  target = get_char_room_vis(ch, arg, NULL);
  if (target == NULL)
  {
    send_to_char(ch, "They need to be here to accept the deed.\r\n");
    return;
  }
  if (IS_NPC(target))
  {
    send_to_char(ch, "NPCs cannot hold a ship's deed.\r\n");
    return;
  }

  log("Info: %s deeded ship %d '%s' to %s", GET_NAME(ch), ship->shipnum, ship->name,
      GET_NAME(target));
  vessel_clear_pvp_grace(ship);
  strlcpy(ship->owner, GET_NAME(target), sizeof(ship->owner));
  vessel_db_save_owner(ship);

  send_to_char(ch, "You sign over %s to %s.\r\n", ship->name, GET_NAME(target));
  send_to_char(target, "%s signs over %s to you - she's yours now.\r\n", GET_NAME(ch), ship->name);
  send_to_ship(ship, "%s is under new ownership: %s.", ship->name, GET_NAME(target));
}
