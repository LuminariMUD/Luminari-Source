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

| Phase | Item | Defect | Change | State |
| -- | -- | -- | -- | -- |
| 1 | #19 | A saved special ability with no command word is rejected at load | One parser, tests | In review: [MR !23](https://gitlab.com/max757/Luminari-Source/-/merge_requests/23) |
| 2 | #18 | Pet legacy-migration test fails on a `master_schema.sql`-only database | Test fixture | Not started |
| 3 | #15 | An object with no prototype leaks its arcane mark | One free path, one test | Not started |
| 4 | #16 | Bagged objects stay in memory when their owner leaves | Leaving save, `extract_char()` | Not started |
| 5 | #17 | A loaded sheath loses its weapons on a pet and leaks them when extracted | Pet save, `extract_obj()` | Not started |
| 6 | #1 | Extra Arms and the monk third hand are not priced after the arm count | Docs, data audit | Not started |

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
5. Merge request with `Closes #<iid>`, then the annotated tag `object-save-fixes-p<phase>` on the
   commit handed to review, pushed to `gitlab`. A pushed tag is never moved; review fixes are the
   commits after it.
6. Review, merge commit (no squash). Set the phase's State in the table above.

No phase deploys. The fixes reach production with the next release, and its copyover needs its
own approval.

Database tests need a database of their own. A disposable one:

```bash
docker run -d --rm --name objsave-db -p 127.0.0.6:3306:3306 -e MARIADB_ROOT_PASSWORD=root \
  -e MARIADB_DATABASE=luminari_test -e MARIADB_USER=luminari_test \
  -e MARIADB_PASSWORD=test_password mariadb:10.11
docker exec -i objsave-db mariadb -uroot -proot luminari_test < sql/master_schema.sql
docker exec objsave-db mariadb -uroot -proot -e "GRANT ALL ON *.* TO 'luminari_test'@'%'"
export LUMINARI_TEST_MYSQL_ENABLE=1 LUMINARI_TEST_MYSQL_HOST=127.0.0.6 \
  LUMINARI_TEST_MYSQL_USER=luminari_test LUMINARI_TEST_MYSQL_PASSWORD=test_password \
  LUMINARI_TEST_MYSQL_DATABASE=luminari_test LUMINARI_TEST_MYSQL_PORT=3306 \
  LUMINARI_TEST_SKIP_SYNTAX_BOOT=1 LUMINARI_TEST_ROOT="$PWD" \
  LUMINARI_TEST_SPEC_WORLD_ROOT="$PWD/unittests/CuTest/fixtures/spec_world_inventory"
```

`docker rm -f objsave-db` when done. On that database the whole suite passes except the phase 2
test, until phase 2 is merged.

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

### Built 2026-10-05

Branch `19-a-saved-special-ability-without-a-command-word-is-rejected-at-load`, MR !23, tag
`object-save-fixes-p1`.

- `objsave_replace_special_ability()` accepts seven numbers and leaves `command_word` NULL, as
  `parse_object()` does for a prototype's seven-field record. NULL, not an empty string: that is
  what the ability holds before it is saved, and the displays print "Not set." for NULL only.
- `Test_object_special_ability_without_a_command_word_survives_save_and_load` saves the
  production record's ability (vampiric, level 30, on hit and on critical hit) and loads it
  through the file loader always, and through the player-row and pet-row loaders when the
  database tests are on. The six-number record is one more entry in the `invalid[]` list of
  `Test_pet_object_decoder_validates_fields_and_preserves_text`.

Found while building:

- `utter` crashed the server on an ability that answers to a command word and has none:
  `process_weapon_abilities()` and `process_armor_abilities()` passed the NULL word to
  `strcmp()`. A prototype's seven-field record or a builder's edit could already produce such
  an ability; this phase's loader fix adds saved objects. Both functions now skip it
  (`Test_gameplay_utter_passes_over_an_ability_with_no_command_word`). It is the first commit of
  the merge request. `process_item_abilities()` has the same comparison, but no caller gives it
  a command word.
- The loader before `853a3f226` did not leave the word empty, as #19 says: it copied what the
  last `Flag:`, `Wear:` or `Perm:` line had left in the scan buffer, a number. An ability that
  loader read and the game saved again has an eight-field record with that number as its word,
  and has always loaded.
- `objsave_parse_objects_db()` calls `real_object()` before it checks for an unnumbered object,
  and `real_object()` reads `obj_index[0]`. A test that loads player rows without a world needs
  an object index of at least one entry (the new test has one). Phases 3 to 5 add tests here.

Verification, on `ea33ddd29`:

- The new round-trip test fails before the fix, with the rejected record logged by each of the
  three loaders, and passes after it, with the database tests on and off. The `utter` test ends
  in SIGSEGV before its fix.
- `make -j16 test`: 2038 tests pass. `make install` done.
- `./cutest` with the database tests on (the disposable database above): 2037 of 2038 pass. The
  one failure is `Test_pet_persistence_legacy_schema_migration_is_idempotent`, which is #18.
- `python3 scripts/ci/local/run.py --base gitlab/master`: 33 of 33 jobs pass (361 s), the
  memory-check, sanitizer and coverage jobs among them; `test-unit-tests` runs the suite with the
  database tests on.

### Review 2026-10-05

One finding, blocking, fixed in `1d904677a` (the commit after the tag):

- The writers store an object's first ability only, and the loader freed the whole list before it
  installed the record. On master a first record with no command word was rejected, so a
  prototype-backed object kept its prototype's list. Accepting the record cut the three
  prototypes with two real abilities (#1213, #1244, #20621) to their first one at every load.
- The record now replaces the ability of its kind, or else the first one, and the rest of the
  list stays. By kind, not by position: `parse_object()` puts each record in front and an OLC
  save writes the list in order, so the abilities change places at the boot after a zone save.
  A first ability that has a command word keeps the rest too, which it did not on master.
- `Test_object_special_ability_record_keeps_the_rest_of_the_prototype_list`: the two abilities
  of #1213 through the file loader and the player-row loader, then with the prototype's list the
  other way round, with a first ability that has a command word, and with an ability of a kind
  the prototype does not have. It fails on the loader of `ab7a35b84`, and on the fix when that
  matches by position only or puts an unmatched ability in front of the list.

Filed, not fixed here:

- #28: the writers save the first ability only, so a change to another ability, an emptied list,
  and the later abilities of an object with no prototype do not survive a load.
- #29: `downgrade` and `applyoil` drop ability lists without freeing them, and `downgrade` leaves
  its preview object in `object_list`. Seen while tracing the finding.

Verification, on `1d904677a`:

- `make -j16 test`: 2039 tests pass. `make install` done.
- `./cutest` with the database tests on, on a freshly loaded disposable database: 2038 of 2039
  pass, the one failure again the #18 test.
- `python3 scripts/ci/local/run.py --base gitlab/master`: 33 of 33 jobs pass (357 s).

Left: merge; after the next release, look for the `Invalid SpAb record` line in the production
log.

## Phase 2: #18 pet legacy-migration test

**Cause (to confirm with the item's repro).** The fixture `create_legacy_pet_temporary_schema()`
shadows `pet_data`, `pet_save_objs` and `schema_migrations` with temporary tables and records
migration 2026091007 as applied, because InnoDB refuses a foreign key on a temporary table.
`pet_schema_has_foreign_key()` reads `information_schema`, which lists only the permanent
tables. So the check passes only when a boot has already put the key on the permanent
`pet_save_objs`. This is the fixture, not the migrations: on a fresh install the boot runs
2026091007 on the permanent tables, and
`Test_pet_live_schema_cascades_objects_and_rejects_orphans` checks the result.

**Seen 2026-10-05, in phase 1's run.** On a `master_schema.sql`-only database with the
syntax-check boot skipped, the test fails at its `first_verification` assertion: the migrations
ran and `verify_pet_persistence_schema()` refused the result. A second run of the whole suite on
the same database passes it, so the first run changes the permanent tables. That fits the cause;
step 1 is still to do.

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
- Saving every ability of an object, not the first alone: it needs a new row format, and is work
  item #28.
- Measuring the leaks of #15 and #16: the tests and the memory-check jobs prove them.

## Closing

When all six items are closed, delete this file; nothing here needs to outlive them.
