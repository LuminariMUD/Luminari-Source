#!/usr/bin/env bash

set -euo pipefail

project_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
actual=$(mktemp)
expected=$(mktemp)
trap 'rm -f "$actual" "$expected"' EXIT

fail()
{
  echo "legacy event admission test: $*" >&2
  exit 1
}

collect_counts()
{
  local pattern=$1
  local file
  local count

  while IFS= read -r file; do
    count=$(grep -Eoc -- "$pattern" "$file" || true)
    if (( count > 0 )); then
      printf '%s %d\n' "${file#"$project_root/"}" "$count"
    fi
  done < <(find "$project_root/src" -type f \( -name '*.c' -o -name '*.h' \) \
    ! -path "$project_root/src/dgscript/dg_event.c" \
    ! -path "$project_root/src/dgscript/dg_event.h" \
    ! -path "$project_root/src/dgscript/dg_event_rollback.h" \
    ! -path "$project_root/src/dgscript/dg_event_internal.h" | sort)
}

# New gameplay must use a native owner/service event type. This inventory is a
# burn-down list for already-migrated opaque handles, not an extension point.
collect_counts '(^|[^[:alnum:]_])event_schedule(_[[:alnum:]_]+)?[[:space:]]*\(' >"$actual"
cat >"$expected" <<'EOF'
src/ai_events.c 2
src/dgscript/dg_scripts.c 1
src/mud_event.c 1
EOF

if ! diff -u "$expected" "$actual"; then
  fail "opaque compatibility-adapter producer inventory changed; migrate callers instead of adding one"
fi

# The retained calls must disappear from ordinary translation units. They are
# compiled only for an explicit rollback build or the dedicated parity suite.
for file in src/ai_events.c src/dgscript/dg_scripts.c src/mud_event.c; do
  if "${CC:-cc}" -E -P -I"$project_root/src" "$project_root/$file" |
      grep -Eq '(^|[^[:alnum:]_])event_schedule(_[[:alnum:]_]+)?[[:space:]]*\('; then
    fail "$file exposes compatibility scheduling in the default build"
  fi
done

if printf '#include "dgscript/dg_event.h"\n' |
    "${CC:-cc}" -E -P -I"$project_root/src" -xc - |
    grep -Eq 'EVENTFUNC|event_schedule(_[[:alnum:]_]+)?[[:space:]]*\(|event_handle_(cancel|time|is_live|is_queued)'; then
  fail "the default public timed-event header still exposes the rollback facade"
fi

raw_pattern='(^|[^[:alnum:]_])(event_create(_[[:alnum:]_]+)?|event_cancel|event_time|event_is_queued|cleanup_event_obj|queue_(init|enq|deq|head|key|elmt_key|free))[[:space:]]*\('
if collect_counts "$raw_pattern" | grep -q .; then
  collect_counts "$raw_pattern" >&2
  fail "raw pointer or legacy queue API escaped the private facade"
fi

grep -RIl --include='*.c' --include='*.h' 'dg_event_internal\.h' "$project_root/src" \
  | sed "s|^$project_root/||" >"$actual" || true
cat >"$expected" <<'EOF'
src/dgscript/dg_event.c
EOF
if ! diff -u "$expected" "$actual"; then
  fail "private legacy event header escaped its facade implementation"
fi

legacy_runtime_pattern='EVENT_BACKEND_LEGACY_QUEUE|event_process_compatibility_pulse[[:space:]]*\(|LUMINARI_RUNTIME_SERVICES'
collect_counts "$legacy_runtime_pattern" >"$actual"
cat >"$expected" <<'EOF'
src/comm.c 7
EOF

if ! diff -u "$expected" "$actual"; then
  fail "quarantined legacy runtime inventory changed; remove dependencies instead of adding one"
fi

if "${CC:-cc}" -E -P -I"$project_root/src" "$project_root/src/comm.c" |
    grep -Eq 'LUMINARI_EVENT_BACKEND|LUMINARI_RUNTIME_SERVICES|event_process_compatibility_pulse[[:space:]]*\('; then
  fail "the default main loop still contains a runtime rollback selector or compatibility pulse"
fi

grep -Eq '^option\(LUMINARI_ENABLE_EVENT_ROLLBACK' "$project_root/CMakeLists.txt" ||
  fail "CMake rollback option is missing"
grep -A2 -E '^option\(LUMINARI_ENABLE_EVENT_ROLLBACK' "$project_root/CMakeLists.txt" |
  grep -q 'OFF)' || fail "CMake rollback option is not default-disabled"
grep -q 'enable_event_rollback=no' "$project_root/configure.ac" ||
  fail "Autotools rollback option is not default-disabled"

# Gameplay APIs describe intent. Pulse terminology is reserved for the physical
# compatibility tick and perfmon's measurement of that tick.
gameplay_pulse_definitions=$(
  find "$project_root/src" -type f \( -name '*.c' -o -name '*.h' \) -print0 |
    xargs -0 grep -En -- \
      '^[[:space:]]*(static[[:space:]]+)?[A-Za-z_][A-Za-z0-9_ *]*[[:space:]]+(pulse_[A-Za-z0-9_]+|[A-Za-z0-9_]+_pulse)[[:space:]]*\(' |
    grep -Ev '\b(capture_slow_pulse|PERF_log_pulse|PERF_prof_repr_pulse|event_process_compatibility_pulse)[[:space:]]*\(' \
    || true
)
if [[ -n $gameplay_pulse_definitions ]]; then
  printf '%s\n' "$gameplay_pulse_definitions" >&2
  fail "gameplay pulse-named API found; name the callback for its responsibility"
fi

echo "legacy event admission test: PASS"
