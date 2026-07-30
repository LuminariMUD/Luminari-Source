#!/usr/bin/env bash

set -euo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo_root=${LUMINARI_PROJECT_ROOT:-$(cd "$script_dir/.." && pwd)}
package_dir="$repo_root/lib/world/vessel_harbor"
temporary_dir=$(mktemp -d /tmp/luminari-vessel-harbor.XXXXXX)
server_unit=luminari-dev-login-smoke.service
ferry_passenger_fare=10
territorial_region_vnum=7000001
free_seas_region_vnum=7000002
pirate_cove_region_vnum=7000003

cleanup()
{
  find "$temporary_dir" -depth -delete
}
trap cleanup EXIT

fail()
{
  printf 'vessel harbor provisioner: %s\n' "$*" >&2
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
  local updated_file="$temporary_dir/index.updated"

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

merge_missing_records()
{
  local package_file=$1
  local live_file=$2
  local additions_file="$temporary_dir/records.add"
  local merged_file="$temporary_dir/records.merged"

  awk -v live_file="$live_file" '
    BEGIN {
      while ((getline line < live_file) > 0) {
        if (line ~ /^#[0-9]+$/) {
          vnum = substr(line, 2) + 0
          present[vnum] = 1
        }
      }
      close(live_file)
      emit = 0
    }
    /^#[0-9]+$/ {
      vnum = substr($0, 2) + 0
      emit = (vnum in present) ? 0 : 1
    }
    /^\$~?$/ {
      emit = 0
      next
    }
    emit {
      print
    }
  ' "$package_file" >"$additions_file"

  if [[ ! -s "$additions_file" ]]; then
    return
  fi

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
  local filename=$2
  local destination_dir="$repo_root/lib/world/$kind"
  local package_file="$package_dir/$filename"
  local live_file="$destination_dir/$filename"

  [[ -f "$package_file" ]] || fail "missing package file: $package_file"
  mkdir -p "$destination_dir"

  if [[ -f "$live_file" ]]; then
    merge_missing_records "$package_file" "$live_file"
  else
    cp "$package_file" "$live_file"
  fi
  ensure_index_entry "$destination_dir/index" "$filename"
}

zone_range()
{
  local zone_file=$1

  awk '
    $1 ~ /^[0-9]+$/ && $2 ~ /^[0-9]+$/ {
      print $1, $2
      exit
    }
  ' "$zone_file"
}

ensure_vessel_zone_range()
{
  local package_file="$package_dir/700.zon"
  local destination_dir="$repo_root/lib/world/zon"
  local live_file="$destination_dir/700.zon"
  local updated_file="$temporary_dir/700.zon.updated"
  local candidate
  local range_values
  local range_min
  local range_max

  [[ -f "$package_file" ]] || fail "missing package file: $package_file"
  mkdir -p "$destination_dir"

  for candidate in "$destination_dir"/*.zon; do
    [[ -e "$candidate" ]] || continue
    [[ "$candidate" == "$live_file" ]] && continue
    range_values=$(zone_range "$candidate")
    [[ -n "$range_values" ]] ||
      fail "$candidate has no numeric zone range"
    read -r range_min range_max <<<"$range_values"
    if ((range_min <= 80019 && range_max >= 80000)); then
      fail "$candidate overlaps required vessel interior VNUMs 80000-80019"
    fi
  done

  if [[ ! -f "$live_file" ]]; then
    cp "$package_file" "$live_file"
    ensure_index_entry "$destination_dir/index" "700.zon"
    return
  fi

  [[ $(awk 'NR == 1 { print; exit }' "$live_file") == "#700" ]] ||
    fail "$live_file is not zone 700"
  range_values=$(zone_range "$live_file")
  [[ -n "$range_values" ]] || fail "$live_file has no numeric zone range"
  read -r range_min range_max <<<"$range_values"
  [[ "$range_min" == 70000 ]] ||
    fail "$live_file starts at $range_min instead of reserved VNUM 70000"

  case "$range_max" in
    80019)
      ;;
    79999)
      awk '
        !updated && $1 ~ /^[0-9]+$/ && $2 ~ /^[0-9]+$/ {
          sub(/^[[:space:]]*[0-9]+[[:space:]]+[0-9]+/, "70000 80019")
          updated = 1
        }
        {
          print
        }
        END {
          if (!updated) {
            exit 42
          }
        }
      ' "$live_file" >"$updated_file" ||
        fail "could not extend $live_file"
      chmod --reference="$live_file" "$updated_file"
      mv "$updated_file" "$live_file"
      ;;
    *)
      fail "$live_file ends at unexpected VNUM $range_max"
      ;;
  esac

  ensure_index_entry "$destination_dir/index" "700.zon"
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
    --host="$database_host" --user="$database_user" "$database_name" <"$sql_file"
}

development_port()
{
  awk -F= '
    /^[[:space:]]*DFLT_PORT[[:space:]]*=/ {
      value = $2
      gsub(/[[:space:]]/, "", value)
      print value
      exit
    }
  ' "$repo_root/lib/etc/config"
}

restart_development_mud()
{
  local mud_port

  mud_port=$(development_port)
  [[ "$mud_port" =~ ^[0-9]+$ ]] || fail "could not read the development MUD port"

  if systemctl --user is-active --quiet "$server_unit"; then
    systemctl --user stop "$server_unit"
  fi
  if ss -H -ltn "sport = :$mud_port" 2>/dev/null | grep -q .; then
    fail "port $mud_port is still active; stop the manually started development MUD"
  fi

  "$script_dir/dev_kohdee_login_smoke.sh" >/dev/null
}

for command_name in awk chmod cp find grep mariadb mktemp mv ss systemctl; do
  command -v "$command_name" >/dev/null 2>&1 ||
    fail "required command not found: $command_name"
done

[[ -r "$repo_root/lib/.env" ]] || fail "cannot read lib/.env"
[[ -r "$repo_root/lib/mysql_config" ]] || fail "cannot read lib/mysql_config"
[[ -x "$repo_root/bin/circle" ]] || fail "bin/circle is missing; build and install first"

app_environment=$(config_value "$repo_root/lib/.env" APP_ENV)
[[ "$app_environment" == development ]] ||
  fail "refusing to run because APP_ENV is not development"

database_host=$(config_value "$repo_root/lib/mysql_config" mysql_host)
database_name=$(config_value "$repo_root/lib/mysql_config" mysql_database)
database_user=$(config_value "$repo_root/lib/mysql_config" mysql_username)
database_password=$(config_value "$repo_root/lib/mysql_config" mysql_password)
[[ -n "$database_host" && -n "$database_name" && -n "$database_user" ]] ||
  fail "lib/mysql_config is incomplete"

region_collision_count=$(database_scalar \
  "SELECT
     (
       SELECT COUNT(*)
        FROM region_data
        WHERE (vnum = $territorial_region_vnum
               AND COALESCE(name, '') <> 'Harbor Sandbox Territorial Waters')
           OR (name = 'Harbor Sandbox Territorial Waters'
               AND vnum <> $territorial_region_vnum)
           OR (vnum = $free_seas_region_vnum
               AND COALESCE(name, '') <> 'Harbor Sandbox Free Seas')
           OR (name = 'Harbor Sandbox Free Seas'
               AND vnum <> $free_seas_region_vnum)
           OR (vnum = $pirate_cove_region_vnum
               AND COALESCE(name, '') <> 'Harbor Sandbox Pirate Cove')
           OR (name = 'Harbor Sandbox Pirate Cove'
               AND vnum <> $pirate_cove_region_vnum)
     )
     +
     (
       SELECT COUNT(*)
         FROM region_index AS region_idx
         LEFT JOIN region_data AS region ON region.vnum = region_idx.vnum
        WHERE region_idx.vnum IN (
          $territorial_region_vnum,
          $free_seas_region_vnum,
          $pirate_cove_region_vnum
        )
          AND (
            region.vnum IS NULL
            OR (region_idx.vnum = $territorial_region_vnum
                AND COALESCE(region.name, '') <>
                    'Harbor Sandbox Territorial Waters')
            OR (region_idx.vnum = $free_seas_region_vnum
                AND COALESCE(region.name, '') <> 'Harbor Sandbox Free Seas')
            OR (region_idx.vnum = $pirate_cove_region_vnum
                AND COALESCE(region.name, '') <> 'Harbor Sandbox Pirate Cove')
          )
     )")
[[ "$region_collision_count" == 0 ]] ||
  fail "reserved harbor legal-water region VNUM or name is already in use"

ensure_vessel_zone_range
provision_world_file wld 10000.wld
provision_world_file mob 700.mob
provision_world_file trg 700.trg

apply_database_file "$repo_root/sql/components/vessels_phase11_schema.sql"
apply_database_file "$repo_root/sql/components/vessels_phase12_schema.sql"
apply_database_file "$repo_root/sql/components/vessels_phase13_schema.sql"
apply_database_file "$repo_root/sql/components/vessels_harbor_sandbox.sql"

legal_waters_valid=$(database_scalar \
  "SELECT IF(
       COUNT(*) = 3
       AND SUM(
         CASE
           WHEN law.region_vnum = $territorial_region_vnum
            AND region.name = 'Harbor Sandbox Territorial Waters'
            AND law.waters_type = 1
            AND law.priority = 100
            AND law.bounty_percent = 150
            AND law.authority = 'Harbor Admiralty'
            AND ST_Within(
              ST_GeomFromText('POINT(-66 92)'),
              region_idx.region_polygon
            )
             THEN 1
           WHEN law.region_vnum = $free_seas_region_vnum
            AND region.name = 'Harbor Sandbox Free Seas'
            AND law.waters_type = 2
            AND law.priority = 150
            AND law.bounty_percent = 100
            AND law.authority = 'Free Captains'' Compact'
            AND ST_Within(
              ST_GeomFromText('POINT(-64 82)'),
              region_idx.region_polygon
            )
             THEN 1
           WHEN law.region_vnum = $pirate_cove_region_vnum
            AND region.name = 'Harbor Sandbox Pirate Cove'
            AND law.waters_type = 3
            AND law.priority = 200
            AND law.bounty_percent = 0
            AND law.authority = 'Cove Brotherhood'
            AND ST_Within(
              ST_GeomFromText('POINT(-58 91)'),
              region_idx.region_polygon
            )
             THEN 1
           ELSE 0
         END
       ) = 3,
       1,
       0
     )
     FROM vessel_region_law AS law
     JOIN region_data AS region ON region.vnum = law.region_vnum
     JOIN region_index AS region_idx ON region_idx.vnum = law.region_vnum
    WHERE law.region_vnum IN (
      $territorial_region_vnum,
      $free_seas_region_vnum,
      $pirate_cove_region_vnum
    )")
[[ "$legal_waters_valid" == 1 ]] ||
  fail "the harbor legal-water regions or vessel-law metadata are invalid"

restart_development_mud

ferry_prototype_id=$(database_scalar \
  "SELECT MIN(prototype_id) FROM ship_prototypes WHERE name = 'Harbor Sandbox Ferry'")
[[ "$ferry_prototype_id" =~ ^[0-9]+$ ]] ||
  fail "the harbor ferry prototype was not seeded"

ferry_slot=$(database_scalar \
  "SELECT MIN(r.ship_id)
     FROM ship_runtime_state AS r
     JOIN ship_interiors AS i ON i.ship_id = r.ship_id
    WHERE r.prototype_id = $ferry_prototype_id
      AND i.owner = ''
      AND i.vessel_name = 'Harbor Sandbox Ferry'")

ferry_was_created=false
if [[ ! "$ferry_slot" =~ ^[0-9]+$ ]]; then
  "$script_dir/dev_kohdee_login_smoke.sh" --commands \
    "goto -66 92" \
    "vedit spawnpublic $ferry_prototype_id"
  ferry_was_created=true
  ferry_slot=$(database_scalar \
    "SELECT MIN(r.ship_id)
       FROM ship_runtime_state AS r
       JOIN ship_interiors AS i ON i.ship_id = r.ship_id
      WHERE r.prototype_id = $ferry_prototype_id
        AND i.owner = ''
        AND i.vessel_name = 'Harbor Sandbox Ferry'")
fi
[[ "$ferry_slot" =~ ^[0-9]+$ ]] || fail "the public harbor ferry was not created"

pilot_count=$(database_scalar \
  "SELECT COUNT(*)
     FROM ship_crew_roster
    WHERE ship_id = $ferry_slot
      AND crew_role = 'pilot'
      AND npc_vnum = 70001
      AND status = 'active'")
schedule_count=$(database_scalar \
  "SELECT COUNT(*)
     FROM ship_schedules AS s
     JOIN ship_routes AS r ON r.route_id = s.route_id
    WHERE s.ship_id = $ferry_slot
      AND r.name = 'harbor_ferry_loop'
      AND s.enabled = 1
      AND s.passenger_fare = $ferry_passenger_fare")
active_route_count=$(database_scalar \
  "SELECT COUNT(*)
     FROM ship_runtime_state AS state
     JOIN ship_routes AS route ON route.route_id = state.current_route_id
    WHERE state.ship_id = $ferry_slot
      AND route.name = 'harbor_ferry_loop'
      AND state.autopilot_state IN (1, 2)")
route_topology_valid=$(database_scalar \
  "SELECT IF(
       COUNT(*) = 4
       AND SUM(
         CASE
           WHEN route_point.sequence_num = 0 AND waypoint.name = 'harbor_west_dock' THEN 1
           WHEN route_point.sequence_num = 1 AND waypoint.name = 'harbor_channel_turn' THEN 1
           WHEN route_point.sequence_num = 2 AND waypoint.name = 'harbor_east_dock' THEN 1
           WHEN route_point.sequence_num = 3 AND waypoint.name = 'harbor_channel_turn' THEN 1
           ELSE 0
         END
       ) = 4,
       1,
       0
     )
     FROM ship_route_waypoints AS route_point
     JOIN ship_routes AS route ON route.route_id = route_point.route_id
     JOIN ship_waypoints AS waypoint ON waypoint.waypoint_id = route_point.waypoint_id
    WHERE route.route_id = (
      SELECT MIN(route_id)
        FROM ship_routes
       WHERE name = 'harbor_ferry_loop'
    )")

if [[ "$ferry_was_created" == true || "$pilot_count" != 1 ||
      "$schedule_count" != 1 || "$active_route_count" != 1 ||
      "$route_topology_valid" != 1 ]]; then
  ferry_commands=(
    "shipgoto $ferry_slot"
    "setroute harbor_ferry_loop"
    "speed 2"
    "setschedule harbor_ferry_loop 1 $ferry_passenger_fare"
  )
  if [[ "$pilot_count" == 0 ]]; then
    ferry_commands+=("load mob 70001" "assignpilot ferrymaster")
  elif [[ "$pilot_count" != 1 ]]; then
    ferry_commands+=("unassignpilot" "assignpilot ferrymaster")
  else
    ferry_commands+=("autopilot on")
  fi
  "$script_dir/dev_kohdee_login_smoke.sh" --commands "${ferry_commands[@]}"
  restart_development_mud
fi

pilot_count=$(database_scalar \
  "SELECT COUNT(*)
     FROM ship_crew_roster
    WHERE ship_id = $ferry_slot
      AND crew_role = 'pilot'
      AND npc_vnum = 70001
      AND status = 'active'")
schedule_count=$(database_scalar \
  "SELECT COUNT(*)
     FROM ship_schedules AS s
     JOIN ship_routes AS r ON r.route_id = s.route_id
    WHERE s.ship_id = $ferry_slot
      AND r.name = 'harbor_ferry_loop'
      AND s.enabled = 1
      AND s.passenger_fare = $ferry_passenger_fare")
route_topology_valid=$(database_scalar \
  "SELECT IF(
       COUNT(*) = 4
       AND SUM(
         CASE
           WHEN route_point.sequence_num = 0 AND waypoint.name = 'harbor_west_dock' THEN 1
           WHEN route_point.sequence_num = 1 AND waypoint.name = 'harbor_channel_turn' THEN 1
           WHEN route_point.sequence_num = 2 AND waypoint.name = 'harbor_east_dock' THEN 1
           WHEN route_point.sequence_num = 3 AND waypoint.name = 'harbor_channel_turn' THEN 1
           ELSE 0
         END
       ) = 4,
       1,
       0
     )
     FROM ship_route_waypoints AS route_point
     JOIN ship_routes AS route ON route.route_id = route_point.route_id
     JOIN ship_waypoints AS waypoint ON waypoint.waypoint_id = route_point.waypoint_id
    WHERE route.route_id = (
      SELECT MIN(route_id)
        FROM ship_routes
       WHERE name = 'harbor_ferry_loop'
    )")
bridge_room=$(database_scalar \
  "SELECT bridge_room FROM ship_interiors WHERE ship_id = $ferry_slot")
cargo_room=$(database_scalar \
  "SELECT cargo_room1 FROM ship_interiors WHERE ship_id = $ferry_slot")

[[ "$pilot_count" == 1 ]] || fail "the ferry pilot did not survive restart"
[[ "$schedule_count" == 1 ]] || fail "the ferry schedule did not survive restart"
[[ "$route_topology_valid" == 1 ]] ||
  fail "the ferry route does not contain the expected four-leg channel loop"
[[ "$bridge_room" =~ ^[0-9]+$ && "$bridge_room" -gt 0 ]] ||
  fail "the ferry bridge is unavailable after restart"
[[ "$cargo_room" =~ ^[0-9]+$ && "$cargo_room" -gt 0 ]] ||
  fail "the ferry cargo hold is unavailable after restart"

verification_output=$("$script_dir/dev_kohdee_login_smoke.sh" --commands \
  "shipgoto $ferry_slot" \
  "look ferrymaster" \
  "say harborcheck" \
  "goto $cargo_room" \
  "say cargocheck" \
  "goto 1000390" \
  "vstat r 1000390" \
  "shipgoto $ferry_slot" \
  "showschedule" \
  "seastate" \
  "shipstatus")
printf '%s\n' "$verification_output"

grep -Fq "bridge trigger ready" <<<"$verification_output" ||
  fail "the generated bridge trigger did not fire"
grep -Fq "cargo trigger ready" <<<"$verification_output" ||
  fail "the generated cargo trigger did not fire"
grep -Fq "Room name: Harbor Sandbox East Dock" <<<"$verification_output" ||
  fail "the east harbor dock did not load"
grep -Fq "Passenger Fare: $ferry_passenger_fare gold per boarding" \
  <<<"$verification_output" ||
  fail "the ferry passenger fare did not survive restart"
grep -Eq \
  "Waters    : Harbor Sandbox (Territorial Waters|Free Seas)" \
  <<<"$verification_output" ||
  fail "seastate did not resolve the ferry's canonical legal-water region"

gold_output=$("$script_dir/dev_kohdee_login_smoke.sh" --commands "gold")
kohdee_gold=$(awk '
  /You have [0-9]+ gold coins[.]/ {
    print $3
    exit
  }
' <<<"$gold_output")
[[ "$kohdee_gold" =~ ^[0-9]+$ && "$kohdee_gold" -ge "$ferry_passenger_fare" ]] ||
  fail "Kohdee needs at least $ferry_passenger_fare gold for the fare check"
expected_after_fare=$((kohdee_gold - ferry_passenger_fare))

set +e
fare_output=$("$script_dir/dev_kohdee_login_smoke.sh" --commands \
  "shipgoto $ferry_slot" \
  "autopilot pause" \
  "disembark" \
  "gold" \
  "board ferry" \
  "gold" \
  "autopilot on" \
  "set Kohdee gold $kohdee_gold" \
  "gold" \
  "disembark" \
  "goto 1000389")
fare_status=$?
set -e
printf '%s\n' "$fare_output"

fare_failure=
if ((fare_status != 0)); then
  fare_failure="the public ferry fare session failed with status $fare_status"
elif ! grep -Fq \
  "The purser collects $ferry_passenger_fare gold for passage aboard Harbor Sandbox Ferry." \
  <<<"$fare_output"; then
  fare_failure="the public ferry did not collect its configured fare"
elif ! grep -Fq "You have $expected_after_fare gold coins." <<<"$fare_output"; then
  fare_failure="the public ferry did not deduct exactly one fare"
fi
restored_gold_count=$(grep -Fc "You have $kohdee_gold gold coins." <<<"$fare_output" || true)
if [[ -z "$fare_failure" && "$restored_gold_count" -lt 2 ]]; then
  fare_failure="Kohdee's pre-check gold was not restored after the fare test"
fi

if [[ -n "$fare_failure" ]]; then
  "$script_dir/dev_kohdee_login_smoke.sh" --commands \
    "shipgoto $ferry_slot" \
    "autopilot on" \
    "set Kohdee gold $kohdee_gold" \
    "goto 1000389" >/dev/null 2>&1 || true
  fail "$fare_failure"
fi

set +e
crossing_output=$(
  "$script_dir/dev_kohdee_login_smoke.sh" --vessel-crossing-check "$ferry_slot"
)
crossing_status=$?
set -e
printf '%s\n' "$crossing_output"
((crossing_status == 0)) ||
  fail "the named-water crossing session failed with status $crossing_status"
grep -Fq "announced a canonical boundary crossing" <<<"$crossing_output" ||
  fail "the moving ferry did not announce a canonical named-water crossing"
grep -Fq "seastate matched its water type, authority, and piracy bounty" \
  <<<"$crossing_output" ||
  fail "the crossing transcript did not match the canonical vessel-law metadata"

set +e
channel_output=$(
  "$script_dir/dev_kohdee_login_smoke.sh" \
    --vessel-channel-check "$ferry_slot" 2>&1
)
channel_status=$?
set -e
printf '%s\n' "$channel_output"

if ((channel_status != 0)) &&
   grep -Fq "the master account has no other usable character" \
     <<<"$channel_output"; then
  "$script_dir/dev_create_test_character.sh" Vesselmate ||
    fail "could not add Vesselmate to the existing master account"

  set +e
  channel_output=$(
    "$script_dir/dev_kohdee_login_smoke.sh" \
      --vessel-channel-check "$ferry_slot" 2>&1
  )
  channel_status=$?
  set -e
  printf '%s\n' "$channel_output"
fi

((channel_status == 0)) ||
  fail "the same-account vessel-channel session failed with status $channel_status"
grep -Fq "exchanged identified captain-channel messages across separate rooms" \
  <<<"$channel_output" ||
  fail "the channel transcript did not prove cross-room delivery"
grep -Fq "aboard channel stayed isolated" <<<"$channel_output" ||
  fail "the channel transcript did not prove ashore isolation and refusals"

printf 'PASS: harbor sandbox and persistent ferry verified in ship slot %s.\n' \
  "$ferry_slot"
