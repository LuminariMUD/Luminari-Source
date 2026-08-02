# Vessel System - Manual Testing Guide

Numbered manual regression script for the Vessel System. Run on dev after any
vessel-related change. Every step lists the expected result; any deviation is a
regression. The durable quality gate is in [PRD.md](../PRD.md); unresolved
findings are tracked in
[VESSELS_TODO.md](../project-management-zusuk/vessels/VESSELS_TODO.md).

**Current run status (August 2, 2026): all 30 steps, the bounded ferry gate,
the complete 500-vessel scale gate, the Vailand campaign shipping gate, the
Blackwake derelict discovery gate, and the current candidate's focused build
gates pass on local development.**
The legacy identity, generated-room insertion, sailing, route persistence, and
vehicle transport defects found during the run were repaired and retested with
Kohdee. Cleanup removed all disposable regression data. The separately named
shared harbor prototypes, route, ferry, pilot, and schedule intentionally
remain as reusable development fixtures.

The current normal candidate passes all 277 production-linked tests and all 13
focused protocol-parser tests. Strict actionable Memcheck across those 277
tests reports zero errors and no definite, indirect, or possible loss; a fresh
CMake build of the integrated candidate passed all 6 CTest targets. Required
`make install` removed the root artifact and installed non-profiled SHA-256
`281c7469702fbbeaa52f40a916a3911b121d3cfa9bd1050ed9feb4f1bad92075`.

Prerequisites: staff character (LVL_BUILDER+), MySQL running, server booted
with vessel commands and ticks enabled. The cedit
`CONFIG_VESSEL_SYSTEM` setting is a load-bearing kill switch: keep it `On` for
the numbered regression. Setting it `Off` blocks vessel command dispatch and
both heartbeat tick groups while retaining staff recovery commands.

## Shared Harbor Fast Check

Build/install first, then provision and verify the complete shared fixture with
one command:

```bash
./scripts/provision_vessel_harbor.sh
```

The development-only command merges the tracked world package, seeds three
representative prototypes and the four-waypoint, two-stop channel route,
creates the public ferry only if absent, and performs a hard-restart
persistence check. A passing run proves both seaports, the generated bridge
and cargo DG triggers, the restored ferrymaster, exact route topology, and
hourly schedule with its 10-gold fare. It validates the three canonical
legal-water polygons and proves that `seastate` resolves the moving ferry into
territorial waters or the nested free seas after restart. It also boards
through the ordinary object path, verifies one exact deduction, restores
Kohdee's starting gold, resumes the route, waits at most 45 seconds for an
actual named-water crossing announcement, and requires the immediately
reported `seastate` type, authority, and bounty to match it. The provisioner
discovers the ferry slot and runs the crossing session itself. It then opens
Kohdee and another character from the same master account, verifies
bidirectional cross-room `shiptalk`, ashore isolation, and both ashore
refusals, and logs out both sessions. If no second usable Name exists, it adds
reusable `Vesselmate` to that account and retries without creating another
account. It then ends with:

```text
PASS: harbor sandbox and persistent ferry verified in ship slot N.
```

It uses the existing master account and Kohdee character; do not create a new
account, look up a slot or character manually, or perform one login per
command. The first run includes ferry creation and a second restart. Before
the fare, crossing, and channel checks were added, later idempotent runs reused
the ferry and completed in about 30 seconds on the current development host.
Remeasure the augmented path with the current installed candidate.

## Vailand Campaign Shipping Check

Provision and exercise the tracked Luminari campaign package only on local
development:

```bash
./scripts/provision_vessel_campaign.sh
```

The provisioner validates the existing North Vailand Sea Port 1000360 at
`(-599, 455)` and Central Vailand Sea Port 1000362 at `(-467, 204)` before it
changes the database. It applies the Phase 13/14 prerequisites and idempotent
campaign content, rejects region or identity collisions, and verifies four
named legal-water regions, the exact 18-link route, merchant prototype,
faction, real iron cargo, pilot 31810, schedule, and two-port market gradient.
It resets only the campaign merchant's runtime to its canonical start when an
earlier route revision left that hull paused.

Two actual Kohdee sessions then observe the merchant for 45 seconds each,
with a hard server restart between them. The gate requires distinct live
`shipstatus` positions, shutdown-persisted movement, the same slot and
generation, active autopilot after restart, named legal waters, the exact
route, and arrival at the Central port. It rejects any campaign-related
`SYSERR`, then performs a final controlled restart with the merchant in North
Vailand territorial waters. Success ends with:

```text
PASS: Vailand campaign waters and merchant ship N passed through actual Kohdee (Ns).
```

The August 2 run passed in 167 seconds on source `923c8024` under
`/tmp/luminari-vessel-campaign-1000/runs/20260802T065410Z-1061371`. Kohdee
observed movement from `(-599, 455)` to `(-487, 244)`, the shutdown saved
`(-480, 229)`, the same generation resumed after restart, reached
`vailand_central_port`, and continued on the return leg. The final state was
the same slot/generation, traveling at `(-599, 455)`. All four SQL region
rows, 18 route links, the exact sequence, merchant definition, and both market
rows matched their verifier expectations.

Exercise the destructive campaign lifecycle immediately after provisioning:

```bash
./scripts/test_vessel_merchant_in_game.sh \
  --merchant "Vailand Ironwind Trader" --temporary-respawn 5
```

Run `20260802T065717Z-1068792` passed in 22 seconds. Kohdee sank merchant 18
generation 1 in slot 7, observed 165 total standing loss and a 900-gold
bounty, and inspected generation 2 with 40 iron, pilot 31810, the Vailand
route, and its active schedule. Cleanup restored Kohdee's player file and all
snapshotted vessel/economy tables exactly; the before/restored database dump
SHA-256 is `d21a9c2da318d6f6648ef1de57c8eaf8636d5047b46c0eb8e86e4e3563d24e21`.

## Blackwake Derelict Discovery Check

Provision and exercise the first tracked data/DG-driven derelict only on local
development:

```bash
./scripts/provision_vessel_derelict.sh
./scripts/test_vessel_derelict_in_game.sh
```

The collision-sensitive provisioner installs reserved object VNUMs
70010-70012 and trigger VNUMs 70010-70014, applies the Phase 11 prerequisite
and idempotent content SQL, and creates at most one ownerless ship-class
`Blackwake Derelict` at `(-533, 330)`. Actual Kohdee sessions must reach its
generated bridge, crew quarters, main cargo hold, and main deck on both sides
of a hard restart. The same slot, prototype, coordinates, room list, bridge,
entrance, cargo room, and all three room-trigger mappings must remain stable.

The reversible acceptance harness snapshots Kohdee's file-backed and
database-backed object state before its first preflight login. It rejects
existing Blackwake progress, then temporarily sets Kohdee to level 30 through
the normal staff command because DG command triggers do not target level-33+
staff. Actual commands must prove the chart is initially clue-gated, recover
and read exactly one captain log, recover exactly one chart, and leave the
cargo panel gated before logout. The ASCII player file must hold only the
first three discovery variables, while both object-save mirrors must contain
exactly one log and one chart.

After a hard restart, Kohdee must study the persisted chart, recover exactly
one tidefinder, receive exactly 180 gold from the production `salvage`
command, and receive the already-recovered response on a second attempt. The
final player file must contain all five discovery variables; both object
stores must contain `1|1|0` for log, chart, and tidefinder. Cleanup stops the
MUD, restores and compares every snapshotted file plus
`player_data.obj_save_header` and all matching `player_save_objs` rows, checks
the derelict identity again, and restarts the same installed binary without a
login.

Provision run `20260802T072737Z-1135588` passed in 61 seconds on source
`71cba1a2`, preserving slot 8, prototype 17, and rooms 70160-70163. Discovery
run `20260802T075751Z-1199403` passed in 55 seconds on source `a390a387`.
Kohdee's gold changed from 89,280 to 89,460 inside the snapshot, the exact
one-copy persistence checks passed, the database before/restored canonical
state hash was
`f0a2ede01ed085b07cd970744582de2e33b8bb7cd529930f0b66a717bf2072d7`,
and cleanup restored the level-34 baseline. Optional first-finder naming is not
enabled in this initial content package.

## Shared Harbor Merchant Loss Check

The provisioner validates but deliberately does not sink its NPC merchant.
Exercise the real loss and recovery path separately with:

```bash
./scripts/test_vessel_merchant_in_game.sh
```

The development-only harness uses actual Kohdee sessions to invoke the
production sink path, requires one 25-point attack consequence and one cargo-
plus-100 total-loss consequence, observes the regional bounty, waits through
the configured delay, and checks the next merchant generation in SQL and in
game. Its replacement must have the same durable definition with a public
hull, real cargo, active pilot, route, and schedule. The harness snapshots all
mutable vessel/economy tables and Kohdee's player file before the test. It
stops the MUD for exact restoration and byte comparison, then restarts the
installed candidate without logging Kohdee back in.

The August 2 installed-candidate run passed in 41 seconds. Merchant 1 moved
from generation 1 to 2 in slot 6; actual Kohdee observed 150 total standing
loss, a 510-gold bounty, 25 units of spice, pilot 70001, route 4, and the
replacement's active schedule and registry identity. The stopped database
dumps and Kohdee player files compared byte-for-byte before the no-login
restart launched the same installed executable.

The same provisioner now validates the Phase 15 HUNTED raft, Admiralty
warship, captain mobile 70002, encounter region 7000004, and deterministic
hunter policy without altering Kohdee's bounty. After installation, exercise
the complete reversible lifecycle with:

```bash
./scripts/test_vessel_hunter_in_game.sh
```

Require one real Kohdee encounter to create exactly one ownerless hunter with
the configured pilot and target. The same generation, slot, name, prototype,
pilot, and combat attribution must reattach after the script's hard restart.
Pardoning Kohdee must move the lifecycle to bounded cooldown, remove every
hunter persistence row, preserve the target, purge the temporary target, and
restore Kohdee's exact pre-test bounty/hunt rows. The script refuses an active
ferry soak or scale run and prints the artifact directory and elapsed time.
The August 2 current candidate passed this full sequence in 57 seconds under
`/tmp/luminari-vessel-hunter-check-1000/runs/20260802T001910Z-302111`. The raft
proved effective speed 1 after terrain adjustment; the same hunter identity
survived PID 299248 to 302590; pardon and cleanup restored the exact baseline.

Run the bounded ferry release gate through its supervised monitor. The
45-minute observation leaves 15 minutes for restart, review, and cleanup so
the complete task stays within one hour:

```bash
./scripts/run_vessel_ferry_soak.sh start 2700 60 900
./scripts/run_vessel_ferry_soak.sh status
```

The runner retains a historical longer default. Never omit the explicit
bounded arguments above and never restart the retired long-duration gate.

This keeps the otherwise idle game loop awake without occupying a character.
It submits a generated, nonexistent account name but never confirms it, so the
descriptor remains in a non-expiring confirmation state without creating an
account. The monitor requires that socket to remain `ESTABLISHED` every 20
seconds and fails if the server reports that it went to sleep. A copyover
drops that non-playing descriptor by design; the monitor accepts only a
log-proven same-PID, same-binary copyover, waits for boot, reconnects the
descriptor, and records the recovery. Any other socket loss remains a hard
failure. It checks unchanged process and database invariants every minute and
uses the existing account and Kohdee for live checks every 15 minutes. Launch
metadata records the source commit and installed executable SHA-256; a changed binary
fingerprint fails the run. A failure writes terminal status before cleanup.
After the requested duration, it pauses through the game, hard-restarts local
development, compares the exact coordinates, route, pilot, schedule, rooms,
structure, and executable hash, then resumes the ferry. The run is incomplete
until `status` reports `PASS`.

The August 2 run `20260801T230546Z-160058` is terminal `PASS`. Its complete
request-to-result time was 2,779 seconds, including a 2,740-second continuous
observation. Five actual Kohdee sessions, 46 database samples, and 46 process
samples recorded 1,476 movement steps, 246 waypoint arrivals, 62 route
completions, constant fleet count 6, at most 13 of 2,000 dynamic rooms, and
zero buffer overflows. The continuous process retained PID 160111, two
threads, 12 descriptors, and the installed executable hash. The final hard
restart changed to PID 252880, restored the exact paused vessel and route
state under the same executable, and resumed the ferry. Its memory analysis is
`REPORT_ONLY` because the bounded RSS increase coincided with awake-world
mobile, object, and movement-trail growth.

After that terminal `PASS`, use `make test`, `make install`, and
`run_vessel_scale_benchmark.sh start 1800` in that order. Use the explicit
1,800-second scale measurement so its complete supervised task stays within
one hour. The scale runner invokes
the harbor provisioner and all installed-character component gates itself;
running those commands separately before it wastes time and produces
fragmented evidence. Poll its `status` command for the one terminal result.

The August 2 first current-candidate run stopped before measurement under
artifact
`/tmp/luminari-vessel-scale-benchmark-1000/runs/20260802T003029Z-328201`.
It reached slot 500 and passed the harbor, channel, economy, and other live
workload setup. Its airship started with active autopilot and descended from
the seeded ceiling to Z 480 before Kohdee reached it, so the valid 480 to 490
manual climb could not satisfy the rejection assertion. The fixture now holds
that one airship paused at Z 500 through the boundary command and resumes it
afterward. Cleanup restored the exact six-vessel baseline. Do not treat this
premeasurement harness failure as performance evidence; rerun the gate.

Retry `20260802T004119Z-349856` passed the corrected Z-500 rejection and again
proved 500 of 500 live through Kohdee, then stopped before profiler reset on a
log-slice defect. The development login helper truncates its server log on the
workload restart, so carrying the old file's byte offset skipped the new boot
summary. The runner now captures that fresh reconstruction log from byte zero.
The retry also revealed that returning Kohdee to a harbor holding hundreds of
benchmark hulls can render `**OVERFLOW**` and contaminate the global buffer
counter before measurement. Generic ashore/MSDP/message transitions now use
staff room 1204; harbor-specific checks remain at the actual docks. Cleanup
again restored the six-vessel baseline. Rerun for the first performance sample.

Launch `20260802T005623Z-378533` then stopped before fleet creation because the
fare session's west-coordinate disembarkation did not share the canonical room
holding the ferry hull. `board ferry` correctly refused proximity. The gate
now waits specifically for west dock `(-66, 92)`, pauses and stops, disembarks,
resolves room 1000389, and performs ordinary boarding there. The full
standalone provisioner passed the exact charge/restoration and the subsequent
crossing/channel checks. A tooling assertion preserves this command order.

Run `20260802T010309Z-392860` passed those repairs, reconstruction, and the
live reciprocal-combat observation. Kohdee saw repeated return fire and
`vessel_messages_throttled=393`, but LF-CR Telnet output put a carriage return
before each printed CSV line and the helper's anchored regex rejected the
valid counter. The common output cleaner now removes that leading CR while
retaining indentation. Cleanup restored the baseline. Its 18-tick diagnostic
profile had a 226,912-usec maximum and 26 missed pulses, so the full run must
still determine and address the actual performance verdict.

Run `20260802T011448Z-414722` passed all of those repaired stages, spawned all
500 vessels through 496 Kohdee commands, completed economy, Z, and fresh boot
checks, and observed reciprocal fire with 18 suppressed messages. It stopped
before steady measurement at the native MSDP client: `whois` proved the server
had MSDP enabled, but cooked pseudo-terminal input buffered and caret-echoed
the binary REPORT controls. The helper's MSDP-only connection now runs raw and
without echo, completes TTYPE-first negotiation, and evaluates the effective
client cache when unchanged neutral values do not produce dirty updates. An
actual seven-second Kohdee check against baseline ship 1 received all nine
aboard values and proved all nine neutral ashore. An eight-second check on the
actively navigating public ferry paused slot 5 at `(-63, 82)`, received the
same complete state, resumed autopilot, and cleared ashore. The helper applies
the last frame so a late movement update cannot mask the final clear. Cleanup
restored the six-vessel baseline and exact installed candidate on PID 431693.
This fifth artifact also contains no steady performance result.

Run `20260802T013644Z-457615` passed every component gate and completed the
full 1,800-second window with 500 vessels on PID 466495. It failed terminal
live-system validation because the reconstruction login inherited Kohdee's
crowded post-spawn harbor location and overflowed while rendering hundreds of
hulls before its first command. Both system checkpoints recorded one overflow.
The complete 3,676-tick profile is valid diagnostic evidence: median 764.50
usec, p95 130,928.50, p99 166,398.50, maximum 1,027,228, 6,157 missed pulses,
23,888 suppressed messages, and 80,950 database queries. Crew wages peaked at
1,014,543 usec, autopilot at 213,780, encounters at 149,653, and schedules at
24,889. The 57-sample, 1,861-second memory series is `REPORT_ONLY`: RSS rose
786,784 to 854,412 KiB while movement trails rose 30,426 to 289,000; threads
stayed 2 and descriptors 11-12. Cleanup restored the six-vessel baseline and
exact installed candidate on PID 522541. The next candidate must move Kohdee
to room 1204 before restart, spread synchronized payroll work, reuse dynamic-
room metadata, and resolve encounter regions once per shared room.

The resulting optimization candidate spreads payroll across 100 stable
batches, limiting a 500-slot fleet to five due ships per tick, and combines
each changed roster into one insert after its delete. It caches encounter
containment by shared exterior room for one pass and reuses released dynamic
rooms whose spatial metadata already matches a known coordinate. The runner
also saves Kohdee in room 1204 immediately after spawning. The warning-free
production suite passes 271 of 271, all vessel tooling passes, and strict
actionable Memcheck reports zero errors and no definite, indirect, or possible
loss. Installed SHA-256
`ade8d4db466ec5d2f49a5cd7f30ceda4a3e29af570921e8e6005797c7e8db12e`
runs on PID 565375. An actual Kohdee smoke saw six of 500 slots and returned to
room 1204.

Run `20260802T024352Z-573327` exercised that binary for another complete
1,800-second window with 500 vessels on PID 582492. Initial and final live
samples both reported zero buffer overflows, so the crowded-login fix is
accepted. The first terminal failure was a harness defect: generic `@wait`
discarded asynchronous socket output, although the server recorded 20
encounter deliveries for Kohdee's moving airship and 20 shared encounters for
60 vessels. The server log also exposes 225 real route failures through an
invalid intermediate water-route cell. Six additional full-capacity messages
were traced to baseline NPC merchant prototype 7 reconciliation, not hunter
spawning; this is expected deferred work while all slots are occupied.

The resulting 3,665-tick profile remains a release failure: median 802.00
usec, p95 131,989.20, p99 176,272.80, maximum 355,394, and 6,217 missed pulses.
Query volume fell to 67,052 and payroll p95 to 9,146.80 usec, while autopilot
p95 remained 130,774, payroll maximum remained 353,062, and encounters peaked
at 60,540. The 59 process samples span 1,854 seconds; RSS rose 786,296 to
853,480 KiB and the analyzer remains `REPORT_ONLY`. Cleanup restored six
vessels and the unchanged installed binary on PID 640439. Correct the helper,
workload fixtures, remaining synchronous hotspots, and memory growth before
the next complete run.

The installed follow-up candidate captures and returns generic `@wait` output,
keeps the reciprocal schedule and every water-class fixture on the verified
`y = 82` channel, and logs full-fleet merchant reconciliation as informational
deferral. An actual Kohdee terrain probe traversed `x = -66` through `x = -62`
at `y = 82` before the route change. Piracy-law coordinates use a bounded
4,096-entry cache, encounter containment uses the canonical in-memory polygons
instead of spatial SQL, and at most five payroll walkoffs share one delete.
Movement trails coalesce duplicate signatures and retain at most 16 per room.
The warning-free build, vessel tooling, all 274 production-linked tests, and
strict actionable Memcheck across those 274 tests pass. `make install`
installed SHA-256
`0e79d6edb09be793d293ac31dee4aa42860368c4381659861309ce1d4bec3021` and
removed the root artifact. Pushed commit `d610d58a` now runs on PID 714795 with
that exact mapped hash. Actual Kohdee passed a 17-second login smoke, reported
the six-ship baseline in a second session, and saved in room 1204. The scale
acceptance remains mandatory.

Run `20260802T035823Z-718533` completed a repaired 600-second diagnostic with
500 vessels. Every actual-Kohdee component and terminal gameplay check passed.
The transcript retains unsolicited scheduled-route and encounter messages,
including six shared multi-ship encounters; ten scheduled departures and nine
distinct live airship Z values were observed. The server recorded zero route
failures, workload errors, high-volume progress logs, or live buffer
overflows. Cleanup restored all six baseline vessel rows and restarted local
development. Treat the helper, route, merchant deferral, containment, and
payroll behavior as accepted.

The run remains a performance failure. Its 1,217 complete ticks reported
median 659 usec, p95 66,429, p99 86,597.80, maximum 103,801, and 2,150 missed
pulses. Autopilot p95 was 66,286.60 usec; one encounter reached 56,901 usec.
Payroll maximum was 112 usec and schedules maximum 15,515. Across 631 seconds,
RSS grew 786,304 to 807,504 KiB while movement trails grew 21,472 to 68,895;
memory remains `REPORT_ONLY`. Remove NPC trail retention and the remaining
synchronous outliers before the required full 1,800-second rerun.

The next memory candidate retains player footprints but does not allocate
trail records for ordinary NPC movement. Production tracing found no gameplay
reader for the retained lists. A production-linked test moves an NPC without
increasing the live count and separately proves that player movement still
adds a trail; the root suite binary passes 275 of 275. The next installed fleet
run must show stable trail count and RSS before accepting the memory repair.

Profile run `20260802T043120Z-786413` then passed every live 500-vessel gate
and held movement trails at exactly zero for all 631 seconds. Mobiles increased
by 669 and objects by 125 while RSS increased 15,384 KiB, so trail retention is
accepted but whole-world memory remains `REPORT_ONLY`. Because the installed
binary used `-pg`, its wall timings are diagnostic only.

The retained call graph shows 14,379 false runtime saves from port-berth
processing on public vessels with zero fee state, versus 14,938 total measured
database executions. Encounter processing issued 3,528 additional synchronous
queries. These are the next two production paths to fix before installing a
normal build and repeating the fleet gate.

The installed normal candidate now returns from departure processing when no
fee marker exists and evaluates encounter candidates from a bounded boot
cache, including optional hunter policy. Staff-forced checks reload that cache
before running. Production-linked regressions cover genuine settled and
unpaid berth state plus the exact region, hull-class, and inclusive depth
filter boundaries. The 277-test root suite and 13-test protocol harness pass;
actual Kohdee cache-boot and normal 500-vessel timing evidence remain required.

The cache-boot half now passes through actual Kohdee. PID 850894 maps the
installed hash above, boot loads one encounter definition without a related
`SYSERR`, and Kohdee reports the six-vessel baseline, 5 of 2,000 dynamic
rooms, zero trails, and zero overflows from room 1204. The session logs out
cleanly in 22 seconds. Keep the timing half open through the scale runner.

Normal diagnostic `20260802T050422Z-854067` now passes the timing half for a
600-second request. All 500 vessels and eight classes survive 1,221 complete
ticks with median 577 usec, p95 1,524, p99 2,011.60, and maximum 2,915.
Autopilot peaks at 2,785 usec, encounters at 232, and schedules at 17,537;
database executions fall from 14,942 to 1,295. The actual Kohdee session
records ten departures, six shared encounters, nine Z values, zero workload
or logging errors, zero overflows, and trails 0/0/0. RSS rises 14,888 KiB
alongside 660 mobiles and 131 objects, so memory remains `REPORT_ONLY`.
Cleanup restores six vessels and restarts development. Repeat for the required
1,800-second window before marking the scale release gate complete.

Full run `20260802T052407Z-896082` completes that gate. One process sustains
500 vessels across all eight classes for 1,862 measured seconds and 3,655
complete ticks. Median, p95, p99, and maximum complete-tick times are 599,
4,079, 5,169.06, and 10,520 usec; every subsystem maximum is below 25 ms. The
actual Kohdee workload records ten departures, 20 shared encounters, 12 Z
values, 4,247 database executions, zero workload or logging errors, zero route
failures, zero overflows, and trails 0/0/0. RSS rises 33,148 KiB while mobiles
rise by 1,613 and objects by 451; rooms remain fixed, allocation lists fall,
and memory remains `REPORT_ONLY`. Cleanup restores the exact six-vessel
baseline and restarts development. The current 277-test Memcheck then passes
with zero actionable errors or leaks, completing the complementary bounded
stability check.

For the builder-independence timing gate, run:

```bash
./scripts/dev_kohdee_login_smoke.sh --vessel-builder-check
```

This keeps one real Kohdee session open, derives the new prototype ID and ship
slot from in-game output, creates/tunes/shows/spawns the vessel through
`vedit`, sails it one cell, and removes both temporary records. The August 2,
2026 current-candidate run used prototype 13 and ship 7, took 2.8 seconds for
the in-game workflow and 8 seconds including login, cleanup, character logout,
and account logout. The generated Boat moved from `(-66, 92)` to `(-67, 92)`
at effective speed 1 in storm. The creation and spawn path made no C, SQL,
world-file, or configuration edits.

## A. Legacy world-file vessel (zone 700 test object)

Intended fixtures: object 70002 (test vessel, ITEM_GREYHAWK_SHIP), room 70003
(interior), room 1000389 (Testing Dock, wilderness (-66, 92)). Before running,
verify that the object's ship index, the initialized fleet slot, the `shipnum`
field, `shiproom`, and `world[room].ship` all identify the same ship, and that
`shiplist` reports aggregate hull structure as 240/240.

1. `goto -66 92` - you arrive at "Testing Dock", room shows `[ Dockable ]`
   and coordinates (-66, 92); the test vessel object is in the room. Do not
   use dynamic wilderness room VNUM 1000389 after a reboot; it may not be
   allocated yet.
2. `board` - "You board the ship."; you are in room 70003, flagged
   `[ Vehicle ]`.
3. `disembark` - you return to Testing Dock at (-66, 92) (regression check for
   the `IN_ROOM(shipobj)` exit-point fix; must NOT print "Unable to find a
   valid exit point"). The dynamic wilderness room VNUM is not stable across
   reboots.

## B. Prototype editor (vedit)

4. `vedit` - usage text lists
   list/new/show/set/delete/spawn/spawnpublic.
5. `vedit new 2 The Gull` - "Created Ship prototype N: 'The Gull'
   (speed 15, armor 20)."
6. `vedit list` - table includes prototype N "The Gull", class Ship.
7. `vedit set N speed 12` - "Prototype N updated: speed = 12."
8. `vedit set N armor 999` - rejected: "Armor must be 0-100."
9. `vedit show N` - shows class Ship, speed 12, armor 20->per set, cargo
   12000 lbs.

## C. Spawn and sail

10. `goto -66 92`, then `vedit spawn N` - "Spawned 'The Gull' (Ship) as
    ship S: R interior rooms, entrance E, bridge B." A Ship has at least four
    rooms and may gain discovery rooms; object "The Gull" is moored in the
    room.
11. `board gull` - you enter the generated entrance room. Its name and
    description match the active `ship_room_templates` database row (the
    default is "Crew Quarters"); edited database text appears instead of the
    compiled-in fallback.
12. Move through every listed interior exit, including diagonals when
    discovery rooms exist - all generated rooms connect and every spoke
    returns to the bridge; no dead end traps you.
13. From the bridge: `speed 5`, then `heading 90` - status output reflects
    the values; `shipstatus` shows position (-66, 92).
14. `setsail west` - the ship moves from (-66, 92) to (-67, 92);
    `shipstatus` reflects the new coordinates. An explicit debug build shows
    `[VESSEL_MOVE]` lines only after `vdebug on move`. East of Testing Dock is
    land and must remain non-navigable to this hull.
15. `speed 0`, `disembark` - you exit to the wilderness room at the ship's
    current coordinates.

## D. Autopilot round-trip

16. On the bridge: `setwaypoint dockpoint` - waypoint created at current
    position.
17. Sail west a few steps (repeat step 14), then `setwaypoint westpoint`.
18. `createroute testrun`, `addtoroute testrun dockpoint`,
    `addtoroute testrun westpoint` - both additions confirmed.
19. `setroute testrun`, `autopilot on` - ship begins moving toward the
    first waypoint. In an explicit debug build, `vdebug on auto` enables
    `[VESSEL_AUTO]` lines in syslog.
20. `autopilot status` - shows TRAVELING with waypoint index.
21. `autopilot off` - ship stops; state OFF.
22. Reboot the server (copyover or full restart). `listroutes` - route
    "testrun" persists with both waypoints (route DB round-trip).

## E. Vehicles and vehicle-in-vessel

23. At Testing Dock: `vehiclecreate cart regression_cart`,
    `vmount regression_cart`, `drive east` - the cart and rider move to
    (-65, 92), and `vstatus` agrees. `drive west` returns both to the dock;
    `vdismount`. The other adjacent cells are water and are not a valid cart
    movement test.
24. With The Gull present at the dock and speed 0: board it, move to the
    bridge, then `loadvehicle regression_cart` - "You load regression_cart
    onto The Gull." `unloadvehicle` lists it by name.
25. At the dock, create six wagons named `heavy_1` through `heavy_6`. Load
    `heavy_1` through `heavy_5`. With the 500-lb cart already aboard, those
    five 2,000-lb wagons use 10,500 of the Ship class's 12,000-lb capacity.
    `loadvehicle heavy_6` is refused with "The vessel cannot carry any more
    vehicles." This is the actual boundary; a single second wagon does not
    exceed this hull's capacity.
26. On the bridge, `speed 5`, then `unloadvehicle 1` - refused because the
    vessel is moving. `setsail west`, `shipstatus` - position becomes
    (-67, 92), and loaded vehicle coordinates follow it. `speed 0`, then
    `unloadvehicle 1` - refused because shallow water is unsuitable for a
    cart. Return with `speed 5`, `setsail east`, `speed 0`; at the seaport,
    `unloadvehicle 1` succeeds at (-66, 92).

## F. Docking and boarding defense smoke

27. Create/spawn a distinctly named second prototype, such as "The Tern", in
    the same wilderness cell. From The Gull's bridge: `dock tern` - docking
    completes. Move to the entrance and traverse the temporary gangway in both
    directions; `undock` removes that exit while leaving both hull objects and
    coordinates together until one sails away.
28. (Hostile path, staff-only smoke) From The Gull after undocking:
    `board_hostile tern` - on success, the character enters The Tern and sees
    the warning and "BATTLE STATIONS!" broadcast. On a failed roll, the
    character stays aboard their ship; on a critical failure, they fall into
    its exterior wilderness room and receive a Swim (Athletics) check. On
    defended ships, idle crew NPCs reposition to the entrance/bridge.

## G. Cleanup

29. Use `shippurge <slot>` on each prototype-spawned vessel. `shiplist` drops
    the slot, its hull disappears, its generated room VNUMs no longer resolve,
    and its loaded vehicles are released beside the hull. Respawn once into
    the same slot to prove the room VNUM range can be reused without reboot,
    then purge it again.
30. Purge each test vehicle with `vehiclepurge <vehicle-id>`. Delete the test
    prototypes with `vedit delete <id>` and verify `vedit list` is empty.
    Delete test navigation data with `delroute testrun`, then
    `delwaypoint dockpoint` and `delwaypoint westpoint`; both list commands
    report no remaining test data.

## Release-Boundary Evidence

The July 29-30, 2026 local-development runs also proved the boundaries around the
numbered gameplay flow:

- With a real autopilot route active, cedit `Off` held Test Vessel at
  `(-70, 92)` across two separate Kohdee sessions. `autopilot status` and
  `speed` were refused, while `look` and recovery `shiplist` remained
  available. Cedit `On` restored tick processing and persisted to
  `lib/etc/config`.
- An explicit `-DVESSEL_SYSTEM_DEBUG=1` development build enabled only the
  requested `move` category at runtime and produced `[VESSEL_MOVE]` diagnostics
  during a Kohdee sailing test. The clean default build then refused
  `vdebug on move` and reported debug support `compiled out`.
- The earlier help release gate passed 31 maintained entries and 75 exact
  command keywords. The August 2 current-candidate gate first exposed a stale
  development database with no `SHIPTALK` mapping or text. Applying the
  tracked idempotent migration and restarting produced 32 entries, 78 exact
  mappings, correct access levels, nonempty content, zero obsolete duplicates,
  and a database `Help Tag` for all 78 commands in one 41-second Kohdee login.
- The current candidate maintains 32 authoritative entries and 78 exact
  keywords, including `shiptalk`, `vtradecheck`, `vmerchant`, and the Phase 15
  `vesseldebug encounter` guidance. Production-linked coverage sends an
  identified message between two different rooms of one vessel, proves an
  adjacent non-passenger receives nothing, and checks the ashore and silenced
  refusals. Both the MariaDB verifier and installed-build exhaustive help
  transcript now pass the entry, keyword, access, content, duplicate, and
  channel-text checks. The installed-build two-character transcript also
  passes on the current candidate and reuses the existing master account. The channel
  helper automatically selects another non-deleted account-menu character
  without a manual slot or Name lookup. A July 30 read-only database check
  found that account currently contains only Kohdee; the harbor provisioner
  adds reusable `Vesselmate` to that account when needed and runs the
  transcript automatically rather than creating a second account.
- All 22 component migrations preceding Phase 12 applied independently to a
  fresh MariaDB 10.11 master schema. Phase 12 separately passed real MariaDB
  install, 10-gold persistence, rollback, and reapplication against a
  session-scoped table that shadowed rather than changed the active ferry
  schedule table. Phase 13 passed schema creation, all three spatial fixtures,
  verifier content queries, rollback, and reapplication in session-scoped
  shadow tables; the active database remained unchanged. Production-linked
  geometry coverage also requires polygon interiors to resolve while edges and
  vertices remain outside, matching MariaDB `ST_Within()`.
- The complete disposable MariaDB chain through Phases 14 and 15 passes
  installation, repeated application, harbor seed, verification, reverse
  rollback, and two reapplications. Phase 15 verifies the encounter extension,
  HUNTED threshold, warship/pilot policy, one-row target lifecycle, cooldown
  fields, indexes, and clean rollback. The active development database was not
  used for this rehearsal.
- The Phase 09 runtime migration and verifier passed against local MariaDB.
  `ship_runtime_state` held the expected parent-linked live snapshots and
  `ship_schedules` held the scheduled route.
- The Phase 10 verifier found both lifecycle tables, all five runtime columns,
  four normalized installed-weapon rows, no orphan or invalid weapons, no
  invalid insurance claims, and no invalid dock-fee state.
- The Phase 11 verifier found the generated-room trigger attachment table with
  no invalid mappings or room types above the eight-trigger limit.
- The shared harbor provisioner created the second static seaport, three
  representative prototypes, and public ferry in slot 5. A hard restart
  restored its ferrymaster, active two-stop route, progress, and hourly
  schedule. The route contains west dock, channel turn, east dock, and the
  return channel turn in that order. Speech diagnostics fired trigger 70001
  on the generated bridge and 70002 in the generated cargo hold. An
  idempotent rerun reused the same public ferry and finished in about 30
  seconds.
- The single-session builder gate used Kohdee and only `vedit` for
  prototype creation, tuning, inspection, and spawn. It discovered generated
  IDs from game output, sailed the Boat west one cell, and removed all
  temporary data in 2.7 seconds of workflow time and 8 seconds end to end. The
  run also exposed and fixed truncated long vessel names in `shippurge`
  diagnostics.
- The unattended ferry pre-soak exposed a gale hit while its assigned
  ferrymaster wandered out of the bridge. Assigned pilots now skip ordinary
  mobile wandering, the fixture mob is Sentinel, and structural gale damage
  requires both no sailmaster and no pilot physically at the bridge. In a
  60-second live storm run, the ferrymaster remained at the helm through
  multiple hazard pulses and the repaired hull stayed 20/20 on all sides.
  `shipfix` persisted the repair, and a full service restart restored the same
  full condition and pilot.
- The next live ferry pass exposed two independent navigation defects.
  Floating movement converted negative coordinates by adding 0.5 and casting,
  which truncates toward zero; signed nearest-cell rounding now lets the ferry
  advance west of the origin. The original direct dock-to-dock fixture leg
  also crossed Beach at `(-63, 84)`, so the route now takes the navigable
  `(-64, 82)` channel turn on both legs. Kohdee observed a complete west ->
  channel -> east -> channel -> west loop, followed by another arrival at the
  east dock, with no ship-5 impassable-terrain stall. A simultaneous one-point
  decrease across all armor arcs was the expected persisted
  `SHIP_WEAR_INTERVAL`, not a gale hit.
- The first accelerated monitor shakedown was rejected despite its apparent
  movement and final-restart success: the idle account-name descriptor expired
  after about 49 seconds, and later Kohdee samples temporarily woke the game
  loop. The corrected monitor enters unconfirmed account-name confirmation,
  checks the socket every 20 seconds, and treats any game-loop sleep line as a
  hard failure. The corrected bounded shakedown below validates the monitor
  beyond the old timeout; no long-duration clock is required.
- The corrected replacement ran continuously for 150 seconds: 84 movement
  steps, 22 distinct positions, 3 west and 4 east arrivals, 5 actual Kohdee
  inspections, and 16 database/process samples under one travel PID. The
  final restart restored the exact paused coordinate and route under a new
  PID, and Kohdee resumed the ferry. This validates the monitor path used by
  the completed August 2 supervised ferry gate.
- A follow-up provenance shakedown pinned source commit
  `0afad17bdb8fd67a78a58fa1af9e41d6ccc79efc` and executable SHA-256
  `ae7c6414bc934f4ddf09f6c35a3d97b15a9a5fa1845c13a109142eaf9b5ca2a2`.
  The final restart launched the identical hash and restored exact state.
  Deliberate SIGTERM on a separate run wrote a terminal `FAIL` artifact
  immediately instead of leaving stale `RUNNING` state.
- The pinned 24-hour attempt remained vessel-healthy for 34,382 seconds with
  18,720 movement steps, 780 arrivals at each dock, 10 Kohdee inspections, and
  574 database/process samples. The normal 11:00:30 IDT automated copyover
  preserved the MUD PID and ferry but intentionally dropped the monitor's
  unauthenticated descriptor. The old monitor misclassified that handoff and
  failed to finalize status, so the run is `ABANDONED`. The replacement
  monitor recognizes only a log-proven same-PID/same-binary copyover,
  reconnects after boot, preserves its evidence, and writes terminal failure
  status before cleanup. The former long-duration gate is retired and must not
  be restarted.
- A graceful full restart reconstructed two prototype-spawned hull objects and
  their 7-room and 6-room dynamic interiors. The transport retained Kohdee as
  owner, 400 pounds of timber, able sailmaster and green quartermaster, hull
  reinforcement, 20,000 gold insurance, and starboard armor damage at 6/20.
  The warship retained position `(-64, 85)`, heading 345, port armor damage at
  36/40, paused route progress, and its 24-hour schedule.
- `--copyover-check` kept Kohdee's live descriptor across a real process
  replacement. In the stronger active-voyage run, the warship was Traveling
  toward waypoint 1 at `(-63, 81)` before copyover and recovered Traveling on
  the same route, advancing to `(-62, 82)`. The transport's ownership, combat
  link, cargo, crew, refit, insurance, and damage remained intact.
- Kohdee sailed the owned transport into Testing Dock and received one
  35-gold fee. `shipstatus` and `dockfees` reported the debt, departure was
  refused, and the debt survived copyover and a full service restart. Kohdee
  then paid 35 gold, departed, returned, received exactly one new fee for the
  new visit, paid it, and departed again. The final balance was clear. These
  runs also exposed and fixed stale dynamic-room port identity after recovery.
- The level-1 Veska bought a 50-gold policy for 10 gold, verified a 9,990-gold
  balance, and logged out as owner of the insured raft. Kohdee then sank it
  with `shipfire`. Before Veska returned, one pending claim and one underwriter
  receipt mail existed. Her next actual login showed 10,040 gold and changed
  the claim to paid with player-file high-water mark `VIns: 1`; a second login
  remained at 10,040 with no duplicate claim or payment.
- Corven owned a raft and held a helm permit on Kohdee's transport. Setting the
  reversible deleted flag blocked Corven's login while preserving both
  relationships; clearing it restored play aboard the same raft. Corven then
  used the actual character-menu password and `yes` flow with fast wipe
  enabled. The player file and `player_data` row disappeared, the raft became
  unclaimed, the permit disappeared, and a controlled pending claim changed to
  `void`. SQL plus live `shiplist` and `shipcrew` agreed.
- Elyra then owned a separate raft and held Corven's former helm permit on the
  Tern. A temporary MariaDB trigger rejected only the attempt to clear Elyra's
  ownership during permanent removal. The actual account password and `yes`
  flow reported that deletion was cancelled and returned to the character
  menu. The trigger was removed immediately. SQL retained one account
  membership, one owned ship, and one helm permit; Elyra logged straight back
  in aboard the raft, while Kohdee's live `shipcrew` still listed her Tern
  permit.
- Dorrin, Elyra, and Veska each enabled PvP through their own level-1
  character session. Dorrin owned the armed Tern and attacked Elyra's online
  raft without sinking it, producing reciprocal 300-second SQL snapshots.
  Elyra logged out; Dorrin stayed connected through a real copyover and was
  explicitly allowed to continue. Veska was transferred aboard the same Tern
  and was refused as a third party. After waiting out the actual five-minute
  clock, Dorrin was refused too.
- The same run found that `shipdeed` cleared grace only in memory. Owner
  transfer and runtime reset are now one transaction, capture uses the same
  boundary, and permanent removal clears persisted grace in its existing
  cleanup transaction. After a fresh boot restored the stale test row, Veska
  deeded the raft to Elyra in live sessions; SQL changed atomically to owner
  `Elyra`, grace `0`, and empty attacker. The Tern was returned to Kohdee, the
  raft was repaired, test wages were paid, and all three PvP flags were
  disabled.
- A hard local service replacement reproduced stale autopilot resurrection:
  the Goshawk reloaded Traveling on route 3 after an earlier in-memory
  `autopilot off`. The installed fix made player `setroute`, `autopilot on`,
  `pause`, resume, and `off` write `ship_runtime_state` before success output.
  Immediate SQL showed Off/route 3, Paused/route 3, Traveling/route 3, and
  Off/route 0 as expected; hard replacements restored Off-with-route and
  Paused exactly.
- A temporary MariaDB trigger rejected only ship 3's runtime insert while
  Kohdee tried to resume from Paused. The command reported that the change
  could not be saved, `autopilot status` remained Paused on `persistroute`,
  SQL remained state 3/route 3, and the trigger count returned to zero.
- Hull recovery resolved saved wilderness locations from authoritative
  coordinates instead of trusting recycled VNUMs. Tern and Dinghy moved from
  stale 1000121/1000120 snapshots to the current Testing Dock room 1000389;
  Goshawk retained dynamic room 1004000 at `(-62, 82)`.
- A fifth prototype-spawned Dinghy shared Goshawk's dynamic exterior room.
  Both hulls were visible and independently boardable, consumed one pool room
  after a 15-second unattended interval, restored together after a hard
  restart, and survived `zreset 10000`. The fixture Test Vessel and two other
  hulls at Testing Dock survived the same reset.
- Generated exterior keywords retained exact names while splitting readable
  tokens. Live `board test`, `board tern`, `board dinghy`, and
  `board goshawk` all entered the intended vessel. The temporary slot 5 then
  purged its two rooms and all persistence, leaving the four durable fixtures.
- The full production-linked root suite passed 220 tests, followed by
  `make install`; no root-level `circle` artifact remained.

## Known Findings

- Legacy ship slot, `shipnum`, object value, and interior room mismatch -
  FIXED and live-tested.
- Runtime-generated interior rooms are inserted in sorted VNUM order and
  reindex live characters, objects, exits, and vehicle room references -
  FIXED and live-tested.
- Vehicle loading now finds the named vehicle beside the exterior hull,
  persists its parent vessel, follows ship coordinates, and unloads beside
  the hull only on compatible terrain - FIXED and live-tested.
- Dock targets resolve by displayed ID, fleet ID, exact name, or name keyword;
  docked gangways are traversable only between mutually docked ships, and
  undocking preserves coherent exterior positions - FIXED and live-tested.
- `shippurge` transactionally removes ship-instance persistence, evacuates
  occupants and loose objects, releases carried vehicles, removes the hull,
  and immediately reclaims generated interior rooms. Same-slot respawn works
  without reboot - FIXED and live-tested.
- Legacy and builder-spawned hulls now share complete condition
  initialization. Test Vessel reports 240/240 internal structure and no longer
  sinks when armor absorbs its first weather hit - FIXED and live-tested.
- Boot now relinks zone-reset hull objects to active fleet slots. Kohdee logged
  out inside room 70003, the server fully restarted, and `disembark` then
  returned to Testing Dock without requiring another `board` command - FIXED
  and live-tested.
- Prototype-spawned vessels now save complete runtime snapshots before full
  shutdown and before copyover descriptor handoff. Boot reconstructs dynamic
  interiors and hull objects, then restores position, condition, combat,
  weapon-slot, autopilot, schedule, owner, cargo, crew, upgrade, and insurance
  state - FIXED and live-tested through both lifecycle paths.
- Owned hulls are charged once per port visit, retain unpaid dock fees through
  copyover and restart, cannot depart or resume autopilot while indebted, and
  can settle safely after dynamic wilderness rooms are recycled - FIXED and
  live-tested.
- An insured loss queues exactly one claim and underwriter receipt while its
  owner is offline, then credits the owner once on login without duplication on
  a later login - FIXED and live-tested with Veska and Kohdee.
- Reversible character deletion preserves ship relationships and restoration;
  normal permanent removal transactionally unowns ships, removes the deleted
  name from permits, and voids pending claims. If the transaction fails,
  fast-wipe deletion is cancelled before account unlinking or success output,
  and the active player plus all vessel relationships survive - FIXED and
  live-tested with Corven and Elyra.
- PvP logout grace is persisted for only the original consenting opponent,
  survives copyover, rejects third parties, expires after five real minutes,
  and is cleared durably with ownership changes - FIXED and live-tested with
  Dorrin, Elyra, Veska, and Kohdee.
- Player autopilot controls previously acknowledged RAM-only changes that
  could be replaced by a stale runtime row after an abrupt process
  replacement. Route assignment and on/off/pause/resume now persist before
  success output and restore the exact prior state if the write fails - FIXED
  and live-tested with Kohdee, hard restarts, direct SQL, and failure
  injection.
- Persisted exterior hulls previously trusted recycled wilderness room VNUMs,
  did not hold dynamic rooms occupied without a character, exposed generated
  names only as punctuation-bound tokens, and could be removed by a zone reset
  sharing prototype 70002. Recovery now resolves wilderness coordinates,
  hulls participate in dynamic-room lifetime, readable name tokens are added,
  and zone cleanup preserves active fleet objects - FIXED and live-tested with
  static and dynamic co-location, hard restarts, reset, boarding, and purge.
- Generated interiors now attach configured DG trigger prototypes by room
  type on both initial creation and restart restoration. The shared harbor
  bridge and cargo diagnostics fired after a hard restart - FIXED and
  live-tested.

# EoF
