# Event-Driven Core Refactor Phase 11k Validation

**Status:** Implementation and local acceptance complete
**Date:** 2026-09-01
**Branch:** `event-driven-core-refactor`
**Scope:** DG time-trigger and movement-trail discovery registries

## 1. Delivered Slice

DG time triggers now register the script owners that can receive mud-hour time
callbacks. `check_time_triggers()` walks only those mobile, object, and room
owners, preserving the established mobile/object/room order, zone-empty rule,
global-trigger exception, and trigger routines.

Movement leaves a location in an active trail registry. Mud-hour cleanup now
visits only those locations, removes trails at the existing one-game-week
threshold, and removes locations from the registry when their final trail
expires. No movement, tracking, visibility, direction, or expiry rule changed.

DG room owners and ordinary trail locations retain vnums rather than room
pointers. DG dispatch resolves the live room when work is due, and ordinary
room replacement forgets its runtime trail state. Wilderness trail locations
instead retain `(zone vnum, x, y)`: dynamic wilderness vnums are reusable
allocator slots and therefore cannot identify a place. Reassigning a slot to a
different coordinate neither moves nor hides the original trail; another slot
materialized at the original coordinate sees it. Shutdown releases all registry
nodes before room storage.

The existing wilderness k-d tree was considered but is intentionally not the
trail authority. It indexes static rooms, has no individual-delete operation,
and is optimized for spatial queries. Trails need frequent expiry and exact
current-location lookup, for which the coordinate hash has bounded direct
access without pinning a dynamic room or scanning a radius.

## 2. Work Bounds And Diagnostics

- Normal time-trigger work is proportional to scripts with a time trigger, not
  all characters, objects, and rooms.
- Normal trail cleanup is proportional to locations containing trail data, not
  all world rooms or wilderness coordinates.
- Script attach, detach, owner bind, and extraction synchronize registration.
- Trigger callbacks may remove the current or next registered script without
  invalidating iteration.
- `eventdebug` reports DG members, mismatches, visits, and executions plus trail
  locations, mismatches, cleanup runs, visits, and removals.
- Validator scans run only when an immortal explicitly requests diagnostics or
  a test invokes them; they are not part of scheduled gameplay.
- Full-world boot exposed about 61,000 NPCs and about 60,500 recurring mobile
  owners. A subsequent unified-owner run remained bounded at about 63,000 live
  events but produced about 20 million callbacks in eleven minutes and consumed
  about 97% of one core. This rejected the architecture, not just its capacity,
  and made explicit pending work the required correction.

The corrected full-world run admitted about 39,000 autonomous agendas, of
which about 37,000 were off-screen wanderers. Boot placement notifications are
suppressed during explicit bootstrap and one final reconciliation pass derives
the real agendas; this removed 37,000 false room-reaction events.

Two production-only costs found during live validation were also removed:

- scheduled mobiles now use an external generation-keyed registry and an
  immutable event payload, so a stale callback cannot dereference freed
  character memory; extraction also cancels every event indexed to that owner;
- character and object domain handles now resolve through lifecycle-maintained
  hash registries instead of scanning `character_list` or `object_list`;
- an NPC movement fact wakes the moving NPC's relevant arrival behavior, while
  only player or pet arrival wakes other room observers. NPC movement no longer
  fans out a reaction to unrelated NPCs.

## 3. Validation Evidence

- The production-linked CuTest binary passed 1,046/1,046 with the authoritative
  source-world fixture after the demand-driven correction. Coverage includes
  owner-wide cancellation on both backends and handle resolution with both
  global population lists empty.
- Focused tests prove one registered owner per DG category is the only owner
  visited, one active trail location is the only location cleaned, registry
  health is exact, and ordinary room identity survives sorted world-array
  reindexing.
- A focused dynamic-wilderness test records an eastbound trail at `(117,-42)`,
  reuses the same dynamic room slot at `(900,901)`, and proves that a different
  slot materialized at `(117,-42)` sees the original trail and direction.
- A copied production-world snapshot passed syntax boot with 762 indexed zones,
  91,735 rooms, wilderness indexing, local MySQL regions and paths, Perlin
  generators, and resource initialization. It exposed the former 32,768
  character-periodic ceiling and synchronized callback model as insufficient.
- The corrected copied-world live run loaded 91,735 rooms and about 39,000
  concrete NPC agendas. Its settled CPU sample was 2.0%, 3.0%, and 2.8%
  (2.6% average) on one host core. `eventdebug` reported 42,138 live events,
  zero ready events, zero overdue pulses, zero late callbacks, zero registry
  mismatches, and zero stale-owner outcomes. About 193,000 autonomous callbacks
  consumed 0.84 CPU-seconds in total, approximately 4 microseconds each.
- The optimized Autotools production build passed; its only warning is the
  pre-existing suppressed-`scanf` warning in `src/players.c`.
- All four supported backend/driver combinations passed 1,046/1,046 tests:
  scheduler/libevent, scheduler/`select()`, legacy queue/libevent, and legacy
  queue/`select()`.
- Authoritative `make test-all` passed the 1,046 C tests, 504 world-tool tests
  with 35 intentional skips, 29 protocol tests, help and source-policy gates,
  process-memory and rename checks, and immutable install verification.
- AddressSanitizer, UndefinedBehaviorSanitizer, and leak detection passed all
  1,046 tests with syntax-child boot disabled as in CI.
- Strict child-tracing Valgrind passed all 1,046 tests with zero errors and zero
  definite, indirect, or possible leaks.
- The final copied production-world syntax boot passed all 762 zones, 91,735
  rooms, 27,067 mobile prototypes, wilderness indexing, MySQL regions and
  paths, Perlin generators, and resource initialization.

## 4. Remaining Work

Phase 11l completed the behavior-preserving intent audit. Gameplay functions
now describe their responsibility, whole-population paths advertise legacy
rollback, and a source contract rejects new gameplay pulse-named definitions.
Establish Camp's historical Survival/Nature decision remains explicitly
deferred.

The physical legacy queue, backend and reactor selectors, compatibility
heartbeat, legacy persistence writer, and archival PubSub schema remain behind
their frozen boundary. Their deletion still requires the specification's
stable-release, rollback-independence, explicit maintainer-approval, and
backup/restore evidence; this development tranche cannot manufacture those
external facts.
