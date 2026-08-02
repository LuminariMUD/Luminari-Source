# Vessel System Remaining Work

**Last audited:** August 2, 2026

**Status:** Mechanics through Phase 17, the first Luminari campaign shipping
package, the first data/DG-driven derelict, and the first wilderness frontier
package are implemented. Regattas, fleet skirmishes, ghost-fleet events, and
durable leaderboards also pass actual-character acceptance. The legacy
tactical grid has been replaced by a canonical wilderness chart, including
live contact damage state, and passes its reversible actual-character gate.
The lookout view now samples the canonical wilderness in eight bearings,
reports the visible horizon and nearby hulls, and passes its reversible
actual-character gate.
Dynamic at-sea descriptions now combine vessel class, speed, depth, the raw
wilderness weather field, and deterministic geographic `region_hints`.
Occupied moving hulls also receive throttled ambient prose, and both paths
pass a reversible actual-character gate.
Hostile boarding now uses a dedicated trainable Boarding ability and separate
opposed grapple and crossing checks. Its reversible two-character gate proves
both rejection and breach with actual player defenders.
The core development release gates for build, regression, Memcheck, bounded
ferry recovery, 500-vessel
performance/stability, economy simulation, shared encounters, Z-axis
boundaries, native MSDP, named-water crossing, captain-channel isolation, and
message throttling pass. The installed development candidate is not approved
for production until the remaining player experience, balance, beta,
production-snapshot rehearsal, preflight, and staged rollout work below is
complete.

**Current release checkpoint (August 2, 2026, 09:04 IDT):** Full run
`/tmp/luminari-vessel-scale-benchmark-1000/runs/20260802T052407Z-896082`
is terminal `PASS`. Its complete request-to-cleanup task took 2,338 seconds,
within the one-hour ceiling, including 1,862 seconds of measurement with 500
ships and all eight classes on one PID. Across 3,655 production ticks,
median/p95/p99/maximum were 599/4,079/5,169.06/10,520 usec. Every subsystem
maximum remained below 25 ms. The run recorded 20 shared multi-ship
encounters, 10 scheduled departures, 12 airship Z values from 128 through 210,
23,181 suppressed messages, 4,247 database executions, zero workload errors,
zero high-volume progress rows, zero buffer overflows, and movement trails
0/0/0. Cleanup restored all six baseline vessel rows and restarted local
development.

The 63-sample process series retained two threads and 11-13 descriptors. RSS
rose 783,032 to 816,180 KiB while the awake world added 1,613 mobiles and 451
objects; rooms remained 52,418 and allocation lists fell from 1,490 to 1,112.
The bounded analyzer result remains `REPORT_ONLY`, not a long-horizon leak or
plateau claim. Complementary Memcheck across all 277 production-linked tests
reports zero errors and zero definite, indirect, or possible loss. The
368,451 reachable bytes remain owned by process-lifetime registries. The
focused protocol parser passes 13 of 13, the prior integrated CMake gate
passes 6 of 6, required `make install` removes the root artifact, and the
current normal candidate passes 297 production-linked tests and installs
SHA-256
`908e809acf0941624d4ce301dc4deaadb14f627d1e9fd140718147ada068079e`.

**Campaign-content checkpoint (August 2, 2026, 09:59 IDT):** The tracked,
idempotent Vailand package now maps two territorial-water regions, the
Vailand Passage free seas, Blackwake Anchorage pirate cove, an 18-link
water-only route between the existing North and Central Vailand seaports, a
merchant-cog prototype, iron market gradient, and the scheduled, faction-1
`Vailand Ironwind Trader`. Development provision run
`/tmp/luminari-vessel-campaign-1000/runs/20260802T065410Z-1061371` passes in
167 seconds on source `923c8024`. Actual Kohdee observed territorial, free,
pirate, and Central territorial waters, real iron cargo, the route, movement,
the Central-port arrival, return movement after a hard restart, and two clean
shutdown checkpoints. Merchant lifecycle run
`/tmp/luminari-vessel-merchant-check-1000/runs/20260802T065717Z-1068792`
passes in 22 seconds: merchant 18 generation 1 was sunk, 165 standing and a
900-gold bounty were observed, generation 2 retained 40 iron, pilot 31810,
route, and schedule, and cleanup byte-restored Kohdee and every snapshotted
vessel/economy table.

**Derelict-content checkpoint (August 2, 2026, 10:59 IDT):** The tracked
Blackwake package supplies three object records, five DG triggers, an
idempotent prototype/mapping migration with verification and guarded rollback,
and a collision-sensitive development provisioner. Provision run
`/tmp/luminari-vessel-derelict-1000/runs/20260802T072737Z-1135588` passed in 61
seconds: actual Kohdee traversed all four generated rooms, and slot 8,
prototype 17, coordinates `(-533, 330)`, rooms 70160-70163, and all three room
trigger mappings remained stable across a hard restart. Reversible discovery
run `/tmp/luminari-vessel-derelict-check-1000/runs/20260802T075751Z-1199403`
passed in 55 seconds on source `a390a387`. Actual Kohdee followed the gated
log-to-chart-to-cargo chain across another hard restart, retained exactly one
log and chart in both object-save mirrors, salvaged one tidefinder for 180
gold, persisted all five DG variables, and then restored the original player,
index, object, optional legacy-variable, database-header, and database-object
state exactly. First-finder naming is intentionally not enabled for this
initial derelict; it remains an optional, non-release-blocking extension.

**Frontier-content checkpoint (August 2, 2026, 12:16 IDT):** The tracked,
idempotent frontier package owns the Starfall Trench bathymetric region,
Aetherwind Skyway altitude lane, Shardspire Sky Island, a 79-cell digitalized
Sablebranch River path, and one acceptance prototype for every vessel class.
Development run
`/tmp/luminari-vessel-frontier-1000/runs/20260802T091531Z-1364409` is terminal
`PASS` in 75 seconds on source `873171ae` and installed SHA-256
`9b329263602de6e1a655e68183389bbae73414bc9d21951e004603809856b6ec`.
Actual Kohdee sailed both river hulls from `(-810, 480)` to `(-809, 480)`,
crossed Starfall waters in a 12,000-pound survey ship, verified all three
warship weapon slots without firing, dived the Starfall Bathyscaphe to Z -90
inside natural depth 104, activated the 125-percent Aetherwind speed lane at Z
100, and entered Shardspire at `(469, 0, 200)`. The 40,000-pound transport
generated three cargo holds, while the magical vessel crossed Plains, River,
Z -10, and Z 10 in one continuous journey. The gate purged every temporary
hull and returned Kohdee to room 1204. End-to-end testing also repaired ignored
`reglist type` and `pathlist type` filters, missing River status output, and an
unspawnable airship speed. The production-linked suite passes 278 tests.

**Showcase-event checkpoint (August 2, 2026, 13:03 IDT):** Phase 16 adds one
staff-managed event at a time, a one-hour ceiling, movement-scored regattas,
damage- and sinking-scored team skirmishes, temporary persistent ghost fleets,
transactional result finalization, boot recovery, and public leaderboards.
Reversible run
`/tmp/luminari-vessel-event-check-1000/runs/20260802T100241Z-1463421`
is terminal `PASS` in 61 seconds on source `9ffe75d0` and installed SHA-256
`ace95edc41320918cd04ef0d6fa93effea9a65f06a04ae99d545a7e63fa0113a`.
Actual Kohdee placed first in a River regatta, dealt 12 live damage for the
winning red skirmish fleet, and hit one of three spawned ghost warships. All
three leaderboard rows advanced exactly once during the snapshot. Cleanup
restored the player file and all four event tables to identical hashes, left
zero event runtime or temporary hull rows, and restarted the exact candidate.
The warning-free production-linked suite passes 282 tests.

**Tactical-chart checkpoint (August 2, 2026, 13:45 IDT):** The `tactical`
command now renders a 21-by-21 canonical wilderness chart with deep water,
shoals, rivers, beaches, ports, coastline, land, public geographic and
threshold-region edges, five- and ten-unit range rings, weather visibility,
and damage-aware contacts. Reversible run
`/tmp/luminari-vessel-tactical-check-1000/runs/20260802T104219Z-1540531`
is terminal `PASS` in 141 seconds on source `d2a1669e` and installed SHA-256
`68d2cd685f8a3bb82a4d0d94e1ddc4d279205650f22c273a0deebe2f6eb1ebdd`.
Actual Kohdee charted the Starfall Trench, five- and ten-unit rings, and a
nearby sound Bastion; a real `shipfire` hit changed that contact to battered
on both the map and nearest-first roster. A second coastal chart showed real
shoal, beach, coastline, and region-edge cells. Cleanup purged every temporary
hull, byte-restored Kohdee's player file (SHA-256
`53061d7ae86ea0dd8dafdfc0e02bfc13bee1022bec7e5318a9ce5d67e33ae0bb`),
left zero Bastion runtime/interior rows, and restarted the exact candidate.
The warning-free production-linked suite passes 287 tests.

**Wilderness-lookout checkpoint (August 2, 2026, 14:15 IDT):** The player-
facing `lookout` command, with backward-compatible `look_outside`, now samples
canonical modified wilderness sectors in all eight compass directions out to
the production weather- and crew-limited horizon. It reports current terrain,
natural elevation, water column, and nearest-first visible vessels with
condition, range, bearing, and relative Z. Reversible run
`/tmp/luminari-vessel-lookout-check-1000/runs/20260802T111510Z-1611249`
is terminal `PASS` in 38 seconds on source `d788c537` and installed SHA-256
`0e7aa43463d67388aa985e6dfca4c854a17d97cf2ea1a1803714e3d3c163530a`.
Actual Kohdee read both authoritative help aliases, scanned open Starfall
waters with a real sound contact two units east, then scanned canonical
Water, Beach, Field, Marshland, City, and road sectors from the coast. Cleanup
purged every temporary hull, restored room 1204 and the byte-identical player
file, left zero Bastion runtimes, and restarted the exact candidate. The
warning-free production-linked suite passes 292 tests.

**Dynamic-narrative checkpoint (August 2, 2026, 14:54 IDT):** The at-sea line
now uses the compact narrative-weaver path to combine class, movement,
wilderness weather, and one deterministic geographic or weather-specific
region hint. A 120-second heartbeat sends class-, speed-, weather-, and
depth-aware ambience only to occupied moving hulls; `vesseldebug ambient`
forces the same production formatter for staff acceptance. The Vailand
content package owns eight idempotent hints, two for each canonical water
region, with read-only verification and guarded rollback. Reversible run
`/tmp/luminari-vessel-narrative-check-1000/runs/20260802T115413Z-1685068`
is terminal `PASS` in 34 seconds on source `547e54b3` and installed SHA-256
`908e809acf0941624d4ce301dc4deaadb14f627d1e9fd140718147ada068079e`.
Actual Kohdee observed overcast conditions at 167/255, a steady warship line
with Vailand Passage prose, and a matching forced ambient message. Cleanup
purged every temporary hull, restored room 1204 and the byte-identical player
file (SHA-256
`16574e8f8c243a152f1fb0a9a2402e31a98534a5ef9ff622fa78f67239b3bc5d`),
left zero acceptance runtime rows, and restarted the exact candidate. Five
new production-linked tests bring the warning-free suite to 297 tests.

**Hostile-boarding checkpoint (August 2, 2026, 15:48 IDT):** Boarding is now a
class ability for every class, uses the better of Strength or Dexterity with
armor penalties, and resolves hostile transfer through opposed grapple and
crossing checks. Ties defend; target hull class, speed, structure, sailmaster,
and bosun modify the defense. Legacy Jump-slot ranks are cleared once through
the `BrdV` player-file marker instead of becoming free Boarding training.
Reversible run
`/tmp/luminari-vessel-boarding-check-1000/runs/20260802T124631Z-1797834`
is terminal `PASS` in 74 seconds on source `e8377caa` and installed SHA-256
`b01e8610325dc40445c8550a8b93752bfc979512145efbe357e48c22db04ed8a`.
Actual Kohdee lost a 26-to-56 grapple to Vesselmate, then reversed the trained
ranks and won grapple 56-to-14 and crossing 56-to-28 before breaching the
target. Vesselmate received both live warnings. Cleanup purged both temporary
hulls, restored both player files to their exact hashes, left zero temporary
runtimes, and restarted the exact candidate. The warning-free
production-linked suite passes 302 tests.

**Exterior-customization checkpoint (August 2, 2026, 16:10 IDT):** Owners can
now use `shipcustomize` to review, set, or clear optional 3-80 character paint
and figurehead descriptions. Phase 17 persists both fields without expanding
the 5 KiB base ship structure. Hull room text and both own-ship and contact
lookout reports render the details. Reversible run
`/tmp/luminari-vessel-lookout-check-1000/runs/20260802T131015Z-1845762`
is terminal `PASS` in 41 seconds on source `302c8b87` and installed SHA-256
`75a62d7c17ed93c3cfc7c4e74db458b59745dd4e993ea075bec3cfb7616f0bf3`.
Actual Kohdee set midnight-blue paint and a gilded sea-dragon figurehead,
observed both in `lookout` and the exterior hull description, cleared both,
and observed the default hull text return. Cleanup purged all temporary
runtimes, restored Kohdee to SHA-256
`16574e8f8c243a152f1fb0a9a2402e31a98534a5ef9ff622fa78f67239b3bc5d`,
and restarted the exact candidate. The warning-free production-linked suite
passes 304 tests.

**Final-release audit checkpoint (August 2, 2026, 16:24 IDT):** The clean
development candidate at source `05144502` was installed as SHA-256
`75a62d7c17ed93c3cfc7c4e74db458b59745dd4e993ea075bec3cfb7616f0bf3`.
Actual Kohdee confirmed that vessel debug support is compiled out, ran the
1,000-trade simulation to `PASS`, resolved `SHIPCUSTOMIZE` through an
authoritative database help tag, and reported 8 of 500 fleet slots plus 32 of
2,000 dynamic wilderness rooms in use. The economy sample completed all 1,000
trades inside the 10..400 supply bounds, found an 18-trip profitable route
worth 8,060 gold before equilibrium, rejected adversarial reversal at
-5,150,000 gold, and returned both simulated ports to supply 100.

This checkpoint is deliberately partial. It does not substitute automated
Kohdee output for human fun ratings or player data. The remaining five items
still require a combat/cost balance record, a structured human beta, an
isolated production-snapshot migration rehearsal, a fresh complete preflight
after Phase 17, and authorized staged production rollout.
The current implementation adds a read-only `vesseldebug balance` report and
one production-linked deterministic duel test; the warning-free suite passes
305 tests. Do not mark balance complete until its installed actual-character
transcript and real beta/player sample are recorded.

**Mechanical-balance checkpoint (August 2, 2026, 16:42 IDT):** Actual Kohdee
ran `vesseldebug balance 1000` on source `e07cd049` and installed SHA-256
`075d552509218a5071067898c34130de31748628f1c40dc6f790cb0cc831b6dc`.
The diagnostic rejected duel counts 0 and 5,001, then passed 1,000 of 1,000
representative equal-warship duels: first/second wins were 569/431 and
min/median/p95/max time-to-kill was 48.5/60.5/72.5/81.5 seconds. The same
session passed 1,000 trades, an 18-trip 8,060-gold finite route, -5,150,000
gold adversarial reversal, restock 100/100, all class cost anchors, and
green/able/veteran full-roster paydays of 165/330/495 gold. Authoritative
`VESSELDEBUG` help also passed.

The anonymized development sample contains only 3 owned hulls, 330 gold owed
in wages, 20,000 insured value, zero completed freight contracts, and zero
showcase entries. That is useful proof that the report reads persistence, but
it is explicitly insufficient player data for balance or fun sign-off. The
balance checkbox therefore remains open for real beta evidence.

Permanent evidence and behavior live in:

- [VESSEL_BENCHMARKS.md](../../testing/VESSEL_BENCHMARKS.md)
- [VESSEL_SYSTEM_TESTING.md](../../testing/VESSEL_SYSTEM_TESTING.md)
- [VESSEL_SYSTEM.md](../../systems/VESSEL_SYSTEM.md)
- [CHANGELOG.md](../../CHANGELOG.md)

Do not restart an unattended long-duration ferry or fleet monitor. Future
agent-run vessel gates must retain the one-hour total ceiling, including setup,
recovery, review, and cleanup. Before destructive merchant or hunter checks,
confirm no benchmark worker owns the development service.

**Remaining checklist:** 5 top-level balance/beta/rollout items.

## 1. Add Living-World Content

- [x] Add data- and DG-driven derelicts with explorable interiors, salvage,
  logs, maps, discovery chains, and optional first-finder naming.
- [x] Add bathymetry-anchored trenches, sky islands, high-altitude lanes, and
  `path_data` river travel for rafts and boats.
- [x] Give each of the eight vessel classes at least one unique destination or
  capability.
- [x] Add regattas, staff-triggered fleet skirmishes, a ghost-fleet event, and
  leaderboards.

## 2. Finish Player Experience and Presentation

- [x] Replace the legacy tactical grid with a wilderness-renderer tactical map
  showing coastline, shoals, region boundaries, contacts, range rings, and
  damage state.
- [x] Build lookout view v2 from actual surrounding wilderness sectors.
- [x] Add dynamic at-sea descriptions through `narrative_weaver` and
  `region_hints`, plus class-, weather-, and speed-aware ambient messages.
- [x] Refine hostile boarding with a grapple step, contested rolls, and a
  dedicated boarding skill instead of the current level-plus-Athletics blend.
- [x] Add optional figurehead and paint customization to ship and lookout
  descriptions.

Named-water announcements, the ship-wide captain channel, and repeated-message
throttling are complete. The harbor provisioner passes a matching crossing/
`seastate` transcript and same-account Kohdee/Vesselmate cross-room isolation.
The full scale gate records 23,181 suppressed combat/ambient messages and 20
shared encounters, confirming the final shared-world encounter model.
The completed tactical chart reads `get_map()` terrain and the same wilderness
region polygons used by travel, hides encounter-only metadata, and resolves
contacts through the production sight-range and damage systems rather than a
private tactical grid.
The completed lookout samples the same canonical modified sectors along eight
bearings, shares production visibility and contact state with tactical, and
reports elevation and water depth without creating a second world model.
The completed narrative layer uses that same raw 0..255 wilderness weather
field, deterministic region hints, and existing ship state. Its 120-second
ambient cadence skips stopped and unoccupied hulls, so presentation does not
create a second simulation or fleet-scale background-message source.
Hostile boarding now prepares NPC defenses, selects the strongest conscious
PvP-consenting defender anywhere aboard, and exposes both opposed totals. A
failed grapple never reaches the crossing; a failed crossing stays aboard the
attacker, with a natural 1 or ten-point loss causing the water-and-Athletics
consequence. A successful crossing enters the target and starts combat only
with eligible defenders.

## 3. Balance, Beta, and Roll Out

- [ ] Tune combat time-to-kill, crew wages, freight margins, refit costs,
  insurance, and dock fees using the simulation, duel tests, and player data.
- [ ] Run a structured player beta against the release scorecard in
  [PRD.md](../../PRD.md). Validate first-hour discovery, multiplayer roles,
  builder independence, a supervised NPC-shipping sample within one hour, and
  at least 70 percent "fun" combat feedback.
- [ ] Rehearse every schema migration and rollback against a production
  snapshot with no data loss. Do not modify the live production database.
- [ ] Confirm production preflight on the release candidate: repeat regression,
  load-bearing-toggle, debug-off, authoritative-help, lifecycle recovery,
  benchmark, bounded stability, and documented operator-recovery checks.
- [ ] Roll out in stages: staff, beta cohort, then all players. Prepare the
  announcement, monitor each stage, retain rollback authority, and write the
  postmortem. Update permanent evidence and behavior references before marking
  the vessel system 3.0.

## Completion Rule

The backlog is complete only when every Must-have release criterion in
[PRD.md](../../PRD.md) has evidence, all production gates pass, and no
release-blocking item remains here. Optional cosmetics may be explicitly
deferred to the general project backlog; they must not be silently reported as
complete.
