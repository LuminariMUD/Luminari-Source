# LuminariMUD Event Systems

This document explains the current timing infrastructure used by LuminariMUD.
The public DG event API selects either the game scheduler or the retained legacy
queue implementation at boot. On the normal scheduler path, the higher-level
MUD event layer schedules each table entry as its own native semantic type. It
supplies generation-aware owners, explicit persistence policy, and entity-scoped
payload lists and queries without exposing scheduler records.

Core source files:
- [src/event_runtime.h](../../src/event_runtime.h)
- [src/event_runtime.c](../../src/event_runtime.c)
- [src/dgscript/dg_event.h](../../src/dgscript/dg_event.h)
- [src/dgscript/dg_event.c](../../src/dgscript/dg_event.c)
- [src/game_scheduler.h](../../src/game_scheduler.h)
- [src/game_scheduler.c](../../src/game_scheduler.c)
- [src/event_handle.h](../../src/event_handle.h)
- [src/domain_events.h](../../src/domain_events.h)
- [src/domain_events.c](../../src/domain_events.c)
- [src/domain_event_types.h](../../src/domain_event_types.h)
- [src/domain_event_runtime.c](../../src/domain_event_runtime.c)
- [src/event_debug.h](../../src/event_debug.h)
- [src/event_debug.c](../../src/event_debug.c)
- [src/combat/combat_encounters.h](../../src/combat/combat_encounters.h)
- [src/combat/combat_encounters.c](../../src/combat/combat_encounters.c)
- [src/point_update_periodic.h](../../src/point_update_periodic.h)
- [src/point_update_periodic.c](../../src/point_update_periodic.c)
- [src/vessels/vessel_periodic.h](../../src/vessels/vessel_periodic.h)
- [src/vessels/vessel_periodic.c](../../src/vessels/vessel_periodic.c)
- [WORLD_PHENOMENA.md](WORLD_PHENOMENA.md)
- [src/mud_event.h](../../src/mud_event.h)
- [src/mud_event.c](../../src/mud_event.c)
- [src/mud_event_list.c](../../src/mud_event_list.c)

Key entry points (clickable declarations):
- [C.EVENTFUNC()](../../src/dgscript/dg_event.h#L28): standard signature for all event functions
- [C.event_create_named_with_cleanup()](../../src/dgscript/dg_event.c): schedule through the active backend
- [C.event_schedule_named_with_cleanup()](../../src/dgscript/dg_event.c): schedule through an opaque compatibility handle
- [C.event_schedule_owned_named_with_terminal_cleanup()](../../src/dgscript/dg_event.c): schedule owned payloads with cleanup on every terminal path
- [C.event_process()](../../src/dgscript/dg_event.c): compatibility entry point for advancing the active backend
- [C.event_cancel()](../../src/dgscript/dg_event.c): cancel a queued or in-flight event safely
- [C.event_handle_cancel()](../../src/dgscript/dg_event.c): cancel without exposing a compatibility record
- [C.event_time()](../../src/dgscript/dg_event.c): remaining pulses until an event fires
- [C.event_free_all()](../../src/dgscript/dg_event.c): bulk free all events (shutdown/reset)
- [C.attach_mud_event()](../../src/mud_event.c#L437): attach a MUD event to an entity and queue it
- [C.mud_event_detach_owner()](../../src/mud_event.c): detach an executing payload before its owner is extracted
- [C.new_mud_event()](../../src/mud_event.c#L579): allocate a MUD event payload
- [C.init_events()](../../src/mud_event.c#L66): initialize global world event list
- [C.event_countdown()](../../src/mud_event.c#L75): generic "countdown" handler with special cases
- [C.event_daily_use_cooldown()](../../src/mud_event.c#L284): unified daily-use recovery logic
- [C.change_event_duration()](../../src/mud_event.c#L1216): recreate a specific event with new duration
- [C.change_event_svariables()](../../src/mud_event.c#L1267): recreate a specific event with new sVariables
- [C.event_cancel_specific()](../../src/mud_event.c#L947): cancel a specific event by ID for a character
- [C.mud_event_index[]](../../src/mud_event_list.c#L46): registry mapping of IDs to handlers, types, and metadata

## 1. Architecture Overview

- The event system is layered:
  - Process-wide native scheduler ownership and typed scheduling API:
    [src/event_runtime.c](../../src/event_runtime.c) and
    [src/event_runtime.h](../../src/event_runtime.h)
  - Compatibility facade and boot-time backend selection:
    [src/dgscript/dg_event.c](../../src/dgscript/dg_event.c) and
    [src/dgscript/dg_event.h](../../src/dgscript/dg_event.h)
  - Default hierarchical timing-wheel backend: [src/game_scheduler.c](../../src/game_scheduler.c) and [src/game_scheduler.h](../../src/game_scheduler.h)
  - Boot-time rollback backend: the legacy ten-bucket queue retained inside `dg_event.c`
  - Higher-level MUD events with entity-scoped lists and safety/memory semantics: [src/mud_event.c](../../src/mud_event.c) and [src/mud_event.h](../../src/mud_event.h)
  - Table-driven registry: [src/mud_event_list.c](../../src/mud_event_list.c) binds event IDs to functions, types, messages, and feat metadata
- Time model:
  - Runtime time is a monotonic 100 ms tick, but scheduled mode does not wake every tick. Many helpers express real-life seconds using a macro RL_SEC which multiplies by PASSES_PER_SEC (10 ticks/sec), as discussed in [C.event_daily_use_cooldown()](../../src/mud_event.c#L284)
  - Events return the number of pulses until they should run again; returning 0 means "do not reschedule"

Scheduler deadlines drive the reactor directly, with bounded event dispatch
after descriptor input, commands, and output. Named service events own normal
cadence work. The compatibility heartbeat runs only for explicit runtime-
service rollback or to advance the legacy timed backend. Gameplay handlers
remain on the main game thread.

### 1.1 Native timed-event runtime

There is exactly one production `event_runtime` and one timing wheel. It owns
creation, shutdown, advancement, nearest-deadline inspection, semantic type
registration, scheduling, cancellation, owner cancellation, rescheduling, and
diagnostic snapshots. No gameplay module owns or stores the scheduler singleton;
the lower-level handler context retains only its dispatch-scoped scheduler
reference for callback-safe cancellation and admission contracts.

Native types register a stable diagnostic name, handler, cleanup policy,
lateness policy, owner contract, and admission limits during boot. Normal,
syntax-check, and copyover startup seal that registry after world and runtime-
service initialization. Scheduling remains available after sealing, while late
or accidental type registration returns `GAME_SCHEDULER_REGISTRATION_CLOSED`.

`struct event_runtime_handle` contains a process-local event identity that is
never reused during one runtime generation. Completed or cancelled handles
therefore cannot resolve to later events. Runtime shutdown invalidates every
remaining handle and invokes each admitted payload's cleanup exactly once.

The compatibility adapter registers one temporary `legacy_event` type in this
same runtime. Its retained four production schedules therefore coexist on the
wheel with native types during migration, but the adapter no longer creates,
owns, advances, inspects, or destroys a scheduler itself.

All 232 usable MUD event IDs register as native owner-required types. Names use
the stable form `mud.<three-digit-id>.<readable-name>`, for example
`mud.004.lay_on_hands`. The ID prevents collisions if two display names are
similar. The normal path stores a native runtime handle and dispatches the
existing table callback directly; a positive result reschedules the same event,
and a terminal result invokes one owner-detaching payload cleanup. The physical
legacy backend alone uses the localized rollback handle.

Character-affect duration and room-affect duration/behavior are the first
production owners migrated directly to native types. Their stable names are
`affected.character.duration` and `affected.room.duration`; each owner may
hold at most one event of its corresponding type.

`character.maintenance` is one nearest-deadline event per eligible character.
It dispatches only owner-local work whose established cadence is due, then
reschedules to that owner's next concrete deadline or retires. It is not one
event per sub-action and does not discover owners through a population scan.

`object.automatic_procedure` gives each eligible automatic object one
six-second deadline. `dg.random_trigger` gives each attached DG mobile, object,
or room script one thirteen-second deadline. Their callbacks resolve only the
scheduled owner; DG mobile and room execution still observes the authored
`GLOBAL` empty-zone rule.

`world.mud_hour_update` is the single world-clock boundary for global aging and
the lifecycle-maintained registries of players and objects with mud-hour work.
It does not discover owners from the general game loop. `vessel.greyhawk.agenda`
and `vessel.rol.agenda` are concrete ship-owner events;
`vessel.shared.agenda` owns only shared vessel-system work and aligned due
reasons.

`mobile.autonomous.agenda` exists only while one NPC has concrete autonomous
work. Its callback resolves that owner, executes only due reason bits, and
returns the next meaningful deadline. `activity.primary.step` advances one
character's active wall-clock activity. `combat.encounter.round` advances one
encounter and its already-indexed participants at the established semantic
round boundary. None of these callbacks scans a general population to discover
work.

Every named runtime cadence in `comm.c` is a distinct native service-owner
type. These are coarse clocks for responsibilities such as zone updates,
weather/time boundaries, usage recording, and persistence-cycle admission;
they do not discover autonomous mobile work. `service.persistence_batch` is a
separate one-owner worker that performs bounded incremental persistence and
reschedules one tick later only while that cycle remains active.

## 2. Timed-Event Compatibility Facade

### 2.1 Data Structures and Signatures

- Event function signature: [C.EVENTFUNC()](../../src/dgscript/dg_event.h#L28)
- The compatibility record is private to `dg_event.c` through
  `dg_event_internal.h`. Gameplay modules cannot declare or inspect it.
- Owned events also carry a typed `(kind, runtime_id, generation)` handle. The
  scheduler indexes this handle independently of timing-wheel location.
- Production defaults to the timing-wheel scheduler. Set `LUMINARI_EVENT_BACKEND=legacy` before boot to select the rollback queue.
- Selection reads the process environment first and then `lib/.env`, occurs only in `event_init()`, and cannot change until `event_free_all()` has emptied and destroyed the active backend.
- Unknown values log a warning and use the scheduler. Scheduler initialization failure logs the error and falls back to the legacy queue.
- `event_backend_name()` and `event_backend_current()` expose the selected backend for diagnostics and tests.

### 2.2 Opaque Handle Migration API

Owners store `event_handle_t`; the raw pointer type is not part of the public
gameplay API.
`EVENT_HANDLE_NONE` is the only empty value. `event_schedule*()` admits work
through either selected backend and returns an opaque handle; callers use
`event_handle_cancel()`, `event_handle_time()`, `event_handle_is_live()`, and
`event_handle_is_queued()` for control and inspection.

The process-local registry is a fixed, bounded array with constant-time lookup.
A handle encodes a one-based 19-bit slot and a 45-bit generation. Releasing an
event advances its generation before the slot can be reused, so stale handles
fail without dereferencing released memory. A generation never wraps: the
impractical exhaustion case retires that slot permanently. Handles are neither
serialized nor valid across `event_free_all()` or process restart.

`event_schedule*_with_cleanup()` accepts a cancellation/shutdown cleanup
callback that receives the opaque handle and owned payload rather than the
internal compatibility record. When that cleanup runs, the handle remains live
for its duration and becomes stale immediately afterward. It therefore has one
final opportunity to detach the handle from its owner and remains responsible
for releasing its payload. Normal completion remains the event callback's
payload responsibility and also invalidates the handle after it returns.

`event_schedule_owned_named_with_terminal_cleanup()` extends that contract for
payloads whose destructor must run after normal completion as well as
cancellation and shutdown. A positive callback return retains both payload and
handle for recurrence. A terminal return invokes cleanup exactly once while the
handle is still live, then invalidates the handle. The MUD-event layer uses
this form so its owner-list detachment and payload destruction do not depend on
the public compatibility record.

The old pointer API and queue declarations are private to the facade and two
low-level parity tests. New production timing code must register a semantic
type through `event_runtime` and use native handles. Compatibility scheduling
is limited to the frozen migration inventory and remains solely to preserve the
physical rollback backend until its external release gate closes.

AI response delivery and retry admission now use native `ai.response.delivery`
and `ai.request.retry` types. Named runtime services, persistence batches, DG
waits, all MUD-event IDs, and AI jobs are native on the normal scheduler path,
with localized fallback calls retained only for physical legacy rollback.
Encounter rounds, primary activities,
autonomous mobiles, character and room affected owners, nearest-deadline
character maintenance, object automatic procedures, DG random triggers,
mud-hour point updates, and vessel agendas now schedule directly through
native types.
They retain existing callback-relative recurrence and owner teardown behavior.
This infrastructure migration does not alter combat, activity, affected,
mobile, automatic-procedure, DG, or Establish Camp gameplay rules.

Greyhawk vessels, fixed RoL ships, the global vessel service, and the mud-hour
point-update service store native runtime handles. Singleton service callbacks
do not expose or own gameplay payload data.

DG script waits also store opaque handles. Their trigger-owned payload pointer
exists only for room OLC owner relocation; normal completion and cancellation
clear both fields without exposing the scheduler compatibility record.

MUD events store native runtime handles on the normal path and a separate,
localized rollback handle only on the physical legacy path. Entity lists
contain `mud_event_data` payload pointers; callers use `mud_event_is_live()`,
`mud_event_remaining()`, and `mud_event_cancel()` without knowing the selected
backend. The facade contains no MUD-specific flag or destructor branch.

### 2.3 Lifecycle (Base)

- Create/schedule: [C.event_schedule_named_with_cleanup()](../../src/dgscript/dg_event.c)
  - Ensures a minimum delay of 1 pulse
  - Converts the relative request to an absolute `live game pulse + delay`
    scheduler deadline, so admission after an idle wheel interval cannot fire
    early from a stale internal scheduler tick
  - Preserves the registered callback name for PERFMON even though all compatibility events share one internal scheduler event type
  - Returns only a generation-safe opaque handle
- Process due work: [C.event_process()](../../src/dgscript/dg_event.c)
  - Advances the timing wheel to the current monotonic game tick when the nearest deadline is due; only the physical rollback queue requires compatibility-pulse advancement
  - Marks the event explicitly as dispatching before invoking its callback
  - Calls the event function; a positive result reschedules relative to the callback pulse, while zero or a negative result completes it
  - Dispatch order is exact deadline followed by FIFO insertion order
  - Terminal-cleanup owners invoke their registered payload destructor after a terminal callback return
- Cancel: [C.event_handle_cancel()](../../src/dgscript/dg_event.c)
  - In-flight cancellation becomes cancel-pending and always wins over a positive callback return
  - In-flight payload cleanup runs after the callback returns, so the callback
    retains valid payload storage for the rest of its invocation
  - Queued cancellation detaches and cleans up synchronously
  - Queued and in-flight cancellation invoke the registered cleanup exactly once
- Query remaining pulses: [C.event_handle_time()](../../src/dgscript/dg_event.c)
- Inspect queued state: [C.event_handle_is_queued()](../../src/dgscript/dg_event.c)

### 2.4 Safety Guards (Base)

- Double-free prevention:
  - Explicit dispatch and cancel-pending flags define in-flight ownership
  - Scheduler terminal states converge on one cleanup callback
  - The rollback queue follows the same self-cancel-wins rule
- Global reentrancy guard:
  - `processing_events` rejects recursive dispatch and disallows bulk
    destruction during active processing
- Capacity protection:
  - Both backends and the opaque-handle registry enforce the 262,144-event
    global ceiling
- Passive telemetry:
  - PERFMON records callback identity and duration, scheduled/cancelled/rescheduled totals, queue depth, maximum due batch, and aggregate requested-delay buckets
  - No event payload, player text, account data, descriptor data, or other player-sensitive content is recorded

### 2.5 Immortal Diagnostics

`eventdebug` is a read-only `LVL_IMMORT` command. It uses the character's
configured screen width, defaults to 80 columns when the setting is invalid,
and clamps every rendered line to 120 columns. Long reports use the normal MUD
pager. Event payloads are never copied into the diagnostic registry or shown.

Useful views:

- `eventdebug` shows backend, live/high-water counts, owner-kind counts,
  timing-wheel occupancy, ready/overdue work, lifecycle totals, admission
  failures, stale-owner outcomes, bounded I3 and AI worker ingress, and
  domain-bus totals.
- `eventdebug queue [limit]` lists the earliest live compatibility and native
  scheduler events without duplicating compatibility-wrapper records.
- `eventdebug id <id>` selects one diagnostic event ID.
- `eventdebug type <text> [limit]` filters callback identities.
- `eventdebug owner <kind> <id> [generation]` filters typed owners.
- `eventdebug player <name> [limit]`, `eventdebug mob <name> [limit]`, and
  `eventdebug object <name> [limit]` select one visible live entity by its
  familiar in-game name.
- `eventdebug room <here|vnum> [limit]` selects the current room or a loaded
  room by vnum.
- `eventdebug scripts <player|mob|object|room> <target> [limit]` shows only
  `dg.` events owned by that entity, including native trigger waits and random
  trigger deadlines.
- Player-owned AI delivery and retry timers appear under the selected player
  as `ai.response.delivery` and `ai.request.retry`; payload text is redacted.
- `eventdebug due <max-pulses> [limit]` and
  `eventdebug range <min> <max> [limit]` filter deadlines.
- `eventdebug state <state> [limit]` filters queued, ready, running, or
  cancel-pending events.
- `eventdebug types [limit]` ranks callback profiles by cumulative time and
  includes live, call, timing, scheduling, cancellation, and recurrence data.
- `eventdebug domain [type]` reports registered domain types and handlers with
  bounded publication and callback telemetry.

Diagnostic IDs are process-local and are reset at event-system teardown. Native
entries use their scheduler IDs. Inspection is backend-neutral, so the queue
view remains available during a
boot with `LUMINARI_EVENT_BACKEND=legacy`; timing-wheel internals are naturally
reported only by the scheduler backend. Compatibility-owner teardown removes
events before owner memory is released, so it has no post-lookup stale-owner
outcome. Typed consumers introduced in later phases must count failed
generation-aware resolution with `event_note_stale_owner_outcome()` rather than
silently folding it into completion.

Entity views use the selected live runtime identity but do not require one
specific owner generation. This is intentional: independently versioned event
subsystems may assign different generation values to the same entity, and an
immortal asking about that entity should see all of them. Player, mobile, and
object lookup follows normal visibility rules; player lookup is online-only,
and room lookup requires a currently loaded room. Diagnostic payloads remain
redacted in every view.

## 3. MUD Event Layer

The MUD layer adds:
- Entity-scoped lists (character, object, descriptor, room, region, world)
- Typed, generation-aware scheduler ownership and lifecycle cancellation
- Central registry of event metadata (names, messages, feat linkage)
- Memory ownership rules for attached data (especially VNUM copies for rooms/regions)
- Utility helpers for querying, clearing, and modifying events

### 3.1 Event IDs, Types, and Registry

- Event identifiers live in an enum: [src/mud_event.h](../../src/mud_event.h) (see event_id enum)
- Type tags identify which entity owns the event: EVENT_WORLD, EVENT_DESC, EVENT_CHAR, EVENT_ROOM, EVENT_REGION, EVENT_OBJECT (see [src/mud_event.h](../../src/mud_event.h))
- Registry table: [C.mud_event_index[]](../../src/mud_event_list.c#L46)
  - Each row defines: event_name, handler func, type, completion_msg, recovery_msg, feat_num, daily_uses
  - Example rows span cooldowns, ongoing abilities, spell effects, and daily-use mechanics

### 3.2 Creating and Attaching MUD Events

- Allocate payload: [C.new_mud_event()](../../src/mud_event.c#L579)
  - Duplicates sVariables string if provided (ownership sits with the MUD event)
- Attach and schedule: [C.attach_mud_event()](../../src/mud_event.c#L437)
  - Builds the owner's typed handle and calls
    `event_schedule_owned_named_with_terminal_cleanup()` with the registry
    handler and private MUD destructor
  - Stores the returned `event_handle_t` in the MUD payload
  - Adds the `mud_event_data` pointer to the owner's event list (`ch->events`,
    `obj->events`, `room->events`, `region->events`, or `world_events`)
  - Special memory handling:
    - For EVENT_ROOM: copies the room VNUM into newly allocated memory and stores that pointer in pStruct; validates room existence; see attach switch case at [C.attach_mud_event()](../../src/mud_event.c#L437)
    - For EVENT_REGION: same pattern for region VNUM; see [C.attach_mud_event()](../../src/mud_event.c#L437)
  - Macro helper: [C.NEW_EVENT()](../../src/mud_event.h#L27) wraps allocation + attach

### 3.3 Owner Handles and Lifecycle

- Character, object, and descriptor handles use the allocated instance address
  plus a lazy process-local generation.
- Room and region handles use stable VNUM identity plus a generation that is
  invalidated on replacement or reload. World work uses one boot-generation
  singleton handle.
- Generations and scheduler IDs are never serialized through copyover.
  Reconstructed descriptor work receives a new-process handle after validation.
- The scheduler enforces global, per-owner, and per-owner/type capacity and
  exposes owner-filtered inspection, bulk cancellation, owner-kind counts, and
  owner rejection telemetry.

### 3.4 Processing and Completion (Common Handlers)

- Generic countdown handler: [C.event_countdown()](../../src/mud_event.c#L75)
  - Emits standard messages defined in registry (completion_msg) if applicable
  - Handles special cases via switch on event_id:
    - eDARKNESS (room): removes ROOM_DARK and sends message; see [C.event_countdown()](../../src/mud_event.c#L75)
    - ePURGEMOB: extracts character from world; see [C.event_countdown()](../../src/mud_event.c#L75)
    - eQUEST_COMPLETE: parses quest vnum and calls complete_quest; see [C.event_countdown()](../../src/mud_event.c#L75)
    - eENCOUNTER_REG_RESET (region): tokenizes encounter rooms, repositions them to random valid coords, then reschedules itself with "return 60 RL_SEC"; see [C.event_countdown()](../../src/mud_event.c#L75)
- Daily-use unified handler: [C.event_daily_use_cooldown()](../../src/mud_event.c#L284)
  - Reads "uses:N" from sVariables and decrements per completion
  - Computes reschedule cooldown using either:
    - Table-provided daily_uses (non-feat abilities)
    - Character feat-derived daily uses via get_daily_uses
  - Carefully handles overflow and division by zero; see math and guards at [C.event_daily_use_cooldown()](../../src/mud_event.c#L284)

### 3.5 Terminal MUD-Event Cleanup

- The scheduler invokes the private MUD cleanup after normal completion,
  queued cancellation, in-flight cancellation, or shutdown.
- Cleanup clears the payload's handle, detaches it from the owning entity list,
  frees the list when that owner type uses empty-list cleanup, releases the
  room/region VNUM copy and `sVariables`, then frees the payload.
- A positive callback return is not terminal: the same payload, handle, owner,
  and list entry remain live for the next callback-relative deadline.
- [C.mud_event_detach_owner()](../../src/mud_event.c) is only for an executing
  callback that must extract its owner before returning. It removes the list
  entry immediately but deliberately leaves payload destruction to terminal
  scheduler cleanup after the callback finishes.
- Room and region payloads retain heap-owned VNUM keys. Detachment resolves the
  key only while the owner remains present; terminal cleanup always frees the
  key even if OLC or reload already removed the owner.

### 3.6 Entity-Scoped Query Helpers

- Characters: [C.char_has_mud_event()](../../src/mud_event.c#L764)
- Rooms: [C.room_has_mud_event()](../../src/mud_event.c#L799)
- Objects: [C.obj_has_mud_event()](../../src/mud_event.c#L831)
- Regions: [C.region_has_mud_event()](../../src/mud_event.c#L863)
- World (global list): [C.world_has_mud_event()](../../src/mud_event.c#L905)

### 3.7 Clearing All Events for an Entity

Character, object, descriptor, room, and region clear helpers use one lifecycle
order: invalidate the generation, detach the payload list, mark each MUD
payload owner-detached, and cancel every event from the detached list. This
includes an event currently dispatching. Its payload remains valid until the
callback returns, while terminal cleanup cannot dereference owner memory that
may already have been released.

### 3.8 Modifying Existing Events

- Change duration: [C.change_event_duration()](../../src/mud_event.c#L1216)
  - Finds the event, duplicates its sVariables, creates a new event with new time, cancels old, attaches new
- Change sVariables: [C.change_event_svariables()](../../src/mud_event.c#L1267)
  - Captures remaining time, creates new event with new sVariables, cancels old, reattaches with preserved time
- Cancel a specific char event by ID: [C.event_cancel_specific()](../../src/mud_event.c#L947)
  - Uses the payload's opaque handle and queued-state guard

### 3.9 Persistence and Reconstruction

Every registry event has an explicit storage class. Of 232 usable event types,
93 player cooldown and recovery timers are persisted, one encounter-region reset
timer is reconstructed from database-backed world state at boot, and 138 are
transient. There are currently no copyover-only timers: the player timers use
the same durable character record for copyover and full reboot.

Persisted records use the versioned `Evn2` player-file section. They contain a
type, per-type schema, stable player ID, remaining pulses, save timestamp, and a
validated daily-use integer where required. Scheduler IDs, wheel locations,
pointers, runtime IDs, and owner generations are process-local and never go to
disk. Restore validates the durable owner and payload, rejects duplicate event
types, then admits a newly allocated event with fresh runtime identity.

Current schema 2 character timers use elapsed wall-clock policy. Restore
subtracts offline time from one-shot cooldowns and active timed states. Daily-use
records recover every charge whose deadline passed and schedule only the next
remaining deadline; they never replay a burst of recovery callbacks. If all
uses recovered, no event is admitted. Schema 1 `Evn2` records migrate through
the same elapsed policy. Expired Spellbattle records also clear their coupled
numeric marker.

Saved six-second player counters use the independent `CkAt` checkpoint. Loading
advances ordinary countdowns, staggered bonus slots, moon bonus uses, and
full-refresh racial or bloodline use pools arithmetically. It does not replay
world-dependent player maintenance, messages, damage, mission cleanup, or
automatic actions. Older player files fall back to `Last` once, then gain a
`CkAt` checkpoint on their next save.

Legacy `Evnt` records remain readable and can be written by setting
`LUMINARI_EVENT_PERSISTENCE_FORMAT=legacy` before boot. That rollback format has
no save timestamp, so it resumes its stored remaining duration and cannot
retrospectively apply offline elapsed time. A later default save upgrades it to
`Evn2` schema 2.

Transient examples include combat rounds, casting, preparation, action waits,
descriptor protocol work, DG waits, AI requests, and live room/object effects.
They are tied to a live runtime owner and are discarded when that owner or the
process ends.

## 4. What This Means In The Game

The scheduler is the game's alarm clock. It remembers when a cooldown ends,
when another combat round is due, or when some delayed effect should act. The
timing wheel is only how that alarm clock is organized internally.

The reactor is the server's front desk. It sleeps until a player connection,
server signal, or scheduler deadline needs attention, then gives player input
and output a turn before running a bounded batch of due alarms. “Network” in
this project means the socket connections between the MUD server and its
players. It does not add web requests, internet gameplay, or a new command
system.

Combat is now one player-visible use of that architecture. One event owns each
fight and wakes a shared six-second round. Due combatants act by initiative;
their standard, move, swift, and reaction resources are participant state, not
independent timer callbacks. One prevalidated queued command runs at the start
of a turn before automatic attacks. The connection loop does not poll that
queue while the encounter owns it.

The typed domain-event runtime now exists and owns one sealed main-thread
registry from boot through world teardown. Eight foundational contracts cover
character movement, damage, death, extraction, combat-state change, object
movement, door-state change, and activity transition. A ninth production
contract, `WorldPhenomenon`, carries sensory facts. Registration and handler
lists cannot change after boot; publication is synchronous and depth-first,
with hard nesting and causal-count limits.

Movement, combat-state, and extraction boundaries publish foundational facts
to maintain autonomous NPC ownership. Combat mechanics remain the existing
combat system and player commands still enter through the interpreter. The
remaining cadence groups run as named scheduler services at their established
intervals; subsystem selectors prevent old and new side-effect paths from
running together.

In game-loop terms, the scheduler is the alarm clock for when an owner should
work, while domain events are immediate facts that can wake relevant owners.
Together they will replace broad scans; the typed bus alone does not remove a
heartbeat scan.

## 5. Typed Domain-Event Foundation

- `domain_event_runtime_init()` creates the process registry during normal and
  syntax-check boot; `domain_event_runtime_shutdown()` destroys it before world
  entities are released.
- Every type has a stable numeric identity, diagnostic name, and exact payload
  size. Handlers run by ascending explicit priority and then registration
  sequence.
- Payload pointers are `const`, borrowed for the publishing call, never copied
  into diagnostics, and never retained by the bus.
- Entity payload members are `(kind, runtime_id, generation)` handles. A handler
  resolves immediately before mutation; extraction or generation reuse makes a
  later resolution return `NULL`.
- The default bounds are 16 nested publications and 1,024 publications in one
  causal chain. Reaching either limit aborts the remaining chain and records
  the rejected type.
- Bus, event-type, and handler statistics expose publication counts, calls,
  maximum depth, total and maximum handler time, slow calls, and rejected
  chains without recording payload data.
- Notification callbacks return no gameplay decision and cannot retroactively
  veto completed state. No current migration needs a decision hook; a future
  pre-operation rule must define a separate typed aggregation contract.
- `WorldPhenomenon` is the first production-published fact. It routes visual and
  audible events through coordinate or bounded room-graph propagation. Meteor
  Swarm now publishes this contract; airships, dragons, weather, terrain
  changes, nearby combat, and explosions can migrate through the same boundary.

The former database-backed `src/pubsub/` subsystem was retired in Phase 6b. It
has no boot hook, heartbeat callback, commands, or runtime readers/writers. Its
database tables remain in `sql/master_schema.sql` only to preserve existing
production data; the game ignores them. Dropping those tables requires a
separate reviewed migration and backup plan.

This removal does not itself make gameplay event-driven. The typed bus reports
facts synchronously, while the scheduler determines when due work runs. Phase 7
uses those two pieces to replace broad heartbeat scans owner by owner. Affected
character and room duration events are scheduler work and do not publish a
domain fact merely to invoke expiry behavior; see
[`AFFECTED_OWNER_EVENTS.md`](AFFECTED_OWNER_EVENTS.md).

Walk-to progress, connected PSP regeneration, bardic verses, player hints,
five-second Luminari character work, six-second damage/effect work, and
connected player maintenance also use scheduler work without synthetic domain
facts. One character-owner event waits for that owner's nearest relevant
legacy boundary, invokes only the due single-character routines, and then
selects the next boundary. Every in-world character is admitted, including
autonomous NPCs in rooms without players. Connected characters are also
admitted for connection-scoped services, and active NPC performers remain
eligible outside the world. Typed movement facts admit characters entering the
world. The registry is capped at 32,768 owners and refills released capacity
from registered active owners without scanning `character_list`. `perfmon
entities` presents the character-owner mode, registry, validation, capacity,
callbacks, and service work on nine labeled rows that remain within 80 columns.

Vessel periodic work follows the same ownership boundary. Every valid
Greyhawk fleet slot has at most one generation-aware event, aligned to the
next 0.5-second boundary and reused for the 75-second mud-hour schedule
boundary. It invokes the established per-vessel autopilot, hunter, combat,
crew, upkeep, narrative, weather, encounter, and schedule routines. A single
service-owned event runs global vessel event reconciliation, trade restocking,
MSDP refresh, and mud-hour merchant work. Converted fixed-interior RoL ships
have one 2.5-second event per loaded canonical hull, admitted and canceled by
direct object lifecycle hooks rather than an `object_list` scan. Vessel
eligibility does not depend on player presence, so unattended routes, NPC
fleets, and combat continue normally. The Greyhawk registry is capped at all
501 fleet slots and refills released capacity from registered owners.
`perfmon entities` reports the vessel mode and lifecycle on compact labeled
rows within 80 columns.

Mud-hour point work uses one aligned service event and two intrusive owner
registries. The named mud-hour service preserves weather, DG time-trigger,
global, player, object, and quest ordering before dispatching due point work.
Every PC is registered because condition, carried
object, artifact, and idle-rent behavior remains relevant. An object is
registered only while it has a positive ordinary or legacy special timer, a
timer trigger, an imbued-missile state, a decay flag, or corpse state. Setting
or clearing those properties synchronizes membership at the mutation boundary;
normal scheduled execution never searches `character_list` or `object_list`.

The global phase invokes happy-hour, activated-item, saved-damage, and staff
maintenance work once. Player and object phases invoke the established
single-owner helpers, so hunger, thirst, sobriety, idle handling, artifact
burn, cooldown recovery, timer scripts, item/portal decay, and corpse decay
retain their behavior. Current-owner extraction is iteration-safe, including
idle rent and object timer triggers. `perfmon entities` reports this subsystem
on five labeled rows bounded to the normal 80-column display.

### 5.1 Combat Encounter Scheduling

Combat defaults to one scheduled `combat_encounter_round` event per live fight.
The process-local encounter registry supplies a typed ID and generation.
Semantic mode resolves one round every six seconds in initiative, Dexterity,
and stable runtime-ID order. Each participant owns its standard, move, swift,
reaction, bounded FIFO intent, and once-per-round state. One prevalidated intent
may dispatch before automatic actions on that participant's turn.

`LUMINARI_COMBAT_ROUNDS=compatibility` retains the encounter-owned three-phase
rules, and `LUMINARI_COMBAT_EVENTS=legacy` is the deeper per-character event
rollback. These modes are exclusive for the boot and never convert live fights.

Hostility creates, extends, or merges encounters. A join or merge during a
callback is queued until the active participant iteration is safe to compact.
Participant deadlines survive a merge, and an in-dispatch join has a
six-second not-before guard. A participant leaving is inactive immediately;
the event continues for the rest of the fight and is canceled exactly once
when no active hostility remains. Movement, committed death, and extraction
facts use generation-aware resolution, with direct extraction cleanup as an
idempotent final guard.

`eventdebug` includes a compact Combat encounters block. Use
`eventdebug type combat_encounter_round` to inspect the shared event, owner ID,
generation, and due time. The summary comparison count covers both the
one-event-per-encounter invariant and phase terminal-accounting invariant.
Payloads and participant identities are not displayed.

### 5.2 Primary Activities

The primary activity manager owns at most one intentional activity per
character. Each activity stores generation-aware actor and target handles,
explicit progress ownership, capability claims, observable traits, and policy
responses for movement, damage, combat, target loss, and incompatible commands.
Terminal teardown detaches the owned timer and manager links before completion
or cancellation callbacks and before publishing the scalar
`ActivityTransitioned` fact.

Outside combat, one owned timer requests each progress step. Inside combat, a
timer cannot grant actions: an eligible activity advances only from the
character's semantic turn after consuming already available action resources.
Command-table metadata keeps information, speech, movement, and `activity`
control behavior centralized and conservative for unclassified commands.

Establish Camp is the first migrated command. It uses three two-second steps,
preserves the existing Survival result and camp benefits, cancels on movement,
delays on damage, pauses across combat, and rechecks its typed room target.
`LUMINARI_CAMP_ACTIVITY=legacy` restores immediate camp resolution for the
boot. Use `activity` for player status/control and `eventdebug` for compact
manager counters.

## 6. Table-Driven Registry (mud_event_index)

- The registry lives in [C.mud_event_index[]](../../src/mud_event_list.c#L46)
- Each entry contains:
  - Name and event handler function (EVENTFUNC)
  - Event type (which owner list to attach to)
  - Optional messages: completion_msg (one-shot countdown) and recovery_msg (daily-use)
  - Feat association: feat_num, or FEAT_UNDEFINED if non-feat
  - Non-feat daily_uses for items or abilities without feats
- Special cases use message fields for UX, but core logic sits in the handler implementations
- Addition of new rows enables features without spreading logic across multiple files

## 7. Important Safety and Memory Practices

- Never access or relink the private `struct event` record from MUD gameplay.
  Use `event_handle_cancel()` or the owner clear helpers; the facade safely
  records in-flight cancellation and prevents recurrence from reviving work.
- Never call [C.event_free_all()](../../src/dgscript/dg_event.c) while processing
  is active; it is guarded, but treat it as shutdown-only.
- For EVENT_ROOM and EVENT_REGION:
  - Always store a heap-allocated copy of VNUMs on attach (do not keep pointers to stack or external memory)
  - On free, copy the VNUM out before freeing the pStruct; then validate real_room/real_region prior to dereferencing world/region_table
- Use the entity clear helpers during extraction, descriptor close, room
  replacement, and region reload. They safely cancel queued and in-flight work.
- sVariables ownership:
  - Always strdup on creation ([C.new_mud_event()](../../src/mud_event.c#L578))
  - Terminal cleanup always frees it after the callback no longer runs

## 8. Timing Semantics and Conversions

- Pulses are internal ticks; event functions return pulses to reschedule
- Conversion helpers:
  - RL_SEC multiplies by PASSES_PER_SEC (10 ticks/sec), so "X RL_SEC" equals X seconds * 10
  - e.g., "return 60 RL_SEC;" means run again in 60 seconds * 10 ticks = 600 pulses; see [C.event_countdown()](../../src/mud_event.c#L75)
- Daily-use math safeguards in [C.event_daily_use_cooldown()](../../src/mud_event.c#L284):
  - Use long math to avoid overflow
  - Clamp to sane maximums (e.g., 1 real day)
  - Guard against division by zero and negative counts

## 7. Typical Patterns and Examples

- Attach a simple countdown to a character:
  - Use [C.NEW_EVENT()](../../src/mud_event.h#L27) with an event_id mapped to [C.event_countdown()](../../src/mud_event.c#L75) in the registry
- Start a daily-use recovery cycle:
  - Create a MUD event with sVariables "uses:N" and handler [C.event_daily_use_cooldown()](../../src/mud_event.c#L284); it will decrement and reschedule until uses exhaust
- Room-based timed effects:
  - Attach EVENT_ROOM events using the VNUM value; the attach logic will copy and validate the room
  - eDARKNESS removal happens via countdown special case; see [C.event_countdown()](../../src/mud_event.c#L75)
- Region encounter reset loop:
  - eENCOUNTER_REG_RESET repositions encounter rooms at randomized valid coordinates per region data and reschedules itself; see [C.event_countdown()](../../src/mud_event.c#L75)

## 8. How to Add a New Event

1) Choose an ID:
- Add a new enumerator to event_id in [src/mud_event.h](../../src/mud_event.h)

2) Declare handler prototype:
- Ensure there is an EVENTFUNC prototype like [C.EVENTFUNC()](../../src/mud_event.h#L252) for your function

3) Implement the handler:
- Implement long my_event_handler(void *event_obj) using [C.EVENTFUNC()](../../src/dgscript/dg_event.h#L28) semantics
- Return value:
  - > 0: number of pulses until reschedule
  - 0: do not reschedule, event completes

4) Register in the table:
- Add a row to [C.mud_event_index[]](../../src/mud_event_list.c#L46) with:
  - name, your handler function, owner type (EVENT_CHAR/ROOM/etc.), optional messages, feat association or daily_uses

5) Attach it:
- Use [C.NEW_EVENT()](../../src/mud_event.h#L27) or explicit [C.new_mud_event()](../../src/mud_event.c#L579) + [C.attach_mud_event()](../../src/mud_event.c#L437)
- For ROOM/REGION, pass a pointer to the VNUM value; attach will copy it into owned memory

6) Manage messages (optional):
- completion_msg for countdowns
- recovery_msg for daily-use recoveries

7) Validate memory and lifecycle:
- If you store additional state in `sVariables`, let MUD terminal cleanup own it
- Avoid touching owner lists directly; rely on attach, handle cancellation, and owner clear helpers

8) Test cancel and rescheduling:
- Test queued cancellation, in-flight cancellation, owner teardown, and
  generation reuse
- If your handler sometimes needs to loop, return the next delay explicitly

## 9. Utility and Query APIs (MUD Layer)

- Initialize global world event list: [C.init_events()](../../src/mud_event.c#L66)
- Query event presence:
  - Characters: [C.char_has_mud_event()](../../src/mud_event.c#L764)
  - Rooms: [C.room_has_mud_event()](../../src/mud_event.c#L799)
  - Objects: [C.obj_has_mud_event()](../../src/mud_event.c#L831)
  - Regions: [C.region_has_mud_event()](../../src/mud_event.c#L863)
  - World: [C.world_has_mud_event()](../../src/mud_event.c#L905)
- Cancel specific by ID: [C.event_cancel_specific()](../../src/mud_event.c#L947)
- Change duration: [C.change_event_duration()](../../src/mud_event.c#L1216)
- Change sVariables: [C.change_event_svariables()](../../src/mud_event.c#L1267)

## 10. Common Pitfalls and Defenses

- Double-free during execution:
  - Handled by explicit dispatch/cancel-pending state in
    [C.event_cancel()](../../src/dgscript/dg_event.c) and the backend terminal
    cleanup path
- Free-then-use in Region/Room cleanup:
  - Prevented by resolving the owned VNUM key during detachment and freeing it only in terminal cleanup
- Modifying lists during iteration:
  - Use the owner-specific clear helper; it detaches the list before cancellation
- Overflows and div-by-zero in cooldown math:
  - Guarded and clamped in [C.event_daily_use_cooldown()](../../src/mud_event.c#L284)
- Attaching to invalid rooms/regions:
  - Attach validates via real_room/real_region and cancels safely if invalid; see [C.attach_mud_event()](../../src/mud_event.c#L437)

## 11. Selected Behavior Details

- eSTRUGGLE completion message only if character is grappled; see [C.event_countdown()](../../src/mud_event.c#L75)
- ePURGEMOB extracts the character (NPC purge); see [C.event_countdown()](../../src/mud_event.c#L75)
- eCOLLECT_DELAY triggers perform_collect; see [C.event_countdown()](../../src/mud_event.c#L75)
- eSPELLBATTLE clears a flag; see [C.event_countdown()](../../src/mud_event.c#L75)
- eENCOUNTER_REG_RESET flow:
  - Tokenize VNUM list, skip occupied rooms, attempt placement with valid sector compatibility, update coords, refresh wilderness lists, and reschedule; see [C.event_countdown()](../../src/mud_event.c#L75)

## 12. Operational Notes

- Backend selection:
  - Default: `LUMINARI_EVENT_BACKEND=scheduler`
  - Rollback: `LUMINARI_EVENT_BACKEND=legacy`
  - Restart after changing the value; live backend switching is unsupported
- Runtime-service selection:
  - Default: `LUMINARI_RUNTIME_SERVICES=scheduled`
  - Rollback: `LUMINARI_RUNTIME_SERVICES=legacy`
  - Scheduled mode derives runtime ticks monotonically and admits named cadence
    events all-or-nothing. Legacy mode restores the complete 100 ms heartbeat.
    The legacy timed backend also retains a 100 ms adapter tick for queue
    advancement even when named runtime services remain selected.
- Character-periodic selection:
  - Default: `LUMINARI_CHARACTER_EVENTS=scheduled`
  - Rollback: `LUMINARI_CHARACTER_EVENTS=legacy`
  - The selection jointly controls walk-to, PSP, bardic verse, hint,
    per-character Luminari, damage/effect, and player-maintenance work;
    scheduled and rollback paths never run together.
- Affected-owner selection:
  - Default: `LUMINARI_AFFECT_EVENTS=scheduled`
  - Rollback: `LUMINARI_AFFECT_EVENTS=legacy`
  - The selection controls affected-character and affected-room duration work
    plus affected-room Luminari behavior. It is independent of the character
    selection, and each legacy wrapper runs only the half it owns.
- Vessel-periodic selection:
  - Default: `LUMINARI_VESSEL_EVENTS=scheduled`
  - Rollback: `LUMINARI_VESSEL_EVENTS=legacy`
  - The selection controls all Greyhawk owner work, global vessel service
    work, mud-hour vessel/merchant work, and fixed-RoL ship movement. Startup
    admission failure selects the legacy path for the whole subsystem;
    scheduled and rollback paths never run together.
- Point-update selection:
  - Default: `LUMINARI_POINT_UPDATE_EVENTS=scheduled`
  - Rollback: `LUMINARI_POINT_UPDATE_EVENTS=legacy`
  - The selection jointly controls global, player, and object mud-hour point
    phases. Startup service-admission failure falls back to the whole legacy
    traversal; partial scheduled mode is not allowed.
- Combat-round selection:
  - Default: `LUMINARI_COMBAT_EVENTS=encounter`
  - Rollback: `LUMINARI_COMBAT_EVENTS=legacy`
  - The selection is immutable for the boot. Encounter mode owns one event per
    fight; legacy mode owns one `eCOMBAT_ROUND` event per active character. The
    paths never execute together and active fights are not converted live.
- Encounter-round rules:
  - Default: `LUMINARI_COMBAT_ROUNDS=semantic`
  - Rollback: `LUMINARI_COMBAT_ROUNDS=compatibility`
  - Semantic mode resolves one initiative-ordered D20 round every six seconds
    with participant-owned action, reaction, intent, and once-per-round state.
    Compatibility mode retains the encounter-owned three-phase rules. This
    selection is read only when encounter combat initializes and never converts
    a live fight.
- Primary-activity selection:
  - Default: managed Establish Camp activity
  - Rollback: `LUMINARI_CAMP_ACTIVITY=legacy`
  - The selector is immutable after manager initialization. It changes only
    camp command decomposition; scheduler, reactor, domain, and combat modes
    remain authoritative.
- Startup logs one `Event backend initialized:` line naming the effective
  backend.
- `perf event total` and the PERFMON CSV representation include lifecycle,
  delay-distribution, queue-depth, due-batch, and callback-duration aggregates.
- The libevent reactor arms the nearest scheduler or queued-wait deadline.
  The select driver, legacy event queue, and full heartbeat remain boot-time
  rollback options until the Phase 11 retirement gate.

- World events:
  - Global list is created in [C.init_events()](../../src/mud_event.c#L66) and defined at [src/mud_event.c](../../src/mud_event.c#L58)
  - Query via [C.world_has_mud_event()](../../src/mud_event.c#L905)
- Descriptor events:
  - Protocol detection is a descriptor-level event ("Protocol") in the registry mapped to [C.get_protocols()](../../src/mud_event_list.c#L18)
- Iteration helpers:
  - simple_list and merge_iterator are used to safely traverse owner lists where appropriate (see various query functions)

## 13. Quick Reference: Add a Daily-Use Ability

- Registry row: handler [C.event_daily_use_cooldown()](../../src/mud_event.c#L284), type EVENT_CHAR (or EVENT_OBJECT), recovery_msg set, feat_num set (or daily_uses if non-feat)
- On use:
  - Create sVariables "uses:N" where N is the number of recoveries remaining for this "chain"
  - Attach via [C.NEW_EVENT()](../../src/mud_event.h#L27) with initial delay of ((SECS_PER_MUD_DAY / uses_per_day) RL_SEC)
- On completion:
  - The handler decrements uses and either reschedules or ends, emitting messages as configured

## 14. Logging and Diagnostics

- System errors and warnings are logged (e.g., invalid room/region, tokenize failures, cooldown clamp warnings)
- If you see warnings about cancel during execution or queue_free during processing, audit call sites for correct usage

## 15. Checklist Before Merging New Event Code

- Event ID added to enum in [src/mud_event.h](../../src/mud_event.h)
- Handler implemented with [C.EVENTFUNC()](../../src/dgscript/dg_event.h#L28)
- Row added to [C.mud_event_index[]](../../src/mud_event_list.c#L46) with correct type and messages
- Attach paths validated for entity type (and VNUM copying for room/region)
- sVariables allocation and free verified
- Lifecycle cancellation uses the appropriate owner clear helper
- Reschedule returns are in pulses or using RL_SEC as appropriate
- Negative/zero divisions guarded, long math used for multiplications
- Unit/system tests cover attach, process, reschedule, cancel, and free flows
