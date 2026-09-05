# Full-World Event Core Acceptance

Date: 2026-09-05
Branch: refactor/fight-combat-safety
Status: Acceptance report complete; functional checks pass, performance approval qualified.

Scope update: the maintainer authorized physical loop-rollback retirement
during this tranche and confirmed that older-binary compatibility is not a
requirement. Acceptance was repeated against that native-only revision.
Earlier measurements below are explicitly pre-retirement checkpoints.

## Native-Only Retirement

Removed the physical legacy queue and public scheduling facade, rollback build
flags, heartbeat gameplay body, inactive rollback service definitions, and
population-loop wrappers for mobile, affect, regeneration, bard, walk-to,
point-update, device, and random-script work. The legacy event save writer is
also removed; existing saves remain readable for migration.

Native event ordering, owner cancellation, script replacement, diagnostics,
and AI ingress tests remain. Obsolete queue parity/cursor tests were retired;
the suite now contains 1,081 tests instead of 1,094. The CI matrix retains both
I/O drivers but no longer pretends to select a legacy timing backend.
Focused test-only service controls remain for fixture isolation and failure
injection; they are not runtime configuration switches.

This report uses migration and recovery guidance, not a promise to
run new character saves through an older executable. No archival database
data has been deleted.

## Isolation

The acceptance instance uses a private user/network namespace, the retrieved
world archive, copied runtime files, and a separate local database named
luminari_phase3_test. Its port 4103 is private to that namespace. The ordinary
development listener on port 4101 is unchanged. No production connection or
user-account edits are part of this tranche. Gameplay uses the copied Aster
agent character.

Private runtime data and raw transcripts are in
.ci-runtime/acceptance-20260905. Credentials and copied world/player data must
not be committed. The archive checksum is in evidence/world-sha256.txt.

The world requires diagonal_dirs=1. The first boot rejected a diagonal exit
with the small development world's defaults; enabling the setting in the
isolated configuration corrected that setup failure.

The copied runtime's empty social-message fixture also logs a zero-size
allocation warning during load_socials in db.c. Boot continues; this is not
an event failure, and social-command content is not certified by this run.

## Pre-Retirement Gameplay Evidence

| Check | Observed result | Evidence under the private runtime |
| --- | --- | --- |
| Full-world boot | 762 zones, 91,735 rooms, 27,067 mobile prototypes | syslog |
| Initial scheduler health | 43,236 live events; 38,993 mobile agendas; zero overdue work, failed callbacks, admission rejections, or registry mismatch | evidence/baseline-session.txt |
| Offscreen combat | Two disposable mobs continued fighting while Aster was in another zone for 25 seconds; health fell from 500 to 480/482; encounter timer remained scheduled | evidence/offscreen-combat-controlled.txt |
| Offscreen wandering | The same mob ID 1000346 moved from room 27501 to 27502 without a movement command, and later from 27503 to 27512 while Aster was elsewhere | evidence/trails-and-wander.txt; evidence/soak-end-and-wander-start.txt; evidence/idle-end-cooldown-start.txt |
| Script inspection | Puff's filtered and unfiltered views both selected the identical dg.random_trigger event | evidence/wander-script-wilderness.txt |
| Dynamic wilderness | Movement entered dynamically allocated room 1004001 at coordinate (0,1) | evidence/wander-script-wilderness.txt |
| Trail admission | Walking a coordinate loop with nohassle disabled left three trail locations and zero registry mismatch | evidence/trails-and-wander.txt |
| Real cooldown admission | Treat Injury admitted a roughly 150-second durable timer, followed by a clean logout | evidence/idle-end-cooldown-start.txt |
| Full-world copyover | Connection survived the 41-second check; the cooldown continued from 113.2 to 75.3 seconds; a transient readied action was cleared | evidence/full-world-copyover.txt |
| Offline cooldown expiry | Login after expiry showed no Treat Injury event; the ability could be used again and created a fresh 149.8-second timer | evidence/offline-cooldown-recovery.txt |
| Distant wilderness observer | Mirel at (1,1) received both visual descriptions and audio from the west when Aster cast at (0,1); the domain handler recorded two publications | evidence/observer-distant.txt; evidence/spatial-distant-publisher.txt |
| DG wait and extraction | A greet trigger scheduled dg.trigger.wait, delivered its four spoken lines, and retired; a second greeting followed by immediate mob extraction produced no delayed dialogue | evidence/dg-dialogue-and-extraction.txt |
| Autonomous offscreen initiation | An isolated GLOBAL random script waited five seconds, then started a fight after Aster left the zone; both mobs remained fighting at 488/500 and 483/500 HP on return | evidence/autonomous-war-final.txt |

The first uncontrolled combat attempt was inconclusive because the wandering
target had already left. The controlled test used a room without exits. Random
wandering is probabilistic: a short observation with no movement is not itself
a failed wakeup.

The first observer attempt remained in the caster's room because an immortal
cannot force an equal-level immortal to move. It is not distant-delivery
evidence. The successful attempt had Mirel move using its own connection.
Mirel was an existing Agentlab-owned test character and was advanced only in
the isolated copy. The initial DG attempt was suppressed by nohassle, as
designed; the successful test disabled nohassle and restored it afterward.

## Pre-Retirement Build And Merge Preparation

- The branch builds and passes all 1,091 production-linked tests after applying
  the repository's pinned clang-format 18.1.8 to five files changed by the
  preceding wakeup repair. These changes are formatting only.
- The single CI inspection found Integration and one Build & Test run passed,
  Code Quality failed on formatting, and other runs were still in progress.
  No pipeline waiting or repeated monitoring was performed.
- Fetched origin/master has 32 commits absent from the branch; the branch has
  133 commits absent from master. The proposed merge tree was computed without
  changing either branch: 23fd5fdd2597998bb1fee83cf69f1408117c959b.
- The proposed merge builds without compiler warnings. Its 19 CTest targets
  initially had one failure because the temporary checkout lacked runtime
  data/database configuration. After supplying separate copied fixture data
  and the acceptance database configuration, the production-cutest target
  passed. The other 18 targets passed in the initial run.
- CMake's tests require BUILD_TESTS=ON and a source-local generated conf.h for
  inherited relative test includes. Those requirements were supplied only in
  the temporary merge checkout. No test was skipped to obtain a passing result.
- A full Valgrind run of the proposed merge passed all 1,091 tests; all 33
  process logs reported zero errors and zero definite/indirect leaks. The
  initial direct invocation omitted CTest's required source-inventory fixture
  environment, causing one assertion and a CuTest failure-message allocation
  leak. Repeating with the exact CTest fixture settings passed without a code
  change or suppression.

Build logs: /tmp/luminari-acceptance-build.log,
/tmp/luminari-acceptance-tests.log, /tmp/luminari-acceptance-merge.log,
/tmp/luminari-acceptance-merge-ctest.log, and
/tmp/luminari-acceptance-merge-cutest-retry.log.
Valgrind evidence is /tmp/luminari-acceptance-merge-valgrind-final-tests.log
and /tmp/luminari-acceptance-merge-valgrind-final.*.log.

## Pre-Retirement Performance

The five-minute pidstat run includes gameplay commands and concurrent build
activity. It averaged 7.21% of one CPU core. Its late-callback count increased,
but later remained unchanged at 82,434 during a quieter interval. Queue depth
did not accumulate: later snapshots still showed zero overdue work and no
failed callbacks or admission rejections. These observations do not yet prove
a latency bound or absence of memory growth.

The second process sample overlaps the start of copyover in its final sample;
its aggregate must not be presented as a clean idle benchmark. Preserve the
raw evidence and obtain uncontaminated measurements before deciding readiness.

A subsequent two-minute sample (evidence/clean-idle-process.txt) contained 24
five-second samples with identical resident memory, 1,528,876 KiB, and averaged
4.59% of one CPU core. No copyover or gameplay commands interrupted that run.
This is bounded idle evidence, not a long-duration leak guarantee or a new
controlled comparison with the retired implementation.

## Acceptance Scope

Equipped-stat and staggered multi-use recovery are covered by the regression
below. Pending-action cancellation, the retired-loop matrix, and wilderness
identity/lifecycle checks are covered by the current production-linked suite.
The final native-only results below supersede earlier build counts.

No merge or push is authorized by this acceptance goal.

## Acceptance Fix: Equipped Cooldown Cadence

A new regression demonstrated an additional offline recovery error. With
effective Charisma 20, Channel Energy has eight daily uses and a 225-second
recovery interval. Loading before equipment restoration saw base Charisma 10,
three uses, and a 600-second interval. Given three spent uses, one second to
the next recovery, and 227 elapsed seconds, the old loader retained two spent
uses instead of one.

The fix captures the effective repeat interval before saving temporarily
removes equipment and affects, and persists it in Evn2 container format 2.
Offline recovery uses that saved cadence. Offline administrative resaves
preserve it; subsequent live callbacks resume calculating from current stats.
Format 1 remains readable, with its previous fallback because those files do
not contain the historical equipped interval.

The pre-retirement production-linked suite passed 1,094 tests. Added coverage includes
the demonstrated recovery discrepancy, the actual format-2 file loader, the
actual save path before unequipping, offline resaves, and invalid intervals.
The regression failed before the fix. Logs are
/tmp/luminari-acceptance-equipped-regression.log and
/tmp/luminari-acceptance-cadence-validation.log.

## Retired-Loop Acceptance Matrix

This supplements the historical Phase 7 inventory, whose intermediate status
labels are not a description of the final branch. Native builds exclude the
old heartbeat gameplay body and rollback population services. Named global
services remain for genuinely shared world/connection work.

| Old responsibility | Native admission/deadline | Acceptance coverage |
| --- | --- | --- |
| Mobile thinking | Concrete wander, patrol, hunt, special, echo, recovery and local-reaction work in active_world.c | Full-world offscreen wandering; GLOBAL script starts offscreen fight; TestIdleNpcPeriodicWorkIsSeparateFromAutonomousAgenda |
| Hostility, scavenging, hunting | State mutation and room-local reaction admission | TestActiveWorldReactionsAndScavengingAreDemandDriven includes real mhunt assignment; hunt target generation invalidation test |
| Posture and perception wakeups | Position, visibility, affect and room changes resynchronize relevant owners | Prior same-branch live posture/visibility logs in event-wakeup-2026-09-05; current production-linked regressions rerun |
| Resource regeneration | Eligible owner starts recovery and retires when full | TestActiveWorldResourceRecoveryWakesAndRetiresOneOwner |
| Walk, bard, counters, hazards and passive equipment | character_periodic nearest eligible deadline and state-change enrollment | Nearest-deadline, late-callback, performing-NPC, mixed-work, typed-movement and free-character tests |
| Combat rounds and automatic actions | Encounter-owned event and membership lifecycle | Live offscreen initiation/continuation; encounter and action-spending regression suite |
| DG random, waits and time | Script attach/lifecycle owners; time-trigger registry | Live random event inspection and delayed dialogue; extraction cancels pending wait; registry checks |
| Character/room affects and object auto-procs | periodic_owners lifecycle events | TestPeriodicOwnersScheduleEveryEligibleOwnerAndCancelLifecycle |
| Mud-hour characters and decaying objects | point_update_periodic active registries | Lifecycle and TestPointUpdateObjectDecayRemainsExtractionSafe |
| Vessels and movement | Per-loaded-hull owners, shared fleet policy service | Owner scheduling, capacity refill and cancellation tests; live builder sailing, scheduled waypoint completion, and extraction cancellation below |
| Zones, clock/weather, auctions and persistence | Named services in comm.c; incremental persistence batch | Full-world resets and service diagnostics; architecture and runtime-service regressions |
| Trails | Coordinate-keyed records with active cleanup | Live three-location admission; TestWildernessTrailIdentitySurvivesDynamicRoomReuse and implicit-coordinate/cleanup tests |
| Readied entry actions | Scoped room subscription followed by deferred safe execution | Live entry/copyover in preceding repair; current test covers filtering, movement, death, explicit cancellation, queued cancellation and leave/return |

The matrix is behavioral coverage, not a claim that every authored zone script
or every long-duration economic timer was manually observed. Future direct
state assignments still require a wakeup API and a dormant-owner regression.

## Gameplay Boundaries

- Footprints are recorded by wilderness coordinate and survive dynamic-room
  slot reuse in production-linked tests. The current named track command uses
  live-target pathfinding; there is not yet a current-room footprint display.
- Distant wilderness visuals/audio were delivered between two live agents.
  Adjacent ordinary-room propagation is tested as a domain capability, but
  existing production publishers do not yet request that propagation mode.
  This is not a claim that every fireball now echoes into adjoining rooms.
- Normal autonomous movement and GLOBAL scripts do not require players in the
  zone. Non-GLOBAL DG random scripts retain their authored empty-zone policy.
- The offscreen duel trigger is a disposable isolated-world fixture, not a
  shipped world change or a claim to have exercised every authored war zone.

## Final Native-Only Verification

- Autotools `make -j8 test` and `make install` passed, including 1,081
  production-linked tests. A final formatting-only adjustment was followed
  by another successful full test/install. Logs:
  /tmp/luminari-native-only-final-format-tests.log and
  /tmp/luminari-native-only-final-format-install.log.
- The isolated proposed merge, with acceptance fixes applied on top of the
  computed merge tree, passed all 19 CTest targets. Two stale documentation
  references exposed by master-side documentation checks were corrected to
  proc_d20_round_one, and the corresponding generated guide was refreshed.
  Log: /tmp/luminari-acceptance-native-final-merge-tests3.log.
- Valgrind passed all 1,081 tests. All 33 process logs reported zero errors,
  zero definite leaks, and zero indirect leaks, without suppressions.
  Logs: /tmp/luminari-acceptance-native-final-valgrind-tests.log and
  /tmp/luminari-acceptance-native-final-valgrind.*.log.
- Pinned clang-format 18.1.8 and `git diff --check` passed. The removed-file
  list must be excluded when invoking the formatter; deleted headers cannot
  be formatted. No failing test was disabled to obtain these results.
- A real full-world copyover retained the connection and Treat Injury timer:
  149.8 seconds before, 107.1 seconds after, with the readied action cleared.
  The helper completed its full check in 47 seconds. The last source change
  afterward was formatting only, not a behavioral change.
- Offscreen autonomous initiation was repeated after physical rollback
  removal. The janitor and fido remained fighting at 490/500 and 493/500 HP
  after the observer spent 30 seconds in another zone. Three semantic rounds
  and six turns executed; extraction cleaned up the test encounter.

Durable, credential-free command evidence is under
[event-acceptance-2026-09-05](event-acceptance-2026-09-05/README.md).
Earlier private transcripts remain available locally for the broader matrix.

## Vessel Scope And Evidence

Vessels are not ignored and the vessel system is not declared finished. The
migration retains per-hull vessel.greyhawk.agenda events, converted transport
events, and genuinely shared vessel services. Autopilot, hunter behavior,
naval combat, crew wages, upkeep, narrative/weather/encounters, and schedules
still have their established callbacks. This preserves the existing grouped
per-hull cadence; it is not a claim that every vessel activity has already
been redesigned into a separate concrete-action event.

The copied test world's vessel_system setting initially defaulted to off.
It was enabled only in the isolated configuration and loaded by copyover.
The live builder test then created, tuned, spawned and sailed a disposable
boat, and removed its hull/prototype successfully. A second test observed the
vessel-owned event complete an autopilot waypoint and remain scheduled. Hull
extraction reduced the matching owned-event count from one to zero. The
waypoint was within arrival tolerance: movement_steps remained zero, so this
does not establish a long autopilot voyage.

The old scale benchmark measured the removed whole-fleet heartbeat. Its
start command now explicitly refuses to produce misleading native-runtime
measurements; historical status/cleanup and parser checks remain available.
No vessel gameplay was disabled by that change. A native large-fleet scale
benchmark and broader vessel feature acceptance remain separate work. This
tranche does not certify 500 active vessels or all unfinished vessel features.

## Final Bounded Performance Result

With the full world and vessel setting enabled, 24 five-second process samples
averaged 4.48% of one CPU core. RSS rose from 1,528,800 to 1,529,184 KiB, a
384 KiB increase (about 0.025%), then remained at the new value for the final
samples. This small increase is measured, not explained away as a proven
allocator effect; a two-minute observation cannot establish a leak trend.

Before/after snapshots showed 42,461 then 42,457 live events, unchanged
43,467 high water, zero ready/overdue work, zero failures and admission
rejections, and no owner-registry mismatch. The late-callback counter rose
from 24,886 to 32,169 over the wider snapshot interval (148.6 scheduler
seconds). That counter records callbacks dispatched after their deadline,
not delay magnitude or a percentile. No tail-latency bound is established.

The helper rejected its requested 130-second wait (maximum 60) and logged
out; the independent process sampler still completed. The after snapshot
was obtained by a separate login. No gameplay/build activity interrupted
the process sample, but these logs must not be called one successful
continuous-session soak. The earlier five-minute busy sample and two-minute
idle sample are retained above, with their limitations.

## Merge Verdict

Functional acceptance passes for the event-core migration, including existing
vessel integration. The branch and isolated proposed merge build and test
successfully, copyover and cooldowns work in the copied world, owner cleanup
passes, and offscreen behavior remains active. No demonstrated functional
migration defect remains open from this acceptance pass.

I do not give an unconditional performance sign-off or recommend treating
this as an immediate production-deployment approval. Low average CPU and an
empty queue snapshot are not evidence of bounded callback delay. The remaining
decision is whether to merge the functionally accepted core with a separately
tracked performance gate, or hold the merge for quantified deadline-lateness
measurements and an agreed acceptance limit. My recommendation is to hold
unconditional migration approval until that focused measurement is complete.
This is a specific evidence gap, not a claim of a diagnosed timing bug.

Next focused acceptance work: expose or record lateness magnitude (maximum
and distribution), run an undisturbed longer full-world sample against an
agreed limit, and replace the obsolete fleet benchmark before making any
large-fleet claim. Broader unfinished vessel features remain their own scope.
No merge, commit, or push was performed in this acceptance goal.

The isolated acceptance server exited cleanly with status zero and its autorun
stopped. The ordinary port-4101 process retained its original PID and was not
restarted or copied over.

## Migration And Recovery

1. Commit and integrate the reviewed acceptance changes only when authorized;
   the checked-out branch has not been merged or pushed by this goal.
2. Before deployment, back up the database, player files, world/runtime data,
   and configuration together. Keep that coherent pre-migration checkpoint.
3. Build and install the native-only binary with the normal deployment tools.
   Keep the site's existing world settings, including its vessel setting.
   No loop-rollback build option or legacy-save writer is available.
4. Perform the controlled restart/copyover and verify login, eventdebug health,
   cooldown recovery, offscreen movement/combat, scripts, and enabled vessels.
   Existing event save formats remain readable; subsequent saves write Evn2
   format 2 with the equipped recovery cadence.
5. If migration fails, stop and preserve logs/data for diagnosis. Prefer a
   forward fix. Reverting to an older executable would require restoring the
   coherent pre-migration checkpoint and accepting intervening progress loss;
   new-save compatibility with that executable is not promised or maintained.

No archival PubSub/database deletion was performed. Network driver selection
is distinct from the removed gameplay loop and remains available.

## In-Game Checks

Use disposable test characters and mobs in a test area, not player property.
`eventdebug` shows health; `eventdebug player <name> 10`,
`eventdebug mob <name> 10`, `eventdebug room here 10`, and
`eventdebug scripts mob <name> 10` narrow inspection. Type and numeric owner
filters also work, for example `eventdebug type vessel.greyhawk.agenda`.

- Spend a cooldown, inspect it, log out until expiry, and log back in. Its use
  should return. Repeat through copyover; remaining time should decrease.
- Ready `look` on entry, have another character enter, then repeat after
  leaving the room or cancelling. Only the still-active subscription fires.
- Observe an autonomous wanderer, leave its area, and return later. Movement
  is probabilistic. In a controlled duel, both mobs should continue fighting
  while the observer is elsewhere.
- Inspect a script wait, then remove its disposable owner. No delayed action
  should occur afterward.
- With vessels enabled, inspect a spawned hull's event, exercise its existing
  sailing/autopilot commands, then purge the disposable hull. Its event must
  disappear. This is a migration check, not complete vessel acceptance.
