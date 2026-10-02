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
| S11 Checked cargo trades (work item #10) | In review (MR !17) | [Phase 11](#phase-11-s11-progress) |
| S12 Owned waypoints and routes (work item #11) | Not started | [Part 5](#part-5-implementation-sequence) |

Production help is current through S10 (help sync plan `78cccae490f9`, 2026-10-02). S1-S10 are
merged: the study's steps, S1-S8; S-immediate, which readied the local Luminari Web
client for S9; S9, which played the whole system in game and recorded it; and S10, which turned
that record into the [Vessel Player Guide](../guides/VESSEL_PLAYER_GUIDE.md) and fixed what
checking its facts against the code found. Two follow-ups join the sequence, both resolved in this
worktree: S11 for GitLab work item #10 and S12 for work item #11.

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

S11 is in review, below. S12 follows from S11's merge, on `feat/vessels-s12` with
`vessels-s12-base`. Each step's merge request says `Closes #10` or `Closes #11`, which lists it on
its work item and closes the item when it merges.

Still open outside these steps: the production deploy of S9's world-data notes and S10's code, the
Open player-data balance and human beta gates in `VESSEL_SYSTEM_REQUIREMENTS.md`,
[work item #12](https://gitlab.com/max757/Luminari-Source/-/work_items/12) (two-phase vessel
settlements, from MR !17's review), and closing these study documents once S12 merges:
`docs/ongoing-projects/` is temporary, and their enduring content lives in `VESSEL_SYSTEM.md` and
the guide.

### Phase 11 (S11) progress

In review (2026-10-02). Branch `feat/vessels-s11` from master `88495e08a` (the S10 merge and its
close-out), where the annotated tag `vessels-s11-base` stands, so
`git log vessels-s11-base..vessels-s11` lists only S11. The first commit after `264469692` (which
added S11 and S12) is this plan. Hand-off as in the routine: tag `vessels-s11` and a merge request
that says `Closes #10`; review fixes go on top. Scope: GitLab work item #10, `cargobuy` and
`cargosell` moving gold without a checked save.

The defect, traced in `src/vessels/vessels_trade.c`: `do_cargobuy()` debits the gold in memory
(`award_gold()`), then `port_adjust_supply()` and `vessel_db_save_cargo()` write the port's
supply and the manifest, each unchecked and outside any transaction; `do_cargosell()` does the
same with the revenue. No player save follows, so a crash before the captain's next routine save
returns a buyer's gold with the cargo recorded aboard, or loses a seller's gold with the cargo
gone; and a failed manifest write leaves sold cargo recorded aboard, to be sold again after a
reboot.

Items:

1. One helper in `vessels_trade.c` records a trade: the port's supply and the ship's manifest in
   one transaction, rolled back if either write fails. It replaces `port_adjust_supply()`, whose
   only callers are these two commands and which re-read the supply the caller had just read; its
   UPDATE becomes a `PREPARED_STMT`, so `vessels_trade.c`'s formatted-SQL baseline drops from 7
   to 6.
2. `cargobuy` and `cargosell` take the shape `a703572e3` gave freight acceptance: change the hold
   in memory and record the trade; on a refused record, put the hold back and move no gold;
   after the commit, move the gold and save it with `save_char_checked()`; if that save fails,
   restore the gold and the hold and record the pre-trade supply and manifest again. The crew's
   sale experience is earned only once the sale is saved.
3. DB-backed tests in `test_vessel_rewards.c`, beside `Test_vessel_freight_bond_pays_for_the_goods`:
   for each command a refused manifest write and a failed character save leave the gold, the
   hold, the manifest rows and the port's supply as they were; a saved trade moves all four. The
   contraband test's trades now save the captain, so it gets the scratch player files the freight
   test uses.
4. `VESSEL_SYSTEM.md`'s trade paragraph says how a trade is recorded, as the freight paragraph
   does.

Interpretations decided while planning S11:

- The compensation after a failed save records the pre-trade state with the same helper, so it
  is as atomic as the trade; if it fails too, a `SYSERR` is logged, as freight acceptance does.
- The hold is restored from a copy of the one lot the trade touched, which puts back both the
  quantity and a lot emptied by a sale.
- No database (or an NPC, whose save always fails) means no trade, as freight acceptance and
  passenger fares already refuse; MySQL is required to run the server.
- The refusal messages name what did not happen: "The harbor office cannot record that trade;
  no gold changed hands." and "Your gold could not be recorded, so the trade is undone."

Ablation (planning): dropped help changes (usage, prices and rules are unchanged; only the two
failure messages are new), new schema SQL with its rollback and verifier (no table changes), the
vessel help verifier (no help change), and the 19 live gates that never run `cargobuy` or
`cargosell` (the changed code is reachable only through those two commands). Simplified: one
helper records both the trade and its undo, instead of separate undo writes; a lot copy restores
the hold instead of moving `contract_unload()` out of `vessels_contracts.c` for reuse. Kept: the
DB-backed failure tests for both commands, the economy gate (it buys and sells contraband with a
real logged-in captain, whose saves go to disk), `make test-all` with the database cases, and the
local CI matrix.

Verification: `make test-all` with the database cases on (the `luminari-vessels-testdb` container,
environment as in S9's `testenv.sh`); `scripts/ci/check_sql_interpolation.py`; the economy gate
`test_vessel_economy_in_game.sh` in the namespace harness on a reload of the development dump;
and `scripts/ci/local/run.py --base gitlab/master`.

Progress log (2026-10-02, kept current as the work goes):

- Plan committed (`3299aaa25`).
- The fix, `c2888ec8f`: `trade_record()` replaces `port_adjust_supply()`; both commands record
  the trade before the gold moves and save the gold checked, as planned; the crew's sale
  experience follows the save; `VESSEL_SYSTEM.md`'s economy paragraph says how a trade is
  recorded; `vessels_trade.c`'s interpolation baseline is 6. The new
  `Test_vessel_cargo_trades_record_the_gold_with_the_goods` covers both commands' refused write
  and failed save (a whole-hold sale's emptied lot included) and a saved trade; the contraband
  test got the scratch player files.
- Notes for whoever continues: a CHECK constraint cannot be added over rows that break it, so
  the sale's refused write uses `CHECK (item_count >= 10)` over the bought lot of 10;
  `check_sql_interpolation.py --update` also lowers other files' counts and re-sorts the baseline,
  so only S11's line was edited by hand. S10's batch economy job (`y20-economy.sh`) relied on
  earlier gates' boots for the Phase 19-21 columns: alone after a dump reload it stops on
  `armor_scale`, so S11's job applies `vessels_phase19_schema.sql` through
  `vessels_phase23_schema.sql` first.

Ablation (building): the third copy of the trade tests' temporary tables became one fixture
helper that the freight and contraband tests use too. `save_char_checked()` can still fail after
its rename only when the character is missing from the player index, which a logged-in player
is not; freight acceptance accepts the same window, so S11 adds nothing for it. No help, schema or
player-guide change was needed.

Verification (2026-10-02, on `c2888ec8f`):

- `make test-all` with the database cases (the `luminari-vessels-testdb` container, S9's
  `testenv.sh`): 1,995 CuTest cases OK (seed 1; the new test's trades are in the log), the
  protocol harness's 32, and the Python suites (542, 37 skipped).
- `scripts/ci/check_sql_interpolation.py`: within baseline, 317 sites.
- The economy gate in the namespace harness (`/tmp/claude-1000/vs4`, jobs `s11b2-reload` and
  `s11c-economy`) on a fresh reload of the development dump: passed in 208 s, buying and selling
  contraband with a real logged-in captain whose saves go to disk.
- The local CI matrix (`scripts/ci/local/run.py --base gitlab/master`): all 33 jobs
  passed in 487 s.

Cleanup: the harness is stopped (its disposable database stops with its namespace); no
characters, pfiles or scratch files were left in the worktree.

Hand-off: tag `vessels-s11` and MR !17 from `feat/vessels-s11`, which says `Closes #10` (range
`vessels-s11-base..vessels-s11`). Review fixes go on top, one commit each.

Review round 1 (2026-10-02, range `vessels-s11..feat/vessels-s11`): one [P2] finding, fixed.

- Failed compensation reported as undone (`a1f08bc7b`). When the save failed and the undo could
  not be recorded either, both commands still restored the gold and the hold in memory and said
  "the trade is undone" while the database kept the trade. This was reproduced on `50ca77f2d` by
  a CHECK constraint added with `check_constraint_checks` off, which refuses only the supply the
  undo writes back. `trade_settle()` now holds the gold, the save and the undo for both commands
  and reports "undone" only after the undo commits. If the undo fails too, the trade stands as
  recorded, in memory too, and the persistence service's minute save (`save_char_checked()` for
  every connected player, a failed save retried after a second) stores the gold. This replaces
  the planning interpretation that a failed undo only logs a `SYSERR`.
- Ablation: the reviewer's persisted undo intent was not added here. A record kept only in the
  database cannot tell recovery whether a later save carried the gold, so it would move the
  crash window to its mirror case. Closing that window needs a settlement table, a marker in the
  player file and a login reconcile. The owner asked for that as a new work item:
  [#12](https://gitlab.com/max757/Luminari-Source/-/work_items/12), which also names freight
  acceptance and dock-fee payment, which share the shape. `PLR_CRASH` was dropped from the fix:
  the minute save already covers every connected player. A block on further trades was dropped too:
  the next trade's own checked save carries the pending gold.
- Verification: the extended `Test_vessel_cargo_trades_record_the_gold_with_the_goods` (a sale
  and a purchase whose undo is refused: the message, no "undone", and the gold, hold, manifest
  and supply of the standing trade) failed on `50ca77f2d` and passes now. `make test-all` with
  the database cases: 1,995 CuTest cases OK, the protocol harness's 32, and the Python suites
  (542, 37 skipped). `check_sql_interpolation.py`: within baseline. The local CI matrix
  (`run.py --base gitlab/master`): all 33 jobs passed. The economy gate was not rerun: the
  success path moved into the helper unchanged, and the DB-backed test drives both commands with
  real saves.

After the merge: no help to sync (S11 changes none); the fix goes with the next production deploy;
move this section to the history, set the status row, and branch S12 from the merge
(`feat/vessels-s12`, tag `vessels-s12-base`).

## Estimate (remaining)

| Step | What drives the size | Days |
| -- | -- | -: |
| S11 Checked cargo trades | Two commands to the freight-bond shape, DB-backed failure tests | 0.5 |
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
