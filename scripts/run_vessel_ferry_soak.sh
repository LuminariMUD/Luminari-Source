#!/usr/bin/env bash

set -euo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(cd "$script_dir/.." && pwd)
state_root="${TMPDIR:-/tmp}/luminari-vessel-ferry-soak-${UID}"
current_pointer="$state_root/current"
server_unit="luminari-dev-login-smoke.service"
server_log="${TMPDIR:-/tmp}/luminari-dev-login-smoke.log"

fail()
{
  printf 'vessel ferry soak: %s\n' "$*" >&2
  exit 1
}

usage()
{
  cat >&2 <<'USAGE'
Usage:
  ./scripts/run_vessel_ferry_soak.sh start [duration [database_interval [live_interval]]]
  ./scripts/run_vessel_ferry_soak.sh status

Defaults are a 24-hour run, 60-second database/process samples, and hourly
actual-Kohdee samples. Durations are in seconds.
USAGE
  exit 1
}

require_integer()
{
  local name=$1
  local value=$2
  local minimum=$3

  [[ "$value" =~ ^[0-9]+$ ]] || fail "$name must be an integer"
  ((value >= minimum)) || fail "$name must be at least $minimum seconds"
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

start_run()
{
  local duration=${1:-86400}
  local database_interval=${2:-60}
  local live_interval=${3:-3600}
  local existing_dir
  local existing_metadata
  local existing_unit
  local existing_status
  local run_id
  local run_dir
  local unit_name
  local pointer_tmp

  (($# <= 3)) || usage
  require_integer duration "$duration" 60
  require_integer database_interval "$database_interval" 5
  require_integer live_interval "$live_interval" 15
  ((database_interval <= duration)) ||
    fail "database_interval cannot exceed duration"
  ((live_interval <= duration)) ||
    fail "live_interval cannot exceed duration"

  for command_name in date mkdir mv systemctl systemd-run; do
    command -v "$command_name" >/dev/null 2>&1 ||
      fail "required command not found: $command_name"
  done

  umask 077
  mkdir -p "$state_root/runs"

  if [[ -r "$current_pointer" ]]; then
    existing_dir=$(<"$current_pointer")
    existing_metadata="$existing_dir/metadata"
    if [[ -r "$existing_metadata" ]]; then
      existing_unit=$(metadata_value "$existing_metadata" unit)
      if [[ -n "$existing_unit" ]] &&
         systemctl --user is-active --quiet "$existing_unit"; then
        fail "a ferry soak is already active: $existing_unit"
      fi
      existing_status=""
      [[ -r "$existing_dir/status" ]] &&
        existing_status=$(<"$existing_dir/status")
      if [[ "$existing_status" == STARTING* || "$existing_status" == RUNNING* ]]; then
        printf 'ABANDONED superseded_at=%s\n' "$(date +%s)" \
          >"$existing_dir/status"
        {
          printf 'Result: ABANDONED\n'
          printf 'Reason: monitor unit ended without a terminal result\n'
          printf 'Superseded at: %s\n' "$(date +%s)"
        } >"$existing_dir/summary.txt"
      fi
    fi
  fi

  run_id="$(date -u +%Y%m%dT%H%M%SZ)-$$"
  run_dir="$state_root/runs/$run_id"
  unit_name="luminari-vessel-ferry-soak-${run_id}.service"
  mkdir "$run_dir"

  {
    printf 'run_id=%s\n' "$run_id"
    printf 'run_dir=%s\n' "$run_dir"
    printf 'unit=%s\n' "$unit_name"
    printf 'duration=%s\n' "$duration"
    printf 'database_interval=%s\n' "$database_interval"
    printf 'live_interval=%s\n' "$live_interval"
    printf 'requested_at=%s\n' "$(date +%s)"
  } >"$run_dir/metadata"
  printf 'STARTING\n' >"$run_dir/status"

  pointer_tmp="$state_root/current.$$"
  printf '%s\n' "$run_dir" >"$pointer_tmp"
  mv "$pointer_tmp" "$current_pointer"

  if ! systemd-run --user --quiet --collect \
    --unit="${unit_name%.service}" \
    --property="KillMode=mixed" \
    --property="WorkingDirectory=$repo_root" \
    --property="TimeoutStopSec=30" \
    "$script_dir/run_vessel_ferry_soak.sh" __run \
    "$run_dir" "$duration" "$database_interval" "$live_interval"; then
    printf 'LAUNCH_FAILED\n' >"$run_dir/status"
    fail "could not launch $unit_name"
  fi

  printf 'Started vessel ferry soak: unit=%s\n' "$unit_name"
  printf 'Run directory: %s\n' "$run_dir"
  printf 'Requested duration: %s seconds\n' "$duration"
}

show_status()
{
  local run_dir
  local metadata_file
  local unit_name
  local unit_state

  [[ -r "$current_pointer" ]] ||
    fail "no ferry soak has been started"
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
    cat "$run_dir/status"
  else
    printf 'UNKNOWN\n'
  fi

  if [[ -s "$run_dir/summary.txt" ]]; then
    printf '\n'
    cat "$run_dir/summary.txt"
  elif [[ -s "$run_dir/monitor.log" ]]; then
    printf '\nRecent monitor output:\n'
    tail -n 15 "$run_dir/monitor.log"
  fi
}

run_monitor()
{
  local run_dir=$1
  local duration=$2
  local database_interval=$3
  local live_interval=$4
  local app_environment
  local database_host
  local database_name
  local database_user
  local database_password
  local mud_port
  local ferry_rows_output
  local topology_valid
  local repair_output
  local started_epoch
  local end_epoch
  local next_database_sample
  local next_live_sample
  local next_keepalive_check
  local now
  local sleep_until
  local sleep_seconds

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

  run_complete=false
  failure_reason=""
  ferry_paused=false
  ferry_slot=""
  route_id=""
  keepalive_fd=""
  initial_mud_pid=""
  final_mud_pid=""
  log_offset=0
  movement_steps=0
  west_arrivals=0
  east_arrivals=0
  live_samples=0
  database_samples=0
  process_samples=0
  initial_rss=0
  maximum_rss=0
  final_rss=0
  persistence_verified=false

  fail_run()
  {
    failure_reason=$*
    printf 'FAIL: %s\n' "$failure_reason"
    exit 1
  }

  write_status()
  {
    printf '%s\n' "$*" >"$run_dir/status"
  }

  database_query()
  {
    local query=$1

    MYSQL_PWD="$database_password" mariadb --no-defaults --batch \
      --skip-column-names --host="$database_host" --user="$database_user" \
      "$database_name" --execute="$query"
  }

  close_keepalive()
  {
    if [[ -n "$keepalive_fd" ]]; then
      exec {keepalive_fd}>&- || true
      keepalive_fd=""
    fi
    return 0
  }

  read_keepalive_until()
  {
    local first_pattern=$1
    local second_pattern=${2:-}
    local character
    local character_count

    keepalive_reply=""
    for ((character_count = 0; character_count < 200000; character_count++)); do
      if ! IFS= read -r -t 5 -N 1 -u "$keepalive_fd" character; then
        return 1
      fi
      keepalive_reply+="$character"
      if [[ "$keepalive_reply" == *"$first_pattern"* ]]; then
        return 0
      fi
      if [[ -n "$second_pattern" &&
            "$keepalive_reply" == *"$second_pattern"* ]]; then
        return 0
      fi
      if ((${#keepalive_reply} > 2048)); then
        keepalive_reply=${keepalive_reply: -2048}
      fi
    done
    return 1
  }

  open_keepalive()
  {
    local account_name

    account_name="Vesselsoak$(printf '%s' "$$" | tr '0-9' 'abcdefghij')"
    if ! exec {keepalive_fd}<>"/dev/tcp/127.0.0.1/$mud_port"; then
      fail_run "could not open the game-loop keepalive connection"
    fi
    if ! read_keepalive_until "What is your account name"; then
      fail_run "the game-loop keepalive did not receive the account prompt"
    fi
    if ! printf '%s\r' "$account_name" >&"$keepalive_fd"; then
      fail_run "the game-loop keepalive could not submit its hold name"
    fi
    if ! read_keepalive_until "Did I get that right" "Password:"; then
      fail_run "the game-loop keepalive did not reach confirmation state"
    fi
    if [[ "$keepalive_reply" == *"Password:"* ]]; then
      fail_run "the generated game-loop hold name unexpectedly exists"
    fi
  }

  verify_keepalive()
  {
    local socket_rows

    socket_rows=$(ss -H -tnp state established \
      "( dport = :$mud_port )" 2>/dev/null || true)
    grep -Fq "pid=$$,fd=$keepalive_fd" <<<"$socket_rows" ||
      fail_run "the game-loop keepalive socket is no longer established"
  }

  resume_after_failure()
  {
    if [[ "$ferry_paused" != true || ! "$ferry_slot" =~ ^[0-9]+$ ]]; then
      return
    fi
    if ! systemctl --user is-active --quiet "$server_unit"; then
      return
    fi

    timeout 90 "$script_dir/dev_kohdee_login_smoke.sh" --commands \
      "shipgoto $ferry_slot" \
      "autopilot on" >"$run_dir/failure-resume.log" 2>&1 || true
    ferry_paused=false
  }

  finish_run()
  {
    local exit_code=$?
    local finished_epoch
    local unique_positions

    close_keepalive
    if [[ "$run_complete" != true ]]; then
      resume_after_failure
      finished_epoch=$(date +%s)
      [[ -n "$failure_reason" ]] || failure_reason="monitor exited with status $exit_code"
      write_status "FAIL finished=$finished_epoch reason=$failure_reason"
      unique_positions=0
      if [[ -s "$run_dir/positions.txt" ]]; then
        unique_positions=$(sort -u "$run_dir/positions.txt" | wc -l)
      fi
      {
        printf 'Result: FAIL\n'
        printf 'Reason: %s\n' "$failure_reason"
        printf 'Ferry slot: %s\n' "${ferry_slot:-unknown}"
        printf 'Movement steps: %s\n' "$movement_steps"
        printf 'Unique positions: %s\n' "$unique_positions"
        printf 'West/east arrivals: %s/%s\n' "$west_arrivals" "$east_arrivals"
        printf 'Live/database/process samples: %s/%s/%s\n' \
          "$live_samples" "$database_samples" "$process_samples"
      } >"$run_dir/summary.txt"
    fi
    return 0
  }
  trap finish_run EXIT
  trap 'failure_reason="monitor received SIGTERM"; exit 143' TERM
  trap 'failure_reason="monitor received SIGINT"; exit 130' INT

  run_live_sample()
  {
    local label=$1
    local expected_state=$2
    local output_file="$run_dir/live-${live_samples}-${label}.log"
    local fleet_line
    local coordinates
    local x
    local y
    local armor_rows
    local armor_name
    local armor_value
    local armor_maximum

    if ! "$script_dir/dev_kohdee_login_smoke.sh" --commands \
      "shiplist" \
      "shipgoto $ferry_slot" \
      "look ferrymaster" \
      "autopilot status" \
      "ship_rooms" \
      "showschedule" \
      "shipstatus" \
      "seastate" >"$output_file" 2>&1; then
      fail_run "actual Kohdee sample failed: $label"
    fi

    fleet_line=$(awk -v slot="$ferry_slot" '
      $1 == slot && index($0, "Harbor Sandbox Ferry") {
        print
        exit
      }
    ' "$output_file")
    [[ -n "$fleet_line" ]] ||
      fail_run "shiplist lost the ferry during $label"
    [[ "$fleet_line" == *"80/80"* ]] ||
      fail_run "ferry structure was not 80/80 during $label"

    grep -Fq "Aboard Harbor Sandbox Ferry (slot $ferry_slot)." "$output_file" ||
      fail_run "Kohdee could not enter the ferry during $label"
    grep -Fq "The harbor ferrymaster" "$output_file" ||
      fail_run "the ferrymaster was absent during $label"
    grep -Fq "Route: harbor_ferry_loop (4 waypoints, looping)" "$output_file" ||
      fail_run "live route topology changed during $label"
    grep -Fq "Vessel Type: Ship (4 rooms)" "$output_file" ||
      fail_run "live ferry interior topology changed during $label"
    grep -Fq "Interval: Every 1 MUD hour" "$output_file" ||
      fail_run "live ferry interval changed during $label"
    grep -Fq "Status: Active" "$output_file" ||
      fail_run "live ferry schedule was inactive during $label"
    grep -Fq "Pilot: Assigned" "$output_file" ||
      fail_run "live ferry schedule lost its pilot during $label"

    if [[ "$expected_state" == active ]]; then
      grep -Eq "^State: (Traveling|Waiting at Waypoint)$" "$output_file" ||
        fail_run "live ferry was not traveling or waiting during $label"
    else
      grep -Fq "State: Paused" "$output_file" ||
        fail_run "live ferry was not paused during $label"
    fi

    coordinates=$(sed -nE \
      's/^Coordinates: \((-?[0-9]+), (-?[0-9]+)\)$/\1 \2/p' \
      "$output_file" | tail -n 1)
    [[ "$coordinates" =~ ^-?[0-9]+[[:space:]]+-?[0-9]+$ ]] ||
      fail_run "could not read live coordinates during $label"
    read -r x y <<<"$coordinates"
    ((x >= -66 && x <= -62 && y >= 82 && y <= 92)) ||
      fail_run "live ferry left the harbor corridor at ($x,$y) during $label"

    armor_rows=$(sed -nE \
      's/^(Forward|Port|Starboard|Rear): ([0-9]+)\/([0-9]+)$/\1 \2 \3/p' \
      "$output_file")
    [[ $(wc -l <<<"$armor_rows") -eq 4 ]] ||
      fail_run "could not read all four armor arcs during $label"
    while read -r armor_name armor_value armor_maximum; do
      [[ "$armor_value" =~ ^[0-9]+$ && "$armor_maximum" == 20 ]] ||
        fail_run "invalid $armor_name armor during $label"
      ((armor_value >= 0 && armor_value <= armor_maximum)) ||
        fail_run "out-of-range $armor_name armor during $label"
    done <<<"$armor_rows"

    printf '%s\t%s\t%s\t%s\t%s\n' \
      "$(date +%s)" "$label" "$x" "$y" "$fleet_line" >>"$run_dir/live-samples.tsv"
    printf '%s %s\n' "$x" "$y" >>"$run_dir/positions.txt"
    last_live_x=$x
    last_live_y=$y
    live_samples=$((live_samples + 1))
  }

  sample_database()
  {
    local row
    local verdict
    local detail

    if ! row=$(database_query "
      SELECT
        IF(
          interior.vessel_name = 'Harbor Sandbox Ferry'
          AND interior.owner = ''
          AND interior.vessel_type = 2
          AND interior.num_rooms = 4
          AND interior.bridge_room > 0
          AND interior.entrance_room > 0
          AND interior.cargo_room1 > 0
          AND runtime.location_vnum > 0
          AND runtime.current_route_id = $route_id
          AND runtime.autopilot_state IN (1, 2)
          AND runtime.current_waypoint_index BETWEEN 0 AND 3
          AND ROUND(runtime.x) BETWEEN -66 AND -62
          AND ROUND(runtime.y) BETWEEN 82 AND 92
          AND ROUND(runtime.z) = 0
          AND runtime.finternal = runtime.maxfinternal
          AND runtime.rinternal = runtime.maxrinternal
          AND runtime.pinternal = runtime.maxpinternal
          AND runtime.sinternal = runtime.maxsinternal
          AND runtime.farmor BETWEEN 0 AND runtime.maxfarmor
          AND runtime.rarmor BETWEEN 0 AND runtime.maxrarmor
          AND runtime.parmor BETWEEN 0 AND runtime.maxparmor
          AND runtime.sarmor BETWEEN 0 AND runtime.maxsarmor
          AND runtime.mainsail BETWEEN 1 AND runtime.maxmainsail
          AND runtime.turnrate BETWEEN 1 AND runtime.maxturnrate
          AND (
            SELECT COUNT(*)
              FROM ship_crew_roster AS crew
             WHERE crew.ship_id = runtime.ship_id
               AND crew.crew_role = 'pilot'
               AND crew.npc_vnum = 70001
               AND crew.status = 'active'
          ) = 1
          AND (
            SELECT COUNT(*)
              FROM ship_schedules AS schedule
             WHERE schedule.ship_id = runtime.ship_id
               AND schedule.route_id = $route_id
               AND schedule.interval_hours = 1
               AND schedule.enabled = 1
          ) = 1,
          'PASS',
          'FAIL'
        ),
        CONCAT_WS(
          '|',
          runtime.ship_id,
          ROUND(runtime.x),
          ROUND(runtime.y),
          ROUND(runtime.z),
          runtime.autopilot_state,
          runtime.current_waypoint_index,
          runtime.farmor,
          runtime.rarmor,
          runtime.parmor,
          runtime.sarmor,
          runtime.finternal,
          runtime.rinternal,
          runtime.pinternal,
          runtime.sinternal,
          runtime.mainsail,
          runtime.turnrate,
          runtime.wear_ticks,
          interior.num_rooms,
          runtime.location_vnum
        )
      FROM ship_runtime_state AS runtime
      JOIN ship_interiors AS interior ON interior.ship_id = runtime.ship_id
      WHERE runtime.ship_id = $ferry_slot;"); then
      fail_run "database sample query failed"
    fi

    IFS=$'\t' read -r verdict detail <<<"$row"
    [[ "$verdict" == PASS && -n "$detail" ]] ||
      fail_run "database ferry invariant failed: ${detail:-missing row}"
    printf '%s\t%s\n' "$(date +%s)" "$detail" >>"$run_dir/database-samples.tsv"
    database_samples=$((database_samples + 1))
  }

  sample_process()
  {
    local current_pid
    local process_row
    local rss
    local vsz
    local threads
    local descriptor_count

    current_pid=$(systemctl --user show --property=MainPID --value "$server_unit")
    [[ "$current_pid" == "$initial_mud_pid" ]] ||
      fail_run "MUD PID changed during the continuous run: $initial_mud_pid -> $current_pid"
    [[ -r "/proc/$current_pid/status" ]] ||
      fail_run "MUD process $current_pid disappeared"

    process_row=$(ps -o rss=,vsz=,nlwp= -p "$current_pid")
    read -r rss vsz threads <<<"$process_row"
    [[ "$rss" =~ ^[0-9]+$ && "$vsz" =~ ^[0-9]+$ && "$threads" =~ ^[0-9]+$ ]] ||
      fail_run "could not read MUD process metrics"
    descriptor_count=$(find "/proc/$current_pid/fd" -mindepth 1 -maxdepth 1 | wc -l)

    if ((process_samples == 0)); then
      initial_rss=$rss
    fi
    ((rss > maximum_rss)) && maximum_rss=$rss
    final_rss=$rss
    printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
      "$(date +%s)" "$current_pid" "$rss" "$vsz" "$threads" \
      "$descriptor_count" >>"$run_dir/process-samples.tsv"
    process_samples=$((process_samples + 1))
  }

  sample_server_log()
  {
    local current_size
    local byte_count
    local chunk_file="$run_dir/server-log-chunk"
    local new_steps
    local new_west
    local new_east
    local x
    local y
    local ferry_error_pattern

    current_size=$(stat -c %s "$server_log")
    ((current_size >= log_offset)) ||
      fail_run "server log was truncated during the continuous run"
    byte_count=$((current_size - log_offset))
    ((byte_count > 0)) || return 0

    dd if="$server_log" of="$chunk_file" iflag=skip_bytes,count_bytes \
      skip="$log_offset" count="$byte_count" status=none
    log_offset=$current_size

    ferry_error_pattern="Autopilot ship $ferry_slot - (impassable terrain|failed)"
    ferry_error_pattern+="|SYSERR:.*(ship|vessel) $ferry_slot"
    if grep -Eiq "$ferry_error_pattern" "$chunk_file"; then
      grep -Ei "$ferry_error_pattern" "$chunk_file" \
        >>"$run_dir/ferry-errors.log" || true
      fail_run "server log reported a ferry movement or persistence error"
    fi
    if grep -Fq "No connections.  Going to sleep." "$chunk_file"; then
      grep -F "No connections.  Going to sleep." "$chunk_file" \
        >>"$run_dir/ferry-errors.log" || true
      fail_run "the MUD game loop went to sleep during the continuous run"
    fi

    grep -E \
      "Ship $ferry_slot (departing|arrived|wait complete|route)|Autopilot ship $ferry_slot" \
      "$chunk_file" >>"$run_dir/ferry-events.log" || true

    new_steps=$(grep -Fc "Ship $ferry_slot departing room" "$chunk_file" || true)
    new_west=$(grep -Fc \
      "Ship $ferry_slot arrived at waypoint 'harbor_west_dock'" "$chunk_file" || true)
    new_east=$(grep -Fc \
      "Ship $ferry_slot arrived at waypoint 'harbor_east_dock'" "$chunk_file" || true)
    movement_steps=$((movement_steps + new_steps))
    west_arrivals=$((west_arrivals + new_west))
    east_arrivals=$((east_arrivals + new_east))

    while read -r x y; do
      [[ "$x" =~ ^-?[0-9]+$ && "$y" =~ ^-?[0-9]+$ ]] ||
        fail_run "could not parse a logged ferry position"
      ((x >= -66 && x <= -62 && y >= 82 && y <= 92)) ||
        fail_run "logged ferry movement left the harbor corridor at ($x,$y)"
      printf '%s %s\n' "$x" "$y" >>"$run_dir/positions.txt"
    done < <(
      sed -nE \
        "s/.*Ship $ferry_slot departing.* at \\((-?[0-9]+), (-?[0-9]+)\\).*/\\1 \\2/p" \
        "$chunk_file"
    )
  }

  verify_persistence_restart()
  {
    local pause_output="$run_dir/pre-restart-pause.log"
    local post_output
    local resume_output="$run_dir/post-restart-resume.log"
    local pause_coordinates
    local pause_x
    local pause_y
    local post_x
    local post_y
    local database_coordinates
    local ready=false
    local attempt

    ferry_paused=true
    if ! "$script_dir/dev_kohdee_login_smoke.sh" --commands \
      "shipgoto $ferry_slot" \
      "autopilot pause" \
      "autopilot status" \
      "shipstatus" \
      "shiplist" >"$pause_output" 2>&1; then
      fail_run "could not pause the ferry for the persistence restart"
    fi
    grep -Fq "Autopilot paused." "$pause_output" ||
      fail_run "the ferry did not acknowledge the persistence pause"
    grep -Fq "State: Paused" "$pause_output" ||
      fail_run "the ferry was not paused before restart"
    grep -Fq "80/80" "$pause_output" ||
      fail_run "ferry structure was damaged before the persistence restart"
    pause_coordinates=$(sed -nE \
      's/^Coordinates: \((-?[0-9]+), (-?[0-9]+)\)$/\1 \2/p' \
      "$pause_output" | tail -n 1)
    [[ "$pause_coordinates" =~ ^-?[0-9]+[[:space:]]+-?[0-9]+$ ]] ||
      fail_run "could not read pre-restart ferry coordinates"
    read -r pause_x pause_y <<<"$pause_coordinates"

    database_coordinates=$(database_query "
      SELECT CONCAT(ROUND(x), ' ', ROUND(y))
        FROM ship_runtime_state
       WHERE ship_id = $ferry_slot
         AND autopilot_state = 3;")
    [[ "$database_coordinates" == "$pause_x $pause_y" ]] ||
      fail_run "paused runtime did not persist exact coordinates"

    close_keepalive
    systemctl --user restart "$server_unit" ||
      fail_run "could not restart the local MUD for persistence verification"

    for ((attempt = 0; attempt < 600; attempt++)); do
      final_mud_pid=$(systemctl --user show --property=MainPID --value "$server_unit")
      if systemctl --user is-active --quiet "$server_unit" &&
         [[ "$final_mud_pid" =~ ^[1-9][0-9]*$ ]] &&
         [[ "$final_mud_pid" != "$initial_mud_pid" ]] &&
         ss -H -ltn "sport = :$mud_port" 2>/dev/null | grep -q .; then
        ready=true
        break
      fi
      sleep 0.1
    done
    [[ "$ready" == true ]] ||
      fail_run "the local MUD did not return after the persistence restart"

    run_live_sample "post-restart" paused
    post_x=$last_live_x
    post_y=$last_live_y
    [[ "$post_x" == "$pause_x" && "$post_y" == "$pause_y" ]] ||
      fail_run "ferry coordinates changed across restart: ($pause_x,$pause_y) -> ($post_x,$post_y)"

    post_output="$run_dir/live-$((live_samples - 1))-post-restart.log"
    grep -Fq "Route: harbor_ferry_loop (4 waypoints, looping)" "$post_output" ||
      fail_run "ferry route was not restored after restart"

    if ! "$script_dir/dev_kohdee_login_smoke.sh" --commands \
      "shipgoto $ferry_slot" \
      "autopilot on" \
      "autopilot status" >"$resume_output" 2>&1; then
      fail_run "could not resume the ferry after persistence verification"
    fi
    grep -Fq "Autopilot resumed." "$resume_output" ||
      fail_run "the ferry did not acknowledge post-restart resume"
    grep -Fq "State: Traveling" "$resume_output" ||
      fail_run "the ferry was not traveling after post-restart resume"
    ferry_paused=false
    persistence_verified=true
  }

  for command_name in awk bash date dd find grep mariadb ps sed sleep sort ss \
    stat systemctl tail timeout tr wc; do
    command -v "$command_name" >/dev/null 2>&1 ||
      fail_run "required command not found: $command_name"
  done

  [[ -r "$repo_root/lib/.env" ]] ||
    fail_run "cannot read lib/.env"
  [[ -r "$repo_root/lib/mysql_config" ]] ||
    fail_run "cannot read lib/mysql_config"
  [[ -x "$repo_root/bin/circle" ]] ||
    fail_run "bin/circle is missing; build and install first"
  [[ -x "$script_dir/dev_kohdee_login_smoke.sh" ]] ||
    fail_run "the local character login helper is unavailable"
  [[ -f "$server_log" ]] ||
    fail_run "the local MUD log is unavailable"

  app_environment=$(config_value "$repo_root/lib/.env" APP_ENV)
  [[ "$app_environment" == development ]] ||
    fail_run "refusing to run because APP_ENV is not development"

  database_host=$(config_value "$repo_root/lib/mysql_config" mysql_host)
  database_name=$(config_value "$repo_root/lib/mysql_config" mysql_database)
  database_user=$(config_value "$repo_root/lib/mysql_config" mysql_username)
  database_password=$(config_value "$repo_root/lib/mysql_config" mysql_password)
  [[ -n "$database_host" && -n "$database_name" && -n "$database_user" ]] ||
    fail_run "lib/mysql_config is incomplete"

  mud_port=$(awk -F= '
    /^[[:space:]]*DFLT_PORT[[:space:]]*=/ {
      value = $2
      gsub(/[[:space:]]/, "", value)
      print value
      exit
    }
  ' "$repo_root/lib/etc/config")
  [[ "$mud_port" =~ ^[0-9]+$ ]] ||
    fail_run "could not read the development MUD port"
  systemctl --user is-active --quiet "$server_unit" ||
    fail_run "$server_unit is not active"
  initial_mud_pid=$(systemctl --user show --property=MainPID --value "$server_unit")
  [[ "$initial_mud_pid" =~ ^[1-9][0-9]*$ ]] ||
    fail_run "could not read the local MUD PID"
  ss -H -ltnp "sport = :$mud_port" 2>/dev/null | grep -q circle ||
    fail_run "the development port is not owned by circle"

  if ! ferry_rows_output=$(database_query "
    SELECT runtime.ship_id, route.route_id
      FROM ship_runtime_state AS runtime
      JOIN ship_interiors AS interior ON interior.ship_id = runtime.ship_id
      JOIN ship_routes AS route ON route.route_id = runtime.current_route_id
     WHERE interior.vessel_name = 'Harbor Sandbox Ferry'
       AND interior.owner = ''
       AND route.name = 'harbor_ferry_loop'
     ORDER BY runtime.ship_id;"); then
    fail_run "could not discover the persistent ferry"
  fi
  [[ -n "$ferry_rows_output" && $(wc -l <<<"$ferry_rows_output") -eq 1 ]] ||
    fail_run "expected exactly one active public harbor ferry"
  IFS=$'\t' read -r ferry_slot route_id <<<"$ferry_rows_output"
  [[ "$ferry_slot" =~ ^[0-9]+$ && "$route_id" =~ ^[0-9]+$ ]] ||
    fail_run "the persistent ferry identifiers are invalid"

  topology_valid=$(database_query "
    SELECT IF(
      COUNT(*) = 4
      AND SUM(
        CASE
          WHEN route_point.sequence_num = 0
            AND waypoint.name = 'harbor_west_dock'
            AND waypoint.x = -66 AND waypoint.y = 92 AND waypoint.wait_time = 15 THEN 1
          WHEN route_point.sequence_num = 1
            AND waypoint.name = 'harbor_channel_turn'
            AND waypoint.x = -64 AND waypoint.y = 82 AND waypoint.wait_time = 0 THEN 1
          WHEN route_point.sequence_num = 2
            AND waypoint.name = 'harbor_east_dock'
            AND waypoint.x = -62 AND waypoint.y = 82 AND waypoint.wait_time = 15 THEN 1
          WHEN route_point.sequence_num = 3
            AND waypoint.name = 'harbor_channel_turn'
            AND waypoint.x = -64 AND waypoint.y = 82 AND waypoint.wait_time = 0 THEN 1
          ELSE 0
        END
      ) = 4,
      1,
      0
    )
    FROM ship_route_waypoints AS route_point
    JOIN ship_waypoints AS waypoint ON waypoint.waypoint_id = route_point.waypoint_id
    WHERE route_point.route_id = $route_id;")
  [[ "$topology_valid" == 1 ]] ||
    fail_run "the ferry route is not the expected four-leg channel loop"

  open_keepalive
  verify_keepalive

  repair_output="$run_dir/initial-repair.log"
  if ! "$script_dir/dev_kohdee_login_smoke.sh" --commands \
    "shipgoto $ferry_slot" \
    "shipfix $ferry_slot" >"$repair_output" 2>&1; then
    fail_run "could not repair the ferry before the soak"
  fi
  grep -Fq "restored to full condition." "$repair_output" ||
    fail_run "the ferry repair did not report success"

  : >"$run_dir/positions.txt"
  run_live_sample initial active
  sample_database
  sample_process

  log_offset=$(stat -c %s "$server_log")
  started_epoch=$(date +%s)
  end_epoch=$((started_epoch + duration))
  next_database_sample=$((started_epoch + database_interval))
  next_live_sample=$((started_epoch + live_interval))
  next_keepalive_check=$((started_epoch + 20))
  {
    printf 'started_at=%s\n' "$started_epoch"
    printf 'ferry_slot=%s\n' "$ferry_slot"
    printf 'route_id=%s\n' "$route_id"
    printf 'initial_mud_pid=%s\n' "$initial_mud_pid"
  } >>"$run_dir/metadata"
  write_status \
    "RUNNING started=$started_epoch elapsed=0/$duration slot=$ferry_slot pid=$initial_mud_pid"
  printf 'Continuous ferry observation started at %s for %s seconds.\n' \
    "$started_epoch" "$duration"

  while :; do
    now=$(date +%s)
    ((now >= end_epoch)) && break

    if ((now >= next_database_sample)); then
      sample_server_log
      sample_database
      sample_process
      while ((next_database_sample <= now)); do
        next_database_sample=$((next_database_sample + database_interval))
      done
    fi

    if ((now >= next_live_sample)); then
      run_live_sample "elapsed-$((now - started_epoch))" active
      while ((next_live_sample <= now)); do
        next_live_sample=$((next_live_sample + live_interval))
      done
    fi

    if ((now >= next_keepalive_check)); then
      verify_keepalive
      while ((next_keepalive_check <= now)); do
        next_keepalive_check=$((next_keepalive_check + 20))
      done
    fi

    now=$(date +%s)
    write_status "RUNNING started=$started_epoch" \
      "elapsed=$((now - started_epoch))/$duration slot=$ferry_slot" \
      "steps=$movement_steps arrivals=$west_arrivals/$east_arrivals" \
      "live=$live_samples db=$database_samples"
    sleep_until=$end_epoch
    ((next_database_sample < sleep_until)) && sleep_until=$next_database_sample
    ((next_live_sample < sleep_until)) && sleep_until=$next_live_sample
    ((next_keepalive_check < sleep_until)) && sleep_until=$next_keepalive_check
    sleep_seconds=$((sleep_until - now))
    ((sleep_seconds > 0)) && sleep "$sleep_seconds"
  done

  sample_server_log
  sample_database
  sample_process
  run_live_sample final active

  unique_positions=$(sort -u "$run_dir/positions.txt" | wc -l)
  ((movement_steps >= 4)) ||
    fail_run "the ferry logged only $movement_steps movement steps"
  ((unique_positions >= 4)) ||
    fail_run "the ferry visited only $unique_positions distinct positions"
  ((west_arrivals >= 1 && east_arrivals >= 1)) ||
    fail_run "the ferry did not arrive at both docks"

  verify_persistence_restart

  finished_epoch=$(date +%s)
  observed_duration=$((finished_epoch - started_epoch))
  ((observed_duration >= duration)) ||
    fail_run "observed duration was shorter than requested"
  [[ "$persistence_verified" == true ]] ||
    fail_run "the final persistence restart was not verified"

  {
    printf 'Result: PASS\n'
    printf 'Requested continuous duration: %s seconds\n' "$duration"
    printf 'Observed wall duration: %s seconds\n' "$observed_duration"
    printf 'Ferry slot and route: %s/%s\n' "$ferry_slot" "$route_id"
    printf 'MUD PID during run: %s\n' "$initial_mud_pid"
    printf 'MUD PID after final persistence restart: %s\n' "$final_mud_pid"
    printf 'Movement steps: %s\n' "$movement_steps"
    printf 'Unique positions: %s\n' "$unique_positions"
    printf 'West/east arrivals: %s/%s\n' "$west_arrivals" "$east_arrivals"
    printf 'Live/database/process samples: %s/%s/%s\n' \
      "$live_samples" "$database_samples" "$process_samples"
    printf 'RSS initial/maximum/final KiB: %s/%s/%s\n' \
      "$initial_rss" "$maximum_rss" "$final_rss"
    printf 'Final restart preserved exact paused coordinates and route: yes\n'
    printf 'Ferry resumed after verification: yes\n'
  } >"$run_dir/summary.txt"
  write_status "PASS finished=$finished_epoch duration=$observed_duration slot=$ferry_slot"
  run_complete=true
  printf 'PASS: continuous ferry run and persistence restart verified.\n'
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
  __run)
    (($# == 5)) || usage
    run_monitor "$2" "$3" "$4" "$5"
    ;;
  *)
    usage
    ;;
esac
