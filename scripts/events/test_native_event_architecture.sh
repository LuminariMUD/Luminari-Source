#!/usr/bin/env bash

set -euo pipefail

project_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
default_dg_event=$(mktemp)
default_event_runtime=$(mktemp)
default_public_header=$(mktemp)
actual=$(mktemp)
expected=$(mktemp)
trap 'rm -f "$default_dg_event" "$default_event_runtime" "$default_public_header" "$actual" "$expected"' EXIT

fail()
{
  echo "native event architecture test: $*" >&2
  exit 1
}

# The physical timing wheel is private to event_runtime. Gameplay code can
# express only semantic type registration and opaque-handle operations.
scheduler_api_pattern='game_scheduler_(create|shutdown|destroy|register_type|seal_types|types_are_sealed|type_name|type_live_count|current_tick|event_count|schedule|cancel|reschedule|remaining|advance|next_deadline|inspect|get_stats)[[:alnum:]_]*[[:space:]]*\('
direct_scheduler_users=$(
  find "$project_root/src" -type f \( -name '*.c' -o -name '*.h' \) -print0 |
    xargs -0 grep -En -- "$scheduler_api_pattern" |
    grep -Ev '/(game_scheduler|event_runtime)\.[ch]:' || true
)
if [[ -n $direct_scheduler_users ]]; then
  printf '%s\n' "$direct_scheduler_users" >&2
  fail "a production module bypasses the game-facing event runtime"
fi

grep -Rl --include='*.c' 'struct game_scheduler \*' "$project_root/src" |
  sed "s|^$project_root/||" | sort >"$actual"
cat >"$expected" <<'EOF'
src/event_runtime.c
src/game_scheduler.c
EOF
if ! diff -u "$expected" "$actual"; then
  fail "physical scheduler ownership escaped its implementation and runtime boundary"
fi

if [[ $(grep -Eoc 'game_scheduler_create[[:space:]]*\(' "$project_root/src/event_runtime.c") -ne 1 ]]; then
  fail "the game-facing runtime must create exactly one physical timing wheel"
fi

# Compile the public/event implementation surfaces exactly as an ordinary
# product build sees them. Rollback APIs and selectors must disappear, not
# merely remain unused.
printf '#include "dgscript/dg_event.h"\n' |
  "${CC:-cc}" -E -P -I"$project_root/src" -xc - >"$default_public_header"
if grep -Eq 'EVENTFUNC|EVENT_BACKEND_LEGACY_QUEUE|event_schedule(_[[:alnum:]_]+)?[[:space:]]*\(|event_handle_(cancel|time|is_live|is_queued)' \
    "$default_public_header"; then
  fail "the default public header exposes the rollback event facade"
fi
printf '#include "dgscript/dg_scripts.h"\n' |
  "${CC:-cc}" -DLUMINARI_ENABLE_EVENT_ROLLBACK=0 -E -P \
    -I"$project_root/src" -xc - >"$default_public_header"
if grep -Eq 'EVENTFUNC|event_schedule(_[[:alnum:]_]+)?[[:space:]]*\(|event_handle_(cancel|time|is_live|is_queued)' \
    "$default_public_header"; then
  fail "an explicit zero rollback definition exposes the DG rollback facade"
fi

"${CC:-cc}" -E -P -I"$project_root/src" \
  "$project_root/src/dgscript/dg_event.c" >"$default_dg_event"
if grep -Eq 'EVENT_BACKEND_LEGACY_QUEUE|legacy_event|event_schedule(_[[:alnum:]_]+)?[[:space:]]*\(|event_create(_[[:alnum:]_]+)?[[:space:]]*\(|queue_(init|enq|deq|head|key|free)[[:space:]]*\(' \
    "$default_dg_event"; then
  fail "the default timed-event implementation still contains rollback architecture"
fi
"${CC:-cc}" -E -P -I"$project_root/src" \
  "$project_root/src/event_runtime.c" >"$default_event_runtime"
if grep -Eq 'legacy_event|EVENT_BACKEND_LEGACY_QUEUE|event_schedule(_[[:alnum:]_]+)?[[:space:]]*\(' \
    "$default_event_runtime"; then
  fail "the default game-facing runtime still contains rollback adapter identity"
fi
if [[ $(grep -Eoc '^[[:space:]]*status = event_runtime_init\(&config\);' \
    "$default_dg_event") -ne 1 ]]; then
  fail "the default timed-event implementation does not own exactly one runtime"
fi
grep -Fq 'depth_before = event_runtime_event_count();' \
  "$project_root/src/dgscript/dg_event.c" ||
  fail "scheduler dispatch no longer samples queue depth in constant time"
grep -Fq 'depth_after = event_runtime_event_count();' \
  "$project_root/src/dgscript/dg_event.c" ||
  fail "scheduler dispatch no longer records post-dispatch depth in constant time"

# Native producers must retain stable, human-readable semantic identities.
semantic_types=(
  'affected.character.duration|src/affected_owners.c'
  'affected.room.duration|src/affected_owners.c'
  'character.maintenance|src/character_periodic.c'
  'object.automatic_procedure|src/periodic_owners.c'
  'dg.random_trigger|src/periodic_owners.c'
  'dg.trigger.wait|src/dgscript/dg_scripts.c'
  'world.mud_hour_update|src/point_update_periodic.c'
  'vessel.greyhawk.agenda|src/vessels/vessel_periodic.c'
  'vessel.shared.agenda|src/vessels/vessel_periodic.c'
  'vessel.rol.agenda|src/vessels/vessels_rol.c'
  'mobile.autonomous.agenda|src/active_world.c'
  'activity.primary.step|src/activity_manager.c'
  'combat.encounter.round|src/combat/combat_encounters.c'
  'ai.response.delivery|src/ai_events.c'
  'ai.request.retry|src/ai_events.c'
  'service.persistence_batch|src/comm.c'
)
for registration in "${semantic_types[@]}"; do
  name=${registration%%|*}
  file=${registration#*|}
  grep -Fq "\"$name\"" "$project_root/$file" ||
    fail "semantic event type '$name' is missing from $file"
done

grep -Fq 'snprintf(name, capacity, "mud.%03u."' "$project_root/src/mud_event.c" ||
  fail "MUD events no longer expose stable per-ID semantic names"
grep -Fq 'for (id = ePROTOCOLS; id < eMUD_EVENT_COUNT; id++)' \
  "$project_root/src/mud_event.c" ||
  fail "MUD event registration no longer covers the complete usable ID range"

grep -Fq 'event_runtime_seal_types()' "$project_root/src/comm.c" ||
  fail "boot no longer seals the immutable semantic type registry"

# Immortal diagnostics must support direct entity ownership and script-only
# filtering without exposing payloads or exceeding the documented width.
for command in \
  'eventdebug player <name> [limit]' \
  'eventdebug mob <name> [limit]' \
  'eventdebug object <name> [limit]' \
  'eventdebug room <here|vnum> [limit]' \
  'eventdebug scripts <kind> <target> [limit]'; do
  grep -Fq "$command" "$project_root/src/event_debug.c" ||
    fail "eventdebug help is missing '$command'"
done
grep -Fq 'filter.type_contains = "dg.";' "$project_root/src/event_debug.c" ||
  fail "eventdebug script filtering no longer selects DG semantic types"
grep -Fq 'filter->owner_generation_set = false;' "$project_root/src/event_debug.c" ||
  fail "entity filtering no longer spans all live generations for the selected owner"
grep -Fq 'Payloads: redacted' "$project_root/src/event_debug.c" ||
  fail "eventdebug payload redaction is missing"

# These linked CuTests supply runtime evidence for the source constraints above.
for test_name in \
  Test_event_runtime_profiles_native_semantic_callbacks \
  Test_event_runtime_owner_cancel_invalidates_handle_and_cleans_once \
  TestActiveWorldDormantPopulationDoesNotCreateScheduledWork \
  Test_primary_activity_scheduler_registers_timer_when_camp_is_unmanaged \
  Test_event_debug_registry_is_backend_neutral_filterable_and_width_bounded; do
  grep -REq --include='*.c' "void ${test_name}[[:space:]]*\(" \
    "$project_root/unittests/CuTest" ||
    fail "required production-linked regression '$test_name' is missing"
done

echo "native event architecture test: PASS"
