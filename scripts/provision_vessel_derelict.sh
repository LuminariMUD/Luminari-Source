#!/usr/bin/env bash

set -Eeuo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo_root=${LUMINARI_PROJECT_ROOT:-$(cd "$script_dir/.." && pwd)}
package_dir="$repo_root/lib/world/vessel_derelict"
zone_package="$repo_root/lib/world/vessel_harbor/700.zon"
server_unit=luminari-dev-login-smoke.service
server_log="${TMPDIR:-/tmp}/luminari-dev-login-smoke.log"
state_root="${TMPDIR:-/tmp}/luminari-vessel-derelict-${UID}"
run_id="$(date -u +%Y%m%dT%H%M%SZ)-$$"
run_dir="$state_root/runs/$run_id"
work_dir="$run_dir/work"
started_epoch=$(date +%s)
database_host=
database_name=
database_user=
database_password=
mud_port=
restart_needed=false
derelict_name='Blackwake Derelict'
derelict_x=-533
derelict_y=330

mkdir -p "$work_dir"

fail()
{
  printf 'vessel derelict provisioner: %s\n' "$*" >&2
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

database_scalar()
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

apply_database_file()
{
  local sql_file=$1

  MYSQL_PWD="$database_password" mariadb --no-defaults --batch \
    --host="$database_host" --user="$database_user" \
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

recover_server()
{
  local exit_status=$?

  trap - EXIT
  if [[ "$restart_needed" == true ]] && ! port_is_listening; then
    "$script_dir/dev_kohdee_login_smoke.sh" \
      >"$run_dir/recovery-boot.log" 2>&1 || true
  fi
  find "$work_dir" -depth -delete 2>/dev/null || true
  exit "$exit_status"
}
trap recover_server EXIT

ensure_index_entry()
{
  local index_file=$1
  local entry=$2
  local updated_file="$work_dir/index.updated"

  if [[ ! -f "$index_file" ]]; then
    printf '%s\n$\n' "$entry" >"$index_file"
    return
  fi
  if grep -Fqx "$entry" "$index_file"; then
    return
  fi

  awk -v entry="$entry" '
    $0 == "$" && !inserted {
      print entry
      inserted = 1
    }
    {
      print
    }
    END {
      if (!inserted) {
        print entry
        print "$"
      }
    }
  ' "$index_file" >"$updated_file"
  chmod --reference="$index_file" "$updated_file"
  mv "$updated_file" "$index_file"
}

record_title()
{
  local world_file=$1
  local vnum=$2

  awk -v header="#$vnum" '
    $0 == header {
      if (getline title > 0) {
        print title
      }
      exit
    }
  ' "$world_file"
}

assert_world_record_identity()
{
  local kind=$1
  local vnum=$2
  local expected_title=$3
  local destination_dir="$repo_root/lib/world/$kind"
  local package_file="$package_dir/700.$kind"
  local live_file="$destination_dir/700.$kind"
  local candidate
  local actual_title
  local found_count=0

  [[ $(record_title "$package_file" "$vnum") == "$expected_title" ]] ||
    fail "package record $kind $vnum has an unexpected title"

  for candidate in "$destination_dir"/*."$kind"; do
    [[ -e "$candidate" ]] || continue
    actual_title=$(record_title "$candidate" "$vnum")
    [[ -n "$actual_title" ]] || continue
    found_count=$((found_count + 1))
    [[ "$candidate" == "$live_file" ]] ||
      fail "$kind VNUM $vnum already exists in $candidate"
    [[ "$actual_title" == "$expected_title" ]] ||
      fail "$kind VNUM $vnum is already used by '$actual_title'"
  done
  ((found_count <= 1)) || fail "$kind VNUM $vnum is duplicated"
}

assert_base_hull_exists()
{
  local candidate
  local found_count=0

  for candidate in "$repo_root/lib/world/obj"/*.obj; do
    [[ -e "$candidate" ]] || continue
    if [[ -n $(record_title "$candidate" 70002) ]]; then
      found_count=$((found_count + 1))
    fi
  done
  [[ "$found_count" == 1 ]] ||
    fail "expected exactly one live base hull object VNUM 70002"
}

merge_missing_records()
{
  local package_file=$1
  local live_file=$2
  local additions_file="$work_dir/records.add"
  local merged_file="$work_dir/records.merged"

  awk -v live_file="$live_file" '
    BEGIN {
      while ((getline line < live_file) > 0) {
        if (line ~ /^#[0-9]+$/) {
          present[substr(line, 2) + 0] = 1
        }
      }
      close(live_file)
      emit = 0
    }
    /^#[0-9]+$/ {
      emit = ((substr($0, 2) + 0) in present) ? 0 : 1
    }
    /^\$~?$/ {
      emit = 0
      next
    }
    emit {
      print
    }
  ' "$package_file" >"$additions_file"

  [[ -s "$additions_file" ]] || return

  if ! awk -v additions_file="$additions_file" '
    BEGIN {
      while ((getline line < additions_file) > 0) {
        if (line ~ /^#[0-9]+$/) {
          addition_count++
          addition_vnum[addition_count] = substr(line, 2) + 0
        }
        addition[addition_count] = addition[addition_count] line ORS
      }
      close(additions_file)
      next_addition = 1
    }
    function add_records_before(vnum) {
      while (next_addition <= addition_count &&
             addition_vnum[next_addition] < vnum) {
        printf "%s", addition[next_addition]
        next_addition++
      }
    }
    /^#[0-9]+$/ {
      add_records_before(substr($0, 2) + 0)
      print
      next
    }
    /^\$~?$/ && !inserted {
      add_records_before(2147483647)
      inserted = 1
    }
    {
      print
    }
    END {
      if (!inserted) {
        exit 42
      }
    }
  ' "$live_file" >"$merged_file"; then
    fail "$live_file has no world-file terminator"
  fi

  chmod --reference="$live_file" "$merged_file"
  mv "$merged_file" "$live_file"
}

provision_world_file()
{
  local kind=$1
  local destination_dir="$repo_root/lib/world/$kind"
  local package_file="$package_dir/700.$kind"
  local live_file="$destination_dir/700.$kind"

  mkdir -p "$destination_dir"
  if [[ -f "$live_file" ]]; then
    merge_missing_records "$package_file" "$live_file"
  else
    cp "$package_file" "$live_file"
  fi
  ensure_index_entry "$destination_dir/index" "700.$kind"
}

zone_range()
{
  local zone_file=$1

  awk '$1 ~ /^[0-9]+$/ && $2 ~ /^[0-9]+$/ { print $1, $2; exit }' \
    "$zone_file"
}

ensure_vessel_zone_range()
{
  local destination_dir="$repo_root/lib/world/zon"
  local live_file="$destination_dir/700.zon"
  local updated_file="$work_dir/700.zon.updated"
  local range_values
  local range_min
  local range_max

  [[ -f "$zone_package" ]] || fail "missing reserved vessel zone package"
  mkdir -p "$destination_dir"
  if [[ ! -f "$live_file" ]]; then
    cp "$zone_package" "$live_file"
  fi

  [[ $(awk 'NR == 1 { print; exit }' "$live_file") == '#700' ]] ||
    fail "$live_file is not reserved zone 700"
  range_values=$(zone_range "$live_file")
  read -r range_min range_max <<<"$range_values"
  [[ "$range_min" == 70000 ]] ||
    fail "$live_file does not start at reserved VNUM 70000"

  case "$range_max" in
    80019)
      ;;
    79999)
      awk '
        !updated && $1 ~ /^[0-9]+$/ && $2 ~ /^[0-9]+$/ {
          sub(/^[[:space:]]*[0-9]+[[:space:]]+[0-9]+/, "70000 80019")
          updated = 1
        }
        { print }
        END { if (!updated) exit 42 }
      ' "$live_file" >"$updated_file" || fail "could not extend $live_file"
      chmod --reference="$live_file" "$updated_file"
      mv "$updated_file" "$live_file"
      ;;
    *)
      fail "$live_file ends at unexpected VNUM $range_max"
      ;;
  esac
  ensure_index_entry "$destination_dir/index" '700.zon'
}

for command_name in awk chmod cp date find git grep mariadb mkdir mv \
  readlink sha256sum sleep ss systemctl timeout; do
  command -v "$command_name" >/dev/null 2>&1 ||
    fail "required command not found: $command_name"
done

[[ -r "$repo_root/lib/.env" ]] || fail "cannot read lib/.env"
[[ -r "$repo_root/lib/mysql_config" ]] || fail "cannot read lib/mysql_config"
[[ -x "$repo_root/bin/circle" ]] || fail "bin/circle is missing; run make install"
[[ -x "$script_dir/dev_kohdee_login_smoke.sh" ]] ||
  fail "the local character login helper is unavailable"
[[ -f "$package_dir/700.obj" && -f "$package_dir/700.trg" ]] ||
  fail "the derelict world package is incomplete"

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

[[ -z $(git -C "$repo_root" status --porcelain --untracked-files=all) ]] ||
  fail "the source tree must be clean"
if find "$repo_root/src" -type f \( -name '*.c' -o -name '*.h' \) \
  -newer "$repo_root/bin/circle" -print -quit | grep -q .; then
  fail "bin/circle is stale; run make install"
fi

for workload_unit in luminari-vessel-ferry-soak.service \
  luminari-vessel-scale-benchmark.service; do
  systemctl --user is-active --quiet "$workload_unit" &&
    fail "$workload_unit owns the development service"
done

if systemctl --user is-active --quiet "$server_unit"; then
  server_pid=$(systemctl --user show -p MainPID --value "$server_unit")
  [[ "$server_pid" =~ ^[1-9][0-9]*$ ]] || fail "development service has no PID"
  [[ $(readlink -f "/proc/$server_pid/exe") == \
     $(readlink -f "$repo_root/bin/circle") ]] ||
    fail "the running development service uses a different executable"
elif port_is_listening; then
  fail "a manually started process owns development port $mud_port"
fi

assert_base_hull_exists
assert_world_record_identity obj 70010 \
  'blackwake ash-stained captain log ledger~'
assert_world_record_identity obj 70011 \
  'blackwake salt-stiff chart map soundings~'
assert_world_record_identity obj 70012 \
  'blackwake bronze tidefinder gear assembly salvage~'
assert_world_record_identity trg 70010 'Blackwake bridge log discovery~'
assert_world_record_identity trg 70011 'Blackwake quarters chart discovery~'
assert_world_record_identity trg 70012 'Blackwake cargo salvage discovery~'
assert_world_record_identity trg 70013 'Blackwake captain log reading~'
assert_world_record_identity trg 70014 'Blackwake chart study~'

prototype_collision=$(database_scalar "
  SELECT
    (SELECT IF(COUNT(*) > 1, 1, 0)
       FROM ship_prototypes WHERE name = '$derelict_name')
    +
    (SELECT COUNT(*)
       FROM ship_prototypes
      WHERE name = '$derelict_name'
        AND (vessel_class <> 2 OR max_speed <> 6 OR armor <> 15));")
[[ "$prototype_collision" == 0 ]] ||
  fail "the Blackwake prototype name is duplicated or incompatible"

trigger_table_exists=$(database_scalar "
  SELECT COUNT(*)
    FROM information_schema.TABLES
   WHERE TABLE_SCHEMA = DATABASE()
     AND TABLE_NAME = 'ship_room_template_triggers';")
if [[ "$trigger_table_exists" == 1 ]]; then
  trigger_collision=$(database_scalar "
    SELECT COUNT(*)
      FROM ship_room_template_triggers
     WHERE trigger_vnum BETWEEN 70010 AND 70012
       AND NOT (
         vessel_type = 0
         AND ((room_type = 'bridge' AND trigger_vnum = 70010)
           OR (room_type = 'quarters_crew' AND trigger_vnum = 70011)
           OR (room_type = 'cargo_main' AND trigger_vnum = 70012))
       );")
  [[ "$trigger_collision" == 0 ]] ||
    fail "a reserved derelict trigger VNUM has an incompatible mapping"
fi

if [[ -f "$server_log" ]]; then
  server_log_start_lines=$(wc -l <"$server_log")
else
  server_log_start_lines=0
fi

stop_development_mud
restart_needed=true
ensure_vessel_zone_range
provision_world_file obj
provision_world_file trg
apply_database_file "$repo_root/sql/components/vessels_phase11_schema.sql"
apply_database_file "$repo_root/sql/components/vessels_derelict_content.sql"

prototype_id=$(database_scalar "
  SELECT MIN(prototype_id)
    FROM ship_prototypes
   WHERE name = '$derelict_name';")
[[ "$prototype_id" =~ ^[1-9][0-9]*$ ]] ||
  fail "the Blackwake derelict prototype was not created"

derelict_count=$(database_scalar "
  SELECT COUNT(*)
    FROM ship_runtime_state AS runtime
    JOIN ship_interiors AS interior ON interior.ship_id = runtime.ship_id
   WHERE runtime.prototype_id = $prototype_id
      OR interior.vessel_name = '$derelict_name';")
((derelict_count <= 1)) || fail "more than one Blackwake derelict exists"

if [[ "$derelict_count" == 1 ]]; then
  derelict_slot=$(database_scalar "
    SELECT runtime.ship_id
      FROM ship_runtime_state AS runtime
      JOIN ship_interiors AS interior ON interior.ship_id = runtime.ship_id
     WHERE runtime.prototype_id = $prototype_id
       AND interior.vessel_name = '$derelict_name'
       AND interior.owner = '';")
  [[ "$derelict_slot" =~ ^[0-9]+$ ]] ||
    fail "the existing Blackwake hull has incompatible identity or ownership"
  attachment_count=$(database_scalar "
    SELECT
      (SELECT COUNT(*) FROM ship_schedules WHERE ship_id = $derelict_slot)
      +
      (SELECT COUNT(*) FROM ship_crew_roster WHERE ship_id = $derelict_slot);")
  [[ "$attachment_count" == 0 ]] ||
    fail "the existing Blackwake hull has a schedule or assigned crew"
  database_execute "
    UPDATE ship_runtime_state
       SET location_vnum = 0,
           x = $derelict_x,
           y = $derelict_y,
           z = 0,
           dx = 0,
           dy = 0,
           dz = 0,
           speed = 0,
           setspeed = 0,
           autopilot_state = 0,
           current_route_id = 0,
           current_waypoint_index = 0,
           autopilot_tick_counter = 0,
           wait_remaining = 0
     WHERE ship_id = $derelict_slot;"
fi

start_development_mud "$run_dir/01-content-boot.log"

if [[ "$derelict_count" == 0 ]]; then
  timeout 45 "$script_dir/dev_kohdee_login_smoke.sh" --commands \
    "goto $derelict_x $derelict_y" \
    'look' \
    "vedit spawnpublic $prototype_id" \
    'goto 1204' \
    >"$run_dir/02-kohdee-spawn.log" 2>&1 ||
    fail "Kohdee could not spawn the public Blackwake hull"
  grep -Fq 'is public and unclaimed' "$run_dir/02-kohdee-spawn.log" ||
    fail "the staff spawn did not create an unclaimed public hull"
fi

derelict_slot=$(database_scalar "
  SELECT runtime.ship_id
    FROM ship_runtime_state AS runtime
    JOIN ship_interiors AS interior ON interior.ship_id = runtime.ship_id
   WHERE runtime.prototype_id = $prototype_id
     AND interior.vessel_name = '$derelict_name'
     AND interior.owner = '';")
[[ "$derelict_slot" =~ ^[0-9]+$ && "$derelict_slot" -le 500 ]] ||
  fail "the Blackwake public hull is not active"

timeout 45 "$script_dir/dev_kohdee_login_smoke.sh" --commands \
  "shipgoto $derelict_slot" \
  'shipstatus' \
  'north' \
  'look' \
  'south' \
  'east' \
  'look' \
  'west' \
  'south' \
  'look' \
  'north' \
  'goto 1204' \
  >"$run_dir/03-kohdee-interior.log" 2>&1 ||
  fail "Kohdee could not explore the generated Blackwake interior"

for expected_text in \
  "The Bridge of $derelict_name" \
  "Crew Quarters aboard $derelict_name" \
  "Cargo Hold of $derelict_name" \
  "Main Deck of $derelict_name"; do
  grep -Fq "$expected_text" "$run_dir/03-kohdee-interior.log" ||
    fail "Kohdee did not reach '$expected_text'"
done

before_restart_state=$(database_scalar "
  SELECT CONCAT(runtime.ship_id, '|', runtime.prototype_id, '|',
                interior.room_vnums, '|', interior.bridge_room, '|',
                interior.entrance_room, '|', interior.cargo_room1)
    FROM ship_runtime_state AS runtime
    JOIN ship_interiors AS interior ON interior.ship_id = runtime.ship_id
   WHERE runtime.ship_id = $derelict_slot;")

stop_development_mud
restart_needed=true
start_development_mud "$run_dir/04-hard-restart.log"

after_restart_state=$(database_scalar "
  SELECT CONCAT(runtime.ship_id, '|', runtime.prototype_id, '|',
                interior.room_vnums, '|', interior.bridge_room, '|',
                interior.entrance_room, '|', interior.cargo_room1)
    FROM ship_runtime_state AS runtime
    JOIN ship_interiors AS interior ON interior.ship_id = runtime.ship_id
   WHERE runtime.ship_id = $derelict_slot;")
[[ "$after_restart_state" == "$before_restart_state" ]] ||
  fail "the Blackwake identity or interior changed across restart"

runtime_valid=$(database_scalar "
  SELECT IF(
    COUNT(*) = 1
    AND MIN(runtime.prototype_id) = $prototype_id
    AND MIN(interior.vessel_type) = 2
    AND MIN(interior.owner) = ''
    AND MIN(interior.num_rooms) BETWEEN 4 AND 8
    AND MIN(interior.bridge_room) > 0
    AND MIN(interior.entrance_room) > 0
    AND MIN(interior.cargo_room1) > 0
    AND ROUND(MIN(runtime.x)) = $derelict_x
    AND ROUND(MIN(runtime.y)) = $derelict_y
    AND MIN(runtime.speed) = 0
    AND MIN(runtime.autopilot_state) = 0
    AND MIN(runtime.current_route_id) = 0
    AND MIN(runtime.room_types) REGEXP '^[4-8],0,1,2,9(,|$)',
    1, 0)
    FROM ship_runtime_state AS runtime
    JOIN ship_interiors AS interior ON interior.ship_id = runtime.ship_id
   WHERE runtime.ship_id = $derelict_slot;")
[[ "$runtime_valid" == 1 ]] ||
  fail "the restarted Blackwake hull or required rooms are invalid"

mapping_count=$(database_scalar "
  SELECT COUNT(*)
    FROM ship_room_template_triggers
   WHERE vessel_type = 0
     AND (room_type, trigger_vnum) IN (
       ('bridge', 70010),
       ('quarters_crew', 70011),
       ('cargo_main', 70012));")
[[ "$mapping_count" == 3 ]] || fail "the derelict DG mappings are incomplete"

timeout 45 "$script_dir/dev_kohdee_login_smoke.sh" --commands \
  "shipgoto $derelict_slot" \
  'look' \
  'north' \
  'look' \
  'south' \
  'east' \
  'look' \
  'west' \
  'south' \
  'look' \
  'north' \
  'goto 1204' \
  >"$run_dir/05-kohdee-after-restart.log" 2>&1 ||
  fail "Kohdee could not explore the restarted Blackwake interior"

for expected_text in \
  "The Bridge of $derelict_name" \
  "Crew Quarters aboard $derelict_name" \
  "Cargo Hold of $derelict_name" \
  "Main Deck of $derelict_name"; do
  grep -Fq "$expected_text" "$run_dir/05-kohdee-after-restart.log" ||
    fail "the restarted interior is missing '$expected_text'"
done

if [[ -f "$server_log" ]]; then
  tail -n "+$((server_log_start_lines + 1))" "$server_log" \
    >"$run_dir/06-server-log.log"
  if grep -E 'SYSERR:.*(Blackwake|7001[0-4])' \
    "$run_dir/06-server-log.log" >"$run_dir/07-related-syserr.log"; then
    fail "the server logged a Blackwake content SYSERR"
  fi
fi

source_commit=$(git -C "$repo_root" rev-parse HEAD)
binary_sha256=$(sha256sum "$repo_root/bin/circle" | awk '{ print $1 }')
elapsed_seconds=$(($(date +%s) - started_epoch))
{
  printf 'source_commit=%s\n' "$source_commit"
  printf 'binary_sha256=%s\n' "$binary_sha256"
  printf 'prototype_id=%s\n' "$prototype_id"
  printf 'derelict_slot=%s\n' "$derelict_slot"
  printf 'coordinates=%s|%s\n' "$derelict_x" "$derelict_y"
  printf 'before_restart_state=%s\n' "$before_restart_state"
  printf 'after_restart_state=%s\n' "$after_restart_state"
  printf 'mapping_count=%s\n' "$mapping_count"
  printf 'elapsed_seconds=%s\n' "$elapsed_seconds"
} >"$run_dir/result"

find "$work_dir" -depth -delete
trap - EXIT
printf 'PASS: Blackwake derelict provisioned and restart-verified in %ss.\n' \
  "$elapsed_seconds"
printf 'Artifacts: %s\n' "$run_dir"
