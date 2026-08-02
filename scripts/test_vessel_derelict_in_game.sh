#!/usr/bin/env bash

set -Eeuo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo_root=${LUMINARI_PROJECT_ROOT:-$(cd "$script_dir/.." && pwd)}
server_unit=luminari-dev-login-smoke.service
target_player=Kohdee
derelict_name='Blackwake Derelict'
player_file="$repo_root/lib/plrfiles/K-O/kohdee.plr"
player_index_file="$repo_root/lib/plrfiles/index"
object_file="$repo_root/lib/plrobjs/K-O/kohdee.objs"
variable_file="$repo_root/lib/plrvars/K-O/kohdee.mem"
state_root="${TMPDIR:-/tmp}/luminari-vessel-derelict-check-${UID}"
run_id="$(date -u +%Y%m%dT%H%M%SZ)-$$"
run_dir="$state_root/runs/$run_id"
server_log="$run_dir/server.log"
started_epoch=$(date +%s)
database_host=
database_name=
database_user=
database_password=
mud_port=
candidate_sha256=
source_commit=
derelict_slot=
derelict_bridge_room=
baseline_derelict_state=
restart_derelict_state=
baseline_gold=
observed_gold=
snapshot_ready=false
cleanup_needed=false
acceptance_complete=false
state_keys=(player player_index objects variables)
declare -A state_paths
declare -A state_snapshots
declare -A state_present
declare -A state_sha256
declare -A state_stat

state_paths[player]=$player_file
state_paths[player_index]=$player_index_file
state_paths[objects]=$object_file
state_paths[variables]=$variable_file
state_snapshots[player]="$run_dir/kohdee.plr.before"
state_snapshots[player_index]="$run_dir/player-index.before"
state_snapshots[objects]="$run_dir/kohdee.objs.before"
state_snapshots[variables]="$run_dir/kohdee.mem.before"

umask 077
mkdir -p "$run_dir"

fail()
{
  printf 'vessel derelict in-game check: %s\n' "$*" >&2
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

database_query()
{
  local query=$1

  MYSQL_PWD="$database_password" mariadb --no-defaults --batch \
    --skip-column-names --host="$database_host" --user="$database_user" \
    "$database_name" --execute="$query"
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

running_binary_sha256()
{
  local server_pid

  server_pid=$(systemctl --user show --property=MainPID --value "$server_unit")
  [[ "$server_pid" =~ ^[1-9][0-9]*$ ]] || return 1
  sha256sum "/proc/$server_pid/exe" | awk '{ print $1 }'
}

stop_server()
{
  local attempt

  if systemctl --user is-active --quiet "$server_unit"; then
    systemctl --user stop "$server_unit"
  fi
  for ((attempt = 0; attempt < 300; attempt++)); do
    port_is_listening || return 0
    sleep 0.1
  done
  return 1
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
    fi
  else
    port_is_listening &&
      fail "the development port is owned by an unsupervised process"
    timeout 90 env DEV_MUD_CHARACTER="$target_player" \
      "$script_dir/dev_kohdee_login_smoke.sh" \
      >"$run_dir/00-server-start.log" 2>&1 ||
      fail "the installed development MUD did not start"
  fi

  wait_for_server || fail "the development MUD is not ready"
  working_directory=$(
    systemctl --user show --property=WorkingDirectory --value "$server_unit"
  )
  [[ "$working_directory" == "$repo_root" ]] ||
    fail "the supervised MUD is running from $working_directory"
  running_sha256=$(running_binary_sha256)
  [[ "$running_sha256" == "$candidate_sha256" ]] ||
    fail "the running MUD does not match the installed candidate"
}

start_server_without_login()
{
  local attempt
  local launched=false

  : >>"$server_log"
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
  wait_for_server
}

run_kohdee_commands()
{
  local output_file=$1

  shift
  timeout 120 env DEV_MUD_CHARACTER="$target_player" \
    "$script_dir/dev_kohdee_login_smoke.sh" --commands "$@" \
    >"$output_file" 2>&1
}

snapshot_player_state()
{
  local key
  local path
  local snapshot

  for key in "${state_keys[@]}"; do
    path=${state_paths[$key]}
    snapshot=${state_snapshots[$key]}
    [[ ! -L "$path" ]] || fail "$path is a symlink"
    if [[ -f "$path" ]]; then
      cp --preserve=mode,ownership,timestamps "$path" "$snapshot" ||
        fail "could not snapshot $path"
      state_present[$key]=1
      state_sha256[$key]=$(sha256sum "$snapshot" | awk '{ print $1 }')
      state_stat[$key]=$(stat -c '%a|%u|%g|%Y|%s' "$snapshot")
    else
      state_present[$key]=0
      state_sha256[$key]=absent
      state_stat[$key]=absent
    fi
  done
}

verify_player_state_restored()
{
  local key
  local path
  local snapshot
  local restored_sha256
  local restored_stat

  for key in "${state_keys[@]}"; do
    path=${state_paths[$key]}
    snapshot=${state_snapshots[$key]}
    if [[ ${state_present[$key]} == 1 ]]; then
      [[ -f "$path" && ! -L "$path" ]] || return 1
      cmp -s "$snapshot" "$path" || return 1
      restored_sha256=$(sha256sum "$path" | awk '{ print $1 }')
      restored_stat=$(stat -c '%a|%u|%g|%Y|%s' "$path")
      [[ "$restored_sha256" == "${state_sha256[$key]}" &&
         "$restored_stat" == "${state_stat[$key]}" ]] || return 1
    else
      [[ ! -e "$path" ]] || return 1
    fi
  done
}

restore_player_state()
{
  local key
  local path
  local snapshot
  local restore_tmp

  for key in "${state_keys[@]}"; do
    path=${state_paths[$key]}
    snapshot=${state_snapshots[$key]}
    [[ ! -L "$path" ]] || return 1
    if [[ ${state_present[$key]} == 1 ]]; then
      restore_tmp="${path}.derelict-restore-$$"
      cp --preserve=mode,ownership,timestamps "$snapshot" "$restore_tmp" ||
        return 1
      mv -f "$restore_tmp" "$path" || return 1
    elif [[ -e "$path" ]]; then
      unlink "$path" || return 1
    fi
  done
}

restore_baseline()
{
  local cleanup_status=0
  local restored_derelict_state
  local running_sha256

  [[ "$snapshot_ready" == true ]] || return 0

  printf 'Stopping the development MUD before restoring Kohdee.\n'
  stop_server || cleanup_status=1
  if [[ "$cleanup_status" == 0 ]]; then
    restore_player_state || cleanup_status=1
  fi
  if [[ "$cleanup_status" == 0 ]]; then
    verify_player_state_restored || cleanup_status=1
  fi

  restored_derelict_state=$(database_query "
    SELECT CONCAT(runtime.ship_id, '|', runtime.prototype_id, '|',
                  ROUND(runtime.x), '|', ROUND(runtime.y), '|', runtime.speed, '|',
                  runtime.autopilot_state, '|', interior.room_vnums, '|',
                  interior.bridge_room, '|', interior.entrance_room, '|',
                  interior.cargo_room1, '|', interior.owner)
      FROM ship_runtime_state AS runtime
      JOIN ship_interiors AS interior ON interior.ship_id = runtime.ship_id
     WHERE runtime.ship_id = $derelict_slot;") || cleanup_status=1
  [[ "$restored_derelict_state" == "$baseline_derelict_state" ]] ||
    cleanup_status=1

  if [[ "$cleanup_status" == 0 ]]; then
    start_server_without_login || cleanup_status=1
  fi
  if [[ "$cleanup_status" == 0 ]]; then
    running_sha256=$(running_binary_sha256)
    [[ "$running_sha256" == "$candidate_sha256" ]] || cleanup_status=1
    verify_player_state_restored || cleanup_status=1
  fi

  if [[ "$cleanup_status" == 0 ]]; then
    cleanup_needed=false
    printf 'Restored Kohdee player, index, object, and DG-variable files exactly.\n'
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
    restore_baseline >>"$run_dir/cleanup.log" 2>&1
    cleanup_status=$?
    set -e
  fi

  elapsed_seconds=$(($(date +%s) - started_epoch))
  if [[ "$exit_status" == 0 && "$cleanup_status" == 0 &&
        "$acceptance_complete" == true ]]; then
    {
      printf 'PASS elapsed=%s derelict_slot=%s gold=%s->%s ' \
        "$elapsed_seconds" "$derelict_slot" "$baseline_gold" "$observed_gold"
      printf 'restart=stable cleanup=restored\n'
    } >"$run_dir/result"
    printf 'PASS: Kohdee completed the Blackwake discovery chain across a hard restart, '
    printf 'salvaged 180 gold, and was restored exactly (%ss).\n' "$elapsed_seconds"
    printf 'Artifacts: %s\n' "$run_dir"
    exit 0
  fi

  printf 'FAIL elapsed=%s command_status=%s cleanup_status=%s\n' \
    "$elapsed_seconds" "$exit_status" "$cleanup_status" >"$run_dir/result"
  if [[ "$cleanup_status" != 0 ]]; then
    printf 'vessel derelict in-game check: cleanup failed; inspect %s/cleanup.log\n' \
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

for command_name in awk cmp cp date env find flock git grep mariadb mkdir \
  mv sed sha256sum sleep ss stat systemctl systemd-run timeout unlink; do
  command -v "$command_name" >/dev/null 2>&1 ||
    fail "required command not found: $command_name"
done

exec 8>"${TMPDIR:-/tmp}/luminari-vessel-derelict-check-${UID}.lock"
flock -n 8 || fail "another derelict acceptance check is already running"

[[ -r "$repo_root/lib/.env" ]] || fail "cannot read lib/.env"
[[ -r "$repo_root/lib/mysql_config" ]] || fail "cannot read lib/mysql_config"
[[ -x "$repo_root/bin/circle" ]] || fail "bin/circle is missing; run make install"
[[ -x "$script_dir/dev_kohdee_login_smoke.sh" ]] ||
  fail "the local character login helper is unavailable"
[[ -f "$player_file" && ! -L "$player_file" ]] ||
  fail "the exact Kohdee player file is missing or unsafe"
[[ -f "$player_index_file" && ! -L "$player_index_file" ]] ||
  fail "the player index is missing or unsafe"
grep -Fqx "Name: $target_player" "$player_file" ||
  fail "the expected Kohdee identity is not in $player_file"

app_environment=$(config_value "$repo_root/lib/.env" APP_ENV)
[[ "$app_environment" == development ]] ||
  fail "refusing to run because APP_ENV is not development"
configured_character=$(config_value "$repo_root/lib/.env" DEV_MUD_CHARACTER)
[[ -z "$configured_character" ||
   "${configured_character,,}" == "${target_player,,}" ]] ||
  fail "DEV_MUD_CHARACTER must be Kohdee"

active_workload_unit=$(active_vessel_workload)
[[ -z "$active_workload_unit" ]] ||
  fail "$active_workload_unit owns the development MUD"
[[ -z $(git -C "$repo_root" status --porcelain --untracked-files=all) ]] ||
  fail "the source worktree must be clean"
stale_binary_input=$(newer_binary_input "$repo_root" "$repo_root/bin/circle") ||
  fail "could not compare the installed MUD with current build inputs"
[[ -z "$stale_binary_input" ]] ||
  fail "bin/circle is older than $stale_binary_input; run make test and make install"

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
source_commit=$(git -C "$repo_root" rev-parse HEAD)
start_current_server

derelict_slot=$(database_query "
  SELECT runtime.ship_id
    FROM ship_runtime_state AS runtime
    JOIN ship_interiors AS interior ON interior.ship_id = runtime.ship_id
    JOIN ship_prototypes AS prototype
      ON prototype.prototype_id = runtime.prototype_id
   WHERE prototype.name = '$derelict_name'
     AND interior.vessel_name = '$derelict_name'
     AND interior.owner = '';")
[[ "$derelict_slot" =~ ^[0-9]+$ && "$derelict_slot" -le 500 ]] ||
  fail "the provisioned Blackwake derelict is unavailable"

derelict_bridge_room=$(database_query "
  SELECT bridge_room FROM ship_interiors WHERE ship_id = $derelict_slot;")
[[ "$derelict_bridge_room" =~ ^[1-9][0-9]*$ ]] ||
  fail "the Blackwake bridge room is unavailable"

content_valid=$(database_query "
  SELECT IF(
    (SELECT COUNT(*) FROM ship_room_template_triggers
      WHERE vessel_type = 0
        AND (room_type, trigger_vnum) IN (
          ('bridge', 70010),
          ('quarters_crew', 70011),
          ('cargo_main', 70012))) = 3
    AND
    (SELECT COUNT(*)
       FROM ship_runtime_state AS runtime
       JOIN ship_interiors AS interior ON interior.ship_id = runtime.ship_id
      WHERE runtime.ship_id = $derelict_slot
        AND runtime.speed = 0
        AND runtime.autopilot_state = 0
        AND interior.num_rooms BETWEEN 4 AND 8
        AND interior.cargo_room1 > 0) = 1,
    1, 0);")
[[ "$content_valid" == 1 ]] ||
  fail "the Blackwake runtime or DG mappings are incomplete"

run_kohdee_commands "$run_dir/01-preflight.log" \
  "shipgoto $derelict_slot" 'shipstatus' 'goto 1204' ||
  fail "Kohdee could not inspect the provisioned Blackwake hull"
grep -Fq "Ship Name: $derelict_name" "$run_dir/01-preflight.log" ||
  fail "shipstatus did not expose the Blackwake identity"

stop_server || fail "the development MUD did not stop for the snapshot"
snapshot_player_state
baseline_gold=$(player_file_value Gold "${state_snapshots[player]}")
[[ "$baseline_gold" =~ ^[0-9]+$ ]] || fail "Kohdee's baseline gold is malformed"
grep -Fqx 'Room: 1204' "${state_snapshots[player]}" ||
  fail "Kohdee did not leave the preflight in the staff room"

baseline_derelict_state=$(database_query "
  SELECT CONCAT(runtime.ship_id, '|', runtime.prototype_id, '|',
                ROUND(runtime.x), '|', ROUND(runtime.y), '|', runtime.speed, '|',
                runtime.autopilot_state, '|', interior.room_vnums, '|',
                interior.bridge_room, '|', interior.entrance_room, '|',
                interior.cargo_room1, '|', interior.owner)
    FROM ship_runtime_state AS runtime
    JOIN ship_interiors AS interior ON interior.ship_id = runtime.ship_id
   WHERE runtime.ship_id = $derelict_slot;")
[[ -n "$baseline_derelict_state" ]] || fail "could not snapshot derelict identity"

snapshot_ready=true
cleanup_needed=true
{
  printf 'source_commit=%s\n' "$source_commit"
  printf 'binary_sha256=%s\n' "$candidate_sha256"
  printf 'derelict_slot=%s\n' "$derelict_slot"
  printf 'derelict_bridge_room=%s\n' "$derelict_bridge_room"
  printf 'baseline_gold=%s\n' "$baseline_gold"
  printf 'baseline_derelict_state=%s\n' "$baseline_derelict_state"
  for key in "${state_keys[@]}"; do
    printf '%s_present=%s\n' "$key" "${state_present[$key]}"
    printf '%s_sha256=%s\n' "$key" "${state_sha256[$key]}"
    printf '%s_stat=%s\n' "$key" "${state_stat[$key]}"
  done
} >"$run_dir/metadata"

start_server_without_login || fail "the no-login snapshot restart failed"

run_kohdee_commands "$run_dir/03-discovery-before-restart.log" \
  "shipgoto $derelict_slot" \
  'set Kohdee level 30' \
  'north' \
  'searchashchart' \
  'south' \
  'searchashlog' \
  'inventory' \
  'readashlog' \
  'north' \
  'searchashchart' \
  'inventory' \
  'south' \
  'east' \
  'recoverashsalvage' \
  'west' ||
  fail "the actual Kohdee discovery session failed"

for expected_text in \
  "Kohdee's level set to 30." \
  'You lack the clue needed to choose among them.' \
  "your hand closes around an ash-stained captain's log" \
  'Fire below. Quartermaster secured the reef soundings beneath berth three.' \
  'SEARCHASHCHART there to follow the captain' \
  "Inside waits the Blackwake's salt-stiff chart." \
  "the Blackwake's salt-stiff chart"; do
  grep -Fq "$expected_text" "$run_dir/03-discovery-before-restart.log" ||
    fail "the first discovery session missed '$expected_text'"
done
grep -Fq 'Buckled panels line the hold' \
  "$run_dir/03-discovery-before-restart.log" ||
  fail "cargo salvage was not gated behind studying the chart"

[[ -f "$variable_file" && -f "$object_file" ]] ||
  fail "the first discovery stage did not persist player state"
for expected_var in blackwake_log_found blackwake_log_read \
  blackwake_chart_found; do
  grep -Eq "^${expected_var}[[:space:]]+0[[:space:]]+1$" "$variable_file" ||
    fail "$expected_var did not persist before restart"
done
if grep -Eq '^blackwake_(chart_read|salvage_recovered)[[:space:]]' \
  "$variable_file"; then
  fail "a later Blackwake discovery stage completed too early"
fi
grep -Fqx '#70010' "$object_file" || fail "the captain log was not saved"
grep -Fqx '#70011' "$object_file" || fail "the chart was not saved"
grep -Fqx "Gold: $baseline_gold" "$player_file" ||
  fail "the first discovery stage changed Kohdee's gold"
grep -Fqx 'Levl: 30' "$player_file" ||
  fail "Kohdee did not persist as a mortal DG target"
grep -Fqx "Room: $derelict_bridge_room" "$player_file" ||
  fail "Kohdee did not log out on the Blackwake bridge"

stop_server || fail "the development MUD did not stop for the hard restart"
start_server_without_login || fail "the hard restart did not start the MUD"

restart_derelict_state=$(database_query "
  SELECT CONCAT(runtime.ship_id, '|', runtime.prototype_id, '|',
                ROUND(runtime.x), '|', ROUND(runtime.y), '|', runtime.speed, '|',
                runtime.autopilot_state, '|', interior.room_vnums, '|',
                interior.bridge_room, '|', interior.entrance_room, '|',
                interior.cargo_room1, '|', interior.owner)
    FROM ship_runtime_state AS runtime
    JOIN ship_interiors AS interior ON interior.ship_id = runtime.ship_id
   WHERE runtime.ship_id = $derelict_slot;")
[[ "$restart_derelict_state" == "$baseline_derelict_state" ]] ||
  fail "the Blackwake identity changed across the discovery restart"

run_kohdee_commands "$run_dir/04-discovery-after-restart.log" \
  'look' \
  'north' \
  'studyashchart' \
  'south' \
  'east' \
  'recoverashsalvage' \
  'inventory' \
  'salvage tidefinder' \
  'recoverashsalvage' \
  'gold' \
  'west' \
  'south' \
  'look' \
  'north' ||
  fail "the post-restart discovery and salvage session failed"

for expected_text in \
  "Blackwake Derelict's Bridge" \
  'Soundings outline Blackwake reef' \
  'RECOVERASHSALVAGE from the marked panel' \
  'recover a corroded bronze gear' \
  'a corroded bronze tidefinder gear' \
  'salvaging 180 gold coins worth of materials' \
  'Its tidefinder gear has already been recovered.' \
  'Main Deck'; do
  grep -Fq "$expected_text" "$run_dir/04-discovery-after-restart.log" ||
    fail "the post-restart session missed '$expected_text'"
done

observed_gold=$((baseline_gold + 180))
grep -Fq "You have $observed_gold gold coins." \
  "$run_dir/04-discovery-after-restart.log" ||
  fail "the in-game gold command did not expose the salvage payment"
grep -Fqx "Gold: $observed_gold" "$player_file" ||
  fail "the 180-gold salvage payment did not persist"
grep -Fqx 'Levl: 30' "$player_file" ||
  fail "Kohdee's mortal acceptance level did not survive restart"
grep -Fqx "Room: $derelict_bridge_room" "$player_file" ||
  fail "Kohdee did not finish the acceptance run on the Blackwake bridge"

for expected_var in blackwake_log_found blackwake_log_read \
  blackwake_chart_found blackwake_chart_read blackwake_salvage_recovered; do
  grep -Eq "^${expected_var}[[:space:]]+0[[:space:]]+1$" "$variable_file" ||
    fail "$expected_var did not persist after restart"
done
grep -Fqx '#70010' "$object_file" ||
  fail "the captain log did not survive the hard restart"
grep -Fqx '#70011' "$object_file" ||
  fail "the chart did not survive the hard restart"
if grep -Fqx '#70012' "$object_file"; then
  fail "the salvaged tidefinder still exists in Kohdee's object file"
fi

if grep -E 'SYSERR:.*(Blackwake|7001[0-4])' "$server_log" \
  >"$run_dir/05-related-syserr.log"; then
  fail "the server logged a Blackwake discovery SYSERR"
fi

{
  printf 'derelict_state_before=%s\n' "$baseline_derelict_state"
  printf 'derelict_state_after_restart=%s\n' "$restart_derelict_state"
  printf 'gold_before=%s\n' "$baseline_gold"
  printf 'gold_after_salvage=%s\n' "$observed_gold"
  printf 'saved_objects=70010,70011\n'
  printf 'salvaged_object=70012\n'
  printf 'discovery_variables=5\n'
} >"$run_dir/observed-state"

acceptance_complete=true
