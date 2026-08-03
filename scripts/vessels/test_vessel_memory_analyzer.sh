#!/usr/bin/env bash

set -euo pipefail

# Keep the analyzer and its fixture-driven regression test together.
script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
analyzer="$script_dir/analyze_vessel_memory_samples.sh"

fail()
{
  printf 'vessel memory analyzer test: %s\n' "$*" >&2
  exit 1
}

require_line()
{
  local output=$1
  local expected=$2

  grep -Fqx "$expected" <<<"$output" ||
    fail "missing expected output: $expected"
}

expect_failure()
{
  local input_file=$1
  local expected_error=$2
  local stderr_file="$test_root/stderr"

  if "$analyzer" --format kv "$input_file" \
    >"$test_root/stdout" 2>"$stderr_file"; then
    fail "malformed fixture unexpectedly passed: $input_file"
  fi
  grep -Fq "$expected_error" "$stderr_file" ||
    fail "malformed fixture did not report: $expected_error"
}

[[ -x "$analyzer" ]] || fail "analyzer is not executable: $analyzer"

test_root=$(mktemp -d "${TMPDIR:-/tmp}/vessel-memory-analyzer-test.XXXXXX")
trap 'rm -rf -- "$test_root"' EXIT

plateau_file="$test_root/headered-plateau.tsv"
printf 'epoch\tpid\trss_kib\tvsz_kib\tthreads\tfile_descriptors\n' \
  >"$plateau_file"
for ((offset = 0; offset <= 10800; offset += 600)); do
  if ((offset < 3600)); then
    rss=$((500000 + offset * 10))
  else
    rss=560000
  fi
  printf '%s\t1234\t%s\t700000\t2\t12\n' \
    "$((100000 + offset))" "$rss" >>"$plateau_file"
done

plateau_output=$(
  "$analyzer" \
    --format kv \
    --warmup-seconds 3600 \
    --block-seconds 3600 \
    --windows 1800,3600 \
    "$plateau_file"
)
require_line "$plateau_output" "result=REPORT_ONLY"
require_line "$plateau_output" "input_header=present"
require_line "$plateau_output" "analysis_rss_slope_kib_per_hour=0.000000"
require_line "$plateau_output" "window_1800_status=available"
require_line "$plateau_output" "window_1800_rss_slope_kib_per_hour=0.000000"
require_line "$plateau_output" "window_3600_status=available"
require_line "$plateau_output" "window_3600_rss_slope_kib_per_hour=0.000000"

rising_file="$test_root/headerless-rising.tsv"
for ((offset = 0; offset <= 7200; offset += 600)); do
  rss=$((500000 + (offset / 600) * 1000))
  vsz=$((700000 + (offset / 600) * 1000))
  printf '%s\t1234\t%s\t%s\t2\t12\n' \
    "$((200000 + offset))" "$rss" "$vsz" >>"$rising_file"
done

rising_output=$(
  "$analyzer" \
    --format kv \
    --block-seconds 3600 \
    --windows 3600,7200 \
    "$rising_file"
)
require_line "$rising_output" "input_header=absent"
require_line "$rising_output" "analysis_rss_slope_kib_per_hour=6000.000000"
require_line "$rising_output" "window_3600_rss_slope_kib_per_hour=6000.000000"
require_line "$rising_output" "window_7200_rss_slope_kib_per_hour=6000.000000"

pid_change_file="$test_root/pid-change.tsv"
printf '100\t1234\t500000\t700000\t2\t12\n' >"$pid_change_file"
printf '200\t5678\t500000\t700000\t2\t12\n' >>"$pid_change_file"
expect_failure "$pid_change_file" "PID changed within the series"

timestamp_change_file="$test_root/timestamp-change.tsv"
printf '200\t1234\t500000\t700000\t2\t12\n' >"$timestamp_change_file"
printf '100\t1234\t500000\t700000\t2\t12\n' >>"$timestamp_change_file"
expect_failure "$timestamp_change_file" "epochs must be strictly increasing"

bad_metric_file="$test_root/bad-metric.tsv"
printf '100\t1234\t500000\t700000\t2\t12\n' >"$bad_metric_file"
printf '200\t1234\tnot-a-number\t700000\t2\t12\n' >>"$bad_metric_file"
expect_failure "$bad_metric_file" "RSS must be a positive integer"

printf 'PASS: vessel memory analyzer accepted plateau and rising series and rejected malformed input.\n'
