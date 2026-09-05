# Event-Driven Core Rollback Quarantine

**Date:** 2026-09-01

**Branch:** `event-driven-core-refactor`

## Result

The normal game executable is now a native-event-only product. It compiles one
process-owned timing wheel, semantic event registrations, owner-safe handles,
named runtime services, the libevent reactor, and typed MUD/DG/AI jobs. It does
not compile the old ten-bucket queue, public compatibility scheduling facade,
100 ms heartbeat body, whole-mobile activity loop, physical rollback adapters,
or runtime selectors that could restore those paths.

Native runtime or service initialization failure aborts boot. It cannot quietly
fall back to population scans or the old queue.

## Build Contract

The normal CMake and Autotools configurations leave rollback disabled:

```sh
cmake -S . -B build-native
./configure
```

For the externally approved retention period, create a visibly separate
rollback executable:

```sh
cmake -S . -B build-rollback -DLUMINARI_ENABLE_EVENT_ROLLBACK=ON
./configure --enable-event-rollback
```

Only that executable recognizes `LUMINARI_EVENT_BACKEND=legacy`,
`LUMINARI_RUNTIME_SERVICES=legacy`, and the affected-owner,
character-periodic, point-update, vessel, and active-world rollback selectors.
Selecting the physical legacy queue forces the complete legacy heartbeat and
legacy forms of native-timer owners; mixed queue/native-service ownership is no
longer supported.
The option defaults to off in both build systems. CuTest uses the separate
`LUMINARI_EVENT_ROLLBACK_TESTS` definition so backend parity remains testable
without making rollback a production default.

## API Boundary

`dg_event.h` exposes only native lifecycle, dispatch, diagnostics, and backend
reporting. It does not export `EVENTFUNC`, `event_schedule*`, compatibility
handles, raw event records, or queue operations. The quarantined API lives in
`dg_event_rollback.h`, which rejects inclusion unless a rollback or parity-test
definition is active.

Gameplay MUD callbacks use `MUD_EVENT_CALLBACK` from
`mud_event_callback.h`. This keeps the callback ABI needed by the table-driven
MUD layer without teaching new code the old scheduler API.

## Runtime Behavior

The normal reactor derives game ticks from monotonic time, processes one native
scheduler, and sleeps to the nearest listener, signal, worker wakeup, queued
wait, or event deadline. Named service events own fixed global cadences.
Autonomous NPCs own events only while they have concrete work, and callbacks
execute due owner reasons before scheduling their next meaningful deadline.

DG waits, every usable table-driven MUD event ID, AI delivery/retry jobs,
affected owners, character maintenance, object automatic procedures, DG random
triggers, point-update work, vessels, encounters, activities, mobile agendas,
runtime services, and persistence batches all use native semantic types in the
normal build.

## Validation Contract

`scripts/events/test_legacy_event_admission.sh` preprocesses normal source and
proves that the public header exposes no facade, production adapters make no
rollback schedule calls, the game loop contains no runtime physical-backend
selector, and both build systems default rollback off.

`scripts/events/test_demand_driven_architecture.sh` proves the normal mobile
agenda does not call whole-population dispatch and that the legacy cycle is
guarded. Production artifact checks additionally reject legacy selector strings,
queue symbols, compatibility schedule symbols, and whole-mobile loop symbols.

`scripts/events/test_native_event_architecture.sh` additionally enforces one
physical timing-wheel owner, zero direct scheduler bypasses, stable semantic
registrations, sealed boot registration, entity/script diagnostics, and the
presence of linked lifecycle, dormant-population, and width regressions. All
event contracts run under both CTest and the authoritative `make test` path.

The retained rollback build and parity tests are deliberately separate
evidence. Their success proves emergency reversibility; it does not make their
APIs acceptable for new gameplay code.

## Removal

After the stable-release, rollback-independence, maintainer-approval, and PubSub
backup/restore gates close, delete the guarded queue, facade, heartbeat,
population loops, adapter branches, selectors, and rollback-only tests. No
normal caller migration should be required at that point because the ordinary
build already links without them.
