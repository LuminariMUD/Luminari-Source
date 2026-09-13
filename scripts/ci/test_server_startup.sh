#!/usr/bin/env bash
# Real-port boot, health endpoint, and graceful shutdown using the installed server.
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
runtime=$(readlink -f "${LUMINARI_TEST_DATA_DIR:?prepare an isolated test runtime first}")
sandbox=$(mktemp -d "${TMPDIR:-/tmp}/luminari-startup.XXXXXX")
export LUMINARI_PROJECT_ROOT="$sandbox"
export TERRAIN_API_PORT=${TERRAIN_API_PORT:-4182}
server_log="$sandbox/server.log"
server_pid=
supervisor_pid=

cleanup()
{
  if [[ -n "$supervisor_pid" ]]; then
    (cd "$sandbox" && MUD_PORT=4100 ./scripts/autorun/autorun.sh stop) >/dev/null 2>&1 || true
    kill "$server_pid" 2>/dev/null || true
    wait "$supervisor_pid" 2>/dev/null || true
  fi
  rm -rf "$sandbox"
}
trap cleanup EXIT

fail()
{
  printf '%s\n' "$*" >&2
  tail -n 100 "$server_log" "$sandbox/launcher.log" 2>/dev/null || true
  exit 1
}

mkdir -p "$sandbox/bin" "$sandbox/scripts/autorun"
cp "$repo_root/scripts/autorun/autorun.sh" "$sandbox/scripts/autorun/autorun.sh"
ln -s "$(readlink -f "$repo_root/bin/luminari")" "$sandbox/bin/luminari"
ln -s "$runtime" "$sandbox/lib"
(
  cd "$sandbox"
  MUD_PORT=4100 MUD_FLAGS="-f lib/etc/config -d lib -o $server_log -q -s" \
    AUTORUN_STATE_INTERVAL=0.2 ./scripts/autorun/autorun.sh foreground
) > "$sandbox/launcher.log" 2>&1 &
supervisor_pid=$!

for _attempt in {1..200}; do
  if [[ -s "$sandbox/.mud.pid" ]]; then
    read -r server_pid < "$sandbox/.mud.pid"
    if kill -0 "$server_pid" 2>/dev/null && nc -z 127.0.0.1 4100 2>/dev/null; then
      break
    fi
  fi
  sleep 0.1
done
[[ -n "$server_pid" ]] && kill -0 "$server_pid" 2>/dev/null && \
  nc -z 127.0.0.1 4100 2>/dev/null || fail 'Server did not accept connections on port 4100'
echo 'Server is accepting connections on port 4100'
LUMINARI_HEALTH_URL=http://127.0.0.1:4182/health LUMINARI_HEALTH_TIMEOUT_SECONDS=20 \
  "$repo_root/scripts/operations/healthcheck.sh" --wait
[[ -f "$server_log" ]] || fail 'Server log was not created'
if grep -qi SYSERR "$server_log"; then
  fail 'Unexpected SYSERR found during startup'
fi

(cd "$sandbox" && MUD_PORT=4100 ./scripts/autorun/autorun.sh stop) >> "$sandbox/launcher.log" 2>&1
for _attempt in {1..50}; do
  if grep -q 'MUD server exited with code 0' "$sandbox/launcher.log" && \
     grep -q 'Autorun terminated gracefully' "$sandbox/launcher.log"; then
    break
  fi
  sleep 0.1
done
grep -q 'MUD server exited with code 0' "$sandbox/launcher.log" ||
  fail 'Server did not exit successfully'
grep -q 'Normal termination of game' "$server_log" ||
  fail 'Server log did not record normal termination'
grep -q 'Autorun terminated gracefully' "$sandbox/launcher.log" ||
  fail 'Supervisor did not complete graceful shutdown'
! kill -0 "$server_pid" 2>/dev/null || fail 'Server is still running'
wait "$supervisor_pid" || fail 'Supervisor exited unsuccessfully'
supervisor_pid=
echo 'Server startup smoke test PASSED'
