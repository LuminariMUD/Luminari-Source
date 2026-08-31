# Event-Driven Core Refactor Phase 11j Validation

**Status:** Accepted 2026-08-31
**Date:** 2026-08-31
**Branch:** `event-driven-core-refactor`
**Scope:** Residual character discovery scans and compatibility admission freeze

## 1. Delivered Slice

The existing lifecycle-owned character event now dispatches each character's
six-second D20 maintenance, thirty-second artificer-device recovery, and
mud-hour timed-quest work at the same cadence as the legacy global loops. The
legacy wrappers remain only behind `LUMINARI_CHARACTER_EVENTS=legacy`; the
default scheduler no longer rediscovers every character for these jobs.

Hunt rotation no longer scans every character to find old hunt targets. Each
new target records the current hunt generation. A target that observes a later
generation through its normal six-second owner callback begins the established
five-minute retirement countdown and then enters deferred extraction safely.

The remaining opaque compatibility-adapter producers are an exact burn-down
inventory. The normal test suite rejects additions to that set, raw pointer or
queue API use, private-header leakage, and new legacy-backend or compatibility-
heartbeat dependencies. Retained rollback implementation is therefore not an
extension point for new gameplay.

`eventdebug` includes a compact character-owner section with mode, registry
health, callback count, and D20/device/quest execution counters. Lines remain
bounded by the command's 80-column default and 120-column hard maximum.

## 2. Gameplay And Ordering

- D20 cooldowns, auras, encounter timers, and hunt retirement still advance
  once per six-second round for every live in-world character, including
  off-screen mobiles.
- Device recovery still processes at most one eligible invention every thirty
  seconds and retains its out-of-combat rule.
- Timed quests still lose one mud-hour unit per mud hour for players admitted
  to the character owner registry.
- Character extraction remains deferred to the reactor safe point. A callback
  stops character work once it has marked an expired hunt target.
- Scheduler mode preserves owner iteration rather than player-proximity
  activation. NPC movement, scripts, and zone wars remain independent active
  simulation.

## 3. Validation Evidence

- Production Autotools build passed.
- Production-linked CuTest passed 1,039/1,039 in all four supported backend and
  driver combinations: scheduler/libevent, scheduler/`select()`, legacy queue/
  libevent, and legacy queue/`select()`.
- Focused coverage proves owner dispatch for D20, device, and quest cadences,
  plus lazy hunt-generation observation and countdown progression.
- `make test-all` passed: 1,039 C tests, 504 world-tool tests with 35 intentional
  skips, 29 protocol tests, 36 help tests with 8 intentional skips, process-
  memory and rename suites, source contracts, and install verification.
- The production-linked 1,039-test binary passed AddressSanitizer and
  UndefinedBehaviorSanitizer with no report.
- Valgrind passed all 1,039 tests with zero errors and no definite, indirect,
  or possible leaks. The 1,721,233 reachable bytes are process-lifetime test
  fixtures and globals rather than lost allocations.
- An isolated scheduler/libevent MUD on port 4103 accepted level-34 Ornir,
  reported one live character owner with zero registry mismatch, advanced
  callbacks, kept `eventdebug` output within 80 columns, and closed the port on
  shutdown. Logs are retained under `.ci-runtime/phase11j/`.

## 4. Remaining Work

Phase 11k replaces DG time-trigger and trail-cleanup room discovery scans with
lifecycle-maintained registries. A separate behavior-preserving intent audit
then renames or decomposes misleading cadence-oriented `pulse_*` functions.
The Survival/Nature rule used by Establish Camp remains deliberately unchanged
pending a human gameplay decision.

The physical legacy timed queue, backend and reactor selectors, compatibility
heartbeat, legacy persistence writer, and archival PubSub schema still require
the specification's stable-release, rollback-independence, maintainer-approval,
and backup/restore evidence before irreversible deletion.
