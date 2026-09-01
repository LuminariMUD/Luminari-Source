# Event-Driven Core Native Mud-Hour And Vessel Agendas

**Status:** Accepted 2026-09-01
**Date:** 2026-09-01
**Branch:** `event-driven-core-refactor`
**Scope:** Native point-update and vessel owner/service timing

## Delivered Slice

Four responsibilities now schedule directly on the native runtime:

- `world.mud_hour_update` owns the single world mud-hour aging boundary;
- `vessel.greyhawk.agenda` owns one active Greyhawk ship;
- `vessel.shared.agenda` owns shared vessel-system work; and
- `vessel.rol.agenda` owns one loaded fixed-interior RoL ship.

All four types are boot-registered and owner-required. Service events use
stable service owners and no payload. Ship agendas use borrowed, generation-
aware vessel owners with opaque native handles.

## Work Model

The mud-hour event is intentionally one global world-clock event. When it is
due, it dispatches global aging and walks only lifecycle-maintained registries
of players and objects whose mud-hour responsibilities are active. It does not
scan general character or object populations on each game-loop iteration, and
per-owner synchronized mud-hour events would perform the same due work with
substantially more queued records.

Greyhawk and RoL callbacks receive one already-selected ship. The shared vessel
event owns only system-wide vessel work and explicit aligned due reasons.
Callbacks return their established delay so the timing wheel recurs the same
event; no vessel population is rediscovered by the callback.

## Preserved Behavior

- Point updates remain aligned to the mud-hour boundary and preserve global,
  player, and active-object ordering.
- Object decay remains extraction-safe while iterating the due registry.
- Greyhawk fast work remains aligned to `AUTOPILOT_TICK_INTERVAL`; schedule work
  retains the mud-hour boundary.
- Wage batching, narrative, hazard, encounter, event, trade, MSDP, and merchant
  due reasons retain their established cadence and ordering.
- Vessel feature disable/enable cancels and reconstructs service and owner
  events, and Greyhawk capacity cancellation refills from registered owners.
- Fixed-RoL ship placement and extraction schedule or cancel only that ship.

## Rollback And Copyover

`LUMINARI_POINT_UPDATE_EVENTS` and `LUMINARI_VESSEL_EVENTS` remain boot-time
rollback selectors. If the physical legacy timed backend is selected, native
types are unavailable and both requested systems automatically use their
established heartbeat paths. Copyover creates a fresh runtime, registers the
same immutable types, and reconstructs service and loaded-owner events from
normal subsystem initialization.

## Validation

- Production-linked CuTest passes 1,050/1,050.
- Focused tests verify all four semantic registrations and inspect a live
  Greyhawk event as `vessel.greyhawk.agenda`.
- Existing point-update selection, mutation, extraction, and recurrence tests
  pass, as do vessel callback, feature-toggle, generation, capacity-refill, and
  fixed-RoL lifecycle tests.
- Physical legacy-backend tests prove requested native point-update and vessel
  modes select heartbeat fallback with no native queue entries.
- The compatibility admission contract passes with nine schedules across seven
  production files; the demand-driven architecture gate remains green.
- The production binary builds successfully.
- A copied-world staff login on isolated port 4104 reports the scheduler
  backend, ten sealed native types, zero registry mismatches, and zero failed
  events. The server shuts down cleanly and releases the port.
- The preceding periodic-owner Build & Test and Security workflows were still
  running without a reported failure at the single pre-push CI check; this
  tranche proceeds on completed local acceptance rather than waiting idle.

## Next Slice

Migrate autonomous-mobile agendas, combat encounters, and primary activities
to their native semantic event types. Their concrete owner models already
exist; this removes their compatibility records without changing gameplay.
