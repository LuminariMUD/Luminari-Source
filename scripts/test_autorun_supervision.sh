#!/usr/bin/env bash

set -euo pipefail

project_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
test_root=$(mktemp -d "${TMPDIR:-/tmp}/luminari-autorun-test.XXXXXX")

fail()
{
  echo "autorun supervision test: $*" >&2
  exit 1
}

terminate_tree()
{
  local child
  local pid=$1

  while read -r child; do
    [[ -n "$child" ]] && terminate_tree "$child"
  done < <(pgrep -P "$pid" 2>/dev/null || true)

  kill -KILL "$pid" 2>/dev/null || true
}

cleanup()
{
  local pid

  set +e
  if [[ -n "${test_root:-}" ]] && [[ -d "$test_root" ]]; then
    while read -r pid; do
      if [[ -n "$pid" ]] && [[ "$pid" != "$$" ]]; then
        terminate_tree "$pid"
      fi
    done < <(pgrep -f -- "$test_root" 2>/dev/null || true)

    if [[ $(basename "$test_root") == luminari-autorun-test.* ]]; then
      rm -rf -- "$test_root"
    fi
  fi
}
trap cleanup EXIT

wait_for_file()
{
  local attempt
  local file=$1

  for ((attempt = 0; attempt < 100; attempt++)); do
    [[ -f "$file" ]] && return 0
    sleep 0.1
  done

  fail "timed out waiting for ${file#"$test_root"/}"
}

wait_for_pattern()
{
  local attempt
  local file=$1
  local pattern=$2

  for ((attempt = 0; attempt < 100; attempt++)); do
    if [[ -f "$file" ]] && grep -Fq -- "$pattern" "$file"; then
      return 0
    fi
    sleep 0.1
  done

  fail "timed out waiting for '$pattern' in ${file#"$test_root"/}"
}

wait_for_pid_exit()
{
  local attempt
  local pid=$1

  for ((attempt = 0; attempt < 100; attempt++)); do
    if ! kill -0 "$pid" 2>/dev/null; then
      return 0
    fi
    sleep 0.1
  done

  fail "process $pid did not exit"
}

wait_for_lock_release()
{
  local attempt
  local lock_file=$1

  for ((attempt = 0; attempt < 100; attempt++)); do
    if flock -n "$lock_file" true; then
      return 0
    fi
    sleep 0.1
  done

  fail "lock was not released"
}

find_unused_port()
{
  local port=$((45000 + $$ % 10000))

  while ss -H -ltn "sport = :$port" 2>/dev/null | grep -q .; do
    port=$((port + 1))
  done

  printf '%s\n' "$port"
}

test_autorun_startup_and_locking()
{
  local circle_pid
  local current_update
  local daemon_dir="$test_root/daemon"
  local heartbeat_attempt
  local initial_update
  local inode_after
  local inode_before
  local port
  local second_status
  local state_pid
  local updated=false

  if grep -Eq 'rm[[:space:]].*\.autorun\.lock([[:space:]]|$)' "$project_root/Makefile.am"; then
    fail "a cleanup target removes the persistent autorun lock file"
  fi

  mkdir -p "$daemon_dir/bin" "$daemon_dir/log" "$daemon_dir/dumps"
  cp "$project_root/autorun.sh" "$daemon_dir/autorun.sh"

  cat > "$daemon_dir/bin/circle" <<'EOF'
#!/usr/bin/env bash
set -u
script_dir=$(cd "$(dirname "$0")/.." && pwd)
printf '%s\n' "$$" > "$script_dir/.circle.pid"
trap 'exit 0' INT TERM
while [[ ! -f "$script_dir/.stop-circle" ]]; do
  sleep 0.1
done
EOF

  cat > "$daemon_dir/autorun-watchdog.sh" <<'EOF'
#!/usr/bin/env bash
set -u
script_dir=$(cd "$(dirname "$0")" && pwd)
if [[ "${1:-}" != "start" ]]; then
  exit 0
fi
if [[ -s "$script_dir/.autorun.state" ]] &&
   grep -Eq '^PID=[0-9]+$' "$script_dir/.autorun.state"; then
  touch "$script_dir/.watchdog-saw-state"
  exit 0
fi
touch "$script_dir/.watchdog-missing-state"
exit 1
EOF
  chmod +x "$daemon_dir/bin/circle" "$daemon_dir/autorun-watchdog.sh"

  port=$(find_unused_port)
  (
    cd "$daemon_dir"
    AUTORUN_STATE_INTERVAL=1 MUD_PORT="$port" ./autorun.sh
  ) > "$daemon_dir/launcher.log" 2>&1

  wait_for_file "$daemon_dir/.watchdog-saw-state"
  [[ ! -e "$daemon_dir/.watchdog-missing-state" ]] ||
    fail "watchdog ran before the initial autorun state was published"
  wait_for_file "$daemon_dir/.circle.pid"
  wait_for_file "$daemon_dir/.autorun.lock.pid"

  state_pid=$(awk -F= '$1 == "PID" {print $2}' "$daemon_dir/.autorun.state")
  circle_pid=$(sed -n '1p' "$daemon_dir/.circle.pid")
  kill -0 "$state_pid" 2>/dev/null || fail "foreground supervisor is not running"
  kill -0 "$circle_pid" 2>/dev/null || fail "fake MUD is not running"

  [[ ! -e "/proc/$state_pid/fd/200" ]] ||
    fail "foreground supervisor inherited the autorun lock descriptor"
  [[ ! -e "/proc/$circle_pid/fd/200" ]] ||
    fail "MUD process inherited the autorun lock descriptor"

  initial_update=$(awk -F= '$1 == "LAST_UPDATE" {print $2}' "$daemon_dir/.autorun.state")
  for ((heartbeat_attempt = 0; heartbeat_attempt < 40; heartbeat_attempt++)); do
    sleep 0.1
    current_update=$(awk -F= '$1 == "LAST_UPDATE" {print $2}' "$daemon_dir/.autorun.state")
    if [[ "$current_update" -gt "$initial_update" ]]; then
      updated=true
      break
    fi
  done
  [[ "$updated" == true ]] || fail "autorun state heartbeat did not advance"

  inode_before=$(stat -c '%i' "$daemon_dir/.autorun.lock")
  if flock -n "$daemon_dir/.autorun.lock" true; then
    fail "autorun lock was not held"
  fi

  set +e
  (
    cd "$daemon_dir"
    MUD_PORT="$port" ./autorun.sh
  ) > "$daemon_dir/second-launcher.log" 2>&1
  second_status=$?
  set -e

  [[ "$second_status" -ne 0 ]] || fail "a second autorun instance was accepted"
  grep -Fq "Another autorun instance is already running" "$daemon_dir/second-launcher.log" ||
    fail "second launcher did not report the lock conflict"

  inode_after=$(stat -c '%i' "$daemon_dir/.autorun.lock")
  [[ "$inode_before" == "$inode_after" ]] ||
    fail "actively locked autorun file was replaced"

  touch "$daemon_dir/.killscript" "$daemon_dir/.stop-circle"
  wait_for_pid_exit "$state_pid"
  wait_for_lock_release "$daemon_dir/.autorun.lock"

  inode_after=$(stat -c '%i' "$daemon_dir/.autorun.lock")
  [[ "$inode_before" == "$inode_after" ]] ||
    fail "autorun lock file was unlinked during cleanup"
}

test_watchdog_startup_grace()
{
  local grace_dir="$test_root/grace"
  local watchdog_pid

  mkdir -p "$grace_dir/log"
  cp "$project_root/autorun-watchdog.sh" "$grace_dir/autorun-watchdog.sh"

  cat > "$grace_dir/autorun.sh" <<'EOF'
#!/usr/bin/env bash
touch "$(dirname "$0")/.autorun-started"
exit 1
EOF
  chmod +x "$grace_dir/autorun.sh"

  (
    cd "$grace_dir"
    WATCHDOG_STARTUP_GRACE_PERIOD=2 ./autorun-watchdog.sh loop
  ) > "$grace_dir/loop.log" 2>&1 &
  watchdog_pid=$!

  wait_for_pattern "$grace_dir/log/watchdog.log" "startup grace period"
  [[ ! -e "$grace_dir/.autorun-started" ]] ||
    fail "watchdog restarted autorun during its startup grace period"

  touch "$grace_dir/.killwatchdog"
  wait "$watchdog_pid" 2>/dev/null || true
}

test_watchdog_daemon_recovery()
{
  local recovered_pid
  local recovery_dir="$test_root/recovery"
  local watchdog_pid

  mkdir -p "$recovery_dir/log"
  cp "$project_root/autorun-watchdog.sh" "$recovery_dir/autorun-watchdog.sh"

  cat > "$recovery_dir/fake-supervisor.sh" <<'EOF'
#!/usr/bin/env bash
trap 'exit 0' INT TERM
while true; do
  sleep 1
done
EOF

  cat > "$recovery_dir/autorun.sh" <<'EOF'
#!/usr/bin/env bash
set -u
script_dir=$(cd "$(dirname "$0")" && pwd)
"$script_dir/fake-supervisor.sh" > /dev/null 2>&1 &
supervisor_pid=$!
now=$(date +%s)
state_tmp="$script_dir/.autorun.state.tmp"
cat > "$state_tmp" <<STATE
PID=$supervisor_pid
START_TIME=$now
LAST_UPDATE=$now
STATUS=RUNNING
CRASH_COUNT=0
MUD_PORT=
STATE
mv -f "$state_tmp" "$script_dir/.autorun.state"
exit 0
EOF
  chmod +x "$recovery_dir/autorun.sh" "$recovery_dir/fake-supervisor.sh"

  (
    cd "$recovery_dir"
    WATCHDOG_AUTORUN_STARTUP_TIMEOUT=5 \
      WATCHDOG_CHECK_INTERVAL=1 \
      WATCHDOG_RESTART_COOLDOWN=0 \
      WATCHDOG_STARTUP_GRACE_PERIOD=0 \
      WATCHDOG_STATE_STALE_THRESHOLD=30 \
      ./autorun-watchdog.sh loop
  ) > "$recovery_dir/loop.log" 2>&1 &
  watchdog_pid=$!

  wait_for_pattern "$recovery_dir/log/watchdog.log" "Autorun restarted successfully"
  recovered_pid=$(awk -F= '$1 == "PID" {print $2}' "$recovery_dir/.autorun.state")
  kill -0 "$recovered_pid" 2>/dev/null || fail "recovered supervisor is not running"

  touch "$recovery_dir/.killwatchdog"
  wait "$watchdog_pid" 2>/dev/null || true
  kill -TERM "$recovered_pid" 2>/dev/null || true
  wait_for_pid_exit "$recovered_pid"
}

test_autorun_startup_and_locking
test_watchdog_startup_grace
test_watchdog_daemon_recovery

echo "autorun supervision test: PASS"
