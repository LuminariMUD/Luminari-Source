/* ************************************************************************
 *      File:   vessels_merchants.c                   Part of LuminariMUD  *
 *   Purpose:   Durable data-driven NPC merchant shipping (Phase 14).      *
 *              Definitions assemble real hulls, cargo, pilots, routes,    *
 *              schedules, faction consequences, and loss recovery.       *
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
#include "wilderness/wilderness.h"
#include "quest/missions.h"
#include "constants.h"

extern MYSQL *conn;
extern bool mysql_available;
extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];
extern struct char_data *character_list;

#define VESSEL_MERCHANT_RETRY_SECONDS 300
#define VESSEL_MERCHANT_NAME_LENGTH 128
#define VESSEL_MERCHANT_ERROR_LENGTH 256
#define VESSEL_MERCHANT_EVENT_LENGTH 16
#define VESSEL_MERCHANT_DEDUPE_LENGTH 192

struct vessel_merchant_profile
{
  int merchant_id;
  char name[VESSEL_MERCHANT_NAME_LENGTH];
  int faction_id;
  int prototype_id;
  int route_id;
  int pilot_mob_vnum;
  int spawn_x;
  int spawn_y;
  int spawn_z;
  int cargo_commodity_id;
  int cargo_quantity;
  int schedule_interval_hours;
  int respawn_delay_seconds;
  int active_ship_id;
  time_t next_respawn_at;
  unsigned int generation;
  bool enabled;
  char last_attacker_name[64];
  time_t last_attacked_at;
};

/**
 * Create the durable merchant definition and consequence tables.
 *
 * Definitions deliberately reference builder data by IDs without foreign
 * keys: builders may stage a profile before its prototype, route, pilot, or
 * commodity is published. Runtime validation keeps an incomplete row
 * disabled in practice and records the exact error for operators.
 */
void vessel_merchant_ensure_schema(void)
{
  const char *merchant_sql =
      "CREATE TABLE IF NOT EXISTS vessel_npc_merchants ("
      "merchant_id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,"
      "name VARCHAR(127) NOT NULL,"
      "faction_id TINYINT NOT NULL DEFAULT 0,"
      "prototype_id INT NOT NULL,"
      "route_id INT NOT NULL,"
      "pilot_mob_vnum INT NOT NULL,"
      "spawn_x INT NOT NULL,"
      "spawn_y INT NOT NULL,"
      "spawn_z INT NOT NULL DEFAULT 0,"
      "cargo_commodity_id INT NOT NULL,"
      "cargo_quantity INT NOT NULL,"
      "schedule_interval_hours INT NOT NULL DEFAULT 1,"
      "respawn_delay_seconds INT NOT NULL DEFAULT 3600,"
      "active_ship_id INT NULL,"
      "next_respawn_at BIGINT NOT NULL DEFAULT 0,"
      "generation INT UNSIGNED NOT NULL DEFAULT 0,"
      "last_spawn_at BIGINT NOT NULL DEFAULT 0,"
      "last_destroyed_at BIGINT NOT NULL DEFAULT 0,"
      "last_destroyed_by VARCHAR(63) NOT NULL DEFAULT '',"
      "last_attacker_name VARCHAR(63) NOT NULL DEFAULT '',"
      "last_attacked_at BIGINT NOT NULL DEFAULT 0,"
      "loss_count INT UNSIGNED NOT NULL DEFAULT 0,"
      "enabled TINYINT NOT NULL DEFAULT 1,"
      "last_error VARCHAR(255) NOT NULL DEFAULT '',"
      "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
      "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
      "UNIQUE KEY uk_vessel_merchant_name (name),"
      "UNIQUE KEY uk_vessel_merchant_active_ship (active_ship_id),"
      "INDEX idx_vessel_merchant_last_attacker (last_attacker_name),"
      "INDEX idx_vessel_merchant_due (enabled, next_respawn_at)"
      ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
  const char *consequence_sql =
      "CREATE TABLE IF NOT EXISTS vessel_merchant_consequences ("
      "consequence_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,"
      "merchant_id INT NOT NULL,"
      "generation INT UNSIGNED NOT NULL,"
      "player_name VARCHAR(63) NOT NULL,"
      "faction_id TINYINT NOT NULL DEFAULT 0,"
      "standing_penalty INT NOT NULL DEFAULT 0,"
      "bounty_delta INT NOT NULL DEFAULT 0,"
      "cargo_units INT NOT NULL DEFAULT 0,"
      "event_type VARCHAR(15) NOT NULL,"
      "dedupe_key VARCHAR(191) NULL,"
      "status VARCHAR(16) NOT NULL DEFAULT 'pending',"
      "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
      "applied_at TIMESTAMP NULL DEFAULT NULL,"
      "UNIQUE KEY uk_vessel_merchant_consequence_dedupe (dedupe_key),"
      "INDEX idx_vessel_merchant_consequence_player (player_name, status),"
      "INDEX idx_vessel_merchant_consequence_merchant (merchant_id, generation)"
      ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";

  if (!mysql_available || conn == NULL)
  {
    return;
  }

  if (mysql_query(conn, merchant_sql))
  {
    log("SYSERR: Could not create vessel_npc_merchants: %s", mysql_error(conn));
    return;
  }
  if (mysql_query(conn, consequence_sql))
  {
    log("SYSERR: Could not create vessel_merchant_consequences: %s", mysql_error(conn));
  }
}

/**
 * Is this definition eligible to own a newly spawned hull now?
 */
bool vessel_merchant_should_spawn(bool enabled, int active_ship_id, time_t next_respawn_at,
                                  time_t now)
{
  return enabled && active_ship_id <= 0 && next_respawn_at <= now;
}

/**
 * Is the last hostile act recent enough to assign a later total loss?
 */
bool vessel_merchant_responsibility_active(time_t attacked_at, time_t now)
{
  return attacked_at > 0 && attacked_at <= now &&
         now - attacked_at <= VESSEL_MERCHANT_RESPONSIBILITY_SECONDS;
}

/**
 * Calculate standing lost for cargo theft or a total merchant loss.
 */
int vessel_merchant_faction_penalty(int cargo_units, bool total_loss)
{
  long long penalty;

  if (cargo_units < 0)
  {
    cargo_units = 0;
  }
  penalty = cargo_units;
  if (total_loss)
  {
    penalty += VESSEL_MERCHANT_LOSS_STANDING_PENALTY;
  }
  if (penalty > INT_MAX)
  {
    return INT_MAX;
  }
  return (int)penalty;
}

static bool vessel_merchant_profile_values_are_valid(const struct vessel_merchant_profile *profile)
{
  if (profile == NULL || profile->merchant_id <= 0 || profile->name[0] == '\0')
  {
    return FALSE;
  }
  if (profile->faction_id < FACTION_NONE || profile->faction_id >= NUM_FACTIONS)
  {
    return FALSE;
  }
  if (profile->prototype_id <= 0 || profile->route_id <= 0 || profile->pilot_mob_vnum <= 0 ||
      profile->cargo_commodity_id <= 0 || profile->cargo_quantity <= 0)
  {
    return FALSE;
  }
  if (profile->spawn_x < -1024 || profile->spawn_x > 1024 || profile->spawn_y < -1024 ||
      profile->spawn_y > 1024)
  {
    return FALSE;
  }
  if (profile->schedule_interval_hours < SCHEDULE_INTERVAL_MIN ||
      profile->schedule_interval_hours > SCHEDULE_INTERVAL_MAX)
  {
    return FALSE;
  }
  if (profile->respawn_delay_seconds < VESSEL_MERCHANT_RESPAWN_MIN ||
      profile->respawn_delay_seconds > VESSEL_MERCHANT_RESPAWN_MAX)
  {
    return FALSE;
  }
  return TRUE;
}

static bool vessel_merchant_profile_from_row(MYSQL_ROW row, struct vessel_merchant_profile *profile)
{
  if (row == NULL || profile == NULL)
  {
    return FALSE;
  }

  memset(profile, 0, sizeof(*profile));
  profile->merchant_id = row[0] ? atoi(row[0]) : 0;
  strlcpy(profile->name, row[1] ? row[1] : "", sizeof(profile->name));
  profile->faction_id = row[2] ? atoi(row[2]) : 0;
  profile->prototype_id = row[3] ? atoi(row[3]) : 0;
  profile->route_id = row[4] ? atoi(row[4]) : 0;
  profile->pilot_mob_vnum = row[5] ? atoi(row[5]) : 0;
  profile->spawn_x = row[6] ? atoi(row[6]) : 0;
  profile->spawn_y = row[7] ? atoi(row[7]) : 0;
  profile->spawn_z = row[8] ? atoi(row[8]) : 0;
  profile->cargo_commodity_id = row[9] ? atoi(row[9]) : 0;
  profile->cargo_quantity = row[10] ? atoi(row[10]) : 0;
  profile->schedule_interval_hours = row[11] ? atoi(row[11]) : 0;
  profile->respawn_delay_seconds = row[12] ? atoi(row[12]) : 0;
  profile->active_ship_id = row[13] ? atoi(row[13]) : 0;
  profile->next_respawn_at = row[14] ? (time_t)strtoll(row[14], NULL, 10) : 0;
  profile->generation = row[15] ? (unsigned int)strtoul(row[15], NULL, 10) : 0;
  profile->enabled = row[16] ? atoi(row[16]) != 0 : FALSE;
  strlcpy(profile->last_attacker_name, row[17] ? row[17] : "", sizeof(profile->last_attacker_name));
  profile->last_attacked_at = row[18] ? (time_t)strtoll(row[18], NULL, 10) : 0;
  return vessel_merchant_profile_values_are_valid(profile);
}

static const char *vessel_merchant_profile_query(void)
{
  return "SELECT merchant_id, name, faction_id, prototype_id, route_id, "
         "pilot_mob_vnum, spawn_x, spawn_y, spawn_z, cargo_commodity_id, "
         "cargo_quantity, schedule_interval_hours, respawn_delay_seconds, "
         "active_ship_id, next_respawn_at, generation, enabled, "
         "last_attacker_name, last_attacked_at FROM vessel_npc_merchants";
}

static bool vessel_merchant_fetch_profile(int merchant_id, struct vessel_merchant_profile *profile)
{
  char query[MAX_STRING_LENGTH];
  MYSQL_RES *result;
  MYSQL_ROW row;
  bool valid;

  if (!mysql_available || conn == NULL || merchant_id <= 0 || profile == NULL)
  {
    return FALSE;
  }

  snprintf(query, sizeof(query), "%s WHERE merchant_id = %d", vessel_merchant_profile_query(),
           merchant_id);
  if (mysql_query(conn, query))
  {
    log("SYSERR: Could not load NPC merchant %d: %s", merchant_id, mysql_error(conn));
    return FALSE;
  }
  result = mysql_store_result(conn);
  if (result == NULL)
  {
    return FALSE;
  }
  row = mysql_fetch_row(result);
  valid = vessel_merchant_profile_from_row(row, profile);
  mysql_free_result(result);
  return valid;
}

static void vessel_merchant_set_error(int merchant_id, const char *message, time_t retry_at)
{
  char escaped[VESSEL_MERCHANT_ERROR_LENGTH * 2 + 1];
  char query[MAX_STRING_LENGTH];
  const char *safe_message;
  size_t message_length;

  if (!mysql_available || conn == NULL || merchant_id <= 0)
  {
    return;
  }

  safe_message = message != NULL ? message : "unknown lifecycle error";
  message_length = strnlen(safe_message, VESSEL_MERCHANT_ERROR_LENGTH - 1);
  mysql_real_escape_string(conn, escaped, safe_message, message_length);
  snprintf(query, sizeof(query),
           "UPDATE vessel_npc_merchants SET last_error = '%s', "
           "next_respawn_at = %lld WHERE merchant_id = %d",
           escaped, (long long)retry_at, merchant_id);
  if (mysql_query(conn, query))
  {
    log("SYSERR: Could not record NPC merchant %d error: %s", merchant_id, mysql_error(conn));
  }
}

static struct ship_route *vessel_merchant_route(int route_id)
{
  struct route_node *node;
  struct waypoint_node *waypoint_node;
  struct ship_route *route;
  int i;

  for (node = route_list; node != NULL; node = node->next)
  {
    if (node->route_id == route_id)
    {
      break;
    }
  }
  if (node == NULL || !node->active || node->num_waypoints <= 0)
  {
    return NULL;
  }

  route = route_create(node->name);
  if (route == NULL)
  {
    return NULL;
  }
  route->route_id = node->route_id;
  route->loop = node->loop;
  route->active = node->active;
  for (i = 0; i < node->num_waypoints && i < MAX_WAYPOINTS_PER_ROUTE; i++)
  {
    waypoint_node = waypoint_cache_find(node->waypoint_ids[i]);
    if (waypoint_node != NULL)
    {
      route->waypoints[route->num_waypoints] = waypoint_node->data;
      route->num_waypoints++;
    }
  }
  if (route->num_waypoints <= 0)
  {
    route_destroy(route);
    return NULL;
  }
  return route;
}

static int vessel_merchant_commodity_weight(int commodity_id)
{
  char query[MAX_STRING_LENGTH];
  MYSQL_RES *result;
  MYSQL_ROW row;
  int weight;

  snprintf(query, sizeof(query),
           "SELECT unit_weight FROM trade_commodities "
           "WHERE commodity_id = %d",
           commodity_id);
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
  weight = row != NULL && row[0] != NULL ? atoi(row[0]) : 0;
  mysql_free_result(result);
  return MAX(0, weight);
}

static bool vessel_merchant_load_cargo(struct greyhawk_ship_data *ship,
                                       const struct vessel_merchant_profile *profile)
{
  char query[MAX_STRING_LENGTH];
  MYSQL_RES *result;
  MYSQL_ROW row;
  long long weight;
  int unit_weight;
  int capacity;
  bool saved;

  if (ship == NULL || profile == NULL)
  {
    return FALSE;
  }

  unit_weight = vessel_merchant_commodity_weight(profile->cargo_commodity_id);
  capacity = vessel_effective_cargo_capacity(ship);
  weight = (long long)unit_weight * profile->cargo_quantity;
  if (unit_weight <= 0 || weight <= 0 || weight > capacity)
  {
    return FALSE;
  }

  memset(ship->cargo, 0, sizeof(ship->cargo));
  ship->cargo[0].commodity_id = profile->cargo_commodity_id;
  ship->cargo[0].quantity = profile->cargo_quantity;
  ship->num_cargo_lots = 1;
  vessel_db_save_cargo(ship);

  snprintf(query, sizeof(query),
           "SELECT COUNT(*) FROM ship_cargo_manifest "
           "WHERE ship_id = %d AND cargo_room = 0 "
           "AND item_vnum = %d AND item_count = %d",
           ship->shipnum, profile->cargo_commodity_id, profile->cargo_quantity);
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
  saved = row != NULL && row[0] != NULL && atoi(row[0]) == 1;
  mysql_free_result(result);
  return saved;
}

static bool vessel_merchant_assign_pilot(struct greyhawk_ship_data *ship,
                                         const struct vessel_merchant_profile *profile)
{
  struct char_data *pilot;
  mob_rnum pilot_rnum;
  room_rnum bridge;

  if (ship == NULL || profile == NULL)
  {
    return FALSE;
  }

  pilot_rnum = real_mobile(profile->pilot_mob_vnum);
  bridge = real_room(ship->bridge_room);
  if (pilot_rnum == NOBODY || bridge == NOWHERE)
  {
    return FALSE;
  }

  pilot = read_mobile(pilot_rnum, REAL);
  if (pilot == NULL)
  {
    return FALSE;
  }
  char_to_room(pilot, bridge);

  if (ship->autopilot == NULL)
  {
    ship->autopilot = autopilot_init(ship);
  }
  if (ship->autopilot == NULL)
  {
    extract_char(pilot);
    return FALSE;
  }
  ship->autopilot->pilot_mob_vnum = profile->pilot_mob_vnum;
  if (!vessel_db_save_pilot(ship))
  {
    ship->autopilot->pilot_mob_vnum = -1;
    extract_char(pilot);
    return FALSE;
  }
  return TRUE;
}

static void vessel_merchant_abort_spawn(struct greyhawk_ship_data *ship)
{
  struct obj_data *hull;
  struct char_data *pilot;
  room_rnum exterior;

  if (ship == NULL || !is_valid_ship(ship))
  {
    return;
  }

  hull = ship->shipobj;
  exterior = hull != NULL ? IN_ROOM(hull) : NOWHERE;
  pilot = get_pilot_from_ship(ship);
  if (pilot != NULL)
  {
    extract_char(pilot);
  }

  vessel_abort_docking(ship);
  vehicle_release_all_from_vessel(ship, exterior);
  vessel_reclaim_interior_rooms(ship, exterior);
  autopilot_cleanup(ship);
  if (ship->schedule != NULL)
  {
    free(ship->schedule);
    ship->schedule = NULL;
  }
  if (hull != NULL)
  {
    ship->shipobj = NULL;
    extract_obj(hull);
  }
  if (!vessel_delete_persistence(ship->shipnum))
  {
    log("SYSERR: Could not roll back failed NPC merchant ship %d", ship->shipnum);
  }
  memset(ship, 0, sizeof(*ship));
}

static bool vessel_merchant_activate_profile(const struct vessel_merchant_profile *profile)
{
  char query[MAX_STRING_LENGTH];
  struct greyhawk_ship_data *ship;
  struct ship_route *route;
  time_t now;
  unsigned int generation;
  int slot;

  if (profile == NULL)
  {
    return FALSE;
  }
  if (profile->generation == UINT_MAX)
  {
    vessel_merchant_set_error(profile->merchant_id, "generation counter is exhausted",
                              time(0) + VESSEL_MERCHANT_RETRY_SECONDS);
    return FALSE;
  }

  route = vessel_merchant_route(profile->route_id);
  if (route == NULL)
  {
    vessel_merchant_set_error(profile->merchant_id, "route is missing, inactive, or empty",
                              time(0) + VESSEL_MERCHANT_RETRY_SECONDS);
    return FALSE;
  }
  route_destroy(route);

  if (real_mobile(profile->pilot_mob_vnum) == NOBODY)
  {
    vessel_merchant_set_error(profile->merchant_id, "pilot mobile prototype is unavailable",
                              time(0) + VESSEL_MERCHANT_RETRY_SECONDS);
    return FALSE;
  }

  slot = vessel_spawn_public_from_prototype_at(
      profile->prototype_id, profile->name, profile->spawn_x, profile->spawn_y, profile->spawn_z);
  if (slot < 0)
  {
    vessel_merchant_set_error(profile->merchant_id, "hull constructor rejected the profile",
                              time(0) + VESSEL_MERCHANT_RETRY_SECONDS);
    return FALSE;
  }

  ship = &greyhawk_ships[slot];
  if (!vessel_merchant_load_cargo(ship, profile) || !vessel_merchant_assign_pilot(ship, profile))
  {
    vessel_merchant_abort_spawn(ship);
    vessel_merchant_set_error(profile->merchant_id, "cargo or pilot assembly failed",
                              time(0) + VESSEL_MERCHANT_RETRY_SECONDS);
    return FALSE;
  }

  ship->speed = MAX(1, ship->maxspeed / 2);
  ship->setspeed = ship->speed;
  if (!schedule_create(ship, profile->route_id, profile->schedule_interval_hours, 0) ||
      !schedule_trigger_departure(ship))
  {
    vessel_merchant_abort_spawn(ship);
    vessel_merchant_set_error(profile->merchant_id, "schedule or initial departure failed",
                              time(0) + VESSEL_MERCHANT_RETRY_SECONDS);
    return FALSE;
  }

  now = time(0);
  generation = profile->generation + 1;
  snprintf(query, sizeof(query),
           "UPDATE vessel_npc_merchants SET active_ship_id = %d, "
           "generation = %u, last_spawn_at = %lld, next_respawn_at = 0, "
           "last_attacker_name = '', last_attacked_at = 0, last_error = '' "
           "WHERE merchant_id = %d AND enabled = 1 "
           "AND (active_ship_id IS NULL OR active_ship_id = 0)",
           slot, generation, (long long)now, profile->merchant_id);
  if (mysql_query(conn, query) || mysql_affected_rows(conn) != 1)
  {
    vessel_merchant_abort_spawn(ship);
    vessel_merchant_set_error(profile->merchant_id, "definition could not claim the spawned hull",
                              now + VESSEL_MERCHANT_RETRY_SECONDS);
    return FALSE;
  }

  ship->merchant_id = profile->merchant_id;
  ship->merchant_generation = generation;
  ship->merchant_faction_id = profile->faction_id;
  log("Info: NPC merchant %d '%s' generation %u entered service as "
      "ship %d on route %d with %d cargo units",
      profile->merchant_id, profile->name, generation, slot, profile->route_id,
      profile->cargo_quantity);
  return TRUE;
}

static void vessel_merchant_reconcile(void)
{
  char query[MAX_STRING_LENGTH];
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct vessel_merchant_profile profile;
  struct greyhawk_ship_data *ship;
  time_t now;
  time_t retry_at;

  if (!mysql_available || conn == NULL)
  {
    return;
  }

  if (mysql_query(conn, vessel_merchant_profile_query()))
  {
    log("SYSERR: Could not enumerate NPC merchants: %s", mysql_error(conn));
    return;
  }
  result = mysql_store_result(conn);
  if (result == NULL)
  {
    return;
  }

  now = time(0);
  while ((row = mysql_fetch_row(result)) != NULL)
  {
    if (!vessel_merchant_profile_from_row(row, &profile))
    {
      if (row[0] != NULL)
      {
        vessel_merchant_set_error(atoi(row[0]), "definition values are outside Phase 14 bounds",
                                  now + VESSEL_MERCHANT_RETRY_SECONDS);
      }
      continue;
    }

    if (profile.active_ship_id > 0)
    {
      ship = profile.active_ship_id < GREYHAWK_MAXSHIPS ? &greyhawk_ships[profile.active_ship_id]
                                                        : NULL;
      if (ship != NULL && is_valid_ship(ship) && ship->prototype_id == profile.prototype_id &&
          ship->owner[0] == '\0' && !str_cmp(ship->name, profile.name) && profile.generation > 0)
      {
        ship->merchant_id = profile.merchant_id;
        ship->merchant_generation = profile.generation;
        ship->merchant_faction_id = profile.faction_id;
        continue;
      }

      retry_at = profile.next_respawn_at > now ? profile.next_respawn_at
                                               : now + profile.respawn_delay_seconds;
      snprintf(query, sizeof(query),
               "UPDATE vessel_npc_merchants SET active_ship_id = NULL, "
               "next_respawn_at = %lld, "
               "last_error = 'active hull was missing or no longer public' "
               "WHERE merchant_id = %d AND active_ship_id = %d",
               (long long)retry_at, profile.merchant_id, profile.active_ship_id);
      if (mysql_query(conn, query))
      {
        log("SYSERR: Could not reconcile stale NPC merchant %d: %s", profile.merchant_id,
            mysql_error(conn));
      }
      continue;
    }

    if (vessel_merchant_should_spawn(profile.enabled, profile.active_ship_id,
                                     profile.next_respawn_at, now))
    {
      vessel_merchant_activate_profile(&profile);
    }
  }
  mysql_free_result(result);
}

/**
 * Attach persisted definitions to reconstructed hulls and start any due rows.
 */
void vessel_merchant_boot(void)
{
  vessel_merchant_ensure_schema();
  vessel_merchant_reconcile();
}

/**
 * Reconcile definitions from the MUD-hour schedule tick.
 */
void vessel_merchant_tick(void)
{
  vessel_merchant_reconcile();
}

static struct char_data *vessel_merchant_effective_player(struct char_data *ch)
{
  if (ch != NULL && IS_NPC(ch) && ch->master != NULL && !IS_NPC(ch->master))
  {
    return ch->master;
  }
  if (ch == NULL || IS_NPC(ch))
  {
    return NULL;
  }
  return ch;
}

static struct char_data *vessel_merchant_online_player(const char *name)
{
  struct char_data *tch;

  if (name == NULL || !*name)
  {
    return NULL;
  }
  for (tch = character_list; tch != NULL; tch = tch->next)
  {
    if (!IS_NPC(tch) && GET_NAME(tch) != NULL && !str_cmp(GET_NAME(tch), name))
    {
      return tch;
    }
  }
  return NULL;
}

static bool vessel_merchant_add_bounty_in_transaction(const char *player_name, int amount)
{
  char escaped_name[MAX_NAME_LENGTH * 2 + 1];
  char query[MAX_STRING_LENGTH];

  if (player_name == NULL || !*player_name || amount <= 0)
  {
    return TRUE;
  }

  mysql_real_escape_string(conn, escaped_name, player_name, strlen(player_name));
  snprintf(query, sizeof(query),
           "INSERT INTO vessel_bounties (player_name, bounty) "
           "VALUES ('%s', %d) ON DUPLICATE KEY UPDATE "
           "bounty = LEAST(%d, CAST(bounty AS DECIMAL(20,0)) + %d)",
           escaped_name, amount, INT_MAX, amount);
  if (mysql_query(conn, query))
  {
    log("SYSERR: Could not add durable merchant bounty for %s: %s", player_name, mysql_error(conn));
    return FALSE;
  }
  return TRUE;
}

static bool vessel_merchant_queue_consequence(const struct vessel_merchant_profile *profile,
                                              const char *player_name, const char *event_type,
                                              int cargo_units, int bounty_delta,
                                              int standing_penalty, bool bounty_already_applied,
                                              const char *dedupe_key)
{
  char escaped_player[MAX_NAME_LENGTH * 2 + 1];
  char escaped_event[VESSEL_MERCHANT_EVENT_LENGTH * 2 + 1];
  char escaped_key[(VESSEL_MERCHANT_DEDUPE_LENGTH - 1) * 2 + 1];
  char query[MAX_STRING_LENGTH];
  struct char_data *player;
  unsigned long long consequence_id;
  const char *status;

  if (!mysql_available || conn == NULL || profile == NULL || player_name == NULL || !*player_name ||
      event_type == NULL || !*event_type)
  {
    return FALSE;
  }
  if (strlen(event_type) >= VESSEL_MERCHANT_EVENT_LENGTH ||
      (dedupe_key != NULL && strlen(dedupe_key) >= VESSEL_MERCHANT_DEDUPE_LENGTH))
  {
    log("SYSERR: NPC merchant consequence key exceeds schema bounds");
    return FALSE;
  }

  vessel_piracy_ensure_schema();
  vessel_merchant_ensure_schema();
  mysql_real_escape_string(conn, escaped_player, player_name, strlen(player_name));
  mysql_real_escape_string(conn, escaped_event, event_type, strlen(event_type));
  if (dedupe_key != NULL && *dedupe_key)
  {
    mysql_real_escape_string(conn, escaped_key, dedupe_key, strlen(dedupe_key));
  }
  else
  {
    escaped_key[0] = '\0';
  }
  status = standing_penalty > 0 ? "pending" : "applied";

  if (mysql_query(conn, "START TRANSACTION"))
  {
    return FALSE;
  }
  if (escaped_key[0] != '\0')
  {
    snprintf(query, sizeof(query),
             "INSERT IGNORE INTO vessel_merchant_consequences "
             "(merchant_id, generation, player_name, faction_id, "
             "standing_penalty, bounty_delta, cargo_units, event_type, "
             "dedupe_key, status, applied_at) "
             "VALUES (%d, %u, '%s', %d, %d, %d, %d, '%s', '%s', "
             "'%s', %s)",
             profile->merchant_id, profile->generation, escaped_player, profile->faction_id,
             MAX(0, standing_penalty), MAX(0, bounty_delta), MAX(0, cargo_units), escaped_event,
             escaped_key, status, standing_penalty > 0 ? "NULL" : "NOW()");
  }
  else
  {
    snprintf(query, sizeof(query),
             "INSERT INTO vessel_merchant_consequences "
             "(merchant_id, generation, player_name, faction_id, "
             "standing_penalty, bounty_delta, cargo_units, event_type, "
             "dedupe_key, status, applied_at) "
             "VALUES (%d, %u, '%s', %d, %d, %d, %d, '%s', NULL, "
             "'%s', %s)",
             profile->merchant_id, profile->generation, escaped_player, profile->faction_id,
             MAX(0, standing_penalty), MAX(0, bounty_delta), MAX(0, cargo_units), escaped_event,
             status, standing_penalty > 0 ? "NULL" : "NOW()");
  }

  if (mysql_query(conn, query))
  {
    log("SYSERR: Could not queue merchant consequence for %s: %s", player_name, mysql_error(conn));
    mysql_query(conn, "ROLLBACK");
    return FALSE;
  }
  if (mysql_affected_rows(conn) == 0)
  {
    mysql_query(conn, "COMMIT");
    return TRUE; /* An idempotent attack/loss record already exists. */
  }

  consequence_id = mysql_insert_id(conn);
  if (!bounty_already_applied &&
      !vessel_merchant_add_bounty_in_transaction(player_name, bounty_delta))
  {
    mysql_query(conn, "ROLLBACK");
    return FALSE;
  }
  if (mysql_query(conn, "COMMIT"))
  {
    log("SYSERR: Could not commit merchant consequence %llu for %s: %s", consequence_id,
        player_name, mysql_error(conn));
    mysql_query(conn, "ROLLBACK");
    return FALSE;
  }

  player = vessel_merchant_online_player(player_name);
  if (player != NULL && standing_penalty > 0)
  {
    vessel_merchant_deliver_pending_consequences(player);
  }
  if (player != NULL && bounty_delta > 0 && !bounty_already_applied)
  {
    send_to_char(player,
                 "The admiralty records %d gold against you for the loss "
                 "of %s.\r\n",
                 bounty_delta, profile->name);
  }
  return TRUE;
}

/**
 * Apply pending faction losses exactly once to a loaded player.
 *
 * The saved consequence high-water mark is advanced before rows are closed.
 * A stop between player save and SQL update therefore closes the same rows
 * on the next login without deducting standing twice.
 */
int vessel_merchant_deliver_pending_consequences(struct char_data *ch)
{
  char escaped_player[MAX_NAME_LENGTH * 2 + 1];
  char query[MAX_STRING_LENGTH];
  MYSQL_RES *result;
  MYSQL_ROW row;
  unsigned long long consequence_id;
  unsigned long long previous_id;
  unsigned long long highest_id;
  long long penalty_by_faction[NUM_FACTIONS];
  long old_standing[NUM_FACTIONS];
  long long updated;
  int faction_id;
  int pending;
  int applied;
  int i;

  if (ch == NULL || IS_NPC(ch) || GET_NAME(ch) == NULL || !mysql_available || conn == NULL)
  {
    return 0;
  }

  vessel_merchant_ensure_schema();
  mysql_real_escape_string(conn, escaped_player, GET_NAME(ch), strlen(GET_NAME(ch)));
  snprintf(query, sizeof(query),
           "SELECT consequence_id, faction_id, standing_penalty "
           "FROM vessel_merchant_consequences "
           "WHERE player_name = '%s' AND status = 'pending' "
           "ORDER BY consequence_id",
           escaped_player);
  if (mysql_query(conn, query))
  {
    log("SYSERR: Could not load merchant consequences for %s: %s", GET_NAME(ch), mysql_error(conn));
    return 0;
  }
  result = mysql_store_result(conn);
  if (result == NULL)
  {
    return 0;
  }

  memset(penalty_by_faction, 0, sizeof(penalty_by_faction));
  previous_id = GET_VESSEL_MERCHANT_CONSEQUENCE(ch);
  highest_id = previous_id;
  pending = 0;
  applied = 0;
  while ((row = mysql_fetch_row(result)) != NULL)
  {
    pending++;
    consequence_id = row[0] ? strtoull(row[0], NULL, 10) : 0;
    faction_id = row[1] ? atoi(row[1]) : FACTION_NONE;
    if (consequence_id > highest_id)
    {
      highest_id = consequence_id;
    }
    if (consequence_id <= previous_id || faction_id <= FACTION_NONE || faction_id >= NUM_FACTIONS ||
        row[2] == NULL)
    {
      continue;
    }
    if (penalty_by_faction[faction_id] > LLONG_MAX - MAX(0, atoi(row[2])))
    {
      penalty_by_faction[faction_id] = LLONG_MAX;
    }
    else
    {
      penalty_by_faction[faction_id] += MAX(0, atoi(row[2]));
    }
    applied++;
  }
  mysql_free_result(result);

  if (pending == 0)
  {
    return 0;
  }

  for (i = 0; i < NUM_FACTIONS; i++)
  {
    old_standing[i] = GET_FACTION_STANDING(ch, i);
    if (penalty_by_faction[i] > 0 && old_standing[i] < LONG_MIN + penalty_by_faction[i])
    {
      GET_FACTION_STANDING(ch, i) = LONG_MIN;
    }
    else
    {
      updated = (long long)old_standing[i] - penalty_by_faction[i];
      GET_FACTION_STANDING(ch, i) = (long)updated;
    }
  }
  GET_VESSEL_MERCHANT_CONSEQUENCE(ch) = highest_id;
  if (applied > 0 && !save_char_checked(ch, 0))
  {
    for (i = 0; i < NUM_FACTIONS; i++)
    {
      GET_FACTION_STANDING(ch, i) = old_standing[i];
    }
    GET_VESSEL_MERCHANT_CONSEQUENCE(ch) = previous_id;
    log("SYSERR: Could not save merchant consequences for %s", GET_NAME(ch));
    return 0;
  }

  snprintf(query, sizeof(query),
           "UPDATE vessel_merchant_consequences "
           "SET status = 'applied', applied_at = NOW() "
           "WHERE player_name = '%s' AND status = 'pending' "
           "AND consequence_id <= %llu",
           escaped_player, highest_id);
  if (mysql_query(conn, query))
  {
    log("SYSERR: Could not close merchant consequences for %s: %s", GET_NAME(ch),
        mysql_error(conn));
  }

  if (applied > 0)
  {
    send_to_char(ch,
                 "News of your attack on merchant shipping costs faction "
                 "standing in %d recorded incident%s.\r\n",
                 applied, applied == 1 ? "" : "s");
  }
  return applied;
}

/**
 * Record the responsible player before a merchant takes hostile damage.
 *
 * One attack consequence per player and merchant generation prevents a
 * reload timer from turning every projectile into another standing loss.
 */
void vessel_merchant_note_attacker(struct char_data *ch, struct greyhawk_ship_data *ship)
{
  char escaped_player[MAX_NAME_LENGTH * 2 + 1];
  char query[MAX_STRING_LENGTH];
  char dedupe_key[256];
  struct vessel_merchant_profile profile;
  struct char_data *player;

  player = vessel_merchant_effective_player(ch);
  if (player == NULL || ship == NULL || ship->merchant_id <= 0 || GET_NAME(player) == NULL ||
      !vessel_merchant_fetch_profile(ship->merchant_id, &profile) ||
      profile.active_ship_id != ship->shipnum || profile.generation != ship->merchant_generation)
  {
    return;
  }

  mysql_real_escape_string(conn, escaped_player, GET_NAME(player), strlen(GET_NAME(player)));
  snprintf(query, sizeof(query),
           "UPDATE vessel_npc_merchants SET last_attacker_name = '%s', "
           "last_attacked_at = %lld "
           "WHERE merchant_id = %d AND generation = %u "
           "AND active_ship_id = %d",
           escaped_player, (long long)time(0), profile.merchant_id, profile.generation,
           ship->shipnum);
  if (mysql_query(conn, query))
  {
    log("SYSERR: Could not record attacker for NPC merchant %d: %s", profile.merchant_id,
        mysql_error(conn));
  }

  snprintf(dedupe_key, sizeof(dedupe_key), "attack:%d:%u:%s", profile.merchant_id,
           profile.generation, GET_NAME(player));
  vessel_merchant_queue_consequence(
      &profile, GET_NAME(player), "attack", 0, 0,
      profile.faction_id == FACTION_NONE ? 0 : VESSEL_MERCHANT_ATTACK_STANDING_PENALTY, FALSE,
      dedupe_key);
}

/**
 * Add cargo-scaled faction loss after a successful plunder.
 *
 * The piracy module already adds the regional bounty; this durable row
 * records that exact delta beside the separate merchant-faction consequence.
 */
void vessel_merchant_record_plunder(struct char_data *ch, struct greyhawk_ship_data *ship,
                                    int cargo_units, int bounty_delta)
{
  struct vessel_merchant_profile profile;
  struct char_data *player;
  int penalty;

  player = vessel_merchant_effective_player(ch);
  if (player == NULL || ship == NULL || ship->merchant_id <= 0 || cargo_units <= 0 ||
      GET_NAME(player) == NULL || !vessel_merchant_fetch_profile(ship->merchant_id, &profile) ||
      profile.active_ship_id != ship->shipnum || profile.generation != ship->merchant_generation)
  {
    return;
  }

  vessel_merchant_note_attacker(player, ship);
  penalty =
      profile.faction_id == FACTION_NONE ? 0 : vessel_merchant_faction_penalty(cargo_units, FALSE);
  vessel_merchant_queue_consequence(&profile, GET_NAME(player), "plunder", cargo_units,
                                    MAX(0, bounty_delta), penalty, TRUE, NULL);

  log("Info: %s incurred %d merchant-faction standing and %d regional "
      "bounty for plundering %d units from merchant %d generation %u",
      GET_NAME(player), penalty, MAX(0, bounty_delta), cargo_units, profile.merchant_id,
      profile.generation);
}

static int vessel_merchant_cargo_units(const struct greyhawk_ship_data *ship)
{
  long long units;
  int i;

  if (ship == NULL)
  {
    return 0;
  }
  units = 0;
  for (i = 0; i < MAX_CARGO_LOTS; i++)
  {
    if (ship->cargo[i].commodity_id > 0 && ship->cargo[i].quantity > 0)
    {
      units += ship->cargo[i].quantity;
      if (units > INT_MAX)
      {
        return INT_MAX;
      }
    }
  }
  return (int)units;
}

static void vessel_merchant_loss(struct greyhawk_ship_data *ship, const char *event_type,
                                 const char *explicit_player, bool penalize)
{
  char escaped_player[MAX_NAME_LENGTH * 2 + 1];
  char escaped_event[VESSEL_MERCHANT_EVENT_LENGTH * 2 + 1];
  char query[MAX_STRING_LENGTH];
  char dedupe_key[256];
  struct vessel_merchant_profile profile;
  struct vessel_piracy_law law;
  const char *player_name;
  int cargo_units;
  int bounty_units;
  int bounty_delta;
  int standing_penalty;
  time_t now;

  if (ship == NULL || ship->merchant_id <= 0 ||
      !vessel_merchant_fetch_profile(ship->merchant_id, &profile) ||
      profile.active_ship_id != ship->shipnum || profile.generation != ship->merchant_generation)
  {
    return;
  }

  now = time(0);
  player_name = explicit_player != NULL && *explicit_player
                    ? explicit_player
                    : (vessel_merchant_responsibility_active(profile.last_attacked_at, now)
                           ? profile.last_attacker_name
                           : NULL);
  if ((player_name == NULL || !*player_name) &&
      vessel_merchant_responsibility_active(profile.last_attacked_at, now) &&
      ship->last_attacker > 0 && ship->last_attacker < GREYHAWK_MAXSHIPS &&
      is_valid_ship(&greyhawk_ships[ship->last_attacker]) &&
      greyhawk_ships[ship->last_attacker].owner[0] != '\0')
  {
    player_name = greyhawk_ships[ship->last_attacker].owner;
  }

  cargo_units = vessel_merchant_cargo_units(ship);
  bounty_delta = 0;
  standing_penalty = 0;
  if (penalize && player_name != NULL && *player_name)
  {
    vessel_piracy_law_for_ship(ship, &law);
    bounty_units = MAX(cargo_units, VESSEL_MERCHANT_LOSS_BOUNTY_UNITS);
    if (!vessel_has_letter_of_marque(player_name))
    {
      bounty_delta = vessel_piracy_bounty_for_units(bounty_units, law.bounty_percent);
    }
    if (profile.faction_id != FACTION_NONE)
    {
      standing_penalty = vessel_merchant_faction_penalty(cargo_units, TRUE);
    }
    snprintf(dedupe_key, sizeof(dedupe_key), "%s:%d:%u:%s", event_type, profile.merchant_id,
             profile.generation, player_name);
    vessel_merchant_queue_consequence(&profile, player_name, event_type, cargo_units, bounty_delta,
                                      standing_penalty, FALSE, dedupe_key);
  }

  if (player_name != NULL && *player_name)
  {
    mysql_real_escape_string(conn, escaped_player, player_name, strlen(player_name));
  }
  else
  {
    escaped_player[0] = '\0';
  }
  mysql_real_escape_string(conn, escaped_event, event_type, strlen(event_type));
  snprintf(query, sizeof(query),
           "UPDATE vessel_npc_merchants SET active_ship_id = NULL, "
           "next_respawn_at = %lld, last_destroyed_at = %lld, "
           "last_destroyed_by = '%s', last_attacker_name = '', "
           "last_attacked_at = 0, "
           "loss_count = loss_count + %d, last_error = '%s' "
           "WHERE merchant_id = %d AND generation = %u "
           "AND active_ship_id = %d",
           (long long)(now + profile.respawn_delay_seconds), (long long)now, escaped_player,
           penalize ? 1 : 0, escaped_event, profile.merchant_id, profile.generation, ship->shipnum);
  if (mysql_query(conn, query))
  {
    log("SYSERR: Could not release lost NPC merchant %d ship %d: %s", profile.merchant_id,
        ship->shipnum, mysql_error(conn));
  }
  else
  {
    log("Info: NPC merchant %d generation %u ship %d recorded %s; "
        "replacement due at %lld (actor %s, standing %d, bounty %d)",
        profile.merchant_id, profile.generation, ship->shipnum, event_type,
        (long long)(now + profile.respawn_delay_seconds),
        player_name != NULL && *player_name ? player_name : "none", standing_penalty, bounty_delta);
  }

  ship->merchant_id = 0;
  ship->merchant_generation = 0;
  ship->merchant_faction_id = FACTION_NONE;
}

/**
 * Mark a merchant hull lost before vessel_sink() removes its persistence.
 */
void vessel_merchant_handle_sink(struct greyhawk_ship_data *ship)
{
  vessel_merchant_loss(ship, "sink", NULL, TRUE);
}

/**
 * Capturing a merchant keeps the prize but releases its definition to spawn
 * a replacement after the same configured recovery delay.
 */
void vessel_merchant_handle_capture(struct char_data *ch, struct greyhawk_ship_data *ship)
{
  struct char_data *player;

  player = vessel_merchant_effective_player(ch);
  vessel_merchant_loss(ship, "capture",
                       player != NULL && GET_NAME(player) != NULL ? GET_NAME(player) : NULL, TRUE);
}

/**
 * Operator purge is a lifecycle restart, not piracy.
 */
void vessel_merchant_handle_purge(struct greyhawk_ship_data *ship, const char *staff_name)
{
  vessel_merchant_loss(ship, "purge", staff_name, FALSE);
}

static void vessel_merchant_list(struct char_data *ch)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  long long now;
  long long due;
  int active_ship_id;
  bool enabled;

  if (!mysql_available || conn == NULL)
  {
    send_to_char(ch, "The merchant registry is unavailable.\r\n");
    return;
  }
  vessel_merchant_ensure_schema();
  if (mysql_query(conn, "SELECT merchant_id, name, faction_id, prototype_id, route_id, "
                        "pilot_mob_vnum, cargo_commodity_id, cargo_quantity, "
                        "active_ship_id, next_respawn_at, generation, enabled, "
                        "loss_count, last_error FROM vessel_npc_merchants "
                        "ORDER BY merchant_id"))
  {
    send_to_char(ch, "The merchant registry could not be read.\r\n");
    return;
  }
  result = mysql_store_result(conn);
  if (result == NULL)
  {
    send_to_char(ch, "The merchant registry could not be read.\r\n");
    return;
  }

  now = (long long)time(0);
  send_to_char(ch, "ID  Gen Ship State       Faction    Proto Route Pilot "
                   "Cargo       Loss Name\r\n");
  send_to_char(ch, "--- --- ---- ----------- ---------- ----- ----- ----- "
                   "----------- ---- ------------------------\r\n");
  while ((row = mysql_fetch_row(result)) != NULL)
  {
    due = row[9] ? atoll(row[9]) : 0;
    active_ship_id = row[8] ? atoi(row[8]) : 0;
    enabled = row[11] && atoi(row[11]);
    send_to_char(ch, "%3d %3u %4s %-11s %-10.10s %5d %5d %5d %4d x %-4d %4d %s\r\n",
                 row[0] ? atoi(row[0]) : 0, row[10] ? (unsigned int)strtoul(row[10], NULL, 10) : 0,
                 active_ship_id > 0 ? row[8] : "-",
                 !enabled ? "disabled"
                          : (active_ship_id > 0 ? "active" : (due > now ? "recovering" : "due")),
                 row[2] && atoi(row[2]) >= 0 && atoi(row[2]) < NUM_FACTIONS ? factions[atoi(row[2])]
                                                                            : "invalid",
                 row[3] ? atoi(row[3]) : 0, row[4] ? atoi(row[4]) : 0, row[5] ? atoi(row[5]) : 0,
                 row[7] ? atoi(row[7]) : 0, row[6] ? atoi(row[6]) : 0, row[12] ? atoi(row[12]) : 0,
                 row[1] ? row[1] : "(unnamed)");
    if (row[13] != NULL && *row[13])
    {
      send_to_char(ch, "    error: %s\r\n", row[13]);
    }
  }
  mysql_free_result(result);
}

/**
 * vmerchant [list|sync|sink <id> confirm]
 *
 * `sink` is intentionally explicit and staff-only through the command table.
 * It exercises the same production sink, consequence, and respawn path as
 * combat while making a development acceptance transcript deterministic.
 */
ACMD(do_vmerchant)
{
  char action[MAX_INPUT_LENGTH];
  char id_arg[MAX_INPUT_LENGTH];
  char confirmation[MAX_INPUT_LENGTH];
  char *end;
  struct vessel_merchant_profile profile;
  struct greyhawk_ship_data *ship;
  long parsed_id;
  int merchant_id;

  three_arguments_u((char *)argument, action, id_arg, confirmation);
  if (!*action || !str_cmp(action, "list"))
  {
    vessel_merchant_list(ch);
    return;
  }
  if (!str_cmp(action, "sync"))
  {
    vessel_merchant_tick();
    send_to_char(ch, "NPC merchant definitions reconciled.\r\n");
    vessel_merchant_list(ch);
    return;
  }
  if (str_cmp(action, "sink"))
  {
    send_to_char(ch, "Usage: vmerchant [list|sync|sink <id> confirm]\r\n");
    return;
  }

  parsed_id = strtol(id_arg, &end, 10);
  if (!*id_arg || *end != '\0' || parsed_id <= 0 || parsed_id > INT_MAX ||
      str_cmp(confirmation, "confirm"))
  {
    send_to_char(ch, "This destroys the active merchant hull and cargo. "
                     "Use: vmerchant sink <id> confirm\r\n");
    return;
  }
  merchant_id = (int)parsed_id;
  if (!vessel_merchant_fetch_profile(merchant_id, &profile) || profile.active_ship_id <= 0 ||
      profile.active_ship_id >= GREYHAWK_MAXSHIPS)
  {
    send_to_char(ch, "Merchant %d has no active hull.\r\n", merchant_id);
    return;
  }
  ship = &greyhawk_ships[profile.active_ship_id];
  if (!is_valid_ship(ship) || ship->merchant_id != merchant_id)
  {
    send_to_char(ch, "Merchant %d's registry is stale; run 'vmerchant sync'.\r\n", merchant_id);
    return;
  }

  vessel_merchant_note_attacker(ch, ship);
  send_to_char(ch, "Forcing the documented loss path for merchant %d, ship %d.\r\n", merchant_id,
               ship->shipnum);
  vessel_sink(ship->shipnum);
}
