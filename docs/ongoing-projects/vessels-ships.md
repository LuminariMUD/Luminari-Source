# vessels-ships.md improvement pass based on duris code

The goal is to improve the vessel/ship system implementation BASED on DurisMUD system.

This is the entry point: where the work stands, how a step is worked, the rules and decisions that
still bind, the active step's record, and what remains. The
companion [vessels-ships-history.md](vessels-ships-history.md) keeps everything else unabridged:
where the code and documents are, the 2026-09-28 study (Parts 0-4: DurisMUD and LuminariMUD at the
baseline, the feature matrix, the combat parity checklist, the design values already built, and
the defects), each merged step's progress record, and the original estimate. Both documents use
the study's section numbers, which the code, the SQL, and `VESSEL_SYSTEM.md` cite as "study
3.3.x". When a step merges, its progress section moves to the history document and its status row
here records the merge.

## Status

| Step | State | Record |
| -- | -- | -- |
| S1 Defect fixes | Merged `34dcbb34e` (MR !6) | [Phase 1](vessels-ships-history.md#phase-1-s1-progress) |
| S2 Movement and pacing | Merged `89cfabcbe` (MR !7) | [Phase 2](vessels-ships-history.md#phase-2-s2-progress) |
| S3 Damage model | Merged `a85e97d9f` (MR !8) | [Phase 3](vessels-ships-history.md#phase-3-s3-progress) |
| S4 Weapons and gunnery | Merged `c8bab4576` (MR !9) | [Phase 4](vessels-ships-history.md#phase-4-s4-progress) |
| S5 Crew, repair and loss | Merged `23a0726e4` (MR !10) | [Phase 5](vessels-ships-history.md#phase-5-s5-progress) |
| S6 NPC raiders and AI | Merged `85914a03d` (MR !11) | [Phase 6](vessels-ships-history.md#phase-6-s6-progress) |
| S7 Rewards and economy | Merged `cce9ff323` (MR !12) | [Phase 7](vessels-ships-history.md#phase-7-s7-progress) |
| S8 Client data | Merged `2a4815b1a` (MR !13) | [Phase 8](vessels-ships-history.md#phase-8-s8-progress) |
| S-immediate Luminari Web for S9 | Merged `1c7e4bffb` (MR !14) | [S-immediate](vessels-ships-history.md#s-immediate-progress) |
| S9 Claude Code play tests | Merged `a6adb46a8` (MR !15) | [Phase 9](vessels-ships-history.md#phase-9-s9-progress) |
| S10 Player guide | Merged `60ef66ad1` (MR !16) | [Phase 10](vessels-ships-history.md#phase-10-s10-progress) |
| S11 Checked cargo trades (work item #10) | Merged `f18208549` (MR !17) | [Phase 11](vessels-ships-history.md#phase-11-s11-progress) |
| S12 Owned waypoints and routes (work item #11) | In progress on `feat/vessels-s12` | [Phase 12](#phase-12-s12-progress) |

Production help is current through S10 (help sync plan `78cccae490f9`, 2026-10-02); S11 changed
no help. S1-S11 are merged: the study's steps, S1-S8; S-immediate, which readied the local
Luminari Web client for S9; S9, which played the whole system in game and recorded it; S10, which
turned that record into the [Vessel Player Guide](../guides/VESSEL_PLAYER_GUIDE.md) and fixed what
checking its facts against the code found; and S11, which made `cargobuy` and `cargosell` record a
trade before the gold moves and save the gold checked (GitLab work item #10). S12, for work item
#11, is in progress.

## Working a step

- Branch `feat/vessels-sN` from the previous step's merge commit, and push an annotated tag
  `vessels-sN-base` on that commit, so `git log vessels-sN-base..vessels-sN` lists only the step.
  The first commit is the step's plan: a "Phase N (SN) progress" section under Active step, with
  the items, the interpretations, and the planning ablation.
- Build to Part 5's standard, and keep the step's section current as the work goes (decisions
  made while building included), so another session can take it over.
- Verify with `make test-all` with the database cases on, the new SQL (schema, rollback,
  verifier) on the test database, the vessel help verifier, every live gate in `scripts/vessels/`
  (the private-namespace harness on a reload of the development dump), and the local CI matrix
  `scripts/ci/local/run.py --base gitlab/master`; record the results in the step's section.
- Hand off with an annotated tag `vessels-sN` at the head given to review and a GitLab merge
  request from the branch, with remove-source-branch and squash off. Review fixes go on top, one
  commit each, recorded in the step's section with a finding, fix, and commit table. Pushed tags
  never move.
- Merge with a merge commit, never a squash (the records cite commit hashes), and keep the branch.
  Then sync the step's help to production, move the step's section to the history document, update
  the status row, and tag and branch the next step from the merge.

## Key locations

- DurisMUD: `/home/aiwithapex/projects/duris/`. The ship code is in `src/ships/`: `ships.h`
  holds the module map and each `.c` file an `OVERVIEW` block. The references are
  `docs/reference/SHIPS.md`, `docs/reference/SHIP_GAMEPLAY.md`, and `lib/information/helpships`.
- LuminariMUD (this repository):
  - `src/vessels/` holds the code; start with `vessels.h`.
  - `docs/systems/VESSEL_SYSTEM.md` is the current-behavior reference.
  - `docs/product-requirements/VESSEL_SYSTEM_REQUIREMENTS.md` holds the release gates.
  - Help lives in `lib/text/help/help.hlp` and `sql/components/help_vessel_entries.sql`, checked
    by `verify_help_vessel_entries.sql`.
  - Schema and content SQL is `sql/components/vessels_*`.
  - The tests are `unittests/CuTest/test_vessel_*.c` and `test_transport_production.c`.
  - The live gates and content provisioners are in `scripts/vessels/`.
- The full lists, with line references at the study baseline, are in the
  [history](vessels-ships-history.md#durismud-vessel--ship-code).

## Rules and units

From the study's scope ([history](vessels-ships-history.md#study-scope-and-baselines)):

- Rules applied, from the earlier Duris conversion work: when an existing LuminariMUD mechanic
  already produces a Duris effect, the gap is closed in one line; when LuminariMUD lacks the
  mechanic, the recommendation is the real mechanic, never a cheaper stand-in. This is not an
  exact-parity port. Duris numbers are the player-tested prior for the Open "Player-data
  balance" gate in `VESSEL_SYSTEM_REQUIREMENTS.md`, translated into LuminariMUD's D20 rules, units
  and economy.
- Units. Both games measure range in map rooms (a Duris ocean map room and a LuminariMUD
  wilderness coordinate are both one room), so Duris ranges transfer unchanged. A Duris ship tick
  is 1 s; a LuminariMUD vessel tick is 0.5 s (`VESSEL_PERIODIC_FAST_CADENCE`, 5 pulses at 10
  pulses per second, `src/vessels/vessel_periodic.c:15`). Duris prices are platinum (pp). On the
  default hull prices, 1 pp is about 2 LuminariMUD gold for ship, transport and warship hulls
  (Part 2.2); that is a starting conversion, not an economy measurement.

## 3.4 Owner decisions (2026-09-28)

- D1 Pacing: the Duris scale, `speed / 90` rooms per 0.5 s tick.
- D2 Time to kill: 3-8 minute median for equal warships.
- D3 Loss model: Duris; the hull is lost, the ship's identity survives (3.3.7).
- D4 Crew economics: one-time hire, no wages (3.3.5).
- D5 Ownership: a small configurable cap, 3 hulls per owner by default, `cedit` range 1-10;
  public and NPC hulls do not count, and a capture at the cap is refused.
- D6 Capture: only disabled prizes: a breached arc, immobile, colors struck (`strikecolors`,
  owner or permit holder, until the hull moves or 10 minutes pass), or abandoned at sea (no
  conscious character or crew aboard). Hostile boarding needs the target at speed 3 or less or
  disabled.

## Part 5: Implementation sequence

Each step ships with production-linked CuTest coverage in `unittests/CuTest/`, help in both
`lib/text/help/help.hlp` and the database (`sql/components/help_vessel_entries.sql`), schema and
rollback SQL where tables change, `VESSEL_SYSTEM.md` updates, and, where play changes, an
actual-character gate in the `scripts/vessels/` pattern.

01. S1 Defects that do not wait for the redesign: L4 weather bands, L5 gunnery authorization and
    owner consent, L7 miss lag, L8 contact list and IDs, L9 port immunity, L11 `for_sale`,
    `min_level`, departure level check and the ownership cap (D5), L12 wage removal (D4), L13
    bounty pay-off and decay.
02. S2 Movement and pacing (D1): fractional movement with per-cell validation, class accel and
    turn with the sailmaster and rudder factors, the load and sail limits on maximum speed,
    `setsail` as the maneuver command, `undock` departure and `anchor`, SEADOG +1 speed, the class
    table's speeds, and every automated mover rebased. Ferry soak and scale benchmark are
    re-baselined in `docs/testing/VESSEL_BENCHMARKS.md`.
03. S3 Damage model: Duris arcs and armor profiles with the 229 armor limit, sail HP, breach
    states and sink timers, criticals, weapon damage, knockdown saves, cargo spill and `salvage`,
    and the D6 capture, plunder and boarding rules with `strikecolors`; also the 3.3.1 refit
    rescaling (plating and reinforcement +20%, rigging +10% speed), moved here from S2.
04. S4 Weapons and gunnery: the weapon and equipment tables, 16 slots, fitting, ammo and resupply
    commands, `lock`, battle stations, arc fire, the geometry DC hit model, `sight`, `scan`,
    crew-stun and flight rules; the duel harness moved to the D2 bounds; and, with battle
    stations, the 3.3.2 crash check for refused rooms and shallows, moved here from S2.
05. S5 Crew, repair and loss: one-time hire prices and gates, crew experience, promotion and
    casualties, stamina, the repair stock and dock repairs, the D3 wreck registry with automatic
    insurance, `shipsummon`, trade-in and the rename fee.
06. S6 NPC raiders and AI: the hunter lifecycle generalized into the raider tiers with the 3.3.8
    cadence, fit-outs from prototypes and weapon rows, basic and advanced AI, NPC boarding and
    looting, despawn rules, ramming with the ram, and neutral colors.
07. S7 Rewards and economy: renown, salvage and bounty payouts, the renown board, Ship Damage
    Control, contraband and customs, and the trade modifiers.
08. S8 Client: the MSDP additions.
09. S9 Claude Code play tests: the whole system played end to end in game through a real client,
    a staff character staging the mortal players, with screenshots and guide notes for S10; every
    defect found is fixed to the standard above.
10. S10 Player guide: an illustrated guide to the whole system, written from `guide-notes.md`
    and the S9 screenshots.
11. S11 Checked cargo trades
    ([work item #10](https://gitlab.com/max757/Luminari-Source/-/work_items/10)): `cargobuy` and
    `cargosell` move gold in memory only and leave `vessel_db_save_cargo()`'s result unchecked, so a
    crash or a failed write gives free cargo or sold cargo that sells again. Both take the shape
    `a703572e3` gave freight acceptance: the manifest and port supply in one transaction, the gold
    moved after it commits and saved with `save_char_checked()`, and a failed save undone; with
    DB-backed tests for a refused manifest write and a failed save, and values bound with
    `PREPARED_STMT`.
12. S12 Owned waypoints and routes
    ([work item #11](https://gitlab.com/max757/Luminari-Source/-/work_items/11)): waypoints and
    routes record no creator, so any captain can delete another's idle route. A creator column on
    `ship_waypoints` and `ship_routes` (schema Phase 24 with rollback and verifier SQL,
    `master_schema.sql` and the boot ensure functions); `setwaypoint` and `createroute` record it;
    `delwaypoint` and `delroute` allow the creator and immortals and keep S10's in-use refusals; a
    decision on rows that predate the column; help in both places and `VESSEL_SYSTEM.md`.

S-immediate runs before S9: it gives the local Luminari Web client every feature S9 needs, the
ship data panel among them.

## Active step

### Phase 12 (S12) progress

In progress (2026-10-02). Branch `feat/vessels-s12` from master `f18208549` (the S11 merge), where
the annotated tag `vessels-s12-base` stands, so `git log vessels-s12-base..vessels-s12` lists only
S12; the S11 close-out `a16fe7a64` comes first, then this plan. Hand-off as in the routine: tag
`vessels-s12` and a merge request that says `Closes #11`; review fixes go on top. Scope: GitLab work
item #11, waypoints and routes that record no creator.

The defect, traced in `src/vessels/vessels_autopilot.c`: `ship_waypoints` and `ship_routes` have
no creator column, and the cache nodes (`struct waypoint_node`, `struct route_node` in
`vessels.h`) none either. `do_delwaypoint()` and `do_delroute()` let any captain (anyone on a helm,
or a hull's owner anywhere aboard her) delete any waypoint or route that S10's guards do not
protect: a waypoint on no route, a route no hull runs on a schedule or is sailing. Found while
tracing: `do_addtoroute()` lets any captain append a waypoint to any route, and a scheduled hull
rebuilds her route from that cache at every departure (`schedule_trigger_departure()`), so a
rival can change a public ferry's or merchant's course, or disable her schedule by appending a
waypoint her route check cannot reach. Name lookups take the first match in the shared pool, so
once ownership refuses a rival's row, a captain whose route shares a rival's name could not reach
their own.

Items:

1. Schema Phase 24: `creator_id INT UNSIGNED NOT NULL DEFAULT 0` on `ship_waypoints` and
   `ship_routes` (the player's ID, as `player_data.player_idnum`; 0 is no player: content and
   rows made before S12). `vessels_phase24_schema.sql`, `vessels_phase24_rollback.sql`,
   `verify_vessels_phase24.sql`, their `ci_schema_manifest.txt` lines, the two `CREATE TABLE`s in
   `master_schema.sql`, and the same two `ALTER`s at boot in `vessel_ownership_ensure_schema()`,
   which already runs before `load_all_waypoints()` and `load_all_routes()`.
2. The cache nodes carry `creator_id`; `waypoint_db_create()` and `route_db_create()` take it and
   write it; the boot and single-row loaders read it. `setwaypoint` and `createroute` record the
   player (an NPC records 0); `route_save()`, which has no callers, records 0.
3. One rule for changing a row: its creator or an immortal (`LVL_IMMORT`). `delwaypoint` and
   `delroute` apply it before S10's in-use refusals, which stay; `addtoroute` applies it to the
   route (any captain may still put any waypoint on their own route).
4. Two lookups replace the six inline name loops (`delwaypoint`, `addtoroute` twice, `delroute`,
   `setroute`, `setschedule`): the caller's own row of that name if he has one, otherwise the
   first.
5. Production-linked tests in `test_vessel_movement.c`: a mortal captain is refused deleting
   another captain's or a content waypoint and route and adding to them, reaches their own
   same-named route, and an immortal is not refused; S10's in-use test runs as the creator. A
   DB-backed test: the boot ensure adds the column to a pre-S12 table, `setwaypoint` and
   `createroute` record the player, a reload reads the creator back, and the creator's
   `delwaypoint` and `delroute` remove the rows.
6. Help in both places (`help.hlp`, `help_vessel_entries.sql`): DELWAYPOINT, DELROUTE and
   ADDTOROUTE state the rule; three new `verify_help_vessel_entries.sql` patterns. Docs:
   `VESSEL_SYSTEM.md` (autopilot commands, Phase 24 in the SQL table) and the player guide's
   routes section.
7. The rules gate (`test_vessel_rules_in_game.sh`, Kohdee an immortal and Vesselmate a mortal
   owner on her bridge) gains the rule with real characters: Vesselmate is refused Kohdee's
   waypoint and route, deletes their own waypoint, and Kohdee deletes Vesselmate's route; DELROUTE
   joins its help check.

Interpretations decided while planning S12:

- Rows that predate the column keep `creator_id` 0: nothing records who made them (no column, and
  the creation log lines name no player), and on the development database all three routes are
  content on schedules. A player who wants a pre-S12 row removed asks the staff.
- The creator is the player's ID, not their name: a rename keeps their rows without a
  `player_rename.c` key (numeric-ID ownership needs none), and IDs are not reused.
- Using a route stays open to every captain (`setroute`, `setschedule`, `listroutes`, and a
  rival's waypoint on one's own route); only changing or deleting a row needs its creator.
- Refusals name the rule: "Route 'x' is not yours: only the captain who created it, or the staff,
  may change it." and "Waypoint 'x' is not yours: only the captain who set it, or the staff, may
  delete it."

Ablation (planning): dropped a creator column in `listroutes`/`listwaypoints` (the refusal says
whose a row is not, and nothing needs the list to say more), the `db_init.c` `CREATE TABLE` edits
and a Phase 24 line in `provision_vessel_harbor.sh` (the boot ensure adds the column to every
database, fresh or old, before anything reads it, and no content needs it earlier), cleanup of a
deleted player's rows (they stay harmless and staff-deletable), help changes to SETWAYPOINT and
CREATEROUTE (the rule lives where it refuses), and converting the two INSERTs to prepared
statements (the new value is an integer from the character, and the interpolation baseline does
not rise). Simplified: one ownership check and two lookups instead of per-command code, and the
boot `ALTER`s go in the existing ownership ensure function instead of a new hook in `boot_db()`.
Kept: Phase 24 SQL with rollback and verifier (work item, CI migration job), `master_schema.sql`
(CI's test database is built from it), the `addtoroute` rule (traced defect against public
ferries), the lookups (without them ownership would lock a captain out of their own same-named
route), the DB-backed test, and the rules gate.

Verification: `make test-all` with the database cases on (S9's `testenv.sh`, the
`luminari-vessels-testdb` container); Phase 24 schema, verifier, rollback and re-apply on the test
database, with sqlfluff; `verify_help_vessel_entries.sql` on the development database; the rules
gate and, because scheduled hulls load their routes through the changed loader, the merchant gate,
in the namespace harness on a reload of the development dump; and the local CI matrix
`scripts/ci/local/run.py --base gitlab/master`.

Progress log (2026-10-02, kept current as the work goes):

- Plan committed.

Still open outside these steps: the production deploy of S9's world-data notes and S10's and
S11's code, the Open player-data balance and human beta gates in `VESSEL_SYSTEM_REQUIREMENTS.md`,
[work item #12](https://gitlab.com/max757/Luminari-Source/-/work_items/12) (two-phase vessel
settlements, from MR !17's review), and closing these study documents once S12 merges:
`docs/ongoing-projects/` is temporary, and their enduring content lives in `VESSEL_SYSTEM.md` and
the guide.

## Estimate (remaining)

| Step | What drives the size | Days |
| -- | -- | -: |
| S12 Owned waypoints and routes | Schema phase with rollback and verifier, legacy rows, tests, help | 1 |

The Open player-data balance and human beta gates depend on player availability, not engineering
time.

## Ablation record

- Dropped as Duris-specific or already covered: racewar ocean-PvP state and `signal` (PvP consent
  and `shiptalk` cover them), the Redis and flat-file backends, Duris ferries and autopilot
  (schedules and routes cover them), runtime identity references (generation-aware events cover
  them), the levistone (airships cover flight), delayed market prices (per-unit batch pricing
  already stops quick flips), ship coffers (the settlement path pays owners), CTF speed penalty,
  the Trader achievement, the Sailor's Tattoo, and the unique Cyric's Revenge and automatons
  quest content.
- Simplified: continuous crew skills become experience on the existing four positions and three
  tiers, chiefs are those positions, NPC raiders extend the hunter lifecycle instead of a new
  spawner, the hit model reuses Duris's geometry as a D20 DC instead of a new table, and volley
  flight time is omitted (Duris freezes the chance at firing, so resolving at once changes only
  the delay).
- Kept: everything that satisfies requirement 4.4 (range, bearing, arcs, reloads, armor sections,
  subsystem damage, repair, sinking, wrecks, boarding, capture, NPC doctrine, observable disabled
  subsystems), the Duris features added for parity (stamina, anchor, contraband, damage control,
  equipment), and the pacing change without which none of it can matter.
