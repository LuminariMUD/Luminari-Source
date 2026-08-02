#!/usr/bin/env bash

set -Eeuo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo_root=${LUMINARI_PROJECT_ROOT:-$(cd "$script_dir/.." && pwd)}
server_unit=luminari-dev-login-smoke.service
server_log="${TMPDIR:-/tmp}/luminari-dev-login-smoke.log"
target_player=Kohdee
player_file="$repo_root/lib/plrfiles/K-O/kohdee.plr"
state_root="${TMPDIR:-/tmp}/luminari-vessel-event-check-${UID}"
run_id="$(date -u +%Y%m%dT%H%M%SZ)-$$"
run_dir="$state_root/runs/$run_id"
started_epoch=$(date +%s)
event_tables=(
  vessel_showcase_events
  vessel_event_participants
  vessel_event_leaderboards
  vessel_event_runtimes
)
database_host=
database_name=
database_user=
database_password=
mud_port=
candidate_sha256=
source_commit=
baseline_player_sha256=
baseline_event_auto_increment=1
baseline_event_highwater=0
player_idnum=
declare -A baseline_entries
declare -A baseline_wins
declare -A baseline_points
raft_prototype_id=
warship_prototype_id=
snapshot_ready=false
cleanup_needed=false
acceptance_complete=false
server_restart_needed=false

umask 077
mkdir -p "$run_dir"

fail()
{
  printf 'vessel event in-game check: %s\n' "$*" >&2
  exit 1
}

config_value()
{
  local config_file=$1
  local requested_key=$2

  awk -v requested_key="$requested_key" '
    /^[[:space:]]*#/ {
      next
    }
    index($0, "=") {
      line = $0
      sub(/^[[:space:]]*/, "", line)
      position = index(line, "=")
      key = substr(line, 1, position - 1)
      value = substr(line, position + 1)
      sub(/[[:space:]]*$/, "", key)
      sub(/^[[:space:]]*/, "", value)
      sub(/[[:space:]]*$/, "", value)
      if (key == requested_key) {
        if ((substr(value, 1, 1) == "\"" &&
             substr(value, length(value), 1) == "\"") ||
            (substr(value, 1, 1) == "\047" &&
             substr(value, length(value), 1) == "\047")) {
          value = substr(value, 2, length(value) - 2)
        }
        print value
        exit
      }
    }
  ' "$config_file"
}

player_file_value()
{
  local tag=$1
  local input_file=$2

  awk -F: -v requested_tag="$tag" '
    $1 == requested_tag {
      value = substr($0, index($0, ":") + 1)
      sub(/^[[:space:]]*/, "", value)
      sub(/[[:space:]]*$/, "", value)
      print value
      found = 1
      exit
    }
    END {
      if (!found) {
        print 0
      }
    }
  ' "$input_file"
}

newer_binary_input()
{
  local input_root=$1
  local binary_path=$2
  local candidate

  [[ -e "$binary_path" ]] || return 2
  for candidate in Makefile Makefile.am CMakeLists.txt configure.ac config.h; do
    if [[ -f "$input_root/$candidate" &&
          "$input_root/$candidate" -nt "$binary_path" ]]; then
      printf '%s\n' "$input_root/$candidate"
      return 0
    fi
  done
  find "$input_root/src" -type f \( -name '*.c' -o -name '*.h' \) \
    -newer "$binary_path" -print -quit
}

database_query()
{
  local query=$1

  MYSQL_PWD="$database_password" mariadb --no-defaults --batch \
    --skip-column-names --host="$database_host" --user="$database_user" \
    "$database_name" --execute="$query"
}

database_execute()
{
  local query=$1

  MYSQL_PWD="$database_password" mariadb --no-defaults --batch \
    --host="$database_host" --user="$database_user" \
    "$database_name" --execute="$query"
}

database_apply_file()
{
  local sql_file=$1

  MYSQL_PWD="$database_password" mariadb --no-defaults --batch \
    --abort-source-on-error --host="$database_host" --user="$database_user" \
    "$database_name" <"$sql_file"
}

database_dump_events()
{
  local output_file=$1
  local temporary_file="$output_file.tmp"

  MYSQL_PWD="$database_password" mariadb-dump --no-defaults \
    --host="$database_host" --user="$database_user" \
    --single-transaction --quick --skip-lock-tables --skip-comments \
    --skip-dump-date --skip-add-locks --no-create-info --skip-triggers \
    "$database_name" "${event_tables[@]}" >"$temporary_file" || return 1
  [[ -s "$temporary_file" ]] || return 1
  mv "$temporary_file" "$output_file"
}

port_is_listening()
{
  ss -H -ltn "sport = :$mud_port" 2>/dev/null | grep -q .
}

active_vessel_workload()
{
  systemctl --user list-units --type=service --state=active \
    --no-legend --plain 2>/dev/null |
    awk '
      $1 ~ /^luminari-vessel-ferry-soak-/ ||
      $1 ~ /^luminari-vessel-scale-benchmark-/ {
        print $1
        exit
      }
    '
}

wait_for_server()
{
  local attempt

  for ((attempt = 0; attempt < 900; attempt++)); do
    if systemctl --user is-active --quiet "$server_unit" &&
       port_is_listening; then
      return 0
    fi
    sleep 0.1
  done
  return 1
}

stop_development_mud()
{
  local attempt

  if systemctl --user is-active --quiet "$server_unit"; then
    systemctl --user stop "$server_unit"
  fi
  for ((attempt = 0; attempt < 300; attempt++)); do
    if ! port_is_listening; then
      server_restart_needed=true
      return 0
    fi
    sleep 0.1
  done
  return 1
}

start_server_without_login()
{
  local attempt
  local launched=false

  : >"$server_log"
  for ((attempt = 0; attempt < 100; attempt++)); do
    systemctl --user reset-failed "$server_unit" 2>/dev/null || true
    if systemd-run --user --quiet --collect \
      --unit="${server_unit%.service}" \
      --property="WorkingDirectory=$repo_root" \
      --property="StandardOutput=append:$server_log" \
      --property="StandardError=append:$server_log" \
      "$repo_root/bin/circle" -d "$repo_root/lib"; then
      launched=true
      break
    fi
    sleep 0.1
  done
  [[ "$launched" == true ]] || return 1
  wait_for_server || return 1
  server_restart_needed=false
}

running_binary_sha256()
{
  local server_pid

  server_pid=$(systemctl --user show --property=MainPID --value "$server_unit")
  [[ "$server_pid" =~ ^[1-9][0-9]*$ ]] || return 1
  sha256sum "/proc/$server_pid/exe" | awk '{ print $1 }'
}

event_prototype_slots()
{
  database_query "
    SELECT COALESCE(GROUP_CONCAT(runtime.ship_id ORDER BY runtime.ship_id
                                SEPARATOR ','), '')
      FROM ship_runtime_state AS runtime
     WHERE runtime.prototype_id IN ($raft_prototype_id, $warship_prototype_id);"
}

run_kohdee_commands()
{
  local output_file=$1

  shift
  timeout 150 env DEV_MUD_CHARACTER="$target_player" \
    "$script_dir/dev_kohdee_login_smoke.sh" --commands "$@" \
    >"$output_file" 2>&1
}

retire_test_runtime()
{
  local slots
  local ship_slot
  local -a slot_list
  local -a cleanup_commands

  if ! systemctl --user is-active --quiet "$server_unit" ||
     ! port_is_listening; then
    start_server_without_login || return 1
  fi

  run_kohdee_commands "$run_dir/cleanup-event.log" \
    'vevent cancel' 'goto 1204' || true
  slots=$(event_prototype_slots) || return 1
  cleanup_commands=('goto 1204')
  if [[ -n "$slots" ]]; then
    IFS=',' read -r -a slot_list <<<"$slots"
    for ship_slot in "${slot_list[@]}"; do
      cleanup_commands+=("shippurge $ship_slot")
    done
  fi
  cleanup_commands+=('goto 1204')
  run_kohdee_commands "$run_dir/cleanup-ships.log" \
    "${cleanup_commands[@]}" || return 1
  [[ -z $(event_prototype_slots) ]] || return 1
  [[ $(database_query 'SELECT COUNT(*) FROM vessel_event_runtimes;') == 0 ]]
}

restore_baseline()
{
  local cleanup_status=0
  local restored_sha256
  local restore_tmp="$repo_root/lib/plrfiles/K-O/.kohdee.plr.event-restore-$$"

  [[ "$snapshot_ready" == true ]] || return 0

  retire_test_runtime || cleanup_status=1
  stop_development_mud || cleanup_status=1
  if [[ "$cleanup_status" == 0 ]]; then
    database_execute "
      SET FOREIGN_KEY_CHECKS = 0;
      DELETE FROM vessel_event_runtimes;
      DELETE FROM vessel_event_participants;
      DELETE FROM vessel_event_leaderboards;
      DELETE FROM vessel_showcase_events;
      SET FOREIGN_KEY_CHECKS = 1;" || cleanup_status=1
    database_apply_file "$run_dir/event-tables-before.sql" || cleanup_status=1
    database_execute "ALTER TABLE vessel_showcase_events
                      AUTO_INCREMENT = $baseline_event_auto_increment;" ||
      cleanup_status=1
  fi

  if cp --preserve=mode,ownership,timestamps \
       "$run_dir/kohdee.plr.before" "$restore_tmp" &&
     mv -f "$restore_tmp" "$player_file"; then
    restored_sha256=$(sha256sum "$player_file" | awk '{ print $1 }')
    [[ "$restored_sha256" == "$baseline_player_sha256" ]] || cleanup_status=1
  else
    cleanup_status=1
  fi

  if [[ "$cleanup_status" == 0 ]]; then
    database_dump_events "$run_dir/event-tables-restored.sql" || cleanup_status=1
    cmp -s "$run_dir/event-tables-before.sql" \
      "$run_dir/event-tables-restored.sql" || cleanup_status=1
  fi
  if [[ "$cleanup_status" == 0 ]]; then
    start_server_without_login || cleanup_status=1
  fi
  if [[ "$cleanup_status" == 0 ]]; then
    [[ $(running_binary_sha256) == "$candidate_sha256" ]] || cleanup_status=1
    cleanup_needed=false
    return 0
  fi
  return 1
}

finish()
{
  local exit_status=$?
  local cleanup_status=0
  local elapsed_seconds

  trap - EXIT INT TERM
  if [[ "$cleanup_needed" == true ]]; then
    set +e
    restore_baseline >"$run_dir/cleanup.log" 2>&1
    cleanup_status=$?
    set -e
  elif [[ "$server_restart_needed" == true ]]; then
    set +e
    start_server_without_login >"$run_dir/cleanup.log" 2>&1
    cleanup_status=$?
    set -e
  fi
  elapsed_seconds=$(($(date +%s) - started_epoch))

  if [[ "$exit_status" == 0 && "$cleanup_status" == 0 &&
        "$acceptance_complete" == true ]]; then
    {
      printf 'PASS source_commit=%s binary_sha256=%s elapsed=%s ' \
        "$source_commit" "$candidate_sha256" "$elapsed_seconds"
      printf 'events=3 runtime_rows=0 cleanup=restored\n'
    } >"$run_dir/result"
    printf 'PASS: Kohdee completed regatta, skirmish, and ghost-fleet events '
    printf 'with durable scores and exact baseline restoration (%ss).\n' \
      "$elapsed_seconds"
    printf 'Artifacts: %s\n' "$run_dir"
    exit 0
  fi

  printf 'FAIL command_status=%s cleanup_status=%s\n' \
    "$exit_status" "$cleanup_status" >"$run_dir/result"
  if [[ "$cleanup_status" != 0 ]]; then
    printf 'vessel event in-game check: cleanup failed; inspect %s/cleanup.log\n' \
      "$run_dir" >&2
  fi
  printf 'Artifacts: %s\n' "$run_dir" >&2
  [[ "$exit_status" != 0 ]] && exit "$exit_status"
  exit 1
}

trap finish EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

for command_name in awk cmp cp date env find flock git grep mariadb \
  mariadb-dump mkdir mv sha256sum sleep ss systemctl systemd-run timeout tr; do
  command -v "$command_name" >/dev/null 2>&1 ||
    fail "required command not found: $command_name"
done

exec 8>"${TMPDIR:-/tmp}/luminari-vessel-event-check-${UID}.lock"
flock -n 8 || fail "another vessel event acceptance check is running"

[[ -r "$repo_root/lib/.env" ]] || fail "cannot read lib/.env"
[[ -r "$repo_root/lib/mysql_config" ]] || fail "cannot read lib/mysql_config"
[[ -x "$repo_root/bin/circle" ]] || fail "bin/circle is missing; run make install"
[[ -x "$script_dir/dev_kohdee_login_smoke.sh" ]] ||
  fail "the local character login helper is unavailable"
[[ -f "$player_file" && ! -L "$player_file" ]] ||
  fail "the Kohdee player file is missing or unsafe to replace"
grep -Fqx "Name: $target_player" "$player_file" ||
  fail "the expected Kohdee identity is not in $player_file"
[[ -r "$repo_root/lib/plrfiles/index" ]] ||
  fail "the player index is unavailable"

app_environment=$(config_value "$repo_root/lib/.env" APP_ENV)
[[ "$app_environment" == development ]] ||
  fail "refusing to run because APP_ENV is not development"

active_workload_unit=$(active_vessel_workload)
[[ -z "$active_workload_unit" ]] ||
  fail "$active_workload_unit owns the installed development MUD"

database_host=$(config_value "$repo_root/lib/mysql_config" mysql_host)
database_name=$(config_value "$repo_root/lib/mysql_config" mysql_database)
database_user=$(config_value "$repo_root/lib/mysql_config" mysql_username)
database_password=$(config_value "$repo_root/lib/mysql_config" mysql_password)
[[ -n "$database_host" && -n "$database_name" && -n "$database_user" ]] ||
  fail "lib/mysql_config is incomplete"

mud_port=$(awk -F= '
  /^[[:space:]]*DFLT_PORT[[:space:]]*=/ {
    value = $2
    gsub(/[[:space:]]/, "", value)
    print value
    exit
  }
' "$repo_root/lib/etc/config")
[[ "$mud_port" =~ ^[0-9]+$ ]] || fail "could not read the development MUD port"

[[ -z $(git -C "$repo_root" status --porcelain --untracked-files=all) ]] ||
  fail "source worktree must be clean before acceptance provenance is recorded"
stale_binary_input=$(newer_binary_input "$repo_root" "$repo_root/bin/circle") ||
  fail "could not compare installed MUD with build inputs"
[[ -z "$stale_binary_input" ]] ||
  fail "bin/circle is older than $stale_binary_input; run make test and make install"

candidate_sha256=$(sha256sum "$repo_root/bin/circle" | awk '{ print $1 }')
source_commit=$(git -C "$repo_root" rev-parse HEAD)

prototype_ids=$(database_query "
  SELECT GROUP_CONCAT(prototype_id ORDER BY vessel_class SEPARATOR ',')
    FROM ship_prototypes
   WHERE (name = 'Sablebranch Raft' AND vessel_class = 0)
      OR (name = 'Starfall Bastion' AND vessel_class = 3);")
[[ "$prototype_ids" =~ ^[1-9][0-9]*,[1-9][0-9]*$ ]] ||
  fail "the raft and warship acceptance prototypes are unavailable"
IFS=',' read -r raft_prototype_id warship_prototype_id <<<"$prototype_ids"
[[ -z $(event_prototype_slots) ]] ||
  fail "an acceptance prototype already has a runtime vessel"

player_idnum=$(player_file_value 'Id  ' "$player_file")
[[ "$player_idnum" =~ ^[1-9][0-9]*$ ]] ||
  fail "could not resolve Kohdee's persistent character id"
indexed_player_idnum=$(awk -v wanted="${target_player,,}" '
  tolower($2) == wanted {
    print $1
    exit
  }
' "$repo_root/lib/plrfiles/index")
[[ "$indexed_player_idnum" == "$player_idnum" ]] ||
  fail "Kohdee's player file and player index ids do not agree"

stop_development_mud || fail "the development MUD did not stop"
database_apply_file "$repo_root/sql/components/vessels_phase16_schema.sql"
database_apply_file "$repo_root/sql/components/help_vessel_entries.sql"

phase16_schema_state=$(database_query "
  SELECT CONCAT(
    (SELECT COUNT(*)
       FROM information_schema.TABLES
      WHERE TABLE_SCHEMA = DATABASE()
        AND TABLE_NAME IN (
          'vessel_showcase_events', 'vessel_event_participants',
          'vessel_event_leaderboards', 'vessel_event_runtimes')),
    '|',
    (SELECT COUNT(*)
       FROM information_schema.COLUMNS
      WHERE TABLE_SCHEMA = DATABASE()
        AND (
          (TABLE_NAME = 'vessel_showcase_events' AND COLUMN_NAME IN (
            'event_id', 'event_type', 'status', 'staff_idnum', 'start_x',
            'start_y', 'finish_x', 'finish_y', 'started_at', 'ended_at',
            'end_reason'))
          OR (TABLE_NAME = 'vessel_event_participants' AND COLUMN_NAME IN (
            'event_id', 'ship_id', 'player_idnum', 'team', 'score',
            'finish_seconds', 'placement', 'status'))
          OR (TABLE_NAME = 'vessel_event_leaderboards' AND COLUMN_NAME IN (
            'event_type', 'player_idnum', 'entries', 'wins', 'points',
            'best_time_seconds'))
          OR (TABLE_NAME = 'vessel_event_runtimes' AND COLUMN_NAME IN (
            'ship_id', 'event_id', 'role', 'ordinal_num'))))
  );")
[[ "$phase16_schema_state" == '4|29' ]] ||
  fail "the Phase 16 schema is incomplete: $phase16_schema_state"

open_events=$(database_query "
  SELECT COUNT(*) FROM vessel_showcase_events
   WHERE status IN ('active', 'spawning', 'recovery_failed');")
[[ "$open_events" == 0 ]] || fail "an existing vessel event needs recovery"

baseline_event_auto_increment=$(database_query "
  SELECT COALESCE(AUTO_INCREMENT, 1)
    FROM information_schema.TABLES
   WHERE TABLE_SCHEMA = DATABASE()
     AND TABLE_NAME = 'vessel_showcase_events';")
baseline_event_highwater=$(database_query \
  'SELECT COALESCE(MAX(event_id), 0) FROM vessel_showcase_events;')
baseline_board=$(database_query "
  SELECT GROUP_CONCAT(CONCAT(
           event_type, ':', entries, ':', wins, ':', points)
           ORDER BY FIELD(event_type, 'regatta', 'skirmish', 'ghost')
           SEPARATOR '|')
    FROM vessel_event_leaderboards
   WHERE player_idnum = $player_idnum
     AND event_type IN ('regatta', 'skirmish', 'ghost');")
for event_type in regatta skirmish ghost; do
  event_row=$(tr '|' '\n' <<<"$baseline_board" |
    awk -F: -v wanted="$event_type" '$1 == wanted { print; exit }')
  if [[ -z "$event_row" ]]; then
    event_row="$event_type:0:0:0"
  fi
  IFS=: read -r _ entries wins points <<<"$event_row"
  baseline_entries[$event_type]=$entries
  baseline_wins[$event_type]=$wins
  baseline_points[$event_type]=$points
done

cp --preserve=mode,ownership,timestamps "$player_file" \
  "$run_dir/kohdee.plr.before"
baseline_player_sha256=$(sha256sum "$run_dir/kohdee.plr.before" |
  awk '{ print $1 }')
database_dump_events "$run_dir/event-tables-before.sql" ||
  fail "could not snapshot the vessel event tables"
snapshot_ready=true
cleanup_needed=true

{
  printf 'source_commit=%s\n' "$source_commit"
  printf 'binary_sha256=%s\n' "$candidate_sha256"
  printf 'player_idnum=%s\n' "$player_idnum"
  printf 'raft_prototype_id=%s\n' "$raft_prototype_id"
  printf 'warship_prototype_id=%s\n' "$warship_prototype_id"
  printf 'baseline_event_highwater=%s\n' "$baseline_event_highwater"
  printf 'baseline_player_sha256=%s\n' "$baseline_player_sha256"
} >"$run_dir/metadata"

start_server_without_login || fail "the current development MUD did not start"
[[ $(running_binary_sha256) == "$candidate_sha256" ]] ||
  fail "the running MUD does not match the installed candidate"

timeout 120 env DEV_MUD_CHARACTER="$target_player" \
  "$script_dir/dev_kohdee_login_smoke.sh" --help-check VEVENT \
  >"$run_dir/01-vevent-help.log" 2>&1 ||
  fail "Kohdee could not read the authoritative VEVENT help"

timeout 240 env DEV_MUD_CHARACTER="$target_player" \
  "$script_dir/dev_kohdee_login_smoke.sh" --vessel-event-check \
  "$raft_prototype_id" "$warship_prototype_id" \
  >"$run_dir/02-kohdee-vessel-events.log" 2>&1 ||
  fail "the actual Kohdee vessel-event session failed"

for expected_text in \
  'PASS: regatta movement recorded first place' \
  'PASS: two-team fleet skirmish recorded live naval damage' \
  'PASS: three ghost warships spawned, scored, and retired' \
  'PASS: all vessel showcase events passed'; do
  grep -Fq "$expected_text" "$run_dir/02-kohdee-vessel-events.log" ||
    fail "the event session did not report '$expected_text'"
done

new_event_state=$(database_query "
  SELECT CONCAT(
    COUNT(*), '|',
    SUM(event_type = 'regatta' AND status = 'completed'), '|',
    SUM(event_type = 'skirmish' AND status = 'completed'), '|',
    SUM(event_type = 'ghost' AND status = 'completed'))
  FROM vessel_showcase_events
  WHERE event_id > $baseline_event_highwater;")
[[ "$new_event_state" == '3|1|1|1' ]] ||
  fail "the three completed event records are inconsistent: $new_event_state"

participant_state=$(database_query "
  SELECT CONCAT(
    SUM(event.event_type = 'regatta' AND participant.placement = 1
        AND participant.score = 100), '|',
    SUM(event.event_type = 'skirmish' AND participant.score > 0), '|',
    SUM(event.event_type = 'ghost' AND participant.score > 0))
  FROM vessel_event_participants AS participant
  JOIN vessel_showcase_events AS event
    ON event.event_id = participant.event_id
  WHERE event.event_id > $baseline_event_highwater
    AND participant.player_idnum = $player_idnum;")
[[ "$participant_state" == '1|1|1' ]] ||
  fail "Kohdee's event results are inconsistent: $participant_state"

for event_type in regatta skirmish ghost; do
  leaderboard_state=$(database_query "
    SELECT CONCAT(entries, '|', wins, '|', points)
      FROM vessel_event_leaderboards
     WHERE event_type = '$event_type'
       AND player_idnum = $player_idnum;")
  IFS='|' read -r entries wins points <<<"$leaderboard_state"
  ((entries == baseline_entries[$event_type] + 1)) ||
    fail "$event_type leaderboard entry count did not advance once"
  ((wins == baseline_wins[$event_type] + 1)) ||
    fail "$event_type leaderboard win count did not advance once"
  ((points > baseline_points[$event_type])) ||
    fail "$event_type leaderboard points did not advance"
done

[[ $(database_query 'SELECT COUNT(*) FROM vessel_event_runtimes;') == 0 ]] ||
  fail "ghost runtime ownership rows remained"
[[ $(database_query "
  SELECT COUNT(*) FROM ship_interiors
   WHERE vessel_name LIKE 'Ghost Fleet Wraith %';") == 0 ]] ||
  fail "a ghost fleet hull remained in persistence"
[[ -z $(event_prototype_slots) ]] || fail "a temporary event vessel remained"
grep -Fqx 'Room: 1204' "$player_file" ||
  fail "Kohdee did not return to room 1204"

database_apply_file "$repo_root/sql/components/verify_vessels_phase16.sql" \
  >"$run_dir/03-phase16-verification.log"
if [[ -f "$server_log" ]] &&
   grep -E 'SYSERR:.*(vessel event|ghost fleet|vessel_showcase|vessel_event_)' \
     "$server_log" >"$run_dir/04-related-syserr.log"; then
  fail "the server logged a Phase 16 event SYSERR"
fi

acceptance_complete=true
