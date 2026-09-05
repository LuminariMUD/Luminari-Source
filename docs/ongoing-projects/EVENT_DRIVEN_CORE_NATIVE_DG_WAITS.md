# Event-Driven Core Native DG Waits and Entity Diagnostics

**Status:** Accepted 2026-09-01
**Date:** 2026-09-01
**Branch:** `event-driven-core-refactor`
**Scope:** Native DG wait ownership and focused immortal queue inspection

## Delivered Slice

DG script `wait` now schedules the native owner-required `dg.trigger.wait`
semantic type whenever the normal game scheduler is active. Character, object,
and room triggers use their generation-aware domain entity owner. A due callback
resumes exactly its one trigger and performs no script, entity, room, or world
discovery scan.

The existing wait grammar and pulse timing are unchanged. Normal completion,
trigger or owner extraction, OLC trigger replacement, room reindexing,
shutdown, and cancellation converge on one terminal payload cleanup. The old
physical event backend retains a localized owned `dg.trigger.wait.rollback`
adapter until that explicit rollback path is quarantined; it is not used by the
normal scheduler path.

`eventdebug` can now inspect one selected live entity:

- `eventdebug player <name> [limit]`
- `eventdebug mob <name> [limit]`
- `eventdebug object <name> [limit]`
- `eventdebug room <here|vnum> [limit]`
- `eventdebug scripts <player|mob|object|room> <target> [limit]`

Entity views match owner kind and runtime identity across subsystem generation
counters, gathering every event family for the entity. The `scripts` form adds
a `dg.` semantic-type filter. Name lookup follows ordinary immortal visibility,
rooms must be loaded, player lookup is online-only, queue limits and paging are
preserved, and event payloads remain redacted.

## Validation Contract

- Scheduler-mode waits appear as `dg.trigger.wait` under their typed owner.
- Physical rollback waits appear as `dg.trigger.wait.rollback` and still resume.
- Due waits detach before script restart, allowing a resumed script to schedule
  another wait without the completing event clearing it.
- Trigger teardown cancels the live native handle and cleans its payload once.
- Production source checks reject trigger or population scans in wait dispatch.
- Help and queue output continue to fit configured 80-column clients and the
  120-column hard ceiling.

The production binary, all 1,050 production-linked CuTests, the legacy-event
admission gate, demand-driven architecture gate, PubSub-retirement gate, and an
isolated ASan/UBSan build and full test run passed on 2026-09-01. Live-MUD
validation was intentionally deferred because the local test servers were left
down at maintainer request.

The next migration slice owns MUD-event admission, entity lists, duration and
payload mutation, diagnostics, and durable reconstruction.
