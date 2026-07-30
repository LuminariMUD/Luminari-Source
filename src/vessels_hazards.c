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
 * Resolve overlapping encounter regions independently of database row order.
 *
 * Prefer the strongest containment position, then the lowest region VNUM as a
 * stable builder-visible tie-break. get_enclosing_regions() does not promise
 * result order, so selecting its first node made identical coordinates depend
 * on the query plan.
 */
bool vessel_encounter_region_from_list(const struct region_list *regions, int *output_region_vnum)
{
  const struct region_list *curr;
  int best_position = -1;
  region_vnum best_vnum = 0;
  int position;
  bool found = FALSE;

  if (output_region_vnum == NULL)
  {
    return FALSE;
  }
  *output_region_vnum = 0;

  if (region_table == NULL || top_of_region_table == NOWHERE)
  {
    return FALSE;
  }

  for (curr = regions; curr != NULL; curr = curr->next)
  {
    if (curr->rnum == NOWHERE || curr->rnum > top_of_region_table ||
        region_table[curr->rnum].region_type != REGION_ENCOUNTER)
    {
      continue;
    }

    switch (curr->pos)
    {
    case REGION_POS_CENTER:
      position = 3;
      break;
    case REGION_POS_INSIDE:
      position = 2;
      break;
    case REGION_POS_EDGE:
      position = 1;
      break;
    default:
      position = 0;
      break;
    }

    if (!found || position > best_position ||
        (position == best_position && region_table[curr->rnum].vnum < best_vnum))
    {
      best_position = position;
      best_vnum = region_table[curr->rnum].vnum;
      found = TRUE;
    }
  }

  if (found)
  {
    *output_region_vnum = (int)best_vnum;
  }

  return found;
}

/**
 * Apply an encounter chance to a supplied d100 result.
 *
 * Keeping the boundary rule separate makes the random table reproducible in
 * production-linked tests while runtime still obtains its roll from the
 * shared random-number generator.
 */
bool vessel_encounter_chance_succeeds(int chance, int roll)
{
  if (roll < 1 || roll > 100 || chance <= 0)
  {
    return FALSE;
  }
  if (chance >= 100)
  {
    return TRUE;
  }
  return roll <= chance;
}

/**
 * Claim one exterior wilderness room for this encounter tick.
 *
 * Multiple ships at one coordinate share the same dynamic room. Only the
 * first successful encounter may claim it, preventing duplicate creatures
 * from being spawned into what is meant to be one shared-world encounter.
 */
bool vessel_encounter_claim_room(room_rnum room, room_rnum *claimed_rooms, int *claimed_count,
                                 int claimed_capacity)
{
  int i;

  if (room == NOWHERE || claimed_rooms == NULL || claimed_count == NULL || *claimed_count < 0 ||
      *claimed_count >= claimed_capacity)
  {
    return FALSE;
  }

  for (i = 0; i < *claimed_count; i++)
  {
    if (claimed_rooms[i] == room)
    {
      return FALSE;
    }
  }

  claimed_rooms[*claimed_count] = room;
  (*claimed_count)++;
  return TRUE;
}

static bool vessel_encounter_room_is_claimed(room_rnum room, const room_rnum *claimed_rooms,
                                             int claimed_count)
{
  int i;

  if (room == NOWHERE || claimed_rooms == NULL || claimed_count <= 0)
  {
    return FALSE;
  }

  for (i = 0; i < claimed_count; i++)
  {
    if (claimed_rooms[i] == room)
    {
      return TRUE;
    }
  }

  return FALSE;
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
  bool found;

  if (ship == NULL || region_vnum == NULL)
  {
    return FALSE;
  }

  regions = get_enclosing_regions(real_zone(WILD_ZONE_VNUM), (int)ship->x, (int)ship->y);
  found = vessel_encounter_region_from_list(regions, region_vnum);
  free_region_list(regions);

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
        send_to_ship_throttled(ship, VESSEL_MESSAGE_AMBIENT_DEPTH,
                               VESSEL_AMBIENT_MESSAGE_COOLDOWN,
                               "The hull GROANS - you are far too deep!");
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
      send_to_ship_throttled(ship, VESSEL_MESSAGE_AMBIENT_SQUALL,
                             VESSEL_AMBIENT_MESSAGE_COOLDOWN,
                             "A squall slaps spray across the deck.");
      break;
    case 2:
      send_to_ship_throttled(ship, VESSEL_MESSAGE_AMBIENT_STORM,
                             VESSEL_AMBIENT_MESSAGE_COOLDOWN,
                             "The storm tears at the rigging!");
      if (ship->mainsail > 1)
      {
        ship->mainsail--;
      }
      break;
    case 3:
    default:
      send_to_ship_throttled(ship, VESSEL_MESSAGE_AMBIENT_GALE,
                             VESSEL_AMBIENT_MESSAGE_COOLDOWN,
                             "A GALE hammers the ship - the masts scream under the strain!");
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

static int vessel_broadcast_encounter(room_rnum ship_room,
                                      struct greyhawk_ship_data *source,
                                      const char *warn_message, const char *arrive_message,
                                      const char *encounter_name)
{
  struct greyhawk_ship_data *recipient;
  int recipient_count = 0;
  int i;

  if (ship_room != NOWHERE)
  {
    for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
    {
      recipient = &greyhawk_ships[i];
      if (!is_valid_ship(recipient) || recipient->shipobj == NULL ||
          IN_ROOM(recipient->shipobj) != ship_room)
      {
        continue;
      }

      if (warn_message != NULL && *warn_message && vessel_lookout_bonus(recipient) > 0)
      {
        send_to_ship(recipient, "%s", warn_message);
      }
      if (arrive_message != NULL && *arrive_message)
      {
        send_to_ship(recipient, "%s", arrive_message);
      }
      else
      {
        send_to_ship(recipient, "%s closes on the ship!",
                     encounter_name != NULL ? encounter_name : "Something");
      }
      recipient_count++;
    }
  }

  if (recipient_count == 0 && source != NULL)
  {
    if (warn_message != NULL && *warn_message && vessel_lookout_bonus(source) > 0)
    {
      send_to_ship(source, "%s", warn_message);
    }
    if (arrive_message != NULL && *arrive_message)
    {
      send_to_ship(source, "%s", arrive_message);
    }
    else
    {
      send_to_ship(source, "%s closes on the ship!",
                   encounter_name != NULL ? encounter_name : "Something");
    }
    recipient_count = 1;
  }

  return recipient_count;
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
  struct vessel_hunter_config hunter_config;
  room_rnum claimed_rooms[GREYHAWK_MAXSHIPS];
  room_rnum ship_room;
  int claimed_count = 0;
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

    ship_room = ship->shipobj != NULL ? IN_ROOM(ship->shipobj) : NOWHERE;
    if (vessel_encounter_room_is_claimed(ship_room, claimed_rooms, claimed_count))
    {
      continue;
    }

    depth_units = wild_waterline - get_modified_elevation((int)ship->x, (int)ship->y);

    /* Draw candidates matching this region, depth band, and hull class */
    snprintf(query, sizeof(query),
             "SELECT encounter_id, name, mob_vnum, chance, warn_message, "
             "arrive_message "
             "FROM vessel_encounters WHERE region_vnum = %d "
             "AND (vessel_class = -1 OR vessel_class = %d) "
             "AND (max_depth = 0 OR (%d BETWEEN min_depth AND max_depth)) "
             "ORDER BY chance DESC, encounter_id ASC",
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
      int chance = row[3] ? atoi(row[3]) : 0;
      int hunter_configured;
      int recipient_count;

      hunter_configured =
          vessel_hunter_load_config(row[0] ? atoi(row[0]) : 0,
                                    &hunter_config);
      if (hunter_configured < 0)
      {
        continue;
      }
      if (hunter_configured > 0 &&
          !vessel_hunter_target_is_eligible(ship, &hunter_config, time(0)))
      {
        continue;
      }

      /* A good lookout gives warning before the thing arrives */
      if (!vessel_encounter_chance_succeeds(chance, rand_number(1, 100)))
      {
        continue;
      }

      if (ship_room != NOWHERE &&
          !vessel_encounter_claim_room(ship_room, claimed_rooms, &claimed_count,
                                       GREYHAWK_MAXSHIPS))
      {
        break;
      }

      if (hunter_configured > 0 &&
          !vessel_hunter_spawn(ship, &hunter_config, row[1]))
      {
        if (ship_room != NOWHERE && claimed_count > 0)
        {
          claimed_count--;
        }
        continue;
      }

      recipient_count =
          vessel_broadcast_encounter(ship_room, ship, row[4], row[5], row[1]);
      log("Info: Shared encounter '%s' in room %d from ship %d notified %d "
          "vessels in region %d",
          row[1] ? row[1] : "?", ship_room, i, recipient_count, region_vnum);

      /* Spawn the encounter's creature into the ship's wilderness room so
       * it can be fought, fled, or fired upon like anything else. */
      if (hunter_configured == 0 && row[2] != NULL && atoi(row[2]) > 0 &&
          ship_room != NOWHERE)
      {
        mob = read_mobile(atoi(row[2]), VIRTUAL);
        if (mob != NULL)
        {
          char_to_room(mob, ship_room);
          act("$n rises from the depths!", FALSE, mob, 0, 0, TO_ROOM);
          log("Info: Encounter '%s' spawned for shared room %d from ship %d in region %d",
              row[1] ? row[1] : "?", ship_room, i, region_vnum);
        }
      }

      break; /* One encounter per check */
    }

    mysql_free_result(result);
  }
}

/**
 * Staff acceptance hook: run the next normal encounter check immediately.
 *
 * This changes only the cadence counter. Selection, region/class/depth
 * filtering, chance, HUNTED eligibility, spawning, and lifecycle work all
 * remain on the production encounter path.
 */
void vessel_encounter_force_check(void)
{
  encounter_ticks = VESSEL_ENCOUNTER_INTERVAL - 1;
  vessel_encounter_tick();
}

/**
 * seastate - report the weather, sea state, and visibility around the ship.
 */
ACMD(do_seastate)
{
  struct greyhawk_ship_data *ship;
  struct vessel_piracy_law law;
  bool named_waters;
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

  named_waters = vessel_piracy_law_for_ship(ship, &law);
  if (named_waters && law.configured)
  {
    send_to_char(ch, "  Waters    : %s (%s; %s; piracy bounty %d%%)\r\n",
                 law.region_name, vessel_waters_type_name(law.waters_type),
                 law.authority, law.bounty_percent);
  }
  else if (named_waters)
  {
    send_to_char(ch, "  Waters    : %s (standard maritime law)\r\n", law.region_name);
  }
  else
  {
    send_to_char(ch, "  Waters    : Unnamed open waters (standard maritime law)\r\n");
  }

  if (vessel_in_encounter_region(ship, &region_vnum))
  {
    send_to_char(ch, "  These are dangerous waters - keep a sharp watch.\r\n");
  }
}
