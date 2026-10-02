#!/usr/bin/env bash

set -Eeuo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo_root=${LUMINARI_PROJECT_ROOT:-$(cd "$script_dir/../.." && pwd)}
acceptance_mode=tactical
if [[ $# -gt 0 ]]; then
  [[ $# -eq 1 ]] || {
    printf 'usage: %s [--lookout|--narrative|--boarding|--rules|--movement|--damage|--gunnery|--loss|--raider|--economy|--client]\n' "$0" >&2
    exit 2
  }
  case "$1" in
    --lookout)
      acceptance_mode=lookout
      ;;
    --narrative)
      acceptance_mode=narrative
      ;;
    --boarding)
      acceptance_mode=boarding
      ;;
    --rules)
      acceptance_mode=rules
      ;;
    --movement)
      acceptance_mode=movement
      ;;
    --damage)
      acceptance_mode=damage
      ;;
    --gunnery)
      acceptance_mode=gunnery
      ;;
    --loss)
      acceptance_mode=loss
      ;;
    --raider)
      acceptance_mode=raider
      ;;
    --economy)
      acceptance_mode=economy
      ;;
    --client)
      acceptance_mode=client
      ;;
    *)
      printf 'usage: %s [--lookout|--narrative|--boarding|--rules|--movement|--damage|--gunnery|--loss|--raider|--economy|--client]\n' "$0" >&2
      exit 2
      ;;
  esac
fi
server_unit=luminari-dev-login-smoke.service
server_log="${TMPDIR:-/tmp}/luminari-dev-login-smoke.log"
target_player=Kohdee
player_file="$repo_root/lib/plrfiles/K-O/kohdee.plr"
secondary_player=Vesselmate
secondary_player_file="$repo_root/lib/plrfiles/U-Z/vesselmate.plr"
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
baseline_secondary_player_sha256=
baseline_secondary_bounty=
warship_prototype_id=
contraband_unstocked=false
snapshot_ready=false
cleanup_needed=false
acceptance_complete=false
server_restart_needed=false

umask 077
mkdir -p "$run_dir"

fail() {
  printf 'vessel %s in-game check: %s\n' "$acceptance_mode" "$*" >&2
  exit 1
}

uses_secondary_player() {
  [[ "$acceptance_mode" == boarding || "$acceptance_mode" == rules ||
    "$acceptance_mode" == loss || "$acceptance_mode" == economy ]]
}

config_value() {
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

newer_binary_input() {
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

database_query() {
  local query=$1

  MYSQL_PWD="$database_password" mariadb --no-defaults --batch \
    --skip-column-names --host="$database_host" --user="$database_user" \
    "$database_name" --execute="$query"
}

database_apply_file() {
  local sql_file=$1

  MYSQL_PWD="$database_password" mariadb --no-defaults \
    --host="$database_host" --user="$database_user" "$database_name" \
    <"$sql_file"
}

port_is_listening() {
  ss -H -ltn "sport = :$mud_port" 2>/dev/null | grep -q .
}

active_vessel_workload() {
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

wait_for_server() {
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

stop_development_mud() {
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

start_server_without_login() {
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
      "$repo_root/bin/luminari" -d "$repo_root/lib"; then
      launched=true
      break
    fi
    sleep 0.1
  done
  [[ "$launched" == true ]] || return 1
  wait_for_server || return 1
  server_restart_needed=false
}

running_binary_sha256() {
  local server_pid

  server_pid=$(systemctl --user show --property=MainPID --value "$server_unit")
  [[ "$server_pid" =~ ^[1-9][0-9]*$ ]] || return 1
  sha256sum "/proc/$server_pid/exe" | awk '{ print $1 }'
}

tactical_runtime_slots() {
  database_query "
    SELECT COALESCE(GROUP_CONCAT(ship_id ORDER BY ship_id SEPARATOR ','), '')
      FROM ship_runtime_state
     WHERE prototype_id = $warship_prototype_id
        OR prototype_id IN (
          SELECT prototype_id
            FROM ship_prototypes
           WHERE name LIKE 'Movecheck Boat%' OR name LIKE 'Losscheck %'
              OR name LIKE 'Econcheck %'
        )
        OR ship_id IN (
          SELECT ship_id
            FROM ship_interiors
           WHERE owner = '$secondary_player'
        );"
}

run_kohdee_commands() {
  local output_file=$1

  shift
  timeout 150 env DEV_MUD_CHARACTER="$target_player" \
    "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" --commands "$@" \
    >"$output_file" 2>&1
}

retire_test_runtime() {
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

# The harbor East Dock stocks forbidden tomes (vessels_harbor_sandbox.sql);
# the economy check lifts that stock so customs meet the tomes it sold.
set_east_dock_tomes_stock() {
  local stocked=$1

  if [[ "$stocked" == true ]]; then
    database_query "
      INSERT IGNORE INTO port_commodities (port_vnum, commodity_id, supply)
      SELECT 1000390, commodity_id, 100
        FROM trade_commodities
       WHERE name = 'forbidden tomes';"
  else
    database_query "
      DELETE stock
        FROM port_commodities AS stock
        JOIN trade_commodities AS commodity
          ON commodity.commodity_id = stock.commodity_id
       WHERE stock.port_vnum = 1000390
         AND commodity.name = 'forbidden tomes';"
  fi
}

# A value the economy session reported, if any.
economy_value() {
  local key=$1
  local economy_log="$run_dir/02-kohdee-vessel-economy.log"

  [[ -f "$economy_log" ]] || return 0
  sed -n "s/^${key}=\([1-9][0-9]*\)\r\{0,1\}\$/\1/p" "$economy_log" | head -n 1
}

# The shipyard prototype the rules session reported creating, if any.
rules_prototype_id() {
  local rules_log="$run_dir/02-kohdee-vessel-rules.log"

  [[ -f "$rules_log" ]] || return 0
  sed -n 's/^rules_prototype_id=\([1-9][0-9]*\)\r\{0,1\}$/\1/p' "$rules_log" | head -n 1
}

restore_secondary_rules_state() {
  local bounty
  local marque_until
  local last_offense
  local prototype_id

  if [[ "$baseline_secondary_bounty" == absent ]]; then
    database_query "
      DELETE FROM vessel_bounties
       WHERE player_name = '$secondary_player';" || return 1
  elif [[ -n "$baseline_secondary_bounty" ]]; then
    IFS='|' read -r bounty marque_until last_offense <<<"$baseline_secondary_bounty"
    database_query "
      UPDATE vessel_bounties
         SET bounty = $bounty,
             marque_until = $marque_until,
             last_offense_at = FROM_UNIXTIME($last_offense)
       WHERE player_name = '$secondary_player';" || return 1
  fi
  prototype_id=$(rules_prototype_id) || return 1
  [[ -n "$prototype_id" ]] || return 0
  database_query "
    DELETE FROM ship_prototypes
     WHERE prototype_id = $prototype_id
       AND name LIKE 'Rulesraft%'
       AND NOT EXISTS (
         SELECT 1
           FROM ship_runtime_state AS runtime
          WHERE runtime.prototype_id = ship_prototypes.prototype_id);"
}

# Remove any temporary prototype a movement, loss, or economy session left
# behind.
restore_movement_state() {
  database_query "
    DELETE FROM ship_prototypes
     WHERE (name LIKE 'Movecheck Boat%' OR name LIKE 'Losscheck %'
            OR name LIKE 'Econcheck %')
       AND NOT EXISTS (
         SELECT 1
           FROM ship_runtime_state AS runtime
          WHERE runtime.prototype_id = ship_prototypes.prototype_id);"
}

restore_baseline() {
  local cleanup_status=0
  local restored_sha256
  local restore_tmp="$repo_root/lib/plrfiles/K-O/.kohdee.plr.vessel-view-restore-$$"
  local secondary_restored_sha256
  local secondary_restore_tmp="$repo_root/lib/plrfiles/U-Z/.vesselmate.plr.vessel-view-restore-$$"

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
  if uses_secondary_player; then
    if cp --preserve=mode,ownership,timestamps \
      "$run_dir/vesselmate.plr.before" "$secondary_restore_tmp" &&
      mv -f "$secondary_restore_tmp" "$secondary_player_file"; then
      secondary_restored_sha256=$(sha256sum "$secondary_player_file" |
        awk '{ print $1 }')
      [[ "$secondary_restored_sha256" == "$baseline_secondary_player_sha256" ]] ||
        cleanup_status=1
    else
      cleanup_status=1
    fi
  fi
  if [[ "$acceptance_mode" == rules ]]; then
    restore_secondary_rules_state || cleanup_status=1
  fi
  if [[ "$acceptance_mode" == movement || "$acceptance_mode" == loss ||
    "$acceptance_mode" == economy ]]; then
    restore_movement_state || cleanup_status=1
  fi
  if [[ "$contraband_unstocked" == true ]]; then
    set_east_dock_tomes_stock true || cleanup_status=1
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

finish() {
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
    elif [[ "$acceptance_mode" == lookout ]]; then
      printf 'PASS: Kohdee validated the wilderness lookout bearings, live contact, '
      printf 'and coastal sectors with exact character restoration (%ss).\n' \
        "$elapsed_seconds"
    elif [[ "$acceptance_mode" == boarding ]]; then
      printf 'PASS: Kohdee and Vesselmate validated opposed grappling, crossing, '
      printf 'breach warnings, and exact two-character restoration (%ss).\n' \
        "$elapsed_seconds"
    elif [[ "$acceptance_mode" == movement ]]; then
      printf 'PASS: Kohdee validated berths, departure, momentum, turning, maneuvers, '
      printf 'and anchoring with exact character restoration (%ss).\n' "$elapsed_seconds"
    elif [[ "$acceptance_mode" == damage ]]; then
      printf 'PASS: Kohdee validated the hull condition display, struck colors, holing, '
      printf 'sinking and its refusals, the wreck registry, a summons, and a dock repair '
      printf 'with exact character restoration (%ss).\n' \
        "$elapsed_seconds"
    elif [[ "$acceptance_mode" == loss ]]; then
      printf 'PASS: Kohdee and Vesselmate validated the retired insurance command, the crew '
      printf 'hiring gate and experience, the rename fee, a summons, and a trade-in with '
      printf 'exact two-character restoration (%ss).\n' "$elapsed_seconds"
    elif [[ "$acceptance_mode" == economy ]]; then
      printf 'PASS: Kohdee and Vesselmate validated contraband buying, the neutral-colors '
      printf 'sale, customs, prize money, and renown with exact two-character restoration (%ss).\n' \
        "$elapsed_seconds"
    elif [[ "$acceptance_mode" == raider ]]; then
      printf 'PASS: Kohdee validated ramming, a raider launch, her approach and boarding '
      printf 'attempt, her dead captain, and her retirement at restart with exact '
      printf 'character restoration (%ss).\n' "$elapsed_seconds"
    elif [[ "$acceptance_mode" == gunnery ]]; then
      printf 'PASS: Kohdee validated the shipyard, locks, battle stations, the harbor '
      printf 'refusal, scanning, sighting, and arc fire with exact character restoration (%ss).\n' \
        "$elapsed_seconds"
    elif [[ "$acceptance_mode" == client ]]; then
      printf 'PASS: Kohdee validated the native MSDP client data at sea and its empty '
      printf 'state ashore with exact character restoration (%ss).\n' "$elapsed_seconds"
    elif [[ "$acceptance_mode" == rules ]]; then
      printf 'PASS: Kohdee and Vesselmate validated the shipyard listing, contact IDs, '
      printf 'gunnery authorization, hull level, hull cap, and bounty pay-off with exact '
      printf 'two-character restoration (%ss).\n' "$elapsed_seconds"
    else
      printf 'PASS: Kohdee validated regional at-sea prose and contextual ambience '
      printf 'with exact character restoration (%ss).\n' "$elapsed_seconds"
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
[[ -x "$repo_root/bin/luminari" ]] || fail "bin/luminari is missing; run make install"
[[ -x "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" ]] ||
  fail "the local character login helper is unavailable"
[[ -f "$player_file" && ! -L "$player_file" ]] ||
  fail "the Kohdee player file is missing or unsafe to replace"
grep -Fqx "Name: $target_player" "$player_file" ||
  fail "the expected Kohdee identity is not in $player_file"
if uses_secondary_player; then
  [[ -f "$secondary_player_file" && ! -L "$secondary_player_file" ]] ||
    fail "the Vesselmate player file is missing or unsafe to replace"
  grep -Fqx "Name: $secondary_player" "$secondary_player_file" ||
    fail "the expected Vesselmate identity is not in $secondary_player_file"
fi

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
stale_binary_input=$(newer_binary_input "$repo_root" "$repo_root/bin/luminari") ||
  fail "could not compare installed MUD with build inputs"
[[ -z "$stale_binary_input" ]] ||
  fail "bin/luminari is older than $stale_binary_input; run make test and make install"

candidate_sha256=$(sha256sum "$repo_root/bin/luminari" | awk '{ print $1 }')
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
if [[ "$acceptance_mode" == narrative ]]; then
  database_apply_file "$repo_root/sql/components/vessels_narrative_content.sql"
  narrative_content_state=$(database_query "
    SELECT CONCAT(
      COUNT(*), '|', COUNT(DISTINCT region_vnum), '|',
      SUM(priority = 10), '|', SUM(priority = 11), '|',
      SUM(is_active <> 1))
      FROM region_hints
     WHERE region_vnum IN (1000013, 1000014, 1000015, 1000016)
       AND agent_id = 'vessel_narrative_v1';")
  [[ "$narrative_content_state" == '8|4|4|4|0' ]] ||
    fail "the Vailand narrative content is incomplete: $narrative_content_state"
fi
if [[ "$acceptance_mode" == raider ]]; then
  raider_tier_rows=$(database_query "SELECT COUNT(*) FROM vessel_raider_tiers;") ||
    raider_tier_rows=0
  [[ "$raider_tier_rows" -ge 10 ]] &&
    grep -Fqx '#70020' "$repo_root/lib/world/mob/700.mob" &&
    grep -Fqx '#70021' "$repo_root/lib/world/obj/700.obj" ||
    fail "the raider content is not installed; run scripts/vessels/provision_vessel_harbor.sh"
fi
if [[ "$acceptance_mode" == economy ]]; then
  economy_stock_state=$(database_query "
    SELECT COUNT(*)
      FROM port_commodities AS stock
      JOIN trade_commodities AS commodity
        ON commodity.commodity_id = stock.commodity_id
     WHERE stock.port_vnum = 1000390
       AND commodity.name = 'forbidden tomes'
       AND commodity.contraband_renown = 150;") || economy_stock_state=0
  [[ "$economy_stock_state" == 1 ]] ||
    fail "the contraband content is not installed; run scripts/vessels/provision_vessel_harbor.sh"
fi
if [[ "$acceptance_mode" == rules || "$acceptance_mode" == loss ||
  "$acceptance_mode" == economy ]]; then
  secondary_hull_count=$(database_query "
    SELECT COUNT(*)
      FROM ship_interiors
     WHERE owner = '$secondary_player';")
  [[ "$secondary_hull_count" == 0 ]] ||
    fail "Vesselmate already owns $secondary_hull_count hulls"
fi
if [[ "$acceptance_mode" == rules ]]; then
  database_apply_file "$repo_root/sql/components/vessels_phase18_schema.sql"
  baseline_secondary_bounty=$(database_query "
    SELECT CONCAT(bounty, '|', marque_until, '|', UNIX_TIMESTAMP(last_offense_at))
      FROM vessel_bounties
     WHERE player_name = '$secondary_player';")
  [[ -n "$baseline_secondary_bounty" ]] || baseline_secondary_bounty=absent
fi
cp --preserve=mode,ownership,timestamps "$player_file" \
  "$run_dir/kohdee.plr.before"
baseline_player_sha256=$(sha256sum "$run_dir/kohdee.plr.before" |
  awk '{ print $1 }')
if uses_secondary_player; then
  cp --preserve=mode,ownership,timestamps "$secondary_player_file" \
    "$run_dir/vesselmate.plr.before"
  baseline_secondary_player_sha256=$(
    sha256sum "$run_dir/vesselmate.plr.before" | awk '{ print $1 }'
  )
fi
snapshot_ready=true
cleanup_needed=true
if [[ "$acceptance_mode" == rules ]]; then
  # A WANTED (not HUNTED) bounty, so no bounty hunter is drawn.
  database_query "
    INSERT INTO vessel_bounties (player_name, bounty, last_offense_at)
    VALUES ('$secondary_player', 600, NOW())
    ON DUPLICATE KEY UPDATE bounty = 600, last_offense_at = NOW();"
fi

{
  printf 'source_commit=%s\n' "$source_commit"
  printf 'binary_sha256=%s\n' "$candidate_sha256"
  printf 'warship_prototype_id=%s\n' "$warship_prototype_id"
  printf 'baseline_player_sha256=%s\n' "$baseline_player_sha256"
  if uses_secondary_player; then
    printf 'baseline_secondary_player_sha256=%s\n' \
      "$baseline_secondary_player_sha256"
  fi
} >"$run_dir/metadata"

start_server_without_login || fail "the current development MUD did not start"
[[ $(running_binary_sha256) == "$candidate_sha256" ]] ||
  fail "the running MUD does not match the installed candidate"

if [[ "$acceptance_mode" == tactical ]]; then
  timeout 120 env DEV_MUD_CHARACTER="$target_player" \
    "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" --help-check TACTICAL \
    >"$run_dir/01-tactical-help.log" 2>&1 ||
    fail "Kohdee could not read the authoritative TACTICAL help"

  timeout 600 env DEV_MUD_CHARACTER="$target_player" \
    "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" --vessel-tactical-check \
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
    'Starfall Bastion         sound'; do
    grep -Fq "$expected_text" "$run_dir/02-kohdee-vessel-tactical.log" ||
      fail "the tactical transcript did not contain '$expected_text'"
  done
  # A holed hull reads crippled, so the shot target may read either.
  grep -Eq 'Starfall Bastion +(battered|crippled)' \
    "$run_dir/02-kohdee-vessel-tactical.log" ||
    fail "the tactical transcript did not show the target battered or crippled"
elif [[ "$acceptance_mode" == lookout ]]; then
  timeout 120 env DEV_MUD_CHARACTER="$target_player" \
    "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" --help-check LOOKOUT LOOK_OUTSIDE \
    >"$run_dir/01-lookout-help.log" 2>&1 ||
    fail "Kohdee could not read the authoritative LOOKOUT help"
  lookout_help_state=$(database_query "
    SELECT COUNT(*)
      FROM help_entries
     WHERE BINARY tag = 'VESSELS'
       AND entry LIKE '%scan canonical wilderness%'
       AND entry LIKE '%relative altitude%';")
  [[ "$lookout_help_state" == 1 ]] ||
    fail "the authoritative LOOK_OUTSIDE help is stale"

  timeout 300 env DEV_MUD_CHARACTER="$target_player" \
    "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" --vessel-lookout-check \
    "$warship_prototype_id" >"$run_dir/02-kohdee-vessel-lookout.log" 2>&1 ||
    fail "the actual Kohdee vessel-lookout session failed"

  for expected_text in \
    'PASS: the lookout used all eight canonical wilderness bearings' \
    'PASS: the lookout reported a real nearby vessel' \
    'PASS: optional paint and figurehead details appeared in LOOKOUT' \
    'PASS: the coastal lookout reported actual shoal, beach, and field sectors.' \
    'PASS: the vessel lookout check completed and purged all temporary hulls'; do
    grep -Fq "$expected_text" "$run_dir/02-kohdee-vessel-lookout.log" ||
      fail "the lookout session did not report '$expected_text'"
  done

  for expected_text in \
    'LOOKOUT VIEW FROM Azure Watch' \
    'Paint: midnight blue with silver trim; figurehead: a gilded sea dragon.' \
    'Azure Watch is here, painted midnight blue with silver trim' \
    'Surrounding wilderness (sampled to the visible horizon):' \
    'Visible vessels (nearest first):' \
    'Current sector: Ocean'; do
    grep -Fq "$expected_text" "$run_dir/02-kohdee-vessel-lookout.log" ||
      fail "the lookout transcript did not contain '$expected_text'"
  done
elif [[ "$acceptance_mode" == boarding ]]; then
  timeout 120 env DEV_MUD_CHARACTER="$target_player" \
    "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" --help-check \
    BOARD_HOSTILE BOARDING >"$run_dir/01-boarding-help.log" 2>&1 ||
    fail "Kohdee could not read the authoritative hostile-boarding help"
  boarding_help_state=$(database_query "
    SELECT COUNT(*)
      FROM help_entries
     WHERE BINARY tag = 'VESSELS'
       AND entry LIKE '%opposed Boarding check secures grappling lines%'
       AND entry LIKE '%opposed Boarding check resolves the crossing%'
       AND entry LIKE '%natural 1 throws the%'
       AND entry LIKE '%Athletics swim check%';")
  [[ "$boarding_help_state" == 1 ]] ||
    fail "the authoritative hostile-boarding help is stale"

  timeout 300 env DEV_MUD_CHARACTER="$target_player" \
    "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" --vessel-boarding-check \
    "$warship_prototype_id" "$secondary_player" \
    >"$run_dir/02-kohdee-vessel-boarding.log" 2>&1 ||
    fail "the actual Kohdee and Vesselmate boarding session failed"

  for expected_text in \
    "PASS: Kohdee's weak grapple was opposed and rejected by Vesselmate." \
    'PASS: reversed Boarding ranks produced successful grapple and crossing contests.' \
    'PASS: Kohdee breached the target while Vesselmate received both live warnings.' \
    'PASS: both temporary hulls were purged after the two-character check'; do
    grep -Fq "$expected_text" "$run_dir/02-kohdee-vessel-boarding.log" ||
      fail "the boarding session did not report '$expected_text'"
  done

  for expected_text in \
    'Grapple contest: Boarding' \
    'Crossing contest: Boarding' \
    'Vesselmate Boarding' \
    'breach the enemy vessel'; do
    grep -Fq "$expected_text" "$run_dir/02-kohdee-vessel-boarding.log" ||
      fail "the boarding transcript did not contain '$expected_text'"
  done
elif [[ "$acceptance_mode" == movement ]]; then
  timeout 120 env DEV_MUD_CHARACTER="$target_player" \
    "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" --help-check \
    SETSAIL ANCHOR UNDOCK >"$run_dir/01-movement-help.log" 2>&1 ||
    fail "Kohdee could not read the authoritative vessel movement help"
  movement_help_state=$(database_query "
    SELECT COUNT(*)
      FROM help_entries
     WHERE BINARY tag = 'VESSELS'
       AND entry LIKE '%needs speed 6 or less%'
       AND entry LIKE '%weigh anchor in 13%';")
  [[ "$movement_help_state" == 1 ]] ||
    fail "the authoritative vessel movement help is stale"

  timeout 300 env DEV_MUD_CHARACTER="$target_player" \
    "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" --vessel-movement-check \
    "$warship_prototype_id" >"$run_dir/02-kohdee-vessel-movement.log" 2>&1 ||
    fail "the actual Kohdee vessel-movement session failed"

  for expected_text in \
    'PASS: a hull launched in port was berthed, refused speed, and cast off in 30 seconds.' \
    'PASS: setsail maneuvered one room, waited five seconds, and berthed back in port.' \
    'PASS: the warship gathered way to speed 12 and covered one or two rooms in six seconds.' \
    'PASS: the warship came about to 90 degrees and refused a maneuver at speed.' \
    'PASS: the warship lost way, anchored, refused speed, and weighed anchor in 13 seconds.' \
    'PASS: the vessel movement check completed and purged all temporary hulls'; do
    grep -Fq "$expected_text" "$run_dir/02-kohdee-vessel-movement.log" ||
      fail "the movement session did not report '$expected_text'"
  done

  for expected_text in \
    'Moorings: Berthed' \
    'Moorings: Casting off' \
    'Moorings: Anchored' \
    '(coming about to 90)' \
    'Lines go ashore; Movecheck Boat'; do
    grep -Fq "$expected_text" "$run_dir/02-kohdee-vessel-movement.log" ||
      fail "the movement transcript did not contain '$expected_text'"
  done
elif [[ "$acceptance_mode" == damage ]]; then
  timeout 120 env DEV_MUD_CHARACTER="$target_player" \
    "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" --help-check \
    SHIPSTATUS SHIPSALVAGE STRIKECOLORS >"$run_dir/01-damage-help.log" 2>&1 ||
    fail "Kohdee could not read the authoritative vessel damage help"
  damage_help_state=$(database_query "
    SELECT COUNT(*)
      FROM help_entries
     WHERE (BINARY tag = 'VESSELS'
            AND entry LIKE '%before a%sinking hull goes down%')
        OR (BINARY tag = 'SHIPFIRE'
            AND entry LIKE '%SHIPSALVAGE%'
            AND entry LIKE '%STRIKECOLORS%');")
  [[ "$damage_help_state" == 2 ]] ||
    fail "the authoritative vessel damage help is stale"

  timeout 900 env DEV_MUD_CHARACTER="$target_player" \
    "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" --vessel-damage-check \
    "$warship_prototype_id" >"$run_dir/02-kohdee-vessel-damage.log" 2>&1 ||
    fail "the actual Kohdee vessel-damage session failed"

  for expected_text in \
    'PASS: a new Starfall Bastion showed the rescaled warship armor, structure, sails, rudder, and ready weapons.' \
    'PASS: struck colors showed on shipstatus and flew again when she got under way.' \
    'PASS: gunnery from one side holed her port side in ' \
    'started her sinking; she refused gunnery, repair, and salvage.' \
    'PASS: she went down on her sink timer and waits in the wreck registry as a boat.' \
    'PASS: summoned from the registry, she made port as a boat without sails and bought new ones.' \
    'PASS: the vessel damage check completed and purged all temporary hulls'; do
    grep -Fq "$expected_text" "$run_dir/02-kohdee-vessel-damage.log" ||
      fail "the damage session did not report '$expected_text'"
  done

  for expected_text in \
    'Structure: bow 33/33, port 41/41, starboard 41/41, stern 20/20' \
    'Holed: port side. She cannot move.' \
    'Holed: port side and stern. SINKING: she goes down in about' \
    'Direct hit on Starfall Bastion!'; do
    grep -Fq "$expected_text" "$run_dir/02-kohdee-vessel-damage.log" ||
      fail "the damage transcript did not contain '$expected_text'"
  done

  # Boot migrated the prototype to the S3 scale (35 to 95).
  [[ $(database_query "
    SELECT CONCAT(armor, '|', armor_scale)
      FROM ship_prototypes
     WHERE prototype_id = $warship_prototype_id;") == '95|1' ]] ||
    fail "the Starfall Bastion prototype is not on the S3 armor scale"
elif [[ "$acceptance_mode" == gunnery ]]; then
  timeout 120 env DEV_MUD_CHARACTER="$target_player" \
    "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" --help-check \
    SHIPFIRE SHIPWEAPON >"$run_dir/01-gunnery-help.log" 2>&1 ||
    fail "Kohdee could not read the authoritative vessel gunnery help"
  gunnery_help_state=$(database_query "
    SELECT COUNT(*)
      FROM help_entries
     WHERE (BINARY tag = 'SHIPFIRE'
            AND entry LIKE '%SHIPLOCK%'
            AND entry LIKE '%no harbor admits her%')
        OR (BINARY tag = 'SHIPHIRE'
            AND entry LIKE '%SHIPWEAPON buy%'
            AND entry LIKE '%SHIPREARM%');")
  [[ "$gunnery_help_state" == 2 ]] ||
    fail "the authoritative vessel gunnery help is stale"

  timeout 300 env DEV_MUD_CHARACTER="$target_player" \
    "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" --vessel-gunnery-check \
    "$warship_prototype_id" >"$run_dir/02-kohdee-vessel-gunnery.log" 2>&1 ||
    fail "the actual Kohdee vessel-gunnery session failed"

  for expected_text in \
    'PASS: the shipyard listed the catalogue, fitted and sold a ballista and a ram' \
    'PASS: a lock called battle stations and the harbor refused the warship' \
    'PASS: contacts, shipscan, and shipsight read the target, and the starboard arc fired' \
    'PASS: the vessel gunnery check completed and purged all temporary hulls'; do
    grep -Fq "$expected_text" "$run_dir/02-kohdee-vessel-gunnery.log" ||
      fail "the gunnery session did not report '$expected_text'"
  done
elif [[ "$acceptance_mode" == client ]]; then
  timeout 300 env DEV_MUD_CHARACTER="$target_player" \
    "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" --vessel-client-check \
    "$warship_prototype_id" >"$run_dir/01-kohdee-vessel-client.log" 2>&1 ||
    fail "the actual Kohdee vessel-client session failed"

  for expected_text in \
    'PASS: native MSDP reported the identity, lock, condition, weapons, and contacts' \
    'PASS: native MSDP emptied the client data after leaving the vessel.' \
    'PASS: the vessel client check completed and purged all temporary hulls'; do
    grep -Fq "$expected_text" "$run_dir/01-kohdee-vessel-client.log" ||
      fail "the client session did not report '$expected_text'"
  done
elif [[ "$acceptance_mode" == loss ]]; then
  timeout 120 env DEV_MUD_CHARACTER="$target_player" \
    "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" --help-check \
    SHIPSUMMON SHIPREPAIR >"$run_dir/01-loss-help.log" 2>&1 ||
    fail "Kohdee could not read the authoritative vessel loss help"
  loss_help_state=$(database_query "
    SELECT COUNT(*)
      FROM help_entries
     WHERE (BINARY tag = 'SHIPBROWSE'
            AND entry LIKE '%SHIPSUMMON%'
            AND entry LIKE '%wreck registry%')
        OR (BINARY tag = 'SHIPFIRE'
            AND entry LIKE '%Craft (woodworking) check, DC 15%');")
  [[ "$loss_help_state" == 2 ]] ||
    fail "the authoritative vessel loss help is stale"

  timeout 300 env DEV_MUD_CHARACTER="$target_player" \
    "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" --vessel-loss-check \
    "$secondary_player" >"$run_dir/02-kohdee-vessel-loss.log" 2>&1 ||
    fail "the actual Kohdee and Vesselmate vessel-loss session failed"

  for expected_text in \
    'PASS: the retired SHIPINSURE command is gone.' \
    'was refused an able gunner and hired a green bosun' \
    'PASS: the first christening was free and the rename cost 60 gold.' \
    'PASS: summoned from sea to the east dock, the boat made port in 37 seconds.' \
    'PASS: traded in at the east dock, she became a warship design' \
    'PASS: the vessel loss check completed and purged all temporary hulls'; do
    grep -Fq "$expected_text" "$run_dir/02-kohdee-vessel-loss.log" ||
      fail "the loss session did not report '$expected_text'"
  done
elif [[ "$acceptance_mode" == economy ]]; then
  timeout 120 env DEV_MUD_CHARACTER="$target_player" \
    "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" --help-check \
    SHIPRENOWN CONTRABAND >"$run_dir/01-economy-help.log" 2>&1 ||
    fail "Kohdee could not read the authoritative vessel economy help"
  economy_help_state=$(database_query "
    SELECT COUNT(*)
      FROM help_entries
     WHERE (BINARY tag = 'SHIPRENOWN' AND entry LIKE '%2.5 gold for each point%')
        OR (BINARY tag = 'MARKET' AND entry LIKE '%contraband it does not stock%');")
  [[ "$economy_help_state" == 2 ]] ||
    fail "the authoritative vessel economy help is stale"

  timeout 600 env DEV_MUD_CHARACTER="$target_player" \
    "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" --vessel-economy-check \
    "$warship_prototype_id" "$secondary_player" \
    >"$run_dir/02-kohdee-vessel-economy.log" 2>&1 ||
    fail "the actual Kohdee and Vesselmate vessel-economy session failed"
  for expected_text in \
    'PASS: the East Dock refused its forbidden tomes to a hull of no renown and sold them to an able crew.' \
    'PASS: under neutral colors the merchants paid a tenth less.' \
    "PASS: Kohdee's warship sank Vesselmate's boat, won her 25 renown, and was paid prize money." \
    'PASS: the vessel economy check completed and left the smuggler a room off the East Dock'; do
    grep -Fq "$expected_text" "$run_dir/02-kohdee-vessel-economy.log" ||
      fail "the economy session did not report '$expected_text'"
  done

  # With the dock's own stock of tomes lifted, its customs meet the ones
  # she carries back in.
  economy_ship_slot=$(economy_value economy_ship_slot)
  economy_ship_prototype_id=$(economy_value economy_ship_prototype_id)
  [[ -n "$economy_ship_slot" && -n "$economy_ship_prototype_id" ]] ||
    fail "the economy session did not report its smuggler"
  contraband_unstocked=true
  set_east_dock_tomes_stock false ||
    fail "could not lift the East Dock's stock of forbidden tomes"
  timeout 300 env DEV_MUD_CHARACTER="$target_player" \
    "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" --vessel-customs-check \
    "$economy_ship_slot" "$economy_ship_prototype_id" \
    >"$run_dir/03-kohdee-vessel-customs.log" 2>&1 ||
    fail "the actual Kohdee vessel-customs session failed"
  set_east_dock_tomes_stock true ||
    fail "could not restore the East Dock's stock of forbidden tomes"
  contraband_unstocked=false
  for expected_text in \
    'PASS: sailing into the East Dock, customs seized forbidden tomes it does not stock.' \
    'PASS: the vessel customs check completed and purged the smuggler'; do
    grep -Fq "$expected_text" "$run_dir/03-kohdee-vessel-customs.log" ||
      fail "the customs session did not report '$expected_text'"
  done
elif [[ "$acceptance_mode" == raider ]]; then
  timeout 120 env DEV_MUD_CHARACTER="$target_player" \
    "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" --help-check \
    SHIPRAM RAIDERS >"$run_dir/01-raider-help.log" 2>&1 ||
    fail "Kohdee could not read the authoritative vessel raider help"
  raider_help_state=$(database_query "
    SELECT COUNT(*)
      FROM help_entries
     WHERE BINARY tag = 'SHIPFIRE'
       AND entry LIKE '%she needs speed 6 or more%'
       AND entry LIKE '%about once in 17 minutes of sailing%';")
  [[ "$raider_help_state" == 1 ]] ||
    fail "the authoritative vessel raider help is stale"

  timeout 600 env DEV_MUD_CHARACTER="$target_player" \
    "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" --vessel-raider-check \
    "$warship_prototype_id" >"$run_dir/02-kohdee-vessel-raider.log" 2>&1 ||
    fail "the actual Kohdee vessel-raider session failed"

  for expected_text in \
    'PASS: shipram refused without a lock and below speed 6, rammed the locked hull' \
    'PASS: a tier 0 raider launched with her captain, crew, fit-out, and strongbox key' \
    'PASS: with her captain dead the raider hove to.' \
    'PASS: the vessel raider check completed'; do
    grep -Fq "$expected_text" "$run_dir/02-kohdee-vessel-raider.log" ||
      fail "the raider session did not report '$expected_text'"
  done

  # Raiders are never kept: the restart retires the one left at sea. The
  # port opens before the world has loaded, so wait for boot to report it.
  stop_development_mud || fail "the development MUD did not stop"
  start_server_without_login || fail "the development MUD did not restart"
  for ((attempt = 0; attempt < 600; attempt++)); do
    grep -Fq 'Retired 1 raider restored by the restart' "$server_log" && break
    sleep 0.1
  done
  grep -Fq 'Retired 1 raider restored by the restart' "$server_log" ||
    fail "the restart did not retire the raider left at sea"
  raider_runtime_count=$(database_query "
    SELECT COUNT(*)
      FROM ship_runtime_state
     WHERE prototype_id IN (SELECT prototype_id FROM vessel_raider_tiers);")
  [[ "$raider_runtime_count" == 0 ]] ||
    fail "$raider_runtime_count raider hulls survived the restart"
elif [[ "$acceptance_mode" == rules ]]; then
  timeout 120 env DEV_MUD_CHARACTER="$target_player" \
    "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" --help-check \
    SHIPFIRE BOUNTY SHIPBROWSE >"$run_dir/01-rules-help.log" 2>&1 ||
    fail "Kohdee could not read the authoritative vessel rules help"
  rules_help_state=$(database_query "
    SELECT COUNT(*)
      FROM help_entries
     WHERE (BINARY tag = 'SHIPFIRE' AND entry LIKE '%Harbors are neutral ground%')
        OR (BINARY tag = 'PLUNDER' AND entry LIKE '%BOUNTY PAY%')
        OR (BINARY tag = 'SHIPBROWSE' AND entry LIKE '%at most three%');")
  [[ "$rules_help_state" == 3 ]] ||
    fail "the authoritative vessel rules help is stale"

  timeout 300 env DEV_MUD_CHARACTER="$target_player" \
    "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" --vessel-rules-check \
    "$warship_prototype_id" "$secondary_player" \
    >"$run_dir/02-kohdee-vessel-rules.log" 2>&1 ||
    fail "the actual Kohdee and Vesselmate vessel-rules session failed"

  for expected_text in \
    'PASS: the shipyard listed only for-sale prototypes, with their departure level.' \
    'PASS: contacts and tactical shared two-letter IDs, and shipfire targeted a contact by ID.' \
    "PASS: Vesselmate could not fire the weapons of another captain's warship." \
    'PASS: Vesselmate was refused command of a level-' \
    'PASS: a fourth deed to Vesselmate was refused at the three-hull cap.' \
    "PASS: Vesselmate saw the WANTED bounty's 125% pay-off, refused away from port." \
    'PASS: the vessel rules check completed and purged all temporary hulls'; do
    grep -Fq "$expected_text" "$run_dir/02-kohdee-vessel-rules.log" ||
      fail "the rules session did not report '$expected_text'"
  done

  for expected_text in \
    'CONTACT LIST' \
    'No contact in sight matches' \
    'guns answer to her owner' \
    'Vesselmate already owns 3 hulls' \
    "clears it for 750 gold ('bounty pay')"; do
    grep -Fq "$expected_text" "$run_dir/02-kohdee-vessel-rules.log" ||
      fail "the rules transcript did not contain '$expected_text'"
  done
else
  timeout 120 env DEV_MUD_CHARACTER="$target_player" \
    "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" --help-check LOOKOUT VESSELDEBUG \
    >"$run_dir/01-narrative-help.log" 2>&1 ||
    fail "Kohdee could not read the authoritative narrative command help"
  narrative_help_state=$(database_query "
    SELECT COUNT(*)
      FROM help_entries
     WHERE BINARY tag = 'VESSELDEBUG'
       AND entry LIKE '%vesseldebug ambient%'
       AND entry LIKE '%class-, speed-, weather-, and depth-aware%';")
  [[ "$narrative_help_state" == 1 ]] ||
    fail "the authoritative VESSELDEBUG ambient help is stale"

  timeout 300 env DEV_MUD_CHARACTER="$target_player" \
    "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" --vessel-narrative-check \
    "$warship_prototype_id" >"$run_dir/02-kohdee-vessel-narrative.log" 2>&1 ||
    fail "the actual Kohdee vessel-narrative session failed"

  for expected_text in \
    'PASS: LOOKOUT combined the live warship class, speed, and wilderness weather.' \
    'PASS: wilderness weather ' \
    'PASS: narrative_weaver selected the Vailand Passage region_hints content.' \
    'PASS: the forced heartbeat emitted class-, speed-, and weather-aware ambience.' \
    'PASS: the vessel narrative check completed and purged its temporary hull'; do
    grep -Fq "$expected_text" "$run_dir/02-kohdee-vessel-narrative.log" ||
      fail "the narrative session did not report '$expected_text'"
  done

  for expected_text in \
    'At sea: A warship is holding steady way under' \
    'Vailand Passage' \
    "The warship's armored hull shoulders through the water."; do
    grep -Fq "$expected_text" "$run_dir/02-kohdee-vessel-narrative.log" ||
      fail "the narrative transcript did not contain '$expected_text'"
  done
  grep -Eq \
    'Conditions: (clear skies|overcast skies|rain|a heavy storm|a thunderstorm) \([0-9]+/255\)' \
    "$run_dir/02-kohdee-vessel-narrative.log" ||
    fail "the narrative transcript did not contain a recognized weather band"
  weather_clause_pattern='Clear light runs cleanly|Cloud cover flattens the light'
  weather_clause_pattern+='|Rain stipples the surrounding water'
  weather_clause_pattern+='|Storm winds drive dark water|Lightning throws the vessel'
  grep -Eq "$weather_clause_pattern" \
    "$run_dir/02-kohdee-vessel-narrative.log" ||
    fail "the narrative transcript did not contain a contextual weather clause"

  database_apply_file \
    "$repo_root/sql/components/verify_vessels_narrative_content.sql" \
    >"$run_dir/03-narrative-content-verification.log"
fi

[[ -z $(tactical_runtime_slots) ]] ||
  fail "a temporary Starfall Bastion runtime remained"
grep -Fqx 'Room: 1204' "$player_file" ||
  fail "Kohdee did not return to room 1204"
if uses_secondary_player; then
  grep -Fqx 'Room: 1204' "$secondary_player_file" ||
    fail "Vesselmate did not return to room 1204"
fi
if [[ "$acceptance_mode" == movement || "$acceptance_mode" == loss ]]; then
  [[ $(database_query "
    SELECT COUNT(*)
      FROM ship_prototypes
     WHERE name LIKE 'Movecheck Boat%' OR name LIKE 'Losscheck %';") == 0 ]] ||
    fail "a temporary check boat prototype remained"
fi
if [[ "$acceptance_mode" == rules ]]; then
  rules_prototype=$(rules_prototype_id)
  [[ -n "$rules_prototype" ]] ||
    fail "the rules session did not report its shipyard test prototype"
  [[ $(database_query "
    SELECT COUNT(*)
      FROM ship_prototypes
     WHERE prototype_id = $rules_prototype;") == 0 ]] ||
    fail "the temporary shipyard test prototype $rules_prototype remained"
fi
if grep -E 'SYSERR:.*(tactical|lookout|narrative|boarding|Boardatk|Boarddef|Rulesraft|Movecheck|Losscheck|bounty|refits|Starfall Bastion|Starfall Trench|Vailand)' \
  "$server_log" >"$run_dir/04-related-syserr.log"; then
  fail "the server logged a vessel-view SYSERR"
fi

acceptance_complete=true
