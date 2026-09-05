# Event-Driven Core Refactor Phase 10 Validation

**Status:** Accepted on 2026-08-31

## Scope

Phase 10 adds one lifecycle-owned primary activity per character and migrates
the existing Establish Camp command as the first reviewed consumer. The
accepted policy is recorded in
[`EVENT_DRIVEN_CORE_REFACTOR_PHASE10_ACTIVITY_MATRIX.md`](EVENT_DRIVEN_CORE_REFACTOR_PHASE10_ACTIVITY_MATRIX.md),
and the source inventory and intentionally deferred candidates are recorded in
[`EVENT_DRIVEN_CORE_REFACTOR_PHASE10_INVENTORY.md`](EVENT_DRIVEN_CORE_REFACTOR_PHASE10_INVENTORY.md).

This tranche does not create a second camp feature and does not rename or split
the Survival/Nature ability slot. It decomposes the existing command's work
while preserving its roll, action cost, group shelter, recovery bonus, and
load-room result.

## Accepted Behavior

| Contract | Result |
|---|---|
| Ownership | At most one primary activity pointer per character; manager list and one owned timer detach on every terminal path |
| Identity | Actor and target use runtime ID plus generation handles; stale or extracted entities do not resolve |
| Progress | Camp has three character-owned steps at two-second wall-clock intervals |
| Claims | Movement, hands, attention, standard action, and move action |
| Traits | Stationary, distracted, hands occupied, and obvious |
| Commands | Information, communication, and `activity` control remain responsive; conflicts are explicitly rejected |
| Movement | Cancels camp and discards progress |
| Damage | Preserves progress and delays the next request by two seconds |
| Combat | Entry pauses and exit resumes preserved wall time; a combat-clock activity can advance only on its semantic turn after consuming existing actions |
| Target loss | Cancels without completing or dereferencing stale state |
| Completion | Existing camp result runs exactly once after terminal teardown |
| Rollback | `LUMINARI_CAMP_ACTIVITY=legacy` restores the prior immediate command for the whole boot |

The `activity` command reports state, progress, claims, traits, clock, and next
step, and provides explicit cancel, pause, and resume controls. Its field
wrapping targets 78 visible columns. `eventdebug` adds a compact Primary
activities block with active/high-water, transition, delay, rejection, and
stale-callback counters.

## Adversarial Audit

The implementation was reviewed against every Phase 10 gate and lifecycle
edge. Fixes made during that audit include:

- honoring every start-in-combat response instead of treating pause as the only
  meaningful policy;
- tearing terminal activities down before callbacks and scalar domain
  publication so observers cannot see half-destroyed manager state;
- guarding callback re-entry by activity identity and current state;
- snapshotting matching activity IDs before extraction can mutate the manager
  list;
- clearing timer-dispatch state when a progress callback pauses itself;
- rechecking and advancing combat-clock work once per semantic turn, not once
  per compatibility phase or twice per turn;
- moving the activity action commitment to the actual semantic-round dispatch;
- retaining `activity` control while casting, hidden, or inside an old crafting
  busy state;
- treating an unset or empty camp selector as managed mode and requiring an
  explicit `legacy`, `off`, or `0` value for rollback;
- applying cancel, pause, delay, and recheck movement policy only after a real
  `CharacterMoved` fact, so a blocked direction does not discard progress;
- adding direct managed and legacy `camp` command tests, rather than validating
  only the manager in isolation; and
- extending the help migration's exact keyword cleanup and verifier to include
  `ACTIVITY`, closing an idempotence gap found before database deployment.

The live audit also exposed the repository's older Survival/Nature naming
ambiguity. History records an intentional 2017 display rename from Survival to
Nature while `ABILITY_SURVIVAL` remained the slot identifier and
`ABILITY_NATURE` became its alias. Phase 10 leaves that shared slot and camp's
existing `ABILITY_SURVIVAL` call unchanged; a separate maintainer decision owns
any global skill-model change.

## Automated Evidence

The accepted tree passed:

- warning-clean production Autotools build;
- authoritative `make test-all`: 1,029 production-linked C tests, 504 world
  tool tests, 29 protocol tests, 36 help tests, process-memory and
  character-rename checks, and release installation;
- CMake AddressSanitizer and UndefinedBehaviorSanitizer Debug build with leak
  detection and all 1,029 C tests;
- strict child-tracing Valgrind with all 1,029 C tests, zero errors, and zero
  definite, indirect, or possible leaks;
- semantic/scheduler/libevent, semantic/legacy-backend/libevent, and
  semantic/scheduler/select syntax boots;
- compatibility-round, per-character-combat, and immediate-camp rollback
  syntax smokes under scheduler/libevent; and
- two idempotent help migration applications plus read-only verification in
  both the disposable test database and local development database.

Database verification reports six maintained entries, eleven exact keywords,
no conflicting command keyword owner, and all twenty-three required content
checks. Evidence logs are retained under `.ci-runtime/phase10/` and
`.ci-runtime/phase10-*`; they are intentionally untracked.

## Live MUD Evidence

An isolated copy of level-34 Ornir and a five-room test world booted the
production scheduler/libevent/semantic combination on temporary port 4102.
Neither the real player file nor the production database record was modified.
The transcript proves:

- database-backed `help activity` and `help camp` show the timed behavior;
- inactive, active, paused, resumed, and completed status is readable at an
  80-column client width;
- `look` remains available while camp work is active;
- `cast` is rejected with the conflicting activity named and a status hint;
- explicit pause preserves `0/3` progress across elapsed wall time, resume
  advances it, and cancel removes it;
- moving east cancels camp before entering the next room;
- uninterrupted work advances at two, four, and six seconds, then applies the
  existing 20-minute Establish Camp affect;
- `eventdebug` reports the active activity and accumulated lifecycle counters
  without exceeding 80 columns; and
- graceful shutdown closes port 4102 with no retained process.

The accepted client and server transcripts are
`.ci-runtime/phase10-live-session.log` and
`.ci-runtime/phase10-live-server.log`.

## Rollback

Restart with `LUMINARI_CAMP_ACTIVITY=legacy` to restore immediate camp
resolution. This does not switch the scheduler, reactor, domain bus, or combat
model. The selector is immutable after activity-manager initialization, and
managed and legacy camp paths cannot both execute for one boot.
