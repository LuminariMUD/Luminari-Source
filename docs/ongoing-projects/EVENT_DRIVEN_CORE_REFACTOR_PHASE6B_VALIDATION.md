# Event-Driven Core Refactor Phase 6b Validation

**Status:** Pass
**Date:** 2026-08-30
**Branch:** `event-driven-core-refactor`
**Scope:** Database-backed pub/sub retirement and native sensory domain events

## 1. Specification Audit

| Requirement | Accepted disposition |
|-------------|----------------------|
| Runtime retirement | Removed old initialization, once-per-second queue processing, source/build entries, player and staff commands, and deploy-time schema application. |
| Wilderness metadata | Removed obsolete string topic/handler fields and the old PubSub adapter. Spatial delivery is now a typed subscriber. |
| Rename handling | Removed runtime rename keys and cache invalidation. Isolated schema tests prove preserved rows remain unchanged under their historical name. |
| Database safety | Retained and marked all old table definitions as deprecated archival data. No drop statement or automatic cleanup migration exists. `database info pubsub` reports the complete known footprint. |
| Help and documentation | Removed command references from flat help and replaced system, wilderness, deployment, rename, task, ADR, and getting-started guidance. |
| Native gameplay capability | Added `WorldPhenomenon`, coordinate and room-graph propagation, independent visual/audio ranges, and generation-aware room origins. Meteor Swarm is the first production publisher. |
| Dormant-world cost | Publications perform bounded observer delivery only when a phenomenon occurs. Room mode is capped at eight hops and 256 rooms; coordinate mode visits connected wilderness players, not every room or mobile. |

The archived world-renumber mappings remain intentionally available. They do
not reactivate runtime delivery; they prevent maintenance tooling from
corrupting dormant historical room and zone references.

## 2. Sensory Contract

The gameplay owner publishes one immutable fact after an event occurs. The
subscriber decides who can perceive it:

- coordinate mode applies wilderness distance, elevation, terrain, weather,
  obstruction, intensity, and audio-frequency behavior;
- room mode performs a bounded breadth-first walk for adjacent combat, spell
  impacts, and similar local phenomena;
- visual delivery does not cross a closed door, while sound may;
- `minimum_range` avoids duplicating the source room's ordinary local message;
  and
- borrowed descriptions are neither retained nor logged by the domain bus.

This supports epic spells, airships, dragons, weather, terrain changes,
explosions, and nearby fighting through the same typed boundary. Individual
combat, fireball, vessel, creature, and weather publishers remain owning-system
migrations rather than special cases in the event core.

## 3. Focused Coverage

The production-linked suite now contains 968 tests. The new room-propagation
test proves that sound crosses a closed door while sight does not, visual range
stops at the configured room hop, and an invalidated origin-room generation
delivers nothing. Existing domain tests also verify the ninth type's identity,
payload size, registered production subscriber, borrowed payload, bounded
causality, deterministic order, and runtime lifecycle.

The retirement script proves that no old source, build entry, boot/heartbeat
symbol, interpreter command, wilderness metadata, or public direct Meteor
helper remains, while every archived schema family remains and no PubSub table
drop is introduced.

## 4. Validation Evidence

- Normal Autotools `luminari` and `cutest` build: pass without a new warning.
- Full repository `make test` with isolated runtime/schema fixtures: pass.
- Production-linked 2x2 rollback matrix: 968/968 under scheduler/libevent,
  scheduler/select, legacy/libevent, and legacy/select.
- ASan and UBSan: 968/968 with leak detection and halt-on-error enabled.
- Strict Valgrind: 968/968; zero errors and zero definite, indirect, or
  possible leaks. A local suppression covers five pre-existing `parse_room`
  allocations in the syntax test's deliberate `_exit` child only.
- Fresh CMake Debug build: `luminari` and production-linked `cutest` targets
  configure and link with the new modules.
- Character rename static and isolated MariaDB schema suites: pass, including
  byte/state-equivalent deprecated rows.
- Pub/sub retirement/schema-preservation test: pass.
- Live candidate binary: booted on port 4101; disposable account `Phenonine`
  and character `Phenochar` were created through the real nanny flow, entered
  the world, ran `look` and `score`, and logged out. Follow-up sessions proved
  both `pubsub` and `topics` return the unknown-command response.
- Working-tree whitespace validation: `git diff --check` pass.

Local logs are retained under `.ci-runtime/phase6b-*` and are intentionally
untracked.

## 5. Rollback and Next Tranche

The Phase 6b commit can be reverted while retaining the Phase 6a typed core.
No rollback schema operation is needed because the legacy tables were never
dropped or rewritten.

Phase 7 is next: inventory every remaining heartbeat/global scan, select
high-value systems, and migrate each behind an independently reversible
active/cooling-down/dormant owner boundary. The aim is the original gameplay
architecture: mobs, rooms, effects, resources, and automatic work wake because
something happened or a deadline is due, rather than because the main loop
repeatedly scans the dormant world.
