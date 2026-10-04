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
| S12 Owned waypoints and routes (work item #11) | Merged `e6881c1d1` (MR !18), review fixes `bde4a0f44` (MR !19) | [Phase 12](vessels-ships-history.md#phase-12-s12-progress) |
| S13 Transactions that survive a lost connection (work item #13) | Merged `9eb723913` (MR !20) | [Phase 13](vessels-ships-history.md#phase-13-s13-progress) |
| S14 Two-phase vessel settlements (work item #12) | In progress on `feat/vessels-s14` | [Phase 14](#phase-14-s14-progress) |
| S15 Checked vessel purchases and payouts (work item #14) | Not started | [Part 5](#part-5-implementation-sequence) |

Production help is current through S12 and its review fixes (help sync plan `e0d8a08faa27`,
2026-10-03); S13 changed no help. S1-S13 are merged: the study's steps, S1-S8; S-immediate, which
readied the local Luminari Web client for S9; S9, which played the whole system in game and
recorded it; S10, which turned that record into the
[Vessel Player Guide](../guides/VESSEL_PLAYER_GUIDE.md) and fixed what checking its facts against
the code found; S11, which made `cargobuy` and `cargosell` record a trade before the gold moves
and save the gold checked (GitLab work item #10); S12, which gave waypoints and routes a creator
who alone (with the staff) may change them (work item #11); and S13, which keeps a database
transaction whole when its connection is lost or its `COMMIT` goes unanswered (work item #13). Two
follow-ups remain in the sequence, both resolved in this worktree: S14 for work item #12, which is
next, and S15 for work item #14.

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
13. S13 Transactions that survive a lost connection
    ([work item #13](https://gitlab.com/max757/Luminari-Source/-/work_items/13)): every database
    connection reconnects by itself (`MYSQL_OPT_RECONNECT`). When one drops inside a transaction,
    the server rolls the transaction back, one statement fails, and the statements after it run in
    autocommit on a new session, a `COMMIT` among them reporting success; and a `COMMIT` whose reply
    is lost is taken for a rollback when it may have committed. The step makes "after a lost
    connection, nothing but `ROLLBACK`" a rule of the query layer. Only `src/database/mysql.c` sends
    commands to the server (`luminari_mysql_query()`, which every `mysql_query()` expands to, the
    prepared-statement wrappers, and the pings), so there a connection that lost its transaction
    takes nothing more until it is rolled back. None of the audit's sites (object, house and
    pet-gear saves, the help import and save, the bounty write) can then write outside its
    transaction, so no writer has to stop at its first failure for that. A shared `COMMIT` helper
    tells committed, refused and unanswered apart (`trade_commit()`'s test, moved to the database
    layer), and the sites that take an unanswered `COMMIT` for a rollback read the database back
    before choosing a state: pet store and retrieve, owner transfer, and event finish (freight
    acceptance is S14's). The site fixes the rule does not reach: `Crash_idlesave` commits, a failed
    object save keeps `PLR_CRASH` so the next pass retries it, a failed bounty read is not read as
    no bounty, the pool stops freeing the handle the global `conn` points at, a lost session's
    help-sync lock is noticed, and `hedit` deletes removed keywords (it reads a prepared SELECT with
    `mysql_store_result()`). One case is not a lost connection: a row that fails while the
    connection is good. The object, sheath and house writers always report success, so the save
    commits without that item and tells no one. No item is to be unsaveable, so the two causes that
    lie in the item or the schema go. The writers' buffer is sized to hold every bounded part of a
    record, and an extra description that does not fit (the only unbounded part) is left out instead
    of the whole record refused. The fresh-install `house_data` definition (`master_schema.sql`,
    `db_init.c`) loses the unique key on `vnum` that lets a house keep one object, with a migration
    for databases created from it. What is left is a database fault: the writers report the failed
    row, the save commits the rest but counts as incomplete, keeping `PLR_CRASH` or
    `ROOM_HOUSE_CRASH` so the next pass retries it, and the staff are told whose item it was. The
    save does not fail as a whole: committing the rest loses less, and the rent save rolls back only
    after worn gear has left memory. DB-backed tests for each fix, the drops real (the statement
    sent, then the socket shut down) on persistent tables. The step reaches outside `src/vessels/`
    because the work item does.
14. S14 Two-phase vessel settlements
    ([work item #12](https://gitlab.com/max757/Luminari-Source/-/work_items/12)): a cargo trade, a
    freight acceptance and a dock-fee payment each write the ship's side to MariaDB and the
    captain's gold to the player file, and nothing spans the two: after a failed gold save and a
    failed undo, a crash leaves the trade in the database and the old gold in the file. Each
    becomes two-phase. A settlement row (schema Phase 25 with rollback and verifier SQL,
    `master_schema.sql` and the boot ensure function) is written in the ship-side transaction and
    carries its undo as signed deltas. The player file records the newest settlement id whose gold
    it holds, saved with the gold, as `VIns` and `VMer` do for insurance claims and merchant
    consequences. A reconcile at login (beside those two deliveries) and before the ship's next
    trade or cargo save deletes a row the file covers and undoes one it does not, and the ship's
    trading is held while a row is open; "undone" is reported only after the undo commits.
    `cargobuy`, `cargosell`, `contractaccept` and `dockfees pay` settle this way, and the row is
    the durable marker that settles their unanswered `COMMIT` through S13's helper. DB-backed
    tests at each crash point, the economy gate, and help and `VESSEL_SYSTEM.md` where the
    messages change.
15. S15 Checked vessel purchases and payouts
    ([work item #14](https://gitlab.com/max757/Luminari-Source/-/work_items/14)): the vessel
    commands S11 did not reach still write the ship's side to MariaDB at once and move the gold
    in memory only, so a crash before the next per-minute save keeps a purchase and gives its gold
    back, or takes a sold item or delivered freight and never pays. The purchases are `shipbuy`
    and its trade-in, `shipchristen`, `shiphire`, `shipweapon` and `shipequip` buying,
    `shiprearm`, `shiprepair`, `shipupgrade`, `shipsummon`, the bounty pay-off and `marque`; the
    payouts are `shipweapon` and `shipequip` selling and `contractdeliver`. Each takes S11's level
    in the shape the passenger fare has (`vessel_collect_passenger_fare()`), through one shared
    helper: both stores written and checked inside the command, a purchase whose gold cannot be
    saved refused, one whose ship-side write fails refunded, and a payout whose gold cannot be
    saved put back. DB-backed tests per command family for a failed gold save and a refused
    ship-side write, and help and `VESSEL_SYSTEM.md` where the messages change. Not S14's
    settlement record: each command would need its own stored undo.

S-immediate runs before S9: it gives the local Luminari Web client every feature S9 needs, the
ship data panel among them.

## Active step

S14 is the active step, on `feat/vessels-s14` from the S13 merge `9eb723913`, where the annotated
tag `vessels-s14-base` stands. Its merge request says `Closes #12`, which lists it on the work item
and closes the item when it merges. S15 follows from S14's merge, on `feat/vessels-s15` with
`vessels-s15-base`; its merge request says `Closes #14`.

### Phase 14 (S14) progress

In progress. Branch `feat/vessels-s14` from the S13 merge `9eb723913` (tag `vessels-s14-base`); the
S13 close-out `96b3af5a1` comes first, then this plan. Hand-off as in the routine: tag
`vessels-s14` and a merge request that says `Closes #12`; review fixes go on top. Scope:
[work item #12](https://gitlab.com/max757/Luminari-Source/-/work_items/12) and its two notes.

The model. A settlement has a ship side in MariaDB and the captain's gold in the player file. The
ship side is written with a settlement row in one transaction; the gold is saved with the row's id
as the file's marker; then the row is deleted. A row that is still there is judged by the file of
the player it names: the marker is its id, so the gold is saved and the row is deleted; any other
marker, so the gold never reached the file and the ship side is undone from the row. The undo
never touches gold. Outside the function that pays, a character's marker in memory equals the one
in the file (a failed save puts it back), so the acting character is judged from memory and anyone
else from the file, online or not.

Items:

01. Schema Phase 25, `vessel_settlements`: `settlement_id` (auto-increment), `player_id` (the
    player's ID, 0 for a mob), `ship_id`, and the undo: `port_vnum`, `commodity_id`,
    `supply_delta` and `cargo_delta` (signed, added to the port's supply and to the hold's lot),
    `contract_id` (the freight contract to reopen), `fee_amount`, `fee_port` and `fee_clan` (the
    dock fee owed again), and `created_at`. Unique keys on `ship_id` and on `player_id`: the
    schema allows one open settlement for a ship and one for a player, which is "the ship's
    trading is held while a row is open". `vessels_phase25_schema.sql`,
    `vessels_phase25_rollback.sql`, `verify_vessels_phase25.sql`, their `ci_schema_manifest.txt`
    lines, `master_schema.sql`, and a boot ensure function called from
    `vessel_trade_ensure_schema()`.
02. Player file: `VSet`, the id of the newest settlement whose gold the file holds
    (`GET_VESSEL_SETTLEMENT()`, `vessel_settlement_id` beside the `VIns` and `VMer` fields), read
    and written where they are.
03. A new `src/vessels/vessels_settlement.c` (both build lists, the parity check) with one
    record, `struct vessel_settlement`, used for the insert, the undo and the rows read back:
    - `vessel_settlement_commit()`: inserts the row and commits the caller's open transaction
      through `mysql_commit_transaction()`. An unanswered `COMMIT` is settled by a locking read
      of the row: there, the settlement stands; gone, it does not; unreadable, see item 05.
    - `vessel_settlement_pay()`: moves the gold, sets the marker, saves with
      `save_char_checked()`, and deletes the row (a failed delete is the reconcile's to repeat).
      If the save fails, the gold and the marker go back and the settlement is undone; "undone"
      is said only when the undo committed, and otherwise the player is told the account is held.
    - The undo, one transaction: the supply moved back by `supply_delta` inside the market's
      band, the contract reopened if this captain still holds it, the hold's lot changed by
      `cargo_delta` (never below zero; goods with no free bay to return to are left ashore and
      logged) and the manifest written, the fee owed again if the hull owes none (a runtime save
      rewrites the balance from memory at any time, so this part is a restore, not a delta), and
      the row deleted.
    - `vessel_settlements_reconcile()`: reads the open rows of a ship or a player (at most one
      each) and deletes or undoes each. It returns whether none is left.
04. The reconcile runs at login beside the two deliveries (`src/core/interpreter.c`; from inside
    the first of them in the end, see "Decided while building"), where a player whose settlement
    was undone is told; and as a gate at the start of `cargobuy`,
    `cargosell`, `contractaccept`, `contractdeliver`, `contractabandon` and `dockfees pay`, which
    refuse while a row of the ship or the player stays open. Delivering or abandoning a contract
    whose acceptance is unsettled would keep its freight or payout, so they wait too; they do
    not become two-phase (the payout is S15's).
05. A settlement whose outcome is unknown. When a `COMMIT` goes unanswered and the row cannot be
    read back (the usual case after a server restart), or an undo fails, memory is left without
    the settlement and the hull remembers the row's id (`settlement_unresolved`): if that row is
    there, its cargo is not in the hold in memory. The next reconcile then undoes it in the
    database only, or finds no row and forgets it. Until then `vessel_db_save_cargo()` refuses,
    after trying the reconcile itself: a manifest written from memory under an open row would
    have the lot undone twice after a reboot. A reboot forgets the id and reloads the hold from
    the manifest, which then agrees with the row.
06. `cargobuy` and `cargosell`: `trade_record()` writes the supply, the manifest and the row,
    and `trade_settle()` becomes `vessel_settlement_pay()`. The second write after an unanswered
    `COMMIT` (S11 review round 2) and "the trade stands and will be saved shortly" (S11 review
    round 1) go: the row settles both.
07. `contractaccept`: the claim, the manifest and the row in one transaction, through the same
    two functions. `contractdeliver` and `contractabandon` take the gate.
08. `dockfees pay`: the cleared balance (`vessel_db_save_runtime()`) and the row in one
    transaction, then the gold; the clan is credited only after the gold is saved, as now.
09. Removal: a purged ship's row goes with her other rows (`vessel_delete_persistence()`); a removed
    player's row is judged as any other, and with no file left it is undone.
10. Tests, DB-backed, in `unittests/CuTest/test_vessel_rewards.c` beside the S11 trade tests
    they replace (a new file in the end, see "Decided while building"), at each crash point, for a purchase, a sale, a freight acceptance and a fee
    payment: stopped after the ship side committed (row open, file without the marker: undone at
    login, by deltas, after the supply and the hold moved meanwhile); stopped after the gold was
    saved (row open, marker in the file: deleted, nothing undone); a failed save (undone at
    once); a failed save and a failed undo (held, cargo saves refused, undone by the next
    reconcile, and after a simulated reboot); an unanswered `COMMIT` both ways and unreadable;
    another player's row on the ship, judged from the file; the gate on all six commands; the
    marker's round trip through the player file. Real drops on persistent tables, as S13.
11. The economy gate: the session's trades leave no open row and Vesselmate's file carries the
    marker (two checks in the harness's economy branch), and the full gate passes.
12. Help and docs: SHIP TRADE (the entry `cargobuy` and `cargosell` share) says what happens to
    an account whose gold could not be saved, in `help.hlp` and `help_vessel_entries.sql` with
    the verifier; `VESSEL_SYSTEM.md` (trade, freight, dock fees, login, the tables and files);
    the player guide only if it quotes a changed message (it does not).

Interpretations:

- "Covered" is `marker == settlement_id`, not `<=` as `VIns` and `VMer` use. With one open row
  for a player the two agree, and equality does not depend on ids never restarting (the table is
  normally empty, so a re-created table would hand out ids below old markers).
- A settlement of another player on the ship is judged from that player's file. Holding the ship
  until that player logs in would let an absent helmsman's row stop the owner's trading.
- An undo that cannot be recorded no longer leaves the trade standing in memory. Memory is
  undone at once and the row waits for the reconcile; the player keeps the gold and is told the
  account is held. One undo path, and no state in which memory holds gold the file may never get.
- The dock fee's clan credit stays after the gold save and outside the settlement: the work item
  names the captain's gold and the ship's side.

Planning ablation:

- Dropped: a gold column on the row (the undo applies only when no file holds the gold, and the
  command that pays knows the amount); a kind or status column (a settled row is deleted, and
  the undo follows from which columns are set); the second write after an unanswered `COMMIT`
  and the "stands" branch; a search for the row's player among those online (memory equals
  file); a reconcile pass at boot or on a timer (login, the gate and the flagged cargo save
  reach every row); a change to the player-removal hook; making `contractdeliver` two-phase.
- Simplified: one undo serves the command, the login and the gate; unique keys hold a ship's
  and a player's accounts instead of code; the fee part of the undo is a restore.
- Kept: the file read for another player's row; `settlement_unresolved` and the cargo-save
  refusal (S13's review: an unreadable read-back is the usual case, so it needs a pending
  state); the gate on `contractdeliver` and `contractabandon`.

Built (2026-10-04), items 01-12 as planned except where "Decided while building" says otherwise:

- Phase 25 SQL (schema, rollback, verifier, manifest lines), `master_schema.sql`, and
  `vessel_settlement_ensure_schema()`, called from `vessel_trade_ensure_schema()`.
- `VSet` and `GET_VESSEL_SETTLEMENT()` in `structs.h`, `utils.h` and `players.c`.
- `src/vessels/vessels_settlement.c`: `vessel_settlement_commit()`, `vessel_settlement_pay()`,
  `vessel_settlements_reconcile()`, `vessel_settlement_gate()`, `vessel_settlement_forget_ship()`,
  and `settlement_unresolved` on the hull.
- `trade_record()` (supply, manifest and row in one transaction) replaces `trade_write()`, the old
  `trade_record()` and `trade_settle()`; `do_cargobuy`, `do_cargosell`, `do_contractaccept` and
  `do_dockfees` record and pay through the settlement; `contract_reopen()` is gone (the undo
  reopens the job); the gate is in the six commands and the reconcile in the login path.
- `vessel_db_save_cargo()` refuses for a hull that remembers a settlement, after trying the
  reconcile, and always inside another transaction.
- Help: MARKET (the SHIP TRADE entry) has a "Settling up" paragraph, CONTRACTS and VESSELS
  (`contractaccept`, `dockfees`) a sentence each, in `help.hlp` and `help_vessel_entries.sql`; the
  verifier checks the three phrases (58 content checks).
- `VESSEL_SYSTEM.md`: a "Two-phase settlements" passage in the economy model, the freight and
  dock-fee paragraphs, the table and file lists.
- The economy gate checks that the session's trades left no settlement open and that
  Vesselmate's file has a `VSet` line.

Decided while building:

- The tests are a new file, `unittests/CuTest/test_vessel_settlement.c` (nine tests), not more
  cases in `test_vessel_rewards.c`: they lose real connections, so they need the real tables, and
  the rewards file was already past 1,200 lines. S11's two trade tests moved there, rewritten;
  the rewards file keeps the freight and contraband tests on TEMPORARY tables, with a TEMPORARY
  `vessel_settlements`.
- The gate comes before any check that reads the hold or the fee: the undo it may run changes
  them. In `dockfees` it runs only for `pay`, before the balance is read.
- The undo reopens a contract by its id and its taken status, without the taker's name: while
  the settlement is open only its captain can hold the job, since delivering and abandoning wait.
- `vessel_delete_persistence()` makes sure the table exists before its transaction, as it does
  for the tables it already clears.
- The login reconcile is called from `vessel_deliver_pending_insurance()`, which
  `enter_player_game()` already calls, not from a new line in `src/core/interpreter.c`: no unit
  test runs the login path, and the coverage gate holds the changed lines of that file to its
  subsystem's floor (S6 put its boot hook inside a vessel boot function for the same reason).
  The delivery also runs when a claim is queued for an owner in the game, after that claim's
  transaction is committed; a reconcile there reads one row at most. The settlement tests enter
  through that function.
- The verifier counts the columns and the unique keys and lists nothing: a query on the table
  fails after the rollback.
- `GET_VESSEL_SETTLEMENT()` parenthesizes its argument; the two older marker macros do not, and
  are left alone.
- A reconcile reads two rows, the most the unique keys allow. More than two (a table without the
  keys) leaves the ship held until the next call.
- `save_char_checked()` returns FALSE after the file is in place only for a character missing
  from the player index, which no player in the game is; so a failed save means the file kept
  its old gold and marker, which the undo relies on.
- Messages. Paid: unchanged. Save failed and undone: "Your gold could not be recorded, so the
  trade (contract, payment) is undone." Not undone yet: "... could not be undone yet. No gold
  changed hands; the harbor office holds this ship's accounts until it is undone." Gate: "The
  harbor office is still settling an earlier account. Try again shortly." Login or gate, to the
  captain whose settlement was undone: "The harbor office never recorded your gold for a cargo
  trade (freight contract, dock-fee payment) aboard <ship>, so it has been undone."

Found before hand-off, and fixed:

- An independent read of the build (a side agent, read-only, on `8ed5d8a3a`) found no way to
  free cargo, a double sale or a lost bond or fee from a single fault, and two cases that need a
  second, rare one:
  - A `COMMIT` that got no reply and could not be read back may still be on its way (a stalled
    server). A reconcile read no row, took the settlement for never recorded and forgot it; the
    `COMMIT` then landed, and the undo changed a hold that no longer held the trade. A hull's
    remembered settlement is now forgotten only after a locking read of its row, which waits
    for the lost session.
  - A hull that boot could not rebuild ("leaving persistence intact") was taken for gone: the
    undo moved the port's stock, skipped her manifest and fee, and deleted the row. Her
    settlement is now kept until she is loaded (or purged, which deletes it), and its captain's
    accounts wait. Since a purge deletes the row, a settlement never outlives its hull's rows,
    so "a lost ship" has no case left.
- The same read named four things no test ran; each has one now: a mob's trade (undone in the
  command), the manifest refused inside another transaction, a sale and a fee payment made right
  after their gate undid an earlier settlement (unpaid goods cannot be sold; the fee is owed
  again and then paid), and the two fixes above (a second session holds the row uncommitted; a
  slot that is empty and then loaded). Ten tests in all.
- The damage gate failed once in the batch on a race of its own: the tick that gets the hull
  under way after `strikecolors` can land with the output of `speed 1`, and the check looked
  for "colors fly again" only in what followed. It now accepts the line in either
  (`scripts/development/dev_kohdee_login_smoke.sh`).

Still open outside these steps: the production deploy of S9's world-data notes and S10's, S11's,
S12's and S13's code (S12's with schema Phase 24, which boot adds, and S13's with migration
`2026100401`, which boot applies) and the review fixes, the Open player-data balance and human
beta gates in `VESSEL_SYSTEM_REQUIREMENTS.md`, and closing these study documents once S15 merges:
`docs/ongoing-projects/` is temporary, and their enduring content lives in `VESSEL_SYSTEM.md` and
the guide.

## Estimate (remaining)

| Step | What drives the size | Days |
| -- | -- | -: |
| S14 Two-phase vessel settlements | Schema phase with rollback and verifier, player-file marker, login reconcile, four commands, crash-point tests, the economy gate | 2 |
| S15 Checked vessel purchases and payouts | One shared helper, fifteen gold movements in twelve commands, failure tests per command family | 1.5 |

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
