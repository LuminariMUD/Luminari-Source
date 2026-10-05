# Object save fixes and arm pricing

High-level plan for work items
[#19](https://gitlab.com/max757/Luminari-Source/-/work_items/19),
[#18](https://gitlab.com/max757/Luminari-Source/-/work_items/18),
[#15](https://gitlab.com/max757/Luminari-Source/-/work_items/15),
[#16](https://gitlab.com/max757/Luminari-Source/-/work_items/16),
[#17](https://gitlab.com/max757/Luminari-Source/-/work_items/17) and
[#1](https://gitlab.com/max757/Luminari-Source/-/work_items/1). Written 2026-10-05 against
master `cdcbf1db0`. One work item is one phase. Causes below were read from the code on that
commit, not run, unless a line says otherwise.

## Phases

| Phase | Item | Defect | Change |
| -- | -- | -- | -- |
| 1 | #19 | A saved special ability with no command word is rejected at load | One parser, tests |
| 2 | #18 | Pet legacy-migration test fails on a `master_schema.sql`-only database | Test fixture |
| 3 | #15 | An object with no prototype leaks its arcane mark | One free path, one test |
| 4 | #16 | Bagged objects stay in memory when their owner leaves | Leaving save, `extract_char()` |
| 5 | #17 | A loaded sheath loses its weapons on a pet and leaks them when extracted | Pet save, `extract_obj()` |
| 6 | #1 | Extra Arms and the monk third hand are not priced after the arm count | Docs, data audit |

Why this order:

- #19 first. It is the only one seen in production, and it is worse than the item says for
  pets (see phase 1).
- #18 second. Phases 3 to 5 add tests to `unittests/CuTest/test_database_persistence.c` and are
  checked on disposable databases built from `sql/master_schema.sql`; every test in that file
  has to pass alone there first.
- #15, #16, #17 in that order. All three change what happens when an object is extracted or
  freed. #16 and #17 both change the leaving save's extraction in `src/obj/objsave.c`, so they
  run one after the other, never side by side.
- #1 last. It shares no code with the others and can move anywhere in the order.

## Every phase

1. Branch `<iid>-<slug>` from `gitlab/master`, with its own worktree.
2. Write the failing test (for #18, run the repro), then the fix.
3. `CUTEST_FILTER=<test> ./cutest` while working, with the `LUMINARI_TEST_MYSQL_*` settings for
   database tests; then `make -j$(nproc) test && make install`.
4. `python3 scripts/ci/local/run.py --base gitlab/master`.
5. Merge request with `Closes #<iid>`, review, merge commit (no squash).

No phase deploys. The fixes reach production with the next release, and its copyover needs its
own approval.

## Phase 1: #19 special ability without a command word

**Cause.** `objsave_replace_special_ability()` (`src/obj/objsave.c:111`) requires eight fields.
The three writers store seven numbers and an empty word when the ability has no command word.
All four loaders call this one function: `objsave_parse_objects()`,
`objsave_parse_objects_db()`, `objsave_parse_objects_db_sheath()` and
`objsave_parse_objects_db_pet()`.

**Worse for pets.** The first three log the record and go on. The pet loader treats the failure
as a malformed record (`goto malformed`, `src/obj/objsave.c:4232`), and one malformed record
rejects the pet's whole object set.

**Fix.** Accept seven numbers with an empty command word; keep rejecting fewer than seven.

**Tests.** Next to the `SpAb: 0 0 0 0 0 0 0 cccc` case: save and load an object whose ability
has no command word and compare the ability, once through the player path and once through the
pet path; one record with six numbers is still rejected.

**Done when.** Both round trips pass, and after the next release the
`Invalid SpAb record ... 61 30 48 0 0 0 0` line no longer appears in the production log.

## Phase 2: #18 pet legacy-migration test

**Cause (to confirm with the item's repro).** The fixture `create_legacy_pet_temporary_schema()`
shadows `pet_data`, `pet_save_objs` and `schema_migrations` with temporary tables and records
migration 2026091007 as applied, because InnoDB refuses a foreign key on a temporary table.
`pet_schema_has_foreign_key()` reads `information_schema`, which lists only the permanent
tables. So the check passes only when a boot has already put the key on the permanent
`pet_save_objs`. This is the fixture, not the migrations: on a fresh install the boot runs
2026091007 on the permanent tables, and
`Test_pet_live_schema_cascades_objects_and_rejects_orphans` checks the result.

**Steps.**

1. Run the repro. Add the key to the probe database's permanent table by hand and rerun: a pass
   confirms the cause.
2. Make the test bring the permanent tables to the booted shape itself
   (`run_pet_persistence_migrations()` before the temporary tables are created).
3. Run the test alone on a `master_schema.sql`-only database, with and without the syntax-check
   boot, and the same for the two neighbours that call `verify_pet_persistence_schema()`.

**Done when.** Step 3 passes and the item is closed with the cause named.

## Phase 3: #15 arcane mark of an object with no prototype

**Cause.** `free_obj()` sends an object with no prototype through `free_object_strings()`
(`src/olc/genobj.c:469`), which does not free `arcane_mark`. `free_object_strings_proto()` does.

**Fix.** Free `arcane_mark` in `free_object_strings()`.

**Check before the change.** Its other callers must drop or overwrite the pointer afterwards,
or the change becomes a double free: `add_object()` (`src/olc/genobj.c:41`), the OLC cleanup
(`src/olc/oasis.c:138`), `src/olc/oedit.c:4021`, and `unittests/CuTest/test_spec_fixtures.c`.

**Tests.** Remove the two hand frees of `arcane_mark` before `extract_obj()` in
`Test_object_saves_bind_player_house_and_serialized_text`. The sanitizer and valgrind jobs of
the matrix then fail without the fix and pass with it.

## Phase 4: #16 bagged objects of a leaving character

**Cause.** `empty_bags_to_inventory()` (`src/core/handler.c:4262`) only unlinks each bagged
object: nothing moves it to the inventory and nothing extracts it. It also passes bag number 1
to `obj_from_bag()` for all ten bags.

**Decisions.**

- Paths that save first (rent, cryo, idle-out, timed out): `objsave_save_and_extract()` extracts
  the bagged objects where it extracts the worn and carried ones. Their rows are already
  written by `Crash_save_bags()`.
- Paths that do not save first (staff purge of a player, the duplicate-login cleanup, quit with
  `free_rent` off): bagged objects follow the carried ones. `empty_bags_to_inventory()` does
  what its name says, and the loop after it in `extract_char()` puts them in the room, or
  extracts them when the character has no room.
- Each bag passes its own number to `obj_from_bag()`.

**Check before the change.** Whatever keeps a carried object from being both dropped in the room
and loaded again from the last save must cover the bag rows too.

**Tests.** A character with bagged objects goes through the leaving save: the objects are gone
from `object_list`, and a load brings them back into their bags. The same character extracted
without a save: the objects are in the room, and the transfer record names the right bag.

## Phase 5: #17 loaded sheath on a pet and at extraction

**Cause.** A sheath's weapons hang off `sheath_primary` and `sheath_secondary`. Only the player
save and `sheath` / `unsheath` handle them. `objsave_save_obj_record_db_pet()` writes no sheath
rows, and `extract_obj()` does not look at them.

**Decisions.**

- Pet save: store a sheath's weapons as the pet's own objects beside the sheath, as
  `House_save()` does for a house (`src/obj/house.c:136`). A restored pet has the weapons in its
  inventory and an empty sheath. No new table and no migration.
- The weapons are written at `MIN(0, location)`, not at the sheath's wear position:
  `pet_object_graph_valid()` rejects two records on one position and with them the whole set.
- `extract_obj()` extracts what a sheath holds, as it does a container's contents. Junking or
  selling a loaded sheath destroys its weapons with it, and the leak after a rent save ends.

**Check before the change.** No copy of an object (`copy_object_main()`, the save writers'
temporary objects) may carry the sheath pointers into an object that is later extracted.

**Tests.** A pet with a loaded sheath, worn and carried, saved and restored: both weapons are
back and the record set is accepted. A loaded sheath extracted: both weapons leave
`object_list`.

**Docs.** The pet rule in `docs/systems/SAVE_SYSTEMS_BREAKDOWN.md`.

## Phase 6: #1 price Extra Arms and the monk third hand

No code: `docs/guides/PLAYER_RACES_REFERENCE.md` is the only place the prices live, and no race
grants Extra Arms.

**Steps.**

1. Audit grants of feat 1320. Production has run the arm count since the 2026-10-04 release
   (`d5cd6a735` contains merge `2c5859388`), so the item's pre-deploy check is now an audit of
   live data: objects with `APPLY_FEAT` 1320 and mobiles with the feat in the world files, then
   characters and saved objects on production. The production query is read-only and still
   needs approval when the phase starts. Each grant found is listed in the item with what it
   gave before (swings) and gives now (arms).
2. Price by the arm a rank adds, not by the rank. Derive the third and the fourth arm's price
   from what each opens (`wear_slot_arms_needed()`, the second weapon pair), with one fixed
   constraint: the two add up to 15 RP, the Four Arms price, because two ranks and Four Arms
   are the same four arms. Each arm past four costs 1 RP: it adds hands only.
3. Mixed sources: price the arm count the race ends with, whatever mix of Four Arms, Extra Arms
   and `arm_adjust` produces it, since `arm_count()` is the only source. A negative `arm_adjust`
   is a drawback priced by the lost-slot row.
4. Cap exemption: stays with Thri-Kreen's Four Arms alone. Another race taking arms fits
   composition rule 2 or records its own exemption.
5. Monk third hand: confirm in the attack plan (`plan_second_pair()`) that the empty third hand
   of a monk makes the swing another character makes with a weapon there, and no extra one. If
   so it is inside the 15 RP, under the existing rule that class interaction is not priced; if
   not, raise Four Arms and rescore Thri-Kreen (20.0 RP, bottom of the Epic band).
6. Write the result into the arms paragraph and composition rule 2, drop "under review", and
   close the item with the decisions.

**Done when.** The reference prices every arm source, the audit result is in the item, and no
text says the price is open.

## Left out

- A `pet_save_objs_sheathed` table (#17): the house rule already covers it without a schema
  change.
- A change to the pet migrations or `master_schema.sql` (#18), unless the repro contradicts the
  cause above.
- Restoring abilities dropped from saved rows before the #19 fix: a row that lost its line
  cannot be told from a row that never had one.
- Measuring the leaks of #15 and #16: the tests and the memory-check jobs prove them.

## Closing

When all six items are closed, delete this file; nothing here needs to outlive them.
