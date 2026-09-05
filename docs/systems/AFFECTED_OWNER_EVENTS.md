# Affected Owner Events

Phase 7 moves character-affect and room-affect duration expiry from one
six-second cadence sweep to owner-scoped scheduler deadlines. The affected-room
event also owns its five-second room-affect behavior boundary. The eligibility
registries remain available for boot reconstruction and staff validation, but
normal scheduled operation visits only owners whose deadline is due.

## Gameplay Behavior

Every live affected character owns one duration event on the global
`PULSE_VIOLENCE` boundary. Every room containing one or more room affects owns
one event shared by all of those affects. The room event selects the nearer of
the established five-second `PULSE_LUMINARI` behavior boundary and six-second
duration boundary. Initial deadlines are not spread because both behaviors
are authored against shared world cadence and must retain their boundary
ordering.

The callback uses the established expiry functions. Positive character
durations decrement, negative durations remain unlimited, and a zero-duration
component expires with the existing wear-off message, Phantom Heal adjustment,
Death Pact position update, bit removal, and total recalculation. Room timers
decrement together; expiry removes the room affect and clears its room bit only
when no same-kind affect remains.

This changes how due owners are found, not spell duration or combat behavior.
Room-affect behavior such as fog or blade-barrier ticks now runs from the room
owner event. Removing the final room affect during either behavior or duration
work cancels that event before it can recur.

## MSDP Boundary

MSDP is connected-client work, not an affect-duration owner. Affect addition
and removal already update MSDP immediately through `affect_total()`, and the
one-second connected-descriptor service continues to refresh `AFFECTS`. The old
six-second sweep's duplicate refresh of both affected and unaffected players
was removed with the duration scan.

## Lifecycle And Bounds

Character attach, affect mutation, extraction, and free boundaries schedule or
cancel directly. Room-affect creation links the node into its room owner;
manual or timed removal unlinks it. Cancellation during the owner's own
callback wins over recurrence. Each room-affect node records membership so a
duplicate add or remove cannot corrupt counts or links.

Room OLC treats these links and deadlines as live runtime state. Saving an
existing room preserves its current affects rather than restoring the editor's
older snapshot. Inserting or deleting a room first cancels affected-room
deadlines, moves the world array, adjusts every surviving room-affect index,
rebuilds ownership, and schedules the next shared round boundary. Deleting an
affected room removes its affects before the room disappears. A newly inserted
room never inherits runtime links or generation identity from an editor copy.

Scheduled mode admits 32,768 affected characters and 16,384 affected rooms.
When either subsystem limit is full, additional eligible owners remain in the
bounded eligibility registry and admission is rejected with a rate-limited
warning. Releasing capacity immediately schedules waiting owners, so an affect
cannot remain frozen after pressure subsides.

The shared compatibility event ceiling is 262,144. Current high-cardinality
owner limits total 196,608, leaving at least 65,536 slots for combat, waits,
activities, and services when every owner subsystem reaches its own limit.

## Diagnostics And Rollback

`perfmon entities` reports scheduled or legacy mode, eligible and scheduled
character and room counts, validation mismatches, limits, rejections,
callbacks, duration work, and room-behavior work. The affected section uses
short labeled lines intended for an ordinary 80-column client. `perfmon reset`
clears work counters without changing owners or deadlines. Callback telemetry
is recorded under `affected_character` and `affected_room`.

Selection is immutable for one boot:

- `LUMINARI_AFFECT_EVENTS=scheduled` is the default; and
- `LUMINARI_AFFECT_EVENTS=legacy` restores the six-second `affect_update()`
  compatibility sweep and the room half of the five-second Luminari pulse.

The `active`, `event`, `heartbeat`, and `off` aliases follow the other Phase 7
owner gates. Scheduled and legacy duration paths cannot execute together. A
change requires a restart.
