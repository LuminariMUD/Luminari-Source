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
  fail "legacy runtime admission surface changed; remove dependencies instead of adding one"
fi

# Gameplay APIs describe intent. Pulse terminology is reserved for the physical
# compatibility tick and perfmon's measurement of that tick.
gameplay_pulse_definitions=$(
  rg -n --glob '*.[ch]' \
    '^[[:space:]]*(static[[:space:]]+)?[A-Za-z_][A-Za-z0-9_ *]*[[:space:]]+(pulse_[A-Za-z0-9_]+|[A-Za-z0-9_]+_pulse)[[:space:]]*\(' \
    "$project_root/src" \
    | rg -v '\b(capture_slow_pulse|PERF_log_pulse|PERF_prof_repr_pulse|event_process_compatibility_pulse)[[:space:]]*\(' \
    || true
)
if [[ -n $gameplay_pulse_definitions ]]; then
  printf '%s\n' "$gameplay_pulse_definitions" >&2
  fail "gameplay pulse-named API found; name the callback for its responsibility"
fi

echo "legacy event admission test: PASS"
