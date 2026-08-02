#!/usr/bin/env bash

set -Eeuo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo_root=${LUMINARI_PROJECT_ROOT:-$(cd "$script_dir/.." && pwd)}
server_unit=luminari-dev-login-smoke.service
server_log="${TMPDIR:-/tmp}/luminari-dev-login-smoke.log"
target_player=Kohdee
merchant_name="Harbor Sandbox Merchant"
temporary_respawn_seconds=
player_file="$repo_root/lib/plrfiles/K-O/kohdee.plr"
state_root="${TMPDIR:-/tmp}/luminari-vessel-merchant-check-${UID}"
run_id="$(date -u +%Y%m%dT%H%M%SZ)-$$"
run_dir="$state_root/runs/$run_id"
started_epoch=$(date +%s)
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
  vessel_bounty_hunts
  vessel_encounters
  vessel_hunter_encounters
  vessel_insurance_claims
  vessel_merchant_consequences
  vessel_npc_merchants
)
snapshot_ready=false
cleanup_needed=false
acceptance_complete=false
database_host=
database_name=
database_user=
database_password=
mud_port=
candidate_sha256=
source_commit=
baseline_database_sha256=
baseline_player_sha256=
baseline_bounty=0
baseline_marque=0
baseline_consequence_count=0
baseline_pending_count=0
baseline_faction_standing=0
baseline_consequence_highwater=0
merchant_id=
merchant_faction_id=
merchant_prototype_id=
merchant_route_id=
merchant_pilot_vnum=
merchant_commodity_id=
merchant_cargo_quantity=
merchant_respawn_delay=
merchant_commodity_name=
test_respawn_delay=
baseline_ship_id=
baseline_generation=
baseline_loss_count=
replacement_ship_id=
replacement_generation=
standing_penalty_total=0
bounty_delta=0

while (($# > 0)); do
  case "$1" in
    --merchant)
      [[ $# -ge 2 ]] || {
        printf 'usage: %s [--merchant <name>] [--temporary-respawn <1-59>]\n' \
          "$0" >&2
        exit 2
      }
      merchant_name=$2
      shift 2
      ;;
    --temporary-respawn)
      [[ $# -ge 2 ]] || {
        printf 'usage: %s [--merchant <name>] [--temporary-respawn <1-59>]\n' \
          "$0" >&2
        exit 2
      }
      temporary_respawn_seconds=$2
      shift 2
      ;;
    -h|--help)
      printf 'usage: %s [--merchant <name>] [--temporary-respawn <1-59>]\n' \
        "$0"
      exit 0
      ;;
    *)
      printf 'unknown argument: %s\n' "$1" >&2
      exit 2
      ;;
  esac
done

umask 077
mkdir -p "$run_dir"

fail()
{
  printf 'vessel merchant in-game check: %s\n' "$*" >&2
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

database_dump()
{
  local output_file=$1
  local temporary_file="$output_file.tmp"

  MYSQL_PWD="$database_password" mariadb-dump --no-defaults \
    --host="$database_host" --user="$database_user" \
    --single-transaction --quick --skip-lock-tables --skip-comments \
    --skip-dump-date --add-drop-table "$database_name" \
    "${snapshot_tables[@]}" >"$temporary_file" || return 1
  [[ -s "$temporary_file" ]] || return 1
  mv "$temporary_file" "$output_file"
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

restore_baseline()
{
  local cleanup_status=0
  local database_restored=false
  local player_restored=false
  local restore_tmp="$repo_root/lib/plrfiles/K-O/.kohdee.plr.merchant-restore-$$"
  local running_sha256
  local restored_fixture_valid
  local restored_player_sha256
  local attempt

  [[ "$snapshot_ready" == true ]] || return 0

  printf 'Stopping the development MUD before restoring the baseline.\n'
  if systemctl --user is-active --quiet "$server_unit"; then
    systemctl --user stop "$server_unit" || cleanup_status=1
  fi
  for ((attempt = 0; attempt < 300; attempt++)); do
    port_is_listening || break
    sleep 0.1
  done
  if port_is_listening; then
    printf 'The development port remained active during cleanup.\n' >&2
    cleanup_status=1
  fi

  if database_apply_file "$run_dir/vessel-database-before.sql"; then
    database_restored=true
  else
    printf 'The vessel/economy database snapshot could not be restored.\n' >&2
    cleanup_status=1
  fi

  if cp --preserve=mode,ownership,timestamps \
       "$run_dir/kohdee.plr.before" "$restore_tmp" &&
     mv -f "$restore_tmp" "$player_file"; then
    player_restored=true
  else
    printf 'Kohdee player-file restoration failed.\n' >&2
    [[ ! -e "$restore_tmp" ]] || unlink "$restore_tmp"
    cleanup_status=1
  fi

  if [[ "$database_restored" == true ]]; then
    if database_dump "$run_dir/vessel-database-restored.sql" &&
       cmp -s "$run_dir/vessel-database-before.sql" \
         "$run_dir/vessel-database-restored.sql"; then
      printf 'The stopped database matches the exact pre-test dump.\n'
    else
      printf 'The restored database differs from the pre-test dump.\n' >&2
      cleanup_status=1
    fi
  fi

  if [[ "$player_restored" == true ]]; then
    restored_player_sha256=$(sha256sum "$player_file" | awk '{ print $1 }')
    if [[ "$restored_player_sha256" != "$baseline_player_sha256" ]] ||
       ! cmp -s "$run_dir/kohdee.plr.before" "$player_file"; then
      printf 'The restored Kohdee player file differs from the baseline.\n' >&2
      cleanup_status=1
    fi
  fi

  if [[ "$cleanup_status" == 0 ]]; then
    start_server_without_login || cleanup_status=1
  fi
  if [[ "$cleanup_status" == 0 ]]; then
    running_sha256=$(running_binary_sha256)
    [[ "$running_sha256" == "$candidate_sha256" ]] || cleanup_status=1
    restored_player_sha256=$(sha256sum "$player_file" | awk '{ print $1 }')
    [[ "$restored_player_sha256" == "$baseline_player_sha256" ]] ||
      cleanup_status=1
    restored_fixture_valid=$(database_query "
      SELECT IF(
               merchant.generation = $baseline_generation
               AND merchant.loss_count = $baseline_loss_count
               AND merchant.active_ship_id = $baseline_ship_id
               AND runtime.ship_id = $baseline_ship_id
               AND interior.owner = '',
               1,
               0
             )
        FROM vessel_npc_merchants AS merchant
        JOIN ship_runtime_state AS runtime
          ON runtime.ship_id = merchant.active_ship_id
        JOIN ship_interiors AS interior
          ON interior.ship_id = merchant.active_ship_id
       WHERE merchant.merchant_id = $merchant_id;") || cleanup_status=1
    [[ "$restored_fixture_valid" == 1 ]] || cleanup_status=1
  fi

  if [[ "$cleanup_status" == 0 ]]; then
    cleanup_needed=false
    printf 'Restored Kohdee and the complete vessel/economy baseline.\n'
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
      printf 'PASS elapsed=%s merchant=%s generation=%s->%s ship=%s->%s ' \
        "$elapsed_seconds" "$merchant_id" "$baseline_generation" \
        "$replacement_generation" "$baseline_ship_id" "$replacement_ship_id"
      printf 'standing=%s bounty=%s cleanup=restored\n' \
        "$standing_penalty_total" "$bounty_delta"
    } >"$run_dir/result"
    printf 'PASS: Kohdee sank merchant %s generation %s and verified ' \
      "$merchant_id" "$baseline_generation"
    printf 'generation %s with cargo, pilot, route, and schedule (%ss).\n' \
      "$replacement_generation" "$elapsed_seconds"
    printf 'PASS: standing loss %s and bounty %s were observed; ' \
      "$standing_penalty_total" "$bounty_delta"
    printf 'Kohdee and all vessel/economy tables were restored exactly.\n'
    printf 'Artifacts: %s\n' "$run_dir"
    exit 0
  fi

  printf 'FAIL elapsed=%s command_status=%s cleanup_status=%s\n' \
    "$elapsed_seconds" "$exit_status" "$cleanup_status" >"$run_dir/result"
  if [[ "$cleanup_status" != 0 ]]; then
    printf 'vessel merchant in-game check: automatic cleanup failed; inspect %s/cleanup.log\n' \
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

for command_name in awk cmp cp date env find flock git grep mariadb \
  mariadb-dump mkdir mv sed sha256sum sleep ss systemctl systemd-run \
  timeout unlink; do
  command -v "$command_name" >/dev/null 2>&1 ||
    fail "required command not found: $command_name"
done

[[ "$merchant_name" =~ ^[[:alnum:]][[:alnum:]_.-]*(\ [[:alnum:]_.-]+)*$ &&
   ${#merchant_name} -le 127 ]] ||
  fail "merchant name contains unsupported characters"
if [[ -n "$temporary_respawn_seconds" ]]; then
  [[ "$temporary_respawn_seconds" =~ ^[1-9][0-9]*$ &&
     "$temporary_respawn_seconds" -le 59 ]] ||
    fail "temporary respawn must be from 1 through 59 seconds"
fi

exec 8>"${TMPDIR:-/tmp}/luminari-vessel-merchant-check-${UID}.lock"
flock -n 8 || fail "another merchant acceptance check is already running"

[[ -r "$repo_root/lib/.env" ]] || fail "cannot read lib/.env"
[[ -r "$repo_root/lib/mysql_config" ]] || fail "cannot read lib/mysql_config"
[[ -x "$repo_root/bin/circle" ]] ||
  fail "bin/circle is missing; run make install first"
[[ -x "$script_dir/dev_kohdee_login_smoke.sh" ]] ||
  fail "the local character login helper is unavailable"
[[ -f "$repo_root/src/vessels_merchants.c" ]] ||
  fail "the Phase 14 merchant source is missing"
[[ -f "$player_file" && ! -L "$player_file" ]] ||
  fail "the exact Kohdee player file is missing or unsafe to replace"
grep -Fqx "Name: $target_player" "$player_file" ||
  fail "the expected Kohdee identity is not in $player_file"

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
[[ -z "$(git -C "$repo_root" status --porcelain)" ]] ||
  fail "source worktree must be clean before acceptance provenance is recorded"
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

run_kohdee_commands "$run_dir/01-preflight.log" \
  "vmerchant list" "bounty" ||
  fail "Kohdee could not inspect the live merchant registry"
grep -Fq "$merchant_name" "$run_dir/01-preflight.log" ||
  fail "the in-game registry did not list the selected merchant"

merchant_state=$(database_query "
  SELECT CONCAT(
           merchant_id, '|', faction_id, '|', prototype_id, '|', route_id,
           '|', pilot_mob_vnum, '|', cargo_commodity_id, '|', cargo_quantity,
           '|', respawn_delay_seconds, '|', COALESCE(active_ship_id, 0),
           '|', generation, '|', loss_count, '|', commodity.name
         )
    FROM vessel_npc_merchants AS merchant
    JOIN trade_commodities AS commodity
      ON commodity.commodity_id = merchant.cargo_commodity_id
   WHERE merchant.name = '$merchant_name'
     AND merchant.enabled = 1;")
IFS='|' read -r merchant_id merchant_faction_id merchant_prototype_id \
  merchant_route_id merchant_pilot_vnum merchant_commodity_id \
  merchant_cargo_quantity merchant_respawn_delay baseline_ship_id \
  baseline_generation baseline_loss_count merchant_commodity_name \
  <<<"$merchant_state"
[[ "$merchant_id" =~ ^[1-9][0-9]*$ &&
   "$merchant_faction_id" =~ ^[1-3]$ &&
   "$merchant_prototype_id" =~ ^[1-9][0-9]*$ &&
   "$merchant_route_id" =~ ^[1-9][0-9]*$ &&
   "$merchant_pilot_vnum" =~ ^[1-9][0-9]*$ &&
   "$merchant_commodity_id" =~ ^[1-9][0-9]*$ &&
   "$merchant_cargo_quantity" =~ ^[1-9][0-9]*$ &&
   "$merchant_respawn_delay" =~ ^[1-9][0-9]*$ &&
   -n "$merchant_commodity_name" &&
   "$baseline_ship_id" =~ ^[1-9][0-9]*$ && "$baseline_ship_id" -le 500 &&
   "$baseline_generation" =~ ^[1-9][0-9]*$ &&
   "$baseline_loss_count" =~ ^[0-9]+$ ]] ||
  fail "the selected merchant definition or active identity is incomplete"

test_respawn_delay=$merchant_respawn_delay
if [[ -n "$temporary_respawn_seconds" ]]; then
  test_respawn_delay=$temporary_respawn_seconds
fi
[[ "$test_respawn_delay" -le 59 ]] ||
  fail "the merchant respawn exceeds 59 seconds; use --temporary-respawn"

fixture_valid=$(database_query "
  SELECT IF(
           COUNT(*) = 1
           AND MAX(interior.owner) = ''
           AND MAX(interior.vessel_name) = '$merchant_name'
           AND MAX(runtime.prototype_id) = $merchant_prototype_id
           AND MAX(cargo.item_count) = $merchant_cargo_quantity
           AND MAX(schedule.route_id) = $merchant_route_id
           AND MAX(schedule.enabled) = 1,
           1,
           0
         )
    FROM ship_runtime_state AS runtime
    JOIN ship_interiors AS interior
      ON interior.ship_id = runtime.ship_id
    JOIN ship_cargo_manifest AS cargo
      ON cargo.ship_id = runtime.ship_id
     AND cargo.cargo_room = 0
     AND cargo.item_vnum = $merchant_commodity_id
    JOIN ship_crew_roster AS crew
      ON crew.ship_id = runtime.ship_id
     AND crew.crew_role = 'pilot'
     AND crew.npc_vnum = $merchant_pilot_vnum
     AND crew.status = 'active'
    JOIN ship_schedules AS schedule
      ON schedule.ship_id = runtime.ship_id
   WHERE runtime.ship_id = $baseline_ship_id;")
[[ "$fixture_valid" == 1 ]] ||
  fail "the active merchant cargo, pilot, route, or schedule is incomplete"

baseline_pending_count=$(database_query "
  SELECT COUNT(*)
    FROM vessel_merchant_consequences
   WHERE player_name = '$target_player'
     AND status = 'pending';")
[[ "$baseline_pending_count" == 0 ]] ||
  fail "Kohdee still has pending merchant consequences after the preflight login"
baseline_consequence_count=$(database_query \
  "SELECT COUNT(*) FROM vessel_merchant_consequences;")
baseline_bounty_state=$(database_query "
  SELECT CONCAT(
           COALESCE((SELECT bounty FROM vessel_bounties
                      WHERE player_name = '$target_player'), 0),
           '|',
           COALESCE((SELECT marque_until FROM vessel_bounties
                      WHERE player_name = '$target_player'), 0)
         );")
IFS='|' read -r baseline_bounty baseline_marque <<<"$baseline_bounty_state"

printf -v faction_tag 'Fa%02d' "$merchant_faction_id"
baseline_faction_standing=$(player_file_value "$faction_tag" "$player_file")
baseline_consequence_highwater=$(player_file_value VMer "$player_file")
[[ "$baseline_faction_standing" =~ ^-?[0-9]+$ &&
   "$baseline_consequence_highwater" =~ ^[0-9]+$ ]] ||
  fail "Kohdee's faction or merchant high-water state is malformed"

cp --preserve=mode,ownership,timestamps \
  "$player_file" "$run_dir/kohdee.plr.before" ||
  fail "could not snapshot Kohdee's player file"
baseline_player_sha256=$(sha256sum "$run_dir/kohdee.plr.before" |
  awk '{ print $1 }')
database_dump "$run_dir/vessel-database-before.sql" ||
  fail "could not snapshot the vessel/economy database"
baseline_database_sha256=$(sha256sum "$run_dir/vessel-database-before.sql" |
  awk '{ print $1 }')
snapshot_ready=true
cleanup_needed=true

{
  printf 'source_commit=%s\n' "$source_commit"
  printf 'binary_sha256=%s\n' "$candidate_sha256"
  printf 'started_at=%s\n' "$started_epoch"
  printf 'merchant_id=%s\n' "$merchant_id"
  printf 'merchant_generation=%s\n' "$baseline_generation"
  printf 'merchant_ship_id=%s\n' "$baseline_ship_id"
  printf 'merchant_name=%s\n' "$merchant_name"
  printf 'merchant_commodity=%s\n' "$merchant_commodity_name"
  printf 'merchant_respawn_baseline=%s\n' "$merchant_respawn_delay"
  printf 'merchant_respawn_test=%s\n' "$test_respawn_delay"
  printf 'database_sha256=%s\n' "$baseline_database_sha256"
  printf 'player_file_sha256=%s\n' "$baseline_player_sha256"
  printf 'baseline_bounty=%s\n' "$baseline_bounty"
  printf 'baseline_marque_until=%s\n' "$baseline_marque"
  printf 'baseline_consequence_rows=%s\n' "$baseline_consequence_count"
} >"$run_dir/metadata"

database_execute "
  START TRANSACTION;
  UPDATE vessel_npc_merchants
     SET respawn_delay_seconds = $test_respawn_delay
   WHERE merchant_id = $merchant_id;
  DELETE FROM vessel_merchant_consequences
   WHERE merchant_id = $merchant_id
     AND generation = $baseline_generation
     AND player_name = '$target_player';
  INSERT INTO vessel_bounties (player_name, bounty, marque_until)
  VALUES ('$target_player', 0, 0)
  ON DUPLICATE KEY UPDATE bounty = 0, marque_until = 0;
  COMMIT;"

run_kohdee_commands "$run_dir/02-loss-and-recovery.log" \
  "vmerchant list" \
  "vmerchant sink $merchant_id confirm" \
  "bounty" \
  "@wait $((test_respawn_delay + 1))" \
  "vmerchant sync" \
  "vmerchant list" \
  "bounty" ||
  fail "the actual Kohdee merchant-loss session failed"
grep -Fq \
  "Forcing the documented loss path for merchant $merchant_id, ship $baseline_ship_id." \
  "$run_dir/02-loss-and-recovery.log" ||
  fail "the production sink path was not invoked"
[[ $(grep -Fc "News of your attack on merchant shipping costs faction standing" \
       "$run_dir/02-loss-and-recovery.log") -eq 2 ]] ||
  fail "the attack and total-loss standing consequences were not both delivered"
grep -Fq "NPC merchant definitions reconciled." \
  "$run_dir/02-loss-and-recovery.log" ||
  fail "the replacement reconciliation command did not complete"

attack_state=$(database_query "
  SELECT CONCAT(standing_penalty, '|', bounty_delta, '|', cargo_units,
                '|', status, '|', consequence_id)
    FROM vessel_merchant_consequences
   WHERE merchant_id = $merchant_id
     AND generation = $baseline_generation
     AND player_name = '$target_player'
     AND event_type = 'attack';")
sink_state=$(database_query "
  SELECT CONCAT(standing_penalty, '|', bounty_delta, '|', cargo_units,
                '|', status, '|', consequence_id)
    FROM vessel_merchant_consequences
   WHERE merchant_id = $merchant_id
     AND generation = $baseline_generation
     AND player_name = '$target_player'
     AND event_type = 'sink';")
IFS='|' read -r attack_standing attack_bounty attack_cargo attack_status \
  attack_consequence_id <<<"$attack_state"
IFS='|' read -r sink_standing bounty_delta sink_cargo sink_status \
  sink_consequence_id <<<"$sink_state"
[[ "$attack_standing" == 25 && "$attack_bounty" == 0 &&
   "$attack_cargo" == 0 && "$attack_status" == applied &&
   "$attack_consequence_id" =~ ^[1-9][0-9]*$ ]] ||
  fail "the durable attack consequence is incorrect"
expected_sink_standing=$((merchant_cargo_quantity + 100))
[[ "$sink_standing" == "$expected_sink_standing" &&
   "$bounty_delta" =~ ^[1-9][0-9]*$ &&
   "$sink_cargo" == "$merchant_cargo_quantity" &&
   "$sink_status" == applied &&
   "$sink_consequence_id" =~ ^[1-9][0-9]*$ ]] ||
  fail "the cargo-scaled sink consequence or bounty is incorrect"
standing_penalty_total=$((attack_standing + sink_standing))

current_bounty=$(database_query "
  SELECT bounty
    FROM vessel_bounties
   WHERE player_name = '$target_player';")
[[ "$current_bounty" == "$bounty_delta" ]] ||
  fail "Kohdee's durable bounty does not match the sink consequence"

replacement_state=$(database_query "
  SELECT CONCAT(generation, '|', COALESCE(active_ship_id, 0), '|',
                loss_count, '|', last_destroyed_by, '|', next_respawn_at,
                '|', last_error)
    FROM vessel_npc_merchants
   WHERE merchant_id = $merchant_id;")
IFS='|' read -r replacement_generation replacement_ship_id replacement_loss_count \
  replacement_destroyed_by replacement_due replacement_error \
  <<<"$replacement_state"
expected_generation=$((baseline_generation + 1))
expected_loss_count=$((baseline_loss_count + 1))
[[ "$replacement_generation" == "$expected_generation" &&
   "$replacement_ship_id" =~ ^[1-9][0-9]*$ &&
   "$replacement_ship_id" -le 500 &&
   "$replacement_loss_count" == "$expected_loss_count" &&
   "$replacement_destroyed_by" == "$target_player" &&
   "$replacement_due" == 0 && -z "$replacement_error" ]] ||
  fail "the merchant registry did not publish the expected replacement generation"

replacement_valid=$(database_query "
  SELECT IF(
           COUNT(*) = 1
           AND MAX(interior.owner) = ''
           AND MAX(interior.vessel_name) = '$merchant_name'
           AND MAX(runtime.prototype_id) = $merchant_prototype_id
           AND MAX(cargo.item_count) = $merchant_cargo_quantity
           AND MAX(schedule.route_id) = $merchant_route_id
           AND MAX(schedule.enabled) = 1,
           1,
           0
         )
    FROM ship_runtime_state AS runtime
    JOIN ship_interiors AS interior
      ON interior.ship_id = runtime.ship_id
    JOIN ship_cargo_manifest AS cargo
      ON cargo.ship_id = runtime.ship_id
     AND cargo.cargo_room = 0
     AND cargo.item_vnum = $merchant_commodity_id
    JOIN ship_crew_roster AS crew
      ON crew.ship_id = runtime.ship_id
     AND crew.crew_role = 'pilot'
     AND crew.npc_vnum = $merchant_pilot_vnum
     AND crew.status = 'active'
    JOIN ship_schedules AS schedule
      ON schedule.ship_id = runtime.ship_id
   WHERE runtime.ship_id = $replacement_ship_id;")
[[ "$replacement_valid" == 1 ]] ||
  fail "the replacement hull, cargo, pilot, route, or schedule is incomplete"

run_kohdee_commands "$run_dir/03-replacement.log" \
  "vmerchant list" \
  "shipgoto $replacement_ship_id" \
  "cargomanifest" \
  "showschedule" \
  "shipstatus" \
  "bounty" ||
  fail "Kohdee could not inspect the replacement merchant hull"
grep -Fq "$merchant_name" "$run_dir/03-replacement.log" ||
  fail "the replacement was absent from the in-game merchant registry"
grep -Fq "$merchant_commodity_name" "$run_dir/03-replacement.log" ||
  fail "the replacement did not expose its real cargo"
grep -Fq \
  "Merchant Registry: $merchant_id generation $replacement_generation" \
  "$run_dir/03-replacement.log" ||
  fail "shipstatus did not attach the replacement merchant identity"
grep -Fq "You: $bounty_delta gold on your head" \
  "$run_dir/03-replacement.log" ||
  fail "the in-game bounty command did not expose the durable consequence"

mutated_faction_standing=$(player_file_value "$faction_tag" "$player_file")
mutated_consequence_highwater=$(player_file_value VMer "$player_file")
expected_faction_standing=$((baseline_faction_standing - standing_penalty_total))
highest_consequence_id=$((attack_consequence_id > sink_consequence_id \
  ? attack_consequence_id : sink_consequence_id))
[[ "$mutated_faction_standing" == "$expected_faction_standing" ]] ||
  fail "Kohdee's saved faction standing did not reflect both consequences"
[[ "$mutated_consequence_highwater" == "$highest_consequence_id" ]] ||
  fail "Kohdee's saved merchant consequence high-water mark is incorrect"

{
  printf 'merchant_id=%s\n' "$merchant_id"
  printf 'generation_before=%s\n' "$baseline_generation"
  printf 'generation_after=%s\n' "$replacement_generation"
  printf 'ship_before=%s\n' "$baseline_ship_id"
  printf 'ship_after=%s\n' "$replacement_ship_id"
  printf 'standing_before=%s\n' "$baseline_faction_standing"
  printf 'standing_after=%s\n' "$mutated_faction_standing"
  printf 'standing_penalty=%s\n' "$standing_penalty_total"
  printf 'bounty_delta=%s\n' "$bounty_delta"
  printf 'consequence_highwater=%s\n' "$mutated_consequence_highwater"
} >"$run_dir/observed-state"

acceptance_complete=true
