/* ************************************************************************
 *      File:   vessels_hazards.c                     Part of LuminariMUD  *
 *   Purpose:   Living world: weather hazards and encounters (Phase 08).   *
 *              Every signal here comes from the wilderness system - the   *
 *              same weather field, regions, and bathymetry a walker on    *
 *              the coast experiences. No vessel-private geography.        *
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
#include "wilderness.h"
#include "mysql.h"
#include "constants.h"
#include "act.h"

extern MYSQL *conn;
extern bool mysql_available;
extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];
extern struct room_data *world;
extern struct region_data *region_table;
extern int wild_waterline;

static int hazard_ticks = 0;
static int encounter_ticks = 0;

/**
 * Create the encounter table, keyed to wilderness region vnums.
 *
 * Encounters attach to REGION_ENCOUNTER regions authored with the existing
 * region tooling - the vessel system never invents its own geography
 * (PRD Section 4, ground rule 2). Mirrored by
 * sql/components/vessels_phase8_schema.sql.
 */
void vessel_hazard_ensure_schema(void)
{
  if (!mysql_available || conn == NULL)
  {
    return;
  }

  if (mysql_query(conn, "CREATE TABLE IF NOT EXISTS vessel_encounters ("
                        "  encounter_id INT AUTO_INCREMENT PRIMARY KEY,"
                        "  region_vnum INT NOT NULL,"
                        "  name VARCHAR(127) NOT NULL,"
                        "  mob_vnum INT NOT NULL DEFAULT 0,"
                        "  min_depth INT NOT NULL DEFAULT 0,"
                        "  max_depth INT NOT NULL DEFAULT 0,"
                        "  vessel_class INT NOT NULL DEFAULT -1,"
                        "  chance INT NOT NULL DEFAULT 10,"
                        "  warn_message VARCHAR(255) NOT NULL DEFAULT '',"
                        "  arrive_message VARCHAR(255) NOT NULL DEFAULT '',"
                        "  INDEX idx_region (region_vnum)"
                        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"))
  {
    log("SYSERR: vessel_encounters create failed: %s", mysql_error(conn));
  }
}

/**
 * How far this ship can see, in coordinate units.
 *
 * Fog closes the horizon; a lookout partially compensates. Reads the same
 * wilderness weather field as everything else.
 */
int vessel_sight_range(const struct greyhawk_ship_data *ship)
{
  int weather;
  int range = VESSEL_SIGHT_CLEAR;

  if (ship == NULL)
  {
    return VESSEL_SIGHT_CLEAR;
  }

  weather = get_weather((int)ship->x, (int)ship->y);
  if (weather >= VESSEL_WEATHER_FOG)
  {
    range = VESSEL_SIGHT_FOG;
  }
  else if (weather >= VESSEL_WEATHER_FOG / 2)
  {
    range = (VESSEL_SIGHT_CLEAR + VESSEL_SIGHT_FOG) / 2;
  }

  /* An able lookout sees further through murk */
  range += vessel_lookout_bonus(ship);

  return range;
}

/**
 * Storm severity band at the ship's position (0 = calm).
 *
 * Airships fly through the same weather column and feel it harder;
 * submerged submarines are sheltered from surface weather entirely.
 */
int vessel_storm_severity(const struct greyhawk_ship_data *ship)
{
  int weather;

  if (ship == NULL)
  {
    return 0;
  }

  /* Submerged boats ride out surface weather */
  if (ship->vessel_type == VESSEL_SUBMARINE && ship->z < 0)
  {
    return 0;
  }

  weather = get_weather((int)ship->x, (int)ship->y);

  if (weather >= VESSEL_WEATHER_GALE)
  {
    return 3;
  }
  if (weather >= VESSEL_WEATHER_STORM)
  {
    return 2;
  }
  if (weather >= VESSEL_WEATHER_SQUALL)
  {
    return 1;
  }

  return 0;
}

/**
 * Lookout quality bonus. The sailmaster's watch doubles as lookout until
 * a dedicated position exists; a crewed ship simply sees better.
 */
int vessel_lookout_bonus(const struct greyhawk_ship_data *ship)
{
  if (ship == NULL)
  {
    return 0;
  }

  return ship->crew_tier[CREW_SAILMASTER] * 5;
}

/**
 * Is this ship inside a wilderness encounter region?
 *
 * @param region_vnum Out: the containing encounter region's vnum
 * @return TRUE if inside one
 */
bool vessel_in_encounter_region(const struct greyhawk_ship_data *ship, int *region_vnum)
{
  struct region_list *regions;
  struct region_list *curr;
  struct region_list *next;
  bool found = FALSE;

  if (ship == NULL || region_vnum == NULL)
  {
    return FALSE;
  }

  regions = get_enclosing_regions(real_zone(WILD_ZONE_VNUM), (int)ship->x, (int)ship->y);

  for (curr = regions; curr != NULL; curr = curr->next)
  {
    if (curr->rnum == NOWHERE)
    {
      continue;
    }
    if (region_table[curr->rnum].region_type == REGION_ENCOUNTER)
    {
      *region_vnum = region_table[curr->rnum].vnum;
      found = TRUE;
      break;
    }
  }

  /* get_enclosing_regions allocates its list; free it */
  for (curr = regions; curr != NULL; curr = next)
  {
    next = curr->next;
    free(curr);
  }

  return found;
}

/**
 * Weather hazard tick: storms damage rigging and force helm checks, and
 * submarines beyond crush depth take hull damage.
 */
void vessel_weather_tick(void)
{
  struct greyhawk_ship_data *ship;
  int severity;
  int depth_units;
  int i;

  hazard_ticks++;
  if (hazard_ticks < VESSEL_HAZARD_INTERVAL)
  {
    return;
  }
  hazard_ticks = 0;

  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
  {
    ship = &greyhawk_ships[i];
    if (!is_valid_ship(ship))
    {
      continue;
    }

    /* Crush depth: submarines diving past the seabed's depth at this
     * coordinate are being pressed against the bottom. */
    if (ship->vessel_type == VESSEL_SUBMARINE && ship->z < 0)
    {
      depth_units = wild_waterline - get_modified_elevation((int)ship->x, (int)ship->y);
      if (-((int)ship->z) > depth_units * 8)
      {
        send_to_ship(ship, "The hull GROANS - you are far too deep!");
        vessel_apply_damage(i, dice(2, 6), GREYHAWK_FORE, "Crushing pressure");
        continue;
      }
    }

    severity = vessel_storm_severity(ship);
    if (severity == 0)
    {
      continue;
    }

    /* A moored ship rides out weather at its lines */
    if (ship->speed == 0)
    {
      continue;
    }

    switch (severity)
    {
    case 1:
      send_to_ship(ship, "A squall slaps spray across the deck.");
      break;
    case 2:
      send_to_ship(ship, "The storm tears at the rigging!");
      if (ship->mainsail > 1)
      {
        ship->mainsail--;
      }
      break;
    case 3:
    default:
      send_to_ship(ship, "A GALE hammers the ship - the masts scream under the strain!");
      if (ship->mainsail > 2)
      {
        ship->mainsail -= 2;
      }
      /* A gale costs structure only when neither a sailmaster nor the
       * vessel's assigned pilot is physically at the helm. */
      if (ship->crew_tier[CREW_SAILMASTER] == CREW_TIER_NONE &&
          get_pilot_from_ship(ship) == NULL)
      {
        vessel_apply_damage(i, dice(1, 6), GREYHAWK_PORT, "The gale");
      }
      break;
    }

    VSSL_DEBUG("Ship %d weather hazard: severity %d sail %d", i, severity, ship->mainsail);
  }
}

/**
 * Encounter tick: ships inside wilderness encounter regions may draw an
 * encounter from that region's table.
 */
void vessel_encounter_tick(void)
{
  char query[MAX_STRING_LENGTH];
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct greyhawk_ship_data *ship;
  struct char_data *mob;
  room_rnum ship_room;
  int region_vnum = 0;
  int depth_units;
  int i;

  encounter_ticks++;
  if (encounter_ticks < VESSEL_ENCOUNTER_INTERVAL)
  {
    return;
  }
  encounter_ticks = 0;

  if (!mysql_available || conn == NULL)
  {
    return;
  }

  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
  {
    ship = &greyhawk_ships[i];
    if (!is_valid_ship(ship) || ship->speed == 0)
    {
      continue; /* Encounters find ships that are moving */
    }

    if (!vessel_in_encounter_region(ship, &region_vnum))
    {
      continue;
    }

    depth_units = wild_waterline - get_modified_elevation((int)ship->x, (int)ship->y);

    /* Draw candidates matching this region, depth band, and hull class */
    snprintf(query, sizeof(query),
             "SELECT name, mob_vnum, chance, warn_message, arrive_message "
             "FROM vessel_encounters WHERE region_vnum = %d "
             "AND (vessel_class = -1 OR vessel_class = %d) "
             "AND (max_depth = 0 OR (%d BETWEEN min_depth AND max_depth)) "
             "ORDER BY chance DESC",
             region_vnum, (int)ship->vessel_type, depth_units);
    if (mysql_query(conn, query))
    {
      log("SYSERR: encounter query failed: %s", mysql_error(conn));
      continue;
    }

    result = mysql_store_result(conn);
    if (result == NULL)
    {
      continue;
    }

    while ((row = mysql_fetch_row(result)) != NULL)
    {
      int chance = row[2] ? atoi(row[2]) : 0;

      /* A good lookout gives warning before the thing arrives */
      if (rand_number(1, 100) > chance)
      {
        continue;
      }

      if (row[3] != NULL && *row[3] && vessel_lookout_bonus(ship) > 0)
      {
        send_to_ship(ship, "%s", row[3]);
      }

      if (row[4] != NULL && *row[4])
      {
        send_to_ship(ship, "%s", row[4]);
      }
      else
      {
        send_to_ship(ship, "%s closes on the ship!", row[0] ? row[0] : "Something");
      }

      /* Spawn the encounter's creature into the ship's wilderness room so
       * it can be fought, fled, or fired upon like anything else. */
      if (row[1] != NULL && atoi(row[1]) > 0 && ship->shipobj != NULL)
      {
        ship_room = IN_ROOM(ship->shipobj);
        if (ship_room != NOWHERE)
        {
          mob = read_mobile(atoi(row[1]), VIRTUAL);
          if (mob != NULL)
          {
            char_to_room(mob, ship_room);
            act("$n rises from the depths!", FALSE, mob, 0, 0, TO_ROOM);
            log("Info: Encounter '%s' spawned for ship %d in region %d", row[0] ? row[0] : "?", i,
                region_vnum);
          }
        }
      }

      break; /* One encounter per check */
    }

    mysql_free_result(result);
  }
}

/**
 * seastate - report the weather, sea state, and visibility around the ship.
 */
ACMD(do_seastate)
{
  struct greyhawk_ship_data *ship;
  int weather;
  int severity;
  int depth_units;
  int region_vnum = 0;
  int sector;

  ship = get_ship_from_room(IN_ROOM(ch));
  if (ship == NULL)
  {
    send_to_char(ch, "You must be aboard a ship to read the sea.\r\n");
    return;
  }

  weather = get_weather((int)ship->x, (int)ship->y);
  severity = vessel_storm_severity(ship);
  depth_units = wild_waterline - get_modified_elevation((int)ship->x, (int)ship->y);
  sector = get_ship_terrain_type(ship->shipnum);

  send_to_char(ch, "Sea state around %s at (%d, %d):\r\n", ship->name, (int)ship->x, (int)ship->y);
  send_to_char(ch, "  Water     : %s\r\n", sector_types[sector]);
  send_to_char(ch, "  Depth     : %s\r\n",
               depth_units <= 0 ? "aground or dry"
                                : (depth_units < 20 ? "shallow - watch your draft"
                                                    : (depth_units < 60 ? "moderate" : "deep")));
  send_to_char(ch, "  Weather   : %s\r\n",
               severity >= 3                   ? "a howling gale"
               : severity == 2                 ? "a full storm"
               : severity == 1                 ? "squally"
               : weather >= VESSEL_WEATHER_FOG ? "fogbound"
                                               : "fair");
  send_to_char(ch, "  Visibility: %d units%s\r\n", vessel_sight_range(ship),
               vessel_lookout_bonus(ship) > 0 ? " (lookout posted)" : "");
  send_to_char(ch, "  Hull      : %s\r\n", vessel_status_name(vessel_status(ship)));

  if (vessel_in_encounter_region(ship, &region_vnum))
  {
    send_to_char(ch, "  These are dangerous waters - keep a sharp watch.\r\n");
  }
}
