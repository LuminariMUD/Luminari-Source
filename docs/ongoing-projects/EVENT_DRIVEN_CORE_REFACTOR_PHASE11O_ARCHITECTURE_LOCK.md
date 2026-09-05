# Event-Driven Core Refactor Phase 11o Architecture Lock

**Date:** 2026-09-01

**Branch:** `event-driven-core-refactor`

**Status:** Implemented and accepted

## Decision

The architecture described by the maintainer is the controlling architecture
and is already the production-default implementation:

- one timing wheel stores all future work;
- an NPC owns at most one autonomous agenda handle;
- the agenda exists only for explicit pending responsibilities;
- its callback dispatches only reason bits whose deadlines are due;
- completion or state loss removes reasons and cancels an empty agenda;
- local movement and combat facts wake bounded room or adjacent-room owners;
- off-screen wandering, patrols, hunts, scripts, recovery, and wars remain
  active without player-proximity gating; and
- the whole-mobile list walk is rollback code, not a normal gameplay service.

The earlier description of the normal implementation as a recurring class-wide
scheduler was incorrect. `RUNTIME_SERVICE_MOBILE_ACTIVITY` is admitted only
when `active_world_enabled()` is false. With default selectors,
`active_world_mobile_agenda` carries an immutable generation-aware owner and
calls `mobile_activity_run_scheduled(ch, due)`, which is bounded to one owner
and one explicit due-reason mask.

## Regression Coverage

`TestActiveWorldDormantPopulationDoesNotCreateScheduledWork` constructs a
production-linked world with 512 loaded dormant sentinel NPCs and one
off-screen wanderer. It proves:

- active owner count is one;
- wander-reason count is one;
- scheduler queue depth is one;
- every dormant NPC has no agenda handle;
- advancing to the deadline executes exactly one mobile callback; and
- removing the wanderer's final reason immediately reduces queue depth to zero.

`scripts/events/test_demand_driven_architecture.sh` is included in the normal
`make test` target. It fails if:

- the normal agenda callback traverses `character_list` or `object_list`;
- it calls a legacy whole-mobile function or reason-blind one-mobile wrapper;
- scheduled activity stops being bounded to one explicit owner;
- the rollback service stops being conditional on active-world disablement; or
- the exact legacy-dispatch inventory grows.

## Runtime-Service Audit

The 14 default runtime services were re-read at their call sites. They remain
singleton state, fixed/indexed world rules, active registries, bounded queues,
or connected-descriptor work. The mobile, character, affected-owner, DG,
vessel, and point-update population scans occur only in selectors that restore
an explicit rollback subsystem.

In particular, "one service per class" is not the gameplay model. Event types
identify callback behavior inside the single scheduler. A naturally global
clock, weather, persistence, or maintenance boundary may own one service event;
an NPC class does not receive a coordinator that scans all members.

## Validation

- `scripts/events/test_demand_driven_architecture.sh`: PASS
- integrated `make test`: PASS, including all 1,048 C tests
- scheduler/libevent, scheduler/`select()`, legacy queue/libevent, and legacy
  queue/`select()` each pass all 1,048 tests
- authoritative `make test-all`: PASS, including 504 world-tool tests, protocol
  tests, process-memory tests, schema/source policy, and immutable installation
- AddressSanitizer, UndefinedBehaviorSanitizer, and leak detection: PASS for all
  1,048 tests
- strict Valgrind: PASS for all 1,048 tests with zero errors and zero definite,
  indirect, or possible leaks
- Build & Test run `33456813692`: PASS for exact source
  `85504e5afec35b073d9d401a32821225354e4fa1`
- Security run `33456854683`: PASS for the same exact source
- existing copied production-world evidence: about 39,000 concrete autonomous
  agendas, 2.6% settled CPU, and no ready backlog, overdue work, late callbacks,
  registry mismatches, or stale-owner outcomes

A fresh immortal session against the retained copied-world server on port 4103
is recorded in
`.ci-runtime/user-test/phase11o-eventdebug-clean.log`. It reported scheduler
mode, all 14 default named services live, 40,881 concrete autonomous agendas
(39,046 wander, 102 patrol, no active hunts), zero ready work, zero overdue
age, zero registry mismatches, and zero stale-owner outcomes. The session
password is redacted from the transcript.

The tranche changes no live gameplay cadence or selector. Its purpose is to
make the accepted architecture unambiguous and mechanically resistant to
regression.
