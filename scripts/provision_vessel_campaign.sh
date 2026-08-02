#!/usr/bin/env bash

set -Eeuo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo_root=${LUMINARI_PROJECT_ROOT:-$(cd "$script_dir/.." && pwd)}
server_unit=luminari-dev-login-smoke.service
server_log="${TMPDIR:-/tmp}/luminari-dev-login-smoke.log"
state_root="${TMPDIR:-/tmp}/luminari-vessel-campaign-${UID}"
run_id="$(date -u +%Y%m%dT%H%M%SZ)-$$"
run_dir="$state_root/runs/$run_id"
started_epoch=$(date +%s)
database_host=
database_name=
database_user=
database_password=
mud_port=
restart_needed=false

mkdir -p "$run_dir"

fail()
{
  printf 'vessel campaign provisioner: %s\n' "$*" >&2
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
  "$script_dir/dev_kohdee_login_smoke.sh" >"$run_dir/01-boot.log" 2>&1
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
  exit "$exit_status"
}
trap recover_server EXIT

for command_name in awk date git grep mariadb mkdir sha256sum sleep ss \
  systemctl timeout; do
  command -v "$command_name" >/dev/null 2>&1 ||
    fail "required command not found: $command_name"
done

[[ -r "$repo_root/lib/.env" ]] || fail "cannot read lib/.env"
[[ -r "$repo_root/lib/mysql_config" ]] || fail "cannot read lib/mysql_config"
[[ -x "$repo_root/bin/circle" ]] || fail "bin/circle is missing; run make install"
[[ -x "$script_dir/dev_kohdee_login_smoke.sh" ]] ||
  fail "the local character login helper is unavailable"

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

for workload_unit in luminari-vessel-ferry-soak.service \
  luminari-vessel-scale-benchmark.service; do
  systemctl --user is-active --quiet "$workload_unit" &&
    fail "$workload_unit owns the development service"
done

port_contract=$(awk '
  /^#1000360$/ { wanted = 1; title = ""; coordinates = ""; next }
  /^#1000362$/ { wanted = 1; title = ""; coordinates = ""; next }
  wanted && title == "" { title = $0; next }
  wanted && $0 == "C" {
    getline coordinates
    print title "|" coordinates
    wanted = 0
  }
' "$repo_root/lib/world/wld/10000.wld")
grep -Fqx 'North Vailand Sea Port~|-599 455' <<<"$port_contract" ||
  fail "North Vailand Sea Port 1000360 is missing or moved"
grep -Fqx 'Central Vailand Sea Port~|-467 204' <<<"$port_contract" ||
  fail "Central Vailand Sea Port 1000362 is missing or moved"

collision_count=$(database_scalar "
  SELECT
    (SELECT COUNT(*) FROM region_data
      WHERE (vnum = 1000013 AND name <> 'North Vailand Territorial Waters')
         OR (name = 'North Vailand Territorial Waters' AND vnum <> 1000013)
         OR (vnum = 1000014 AND name <> 'Central Vailand Territorial Waters')
         OR (name = 'Central Vailand Territorial Waters' AND vnum <> 1000014)
         OR (vnum = 1000015 AND name <> 'Vailand Passage')
         OR (name = 'Vailand Passage' AND vnum <> 1000015)
         OR (vnum = 1000016 AND name <> 'Blackwake Anchorage')
         OR (name = 'Blackwake Anchorage' AND vnum <> 1000016))
    + (SELECT IF(COUNT(*) > 1, 1, 0) FROM ship_routes
        WHERE name = 'Vailand Iron Passage')
    + (SELECT IF(COUNT(*) > 1, 1, 0) FROM ship_prototypes
        WHERE name = 'Vailand Merchant Cog')
    + (SELECT IF(COUNT(*) > 1, 1, 0) FROM vessel_npc_merchants
        WHERE name = 'Vailand Ironwind Trader')
    + (SELECT COUNT(*)
         FROM (
           SELECT name
             FROM ship_waypoints
            WHERE name IN (
              'vailand_north_port', 'vailand_northing',
              'vailand_outer_passage', 'blackwake_anchorage',
              'vailand_southing', 'vailand_central_approach',
              'vailand_central_port')
            GROUP BY name
           HAVING COUNT(*) > 1
         ) AS duplicate_waypoints);")
[[ "$collision_count" == 0 ]] ||
  fail "campaign region, route, prototype, merchant, or waypoint names collide"

stop_development_mud
restart_needed=true
apply_database_file "$repo_root/sql/components/vessels_phase13_schema.sql"
apply_database_file "$repo_root/sql/components/vessels_phase14_schema.sql"
apply_database_file "$repo_root/sql/components/vessels_campaign_content.sql"
start_development_mud

merchant_slot=
for ((attempt = 0; attempt < 40; attempt++)); do
  merchant_slot=$(database_scalar "
    SELECT COALESCE(active_ship_id, 0)
      FROM vessel_npc_merchants
     WHERE name = 'Vailand Ironwind Trader';")
  if [[ "$merchant_slot" =~ ^[1-9][0-9]*$ && "$merchant_slot" -le 500 ]]; then
    break
  fi
  sleep 0.5
done
[[ "$merchant_slot" =~ ^[1-9][0-9]*$ && "$merchant_slot" -le 500 ]] ||
  fail "the Vailand merchant did not enter service after restart"

content_valid=$(database_scalar "
  SELECT IF(
    (SELECT COUNT(*)
       FROM region_data AS region
       JOIN region_index AS idx ON idx.vnum = region.vnum
       JOIN vessel_region_law AS law ON law.region_vnum = region.vnum
      WHERE region.vnum IN (1000013, 1000014, 1000015, 1000016)
        AND region.region_type = 1
        AND region.zone_vnum = 10000) = 4
    AND (SELECT COUNT(*)
           FROM ship_routes AS route
           JOIN ship_route_waypoints AS link
             ON link.route_id = route.route_id
          WHERE route.name = 'Vailand Iron Passage'
            AND route.loop_route = 1
            AND route.active = 1) = 12
    AND (SELECT COUNT(*)
           FROM vessel_npc_merchants AS merchant
           JOIN ship_runtime_state AS runtime
             ON runtime.ship_id = merchant.active_ship_id
           JOIN ship_interiors AS interior
             ON interior.ship_id = merchant.active_ship_id
           JOIN ship_cargo_manifest AS cargo
             ON cargo.ship_id = merchant.active_ship_id
            AND cargo.cargo_room = 0
           JOIN trade_commodities AS commodity
             ON commodity.commodity_id = cargo.item_vnum
           JOIN ship_crew_roster AS crew
             ON crew.ship_id = merchant.active_ship_id
            AND crew.crew_role = 'pilot'
            AND crew.status = 'active'
           JOIN ship_schedules AS schedule
             ON schedule.ship_id = merchant.active_ship_id
          WHERE merchant.name = 'Vailand Ironwind Trader'
            AND interior.owner = ''
            AND interior.vessel_name = merchant.name
            AND cargo.item_count = 40
            AND commodity.name = 'iron'
            AND crew.npc_vnum = 31810
            AND schedule.route_id = merchant.route_id
            AND schedule.enabled = 1) = 1,
    1, 0);")
[[ "$content_valid" == 1 ]] ||
  fail "the campaign regions, route, merchant, cargo, pilot, or schedule are invalid"

before_position=$(database_scalar "
  SELECT CONCAT(ROUND(runtime.x), '|', ROUND(runtime.y), '|',
                merchant.generation)
    FROM vessel_npc_merchants AS merchant
    JOIN ship_runtime_state AS runtime
      ON runtime.ship_id = merchant.active_ship_id
   WHERE merchant.name = 'Vailand Ironwind Trader';")

timeout 150 "$script_dir/dev_kohdee_login_smoke.sh" --commands \
  "vmerchant list" \
  "shipgoto $merchant_slot" \
  "shipstatus" \
  "cargomanifest" \
  "showschedule" \
  "seastate" \
  "@wait 45" \
  "shipstatus" \
  "seastate" \
  "goto 1204" \
  >"$run_dir/02-kohdee-campaign.log" 2>&1 ||
  fail "the actual Kohdee campaign session failed"

grep -Fq 'Vailand Ironwind Trader' "$run_dir/02-kohdee-campaign.log" ||
  fail "Kohdee did not see the campaign merchant identity"
grep -Fq 'iron' "$run_dir/02-kohdee-campaign.log" ||
  fail "Kohdee did not see the merchant's real iron cargo"
grep -Fq 'Vailand Iron Passage' "$run_dir/02-kohdee-campaign.log" ||
  fail "Kohdee did not see the campaign route"
grep -Eq 'North Vailand Territorial Waters|Central Vailand Territorial Waters|Vailand Passage|Blackwake Anchorage' \
  "$run_dir/02-kohdee-campaign.log" ||
  fail "Kohdee did not resolve the campaign legal waters"

after_position=$(database_scalar "
  SELECT CONCAT(ROUND(runtime.x), '|', ROUND(runtime.y), '|',
                merchant.generation)
    FROM vessel_npc_merchants AS merchant
    JOIN ship_runtime_state AS runtime
      ON runtime.ship_id = merchant.active_ship_id
   WHERE merchant.name = 'Vailand Ironwind Trader';")
[[ "$after_position" != "$before_position" ]] ||
  fail "the campaign merchant did not move during the actual-character window"

if [[ -f "$server_log" ]] &&
   grep -E 'SYSERR:.*(Vailand Ironwind|Vailand Iron Passage|100001[3-6])' \
     "$server_log" >"$run_dir/03-related-syserr.log"; then
  fail "the server logged a campaign vessel SYSERR"
fi

source_commit=$(git -C "$repo_root" rev-parse HEAD)
binary_sha256=$(sha256sum "$repo_root/bin/circle" | awk '{ print $1 }')
elapsed_seconds=$(($(date +%s) - started_epoch))
{
  printf 'source_commit=%s\n' "$source_commit"
  printf 'binary_sha256=%s\n' "$binary_sha256"
  printf 'merchant_slot=%s\n' "$merchant_slot"
  printf 'position_before=%s\n' "$before_position"
  printf 'position_after=%s\n' "$after_position"
  printf 'elapsed_seconds=%s\n' "$elapsed_seconds"
} >"$run_dir/result"

trap - EXIT
printf 'PASS: Vailand campaign waters and merchant ship %s passed through ' \
  "$merchant_slot"
printf 'actual Kohdee (%ss).\n' "$elapsed_seconds"
printf 'Artifacts: %s\n' "$run_dir"
