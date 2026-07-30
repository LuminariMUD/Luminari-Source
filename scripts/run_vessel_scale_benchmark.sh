#!/usr/bin/env bash

set -euo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(cd "$script_dir/.." && pwd)
state_root="${TMPDIR:-/tmp}/luminari-vessel-scale-benchmark-${UID}"
current_pointer="$state_root/current"
server_unit="luminari-dev-login-smoke.service"
server_log="${TMPDIR:-/tmp}/luminari-dev-login-smoke.log"
benchmark_prefix="__Vessel Scale Benchmark"
snapshot_tables=(
  freight_contracts
  port_commodities
  ship_cargo_manifest
  ship_crew_roster
  ship_docking
  ship_interiors
  ship_prototypes
  ship_route_waypoints
  ship_routes
  ship_runtime_state
  ship_schedules
  ship_waypoints
  ship_weapons
  trade_commodities
  vessel_bounties
  vessel_encounters
  vessel_insurance_claims
)

fail()
{
  printf 'vessel scale benchmark: %s\n' "$*" >&2
  exit 1
}

usage()
{
  printf '%s\n' \
    "Usage:" \
    "  ./scripts/run_vessel_scale_benchmark.sh start [measurement_seconds]" \
    "  ./scripts/run_vessel_scale_benchmark.sh status" \
    "  ./scripts/run_vessel_scale_benchmark.sh cleanup" >&2
  exit 1
}

metadata_value()
{
  local metadata_file=$1
  local requested_key=$2

  awk -F= -v requested_key="$requested_key" '
    $1 == requested_key {
      print substr($0, index($0, "=") + 1)
      exit
    }
  ' "$metadata_file"
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

active_ferry_soak_unit()
{
  systemctl --user list-units --type=service --state=active --no-legend \
    "luminari-vessel-ferry-soak-*" 2>/dev/null |
    awk 'NR == 1 { print $1; exit }'
}

load_database_config()
{
  [[ -r "$repo_root/lib/.env" ]] || return 1
  [[ -r "$repo_root/lib/mysql_config" ]] || return 1

  app_environment=$(config_value "$repo_root/lib/.env" APP_ENV)
  [[ "$app_environment" == development ]] || return 1

  database_host=$(config_value "$repo_root/lib/mysql_config" mysql_host)
  database_name=$(config_value "$repo_root/lib/mysql_config" mysql_database)
  database_user=$(config_value "$repo_root/lib/mysql_config" mysql_username)
  database_password=$(config_value "$repo_root/lib/mysql_config" mysql_password)
  [[ -n "$database_host" && -n "$database_name" && -n "$database_user" ]]
}

database_query()
{
  local query=$1

  MYSQL_PWD="$database_password" mariadb --no-defaults --batch \
    --skip-column-names --host="$database_host" --user="$database_user" \
    "$database_name" --execute="$query"
}

database_apply()
{
  MYSQL_PWD="$database_password" mariadb --no-defaults --batch \
    --host="$database_host" --user="$database_user" "$database_name"
}

write_status()
{
  local run_dir=$1

  shift
  printf '%s\n' "$*" >"$run_dir/status"
}

parse_live_system_samples()
{
  local input_file=$1
  local output_file=$2

  awk '
    BEGIN {
      OFS = "\t"
      print "epoch", "label", "fleet", "dynamic_rooms", "dynamic_capacity", \
            "mobiles", "objects", "rooms", "lists", "movement_trails", \
            "buffer_switches", "buffer_overflows"
    }
    function clear_sample() {
      fleet = dynamic_rooms = dynamic_capacity = ""
      mobiles = objects = rooms = lists = ""
      movement_trails = buffer_switches = buffer_overflows = ""
    }
    function emit_sample() {
      if (!active) {
        return
      }
      if (fleet == "" || dynamic_rooms == "" || dynamic_capacity == "" ||
          mobiles == "" || objects == "" || rooms == "" || lists == "" ||
          movement_trails == "" || buffer_switches == "" ||
          buffer_overflows == "") {
        print "incomplete live-system sample: " label > "/dev/stderr"
        invalid = 1
      } else {
        print epoch, label, fleet, dynamic_rooms, dynamic_capacity, mobiles, \
              objects, rooms, lists, movement_trails, buffer_switches, \
              buffer_overflows
      }
      active = 0
    }
    /^# checkpoint epoch=[0-9]+ label=system-[0-9]+$/ {
      emit_sample()
      checkpoint = $0
      sub(/^# checkpoint epoch=/, "", checkpoint)
      split(checkpoint, fields, " label=")
      epoch = fields[1]
      label = fields[2]
      clear_sample()
      active = 1
      next
    }
    active && /^[0-9]+ of 500 active fleet slots in use\.$/ {
      fleet = $1
      next
    }
    active &&
      /^Wilderness dynamic room pool: [0-9]+\/[0-9]+ occupied \([0-9]+%\)$/ {
      value = $0
      sub(/^Wilderness dynamic room pool: /, "", value)
      split(value, fields, /[\/[:space:]]+/)
      dynamic_rooms = fields[1]
      dynamic_capacity = fields[2]
      next
    }
    active && /^[[:space:]]*[0-9]+ mobiles/ {
      mobiles = $1
      next
    }
    active && /^[[:space:]]*[0-9]+ objects/ {
      objects = $1
      next
    }
    active && /^[[:space:]]*[0-9]+ rooms/ {
      rooms = $1
      next
    }
    active && /^[[:space:]]*[0-9]+ lists$/ {
      lists = $1
      next
    }
    active && /^[[:space:]]*[0-9]+ movement trails$/ {
      movement_trails = $1
      next
    }
    active &&
      /^[[:space:]]*[0-9]+ buf switches[[:space:]]+[0-9]+ overflows$/ {
      value = $0
      sub(/^[[:space:]]*/, "", value)
      split(value, fields, /[[:space:]]+/)
      buffer_switches = fields[1]
      buffer_overflows = fields[4]
      next
    }
    END {
      emit_sample()
      if (invalid) {
        exit 2
      }
    }
  ' "$input_file" >"$output_file"
}

count_high_volume_progress_logs()
{
  local input_file=$1
  local progress_pattern

  [[ -r "$input_file" ]] || return 2
  progress_pattern="Info: Ship [0-9]+ (departing room|arrived at waypoint|"
  progress_pattern+="wait complete|completed route)"
  progress_pattern+="|\\[VESSEL_(MOVE|AUTO)\\].*"
  progress_pattern+="(departing room|position updated|arrived at waypoint|"
  progress_pattern+="wait complete|completed route)"
  grep -Eic "$progress_pattern" "$input_file" || true
}

validate_live_system_samples()
{
  local input_file=$1
  local expected_final_label=$2

  [[ -r "$input_file" ]] || return 2
  [[ "$expected_final_label" =~ ^[1-9][0-9]*$ ]] || return 2

  awk -F '\t' -v expected_final="$expected_final_label" '
    function reject(message) {
      print "invalid live-system series: " message > "/dev/stderr"
      invalid = 1
      exit 2
    }
    function is_uint(value) {
      return value ~ /^[0-9]+$/
    }
    NR == 1 {
      expected[1] = "epoch"
      expected[2] = "label"
      expected[3] = "fleet"
      expected[4] = "dynamic_rooms"
      expected[5] = "dynamic_capacity"
      expected[6] = "mobiles"
      expected[7] = "objects"
      expected[8] = "rooms"
      expected[9] = "lists"
      expected[10] = "movement_trails"
      expected[11] = "buffer_switches"
      expected[12] = "buffer_overflows"
      if (NF != 12) {
        reject("header field count")
      }
      for (field = 1; field <= 12; field++) {
        if ($field != expected[field]) {
          reject("header field " field)
        }
      }
      next
    }
    {
      if (NF != 12 || !is_uint($1)) {
        reject("sample shape or epoch")
      }
      for (field = 3; field <= 12; field++) {
        if (!is_uint($field)) {
          reject("nonnumeric metric")
        }
      }
      label = $2
      if (label !~ /^system-[0-9]+$/) {
        reject("checkpoint label")
      }
      sub(/^system-/, "", label)
      label += 0
      sample_count++
      if (sample_count == 1) {
        if (label != 0) {
          reject("first checkpoint is not system-0")
        }
        capacity = $5
      } else {
        if ($1 <= previous_epoch) {
          reject("epochs are not strictly increasing")
        }
        if (label <= previous_label) {
          reject("labels are not strictly increasing")
        }
      }
      if (label > expected_final) {
        reject("checkpoint exceeds requested duration")
      }
      if (label != 0 && label != expected_final && label % 3600 != 0) {
        reject("intermediate checkpoint is not hourly")
      }
      if ($3 != 500 || $4 > $5 || $5 != capacity || $12 != 0) {
        reject("fleet, room-capacity, or buffer invariant")
      }
      previous_epoch = $1
      previous_label = label
    }
    END {
      if (invalid) {
        exit 2
      }
      expected_count = 2 + int((expected_final - 1) / 3600)
      if (sample_count != expected_count) {
        print "invalid live-system series: checkpoint count" > "/dev/stderr"
        exit 2
      }
      if (previous_label != expected_final) {
        print "invalid live-system series: final checkpoint label" > "/dev/stderr"
        exit 2
      }
    }
  ' "$input_file"
}

restore_snapshot()
{
  local run_dir=$1
  local baseline_count
  local restored_count
  local restored_marker_count

  [[ -s "$run_dir/vessel-database-before.sql" ]] || return 0
  [[ ! -e "$run_dir/cleanup.complete" ]] || return 0
  load_database_config || return 1

  printf 'Restoring the pre-benchmark vessel database.\n' \
    >>"$run_dir/cleanup.log"
  if systemctl --user is-active --quiet "$server_unit"; then
    systemctl --user stop "$server_unit" >>"$run_dir/cleanup.log" 2>&1 ||
      return 1
  fi

  MYSQL_PWD="$database_password" mariadb --no-defaults --batch \
    --host="$database_host" --user="$database_user" "$database_name" \
    <"$run_dir/vessel-database-before.sql" >>"$run_dir/cleanup.log" 2>&1 ||
    return 1

  baseline_count=$(metadata_value "$run_dir/metadata" baseline_vessels)
  [[ "$baseline_count" =~ ^[0-9]+$ ]] || return 1
  restored_count=$(database_query "SELECT COUNT(*) FROM ship_runtime_state;") ||
    return 1
  [[ "$restored_count" == "$baseline_count" ]] || return 1

  restored_marker_count=$(database_query "
    SELECT
      (SELECT COUNT(*) FROM ship_prototypes
        WHERE LEFT(name, LENGTH('${benchmark_prefix}')) = '${benchmark_prefix}') +
      (SELECT COUNT(*) FROM ship_routes
        WHERE LEFT(name, LENGTH('${benchmark_prefix}')) = '${benchmark_prefix}') +
      (SELECT COUNT(*) FROM ship_waypoints
        WHERE LEFT(name, LENGTH('${benchmark_prefix}')) = '${benchmark_prefix}') +
      (SELECT COUNT(*) FROM vessel_encounters
        WHERE LEFT(name, LENGTH('${benchmark_prefix}')) = '${benchmark_prefix}');") ||
    return 1
  [[ "$restored_marker_count" == 0 ]] || return 1

  "$script_dir/dev_kohdee_login_smoke.sh" --commands "goto 1000389" \
    >>"$run_dir/cleanup.log" 2>&1 || return 1
  printf 'restored_at=%s\n' "$(date +%s)" >"$run_dir/cleanup.complete"
  printf 'Restored %s baseline vessel rows and restarted local development.\n' \
    "$restored_count" >>"$run_dir/cleanup.log"
}

start_run()
{
  local measurement_seconds=${1:-660}
  local existing_dir
  local existing_metadata
  local existing_unit
  local run_id
  local run_dir
  local unit_name
  local pointer_tmp
  local soak_unit

  (($# <= 1)) || usage
  [[ "$measurement_seconds" =~ ^[0-9]+$ ]] ||
    fail "measurement_seconds must be an integer"
  ((measurement_seconds >= 600 && measurement_seconds <= 7200)) ||
    fail "measurement_seconds must be between 600 and 7200"

  for command_name in date mkdir mv systemctl systemd-run; do
    command -v "$command_name" >/dev/null 2>&1 ||
      fail "required command not found: $command_name"
  done

  soak_unit=$(active_ferry_soak_unit)
  [[ -z "$soak_unit" ]] ||
    fail "refusing to disturb active ferry soak $soak_unit"

  umask 077
  mkdir -p "$state_root/runs"
  if [[ -r "$current_pointer" ]]; then
    existing_dir=$(<"$current_pointer")
    existing_metadata="$existing_dir/metadata"
    if [[ -r "$existing_metadata" ]]; then
      existing_unit=$(metadata_value "$existing_metadata" unit)
      if [[ -n "$existing_unit" ]] &&
         systemctl --user is-active --quiet "$existing_unit"; then
        fail "a vessel scale benchmark is already active: $existing_unit"
      fi
    fi
    if [[ -s "$existing_dir/vessel-database-before.sql" &&
          ! -e "$existing_dir/cleanup.complete" ]]; then
      fail "the previous run still needs cleanup: $existing_dir"
    fi
  fi

  run_id="$(date -u +%Y%m%dT%H%M%SZ)-$$"
  run_dir="$state_root/runs/$run_id"
  unit_name="luminari-vessel-scale-benchmark-${run_id}.service"
  mkdir "$run_dir"
  {
    printf 'run_id=%s\n' "$run_id"
    printf 'run_dir=%s\n' "$run_dir"
    printf 'unit=%s\n' "$unit_name"
    printf 'measurement_seconds=%s\n' "$measurement_seconds"
    printf 'requested_at=%s\n' "$(date +%s)"
  } >"$run_dir/metadata"
  write_status "$run_dir" "STARTING"

  pointer_tmp="$state_root/current.$$"
  printf '%s\n' "$run_dir" >"$pointer_tmp"
  mv "$pointer_tmp" "$current_pointer"

  if ! systemd-run --user --quiet --collect \
    --unit="${unit_name%.service}" \
    --property="KillMode=control-group" \
    --property="WorkingDirectory=$repo_root" \
    --property="TimeoutStopSec=600" \
    "$script_dir/run_vessel_scale_benchmark.sh" __run \
    "$run_dir" "$measurement_seconds"; then
    write_status "$run_dir" "LAUNCH_FAILED"
    fail "could not launch $unit_name"
  fi

  printf 'Started vessel scale benchmark: unit=%s\n' "$unit_name"
  printf 'Run directory: %s\n' "$run_dir"
  printf 'Steady measurement window: %s seconds\n' "$measurement_seconds"
}

show_status()
{
  local run_dir
  local metadata_file
  local unit_name
  local unit_state

  [[ -r "$current_pointer" ]] ||
    fail "no vessel scale benchmark has been started"
  run_dir=$(<"$current_pointer")
  metadata_file="$run_dir/metadata"
  [[ -r "$metadata_file" ]] ||
    fail "current run metadata is missing: $run_dir"

  unit_name=$(metadata_value "$metadata_file" unit)
  unit_state=$(systemctl --user is-active "$unit_name" 2>/dev/null || true)
  [[ -n "$unit_state" ]] || unit_state="not-found"

  printf 'Unit: %s (%s)\n' "$unit_name" "$unit_state"
  printf 'Run directory: %s\n' "$run_dir"
  printf 'Status: '
  if [[ -r "$run_dir/status" ]]; then
    sed -n '1,3p' "$run_dir/status"
  else
    printf 'UNKNOWN\n'
  fi
  if [[ -s "$run_dir/summary.txt" ]]; then
    printf '\n'
    sed -n '1,80p' "$run_dir/summary.txt"
  elif [[ -s "$run_dir/monitor.log" ]]; then
    printf '\nRecent monitor output:\n'
    tail -n 20 "$run_dir/monitor.log"
  fi
}

cleanup_run()
{
  local run_dir
  local metadata_file
  local unit_name
  local attempt

  [[ -r "$current_pointer" ]] ||
    fail "no vessel scale benchmark has been started"
  run_dir=$(<"$current_pointer")
  metadata_file="$run_dir/metadata"
  [[ -r "$metadata_file" ]] ||
    fail "current run metadata is missing: $run_dir"
  unit_name=$(metadata_value "$metadata_file" unit)

  if [[ -n "$unit_name" ]] &&
     systemctl --user is-active --quiet "$unit_name"; then
    systemctl --user stop --no-block "$unit_name"
  fi

  for ((attempt = 0; attempt < 600; attempt++)); do
    [[ -e "$run_dir/cleanup.complete" ]] && break
    if ! systemctl --user is-active --quiet "$unit_name"; then
      break
    fi
    sleep 1
  done

  if [[ -s "$run_dir/vessel-database-before.sql" &&
        ! -e "$run_dir/cleanup.complete" ]]; then
    "$script_dir/run_vessel_scale_benchmark.sh" __restore "$run_dir"
  fi
  [[ ! -s "$run_dir/vessel-database-before.sql" ||
     -e "$run_dir/cleanup.complete" ]] ||
    fail "the pre-benchmark database could not be restored"
  printf 'Vessel scale benchmark cleanup is complete: %s\n' "$run_dir"
}

run_benchmark()
{
  local run_dir=$1
  local measurement_seconds=$2
  local app_environment
  local database_host
  local database_name
  local database_user
  local database_password
  local soak_unit
  local source_commit
  local binary_sha256
  local binary_fingerprint
  local snapshot_tmp
  local baseline_count
  local valid_baseline_count
  local baseline_slot_rows
  local marker_count
  local encounter_region
  local prototype_rows
  local spawn_needed
  local spawned_count
  local persisted_count
  local verification_output
  local message_output
  local msdp_output
  local reconstruction_error_pattern
  local workload_counts
  local active_count
  local class_count
  local pilot_count
  local crew_count
  local schedule_count
  local cargo_count
  local weapon_count
  local slot500_room_count
  local combat_fixture_count
  local combat_fixture_weapon_count
  local msdp_slot
  local submarine_slot
  local combat_slot
  local schedule_rows
  local schedule_id_list
  local schedule_paused_count
  local server_pid
  local workload_log_offset
  local workload_log_size
  local log_offset
  local measurement_pid
  local helper_status
  local fleet_after
  local schedule_trigger_count
  local shared_encounter_count
  local vessel_error_count
  local measurement_error_pattern
  local measurement_log_bytes
  local progress_log_count
  local perf_rows
  local perf_row_count
  local vessel_tick_row
  local vessel_tick_calls
  local vessel_tick_median
  local vessel_tick_p95
  local vessel_tick_p99
  local vessel_tick_max
  local missed_pulses
  local throttled_messages
  local database_queries
  local air_z_sample_summary
  local air_z_sample_count
  local air_z_minimum
  local air_z_maximum
  local initial_rss
  local maximum_rss
  local final_rss
  local initial_vsz
  local maximum_vsz
  local final_vsz
  local initial_threads
  local maximum_threads
  local final_threads
  local initial_descriptors
  local maximum_descriptors
  local final_descriptors
  local live_system_sample_count
  local expected_live_system_samples
  local live_system_summary
  local initial_dynamic_rooms
  local maximum_dynamic_rooms
  local final_dynamic_rooms
  local dynamic_room_capacity
  local initial_world_lists
  local maximum_world_lists
  local final_world_lists
  local initial_movement_trails
  local maximum_movement_trails
  local final_movement_trails
  local initial_mobiles
  local maximum_mobiles
  local final_mobiles
  local initial_objects
  local maximum_objects
  local final_objects
  local initial_rooms
  local maximum_rooms
  local final_rooms
  local process_row
  local rss
  local vsz
  local threads
  local descriptors
  local current_binary_fingerprint
  local remaining
  local wait_seconds
  local measurement_elapsed
  local next_live_system_sample
  local next_memory_detail
  local benchmark_outcome
  local failure_reason
  local cleanup_ok
  local exit_code
  local prototype_id
  local ship_id
  local elapsed
  local started_epoch
  local finished_epoch
  local -a prototype_ids
  local -a baseline_ship_ids
  local -a spawn_commands
  local -a preparation_commands
  local -a measurement_commands
  local -a schedule_ship_ids
  local -A occupied_slots

  case "$run_dir" in
    "$state_root"/runs/*)
      ;;
    *)
      fail "invalid internal run directory"
      ;;
  esac
  [[ -d "$run_dir" ]] || fail "internal run directory is missing"

  umask 077
  exec >>"$run_dir/monitor.log" 2>&1
  exec 8>"$run_dir/cleanup.lock"
  flock -n 8 || fail "another process owns this benchmark run"
  benchmark_outcome=""
  failure_reason=""
  cleanup_ok=false
  measurement_pid=""

  finish_worker()
  {
    exit_code=${1:-$?}
    trap - EXIT TERM INT
    if [[ -n "$measurement_pid" ]] && kill -0 "$measurement_pid" 2>/dev/null; then
      kill "$measurement_pid" 2>/dev/null || true
      wait "$measurement_pid" 2>/dev/null || true
    fi

    if restore_snapshot "$run_dir"; then
      cleanup_ok=true
    else
      cleanup_ok=false
      [[ -n "$failure_reason" ]] ||
        failure_reason="pre-benchmark database restoration failed"
    fi

    if [[ -z "$benchmark_outcome" ]]; then
      benchmark_outcome=FAIL
      [[ -n "$failure_reason" ]] ||
        failure_reason="benchmark worker exited with status $exit_code"
      {
        printf 'Result: FAIL\n'
        printf 'Reason: %s\n' "$failure_reason"
        printf 'Cleanup restored baseline: %s\n' "$cleanup_ok"
      } >"$run_dir/summary.txt"
    elif [[ "$cleanup_ok" != true ]]; then
      benchmark_outcome=FAIL
      failure_reason="benchmark completed but baseline restoration failed"
      {
        printf '\nCleanup restored baseline: no\n'
        printf 'Cleanup failure: %s\n' "$failure_reason"
      } >>"$run_dir/summary.txt"
    fi

    finished_epoch=$(date +%s)
    if [[ "$benchmark_outcome" == PASS && "$cleanup_ok" == true ]]; then
      write_status "$run_dir" "PASS finished=$finished_epoch cleanup=restored"
    else
      write_status "$run_dir" \
        "FAIL finished=$finished_epoch cleanup=$cleanup_ok reason=$failure_reason"
    fi
  }
  trap 'finish_worker $?' EXIT
  trap 'failure_reason="benchmark worker received SIGTERM"; exit 143' TERM
  trap 'failure_reason="benchmark worker received SIGINT"; exit 130' INT

  benchmark_fail()
  {
    failure_reason=$*
    printf 'FAIL: %s\n' "$failure_reason"
    exit 1
  }

  sample_process()
  {
    local current_pid

    current_pid=$(systemctl --user show --property=MainPID --value "$server_unit")
    [[ "$current_pid" == "$server_pid" ]] ||
      benchmark_fail "development MUD PID changed during measurement"
    current_binary_fingerprint=$(stat -Lc '%d:%i:%s:%Y' "$repo_root/bin/circle")
    [[ "$current_binary_fingerprint" == "$binary_fingerprint" ]] ||
      benchmark_fail "the installed MUD executable changed during measurement"
    process_row=$(ps -o rss=,vsz=,nlwp= -p "$current_pid")
    read -r rss vsz threads <<<"$process_row"
    [[ "$rss" =~ ^[0-9]+$ && "$vsz" =~ ^[0-9]+$ &&
       "$threads" =~ ^[0-9]+$ ]] ||
      benchmark_fail "could not read development MUD process metrics"
    descriptors=$(find "/proc/$current_pid/fd" -mindepth 1 -maxdepth 1 | wc -l)
    [[ "$descriptors" =~ ^[0-9]+$ ]] ||
      benchmark_fail "could not count development MUD file descriptors"
    if [[ -z "$initial_rss" ]]; then
      initial_rss=$rss
      initial_vsz=$vsz
      initial_threads=$threads
      initial_descriptors=$descriptors
    fi
    ((rss > maximum_rss)) && maximum_rss=$rss
    ((vsz > maximum_vsz)) && maximum_vsz=$vsz
    ((threads > maximum_threads)) && maximum_threads=$threads
    ((descriptors > maximum_descriptors)) && maximum_descriptors=$descriptors
    final_rss=$rss
    final_vsz=$vsz
    final_threads=$threads
    final_descriptors=$descriptors
    printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
      "$(date +%s)" "$current_pid" "$rss" "$vsz" "$threads" "$descriptors" \
      >>"$run_dir/process-samples.tsv"
  }

  sample_memory_detail()
  {
    local label=$1

    if ! "$script_dir/sample_process_memory_details.sh" \
      --sample "$server_pid" "$label" \
      >>"$run_dir/process-memory-details.tsv"; then
      benchmark_fail "could not capture detailed development MUD memory"
    fi
  }

  for command_name in awk date dd find flock git grep mariadb mariadb-dump \
    mv ps sed sha256sum sleep stat systemctl timeout wc; do
    command -v "$command_name" >/dev/null 2>&1 ||
      benchmark_fail "required command not found: $command_name"
  done
  [[ -x "$repo_root/bin/circle" ]] ||
    benchmark_fail "bin/circle is missing; build and install first"
  [[ -x "$script_dir/dev_kohdee_login_smoke.sh" ]] ||
    benchmark_fail "the local character login helper is unavailable"
  [[ -x "$script_dir/provision_vessel_harbor.sh" ]] ||
    benchmark_fail "the harbor provisioner is unavailable"
  [[ -x "$script_dir/sample_process_memory_details.sh" ]] ||
    benchmark_fail "the detailed process-memory sampler is unavailable"
  load_database_config ||
    benchmark_fail "development database configuration is unavailable"

  soak_unit=$(active_ferry_soak_unit)
  [[ -z "$soak_unit" ]] ||
    benchmark_fail "refusing to disturb active ferry soak $soak_unit"
  [[ -z "$(git -C "$repo_root" status --porcelain)" ]] ||
    benchmark_fail "source worktree must be clean before benchmark provenance is recorded"
  source_commit=$(git -C "$repo_root" rev-parse HEAD)
  binary_sha256=$(sha256sum "$repo_root/bin/circle" | awk '{print $1}')

  write_status "$run_dir" "PREPARING harbor"
  "$script_dir/provision_vessel_harbor.sh" >"$run_dir/harbor-provision.log" 2>&1 ||
    benchmark_fail "shared harbor provisioning failed"

  verification_output="$run_dir/capacity-preflight.log"
  "$script_dir/dev_kohdee_login_smoke.sh" --commands "shiplist summary" \
    >"$verification_output" 2>&1 ||
    benchmark_fail "capacity preflight login failed"
  grep -Eq "[0-9]+ of 500 active fleet slots in use\\." "$verification_output" ||
    benchmark_fail "installed MUD does not expose the 500-active-slot build"
  if grep -Fq "Slot Name" "$verification_output"; then
    benchmark_fail "installed MUD does not expose compact shiplist summary output"
  fi

  encounter_region=$(database_query "
    SELECT vnum
      FROM region_data
     WHERE region_type = 2
       AND ST_Contains(region_polygon, ST_GeomFromText('POINT(-49 97)'))
       AND ST_Contains(region_polygon, ST_GeomFromText('POINT(-46 98)'))
       AND ST_Contains(region_polygon, ST_GeomFromText('POINT(-44 97)'))
       AND ST_Contains(region_polygon, ST_GeomFromText('POINT(-48 95)'))
     ORDER BY vnum
     LIMIT 1;") ||
    benchmark_fail "could not query the development encounter region"
  [[ "$encounter_region" =~ ^[0-9]+$ ]] ||
    benchmark_fail "no encounter region contains the benchmark air route"

  baseline_count=$(database_query "SELECT COUNT(*) FROM ship_runtime_state;") ||
    benchmark_fail "could not count baseline vessels"
  valid_baseline_count=$(database_query "
    SELECT COUNT(*)
      FROM ship_runtime_state AS runtime
      JOIN ship_interiors AS interior USING (ship_id)
     WHERE runtime.ship_id BETWEEN 1 AND 500;") ||
    benchmark_fail "could not validate baseline vessel rows"
  [[ "$baseline_count" =~ ^[0-9]+$ && "$valid_baseline_count" == "$baseline_count" ]] ||
    benchmark_fail "baseline contains incomplete or out-of-range vessel rows"
  ((baseline_count >= 1 && baseline_count <= 500)) ||
    benchmark_fail "baseline vessel count $baseline_count is outside 1-500"
  marker_count=$(database_query "
    SELECT
      (SELECT COUNT(*) FROM ship_prototypes
        WHERE LEFT(name, LENGTH('${benchmark_prefix}')) = '${benchmark_prefix}') +
      (SELECT COUNT(*) FROM ship_routes
        WHERE LEFT(name, LENGTH('${benchmark_prefix}')) = '${benchmark_prefix}') +
      (SELECT COUNT(*) FROM ship_waypoints
        WHERE LEFT(name, LENGTH('${benchmark_prefix}')) = '${benchmark_prefix}') +
      (SELECT COUNT(*) FROM vessel_encounters
        WHERE LEFT(name, LENGTH('${benchmark_prefix}')) = '${benchmark_prefix}');") ||
    benchmark_fail "could not check for stale benchmark data"
  [[ "$marker_count" == 0 ]] ||
    benchmark_fail "stale benchmark marker rows exist; restore the prior run first"

  {
    printf 'baseline_vessels=%s\n' "$baseline_count"
    printf 'encounter_region=%s\n' "$encounter_region"
    printf 'source_commit=%s\n' "$source_commit"
    printf 'binary_sha256=%s\n' "$binary_sha256"
    printf 'snapshot_started_at=%s\n' "$(date +%s)"
  } >>"$run_dir/metadata"

  snapshot_tmp="$run_dir/vessel-database-before.sql.tmp"
  MYSQL_PWD="$database_password" mariadb-dump --no-defaults \
    --host="$database_host" --user="$database_user" \
    --single-transaction --quick --skip-lock-tables --skip-comments \
    --skip-dump-date --add-drop-table "$database_name" \
    "${snapshot_tables[@]}" >"$snapshot_tmp" ||
    benchmark_fail "could not snapshot the vessel database"
  [[ -s "$snapshot_tmp" ]] ||
    benchmark_fail "the vessel database snapshot is empty"
  mv "$snapshot_tmp" "$run_dir/vessel-database-before.sql" ||
    benchmark_fail "could not publish the vessel database snapshot"
  printf 'snapshot_ready_at=%s\n' "$(date +%s)" >>"$run_dir/metadata"

  write_status "$run_dir" "PREPARING spawning"
  database_query "
    INSERT INTO ship_prototypes (name, vessel_class, max_speed, armor) VALUES
      ('${benchmark_prefix} Raft', 0, 5, 80),
      ('${benchmark_prefix} Boat', 1, 8, 80),
      ('${benchmark_prefix} Ship', 2, 10, 80),
      ('${benchmark_prefix} Warship', 3, 15, 80),
      ('${benchmark_prefix} Airship', 4, 20, 80),
      ('${benchmark_prefix} Submarine', 5, 12, 80),
      ('${benchmark_prefix} Transport', 6, 10, 80),
      ('${benchmark_prefix} Magical', 7, 20, 80);" ||
    benchmark_fail "could not create benchmark prototypes"
  prototype_rows=$(database_query "
    SELECT prototype_id
      FROM ship_prototypes
     WHERE LEFT(name, LENGTH('${benchmark_prefix}')) = '${benchmark_prefix}'
     ORDER BY vessel_class;") ||
    benchmark_fail "could not load benchmark prototype IDs"
  mapfile -t prototype_ids <<<"$prototype_rows"
  ((${#prototype_ids[@]} == 8)) ||
    benchmark_fail "expected eight benchmark prototypes"
  for prototype_id in "${prototype_ids[@]}"; do
    [[ "$prototype_id" =~ ^[0-9]+$ ]] ||
      benchmark_fail "invalid benchmark prototype ID"
  done

  baseline_slot_rows=$(database_query "
    SELECT ship_id FROM ship_runtime_state ORDER BY ship_id;") ||
    benchmark_fail "could not load the occupied baseline slots"
  mapfile -t baseline_ship_ids <<<"$baseline_slot_rows"
  for ship_id in "${baseline_ship_ids[@]}"; do
    [[ "$ship_id" =~ ^[1-9][0-9]*$ && "$ship_id" -le 500 ]] ||
      benchmark_fail "invalid occupied baseline slot: $ship_id"
    occupied_slots["$ship_id"]=1
  done
  [[ -n "${occupied_slots[1]:-}" ]] ||
    benchmark_fail "baseline slot 1 must exist because builder spawning starts at slot 2"

  spawn_needed=$((500 - baseline_count))
  spawn_commands=("goto -66 92")
  for ((ship_id = 1; ship_id <= 500; ship_id++)); do
    if [[ -z "${occupied_slots[$ship_id]:-}" ]]; then
      spawn_commands+=(
        "vedit spawnpublic ${prototype_ids[(ship_id - 1) % 8]}"
      )
    fi
  done
  ((${#spawn_commands[@]} == spawn_needed + 1)) ||
    benchmark_fail "could not map every free fleet slot to a benchmark prototype"
  spawn_commands+=("shiplist summary")
  timeout 3600 "$script_dir/dev_kohdee_login_smoke.sh" \
    --commands "${spawn_commands[@]}" >"$run_dir/spawn.log" 2>&1 ||
    benchmark_fail "in-game fleet spawning failed"
  spawned_count=$(grep -Ec "^Spawned '${benchmark_prefix}" "$run_dir/spawn.log" || true)
  [[ "$spawned_count" == "$spawn_needed" ]] ||
    benchmark_fail "spawned $spawned_count of $spawn_needed required vessels"
  grep -Fq "500 of 500 active fleet slots in use." "$run_dir/spawn.log" ||
    benchmark_fail "in-game fleet did not reach 500 active vessels"
  persisted_count=$(database_query "SELECT COUNT(*) FROM ship_runtime_state;") ||
    benchmark_fail "could not count vessels after in-game spawning"
  [[ "$persisted_count" == 500 ]] ||
    benchmark_fail "database contains $persisted_count vessels after spawning"

  workload_log_offset=$(stat -c %s "$server_log")
  systemctl --user stop "$server_unit" ||
    benchmark_fail "could not stop local development before workload configuration"

  write_status "$run_dir" "PREPARING workload"
  if ! database_apply <<SQL
START TRANSACTION;

INSERT INTO ship_waypoints
  (name, x, y, z, tolerance, wait_time, flags)
VALUES
  ('${benchmark_prefix} Surface A', -66, 92, 0, 0.5, 10, 0),
  ('${benchmark_prefix} Surface B', -66, 92, 0, 0.5, 10, 0),
  ('${benchmark_prefix} Submerged A', -64, 82, -1, 0.5, 10, 0),
  ('${benchmark_prefix} Submerged B', -64, 82, -1, 0.5, 10, 0),
  ('${benchmark_prefix} Water West', -66, 92, 0, 0.5, 5, 0),
  ('${benchmark_prefix} Water Turn A', -64, 82, 0, 0.5, 0, 0),
  ('${benchmark_prefix} Water East', -62, 82, 0, 0.5, 5, 0),
  ('${benchmark_prefix} Water Turn B', -64, 82, 0, 0.5, 0, 0),
  ('${benchmark_prefix} Air A', -49, 97, 150, 0.5, 0, 0),
  ('${benchmark_prefix} Air B', -46, 98, 180, 0.5, 0, 0),
  ('${benchmark_prefix} Air C', -44, 97, 120, 0.5, 0, 0),
  ('${benchmark_prefix} Air D', -48, 95, 220, 0.5, 0, 0);

INSERT INTO ship_routes (name, loop_route, active) VALUES
  ('${benchmark_prefix} Surface Route', 1, 1),
  ('${benchmark_prefix} Submerged Route', 1, 1),
  ('${benchmark_prefix} Water Route', 1, 1),
  ('${benchmark_prefix} Air Route', 1, 1);

SET @surface_route = (
  SELECT route_id FROM ship_routes
   WHERE name = '${benchmark_prefix} Surface Route'
);
SET @submerged_route = (
  SELECT route_id FROM ship_routes
   WHERE name = '${benchmark_prefix} Submerged Route'
);
SET @water_route = (
  SELECT route_id FROM ship_routes
   WHERE name = '${benchmark_prefix} Water Route'
);
SET @air_route = (
  SELECT route_id FROM ship_routes
   WHERE name = '${benchmark_prefix} Air Route'
);

INSERT INTO ship_route_waypoints (route_id, waypoint_id, sequence_num)
SELECT @surface_route, waypoint_id,
       CASE WHEN name LIKE '% Surface A' THEN 0 ELSE 1 END
  FROM ship_waypoints
 WHERE name IN (
   '${benchmark_prefix} Surface A',
   '${benchmark_prefix} Surface B'
 );
INSERT INTO ship_route_waypoints (route_id, waypoint_id, sequence_num)
SELECT @submerged_route, waypoint_id,
       CASE WHEN name LIKE '% Submerged A' THEN 0 ELSE 1 END
  FROM ship_waypoints
 WHERE name IN (
   '${benchmark_prefix} Submerged A',
   '${benchmark_prefix} Submerged B'
 );
INSERT INTO ship_route_waypoints (route_id, waypoint_id, sequence_num)
SELECT @water_route, waypoint_id,
       CASE name
         WHEN '${benchmark_prefix} Water West' THEN 0
         WHEN '${benchmark_prefix} Water Turn A' THEN 1
         WHEN '${benchmark_prefix} Water East' THEN 2
         ELSE 3
       END
  FROM ship_waypoints
 WHERE name IN (
   '${benchmark_prefix} Water West',
   '${benchmark_prefix} Water Turn A',
   '${benchmark_prefix} Water East',
   '${benchmark_prefix} Water Turn B'
 );
INSERT INTO ship_route_waypoints (route_id, waypoint_id, sequence_num)
SELECT @air_route, waypoint_id,
       CASE name
         WHEN '${benchmark_prefix} Air A' THEN 0
         WHEN '${benchmark_prefix} Air B' THEN 1
         WHEN '${benchmark_prefix} Air C' THEN 2
         ELSE 3
       END
  FROM ship_waypoints
 WHERE name IN (
   '${benchmark_prefix} Air A',
   '${benchmark_prefix} Air B',
   '${benchmark_prefix} Air C',
   '${benchmark_prefix} Air D'
 );

UPDATE ship_interiors
   SET vessel_name = CONCAT('${benchmark_prefix} ', LPAD(ship_id, 3, '0')),
       owner = '',
       upgrades = MOD(ship_id, 8),
       insured_for = CASE WHEN vessel_type = 3 THEN 10000 ELSE 0 END,
       wages_owed = 0;

UPDATE ship_runtime_state AS runtime
JOIN ship_interiors AS interior USING (ship_id)
   SET runtime.location_vnum = 0,
       runtime.x = CASE
         WHEN interior.vessel_type = 4 THEN
           CASE MOD(runtime.ship_id, 4)
             WHEN 0 THEN -49 WHEN 1 THEN -46 WHEN 2 THEN -44 ELSE -48
           END
         WHEN interior.vessel_type = 5 THEN -64
         WHEN interior.vessel_type IN (0, 1) THEN -66
         ELSE
           CASE MOD(runtime.ship_id, 4)
             WHEN 0 THEN -66 WHEN 1 THEN -64 WHEN 2 THEN -62 ELSE -64
           END
       END,
       runtime.y = CASE
         WHEN interior.vessel_type = 4 THEN
           CASE MOD(runtime.ship_id, 4)
             WHEN 0 THEN 97 WHEN 1 THEN 98 WHEN 2 THEN 97 ELSE 95
           END
         WHEN interior.vessel_type = 5 THEN 82
         WHEN interior.vessel_type IN (0, 1) THEN 92
         ELSE
           CASE MOD(runtime.ship_id, 4)
             WHEN 0 THEN 92 ELSE 82
           END
       END,
       runtime.z = CASE
         WHEN interior.vessel_type = 4 THEN 150
         WHEN interior.vessel_type = 5 THEN -1
         ELSE 0
       END,
       runtime.dx = 0,
       runtime.dy = 0,
       runtime.dz = 0,
       runtime.minspeed = 0,
       runtime.maxspeed = 20,
       runtime.speed = 2,
       runtime.setspeed = 2,
       runtime.dock_room = 0,
       runtime.docked_to_ship = -1,
       runtime.docking_room = 0,
       runtime.maxfarmor = 80,
       runtime.maxrarmor = 80,
       runtime.maxparmor = 80,
       runtime.maxsarmor = 80,
       runtime.farmor = 80,
       runtime.rarmor = 80,
       runtime.parmor = 80,
       runtime.sarmor = 80,
       runtime.maxfinternal = 100,
       runtime.maxrinternal = 100,
       runtime.maxpinternal = 100,
       runtime.maxsinternal = 100,
       runtime.finternal = 100,
       runtime.rinternal = 100,
       runtime.pinternal = 100,
       runtime.sinternal = 100,
       runtime.maxturnrate = 20,
       runtime.turnrate = 20,
       runtime.maxmainsail = 20,
       runtime.mainsail = 20,
       runtime.last_attacker = 0,
       runtime.pvp_grace_until = 0,
       runtime.pvp_grace_attacker = '',
       runtime.dock_fee_balance = 0,
       runtime.dock_fee_port = 0,
       runtime.dock_fee_clan = 0,
       runtime.wear_ticks = 899,
       runtime.wage_ticks = 599,
       runtime.autopilot_state = 1,
       runtime.current_route_id = CASE
         WHEN interior.vessel_type = 4 THEN @air_route
         WHEN interior.vessel_type = 5 THEN @submerged_route
         WHEN interior.vessel_type IN (0, 1) THEN @surface_route
         ELSE @water_route
       END,
       runtime.current_waypoint_index = CASE
         WHEN interior.vessel_type IN (0, 1, 5) THEN 0
         ELSE MOD(MOD(runtime.ship_id, 4) + 1, 4)
       END,
       runtime.autopilot_tick_counter = MOD(runtime.ship_id, 10),
       runtime.wait_remaining = 0,
       runtime.last_update = UNIX_TIMESTAMP();

SET @msdp_ship = (
  SELECT MIN(ship_id) FROM ship_interiors WHERE vessel_type = 4
);
UPDATE ship_runtime_state
   SET z = 500
 WHERE ship_id = @msdp_ship;

SET @combat_ship_a = (
  SELECT MAX(ship_id) FROM ship_interiors WHERE vessel_type = 5
);
SET @combat_ship_b = (
  SELECT MAX(ship_id)
    FROM ship_interiors
   WHERE vessel_type = 5
     AND ship_id < @combat_ship_a
);
UPDATE ship_runtime_state
   SET x = -64,
       y = 82,
       z = -1,
       dx = 0,
       dy = 0,
       dz = 0,
       heading = 0,
       setheading = 0,
       maxspeed = 110,
       speed = 110,
       setspeed = 110,
       autopilot_state = 3,
       current_waypoint_index = 0,
       autopilot_tick_counter = 0,
       last_attacker = CASE
         WHEN ship_id = @combat_ship_a THEN @combat_ship_b
         ELSE @combat_ship_a
       END
 WHERE ship_id IN (@combat_ship_a, @combat_ship_b);

DELETE FROM ship_docking;
DELETE FROM ship_schedules;
INSERT INTO ship_schedules
  (ship_id, route_id, interval_hours, next_departure, enabled)
SELECT ship_id, current_route_id, 1, 0, 1
  FROM ship_runtime_state;
UPDATE ship_schedules
   SET next_departure = UNIX_TIMESTAMP() + 86400
 WHERE ship_id IN (@combat_ship_a, @combat_ship_b);

DELETE FROM ship_crew_roster;
INSERT INTO ship_crew_roster
  (ship_id, npc_vnum, npc_name, crew_role, assigned_room, status)
SELECT ship_id, 70001, 'benchmark ferrymaster', 'pilot', bridge_room, 'active'
  FROM ship_interiors;
INSERT INTO ship_crew_roster
  (ship_id, npc_vnum, npc_name, crew_role, loyalty_rating, status)
SELECT interior.ship_id,
       -100 - positions.position,
       CASE positions.position
         WHEN 0 THEN 'sailmaster'
         WHEN 1 THEN 'gunner'
         WHEN 2 THEN 'bosun'
         ELSE 'quartermaster'
       END,
       'crew',
       3,
       'active'
  FROM ship_interiors AS interior
 CROSS JOIN (
   SELECT 0 AS position
   UNION ALL SELECT 1
   UNION ALL SELECT 2
   UNION ALL SELECT 3
 ) AS positions;

SET @commodity_id = (
  SELECT MIN(commodity_id) FROM trade_commodities
);
DELETE FROM ship_cargo_manifest;
INSERT INTO ship_cargo_manifest
  (ship_id, cargo_room, item_vnum, item_name, item_count, item_weight,
   loaded_by)
SELECT interior.ship_id, 0, commodity.commodity_id, commodity.name,
       10 + MOD(interior.ship_id, 20),
       commodity.unit_weight * (10 + MOD(interior.ship_id, 20)),
       'scale benchmark'
  FROM ship_interiors AS interior
  JOIN trade_commodities AS commodity
    ON commodity.commodity_id = @commodity_id;

UPDATE ship_weapons AS weapon
JOIN ship_interiors AS interior USING (ship_id)
   SET weapon.reload_timer = MOD(weapon.ship_id + weapon.slot_index, 10) + 1,
       weapon.val0 = CASE
         WHEN interior.vessel_type IN (3, 4) THEN 5
         ELSE weapon.val0
       END;

INSERT INTO ship_weapons
  (ship_id, slot_index, slot_type, position, equipment_weight, description,
   val0, val1, val2, val3, slot_x, slot_y, reload_timer)
SELECT interior.ship_id, slots.slot_index, 1, 3, 0,
       CONCAT('benchmark ballista ', slots.slot_index + 1),
       5, 0, 1, 1, 0, 0, MOD(interior.ship_id, 10) + 1
  FROM ship_interiors AS interior
 CROSS JOIN (
   SELECT 0 AS slot_index
   UNION ALL SELECT 1
 ) AS slots
 WHERE interior.vessel_type IN (3, 4, 5)
ON DUPLICATE KEY UPDATE
  slot_type = VALUES(slot_type),
  position = VALUES(position),
  description = VALUES(description),
  val0 = VALUES(val0),
  val2 = VALUES(val2),
  val3 = VALUES(val3),
  reload_timer = VALUES(reload_timer);

DELETE FROM vessel_encounters;
INSERT INTO vessel_encounters
  (region_vnum, name, mob_vnum, min_depth, max_depth, vessel_class,
   chance, warn_message, arrive_message)
VALUES
  ($encounter_region, '${benchmark_prefix} Message Encounter', 0, 0, 0, 4,
   100, 'The benchmark lookout reports movement.',
   'A benchmark contact passes the airship.');

COMMIT;
SQL
  then
    benchmark_fail "could not configure the benchmark database workload"
  fi

  workload_counts=$(database_query "
    SELECT
      (SELECT COUNT(*) FROM ship_runtime_state),
      (SELECT COUNT(DISTINCT vessel_type) FROM ship_interiors),
      (SELECT COUNT(*) FROM ship_crew_roster
        WHERE crew_role = 'pilot' AND npc_vnum = 70001 AND status = 'active'),
      (SELECT COUNT(*) FROM ship_crew_roster WHERE npc_vnum <= -100),
      (SELECT COUNT(*) FROM ship_schedules WHERE enabled = 1),
      (SELECT COUNT(*) FROM ship_cargo_manifest WHERE cargo_room = 0),
      (SELECT COUNT(*) FROM ship_weapons),
      (SELECT COUNT(*) FROM ship_interiors
        WHERE ship_id = 500
          AND num_rooms > 0
          AND bridge_room BETWEEN 80000 AND 80019
          AND CAST(SUBSTRING_INDEX(room_vnums, ',', 1) AS UNSIGNED)
                BETWEEN 80000 AND 80019
          AND CAST(SUBSTRING_INDEX(room_vnums, ',', -1) AS UNSIGNED)
                BETWEEN 80000 AND 80019
          AND num_rooms =
                1 + LENGTH(room_vnums) - LENGTH(REPLACE(room_vnums, ',', ''))),
      (SELECT COUNT(*)
         FROM ship_runtime_state AS attacker
         JOIN ship_runtime_state AS target
           ON target.ship_id = attacker.last_attacker
          AND target.last_attacker = attacker.ship_id
         JOIN ship_interiors AS interior
           ON interior.ship_id = attacker.ship_id
         JOIN ship_schedules AS schedule
           ON schedule.ship_id = attacker.ship_id
        WHERE interior.vessel_type = 5
          AND attacker.autopilot_state = 3
          AND attacker.speed = 110
          AND target.speed = 110
          AND attacker.heading = 0
          AND target.heading = 0
          AND attacker.x = target.x
          AND attacker.y = target.y
          AND attacker.z = target.z
          AND attacker.z = -1
          AND schedule.enabled = 1
          AND schedule.next_departure >= UNIX_TIMESTAMP() + 80000),
      (SELECT COUNT(*)
         FROM ship_weapons AS weapon
         JOIN ship_runtime_state AS runtime USING (ship_id)
        WHERE runtime.speed = 110
          AND weapon.slot_index IN (0, 1)
          AND weapon.slot_type = 1
          AND weapon.position = 3
          AND weapon.val0 = 5
          AND weapon.val2 = 1
          AND weapon.val3 = 1
          AND weapon.reload_timer = MOD(weapon.ship_id, 10) + 1);") ||
    benchmark_fail "could not validate configured workload counts"
  IFS=$'\t' read -r active_count class_count pilot_count crew_count \
    schedule_count cargo_count weapon_count slot500_room_count \
    combat_fixture_count combat_fixture_weapon_count <<<"$workload_counts"
  [[ "$active_count" == 500 && "$class_count" == 8 &&
     "$pilot_count" == 500 && "$crew_count" == 2000 &&
     "$schedule_count" == 500 && "$cargo_count" == 500 &&
     "$weapon_count" =~ ^[1-9][0-9]*$ && "$slot500_room_count" == 1 &&
     "$combat_fixture_count" == 2 && "$combat_fixture_weapon_count" == 4 ]] ||
    benchmark_fail "configured workload counts are incomplete: $workload_counts"

  msdp_slot=$(database_query "
    SELECT MIN(ship_id) FROM ship_interiors WHERE vessel_type = 4;")
  [[ "$msdp_slot" =~ ^[0-9]+$ ]] ||
    benchmark_fail "could not choose an MSDP airship"
  submarine_slot=$(database_query "
    SELECT MIN(ship_id) FROM ship_interiors WHERE vessel_type = 5;")
  [[ "$submarine_slot" =~ ^[0-9]+$ ]] ||
    benchmark_fail "could not choose a boundary-test submarine"
  combat_slot=$(database_query "
    SELECT MAX(ship_id) FROM ship_interiors WHERE vessel_type = 5;")
  [[ "$combat_slot" =~ ^[0-9]+$ && "$combat_slot" != "$submarine_slot" ]] ||
    benchmark_fail "could not choose the reciprocal combat submarine"
  schedule_rows=$(database_query "
    SELECT ship_id
      FROM ship_interiors
     WHERE vessel_type IN (2, 3, 6, 7)
     ORDER BY ship_id
     LIMIT 10;")
  mapfile -t schedule_ship_ids <<<"$schedule_rows"
  ((${#schedule_ship_ids[@]} == 10)) ||
    benchmark_fail "could not select ten scheduled-departure vessels"
  for ship_id in "${schedule_ship_ids[@]}"; do
    [[ "$ship_id" =~ ^[1-9][0-9]*$ ]] ||
      benchmark_fail "invalid scheduled-departure ship ID"
  done
  schedule_id_list=$(IFS=,; printf '%s' "${schedule_ship_ids[*]}")

  preparation_commands=(
    "shiplist summary"
    "vtradecheck 1000"
    "shipgoto 500"
    "shipstatus"
    "autopilot pause"
    "setsail up"
    "shipstatus"
    "autopilot on"
    "showschedule"
    "shipcrew"
    "cargomanifest"
    "shipgoto $msdp_slot"
    "autopilot pause"
    "shipstatus"
    "setsail up"
    "shipstatus"
    "autopilot on"
    "seastate"
    "@wait 60"
    "shipgoto $submarine_slot"
    "autopilot pause"
    "shipstatus"
    "setsail up"
    "shipstatus"
    "autopilot on"
  )
  for ship_id in "${schedule_ship_ids[@]}"; do
    preparation_commands+=("shipgoto $ship_id" "autopilot pause")
  done
  preparation_commands+=("@wait 60" "@wait 30")
  for ship_id in "${schedule_ship_ids[@]}"; do
    preparation_commands+=("shipgoto $ship_id" "autopilot pause")
  done
  preparation_commands+=("goto 1000389")

  verification_output="$run_dir/workload-preparation.log"
  timeout 900 "$script_dir/dev_kohdee_login_smoke.sh" \
    --commands "${preparation_commands[@]}" >"$verification_output" 2>&1 ||
    benchmark_fail "configured 500-vessel workload did not boot and warm up"
  grep -Fq "500 of 500 active fleet slots in use." "$verification_output" ||
    benchmark_fail "configured workload did not reconstruct 500 live vessels"
  grep -Fq "Aboard ${benchmark_prefix} 500 (slot 500)." "$verification_output" ||
    benchmark_fail "final fleet slot did not reconstruct in game"
  grep -Fq "Status: Active" "$verification_output" ||
    benchmark_fail "slot 500 schedule did not reconstruct"
  grep -Fq "Cargo manifest for ${benchmark_prefix} 500:" "$verification_output" ||
    benchmark_fail "slot 500 cargo did not reconstruct"
  grep -Fq "Vessel economy simulation: PASS" "$verification_output" ||
    benchmark_fail "the in-game 1,000-trade economy simulation failed"
  grep -Fq "Trades executed: 1000/1000" "$verification_output" ||
    benchmark_fail "the in-game economy simulation did not execute 1,000 trades"
  grep -Fq "Supply range: 10..400 (hard bounds 10..400)" "$verification_output" ||
    benchmark_fail "the in-game economy simulation escaped its supply bounds"
  grep -Eq "Adversarial reversal profit: -[1-9][0-9]* gold" \
    "$verification_output" ||
    benchmark_fail "the in-game economy simulation retained a reversal profit"
  grep -Fq "Restock convergence: 100/100 (baseline 100)" \
    "$verification_output" ||
    benchmark_fail "the in-game economy simulation did not converge to baseline"
  grep -Fq "The ship cannot navigate this terrain!" "$verification_output" ||
    benchmark_fail "surface hull did not reject positive Z through Kohdee"
  grep -Fq "the altitude is too extreme." "$verification_output" ||
    benchmark_fail "airship did not reject Z above its ceiling through Kohdee"
  [[ "$(grep -Fc "Elevation/Depth: 500" "$verification_output" || true)" -ge 2 ]] ||
    benchmark_fail "airship ceiling rejection changed its live Z"
  grep -Fq "must dive to navigate underwater terrain!" "$verification_output" ||
    benchmark_fail "submarine did not reject positive Z through Kohdee"
  [[ "$(grep -Fc "Elevation/Depth: -1" "$verification_output" || true)" -ge 2 ]] ||
    benchmark_fail "submarine surface-boundary rejection changed its live Z"
  workload_log_size=$(stat -c %s "$server_log")
  ((workload_log_size >= workload_log_offset)) ||
    benchmark_fail "server log was truncated during workload reconstruction"
  dd if="$server_log" of="$run_dir/server-workload-boot.log" \
    iflag=skip_bytes skip="$workload_log_offset" status=none
  grep -Fq "Reconstructed 500 persisted vessel instances" \
    "$run_dir/server-workload-boot.log" ||
    benchmark_fail "server boot did not report 500 reconstructed vessels"
  reconstruction_error_pattern="Dynamic ship .*could not|No zone owns ship interior|"
  reconstruction_error_pattern+="interior VNUM .*exceeds|Room pool exhausted"
  if grep -Eiq "$reconstruction_error_pattern" \
    "$run_dir/server-workload-boot.log"; then
    benchmark_fail "server log reported a fleet reconstruction or room-capacity error"
  fi
  schedule_paused_count=$(database_query "
    SELECT COUNT(*)
      FROM ship_runtime_state
     WHERE ship_id IN ($schedule_id_list)
       AND autopilot_state = 3;") ||
    benchmark_fail "could not verify the staged scheduled vessels"
  [[ "$schedule_paused_count" == 10 ]] ||
    benchmark_fail "only $schedule_paused_count of 10 schedule vessels are paused"

  message_output="$run_dir/vessel-message-throttling.log"
  timeout 90 "$script_dir/dev_kohdee_login_smoke.sh" \
    --vessel-message-check "$combat_slot" >"$message_output" 2>&1 ||
    benchmark_fail "live vessel-message throttling validation failed"
  grep -Fq "observed live reciprocal fire" "$message_output" ||
    benchmark_fail "Kohdee did not observe the reciprocal combat fixture"
  grep -Eq "^# vessel_messages_throttled=[1-9][0-9]*$" "$message_output" ||
    benchmark_fail "Kohdee's message transcript contained no suppression counter"

  msdp_output="$run_dir/native-msdp-vessel-state.log"
  timeout 90 "$script_dir/dev_kohdee_login_smoke.sh" \
    --vessel-msdp-check "$msdp_slot" >"$msdp_output" 2>&1 ||
    benchmark_fail "native MSDP vessel-state validation failed"
  grep -Fq "native MSDP reported all nine vessel variables" "$msdp_output" ||
    benchmark_fail "native MSDP did not report the complete aboard vessel state"
  grep -Fq "native MSDP cleared all nine vessel variables" "$msdp_output" ||
    benchmark_fail "native MSDP did not clear vessel state after Kohdee went ashore"

  measurement_commands=(
    "shipgoto $msdp_slot"
    "perfmon reset"
    "@checkpoint system-0"
    "shiplist summary"
    "show stats"
  )
  remaining=$measurement_seconds
  measurement_elapsed=0
  next_live_system_sample=3600
  expected_live_system_samples=1
  while ((remaining > 0)); do
    wait_seconds=$remaining
    ((wait_seconds > 60)) && wait_seconds=60
    measurement_commands+=("@wait $wait_seconds" "shipstatus")
    remaining=$((remaining - wait_seconds))
    measurement_elapsed=$((measurement_elapsed + wait_seconds))
    if ((measurement_elapsed >= next_live_system_sample || remaining == 0)); then
      measurement_commands+=(
        "@checkpoint system-$measurement_elapsed"
        "shiplist summary"
        "show stats"
      )
      expected_live_system_samples=$((expected_live_system_samples + 1))
      while ((next_live_system_sample <= measurement_elapsed)); do
        next_live_system_sample=$((next_live_system_sample + 3600))
      done
    fi
  done
  measurement_commands+=(
    "perfmon csv"
    "shipstatus"
    "goto 1000389"
  )

  server_pid=$(systemctl --user show --property=MainPID --value "$server_unit")
  [[ "$server_pid" =~ ^[1-9][0-9]*$ ]] ||
    benchmark_fail "could not read the development MUD PID"
  [[ "$(sha256sum "/proc/$server_pid/exe" | awk '{print $1}')" == "$binary_sha256" ]] ||
    benchmark_fail "running MUD does not match the recorded installed binary"
  binary_fingerprint=$(stat -Lc '%d:%i:%s:%Y' "$repo_root/bin/circle")
  log_offset=$(stat -c %s "$server_log")
  initial_rss=""
  maximum_rss=0
  final_rss=0
  initial_vsz=""
  maximum_vsz=0
  final_vsz=0
  initial_threads=""
  maximum_threads=0
  final_threads=0
  initial_descriptors=""
  maximum_descriptors=0
  final_descriptors=0
  printf 'epoch\tpid\trss_kib\tvsz_kib\tthreads\tfile_descriptors\n' \
    >"$run_dir/process-samples.tsv"
  "$script_dir/sample_process_memory_details.sh" --header \
    >"$run_dir/process-memory-details.tsv"
  started_epoch=$(date +%s)
  next_memory_detail=3600
  sample_memory_detail measurement-0
  {
    printf 'measurement_started_at=%s\n' "$started_epoch"
    printf 'server_pid=%s\n' "$server_pid"
    printf 'msdp_ship=%s\n' "$msdp_slot"
  } >>"$run_dir/metadata"
  write_status "$run_dir" \
    "RUNNING started=$started_epoch elapsed=0/$measurement_seconds vessels=500"

  timeout "$((measurement_seconds + 900))" \
    "$script_dir/dev_kohdee_login_smoke.sh" \
    --commands "${measurement_commands[@]}" \
    >"$run_dir/measurement.log" 2>&1 &
  measurement_pid=$!
  elapsed=0
  while kill -0 "$measurement_pid" 2>/dev/null; do
    sample_process
    sleep 30
    elapsed=$(( $(date +%s) - started_epoch ))
    while ((next_memory_detail < measurement_seconds &&
            elapsed >= next_memory_detail)); do
      sample_memory_detail "measurement-$next_memory_detail"
      next_memory_detail=$((next_memory_detail + 3600))
    done
    write_status "$run_dir" \
      "RUNNING started=$started_epoch elapsed=$elapsed/$measurement_seconds vessels=500"
  done
  if wait "$measurement_pid"; then
    helper_status=0
  else
    helper_status=$?
  fi
  measurement_pid=""
  [[ "$helper_status" == 0 ]] ||
    benchmark_fail "the live Kohdee measurement session failed with status $helper_status"
  sample_process
  sample_memory_detail "measurement-$measurement_seconds"

  parse_live_system_samples \
    "$run_dir/measurement.log" "$run_dir/live-system-samples.tsv" ||
    benchmark_fail "could not parse complete live-system samples"

  live_system_sample_count=$(awk 'NR > 1 { count++ } END { print count + 0 }' \
    "$run_dir/live-system-samples.tsv")
  [[ "$live_system_sample_count" == "$expected_live_system_samples" ]] ||
    benchmark_fail \
      "parsed $live_system_sample_count of $expected_live_system_samples live-system samples"
  validate_live_system_samples \
    "$run_dir/live-system-samples.tsv" "$measurement_seconds" ||
    benchmark_fail "live-system sample chronology or invariants are invalid"
  live_system_summary=$(awk -F '\t' '
    NR == 2 {
      initial_dynamic = maximum_dynamic = $4
      capacity = $5
      initial_mobiles = maximum_mobiles = $6
      initial_objects = maximum_objects = $7
      initial_rooms = maximum_rooms = $8
      initial_lists = maximum_lists = $9
      initial_trails = maximum_trails = $10
    }
    NR > 1 {
      if ($4 > maximum_dynamic) maximum_dynamic = $4
      if ($6 > maximum_mobiles) maximum_mobiles = $6
      if ($7 > maximum_objects) maximum_objects = $7
      if ($8 > maximum_rooms) maximum_rooms = $8
      if ($9 > maximum_lists) maximum_lists = $9
      if ($10 > maximum_trails) maximum_trails = $10
      final_dynamic = $4
      final_mobiles = $6
      final_objects = $7
      final_rooms = $8
      final_lists = $9
      final_trails = $10
    }
    END {
      print initial_dynamic, maximum_dynamic, final_dynamic, capacity,
            initial_lists, maximum_lists, final_lists,
            initial_trails, maximum_trails, final_trails,
            initial_mobiles, maximum_mobiles, final_mobiles,
            initial_objects, maximum_objects, final_objects,
            initial_rooms, maximum_rooms, final_rooms
    }
  ' "$run_dir/live-system-samples.tsv")
  read -r initial_dynamic_rooms maximum_dynamic_rooms final_dynamic_rooms \
    dynamic_room_capacity initial_world_lists maximum_world_lists \
    final_world_lists initial_movement_trails maximum_movement_trails \
    final_movement_trails initial_mobiles maximum_mobiles final_mobiles \
    initial_objects maximum_objects final_objects initial_rooms \
    maximum_rooms final_rooms <<<"$live_system_summary"

  dd if="$server_log" of="$run_dir/server-measurement.log" \
    iflag=skip_bytes skip="$log_offset" status=none
  measurement_log_bytes=$(stat -c %s "$run_dir/server-measurement.log")
  if ! progress_log_count=$(
    count_high_volume_progress_logs "$run_dir/server-measurement.log"
  ); then
    benchmark_fail "could not inspect the measurement log for progress spam"
  fi
  [[ "$progress_log_count" =~ ^[0-9]+$ ]] ||
    benchmark_fail "could not count high-volume vessel progress log rows"
  ((progress_log_count == 0)) ||
    benchmark_fail \
      "measurement log contains $progress_log_count high-volume vessel progress rows"
  fleet_after=$(database_query "SELECT COUNT(*) FROM ship_runtime_state;") ||
    benchmark_fail "could not count the final steady fleet"
  [[ "$fleet_after" == 500 ]] ||
    benchmark_fail "steady workload lost vessels: $fleet_after remain"
  grep -Fq "500 of 500 active fleet slots in use." "$run_dir/measurement.log" ||
    benchmark_fail "final in-game fleet check did not retain 500 active vessels"

  schedule_trigger_count=$(grep -Fc "Scheduled departure triggered for ship" \
    "$run_dir/server-measurement.log" || true)
  ((schedule_trigger_count >= 10)) ||
    benchmark_fail "only $schedule_trigger_count scheduled departures triggered"
  for ship_id in "${schedule_ship_ids[@]}"; do
    grep -Fq "Scheduled departure triggered for ship $ship_id on route" \
      "$run_dir/server-measurement.log" ||
      benchmark_fail "scheduled departure did not trigger for ship $ship_id"
  done
  grep -Fq "A benchmark contact passes the airship." \
    "$run_dir/measurement.log" ||
    benchmark_fail "Kohdee did not observe the configured live encounter"
  shared_encounter_count=$(grep -Ec \
    "Shared encounter '${benchmark_prefix} Message Encounter'.*notified ([2-9]|[1-9][0-9]+) vessels" \
    "$run_dir/server-measurement.log" || true)
  ((shared_encounter_count >= 1)) ||
    benchmark_fail "no live encounter notified multiple co-located ships"
  measurement_error_pattern="Autopilot ship .* (impassable terrain|failed)|"
  measurement_error_pattern+="No zone owns ship interior|Room pool exhausted|"
  measurement_error_pattern+="SYSERR:.*(vessel|ship)"
  vessel_error_count=$(grep -Eic "$measurement_error_pattern" \
    "$run_dir/server-measurement.log" || true)
  ((vessel_error_count == 0)) ||
    benchmark_fail "server log contains $vessel_error_count vessel workload errors"

  awk '
    /^section,calls,total_usec,/ {
      capture = 1
    }
    capture {
      print
    }
    /^# database_queries=/ {
      exit
    }
  ' "$run_dir/measurement.log" >"$run_dir/perfmon.csv"
  perf_rows=$(awk -F, '$1 ~ /^vessel_/ { count++ } END { print count + 0 }' \
    "$run_dir/perfmon.csv")
  perf_row_count=$perf_rows
  [[ "$perf_row_count" == 10 ]] ||
    benchmark_fail "perfmon CSV contains $perf_row_count of 10 vessel sections"
  vessel_tick_row=$(awk -F, '$1 == "vessel_tick" { print; exit }' \
    "$run_dir/perfmon.csv")
  [[ -n "$vessel_tick_row" ]] ||
    benchmark_fail "perfmon CSV has no vessel_tick row"
  vessel_tick_calls=$(awk -F, '{print $2}' <<<"$vessel_tick_row")
  vessel_tick_median=$(awk -F, '{print $5}' <<<"$vessel_tick_row")
  vessel_tick_p95=$(awk -F, '{print $6}' <<<"$vessel_tick_row")
  vessel_tick_p99=$(awk -F, '{print $7}' <<<"$vessel_tick_row")
  vessel_tick_max=$(awk -F, '{print $8}' <<<"$vessel_tick_row")
  [[ "$vessel_tick_calls" =~ ^[0-9]+$ &&
     "$vessel_tick_max" =~ ^[0-9]+$ ]] ||
    benchmark_fail "could not parse the vessel_tick result"
  ((vessel_tick_calls >= measurement_seconds * 2 - 10)) ||
    benchmark_fail "vessel_tick captured only $vessel_tick_calls calls"
  missed_pulses=$(sed -nE \
    's/^# missed_pulses=([0-9]+)$/\1/p' "$run_dir/perfmon.csv")
  [[ "$missed_pulses" =~ ^[0-9]+$ ]] ||
    benchmark_fail "could not parse the missed-pulse count"
  throttled_messages=$(sed -nE \
    's/^# vessel_messages_throttled=([0-9]+)$/\1/p' "$run_dir/perfmon.csv")
  [[ "$throttled_messages" =~ ^[0-9]+$ ]] ||
    benchmark_fail "could not parse the vessel message-throttling count"
  ((throttled_messages > 0)) ||
    benchmark_fail "synchronized combat reloads produced no throttled vessel messages"
  database_queries=$(sed -nE \
    's/^# database_queries=([0-9]+)$/\1/p' "$run_dir/perfmon.csv")
  [[ "$database_queries" =~ ^[0-9]+$ ]] ||
    benchmark_fail "could not parse the database query count"
  air_z_sample_summary=$(awk '
    /Elevation\/Depth:/ {
      value = $2 + 0
      seen[value] = 1
      if (!found || value < minimum) {
        minimum = value
      }
      if (!found || value > maximum) {
        maximum = value
      }
      found = 1
    }
    END {
      count = 0
      for (value in seen) {
        count++
      }
      if (found) {
        print count, minimum, maximum
      } else {
        print 0, 0, 0
      }
    }
  ' "$run_dir/measurement.log")
  read -r air_z_sample_count air_z_minimum air_z_maximum \
    <<<"$air_z_sample_summary"
  [[ "$air_z_sample_count" =~ ^[0-9]+$ &&
     "$air_z_minimum" =~ ^-?[0-9]+$ &&
     "$air_z_maximum" =~ ^-?[0-9]+$ ]] ||
    benchmark_fail "could not parse the live Kohdee airship Z samples"
  ((air_z_sample_count >= 2)) ||
    benchmark_fail "live Kohdee airship samples did not observe a Z transition"
  ((air_z_minimum >= 0 && air_z_maximum <= 500)) ||
    benchmark_fail "live airship Z escaped its class bounds: $air_z_minimum..$air_z_maximum"

  if ! "$script_dir/analyze_vessel_memory_samples.sh" \
    --format kv "$run_dir/process-samples.tsv" \
    >"$run_dir/memory-analysis.kv"; then
    benchmark_fail "the terminal process-memory analysis rejected its sample series"
  fi
  if ! "$script_dir/sample_process_memory_details.sh" \
    --validate "$run_dir/process-memory-details.tsv"; then
    benchmark_fail "the terminal detailed process-memory series is invalid"
  fi

  finished_epoch=$(date +%s)
  if ((vessel_tick_max <= 25000)); then
    benchmark_outcome=PASS
    failure_reason=""
  else
    benchmark_outcome=FAIL
    failure_reason="vessel_tick maximum ${vessel_tick_max} usec exceeds 25000 usec"
  fi
  {
    printf 'Result: %s\n' "$benchmark_outcome"
    [[ -z "$failure_reason" ]] || printf 'Reason: %s\n' "$failure_reason"
    printf 'Source commit: %s\n' "$source_commit"
    printf 'Installed binary SHA-256: %s\n' "$binary_sha256"
    printf 'Active vessels/classes: 500/8\n'
    printf 'Pilots/hired crew/schedules/cargo lots: %s/%s/%s/%s\n' \
      "$pilot_count" "$crew_count" "$schedule_count" "$cargo_count"
    printf 'Installed weapon rows: %s\n' "$weapon_count"
    printf 'Measurement duration requested: %s seconds\n' "$measurement_seconds"
    printf 'Measurement wall duration: %s seconds\n' "$((finished_epoch - started_epoch))"
    printf 'Vessel tick calls: %s\n' "$vessel_tick_calls"
    printf 'Vessel tick median/p95/p99/max usec: %s/%s/%s/%s\n' \
      "$vessel_tick_median" "$vessel_tick_p95" "$vessel_tick_p99" "$vessel_tick_max"
    printf 'Missed pulses after reset: %s\n' "$missed_pulses"
    printf 'Vessel messages throttled after reset: %s\n' "$throttled_messages"
    printf 'Database execution attempts after reset: %s\n' "$database_queries"
    printf 'Scheduled departures during measurement: %s\n' "$schedule_trigger_count"
    printf 'Shared multi-ship encounters observed: %s\n' "$shared_encounter_count"
    printf 'Live airship Z distinct/min/max: %s/%s/%s\n' \
      "$air_z_sample_count" "$air_z_minimum" "$air_z_maximum"
    printf 'Vessel workload errors: %s\n' "$vessel_error_count"
    printf 'Measurement server-log bytes: %s\n' "$measurement_log_bytes"
    printf 'High-volume vessel progress log rows: %s\n' "$progress_log_count"
    printf 'Live-system samples: %s\n' "$live_system_sample_count"
    printf 'Dynamic wilderness rooms initial/maximum/final/capacity: %s/%s/%s/%s\n' \
      "$initial_dynamic_rooms" "$maximum_dynamic_rooms" \
      "$final_dynamic_rooms" "$dynamic_room_capacity"
    printf 'World lists initial/maximum/final: %s/%s/%s\n' \
      "$initial_world_lists" "$maximum_world_lists" "$final_world_lists"
    printf 'Movement trails initial/maximum/final: %s/%s/%s\n' \
      "$initial_movement_trails" "$maximum_movement_trails" \
      "$final_movement_trails"
    printf 'Mobiles initial/maximum/final: %s/%s/%s\n' \
      "$initial_mobiles" "$maximum_mobiles" "$final_mobiles"
    printf 'Objects initial/maximum/final: %s/%s/%s\n' \
      "$initial_objects" "$maximum_objects" "$final_objects"
    printf 'Rooms initial/maximum/final: %s/%s/%s\n' \
      "$initial_rooms" "$maximum_rooms" "$final_rooms"
    printf 'Buffer overflows during live samples: 0\n'
    printf 'RSS initial/maximum/final KiB: %s/%s/%s\n' \
      "$initial_rss" "$maximum_rss" "$final_rss"
    printf 'VSZ initial/maximum/final KiB: %s/%s/%s\n' \
      "$initial_vsz" "$maximum_vsz" "$final_vsz"
    printf 'Threads initial/maximum/final: %s/%s/%s\n' \
      "$initial_threads" "$maximum_threads" "$final_threads"
    printf 'File descriptors initial/maximum/final: %s/%s/%s\n' \
      "$initial_descriptors" "$maximum_descriptors" "$final_descriptors"
    printf 'Memory trend artifact: memory-analysis.kv (REPORT_ONLY)\n'
    printf 'Detailed memory artifact: process-memory-details.tsv\n'
  } >"$run_dir/summary.txt"

  if ! restore_snapshot "$run_dir"; then
    failure_reason="pre-benchmark database restoration failed"
    benchmark_outcome=FAIL
    exit 1
  fi
  cleanup_ok=true
  {
    printf 'Cleanup restored baseline: yes\n'
    printf 'Baseline vessel rows restored: %s\n' "$baseline_count"
  } >>"$run_dir/summary.txt"
  if [[ "$benchmark_outcome" == PASS ]]; then
    printf 'PASS: complete 500-vessel tick stayed within 25 ms.\n'
  else
    printf 'FAIL: %s\n' "$failure_reason"
    exit 1
  fi
}

restore_internal()
{
  local run_dir=$1

  case "$run_dir" in
    "$state_root"/runs/*)
      ;;
    *)
      fail "invalid internal restore directory"
      ;;
  esac
  [[ -d "$run_dir" ]] || fail "restore directory is missing"
  exec 8>"$run_dir/cleanup.lock"
  flock -w 600 8 || fail "timed out waiting for benchmark cleanup"
  restore_snapshot "$run_dir" ||
    fail "could not restore the pre-benchmark database"
}

case "${1:-}" in
  start)
    shift
    start_run "$@"
    ;;
  status)
    (($# == 1)) || usage
    show_status
    ;;
  cleanup)
    (($# == 1)) || usage
    cleanup_run
    ;;
  __run)
    (($# == 3)) || usage
    run_benchmark "$2" "$3"
    ;;
  __restore)
    (($# == 2)) || usage
    restore_internal "$2"
    ;;
  __parse-live-system)
    (($# == 3)) || usage
    parse_live_system_samples "$2" "$3"
    ;;
  __count-progress-logs)
    (($# == 2)) || usage
    count_high_volume_progress_logs "$2"
    ;;
  __validate-live-system)
    (($# == 3)) || usage
    validate_live_system_samples "$2" "$3"
    ;;
  *)
    usage
    ;;
esac
