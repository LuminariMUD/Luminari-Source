# Pet Schema and Identity Repairs - Issue 120

Status: complete, ready for review.
Branch: `fix/pet-schema-identity-120` (branched from `master` at `adf3afdd9`).
Issue: https://github.com/LuminariMUD/Luminari-Source/issues/120

The durable description of the resulting design lives in
`docs/systems/SAVE_SYSTEMS_BREAKDOWN.md` (Persistent Followers) and the test
note in `docs/guides/TESTING_GUIDE.md`. This file records how the issue's
checklist was closed.

## One schema authority

`run_pet_persistence_migrations()` in `src/db_init.c` gained migrations
`2026091001` through `2026091007`, and `PET_PERSISTENCE_SCHEMA_VERSION` in
`src/db_init.h` is `2026091007`:

- `2026091000` - fill null `intel` / `wis` / `cha` with 10; the original master
  schema left them nullable.
- `2026091001` - fill null `pet_name` / `pet_sdesc` / `pet_ldesc` / `pet_ddesc`
  before the columns become `NOT NULL`.
- `2026091002` - `pet_name` and `pet_sdesc` `VARCHAR(255) NOT NULL`,
  `pet_ldesc` and `pet_ddesc` `TEXT NOT NULL`. TEXT replaces the old
  `VARCHAR(255)` in `master_schema.sql` because live prototype strings exceed
  255 bytes and would be truncated on restore.
- `2026091003` - `intel` / `wis` / `cha` become `INT NOT NULL DEFAULT 10`.
- `2026091004` - `pet_data.pet_data_id` becomes `INT UNSIGNED NOT NULL AUTO_INCREMENT`.
- `2026091005` - delete `pet_save_objs` rows with no surviving pet row or a
  non-positive `pet_idnum`; they can never be restored and would block the
  constraint.
- `2026091006` - `pet_save_objs.pet_idnum` becomes `INT UNSIGNED NOT NULL` and
  `serialized_obj` becomes `LONGTEXT NOT NULL`.
- `2026091007` - the cascading foreign key, a plain `ADD CONSTRAINT ...
  FOREIGN KEY`. This migration is the only place the constraint is created:
  the runtime `CREATE TABLE` and `sql/master_schema.sql` deliberately omit it,
  because MariaDB refuses to modify a constrained column and a base table that
  already carried the key made migration `2026080503` fail on every fresh CI
  database. The recorded version is the idempotence guard, as for every other
  migration.

The runtime `CREATE TABLE` statements in `init_core_player_tables()` and
`sql/master_schema.sql` match the same base shape; `pet_save_objs` sits after
`pet_data` in the master schema. The duplicate ad-hoc `runtime_state` ALTER in
`init_core_player_tables()` was removed because migration `2026080501` owns it.

`verify_pet_persistence_schema()` in `src/db_init_data.c` asserts the unsigned
identifiers, the description column types, `serialized_obj` as `LONGTEXT`, and
(through `pet_schema_has_foreign_key()`, which reads `information_schema`) a
`pet_idnum -> pet_data_id` key whose delete and update rules are both `CASCADE`.

## Owner and object identity

- `objsave_parse_objects_db_pet()` takes the owner `char_data` and selects on
  `pet_idnum` alone. The owner name on an object row is display data.
- Both save paths in `src/players.c` delete replaced object rows by pet
  identity (`pet_idnum IN (SELECT pet_data_id ...)` for the owner snapshot,
  `pet_idnum = <id>` for keeper storage) before deleting the pet row. The
  foreign key cascade is the enforced backstop; the explicit statement keeps
  the transaction correct on tables that cannot carry the constraint and keeps
  the existing query-count assertions meaningful.
- `owner_id` / `owner_created` are not backfilled. `owner_created` cannot be
  derived in SQL, and setting `owner_id` alone would break the two-part
  binding check in `prepare_saved_pet_row()`. Legacy rows keep `owner_id = 0`
  and are adopted on the owner's next save, which is the behavior migration
  `2026080505` already documented.

## Load-failure audit

No path reachable from a pet restore terminates the server. Query and result
failures, unknown or unreadable prototypes, partial numeric fields, unclosed
containers, doubly claimed wear slots, and out-of-range locations all return
`PET_OBJECT_LOAD_FAILED` after discarding decoded objects, and
`load_char_pets()` drops the follower, marks the roster restore failed, and
refuses to replace the stored snapshot. `handle_obj()` returns the contents of
a missing container to the inventory. The six `exit()` calls in
`src/obj/objsave.c` belong to the player, house, and sheath loaders.

## Churn measurement

Local snapshot: 709 pet rows across 370 owners, average under two pets per
owner, maximum twenty; the largest pet inventory is 17 object rows and under
two kilobytes. With the fingerprint shortcut skipping unchanged owners, a
rewrite touches at most a few dozen rows in one transaction. Dirty-record
saves or batching were not adopted; see the reasoning in
`SAVE_SYSTEMS_BREAKDOWN.md`.

## Logging

`log_pet_object_failure()` in `src/obj/objsave.c` reports operation, owner ID,
pet row ID, object vnum and `mysql_errno`, never the serialized payload or the
SQL statement. Previously silent `goto cleanup` paths in
`objsave_save_obj_record_db_pet()` now record a reason.

## Tests

- `unittests/CuTest/test_pet_persistence.c` (new): stable owner binding and
  restore-graph rejection of malformed object sets.
- `unittests/CuTest/test_database_persistence.c`:
  `Test_pet_persistence_legacy_schema_migration_is_idempotent` now starts from
  the real pre-migration shape (signed identifier, nullable descriptions,
  orphaned and unbound object rows) and asserts the fills and purge. InnoDB
  refuses foreign keys on temporary tables, so the fixture records
  `2026091007` as applied and
  `Test_pet_live_schema_cascades_objects_and_rejects_orphans` (new) exercises
  the migration runner, validator, cascade, and orphan rejection (MariaDB
  error 1452) against the live tables; the fixture rows are rolled back and
  the live schema stays migrated.
  `Test_pet_restore_failure_blocks_snapshot_replacement` asserts the empty
  result on a pet row without objects instead of on a mismatched owner name.

Verified with `make test` (1364 tests), again with
`LUMINARI_TEST_MYSQL_ENABLE=1` against the local development database (row
counts unchanged), and with the full CI sequence against a throwaway MariaDB
loaded from `sql/master_schema.sql`: `prepare_test_runtime.sh`, `make test`,
`make test-all`, the startup smoke test with zero SYSERR lines, Valgrind, and
the ASan/UBSan suite. The fresh database applied migrations `2026091001`
through `2026091007` at boot.

## Issue checklist

- [x] Reconcile runtime initialization and `sql/master_schema.sql` against one
      migration authority.
- [x] Stable owner identity and an enforced pet-to-object relationship with
      orphan handling.
- [x] Audit pet object-load failure paths for recoverable errors.
- [x] Profile owner-rewrite churn before choosing dirty-record saves or
      batching.
- [x] Logging without serialized SQL payloads, plus schema, failure-injection
      and restore checks.
