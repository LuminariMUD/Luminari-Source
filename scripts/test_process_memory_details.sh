#!/usr/bin/env bash

set -euo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
sampler="$script_dir/sample_process_memory_details.sh"

fail()
{
  printf 'process memory detail sampler test: %s\n' "$*" >&2
  exit 1
}

expect_validation_failure()
{
  local input_file=$1
  local expected_error=$2

  if "$sampler" --validate "$input_file" \
    >"$test_root/stdout" 2>"$test_root/stderr"; then
    fail "invalid series unexpectedly passed: $input_file"
  fi
  grep -Fq "$expected_error" "$test_root/stderr" ||
    fail "invalid series did not report: $expected_error"
}

[[ -x "$sampler" ]] || fail "sampler is not executable: $sampler"

test_root=$(mktemp -d "${TMPDIR:-/tmp}/process-memory-detail-test.XXXXXX")
trap 'rm -rf -- "$test_root"' EXIT

status_file="$test_root/status"
smaps_file="$test_root/smaps"
samples_file="$test_root/samples.tsv"

printf '%s\n' \
  "Name: fixture" \
  "VmSize: 2000 kB" \
  "VmRSS: 1500 kB" \
  "RssAnon: 1200 kB" \
  "RssFile: 300 kB" \
  "RssShmem: 0 kB" \
  "VmData: 1600 kB" \
  "VmSwap: 0 kB" >"$status_file"
printf '%s\n' \
  "00400000-00401000 r--p 00000000 00:00 0 /fixture" \
  "Size: 4 kB" \
  "Rss: 4 kB" \
  "Private_Dirty: 0 kB" \
  "00600000-00700000 rw-p 00000000 00:00 0 [heap]" \
  "Size: 1000 kB" \
  "Rss: 900 kB" \
  "Private_Dirty: 850 kB" \
  "7f000000-7f001000 r--p 00000000 00:00 0 /library" \
  "Size: 4 kB" \
  "Rss: 4 kB" \
  "Private_Dirty: 0 kB" >"$smaps_file"

"$sampler" --header >"$samples_file"
"$sampler" __parse "$status_file" "$smaps_file" 100 1234 system-0 \
  >>"$samples_file"
"$sampler" __parse "$status_file" "$smaps_file" 200 1234 system-100 \
  >>"$samples_file"
expected_row=$'100\tsystem-0\t1234\t2000\t1500\t1200\t300\t0\t1600\t0'
expected_row+=$'\t1000\t900\t850'
grep -Fqx "$expected_row" "$samples_file" ||
  fail "fixture metrics were parsed incorrectly"
"$sampler" --validate "$samples_file" ||
  fail "valid detail series was rejected"

live_row=$("$sampler" --sample "$$" live)
[[ "$(awk -F '\t' '{ print NF }' <<<"$live_row")" == 13 ]] ||
  fail "live process sample does not have 13 fields"
[[ "$live_row" == *$'\tlive\t'* ]] ||
  fail "live process sample lost its label"

pid_change_file="$test_root/pid-change.tsv"
head -n 2 "$samples_file" >"$pid_change_file"
sed 's/\t1234\t/\t5678\t/' <(tail -n 1 "$samples_file") >>"$pid_change_file"
expect_validation_failure "$pid_change_file" "PID changed within the series"

epoch_change_file="$test_root/epoch-change.tsv"
head -n 2 "$samples_file" >"$epoch_change_file"
sed 's/^200\t/100\t/' <(tail -n 1 "$samples_file") >>"$epoch_change_file"
expect_validation_failure "$epoch_change_file" "epochs are not strictly increasing"

relationship_file="$test_root/relationship.tsv"
head -n 1 "$samples_file" >"$relationship_file"
awk -F '\t' 'BEGIN { OFS = "\t" } NR == 2 { $12 = $11 + 1; print }' \
  "$samples_file" >>"$relationship_file"
expect_validation_failure "$relationship_file" "metric relationships"

missing_heap_file="$test_root/missing-heap.smaps"
printf '%s\n' \
  "00400000-00401000 r--p 00000000 00:00 0 /fixture" \
  "Size: 4 kB" \
  "Rss: 4 kB" \
  "Private_Dirty: 0 kB" >"$missing_heap_file"
if "$sampler" __parse "$status_file" "$missing_heap_file" 300 1234 missing \
  >"$test_root/stdout" 2>"$test_root/stderr"; then
  fail "snapshot without a heap unexpectedly passed"
fi
grep -Fq "smaps snapshot is missing the heap metrics" "$test_root/stderr" ||
  fail "snapshot without a heap reported the wrong error"

printf '%s\n' \
  "PASS: process memory details parsed live and fixture data and rejected malformed series."
