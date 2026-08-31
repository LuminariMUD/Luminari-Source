/* ************************************************************************
 *      File:   vessels_events.c                      Part of LuminariMUD  *
 *   Purpose:   Staff-triggered naval showcase events and leaderboards.   *
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
#include "vessel_periodic.h"
#include "wilderness/wilderness.h"
#include "act.h"
#include "mysql.h"

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];
extern struct descriptor_data *descriptor_list;
extern struct room_data *world;
extern MYSQL *conn;
extern bool mysql_available;

#define VESSEL_EVENT_MAX_PARTICIPANTS 64
#define VESSEL_EVENT_MAX_GHOSTS 5
#define VESSEL_EVENT_MAX_DURATION 3600
#define VESSEL_EVENT_SINK_BONUS 100

struct vessel_event_participant
{
  int ship_id;
  long player_idnum;
  char captain_name[MAX_NAME_LENGTH + 1];
  int team;
  int score;
  int finish_seconds;
  int placement;
  bool finished;
  bool active;
};

struct vessel_event_state
{
  bool active;
  bool pending_end;
  bool recovery_required;
  enum vessel_event_type type;
  unsigned long long event_id;
  time_t started_at;
  long staff_idnum;
  int start_x;
  int start_y;
  int finish_x;
  int finish_y;
  struct vessel_event_participant participants[VESSEL_EVENT_MAX_PARTICIPANTS];
  int participant_count;
  int ghost_slots[VESSEL_EVENT_MAX_GHOSTS];
  int ghost_count;
};

static struct vessel_event_state vessel_event;

static void vessel_event_clear_state(void);
static bool vessel_event_finish(const char *reason, bool record_scores);
static bool vessel_event_retire_ship(int shipnum, const char *reason);

const char *vessel_event_type_name(enum vessel_event_type event_type)
{
  switch (event_type)
  {
  case VESSEL_EVENT_REGATTA:
    return "regatta";
  case VESSEL_EVENT_SKIRMISH:
    return "skirmish";
  case VESSEL_EVENT_GHOST_FLEET:
    return "ghost";
  case VESSEL_EVENT_NONE:
  default:
    return "none";
  }
}

enum vessel_event_type vessel_event_type_from_name(const char *name)
{
  if (name == NULL || *name == '\0')
  {
    return VESSEL_EVENT_NONE;
  }
  if (!strcasecmp(name, "regatta") || !strcasecmp(name, "race"))
  {
    return VESSEL_EVENT_REGATTA;
  }
  if (!strcasecmp(name, "skirmish") || !strcasecmp(name, "battle"))
  {
    return VESSEL_EVENT_SKIRMISH;
  }
  if (!strcasecmp(name, "ghost") || !strcasecmp(name, "ghostfleet") ||
      !strcasecmp(name, "ghost-fleet"))
  {
    return VESSEL_EVENT_GHOST_FLEET;
  }
  return VESSEL_EVENT_NONE;
}

bool vessel_event_finish_reached(int old_x, int old_y, int new_x, int new_y, int finish_x,
                                 int finish_y)
{
  return (new_x == finish_x && new_y == finish_y) && (old_x != finish_x || old_y != finish_y);
}

int vessel_event_placement_points(int placement)
{
  if (placement <= 0)
  {
    return 0;
  }
  return MAX(10, 110 - placement * 10);
}

int vessel_event_winning_team(int red_score, int blue_score)
{
  if (red_score == blue_score)
  {
    return VESSEL_EVENT_TEAM_NONE;
  }
  return red_score > blue_score ? VESSEL_EVENT_TEAM_RED : VESSEL_EVENT_TEAM_BLUE;
}

static const char *vessel_event_team_name(int team)
{
  switch (team)
  {
  case VESSEL_EVENT_TEAM_RED:
    return "red";
  case VESSEL_EVENT_TEAM_BLUE:
    return "blue";
  default:
    return "none";
  }
}

static void vessel_event_broadcast(const char *format, ...)
{
  struct descriptor_data *d;
  char message[MAX_STRING_LENGTH];
  va_list args;

  va_start(args, format);
  vsnprintf(message, sizeof(message), format, args);
  va_end(args);

  for (d = descriptor_list; d != NULL; d = d->next)
  {
    if (IS_PLAYING(d) && d->character != NULL)
    {
      send_to_char(d->character, "\r\n[Vessel Event] %s\r\n", message);
    }
  }
}

void vessel_event_ensure_schema(void)
{
  const char *create_events =
      "CREATE TABLE IF NOT EXISTS vessel_showcase_events ("
      "event_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,"
      "event_type VARCHAR(16) NOT NULL,"
      "status VARCHAR(16) NOT NULL DEFAULT 'active',"
      "staff_idnum BIGINT NOT NULL DEFAULT 0,"
      "start_x INT NOT NULL DEFAULT 0,start_y INT NOT NULL DEFAULT 0,"
      "finish_x INT NOT NULL DEFAULT 0,finish_y INT NOT NULL DEFAULT 0,"
      "started_at BIGINT NOT NULL DEFAULT 0,ended_at BIGINT NOT NULL DEFAULT 0,"
      "end_reason VARCHAR(127) NOT NULL DEFAULT '',"
      "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
      "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
      "INDEX idx_vessel_showcase_status (status,event_type),"
      "INDEX idx_vessel_showcase_staff (staff_idnum)"
      ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
  const char *create_participants =
      "CREATE TABLE IF NOT EXISTS vessel_event_participants ("
      "event_id BIGINT UNSIGNED NOT NULL,ship_id INT NOT NULL,"
      "player_idnum BIGINT NOT NULL DEFAULT 0,team VARCHAR(8) NOT NULL DEFAULT 'none',"
      "score INT NOT NULL DEFAULT 0,finish_seconds INT NOT NULL DEFAULT 0,"
      "placement INT NOT NULL DEFAULT 0,status VARCHAR(16) NOT NULL DEFAULT 'active',"
      "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
      "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
      "PRIMARY KEY (event_id,ship_id),"
      "INDEX idx_vessel_event_player (player_idnum,event_id),"
      "CONSTRAINT fk_vessel_event_participant_event FOREIGN KEY (event_id) "
      "REFERENCES vessel_showcase_events(event_id) ON DELETE CASCADE"
      ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
  const char *create_leaderboards =
      "CREATE TABLE IF NOT EXISTS vessel_event_leaderboards ("
      "event_type VARCHAR(16) NOT NULL,player_idnum BIGINT NOT NULL,"
      "entries INT NOT NULL DEFAULT 0,wins INT NOT NULL DEFAULT 0,"
      "points BIGINT NOT NULL DEFAULT 0,best_time_seconds INT NULL,"
      "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
      "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
      "PRIMARY KEY (event_type,player_idnum),"
      "INDEX idx_vessel_event_rank (event_type,wins,points),"
      "INDEX idx_vessel_event_player_rank (player_idnum)"
      ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
  const char *create_runtimes =
      "CREATE TABLE IF NOT EXISTS vessel_event_runtimes ("
      "ship_id INT NOT NULL PRIMARY KEY,event_id BIGINT UNSIGNED NOT NULL,"
      "role VARCHAR(16) NOT NULL DEFAULT 'ghost',ordinal_num INT NOT NULL DEFAULT 0,"
      "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
      "INDEX idx_vessel_event_runtime_event (event_id),"
      "CONSTRAINT fk_vessel_event_runtime_event FOREIGN KEY (event_id) "
      "REFERENCES vessel_showcase_events(event_id) ON DELETE CASCADE"
      ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";

  if (!mysql_available || conn == NULL)
  {
    return;
  }
  if (mysql_query(conn, create_events))
  {
    log("SYSERR: Could not create vessel_showcase_events: %s", mysql_error(conn));
    return;
  }
  if (mysql_query(conn, create_participants))
  {
    log("SYSERR: Could not create vessel_event_participants: %s", mysql_error(conn));
    return;
  }
  if (mysql_query(conn, create_leaderboards))
  {
    log("SYSERR: Could not create vessel_event_leaderboards: %s", mysql_error(conn));
    return;
  }
  if (mysql_query(conn, create_runtimes))
  {
    log("SYSERR: Could not create vessel_event_runtimes: %s", mysql_error(conn));
  }
}

static bool vessel_event_database_ready(struct char_data *ch)
{
  if (!mysql_available || conn == NULL)
  {
    if (ch != NULL)
    {
      send_to_char(ch, "Vessel event records are unavailable.\r\n");
    }
    return FALSE;
  }
  vessel_event_ensure_schema();
  return TRUE;
}

static bool vessel_event_open_database_event(void)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  bool open_event;

  if (mysql_query(conn, "SELECT COUNT(*) FROM vessel_showcase_events "
                        "WHERE status IN ('active','spawning','recovery_failed')"))
  {
    log("SYSERR: Could not check open vessel events: %s", mysql_error(conn));
    return TRUE;
  }
  result = mysql_store_result(conn);
  if (result == NULL)
  {
    return TRUE;
  }
  row = mysql_fetch_row(result);
  open_event = row != NULL && row[0] != NULL && atoi(row[0]) > 0;
  mysql_free_result(result);
  return open_event;
}

static bool vessel_event_set_database_status(const char *status, const char *reason)
{
  char escaped_reason[256];
  char query[MAX_STRING_LENGTH];
  size_t reason_length;

  if (vessel_event.event_id == 0 || status == NULL || reason == NULL)
  {
    return FALSE;
  }
  reason_length = strlen(reason);
  if (reason_length > sizeof(escaped_reason) / 2 - 1)
  {
    reason_length = sizeof(escaped_reason) / 2 - 1;
  }
  mysql_real_escape_string(conn, escaped_reason, reason, reason_length);
  snprintf(query, sizeof(query),
           "UPDATE vessel_showcase_events SET status='%s',ended_at=%lld,"
           "end_reason='%s' WHERE event_id=%llu",
           status, (long long)time(NULL), escaped_reason, vessel_event.event_id);
  if (mysql_query(conn, query))
  {
    log("SYSERR: Could not update vessel event %llu: %s", vessel_event.event_id, mysql_error(conn));
    return FALSE;
  }
  return TRUE;
}

static bool vessel_event_begin(enum vessel_event_type type, struct char_data *ch, int start_x,
                               int start_y, int finish_x, int finish_y, const char *status)
{
  char query[MAX_STRING_LENGTH];
  time_t now;

  if (vessel_event.active || vessel_event_open_database_event())
  {
    send_to_char(ch, "A vessel event is already active or needs recovery.\r\n");
    return FALSE;
  }

  now = time(NULL);
  snprintf(query, sizeof(query),
           "INSERT INTO vessel_showcase_events "
           "(event_type,status,staff_idnum,start_x,start_y,finish_x,finish_y,started_at) "
           "VALUES ('%s','%s',%ld,%d,%d,%d,%d,%lld)",
           vessel_event_type_name(type), status, GET_IDNUM(ch), start_x, start_y, finish_x,
           finish_y, (long long)now);
  if (mysql_query(conn, query))
  {
    log("SYSERR: Could not create vessel event: %s", mysql_error(conn));
    send_to_char(ch, "The vessel event record could not be created.\r\n");
    return FALSE;
  }

  vessel_event_clear_state();
  vessel_event.active = TRUE;
  vessel_event.type = type;
  vessel_event.event_id = mysql_insert_id(conn);
  vessel_event.started_at = now;
  vessel_event.staff_idnum = GET_IDNUM(ch);
  vessel_event.start_x = start_x;
  vessel_event.start_y = start_y;
  vessel_event.finish_x = finish_x;
  vessel_event.finish_y = finish_y;
  return TRUE;
}

static void vessel_event_clear_state(void)
{
  memset(&vessel_event, 0, sizeof(vessel_event));
  vessel_event.type = VESSEL_EVENT_NONE;
}

static struct vessel_event_participant *vessel_event_participant_by_ship(int ship_id)
{
  int i;

  for (i = 0; i < vessel_event.participant_count; i++)
  {
    if (vessel_event.participants[i].active && vessel_event.participants[i].ship_id == ship_id)
    {
      return &vessel_event.participants[i];
    }
  }
  return NULL;
}

static struct vessel_event_participant *vessel_event_participant_by_player(long player_idnum)
{
  int i;

  if (player_idnum <= 0)
  {
    return NULL;
  }
  for (i = 0; i < vessel_event.participant_count; i++)
  {
    if (vessel_event.participants[i].active &&
        vessel_event.participants[i].player_idnum == player_idnum)
    {
      return &vessel_event.participants[i];
    }
  }
  return NULL;
}

static bool vessel_event_save_participant(const struct vessel_event_participant *participant,
                                          bool insert)
{
  char query[MAX_STRING_LENGTH];
  const char *status;

  if (participant == NULL)
  {
    return FALSE;
  }
  status = participant->finished ? "finished" : (participant->active ? "active" : "withdrawn");
  if (insert)
  {
    snprintf(query, sizeof(query),
             "INSERT INTO vessel_event_participants "
             "(event_id,ship_id,player_idnum,team,score,finish_seconds,placement,status) "
             "VALUES (%llu,%d,%ld,'%s',%d,%d,%d,'%s')",
             vessel_event.event_id, participant->ship_id, participant->player_idnum,
             vessel_event_team_name(participant->team), participant->score,
             participant->finish_seconds, participant->placement, status);
  }
  else
  {
    snprintf(query, sizeof(query),
             "UPDATE vessel_event_participants SET score=%d,finish_seconds=%d,"
             "placement=%d,status='%s' WHERE event_id=%llu AND ship_id=%d",
             participant->score, participant->finish_seconds, participant->placement, status,
             vessel_event.event_id, participant->ship_id);
  }
  if (mysql_query(conn, query))
  {
    log("SYSERR: Could not save vessel event participant %d: %s", participant->ship_id,
        mysql_error(conn));
    return FALSE;
  }
  return TRUE;
}

static bool vessel_event_add_participant(int ship_id, long player_idnum, const char *captain_name,
                                         int team)
{
  struct vessel_event_participant *participant;

  if (vessel_event.participant_count >= VESSEL_EVENT_MAX_PARTICIPANTS ||
      vessel_event_participant_by_ship(ship_id) != NULL ||
      vessel_event_participant_by_player(player_idnum) != NULL)
  {
    return FALSE;
  }

  participant = &vessel_event.participants[vessel_event.participant_count];
  memset(participant, 0, sizeof(*participant));
  participant->ship_id = ship_id;
  participant->player_idnum = player_idnum;
  participant->team = team;
  participant->active = TRUE;
  strlcpy(participant->captain_name, captain_name != NULL ? captain_name : "Staff Fleet",
          sizeof(participant->captain_name));
  if (!vessel_event_save_participant(participant, TRUE))
  {
    memset(participant, 0, sizeof(*participant));
    return FALSE;
  }
  vessel_event.participant_count++;
  return TRUE;
}

static bool vessel_event_add_runtime(int ship_id, int ordinal)
{
  char query[MAX_STRING_LENGTH];

  snprintf(query, sizeof(query),
           "INSERT INTO vessel_event_runtimes (ship_id,event_id,role,ordinal_num) "
           "VALUES (%d,%llu,'ghost',%d)",
           ship_id, vessel_event.event_id, ordinal);
  if (mysql_query(conn, query))
  {
    log("SYSERR: Could not track ghost fleet ship %d: %s", ship_id, mysql_error(conn));
    return FALSE;
  }
  return TRUE;
}

static void vessel_event_remove_runtime(int ship_id)
{
  char query[128];

  snprintf(query, sizeof(query), "DELETE FROM vessel_event_runtimes WHERE ship_id=%d", ship_id);
  if (mysql_query(conn, query))
  {
    log("SYSERR: Could not remove ghost fleet runtime %d: %s", ship_id, mysql_error(conn));
  }
}

static bool vessel_event_retire_ship(int shipnum, const char *reason)
{
  struct greyhawk_ship_data *ship;
  struct char_data *pilot;
  struct obj_data *hull;
  char ship_name[sizeof(greyhawk_ships[0].name)];
  room_rnum exterior;
  int i;

  if (shipnum < 2 || shipnum >= GREYHAWK_MAXSHIPS || !is_valid_ship(&greyhawk_ships[shipnum]))
  {
    vessel_event_remove_runtime(shipnum);
    return TRUE;
  }
  ship = &greyhawk_ships[shipnum];
  strlcpy(ship_name, ship->name, sizeof(ship_name));
  if (!vessel_delete_persistence(shipnum))
  {
    log("SYSERR: Could not retire vessel event ship %d", shipnum);
    return FALSE;
  }

  hull = ship->shipobj;
  exterior = hull != NULL ? IN_ROOM(hull) : NOWHERE;
  pilot = get_pilot_from_ship(ship);
  if (reason != NULL && *reason)
  {
    send_to_ship(ship, "%s", reason);
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

  vessel_periodic_forget(ship);
  memset(ship, 0, sizeof(*ship));
  vessel_event_remove_runtime(shipnum);
  log("Info: Retired vessel event ship %d '%s'", shipnum, ship_name);
  return TRUE;
}

static int vessel_event_cleanup_ghosts(void)
{
  int cleaned;
  int i;

  cleaned = 0;
  for (i = 0; i < vessel_event.ghost_count; i++)
  {
    if (vessel_event.ghost_slots[i] <= 0)
    {
      continue;
    }
    if (vessel_event_retire_ship(vessel_event.ghost_slots[i],
                                 "The spectral hull dissolves into cold mist."))
    {
      vessel_event.ghost_slots[i] = 0;
      cleaned++;
    }
  }
  return cleaned;
}

static bool vessel_event_record_leaderboard(const struct vessel_event_participant *participant,
                                            bool winner)
{
  char best_time[32];
  char query[MAX_STRING_LENGTH];

  if (participant == NULL || participant->player_idnum <= 0)
  {
    return TRUE;
  }
  if (participant->finish_seconds > 0)
  {
    snprintf(best_time, sizeof(best_time), "%d", participant->finish_seconds);
  }
  else
  {
    strlcpy(best_time, "NULL", sizeof(best_time));
  }

  snprintf(query, sizeof(query),
           "INSERT INTO vessel_event_leaderboards "
           "(event_type,player_idnum,entries,wins,points,best_time_seconds) "
           "VALUES ('%s',%ld,1,%d,%d,%s) "
           "ON DUPLICATE KEY UPDATE entries=entries+1,wins=wins+VALUES(wins),"
           "points=points+VALUES(points),best_time_seconds=CASE "
           "WHEN VALUES(best_time_seconds) IS NULL THEN best_time_seconds "
           "WHEN best_time_seconds IS NULL OR VALUES(best_time_seconds)<best_time_seconds "
           "THEN VALUES(best_time_seconds) ELSE best_time_seconds END",
           vessel_event_type_name(vessel_event.type), participant->player_idnum, winner ? 1 : 0,
           participant->score, best_time);
  if (mysql_query(conn, query))
  {
    log("SYSERR: Could not update %s leaderboard for player %ld: %s",
        vessel_event_type_name(vessel_event.type), participant->player_idnum, mysql_error(conn));
    return FALSE;
  }
  return TRUE;
}

static bool vessel_event_finish(const char *reason, bool record_scores)
{
  int red_score;
  int blue_score;
  int winning_team;
  int winning_participant;
  int highest_score;
  int highest_count;
  int active_ghosts;
  int cleaned;
  bool records_saved;
  int i;

  if (!vessel_event.active)
  {
    return FALSE;
  }

  active_ghosts = 0;
  for (i = 0; i < vessel_event.ghost_count; i++)
  {
    if (vessel_event.ghost_slots[i] > 0)
    {
      active_ghosts++;
    }
  }
  cleaned = vessel_event_cleanup_ghosts();
  if (cleaned != active_ghosts)
  {
    vessel_event.pending_end = FALSE;
    vessel_event.recovery_required = TRUE;
    vessel_event_set_database_status("recovery_failed", "ghost fleet runtime cleanup failed");
    vessel_event_broadcast(
        "%s event #%llu could not retire every ghost hull; staff recovery is required.",
        vessel_event_type_name(vessel_event.type), vessel_event.event_id);
    return FALSE;
  }

  red_score = 0;
  blue_score = 0;
  winning_participant = -1;
  highest_score = -1;
  highest_count = 0;
  for (i = 0; i < vessel_event.participant_count; i++)
  {
    if (vessel_event.participants[i].team == VESSEL_EVENT_TEAM_RED)
    {
      red_score += vessel_event.participants[i].score;
    }
    else if (vessel_event.participants[i].team == VESSEL_EVENT_TEAM_BLUE)
    {
      blue_score += vessel_event.participants[i].score;
    }

    if (vessel_event.type == VESSEL_EVENT_REGATTA && vessel_event.participants[i].placement == 1)
    {
      winning_participant = i;
    }
    else if (vessel_event.type == VESSEL_EVENT_GHOST_FLEET)
    {
      if (vessel_event.participants[i].score > highest_score)
      {
        highest_score = vessel_event.participants[i].score;
        winning_participant = i;
        highest_count = 1;
      }
      else if (vessel_event.participants[i].score == highest_score)
      {
        highest_count++;
      }
    }
  }
  if (vessel_event.type == VESSEL_EVENT_GHOST_FLEET && highest_count != 1)
  {
    winning_participant = -1;
  }
  winning_team = vessel_event_winning_team(red_score, blue_score);

  records_saved = mysql_query(conn, "START TRANSACTION") == 0;
  if (!records_saved)
  {
    log("SYSERR: Could not begin vessel event completion transaction: %s", mysql_error(conn));
  }
  if (record_scores && records_saved)
  {
    for (i = 0; i < vessel_event.participant_count; i++)
    {
      bool winner;

      winner = i == winning_participant || (vessel_event.type == VESSEL_EVENT_SKIRMISH &&
                                            winning_team != VESSEL_EVENT_TEAM_NONE &&
                                            vessel_event.participants[i].team == winning_team);
      if (!vessel_event_record_leaderboard(&vessel_event.participants[i], winner))
      {
        records_saved = FALSE;
        break;
      }
    }
  }

  if (records_saved)
  {
    records_saved = vessel_event_set_database_status(record_scores ? "completed" : "cancelled",
                                                     reason != NULL ? reason : "event ended");
  }
  if (records_saved && mysql_query(conn, "COMMIT"))
  {
    log("SYSERR: Could not commit vessel event completion: %s", mysql_error(conn));
    records_saved = FALSE;
  }
  if (!records_saved)
  {
    if (mysql_query(conn, "ROLLBACK"))
    {
      log("SYSERR: Could not roll back vessel event completion: %s", mysql_error(conn));
    }
    vessel_event.pending_end = FALSE;
    vessel_event.recovery_required = TRUE;
    vessel_event_set_database_status("recovery_failed", "event score persistence failed");
    vessel_event_broadcast(
        "%s event #%llu could not save final scores; staff recovery is required.",
        vessel_event_type_name(vessel_event.type), vessel_event.event_id);
    return FALSE;
  }
  vessel_event_broadcast("%s event #%llu ended: %s%s", vessel_event_type_name(vessel_event.type),
                         vessel_event.event_id, reason != NULL ? reason : "event ended",
                         cleaned > 0 ? " (ghost fleet retired)" : "");
  vessel_event_clear_state();
  return TRUE;
}

void vessel_event_boot(void)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  int runtime_slots[GREYHAWK_MAXSHIPS];
  int runtime_count;
  int cleanup_failures;
  int i;

  vessel_event_clear_state();
  if (!mysql_available || conn == NULL)
  {
    return;
  }
  vessel_event_ensure_schema();
  if (mysql_query(conn, "SELECT ship_id FROM vessel_event_runtimes "
                        "ORDER BY ship_id"))
  {
    log("SYSERR: Could not load vessel event recovery rows: %s", mysql_error(conn));
    return;
  }
  result = mysql_store_result(conn);
  if (result == NULL)
  {
    return;
  }

  runtime_count = 0;
  while ((row = mysql_fetch_row(result)) != NULL && runtime_count < GREYHAWK_MAXSHIPS)
  {
    runtime_slots[runtime_count++] = row[0] != NULL ? atoi(row[0]) : 0;
  }
  mysql_free_result(result);

  cleanup_failures = 0;
  for (i = 0; i < runtime_count; i++)
  {
    if (!vessel_event_retire_ship(runtime_slots[i], "The interrupted event releases this hull."))
    {
      cleanup_failures++;
    }
  }

  if (cleanup_failures == 0)
  {
    if (mysql_query(conn, "UPDATE vessel_showcase_events SET status='recovered',"
                          "ended_at=UNIX_TIMESTAMP(),end_reason='server restart recovery' "
                          "WHERE status IN ('active','spawning','recovery_failed')"))
    {
      log("SYSERR: Could not close recovered vessel events: %s", mysql_error(conn));
    }
    if (mysql_query(conn, "DELETE FROM vessel_event_runtimes"))
    {
      log("SYSERR: Could not clear recovered vessel event runtimes: %s", mysql_error(conn));
    }
  }
  else
  {
    if (mysql_query(conn, "UPDATE vessel_showcase_events SET status='recovery_failed',"
                          "end_reason='automatic runtime cleanup failed' "
                          "WHERE status IN ('active','spawning','recovery_failed')"))
    {
      log("SYSERR: Could not mark vessel event recovery failure: %s", mysql_error(conn));
    }
  }

  if (runtime_count > 0 || cleanup_failures > 0)
  {
    log("Info: Vessel event boot recovery inspected %d runtime%s with %d failure%s", runtime_count,
        runtime_count == 1 ? "" : "s", cleanup_failures, cleanup_failures == 1 ? "" : "s");
  }
}

static bool vessel_event_current_coordinates(struct char_data *ch, int *x, int *y, int *z)
{
  struct greyhawk_ship_data *ship;
  room_rnum room;

  if (ch == NULL || x == NULL || y == NULL || z == NULL)
  {
    return FALSE;
  }
  ship = get_ship_from_room(IN_ROOM(ch));
  if (ship != NULL)
  {
    *x = (int)ship->x;
    *y = (int)ship->y;
    *z = (int)ship->z;
    return TRUE;
  }

  room = IN_ROOM(ch);
  if (room == NOWHERE)
  {
    return FALSE;
  }
  *x = world[room].coords[0];
  *y = world[room].coords[1];
  *z = 0;
  return TRUE;
}

static bool vessel_event_parse_integer(const char *text, int minimum, int maximum, int *value)
{
  char *end;
  long parsed;

  if (text == NULL || *text == '\0' || value == NULL)
  {
    return FALSE;
  }
  parsed = strtol(text, &end, 10);
  if (*end != '\0' || parsed < minimum || parsed > maximum)
  {
    return FALSE;
  }
  *value = (int)parsed;
  return TRUE;
}

static bool vessel_event_prototype_is_warship(int prototype_id)
{
  char query[256];
  MYSQL_RES *result;
  MYSQL_ROW row;
  bool warship;

  snprintf(query, sizeof(query), "SELECT vessel_class FROM ship_prototypes WHERE prototype_id=%d",
           prototype_id);
  if (mysql_query(conn, query))
  {
    log("SYSERR: Could not inspect ghost fleet prototype %d: %s", prototype_id, mysql_error(conn));
    return FALSE;
  }
  result = mysql_store_result(conn);
  if (result == NULL)
  {
    return FALSE;
  }
  row = mysql_fetch_row(result);
  warship = row != NULL && row[0] != NULL && atoi(row[0]) == VESSEL_WARSHIP;
  mysql_free_result(result);
  return warship;
}

static void vessel_event_start_regatta(struct char_data *ch, const char *arguments)
{
  struct greyhawk_ship_data *ship;
  char finish_x_arg[MAX_INPUT_LENGTH];
  char finish_y_arg[MAX_INPUT_LENGTH];
  const char *remainder;
  int finish_x;
  int finish_y;

  ship = get_ship_from_room(IN_ROOM(ch));
  if (ship == NULL)
  {
    send_to_char(ch, "Start a regatta from an authorized vessel helm.\r\n");
    return;
  }
  if (!is_pilot(ch, ship))
  {
    send_to_char(ch, "You must be authorized at the helm to set the course.\r\n");
    return;
  }

  remainder = any_one_arg_c(arguments, finish_x_arg, sizeof(finish_x_arg));
  any_one_arg_c(remainder, finish_y_arg, sizeof(finish_y_arg));
  if (!vessel_event_parse_integer(finish_x_arg, -1024, 1024, &finish_x) ||
      !vessel_event_parse_integer(finish_y_arg, -1024, 1024, &finish_y) ||
      (finish_x == (int)ship->x && finish_y == (int)ship->y))
  {
    send_to_char(ch, "Usage: vevent start regatta <finish-x> <finish-y>\r\n");
    return;
  }

  if (!vessel_event_begin(VESSEL_EVENT_REGATTA, ch, (int)ship->x, (int)ship->y, finish_x, finish_y,
                          "active"))
  {
    return;
  }
  vessel_event_broadcast("Regatta #%llu opened from (%d,%d) to (%d,%d). Captains may VEVENT JOIN.",
                         vessel_event.event_id, vessel_event.start_x, vessel_event.start_y,
                         vessel_event.finish_x, vessel_event.finish_y);
  send_to_char(ch, "Started vessel regatta event #%llu.\r\n", vessel_event.event_id);
}

static void vessel_event_start_skirmish(struct char_data *ch)
{
  int x;
  int y;
  int z;

  if (!vessel_event_current_coordinates(ch, &x, &y, &z))
  {
    send_to_char(ch, "Your event location could not be resolved.\r\n");
    return;
  }
  if (!vessel_event_begin(VESSEL_EVENT_SKIRMISH, ch, x, y, x, y, "active"))
  {
    return;
  }
  vessel_event_broadcast("Fleet skirmish #%llu opened. Captains may VEVENT JOIN RED or BLUE.",
                         vessel_event.event_id);
  send_to_char(ch, "Started vessel skirmish event #%llu.\r\n", vessel_event.event_id);
}

static void vessel_event_start_ghost_fleet(struct char_data *ch, const char *arguments)
{
  char prototype_arg[MAX_INPUT_LENGTH];
  char count_arg[MAX_INPUT_LENGTH];
  char instance_name[128];
  const char *remainder;
  int prototype_id;
  int requested_count;
  int x;
  int y;
  int z;
  int slot;
  int i;

  remainder = any_one_arg_c(arguments, prototype_arg, sizeof(prototype_arg));
  any_one_arg_c(remainder, count_arg, sizeof(count_arg));
  if (!vessel_event_parse_integer(prototype_arg, 1, INT_MAX, &prototype_id))
  {
    send_to_char(ch, "Usage: vevent start ghost <warship-prototype-id> [count 1-5]\r\n");
    return;
  }
  requested_count = 3;
  if (*count_arg &&
      !vessel_event_parse_integer(count_arg, 1, VESSEL_EVENT_MAX_GHOSTS, &requested_count))
  {
    send_to_char(ch, "Usage: vevent start ghost <warship-prototype-id> [count 1-5]\r\n");
    return;
  }
  if (!vessel_event_prototype_is_warship(prototype_id))
  {
    send_to_char(ch, "Ghost fleets require a valid Warship prototype.\r\n");
    return;
  }
  if (!vessel_event_current_coordinates(ch, &x, &y, &z) || z != 0)
  {
    send_to_char(ch, "Start the ghost fleet from surface wilderness waters.\r\n");
    return;
  }
  if (!vessel_event_begin(VESSEL_EVENT_GHOST_FLEET, ch, x, y, x, y, "spawning"))
  {
    return;
  }

  for (i = 0; i < requested_count; i++)
  {
    snprintf(instance_name, sizeof(instance_name), "Ghost Fleet Wraith %llu-%d",
             vessel_event.event_id, i + 1);
    slot = vessel_spawn_public_from_prototype_at(prototype_id, instance_name, x, y, 0);
    if (slot < 2 || !vessel_event_add_runtime(slot, i + 1))
    {
      if (slot >= 2)
      {
        vessel_event_retire_ship(slot, "The unstable apparition disperses.");
      }
      if (vessel_event_finish("ghost fleet spawn failed", FALSE))
      {
        send_to_char(ch, "Ghost fleet creation failed and was rolled back.\r\n");
      }
      else
      {
        send_to_char(ch, "Ghost fleet creation failed; staff recovery is required.\r\n");
      }
      return;
    }
    vessel_event.ghost_slots[vessel_event.ghost_count++] = slot;
  }
  snprintf(instance_name, sizeof(instance_name),
           "UPDATE vessel_showcase_events SET status='active' "
           "WHERE event_id=%llu AND status='spawning'",
           vessel_event.event_id);
  if (mysql_query(conn, instance_name) || mysql_affected_rows(conn) != 1)
  {
    log("SYSERR: Could not activate ghost fleet event %llu: %s", vessel_event.event_id,
        mysql_error(conn));
    if (vessel_event_finish("ghost fleet activation failed", FALSE))
    {
      send_to_char(ch, "Ghost fleet activation failed and was rolled back.\r\n");
    }
    else
    {
      send_to_char(ch, "Ghost fleet activation failed; staff recovery is required.\r\n");
    }
    return;
  }

  vessel_event_broadcast("Ghost fleet #%llu manifested with %d warships at (%d,%d).",
                         vessel_event.event_id, vessel_event.ghost_count, x, y);
  send_to_char(ch, "Started ghost fleet event #%llu with %d wraiths.\r\n", vessel_event.event_id,
               vessel_event.ghost_count);
}

static void vessel_event_join(struct char_data *ch, const char *team_arg)
{
  struct greyhawk_ship_data *ship;
  int team;

  if (!vessel_event.active)
  {
    send_to_char(ch, "No vessel event is active.\r\n");
    return;
  }
  ship = get_ship_from_room(IN_ROOM(ch));
  if (ship == NULL || !is_pilot(ch, ship))
  {
    send_to_char(ch, "Join from an authorized vessel helm.\r\n");
    return;
  }
  if (vessel_event.type == VESSEL_EVENT_REGATTA &&
      ((int)ship->x != vessel_event.start_x || (int)ship->y != vessel_event.start_y))
  {
    send_to_char(ch, "Regatta entries must be at the starting coordinate (%d,%d).\r\n",
                 vessel_event.start_x, vessel_event.start_y);
    return;
  }

  team = VESSEL_EVENT_TEAM_NONE;
  if (vessel_event.type == VESSEL_EVENT_SKIRMISH)
  {
    if (team_arg != NULL && !strcasecmp(team_arg, "red"))
    {
      team = VESSEL_EVENT_TEAM_RED;
    }
    else if (team_arg != NULL && !strcasecmp(team_arg, "blue"))
    {
      team = VESSEL_EVENT_TEAM_BLUE;
    }
    else
    {
      send_to_char(ch, "Join a skirmish with VEVENT JOIN RED or BLUE.\r\n");
      return;
    }
  }

  if (!vessel_event_add_participant(ship->shipnum, GET_IDNUM(ch), GET_NAME(ch), team))
  {
    send_to_char(ch, "That captain or vessel is already entered, or the roster is full.\r\n");
    return;
  }
  send_to_char(ch, "Entered %s (slot %d) in %s event #%llu%s%s.\r\n", ship->name, ship->shipnum,
               vessel_event_type_name(vessel_event.type), vessel_event.event_id,
               team != VESSEL_EVENT_TEAM_NONE ? " on team " : "",
               team != VESSEL_EVENT_TEAM_NONE ? vessel_event_team_name(team) : "");
}

static void vessel_event_enlist(struct char_data *ch, const char *arguments)
{
  char slot_arg[MAX_INPUT_LENGTH];
  char team_arg[MAX_INPUT_LENGTH];
  const char *remainder;
  int ship_id;
  int team;

  if (!vessel_event.active || vessel_event.type != VESSEL_EVENT_SKIRMISH)
  {
    send_to_char(ch, "A fleet skirmish must be active.\r\n");
    return;
  }
  remainder = any_one_arg_c(arguments, slot_arg, sizeof(slot_arg));
  any_one_arg_c(remainder, team_arg, sizeof(team_arg));
  team = !strcasecmp(team_arg, "red")
             ? VESSEL_EVENT_TEAM_RED
             : (!strcasecmp(team_arg, "blue") ? VESSEL_EVENT_TEAM_BLUE : VESSEL_EVENT_TEAM_NONE);
  if (!vessel_event_parse_integer(slot_arg, 2, GREYHAWK_MAXSHIPS - 1, &ship_id) ||
      team == VESSEL_EVENT_TEAM_NONE || !is_valid_ship(&greyhawk_ships[ship_id]))
  {
    send_to_char(ch, "Usage: vevent enlist <ship-slot> <red|blue>\r\n");
    return;
  }
  if (!vessel_event_add_participant(ship_id, 0, "Staff Fleet", team))
  {
    send_to_char(ch, "That vessel is already entered, or the roster is full.\r\n");
    return;
  }
  send_to_char(ch, "Enlisted %s (slot %d) on team %s.\r\n", greyhawk_ships[ship_id].name, ship_id,
               vessel_event_team_name(team));
}

static void vessel_event_show_status(struct char_data *ch)
{
  struct vessel_event_participant *participant;
  const char *ship_name;
  int red_score;
  int blue_score;
  int active_ghosts;
  int i;
  long long elapsed;

  if (!vessel_event.active)
  {
    send_to_char(ch, "No vessel event is active.\r\n");
    return;
  }

  send_to_char(ch, "Vessel Event #%llu: %s\r\n", vessel_event.event_id,
               vessel_event_type_name(vessel_event.type));
  elapsed = (long long)(time(NULL) - vessel_event.started_at);
  if (elapsed < 0)
  {
    elapsed = 0;
  }
  send_to_char(ch, "Elapsed: %lld seconds\r\n", elapsed);
  if (vessel_event.type == VESSEL_EVENT_REGATTA)
  {
    send_to_char(ch, "Course: (%d,%d) -> (%d,%d)\r\n", vessel_event.start_x, vessel_event.start_y,
                 vessel_event.finish_x, vessel_event.finish_y);
  }
  if (vessel_event.recovery_required)
  {
    send_to_char(ch, "Recovery: staff must retry VEVENT END or VEVENT CANCEL.\r\n");
  }

  red_score = 0;
  blue_score = 0;
  send_to_char(ch, "Participants (%d):\r\n", vessel_event.participant_count);
  for (i = 0; i < vessel_event.participant_count; i++)
  {
    participant = &vessel_event.participants[i];
    ship_name = participant->ship_id >= 0 && participant->ship_id < GREYHAWK_MAXSHIPS &&
                        is_valid_ship(&greyhawk_ships[participant->ship_id])
                    ? greyhawk_ships[participant->ship_id].name
                    : "retired vessel";
    send_to_char(ch, "  slot %-3d %-24s captain %-12s team %-4s score %-4d", participant->ship_id,
                 ship_name, participant->captain_name, vessel_event_team_name(participant->team),
                 participant->score);
    if (participant->finished)
    {
      send_to_char(ch, " FINISHED #%d in %ds", participant->placement, participant->finish_seconds);
    }
    else if (!participant->active)
    {
      send_to_char(ch, " withdrawn");
    }
    send_to_char(ch, "\r\n");
    if (participant->team == VESSEL_EVENT_TEAM_RED)
    {
      red_score += participant->score;
    }
    else if (participant->team == VESSEL_EVENT_TEAM_BLUE)
    {
      blue_score += participant->score;
    }
  }
  if (vessel_event.type == VESSEL_EVENT_SKIRMISH)
  {
    send_to_char(ch, "Team score: red %d, blue %d\r\n", red_score, blue_score);
  }
  if (vessel_event.type == VESSEL_EVENT_GHOST_FLEET)
  {
    active_ghosts = 0;
    for (i = 0; i < vessel_event.ghost_count; i++)
    {
      if (vessel_event.ghost_slots[i] > 0 &&
          is_valid_ship(&greyhawk_ships[vessel_event.ghost_slots[i]]))
      {
        active_ghosts++;
        send_to_char(ch, "  Ghost contact: slot %d, %s\r\n", vessel_event.ghost_slots[i],
                     greyhawk_ships[vessel_event.ghost_slots[i]].name);
      }
    }
    send_to_char(ch, "Ghost fleet remaining: %d of %d\r\n", active_ghosts,
                 vessel_event.ghost_count);
  }
}

static void vessel_event_show_leaderboard(struct char_data *ch, enum vessel_event_type type)
{
  char query[MAX_STRING_LENGTH];
  char display_name[MAX_NAME_LENGTH + 32];
  const char *indexed_name;
  MYSQL_RES *result;
  MYSQL_ROW row;
  long player_idnum;
  int rank;

  snprintf(query, sizeof(query),
           "SELECT board.player_idnum,board.entries,board.wins,board.points,"
           "COALESCE(board.best_time_seconds,0) "
           "FROM vessel_event_leaderboards AS board "
           "WHERE board.event_type='%s' "
           "ORDER BY board.wins DESC,board.points DESC,"
           "board.best_time_seconds IS NULL,board.best_time_seconds,"
           "board.player_idnum LIMIT 10",
           vessel_event_type_name(type));
  if (mysql_query(conn, query))
  {
    log("SYSERR: Could not read %s vessel leaderboard: %s", vessel_event_type_name(type),
        mysql_error(conn));
    send_to_char(ch, "The vessel leaderboard is unavailable.\r\n");
    return;
  }
  result = mysql_store_result(conn);
  if (result == NULL)
  {
    send_to_char(ch, "The vessel leaderboard is unavailable.\r\n");
    return;
  }

  send_to_char(ch, "Vessel Event Leaderboard: %s\r\n", vessel_event_type_name(type));
  rank = 0;
  while ((row = mysql_fetch_row(result)) != NULL)
  {
    player_idnum = row[0] != NULL ? atol(row[0]) : 0;
    indexed_name = get_name_by_id(player_idnum);
    if (indexed_name != NULL && *indexed_name != '\0')
    {
      strlcpy(display_name, indexed_name, sizeof(display_name));
      display_name[0] = UPPER(display_name[0]);
    }
    else
    {
      snprintf(display_name, sizeof(display_name), "Captain #%ld", player_idnum);
    }
    rank++;
    send_to_char(ch, " %2d. %-20s entries %-3s wins %-3s points %-6s", rank, display_name,
                 row[1] != NULL ? row[1] : "0", row[2] != NULL ? row[2] : "0",
                 row[3] != NULL ? row[3] : "0");
    if (row[4] != NULL && atoi(row[4]) > 0)
    {
      send_to_char(ch, " best %ss", row[4]);
    }
    send_to_char(ch, "\r\n");
  }
  if (rank == 0)
  {
    send_to_char(ch, "  No completed entries yet.\r\n");
  }
  mysql_free_result(result);
}

void vessel_event_handle_move(int shipnum, int old_x, int old_y, int new_x, int new_y)
{
  struct vessel_event_participant *participant;
  int placement;
  int i;

  if (!vessel_event.active || vessel_event.type != VESSEL_EVENT_REGATTA ||
      !vessel_event_finish_reached(old_x, old_y, new_x, new_y, vessel_event.finish_x,
                                   vessel_event.finish_y))
  {
    return;
  }
  participant = vessel_event_participant_by_ship(shipnum);
  if (participant == NULL || participant->finished || !participant->active)
  {
    return;
  }

  placement = 1;
  for (i = 0; i < vessel_event.participant_count; i++)
  {
    if (vessel_event.participants[i].finished)
    {
      placement++;
    }
  }
  participant->finished = TRUE;
  participant->placement = placement;
  participant->finish_seconds = MAX(1, (int)(time(NULL) - vessel_event.started_at));
  participant->score += vessel_event_placement_points(placement);
  vessel_event_save_participant(participant, FALSE);
  if (is_valid_ship(&greyhawk_ships[shipnum]))
  {
    send_to_ship(&greyhawk_ships[shipnum],
                 "REGATTA FINISH: place %d, elapsed %d seconds, score %d.", placement,
                 participant->finish_seconds, participant->score);
  }
  vessel_event_broadcast("%s finished regatta #%llu in place %d (%ds).", participant->captain_name,
                         vessel_event.event_id, placement, participant->finish_seconds);
}

void vessel_event_record_damage(int attacker_ship_id, int target_ship_id, int amount)
{
  struct vessel_event_participant *attacker;
  struct vessel_event_participant *target;
  bool ghost_target;
  int i;

  if (!vessel_event.active || amount <= 0)
  {
    return;
  }
  attacker = vessel_event_participant_by_ship(attacker_ship_id);
  if (attacker == NULL || !attacker->active)
  {
    return;
  }

  if (vessel_event.type == VESSEL_EVENT_SKIRMISH)
  {
    target = vessel_event_participant_by_ship(target_ship_id);
    if (target == NULL || !target->active || target->team == attacker->team ||
        attacker->team == VESSEL_EVENT_TEAM_NONE)
    {
      return;
    }
  }
  else if (vessel_event.type == VESSEL_EVENT_GHOST_FLEET)
  {
    ghost_target = FALSE;
    for (i = 0; i < vessel_event.ghost_count; i++)
    {
      if (vessel_event.ghost_slots[i] == target_ship_id)
      {
        ghost_target = TRUE;
        break;
      }
    }
    if (!ghost_target)
    {
      return;
    }
  }
  else
  {
    return;
  }

  attacker->score += amount;
  vessel_event_save_participant(attacker, FALSE);
  if (is_valid_ship(&greyhawk_ships[attacker_ship_id]))
  {
    send_to_ship(&greyhawk_ships[attacker_ship_id], "Event score: +%d damage (%d total).", amount,
                 attacker->score);
  }
}

void vessel_event_handle_sink(int shipnum)
{
  struct vessel_event_participant *participant;
  struct vessel_event_participant *attacker;
  int attacker_ship_id;
  int active_ghosts;
  int i;

  if (!vessel_event.active || shipnum < 0 || shipnum >= GREYHAWK_MAXSHIPS ||
      !is_valid_ship(&greyhawk_ships[shipnum]))
  {
    return;
  }
  attacker_ship_id = greyhawk_ships[shipnum].last_attacker;
  attacker = vessel_event_participant_by_ship(attacker_ship_id);
  participant = vessel_event_participant_by_ship(shipnum);
  if (participant != NULL)
  {
    participant->active = FALSE;
    vessel_event_save_participant(participant, FALSE);
  }

  if (vessel_event.type == VESSEL_EVENT_SKIRMISH && attacker != NULL && attacker->active &&
      participant != NULL && attacker->team != participant->team)
  {
    attacker->score += VESSEL_EVENT_SINK_BONUS;
    vessel_event_save_participant(attacker, FALSE);
  }
  if (vessel_event.type != VESSEL_EVENT_GHOST_FLEET)
  {
    return;
  }

  for (i = 0; i < vessel_event.ghost_count; i++)
  {
    if (vessel_event.ghost_slots[i] == shipnum)
    {
      vessel_event.ghost_slots[i] = 0;
      vessel_event_remove_runtime(shipnum);
      if (attacker != NULL && attacker->active)
      {
        attacker->score += VESSEL_EVENT_SINK_BONUS;
        vessel_event_save_participant(attacker, FALSE);
      }
      break;
    }
  }

  active_ghosts = 0;
  for (i = 0; i < vessel_event.ghost_count; i++)
  {
    if (vessel_event.ghost_slots[i] > 0)
    {
      active_ghosts++;
    }
  }
  if (active_ghosts == 0)
  {
    vessel_event.pending_end = TRUE;
  }
}

void vessel_event_tick(void)
{
  if (!vessel_event.active)
  {
    return;
  }
  if (vessel_event.recovery_required)
  {
    return;
  }
  if (vessel_event.pending_end)
  {
    vessel_event_finish("all ghost fleet objectives defeated", TRUE);
    return;
  }
  if (time(NULL) - vessel_event.started_at >= VESSEL_EVENT_MAX_DURATION)
  {
    vessel_event_finish("one-hour event ceiling reached", TRUE);
  }
}

static void vessel_event_command_usage(struct char_data *ch)
{
  send_to_char(ch, "Usage: vevent status | join [red|blue] | "
                   "leaderboard [regatta|skirmish|ghost]\r\n");
  if (GET_LEVEL(ch) >= LVL_IMMORT)
  {
    send_to_char(ch, "Staff: vevent start regatta <finish-x> <finish-y>\r\n"
                     "       vevent start skirmish\r\n"
                     "       vevent start ghost <warship-prototype-id> [count 1-5]\r\n"
                     "       vevent enlist <ship-slot> <red|blue>\r\n"
                     "       vevent end | cancel | recover\r\n");
  }
}

ACMD(do_vevent)
{
  char action[MAX_INPUT_LENGTH];
  char option[MAX_INPUT_LENGTH];
  const char *remainder;
  enum vessel_event_type type;

  remainder = any_one_arg_c(argument, action, sizeof(action));
  remainder = any_one_arg_c(remainder, option, sizeof(option));
  if (!*action || !strcasecmp(action, "status"))
  {
    vessel_event_show_status(ch);
    return;
  }
  if (!vessel_event_database_ready(ch))
  {
    return;
  }
  if (!strcasecmp(action, "leaderboard") || !strcasecmp(action, "leaders"))
  {
    if (!*option)
    {
      vessel_event_show_leaderboard(ch, VESSEL_EVENT_REGATTA);
      vessel_event_show_leaderboard(ch, VESSEL_EVENT_SKIRMISH);
      vessel_event_show_leaderboard(ch, VESSEL_EVENT_GHOST_FLEET);
      return;
    }
    type = vessel_event_type_from_name(option);
    if (type == VESSEL_EVENT_NONE)
    {
      send_to_char(ch, "Leaderboard types: regatta, skirmish, ghost.\r\n");
      return;
    }
    vessel_event_show_leaderboard(ch, type);
    return;
  }
  if (!strcasecmp(action, "join"))
  {
    vessel_event_join(ch, option);
    return;
  }

  if (GET_LEVEL(ch) < LVL_IMMORT)
  {
    vessel_event_command_usage(ch);
    return;
  }
  if (!strcasecmp(action, "start"))
  {
    type = vessel_event_type_from_name(option);
    if (type == VESSEL_EVENT_REGATTA)
    {
      vessel_event_start_regatta(ch, remainder);
    }
    else if (type == VESSEL_EVENT_SKIRMISH)
    {
      vessel_event_start_skirmish(ch);
    }
    else if (type == VESSEL_EVENT_GHOST_FLEET)
    {
      vessel_event_start_ghost_fleet(ch, remainder);
    }
    else
    {
      vessel_event_command_usage(ch);
    }
    return;
  }
  if (!strcasecmp(action, "enlist"))
  {
    char enlist_arguments[MAX_INPUT_LENGTH * 2];

    snprintf(enlist_arguments, sizeof(enlist_arguments), "%s %s", option,
             remainder != NULL ? remainder : "");
    vessel_event_enlist(ch, enlist_arguments);
    return;
  }
  if (!strcasecmp(action, "end"))
  {
    if (!vessel_event.active)
    {
      send_to_char(ch, "No vessel event is active.\r\n");
    }
    else if (!vessel_event_finish("ended by staff", TRUE))
    {
      send_to_char(ch, "The event remains open because finalization failed; "
                       "retry VEVENT END or VEVENT CANCEL.\r\n");
    }
    else
    {
      send_to_char(ch, "Vessel event completed and scored.\r\n");
    }
    return;
  }
  if (!strcasecmp(action, "cancel"))
  {
    if (!vessel_event.active)
    {
      send_to_char(ch, "No vessel event is active.\r\n");
    }
    else if (!vessel_event_finish("cancelled by staff", FALSE))
    {
      send_to_char(ch, "The event remains open because finalization failed; "
                       "retry VEVENT CANCEL.\r\n");
    }
    else
    {
      send_to_char(ch, "Vessel event cancelled without leaderboard changes.\r\n");
    }
    return;
  }
  if (!strcasecmp(action, "recover"))
  {
    if (vessel_event.active)
    {
      send_to_char(ch, "End or cancel the active event before recovery.\r\n");
      return;
    }
    vessel_event_boot();
    send_to_char(ch, "Vessel event recovery pass completed; review the system log.\r\n");
    return;
  }
  vessel_event_command_usage(ch);
}
