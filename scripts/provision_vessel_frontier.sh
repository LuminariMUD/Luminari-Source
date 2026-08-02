#!/usr/bin/env bash

set -Eeuo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo_root=${LUMINARI_PROJECT_ROOT:-$(cd "$script_dir/.." && pwd)}
server_unit=luminari-dev-login-smoke.service
server_log="${TMPDIR:-/tmp}/luminari-dev-login-smoke.log"
state_root="${TMPDIR:-/tmp}/luminari-vessel-frontier-${UID}"
run_id="$(date -u +%Y%m%dT%H%M%SZ)-$$"
run_dir="$state_root/runs/$run_id"
started_epoch=$(date +%s)
database_host=
database_name=
database_user=
database_password=
mud_port=
restart_needed=false
frontier_cleanup_authorized=false

umask 077
mkdir -p "$run_dir"

fail()
{
  printf 'vessel frontier provisioner: %s\n' "$*" >&2
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

database_scalar()
{
  local query=$1

  MYSQL_PWD="$database_password" mariadb --no-defaults --batch \
    --skip-column-names --host="$database_host" --user="$database_user" \
    "$database_name" --execute="$query"
}

frontier_runtime_slots()
{
  database_scalar "
    SELECT COALESCE(GROUP_CONCAT(runtime.ship_id ORDER BY runtime.ship_id
                                SEPARATOR ','), '')
      FROM ship_runtime_state AS runtime
      JOIN ship_prototypes AS prototype
        ON prototype.prototype_id = runtime.prototype_id
     WHERE prototype.name IN (
       'Sablebranch Raft', 'Sablebranch Riverboat',
       'Starfall Survey Ship', 'Starfall Bastion',
       'Aetherwind Courier', 'Starfall Bathyscaphe',
       'Sablebranch Grand Freighter', 'Liminal Wayfarer');"
}

apply_database_file()
{
  local sql_file=$1

  MYSQL_PWD="$database_password" mariadb --no-defaults --batch \
    --abort-source-on-error --host="$database_host" --user="$database_user" \
    "$database_name" <"$sql_file"
}

port_is_listening()
{
  ss -H -ltn "sport = :$mud_port" 2>/dev/null | grep -q .
}

stop_development_mud()
{
  local attempt

  if systemctl --user is-active --quiet "$server_unit"; then
    systemctl --user stop "$server_unit"
  fi
  for ((attempt = 0; attempt < 300; attempt++)); do
    port_is_listening || return 0
    sleep 0.1
  done
  fail "development port $mud_port remained active"
}

start_development_mud()
{
  local output_file=$1

  "$script_dir/dev_kohdee_login_smoke.sh" >"$output_file" 2>&1
  restart_needed=false
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

recover_server()
{
  local exit_status=$?
  local cleanup_status=0
  local remaining_slots
  local runtime_slots
  local ship_slot
  local -a cleanup_commands
  local -a slot_list

  trap - EXIT
  if [[ "$restart_needed" == true ]] && ! port_is_listening; then
    "$script_dir/dev_kohdee_login_smoke.sh" \
      >"$run_dir/recovery-boot.log" 2>&1 || true
  fi
  if [[ "$frontier_cleanup_authorized" == true ]]; then
    runtime_slots=$(frontier_runtime_slots) || cleanup_status=1
    cleanup_commands=('goto 1204')
    if [[ -n "$runtime_slots" ]]; then
      IFS=',' read -r -a slot_list <<<"$runtime_slots"
      for ship_slot in "${slot_list[@]}"; do
        cleanup_commands+=("shippurge $ship_slot")
      done
    fi
    cleanup_commands+=('goto 1204')
    if [[ "$cleanup_status" == 0 ]]; then
      timeout 120 "$script_dir/dev_kohdee_login_smoke.sh" --commands \
        "${cleanup_commands[@]}" >"$run_dir/recovery-cleanup.log" 2>&1 ||
        cleanup_status=1
    fi
    if [[ "$cleanup_status" == 0 ]]; then
      remaining_slots=$(frontier_runtime_slots) || cleanup_status=1
      [[ -z "$remaining_slots" ]] || cleanup_status=1
      grep -Fqx 'Room: 1204' "$repo_root/lib/plrfiles/K-O/kohdee.plr" ||
        cleanup_status=1
    fi
    if [[ "$cleanup_status" != 0 ]]; then
      printf 'vessel frontier provisioner: recovery cleanup failed\n' >&2
    fi
  fi
  printf 'Artifacts: %s\n' "$run_dir" >&2
  exit "$exit_status"
}
trap recover_server EXIT

for command_name in awk date find flock git grep mariadb mkdir sha256sum \
  sleep ss systemctl timeout; do
  command -v "$command_name" >/dev/null 2>&1 ||
    fail "required command not found: $command_name"
done

exec 8>"${TMPDIR:-/tmp}/luminari-vessel-frontier-${UID}.lock"
flock -n 8 || fail "another frontier provisioner is already running"

[[ -r "$repo_root/lib/.env" ]] || fail "cannot read lib/.env"
[[ -r "$repo_root/lib/mysql_config" ]] || fail "cannot read lib/mysql_config"
[[ -x "$repo_root/bin/circle" ]] || fail "bin/circle is missing; run make install"
[[ -x "$script_dir/dev_kohdee_login_smoke.sh" ]] ||
  fail "the local character login helper is unavailable"
[[ -r "$repo_root/sql/components/vessels_frontier_content.sql" ]] ||
  fail "the frontier SQL package is unavailable"

app_environment=$(config_value "$repo_root/lib/.env" APP_ENV)
[[ "$app_environment" == development ]] ||
  fail "refusing to run because APP_ENV is not development"

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

active_workload_unit=$(active_vessel_workload)
[[ -z "$active_workload_unit" ]] ||
  fail "$active_workload_unit owns the development service"
[[ -z $(git -C "$repo_root" status --porcelain --untracked-files=all) ]] ||
  fail "the source worktree must be clean"
stale_binary_input=$(newer_binary_input "$repo_root" "$repo_root/bin/circle") ||
  fail "could not compare the installed MUD with current build inputs"
[[ -z "$stale_binary_input" ]] ||
  fail "bin/circle is older than $stale_binary_input; run make test and make install"

collision_count=$(database_scalar "
  SELECT
    (SELECT COUNT(*) FROM region_data
      WHERE (vnum = 7100101 AND name <> 'Starfall Trench')
         OR (name = 'Starfall Trench' AND vnum <> 7100101)
         OR (vnum = 7100102 AND name <> 'Aetherwind Skyway')
         OR (name = 'Aetherwind Skyway' AND vnum <> 7100102)
         OR (vnum = 7100103 AND name <> 'Shardspire Sky Island')
         OR (name = 'Shardspire Sky Island' AND vnum <> 7100103))
    + (SELECT COUNT(*) FROM path_data
        WHERE (vnum = 7100104 AND name <> 'Sablebranch River')
           OR (name = 'Sablebranch River' AND vnum <> 7100104))
    + (SELECT COUNT(*)
         FROM (
           SELECT name
             FROM ship_prototypes
            WHERE name IN (
              'Sablebranch Raft', 'Sablebranch Riverboat',
              'Starfall Survey Ship', 'Starfall Bastion',
              'Aetherwind Courier', 'Starfall Bathyscaphe',
              'Sablebranch Grand Freighter', 'Liminal Wayfarer')
            GROUP BY name
           HAVING COUNT(*) > 1
         ) AS duplicate_prototypes);")
[[ "$collision_count" == 0 ]] ||
  fail "frontier region, path, or prototype identities collide"

existing_frontier_slots=$(frontier_runtime_slots)
[[ -z "$existing_frontier_slots" ]] ||
  fail "frontier prototypes already have runtime ships: $existing_frontier_slots"
frontier_cleanup_authorized=true

stop_development_mud
restart_needed=true
apply_database_file "$repo_root/sql/components/vessels_frontier_content.sql"
start_development_mud "$run_dir/01-boot.log"

content_valid=$(database_scalar "
  SELECT IF(
    (SELECT COUNT(*)
       FROM region_data AS region
       JOIN region_index AS idx ON idx.vnum = region.vnum
      WHERE region.vnum IN (7100101, 7100102, 7100103)
        AND region.zone_vnum = 10000
        AND region.region_type = CASE region.vnum
              WHEN 7100101 THEN 5
              WHEN 7100102 THEN 6
              WHEN 7100103 THEN 7 END
        AND region.region_props = CASE region.vnum
              WHEN 7100101 THEN 96
              WHEN 7100102 THEN 100
              WHEN 7100103 THEN 200 END
        AND ST_AsText(region.region_polygon) = ST_AsText(idx.region_polygon)) = 3
    AND (SELECT COUNT(*)
           FROM path_data AS path
           JOIN path_index AS idx ON idx.vnum = path.vnum
          WHERE path.vnum = 7100104
            AND path.name = 'Sablebranch River'
            AND path.zone_vnum = 10000
            AND path.path_type = 5
            AND path.path_props = 36
            AND ST_NumPoints(path.path_linestring) = 79
            AND ST_AsText(path.path_linestring) = ST_AsText(
                  bresenham_line(ST_GeomFromText(
                    'LINESTRING(-819 480,-780 480,-780 519)')))
            AND ST_AsText(path.path_linestring) =
                ST_AsText(idx.path_linestring)) = 1
    AND (SELECT COUNT(*)
           FROM ship_prototypes
          WHERE (name = 'Sablebranch Raft' AND vessel_class = 0
                 AND max_speed = 10 AND armor = 5)
             OR (name = 'Sablebranch Riverboat' AND vessel_class = 1
                 AND max_speed = 10 AND armor = 8)
             OR (name = 'Starfall Survey Ship' AND vessel_class = 2
                 AND max_speed = 12 AND armor = 20)
             OR (name = 'Starfall Bastion' AND vessel_class = 3
                 AND max_speed = 15 AND armor = 35)
             OR (name = 'Aetherwind Courier' AND vessel_class = 4
                 AND max_speed = 25 AND armor = 15)
             OR (name = 'Starfall Bathyscaphe' AND vessel_class = 5
                 AND max_speed = 10 AND armor = 25)
             OR (name = 'Sablebranch Grand Freighter' AND vessel_class = 6
                 AND max_speed = 8 AND armor = 20)
             OR (name = 'Liminal Wayfarer' AND vessel_class = 7
                 AND max_speed = 15 AND armor = 20)) = 8,
    1, 0);")
[[ "$content_valid" == 1 ]] ||
  fail "frontier regions, spatial indexes, path, or prototypes are invalid"

frontier_prototype_ids=$(database_scalar "
  SELECT GROUP_CONCAT(
           prototype_id
           ORDER BY CASE name
             WHEN 'Sablebranch Raft' THEN 1
             WHEN 'Sablebranch Riverboat' THEN 2
             WHEN 'Starfall Survey Ship' THEN 3
             WHEN 'Starfall Bastion' THEN 4
             WHEN 'Aetherwind Courier' THEN 5
             WHEN 'Starfall Bathyscaphe' THEN 6
             WHEN 'Sablebranch Grand Freighter' THEN 7
             WHEN 'Liminal Wayfarer' THEN 8
           END SEPARATOR ',')
    FROM ship_prototypes
   WHERE name IN (
     'Sablebranch Raft', 'Sablebranch Riverboat',
     'Starfall Survey Ship', 'Starfall Bastion',
     'Aetherwind Courier', 'Starfall Bathyscaphe',
     'Sablebranch Grand Freighter', 'Liminal Wayfarer');")
[[ "$frontier_prototype_ids" =~ ^([1-9][0-9]*,){7}[1-9][0-9]*$ ]] ||
  fail "could not resolve the eight frontier prototype ids"
IFS=',' read -r raft_prototype_id boat_prototype_id ship_prototype_id \
  warship_prototype_id airship_prototype_id submarine_prototype_id \
  transport_prototype_id magical_prototype_id <<<"$frontier_prototype_ids"

timeout 120 "$script_dir/dev_kohdee_login_smoke.sh" --commands \
  'goto -810 480' \
  'reglist type 5' \
  'reglist type 6' \
  'reglist type 7' \
  'pathlist type 5' \
  'look' \
  'survey' \
  'goto 900 225' \
  'survey' \
  'goto 1204' \
  >"$run_dir/02-kohdee-frontier.log" 2>&1 ||
  fail "the actual Kohdee frontier inspection failed"

for expected_text in \
  'Starfall Trench' \
  'Bathymetric' \
  'Aetherwind Skyway' \
  'AltitudeLane' \
  'Shardspire Sky Island' \
  'SkyIsland' \
  'Sablebranch River' \
  'Terrain Type: River' \
  'Elevation: 24'; do
  grep -Fq "$expected_text" "$run_dir/02-kohdee-frontier.log" ||
    fail "Kohdee did not observe '$expected_text'"
done

grep -Fqx 'Room: 1204' "$repo_root/lib/plrfiles/K-O/kohdee.plr" ||
  fail "Kohdee did not return to room 1204"

timeout 300 "$script_dir/dev_kohdee_login_smoke.sh" \
  --vessel-frontier-check \
  "$raft_prototype_id" "$boat_prototype_id" \
  "$ship_prototype_id" "$warship_prototype_id" \
  "$airship_prototype_id" "$submarine_prototype_id" \
  "$transport_prototype_id" "$magical_prototype_id" \
  >"$run_dir/03-kohdee-vessel-frontier.log" 2>&1 ||
  fail "the actual Kohdee vessel-frontier check failed"

for expected_text in \
  'PASS: raft and riverboat traversed the digitalized Sablebranch River.' \
  'PASS: survey ship crossed Starfall waters with its deck and 12000-lb hold.' \
  'PASS: warship exposed three weapon slots and two weapons decks without firing a shot.' \
  'PASS: bathyscaphe reached depth -90 inside the natural-depth Starfall Trench.' \
  'PASS: airship activated the Aetherwind speed lane and reached Shardspire at altitude 200.' \
  'PASS: transport exposed its 40000-lb hold and three generated cargo rooms.' \
  'PASS: magical vessel crossed plains, river, underwater, and air coordinates.' \
  'PASS: all eight vessel classes passed and every temporary frontier vessel was purged'; do
  grep -Fq "$expected_text" "$run_dir/03-kohdee-vessel-frontier.log" ||
    fail "the vessel-frontier check did not report '$expected_text'"
done

remaining_frontier_slots=$(frontier_runtime_slots)
[[ -z "$remaining_frontier_slots" ]] ||
  fail "temporary frontier ships remained: $remaining_frontier_slots"
grep -Fqx 'Room: 1204' "$repo_root/lib/plrfiles/K-O/kohdee.plr" ||
  fail "Kohdee did not return to room 1204 after piloting the frontier vessels"

if [[ -f "$server_log" ]] &&
   grep -E 'SYSERR:.*(710010[1-4]|Starfall|Aetherwind|Shardspire|Sablebranch|Liminal)' \
     "$server_log" >"$run_dir/04-related-syserr.log"; then
  fail "the server logged a frontier-content SYSERR"
fi

source_commit=$(git -C "$repo_root" rev-parse HEAD)
binary_sha256=$(sha256sum "$repo_root/bin/circle" | awk '{ print $1 }')
prototype_rows=$(database_scalar "
  SELECT GROUP_CONCAT(CONCAT(prototype_id, ':', vessel_class)
                      ORDER BY vessel_class SEPARATOR ',')
    FROM ship_prototypes
   WHERE name IN (
     'Sablebranch Raft', 'Sablebranch Riverboat',
     'Starfall Survey Ship', 'Starfall Bastion',
     'Aetherwind Courier', 'Starfall Bathyscaphe',
     'Sablebranch Grand Freighter', 'Liminal Wayfarer');")
elapsed_seconds=$(($(date +%s) - started_epoch))
{
  printf 'source_commit=%s\n' "$source_commit"
  printf 'binary_sha256=%s\n' "$binary_sha256"
  printf 'region_vnums=7100101,7100102,7100103\n'
  printf 'path_vnum=7100104\n'
  printf 'prototype_rows=%s\n' "$prototype_rows"
  printf 'prototype_ids=%s\n' "$frontier_prototype_ids"
  printf 'trench_center=900,225\n'
  printf 'trench_depth_units=104\n'
  printf 'actual_character=Kohdee\n'
  printf 'actual_behavior_log=03-kohdee-vessel-frontier.log\n'
  printf 'elapsed_seconds=%s\n' "$elapsed_seconds"
} >"$run_dir/result"

frontier_cleanup_authorized=false
trap - EXIT
printf 'PASS: frontier regions, river path, and all eight piloted classes passed '
printf 'through actual Kohdee (%ss).\n' "$elapsed_seconds"
printf 'Artifacts: %s\n' "$run_dir"
