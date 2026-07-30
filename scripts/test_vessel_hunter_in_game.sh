#!/usr/bin/env bash

set -Eeuo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo_root=${LUMINARI_PROJECT_ROOT:-$(cd "$script_dir/.." && pwd)}
server_unit=luminari-dev-login-smoke.service
target_player=Kohdee
target_prototype_name="Harbor Sandbox Hunted Raft"
state_root="${TMPDIR:-/tmp}/luminari-vessel-hunter-check-${UID}"
run_id="$(date -u +%Y%m%dT%H%M%SZ)-$$"
run_dir="$state_root/runs/$run_id"
started_epoch=$(date +%s)
snapshot_ready=false
cleanup_needed=false
acceptance_complete=false
target_slot=
target_prototype_id=
baseline_bounty_count=0
baseline_hunt_count=0
database_host=
database_name=
database_user=
database_password=
mud_port=
candidate_sha256=
restart_old_pid=
restart_new_pid=

mkdir -p "$run_dir"

fail()
{
  printf 'vessel hunter in-game check: %s\n' "$*" >&2
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
    --host="$database_host" --user="$database_user" \
    "$database_name" <"$sql_file"
}

snapshot_table_row()
{
  local table_name=$1
  local where_clause=$2
  local output_file=$3

  MYSQL_PWD="$database_password" mariadb-dump --no-defaults \
    --host="$database_host" --user="$database_user" \
    --single-transaction --skip-lock-tables --skip-add-locks \
    --skip-comments --no-create-info --complete-insert --replace \
    --where="$where_clause" "$database_name" "$table_name" >"$output_file"
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

  for ((attempt = 0; attempt < 600; attempt++)); do
    if systemctl --user is-active --quiet "$server_unit" &&
       port_is_listening; then
      return 0
    fi
    sleep 0.1
  done
  return 1
}

running_binary_sha256()
{
  local server_pid

  server_pid=$(systemctl --user show --property=MainPID --value "$server_unit")
  [[ "$server_pid" =~ ^[1-9][0-9]*$ ]] || return 1
  sha256sum "/proc/$server_pid/exe" | awk '{ print $1 }'
}

start_current_server()
{
  local working_directory
  local running_sha256

  if systemctl --user is-active --quiet "$server_unit"; then
    working_directory=$(
      systemctl --user show --property=WorkingDirectory --value "$server_unit"
    )
    if [[ "$working_directory" != "$repo_root" ]]; then
      systemctl --user stop "$server_unit"
    fi
  fi

  if systemctl --user is-active --quiet "$server_unit"; then
    running_sha256=$(running_binary_sha256)
    if [[ "$running_sha256" != "$candidate_sha256" ]]; then
      systemctl --user restart "$server_unit"
      wait_for_server ||
        fail "the current installed development MUD did not restart"
    fi
  else
    port_is_listening &&
      fail "the development port is owned by an unsupervised process"
    timeout 90 env DEV_MUD_CHARACTER="$target_player" \
      "$script_dir/dev_kohdee_login_smoke.sh" \
      >"$run_dir/00-server-start.log" 2>&1 ||
      fail "the current installed development MUD did not start"
  fi

  wait_for_server || fail "the development MUD is not ready"
  working_directory=$(
    systemctl --user show --property=WorkingDirectory --value "$server_unit"
  )
  [[ "$working_directory" == "$repo_root" ]] ||
    fail "the supervised MUD is running from $working_directory, not $repo_root"
  running_sha256=$(running_binary_sha256)
  [[ "$running_sha256" == "$candidate_sha256" ]] ||
    fail "the running MUD does not match the installed candidate"
}

restart_current_server()
{
  local running_sha256

  restart_old_pid=$(
    systemctl --user show --property=MainPID --value "$server_unit"
  )
  [[ "$restart_old_pid" =~ ^[1-9][0-9]*$ ]] ||
    fail "could not identify the pre-restart MUD process"

  systemctl --user restart "$server_unit" ||
    fail "could not restart the local development MUD"
  wait_for_server ||
    fail "the local development MUD did not return after restart"

  restart_new_pid=$(
    systemctl --user show --property=MainPID --value "$server_unit"
  )
  [[ "$restart_new_pid" =~ ^[1-9][0-9]*$ &&
     "$restart_new_pid" != "$restart_old_pid" ]] ||
    fail "the persistence check did not launch a new MUD process"
  running_sha256=$(running_binary_sha256)
  [[ "$running_sha256" == "$candidate_sha256" ]] ||
    fail "the persistence restart launched a different executable"
}

hunter_state()
{
  database_query "
    SELECT CONCAT(
             hunt.hunter_ship_id, '|',
             hunt.generation, '|',
             hunt.target_ship_id, '|',
             HEX(hunt.hunter_name), '|',
             runtime.last_attacker, '|',
             runtime.prototype_id, '|',
             prototype.vessel_class, '|',
             (
               SELECT COUNT(*)
                 FROM ship_crew_roster AS crew
                WHERE crew.ship_id = hunt.hunter_ship_id
                  AND crew.crew_role = 'pilot'
                  AND crew.npc_vnum = 70002
                  AND crew.status = 'active'
             ), '|',
             IF(interior.owner = '', 1, 0)
           )
      FROM vessel_bounty_hunts AS hunt
      JOIN ship_runtime_state AS runtime
        ON runtime.ship_id = hunt.hunter_ship_id
      JOIN ship_interiors AS interior
        ON interior.ship_id = hunt.hunter_ship_id
      JOIN ship_prototypes AS prototype
        ON prototype.prototype_id = runtime.prototype_id
     WHERE hunt.target_player = '$target_player'
       AND hunt.status = 'active';"
}

run_kohdee_commands()
{
  local output_file=$1

  shift
  timeout 90 env DEV_MUD_CHARACTER="$target_player" \
    "$script_dir/dev_kohdee_login_smoke.sh" --commands "$@" \
    >"$output_file" 2>&1
}

restore_baseline_rows()
{
  database_execute "
    DELETE FROM vessel_bounty_hunts
     WHERE target_player = '$target_player';
    DELETE FROM vessel_bounties
     WHERE player_name = '$target_player';"

  if [[ "$baseline_bounty_count" == 1 ]]; then
    database_apply_file "$run_dir/vessel_bounties-before.sql"
  fi
  if [[ "$baseline_hunt_count" == 1 ]]; then
    database_apply_file "$run_dir/vessel_bounty_hunts-before.sql"
  fi
}

cleanup_test_state()
{
  local current_hunter_slot
  local hunter_identity_count
  local target_identity_count
  local remaining_test_rows

  [[ "$snapshot_ready" == true ]] || return 0

  database_execute "
    INSERT INTO vessel_bounties (player_name, bounty)
    VALUES ('$target_player', 0)
    ON DUPLICATE KEY UPDATE bounty = 0;"

  if ! systemctl --user is-active --quiet "$server_unit"; then
    timeout 90 env DEV_MUD_CHARACTER="$target_player" \
      "$script_dir/dev_kohdee_login_smoke.sh" >/dev/null
  fi

  current_hunter_slot=$(database_query "
    SELECT COALESCE(hunter_ship_id, 0)
      FROM vessel_bounty_hunts
     WHERE target_player = '$target_player';")
  if [[ "$current_hunter_slot" =~ ^[2-9][0-9]*$ &&
        "$current_hunter_slot" -le 500 ]]; then
    hunter_identity_count=$(database_query "
      SELECT COUNT(*)
        FROM vessel_bounty_hunts AS hunt
        JOIN ship_interiors AS interior
          ON interior.ship_id = hunt.hunter_ship_id
       WHERE hunt.target_player = '$target_player'
         AND hunt.hunter_ship_id = $current_hunter_slot
         AND interior.owner = ''
         AND interior.vessel_name = hunt.hunter_name;")
    if [[ "$hunter_identity_count" == 1 ]]; then
      run_kohdee_commands "$run_dir/cleanup-hunter.log" \
        "goto 1000389" "shippurge $current_hunter_slot"
    fi
  fi

  if [[ "$target_slot" =~ ^[2-9][0-9]*$ &&
        "$target_slot" -le 500 &&
        "$target_prototype_id" =~ ^[1-9][0-9]*$ ]]; then
    target_identity_count=$(database_query "
      SELECT COUNT(*)
        FROM ship_runtime_state AS runtime
        JOIN ship_interiors AS interior
          ON interior.ship_id = runtime.ship_id
       WHERE runtime.ship_id = $target_slot
         AND runtime.prototype_id = $target_prototype_id
         AND interior.owner = '$target_player'
         AND interior.vessel_name = '$target_prototype_name';")
    if [[ "$target_identity_count" == 1 ]]; then
      run_kohdee_commands "$run_dir/cleanup-target.log" \
        "goto 1000389" "shippurge $target_slot"
    fi
  fi

  remaining_test_rows=$(database_query "
    SELECT
      (
        SELECT COUNT(*)
          FROM vessel_bounty_hunts
         WHERE target_player = '$target_player'
           AND status IN ('active', 'spawning')
      )
      +
      (
        SELECT COUNT(*)
          FROM ship_runtime_state AS runtime
          JOIN ship_interiors AS interior
            ON interior.ship_id = runtime.ship_id
         WHERE runtime.ship_id = ${target_slot:-0}
           AND runtime.prototype_id = ${target_prototype_id:-0}
           AND interior.owner = '$target_player'
           AND interior.vessel_name = '$target_prototype_name'
      );")
  [[ "$remaining_test_rows" == 0 ]] ||
    return 1

  restore_baseline_rows
  cleanup_needed=false
}

finish()
{
  local exit_status=$?
  local cleanup_status=0
  local elapsed_seconds

  trap - EXIT INT TERM
  if [[ "$cleanup_needed" == true ]]; then
    set +e
    cleanup_test_state >>"$run_dir/cleanup.log" 2>&1
    cleanup_status=$?
    set -e
  fi

  elapsed_seconds=$(($(date +%s) - started_epoch))
  if [[ "$exit_status" == 0 && "$cleanup_status" == 0 &&
        "$acceptance_complete" == true ]]; then
    printf 'PASS elapsed=%s target=%s restart=%s->%s cleanup=restored\n' \
      "$elapsed_seconds" "$target_player" "$restart_old_pid" \
      "$restart_new_pid" >"$run_dir/result"
    printf 'PASS: Kohdee triggered, survived restart with, and was pardoned from one durable bounty-hunter warship (%ss).\n' \
      "$elapsed_seconds"
    printf 'PASS: the temporary target hull was purged and Kohdee'\''s exact bounty/hunt rows were restored.\n'
    printf 'Artifacts: %s\n' "$run_dir"
    exit 0
  fi

  printf 'FAIL elapsed=%s command_status=%s cleanup_status=%s\n' \
    "$elapsed_seconds" "$exit_status" "$cleanup_status" >"$run_dir/result"
  if [[ "$cleanup_status" != 0 ]]; then
    printf 'vessel hunter in-game check: automatic cleanup failed; inspect %s/cleanup.log\n' \
      "$run_dir" >&2
  fi
  printf 'Artifacts: %s\n' "$run_dir" >&2
  if [[ "$exit_status" == 0 ]]; then
    exit 1
  fi
  exit "$exit_status"
}

trap finish EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

for command_name in awk date env flock grep mariadb mariadb-dump mkdir \
  sed sha256sum sleep ss systemctl tail timeout; do
  command -v "$command_name" >/dev/null 2>&1 ||
    fail "required command not found: $command_name"
done

exec 8>"${TMPDIR:-/tmp}/luminari-vessel-hunter-check-${UID}.lock"
flock -n 8 ||
  fail "another bounty-hunter acceptance check is already running"

[[ -r "$repo_root/lib/.env" ]] || fail "cannot read lib/.env"
[[ -r "$repo_root/lib/mysql_config" ]] || fail "cannot read lib/mysql_config"
[[ -x "$repo_root/bin/circle" ]] ||
  fail "bin/circle is missing; run make install first"
[[ -f "$repo_root/src/vessels_hunters.c" ]] ||
  fail "the Phase 15 hunter source is missing"
[[ "$repo_root/bin/circle" -nt "$repo_root/src/vessels_hunters.c" ]] ||
  fail "bin/circle predates the Phase 15 source; run make install"

app_environment=$(config_value "$repo_root/lib/.env" APP_ENV)
[[ "$app_environment" == development ]] ||
  fail "refusing to run because APP_ENV is not development"

configured_character=$(config_value "$repo_root/lib/.env" DEV_MUD_CHARACTER)
[[ -z "$configured_character" ||
   "${configured_character,,}" == "${target_player,,}" ]] ||
  fail "DEV_MUD_CHARACTER must be Kohdee for this acceptance check"

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
[[ "$mud_port" =~ ^[0-9]+$ ]] ||
  fail "could not read the development MUD port"

candidate_sha256=$(sha256sum "$repo_root/bin/circle" | awk '{ print $1 }')
printf 'source=%s\nbinary_sha256=%s\nstarted_at=%s\n' \
  "$repo_root" "$candidate_sha256" "$started_epoch" >"$run_dir/metadata"

start_current_server

[[ -f "$repo_root/lib/world/mob/700.mob" ]] ||
  fail "the live harbor mobile file is missing; run the harbor provisioner"
grep -Fqx "#70002" "$repo_root/lib/world/mob/700.mob" ||
  fail "hunter captain 70002 is not provisioned; run the harbor provisioner"

fixture_valid=$(database_query "
  SELECT IF(
           COUNT(*) = 1
           AND MAX(encounter.region_vnum) = 7000004
           AND MAX(encounter.vessel_class) = 0
           AND MAX(encounter.chance) = 100
           AND MAX(hunter.min_bounty) = 2000
           AND MAX(hunter.pilot_mob_vnum) = 70002
           AND MAX(hunter.enabled) = 1
           AND MAX(prototype.vessel_class) = 3
           AND (
             SELECT COUNT(*)
               FROM ship_prototypes
              WHERE name = '$target_prototype_name'
                AND vessel_class = 0
                AND max_speed = 5
                AND armor = 100
           ) = 1,
           1,
           0
         )
    FROM vessel_hunter_encounters AS hunter
    JOIN vessel_encounters AS encounter
      ON encounter.encounter_id = hunter.encounter_id
    JOIN ship_prototypes AS prototype
      ON prototype.prototype_id = hunter.prototype_id
   WHERE encounter.name = 'Harbor Admiralty hunter patrol';")
[[ "$fixture_valid" == 1 ]] ||
  fail "the Phase 15 harbor fixture is missing or stale; run the harbor provisioner"

target_prototype_id=$(database_query "
  SELECT prototype_id
    FROM ship_prototypes
   WHERE name = '$target_prototype_name'
     AND vessel_class = 0
     AND max_speed = 5
     AND armor = 100;")
[[ "$target_prototype_id" =~ ^[1-9][0-9]*$ ]] ||
  fail "could not identify the durable HUNTED target prototype"

baseline_bounty_count=$(database_query "
  SELECT COUNT(*)
    FROM vessel_bounties
   WHERE player_name = '$target_player';")
baseline_hunt_count=$(database_query "
  SELECT COUNT(*)
    FROM vessel_bounty_hunts
   WHERE target_player = '$target_player';")
[[ "$baseline_bounty_count" =~ ^[01]$ && "$baseline_hunt_count" =~ ^[01]$ ]] ||
  fail "Kohdee has duplicate bounty or hunt rows"

if [[ "$baseline_hunt_count" == 1 ]]; then
  baseline_hunt_status=$(database_query "
    SELECT status
      FROM vessel_bounty_hunts
     WHERE target_player = '$target_player';")
  [[ "$baseline_hunt_status" == cooldown ]] ||
    fail "Kohdee already has a $baseline_hunt_status hunt; do not replace live state"
fi

snapshot_table_row vessel_bounties \
  "player_name = '$target_player'" \
  "$run_dir/vessel_bounties-before.sql"
snapshot_table_row vessel_bounty_hunts \
  "target_player = '$target_player'" \
  "$run_dir/vessel_bounty_hunts-before.sql"
snapshot_ready=true
cleanup_needed=true

database_execute "
  START TRANSACTION;
  DELETE FROM vessel_bounty_hunts
   WHERE target_player = '$target_player';
  INSERT INTO vessel_bounties (player_name, bounty)
  VALUES ('$target_player', 2000)
  ON DUPLICATE KEY UPDATE bounty = 2000;
  COMMIT;"

run_kohdee_commands "$run_dir/01-target-spawn.log" \
  "goto -66 92" "vedit spawn $target_prototype_id" ||
  fail "Kohdee could not spawn the temporary HUNTED target hull"
target_slot=$(sed -n \
  "s/.*as ship \([0-9][0-9]*\):.*/\1/p" \
  "$run_dir/01-target-spawn.log" | tail -n 1)
[[ "$target_slot" =~ ^[2-9][0-9]*$ && "$target_slot" -le 500 ]] ||
  fail "could not read the temporary target fleet slot"

target_valid=$(database_query "
  SELECT COUNT(*)
    FROM ship_runtime_state AS runtime
    JOIN ship_interiors AS interior
      ON interior.ship_id = runtime.ship_id
   WHERE runtime.ship_id = $target_slot
     AND runtime.prototype_id = $target_prototype_id
     AND runtime.speed = 0
     AND interior.owner = '$target_player'
     AND interior.vessel_name = '$target_prototype_name';")
[[ "$target_valid" == 1 ]] ||
  fail "the temporary target hull did not persist with Kohdee ownership"

run_kohdee_commands "$run_dir/02-encounter.log" \
  "shipgoto $target_slot" \
  "speed 1" \
  "vesseldebug encounter" \
  "shipstatus" ||
  fail "the real Kohdee encounter session failed"
grep -Fq "Speed set to 1." "$run_dir/02-encounter.log" ||
  fail "the target hull was not moving"
grep -Fq "A Harbor Admiralty warship bears down" \
  "$run_dir/02-encounter.log" ||
  fail "the production encounter path did not announce the hunter"
grep -Fq "Forced the next normal vessel encounter check." \
  "$run_dir/02-encounter.log" ||
  fail "the encounter cadence hook did not complete"

active_hunt_count=$(database_query "
  SELECT COUNT(*)
    FROM vessel_bounty_hunts
   WHERE target_player = '$target_player'
     AND status = 'active';")
[[ "$active_hunt_count" == 1 ]] ||
  fail "the encounter did not claim one active durable hunt"

initial_hunter_state=$(hunter_state)
IFS='|' read -r hunter_slot hunter_generation hunter_target_slot \
  hunter_name_hex hunter_last_attacker hunter_prototype_id hunter_class \
  hunter_pilot_count hunter_unowned <<<"$initial_hunter_state"
[[ "$hunter_slot" =~ ^[2-9][0-9]*$ && "$hunter_slot" -le 500 &&
   "$hunter_slot" != "$target_slot" ]] ||
  fail "the active lifecycle has an invalid hunter fleet slot"
[[ "$hunter_generation" == 1 &&
   "$hunter_target_slot" == "$target_slot" &&
   -n "$hunter_name_hex" &&
   "$hunter_last_attacker" == "$target_slot" &&
   "$hunter_prototype_id" =~ ^[1-9][0-9]*$ &&
   "$hunter_class" == 3 &&
   "$hunter_pilot_count" == 1 &&
   "$hunter_unowned" == 1 ]] ||
  fail "the hunter hull, pilot, target, or lifecycle identity is incomplete"

restart_current_server

run_kohdee_commands "$run_dir/03-post-restart.log" \
  "shipgoto $target_slot" \
  "@wait 1" \
  "shipstatus" ||
  fail "Kohdee could not rejoin the target hull after restart"
grep -Fq "Aboard $target_prototype_name (slot $target_slot)." \
  "$run_dir/03-post-restart.log" ||
  fail "the target hull did not reconstruct after restart"

post_restart_hunter_state=$(hunter_state)
[[ "$post_restart_hunter_state" == "$initial_hunter_state" ]] ||
  fail "the same hunter generation, slot, pilot, and target did not reattach"

database_execute "
  UPDATE vessel_bounties
     SET bounty = 0
   WHERE player_name = '$target_player';"
run_kohdee_commands "$run_dir/04-pardon.log" \
  "shipgoto $target_slot" \
  "@wait 12" \
  "shipstatus" ||
  fail "the real Kohdee pardon session failed"

pardon_state=$(database_query "
  SELECT CONCAT(
           status, '|',
           IF(hunter_ship_id IS NULL, 1, 0), '|',
           end_reason, '|',
           IF(next_eligible_at > ended_at, 1, 0), '|',
           generation
         )
    FROM vessel_bounty_hunts
   WHERE target_player = '$target_player';")
[[ "$pardon_state" == "cooldown|1|target pardoned|1|1" ]] ||
  fail "the pardon did not close generation 1 into a bounded cooldown"

hunter_rows_remaining=$(database_query "
  SELECT
    (SELECT COUNT(*) FROM ship_runtime_state WHERE ship_id = $hunter_slot)
    +
    (SELECT COUNT(*) FROM ship_interiors WHERE ship_id = $hunter_slot)
    +
    (SELECT COUNT(*) FROM ship_crew_roster WHERE ship_id = $hunter_slot);")
[[ "$hunter_rows_remaining" == 0 ]] ||
  fail "the pardoned hunter left runtime, interior, or pilot persistence"

target_rows_remaining=$(database_query "
  SELECT COUNT(*)
    FROM ship_runtime_state AS runtime
    JOIN ship_interiors AS interior
      ON interior.ship_id = runtime.ship_id
   WHERE runtime.ship_id = $target_slot
     AND runtime.prototype_id = $target_prototype_id
     AND interior.owner = '$target_player';")
[[ "$target_rows_remaining" == 1 ]] ||
  fail "the hunter retired by destroying the protected target fixture"

acceptance_complete=true
