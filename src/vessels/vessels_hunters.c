/* ************************************************************************
 *      File:   vessels_hunters.c                     Part of LuminariMUD  *
 *   Purpose:   Durable bounty-hunter warship encounters (Phase 15).       *
 *              REGION_ENCOUNTER rows select a data-driven public warship; *
 *              one durable lifecycle follows an online HUNTED ship owner. *
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
#include "wilderness.h"
#include "constants.h"

extern MYSQL *conn;
extern bool mysql_available;
extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];
extern struct char_data *character_list;
extern struct room_data *world;

#define VESSEL_HUNTER_BOUNTY_CHECK_INTERVAL 20
#define VESSEL_HUNTER_RUNTIME_SAVE_INTERVAL 10
#define VESSEL_HUNTER_SPAWN_RETRY_SECONDS 60
#define VESSEL_HUNTER_STATUS_LENGTH 16
#define VESSEL_HUNTER_REASON_LENGTH 64

struct vessel_hunter_boot_row
{
  char target_player[64];
  char hunter_name[128];
  char status[VESSEL_HUNTER_STATUS_LENGTH];
  unsigned long long generation;
  int target_ship_id;
  int hunter_ship_id;
  time_t expires_at;
  struct vessel_hunter_config config;
};

/**
 * Create the encounter policy and durable lifecycle tables.
 *
 * The policy extends a normal vessel_encounters row without owning geography.
 * The lifecycle deliberately has no fleet-slot foreign key: public hull slots
 * are reusable, so reconciliation validates the durable name, prototype, and
 * target before reattaching a restored ship.
 */
void vessel_hunter_ensure_schema(void)
{
  const char *config_sql =
      "CREATE TABLE IF NOT EXISTS vessel_hunter_encounters ("
      "encounter_id INT NOT NULL PRIMARY KEY,"
      "prototype_id INT NOT NULL,"
      "pilot_mob_vnum INT NOT NULL,"
      "min_bounty INT NOT NULL DEFAULT 2000,"
      "pursuit_speed INT NOT NULL DEFAULT 5,"
      "hunt_duration_seconds INT NOT NULL DEFAULT 600,"
      "target_grace_seconds INT NOT NULL DEFAULT 60,"
      "cooldown_seconds INT NOT NULL DEFAULT 3600,"
      "enabled TINYINT NOT NULL DEFAULT 1,"
      "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
      "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
      "INDEX idx_vessel_hunter_enabled (enabled)"
      ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
  const char *hunt_sql =
      "CREATE TABLE IF NOT EXISTS vessel_bounty_hunts ("
      "target_player VARCHAR(63) NOT NULL PRIMARY KEY,"
      "encounter_id INT NOT NULL,"
      "target_ship_id INT NOT NULL DEFAULT 0,"
      "hunter_ship_id INT NULL,"
      "hunter_name VARCHAR(127) NOT NULL DEFAULT '',"
      "generation BIGINT UNSIGNED NOT NULL DEFAULT 0,"
      "status VARCHAR(15) NOT NULL DEFAULT 'cooldown',"
      "started_at BIGINT NOT NULL DEFAULT 0,"
      "expires_at BIGINT NOT NULL DEFAULT 0,"
      "next_eligible_at BIGINT NOT NULL DEFAULT 0,"
      "ended_at BIGINT NOT NULL DEFAULT 0,"
      "end_reason VARCHAR(63) NOT NULL DEFAULT '',"
      "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
      "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
      "UNIQUE KEY uk_vessel_bounty_hunter_ship (hunter_ship_id),"
      "INDEX idx_vessel_bounty_hunt_due (status, next_eligible_at),"
      "INDEX idx_vessel_bounty_hunt_encounter (encounter_id)"
      ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";

  if (!mysql_available || conn == NULL)
  {
    return;
  }

  if (mysql_query(conn, config_sql))
  {
    log("SYSERR: Could not create vessel_hunter_encounters: %s",
        mysql_error(conn));
  }
  if (mysql_query(conn, hunt_sql))
  {
    log("SYSERR: Could not create vessel_bounty_hunts: %s",
        mysql_error(conn));
  }
}

/**
 * Reject policy rows that could create runaway or non-warship workloads.
 */
bool vessel_hunter_config_is_valid(const struct vessel_hunter_config *config)
{
  if (config == NULL || config->encounter_id <= 0 ||
      config->prototype_id <= 0 || config->pilot_mob_vnum <= 0 ||
      config->min_bounty < BOUNTY_HUNTED ||
      config->pursuit_speed <= 0 ||
      config->pursuit_speed > VESSEL_HUNTER_PURSUIT_SPEED_MAX ||
      config->hunt_duration_seconds < VESSEL_HUNTER_DURATION_MIN ||
      config->hunt_duration_seconds > VESSEL_HUNTER_DURATION_MAX ||
      config->target_grace_seconds < 0 ||
      config->target_grace_seconds > VESSEL_HUNTER_GRACE_MAX ||
      config->cooldown_seconds < VESSEL_HUNTER_COOLDOWN_MIN ||
      config->cooldown_seconds > VESSEL_HUNTER_COOLDOWN_MAX)
  {
    return FALSE;
  }
  return TRUE;
}

/**
 * A stored target row is reusable only after a recognized cooldown expires.
 * Missing rows are handled separately as first-generation hunts.
 */
bool vessel_hunter_lifecycle_allows_spawn(const char *status,
                                          time_t next_eligible_at,
                                          time_t now)
{
  return status != NULL && !str_cmp(status, "cooldown") &&
         next_eligible_at <= now;
}

/**
 * Load the Phase 15 extension for one ordinary encounter row.
 *
 * @return 1 for a configured hunter, 0 for a generic encounter, -1 on error
 */
int vessel_hunter_load_config(int encounter_id,
                              struct vessel_hunter_config *config)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char query[MAX_STRING_LENGTH];

  if (!mysql_available || conn == NULL || encounter_id <= 0 ||
      config == NULL)
  {
    return -1;
  }

  memset(config, 0, sizeof(*config));
  snprintf(query, sizeof(query),
           "SELECT encounter_id, prototype_id, pilot_mob_vnum, min_bounty, "
           "pursuit_speed, hunt_duration_seconds, target_grace_seconds, "
           "cooldown_seconds, enabled FROM vessel_hunter_encounters "
           "WHERE encounter_id = %d",
           encounter_id);
  if (mysql_query(conn, query))
  {
    log("SYSERR: Bounty-hunter encounter %d query failed: %s", encounter_id,
        mysql_error(conn));
    return -1;
  }

  result = mysql_store_result(conn);
  if (result == NULL)
  {
    return -1;
  }
  row = mysql_fetch_row(result);
  if (row == NULL)
  {
    mysql_free_result(result);
    return 0;
  }

  config->encounter_id = row[0] ? atoi(row[0]) : 0;
  config->prototype_id = row[1] ? atoi(row[1]) : 0;
  config->pilot_mob_vnum = row[2] ? atoi(row[2]) : 0;
  config->min_bounty = row[3] ? atoi(row[3]) : 0;
  config->pursuit_speed = row[4] ? atoi(row[4]) : 0;
  config->hunt_duration_seconds = row[5] ? atoi(row[5]) : 0;
  config->target_grace_seconds = row[6] ? atoi(row[6]) : 0;
  config->cooldown_seconds = row[7] ? atoi(row[7]) : 0;
  config->enabled = row[8] ? atoi(row[8]) != 0 : FALSE;
  mysql_free_result(result);

  if (!vessel_hunter_config_is_valid(config))
  {
    log("SYSERR: Bounty-hunter encounter %d has invalid policy values",
        encounter_id);
    return -1;
  }
  return 1;
}

static struct char_data *vessel_hunter_online_owner_aboard(
    const struct greyhawk_ship_data *target)
{
  struct char_data *character;

  if (!is_valid_ship(target) || target->owner[0] == '\0')
  {
    return NULL;
  }

  for (character = character_list; character != NULL;
       character = character->next)
  {
    if (IS_NPC(character) || character->desc == NULL ||
        GET_NAME(character) == NULL ||
        str_cmp(GET_NAME(character), target->owner) ||
        IN_ROOM(character) == NOWHERE)
    {
      continue;
    }
    if (get_ship_from_room(IN_ROOM(character)) == target)
    {
      return character;
    }
  }
  return NULL;
}

static bool vessel_hunter_lifecycle_is_available(const char *target_name,
                                                  time_t now)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char escaped_name[129];
  char query[MAX_STRING_LENGTH];
  const char *status;
  time_t next_eligible_at;
  bool available;

  if (target_name == NULL || !*target_name)
  {
    return FALSE;
  }

  mysql_real_escape_string(conn, escaped_name, target_name,
                           strlen(target_name));
  snprintf(query, sizeof(query),
           "SELECT status, next_eligible_at FROM vessel_bounty_hunts "
           "WHERE target_player = '%s'",
           escaped_name);
  if (mysql_query(conn, query))
  {
    log("SYSERR: Could not read bounty-hunt lifecycle for %s: %s",
        target_name, mysql_error(conn));
    return FALSE;
  }

  result = mysql_store_result(conn);
  if (result == NULL)
  {
    return FALSE;
  }
  row = mysql_fetch_row(result);
  if (row == NULL)
  {
    mysql_free_result(result);
    return TRUE;
  }

  status = row[0] ? row[0] : "";
  next_eligible_at = row[1] ? (time_t)atoll(row[1]) : 0;
  available =
      vessel_hunter_lifecycle_allows_spawn(status, next_eligible_at, now);
  mysql_free_result(result);
  return available;
}

/**
 * A hunter may target only a moving, player-owned hull whose exact owner is
 * online aboard it and currently meets the configured HUNTED threshold.
 */
bool vessel_hunter_target_is_eligible(
    const struct greyhawk_ship_data *target,
    const struct vessel_hunter_config *config, time_t now)
{
  if (!mysql_available || conn == NULL || !is_valid_ship(target) ||
      !vessel_hunter_config_is_valid(config) || !config->enabled ||
      target->speed == 0 || target->owner[0] == '\0' ||
      vessel_hunter_online_owner_aboard(target) == NULL ||
      vessel_get_bounty(target->owner) < config->min_bounty)
  {
    return FALSE;
  }

  return vessel_hunter_lifecycle_is_available(target->owner, now);
}

static bool vessel_hunter_claim_lifecycle(
    const struct greyhawk_ship_data *target,
    const struct vessel_hunter_config *config, const char *encounter_name,
    time_t now, unsigned long long *generation, char *hunter_name,
    size_t hunter_name_size)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char escaped_target[129];
  char escaped_hunter_name[257];
  char query[MAX_STRING_LENGTH];
  const char *status;
  unsigned long long old_generation;
  time_t next_eligible_at;
  bool exists;

  if (target == NULL || config == NULL || encounter_name == NULL ||
      generation == NULL || hunter_name == NULL || hunter_name_size == 0)
  {
    return FALSE;
  }

  mysql_real_escape_string(conn, escaped_target, target->owner,
                           strlen(target->owner));
  if (mysql_query(conn, "START TRANSACTION"))
  {
    return FALSE;
  }

  snprintf(query, sizeof(query),
           "SELECT generation, status, next_eligible_at "
           "FROM vessel_bounty_hunts WHERE target_player = '%s' FOR UPDATE",
           escaped_target);
  if (mysql_query(conn, query))
  {
    goto rollback;
  }
  result = mysql_store_result(conn);
  if (result == NULL)
  {
    goto rollback;
  }

  row = mysql_fetch_row(result);
  exists = row != NULL;
  old_generation = exists && row[0] ? strtoull(row[0], NULL, 10) : 0;
  status = exists && row[1] ? row[1] : "";
  next_eligible_at = exists && row[2] ? (time_t)atoll(row[2]) : 0;
  if ((exists && !vessel_hunter_lifecycle_allows_spawn(
                     status, next_eligible_at, now)) ||
      old_generation == ULLONG_MAX)
  {
    mysql_free_result(result);
    mysql_query(conn, "ROLLBACK");
    return FALSE;
  }
  mysql_free_result(result);

  *generation = old_generation + 1;
  snprintf(hunter_name, hunter_name_size, "%.54s #%llu hunting %.40s",
           encounter_name, *generation, target->owner);
  mysql_real_escape_string(conn, escaped_hunter_name, hunter_name,
                           strlen(hunter_name));

  if (exists)
  {
    snprintf(query, sizeof(query),
             "UPDATE vessel_bounty_hunts SET encounter_id = %d, "
             "target_ship_id = %d, hunter_ship_id = NULL, "
             "hunter_name = '%s', generation = %llu, status = 'spawning', "
             "started_at = %lld, expires_at = %lld, next_eligible_at = 0, "
             "ended_at = 0, end_reason = '' WHERE target_player = '%s'",
             config->encounter_id, target->shipnum, escaped_hunter_name,
             *generation, (long long)now,
             (long long)(now + config->hunt_duration_seconds),
             escaped_target);
  }
  else
  {
    snprintf(query, sizeof(query),
             "INSERT INTO vessel_bounty_hunts "
             "(target_player, encounter_id, target_ship_id, hunter_ship_id, "
             "hunter_name, generation, status, started_at, expires_at, "
             "next_eligible_at, ended_at, end_reason) VALUES "
             "('%s', %d, %d, NULL, '%s', %llu, 'spawning', %lld, %lld, "
             "0, 0, '')",
             escaped_target, config->encounter_id, target->shipnum,
             escaped_hunter_name, *generation, (long long)now,
             (long long)(now + config->hunt_duration_seconds));
  }
  if (mysql_query(conn, query) || mysql_affected_rows(conn) != 1)
  {
    goto rollback;
  }
  if (mysql_query(conn, "COMMIT"))
  {
    goto rollback;
  }
  return TRUE;

rollback:
  log("SYSERR: Could not claim bounty-hunt lifecycle for %s: %s",
      target->owner, mysql_error(conn));
  mysql_query(conn, "ROLLBACK");
  return FALSE;
}

static bool vessel_hunter_set_cooldown(const char *target_name,
                                       unsigned long long generation,
                                       int hunter_ship_id,
                                       const char *reason,
                                       int cooldown_seconds)
{
  char escaped_target[129];
  char escaped_reason[VESSEL_HUNTER_REASON_LENGTH * 2 + 1];
  char query[MAX_STRING_LENGTH];
  time_t now;

  if (!mysql_available || conn == NULL || target_name == NULL ||
      !*target_name || reason == NULL || generation == 0)
  {
    return FALSE;
  }

  now = time(0);
  cooldown_seconds = MAX(VESSEL_HUNTER_COOLDOWN_MIN,
                         MIN(VESSEL_HUNTER_COOLDOWN_MAX, cooldown_seconds));
  mysql_real_escape_string(conn, escaped_target, target_name,
                           strlen(target_name));
  mysql_real_escape_string(conn, escaped_reason, reason, strlen(reason));
  snprintf(query, sizeof(query),
           "UPDATE vessel_bounty_hunts SET hunter_ship_id = NULL, "
           "status = 'cooldown', next_eligible_at = %lld, ended_at = %lld, "
           "end_reason = '%s' WHERE target_player = '%s' "
           "AND generation = %llu AND status IN ('active', 'spawning') "
           "AND (%d <= 0 OR hunter_ship_id = %d OR hunter_ship_id IS NULL)",
           (long long)(now + cooldown_seconds), (long long)now,
           escaped_reason, escaped_target, generation, hunter_ship_id,
           hunter_ship_id);
  if (mysql_query(conn, query))
  {
    log("SYSERR: Could not end bounty hunt for %s: %s", target_name,
        mysql_error(conn));
    return FALSE;
  }
  return mysql_affected_rows(conn) == 1;
}

static bool vessel_hunter_assign_pilot(struct greyhawk_ship_data *ship,
                                       int pilot_mob_vnum)
{
  struct char_data *pilot;
  mob_rnum pilot_rnum;
  room_rnum bridge;

  if (!is_valid_ship(ship) || pilot_mob_vnum <= 0)
  {
    return FALSE;
  }

  pilot = get_pilot_from_ship(ship);
  if (pilot != NULL &&
      GET_MOB_VNUM(pilot) == (mob_vnum)pilot_mob_vnum)
  {
    return TRUE;
  }

  pilot_rnum = real_mobile(pilot_mob_vnum);
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

  ship->autopilot->pilot_mob_vnum = pilot_mob_vnum;
  if (!vessel_db_save_pilot(ship))
  {
    ship->autopilot->pilot_mob_vnum = -1;
    extract_char(pilot);
    return FALSE;
  }
  return TRUE;
}

static void vessel_hunter_clear_runtime(struct greyhawk_ship_data *ship)
{
  if (ship == NULL)
  {
    return;
  }

  ship->bounty_hunter = FALSE;
  ship->hunter_encounter_id = 0;
  ship->hunter_target_ship_id = 0;
  ship->hunter_target_name[0] = '\0';
  ship->hunter_expires_at = 0;
  ship->hunter_target_missing_since = 0;
  ship->hunter_last_runtime_save = 0;
  ship->hunter_min_bounty = 0;
  ship->hunter_pursuit_speed = 0;
  ship->hunter_target_grace_seconds = 0;
  ship->hunter_cooldown_seconds = 0;
  ship->hunter_bounty_check_ticks = 0;
  ship->last_attacker = 0;
}

static void vessel_hunter_attach_runtime(
    struct greyhawk_ship_data *hunter, const char *target_name,
    int target_ship_id, time_t expires_at,
    const struct vessel_hunter_config *config)
{
  if (!is_valid_ship(hunter) || target_name == NULL || config == NULL)
  {
    return;
  }

  hunter->bounty_hunter = TRUE;
  hunter->hunter_encounter_id = config->encounter_id;
  hunter->hunter_target_ship_id = target_ship_id;
  strlcpy(hunter->hunter_target_name, target_name,
          sizeof(hunter->hunter_target_name));
  hunter->hunter_expires_at = expires_at;
  hunter->hunter_target_missing_since = 0;
  hunter->hunter_last_runtime_save = time(0);
  hunter->hunter_min_bounty = config->min_bounty;
  hunter->hunter_pursuit_speed = config->pursuit_speed;
  hunter->hunter_target_grace_seconds = config->target_grace_seconds;
  hunter->hunter_cooldown_seconds = config->cooldown_seconds;
  hunter->hunter_bounty_check_ticks = 0;
  hunter->last_attacker = target_ship_id;
}

static bool vessel_hunter_retire_runtime_ship(int shipnum,
                                              const char *message)
{
  struct greyhawk_ship_data *ship;
  struct char_data *pilot;
  struct obj_data *hull;
  room_rnum exterior;
  int i;

  if (shipnum < 2 || shipnum >= GREYHAWK_MAXSHIPS ||
      !is_valid_ship(&greyhawk_ships[shipnum]))
  {
    return TRUE;
  }
  ship = &greyhawk_ships[shipnum];

  if (!vessel_delete_persistence(shipnum))
  {
    log("SYSERR: Could not retire bounty-hunter ship %d", shipnum);
    return FALSE;
  }

  hull = ship->shipobj;
  exterior = hull != NULL ? IN_ROOM(hull) : NOWHERE;
  pilot = get_pilot_from_ship(ship);
  if (message != NULL && *message)
  {
    send_to_ship(ship, "%s", message);
  }
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

  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
  {
    if (!is_valid_ship(&greyhawk_ships[i]) || i == shipnum)
    {
      continue;
    }
    if (greyhawk_ships[i].last_attacker == shipnum)
    {
      greyhawk_ships[i].last_attacker = 0;
      vessel_db_save_runtime(&greyhawk_ships[i]);
    }
    if (greyhawk_ships[i].docked_to_ship == shipnum)
    {
      greyhawk_ships[i].docked_to_ship = -1;
      greyhawk_ships[i].docking_room = 0;
      vessel_db_save_runtime(&greyhawk_ships[i]);
    }
  }

  log("Info: Retired bounty-hunter ship %d '%s'", shipnum, ship->name);
  memset(ship, 0, sizeof(*ship));
  return TRUE;
}

static bool vessel_hunter_activate_lifecycle(
    const char *target_name, unsigned long long generation, int hunter_ship_id)
{
  char escaped_target[129];
  char query[MAX_STRING_LENGTH];

  mysql_real_escape_string(conn, escaped_target, target_name,
                           strlen(target_name));
  snprintf(query, sizeof(query),
           "UPDATE vessel_bounty_hunts SET hunter_ship_id = %d, "
           "status = 'active' WHERE target_player = '%s' "
           "AND generation = %llu AND status = 'spawning' "
           "AND hunter_ship_id IS NULL",
           hunter_ship_id, escaped_target, generation);
  if (mysql_query(conn, query))
  {
    log("SYSERR: Could not activate bounty hunt for %s: %s", target_name,
        mysql_error(conn));
    return FALSE;
  }
  return mysql_affected_rows(conn) == 1;
}

/**
 * Spawn a configured public warship through the ordinary prototype
 * constructor and bind it to the claimed durable lifecycle.
 */
bool vessel_hunter_spawn(struct greyhawk_ship_data *target,
                         const struct vessel_hunter_config *config,
                         const char *encounter_name)
{
  struct greyhawk_ship_data *hunter;
  char target_name[64];
  char hunter_name[128];
  unsigned long long generation;
  time_t now;
  int slot;

  if (!is_valid_ship(target) ||
      !vessel_hunter_config_is_valid(config) || !config->enabled ||
      encounter_name == NULL || !*encounter_name)
  {
    return FALSE;
  }

  now = time(0);
  strlcpy(target_name, target->owner, sizeof(target_name));
  if (!vessel_hunter_claim_lifecycle(target, config, encounter_name, now,
                                     &generation, hunter_name,
                                     sizeof(hunter_name)))
  {
    return FALSE;
  }

  slot = vessel_spawn_public_from_prototype_at(
      config->prototype_id, hunter_name, (int)target->x, (int)target->y,
      (int)target->z);
  if (slot < 0)
  {
    vessel_hunter_set_cooldown(target_name, generation, 0,
                               "spawn failed",
                               VESSEL_HUNTER_SPAWN_RETRY_SECONDS);
    return FALSE;
  }

  hunter = &greyhawk_ships[slot];
  if (hunter->vessel_type != VESSEL_WARSHIP ||
      !vessel_hunter_assign_pilot(hunter, config->pilot_mob_vnum))
  {
    vessel_hunter_set_cooldown(target_name, generation, slot,
                               "invalid warship or pilot",
                               VESSEL_HUNTER_SPAWN_RETRY_SECONDS);
    vessel_hunter_retire_runtime_ship(slot, NULL);
    return FALSE;
  }

  hunter->speed = MIN(config->pursuit_speed, MAX(1, hunter->maxspeed));
  hunter->setspeed = hunter->speed;
  vessel_hunter_attach_runtime(hunter, target_name, target->shipnum,
                               now + config->hunt_duration_seconds, config);
  if (!vessel_db_save_runtime(hunter) ||
      !vessel_hunter_activate_lifecycle(target_name, generation, slot))
  {
    vessel_hunter_set_cooldown(target_name, generation, slot,
                               "activation failed",
                               VESSEL_HUNTER_SPAWN_RETRY_SECONDS);
    vessel_hunter_retire_runtime_ship(slot, NULL);
    return FALSE;
  }

  log("Info: Bounty hunt generation %llu spawned warship %d '%s' for "
      "HUNTED player %s on ship %d",
      generation, slot, hunter->name, target_name, target->shipnum);
  return TRUE;
}

static int vessel_hunter_find_ship_by_name(const char *hunter_name,
                                           int prototype_id)
{
  int i;

  for (i = 2; i < GREYHAWK_MAXSHIPS; i++)
  {
    if (is_valid_ship(&greyhawk_ships[i]) &&
        greyhawk_ships[i].prototype_id == prototype_id &&
        greyhawk_ships[i].owner[0] == '\0' &&
        !str_cmp(greyhawk_ships[i].name, hunter_name))
    {
      return i;
    }
  }
  return -1;
}

static bool vessel_hunter_ship_identity_matches(int shipnum,
                                                const char *hunter_name,
                                                int prototype_id)
{
  struct greyhawk_ship_data *ship;

  if (shipnum < 2 || shipnum >= GREYHAWK_MAXSHIPS ||
      hunter_name == NULL || !*hunter_name ||
      !is_valid_ship(&greyhawk_ships[shipnum]))
  {
    return FALSE;
  }

  ship = &greyhawk_ships[shipnum];
  return ship->owner[0] == '\0' &&
         !str_cmp(ship->name, hunter_name) &&
         (prototype_id <= 0 || ship->prototype_id == prototype_id);
}

static int vessel_hunter_collect_boot_rows(
    struct vessel_hunter_boot_row *rows, int capacity)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  const char *query =
      "SELECT hunt.target_player, hunt.hunter_name, hunt.generation, "
      "hunt.status, hunt.target_ship_id, hunt.hunter_ship_id, "
      "hunt.expires_at, config.encounter_id, config.prototype_id, "
      "config.pilot_mob_vnum, config.min_bounty, config.pursuit_speed, "
      "config.hunt_duration_seconds, config.target_grace_seconds, "
      "config.cooldown_seconds, config.enabled "
      "FROM vessel_bounty_hunts AS hunt "
      "LEFT JOIN vessel_hunter_encounters AS config "
      "ON config.encounter_id = hunt.encounter_id "
      "WHERE hunt.status IN ('active', 'spawning') "
      "ORDER BY hunt.target_player";
  struct vessel_hunter_boot_row *boot_row;
  int count;

  if (rows == NULL || capacity <= 0 || mysql_query(conn, query))
  {
    if (rows != NULL && capacity > 0)
    {
      log("SYSERR: Could not enumerate active bounty hunts: %s",
          mysql_error(conn));
    }
    return 0;
  }
  result = mysql_store_result(conn);
  if (result == NULL)
  {
    return 0;
  }

  count = 0;
  while (count < capacity && (row = mysql_fetch_row(result)) != NULL)
  {
    boot_row = &rows[count++];
    memset(boot_row, 0, sizeof(*boot_row));
    strlcpy(boot_row->target_player, row[0] ? row[0] : "",
            sizeof(boot_row->target_player));
    strlcpy(boot_row->hunter_name, row[1] ? row[1] : "",
            sizeof(boot_row->hunter_name));
    boot_row->generation = row[2] ? strtoull(row[2], NULL, 10) : 0;
    strlcpy(boot_row->status, row[3] ? row[3] : "",
            sizeof(boot_row->status));
    boot_row->target_ship_id = row[4] ? atoi(row[4]) : 0;
    boot_row->hunter_ship_id = row[5] ? atoi(row[5]) : -1;
    boot_row->expires_at = row[6] ? (time_t)atoll(row[6]) : 0;
    boot_row->config.encounter_id = row[7] ? atoi(row[7]) : 0;
    boot_row->config.prototype_id = row[8] ? atoi(row[8]) : 0;
    boot_row->config.pilot_mob_vnum = row[9] ? atoi(row[9]) : 0;
    boot_row->config.min_bounty = row[10] ? atoi(row[10]) : 0;
    boot_row->config.pursuit_speed = row[11] ? atoi(row[11]) : 0;
    boot_row->config.hunt_duration_seconds =
        row[12] ? atoi(row[12]) : 0;
    boot_row->config.target_grace_seconds =
        row[13] ? atoi(row[13]) : 0;
    boot_row->config.cooldown_seconds = row[14] ? atoi(row[14]) : 0;
    boot_row->config.enabled = row[15] ? atoi(row[15]) != 0 : FALSE;
  }
  mysql_free_result(result);
  return count;
}

/**
 * Reattach active public warships after vessel persistence restoration.
 */
void vessel_hunter_boot(void)
{
  struct vessel_hunter_boot_row rows[GREYHAWK_MAXSHIPS];
  struct vessel_hunter_boot_row *row;
  struct greyhawk_ship_data *hunter;
  time_t now;
  int row_count;
  int hunter_ship_id;
  int attached;
  int i;

  if (!mysql_available || conn == NULL)
  {
    return;
  }
  vessel_hunter_ensure_schema();
  row_count = vessel_hunter_collect_boot_rows(rows, GREYHAWK_MAXSHIPS);
  now = time(0);
  attached = 0;

  for (i = 0; i < row_count; i++)
  {
    row = &rows[i];
    hunter_ship_id = row->hunter_ship_id;
    if (!vessel_hunter_config_is_valid(&row->config) ||
        !row->config.enabled || row->target_player[0] == '\0' ||
        row->generation == 0)
    {
      vessel_hunter_set_cooldown(
          row->target_player, row->generation, hunter_ship_id,
          "configuration unavailable", VESSEL_HUNTER_SPAWN_RETRY_SECONDS);
      if (vessel_hunter_ship_identity_matches(
              hunter_ship_id, row->hunter_name, 0))
      {
        vessel_hunter_retire_runtime_ship(hunter_ship_id, NULL);
      }
      continue;
    }

    if (!str_cmp(row->status, "spawning") && hunter_ship_id < 2)
    {
      hunter_ship_id = vessel_hunter_find_ship_by_name(
          row->hunter_name, row->config.prototype_id);
      if (hunter_ship_id >= 2 &&
          !vessel_hunter_activate_lifecycle(
              row->target_player, row->generation, hunter_ship_id))
      {
        vessel_hunter_set_cooldown(
            row->target_player, row->generation, 0,
            "restart activation failed",
            VESSEL_HUNTER_SPAWN_RETRY_SECONDS);
        vessel_hunter_retire_runtime_ship(hunter_ship_id, NULL);
        hunter_ship_id = -1;
        continue;
      }
    }

    if (!vessel_hunter_ship_identity_matches(
            hunter_ship_id, row->hunter_name, row->config.prototype_id))
    {
      vessel_hunter_set_cooldown(
          row->target_player, row->generation, row->hunter_ship_id,
          "hunter missing after restart", row->config.cooldown_seconds);
      if (vessel_hunter_ship_identity_matches(
              hunter_ship_id, row->hunter_name, 0))
      {
        vessel_hunter_retire_runtime_ship(hunter_ship_id, NULL);
      }
      continue;
    }

    hunter = &greyhawk_ships[hunter_ship_id];
    if (row->expires_at <= now ||
        !vessel_hunter_assign_pilot(hunter,
                                    row->config.pilot_mob_vnum))
    {
      vessel_hunter_set_cooldown(
          row->target_player, row->generation, hunter_ship_id,
          row->expires_at <= now ? "hunt expired during restart"
                                 : "pilot unavailable after restart",
          row->config.cooldown_seconds);
      vessel_hunter_retire_runtime_ship(hunter_ship_id, NULL);
      continue;
    }

    vessel_hunter_attach_runtime(
        hunter, row->target_player, row->target_ship_id, row->expires_at,
        &row->config);
    hunter->speed =
        MIN(row->config.pursuit_speed, MAX(1, hunter->maxspeed));
    hunter->setspeed = hunter->speed;
    vessel_db_save_runtime(hunter);
    attached++;
  }

  log("Info: Reattached %d active bounty-hunter warship%s", attached,
      attached == 1 ? "" : "s");
}

static struct greyhawk_ship_data *vessel_hunter_find_target(
    struct greyhawk_ship_data *hunter)
{
  struct greyhawk_ship_data *target;
  int i;

  if (!is_valid_ship(hunter) || !hunter->bounty_hunter ||
      hunter->hunter_target_name[0] == '\0')
  {
    return NULL;
  }

  if (hunter->hunter_target_ship_id > 0 &&
      hunter->hunter_target_ship_id < GREYHAWK_MAXSHIPS)
  {
    target = &greyhawk_ships[hunter->hunter_target_ship_id];
    if (is_valid_ship(target) &&
        !str_cmp(target->owner, hunter->hunter_target_name) &&
        vessel_hunter_online_owner_aboard(target) != NULL)
    {
      return target;
    }
  }

  for (i = 2; i < GREYHAWK_MAXSHIPS; i++)
  {
    target = &greyhawk_ships[i];
    if (target != hunter && is_valid_ship(target) &&
        !str_cmp(target->owner, hunter->hunter_target_name) &&
        vessel_hunter_online_owner_aboard(target) != NULL)
    {
      return target;
    }
  }
  return NULL;
}

static unsigned long long vessel_hunter_generation_for_ship(int shipnum)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char query[MAX_STRING_LENGTH];
  unsigned long long generation;

  snprintf(query, sizeof(query),
           "SELECT generation FROM vessel_bounty_hunts "
           "WHERE hunter_ship_id = %d AND status = 'active'",
           shipnum);
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
  generation = row != NULL && row[0] != NULL
                   ? strtoull(row[0], NULL, 10)
                   : 0;
  mysql_free_result(result);
  return generation;
}

static bool vessel_hunter_finish_runtime(struct greyhawk_ship_data *hunter,
                                         const char *reason, bool retire,
                                         bool save_runtime)
{
  char target_name[64];
  unsigned long long generation;
  int shipnum;
  int cooldown;

  if (!is_valid_ship(hunter) || !hunter->bounty_hunter)
  {
    return FALSE;
  }

  shipnum = hunter->shipnum;
  cooldown = hunter->hunter_cooldown_seconds;
  strlcpy(target_name, hunter->hunter_target_name, sizeof(target_name));
  generation = vessel_hunter_generation_for_ship(shipnum);
  if (generation == 0 ||
      !vessel_hunter_set_cooldown(target_name, generation, shipnum, reason,
                                  cooldown))
  {
    log("SYSERR: Bounty-hunter ship %d could not close its lifecycle",
        shipnum);
    return FALSE;
  }

  vessel_hunter_clear_runtime(hunter);
  if (retire)
  {
    return vessel_hunter_retire_runtime_ship(
        shipnum, "The navy breaks off the hunt and turns for home.");
  }
  return !save_runtime || vessel_db_save_runtime(hunter);
}

/**
 * Pursue the current owned hull, keep proactive fire armed, and retire after
 * the durable duration, pardon, or bounded absence grace.
 */
void vessel_hunter_tick(void)
{
  struct greyhawk_ship_data *hunter;
  struct greyhawk_ship_data *target;
  struct waypoint waypoint;
  time_t now;
  bool target_changed;
  int target_x;
  int target_y;
  int target_z;
  int speed;
  int i;

  now = time(0);
  for (i = 2; i < GREYHAWK_MAXSHIPS; i++)
  {
    hunter = &greyhawk_ships[i];
    if (!is_valid_ship(hunter) || !hunter->bounty_hunter)
    {
      continue;
    }

    if (hunter->hunter_expires_at <= now)
    {
      vessel_hunter_finish_runtime(hunter, "hunt expired", TRUE, FALSE);
      continue;
    }

    hunter->hunter_bounty_check_ticks++;
    if (hunter->hunter_bounty_check_ticks >=
        VESSEL_HUNTER_BOUNTY_CHECK_INTERVAL)
    {
      hunter->hunter_bounty_check_ticks = 0;
      if (vessel_get_bounty(hunter->hunter_target_name) <
          hunter->hunter_min_bounty)
      {
        vessel_hunter_finish_runtime(hunter, "target pardoned", TRUE, FALSE);
        continue;
      }
    }

    target = vessel_hunter_find_target(hunter);
    if (target == NULL)
    {
      hunter->last_attacker = 0;
      hunter->speed = 0;
      hunter->setspeed = 0;
      if (hunter->hunter_target_missing_since == 0)
      {
        hunter->hunter_target_missing_since = now;
      }
      if (now - hunter->hunter_target_missing_since >=
          hunter->hunter_target_grace_seconds)
      {
        vessel_hunter_finish_runtime(hunter, "target left the vessel", TRUE,
                                     FALSE);
      }
      continue;
    }

    hunter->hunter_target_missing_since = 0;
    target_changed = hunter->hunter_target_ship_id != target->shipnum;
    hunter->hunter_target_ship_id = target->shipnum;
    hunter->last_attacker = target->shipnum;
    speed = MIN(hunter->hunter_pursuit_speed, MAX(1, hunter->maxspeed));
    hunter->speed = speed;
    hunter->setspeed = speed;
    hunter->setheading =
        (short int)greyhawk_bearing(hunter->x, hunter->y, target->x,
                                   target->y);
    hunter->heading = hunter->setheading;

    memset(&waypoint, 0, sizeof(waypoint));
    waypoint.x = target->x;
    waypoint.y = target->y;
    waypoint.z = target->z;
    if (vessel_autopilot_next_position(
            hunter, &waypoint, (float)speed, &target_x, &target_y,
            &target_z) &&
        (target_x != (int)hunter->x || target_y != (int)hunter->y ||
         target_z != (int)hunter->z))
    {
      update_ship_wilderness_position(i, target_x, target_y, target_z);
    }

    if (target_changed ||
        now - hunter->hunter_last_runtime_save >=
            VESSEL_HUNTER_RUNTIME_SAVE_INTERVAL)
    {
      char escaped_target[129];
      char query[MAX_STRING_LENGTH];

      hunter->hunter_last_runtime_save = now;
      vessel_db_save_runtime(hunter);
      if (target_changed)
      {
        mysql_real_escape_string(conn, escaped_target,
                                 hunter->hunter_target_name,
                                 strlen(hunter->hunter_target_name));
        snprintf(query, sizeof(query),
                 "UPDATE vessel_bounty_hunts SET target_ship_id = %d "
                 "WHERE target_player = '%s' AND hunter_ship_id = %d "
                 "AND status = 'active'",
                 target->shipnum, escaped_target, hunter->shipnum);
        if (mysql_query(conn, query))
        {
          log("SYSERR: Could not update bounty-hunt target ship: %s",
              mysql_error(conn));
        }
      }
    }
  }
}

/**
 * A sunk hunter closes its lifecycle before vessel_sink() clears the slot.
 */
void vessel_hunter_handle_sink(struct greyhawk_ship_data *ship)
{
  if (is_valid_ship(ship) && ship->bounty_hunter)
  {
    vessel_hunter_finish_runtime(ship, "hunter sunk", FALSE, FALSE);
  }
}

/**
 * Capturing a navy ship ends the hunt and removes the navy pilot while
 * leaving the ordinary captured hull in the player's possession.
 */
void vessel_hunter_handle_capture(struct char_data *ch,
                                  struct greyhawk_ship_data *ship)
{
  struct char_data *pilot;

  if (!is_valid_ship(ship) || !ship->bounty_hunter)
  {
    return;
  }
  if (!vessel_hunter_finish_runtime(ship, "hunter captured", FALSE, TRUE))
  {
    return;
  }

  pilot = get_pilot_from_ship(ship);
  if (ship->autopilot != NULL)
  {
    ship->autopilot->pilot_mob_vnum = -1;
    vessel_db_save_pilot(ship);
  }
  if (pilot != NULL)
  {
    extract_char(pilot);
  }
  autopilot_cleanup(ship);
  ship->speed = 0;
  ship->setspeed = 0;
  vessel_db_save_runtime(ship);
  if (ch != NULL)
  {
    send_to_char(ch,
                 "With its navy pilot removed, the captured warship is "
                 "yours to command.\r\n");
  }
}

/**
 * Staff purge closes the lifecycle; do_shippurge owns runtime destruction.
 */
void vessel_hunter_handle_purge(struct greyhawk_ship_data *ship,
                                const char *staff_name)
{
  char reason[VESSEL_HUNTER_REASON_LENGTH];

  if (!is_valid_ship(ship) || !ship->bounty_hunter)
  {
    return;
  }
  snprintf(reason, sizeof(reason), "purged by %.47s",
           staff_name != NULL && *staff_name ? staff_name : "staff");
  vessel_hunter_finish_runtime(ship, reason, FALSE, FALSE);
}

/**
 * Keep active in-memory pursuit identity synchronized after a committed
 * character rename. The lifecycle row itself is part of the rename
 * transaction.
 */
void vessel_hunter_handle_player_rename(const char *old_name,
                                        const char *new_name)
{
  int i;

  if (old_name == NULL || !*old_name || new_name == NULL || !*new_name)
  {
    return;
  }

  for (i = 2; i < GREYHAWK_MAXSHIPS; i++)
  {
    if (is_valid_ship(&greyhawk_ships[i]) &&
        greyhawk_ships[i].bounty_hunter &&
        !str_cmp(greyhawk_ships[i].hunter_target_name, old_name))
    {
      strlcpy(greyhawk_ships[i].hunter_target_name, new_name,
              sizeof(greyhawk_ships[i].hunter_target_name));
    }
  }
}

/**
 * Permanent deletion retires any live hunter after the transaction removes
 * its lifecycle row.
 */
void vessel_hunter_handle_player_removal(const char *player_name)
{
  int i;

  if (player_name == NULL || !*player_name)
  {
    return;
  }

  for (i = 2; i < GREYHAWK_MAXSHIPS; i++)
  {
    if (is_valid_ship(&greyhawk_ships[i]) &&
        greyhawk_ships[i].bounty_hunter &&
        !str_cmp(greyhawk_ships[i].hunter_target_name, player_name))
    {
      vessel_hunter_clear_runtime(&greyhawk_ships[i]);
      vessel_hunter_retire_runtime_ship(i, NULL);
    }
  }
}
