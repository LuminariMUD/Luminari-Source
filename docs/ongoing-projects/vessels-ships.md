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
| S13 Transactions that survive a lost connection (work item #13) | In progress on `feat/vessels-s13` | [Phase 13](#phase-13-s13-progress) |
| S14 Two-phase vessel settlements (work item #12) | Not started | [Part 5](#part-5-implementation-sequence) |
| S15 Checked vessel purchases and payouts (work item #14) | Not started | [Part 5](#part-5-implementation-sequence) |

Production help is current through S12 and its review fixes (help sync plan `e0d8a08faa27`,
2026-10-03). S1-S12 are merged: the study's steps, S1-S8; S-immediate, which readied the local
Luminari Web client for S9; S9, which played the whole system in game and recorded it; S10, which
turned that record into the [Vessel Player Guide](../guides/VESSEL_PLAYER_GUIDE.md) and fixed what
checking its facts against the code found; S11, which made `cargobuy` and `cargosell` record a
trade before the gold moves and save the gold checked (GitLab work item #10); and S12, which gave
waypoints and routes a creator who alone (with the staff) may change them (work item #11). Three
follow-ups join the sequence, all resolved in this worktree: S13 for work item #13, S14 for work
item #12 and S15 for work item #14, the three vessel work items still open.

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

S13, on `feat/vessels-s13`. S14 follows from S13's merge, on `feat/vessels-s14` with
`vessels-s14-base`, and S15 from S14's, on `feat/vessels-s15` with `vessels-s15-base`. Each step's
merge request says `Closes #13`, `Closes #12` or `Closes #14`, which lists it on its work item and
closes the item when it merges.

### Phase 13 (S13) progress

In progress (2026-10-04). Branch `feat/vessels-s13` from master `ec8f55b26` (the S12 review fixes'
merge and its record), where the annotated tag `vessels-s13-base` stands. The four planning commits
that put S13, S14 and S15 into Part 5 come first (`b15a8bc60`, `b1a42c2c3`, `5e4ee1425`,
`92efc3d73`), then this plan. Hand-off as in the routine: tag `vessels-s13` and a merge request
that says `Closes #13`; review fixes go on top. Scope:
[work item #13](https://gitlab.com/max757/Luminari-Source/-/work_items/13) and its two notes.

What the client library does, probed on the test MariaDB with real drops (`shutdown()` of the
connection's socket) before planning:

- A statement after a drop inside a transaction fails once with 2006 (the library refuses to
  reconnect and clears its in-transaction flag). The next statement reconnects silently and runs
  in autocommit on a new session.
- A statement whose reply is lost fails with 2013 and leaves the flag set; a `COMMIT` lost that way
  had committed, and a locking read on a new session saw the committed row at once.
- `mysql_ping()` retries by itself: inside a transaction, after a drop, it returns success on a new
  session. So does the `COM_STMT_CLOSE` that `mysql_stmt_close()` sends after a failed execution.
  When the close is the first command to meet the drop, it fails without any statement having
  failed, and the next statement autocommits.
- Data stored on a handle with `MARIADB_OPT_USERDATA` survives a reconnect and dies with the handle.

Items:

01. The rule, in `src/database/mysql.c`. A connection is marked when, with the library's
    in-transaction flag set, a command fails with 2006 or 2013, fails with the flag cleared (how
    the library leaves a reconnect it refused or could not make), or changes the session. A marked
    connection sends nothing: `luminari_mysql_query()`, `mysql_stmt_prepare_query()` and
    `mysql_stmt_execute_prepared()` refuse. The mark ends with the transaction: `ROLLBACK` is sent
    (on a new session if need be), a `COMMIT` is answered with a `ROLLBACK` and fails, and a new
    `START TRANSACTION` starts clean. The two pings (`ensure_mysql_connection()` and the pool's) and
    `mysql_stmt_cleanup()` mark as well. A failed `COMMIT` or `ROLLBACK` clears the library's stale
    flag, so the statement after it is not refused for a transaction that is over. The mark
    does not outlast the game pulse it was set in (see "Found while building").
02. `mysql_commit_transaction()`: committed, refused (the server said no, or the transaction was
    already lost) or unanswered (any client error, `trade_commit()`'s test); anything but committed
    is rolled back there. `trade_commit()` goes, and `trade_write()` calls the helper.
03. One test seam, `mysql_test_drop_connection_at()`: at the Nth statement that begins with a
    given text, a query or a prepared execution, the socket is really shut down, before the
    statement or (plain queries) after it was sent. It replaces `vessel_trade_lose_commit_reply_for_test()`, and the trade test moves to real drops on
    persistent tables.
04. Unanswered `COMMIT`, read back with a locking read before a state is chosen: `pet_store_pet()`
    (is the row stored), `pet_retrieve_stored()` (is it active), `vessel_transfer_owner()` (who
    owns the hull), `vessel_event_finish()` (the event's status).
05. Object saves (`src/obj/objsave.c`, `src/obj/house.c`):
    - The object and sheath writers return whether the row was written, and `Crash_save()` and
      `House_save()` count the rows that were not, their recursive calls included.
    - A save commits what it could write. With unwritten rows it is incomplete: `PLR_CRASH` or
      `ROOM_HOUSE_CRASH` stays, one staff line names the owner and the count, and each row is in
      the log with its item.
    - `Crash_crashsave()` returns whether the save was complete, and `Crash_save_single()` keeps
      `PLR_CRASH` unless both the objects and the character saved.
    - `Crash_idlesave()` commits, the empty save included (it deleted the file and rolled the
      row delete back).
    - An extra description that does not fit the record is left out, in both writers, through one
      helper that reserves room for the parts written after it. The bounded parts of the longest
      record need 35,760 bytes of the 36,767-byte buffer, which stays under the 65,535 bytes of the
      live `BLOB` columns.
06. `house_data` loses the unique key on `vnum` in `sql/master_schema.sql` and
    `src/database/db_init.c`; a boot migration in `run_legacy_table_migrations()` drops it where a
    database was created from them and makes sure of the plain `idx_vnum`.
07. `vessel_bounty_record_offense()` writes nothing when the bounty could not be read;
    `vessel_get_bounty()` keeps returning 0 for its display and threshold callers.
08. The pool reconnects a stale handle in place (`ensure_mysql_connection()`) instead of closing
    it: `conn`, `conn2` and `conn3` are those handles. `mysql_pool_health_check()` and
    `mysql_pool_shrink()`, which close them too and have no callers, are removed. With the
    database away, `mysql_pool_acquire()` returns no connection instead of waiting for one (see
    "Found while building").
09. `help_sync_database_lock_held()` asks the server whether this session still holds the lock.
    `hedit`'s save and delete and the help import check it inside their transaction, where the
    session can no longer change unnoticed, and roll back without it; `helpgen`'s other writers,
    which autocommit, warn the staff member afterwards.
10. `hedit` reads the entry's stored keywords from the prepared statement that selected them, so
    removed keywords are deleted.
11. Tests, DB-backed, in a new `unittests/CuTest/test_database_transactions.c` (and the vessel
    files for the vessel sites): the rule for a query, a prepared statement, a statement close and
    a ping; the helper's three results; each read-back both ways; a crash save, a house save and a
    pet save whose connection drops at a row (nothing lands, the flag stays, the retry saves);
    a row refused on a good connection (the rest commits, the flag stays, the staff line); the
    idle save; the longest record and the oversized extra description; the migration on the old
    table shape; the bounty read; the pool; the lock; the keywords. Real drops use persistent
    tables and rows with test-only keys, removed afterwards.
12. Docs: `VESSEL_SYSTEM.md` where the vessel sites change, the developer guide's database
    section for the rule and the helper, and the testing guide for the seam. No help entry
    changes: no command's use or wording toward players changes.

Found while tracing, fixed here because the same lines change: `Crash_cryosave()` writes its rows
with no transaction and without deleting the previous save's, so a frozen character's objects are
stored twice; it takes the begin and commit the other saves use. It runs only where rent is not
free (`free_rent` is YES by default).

Found while building, fixed here because the same lines change: `mysql_pool_acquire()` could hold
the game while the database was away. A pooled connection that failed its idle check was retired
for good, and once all were retired the function tried to open a new one and, failing, tried
again at once, with no way out. The game runs on that thread, so the first wilderness description,
region hint or wind lookup of an outage would have stopped it for everyone until the database
returned, an error line logged at each attempt; and each outage left the pool that many
connections short. Read from the code, not seen in production; it predates S13. Now a failed check
returns no connection, which the pool's one caller (`mysql_pool_query()`) already takes as a
failed query; the entry stays free, since its handle reconnects by itself at its next use; and a
pool that cannot open another connection says so instead of trying again. The retired state is
gone (`CONN_STATE_ERROR`, and `CONN_STATE_STALE`, which nothing set).
`Test_database_pool_keeps_its_handles_and_never_waits_for_the_database` makes the database really
unreachable.

Found while building, in the rule itself: a caller that failed without ending its transaction
would leave the connection marked, and it would refuse every statement, anyone's, until some later
save began a transaction. The query that loads a player's objects at login stops the server when
it fails (`exit(1)` in `Crash_load_objs()` and the object parsers), so one missed `ROLLBACK` and
one dropped connection could have taken the game down. Two answers:

- Every site was read for such a path. All 26 `START TRANSACTION` sites end their transaction on
  every way out; `begin_account_character_removal()` leaves its transaction to its caller, which
  ends it on both paths. No stored procedure call, `SET autocommit` or savepoint opens one
  elsewhere.
- The mark is limited to the pulse it was set in. All database work is done within the game pulse
  that starts it, so a mark met in a later pulse belongs to code that has returned: the layer
  rolls the transaction back itself, logs a `SYSERR`, and lets the statement through. A missed
  `ROLLBACK` in code written later can then block the connection for the rest of one pulse at
  most. At boot no pulse passes and nothing heals; a boot that loses its connection inside a
  transaction fails loudly either way.

Interpretations decided while planning S13:

- Lost means the connection: 2006, 2013, or a changed session. Other client errors (a statement
  used wrongly) leave the session and its transaction as they were, and stay the caller's.
- An unanswered `COMMIT` is any client error, the wider test: taking a known rollback for unknown
  costs one read, the reverse costs data.
- The read-backs lock (`LOCK IN SHARE MODE`), so they wait until the lost session's `COMMIT` or
  rollback has finished on the server.
- If the read-back fails too, the site does what it did before (takes it for a rollback) and logs
  the row to check: two faults in a row, and the database is unreachable at that moment.
- The locking read waits for the lost session's locks. The database here is local
  (`mysql_host = localhost`), where a lost connection means the server closed the session and
  freed them. With a remote database and a broken network, the server could keep the session, and
  the read would wait out `innodb_lock_wait_timeout` (50 seconds) before the site takes the
  transaction for rolled back; the cargo trade's second write has waited the same way since MR
  !18.
- An incomplete save reports progress to the persistence scheduler, so its retry is the next
  autosave pass. Reporting failure would retry the same player or house every second and hold the
  pass for everyone behind it.
- The staff line is one per save, not one per row: a fault that refuses every row would otherwise
  print the whole inventory each pass.
- The record's order is unchanged; the room kept for the spellbook, special ability and activation
  lines is taken only when the object has them.

Ablation (planning): dropped the writers stopping at their first failure and the per-site checks
of the help writers (item 1 refuses the statements for them), a second, simulated lost-reply seam
(one real seam), repairs to the two uncalled pool functions (removed instead), a table of marked
handles (the handle carries its own mark, so a closed handle leaves none behind), a merge of the
three object writers (the sheath and pet records differ in content), a new SQL component for the
`house_data` key (the boot migration reaches every database), handling of a transaction the
server rolls back on a live connection (a deadlock; no lost connection, and the game is the
only writer of these tables), and a second line of defense for a failed read-back. Simplified:
the helper rolls back itself, so its callers lose their own; one lock check serves three
writers; the extra-description rule is one function for two writers. Kept: real drops on
persistent tables (the work item's proof, and TEMPORARY tables vanish with the session), the ping
and statement-close marks (the probe showed both replace the session silently), and the staff
line.

Verification: `make test-all` with the database cases on (S9's `testenv.sh`, the
`luminari-vessels-testdb` container); the boot migration on a database built from the old
definitions; sqlfluff on `master_schema.sql`; the local CI matrix
`scripts/ci/local/run.py --base gitlab/master`; and every live gate in `scripts/vessels/` in the
namespace harness on a reload of the development dump, because every statement of every gate now
passes through the changed layer.

Progress log (2026-10-04, kept current as the work goes):

- Plan committed.

Still open outside these steps: the production deploy of S9's world-data notes and S10's, S11's and
S12's code (S12's with schema Phase 24, which boot adds) and the review fixes, the Open
player-data balance and human beta gates in `VESSEL_SYSTEM_REQUIREMENTS.md`, and closing these
study documents once S15 merges: `docs/ongoing-projects/` is temporary, and their enduring content
lives in `VESSEL_SYSTEM.md` and the guide.

## Estimate (remaining)

| Step | What drives the size | Days |
| -- | -- | -: |
| S13 Transactions that survive a lost connection | The query-layer rule and the `COMMIT` helper, four unanswered-`COMMIT` sites, six site fixes, and the object writers (record size, the `house_data` key and its migration, the incomplete-save report) across the server, real-drop DB tests | 3 |
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
