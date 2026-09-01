#!/usr/bin/env bash

set -euo pipefail

project_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
active_world="$project_root/src/active_world.c"
mob_activity="$project_root/src/mob/mob_act.c"
runtime_services="$project_root/src/comm.c"
agenda_body=$(mktemp)
scheduled_body=$(mktemp)
service_needed_body=$(mktemp)
trap 'rm -f "$agenda_body" "$scheduled_body" "$service_needed_body"' EXIT

fail()
{
  echo "demand-driven architecture test: $*" >&2
  exit 1
}

awk '
  /^static EVENTFUNC\(active_world_mobile_event\)$/ { capture = 1 }
  capture { print }
  capture && /^static unsigned long fixed_initial_deadline/ { exit }
' "$active_world" >"$agenda_body"

grep -q 'mobile_activity_run_scheduled(ch, due);' "$agenda_body" ||
  fail "the mobile agenda no longer dispatches only its due reason mask"

if grep -Eq '(^|[^[:alnum:]_])(character_list|object_list|mobile_activity_run_legacy_(cycle|slice)|mobile_activity_run_one)([^[:alnum:]_]|$)' \
    "$agenda_body"; then
  cat "$agenda_body" >&2
  fail "the normal mobile agenda contains a global discovery or legacy dispatch path"
fi

awk '
  /^void mobile_activity_run_scheduled\(/ { capture = 1 }
  capture { print }
  capture && /^void mobile_activity_reset\(/ { exit }
' "$mob_activity" >"$scheduled_body"

grep -q 'run_mobile_activity(ch, 1U, NULL, reasons);' "$scheduled_body" ||
  fail "scheduled mobile activity is no longer bounded to one explicit owner"

awk '
  /^static bool runtime_service_needed\(/ { capture = 1 }
  capture { print }
  capture && /^static long runtime_service_boundary_delay\(/ { exit }
' "$runtime_services" >"$service_needed_body"

tr '\n' ' ' <"$service_needed_body" |
  grep -Eq 'case RUNTIME_SERVICE_MOBILE_ACTIVITY:[[:space:]]+return !active_world_enabled\(\);' ||
  fail "whole-mobile rollback service is not exclusively gated behind active-world disablement"

legacy_dispatch_count=$(grep -Eoc 'mobile_activity_run_legacy_(cycle|slice)[[:space:]]*\(' \
  "$runtime_services" || true)
if [[ $legacy_dispatch_count -ne 2 ]]; then
  fail "runtime/heartbeat legacy-mobile inventory changed (expected 2, found $legacy_dispatch_count)"
fi

grep -q '"active_world_mobile_agenda"' "$active_world" ||
  fail "the concrete owner agenda event is missing"

echo "demand-driven architecture test: PASS"
