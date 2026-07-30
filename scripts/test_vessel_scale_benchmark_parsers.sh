#!/usr/bin/env bash

set -euo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(cd "$script_dir/.." && pwd)
runner="$script_dir/run_vessel_scale_benchmark.sh"

fail()
{
  printf 'vessel scale parser test: %s\n' "$*" >&2
  exit 1
}

[[ -x "$runner" ]] || fail "scale runner is not executable: $runner"

test_root=$(mktemp -d "${TMPDIR:-/tmp}/vessel-scale-parser-test.XXXXXX")
trap 'rm -rf -- "$test_root"' EXIT

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
  "Scheduled departure triggered for ship 10 on route 4." >"$noisy_log"
noisy_count=$("$runner" __count-progress-logs "$noisy_log")
[[ "$noisy_count" == 6 ]] ||
  fail "noisy server log reported $noisy_count of 6 high-volume rows"

printf '%s\n' \
  "PASS: vessel scale parsers validated chronology and detected high-volume progress logs."
