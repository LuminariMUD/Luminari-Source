#!/usr/bin/env bash

set -euo pipefail

project_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
active_world="$project_root/src/active_world.c"
mob_activity="$project_root/src/mob/mob_act.c"
runtime_services="$project_root/src/comm.c"
agenda_body=$(mktemp)
scheduled_body=$(mktemp)
service_needed_body=$(mktemp)
default_comm=$(mktemp)
trap 'rm -f "$agenda_body" "$scheduled_body" "$service_needed_body" "$default_comm"' EXIT

fail()
{
  echo "demand-driven architecture test: $*" >&2
  exit 1
}

# An anchor that stops matching yields an empty capture, and every policy grep
# below would then fail with a message blaming the source instead of the
# script. Check the capture itself first so a formatting change reports as a
# stale anchor rather than a phantom regression.
require_capture()
{
  [[ -s $1 ]] || fail "could not locate $2 in $3; the capture anchor in this script is stale"
}

# The definition anchors below match the signature wherever it starts on the
# line: clang-format keeps a short signature on one line with its return type
# but wraps a long one onto its own line, and the choice flips as either the
# name or the column limit changes. Requiring a ')' at end of line (rather than
# ');') is what separates the definition from the forward declaration.
awk '
  /active_world_mobile_event\(const struct game_event_context \*context\)[[:space:]]*$/ { capture = 1 }
  capture { print }
  capture && /fixed_initial_deadline\(struct char_data \*ch\)[[:space:]]*$/ { exit }
' "$active_world" >"$agenda_body"

require_capture "$agenda_body" "active_world_mobile_event()" "$active_world"

grep -q 'mobile_activity_run_scheduled(ch, due);' "$agenda_body" ||
  fail "the mobile agenda no longer dispatches only its due reason mask"

if grep -Eq '(^|[^[:alnum:]_])(character_list|object_list|mobile_activity_run_legacy_(cycle|slice)|mobile_activity_run_one)([^[:alnum:]_]|$)' \
    "$agenda_body"; then
  cat "$agenda_body" >&2
  fail "the normal mobile agenda contains a global discovery or legacy dispatch path"
fi

awk '
  /mobile_activity_run_scheduled\(struct char_data \*ch, mobile_work_mask reasons\)[[:space:]]*$/ { capture = 1 }
  capture { print }
  capture && /mobile_activity_reset\(void\)[[:space:]]*$/ { exit }
' "$mob_activity" >"$scheduled_body"

require_capture "$scheduled_body" "mobile_activity_run_scheduled()" "$mob_activity"

grep -q 'run_mobile_activity(ch, 1U, NULL, reasons);' "$scheduled_body" ||
  fail "scheduled mobile activity is no longer bounded to one explicit owner"

awk '
  /runtime_service_needed\(enum runtime_service_kind kind\)[[:space:]]*$/ { capture = 1 }
  capture { print }
  capture && /runtime_service_boundary_delay\(const struct runtime_service \*service\)[[:space:]]*$/ { exit }
' "$runtime_services" >"$service_needed_body"

require_capture "$service_needed_body" "runtime_service_needed()" "$runtime_services"

tr '\n' ' ' <"$service_needed_body" |
  grep -Eq 'case RUNTIME_SERVICE_MOBILE_ACTIVITY:.*LUMINARI_ENABLE_EVENT_ROLLBACK.*return !active_world_enabled\(\);.*return false;' ||
  fail "whole-mobile rollback service is not quarantined behind the rollback build option"

"${CC:-cc}" -E -P -I"$project_root/src" "$runtime_services" >"$default_comm"
if grep -Eq '^[[:space:]]*mobile_activity_run_legacy_(cycle|slice)[[:space:]]*\(' "$default_comm"; then
  fail "the default preprocessed runtime still dispatches whole-mobile rollback work"
fi

legacy_dispatch_count=$(grep -Eoc 'mobile_activity_run_legacy_(cycle|slice)[[:space:]]*\(' \
  "$runtime_services" || true)
if [[ $legacy_dispatch_count -ne 2 ]]; then
  fail "runtime/heartbeat legacy-mobile inventory changed (expected 2, found $legacy_dispatch_count)"
fi

grep -q '"mobile.autonomous.agenda"' "$active_world" ||
  fail "the native concrete-owner agenda type is missing"

echo "demand-driven architecture test: PASS"
