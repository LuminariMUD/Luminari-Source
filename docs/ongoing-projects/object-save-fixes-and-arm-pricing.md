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
[#30](https://gitlab.com/max757/Luminari-Source/-/work_items/30) was found in phase 3 and is
phase 7.

## Phases

| Phase | Item | Defect | Change | State |
| -- | -- | -- | -- | -- |
| 1 | #19 | A saved special ability with no command word is rejected at load | One parser, tests | Merged 2026-10-05: [MR !23](https://gitlab.com/max757/Luminari-Source/-/merge_requests/23), `5523e04a0` |
| 2 | #18 | Pet legacy-migration test fails on a `master_schema.sql`-only database | Test fixture | Merged 2026-10-05: [MR !24](https://gitlab.com/max757/Luminari-Source/-/merge_requests/24), `0efe4d145` |
| 3 | #15 | An object with no prototype leaks its arcane mark | One free path, two tests | In review: [MR !25](https://gitlab.com/max757/Luminari-Source/-/merge_requests/25), tag `object-save-fixes-p3` |
| 4 | #16 | Bagged objects stay in memory when their owner leaves | Leaving save, `extract_char()` | Not started |
| 5 | #17 | A loaded sheath loses its weapons on a pet and leaks them when extracted | Pet save, `extract_obj()` | Not started |
| 6 | #1 | Extra Arms and the monk third hand are not priced after the arm count | Docs, data audit | Not started |
| 7 | #30 | An object copy in OLC keeps or drops the wrong fields | Object copies in `src/olc/genobj.c` | In review: [MR !26](https://gitlab.com/max757/Luminari-Source/-/merge_requests/26), tag `object-save-fixes-p7`; merge !25 first |

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
- #30 straight after #15, where it was found, and before #17: phase 5 makes `extract_obj()`
  follow the sheath pointers, and an object copy must first stop carrying or dropping them. It
  shares no code with #16.

## Every phase

1. Branch `<iid>-<slug>` from `gitlab/master`, checked out in this plan's worktree
   (`../Luminari-Source-object-save-fixes-and-arm-pricing`): one phase at a time. Phase 7 is the
   one exception: its branch starts at the phase 3 tag, because phase 3 was still in review.
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

`docker rm -f objsave-db` when done. On that database the whole suite passes.

For a fresh database in the same container, drop and create it, then repeat the schema line:

```bash
docker exec objsave-db mariadb -uroot -proot \
  -e "DROP DATABASE luminari_test; CREATE DATABASE luminari_test"
```

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

Merged 2026-10-05 as merge commit `5523e04a0`; #19 is closed. Left: after the next release, look
for the `Invalid SpAb record` line in the production log.

## Phase 2: #18 pet legacy-migration test

**Cause (confirmed 2026-10-05, see Built).** The fixture `create_legacy_pet_temporary_schema()`
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

### Built 2026-10-05

Branch `18-pet-legacy-migration-test-fails-on-a-database-built-from-master_schema-sql-alone`,
MR !24, tag `object-save-fixes-p2`.

Step 1, the test alone on a database built from `sql/master_schema.sql`, boot skipped:

- It fails at `first_verification` with one error, the missing foreign key, and leaves the
  permanent tables as they were: no key, no pet migration recorded. The fourteen migrations it
  ran all went to the temporary tables.
- With the key added to the permanent `pet_save_objs` by hand and nothing else changed, it
  passes. That is the cause: the fixture, not the migrations.
- The migrations are sound from the `master_schema.sql` shape:
  `Test_pet_live_schema_cascades_objects_and_rejects_orphans` alone on a fresh database runs all
  fifteen on the permanent tables, 2026091007 adds the key, and the schema verifies. A fresh
  install is not affected.
- The item's "a second run fails the same way" and this plan's "a second run of the whole suite
  passes" are both right. The test alone never touched the permanent tables; in the whole suite
  the live-schema test runs after it and migrates them.

Step 2: `Test_pet_persistence_legacy_schema_migration_is_idempotent` calls
`run_pet_persistence_migrations()` on the permanent tables before it creates the temporary
ones, and asserts that the call succeeded. The fixture's comment claimed that every fresh test
database runs 2026091007 at startup; it now says what the validator reads. Nothing changes
outside `unittests/CuTest/test_database_persistence.c`.

The test still needs the permanent pet tables, as the live-schema test does. On an empty
database the first migration fails and the new assertion reports it.

Verification, on `b06a2367f`:

- Step 3, boot skipped: each of the three tests passes alone on a fresh
  `master_schema.sql`-only database, and again on the same database.
- Step 3, after the boot: each of the three passes alone on a fresh database on which
  `Test_syntax_check_encounter_world_boots_and_cleans_up_once` ran first. The boot needs
  `scripts/ci/prepare_test_runtime.sh .ci-runtime/lib`, then both runs with
  `LUMINARI_TEST_DATA_DIR="$PWD/.ci-runtime/lib"`,
  `LUMINARI_TEST_CONFIG_FILE=.ci-runtime/lib/etc/config` and `LUMINARI_TEST_SKIP_SYNTAX_BOOT`
  unset.
- All 36 tests of `test_database_persistence.c` pass alone, each on its own fresh database,
  boot skipped. Phases 3 to 5 rely on that.
- `./cutest` with the database tests on, first run on a fresh database: 2039 of 2039 pass.
- `make -j16 test`: 2039 tests pass. `make install` done.
- `python3 scripts/ci/local/run.py --base gitlab/master`: 33 of 33 jobs pass (344 s);
  `test-unit-tests` runs the suite with the database tests on.

### Review 2026-10-05

Two findings, the first blocking, both fixed in the commits after the tag:

- The test's `UPDATE pet_data SET owner_id = 4294967295` is written for the fixture's temporary
  table. When the new migration run on the permanent tables failed, the fixture was skipped and
  the statement reached the permanent `pet_data`: every saved pet got an `owner_id` no owner has.
  Reproduced on `115b6d846` with one pet row in three states: the key added by hand with nothing
  recorded (step 1's probe state), a user without CREATE, and two `./cutest` processes started
  together on a fresh database.
- `bf3ad8c39` runs the statement only when the fixture was created. It is the one write below
  that point, so the test keeps its single exit; the review's patch stopped the test there
  instead. In the three states, and with a user who cannot create temporary tables (a failure the
  test had before this phase too), the test still fails and the row keeps its `owner_id`.
- `docs/guides/TESTING_GUIDE.md` named the live-schema test as the one pet test that touches the
  permanent tables. `f879086e4` names both.

Left as it is: `apply_migration()` checks a version and records it in separate statements, so of
two suites started together on a database that has not been migrated, one fails and passes when
run again. One server boots on one database and every CI job has its own, so only a hand-started
pair meets it; the guide now says so.

Verification, on `f879086e4`:

- Each of the three tests that call `verify_pet_persistence_schema()` passes alone on a fresh
  `master_schema.sql`-only database, boot skipped, and again on the same database.
- `./cutest` with the database tests on, first run on a fresh database: 2039 of 2039 pass.
- `make -j16 test`: 2039 tests pass. `make install` done.
- `python3 scripts/ci/local/run.py --base gitlab/master`: 33 of 33 jobs pass (266 s).

Merged 2026-10-05 as merge commit `0efe4d145`; #18 is closed.

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

### Built 2026-10-06

Branch `15-an-object-with-no-prototype-leaks-its-arcane-mark-when-freed`, MR !25, tag
`object-save-fixes-p3`.

- `free_object_strings()` frees `arcane_mark` (`79a3c99ed`).
- The check before the change held. Each other caller frees the object or writes over it
  afterwards, and each of those objects owns its mark or has none: `add_object()` copies the
  edited object over the prototype and `copy_object_main()` duplicates the mark; the OLC cleanup
  and the iedit commit free an edit copy whose mark `copy_object()` duplicated; the spec fixture
  frees a prototype from `parse_object()`, which reads no mark. The iedit commit leaked its edit
  copy's mark the same way, and no longer does.
- The prototype teardown in `destroy_db()`, which #15 names, frees by hand and does not call the
  function. No prototype carries a mark: no world-file field and no editor field sets one.
- The plan's test alone would not have shown the leak to the valgrind job:
  `Test_object_saves_bind_player_house_and_serialized_text` needs a database and
  `test-memory-check` has none. `Test_object_without_a_prototype_frees_its_arcane_mark` needs
  none. It saves and loads a marked object through the file loader, compares the mark and
  extracts both objects.

Found while building:

- `copy_object()` sent every target through `free_object_strings_proto()`, which compares the
  target's strings with its prototype's. For a target with no prototype that read
  `obj_proto[-1]`; the iedit commit reaches it with any live object that has no prototype. It
  now chooses as `free_obj()` does (`a0757112c`). It is the second commit of the merge request.
  `Test_gameplay_object_editor_copy_frees_an_object_with_no_prototype` copies over such an
  object with no prototype table at all, and ends in SIGSEGV before the change.
- An OLC save of a prototype leaves each live object of it with the prototype's
  special-ability list, which the first extraction then frees under the prototype. Seen under
  valgrind with a probe test, filed as #30, and phase 7 of this plan: it is not a free path of
  #15, and its fix changes what every object copy keeps.

Verification, on `a0757112c`:

- Without the fix (`free_object_strings()` as on master, both tests in place): under valgrind
  each test loses its two marks. Of the matrix, `test-memory-check` fails on the new test's two
  blocks and `test-sanitizers-gcc-address-undefined` reports all four.
- With it: both tests are clean under valgrind, with the database tests on and off.
- `make -j16 test`: 2041 tests pass. `make install` done.
- `./cutest` with the database tests on, first run on a fresh database: 2041 of 2041 pass.
- `python3 scripts/ci/local/run.py --base gitlab/master`: 33 of 33 jobs pass (340 s).

### Review 2026-10-06

One finding, fixed in the commit after the tag:

- `copy_object_strings()` duplicated every string `free_object_strings()` frees except
  `char_sdesc`, a corpse's description of the fallen. An `iedit` of a corpse, an object with no
  prototype, made an edit copy that shared it with the live corpse: the commit freed it from the
  corpse and again from the edit copy, and the corpse kept the freed pointer until its decay freed
  it a third time. A disconnect during the edit freed it from the edit copy alone, under the live
  corpse. On master the same path freed it from `free_object_strings_proto()` after reading
  `obj_proto[-1]`; `a0757112c` removed the read and kept the double free, and the test of that
  commit set no `char_sdesc`, so neither sanitizer job saw it.
- `ddacf712f`: `copy_object_strings()` duplicates `char_sdesc`. Every copy it fills is freed by
  `free_object_strings()` or `free_object_strings_proto()`, and both free `char_sdesc`.
  `Test_gameplay_object_editor_copy_frees_an_object_with_no_prototype` gives both objects a
  description of their own, checks that the target gets the source's text at an address of its
  own, and frees both. It fails on the shared pointer before the change.
- Phase 7 had met the same double free through the `iedit` commit and `584524ec5` carries the
  same line. Its branch has this commit merged in, so the two merge requests share one copy of
  the line and MR !26 merges cleanly after MR !25.

Verification, on `ddacf712f`:

- Without the fix, the test fails on the shared pointer. With it, the two editor-copy tests and
  `Test_object_without_a_prototype_frees_its_arcane_mark` are clean under valgrind.
- `make -j16 test`: 2041 tests pass. `make install` done.
- `python3 scripts/ci/local/run.py --base gitlab/master`: 33 of 33 jobs pass (281 s); `test-unit-tests` runs
  the suite with the database tests on.

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

## Phase 7: #30 object copies in OLC

**Cause.** `update_all_objects()` (`src/olc/genobj.c`) writes the new prototype over every live
object of it (`*obj = *refobj`) and puts back a list of runtime fields. `copy_object_main()`
does the same for one object, with a shorter list of its own: the iedit commit goes through it,
and so does the preview copy of work item #29. Both lists are older than fields the struct has
gained.

**Seen 2026-10-06, under valgrind, on master `ee42019ed`.** A prototype with one special
ability, one live object with an arcane mark, then `oedit_save_internally()`. Afterwards the
live object's list pointer is the prototype's, and its own list and mark are lost. Extracting
it frees the block the prototype still points to.

**Read, not run.** The same save resets `next_in_hash` / `prev_in_hash` of each live object
while it is still on its `obj_rnum_hash` chain, and drops `sheath_primary` /
`sheath_secondary`, `trap`, `next_gitem`, `limb_restore_slot` and the `rol_*` state. The
spellbook array is shared as the ability list is. The iedit commit writes back the hash links
and sheath pointers the object had when the edit began.

**Fix.** One list of what an object keeps when a definition is written over it, used by both
functions. Before a prototype is replaced, each live object releases what it owns, as
`free_obj()` does; after the copy it gets copies of its own of what `read_object()` copies.

**Tests.** An OLC save with a live object that has abilities, a spellbook, a mark and a
neighbour on its hash chain: its lists are its own, `get_obj_num()` still finds the neighbour,
and extracting it leaves the prototype whole. An iedit commit after the hash chain changed:
the live object keeps its current links. Both under the sanitizer and memory-check jobs.

### Built 2026-10-06

Branch `30-an-olc-save-of-an-object-prototype-leaves-its-live-objects-sharing-the-prototype-s-lists`,
MR !26, tag `object-save-fixes-p7`. The branch starts at `object-save-fixes-p3`, so the merge
request shows phase 3's commits until MR !25 is merged. The worktree is on this branch; a
review fix for phase 3 needs `git switch 15-an-object-with-no-prototype-leaks-its-arcane-mark-when-freed`
first. Phase 3's review fix `ddacf712f` and its record were merged into this branch on
2026-10-06 (`15f66c7c9`), so MR !26 merges cleanly after MR !25.

One code commit, `584524ec5`:

- `overwrite_object()` (`src/olc/genobj.c`) is the one list of what a target keeps when another
  object is written over it: where it is, what holds it and what it holds, its list, hash and
  registry links, its events and transfer state, its sheath pointers and trap, and the other
  runtime-only fields. `copy_object_main()` and `update_all_objects()` both go through it. The
  iedit commit's own restore list is gone.
- An OLC save: `free_all_object_strings()` frees each live object's own strings before the
  prototype's are replaced, while the two can still be told apart. After the copy,
  `give_obj_own_copies()` gives the object what `read_object()` gives a new one; it is that part
  of `read_object()`, moved into a function of its own in `src/core/db.c`.
- `index_object()` clears the slot a new prototype goes into and `oedit_setup_existing()` clears
  its edit copy, since a copy now keeps the target's runtime fields.
- What an OLC save does to a live object is unchanged: it still resets values, flags, strings,
  mark and abilities to the prototype's. The phase changes which memory the object points at
  afterwards, not what it holds.

Found while building, fixed in the same commit:

- An edit copy shared `char_sdesc` with the live object and both freed it: the iedit commit of
  a corpse ended in a double free. `copy_object_strings()` now duplicates it. The review of phase
  3 found the same double free on its own, and `ddacf712f` is the same line, so the two merge
  requests share one copy of it.
- A new prototype kept the event and registry fields of whatever its slot held, and a slot at
  the end of the table is not initialised.
- The preview copy of #29 took the list, hash and holder links of the object it previews. The
  keep list ends that (read, not run). What that copy still shares is #29's.

Left as it was:

- The special abilities, spellbook and script a live object held before an OLC save are still
  dropped without a free. Two objects can point at one list today: #29's preview copy shares
  the previewed object's lists and script, and an open iedit copy shares its spellbook. Freeing
  them at the save could free one list twice, so #29 has to end the sharing first. A note on #29
  says so.
- An edit copy and the object it was made from still share the special-ability list (oedit)
  and the spellbook (oedit and iedit). The editors hand those over on purpose.

Tests, all in `unittests/CuTest/test_gameplay_e2e.c`:

- `Test_gameplay_object_prototype_save_leaves_live_objects_whole`: two live objects, one
  restrung, marked and with a weapon in its sheath; the edit adds an ability and a spellbook
  page. Fails before the change on the shared lists.
- `Test_gameplay_object_editor_copy_leaves_each_object_its_own_links`: an edit copy and its
  commit across a change of the live object's hash links and sheath. Ends in the `char_sdesc`
  double free before the change.
- `Test_gameplay_object_new_prototype_takes_nothing_from_its_slot`: a new prototype in a slot
  full of stale data. Fails before the change.

Verification, on `584524ec5`:

- The three tests fail as described on the code of `8556f9ca8`, and are clean under valgrind
  after the change, as are the two older editor-copy tests.
- `make -j16 test`: 2044 tests pass. `make install` done.
- `./cutest` with the database tests on, first run on a fresh database: 2044 of 2044 pass.
- `python3 scripts/ci/local/run.py --base gitlab/master`: 33 of 33 jobs pass (1457 s).

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
