# Production Crash and Pet Persistence Investigation

Status: Development repair in progress; production containment remains operator-owned

Incident: 2026-08-05 20:38:58-20:38:59 UTC

Investigation date: 2026-08-06

Repair started: 2026-08-06

## Development Repair Progress

The repair is being implemented and verified in the development checkout. The
production host remains out of scope for changes; production containment,
backup, schema application, recovery, restart, and validation remain explicit
operator actions.

Status meanings:

- `Verified`: implemented and covered by the named local evidence.
- `In progress`: implementation or task-specific verification is underway.
- `Pending`: no repair evidence has been recorded yet.
- `Operator action`: intentionally not performed from the development checkout.

| Work item | Status | Current evidence and remaining work |
|-----------|--------|-------------------------------------|
| Development environment safety | Verified | `lib/.env` reports `APP_ENV=development`; no production write or configuration action has been performed. |
| Legacy-schema reproduction | Verified | The local development database started with InnoDB pet tables, no `pet_data.runtime_state`, no owner indexes, and a unique `pet_save_objs.idnum` key instead of the required primary key. |
| Unconditional versioned migration | Verified | Startup now runs migrations `2026080501` through `2026080504` independently of help initialization. A MariaDB temporary legacy schema reproduces the unique-key variant, migrates twice with exactly four records, and preserves its pet and pet-object rows. |
| Schema contract and fail-closed startup | Verified | Startup verifies both InnoDB engines, required column types and nullability, primary keys, owner/relation indexes, and migration version. `boot_world()` exits before world load on migration or contract failure; an incompatible MariaDB fixture is rejected. |
| Atomic owner snapshot save | Verified | Each owner replacement now uses one transaction. Pet rows are prepared before it starts; recursive object-save failures propagate; any failed start, delete, pet insert, object insert, or commit rolls back. A two-follower MariaDB fixture preserved its prior linked snapshot at all nine forced query failures and on object-payload overflow. |
| Bounded failure logging | Verified | Full failed INSERT payload logging is removed. Pet-save failures now report rate-limited, bounded operation, owner, pet VNUM, database error code/detail, schema version, and suppressed-count context. |
| Save-churn reduction | Pending | Add a safe dirty/interval policy without allowing logout, extraction, combat, spell, or administrative save sites to lose a changed snapshot. |
| Production-linked regression coverage | In progress | The database-enabled root suite covers legacy migration, idempotence, schema rejection, multi-follower commit, quoted object payloads, nested equipment/inventory links, overflow rejection, and rollback at all nine transaction queries. The same fixture now supports repeated timed-affect mutation and passed 100 saves under sanitizers. Disconnect and extraction transitions remain. |
| Memory reproduction and diagnostics | In progress | On the current repaired source, 100 repeated full snapshots passed ASan/LeakSanitizer and a production-linked seven-test persistence suite passed Valgrind with zero errors and zero definitely lost bytes. Fail-fast UBSan exposed an unrelated pre-existing world-loader shift error. The unavailable exact `2.5033-beta` source and lifecycle transitions remain gaps. |
| Deployment and crash observability | Pending | Audit install/restart coupling, versioned binaries and debug symbols, boot commit/build identity, health identity, and end-to-end core capture. |
| Production containment and recovery | Operator action | Follow `Required Production Containment` only after a verified backup and controlled maintenance window. |

### Repair Checkpoints

- Baseline checkpoint: traced startup, schema verification, pet row save/load,
  pet-object serialization, and database wrappers at `b0a61a8d`. The unrelated
  local deletion of `docs/ongoing-projects/syntax-check-event-init-order.md`
  predates this repair and is excluded from repair commits.
- Schema checkpoint: added unconditional dated migrations, structural contract
  verification, and a fail-closed world boot gate. A warning-free GNU C23 build
  passed. The first database-enabled production-linked run exposed the local
  legacy unique-key variant through the syntax-check boot; migration
  `2026080504` now promotes it to the required primary key. A direct development
  syntax-check boot applied that migration, verified schema version
  `2026080504`, loaded the world, and exited cleanly. The regression fixtures
  remain connection-local temporary tables. The database-enabled
  production-linked suite then passed all 438 tests.
- Schema checkpoint commit: `6e1232c9` (`Make pet schema migration fail
  closed`), pushed to `origin/master`.
- Atomic-save checkpoint: pet row queries are prepared before a single owner
  transaction; pet-object payloads are SQL-escaped; recursive equipment and
  inventory failures propagate; and failed commits roll back. The
  database-enabled suite passes all 439 tests. Its two-follower fixture saves
  one equipped object plus carried and nested objects, then injects failure at
  each of the nine transaction queries and verifies the old linked snapshot
  remains intact. An oversized object payload is rejected and rolled back. The
  installed `bin/circle` then verified schema version `2026080504`, loaded the
  full development world, and completed syntax-check cleanup.
- Atomic-save checkpoint commit: `45abcba5` (`Make pet snapshot saves
  atomic`), pushed to `origin/master`.
- Memory-diagnostic checkpoint: the transaction fixture now attaches a timed
  charm affect, changes its duration on each pass, and performs a configurable
  number of complete saves while retaining two followers plus equipped,
  carried, and nested objects. One hundred passes completed under
  ASan/LeakSanitizer with all 439 tests passing. A generated
  database-persistence CuTest executable then passed all seven tests under
  Valgrind after 8,252 allocations, with zero errors and zero definitely lost
  bytes. The initial ASan run also found and corrected an independent test
  parser width that allowed an 8,191-byte scan into a 512-byte buffer.
- Sanitizer boundary: fail-fast UBSan stops the forked full-world syntax check
  at `src/db.c:7463`, where `check_bitvector_names()` shifts by a wrapped
  negative count when the number of known flags exceeds the bitvector width.
  This occurs before the parent runs the persistence fixture and is not a pet
  persistence failure. A complete Valgrind world boot also reports three
  existing cleanup leak contexts: one four-byte region allocation and 4,782
  room-name/description strings. Those child-process findings are kept
  separate from the clean persistence-suite result.
- Next checkpoint: add disconnect and extraction transition coverage, then
  reduce periodic save churn without weakening explicit lifecycle saves.

### Memory Diagnostic Commands

Database environment values were loaded from the development
`lib/mysql_config` without printing them. The relevant variable names and
commands were:

```bash
cmake -S . -B "$asan_build" -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug \
  '-DCMAKE_C_FLAGS=-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer' \
  '-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined'
cmake --build "$asan_build" --target cutest -j"$(nproc)"

LUMINARI_TEST_MYSQL_ENABLE=1 LUMINARI_TEST_PET_SAVE_LOOPS=100 \
  LUMINARI_TEST_ROOT="$PWD" \
  ASAN_OPTIONS='abort_on_error=1:detect_leaks=1:strict_string_checks=1' \
  UBSAN_OPTIONS='halt_on_error=0:print_stacktrace=1' \
  "$asan_build/cutest"
```

That run passed all 439 tests. Repeating it with
`UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1'` preserved the pet-test
pass but made the overall suite fail on the independent world-loader shift
described above.

The focused Valgrind pass used the complete generated suite from
`test_database_persistence.c`, not an individual test-function filter:

```bash
unittests/CuTest/make-tests.sh \
  unittests/CuTest/test_database_persistence.c \
  > unittests/CuTest/AllTests.c
make -j"$(nproc)" cutest

LUMINARI_TEST_MYSQL_ENABLE=1 LUMINARI_TEST_PET_SAVE_LOOPS=100 \
  valgrind --tool=memcheck --leak-check=full \
    --show-leak-kinds=definite --errors-for-leak-kinds=definite \
    --track-origins=yes --error-exitcode=99 \
    --suppressions=unittests/CuTest/cutest.supp ./cutest
```

The normal 439-test `AllTests.c` runner was regenerated immediately after the
focused diagnostic.

## Scope and Safety

This was a read-only production investigation plus a source and test review in
the development checkout. No production code, schema, data, service, or
configuration was changed.

The production repository was at `cb8748a4` when inspected. The development
checkout was at `61c03285`. The process that crashed had logged version
`2.5033-beta`; its exact commit cannot be recovered. The pet runtime-state code
was active, as demonstrated by its emitted SQL, and the relevant pet save and
startup-migration code did not change between its introduction in `1a0a5976`
and the inspected production revision.

## Executive Conclusion

The immediate crash mechanism is confirmed, but the code that originally
damaged the heap is not.

- glibc detected inconsistent heap chunk metadata and called `abort()`. The
  game exited with status 134/SIGABRT.
- The pet save path is a credible candidate because it was allocating,
  escaping, logging, and freeing pet data repeatedly near the crash. It is not
  proven to be the corrupting path. glibc detected the damage during a later
  allocator operation, and the same failed pet branch had completed 22,767
  times in the retained logs before the one abort.
- A separate P0 defect is proven: existing databases never receive the new
  `pet_data.runtime_state` column. Startup checks only whether `pet_data`
  exists, not whether its required columns exist, so it skips the ALTER and
  reports success.
- The schema mismatch is destructive. `save_char_pets()` deletes the owner's
  old pet and pet-object rows before attempting the incompatible INSERT. The
  INSERT then fails, leaving no durable pet row. Loading also fails because its
  SELECT names the missing column.
- MariaDB did not crash or restart. Its only incident-time warnings were the
  game's three connections disappearing when the game aborted. This was not a
  database-server outage.
- No incident core or backtrace exists. Apport explicitly ignored the crash
  because `bin/circle` had been replaced after the live process started.

The production schema mismatch and ongoing pet data loss require urgent
containment even if they prove unrelated to the heap corruption.

## Confidence Summary

| Conclusion | Confidence | Basis |
|------------|------------|-------|
| glibc aborted after detecting corrupted heap metadata | Confirmed | Allocator diagnostic, exit 134, and SIGABRT log |
| MariaDB itself caused the outage | Ruled out by available evidence | MariaDB stayed active with zero restarts; disconnect warnings followed the game abort |
| Production lacks `pet_data.runtime_state` | Confirmed | Live `information_schema` query returned zero matching columns |
| Startup migration logic skips the required ALTER | Confirmed | Source control flow and five production boot sequences |
| Pet persistence is deleting durable data | Confirmed | Delete-before-insert source order, failed INSERTs, and zero current rows for all four observed owners |
| The pet code corrupted the heap | Unproven, low confidence | Strong temporal correlation but no stack, no core, no obvious error in the failing cleanup branch, and thousands of prior executions |
| Recurrence is possible | Confirmed | The column remains absent and post-restart SELECT failures continued |

## Incident Timeline

All times below are UTC on 2026-08-05.

| Time | Event |
|------|-------|
| 20:38:43 | Gerok's skull spider save fails on missing `runtime_state`. |
| 20:38:49 | The same save fails again. |
| 20:38:55 | The last complete pet INSERT attempt fails. Its runtime state is retained in `syslog.CRASH`. |
| 20:38:56 | The I3 thread successfully completes another presence synchronization. |
| 20:38:58.462 | Apport records the crash but ignores it because the executable changed after process start. MariaDB records the game's three client connections dropping. |
| 20:38:59 | `autorun.sh` records exit 134 after 103,759 seconds and identifies SIGABRT. |
| 20:40:02 | `autorun.sh` starts a replacement game process. |
| 20:40:36 | Boot completes and the server enters the game loop. |

The confirmed unavailable interval was approximately 97 seconds, from the
abort at 20:38:59 through game-loop entry at 20:40:36. The listener existed
while the replacement process booted, but gameplay was not ready until the
game-loop entry.

## Primary Production Evidence

### Crash record

Production `syslog.CRASH` contains:

- Lines 6-7, 14-15, and 20-21: the final three failed Gerok pet INSERTs.
- Line 26: `malloc(): mismatching next->prev_size (unsorted)`.
- Lines 27-29: exit 134, 103,759 seconds of runtime, and SIGABRT.

The complete rotated log has the same final sequence at
`log/syslog.20260805:144767-144790`. No player command appears in the final
three minutes. The recorded activity is the six-second pet save, five-second
I3 synchronization, periodic terrain probes, and ordinary unlogged heartbeat
work.

The allocator message means that adjacent heap chunk boundary metadata no
longer agreed. It is a detection point, not a reliable write site: an earlier
out-of-bounds write, use-after-free, or double free may only be noticed when a
later allocation examines that chunk.

### No usable core

`/var/log/apport.log:1` records:

```text
ERROR: apport (pid 781582) 2026-08-05 20:38:58,462: executable was modified after program start, ignoring
```

The live process began on 2026-08-04 at 15:49:40 UTC. The on-disk
`bin/circle` had a modification time of 2026-08-05 09:54:19 UTC, so an install
or replacement occurred while the old process remained alive. No matching
file exists in the project `dumps/` directory, `/var/crash`, or
`/var/lib/apport/coredump`. `systemd-coredump` is not installed. The binary on
disk has debug information, but there is no process image against which to use
it.

This also means the repository HEAD and current binary cannot identify the
exact bytes that crashed.

### Database health

MariaDB 10.11.14 remained `active/running`, with the same main PID, no restart,
and a successful service result. Its only messages in the incident window were
three `Aborted connection` warnings at 20:38:58. Those are consistent with the
game process disappearing and are consequences of the abort.

There was no kernel OOM, killed-process, segmentation-fault, or general-
protection message in the incident window.

## Confirmed Schema Migration Defect

Commit `1a0a5976` added `runtime_state` in two places:

- The fresh-table definition and an `ALTER TABLE ... ADD COLUMN IF NOT EXISTS`
  in [`src/db_init.c`](../../src/db_init.c).
- The fresh-schema definition in
  [`sql/master_schema.sql`](../../sql/master_schema.sql).

Neither upgrades an existing production table during ordinary startup.

[`initialize_missing_tables()`](../../src/db_startup_init.c) calls
`init_core_player_tables()` only when `player_data` or `pet_data` is entirely
absent. Both tables already exist in production, so the function containing
the ALTER is skipped. `CREATE TABLE IF NOT EXISTS` in `master_schema.sql`
likewise does not add a column to an existing table.

The repository has a `schema_migrations` framework, but it currently contains
help-system migrations only and is itself reached through help-table
initialization. The pet change did not use it. It is not an unconditional
startup migration path.

Production-host boot logs prove the control flow. On the five inspected boots
at 2026-08-04 15:49:40 and 23:05:38, then 2026-08-05 06:12:59, 08:00:38, and
09:54:31, startup logged:

```text
Starting database startup initialization...
Checking for missing database tables...
Database startup initialization completed successfully
```

None logged `Initializing core player tables`, and none applied the ALTER.
The post-crash boot repeated the false-success sequence at 20:40:02.

Read-only `information_schema` checks returned:

```text
production runtime_state columns: 0
development runtime_state columns: 0
```

This is therefore not only unexplained production drift. Existing development
and production databases both demonstrate the migration orchestration defect.

## Confirmed Pet Data-Loss Chain

The periodic call chain is:

```text
heartbeat, every six seconds
  -> update_player_misc()
     -> save_char_pets(owner)
        -> DELETE pet_save_objs for owner
        -> DELETE pet_data for owner
        -> serialize each live charmed follower
        -> INSERT pet_data including runtime_state
           -> production rejects missing column
           -> free temporary buffers and return
        -> pet_save_objs() is never reached
```

The source locations are:

- [`src/comm.c`](../../src/comm.c): the six-second heartbeat.
- [`src/limits.c`](../../src/limits.c): `update_player_misc()` calls
  `save_char_pets()` for every playing descriptor.
- [`src/players.c`](../../src/players.c): delete, serialize, INSERT, and failure
  cleanup.

There is a second destructive path on login:

```text
load_char_pets(owner)
  -> SELECT ..., runtime_state FROM pet_data
     -> production rejects missing column
     -> no saved followers are loaded
  -> next six-second save sees no loaded followers
     -> deletes any old pet and pet-object rows
     -> inserts nothing
```

Thus an owner does not need to issue a pet command to lose persistence. Login
plus the ordinary six-second update is enough.

### Observed extent

After excluding duplicate rotated copies, the two retained pre-crash logs
contain 22,833 missing-column errors:

- 22,767 failed INSERTs.
- 66 failed SELECTs.

The failed INSERT owners were:

| Owner | Failed INSERTs | Last recoverable attempted row |
|-------|---------------:|--------------------------------|
| Gerok | 13,138 | `syslog.CRASH:21`, 20:38:55 UTC |
| Zridt | 5,569 | `log/syslog.20260805:137961`, 19:15:31 UTC |
| Xantos | 2,042 | `log/syslog.20260805:62095`, 03:27:55 UTC |
| Raistalyn | 2,018 | `log/syslog.20260805:62037`, 03:27:25 UTC |

At the read-only database check, all four owners had zero rows in both
`pet_data` and `pet_save_objs`. The whole database still had 763 `pet_data`
rows, which is expected because owners who did not enter the affected runtime
path were not deleted. Five more failed runtime-state SELECTs were already
present in the post-restart log, so the defect remains active.

The logged failed INSERTs preserve the last base pet row and serialized runtime
state for these four owners. They do not preserve deleted `pet_save_objs`
equipment rows. MariaDB binary logging is disabled, no backup matching the
project backup script's convention was found under the production project or
home directory, and off-host/Plesk backups were not audited. Recovery sources
outside the application logs must therefore be checked before declaring the
object rows unrecoverable.

## Pet Path as a Heap-Corruption Candidate

### Evidence supporting investigation of this path

- It is the only application-specific error repeated immediately before the
  abort.
- The runtime-state feature added allocation, serialization, SQL escaping, and
  cleanup work to a path that runs every six seconds.
- The last attempted row contains an active timed affect whose duration changes
  each cycle, so the serializer traversed the live affect list on every save.
- No player command provides a better logged trigger in the final window.

### Evidence against claiming it as the crash root cause

- The exact failed INSERT branch completed 22,767 times before the one abort.
  That is inconsistent with a simple deterministic double free or fixed-size
  overflow in every invocation.
- The Gerok runtime payload is far below the serializer's initial 4 KiB
  allocation, so its dynamic-growth `realloc()` path is not used for this pet.
- `snprintf()` bounds the INSERT buffer. The query allocation includes all
  escaped string lengths plus 768 bytes of fixed overhead.
- `mysql_escape_string_alloc()` allocates the documented worst case of twice
  the input length plus one byte.
- On the failed INSERT branch, the query, five escaped fields, serialized
  runtime state, and escaped owner are distinct allocations and are each freed
  once. Static tracing found no double free or out-of-bounds write in that
  branch.
- `pet_save_objs()` is called only after a successful pet row INSERT, so the
  pet-object serializer is not part of these failed cycles.
- I3 completed another allocation-heavy JSON synchronization after the last
  pet error, and numerous heartbeat paths run without logging. The allocator
  diagnostic does not identify which thread detected the damage.

The remaining pet-specific possibility is state dependent: corrupted follower,
affect, equipment, or follower-list data could be read during serialization,
or a defect elsewhere could corrupt one of those structures before this path
runs. Only a captured stack or an instrumented reproduction can distinguish
that from unrelated older corruption.

## Test Evidence and Gaps

The available development `cutest` binary passed all 416 tests normally and
under Valgrind. Valgrind reported zero errors, zero definitely lost bytes, and
39,260 allocations during the suite.

This is useful but not exonerating. Existing pet coverage tests only:

- one follower runtime-state serialize/restore round trip; and
- rejection of one incomplete serialized payload.

There is no automated coverage for:

- upgrading an already-existing `pet_data` table;
- startup verification of required columns;
- `load_char_pets()` against a legacy schema;
- a failed `save_char_pets()` INSERT preserving old rows;
- repeated live pet saves with a timed affect;
- the complete six-second delete/serialize/error cleanup path; or
- transactionality between `pet_data` and `pet_save_objs`.

The production defect can therefore coexist with a clean test suite and clean
Valgrind run.

## Required Production Containment

These actions were not performed during this investigation.

1. Put the game in a controlled maintenance window so more owners cannot enter
   the destructive load/save path.
2. Take and verify a complete logical database backup before changing schema or
   replaying rows.
3. Apply and verify the missing column:

   ```sql
   ALTER TABLE pet_data
     ADD COLUMN IF NOT EXISTS runtime_state LONGTEXT DEFAULT NULL AFTER cha;
   ```

4. Confirm exactly one `runtime_state` row exists in `information_schema.COLUMNS`
   for the active database.
5. Restart from the exact installed binary so the running image, on-disk image,
   repository revision, and debug symbols match.
6. Validate ordinary pet load and save with a disposable test owner before
   reopening production.
7. Recover affected base pet rows from a trusted backup where possible. If no
   backup exists, review and transactionally replay only the last complete
   logged INSERT for each affected owner after confirming current player state.
   Do not blindly replay all 22,767 attempts. Pet equipment needs a backup or
   another independent source.

## Required Development Repairs

### P0: Schema and persistence safety

- Move the pet column change into an unconditional, versioned startup migration
  path. Startup must verify columns, indexes, and types, not only table names.
- Make startup fail closed, or explicitly disable pet persistence, when the
  runtime schema does not satisfy the code contract. A success log must not be
  emitted after a required migration was skipped or failed.
- Make pet saves transactional. Do not delete the previous durable snapshot
  until all replacement pet and pet-object rows have been serialized and
  inserted successfully. Roll back the whole owner save on any error.
- Add a legacy-schema load fallback only if backward compatibility is required;
  it must not be allowed to feed the current delete-before-insert path.

### P1: Reproduction and memory diagnostics

- Reproduce in an isolated database cloned with the legacy production table,
  Gerok's last logged runtime payload, and the source revision that produced
  version `2.5033-beta`.
- Loop the full six-second save path under ASan/UBSan and Valgrind, including
  forced INSERT failure, timed-affect mutation, equipment, multiple followers,
  and disconnect/extraction transitions.
- Run with allocator diagnostics that fail close to the first inconsistency,
  and preserve a core plus all thread backtraces.
- Audit the periodic I3, terrain, event, affect, and output paths from the same
  final three-minute window if the isolated pet loop remains clean.

### P1: Core and deployment observability

- Never replace a live executable without completing the matching restart.
  In-place replacement made Apport discard the only useful crash artifact.
- Keep versioned binaries and matching debug symbols by build ID until the
  release is retired.
- Log the Git commit and ELF build ID at boot, and expose the active process
  identity in health checks.
- Verify core capture end to end with the actual systemd/Apport configuration.
  `autorun.sh` currently archives only a local `core` file and cannot recover a
  core that Apport ignores.

### P2: Churn, logs, and tests

- Save pets on a dirty flag or a safer interval instead of deleting and
  rebuilding every owner's rows every six seconds.
- Stop logging complete failed INSERT statements. They expose descriptions and
  runtime state and expanded the retained log to about 26 MiB. Log a bounded
  owner identifier, pet VNUM, error code, and schema version instead, with rate
  limiting.
- Add production-linked tests for legacy-schema migration, migration
  idempotence, rollback after every failure point, multi-row owner saves, and
  pet-object foreign-key preservation.

## Open Questions

- Which thread and allocator call detected the corrupted chunk?
- Which earlier write damaged the chunk, and how long before detection did it
  occur?
- Are trustworthy Plesk or off-host database backups available from before
  2026-08-03 21:38 UTC?
- Can the four affected owners' pet equipment be reconstructed independently?
- Why was `bin/circle` replaced while the production process continued to run?

Until a core-backed or sanitizer-backed reproduction answers the first two
questions, the correct statement is: the schema and pet data-loss defects are
confirmed; pet code remains a candidate for the heap corruption, not its proven
cause.
