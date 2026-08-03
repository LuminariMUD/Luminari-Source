#!/usr/bin/env bash

set -euo pipefail

project_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
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

test_compatibility_links()
{
  [[ -L "$project_root/autorun.sh" ]] ||
    fail "project-root autorun compatibility link is missing"
  [[ -L "$project_root/autorun-watchdog.sh" ]] ||
    fail "project-root watchdog compatibility link is missing"
  [[ -L "$project_root/scripts/deploy.sh" ]] ||
    fail "deployment compatibility link is missing"
  [[ -L "$project_root/scripts/setup.sh" ]] ||
    fail "setup compatibility link is missing"
  [[ -L "$project_root/scripts/move_bin.sh" ]] ||
    fail "binary deployment compatibility link is missing"
  [[ "$(readlink "$project_root/autorun.sh")" == \
      "scripts/autorun/autorun.sh" ]] ||
    fail "project-root autorun compatibility link has the wrong target"
  [[ "$(readlink "$project_root/autorun-watchdog.sh")" == \
      "scripts/autorun/autorun-watchdog.sh" ]] ||
    fail "project-root watchdog compatibility link has the wrong target"
  [[ "$(readlink "$project_root/scripts/deploy.sh")" == \
      "deployment/deploy.sh" ]] ||
    fail "deployment compatibility link has the wrong target"
  [[ "$(readlink "$project_root/scripts/setup.sh")" == \
      "deployment/setup.sh" ]] ||
    fail "setup compatibility link has the wrong target"
  [[ "$(readlink "$project_root/scripts/move_bin.sh")" == \
      "deployment/move_bin.sh" ]] ||
    fail "binary deployment compatibility link has the wrong target"
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
  local unrelated_dir="$test_root/unrelated"
  local unrelated_pid
  local updated=false

  if grep -Eq 'rm[[:space:]].*\.autorun\.lock([[:space:]]|$)' "$project_root/Makefile.am"; then
    fail "a cleanup target removes the persistent autorun lock file"
  fi

  mkdir -p "$daemon_dir/bin" "$daemon_dir/log" "$daemon_dir/dumps" "$unrelated_dir"
  cp "$project_root/scripts/autorun/autorun.sh" "$daemon_dir/autorun.sh"

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

  cat > "$unrelated_dir/autorun.sh" <<'EOF'
#!/usr/bin/env bash
trap 'exit 0' INT TERM
while true; do
  sleep 1
done
EOF
  chmod +x "$unrelated_dir/autorun.sh"
  "$unrelated_dir/autorun.sh" foreground &
  unrelated_pid=$!

  port=$(find_unused_port)
  (
    cd "$daemon_dir"
    AUTORUN_STATE_INTERVAL=1 MUD_PORT="$port" ./autorun.sh
  ) > "$daemon_dir/launcher.log" 2>&1

  wait_for_file "$daemon_dir/.watchdog-saw-state"
  [[ ! -e "$daemon_dir/.watchdog-missing-state" ]] ||
    fail "watchdog ran before the initial autorun state was published"
  wait_for_file "$daemon_dir/.circle.pid"
  wait_for_file "$daemon_dir/.mud.pid"
  wait_for_file "$daemon_dir/.autorun.lock.pid"

  state_pid=$(awk -F= '$1 == "PID" {print $2}' "$daemon_dir/.autorun.state")
  circle_pid=$(sed -n '1p' "$daemon_dir/.circle.pid")
  [[ "$(sed -n '1p' "$daemon_dir/.mud.pid")" == "$circle_pid" ]] ||
    fail "MUD PID file does not identify the fake MUD"
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

  # Simulate upgrading a live pre-PID-file supervisor. The safe fallback must
  # use .autorun.state plus the exact supervisor-child relationship.
  rm -f "$daemon_dir/.mud.pid" "$daemon_dir/.autorun.lock.pid"
  (
    cd "$daemon_dir"
    MUD_PORT="$port" ./autorun.sh stop
  ) > "$daemon_dir/stop.log" 2>&1
  grep -Fq "Using verified supervisor child for legacy MUD shutdown" \
    "$daemon_dir/stop.log" ||
    fail "legacy supervisor shutdown did not use the verified child fallback"
  wait_for_pid_exit "$state_pid"
  wait_for_lock_release "$daemon_dir/.autorun.lock"
  kill -0 "$unrelated_pid" 2>/dev/null ||
    fail "stop command signaled another checkout's autorun process"
  [[ ! -e "$daemon_dir/.mud.pid" ]] ||
    fail "MUD PID file survived managed shutdown"

  inode_after=$(stat -c '%i' "$daemon_dir/.autorun.lock")
  [[ "$inode_before" == "$inode_after" ]] ||
    fail "autorun lock file was unlinked during cleanup"

  printf '%s\n' "$unrelated_pid" > "$daemon_dir/.autorun.lock.pid"
  printf '%s\n' "$unrelated_pid" > "$daemon_dir/.mud.pid"
  (
    cd "$daemon_dir"
    MUD_PORT="$port" ./autorun.sh stop
  ) > "$daemon_dir/mismatched-stop.log" 2>&1
  kill -0 "$unrelated_pid" 2>/dev/null ||
    fail "stop command signaled a PID whose command did not match"
  grep -Fq "Refusing to signal autorun supervisor PID $unrelated_pid" \
    "$daemon_dir/mismatched-stop.log" ||
    fail "mismatched autorun PID was not reported"
  grep -Fq "Refusing to signal MUD server PID $unrelated_pid" \
    "$daemon_dir/mismatched-stop.log" ||
    fail "mismatched MUD PID was not reported"

  kill -TERM "$unrelated_pid"
  wait "$unrelated_pid" 2>/dev/null || true
}

test_watchdog_startup_grace()
{
  local grace_dir="$test_root/grace"
  local watchdog_pid

  mkdir -p "$grace_dir/log"
  cp "$project_root/scripts/autorun/autorun-watchdog.sh" \
    "$grace_dir/autorun-watchdog.sh"

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

  (
    cd "$grace_dir"
    ./autorun-watchdog.sh stop
  ) > "$grace_dir/stop.log" 2>&1
  wait "$watchdog_pid" 2>/dev/null || true
}

test_watchdog_transient_killscript()
{
  local guard_dir="$test_root/killscript-guard"
  local now
  local supervisor_pid
  local watchdog_pid

  mkdir -p "$guard_dir/log"
  cp "$project_root/scripts/autorun/autorun-watchdog.sh" \
    "$guard_dir/autorun-watchdog.sh"

  cat > "$guard_dir/fake-supervisor.sh" <<'EOF'
#!/usr/bin/env bash
trap 'exit 0' INT TERM
while true; do
  sleep 1
done
EOF

  cat > "$guard_dir/autorun.sh" <<'EOF'
#!/usr/bin/env bash
touch "$(dirname "$0")/.unexpected-autorun-restart"
exit 1
EOF
  chmod +x "$guard_dir/autorun.sh" "$guard_dir/fake-supervisor.sh"

  "$guard_dir/fake-supervisor.sh" &
  supervisor_pid=$!
  now=$(date +%s)
  cat > "$guard_dir/.autorun.state" <<EOF
PID=$supervisor_pid
START_TIME=$now
LAST_UPDATE=$now
STATUS=RUNNING
CRASH_COUNT=0
MUD_PORT=
EOF
  touch "$guard_dir/.killscript"

  (
    cd "$guard_dir"
    WATCHDOG_CHECK_INTERVAL=1 \
      WATCHDOG_STARTUP_GRACE_PERIOD=0 \
      WATCHDOG_STATE_STALE_THRESHOLD=30 \
      ./autorun-watchdog.sh loop
  ) > "$guard_dir/loop.log" 2>&1 &
  watchdog_pid=$!

  wait_for_pattern "$guard_dir/log/watchdog.log" \
    ".killscript detected while autorun is healthy - deferring shutdown"
  kill -0 "$watchdog_pid" 2>/dev/null ||
    fail "watchdog exited for the MUD's transient startup killscript"
  [[ ! -e "$guard_dir/.unexpected-autorun-restart" ]] ||
    fail "watchdog restarted a healthy autorun during MUD startup"

  rm -f "$guard_dir/.killscript"
  sleep 1.2
  kill -0 "$watchdog_pid" 2>/dev/null ||
    fail "watchdog did not survive successful MUD startup"

  touch "$guard_dir/.killscript"
  kill -TERM "$supervisor_pid"
  wait "$supervisor_pid" 2>/dev/null || true
  wait_for_pattern "$guard_dir/log/watchdog.log" \
    ".killscript detected after autorun stopped - stopping watchdog"
  wait "$watchdog_pid" 2>/dev/null || true
  [[ ! -e "$guard_dir/.watchdog.pid" ]] ||
    fail "watchdog PID file remained after intentional autorun shutdown"
  [[ ! -e "$guard_dir/.unexpected-autorun-restart" ]] ||
    fail "watchdog restarted autorun despite an intentional killscript"
}

test_watchdog_pid_verification()
{
  local mismatch_dir="$test_root/watchdog-mismatch"
  local unrelated_pid

  mkdir -p "$mismatch_dir/log"
  cp "$project_root/scripts/autorun/autorun-watchdog.sh" \
    "$mismatch_dir/autorun-watchdog.sh"

  cat > "$mismatch_dir/unrelated.sh" <<'EOF'
#!/usr/bin/env bash
trap 'exit 0' INT TERM
while true; do
  sleep 1
done
EOF
  chmod +x "$mismatch_dir/unrelated.sh"
  "$mismatch_dir/unrelated.sh" &
  unrelated_pid=$!
  printf '%s\n' "$unrelated_pid" > "$mismatch_dir/.watchdog.pid"

  (
    cd "$mismatch_dir"
    ./autorun-watchdog.sh stop
  ) > "$mismatch_dir/stop.log" 2>&1

  kill -0 "$unrelated_pid" 2>/dev/null ||
    fail "watchdog stop signaled a PID whose command did not match"
  grep -Fq "Refusing to signal PID $unrelated_pid" "$mismatch_dir/stop.log" ||
    fail "mismatched watchdog PID was not reported"

  kill -TERM "$unrelated_pid"
  wait "$unrelated_pid" 2>/dev/null || true
}

test_watchdog_daemon_recovery()
{
  local recovered_pid
  local recovery_dir="$test_root/recovery"
  local watchdog_pid

  mkdir -p "$recovery_dir/log"
  cp "$project_root/scripts/autorun/autorun-watchdog.sh" \
    "$recovery_dir/autorun-watchdog.sh"

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

test_systemd_unit_installation()
{
  local deploy_dir="$test_root/deploy"
  local fake_bin="$deploy_dir/fake-bin"
  local installed_unit="$deploy_dir/installed/luminari.service"

  mkdir -p "$deploy_dir/scripts/autorun" "$deploy_dir/scripts/deployment" \
    "$fake_bin" "$deploy_dir/installed"
  cp "$project_root/scripts/deployment/deploy.sh" \
    "$deploy_dir/scripts/deployment/deploy.sh"
  cp "$project_root/scripts/autorun/autorun.sh" \
    "$deploy_dir/scripts/autorun/autorun.sh"
  cp "$project_root/luminari.service" "$deploy_dir/luminari.service"
  chmod +x "$deploy_dir/scripts/autorun/autorun.sh" \
    "$deploy_dir/scripts/deployment/deploy.sh"

  cat > "$fake_bin/sudo" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
if [[ "$1" == "install" ]]; then
  /usr/bin/install -m "$3" "$4" "$FAKE_SYSTEMD_UNIT"
  exit 0
fi
exec "$@"
EOF

cat > "$fake_bin/systemctl" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
printf '%s\n' "$*" >> "$FAKE_SYSTEMCTL_LOG"
case "${1:-}" in
  daemon-reload)
    exit 0
    ;;
  show)
    if [[ "$*" == *"--property=MainPID"* ]]; then
      printf '%s\n' "4242"
    else
      printf '%s\n' "$FAKE_PROJECT_ROOT/.autorun.lock.pid"
    fi
    exit 0
    ;;
  is-active)
    [[ "${FAKE_SERVICE_ACTIVE:-true}" == "true" ]]
    ;;
  restart)
    exit 0
    ;;
esac
exit 0
EOF
  chmod +x "$fake_bin/sudo" "$fake_bin/systemctl"

  PATH="$fake_bin:$PATH" \
    FAKE_PROJECT_ROOT="$deploy_dir" \
    FAKE_SYSTEMD_UNIT="$installed_unit" \
    FAKE_SYSTEMCTL_LOG="$deploy_dir/systemctl.log" \
    "$deploy_dir/scripts/deployment/deploy.sh" --install-systemd --restart-service \
    > "$deploy_dir/install.log" 2>&1

  [[ -f "$installed_unit" ]] ||
    fail "systemd installer did not install a unit"
  [[ "$(stat -c '%a' "$installed_unit")" == "644" ]] ||
    fail "installed systemd unit mode is not 0644"
  grep -Fxq "Type=forking" "$installed_unit" ||
    fail "installed systemd unit does not use the autorun forking model"
  grep -Fxq "PIDFile=$deploy_dir/.autorun.lock.pid" "$installed_unit" ||
    fail "installed systemd unit does not publish the autorun PID file"
  grep -Fxq "ExecStart=$deploy_dir/scripts/autorun/autorun.sh" "$installed_unit" ||
    fail "installed systemd unit does not start autorun"
  grep -Fxq "ExecStop=$deploy_dir/scripts/autorun/autorun.sh stop" "$installed_unit" ||
    fail "installed systemd unit does not use the PID-safe stop path"
  if grep -Fq "ExecStart=$deploy_dir/bin/circle" "$installed_unit"; then
    fail "deployment installed the obsolete direct-circle unit"
  fi
  grep -Fxq "restart luminari.service" "$deploy_dir/systemctl.log" ||
    fail "requested service restart was not performed"
  grep -Fq "Systemd service restarted with MainPID 4242" "$deploy_dir/install.log" ||
    fail "systemd installer did not verify the restarted MainPID"

  : > "$deploy_dir/.autorun.lock"
  (
    local guard_status

    exec 9> "$deploy_dir/.autorun.lock"
    flock -n 9
    set +e
    PATH="$fake_bin:$PATH" \
      FAKE_PROJECT_ROOT="$deploy_dir" \
      FAKE_SERVICE_ACTIVE=false \
      FAKE_SYSTEMD_UNIT="$installed_unit" \
      FAKE_SYSTEMCTL_LOG="$deploy_dir/guard-systemctl.log" \
      "$deploy_dir/scripts/deployment/deploy.sh" --install-systemd --restart-service \
      > "$deploy_dir/guard.log" 2>&1
    guard_status=$?
    set -e

    [[ "$guard_status" -ne 0 ]] ||
      fail "installer started systemd while an unmanaged autorun held the lock"
  )
  grep -Fq "An unmanaged autorun currently holds the project lock" \
    "$deploy_dir/guard.log" ||
    fail "installer did not explain the unmanaged autorun conflict"
  if grep -Fxq "restart luminari.service" "$deploy_dir/guard-systemctl.log"; then
    fail "installer restarted systemd despite the unmanaged autorun lock"
  fi

  if command -v systemd-analyze >/dev/null 2>&1; then
    systemd-analyze verify "$installed_unit" \
      > "$deploy_dir/systemd-analyze.log" 2>&1 ||
      fail "installed systemd unit failed systemd-analyze verification"
  fi
}

test_compatibility_links
test_autorun_startup_and_locking
test_watchdog_startup_grace
test_watchdog_transient_killscript
test_watchdog_pid_verification
test_watchdog_daemon_recovery
test_systemd_unit_installation

echo "autorun supervision test: PASS"
