#!/usr/bin/env bash

set -Eeuo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo_root=${LUMINARI_PROJECT_ROOT:-$(cd "$script_dir/../.." && pwd)}
package_dir="$repo_root/lib/world/vessel_campaign"
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

ensure_index_entry()
{
  local index_file=$1
  local entry=$2
  local updated_file="$run_dir/index.updated"

  if [[ ! -f "$index_file" ]]; then
    printf '%s\n$\n' "$entry" >"$index_file"
    return
  fi
  if grep -Fqx "$entry" "$index_file"; then
    return
  fi

  awk -v entry="$entry" '
    BEGIN {
      entry_number = entry + 0
    }
    # index order is the rnum order, and the world lookups binary-search it, so
    # the new entry has to land in ascending vnum position, not at the end.
    !inserted && /^[0-9]+\./ && ($0 + 0) > entry_number {
      print entry
      inserted = 1
    }
    $0 == "$" && !inserted {
      print entry
      inserted = 1
    }
    { print }
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

record_identity()
{
  local world_file=$1
  local vnum=$2

  awk -v header="#$vnum" '
    $0 == header {
      if (getline keywords > 0 && getline short_description > 0) {
        print short_description
      }
      exit
    }
  ' "$world_file"
}

remove_campaign_object_records()
{
  local package_file=$1
  local live_file=$2
  local stripped_file="$run_dir/objects.stripped"

  awk -v package_file="$package_file" '
    BEGIN {
      while ((getline line < package_file) > 0) {
        if (line ~ /^#[0-9]+$/) {
          managed[substr(line, 2) + 0] = 1
        }
      }
      close(package_file)
      emit = 1
    }
    /^#[0-9]+$/ {
      emit = !((substr($0, 2) + 0) in managed)
    }
    /^\$~?$/ {
      emit = 1
    }
    emit { print }
  ' "$live_file" >"$stripped_file"
  chmod --reference="$live_file" "$stripped_file"
  mv "$stripped_file" "$live_file"
}

merge_campaign_object_records()
{
  local package_file=$1
  local live_file=$2
  local additions_file="$run_dir/objects.add"
  local merged_file="$run_dir/objects.merged"

  awk '
    /^\$~?$/ { next }
    { print }
  ' "$package_file" >"$additions_file"

  awk -v additions_file="$additions_file" '
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
    { print }
    END {
      if (!inserted) {
        exit 42
      }
    }
  ' "$live_file" >"$merged_file" ||
    fail "$live_file has no world-file terminator"

  chmod --reference="$live_file" "$merged_file"
  mv "$merged_file" "$live_file"
}

provision_campaign_world()
{
  local object_package="$package_dir/700.obj"
  local reset_package="$package_dir/700.resets"
  local object_file="$repo_root/lib/world/obj/700.obj"
  local zone_file="$repo_root/lib/world/zon/700.zon"
  local zone_updated="$run_dir/700.zon.updated"
  local candidate
  local expected_identity
  local live_identity
  local vnum

  [[ -f "$object_package" && -f "$reset_package" ]] ||
    fail "the campaign world package is incomplete"
  [[ -f "$object_file" && -f "$zone_file" ]] ||
    fail "reserved vessel object or zone file is missing"
  [[ $(awk 'NR == 1 { print; exit }' "$zone_file") == '#700' ]] ||
    fail "$zone_file is not reserved zone 700"
  grep -Eq '^70000[[:space:]]+80019([[:space:]]|$)' "$zone_file" ||
    fail "$zone_file does not own the reserved vessel range"

  for vnum in 70015 70016 70017; do
    expected_identity=$(record_identity "$object_package" "$vnum")
    [[ -n "$expected_identity" ]] ||
      fail "campaign object $vnum is missing from its package"
    for candidate in "$repo_root/lib/world/obj"/*.obj; do
      [[ -e "$candidate" ]] || continue
      live_identity=$(record_identity "$candidate" "$vnum")
      [[ -z "$live_identity" ]] && continue
      [[ "$candidate" == "$object_file" &&
         "$live_identity" == "$expected_identity" ]] ||
        fail "campaign object VNUM $vnum collides in $candidate"
    done
  done

  remove_campaign_object_records "$object_package" "$object_file"
  merge_campaign_object_records "$object_package" "$object_file"
  ensure_index_entry "$repo_root/lib/world/obj/index" '700.obj'

  awk -v reset_package="$reset_package" '
    BEGIN {
      managed[70015] = 1
      managed[70016] = 1
      managed[70017] = 1
    }
    $1 == "R" && (($4 + 0) in managed) { next }
    $1 == "O" && (($3 + 0) in managed) { next }
    $0 == "S" && !inserted {
      while ((getline line < reset_package) > 0) {
        print line
      }
      close(reset_package)
      inserted = 1
    }
    { print }
    END { if (!inserted) exit 42 }
  ' "$zone_file" >"$zone_updated" ||
    fail "$zone_file has no reset terminator"
  chmod --reference="$zone_file" "$zone_updated"
  mv "$zone_updated" "$zone_file"
  ensure_index_entry "$repo_root/lib/world/zon/index" '700.zon'
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
  local output_file=${1:-"$run_dir/01-boot.log"}

  "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" >"$output_file" 2>&1
  restart_needed=false
}

reset_campaign_runtime()
{
  local reset_valid

  database_execute "
    UPDATE ship_runtime_state AS runtime
    JOIN vessel_npc_merchants AS merchant
      ON merchant.active_ship_id = runtime.ship_id
    JOIN ship_routes AS route ON route.route_id = merchant.route_id
       SET runtime.location_vnum = 1000360,
           runtime.x = -599,
           runtime.y = 455,
           runtime.z = 0,
           runtime.dx = 0,
           runtime.dy = 0,
           runtime.dz = 0,
           runtime.heading = 0,
           runtime.setheading = 0,
           runtime.speed = 6,
           runtime.setspeed = 6,
           runtime.autopilot_state = 1,
           runtime.current_route_id = route.route_id,
           runtime.current_waypoint_index = 0,
           runtime.autopilot_tick_counter = 0,
           runtime.wait_remaining = 0,
           runtime.last_update = UNIX_TIMESTAMP()
     WHERE merchant.name = 'Vailand Ironwind Trader';"

  reset_valid=$(database_scalar "
    SELECT IF(
      COUNT(*) = 0 OR
      (COUNT(*) = 1
       AND MIN(runtime.location_vnum) = 1000360
       AND ROUND(MIN(runtime.x)) = -599
       AND ROUND(MIN(runtime.y)) = 455
       AND MIN(runtime.autopilot_state) = 1
       AND MIN(runtime.current_route_id) = MIN(merchant.route_id)
       AND MIN(runtime.current_waypoint_index) = 0),
      1, 0)
      FROM vessel_npc_merchants AS merchant
      JOIN ship_runtime_state AS runtime
        ON runtime.ship_id = merchant.active_ship_id
     WHERE merchant.name = 'Vailand Ironwind Trader';")
  [[ "$reset_valid" == 1 ]] ||
    fail "the campaign merchant runtime could not be reset safely"
}

recover_server()
{
  local exit_status=$?

  trap - EXIT
  if [[ "$restart_needed" == true ]] && ! port_is_listening; then
    "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" \
      >"$run_dir/recovery-boot.log" 2>&1 || true
  fi
  exit "$exit_status"
}
trap recover_server EXIT

for command_name in awk chmod date git grep mariadb mkdir mv sha256sum sleep ss \
  systemctl timeout; do
  command -v "$command_name" >/dev/null 2>&1 ||
    fail "required command not found: $command_name"
done

[[ -r "$repo_root/lib/.env" ]] || fail "cannot read lib/.env"
[[ -r "$repo_root/lib/mysql_config" ]] || fail "cannot read lib/mysql_config"
[[ -x "$repo_root/bin/circle" ]] || fail "bin/circle is missing; run make install"
[[ -x "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" ]] ||
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
              'blackwake_anchorage', 'vailand_central_approach',
              'vailand_coast_turn', 'vailand_southwest_turn',
              'vailand_southern_turn', 'vailand_central_offing',
              'vailand_harbor_offing',
              'vailand_central_port')
            GROUP BY name
           HAVING COUNT(*) > 1
         ) AS duplicate_waypoints);")
[[ "$collision_count" == 0 ]] ||
  fail "campaign region, route, prototype, merchant, or waypoint names collide"

stop_development_mud
restart_needed=true
provision_campaign_world
apply_database_file "$repo_root/sql/components/vessels_phase13_schema.sql"
apply_database_file "$repo_root/sql/components/vessels_phase14_schema.sql"
apply_database_file "$repo_root/sql/components/vessels_campaign_content.sql"
reset_campaign_runtime
start_development_mud "$run_dir/01-boot.log"

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
            AND route.active = 1) = 18
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

merchant_generation=$(database_scalar "
  SELECT generation
    FROM vessel_npc_merchants
   WHERE name = 'Vailand Ironwind Trader';")
[[ "$merchant_generation" =~ ^[1-9][0-9]*$ ]] ||
  fail "the campaign merchant generation is invalid"

timeout 150 "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" --commands \
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

mapfile -t first_session_positions < <(
  awk '
    /^Coordinates: \(/ {
      position = $0
      sub(/^Coordinates: \(/, "", position)
      sub(/\).*/, "", position)
      gsub(/, /, "|", position)
      print position
    }
  ' "$run_dir/02-kohdee-campaign.log"
)
first_position_count=${#first_session_positions[@]}
[[ "$first_position_count" -ge 2 ]] ||
  fail "Kohdee did not observe two campaign merchant positions"
before_position=${first_session_positions[0]}
after_position=${first_session_positions[first_position_count - 1]}
[[ "$after_position" != "$before_position" ]] ||
  fail "the campaign merchant did not move during the actual-character window"

if [[ -f "$server_log" ]] &&
   grep -E 'SYSERR:.*(Vailand Ironwind|Vailand Iron Passage|100001[3-6])' \
     "$server_log" >"$run_dir/03-first-related-syserr.log"; then
  fail "the server logged a campaign vessel SYSERR"
fi

stop_development_mud
restart_needed=true

persisted_after_first=$(database_scalar "
  SELECT CONCAT(ROUND(runtime.x), '|', ROUND(runtime.y))
    FROM vessel_npc_merchants AS merchant
    JOIN ship_runtime_state AS runtime
      ON runtime.ship_id = merchant.active_ship_id
   WHERE merchant.name = 'Vailand Ironwind Trader';")
[[ "$persisted_after_first" != "$before_position" ]] ||
  fail "the campaign merchant movement did not persist during shutdown"

start_development_mud "$run_dir/04-restart-boot.log"

restart_state=$(database_scalar "
  SELECT CONCAT(merchant.active_ship_id, '|', merchant.generation, '|',
                runtime.autopilot_state, '|', runtime.speed, '|',
                ROUND(runtime.x), '|', ROUND(runtime.y))
    FROM vessel_npc_merchants AS merchant
    JOIN ship_runtime_state AS runtime
      ON runtime.ship_id = merchant.active_ship_id
   WHERE merchant.name = 'Vailand Ironwind Trader';")
IFS='|' read -r restart_slot restart_generation restart_autopilot \
  restart_speed restart_x restart_y <<<"$restart_state"
[[ "$restart_slot" == "$merchant_slot" &&
   "$restart_generation" == "$merchant_generation" &&
   "$restart_autopilot" =~ ^[12]$ &&
   "$restart_speed" =~ ^[1-9][0-9]*$ &&
   "$restart_x" =~ ^-?[0-9]+$ && "$restart_y" =~ ^-?[0-9]+$ &&
   "$restart_x|$restart_y" == "$persisted_after_first" ]] ||
  fail "the campaign merchant identity or autopilot did not survive restart"

timeout 150 "$repo_root/scripts/development/dev_kohdee_login_smoke.sh" --commands \
  "vmerchant list" \
  "shipgoto $merchant_slot" \
  "shipstatus" \
  "showschedule" \
  "seastate" \
  "@wait 45" \
  "shipstatus" \
  "seastate" \
  "goto 1204" \
  >"$run_dir/05-kohdee-after-restart.log" 2>&1 ||
  fail "the actual Kohdee restart-continuity session failed"

grep -Fq 'Vailand Ironwind Trader' \
  "$run_dir/05-kohdee-after-restart.log" ||
  fail "Kohdee did not see the persisted campaign merchant after restart"
grep -Fq 'Vailand Iron Passage' "$run_dir/05-kohdee-after-restart.log" ||
  fail "Kohdee did not see the persisted campaign route after restart"
grep -Fq "Arriving at vailand_central_port!" \
  "$run_dir/05-kohdee-after-restart.log" ||
  fail "the restarted campaign merchant did not complete the outbound route"

mapfile -t restart_session_positions < <(
  awk '
    /^Coordinates: \(/ {
      position = $0
      sub(/^Coordinates: \(/, "", position)
      sub(/\).*/, "", position)
      gsub(/, /, "|", position)
      print position
    }
  ' "$run_dir/05-kohdee-after-restart.log"
)
restart_position_count=${#restart_session_positions[@]}
[[ "$restart_position_count" -ge 2 ]] ||
  fail "Kohdee did not observe two merchant positions after restart"
restart_before_position=${restart_session_positions[0]}
restart_after_position=${restart_session_positions[restart_position_count - 1]}
[[ "$restart_after_position" != "$restart_before_position" ]] ||
  fail "the campaign merchant did not resume movement after restart"

if [[ -f "$server_log" ]] &&
   grep -E 'SYSERR:.*(Vailand Ironwind|Vailand Iron Passage|100001[3-6])' \
     "$server_log" >"$run_dir/06-restart-related-syserr.log"; then
  fail "the restarted server logged a campaign vessel SYSERR"
fi

stop_development_mud
restart_needed=true

persisted_after_restart=$(database_scalar "
  SELECT CONCAT(ROUND(runtime.x), '|', ROUND(runtime.y))
    FROM vessel_npc_merchants AS merchant
    JOIN ship_runtime_state AS runtime
      ON runtime.ship_id = merchant.active_ship_id
   WHERE merchant.name = 'Vailand Ironwind Trader';")
[[ "$persisted_after_restart" != "$restart_before_position" ]] ||
  fail "the resumed campaign movement did not persist during shutdown"

reset_campaign_runtime
start_development_mud "$run_dir/07-final-boot.log"

final_state=$(database_scalar "
  SELECT CONCAT(merchant.active_ship_id, '|', merchant.generation, '|',
                runtime.autopilot_state, '|', ROUND(runtime.x), '|',
                ROUND(runtime.y))
    FROM vessel_npc_merchants AS merchant
    JOIN ship_runtime_state AS runtime
      ON runtime.ship_id = merchant.active_ship_id
   WHERE merchant.name = 'Vailand Ironwind Trader';")
IFS='|' read -r final_slot final_generation final_autopilot final_x final_y \
  <<<"$final_state"
[[ "$final_slot" == "$merchant_slot" &&
   "$final_generation" == "$merchant_generation" &&
   "$final_autopilot" =~ ^[12]$ &&
   "$final_x" =~ ^-?[0-9]+$ && "$final_y" =~ ^-?[0-9]+$ &&
   "$final_x" -ge -612 && "$final_x" -le -584 &&
   "$final_y" -ge 440 && "$final_y" -le 470 ]] ||
  fail "the final campaign merchant baseline is not in North Vailand waters"

if [[ -f "$server_log" ]] &&
   grep -E 'SYSERR:.*(Vailand Ironwind|Vailand Iron Passage|100001[3-6])' \
     "$server_log" >"$run_dir/08-final-related-syserr.log"; then
  fail "the final server start logged a campaign vessel SYSERR"
fi

source_commit=$(git -C "$repo_root" rev-parse HEAD)
binary_sha256=$(sha256sum "$repo_root/bin/circle" | awk '{ print $1 }')
elapsed_seconds=$(($(date +%s) - started_epoch))
{
  printf 'source_commit=%s\n' "$source_commit"
  printf 'binary_sha256=%s\n' "$binary_sha256"
  printf 'merchant_slot=%s\n' "$merchant_slot"
  printf 'merchant_generation=%s\n' "$merchant_generation"
  printf 'position_before=%s\n' "$before_position"
  printf 'position_after=%s\n' "$after_position"
  printf 'persisted_after_first=%s\n' "$persisted_after_first"
  printf 'restart_position_before=%s\n' "$restart_before_position"
  printf 'restart_position_after=%s\n' "$restart_after_position"
  printf 'persisted_after_restart=%s\n' "$persisted_after_restart"
  printf 'final_state=%s\n' "$final_state"
  printf 'elapsed_seconds=%s\n' "$elapsed_seconds"
} >"$run_dir/result"

trap - EXIT
printf 'PASS: Vailand campaign waters and merchant ship %s passed through ' \
  "$merchant_slot"
printf 'actual Kohdee (%ss).\n' "$elapsed_seconds"
printf 'Artifacts: %s\n' "$run_dir"
