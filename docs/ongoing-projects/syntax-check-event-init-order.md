# Syntax-Check Event Initialization Finding

Date: 2026-08-05

Status: Diagnosed; not part of the bardic instrument repair

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

## Suggested Repair and Verification

1. Initialize the event queue before `boot_world()` in syntax-check mode, or make syntax-check
   region loading validate encounter data without scheduling runtime events.
2. Add an automated `circle -c -q -d <fixture>` regression that requires a clean exit.
3. Verify both an encounter-bearing world and a minimal world, then confirm normal startup and
   shutdown still initialize and free the queue exactly once.
