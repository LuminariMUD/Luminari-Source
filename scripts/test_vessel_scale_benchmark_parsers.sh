#!/usr/bin/env bash

set -euo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(cd "$script_dir/.." && pwd)
runner="$script_dir/run_vessel_scale_benchmark.sh"
spawn_safe_command="spawn_commands+=(\"shiplist summary\" \"goto \$benchmark_safe_room\")"

fail()
{
  printf 'vessel scale parser test: %s\n' "$*" >&2
  exit 1
}

[[ -x "$runner" ]] || fail "scale runner is not executable: $runner"

test_root=$(mktemp -d "${TMPDIR:-/tmp}/vessel-scale-parser-test.XXXXXX")
trap 'rm -rf -- "$test_root"' EXIT

provenance_root="$test_root/provenance"
provenance_binary="$provenance_root/bin/circle"
provenance_source="$provenance_root/src/vessels/vessels.c"
mkdir -p "$provenance_root/bin" "$provenance_root/src/vessels"
touch -t 202608010100 "$provenance_binary"
touch -t 202608010200 "$provenance_source"
stale_input=$(
  "$runner" __newer-binary-input "$provenance_root" "$provenance_binary"
)
[[ "$stale_input" == "$provenance_source" ]] ||
  fail "stale installed-binary fixture did not identify the newer source"
touch -t 202608010300 "$provenance_binary"
stale_input=$(
  "$runner" __newer-binary-input "$provenance_root" "$provenance_binary"
)
[[ -z "$stale_input" ]] ||
  fail "current installed-binary fixture was incorrectly rejected"

valid_tick_row="vessel_tick,1200,120000,100.00,80.00,120.00,180.00,400,1200,1200"
"$runner" __validate-vessel-tick-row "$valid_tick_row" ||
  fail "valid vessel-tick percentile row was rejected"
"$runner" __validate-vessel-tick-row "$valid_tick_row"$'\r' ||
  fail "valid CR-terminated vessel-tick percentile row was rejected"
invalid_tick_row="vessel_tick,1200,120000,100.00,80.00,,180.00,400,1200,1200"
if "$runner" __validate-vessel-tick-row "$invalid_tick_row"; then
  fail "vessel-tick row with a missing p95 unexpectedly passed"
fi
invalid_tick_row="vessel_tick,1200,120000,100.00,80.00,220.00,180.00,400,1200,1200"
if "$runner" __validate-vessel-tick-row "$invalid_tick_row"; then
  fail "vessel-tick row with inverted percentiles unexpectedly passed"
fi
invalid_tick_row="vessel_tick,1200,120000,100.00,80.00,120.00,180.00,400,1200,1199"
if "$runner" __validate-vessel-tick-row "$invalid_tick_row"; then
  fail "vessel-tick row with impossible sample counts unexpectedly passed"
fi

source_perf_sections="$test_root/source-perf-sections.txt"
runner_perf_sections="$test_root/runner-perf-sections.txt"
sed -nE \
  's/.*PERF_prof_sect_init\([^,]+, "(vessel_[^"]+)"\);.*/\1/p' \
  "$repo_root/src/comm.c" | sort -u >"$source_perf_sections"
awk '
  /^vessel_perf_sections=\(/ {
    capture = 1
    next
  }
  capture && /^\)/ {
    exit
  }
  capture {
    gsub(/^[[:space:]]+|[[:space:]]+$/, "")
    if (length($0) > 0) {
      print
    }
  }
' "$runner" | sort -u >"$runner_perf_sections"
diff -u "$source_perf_sections" "$runner_perf_sections" ||
  fail "scale-runner profiler contract drifted from the production heartbeat"

runner_snapshot_tables="$test_root/runner-snapshot-tables.txt"
awk '
  /^snapshot_tables=\(/ {
    capture = 1
    next
  }
  capture && /^\)/ {
    exit
  }
  capture {
    gsub(/^[[:space:]]+|[[:space:]]+$/, "")
    if (length($0) > 0) {
      print
    }
  }
' "$runner" | sort -u >"$runner_snapshot_tables"
for required_table in vessel_merchant_consequences vessel_npc_merchants; do
  grep -Fxq "$required_table" "$runner_snapshot_tables" ||
    fail "scale-runner snapshot omits mutable table $required_table"
done

msdp_boundary_update=$(awk '
  /^SET @msdp_ship =/ {
    selected = 1
  }
  selected && /^UPDATE ship_runtime_state$/ {
    capture = 1
  }
  capture {
    print
  }
  capture && /WHERE ship_id = @msdp_ship;/ {
    exit
  }
' "$runner")
grep -Eq 'autopilot_state[[:space:]]*=[[:space:]]*3' <<<"$msdp_boundary_update" ||
  fail "scale-runner airship boundary fixture is not paused at its ceiling"
preparation_block=$(sed -n '/^  preparation_commands=(/,/^  )/p' "$runner")
[[ "$(grep -Fc '    "speed 2"' <<<"$preparation_block")" == 3 ]] ||
  fail "scale-runner vertical probes do not all set positive speed"
[[ "$(grep -Fc '    "setsail up"' <<<"$preparation_block")" == 3 ]] ||
  fail "scale-runner vertical-probe count changed unexpectedly"
grep -Fq 'a vertical traversal probe did not reach the terrain gate' "$runner" ||
  fail "scale-runner does not reject a skipped vertical terrain probe"
grep -Fxq 'benchmark_safe_room=1204' "$runner" ||
  fail "scale-runner does not use the quiet staff room for generic ashore state"
grep -Fq "$spawn_safe_command" "$runner" ||
  fail "scale-runner does not save the spawn character in the quiet staff room"
grep -Fq 'workload_log_offset=0' "$runner" ||
  fail "scale-runner does not read the fresh reconstruction log from byte zero"
if grep -Fq "workload_log_offset=\$(stat" "$runner"; then
  fail "scale-runner still carries an offset across the truncated restart log"
fi
login_helper="$script_dir/dev_kohdee_login_smoke.sh"
[[ "$(grep -Fc 'run_game_command "goto 1204"' \
  "$login_helper")" -ge 2 ]] ||
  fail "generic MSDP and message helpers do not leave crowded benchmark waters"
grep -Fq "set display_line [string trimleft \$line \"\\r\"]" \
  "$login_helper" ||
  fail "login helper does not normalize LF-CR command output"
grep -Fq 'stty raw -echo; exec nc 127.0.0.1' "$login_helper" ||
  fail "native MSDP helper does not use a raw, no-echo Telnet connection"
grep -Fq 'send -- [binary format H* fffc18fffd45]' "$login_helper" ||
  fail "native MSDP helper does not complete TTYPE-first negotiation"
grep -Fq 'proc extract_last_msdp_value' "$login_helper" ||
  fail "native MSDP helper does not apply the last frame for effective state"
grep -Fq 'set output [run_game_command "autopilot pause"]' "$login_helper" ||
  fail "native MSDP helper does not stabilize a moving comparison vessel"
grep -Fq 'run_game_command "autopilot on"' "$login_helper" ||
  fail "native MSDP helper does not resume a vessel it paused"
[[ "$(grep -Ec '^  require_msdp_cleared .* SHIP_' "$login_helper")" == 9 ]] ||
  fail "native MSDP helper does not validate all nine effective ashore values"
wait_handler=$(awk '
  /if \{\[regexp \{\^@wait / {
    capture = 1
  }
  capture {
    print
  }
  capture && /return \$cleaned/ {
    exit
  }
' "$login_helper")
grep -Fq "append output \$expect_out(buffer)" <<<"$wait_handler" ||
  fail "generic login wait does not retain asynchronous game output"
grep -Fq "return \$cleaned" <<<"$wait_handler" ||
  fail "generic login wait does not return its asynchronous game output"
if grep -Fq -- '-re {.+} {}' <<<"$wait_handler"; then
  fail "generic login wait still discards asynchronous game output"
fi
# shellcheck disable=SC2016
dock_wait_handler=$(sed -n \
  '/if {\$command eq "@wait-vessel-dock"/,/if {\[regexp {\^@wait /p' \
  "$login_helper")
grep -Fq 'set stopped' <<<"$dock_wait_handler" ||
  fail "vessel dock wait does not inspect stopped state"
grep -Fq "&& \$stopped" <<<"$dock_wait_handler" ||
  fail "vessel dock wait can still return while the vessel is moving"
grep -Fq \
  "('\${benchmark_prefix} Water West', -66, 82, 0, 0.5, 5, 0)," \
  "$runner" ||
  fail "scale-runner scheduled route does not remain in the verified water channel"
runtime_y_case=$(sed -n '/runtime\.y = CASE/,/       END,/p' "$runner")
grep -Fq '         ELSE 82' <<<"$runtime_y_case" ||
  fail "scale-runner water-class fixtures do not remain in the verified channel"
if grep -Fq 'WHEN 0 THEN 92 ELSE 82' <<<"$runtime_y_case"; then
  fail "scale-runner water-class fixtures still alternate across invalid terrain"
fi
grep -Fq 'Info: NPC vessel prototype %d deferred: fleet is full' \
  "$repo_root/src/vessels/vessels_edit.c" ||
  fail "expected full-fleet NPC spawning is still logged as a server error"

fare_commands=$(sed -n '/^fare_output=.*--commands/,/^fare_status=/p' \
  "$script_dir/provision_vessel_harbor.sh")
fare_wait_line=$(grep -nF '"@wait-vessel-west-dock"' <<<"$fare_commands" |
  cut -d: -f1)
fare_disembark_line=$(grep -nF '"disembark"' <<<"$fare_commands" | head -1 |
  cut -d: -f1)
fare_dock_line=$(grep -nF '"goto 1000389"' <<<"$fare_commands" | head -1 |
  cut -d: -f1)
fare_board_line=$(grep -nF '"board ferry"' <<<"$fare_commands" | cut -d: -f1)
[[ "$fare_wait_line" =~ ^[0-9]+$ && "$fare_disembark_line" =~ ^[0-9]+$ &&
   "$fare_dock_line" =~ ^[0-9]+$ && "$fare_board_line" =~ ^[0-9]+$ &&
   "$fare_wait_line" -lt "$fare_disembark_line" &&
   "$fare_disembark_line" -lt "$fare_dock_line" &&
   "$fare_dock_line" -lt "$fare_board_line" ]] ||
  fail "harbor fare gate does not stop and resolve the canonical west dock before boarding"

live_input="$test_root/live-input.log"
live_output="$test_root/live-output.tsv"
printf '%s\n' \
  "# checkpoint epoch=100000 label=system-0" \
  "500 of 500 active fleet slots in use." \
  "Wilderness dynamic room pool: 20/4000 occupied (0%)" \
  "  37000 mobiles" \
  "  26000 objects" \
  "  50000 rooms" \
  "  900 lists" \
  "  1200000 movement trails" \
  "  123 buf switches   0 overflows" \
  "# checkpoint epoch=103600 label=system-3600" \
  "500 of 500 active fleet slots in use." \
  "Wilderness dynamic room pool: 25/4000 occupied (0%)" \
  "  37100 mobiles" \
  "  26100 objects" \
  "  50005 rooms" \
  "  950 lists" \
  "  1210000 movement trails" \
  "  130 buf switches   0 overflows" >"$live_input"

"$runner" __parse-live-system "$live_input" "$live_output"
[[ "$(wc -l <"$live_output")" == 3 ]] ||
  fail "valid live-system fixture did not produce two samples"
expected_header=$'epoch\tlabel\tfleet\tdynamic_rooms\tdynamic_capacity'
expected_header+=$'\tmobiles\tobjects\trooms\tlists\tmovement_trails'
expected_header+=$'\tbuffer_switches\tbuffer_overflows'
grep -Fqx "$expected_header" "$live_output" ||
  fail "live-system output header changed"
grep -Fqx \
  $'100000\tsystem-0\t500\t20\t4000\t37000\t26000\t50000\t900\t1200000\t123\t0' \
  "$live_output" ||
  fail "first live-system sample was parsed incorrectly"
grep -Fqx \
  $'103600\tsystem-3600\t500\t25\t4000\t37100\t26100\t50005\t950\t1210000\t130\t0' \
  "$live_output" ||
  fail "second live-system sample was parsed incorrectly"
"$runner" __validate-live-system "$live_output" 3600 ||
  fail "valid live-system chronology was rejected"

incomplete_input="$test_root/incomplete-live-input.log"
printf '%s\n' \
  "# checkpoint epoch=100000 label=system-0" \
  "500 of 500 active fleet slots in use." \
  "Wilderness dynamic room pool: 20/4000 occupied (0%)" \
  "  37000 mobiles" \
  "  26000 objects" \
  "  50000 rooms" \
  "  900 lists" \
  "  1200000 movement trails" >"$incomplete_input"
if "$runner" __parse-live-system "$incomplete_input" \
  "$test_root/incomplete-output.tsv" >"$test_root/stdout" 2>"$test_root/stderr"; then
  fail "incomplete live-system fixture unexpectedly passed"
fi
grep -Fq "incomplete live-system sample: system-0" "$test_root/stderr" ||
  fail "incomplete live-system fixture reported the wrong error"

missing_trail_input="$test_root/missing-trail-input.log"
printf '%s\n' \
  "# checkpoint epoch=100000 label=system-0" \
  "500 of 500 active fleet slots in use." \
  "Wilderness dynamic room pool: 20/4000 occupied (0%)" \
  "  37000 mobiles" \
  "  26000 objects" \
  "  50000 rooms" \
  "  900 lists" \
  "  123 buf switches   0 overflows" >"$missing_trail_input"
if "$runner" __parse-live-system "$missing_trail_input" \
  "$test_root/missing-trail-output.tsv" \
  >"$test_root/stdout" 2>"$test_root/stderr"; then
  fail "live-system fixture without movement trails unexpectedly passed"
fi
grep -Fq "incomplete live-system sample: system-0" "$test_root/stderr" ||
  fail "missing movement-trail fixture reported the wrong error"

chronology_input="$test_root/bad-chronology.tsv"
printf '%s\n' \
  "$expected_header" \
  $'100000\tsystem-0\t500\t20\t4000\t37000\t26000\t50000\t900\t1200000\t123\t0' \
  $'100000\tsystem-3600\t500\t25\t4000\t37100\t26100\t50005\t950\t1210000\t130\t0' \
  >"$chronology_input"
if "$runner" __validate-live-system "$chronology_input" 3600 \
  >"$test_root/stdout" 2>"$test_root/stderr"; then
  fail "duplicate live-system epoch unexpectedly passed"
fi
grep -Fq "epochs are not strictly increasing" "$test_root/stderr" ||
  fail "duplicate live-system epoch reported the wrong error"

overflow_input="$test_root/buffer-overflow.tsv"
printf '%s\n' \
  "$expected_header" \
  $'100000\tsystem-0\t500\t20\t4000\t37000\t26000\t50000\t900\t1200000\t123\t0' \
  $'103600\tsystem-3600\t500\t25\t4000\t37100\t26100\t50005\t950\t1210000\t130\t1' \
  >"$overflow_input"
if "$runner" __validate-live-system "$overflow_input" 3600 \
  >"$test_root/stdout" 2>"$test_root/stderr"; then
  fail "nonzero live-system buffer overflow unexpectedly passed"
fi
grep -Fq "fleet, room-capacity, or buffer invariant" "$test_root/stderr" ||
  fail "nonzero buffer overflow reported the wrong error"

final_label_input="$test_root/bad-final-label.tsv"
printf '%s\n' \
  "$expected_header" \
  $'100000\tsystem-0\t500\t20\t4000\t37000\t26000\t50000\t900\t1200000\t123\t0' \
  $'101800\tsystem-1800\t500\t25\t4000\t37100\t26100\t50005\t950\t1210000\t130\t0' \
  >"$final_label_input"
if "$runner" __validate-live-system "$final_label_input" 3600 \
  >"$test_root/stdout" 2>"$test_root/stderr"; then
  fail "incorrect final live-system label unexpectedly passed"
fi
grep -Eq "intermediate checkpoint is not hourly|final checkpoint label" \
  "$test_root/stderr" ||
  fail "incorrect final live-system label reported the wrong error"

clean_log="$test_root/clean-server.log"
printf '%s\n' \
  "Scheduled departure triggered for ship 10 on route 4." \
  "Shared encounter 'test' notified 2 vessels." >"$clean_log"
clean_count=$("$runner" __count-progress-logs "$clean_log")
[[ "$clean_count" == 0 ]] ||
  fail "clean server log reported $clean_count high-volume rows"

noisy_log="$test_root/noisy-server.log"
printf '%s\n' \
  "Info: Ship 5 departing room 100, moving to room 101 at (-64, 82)" \
  "Info: Ship 5 arrived at waypoint 'West', advancing" \
  "Info: Ship 5 wait complete, advancing to next waypoint" \
  "Info: Ship 5 completed route 'Harbor Loop'" \
  "[VESSEL_MOVE] Ship 6 position updated to (-63,82,0) in room 102" \
  "[VESSEL_AUTO] Ship 6 arrived at waypoint 'East', waiting 5 seconds" \
  "-> Processing REGION_TYPE : 2" \
  "  -> Changing (-64, 82) to sector : 5" \
  "  -> Adjusting elevation at (-64, 82) by : 1" \
  "PATH: harbor_channel found!" \
  "COMPREHENSIVE ELEVATION: Adjusting elevation at (-64, 82) by 1 (region: test)" \
  "Scheduled departure triggered for ship 10 on route 4." >"$noisy_log"
noisy_count=$("$runner" __count-progress-logs "$noisy_log")
[[ "$noisy_count" == 11 ]] ||
  fail "noisy server log reported $noisy_count of 11 high-volume rows"

printf '%s\n' \
  "PASS: vessel scale parsers validated provenance, percentiles, chronology, and logs."
