# Local Development Login Quick Guide

**Last verified:** August 2, 2026
**Last updated:** August 2, 2026

**Active execution checkpoint (August 2, 2026, 09:59 IDT):** The full
1,800-second, 500-vessel release scale gate is terminal `PASS` under
`/tmp/luminari-vessel-scale-benchmark-1000/runs/20260802T052407Z-896082`.
Its complete setup, measurement, validation, cleanup, and restart took 2,338
seconds. All subsystem maxima stayed below 25 ms, and the run recorded zero
workload errors, high-volume progress rows, buffer overflows, or movement
trails. All 277 production-linked tests, strict actionable Memcheck, 13 focused
protocol tests, and 6 integrated CTest targets pass. The installed normal
binary SHA-256 remains
`281c7469702fbbeaa52f40a916a3911b121d3cfa9bd1050ed9feb4f1bad92075`.

The first Luminari campaign package now also passes actual-character
acceptance. Run `20260802T065410Z-1061371` provisions the Vailand legal waters,
18-link water-only route, iron markets, and `Vailand Ironwind Trader`, then
proves movement and Central-port arrival across a hard restart in 167 seconds.
Run `20260802T065717Z-1068792` uses Kohdee to sink merchant 18 generation 1,
observes 165 standing loss and a 900-gold bounty, verifies generation 2 with
40 iron, pilot 31810, route, and schedule, and byte-restores every changed
vessel/economy table plus Kohdee's player file in 22 seconds. Both runs use
source `923c8024` and the installed binary hash above. The development MUD is
active with the campaign merchant returned to North Vailand waters.

The first Blackwake derelict also passes actual-character acceptance.
Provision run `20260802T072737Z-1135588` creates or reuses one ownerless hull
at `(-533, 330)`, verifies its four-room generated interior and three DG room
mappings, and preserves slot 8/prototype 17 across a hard restart in 61
seconds. Discovery run `20260802T075751Z-1199403` uses real Kohdee commands to
gate a captain log, chart, cargo clue, and one 180-gold salvage item across a
second hard restart. It proves one-copy file/database persistence and five DG
variables, then restores every snapshotted Kohdee file and database object-save
field exactly in 55 seconds. The discovery run uses source `a390a387` and the
same installed binary hash above.

Use this smoke test to boot the development MUD, authenticate with the game
master account, enter the level-34 character `Kohdee`, and leave both the
character and account cleanly.

Use that same master account for ordinary additional test characters. One
account can hold multiple characters; do not create a separate account for
each character. A separate account is appropriate only when account isolation
is itself part of the test.

`Kohdee` is the character name. Do not search titles or look for the word
`forger`. Agents should run the fast path before inspecting the database,
process list, or menu layout.

## Fast Path

From the repository root, run:

```bash
./scripts/dev_kohdee_login_smoke.sh
```

That single command:

- Refuses to run unless `APP_ENV=development`.
- Reads credentials from `lib/.env` without displaying them.
- Reuses the local MUD if it is already listening, or starts `bin/circle` and
  waits for the game loop.
- Finds `Kohdee` by the exact account-menu Name column rather than a menu
  number.
- Enters the world, runs `quit`, selects `0`, and selects `Q`.
- Leaves a newly started MUD running as the user service
  `luminari-dev-login-smoke.service` and prints its PID/log.

Expected completion time is about 3-5 seconds with a running server, or only the
15-20 second boot time plus the login when the server is stopped. Success ends
with:

```text
PASS: Kohdee entered the world, left the character, and logged out of the account (Ns).
```

## Fast Shared Vessel Harbor

After `make install`, provision and verify the complete reusable harbor with:

```bash
./scripts/provision_vessel_harbor.sh
```

This development-only command reuses the configured master account and Kohdee;
it never creates another account. It installs only missing harbor world
records, seeds the database fixture, hard-restarts the supervised local MUD,
and verifies the two docks, public ferry, NPC pilot, hourly route, 10-gold
passenger fare, territorial/free-sea/pirate-cove wilderness regions, and
generated bridge/cargo triggers. Phase 14 also seeds one scheduled NPC merchant
with a durable faction identity, public hull, real spice cargo, pilot, route,
and five-second recovery delay. The provisioner requires a positive merchant
generation and checks its registry row, runtime identity, manifest, pilot, and
schedule both in SQL and through one batched Kohdee session. If the definition
is stale or still inside that recovery delay, it automatically runs two
in-game reconciliation passes around one six-second wait, then checks the
state again; do not stop to repair the row or relaunch the provisioner by hand.
Phase 15 also installs captain mobile 70002, a HUNTED target raft, an
Admiralty warship, encounter region 7000004, and its deterministic acceptance
policy. Provisioning validates those records but deliberately does not change
Kohdee's bounty or create a live hunter.

After restart it confirms that `seastate` resolves the ferry's canonical legal
waters, keeps one Kohdee session aboard until `shipstatus` reports a boardable
seaport, pauses and stops the ferry, then boards through the ordinary
hull-object path. It proves exactly one fare was deducted, restores Kohdee's
original gold, resumes the ferry, and waits up to 45 seconds for a real
territorial/free-sea boundary announcement. It immediately correlates that
announcement with `seastate`, then runs the two-character, cross-room
`shiptalk` and ashore-isolation proof on the same ferry. If the master account
has only Kohdee, it creates reusable `Vesselmate` on that account and retries
the channel proof automatically. The provisioner discovers every account-menu
Name plus the ferry and merchant slots; do not look them up or create a second
account. Allow about two minutes for the complete idempotent acceptance path;
the dock, crossing, and two-character waits dominate, while an initial fixture
creation may add one server restart.

The provisioner intentionally does not sink the merchant because the invoking
character receives real faction and bounty consequences. After installing and
provisioning the current candidate, run the reversible acceptance harness:

```bash
./scripts/test_vessel_merchant_in_game.sh
```

This development-only script uses actual Kohdee sessions to destroy the active
hull and cargo, observe both durable standing losses and the regional bounty,
wait through the configured recovery delay, and inspect the generation-plus-
one replacement's cargo, pilot, route, schedule, and registry identity. Before
mutation it snapshots Kohdee's exact player file and every mutable
vessel/economy table. Cleanup stops the MUD, restores and byte-compares both
snapshots, then starts the installed binary without logging Kohdee back in.
The player-file hash therefore remains exact after recovery. The script
refuses production, dirty or stale source, a different running executable, or
an active ferry/scale worker. Do not replace it with the raw destructive
`vmerchant sink` command.

The August 2 installed-candidate run passed in 41 seconds under artifact
`20260802T001649Z-297975`. Kohdee sank merchant 1 generation 1 in ship slot 6,
observed 25 attack plus 125 total-loss standing penalty and a 510-gold bounty,
then inspected generation 2 in the reused slot with 25 spice, pilot 70001,
route 4, and its active schedule. The stopped pre/post database dumps both
hash to
`0c24c88896c1d7f53c4b187b6b90bd39291f4722c7f001b5b7d8b9f63fee1b57`;
Kohdee's backup and recovered player file both hash to
`319cf91fd7893d3c06f56ace03c0a6d9a6f2a8fc5d365d85262174199da1069f`.
The no-login restart runs the same installed executable.

## Fast Vailand Campaign Shipping

After installing the current candidate, provision the tracked campaign
shipping package and run its real loss/recovery path:

```bash
./scripts/provision_vessel_campaign.sh
./scripts/test_vessel_merchant_in_game.sh \
  --merchant "Vailand Ironwind Trader" --temporary-respawn 5
```

Both commands refuse any environment other than development and reuse the
configured master account plus Kohdee. The provisioner first validates the
existing North and Central Vailand seaport VNUMs and coordinates, rejects
region/route/prototype/merchant/waypoint collisions, and applies the
idempotent campaign SQL. It then uses two 45-second Kohdee sessions around a
hard restart to prove real cargo, the exact route, territorial/free/pirate
waters, live movement, the Central-port arrival, persisted position, stable
merchant identity, and resumed autopilot. A final controlled restart places
the merchant back in North Vailand territorial waters.

The second command snapshots Kohdee and all mutable vessel/economy tables,
temporarily shortens only the selected merchant's respawn delay, invokes the
production sink path, and verifies the replacement generation in SQL and in
game. Its cleanup stops the server, restores and byte-compares both snapshots,
and restarts the same installed executable. Never replace it with a raw
`vmerchant sink` command. The August 2 passing artifacts are
`/tmp/luminari-vessel-campaign-1000/runs/20260802T065410Z-1061371` and
`/tmp/luminari-vessel-merchant-check-1000/runs/20260802T065717Z-1068792`.

## Fast Blackwake Derelict Discovery

After installing the current candidate, provision the tracked Blackwake
content and run its reversible discovery path:

```bash
./scripts/provision_vessel_derelict.sh
./scripts/test_vessel_derelict_in_game.sh
```

Both commands refuse production, dirty source, stale binaries, identity/VNUM
collisions, and active ferry or scale workers. The provisioner merges object
VNUMs 70010-70012 and DG trigger VNUMs 70010-70014 into reserved zone 700,
applies the Phase 11 prerequisite and idempotent derelict content SQL, and
creates at most one ownerless `Blackwake Derelict` at `(-533, 330)`. Actual
Kohdee sessions traverse its generated bridge, crew quarters, cargo hold, and
main deck before and after a hard restart. The provisioned fixture remains for
repeatable development exploration.

The acceptance harness requires a Kohdee baseline with no existing Blackwake
objects or discovery variables. With the MUD stopped, it snapshots Kohdee's
player file, player index, object file, optional legacy variable file,
`player_data.obj_save_header`, and every matching `player_save_objs` row
before any preflight login. It then uses ordinary in-game commands to prove
the missing-clue refusal, recover and read the captain log, recover the chart,
and leave cargo salvage gated. Because DG command triggers intentionally skip
level-33-and-higher staff targets, the reversible session sets Kohdee to level
30 in game; cleanup restores the exact level-34 baseline.

After a hard server restart, the harness studies the persisted chart, recovers
one bronze tidefinder, invokes the production `salvage` command, and requires
an exact 180-gold increase. Both the file-backed and database-backed object
stores must contain exactly one log, one chart, and no tidefinder after
salvage; all five DG variables must be in the ASCII player file. Cleanup stops
the MUD, restores and compares both persistence mirrors, verifies the derelict
identity is unchanged, and starts the same installed binary without logging
Kohdee back in.

The passing artifacts are
`/tmp/luminari-vessel-derelict-1000/runs/20260802T072737Z-1135588` and
`/tmp/luminari-vessel-derelict-check-1000/runs/20260802T075751Z-1199403`.
An earlier transcript displayed two logs and charts because its preflight had
loaded stale database rows before the old file-only snapshot; it was not a
core double-load. The current harness moves the snapshot boundary ahead of
preflight and restores both object-save mirrors, so any future `(2)` stack is
a real failure. Optional first-finder naming is not enabled for this initial
derelict.

## Fast HUNTED Bounty-Hunter Check

After installing and provisioning the current candidate, run:

```bash
./scripts/test_vessel_hunter_in_game.sh
```

This development-only check uses the existing master account and exact
level-34 character `Kohdee`; it does not create an account or character. It
refuses to run while a ferry soak or scale worker owns the server and requires
the running executable to match the current installed Phase 15 binary.

The script snapshots Kohdee's exact `vessel_bounties` and
`vessel_bounty_hunts` rows, temporarily sets the HUNTED threshold, and uses one
real Kohdee session to spawn and move the fixture raft. `vesseldebug encounter`
then advances only the cadence counter, so region, class, depth, chance,
eligibility, spawn, pilot, pursuit, and combat setup all execute through the
normal encounter path. SQL must prove exactly one ownerless warship with the
configured captain and target.

The script hard-restarts the local development service, returns Kohdee to the
target, and requires the exact same hunter generation, fleet slot, unique
name, prototype, pilot, and target attribution. It then pardons Kohdee, waits
for the normal periodic check, and requires a bounded cooldown plus complete
hunter runtime/interior/pilot cleanup without destroying the target. Its exit
trap purges the temporary target and restores the exact original bounty and
lifecycle rows. Artifacts remain under the printed
`/tmp/luminari-vessel-hunter-check-*/runs/` directory.

Do not perform those steps manually or log in once per assertion. The
automated path is designed to finish in one compact run; record its measured
elapsed time from the final PASS line.

The installed July 30 candidate passed this complete path in 64 seconds. Its
fixture raft requested speed 2 and proved effective speed 1 after the seaport
terrain modifier before the normal encounter check. The same hunter identity
survived the hard restart, pardon cleanup passed, the target survived, and the
script restored Kohdee's exact original rows.

## Bounded Vessel Ferry Validation

The entire supervised ferry gate, including setup, final restart, evidence
review, and cleanup, must finish within one hour. Use a 45-minute observation
to reserve 15 minutes for the terminal work:

```bash
./scripts/run_vessel_ferry_soak.sh start 2700 60 900
./scripts/run_vessel_ferry_soak.sh status
```

The script retains a historical longer default; do not invoke `start` without
the explicit bounded arguments above and do not launch a replacement
long-duration service. Before starting, use `status` to ensure no legacy ferry
monitor owns the development server. Preserve any old run directory only as
historical evidence.

The August 2 release run is terminal `PASS` under
`20260801T230546Z-160058`. The requested 2,700-second window recorded 1,476
movement steps, 246 arrivals, 62 route completions, five live checks, 46
database/process samples, and no buffer overflow. Its final hard restart
preserved the exact paused coordinates, route, rooms, pilot, schedule,
structure, and executable hash before Kohdee resumed the ferry. The complete
request-to-result task took 46 minutes 19 seconds. Re-run this gate only for a
relevant regression, using the bounded arguments above.

If the supervised local MUD is stopped, the monitor first runs the fast Kohdee
login smoke to create the service and its append-only log, then begins its
ownership checks. The bootstrap transcript is retained as
`server-bootstrap.log` in the run directory. An active service whose original
log is missing remains a hard failure because a replacement empty file cannot
recover output from the process's already-open descriptor.

The monitor refuses non-development environments, discovers the ferry and
route IDs instead of
assuming slot 5, repairs the ferry once before starting, and holds a
non-character connection in unconfirmed account-name state so the game loop
does not sleep between inspections. The generated hold name is never
confirmed, so no account is created. The monitor requires that socket to
remain `ESTABLISHED` every 20 seconds and fails if the server reports that it
went to sleep. Scheduled and staff copyovers drop non-playing descriptors by
design. When the server log proves copyover mode, the monitor requires the
same PID and installed executable, waits for boot, reconnects the hold socket,
and records the recovery count. A missing socket without copyover evidence is
still a hard failure. Live checks use the configured master account and
existing Kohdee character; they do not create an account or character.
The launch metadata records the source commit and SHA-256 of `bin/circle`.
Each process sample rejects a changed executable fingerprint, and the final
restart must launch the same SHA-256 that served the continuous window.

Runs started from the current script also issue `shiplist summary` and
`show stats` inside each actual-Kohdee sample. They require a constant fleet
count and dynamic-room capacity, reject any reported buffer overflow, and
write `live-system-samples.tsv` with fleet, dynamic-room, mobile, object,
room, allocation-list, movement-trail, buffer, movement-step,
waypoint-arrival, route-completion, and copyover-recovery counts. Each active
live interval must increase all three autopilot counters. Those counters have
process-executable lifetime and may restart at zero during same-PID `exec`
copyover. The monitor checks the keepalive before each live sample, starts a
new counter segment only after log-proven copyover recovery, requires positive
progress in that segment, and accumulates its deltas in the terminal summary.
The raw sample includes the recovery number that explains any counter reset.
The terminal summary also adds initial, maximum, and final dynamic-room,
world-list, and movement-trail values beside process RSS.

Current candidate builds add an exact `<count> movement trails` row to
`show stats`. This is a full-world count, not a vessel count. The default
world retains movement trails for 12,600 seconds and prunes them every 75
seconds, so correlate this field with anonymous/heap RSS before attributing
awake-world warmup to vessels. The runners invoke the scan only inside their
infrequent actual-character checkpoints.

The candidate build emits per-step movement, arrival, wait, and route-loop
messages only through compiled development debug categories. Normal builds use
the `autopilot status` counters instead. This keeps future fleet-soak logs
bounded without weakening the route-progress gate; the final restart is
allowed to reset the runtime counters after the pre-restart totals are saved.

Inspect process-memory trends without changing the running service:

```bash
run_dir=/path/printed/by/the/status/command
./scripts/analyze_vessel_memory_samples.sh \
  --warmup-seconds 900 \
  --windows 300,900,1800 \
  "$run_dir/process-samples.tsv"
```

The analyzer accepts both the active run's headerless series and newer
headered series. It rejects malformed metrics, non-increasing timestamps, or
a PID change, then reports consecutive block means and linear RSS/VSZ slopes
for the full, post-warmup, and requested trailing windows. Add `--format kv`
for stable `key=value` output. Its result is deliberately `REPORT_ONLY`; do
not treat a low-looking slope from the bounded window as proof that a
long-horizon leak cannot exist.
Run `./scripts/test_vessel_memory_analyzer.sh` after changing the analyzer.
Runs launched from the current ferry or scale scripts generate the same
machine-readable report automatically as `memory-analysis.kv` and fail if the
terminal process series is malformed. The manual command remains safe while a
run is active and supports alternate warmup and window choices.

Current ferry and scale runners also write `process-memory-details.tsv`. The
ferry aligns each detailed sample with an actual Kohdee checkpoint; the scale
runner samples at measurement start, every complete intermediate hour, and
measurement end. Each row records anonymous, file-backed, and shared RSS,
data and swap sizes, and the heap mapping's size, RSS, and private-dirty
pages. Validate a completed series with:

```bash
./scripts/sample_process_memory_details.sh \
  --validate "$run_dir/process-memory-details.tsv"
```

The validator rejects timestamp or PID drift, missing metrics, and impossible
RSS/heap relationships. Reading the pinned 1.1 GiB process's status and heap
mapping took about 0.01 seconds, so the scale runner samples sparsely instead
of scanning `smaps` inside its 30-second process loop. The abandoned July 30
pinned run predates this artifact. The ferry's series intentionally ends at
the final active checkpoint before its hard-restart recovery gate. That gate
verifies the replacement process by executable hash and exact gameplay state;
it does not mix a second PID into the continuous memory artifact.

At the end, the monitor uses Kohdee to pause the ferry, verifies that the exact
coordinates and route were committed, hard-restarts the local service, checks
the recovered state and executable hash, and resumes the ferry. Results and
every sample remain under the run directory printed by `start`. Only `PASS`
from `status` closes the gate.

For a short monitor shakedown:

```bash
./scripts/run_vessel_ferry_soak.sh start 150 10 60
```

The three values are duration, database/process interval, and live-character
interval in seconds. Keep the shakedown longer than the server's login timeout
so it proves the hold connection rather than relying on periodic character
logins to wake a sleeping loop. The corrected July 30, 2026 shakedown ran 150
continuous seconds with 84 movement steps, 22 distinct cells, both docks, 5
Kohdee samples, 16 database/process samples, an unchanged travel PID, exact
state recovery across the final restart, and automatic resume.
A separate provenance shakedown proved the same executable SHA-256 before and
after restart. Deliberate SIGTERM then produced an immediate terminal `FAIL`
artifact, so an interrupted agent or service does not leave a false `RUNNING`
result.

The first pinned 24-hour attempt reached 34,382 seconds, 18,720 movement
steps, 780 arrivals at each dock, 10 actual Kohdee checks, and 574
database/process checks with one stable MUD PID. At 11:00:30 IDT, the server's
normal automated copyover discarded the unauthenticated hold descriptor while
preserving the ferry and process. The old monitor misclassified that expected
handoff as a dead keepalive and then failed to finalize its status, so the run
is `ABANDONED`, not continuity evidence. The current monitor fixes both
harness defects. This is historical evidence; do not restart the retired
long-duration gate.

The current forced-copyover shakedown used:

```bash
./scripts/run_vessel_ferry_soak.sh start 240 10 120
./scripts/dev_kohdee_login_smoke.sh --copyover-check "shiplist summary"
./scripts/run_vessel_ferry_soak.sh status
```

It passed on July 30, 2026 with one same-PID copyover recovery, 132 movement
steps, 22 waypoint arrivals, five route completions, four live checks, 25
database/process samples, zero buffer overflows, a valid continuous
detailed-memory series, and exact-state recovery and resume after the final
hard restart. Use the short form only to verify harness changes; it does not
close the bounded 45-minute release gate.

## Fast Post-Validation Finish

Do not repeat the component gates manually. After the ferry status reports
terminal `PASS`, install the exact current clean candidate and start the
consolidated runner:

```bash
./scripts/run_vessel_ferry_soak.sh status
make test
valgrind --tool=memcheck --leak-check=full \
  --show-leak-kinds=definite,indirect \
  --errors-for-leak-kinds=definite,indirect \
  --error-exitcode=99 ./cutest
make install
./scripts/run_vessel_scale_benchmark.sh start 1800
./scripts/run_vessel_scale_benchmark.sh status
```

Repeat only the final `status` command until it reports terminal `PASS` or
`FAIL`. The scale worker runs the shared harbor provisioner itself, so that one
run captures fare collection and restoration, named-water crossing,
same-account two-character `shiptalk`, builder-created fleet capacity,
surface/air/submarine Z boundaries, shared multi-ship encounter delivery, the
1,000-trade economy simulation, native MSDP state and clearing, live message
suppression, schedules, memory samples, SQL volume, and the complete 500-ship
tick profile. It then restores the pre-run database. Running the standalone
harbor, channel, economy, or MSDP commands first only duplicates work.

The current suite result is 268 of 268. The preceding pre-Phase15 Memcheck
gate reported zero errors and zero definite, indirect, or possible loss;
repeat the command above for the current candidate. Reachable process-lifetime
registries and profiler buffers remain reported but are not classified as
lost. `make test` may leave a root-level `circle` while it builds the
production-linked suite. The required `make install` installs `bin/circle` and
removes that root artifact before the runner records provenance. Root
`make test` and CMake/CTest now also run the vessel memory analyzer, detailed
process-memory, and scale-parser regressions; the release manifest includes
the login, ferry, scale, memory, and supporting vessel test scripts. It also
includes the complete harbor world fixture, vessel docs, authoritative help
data, master schema, and every Phase 2-15 install/verify/rollback input needed
to reproduce this guide from a packaged source tree.

## Reproducible 500-Vessel Scale Gate

After the bounded ferry validation passes and the current clean source is
built and installed, launch the development-only scale gate with:

```bash
./scripts/run_vessel_scale_benchmark.sh start 1800
./scripts/run_vessel_scale_benchmark.sh status
```

The August 2 first current-candidate attempt is a retained premeasurement
`FAIL` at
`/tmp/luminari-vessel-scale-benchmark-1000/runs/20260802T003029Z-328201`.
It reached all 500 active slots and passed harbor, channel, economy, and live
workload setup, but the airship had descended from its seeded Z 500 to 480
before Kohdee issued the ceiling command. The valid 480 to 490 climb therefore
did not produce the expected rejection. The runner now persists that boundary
airship paused at Z 500 until the command, then resumes its route for measured
Z variation. Cleanup restored the six-vessel baseline and restarted the exact
installed candidate. This is harness-regression evidence only; start a fresh
1,800-second run for the release result.

Retry `20260802T004119Z-349856` proved the corrected Z-500 rejection, again
reached 500 of 500 in game, and then stopped before profiler reset because its
boot-log slice began at an offset carried from the pre-restart file. The login
helper truncates that file on restart, so the runner now reads the known-fresh
log from byte zero. The retry also exposed output overflow when generic checks
returned Kohdee to the harbor among hundreds of hulls. Generic ashore, MSDP,
message, and terminal transitions now use quiet staff room 1204; actual harbor
crossing and channel checks still use rooms 1000389/1000390. Cleanup again
restored the exact six-vessel baseline and candidate. This second artifact is
also harness-regression evidence only.

Launch `20260802T005623Z-378533` stopped during harbor preflight before fleet
creation when a west-coordinate disembarkation did not share the canonical
room containing the ferry hull. Ordinary `board ferry` correctly refused
proximity, so no fare was charged. The gate now waits for the west dock,
pauses and stops the ferry, disembarks, resolves static room 1000389, and then
boards normally. A standalone provisioner run passed the exact 10-gold charge
and restoration plus crossing/channel checks after this change. The tooling
test locks that command order. This artifact contains no scale measurement.

Run `20260802T010309Z-392860` passed every repaired setup stage and reached the
reciprocal-combat helper. Kohdee observed repeated return fire and the server
reported 393 suppressed messages, but LF-CR Telnet line endings left a leading
carriage return before the CSV counter and the helper rejected it. The common
output cleaner now strips only that leading CR and preserves indentation. Its
short 18-tick profile also showed 226,912 usec maximum vessel time and 26
missed pulses; treat that as a performance warning, not the required steady
sample. Cleanup restored the baseline.

Run `20260802T011448Z-414722` then passed those repairs, all 500 actual-Kohdee
spawns, economy and Z checks, fresh reconstruction, reciprocal fire, and 18
suppressed messages. It stopped before steady measurement because its native
MSDP connection used a cooked pseudo-terminal: the server showed `MSDP: Yes`,
but the binary REPORT control bytes were buffered and caret-echoed. The helper
now runs only that connection raw with echo disabled, explicitly declines
TTYPE and accepts MSDP, and validates the effective cached ashore state when a
value was already zero. A seven-second Kohdee check against baseline ship 1
received all nine values aboard and proved all nine neutral ashore. A second
eight-second check paused actively navigating ferry slot 5 at `(-63, 82)`,
received the contract, resumed it, and cleared ashore state. Tooling locks
negotiation, last-frame selection, pause/resume, and the clear-state contract.
Cleanup restored six vessels and restarted the exact candidate on PID 431693.
This artifact also contains no steady performance sample.

Run `20260802T013644Z-457615` passed every component gate and completed the
full 1,800-second measurement with 500 vessels on one PID. It then failed the
game-side invariant because the reconstruction login inherited Kohdee's
post-spawn harbor room and rendered hundreds of hulls before its first command,
creating one `**OVERFLOW**`. Save Kohdee in room 1204 before the workload
restart. The complete diagnostic profile also fails the performance target:
3,676 ticks had median 764.50 usec, p95 130,928.50, p99 166,398.50, maximum
1,027,228, and 6,157 missed pulses. Crew wages peaked at 1,014,543 usec,
autopilot at 213,780, encounters at 149,653, and schedules at 24,889. The
memory series is `REPORT_ONLY`: RSS rose 786,784 to 854,412 KiB across 1,861
seconds while movement trails rose 30,426 to 289,000; threads stayed 2 and
descriptors 11-12. Cleanup restored six vessels and the exact binary on PID
522541. Fix the three production hotspots before starting the next full run.

The next candidate bounded those hotspots for the seventh launch. Payroll
uses 100 stable batches, so no more than five of the 500 slots become payable
on one tick, and a changed roster uses one multi-row insert after its delete.
Encounter containment is cached by shared exterior room for each pass.
Autopilot reuses released dynamic rooms whose coordinate, region, path, and
terrain metadata are already valid. The spawn command also saves Kohdee in
room 1204 before reconstruction. `make test` passes 271 of 271 without
warnings, all vessel tooling passes, and strict actionable Memcheck reports
zero errors and no definite, indirect, or possible loss. `make install`
removed the root artifact; PID 565375 maps the installed SHA-256 above. An
actual Kohdee smoke reported six of 500 slots and returned to room 1204.

Run `20260802T024352Z-573327` then completed the full 1,800-second window with
500 vessels on PID 582492. It retained zero game-side overflows at both live
checkpoints, accepting the room-1204 repair. Its first terminal failure was a
test-helper defect: generic `@wait` deliberately drained all asynchronous
socket data, so the transcript omitted an encounter that the server recorded
20 times for Kohdee's airship. The server log also contains 225 real scheduled
route failures at non-navigable intermediate cells. The six full-fleet spawn
messages came from baseline NPC merchant prototype 7 reconciliation, not the
hunter path; capacity correctly defers that merchant until a slot is free.
Repair the route fixture before treating the encounter failure as the only
verdict.

The profile still fails 25 ms: 3,665 vessel ticks reported median 802.00 usec,
p95 131,989.20, p99 176,272.80, maximum 355,394, and 6,217 missed pulses.
Queries fell to 67,052 and payroll p95 to 9,146.80 usec, but autopilot p95 was
130,774, payroll max 353,062, and encounters max 60,540. RSS rose 786,296 to
853,480 KiB over 1,854 seconds and the analyzer remains `REPORT_ONLY`.
Cleanup restored six vessels; PID 640439 maps the same installed SHA-256. A
fresh full gate remains required after the helper, workload, performance, and
memory fixes.

The next installed repair candidate keeps generic `@wait` output, including
unsolicited encounter messages, and its parser regression rejects any return
to the old discard handler. The schedule plus all water-class scale fixtures
now stay on `y = 82`; an actual Kohdee terrain probe verified every coordinate
from `(-66, 82)` through `(-62, 82)` before the change. Full-fleet merchant
reconciliation is logged as expected deferred work rather than a server error.
The remaining synchronous paths are bounded: regional piracy law uses a
4,096-entry coordinate cache, encounter polygons resolve from the canonical
in-memory region table, and up to five payroll walkoffs share one delete.
Movement trails refresh duplicate signatures and retain at most 16 per room.
The warning-free build, scale tooling, and all 274 production-linked tests
pass; strict actionable Memcheck is also clean across all 274 tests. Required
`make install` removed the root artifact and installed SHA-256
`0e79d6edb09be793d293ac31dee4aa42860368c4381659861309ce1d4bec3021`.
Commit `d610d58a` is pushed, and PID 714795 maps that exact installed hash. An
actual Kohdee smoke entered and logged out cleanly in 17 seconds; a second
session reported 6 of 500 fleet slots, 8 of 2,000 dynamic rooms, and saved
Kohdee in room 1204. This is baseline live acceptance, not the required scale
result.

The repaired 600-second diagnostic is retained under
`/tmp/luminari-vessel-scale-benchmark-1000/runs/20260802T035823Z-718533`.
All actual-Kohdee setup and gameplay checks passed. The run created all 500
ships, retained unsolicited scheduled-route and encounter messages through
generic `@wait`, observed six shared encounters, exercised ten departures and
nine live airship Z values, and produced zero route failures, workload errors,
high-volume progress logs, or buffer overflows. Cleanup restored the exact
six-vessel database baseline and restarted local development. The repaired
harness and fixture behavior are accepted.

Do not treat that result as a scale pass. Its 1,217 vessel ticks had median
659 usec, p95 66,429, p99 86,597.80, maximum 103,801, and 2,150 missed pulses.
Autopilot p95 was 66,286.60 usec and one encounter reached 56,901 usec. The
631-second process series grew from 786,304 to 807,504 KiB RSS while movement
trails grew from 21,472 to 68,895; memory remains `REPORT_ONLY`. Fix trail
growth and the remaining synchronous paths, then rebuild, install, and run the
full 1,800-second gate.

The subsequent memory candidate retains player footprints but skips trail
allocation for ordinary NPC movement. Source tracing confirms that the live
trail lists currently have no gameplay reader; staff allocation reports,
player rename, cleanup, and room teardown are their only consumers. The
production-linked regression moves both an NPC and a player: only the player
adds a record, and the root suite binary passes 275 of 275. Recheck the live
trail row in the next installed 500-vessel run; do not close memory from the
unit result alone.

Diagnostic profile run `20260802T043120Z-786413` passed all live gates for 500
vessels and held movement trails at exactly 0 from start through finish. The
631-second world still added 669 mobiles and 125 objects while RSS rose 15,384
KiB, so whole-world memory remains `REPORT_ONLY`; only the trail source is
accepted. The installed binary used `-pg`, so its p95 54,790.75-usec and
119,478-usec maximum are not release timing results.

The preserved call graph attributes 14,379 of 14,926 runtime saves to port-
berth transitions on public ships with no fee state. That accounts for nearly
all 14,938 measured database executions and explains the synchronized
autopilot stalls. Encounter ticks also made 3,528 synchronous queries. Rebuild
normally only after fixing those two paths; do not reuse the profiling hash
for release acceptance.

The normal candidate now suppresses empty berth transitions and loads at most
1,024 encounter definitions plus optional hunter policies into memory after
schema boot. Ordinary encounter heartbeats no longer query MariaDB; a staff-
forced check deliberately reloads the cache first. The release build is
warning-free, all 277 production-linked tests and 13 protocol-parser tests
pass, and `make install` produced non-profiled SHA-256
`281c7469702fbbeaa52f40a916a3911b121d3cfa9bd1050ed9feb4f1bad92075` while
removing the root artifact. Confirm the boot-cache log through an actual
Kohdee smoke, then use a clean committed tree for the next scale run.

That smoke now passes in 22 seconds. The newly started development process is
PID 850894, `/proc` and `bin/circle` both hash to the installed value above,
and boot logs `Loaded 1 vessel encounter definition into memory.` with no
vessel or encounter `SYSERR`. Kohdee entered room 1204, saw 6 of 500 ships,
5 of 2,000 dynamic rooms, zero movement trails, and zero overflows, then
logged out cleanly. Commit this evidence before invoking the scale runner,
which requires a clean tree.

The committed candidate then passes normal 600-second diagnostic
`20260802T050422Z-854067` with 500 vessels. Its 1,221 complete ticks have
median 577 usec, p95 1,524, p99 2,011.60, and maximum 2,915; database
executions fall to 1,295 from 14,942 in the prior release diagnostic.
Autopilot peaks at 2,785 usec, encounters at 232, and schedules at 17,537.
All actual-Kohdee workload gates pass with ten departures, six shared
encounters, nine Z values, no workload or high-volume log errors, no
overflows, and trails 0/0/0. The 14,888-KiB RSS increase remains
`REPORT_ONLY` with 660 new mobiles and 131 objects. Cleanup restores the exact
six-vessel baseline and restarts local development. Use the same command with
the required 1,800-second duration for final scale acceptance.

That final scale acceptance now passes at
`/tmp/luminari-vessel-scale-benchmark-1000/runs/20260802T052407Z-896082`.
The complete task finishes in 2,338 seconds, including 1,862 measured seconds
with 500 vessels on one PID. Its 3,655 ticks have median 599 usec, p95 4,079,
p99 5,169.06, and maximum 10,520; every subsystem maximum remains below
25 ms. The run records 20 shared encounters, ten departures, 12 Z values,
23,181 suppressed messages, 4,247 database executions, zero workload errors,
zero high-volume rows, zero overflows, and trails 0/0/0. RSS rises 33,148 KiB
alongside 1,613 mobiles and 451 objects with constant rooms, so the bounded
memory result remains `REPORT_ONLY`. Cleanup restores six baseline vessels and
restarts development. Strict Memcheck then passes all 277 production-linked
tests with zero errors and zero definite, indirect, or possible loss; required
`make install` preserves the installed hash and removes the root artifact.

The default steady measurement window is 660 seconds. The runner accepts an
explicit value from 600 through 7200 seconds, but this plan permits at most
1,800 seconds so the full setup, measurement, result, and restoration process
can finish within one hour. The command returns immediately after launching a
supervised user service; use `status` for progress.

The held Kohdee session records a timestamped game-side allocation checkpoint
at measurement start, every hour, and at the end. The worker writes these to
`live-system-samples.tsv` and rejects fleet-count drift, dynamic-room-capacity
drift, dynamic occupancy above capacity, or any reported buffer overflow.
Each checkpoint also records the exact live full-world movement-trail count,
and the terminal summary reports its initial, maximum, and final values.
`process-samples.tsv` has a header and records epoch, PID, RSS, VSZ, thread
count, and file-descriptor count every 30 seconds. The worker also rejects a
PID change or replacement of the installed executable during measurement.
The separate sparse `process-memory-details.tsv` series adds anonymous,
file-backed, and shared RSS, data, swap, and heap-mapping measurements at
start, every complete intermediate hour, and end without scanning `smaps` in
the 30-second loop.
The terminal summary includes initial/maximum/final values for these series.
Use the same memory analyzer above on this file so its shape is directly
comparable with the bounded ferry series. The terminal worker
also writes the default report to `memory-analysis.kv`. Workload reconstruction
checks read only the log bytes written by that boot, so an old success or error
cannot affect the verdict. The measured log is preserved separately, its byte
count is reported, and any normal-build per-step, arrival, wait, or route-loop
progress row fails the gate. Former unconditional wilderness region, sector,
elevation, and path progress rows fail it as well. Run
`./scripts/test_vessel_scale_benchmark_parsers.sh` after changing these
parsers. The test rejects missing or inverted tick percentiles and impossible
sample counts; a live result must contain an ordered median, p95, p99, and
maximum in the complete ten-field `vessel_tick` row. The test also covers
checkpoint chronology: the series must start at
`system-0`, use strictly increasing epochs and labels, retain hourly
intermediate labels, and finish at the exact requested measurement duration.

Do not use the runner's larger technical range to create a longer gate. For the
post-benchmark stability task, pass `1800` explicitly and stop and clean up if
the one-hour total execution ceiling approaches. The candidate has removed
normal-build per-movement log volume; confirm actual log growth during the
bounded 500-ship run.

The runner reuses the configured master account and exact `Kohdee` character
for every fleet phase. Its harbor preflight may add the one reusable
`Vesselmate` character to that same account for the channel proof, but it never
creates another account or one character per vessel. One Kohdee session
creates all missing public hulls with `vedit spawnpublic`; later sessions on
that same account verify the reconstructed workload, warm it, collect
`perfmon csv`, and leave Kohdee in quiet staff room 1204.

The runner refuses production, a dirty source worktree, an active ferry soak,
an installed binary older than any current C source, header, or primary build
file, or stale benchmark data. A stale-build refusal names the first newer
input and requires `make test` followed by `make install`. Before mutation it takes
an atomic snapshot of every vessel/economy table it can change. It then fills
active slots 1-500 across all eight vessel classes, configures routes, pilots,
crew, schedules, cargo, weapons, encounters, wear, and economy state, and
holds Kohdee aboard an airship so the normal MSDP path runs. Before timing,
Kohdee proves that a surface hull cannot leave Z 0, an airship cannot exceed
Z 500, and a submarine cannot cross above the waterline. The air route then
changes altitude between Z 120 and Z 220. Minute-by-minute `shipstatus`
samples must contain at least two distinct Z values inside the airship's
class bounds. Automated steps resolve and validate each target wilderness room
once in the central position update, avoiding the old duplicate spatial-query
preflight for otherwise unoccupied target coordinates.

The 100-percent benchmark encounter is attached to the air route's real
`REGION_ENCOUNTER`. Co-located airships share their exterior wilderness room;
the run requires a live encounter that notifies multiple hulls once and
requires Kohdee to receive its arrival message. Ten scheduled ships are also
staged to depart inside the measured window. `shiplist summary` proves the
live fleet count without overflowing the MUD's socket output buffer. A paused
reciprocal submarine pair continuously fires two synchronized, one-damage
weapons; its fixture defense speed is above the NPC attack ceiling, so no shot
can land, and its negative Z excludes surface weather. This makes message
suppression deterministic after the profiler reset even during the longest
supported measurement. Kohdee's `perfmon csv` transcript must report a
nonzero `vessel_messages_throttled` count. The runner discovers that fixture
slot, invokes `--vessel-message-check` automatically, and preserves Kohdee's
observed return-fire traffic plus counter in
`vessel-message-throttling.log`. Do not guess the slot or add a separate
manual login.

Whether the result passes, fails, or receives SIGTERM, the worker stops the
local MUD, restores the snapshot, verifies that benchmark marker rows are
gone, restarts development, and returns Kohdee to the static room. If an
interrupted run needs operator recovery, use:

```bash
./scripts/run_vessel_scale_benchmark.sh cleanup
```

Do not start this gate while any ferry monitor is active. The scale runner
intentionally refuses to disturb the process and executable owned by that
run.

## Fast 1,000-Trade Economy Gate

After the current source is installed on local development, reuse the master
account and exact Kohdee character for the sustained-market proof:

```bash
./scripts/dev_kohdee_login_smoke.sh --commands "vtradecheck 1000"
```

The abandoned July 30 pinned executable came from source `0afad17b`, while
`vtradecheck` was added later in `ac418322`; a safe historical probe therefore
returned `Huh?!` and logged out cleanly. Install the current candidate before
using the single command above.

This staff diagnostic runs the production batch-pricing and supply functions
without changing Kohdee's gold, cargo, or the live port tables. It executes
1,000 deliberately oversized transfers that alternate direction, checks the
10-400 inventory bounds, follows a legitimate profitable route until its
price gap closes, and restocks both simulated ports to 100. The reversal
profit must be negative; a positive value reproduces the old unlimited
bulk-quote exploit. Success includes:

```text
Vessel economy simulation: PASS
  Trades executed: 1000/1000
  Supply range: 10..400 (hard bounds 10..400)
  Adversarial reversal profit: -... gold (must be <= 0)
  Restock convergence: 100/100 (baseline 100)
```

The 500-vessel runner performs and records this same Kohdee command
automatically. Do not run either path while a ferry soak or scale worker owns
the installed server.

## Fast Ship-Wide Channel Gate

Run this only when no ferry soak or scale worker owns the installed build:

```bash
./scripts/dev_kohdee_login_smoke.sh \
  --vessel-channel-check <ship-slot>
```

The helper opens two simultaneous local connections, authenticates both with
the configured master account, selects `Kohdee`, and automatically selects the
first other usable character Name from that same account. It ignores deleted
rows and never assumes a stable menu slot. To require a particular existing
character instead, append its exact Name after the ship slot. It keeps both
descriptors open while it:

1. Moves Kohdee to the requested vessel and transfers the second character
   aboard.
2. Moves Kohdee into a different interior room.
3. Sends a unique `shiptalk` marker from each character and requires the other
   socket to receive the vessel name, speaker name, and exact marker.
4. Moves Kohdee ashore, sends another aboard marker, and requires Kohdee's
   socket to remain quiet.
5. Requires the aboard-only refusal from both characters after bringing the
   second character ashore.
6. Cleanly leaves both characters and both account sessions.

One helper process owns both sockets and the normal login lock, so no manual
character lookup, client synchronization, or second helper process is needed.
This is the preferred release transcript.

If the master account has no second usable character, add one to that same
account first:

```bash
./scripts/dev_create_test_character.sh Vesselmate
```

The one-argument creation form uses the master account. Do not create a second
account for this check. The shared harbor provisioner handles a missing second
character automatically; use the command above only when running the
standalone channel gate first. The gate discovers `Vesselmate` automatically,
so never pass or look up its account-menu slot.

## Fast Native MSDP Vessel-State Gate

After the current source is installed on local development and a vessel slot
exists, run:

```bash
./scripts/dev_kohdee_login_smoke.sh --vessel-msdp-check <ship-slot>
```

The helper reuses the master account and exact Kohdee character. It runs the
socket on a raw no-echo pseudo-terminal, declines the server's initial TTYPE
request, accepts native MSDP option 69, and teleports Kohdee aboard the
requested vessel. It requests all nine `SHIP_*` variables and compares the
position and navigation frames with `shipstatus`. It also validates the hull
totals and status band.
Kohdee then goes to static staff room 1204, and the helper requires the two
string variables to clear and all seven numeric variables to become zero
before it logs out of the character and account. MSDP sends only dirty values;
if an ashore frame is omitted, the helper accepts it only when the prior
aboard value was already the required neutral value.

The 500-vessel runner performs this check automatically against its benchmark
airship and preserves `native-msdp-vessel-state.log`. This is a real
Telnet-option-69 exchange; merely leaving Kohdee aboard while the server calls
the setters does not prove client delivery. Do not run this path while a ferry
soak or scale worker owns the installed server.

## Fast Vessel Builder Gate

Use one logged-in Kohdee session to exercise the complete no-C builder path:

```bash
./scripts/dev_kohdee_login_smoke.sh --vessel-builder-check
```

The helper reads `vedit` usage, moves to the shared harbor, creates a uniquely
named Boat prototype, tunes and shows it, spawns it with `vedit`, discovers the
returned prototype ID and fleet slot, and verifies that the generated hull
sails from `(-66, 92)` to `(-67, 92)`. It then stops and purges the temporary
hull, deletes the temporary prototype, and confirms the prototype is gone.
Creation, editing, and spawning use only the builder-facing `vedit` command;
staff commands are used afterward for sailing verification and cleanup.

Do not split this workflow across logins or query MySQL for generated IDs. The
helper derives both IDs from actual game output in the same connection. The
August 2, 2026 current-candidate run completed the in-game workflow in 2.8
seconds and the entire login, test, cleanup, character logout, and account
logout in 8 seconds. It used temporary prototype 13 and ship 7, reached
effective speed 1 in storm, and removed both records.
Success ends with:

```text
PASS: builder created, tuned, spawned, and sailed a vessel in N.N seconds.
PASS: temporary ship N and prototype N were removed.
PASS: Kohdee completed the vessel builder check and logged out cleanly (Ns total).
```

## Fast Command Batches

Do not repeat the login flow for every test command. Pass a whole sequence as
single-line arguments to `--commands`:

```bash
./scripts/dev_kohdee_login_smoke.sh --commands \
  "goto -66 92" \
  "board gull" \
  "south" \
  "speed 5" \
  "shipstatus"
```

The helper logs in once, runs each command as `Kohdee`, prints the output under
`>>> command` headings, then leaves the character and account cleanly. A
one-command batch takes about 4-6 seconds against a running server; an
18-command vessel batch takes about 20-25 seconds.

Each command is followed by a unique in-game completion marker, so the next
command is not mistaken for delayed output from the previous one. The marker
uses ordinary local speech so it also works for level-1 test characters; run
this helper only on development. The final `PASS` confirms command delivery and
clean logout, not that the gameplay output was correct. Read each command's
output and compare it with the relevant test guide.

For wilderness tests, prefer stable coordinates such as `goto -66 92`.
Rooms 1000389 and 1000390 are static harbor fixtures after provisioning.
Other runtime wilderness VNUMs are pool allocations and can legitimately
change or disappear after reboot.

If a later command depends on an ID printed by an earlier command, run the
smallest useful first batch, capture the ID, and put the remaining commands in
one second batch. This should be the exception, not one login per command.

`@wait N` is a helper-side pause of 1-60 seconds and is not sent to the game.
While paused, the helper continuously drains server output so a long
character observation cannot fill the client pipe or MUD socket buffer. Use
it only to synchronize two local character sessions or wait for a real
heartbeat/reload interval:

```bash
./scripts/dev_kohdee_login_smoke.sh --commands \
  "@wait 2" \
  "trans Testcaptain"
```

## Alternate Local Test Characters

The same fast path can enter another character by exact name. This is intended
for disposable lifecycle and multiplayer fixtures, not production accounts:

```bash
./scripts/dev_create_test_character.sh Testcaptain

DEV_MUD_CHARACTER=Testcaptain \
./scripts/dev_kohdee_login_smoke.sh --commands \
  "score" \
  "look"
```

The creation helper refuses non-development environments, boots or reuses the
local MUD through the established Kohdee preflight, logs into the configured
master account, creates the default human warrior through that account's `C`
option, enters the world once, and logs out cleanly.

An account can contain multiple characters. Do not create one account per
character for ordinary multiplayer testing. Keep fixtures on the master
account and use `DEV_MUD_CHARACTER` to select each exact Name. The login helper
never assumes a stable menu slot.

Only account-isolation or destructive account/deletion tests warrant another
account. For those cases, pass the account explicitly:

```bash
./scripts/dev_create_test_character.sh localtestaccount Testcaptain
```

The same helper logs into that account if it exists or bootstraps it if it
does not.

The password comes from `DEV_MUD_ACCOUNT_PASSWORD` when set; otherwise the
helper uses `GAME_MASTER_ACCOUNT_PASSWORD` from `lib/.env`. This allows local
test accounts deliberately created with the development master password to be
used without putting that password in shell arguments or logs. The default
with no overrides remains the exact level-34 `Kohdee` path.

The helper validates the alternate name, finds exactly one matching Name
column, uses non-staff completion markers, and performs the same clean
character/account logout. It also reports a soft-deleted selection immediately
instead of waiting for the normal login timeout. Keep one entire command
sequence in one invocation.

For menu-driven editors, use one `--dialog` invocation. Each argument is one
input line; the helper confirms that the final input returned to normal command
mode before it logs out:

```bash
./scripts/dev_kohdee_login_smoke.sh --dialog \
  "cedit" "e" "l" "1" "q" "q" "y"
```

This is intended for deterministic menu sequences such as `cedit`. End the
sequence by saving or cancelling out of the editor. If it is still inside an
editor, the helper fails instead of silently reporting success.

For the vessel option, choice `1` is `Off` and choice `2` is `On`. A kill-switch
test must use a second dialog with `2` before cleanup; confirm
`lib/etc/config` ends with `vessel_system = 1`. Do not leave development
disabled for the next agent.

## Fast Copyover Check

Use `--copyover-check` instead of logging in manually, issuing `copyover`, and
reconnecting. The helper keeps Kohdee's descriptor open across the process
replacement, requires the server's recovery confirmation, runs every supplied
command after recovery, and then logs out cleanly:

```bash
./scripts/dev_kohdee_login_smoke.sh --copyover-check \
  "shiplist" \
  "shipgoto 2" \
  "shipstatus"
```

Set up any required pre-copyover state in one earlier `--commands` batch. Keep
all post-copyover verification in the single command above. A copyover includes
the normal development-data boot time, but does not require a second account
login or a hardcoded character slot.

When timing matters, place pre-copyover commands before a standalone `--`. The
helper runs them, starts copyover immediately on the same connection, and runs
the remaining commands only after recovery:

```bash
./scripts/dev_kohdee_login_smoke.sh --copyover-check \
  "shipgoto 3" \
  "autopilot on" \
  -- \
  "autopilot status" \
  "shipstatus"
```

## Pager-Safe Help Checks

Do not use `--commands "help ..."` for a long help entry: the MUD pager can
consume the private completion marker as pager input. Use `--help-check`
instead:

```bash
./scripts/dev_kohdee_login_smoke.sh --help-check \
  delroute vesseldebug loadvehicle
```

For the exhaustive vessel release check, use the single-command form:

```bash
./scripts/dev_kohdee_login_smoke.sh --vessel-help-check
```

It derives every command carrying `CMD_FEATURE_VESSEL` directly from
`src/interpreter.c`, adds the intentionally ungated boarding and staff recovery
commands, and verifies the resulting set in one Kohdee login. The current
source derives 78 keywords. On August 2, 2026 the first sweep correctly found
that the development database had not yet received the tracked `SHIPTALK`
mapping. Applying `sql/components/help_vessel_entries.sql`, running
`sql/components/verify_help_vessel_entries.sql`, and restarting the MUD made
all five SQL checks pass and all 78 keywords resolve in one 41-second Kohdee
session. This replaces one login cycle per keyword and automatically includes
newly gated commands.

Use the remainder of this document only to diagnose a failed fast-path run or
to perform the process manually.

## Fast-Path Preconditions

- Run from the repository root.
- `lib/.env` must say `APP_ENV=development`.
- `GAME_MASTER_ACCOUNT` and `GAME_MASTER_ACCOUNT_PASSWORD` must be set in
  `lib/.env`. Never print, log, or copy their values into a command or document.
- MariaDB must be active.
- `bin/circle` must exist and be executable. Build and install first if it does
  not.
- `expect`, `nc`, `ss`, `systemctl`, and `systemd-run` must be installed.
- The helper reads `DFLT_PORT` from `lib/etc/config`; it is currently 4100.

The helper checks all of these automatically.

## Manual Fallback

Start the server:

```bash
./bin/circle -d lib
```

Wait for `Entering game loop.`, then connect from a second terminal:

```bash
nc 127.0.0.1 4100
```

Then follow this sequence:

1. At the account-name prompt, enter the value of `GAME_MASTER_ACCOUNT`.
2. At `Password:`, enter `GAME_MASTER_ACCOUNT_PASSWORD`.
3. At the account menu, choose the numbered row whose Name column is exactly
   `Kohdee`.
4. At `PRESS RETURN`, press Enter.
5. At the character menu, enter `1` to enter the game.
6. Confirm that the welcome message and room display appear.
7. Enter `quit` to leave the game world for the character menu.
8. At the character menu, enter `0` to return to the account menu.
9. At the account menu, enter `Q`.
10. Confirm `Quitting.` and close the local client if it does not exit by
    itself.

The important state path is:

```text
account menu -> Kohdee row -> character menu -> 1 -> game world
game world -> quit -> character menu -> 0 -> account menu -> Q
```

Do not hardcode the account-menu character number. `load_account_characters()`
currently loads `player_data` without an `ORDER BY`, so slot order is not a
stable contract. Locate `Kohdee` in the Name column on every run.

## Troubleshooting Notes

- Boot takes roughly 15-20 seconds on this development data set.
- Existing world-data warnings, a missing local Ollama model, and an
  unavailable local I3 gateway did not block the verified run.
- The helper suppresses terminal output so neither credential can appear in
  logs.
- It strips ANSI color before matching the exact `Kohdee` Name column.
- It accepts both the configured and compiled-fallback welcome messages.
- It explicitly closes `nc` after `Quitting.` because this local `nc` can keep
  waiting after the server removes the account connection.

Useful server-side confirmation is:

```text
<character> has connected.
<character> ... entering game.
<character> has quit the game.
Losing player: <null>.
No connections.  Going to sleep.
```

If started manually, stop the foreground server with `Ctrl-C`. If the helper
started it, stop the supervised development server with:

```bash
systemctl --user stop luminari-dev-login-smoke.service
```
