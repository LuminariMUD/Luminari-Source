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
| S15 Checked vessel purchases and payouts (work item #14) | In review: tag `vessels-s15`, MR !22, review round 1 fixed on top | [Phase 15](#phase-15-s15-progress) |

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
#14, which is built, verified and in review as MR !22.

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

S15 is the active step and the last in the sequence: built, verified and handed to review as
MR !22, its review round 1 fixed on top, not merged. Its record follows the open items; "After
the merge" at its end says what is left to do.

Still open outside these steps: the production deploy of S9's world-data notes and S10's, S11's,
S12's, S13's and S14's code (S12's with schema Phase 24, which boot adds, S13's with migration
`2026100401`, which boot applies, and S14's with the `vessel_settlements` table, which boot
creates) and the review fixes, the Open player-data balance and human beta gates in
`VESSEL_SYSTEM_REQUIREMENTS.md`, and closing these study documents once S15 merges:
`docs/ongoing-projects/` is temporary, and their enduring content lives in `VESSEL_SYSTEM.md` and
the guide.

### Phase 15 (S15) progress

Built, verified and handed to review (2026-10-04): tag `vessels-s15`, MR !22 (`Closes #14`,
squash and remove-source-branch off), range `vessels-s15-base..vessels-s15`. Branch
`feat/vessels-s15` from the S14 merge `e7bc5242c` (tag `vessels-s15-base`); the S14 close-out
`16c6611fb` comes first, then the plan `c295d6aa8`, the build `aec826b19`, the gate step
`6a1ada514` and `77cc74137`, and the fixes found before hand-off `bc2a45b69`. Review fixes go
on top, one commit each.
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
  (Not so in the end: a refused summons leaves all of it as it was, see "Found before hand-off".)
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
  that still stands; the command writes the emptied manifest (checked, which
  `vessel_save_one()` does not do) and `vessel_save_hull()`, and then takes her out with
  `vessel_restow()`. `vessel_stow()` is left to the wreck registry.
- The trade-in's undo and `shipbuy`'s refused hull write have no unit test: no unit world
  generates ship interiors (the room vnums come from the fleet slot, the rooms from the zone
  table and the runtime room list). `test_vessel_shipyard.c` covers the charge that cannot be
  saved for both, and the refund of a hull the yard cannot deliver. The refused write is played
  in the loss gate instead: Kohdee lists a design, the harness adds a CHECK constraint to
  `ship_runtime_state` that refuses that design's runtime row, and in the two-character loss
  session Vesselmate, who owns only the check's boat, buys a hull of the design and trades the
  boat in for one. The session checks the two messages, his purse before and after, and that
  she is still a boat with her owner and her bosun; then the session's real trade-in rebuilds
  the same hull as a warship. The trade-in's payment that cannot be saved (the same undo) is
  played nowhere: no test can make a player file unsaveable in the middle of a live command.
- Help in the five entries that hold the commands, not only the overview entry item 11 named:
  a captain reads SHIPHIRE or CONTRACTS, not VESSELS, when a purchase is refused.
- `scripts/ci/sql_interpolation_baseline.txt` drops by one: `contractdeliver`'s status update is
  a prepared statement now.
- Messages. Paid or sold: unchanged. A price that cannot be saved: "Your payment could not be
  recorded; no gold was taken." A job that cannot be written: "The harbor office could not
  record that, so nothing was done and your N gold is returned." A sale: "... so nothing was
  sold." and "Your payment could not be recorded, so the sale is undone." `shipbuy`: the
  spawn's own message, then "Your N gold is returned." The trade-in: "The shipwrights cannot
  complete the trade, so it is undone: <ship> is rebuilt as she was and no gold changes hands."
  `shipsummon`: "The harbor master cannot record the summons, so <ship> stays where she is and
  your N gold is returned." The admiralty: "The clerk cannot record the settlement (complete
  the commission); your N gold is returned." `contractdeliver`: "The freight office cannot
  record the delivery; the freight stays aboard.", "Your payment could not be recorded, so the
  delivery is undone.", and, when neither the delivery nor taking it back can be confirmed,
  "The freight office cannot confirm whether the delivery was recorded. The freight stays
  aboard, and the staff have been told."

Found before hand-off, and fixed (`bc2a45b69`):

- The loss gate's first version of the refused purchase ran as Kohdee alone, and his trade-in
  took a harbor fixture: he owns `Persistence_Tern` and `Persistence_Goshawk`, berthed at the
  two docks, and a trade-in takes the owner's first hull berthed there. On the disposable
  database the undo put the fixture back and the gate passed without having tested the raft.
  The check moved into the loss session (`77cc74137`), where the captain owns one hull.
- An independent read of the build (a side agent, read-only, on `aec826b19`) found no path
  where a single failed write loses or duplicates gold or an item, and these, all fixed:
  - A refused summons had already cast off the hull alongside, put her vehicles off and
    stopped her autopilot, while the captain was told she "stays where she is". The summons
    now saves her as cast off and stopped first (her autopilot's two fields are remembered
    beside the copy of the hull), and withdraws the gangway, releases the vehicles and puts
    those aboard ashore only once it is saved.
  - The summons wrote the stowed hull before the emptied manifest, so a crash between the two
    brought her in with her cargo. The manifest goes first.
  - `vessel_save_one()` had begun to stop at a failed owner, crew or refit write, skipping the
    permits, manifest, pilot and schedule that shutdown's fleet save and the wreck registry
    rely on. It attempts every part again and fails if any of the checked ones did;
    `vessel_save_hull()` alone stops at the first.
  - A trade-in whose rebuild failed returned the charge and left the hull without an interior.
    It now takes the same undo as a trade that cannot be recorded. The undo no longer writes
    a hull whose rooms could not be recreated (her rows would say she has none).
  - A spawn wrote the runtime row second. When its rollback's purge failed too, the rows left
    behind became a hull at the next boot, and `shipbuy` had returned the price. The runtime
    row is written last: without it boot leaves the slot held (S14) and builds nothing.
  - A sale that stands rewrote nothing, though a put-back that failed part way may have
    written her weapons back. It writes the hull once more.
  - A delivery whose transaction and whose put-back both fail (the database gone in
    mid-command, after a `COMMIT` that may have been made) told the captain only that the
    freight stays aboard. He and the staff are now told the books may be wrong.
- Left as it is, on two faults: the bounty pay-off and `marque` return the fee when the write
  reports a failure and the row cannot be read back. If the server had made the write before
  it went away, the bounty is cleared or the letter issued unpaid. The other choice keeps the
  fee of a captain whose write never arrived, which is the likelier case when the database
  goes away; the failed write is in the log either way.
- The same read named what no test ran. Added: a refused summons under autopilot with a hull
  alongside, by the manifest's lost reply and by the runtime row's; the delivery's manifest
  half lost, and both its transactions failing; `vessel_save_one()` writing the manifest
  after a failed owner write; a mob's charge. Not testable in a unit world, or needing two
  faults at once: the trade-in past its charge and `shipbuy` past its spawn (the loss gate
  plays them), a lost reply on the prepared bounty statement, a refund whose own save fails,
  and a write-back that fails after a sale's refused write.
- The local CI matrix's first run (`77cc74137`) passed 32 of 33 jobs: clang-tidy refused the
  tests' `memcmp()` of two hulls (a struct with padding has no unique representation). The
  comparison is one helper with a suppression: both are whole copies of one fleet slot.

Verification (2026-10-04), on `bc3f39c30`, whose code is `bc2a45b69`'s (the commit between
them changes this document only):

- `make test-all` with the database cases on: 2,034 production tests and 32 protocol tests pass
  (the test database is the `luminari-vessels-testdb` container, `master_schema.sql` plus what
  the boot ensure functions add).
- The vessel help verifier on the test database, after `help_vessel_entries.sql`: 7 checks
  pass, 64 content phrases. No new SQL.
- Live gates, in the private-namespace harness on a reload of the development dump
  (`/tmp/claude-1000/vs4`, jobs `stage-s15/k01`-`k22`, run as `m01`-`m22`): 22 of 22 jobs pass
  in 47 minutes. Merchant 39 s, campaign provisioning 146 s, Vailand merchant 20 s, builder,
  gunnery 76 s, tactical 282 s, lookout 22 s, boarding 48 s, narrative 22 s, rules 36 s, events
  41 s, movement 104 s, loss 84 s, damage 547 s, derelict 31 s, hunter 90 s, frontier 221 s,
  raider 174 s, economy 240 s, client 25 s, and the economy gate with every database session
  killed twice 313 s.
- The loss gate's new step: with the database refusing the listed design's runtime row,
  Vesselmate's `shipbuy` reported the rolled-back spawn and "Your 8000 gold is returned.", his
  trade-in "The shipwrights cannot complete the trade, so it is undone: Losscheck Tern is
  rebuilt as she was and no gold changes hands.", his purse was 57,938 before and after, the
  boat was still a boat with her owner and her bosun, and the session's real trade-in then
  rebuilt the same hull as a warship.
- The local CI matrix, `scripts/ci/local/run.py --base gitlab/master`: 33 of 33 jobs pass in
  554 s. The first run, on `77cc74137`, passed 32 (see "Found before hand-off").
- Before the fixes, on `77cc74137`: `make test-all` passed on the build, and the first eleven
  gates of a batch passed before it was stopped to make the fixes.

Review round 1 (2026-10-04, range `vessels-s15..feat/vessels-s15`): four findings on MR !22, one
[P2] and three [P3], two shown by the reviewer's probe tests on `6c0975cd1` and two read. All
fixed, one commit each.

- [P2] A summons put the carts aboard ashore with the shipyard's coordinates (`a1a02f325`). The
  summons is saved before anything aboard is touched ("Found before hand-off"), so by the time
  her vehicles were released the hull's own position was already the summoning shipyard's, and
  `vehicle_release_all_from_vessel()` copies that position into each vehicle. A vehicle's
  coordinates are where it drives from and where boot puts it: one `drive` carried the cart and
  its riders to the shipyard, and after a boot the cart stood there. The vehicles are released
  from the copy of the hull taken before the summons. The summons test carries a cart.
- [P3] A purchase whose write-back failed was called undone and refunded while her rows kept it
  (`c53d1601e`). The two failures are usually one event: a reply lost on a statement the server
  ran is the start of an outage, and the write-back cannot reconnect. Nothing saves the fleet
  periodically (`save_all_vessels()` runs at copyover and at shutdown), so the rows kept the
  purchase until her next shipyard job. This replaces "a single failed write is undone and
  reported" above for the case where the undo fails too:
  - `vessel_purchase_recorded()` puts the bought hull back in memory, writes her once more and
    keeps the price, as a sale stands. "Nothing was done" is said only after a write-back that
    succeeded.
  - `shipsummon` carries on as summoned and the fee stays paid. A summons can then stand with
    none of its writes made, her full manifest still in the rows, so a hull that makes port
    saves her manifest before her runtime row (`vessel_summon_arrive()`).
  - The trade-in has rebuilt the old hull by then. `vessel_trade_in_undo()` reports whether she
    is as she was in play and in her rows; if not, the charge is kept, and the captain and the
    staff (`mudlog()`) are told, who settle it.
  - `vessel_charge()` asks the database for an answer before it takes the gold, so one that is
    away before the command refuses the purchase uncharged ("The harbor's records cannot be
    reached; no gold was taken."). Without it every purchase in an outage would stand unsaved.
- [P3] A refunded `shipbuy` could leave a hull the next boot rebuilt for the buyer
  (`c584bdab0`). The spawn's rollback did not look at whether its delete committed. With the
  reply to her runtime row lost (the last write, and made) and the database out of reach for
  the delete, her whole record stayed with no hull in memory to write over it. The rollback
  now deletes her rows before it takes her out of play, as `shippurge` does with a live hull;
  when the delete does not commit she stays in play, the spawn returns her slot and the price
  stays paid. This replaces "rows a failed rollback leaves behind hold her slot" above: the
  runtime row still goes last, for a crash in mid-spawn.
- [P3] A refused summons was written back manifest first (`e5fcba5d3`), the order the forward
  path avoids: with her stowed runtime row written and its reply lost, a stop between the two
  writes left her due at the shipyard with her hold full. The hull is written back first.

New messages: "The harbor's records cannot be reached; no gold was taken." and, for the
trade-in, "The shipwrights cannot complete the trade, and the harbor office cannot put its
records of <ship> right. Your N gold stays paid until the staff settle it; they have been told."
No help entry changes: what a captain is told when one write fails is as the entries say, and
they do not describe a database outage. `VESSEL_SYSTEM.md` follows in `94c3c5a9a`.

Ablation (review fixes): the database check lives in `vessel_charge()`, one place for twelve
commands, not at each command; the trade-in keeps the charge for the staff instead of building
the new hull a second time; no retry of the save in the spawn (its delete is one transaction
and leaves her rows as the failed save left them); no second kind of record for a purchase that
stands (the log line, and her next save); no test for the write-back's order (a unit test cannot
stop between two writes, and a failed write-back is now followed by a second write of the
summons); no permanent gate step for the two-fault cases (a trigger that refuses deletes belongs
in a one-off job, not in the loss gate).

Verification of the fixes (2026-10-04, on `94c3c5a9a`):

- `make test-all` with the database cases: 2,036 production tests and the 32 protocol tests
  pass. Two tests are new (a hire and a summons that stand: the reply lost with the database
  out of reach for the write-back), and the fixture gained `vessel_test_database_away()`. With
  the old argument the cart's check fails (`expected <10> but was <40>`).
- Live, in the private-namespace harness on a reload of the development dump, a database that
  refuses both a write and its undo: a CHECK constraint on `ship_runtime_state` refused the
  runtime row of Kohdee's `Persistence_Goshawk` (ship 3) and of a new warship design, and a
  trigger refused every delete from `ship_interiors`. `shiphire gunner green` signed the gunner
  on and kept its 2,400 gold; `shipbuy <design> trade` reported that the records could not be
  put right and kept its 2,654 gold, with the staff line in the log; `shipbuy <design>` handed
  over the hull in slot 13 and kept its 44,000 gold; `shipsummon` from the west dock stood for
  28 gold and she made port there. The log named each of the five cases. With the refusals
  dropped, her next job (`shiphire bosun green`) wrote both hands and the west dock into her
  rows, and the bought hull's christening wrote her runtime row.
- The gates that run the changed commands, in the same harness on the same binary after a fresh
  reload, all pass: loss (its refused purchase and trade-in undone and refunded as before),
  gunnery 87 s, rules 47 s, damage 642 s and economy 220 s.
- The local CI matrix, `scripts/ci/local/run.py --base gitlab/master`: all 33 jobs pass in 666 s
  (`--jobs 3 --cpus 4`), the coverage policy, clang-tidy, both sanitizer jobs and the
  memory-check job among them.

After the merge (merge commit, never a squash; keep the branch):

- Help sync to production. Five entries changed: SHIPHIRE (the SHIP-CREW entry), SHIPBROWSE
  (SHIP-OWNERSHIP), PLUNDER (the piracy entry, BOUNTY PAY and MARQUE), CONTRACTS, and SHIPFIRE
  (SHIP-COMBAT, SHIPREPAIR). Apply `help_vessel_entries.sql` to the development database
  first, then `sync --authorize-production`.
- Move this section to the history document and set the status row to the merge.
- S15 is the last step: close these study documents, as "Still open outside these steps" says.
- The production deploy then also carries S15's code. It has no schema change.

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
