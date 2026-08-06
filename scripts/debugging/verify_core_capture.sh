#!/usr/bin/env bash

set -uo pipefail

self_test=false
if [[ "${1:-}" == "--self-test" ]]; then
  self_test=true
elif [[ $# -ne 0 ]]; then
  printf 'usage: %s [--self-test]\n' "$0" >&2
  exit 1
fi

core_pattern=$(cat /proc/sys/kernel/core_pattern 2>/dev/null || printf 'unavailable')
soft_limit=$(ulimit -Sc 2>/dev/null || printf 'unavailable')
hard_limit=$(ulimit -Hc 2>/dev/null || printf 'unavailable')

printf 'Kernel core pattern: %s\n' "$core_pattern"
printf 'Current core soft limit: %s\n' "$soft_limit"
printf 'Current core hard limit: %s\n' "$hard_limit"
if [[ "$core_pattern" == \|* ]]; then
  printf 'Capture owner: pipe handler %s\n' "${core_pattern#|}"
  if command -v coredumpctl >/dev/null 2>&1; then
    printf 'Autorun retrieval: systemd-coredump via coredumpctl\n'
  else
    printf 'Autorun retrieval: handler-owned; no coredumpctl fallback is installed\n'
  fi
else
  printf 'Capture owner: kernel file output\n'
  printf 'Autorun retrieval: local core/core.* scan in project and lib directories\n'
fi

if [[ "$self_test" != true ]]; then
  exit 0
fi

command -v cc >/dev/null 2>&1 || {
  printf 'SELF_TEST=FAIL: cc is required\n' >&2
  exit 1
}
command -v gdb >/dev/null 2>&1 || {
  printf 'SELF_TEST=FAIL: gdb is required\n' >&2
  exit 1
}

test_root=$(mktemp -d "${TMPDIR:-/tmp}/luminari-core-capture.XXXXXX")
cleanup()
{
  if [[ -d "$test_root" ]] && [[ $(basename "$test_root") == luminari-core-capture.* ]]; then
    rm -rf -- "$test_root"
  fi
}
trap cleanup EXIT

cat > "$test_root/core_probe.c" <<'EOF'
#include <stdlib.h>

int main(void)
{
  abort();
}
EOF
if ! cc -g -Wl,--build-id -o "$test_root/core-probe" "$test_root/core_probe.c"; then
  printf 'SELF_TEST=FAIL: could not compile crash probe\n' >&2
  exit 1
fi

(
  cd "$test_root" || exit 1
  ulimit -c unlimited || exit 1
  exec ./core-probe
) > "$test_root/probe.log" 2>&1 &
probe_pid=$!
wait "$probe_pid"
probe_status=$?
if [[ "$probe_status" -ne 134 ]]; then
  printf 'SELF_TEST=FAIL: probe exited %s instead of 134/SIGABRT\n' "$probe_status" >&2
  exit 1
fi

core_file=""
if [[ "$core_pattern" == \|* ]]; then
  if ! command -v coredumpctl >/dev/null 2>&1; then
    printf 'SELF_TEST=UNVERIFIED: PID %s was delivered to a pipe handler, but no supported retrieval client is installed\n' \
      "$probe_pid" >&2
    exit 2
  fi

  core_file="$test_root/core.$probe_pid"
  for _ in {1..20}; do
    if coredumpctl dump "$probe_pid" --output="$core_file" >/dev/null 2>&1; then
      break
    fi
    sleep 0.25
  done
  if [[ ! -s "$core_file" ]]; then
    printf 'SELF_TEST=FAIL: systemd-coredump did not return PID %s\n' "$probe_pid" >&2
    exit 1
  fi
elif [[ "$core_pattern" == /* ]]; then
  printf 'SELF_TEST=UNVERIFIED: absolute kernel core paths are not probed without an isolated host configuration\n' >&2
  exit 2
else
  core_file=$(find "$test_root" -maxdepth 1 -type f \
    \( -name 'core' -o -name 'core.*' \) -print -quit)
  if [[ -z "$core_file" ]] || [[ ! -s "$core_file" ]]; then
    printf 'SELF_TEST=FAIL: the kernel produced no core in the probe directory\n' >&2
    exit 1
  fi
fi

if ! gdb "$test_root/core-probe" "$core_file" -batch \
  -ex 'thread apply all bt full' > "$test_root/backtrace.txt" 2>&1; then
  printf 'SELF_TEST=FAIL: GDB could not read the captured core\n' >&2
  exit 1
fi
if ! grep -Eq 'abort|raise|SIGABRT' "$test_root/backtrace.txt"; then
  printf 'SELF_TEST=FAIL: captured backtrace does not contain the abort path\n' >&2
  exit 1
fi

probe_build_id=$(readelf -nW "$test_root/core-probe" 2>/dev/null |
  awk '/Build ID:/ {print tolower($NF); exit}')
printf 'SELF_TEST=PASS: PID %s core opened with ELF build ID %s\n' \
  "$probe_pid" "${probe_build_id:-unavailable}"
