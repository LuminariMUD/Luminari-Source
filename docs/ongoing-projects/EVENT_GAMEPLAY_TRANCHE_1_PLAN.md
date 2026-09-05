# Tranche 1: native-event cleanup and door readiness

Status: Implemented and locally validated; see the
[acceptance record](../testing/EVENT_GAMEPLAY_TRANCHE_1_ACCEPTANCE_2026_09_05.md).
Basis: `refactor/fight-combat-safety` at `2596c9e464d04ddfc4bc9a863315b1ab34de22a8`.

## Outcome

Remove the last unused alternative scheduler source and make successful door
changes observable through the native domain bus. Ship one player-visible
consumer: `ready <command> on door open <direction>`. A player can prepare a
response to a known door opening without repeatedly issuing commands.

This builds on the existing deferred `ready ... on entry` semantics. It does not
implement full tabletop Ready, counterspell interrupts, initiative changes, or
new action budgets. Commands still pass through normal command/action validation.

## Why this slice comes first

The physical runtime rollback has already been retired. Repeating that work
would add no value. The remaining orphan source and stale documentation are
small, concrete closure items. DoorStateChanged is already registered but has
no publisher, making doors a useful end-to-end exercise of authoritative mutation,
scoped facts, lifecycle-safe listeners, and visible gameplay.

Inventory transfer and relocation contracts touch more fundamental lifecycle
paths; casting also needs explicit decision windows. They should follow a proven
fact-to-consumer pattern in separate tranches.

## Ordered implementation steps

### 1. Close source retirement and document the remaining mechanisms

- Remove unused `util/hl_events.c/.h` and the CMake comment advertising it.
- Reconcile both build manifests as required for source deletion. The utility
  is already excluded, so do not invent a replacement build target.
- Extend existing retired-API and physical-scheduler ownership guards over both
  `src/` and `util/`, with explicit implementation boundaries. Prove the guard
  detects an alternate scheduler placed outside `src/`, using an isolated fixture.
- Update MUD_EVENTS.md and ADR 0002 to native-only scheduling, current save
  format/readers, and the two supported I/O drivers. Fix dangling removed-header
  links and obsolete configuration instructions; preserve historical changelogs.
- Inventory active pollers and direct dispatchers with owner, cadence, callers,
  reason retained, and follow-up. Include crafting, self-buffing, travel, supply
  refresh, moving rooms, staff events, DG, special procedures, and quests.
- Explicitly identify within-operation combat reactions, I/O ingress, watchdog,
  archival SQL, and migration readers so none is mistaken for a hidden scheduler.

Deliverable: no orphan timer implementation in either source tree and an honest
inventory; this step does not claim all feature-level polling has been migrated.

### 2. Define and implement the committed door-change contract

Primary surfaces: `domain_event_types.h`, `domain_event_runtime.c/.h`,
`movement/movement_doors.c`, `act.h`, and door utilities in `utils.c`.

- Define an authoritative door mutation API with explicit single-side and paired
  modes. Preserve authored one-way/asymmetric doors; never infer mirroring solely
  from a reverse direction. Validate the reciprocal destination when pairing.
- Keep the existing room/direction/old-state/new-state fact, adding a typed cause
  and optional actor handle if required to distinguish gameplay from reset/edit
  changes. Publish scoped to the affected room.
- Commit all requested sides before notifying. Publish at most one fact per
  changed side for one operation, containing its final flag state. A paired
  operation therefore permits one notification in each adjacent room, not two
  notifications to one room because unlock/open macros ran separately.
- Preserve all lock-strength flags, existing messages, traps, DG vetoes, and
  command costs. Vetoes and unsuccessful operations publish no fact; unchanged
  state publishes no fact. Notifications never veto or undo completed mutation.
- Capture stable handles/state before dispatch and re-resolve after callbacks;
  never keep borrowed room/exit pointers across arbitrary synchronous handlers.
- Bootstrap loading initializes silently. Runtime resets/OLC are classified
  explicitly as administrative causes, excluded from triggering player readiness.
  Replacing/removing an exit invalidates a readiness binding to the old exit.

Deliverable: an event contract whose consumer sees committed, meaningful state.

### 3. Cover live producers, not just the open command

Build a writer inventory before editing; use it as the completion checklist.
Known families already identified:

- Open/close/lock/unlock/pick in `movement/movement_doors.c`.
- Other command and movement helpers in `act.other.c`, `movement/movement.c`.
- Quest helpers in `quest/hlquest.c`.
- DG door field mutation in `dgscript/dg_wldcmd.c`.
- Runtime zone resets in `db.c` and room replacement/edit paths.
- Special procedures, including Neverwinter, Avernus, Darkhold, Lavatubes,
  and Mad Drow, that use macros or write `exit_info` directly.
- Any spell, vessel, or additional special writer found by the full inventory.

Do not instrument every raw macro independently: paired mutations could expose
half-updated state. Object containers share some macros but are not room doors;
preserve container behavior without inventing a room-door fact for a chest.
Account for whole-field flag assignments as well as SET_BIT/REMOVE_BIT calls.

Deliverable: every relevant live door-state writer uses the committed contract
or has a documented initialization/administrative boundary and a test.

### 4. Add door-open readiness using existing native infrastructure

Primary surfaces: `ready_action.c/.h`, domain subscriptions, existing ready tests.

Proposed user syntax:

```text
ready <command> on door open <direction>
ready
ready cancel
```

- Bind to one discoverable local room door, including its destination identity.
  Reject invalid directions, missing/hidden undiscovered exits, and already-open
  doors. Do not disclose the identity of a remote opener.
- Subscribe to that room's DoorStateChanged facts and filter the watched direction
  for the closed-to-open transition. Unlock-only, no-op, another door, reset,
  and editor events do not trigger it.
- Retain one ready action per character, with existing entry syntax supported.
  One matching transition admits at most one `action.ready.execute` callback.
  Extra publications while execution is pending cannot queue another command.
- Execute at the existing one-tick safe boundary, never inside door mutation.
  Revalidate owner, room, exit binding, and currently open state. If the door has
  closed/replaced or the owner moved, consume/cancel the pending action with a
  clear message; do not replay it later.
- Preserve the explicit command text and normal interpreter targeting rules;
  do not auto-target an unseen opener or introduce implicit remote attacks.
- Follow existing action availability and command restrictions. This adds no
  action allowance and does not bypass action cooldowns or combat admission.
- Cover movement, death, extraction, logout, cancellation, re-arming, subscription
  admission failure, scheduler admission failure, copyover, and shutdown.
- Expose listeners through existing `eventdebug subscriptions` and execution
  through `eventdebug`; add no polling loop or separate feature scheduler.
- Update ready help in both the database and `lib/text/help/help.hlp` through the
  established workflow when implementing the feature, plus player examples.

Deliverable: reliable door-triggered commands with predictable cancellation.

### 5. Validate and record acceptance

- Behavioral tests: successful player/NPC/script changes, failed/vetoed operations,
  paired/asymmetric exits, all lock strengths, containers, no-op writes, room
  replacement, resets, OLC, and extraction during notification.
- Ready tests: relevant/unrelated doors, both sides, hidden doors, repeated facts,
  reopen cycles, close-before-execution, exhausted command actions, and lifecycle
  cleanup. Existing entry-ready behavior remains covered.
- Assert zero listeners/native callbacks after cancellation and zero additional
  periodic work for an idle world or an armed-but-untriggered ready action.
- Run all four architecture checks, production-linked `make test` followed by
  `make install`, and the supported CMake/I/O-driver checks. Run targeted memory
  safety checks for subscription cleanup and synchronous reentrancy.
- In an isolated full-world development runtime, demonstrate a player waiting
  inside a room while another character opens the door from outside; exactly one
  command follows. Repeat with a scripted door and copyover cleanup.
- Record event deadline versus actual execution under idle and burst conditions.
  Separate the intentional one-tick delay from scheduler lateness; report p50,
  p95, p99 and maximum with sample counts. Use existing telemetry if sufficient,
  otherwise bounded instrumentation. Do not infer responsiveness from average CPU.

The existing acceptance report leaves an overall performance gate open. This
tranche must report its measurements and any regression explicitly; it cannot
claim unconditional production readiness without an agreed latency limit and
evidence meeting it. A production deployment is not part of this plan.

## Scope boundaries and follow-up

Not included: atomic inventory-transfer facts, movement cause/direction contracts,
ActivityTransitioned topic routing, all-player polling removal, NPC perception,
casting/concentration redesign, full tabletop Ready, combat duration changes,
quest reward rewiring, rest/recovery rebalance, or archival database deletion.

These remain visible in the audit. After this tranche, prioritize migration of
active crafting/self-buffing/transit work and the movement/object/activity
contracts; then build casting and perception features on those foundations.

## Completion and delivery

Five reviewable work areas corresponding to the steps above, with tests alongside
their owning behavior. Stop the tranche at its acceptance report; do not silently
absorb deferred systems or state that every gameplay timer is now owner-driven.
Recheck branch HEAD and worktree before implementation to incorporate intervening
changes without reverting other work.
