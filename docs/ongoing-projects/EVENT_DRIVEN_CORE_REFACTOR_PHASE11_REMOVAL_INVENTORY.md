# Event-Driven Core Refactor Phase 11 Removal Inventory

**Audit date:** 2026-08-31

**Audited source:** `0d86b4d2bb80bb7fff62d67dafa763842fcc65a4`

**Branch:** `event-driven-core-refactor`

**Decision:** Phase 11 irreversible removal gate is not satisfied

## 1. Scope and conclusion

Phase 11 removes rollback infrastructure only after one stable release period
on the scheduler/libevent path, confirmation that no rollback dependency
remains, and explicit maintainer approval. The audited commit is present only
on the development branch. It has no containing release tag, is not contained
by `master`, and the repository release workflow runs only for `v*.*.*` tags.
The extensive local and CI acceptance evidence for Phases 1-10 is development
evidence, not a stable release period.

Deleting the queue, `select()` driver, compatibility pulse, or archival PubSub
schema at this point would contradict the controlling specification. This
inventory records the executable dependencies so the deletion can be made in
reviewable slices after the external release gate is satisfied.

The Survival/Nature naming and shared ability-slot decision is explicitly out
of scope. Establish Camp continues to use the existing `ABILITY_SURVIVAL`
symbol and persisted slot until a human gameplay decision is made.

## 2. Retirement gate

| Requirement | Evidence at audited commit | State |
|-------------|----------------------------|-------|
| Scheduler/libevent stable release period | Branch commits and passing CI exist, but no tag, merge, deployment record, or elapsed release period contains this commit | Pending |
| No rollback dependency | Default paths pass, but fallback selectors and branches remain intentionally supported and have not been observed through a release period | Pending |
| Explicit maintainer approval | The maintainer authorized continued tranche implementation, but has not approved overriding the stable-release or database-retirement requirements | Pending |
| PubSub backup and rollback plan | Runtime is retired and tables are ignored; no reviewed production backup, export, retention, or drop migration exists | Pending |

The gate requires all four rows to close. A passing syntax boot, sanitizer run,
Valgrind run, or GitHub workflow does not substitute for operator release
evidence.

## 3. Old timed-event queue

The ten-bucket queue is private to `src/dgscript/dg_event.c` and
`src/dgscript/dg_event.h`. Production defaults to the timing wheel, but
`LUMINARI_EVENT_BACKEND=legacy` and the CuTest selector can still instantiate
the queue. The retained implementation includes:

- `EVENT_BACKEND_LEGACY_QUEUE`, `event_q`, and backend selection;
- `dg_queue`, `q_element`, and ten queue buckets;
- queue admission, cancellation, due dispatch, remaining-time queries, and
  bulk cleanup;
- queue-specific payload ownership and callback-terminal cleanup branches;
- dual-backend parity tests and the backend dimension of CI/boot matrices.

No normal scheduler-mode gameplay caller directly uses `dg_queue`. Once the
release gate closes, this is mechanically removable together with the backend
selector and queue-only tests. It must not be removed while it is still the
documented production rollback.

## 4. Raw compatibility event records

The timing wheel is authoritative underneath the compatibility facade, but the
facade record is still a public gameplay object. A broad source search found
115 `struct event *` occurrences across 23 tracked C/header files. Three in
`src/reactor.c` are libevent's own opaque type. The remaining 112 compatibility
occurrences span 22 files: 40 are in the facade implementation/header and 72
are in 20 external caller files. Stored event pointers remain in:

- DG trigger wait and random-trigger records;
- MUD-event records;
- object automatic-procedure owners;
- character and room affect owners;
- character periodic and active-world owners;
- vessel action and periodic owners;
- encounter round and primary-activity state.

Callers also rely on `event_create*`, `event_cancel()`, `event_time()`,
`event_is_queued()`, `EVENTFUNC` callback recurrence, and cleanup callbacks
that receive the compatibility record. Therefore deleting only the public
structure would break live ownership, cancellation, persistence inspection,
OLC cleanup, player diagnostics, and shutdown.

After the release gate, migrate these callers to opaque scheduler event IDs or
a Luminari-owned opaque handle. Cleanup should receive the owned payload rather
than a public event record. Preserve callback-relative recurrence, exactly-once
cleanup, owner cancellation, remaining-time queries, diagnostics, and MUD
event persistence while each owner category moves. Remove the compatibility
record only after a zero-caller source check and production-linked tests pass.

### Migration progress after the audited baseline

Phase 11a added the opaque-handle registry and API inside the facade, increasing
the raw source count from 115 to 129 without adding an external caller. Phase
11b then migrated encounter-owned combat rounds and primary-activity timers.
At the Phase 11b tree, the same source check reports 119 occurrences across 21
files: three libevent declarations, 54 private facade declarations, and 62
external compatibility occurrences across 18 files. This slice therefore
removes ten raw-pointer occurrences and two external raw-pointer-owning files.

Phase 11c migrates autonomous-mobile, affected character/room, character
periodic, object automatic-procedure, and DG random-trigger owners. The source
inventory is now 99 occurrences across 14 files: three libevent declarations,
54 private facade references, and 42 external compatibility references across
11 files. The remaining external files belong to vessel owners, point-update,
DG waits, MUD events, and their diagnostics/persistence callers.

Phase 11d migrates Greyhawk and fixed-RoL vessel owners plus the vessel and
point-update singleton services. The source inventory is now 86 occurrences
across 10 files: three libevent declarations, 54 private facade references,
and 29 external compatibility references across seven files. Those seven files
are limited to DG waits, MUD events, and MUD-event diagnostics/persistence.

Phase 11e migrates DG trigger waits, including cancellation cleanup,
remaining-time diagnostics, OLC replacement, and room-owner relocation. The
source inventory is now 83 occurrences across eight files: three libevent
declarations, 54 private facade references, and 26 external compatibility
references across five files. All remaining external declarations are in MUD
event storage and its diagnostics/persistence paths. Phase 11f owns that larger
terminal-cleanup and persistence migration.

Phase 11f migrates the complete MUD-event layer, its entity owner lists,
remaining-time consumers, duration and payload replacement, scripted-special
extraction, and durable-record inspection. Owner lists now contain MUD payload
pointers with opaque handles, and a handle-native terminal destructor owns
normal completion, cancellation, and shutdown cleanup. The source inventory is
now 58 `struct event *` occurrences across three files: three libevent
declarations in `src/reactor.c` and 55 private facade references in
`src/dgscript/dg_event.[ch]`. No external compatibility-record declaration
remains. Two one-shot producers in `src/ai_events.c` still call
`event_create()` while ignoring its return; the zero-caller slice owns their
conversion before the raw API can become private or be removed.

Phase 11g decomposes the residual heartbeat into named actual-cadence services,
derives runtime ticks from monotonic elapsed time, gives queued wait state an
exact reactor deadline, moves deferred extraction to an explicit safe point,
and gives persistence batches an owned event. The complete heartbeat remains
available only as runtime-service rollback or to advance the legacy queue.

Phase 11h corrects durable character timing to elapsed wall-clock semantics.
Versioned MUD-event records use schema 2, migrate schema 1 on read, and catch up
multi-use recovery arithmetically. A separate player-file checkpoint advances
persisted six-second counters without replaying player or world update loops.
The timestamp-free legacy event writer remains an explicit rollback limitation.

Combat cadence, semantic rounds, activity timing, autonomous off-screen mobile
simulation, affects, periodic character work, automatic procedures, DG random
triggers, cancellation, recurrence, diagnostics, and boot-time rollback
selection are unchanged. Establish Camp continues to use the existing
`ABILITY_SURVIVAL` behavior; changing its Survival/Nature model remains a
separate human gameplay decision.

`LUMINARI_EVENT_PERSISTENCE_FORMAT=legacy` also retains the Phase 5 legacy
writer while the loader accepts old and versioned records. Removing that writer
requires evidence that rollback no longer depends on it. Removing old-record
read support additionally requires an inventory or conversion of every durable
player event file; it must not be inferred merely from current writer defaults.

## 5. Compatibility heartbeat

The default main loop derives `pulse` from monotonic elapsed time and schedules
the residual cadence groups as named service events. It does not wake or call
`heartbeat()` every 100 ms. `LUMINARI_RUNTIME_SERVICES=legacy` restores the
whole dispatcher, and the legacy timed-event backend still requires the 100 ms
adapter tick to advance its physical queue.

### Migrated rollback scans

The following heartbeat paths are inactive under the defaults and remain only
as boot-time rollback:

- autonomous mobile activity;
- object automatic procedures and DG random triggers;
- character and room affect duration processing;
- character walk-to, PSP, Luminari, bardic, hint, damage-over-time, and misc
  periodic processing;
- vessel periodic work and schedules;
- point updates;
- per-character and compatibility combat rounds.

Their selectors are `LUMINARI_ACTIVE_WORLD`, `LUMINARI_AUTOPROC_EVENTS`,
`LUMINARI_DG_RANDOM_EVENTS`, `LUMINARI_AFFECT_EVENTS`,
`LUMINARI_CHARACTER_EVENTS`, `LUMINARI_VESSEL_EVENTS`,
`LUMINARI_POINT_UPDATE_EVENTS`, `LUMINARI_COMBAT_EVENTS`, and
`LUMINARI_COMBAT_ROUNDS`. `LUMINARI_CAMP_ACTIVITY` independently controls the
single Establish Camp migration. These paths cannot be deleted before their
release rollback period closes.

### Named scheduled work

Twenty-four service definitions cover the former cadence groups at their real
intervals; only definitions required by the selected subsystem modes are
admitted. A normal live boot currently schedules 14. The reactor includes an
exact queued-input or pending-action `WAIT_STATE` expiry, and wait state consumes
monotonic elapsed ticks. Deferred extraction runs at an explicit post-dispatch
safe point. Minute persistence owns a dynamic named batch-step event.

This decomposition removes generic pulse pacing, but does not by itself prove
that every callback is owner-local. The one-second service still invokes
established connected-player and active gameplay routines; minute maintenance
may inspect connected inventories; and mud-hour work includes time triggers,
timed quests, diplomacy, trails, and world maintenance. The final adversarial
audit must classify each internal traversal as legitimate global work, bounded
active-owner work, or a remaining high-cardinality discovery scan.

## 6. `select()` compatibility driver

`src/reactor.c` still implements both `wait_with_select()` and
`wait_with_libevent()`. `LUMINARI_IO_DRIVER` is parsed at boot, reactor state
retains a driver enum, signal ownership is conditional, and tests exercise both
drivers. `src/comm.c` adapts fd sets through the selected driver.

The libevent driver is the default and passed the Phase 3-10 matrix. There is no
stable release containing this branch, so the old driver has not completed the
specified rollback period. After that period, remove the selector, select-only
wait implementation, conditional signal behavior, driver matrix, and select
documentation in one focused change. POSIX fd-set construction in `comm.c` is
not itself proof that the old I/O driver remains; the public reactor boundary
may be simplified separately.

## 7. Retired PubSub data

No PubSub C/header source is tracked under `src/pubsub/`; files visible in a
built tree are ignored object/dependency artifacts. The retirement test proves
that runtime initialization, queue processing, commands, wilderness metadata,
rename hooks, and build-manifest references remain absent.

The tracked `sql/components/pubsub_v3_schema.sql` and its installed archival
copy intentionally preserve deprecated tables. The retirement test rejects any
opportunistic `DROP TABLE`. Completing schema retirement requires a separately
reviewed migration that records:

1. production table and row inventory;
2. backup/export location and integrity check;
3. retention decision for historical messages and subscriptions;
4. foreign-key-safe drop order for legacy and V3 tables, views, and routines;
5. rollback restore procedure and rehearsal evidence;
6. explicit production approval.

No schema change is authorized by this source audit.

## 8. Post-release removal sequence

1. Record the exact tagged and deployed scheduler/libevent release, the period
   it remained active, operator health evidence, fallback-selector usage, and
   maintainer approval.
2. Replace remaining compatibility-record pointers by owner category with
   opaque scheduler identities and payload cleanup, retaining focused parity
   tests after each category.
3. Completed on the development branch: convert residual heartbeat
   responsibilities into named service events, adopt a monotonic runtime tick,
   and separate deferred safe-point drains from time cadence. Retain rollback
   until the release gate closes.
4. Remove migrated gameplay rollback branches and selectors after verifying no
   production rollback dependency, and retire the legacy persistence writer
   only after durable-record compatibility is accounted for.
5. Remove the old queue/backend selector and the `select()` driver/matrix.
6. Execute the separately approved PubSub archival migration only after backup
   and restore rehearsal.
7. Run the complete production, protocol, database, syntax-boot, sanitizer,
   Valgrind, live-MUD, connection/copyover/signal, event-storm, and long-soak
   gates; then publish permanent architecture, testing, operations, migration,
   and developer documentation.

## 9. Reproducible source checks

```sh
git rev-parse HEAD
git branch -r --contains HEAD
git tag --contains HEAD
git grep -n 'struct event \*' -- 'src/*.c' 'src/*.h' 'src/**/*.c' 'src/**/*.h'
git grep -n 'struct event \*' -- 'src/*.c' 'src/*.h' 'src/**/*.c' 'src/**/*.h' \
  | grep -v '^src/reactor\.[ch]:'
rg -n 'LUMINARI_EVENT_BACKEND|LUMINARI_IO_DRIVER|EVENT_BACKEND_LEGACY_QUEUE' src .github unittests
rg -n 'void heartbeat|event_process_compatibility_pulse' src/comm.c src/dgscript
git ls-files src/pubsub sql/components/pubsub_v3_schema.sql
./scripts/events/test_pubsub_retirement.sh
```
