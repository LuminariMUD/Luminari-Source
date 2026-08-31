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

The main loop still advances `pulse` and calls `heartbeat()` every 100 ms. That
function no longer needs to discover work for the migrated high-cardinality
owners when their scheduled modes are active, but it still has real semantic
responsibilities.

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

### Remaining semantic work

The 100 ms cadence also currently owns or paces:

- scheduler tick time and due-event dispatch eligibility;
- descriptor `WAIT_STATE` countdown;
- deferred character extraction;
- one-second help reload, MSDP, travel, crafting, self-buff, I3, presence, and
  supply-slot work;
- moving-room, zone, idle-password, Avernus room, d20, hunt, weather/time,
  quest, diplomacy, clan, trail, maintenance, persistence, usage, and time-save
  services at their established cadences;
- minute persistence batching and its bounded stepper;
- mud-hour and mud-day work.

Phase 11 must not replace this list with one recurring 100 ms scheduler event.
Approved global work should become named service-owned scheduled events at its
actual cadence. The runtime tick source must become monotonic and independent
of `heartbeat()`, `WAIT_STATE` must consume elapsed ticks or a deadline rather
than loop iterations, and deferred extraction needs an explicit safe-point
queue drain. Only then can the generic pulse and catch-up machinery be removed.

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
3. Convert the residual heartbeat responsibilities into named service events,
   adopt a monotonic runtime tick, and separate deferred safe-point drains from
   time cadence.
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
