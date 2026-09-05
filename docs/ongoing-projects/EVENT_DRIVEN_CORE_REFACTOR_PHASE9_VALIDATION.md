# Event-Driven Core Refactor Phase 9 Validation

**Status:** Accepted
**Accepted:** 2026-08-31
**Scope:** Semantic encounter rounds, action economy, intent dispatch, combat help,
and live timing acceptance

## Accepted Contract

Phase 9 makes semantic combat the default gameplay path while retaining the
Phase 8 encounter-owned compatibility cadence as a boot-time rollback only.

- One generation-aware event owns each live encounter.
- One shared round lasts six seconds and resolves every eligible participant.
- Turn order is descending initiative, descending Dexterity, then ascending
  stable runtime ID.
- Participants joining during a callback wait until the next shared round.
  Ordinary joins and encounter merges preserve their not-before eligibility.
- Each participant owns explicit standard, move, swift, and reaction budgets.
  Reactions and once-per-round flags reset once before initiative begins.
  Spending a standard action alone permits only the first automatic attack;
  standard plus move permits the established full attack rotation.
- Staggered combatants couple standard and move availability.
- Perfect Tempo, Deflective Screen, Smash Defense, and Relentless Assault use
  participant round flags rather than independent cooldown events.
- The action queue is a bounded ten-command FIFO. At most one prevalidated
  intent is considered at the start of each turn, and the connection loop does
  not drain semantic combat intents between turns.
- Action cooldowns crossing a combat boundary round conservatively to whole
  semantic turns and are restored to ordinary MUD events when combat ends.
- `LUMINARI_COMBAT_ROUNDS=compatibility` restores the three two-second phases
  without changing encounter ownership. The two round models are exclusive.

## Implementation Surface

The encounter owner and semantic state live in
`src/combat/combat_encounters.c`. The established attack mechanics remain in
`src/combat/fight.c`, exposed through one semantic-round entry point. Action
queries and cooldown admission route through the participant budget while that
participant is managed by semantic combat. Once-per-round perk gates follow the
same participant state.

`eventdebug` reports the selected round mode, active encounters and
participants, shared event count, semantic rounds and turns, intent outcomes,
and action/reaction spends in width-bounded output. Player help is present in
both the database migration and the legacy help file.

## Adversarial Findings

The first passive live fixture exposed an empty initial callback: the encounter
round count advanced, but no participant turn was due. The timing-wheel facade
had scheduled new compatibility-API events relative to the wheel's last
dispatched tick. That internal tick can lag the live game pulse while the wheel
is idle, causing a newly admitted event to wake early.

The facade now converts a relative request to the absolute deadline
`live pulse + delay` before scheduler admission. Callback recurrence remains
relative to the callback's scheduler tick. A scheduler regression advances the
live pulse across an idle gap before admission, and a combat integration
regression proves that both participants receive their first turn exactly six
seconds after such a join.

The final source audit found that reactions and round flags were initially
reset at each participant's turn. A lower-initiative defender could therefore
spend a reaction or once-per-round defense against an earlier attacker, then
receive a fresh budget at its own turn in the same shared round. One validated
pre-initiative preparation pass now resets every active participant before any
turn runs. Owner handles are resolved before that pass touches character
memory. Come and Get Me's deliberate free counterattack now refunds the
encounter-owned budget instead of changing only the legacy mirror counter. An
initiative-order regression triggers both kinds of state before the defender's
turn and proves they remain spent.

An earlier live attempt used an aggressive production golem and was rejected
as acceptance evidence because unrelated mobile AI altered the encounter. The
accepted run uses two passive, uniquely named mobiles in the disposable world.

## Automated Evidence

The post-fix tree passed:

- warning-clean production Autotools build;
- authoritative `make test-all` against the disposable MariaDB runtime: 1,017
  production-linked C tests, 504 world-tool tests, 29 protocol tests, 36 help
  tests, process-memory and character-rename checks, and release installation;
- CMake AddressSanitizer and UndefinedBehaviorSanitizer build with leak
  detection and all 1,017 tests;
- strict child-tracing Valgrind with all 1,017 tests, zero errors, and zero
  definite, indirect, or possible leaks;
- semantic/scheduler/libevent, semantic/legacy-backend/libevent, and
  semantic/scheduler/select syntax boots; and
- encounter compatibility and per-character rollback syntax smokes under the
  scheduler/libevent production combination.

The database help migrations were applied twice and verified idempotently in
the disposable test database and the local development database. Semantic
combat reported one expected entry, four expected keywords, and no keyword
conflicts. The command-help sweep reported five entries, ten keywords, all ten
content checks, no obsolete content, and no conflicts.

Evidence logs are retained under `.ci-runtime/phase9-*` and are intentionally
untracked. The authoritative post-audit logs are:

- `phase9-test-all-postaudit.log`
- `phase9-asan-postaudit.log`
- `phase9-valgrind-postaudit.log`
- `phase9-boot-*-postaudit.log`
- `phase9-live-session-postaudit.log`
- `phase9-live-server.log`

## Live MUD Evidence

The accepted session booted scheduler/libevent with semantic rounds on port
4101, logged in as level-34 Ornir from an isolated copied player file, set an
80-column client, read the combat and action-queue help, and started a passive
dummy-versus-target encounter.

`eventdebug` showed:

| Observation | Result |
|-------------|--------|
| Initial state | 0 encounters, 0 participants, 0 shared events |
| Active fight | 1 encounter, 2 participants, 1 shared event |
| First boundary | 1 semantic round, 2 turns, 4 action spends |
| Second boundary | 2 semantic rounds, 4 turns, 8 action spends |
| Teardown | 0 encounters, 0 participants, 0 shared events |

The harness asserted both help entries, semantic mode, one shared event, the
first and second turn counts, and clean teardown. It used a disposable account
row, disposable player copy, and disposable world prototypes. The real local
account password and player file were not modified. The game, Discord bridge,
and terrain API shut down normally; ports 4101, 8181, and 8182 were closed.

## Coverage Policy

Semantic encounter combat is the product path and receives full behavioral,
gameplay, timing, database, live-MUD, sanitizer, and Valgrind coverage.
Compatibility combat remains only until its removal gate, so it receives
warning-clean compilation, syntax boot, and focused exclusivity/rollback smoke
coverage. New feature work must not duplicate exhaustive behavior testing in a
path scheduled for removal.

The timed-event facade is also temporary, but it remains a shared admission
boundary until all callers migrate to typed scheduler or domain-event APIs.
Correctness defects in that boundary are fixed and regression-tested because
they affect current event-driven gameplay.

## Rollback And Next Phase

Restart with `LUMINARI_COMBAT_ROUNDS=compatibility` to restore encounter-owned
three-phase combat. `LUMINARI_COMBAT_EVENTS=legacy` remains the deeper
per-character rollback. Live switching and conversion of active fights are
unsupported.

Phase 10 is next: define and approve the activity capability, trait,
interruption, progress, and combat-time matrix, then implement the activity
manager and migrate its first independently reviewed command set.
