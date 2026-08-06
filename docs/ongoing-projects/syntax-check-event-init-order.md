# Syntax-Check Event Initialization Finding

Date: 2026-08-05

Status: Complete

Scope: The installed server's `-c` syntax-check boot path.

## Finding

Running the newly installed binary as:

```text
./bin/circle -c -q -d lib
```

loads the database, zones, triggers, rooms, and regions, then logs:

```text
SYSERR: event_create called before event_init()
```

while creating a region encounter reset event. It then exits with status 139.

This is unrelated to bardic instruments. No bardic source is on the failing path, and the
warning-free production-linked suite passes 410/410 tests.

## Cause

Normal startup enters `init_game()` in `src/comm.c`, which calls `event_init()` before
`boot_db()`. Syntax-check mode instead calls `boot_world()` directly from `main()` without first
initializing the event queue. Region loading schedules encounter reset events through
`event_create()`, whose null-queue guard reports the initialization error.

`destroy_db()` already calls `event_free_all()`, so the syntax-check path has matching cleanup if
it initializes the queue before world boot.

After the event queue was initialized, the clean-exit regression exposed a second partial-boot
cleanup defect: `destroy_db()` unconditionally dereferenced `global_craft_list`, but syntax-check
mode calls `boot_world()` rather than `boot_db()` and therefore never creates that list. Cleanup
must tolerate this intentionally uninitialized subsystem.

The same partial boot creates an encounter event list without the optional `global_lists` debug
registry. `free_list()` must therefore skip registry removal when that registry was never created,
rather than logging a false null-list programming error during otherwise valid cleanup.

## Suggested Repair and Verification

1. Initialize the event queue before `boot_world()` in syntax-check mode, or make syntax-check
   region loading validate encounter data without scheduling runtime events.
2. Add an automated `circle -c -q -d <fixture>` regression that requires a clean exit.
3. Verify both an encounter-bearing world and a minimal world, then confirm normal startup and
   shutdown still initialize and free the queue exactly once.

## Implementation Progress

- [x] Re-traced the syntax-check and normal startup paths against current `src/comm.c`.
- [x] Confirmed `destroy_db()` owns event-queue cleanup through `event_free_all()`.
- [x] Initialize the event queue exactly once before syntax-check world loading.
- [x] Add a production-linked regression that runs the real syntax-check entry point in a child
      process and requires a clean exit without the pre-initialization diagnostic.
- [x] Cover encounter-bearing and minimal event-queue lifecycles.
- [x] Make partial-world cleanup tolerate the craft subsystem remaining uninitialized.
- [x] Run the warning-free production-linked suite, install the binary, and verify the installed
      syntax-check command.

## Verification Log

### 2026-08-06 - Implementation checkpoint

- `make -j$(nproc) cutest`: PASS with `-Wall -Wextra`; no compiler warnings.
- `LUMINARI_TEST_ROOT="$PWD" ./cutest`: PASS, 436/436 tests.
- The production-linked child regression observed an encounter reset event, `Done.`, exit status
  zero, one `event_init()` call, and one `event_free_all()` call.
- The regression confirmed that neither the pre-initialization diagnostic nor the partial-list
  cleanup diagnostic was emitted.

### 2026-08-06 - Final verification

- `make test`: PASS, including auxiliary script checks and 436/436 production-linked tests.
- `make install`: PASS; installed `bin/circle` and removed the root build artifact.
- `./bin/circle -c -q -d lib`: PASS against the encounter-bearing development world; observed
  encounter event creation, world cleanup, `Done.`, and exit status zero. Neither repaired
  diagnostic was emitted.
- `./bin/circle -q -d lib 54321`, followed by SIGTERM after `Entering game loop.`: PASS; observed
  graceful signal handling, normal termination, world cleanup, `Done.`, and exit status zero.
- Source-path review confirms normal startup calls `event_init()` once in `init_game()` and common
  shutdown calls `event_free_all()` once through `destroy_db()`. The child regression separately
  asserts one initialization and one cleanup for syntax-check mode.

## Resolution

Syntax-check mode now initializes the event queue before any world loader can schedule an event.
The common database teardown tolerates subsystems that the partial syntax-check boot intentionally
does not initialize, and list teardown tolerates the absent optional debug registry. The regression
suite covers both an empty queue and the production-linked encounter-bearing syntax-check path.
