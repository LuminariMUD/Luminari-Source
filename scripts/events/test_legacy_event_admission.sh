#!/usr/bin/env bash

set -euo pipefail
project_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
if [[ ${1:-} == --check-root ]]; then
  project_root=$2
fi

fail()
{
  echo "retired event API test: $*" >&2
  exit 1
}

# Reject the retired API in source, not just in the default preprocessed build.
pattern='EVENT_BACKEND_LEGACY_QUEUE|LUMINARI_ENABLE_EVENT_ROLLBACK|LUMINARI_EVENT_ROLLBACK_TESTS|DG_EVENT_ROLLBACK_ENABLED|event_schedule(_[[:alnum:]_]+)?[[:space:]]*\(|event_create(_[[:alnum:]_]+)?[[:space:]]*\(|event_process_compatibility_pulse[[:space:]]*\(|mobile_activity_run_legacy_(cycle|slice)[[:space:]]*\('
if find "$project_root/src" "$project_root/util" -type f \( -name '*.c' -o -name '*.h' \) ! -name conf.h -print0 |
    xargs -0 grep -En "$pattern"; then
  fail "retired event architecture has reappeared"
fi
if grep -Eq 'LUMINARI_ENABLE_EVENT_ROLLBACK|LUMINARI_EVENT_ROLLBACK_TESTS|enable-event-rollback' \
    "$project_root/Makefile.am" "$project_root/CMakeLists.txt" "$project_root/configure.ac"; then
  fail "a build switch still enables the retired runtime"
fi
for header in dg_event_internal.h dg_event_rollback.h; do
  [[ ! -e "$project_root/src/dgscript/$header" ]] || fail "retired header $header remains"
done

# Gameplay APIs describe intent. Pulse terminology is reserved for the physical
# compatibility tick and perfmon's measurement of that tick.
gameplay_pulse_definitions=$(
  find "$project_root/src" "$project_root/util" -type f \( -name '*.c' -o -name '*.h' \) -print0 |
    xargs -0 grep -En -- \
      '^[[:space:]]*(static[[:space:]]+)?[A-Za-z_][A-Za-z0-9_ *]*[[:space:]]+(pulse_[A-Za-z0-9_]+|[A-Za-z0-9_]+_pulse)[[:space:]]*\(' |
    grep -Ev '\b(capture_slow_pulse|PERF_log_pulse|PERF_prof_repr_pulse|event_process_compatibility_pulse)[[:space:]]*\(' \
    || true
)
if [[ -n $gameplay_pulse_definitions ]]; then
  printf '%s\n' "$gameplay_pulse_definitions" >&2
  fail "gameplay pulse-named API found; name the callback for its responsibility"
fi

# Exercise the exact guard against an isolated utility tree, including its
# negative control. Never place deliberately invalid source in the checkout.
if [[ ${1:-} != --check-root ]]; then
  fixture=$(mktemp -d)
  trap 'rm -rf "$fixture"' EXIT
  mkdir -p "$fixture/src/dgscript" "$fixture/util"
  touch "$fixture/Makefile.am" "$fixture/CMakeLists.txt" "$fixture/configure.ac"
  bash "$0" --check-root "$fixture" >/dev/null
  printf 'void event_create(void) {}\n' >"$fixture/util/alternate.c"
  if bash "$0" --check-root "$fixture" >"$fixture/result" 2>&1; then
    fail "utility-tree regression fixture escaped the retired API guard"
  fi
  grep -q 'retired event architecture has reappeared' "$fixture/result" ||
    fail "utility-tree fixture failed for an unrelated reason"
fi

echo "retired event API test: PASS"
