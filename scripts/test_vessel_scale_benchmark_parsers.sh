#!/usr/bin/env bash

set -euo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
runner="$script_dir/run_vessel_scale_benchmark.sh"

fail()
{
  printf 'vessel scale parser test: %s\n' "$*" >&2
  exit 1
}

[[ -x "$runner" ]] || fail "scale runner is not executable: $runner"

test_root=$(mktemp -d "${TMPDIR:-/tmp}/vessel-scale-parser-test.XXXXXX")
trap 'rm -rf -- "$test_root"' EXIT

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
  "  123 buf switches   0 overflows" \
  "# checkpoint epoch=103600 label=system-3600" \
  "500 of 500 active fleet slots in use." \
  "Wilderness dynamic room pool: 25/4000 occupied (0%)" \
  "  37100 mobiles" \
  "  26100 objects" \
  "  50005 rooms" \
  "  950 lists" \
  "  130 buf switches   0 overflows" >"$live_input"

"$runner" __parse-live-system "$live_input" "$live_output"
[[ "$(wc -l <"$live_output")" == 3 ]] ||
  fail "valid live-system fixture did not produce two samples"
expected_header=$'epoch\tlabel\tfleet\tdynamic_rooms\tdynamic_capacity'
expected_header+=$'\tmobiles\tobjects\trooms\tlists\tbuffer_switches'
expected_header+=$'\tbuffer_overflows'
grep -Fqx "$expected_header" "$live_output" ||
  fail "live-system output header changed"
grep -Fqx $'100000\tsystem-0\t500\t20\t4000\t37000\t26000\t50000\t900\t123\t0' \
  "$live_output" ||
  fail "first live-system sample was parsed incorrectly"
grep -Fqx \
  $'103600\tsystem-3600\t500\t25\t4000\t37100\t26100\t50005\t950\t130\t0' \
  "$live_output" ||
  fail "second live-system sample was parsed incorrectly"

incomplete_input="$test_root/incomplete-live-input.log"
printf '%s\n' \
  "# checkpoint epoch=100000 label=system-0" \
  "500 of 500 active fleet slots in use." \
  "Wilderness dynamic room pool: 20/4000 occupied (0%)" \
  "  37000 mobiles" \
  "  26000 objects" \
  "  50000 rooms" \
  "  900 lists" >"$incomplete_input"
if "$runner" __parse-live-system "$incomplete_input" \
  "$test_root/incomplete-output.tsv" >"$test_root/stdout" 2>"$test_root/stderr"; then
  fail "incomplete live-system fixture unexpectedly passed"
fi
grep -Fq "incomplete live-system sample: system-0" "$test_root/stderr" ||
  fail "incomplete live-system fixture reported the wrong error"

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
  "Scheduled departure triggered for ship 10 on route 4." >"$noisy_log"
noisy_count=$("$runner" __count-progress-logs "$noisy_log")
[[ "$noisy_count" == 6 ]] ||
  fail "noisy server log reported $noisy_count of 6 high-volume rows"

printf '%s\n' \
  "PASS: vessel scale parsers accepted current samples and detected high-volume progress logs."
