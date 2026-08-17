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
#include "wilderness/wilderness.h"
#include "mysql.h"
#include "constants.h"
#include "act.h"

#include <float.h>

extern MYSQL *conn;
extern bool mysql_available;
extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];
extern struct room_data *world;
extern struct region_data *region_table;
extern int wild_waterline;

static int hazard_ticks = 0;
static int encounter_ticks = 0;

#define VESSEL_MAX_ENCOUNTER_DEFINITIONS 1024

struct vessel_encounter_definition
{
  int encounter_id;
  int region_vnum;
  int mob_vnum;
  int min_depth;
  int max_depth;
  int vessel_class;
  int chance;
  char name[128];
  char warn_message[256];
  char arrive_message[256];
  int hunter_configured;
  struct vessel_hunter_config hunter_config;
};

static struct vessel_encounter_definition
    vessel_encounter_definitions[VESSEL_MAX_ENCOUNTER_DEFINITIONS];
static int vessel_encounter_definition_count = 0;
static bool vessel_encounter_config_loaded = FALSE;

/**
 * Create the encounter table, keyed to wilderness region vnums.
 *
 * Encounters attach to REGION_ENCOUNTER regions authored with the existing
 * region tooling - the vessel system never invents its own geography
 * (vessel product requirements Section 5, invariant 2). Mirrored by
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
 * Load encounter definitions and optional hunter policies into memory.
 *
 * Encounter cadence runs on the main game thread. Keeping the immutable
 * selection data in a bounded boot cache prevents one blocking SELECT for
 * every moving ship. Staff-forced encounter checks reload this cache first,
 * so direct builder changes remain testable without a server restart.
 */
bool vessel_encounter_reload_config(void)
{
  const char *query = "SELECT encounter.encounter_id, encounter.region_vnum, encounter.name, "
                      "encounter.mob_vnum, encounter.min_depth, encounter.max_depth, "
                      "encounter.vessel_class, encounter.chance, encounter.warn_message, "
                      "encounter.arrive_message, hunter.encounter_id, hunter.prototype_id, "
                      "hunter.pilot_mob_vnum, hunter.min_bounty, hunter.pursuit_speed, "
                      "hunter.hunt_duration_seconds, hunter.target_grace_seconds, "
                      "hunter.cooldown_seconds, hunter.enabled "
                      "FROM vessel_encounters AS encounter "
                      "LEFT JOIN vessel_hunter_encounters AS hunter "
                      "ON hunter.encounter_id = encounter.encounter_id "
                      "ORDER BY encounter.region_vnum, encounter.chance DESC, "
                      "encounter.encounter_id";
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct vessel_encounter_definition *definition;
  int loaded_count;

  if (!mysql_available || conn == NULL)
  {
    return FALSE;
  }

  if (mysql_query(conn, query))
  {
    log("SYSERR: Could not load vessel encounter definitions: %s", mysql_error(conn));
    return FALSE;
  }

  result = mysql_store_result(conn);
  if (result == NULL)
  {
    log("SYSERR: Could not store vessel encounter definitions: %s", mysql_error(conn));
    return FALSE;
  }

  loaded_count = 0;
  while ((row = mysql_fetch_row(result)) != NULL)
  {
    if (loaded_count >= VESSEL_MAX_ENCOUNTER_DEFINITIONS)
    {
      log("SYSERR: Vessel encounter cache is full at %d definitions",
          VESSEL_MAX_ENCOUNTER_DEFINITIONS);
      break;
    }

    definition = &vessel_encounter_definitions[loaded_count++];
    memset(definition, 0, sizeof(*definition));
    definition->encounter_id = row[0] ? atoi(row[0]) : 0;
    definition->region_vnum = row[1] ? atoi(row[1]) : 0;
    strlcpy(definition->name, row[2] ? row[2] : "", sizeof(definition->name));
    definition->mob_vnum = row[3] ? atoi(row[3]) : 0;
    definition->min_depth = row[4] ? atoi(row[4]) : 0;
    definition->max_depth = row[5] ? atoi(row[5]) : 0;
    definition->vessel_class = row[6] ? atoi(row[6]) : -1;
    definition->chance = row[7] ? atoi(row[7]) : 0;
    strlcpy(definition->warn_message, row[8] ? row[8] : "", sizeof(definition->warn_message));
    strlcpy(definition->arrive_message, row[9] ? row[9] : "", sizeof(definition->arrive_message));

    if (row[10] != NULL)
    {
      definition->hunter_configured = 1;
      definition->hunter_config.encounter_id = atoi(row[10]);
      definition->hunter_config.prototype_id = row[11] ? atoi(row[11]) : 0;
      definition->hunter_config.pilot_mob_vnum = row[12] ? atoi(row[12]) : 0;
      definition->hunter_config.min_bounty = row[13] ? atoi(row[13]) : 0;
      definition->hunter_config.pursuit_speed = row[14] ? atoi(row[14]) : 0;
      definition->hunter_config.hunt_duration_seconds = row[15] ? atoi(row[15]) : 0;
      definition->hunter_config.target_grace_seconds = row[16] ? atoi(row[16]) : 0;
      definition->hunter_config.cooldown_seconds = row[17] ? atoi(row[17]) : 0;
      definition->hunter_config.enabled = row[18] ? atoi(row[18]) != 0 : FALSE;
      if (!vessel_hunter_config_is_valid(&definition->hunter_config))
      {
        log("SYSERR: Bounty-hunter encounter %d has invalid policy values",
            definition->encounter_id);
        definition->hunter_configured = -1;
      }
    }
  }
  mysql_free_result(result);

  vessel_encounter_definition_count = loaded_count;
  vessel_encounter_config_loaded = TRUE;
  log("Loaded %d vessel encounter definition%s into memory.", loaded_count,
      loaded_count == 1 ? "" : "s");
  return TRUE;
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
  else if (weather >= VESSEL_WEATHER_CLOUDY)
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

  return vessel_weather_severity_from_value(weather);
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

static double vessel_encounter_distance_squared_to_segment(double x, double y, double start_x,
                                                           double start_y, double end_x,
                                                           double end_y)
{
  double segment_x;
  double segment_y;
  double point_x;
  double point_y;
  double segment_squared;
  double projection;

  segment_x = end_x - start_x;
  segment_y = end_y - start_y;
  point_x = x - start_x;
  point_y = y - start_y;
  segment_squared = segment_x * segment_x + segment_y * segment_y;
  projection =
      segment_squared > 0.0 ? (point_x * segment_x + point_y * segment_y) / segment_squared : 0.0;
  projection = MAX(0.0, MIN(1.0, projection));
  point_x = x - (start_x + projection * segment_x);
  point_y = y - (start_y + projection * segment_y);
  return point_x * point_x + point_y * point_y;
}

/**
 * Classify one strictly enclosed point using the same center/inside/edge
 * distance rule as get_enclosing_regions().
 */
static int vessel_encounter_polygon_position(const struct region_data *region, int x, int y)
{
  double area_twice;
  double centroid_x_numerator;
  double centroid_y_numerator;
  double centroid_x;
  double centroid_y;
  double center_x;
  double center_y;
  double center_distance_squared;
  double edge_distance_squared;
  double segment_distance_squared;
  double cross_product;
  int current;
  int next;

  if (region == NULL || region->vertices == NULL || region->num_vertices < 3 ||
      !vessel_piracy_point_in_polygon(region->vertices, region->num_vertices, x, y))
  {
    return REGION_POS_UNDEFINED;
  }

  area_twice = 0.0;
  centroid_x_numerator = 0.0;
  centroid_y_numerator = 0.0;
  edge_distance_squared = DBL_MAX;
  for (current = 0; current < region->num_vertices; current++)
  {
    next = (current + 1) % region->num_vertices;
    cross_product = (double)region->vertices[current].x * region->vertices[next].y -
                    (double)region->vertices[next].x * region->vertices[current].y;
    area_twice += cross_product;
    centroid_x_numerator +=
        (region->vertices[current].x + region->vertices[next].x) * cross_product;
    centroid_y_numerator +=
        (region->vertices[current].y + region->vertices[next].y) * cross_product;
    segment_distance_squared = vessel_encounter_distance_squared_to_segment(
        x, y, region->vertices[current].x, region->vertices[current].y, region->vertices[next].x,
        region->vertices[next].y);
    edge_distance_squared = MIN(edge_distance_squared, segment_distance_squared);
  }

  if (area_twice > -0.000001 && area_twice < 0.000001)
  {
    return REGION_POS_EDGE;
  }
  centroid_x = centroid_x_numerator / (3.0 * area_twice);
  centroid_y = centroid_y_numerator / (3.0 * area_twice);
  center_x = x - centroid_x;
  center_y = y - centroid_y;
  center_distance_squared = center_x * center_x + center_y * center_y;
  if (center_distance_squared < 0.000001)
  {
    return REGION_POS_CENTER;
  }
  if (4.0 * edge_distance_squared > center_distance_squared)
  {
    return REGION_POS_INSIDE;
  }
  return REGION_POS_EDGE;
}

/**
 * Resolve an encounter polygon entirely from the canonical in-memory region
 * table. This removes synchronous spatial SQL from the vessel heartbeat.
 */
bool vessel_encounter_region_at_coordinates(int x, int y, int *output_region_vnum)
{
  const struct region_data *region;
  int best_position;
  region_vnum best_vnum;
  int position;
  int position_rank;
  int best_rank;
  region_rnum i;

  if (output_region_vnum == NULL)
  {
    return FALSE;
  }
  *output_region_vnum = 0;
  if (region_table == NULL || zone_table == NULL || top_of_region_table == NOWHERE)
  {
    return FALSE;
  }

  best_position = REGION_POS_UNDEFINED;
  best_vnum = 0;
  best_rank = -1;
  for (i = 0; i <= top_of_region_table; i++)
  {
    region = &region_table[i];
    if (region->region_type != REGION_ENCOUNTER || region->zone == NOWHERE ||
        region->zone > top_of_zone_table || zone_table[region->zone].number != WILD_ZONE_VNUM)
    {
      continue;
    }

    position = vessel_encounter_polygon_position(region, x, y);
    switch (position)
    {
    case REGION_POS_CENTER:
      position_rank = 3;
      break;
    case REGION_POS_INSIDE:
      position_rank = 2;
      break;
    case REGION_POS_EDGE:
      position_rank = 1;
      break;
    default:
      continue;
    }

    if (best_rank < 0 || position_rank > best_rank ||
        (position_rank == best_rank && region->vnum < best_vnum))
    {
      best_position = position;
      best_rank = position_rank;
      best_vnum = region->vnum;
    }
  }

  if (best_position == REGION_POS_UNDEFINED)
  {
    return FALSE;
  }
  *output_region_vnum = (int)best_vnum;
  return TRUE;
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
 * Apply the database encounter candidate filters in memory.
 */
bool vessel_encounter_candidate_matches(int candidate_region_vnum, int candidate_vessel_class,
                                        int min_depth, int max_depth, int ship_region_vnum,
                                        enum vessel_class ship_class, int depth_units)
{
  if (candidate_region_vnum != ship_region_vnum ||
      (candidate_vessel_class != -1 && candidate_vessel_class != (int)ship_class))
  {
    return FALSE;
  }

  return max_depth == 0 || (depth_units >= min_depth && depth_units <= max_depth);
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

/**
 * Find a shared exterior room in the current encounter pass cache.
 *
 * Ships in the same exterior room necessarily share coordinates, so one
 * in-memory polygon lookup per room is sufficient.
 */
int vessel_encounter_cached_room_index(room_rnum room, const room_rnum *cached_rooms,
                                       int cached_count)
{
  int i;

  if (room == NOWHERE || cached_rooms == NULL || cached_count <= 0)
  {
    return -1;
  }

  for (i = 0; i < cached_count; i++)
  {
    if (cached_rooms[i] == room)
    {
      return i;
    }
  }

  return -1;
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
  if (ship == NULL || region_vnum == NULL)
  {
    return FALSE;
  }

  return vessel_encounter_region_at_coordinates((int)ship->x, (int)ship->y, region_vnum);
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

  vessel_narrative_tick();
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
        send_to_ship_throttled(ship, VESSEL_MESSAGE_AMBIENT_DEPTH, VESSEL_AMBIENT_MESSAGE_COOLDOWN,
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
      send_to_ship_throttled(ship, VESSEL_MESSAGE_AMBIENT_SQUALL, VESSEL_AMBIENT_MESSAGE_COOLDOWN,
                             "A squall slaps spray across the deck.");
      break;
    case 2:
      send_to_ship_throttled(ship, VESSEL_MESSAGE_AMBIENT_STORM, VESSEL_AMBIENT_MESSAGE_COOLDOWN,
                             "The storm tears at the rigging!");
      if (ship->mainsail > 1)
      {
        ship->mainsail--;
      }
      break;
    case 3:
    default:
      send_to_ship_throttled(ship, VESSEL_MESSAGE_AMBIENT_GALE, VESSEL_AMBIENT_MESSAGE_COOLDOWN,
                             "A GALE hammers the ship - the masts scream under the strain!");
      if (ship->mainsail > 2)
      {
        ship->mainsail -= 2;
      }
      /* A gale costs structure only when neither a sailmaster nor the
       * vessel's assigned pilot is physically at the helm. */
      if (ship->crew_tier[CREW_SAILMASTER] == CREW_TIER_NONE && get_pilot_from_ship(ship) == NULL)
      {
        vessel_apply_damage(i, dice(1, 6), GREYHAWK_PORT, "The gale");
      }
      break;
    }

    VSSL_DEBUG("Ship %d weather hazard: severity %d sail %d", i, severity, ship->mainsail);
  }
}

static int vessel_broadcast_encounter(room_rnum ship_room, struct greyhawk_ship_data *source,
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
  const struct vessel_encounter_definition *definition;
  struct greyhawk_ship_data *ship;
  struct char_data *mob;
  room_rnum claimed_rooms[GREYHAWK_MAXSHIPS];
  room_rnum region_rooms[GREYHAWK_MAXSHIPS];
  bool region_found[GREYHAWK_MAXSHIPS];
  int region_vnums[GREYHAWK_MAXSHIPS];
  room_rnum ship_room;
  int claimed_count = 0;
  int region_count = 0;
  int region_index;
  int region_vnum = 0;
  int depth_units;
  int definition_index;
  int hunter_configured;
  int recipient_count;
  int i;
  bool in_region;

  encounter_ticks++;
  if (encounter_ticks < VESSEL_ENCOUNTER_INTERVAL)
  {
    return;
  }
  encounter_ticks = 0;

  if (!vessel_encounter_config_loaded)
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

    ship_room = ship->shipobj != NULL ? IN_ROOM(ship->shipobj) : NOWHERE;
    if (vessel_encounter_room_is_claimed(ship_room, claimed_rooms, claimed_count))
    {
      continue;
    }

    region_index = vessel_encounter_cached_room_index(ship_room, region_rooms, region_count);
    if (region_index >= 0)
    {
      in_region = region_found[region_index];
      region_vnum = region_vnums[region_index];
    }
    else
    {
      region_vnum = 0;
      in_region = vessel_in_encounter_region(ship, &region_vnum);
      if (ship_room != NOWHERE && region_count < GREYHAWK_MAXSHIPS)
      {
        region_rooms[region_count] = ship_room;
        region_found[region_count] = in_region;
        region_vnums[region_count] = region_vnum;
        region_count++;
      }
    }

    if (!in_region)
    {
      continue;
    }

    depth_units = wild_waterline - get_modified_elevation((int)ship->x, (int)ship->y);

    /* Definitions retain the database chance/ID order loaded at boot. */
    for (definition_index = 0; definition_index < vessel_encounter_definition_count;
         definition_index++)
    {
      definition = &vessel_encounter_definitions[definition_index];
      if (!vessel_encounter_candidate_matches(definition->region_vnum, definition->vessel_class,
                                              definition->min_depth, definition->max_depth,
                                              region_vnum, ship->vessel_type, depth_units))
      {
        continue;
      }

      hunter_configured = definition->hunter_configured;
      if (hunter_configured < 0)
      {
        continue;
      }
      if (hunter_configured > 0 &&
          !vessel_hunter_target_is_eligible(ship, &definition->hunter_config, time(0)))
      {
        continue;
      }

      /* A good lookout gives warning before the thing arrives */
      if (!vessel_encounter_chance_succeeds(definition->chance, rand_number(1, 100)))
      {
        continue;
      }

      if (ship_room != NOWHERE &&
          !vessel_encounter_claim_room(ship_room, claimed_rooms, &claimed_count, GREYHAWK_MAXSHIPS))
      {
        break;
      }

      if (hunter_configured > 0 &&
          !vessel_hunter_spawn(ship, &definition->hunter_config, definition->name))
      {
        if (ship_room != NOWHERE && claimed_count > 0)
        {
          claimed_count--;
        }
        continue;
      }

      recipient_count = vessel_broadcast_encounter(ship_room, ship, definition->warn_message,
                                                   definition->arrive_message, definition->name);
      log("Info: Shared encounter '%s' in room %d from ship %d notified %d "
          "vessels in region %d",
          definition->name[0] ? definition->name : "?", ship_room, i, recipient_count, region_vnum);

      /* Spawn the encounter's creature into the ship's wilderness room so
       * it can be fought, fled, or fired upon like anything else. */
      if (hunter_configured == 0 && definition->mob_vnum > 0 && ship_room != NOWHERE)
      {
        mob = read_mobile_reason(definition->mob_vnum, VIRTUAL, PERF_ENTITY_VESSEL);
        if (mob != NULL)
        {
          char_to_room(mob, ship_room);
          act("$n rises from the depths!", FALSE, mob, 0, 0, TO_ROOM);
          log("Info: Encounter '%s' spawned for shared room %d from ship %d in region %d",
              definition->name[0] ? definition->name : "?", ship_room, i, region_vnum);
        }
      }

      break; /* One encounter per check */
    }
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
  if (!vessel_encounter_reload_config())
  {
    return;
  }
  encounter_ticks = VESSEL_ENCOUNTER_INTERVAL - 1;
  vessel_encounter_tick();
}

/**
 * seastate - report the weather, sea state, and visibility around the ship.
 */
ACMD(do_seastate)
{
  struct greyhawk_ship_data *ship;
  struct vessel_region_feature feature;
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

  if (vessel_region_feature_at_coordinates(REGION_BATHYMETRIC, (int)ship->x, (int)ship->y,
                                           (int)ship->z, &feature))
  {
    send_to_char(ch, "  Trench    : %s (natural depth %d; threshold %d)\r\n", feature.name,
                 depth_units, feature.threshold);
  }
  if (vessel_region_feature_at_coordinates(REGION_ALTITUDE_LANE, (int)ship->x, (int)ship->y,
                                           (int)ship->z, &feature))
  {
    send_to_char(ch, "  Sky lane  : %s (active above %d)\r\n", feature.name, feature.threshold);
  }
  if (vessel_region_feature_at_coordinates(REGION_SKY_ISLAND, (int)ship->x, (int)ship->y,
                                           (int)ship->z, &feature))
  {
    send_to_char(ch, "  Sky island: %s (reachable above %d)\r\n", feature.name, feature.threshold);
  }

  named_waters = vessel_piracy_law_for_ship(ship, &law);
  if (named_waters && law.configured)
  {
    send_to_char(ch, "  Waters    : %s (%s; %s; piracy bounty %d%%)\r\n", law.region_name,
                 vessel_waters_type_name(law.waters_type), law.authority, law.bounty_percent);
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
