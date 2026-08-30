# Periodic Owner Events

Phase 7 moves automatic object procedures and DG random triggers from cadence
registry sweeps to owner-scoped scheduler deadlines. Eligibility registries
remain the authoritative inventory, but normal scheduled operation does not
walk them every six or thirteen seconds.

## Gameplay Behavior

An object with `ITEM_AUTOPROC` owns one event distributed across the existing
six-second `PULSE_MOBILE` interval. Its callback invokes the same special
procedure gateway as `proc_update()`. Unfinished weapons whose first value is
zero remain inert. Worn, carried, contained, room, and otherwise instantiated
objects are not gated by player proximity.

A mobile, object, or room script containing a DG random trigger owns one event
distributed across the existing thirteen-second `PULSE_DG_SCRIPT` interval.
The callback invokes the same random-trigger function and percentage check as
the legacy path.

DG locality remains authored behavior:

- object random triggers run without requiring a nearby player;
- mobile and room random triggers marked `GLOBAL` run in empty zones; and
- non-global mobile and room random triggers retain the existing empty-zone
  suppression rule.

This is different from autonomous NPC thinking, where player presence is never
an eligibility rule. A builder who needs a distant scripted war, weather
controller, or other room/mobile process to advance in an empty zone must use
the existing DG `GLOBAL` trigger flag.

The Avernus room pulse is not an automatic object procedure. It remains an
explicit six-second heartbeat service in both modes.

## Lifecycle

Eligibility changes schedule or cancel the owner directly:

- object load and `ITEM_AUTOPROC` flag synchronization;
- DG script owner binding and trigger attach/detach;
- object, mobile, room-script, and trigger extraction; and
- live object prototype replacement through OLC.

Each object, character, and room has a process-local periodic-owner generation.
The scheduler handle combines owner kind, runtime identity, and generation, so
reused memory cannot identify an old incarnation. An owner has at most one
automatic-procedure event and one aggregated DG random event. If a callback
extracts its own object or script, in-flight cancellation wins over the
callback's recurrence and the borrowed pointer is never cleaned up twice.

Initial deadlines are spread across their established interval. Recurrence is
still six or thirteen seconds, but eligible owners no longer all become due in
one heartbeat burst.

## Bounds And Diagnostics

Scheduled automatic procedures admit at most 16,384 object owners. DG random
triggers admit at most 32,768 owners combined across mobiles, objects, and
rooms. Alongside the 65,536 autonomous-NPC limit, these high-cardinality
consumers reserve at least 16,384 slots beneath the 131,072-event compatibility
queue ceiling for combat, waits, activities, and service work. The queue's
global limit remains the final admission boundary.

Admission above a subsystem limit fails closed and emits a rate-limited
warning. `perfmon entities` reports mode, eligible registry membership,
scheduled membership, validation mismatches, limits, rejections, callbacks,
and DG executions by owner type. `perfmon reset` clears those counters without
changing membership or deadlines. Event callback telemetry uses
`periodic_autoproc` and `periodic_dg_random` identities.

## Rollback

Selection is immutable for one boot and independent by subsystem:

- `LUMINARI_AUTOPROC_EVENTS=scheduled` is the default;
- `LUMINARI_AUTOPROC_EVENTS=legacy` restores `proc_update()`;
- `LUMINARI_DG_RANDOM_EVENTS=scheduled` is the default; and
- `LUMINARI_DG_RANDOM_EVENTS=legacy` restores `script_trigger_check()`.

`active`, `event`, `legacy`, `heartbeat`, and `off` aliases follow the same
selection convention as the active-world manager. The heartbeat checks each
selection at its old cadence, and scheduled and legacy execution cannot run
together. A change requires a restart.

These are timed scheduler events, while typed domain events remain synchronous
facts about things that happened. The two concepts share native ownership and
lifecycle boundaries, but a periodic deadline does not publish a fact merely
to call its established gameplay function.
