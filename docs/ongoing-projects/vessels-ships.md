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
| S14 Two-phase vessel settlements (work item #12) | Merged `e7bc5242c` (MR !21) | [Phase 14](vessels-ships-history.md#phase-14-s14-progress) |
| S15 Checked vessel purchases and payouts (work item #14) | In progress on `feat/vessels-s15`: built, being verified | [Phase 15](#phase-15-s15-progress) |

Production help is current through S14 (help sync plan `20d457173b87`, 2026-10-04). S1-S14 are
merged: the study's steps, S1-S8; S-immediate, which readied the local Luminari Web client for
S9; S9, which played the whole system in game and recorded it; S10, which turned that record into
the [Vessel Player Guide](../guides/VESSEL_PLAYER_GUIDE.md) and fixed what checking its facts
against the code found; S11, which made `cargobuy` and `cargosell` record a trade before the gold
moves and save the gold checked (GitLab work item #10); S12, which gave waypoints and routes a
creator who alone (with the staff) may change them (work item #11); S13, which keeps a database
transaction whole when its connection is lost or its `COMMIT` goes unanswered (work item #13);
and S14, which settles a cargo trade, a freight acceptance and a dock-fee payment in two phases,
so a crash between the ship's side and the captain's gold leaves neither without the other (work
item #12). One follow-up remains in the sequence, resolved in this worktree: S15 for work item
#14, which is the active step.

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

S15 is the active step and the last in the sequence. Its record follows the open items.

Still open outside these steps: the production deploy of S9's world-data notes and S10's, S11's,
S12's, S13's and S14's code (S12's with schema Phase 24, which boot adds, S13's with migration
`2026100401`, which boot applies, and S14's with the `vessel_settlements` table, which boot
creates) and the review fixes, the Open player-data balance and human beta gates in
`VESSEL_SYSTEM_REQUIREMENTS.md`, and closing these study documents once S15 merges:
`docs/ongoing-projects/` is temporary, and their enduring content lives in `VESSEL_SYSTEM.md` and
the guide.

### Phase 15 (S15) progress

Planned and built (2026-10-04); the verification is under way. Branch `feat/vessels-s15` from the
S14 merge `e7bc5242c` (tag `vessels-s15-base`); the S14 close-out `16c6611fb` comes first, then
the plan `c295d6aa8`. Hand-off as in the routine: tag `vessels-s15` and a merge request that says
`Closes #14`; review fixes go on top.
Scope: [work item #14](https://gitlab.com/max757/Luminari-Source/-/work_items/14) and its note.

The model. Each of the fifteen movements has the captain's gold in the player file and a ship side
in MariaDB, and no record spans the two. Each command writes both stores inside itself and checks
both.

- A purchase takes the gold first: the debit is saved with `save_char_checked()`, and a failed
  save puts the gold back and refuses the purchase before anything aboard has changed. Then the
  ship side is written. If that write fails, the hull is put back as she was in memory and
  written back, and the gold is returned.
- A payout writes the ship side first: a failed write puts the item back and pays nothing. Then
  the gold is paid and saved. A failed save takes the gold back and puts the item back, in memory
  and in the database.

A single failed write is undone and reported. Only a crash between a command's two writes splits
the stores, and always against the player: a purchase paid for and not delivered, or a sale
delivered and not paid. Nothing is given free.

Items:

01. The helper, a new `src/vessels/vessels_payment.c` (both build lists, the parity check):
    - `vessel_gold_saved()`: the body of the passenger fare. It moves the gold (a signed amount),
      saves, and puts the gold back on a failed save. `vessel_collect_passenger_fare()` calls it.
    - `vessel_charge()`: a purchase's first half. It tells the buyer when no gold was taken.
    - `vessel_refund()`: gives gold back and saves. A refund whose save fails stays with the
      captain in memory and is logged; the per-minute save stores it.
    - `vessel_purchase_recorded()`: a purchase's second half for work on a hull. It writes the
      hull (item 02); on a failure it restores her from the copy taken before the work, writes
      her back, refunds and tells the buyer.
    - `vessel_sale_recorded()`: a sale off a hull. It writes the hull and pays, and puts her back
      if either fails.
02. `vessel_save_hull()` in `vessels_db.c`: the interior row, the runtime state, the weapons, the
    owner, the crew and the refits, each checked. It is everything a shipyard job changes, so one
    checked write serves every job. `vessel_save_one()` becomes that plus what it wrote besides
    (permits, cargo, pilot, schedule). `vessel_db_save_crew()` and `vessel_db_save_extras()`
    return their result.
03. Through `vessel_charge()` and `vessel_purchase_recorded()`: `shiphire`, `shipweapon buy`,
    `shipequip buy`, `shiprearm`, `shiprepair`, `shipupgrade` and `shipchristen`. Each copies the
    hull before it changes her, and the copy is the undo. `shipchristen` renames the hull object
    and the rooms only after the write.
04. `shipbuy`: the charge, then the spawn, which already rolls itself back when the hull cannot
    be saved; a failed spawn refunds.
05. The trade-in. What the captain owes before the fittings are counted (the price less the
    credit) is charged before the rebuild. The rebuilt hull is saved with `vessel_save_one()`
    (now checked), then the shipwrights pay for the fittings she cannot carry, with any credit
    left over. If the save or that payment fails, the rebuild is undone (the new interior
    reclaimed, the hull restored from the copy taken before the work, her old rooms recreated the
    way boot recreates them, and she is written back) and the charge is refunded.
06. `shipsummon`: the charge, then the summons. `vessel_stow()` returns whether the stowed hull
    was saved. If not, she is put back where she was (restored from the copy, her hull object
    recreated the way a summoned hull's is when she makes port, and written back) and the fee is
    refunded. Those aboard are put ashore only once the summons is saved.
07. The bounty pay-off and `marque`: the charge, then the row; a refused row refunds.
08. `shipweapon sell` and `shipequip sell`: `vessel_sale_recorded()`.
09. `contractdeliver`: the contract marked done and the manifest in one transaction, through
    `mysql_commit_transaction()`, then the payout. A failed save loads the freight again and
    writes the contract back as taken.
10. Tests, DB-backed, on the real tables with real drops (`mysql_test_drop_connection_at()`) and
    an unsaveable player file, for each command: a failed gold save (nothing changed in either
    store) and a refused ship-side write (the hull as she was, the gold as it was, the rows as
    they were), and the success path's two stores (the file holds the new gold and the rows hold
    the work). The tests that already buy or sell through these commands get both stores.
11. Help and docs: the vessel overview entry says what happens to a purchase or a sale that
    cannot be recorded, in `help.hlp` and `help_vessel_entries.sql` with the verifier;
    `VESSEL_SYSTEM.md` describes the checked purchases and payouts.
12. Verification as in the routine: `make test-all` with the database cases on, the help
    verifier, the live gates and the local CI matrix.

Interpretations:

- The order follows the work item: gold first for a purchase (refused, or refunded), the ship
  side first for a payout (put back). A crash between the two writes then costs the player,
  never the economy.
- "The ship-side write" of a shipyard job is the whole of `vessel_save_hull()`, not only the
  table the job changed. One undo (the copy of the hull) and one write-back then serve seven
  commands, and a command cannot check too little.
- The write-back. A ship side of several statements can fail part way, so after the hull is
  restored in memory she is written again. Without it the rows would keep a half-recorded
  purchase until shutdown: the fleet is saved as a whole only then.
- A sale or a delivery whose gold cannot be saved and whose put-back cannot be written stands:
  the gold is paid in memory and the per-minute save stores it. Memory then equals the database,
  and "undone" is never said of a sale the database still holds (S11's review round 1).
- A server without a database refuses these purchases: the ship side cannot be written. The
  game does not run without one; the tests that bought and sold in memory only get the database.
- `shipupgrade` also saves the runtime row. It raised the armor, structure or speed ceilings in
  memory and saved only the refit bit, so a crash kept "installed" without its points.
- The trade-in moves gold twice: the charge known before the rebuild, and what the shipwrights
  pay after it. One movement would need the fittings counted before the rebuild, on a copy of a
  hull that does not exist yet.
- What a failed summons leaves: a docking cast off, vehicles put off, the autopilot stopped.
  Those happen before the hull is stowed and are not undone; the hull, her cargo and the gold are.
- `contractdeliver` after a `COMMIT` without a reply: both writes of its transaction are absolute
  (the contract's status, the whole manifest), so the transaction is sent once more, as the
  query layer allows ("or writes again"). If that fails too, the delivery counts as not recorded
  and the hold's state is written back; no read-back and no pending state.
- A mob cannot buy: it has no player file to save. `shipbuy` and `shipsummon` already refuse
  mobs, and no mob owns a hull.

Planning ablation:

- Dropped: a "no database, nothing to store" branch for tests that buy in memory (a production
  rule that only tests would use); a save callback or a saver per command (item 02 serves all);
  a transaction inside `vessel_db_save_crew()` (the write-back covers its delete-then-insert);
  a quote of the trade-in's fittings on a scratch hull; a read-back and a pending state for
  `contractdeliver`'s unanswered `COMMIT`; holding the shipyard commands behind the settlement
  gate; a rewrite of the existing tests' connection helpers.
- Simplified: the copy of the hull is the undo of every in-memory change; `shipchristen` does its
  cosmetic renames after the checked write, so they need no undo; the trade-in's and the
  summons's undo reuse the paths boot and an arriving hull already take; one fixture module gives
  a test both stores.
- Kept: the write-back; "the sale stands" (a reviewer found its absence in S11); the undo of the
  trade-in and of the summons, without which a failed hull write would leave a paid fee and the
  old hull in the database until shutdown.

Built (2026-10-04), items 01-11 as planned except where "Decided while building" says otherwise:

- `src/vessels/vessels_payment.c` (both build lists): `vessel_gold_saved()`, `vessel_charge()`,
  `vessel_refund()`, `vessel_purchase_recorded()` and `vessel_sale_recorded()`. The passenger
  fare calls the first.
- `vessel_save_hull()`; `vessel_save_one()` is that plus permits, cargo, pilot and schedule, so it
  now stops at an owner, crew or refit write that fails. `vessel_db_save_crew()` and
  `vessel_db_save_extras()` return their result.
- The seven shipyard jobs, `shipbuy` and its trade-in (`vessel_trade_in_undo()`), `shipsummon`,
  the bounty pay-off and `marque`, the two sales, and `contractdeliver` (`contract_record()`),
  each in its own source file.
- Tests: a new `unittests/CuTest/test_vessel_payment.c` (four tests: the seven jobs, the sales,
  the admiralty's fees, the delivery) and the fixture `test_vessel_stores.c` and `.h` (the test
  database as the game's connection with a hull's row, a scratch player directory, the gold a
  player file holds). Cases for `shipsummon` in `test_vessel_loss.c` and for `shipbuy` and the
  trade-in in `test_vessel_shipyard.c`. Seven tests that already bought or sold got both stores
  (the weapons, hire, dock repair, summons, rename and hiring-hall tests, and the end-to-end
  freight delivery), and the bounty test a player file.
- Help: a "Paying the yard" paragraph in SHIP-CREW (the entry SHIPHIRE opens) and a sentence
  each in SHIP-OWNERSHIP, PIRACY (BOUNTY PAY, MARQUE), CONTRACTS (CONTRACTDELIVER) and
  SHIP-COMBAT (SHIPREPAIR), in `help.hlp` and `help_vessel_entries.sql`; the verifier checks the
  five phrases (64 content checks).
- `VESSEL_SYSTEM.md`: a "Checked purchases and payouts" passage in the economy model, the
  freight, refit and summons paragraphs, the component and file lists.
- The loss gate plays a purchase and a trade-in the database refuses to record
  (`scripts/vessels/test_vessel_tactical_in_game.sh`, `VESSEL_SYSTEM_TESTING.md`).

Decided while building:

- What "a refused write" is. Outside a transaction, a statement sent on a connection that has
  just been lost does not fail: the client library reconnects and sends it again. A single
  statement fails only with its reply lost, after the server has run it. So the write-back is
  what undoes a refused purchase in the rows, and the tests lose the reply of the
  single-statement writes (`mysql_test_drop_connection_at(..., TRUE)`) and the connection before
  the statements inside a transaction.
- The bounty pay-off and `marque` read the row back when their write reports a failure, for the
  same reason: the fee is returned only if the row does not show the write. An unreadable row
  counts as not written, and the fee is returned.
- `shipsummon` is saved before the hull leaves the world, so a summons that cannot be saved has
  no hull object to recreate: the undo is the copy of the hull. For that,
  `vessel_db_save_runtime()` saves a stowed hull's own location, not the room of a hull object
  that still stands; the command writes `vessel_save_hull()` and the emptied manifest (checked,
  which `vessel_save_one()` does not do for the manifest) and then takes her out with
  `vessel_restow()`. `vessel_stow()` is left to the wreck registry.
- A trade-in whose rebuild fails ("the shipwrights botch the work") returns the charge; the
  hull is as broken as before S15.
- The trade-in's undo and `shipbuy`'s refused hull write have no unit test: no unit world
  generates ship interiors (the room vnums come from the fleet slot, the rooms from the zone
  table and the runtime room list). `test_vessel_shipyard.c` covers the charge that cannot be
  saved for both, and the refund of a hull the yard cannot deliver. The refused write is played
  in the loss gate instead: the harness adds a CHECK constraint to `ship_runtime_state` that
  refuses one design's runtime row, Kohdee buys that design and trades his raft in for it, and
  the gate checks the two messages, his player file's gold, the rows, and the same trade-in
  once the constraint is dropped.
- Help in the five entries that hold the commands, not only the overview entry item 11 named:
  a captain reads SHIPHIRE or CONTRACTS, not VESSELS, when a purchase is refused.
- `scripts/ci/sql_interpolation_baseline.txt` drops by one: `contractdeliver`'s status update is
  a prepared statement now.
- Messages. Paid or sold: unchanged. A price that cannot be saved: "Your payment could not be
  recorded; no gold was taken." A job that cannot be written: "The harbor office could not
  record that, so nothing was done and your N gold is returned." A sale: "... so nothing was
  sold." and "Your payment could not be recorded, so the sale is undone." `shipbuy`: the
  spawn's own message, then "Your N gold is returned." The trade-in: "The shipwrights cannot
  record the trade, so it is undone: <ship> is put back as she was and no gold changes hands."
  `shipsummon`: "The harbor master cannot record the summons, so <ship> stays where she is and
  your N gold is returned." The admiralty: "The clerk cannot record the settlement (complete
  the commission); your N gold is returned." `contractdeliver`: "The freight office cannot
  record the delivery; the freight stays aboard." and "Your payment could not be recorded, so
  the delivery is undone."

## Estimate (remaining)

| Step | What drives the size | Days |
| -- | -- | -: |
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
