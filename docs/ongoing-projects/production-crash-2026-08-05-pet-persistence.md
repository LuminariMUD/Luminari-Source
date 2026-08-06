# Production Crash and Pet Persistence Investigation

Status: Development repair and local validation complete; production
deployment, recovery, and validation remain operator-owned

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
| Save-churn reduction | Verified | The unconditional heartbeat rewrite moved from the six-second miscellaneous update to the existing 60-second save pulse. Explicit quit, idle extraction, death, charm, summon, dismissal, combat, spell-transfer, manual-save, copyover, and administrative sites remain immediate. |
| Production-linked regression coverage | Verified | The ordinary and development-MariaDB runs both pass all 444 production-linked tests; the database run performs ten complete pet snapshot replacements. Coverage includes legacy migration, schema rejection, multi-follower commit, nested objects, rollback at all nine transaction queries, timed-affect mutation, lifecycle transitions, copyover pinning, partial output writes, terrain range rejection, and callback-safe affect expiration. |
| Memory diagnostics on repaired source | Verified | One hundred repeated full snapshots plus lifecycle transitions passed ASan/LeakSanitizer, and a production-linked eight-test persistence suite passed Valgrind with zero errors and zero definitely lost bytes. A refreshed 444-test ASan/UBSan binary also exits cleanly with leak detection enabled. Fail-fast UBSan separately exposes the documented pre-existing world-loader shift error. The unavailable exact `2.5033-beta` source and historical allocator abort cannot be reproduced. |
| Deployment and crash observability | Verified | Both build systems install immutable build-ID binaries/symbols; autorun pins each PID to its resolved executable, publishes active-versus-installed health identity, and analyzes local/systemd cores with that exact image. Synthetic supervisor/core handling is verified. The local WSL pipe handler has no retrieval client, and the real production core route remains an operator self-test. |
| Incident-window periodic path audit | Verified | Output partial writes had a confirmed write-before-buffer defect and now preserve exact counters. I3 state/authentication reads and writes now share a mutex. Terrain batch input now rejects invalid types, null commands, out-of-bounds coordinates, overflowing products, and batches above 1,000 cells. Affect expiration detaches nodes before invoking list-mutating cleanup callbacks. Event queue ownership and deferred extraction were traced without finding a concrete incident-window defect. |
| Clean build, test, and install | Verified | A detached clean source at `f427c35d`, using fresh-clone configuration headers, the development database, and the tracked minimal world fixture, completes `autoreconf`, `configure`, a warning-free parallel build, the aggregate `make test` target, the database-enabled 444-test run with ten snapshot loops, and `make install`. The installed dirty=0 release has build ID `be2e0fdd81657bff5f12a99c531824148326cf07`, matching symbols, SHA-256, and manifest, and no root `circle` remains. |
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
  ASan/LeakSanitizer with all 440 tests passing. A generated
  database-persistence CuTest executable then passed all eight tests under
  Valgrind after 8,386 allocations, with zero errors and zero definitely lost
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
- Lifecycle and churn checkpoint: tracing every call site found that idle
  extraction detaches `ch->desc` before its explicit pet save, while
  `save_char_pets()` rejected descriptor-less owners. Pet saves now depend on
  player identity rather than an attached socket. A database fixture verifies
  the playing periodic save, confirms disconnected descriptors are skipped,
  saves successfully after descriptor detachment, and atomically removes the
  durable snapshot after follower removal. The ordinary periodic save now
  runs on the existing 60-second character-save pulse instead of every six
  seconds; all event-driven save sites remain unchanged. The database-enabled
  suite passes all 440 tests.
- Lifecycle and churn checkpoint commit: `bb6e70ca` (`Preserve pet lifecycle
  saves and reduce churn`), pushed to `origin/master`.
- Deployment audit checkpoint: Autotools installs the server directly over
  `bin/circle`; CMake writes the linked server directly to the same pathname
  even before its install step. Autorun then launches that mutable pathname,
  identifies live processes against its current target, and runs GDB against
  its current contents after a crash. This explains how an install without a
  restart both invalidates external core handling and loses the exact debugger
  image. The local repair will install immutable build-ID release paths, retain
  matching symbols, launch and record the resolved path, and report both active
  and candidate identities.
- Immutable-release checkpoint: both Autotools and CMake now build away from
  the launch alias and promote candidates through one installer. The installer
  requires an ELF build ID and machine-readable embedded Git identity, retains
  the executable, a matching `circle.debug`, and a SHA-256 manifest under the
  build ID, and atomically updates `bin/circle`. A regression test verifies two
  releases, activation while the first immutable release is executing, symbol
  retention, manifest identity, and refusal to migrate a live legacy regular
  file. The real local Autotools install preserved the prior `19841857...`
  release and symbols, installed the new `5a4c29b3...` release and symbols,
  activated the symlink, and removed the root executable. CMake configuration
  and both identity-bearing C objects also compile cleanly. The full recursive
  install remains blocked only by the unrelated concurrent unmatched `#endif`
  in `src/act.informative.c`; the already-linked root install phase completed.
- Immutable-release checkpoint commit: `1946ccdf` (`Install immutable build-ID
  releases`), pushed to `origin/master`.
- Supervisor identity checkpoint: autorun resolves and verifies the activated
  release once, launches that exact path with its ELF build ID, and atomically
  records PID, executable, Git commit/dirty state, build ID, and SHA-256.
  Process discovery, status, shutdown, and stale-PID handling consult the
  recorded executable, so changing `bin/circle` cannot redirect management of
  an older process. The health state reports active and installed identities
  plus `yes`, `restart-required`, or `not-running`. Managed systemd restart now
  waits for a fresh, distinct game PID and exact installed identity, not only
  the supervisor `MainPID`. Copyover resolves `/proc/self/exe` before teardown
  and re-execs that immutable path, preventing an alias activation from
  switching binaries inside the already-recorded PID.
- Crash-observability checkpoint: autorun retains the last launched identity,
  scans `core` and `core.*` in the project, library, and bin locations, can
  retrieve a PID-specific systemd core through `coredumpctl`, archives an
  identity sidecar, verifies the executable SHA-256, and requests full
  all-thread GDB backtraces from that exact immutable executable. The expanded
  supervision regression rotates the alias from release A to B while A is
  running, observes `restart-required`, safely stops A, and proves both the
  crash identity and fake-GDB executable argument still name A.
- Host core-capture boundary: `verify_core_capture.sh --self-test` generated a
  real local SIGABRT, but this WSL kernel pipes cores to
  `/wsl-capture-crash` and provides neither `coredumpctl` nor another supported
  retrieval client. The verifier therefore exited 2 as `UNVERIFIED`, rather
  than claiming capture success. Production must run the same self-test against
  its actual Apport/systemd configuration.
- Current regression result: the ordinary production-linked suite and the same
  suite with the development MariaDB fixture and ten complete pet snapshot
  loops both pass all 441 tests. The supervision and immutable installer shell
  suites also pass. Changed copyover production objects compile warning-free.
- Supervisor/crash checkpoint commit: `90899995` (`Pin crash diagnostics to
  exact releases`), pushed to `origin/master`.
- Partial-output audit checkpoint: `process_output()` used overlapping
  `strcpy()` when a socket accepted only part of queued output. If the socket
  accepted all queued output but only part of an appended prompt or overflow
  message, the code then subtracted the retained suffix length from a zero
  `bufptr` and added it to `bufspace`. The next ordinary formatted output could
  consequently address memory before the descriptor output buffer. The repair
  uses bounded copies and exact buffer accounting for both partial-write cases;
  a production-linked regression proves the retained content and a subsequent
  append. The ordinary suite passes all 442 tests.
- Periodic-path audit checkpoint: I3 connection state and authentication now
  use the same mutex for all reads and writes exercised by the client thread,
  presence publisher, and status paths. The presence publisher continues to
  traverse descriptors only on the main thread and transfers one owned JSON
  reference through the locked command queue. Terrain batch parsing previously
  coerced arbitrary JSON values to `int`, calculated dimensions in signed
  `int`, and could accept an overflowed product as a small batch; it now
  validates integer types and wilderness bounds before an `int64_t` size
  calculation and rejects batches above 1,000 cells. A null command can no
  longer reach `strcmp()`.
- Affect/event audit checkpoint: custom skill wear-off handlers were hidden
  behind automatically generated generic messages, so rage and defensive
  stance cleanup did not run on timed expiration. This masking means those
  callbacks are not a candidate for the August 5 abort. The corrected dispatch
  order invokes custom cleanup only after every expired affect node has been
  detached, preventing a handler that removes or replaces another affect from
  invalidating the traversal. Event processing was traced from queue removal
  through callbacks, cancellation, extraction, and cleanup; callbacks receive
  dequeued events, self-cancellation avoids double free, and character
  extraction is deferred. No concrete incident-window event defect was found.
  The ordinary production-linked suite passes all 444 tests.
- Periodic-path checkpoint commit: `f0b6f7b9` (`Harden incident-window periodic
  paths`), pushed to `origin/master`.
- Final database and memory checkpoint: the development-MariaDB run passes all
  444 tests with ten complete snapshot replacements. A refreshed 444-test
  ASan/UBSan executable, containing the output, terrain, I3, and affect changes,
  exits zero with strict string checks and leak detection enabled. The complete
  normal executable also exits zero under Valgrind Memcheck with invalid-access
  failures configured to return 99. The earlier focused persistence Memcheck
  remains the authoritative leak summary because full server lifecycle tests
  close Valgrind's reporting descriptor.
- Clean-source validation checkpoint: the primary checkout cannot compile its
  unrelated concurrent `src/act.informative.c` change because line 9998 has an
  unmatched `#endif`; that file is not part of this repair and was not changed.
  A detached worktree at `470f5a1f`, with fresh-clone example configuration
  headers and read-only links to the same development database/world fixture,
  completes `autoreconf`, `configure`, a warning-free parallel build,
  `make test`, and `make install`. The aggregate target passes all 444 CuTests,
  autorun supervision, versioned installation, background help, and vessel
  tooling regressions. Installation activates dirty=0 build ID
  `3f60699d1a84846baf03af239252afacf564a2f8`, preserves its manifest and
  symbols, and removes the root `circle` artifact.
- Completion-audit checkpoint: a second detached, Git-clean worktree at
  `f427c35d` used fresh-clone example configuration headers and read-only links
  to the development database configuration. `autoreconf`, `configure`, and a
  warning-free parallel build passed. With temporary indexes pointing at the
  tracked minimal world fixture, the aggregate `make test` target passed all
  444 CuTests plus autorun supervision, versioned installation, background
  help, vessel preflight and memory analysis, process-memory, and vessel-scale
  parser regressions. The database-enabled binary separately passed all 444
  tests with ten complete pet snapshot replacements and verified apostrophes
  through all four pet text columns and recursive object payloads. The current
  mutable development world was not used for final evidence because its room
  `#9433` has an unrelated format error; the first audit run exposed that
  fixture defect without changing it.
- Completion-audit install: after removing the temporary fixture indexes, the
  worktree was Git-clean and `make install` activated release
  `be2e0fdd81657bff5f12a99c531824148326cf07`. The binary reports commit
  `f427c35da3a6c5be6234dbf55ed8726cea2a8b6f` with `GIT_DIRTY=0`; its executable
  and detached symbols share that ELF build ID; the manifest records SHA-256
  `e027bd9268eaebaa705724942dd8f919d22bc20675eca7c7dd874402d77f8156`; and
  installation removes the root `circle` artifact.
- Development completion boundary: source repairs and available local gates
  are complete. Applying migrations, restarting production, validating pet
  state, testing core capture on the real host, and recovering affected owners
  remain operator actions.

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

That run passed all 440 tests. Repeating it with
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

The normal 440-test `AllTests.c` runner was regenerated immediately after the
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
- A separate output-buffer defect could write before a descriptor buffer after
  a partial socket write. That is a confirmed heap-corruption mechanism and a
  credible incident candidate, but no retained evidence proves that the
  required partial-write state occurred before this abort.
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
| Partial output accounting corrupted this process | Unproven | The old branch can address memory before its buffer after a partial write, but no core or socket-write trace ties that state to the incident |
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

## Baseline and Repaired Test Evidence

### Baseline before repair

The original development `cutest` binary passed all 416 tests normally and
under Valgrind. Valgrind reported zero errors, zero definitely lost bytes, and
39,260 allocations during that suite. Pet coverage then consisted only of one
runtime-state round trip and rejection of one incomplete serialized payload.
It did not exercise an existing-table migration, the startup contract, atomic
replacement, query failures, repeated saves, multiple followers, or pet
objects. The production defect could therefore coexist with that clean
baseline.

### Repaired-source evidence

The production-linked suite now contains 444 tests and adds these incident
controls:

- a real MariaDB temporary legacy schema is migrated twice, retains its linked
  rows, records exactly migrations `2026080501` through `2026080504`, and
  accepts its legacy NULL runtime state;
- an incompatible schema is rejected by the same structural contract used at
  startup;
- a two-follower owner snapshot stores equipped, carried, and nested objects,
  timed runtime state, and apostrophes in pet text and object payloads;
- the previous linked snapshot survives forced failure at every one of the
  nine transaction queries and an oversized recursive object payload;
- repeated snapshots vary the timed-affect duration, and lifecycle coverage
  verifies connected periodic saves, disconnected periodic skips,
  descriptor-detached explicit saves, and follower-removal deletion;
- partial descriptor writes, terrain batch bounds, list-mutating affect
  expiration, immutable copyover, versioned installation, and supervisor
  identity have focused regressions.

The ordinary and development-MariaDB suites both pass all 444 tests. The
database run performs ten complete snapshot replacements; the ASan/LeakSanitizer
run performs 100. The focused persistence suite passes Valgrind with zero
errors and zero definitely lost bytes.

The final completion audit ran from detached clean source at `f427c35d`. The
aggregate target passed against the tracked minimal world fixture, and the
database-enabled rerun passed with all persistence checks active. A separate
attempt against the mutable development world stopped at its unrelated room
`#9433` format error; this is an external world-data fixture issue, not a
failure in the repaired source or tracked test fixture.

A raw legacy table is intentionally not a supported load state: startup must
migrate and verify it before `load_char_pets()` can run, or boot fails closed.
Legacy rows whose newly added `runtime_state` is NULL remain supported. The
unavailable historical `2.5033-beta` source, a historical core, and a live
production pet-load/recovery test remain evidence and operator boundaries, not
unimplemented development paths.

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

## Production Deployment and Validation Checklist

Run this checklist from the production project root during an approved
maintenance window. Do not reopen the game if any required check fails.

### Preflight and backup

1. Record the current source and process identity before making changes:

   ```bash
   git rev-parse HEAD
   readlink -f bin/circle
   ./scripts/autorun/autorun.sh status
   ```

2. Stop normal player access and take a verified full logical database backup.
   Copy it off-host. Record current `pet_data` and `pet_save_objs` totals and
   the rows for Gerok, Zridt, Xantos, and Raistalyn before migration or
   recovery.
3. Confirm the checkout is the intended clean commit. If `bin/circle` is still
   a regular file and that same inode is live, stop the service once before
   installation. The installer deliberately refuses to replace a live legacy
   executable.

### Build and activate

4. Build, test, and install in that order:

   ```bash
   make clean
   make -j"$(nproc)"
   make test
   make install
   ```

   Require every test to pass and confirm no root-level `circle` remains.
5. Verify that the activated alias, immutable release, debug symbols, and
   manifest agree:

   ```bash
   test -L bin/circle
   bin/circle --build-info
   release_dir=$(dirname "$(readlink -f bin/circle)")
   cat "$release_dir/manifest"
   readelf -nW "$release_dir/circle" | awk '/Build ID:/ {print $NF; exit}'
   readelf -nW "$release_dir/circle.debug" | awk '/Build ID:/ {print $NF; exit}'
   sha256sum "$release_dir/circle"
   ```

6. Restart through the managed service and verify the newly installed identity:

   ```bash
   sudo systemctl restart luminari.service
   ./scripts/autorun/autorun.sh status
   awk -F= '$1 == "MUD_IDENTITY_MATCH" {print $2}' .autorun.state
   ```

   Require `MUD_IDENTITY_MATCH=yes`. The active executable, Git commit, ELF
   build ID, and SHA-256 must match the installed release.

### Schema, pet behavior, and recovery

7. Inspect the boot log. Migrations `2026080501` through `2026080504` must
   either apply successfully or already be recorded, followed by:

   ```text
   Info: Pet persistence schema contract verified at version 2026080504
   ```

   Any migration or schema-contract error is a fail-closed startup failure, not
   a warning to bypass. Confirm both pet tables use InnoDB, the required
   columns and indexes exist, and the migration table records all four
   versions.
8. With a disposable test owner, verify all of the following before reopening:
   two followers; equipped, carried, and nested pet objects; punctuation in
   names/descriptions; save and quit; login; a managed restart; login again;
   then dismiss one follower, save, and confirm it does not reappear.
9. Recover Gerok, Zridt, Xantos, and Raistalyn only from a trusted backup or a
   separately reviewed recovery transaction. Validate ownership, follower
   count, runtime state, equipment, carried objects, and nested objects for
   each owner. Keep the pre-recovery snapshot and an audit of every row
   restored.

### Crash capture and observation

10. Verify real host core capture end to end:

    ```bash
    ./scripts/debugging/verify_core_capture.sh --self-test
    ```

    Require `SELF_TEST=PASS`. Exit 2 or `UNVERIFIED` is not sufficient for
    production crash readiness.
11. During the maintenance smoke test and after reopening, monitor the game,
    autorun, MariaDB, and system logs for pet-persistence failures, allocator
    diagnostics, restarts, and identity drift. Preserve any core together with
    its generated identity sidecar and exact immutable release directory.
12. If validation fails, keep maintenance mode active and preserve the new
    schema. Assess whether to reactivate a previous immutable release and
    perform another managed restart; do not reverse the schema migrations or
    replay pet rows blindly.

## Development Repair Requirements and Disposition

All implementable source work is complete and locally verified. The tables
below preserve the original requirements and make their final disposition
explicit.

### P0: Schema and persistence safety

| Requirement | Disposition | Evidence |
|-------------|-------------|----------|
| Unconditional, versioned pet migrations with structural verification | Verified | Migrations `2026080501` through `2026080504`, the startup schema contract, and the idempotent MariaDB legacy fixture |
| Fail closed when migration or contract validation fails | Verified | `startup_database_init()` returns failure and `boot_world()` exits before world loading; the incompatible-schema fixture is rejected |
| Replace each owner snapshot atomically | Verified | Pet rows are prepared first; one InnoDB transaction owns both deletes, all pet/object inserts, and commit; every failure point preserves the prior linked snapshot |
| Legacy compatibility must not reach destructive save behavior | Verified by design | Startup migration is mandatory; NULL legacy runtime state loads through the existing fallback, while an unverified raw legacy schema cannot boot |

### P1: Reproduction and memory diagnostics

| Requirement | Disposition | Evidence |
|-------------|-------------|----------|
| Reproduce the legacy schema and incident-shaped pet state in isolation | Verified for available evidence | The MariaDB fixture reproduces the deployed table defect and exercises two followers, timed state, equipment, inventory, nesting, punctuation, and forced failures |
| Reproduce the exact `2.5033-beta` process and Gerok payload | Historical evidence unavailable | The binary did not expose a commit, its on-disk image was replaced, no core exists, and the retained payload was not copied into development; no local source change can reconstruct these artifacts |
| Loop persistence under ASan/UBSan and Valgrind | Verified on repaired source | ASan/LeakSanitizer completes 100 snapshot loops and lifecycle transitions; the focused production-linked persistence suite passes Memcheck with zero errors and zero definitely lost bytes |
| Audit other periodic paths from the incident window | Verified | Output accounting, I3 state synchronization, terrain input, affect expiration, and event ownership were traced; concrete output, I3, terrain, and affect defects were repaired and tested |

### P1: Core and deployment observability

| Requirement | Disposition | Evidence |
|-------------|-------------|----------|
| Do not replace a live executable in place | Verified | Both build systems install an immutable release and atomically rotate only `bin/circle`; live legacy migration is refused |
| Retain matching binaries and symbols by build ID | Verified | Each release directory contains `circle`, `circle.debug`, and a manifest checked against build ID and SHA-256 |
| Publish active Git and ELF identity | Verified | The binary exposes `--build-info`, boot logs identity, and autorun records and compares active and installed executable, commit, build ID, and SHA-256 |
| Capture and analyze cores with the exact executable | Verified locally; production operator action | Synthetic autorun coverage selects the launched release and GDB image; `verify_core_capture.sh --self-test` reports the local WSL route as `UNVERIFIED`, so the production host must pass the same end-to-end test |

### P2: Churn, logs, and tests

| Requirement | Disposition | Evidence |
|-------------|-------------|----------|
| Stop rewriting every six seconds | Verified | Periodic pet snapshots moved to the existing 60-second save pulse; lifecycle boundaries remain immediate |
| Bound and rate-limit persistence errors | Verified | Full SQL payloads are removed; logs contain bounded operation, owner, pet VNUM, MariaDB code/detail, schema version, and suppression count |
| Add production-linked migration and rollback coverage | Verified | The 444-test suite covers migration idempotence, schema rejection, every transaction query failure, two pet rows, three linked object rows, recursive payload failure, and lifecycle transitions |

Production deployment, schema application, pet recovery, live validation, and
the real host core-capture self-test remain operator-owned and are covered by
the preceding checklist.

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
