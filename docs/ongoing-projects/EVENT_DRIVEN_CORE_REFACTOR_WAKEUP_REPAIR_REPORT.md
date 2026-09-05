# Event Runtime Wakeup Repair Review

Date: 2026-09-05
Branch: refactor/fight-combat-safety

## Outcome

The review identified missing state-change notifications, not a need to restore
the population polling loop. Repairs keep the single timing wheel, concrete NPC
work agendas, and room-local reactions. NPC activity remains independent of
player presence.

## Original Findings

| Finding | Repair | Verification |
| --- | --- | --- |
| Hostility countdown finishes without aggression | Expiry schedules a local reaction | Production-linked countdown regression |
| Scripted hunting never starts | Hunting assignments and cancellation use set_hunting_target | Real mhunt command regression; assignment inventory audited |
| Revealing a stationary player does not provoke aggression | Visibility and relevant affect changes notify room/adjacent observers | Unit regression and live invisible/visible sentinel test |
| Standing does not restore wandering | Position changes synchronize NPC work; the sitting-to-standing command uses that path | Unit regression and live dormant/standing dummy test |
| Background cooldowns never count down | Swindle, entertain, and tribute enroll the target after setting the counter | Mutation-to-admission source trace; maintenance regression suite |
| Abandoned encounters do not expire | Start, reset, and cancellation of encounter timers synchronize maintenance | Production-linked extraction-countdown admission/cancellation regression |
| Passive equipment spells stop on healthy NPCs | Passive equipped spells qualify for maintenance; equip/unequip synchronize it | Equipped-item eligibility and retirement regression |
| Offline charge recovery uses uninitialized stats | Durable records are parsed first and restored after affect_total | Real player-file load with Channel Energy and stats after the event section |
| Relative timers use an outdated dispatched tick | Relative admission/rescheduling and remaining-time queries use the observed clock, bounded below by the dispatched tick | Fake-clock regression without an intervening dispatch |
| Saving overdue events erases remaining charge debt | Live overdue records retain their payload and a minimum one-pulse remaining delay | Overdue multi-use persistence regression |
| Triggered ready actions cannot be cancelled | Keep lifecycle subscriptions and the execution handle until dispatch or cancellation | Cancellation and leave/return regressions; live entry execution |

## Follow-up Audit Repairs

- Restore room reactions for players already present after bootstrap/copyover.
- Wake relevant observers when casting ends, a countdown stun ends, memory is
  added, darkness expires, or daylight changes. Daylight visits connected
  characters, not the global mob population.
- Cover relevant concealment, perception, and incapacitation affect changes.
- Start falling when the final flight/levitation affect is removed, even when
  a healthy NPC no longer qualifies for periodic maintenance.
- Make terror, daze protection, kick/slam, banishing-blade, and eldritch-blast
  counter assignments explicitly synchronize maintenance rather than depending
  on incidental damage or an accompanying affect to do it.
- Remove pending reactions when their eligibility disappears during a sync.
- Charge automatic attack actions even when a killing blow ends encounter
  membership before attack processing returns.
- Cancel object and room domain subscriptions before their owner generation is
  invalidated. Regression coverage checks cleanup runs once for each owner.
- End reactor waits on socket readiness, not only the timer deadline. Both
  drivers retain timeout behavior when no socket is ready.
- Wilderness visual/audio deliveries iterate connected descriptors instead of
  all characters. Visual publication radius uses horizontal map distance;
  source altitude remains available to the visibility/intensity calculation.

## Validation

- Full local make test: 1,091 C tests passed, alongside the architecture,
  admission, deployment, help, and tooling checks run by that target.
- Compiler warnings: none in the final test build.
- Valgrind Memcheck: zero memory-access errors across the C suite. This run
  used leak-check=no; it is not a claim of zero retained allocations or leaks.
  A pre-existing bandit timer assertion was stabilized to measure from the
  invocation that actually starts its timer, rather than a later invocation.
- Local game rebuilt and installed; logged in as the development-owned Aster
  character. No user character or production-server changes were made.
- Live posture test: a seated dummy had no events after eight seconds;
  standing created its autonomous agenda again.
- Live visibility test: a sentinel did not attack invisible Aster; revealing
  Aster started combat and populated initiative without moving rooms.
- Live ready test: loading the matching entrant triggered exactly one resting
  action and left no readied action armed afterward.
- Live copyover completed and post-copyover commands and ready execution worked.
- Event diagnostics showed zero registry mismatches, scheduler failures,
  admission rejections, and overdue work in this local test instance.

Evidence is retained in docs/testing/event-wakeup-2026-09-05. Full local build,
test, and Memcheck logs are also available under /tmp/luminari-event-review-*.
An initial scripted summary command stopped at the game's pager; increasing
the agent character's page length allowed the diagnostic checks to complete.

## Boundaries and Residual Risk

All eleven original findings have a repair. The second audit checked the
related mutation paths described above; no known failing case from that review
is intentionally deferred.

This checkout currently loads a small development world. The live sessions do
not establish full-world throughput or wilderness-scale performance. Wilderness
delivery changes were source-reviewed and built, not exercised against the
production wilderness. Background interactions were not left running in-game
for their entire 100-round cooldown; their enrollment paths were traced and
the underlying maintenance/persistence paths were regression-tested.

The local fixture golem has an undefined default position; the visibility test
explicitly stands it before testing aggression. That world-data issue is not
an event scheduler change. Optional local I3 connection failures are expected.

These tests establish the reviewed behavior, not proof that every possible
script or direct field assignment in the game has been exercised. New callers
must use the state-change APIs and add a dormant-owner regression when they
introduce a new wakeup condition. No CI monitoring is required for this repair.

## In-game Checks

On the development server, using an immortal test character:

1. Load a disposable wandering mob. Inspect eventdebug mob <name> 5, force it
   to sit, wait at least one old mobile interval, and inspect again. Force it
   to stand: its autonomous agenda should return.
2. While invisible, enter an aggressive sentinel's room with nohassle off.
   Remain stationary and become visible. It should react without another entry.
3. Use ready rest on entry <name>, then introduce that entity. Rest should
   execute once; ready should report no action armed. Stand afterward.
4. Use ready rest on entry, then ready cancel before entry. Introducing a mob
   must not execute the cancelled command.
5. Use initiative during combat and eventdebug player <name> 5 for owner events.
   Use eventdebug subscriptions player <name> 5 to inspect a readied listener.

Purge only the disposable test mobs and restore nohassle when finished.
