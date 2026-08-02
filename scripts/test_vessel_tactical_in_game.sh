#!/usr/bin/env bash

set -Eeuo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo_root=${LUMINARI_PROJECT_ROOT:-$(cd "$script_dir/.." && pwd)}
acceptance_mode=tactical
if [[ $# -gt 0 ]]; then
  [[ $# -eq 1 && "$1" == --lookout ]] || {
    printf 'usage: %s [--lookout]\n' "$0" >&2
    exit 2
  }
  acceptance_mode=lookout
fi
server_unit=luminari-dev-login-smoke.service
server_log="${TMPDIR:-/tmp}/luminari-dev-login-smoke.log"
target_player=Kohdee
player_file="$repo_root/lib/plrfiles/K-O/kohdee.plr"
state_root="${TMPDIR:-/tmp}/luminari-vessel-${acceptance_mode}-check-${UID}"
run_id="$(date -u +%Y%m%dT%H%M%SZ)-$$"
run_dir="$state_root/runs/$run_id"
started_epoch=$(date +%s)
database_host=
database_name=
database_user=
database_password=
mud_port=
candidate_sha256=
source_commit=
baseline_player_sha256=
warship_prototype_id=
snapshot_ready=false
cleanup_needed=false
acceptance_complete=false
server_restart_needed=false

umask 077
mkdir -p "$run_dir"

fail()
{
  printf 'vessel %s in-game check: %s\n' "$acceptance_mode" "$*" >&2
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

database_apply_file()
{
  local sql_file=$1

  MYSQL_PWD="$database_password" mariadb --no-defaults \
    --host="$database_host" --user="$database_user" "$database_name" \
    <"$sql_file"
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

tactical_runtime_slots()
{
  database_query "
    SELECT COALESCE(GROUP_CONCAT(ship_id ORDER BY ship_id SEPARATOR ','), '')
      FROM ship_runtime_state
     WHERE prototype_id = $warship_prototype_id;"
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

  slots=$(tactical_runtime_slots) || return 1
  cleanup_commands=('goto 1204')
  if [[ -n "$slots" ]]; then
    IFS=',' read -r -a slot_list <<<"$slots"
    for ship_slot in "${slot_list[@]}"; do
      cleanup_commands+=("shippurge $ship_slot")
    done
  fi
  cleanup_commands+=('goto 1204')
  run_kohdee_commands "$run_dir/recovery-ships.log" \
    "${cleanup_commands[@]}" || return 1
  [[ -z $(tactical_runtime_slots) ]]
}

restore_baseline()
{
  local cleanup_status=0
  local restored_sha256
  local restore_tmp="$repo_root/lib/plrfiles/K-O/.kohdee.plr.vessel-view-restore-$$"

  [[ "$snapshot_ready" == true ]] || return 0

  retire_test_runtime || cleanup_status=1
  stop_development_mud || cleanup_status=1
  if cp --preserve=mode,ownership,timestamps \
       "$run_dir/kohdee.plr.before" "$restore_tmp" &&
     mv -f "$restore_tmp" "$player_file"; then
    restored_sha256=$(sha256sum "$player_file" | awk '{ print $1 }')
    [[ "$restored_sha256" == "$baseline_player_sha256" ]] || cleanup_status=1
  else
    cleanup_status=1
  fi

  if [[ "$cleanup_status" == 0 ]]; then
    start_server_without_login || cleanup_status=1
  fi
  if [[ "$cleanup_status" == 0 ]]; then
    [[ $(running_binary_sha256) == "$candidate_sha256" ]] || cleanup_status=1
    [[ -z $(tactical_runtime_slots) ]] || cleanup_status=1
  fi
  if [[ "$cleanup_status" == 0 ]]; then
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
      printf 'temporary_runtimes=0 cleanup=restored\n'
    } >"$run_dir/result"
    if [[ "$acceptance_mode" == tactical ]]; then
      printf 'PASS: Kohdee validated the wilderness tactical chart, live damage '
      printf 'contact, and coastal symbology with exact character restoration (%ss).\n' \
        "$elapsed_seconds"
    else
      printf 'PASS: Kohdee validated the wilderness lookout bearings, live contact, '
      printf 'and coastal sectors with exact character restoration (%ss).\n' \
        "$elapsed_seconds"
    fi
    printf 'Artifacts: %s\n' "$run_dir"
    exit 0
  fi

  printf 'FAIL command_status=%s cleanup_status=%s\n' \
    "$exit_status" "$cleanup_status" >"$run_dir/result"
  if [[ "$cleanup_status" != 0 ]]; then
    printf 'vessel %s in-game check: cleanup failed; inspect %s/cleanup.log\n' \
      "$acceptance_mode" "$run_dir" >&2
  fi
  printf 'Artifacts: %s\n' "$run_dir" >&2
  [[ "$exit_status" != 0 ]] && exit "$exit_status"
  exit 1
}

trap finish EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

for command_name in awk cp date env find flock git grep mariadb mkdir mv \
  sha256sum sleep ss systemctl systemd-run timeout; do
  command -v "$command_name" >/dev/null 2>&1 ||
    fail "required command not found: $command_name"
done

exec 8>"${TMPDIR:-/tmp}/luminari-vessel-view-check-${UID}.lock"
flock -n 8 || fail "another vessel view acceptance check is running"

[[ -r "$repo_root/lib/.env" ]] || fail "cannot read lib/.env"
[[ -r "$repo_root/lib/mysql_config" ]] || fail "cannot read lib/mysql_config"
[[ -x "$repo_root/bin/circle" ]] || fail "bin/circle is missing; run make install"
[[ -x "$script_dir/dev_kohdee_login_smoke.sh" ]] ||
  fail "the local character login helper is unavailable"
[[ -f "$player_file" && ! -L "$player_file" ]] ||
  fail "the Kohdee player file is missing or unsafe to replace"
grep -Fqx "Name: $target_player" "$player_file" ||
  fail "the expected Kohdee identity is not in $player_file"

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
warship_prototype_id=$(database_query "
  SELECT prototype_id
    FROM ship_prototypes
   WHERE name = 'Starfall Bastion'
     AND vessel_class = 3;")
[[ "$warship_prototype_id" =~ ^[1-9][0-9]*$ ]] ||
  fail "the Starfall Bastion acceptance prototype is unavailable or duplicated"

frontier_region_state=$(database_query "
  SELECT COUNT(*)
    FROM region_data
   WHERE vnum = 7100101
     AND name = 'Starfall Trench'
     AND zone_vnum = 10000
     AND region_type = 5
     AND region_props = 96
     AND ST_AsText(region_polygon) =
         'POLYGON((896 221,904 221,904 229,896 229,896 221))';")
[[ "$frontier_region_state" == 1 ]] ||
  fail "the canonical Starfall Trench test region is unavailable"
[[ -z $(tactical_runtime_slots) ]] ||
  fail "the Starfall Bastion prototype already has a runtime vessel"

stop_development_mud || fail "the development MUD did not stop"
database_apply_file "$repo_root/sql/components/help_vessel_entries.sql"
cp --preserve=mode,ownership,timestamps "$player_file" \
  "$run_dir/kohdee.plr.before"
baseline_player_sha256=$(sha256sum "$run_dir/kohdee.plr.before" |
  awk '{ print $1 }')
snapshot_ready=true
cleanup_needed=true

{
  printf 'source_commit=%s\n' "$source_commit"
  printf 'binary_sha256=%s\n' "$candidate_sha256"
  printf 'warship_prototype_id=%s\n' "$warship_prototype_id"
  printf 'baseline_player_sha256=%s\n' "$baseline_player_sha256"
} >"$run_dir/metadata"

start_server_without_login || fail "the current development MUD did not start"
[[ $(running_binary_sha256) == "$candidate_sha256" ]] ||
  fail "the running MUD does not match the installed candidate"

if [[ "$acceptance_mode" == tactical ]]; then
  timeout 120 env DEV_MUD_CHARACTER="$target_player" \
    "$script_dir/dev_kohdee_login_smoke.sh" --help-check TACTICAL \
    >"$run_dir/01-tactical-help.log" 2>&1 ||
    fail "Kohdee could not read the authoritative TACTICAL help"

  timeout 300 env DEV_MUD_CHARACTER="$target_player" \
    "$script_dir/dev_kohdee_login_smoke.sh" --vessel-tactical-check \
    "$warship_prototype_id" >"$run_dir/02-kohdee-vessel-tactical.log" 2>&1 ||
    fail "the actual Kohdee vessel-tactical session failed"

  for expected_text in \
    'PASS: wilderness tactical terrain, two range rings' \
    'PASS: a real contact changed from sound to damaged' \
    'PASS: the coastal chart rendered actual shoal, beach, and coastline cells.' \
    'PASS: the vessel tactical check completed and purged all temporary hulls'; do
    grep -Fq "$expected_text" "$run_dir/02-kohdee-vessel-tactical.log" ||
      fail "the tactical session did not report '$expected_text'"
  done

  for expected_text in \
    'WILDERNESS TACTICAL CHART' \
    'Starfall Trench (bathymetric)' \
    'Starfall Bastion         sound' \
    'Starfall Bastion         battered'; do
    grep -Fq "$expected_text" "$run_dir/02-kohdee-vessel-tactical.log" ||
      fail "the tactical transcript did not contain '$expected_text'"
  done
else
  timeout 120 env DEV_MUD_CHARACTER="$target_player" \
    "$script_dir/dev_kohdee_login_smoke.sh" --help-check LOOK_OUTSIDE \
    >"$run_dir/01-lookout-help.log" 2>&1 ||
    fail "Kohdee could not read the authoritative LOOK_OUTSIDE help"
  grep -Fq 'scan canonical wilderness' "$run_dir/01-lookout-help.log" ||
    fail "the authoritative LOOK_OUTSIDE help is stale"

  timeout 300 env DEV_MUD_CHARACTER="$target_player" \
    "$script_dir/dev_kohdee_login_smoke.sh" --vessel-lookout-check \
    "$warship_prototype_id" >"$run_dir/02-kohdee-vessel-lookout.log" 2>&1 ||
    fail "the actual Kohdee vessel-lookout session failed"

  for expected_text in \
    'PASS: the lookout used all eight canonical wilderness bearings' \
    'PASS: the lookout reported a real nearby vessel' \
    'PASS: the coastal lookout reported actual shoal, beach, and forest sectors.' \
    'PASS: the vessel lookout check completed and purged all temporary hulls'; do
    grep -Fq "$expected_text" "$run_dir/02-kohdee-vessel-lookout.log" ||
      fail "the lookout session did not report '$expected_text'"
  done

  for expected_text in \
    'LOOKOUT VIEW FROM Starfall Bastion' \
    'Surrounding wilderness (sampled to the visible horizon):' \
    'Visible vessels (nearest first):' \
    'Current sector: Ocean'; do
    grep -Fq "$expected_text" "$run_dir/02-kohdee-vessel-lookout.log" ||
      fail "the lookout transcript did not contain '$expected_text'"
  done
fi

[[ -z $(tactical_runtime_slots) ]] ||
  fail "a temporary Starfall Bastion runtime remained"
grep -Fqx 'Room: 1204' "$player_file" ||
  fail "Kohdee did not return to room 1204"
if grep -E 'SYSERR:.*(tactical|lookout|Starfall Bastion|Starfall Trench)' \
     "$server_log" >"$run_dir/03-related-syserr.log"; then
  fail "the server logged a vessel-view SYSERR"
fi

acceptance_complete=true
