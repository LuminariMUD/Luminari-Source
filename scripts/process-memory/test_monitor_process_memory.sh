#!/usr/bin/env bash
#
# Regression tests for monitor_process_memory.sh
#

set -euo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
monitor="${script_dir}/monitor_process_memory.sh"
sampler="${script_dir}/sample_process_memory_details.sh"

fail()
{
  printf 'test_monitor_process_memory: %s\n' "$*" >&2
  exit 1
}

[[ -x "$monitor" ]] || fail "monitor script is not executable: $monitor"
[[ -x "$sampler" ]] || fail "sampler script is not executable: $sampler"

test_root=$(mktemp -d "${TMPDIR:-/tmp}/memory-monitor-test.XXXXXX")
auto_daemon_pid=""
target_one=""
target_two=""

cleanup()
{
  if [[ -n "$auto_daemon_pid" ]]; then
    kill "$auto_daemon_pid" 2>/dev/null || true
  fi
  if [[ -n "$target_one" ]]; then
    kill "$target_one" 2>/dev/null || true
  fi
  if [[ -n "$target_two" ]]; then
    kill "$target_two" 2>/dev/null || true
  fi
  rm -rf -- "$test_root"
}
trap cleanup EXIT

# Test 1: Help command
"$monitor" --help >"$test_root/help.txt" 2>&1 || true
grep -Fq "Usage:" "$test_root/help.txt" || fail "help text did not contain Usage"
grep -Fq "report" "$test_root/help.txt" || fail "help text did not list report"

# Test 2: Live sample command against current process
live_sample=$("$monitor" sample --pid "$$" --label live_test)
field_count=$(awk -F '\t' '{ print NF }' <<< "$live_sample")
[[ "$field_count" == 13 ]] || fail "live sample does not contain 13 fields: $live_sample"
[[ "$live_sample" == *$'\tlive_test\t'* ]] || fail "live sample did not retain label"

# Test 3: Validate generated sample with sample_process_memory_details.sh --validate
tsv_file="$test_root/samples.tsv"
"$sampler" --header > "$tsv_file"
printf '%s\n' "$live_sample" >> "$tsv_file"
"$sampler" --validate "$tsv_file" || fail "generated sample was rejected by sampler validation"

# Test 4: Report generation on a stable series fixture
stable_fixture="$test_root/stable.tsv"
"$sampler" --header > "$stable_fixture"
printf '1000\tinit\t1234\t200000\t100000\t80000\t20000\t0\t90000\t0\t50000\t45000\t40000\n' >> "$stable_fixture"
printf '1300\tmid\t1234\t200000\t100100\t80050\t20000\t0\t90000\t0\t50000\t45000\t40000\n' >> "$stable_fixture"
printf '1600\tend\t1234\t200000\t100150\t80080\t20000\t0\t90000\t0\t50000\t45000\t40000\n' >> "$stable_fixture"

stable_report="$test_root/stable_report.txt"
"$monitor" report --input "$stable_fixture" > "$stable_report"
grep -Fq "LUMINARI MUD PROCESS MEMORY ANALYSIS" "$stable_report" || fail "report header missing"
grep -Fq "HEALTHY / STABLE" "$stable_report" || fail "stable fixture not assessed as HEALTHY / STABLE"
grep -Fq "Sample Count:             3 samples" "$stable_report" || fail "sample count incorrect in report"

# Test 5: Report generation on a high-growth leak fixture
leak_fixture="$test_root/leak.tsv"
"$sampler" --header > "$leak_fixture"
printf '1000\tinit\t1234\t200000\t100000\t80000\t20000\t0\t90000\t0\t50000\t45000\t40000\n' >> "$leak_fixture"
printf '1300\tmid\t1234\t600000\t500000\t450000\t20000\t0\t90000\t0\t50000\t45000\t40000\n' >> "$leak_fixture"
printf '1600\tend\t1234\t1000000\t900000\t850000\t20000\t0\t90000\t0\t50000\t45000\t40000\n' >> "$leak_fixture"

leak_report="$test_root/leak_report.txt"
"$monitor" report --input "$leak_fixture" > "$leak_report"
grep -Fq "CRITICAL" "$leak_report" || fail "leak fixture not assessed as CRITICAL"
grep -Fq "Memory leak likely" "$leak_report" || fail "leak warning missing"

# Test 6: Report with copyover transition (PID change)
copyover_fixture="$test_root/copyover.tsv"
"$sampler" --header > "$copyover_fixture"
printf '1000\tinit\t1234\t200000\t100000\t80000\t20000\t0\t90000\t0\t50000\t45000\t40000\n' >> "$copyover_fixture"
printf '1300\tpost_copyover\t5678\t200000\t100050\t80020\t20000\t0\t90000\t0\t50000\t45000\t40000\n' >> "$copyover_fixture"

copyover_report="$test_root/copyover_report.txt"
"$monitor" report --input "$copyover_fixture" > "$copyover_report"
grep -Fq "1234 -> 5678" "$copyover_report" || fail "copyover PID transition not reported"
grep -Fq "Copyover Transitions: 1" "$copyover_report" || fail "copyover count not reported"

# Test 7: Daemon start / status / stop lifecycle
env_root="$test_root/sandbox"
mkdir -p "$env_root/log"
output_tsv="$env_root/log/process-memory-timeseries.tsv"

LUMINARI_PROJECT_ROOT="$env_root" "$monitor" start --interval 1 --output "$output_tsv" --pid "$$"
status_out=$(LUMINARI_PROJECT_ROOT="$env_root" "$monitor" status)
grep -Fq "Status: RUNNING" <<< "$status_out" || fail "status did not report RUNNING"

# Allow daemon to collect at least one sample
sleep 2.5
[[ -s "$output_tsv" ]] || fail "daemon did not create or populate TSV output"

LUMINARI_PROJECT_ROOT="$env_root" "$monitor" stop
status_stopped=$(LUMINARI_PROJECT_ROOT="$env_root" "$monitor" status)
grep -Fq "Status: STOPPED" <<< "$status_stopped" || fail "status did not report STOPPED after stop"

# Test 8: Auto-discovery remains unpinned and follows a replacement process
auto_root="$test_root/auto-sandbox"
auto_output="$auto_root/log/process-memory-timeseries.tsv"
fake_bin="$test_root/fake-bin"
pid_source="$test_root/discovered-pid"
mkdir -p "$auto_root/log" "$fake_bin"
printf '%s\n' \
  '#!/usr/bin/env bash' \
  'IFS= read -r discovered_pid < "$MONITOR_TEST_PID_FILE"' \
  'printf "%s\n" "$discovered_pid"' \
  > "$fake_bin/pgrep"
chmod +x "$fake_bin/pgrep"

sleep 30 &
target_one=$!
sleep 30 &
target_two=$!
printf '%s\n' "$target_one" > "$pid_source"

MONITOR_TEST_PID_FILE="$pid_source" PATH="$fake_bin:$PATH" LUMINARI_PROJECT_ROOT="$auto_root" \
  "$monitor" start --interval 1 --output "$auto_output"
IFS= read -r auto_daemon_pid < "$auto_root/.memory-monitor.pid"
daemon_cmdline=$(tr '\0' ' ' < "/proc/$auto_daemon_pid/cmdline")
[[ "$daemon_cmdline" != *" --pid "* ]] || fail "auto-discovered start pinned daemon PID"

sleep 1.5
printf '%s\n' "$target_two" > "$pid_source"
kill "$target_one"
wait "$target_one" 2>/dev/null || true
target_one=""
sleep 2.5

awk -F '\t' -v pid="$target_two" 'NR > 1 && $3 == pid { found = 1 } END { exit !found }' \
  "$auto_output" || fail "daemon did not discover the replacement process"

LUMINARI_PROJECT_ROOT="$auto_root" "$monitor" stop
auto_daemon_pid=""
kill "$target_two"
wait "$target_two" 2>/dev/null || true
target_two=""

printf 'PASS: monitor_process_memory.sh passed all regression tests.\n'
