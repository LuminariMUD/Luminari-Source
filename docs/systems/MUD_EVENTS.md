# LuminariMUD Event Systems

This document explains the current timing infrastructure used by LuminariMUD.
The public DG event API selects either the game scheduler or the retained legacy
queue implementation at boot. The higher-level MUD event layer supplies
generation-aware owner handles, explicit persistence policy, and entity-scoped
compatibility lists and queries.

Core source files:
- [src/dgscript/dg_event.h](../../src/dgscript/dg_event.h)
- [src/dgscript/dg_event.c](../../src/dgscript/dg_event.c)
- [src/game_scheduler.h](../../src/game_scheduler.h)
- [src/game_scheduler.c](../../src/game_scheduler.c)
- [src/domain_events.h](../../src/domain_events.h)
- [src/domain_events.c](../../src/domain_events.c)
- [src/domain_event_types.h](../../src/domain_event_types.h)
- [src/domain_event_runtime.c](../../src/domain_event_runtime.c)
- [WORLD_PHENOMENA.md](WORLD_PHENOMENA.md)
- [src/mud_event.h](../../src/mud_event.h)
- [src/mud_event.c](../../src/mud_event.c)
- [src/mud_event_list.c](../../src/mud_event_list.c)

Key entry points (clickable declarations):
- [C.EVENTFUNC()](../../src/dgscript/dg_event.h#L28): standard signature for all event functions
- [C.event_create_named_with_cleanup()](../../src/dgscript/dg_event.c): schedule through the active backend
- [C.event_process()](../../src/dgscript/dg_event.c): advance and dispatch the active backend every pulse
- [C.event_cancel()](../../src/dgscript/dg_event.c): cancel a queued or in-flight event safely
- [C.cleanup_event_obj()](../../src/dgscript/dg_event.c): free event payloads (MUD or generic)
- [C.event_time()](../../src/dgscript/dg_event.c): remaining pulses until an event fires
- [C.event_free_all()](../../src/dgscript/dg_event.c): bulk free all events (shutdown/reset)
- [C.attach_mud_event()](../../src/mud_event.c#L437): attach a MUD event to an entity and queue it
- [C.free_mud_event()](../../src/mud_event.c#L607): remove from entity lists and free payload
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
  - Compatibility facade and backend selection: [src/dgscript/dg_event.c](../../src/dgscript/dg_event.c) and [src/dgscript/dg_event.h](../../src/dgscript/dg_event.h)
  - Default hierarchical timing-wheel backend: [src/game_scheduler.c](../../src/game_scheduler.c) and [src/game_scheduler.h](../../src/game_scheduler.h)
  - Boot-time rollback backend: the legacy ten-bucket queue retained inside `dg_event.c`
  - Higher-level MUD events with entity-scoped lists and safety/memory semantics: [src/mud_event.c](../../src/mud_event.c) and [src/mud_event.h](../../src/mud_event.h)
  - Table-driven registry: [src/mud_event_list.c](../../src/mud_event_list.c) binds event IDs to functions, types, messages, and feat metadata
- Time model:
  - The game runs on "pulses" (tick frequency). Many helpers express real-life seconds using a macro RL_SEC which multiplies by PASSES_PER_SEC (10 ticks/sec), as discussed in [C.event_daily_use_cooldown()](../../src/mud_event.c#L284)
  - Events return the number of pulses until they should run again; returning 0 means "do not reschedule"

The compatibility heartbeat still runs unmigrated pulse work. Scheduler
deadlines also drive the reactor directly, with bounded event dispatch after
descriptor input, commands, and output. Gameplay handlers remain on the main
game thread.

## 2. Timed-Event Compatibility Facade

### 2.1 Data Structures and Signatures

- Event function signature: [C.EVENTFUNC()](../../src/dgscript/dg_event.h#L28)
- Event structure fields: see [src/dgscript/dg_event.h](../../src/dgscript/dg_event.h). The record contains the callback and payload plus backend-specific identity and explicit dispatch/cancellation state.
- Owned events also carry a typed `(kind, runtime_id, generation)` handle. The
  scheduler indexes this handle independently of timing-wheel location.
- Production defaults to the timing-wheel scheduler. Set `LUMINARI_EVENT_BACKEND=legacy` before boot to select the rollback queue.
- Selection reads the process environment first and then `lib/.env`, occurs only in `event_init()`, and cannot change until `event_free_all()` has emptied and destroyed the active backend.
- Unknown values log a warning and use the scheduler. Scheduler initialization failure logs the error and falls back to the legacy queue.
- `event_backend_name()` and `event_backend_current()` expose the selected backend for diagnostics and tests.

### 2.2 Lifecycle (Base)

- Create/schedule: [C.event_create_named_with_cleanup()](../../src/dgscript/dg_event.c)
  - Ensures a minimum delay of 1 pulse
  - Preserves the registered callback name for PERFMON even though all compatibility events share one internal scheduler event type
  - Returns a heap-allocated struct event whose payload is event_obj (type-specific)
- Process every pulse: [C.event_process()](../../src/dgscript/dg_event.c)
  - Advances the timing wheel to the current game pulse, or processes the current bucket on the rollback backend
  - Marks the event explicitly as dispatching before invoking its callback
  - Calls the event function; a positive result reschedules relative to the callback pulse, while zero or a negative result completes it
  - Dispatch order is exact deadline followed by FIFO insertion order
  - For MUD events, it invokes [C.free_mud_event()](../../src/mud_event.c#L607) if event_obj still present
- Cancel: [C.event_cancel()](../../src/dgscript/dg_event.c)
  - In-flight cancellation becomes cancel-pending and always wins over a positive callback return
  - In-flight payload cleanup runs after the callback returns, so the callback
    retains valid payload storage for the rest of its invocation
  - Queued cancellation detaches and cleans up synchronously
  - For MUD events, cleanup delegates to [C.free_mud_event()](../../src/mud_event.c#L607)
- Query remaining pulses: [C.event_time()](../../src/dgscript/dg_event.c)
- Inspect queued state: [C.event_is_queued()](../../src/dgscript/dg_event.c)

### 2.3 Safety Guards (Base)

- Double-free prevention:
  - Explicit dispatch and cancel-pending flags define in-flight ownership
  - Scheduler terminal states converge on one cleanup callback
  - The rollback queue follows the same self-cancel-wins rule
- Global reentrancy guard:
  - `processing_events` rejects recursive dispatch and disallows bulk
    destruction during active processing
- Capacity protection:
  - Both backends enforce the existing 10,000-event global ceiling
- Passive telemetry:
  - PERFMON records callback identity and duration, scheduled/cancelled/rescheduled totals, queue depth, maximum due batch, and aggregate requested-delay buckets
  - No event payload, player text, account data, descriptor data, or other player-sensitive content is recorded

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
  - Builds the owner's typed handle and calls `event_create_owned_named()` with
    the registry handler
  - Adds the struct event pointer to the owner's event list (ch->events, obj->events, room->events, region->events, or world_events)
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

### 3.5 Freeing MUD Events

- Main payload cleanup: [C.free_mud_event()](../../src/mud_event.c#L607)
  - Removes the event from the owning entity list, with post-cleanup to free empty lists
  - Room/Region safety:
    - Rooms: pStruct stores a heap copy of room_vnum; copy it before free, compute room_rnum, free pStruct, and only touch world array if room still exists; see [C.free_mud_event()](../../src/mud_event.c#L607)
    - Regions: same for region_vnum; copy before free, validate against table, remove safely; see [C.free_mud_event()](../../src/mud_event.c#L607)
  - Frees sVariables if present
  - Nulls the event's event_obj to avoid accidental reuse

### 3.6 Entity-Scoped Query Helpers

- Characters: [C.char_has_mud_event()](../../src/mud_event.c#L764)
- Rooms: [C.room_has_mud_event()](../../src/mud_event.c#L799)
- Objects: [C.obj_has_mud_event()](../../src/mud_event.c#L831)
- Regions: [C.region_has_mud_event()](../../src/mud_event.c#L863)
- World (global list): [C.world_has_mud_event()](../../src/mud_event.c#L905)

### 3.7 Clearing All Events for an Entity

Character, object, descriptor, room, and region clear helpers use one lifecycle
order: invalidate the generation, detach the compatibility list, mark each MUD
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
  - Uses event_is_queued guard before calling cancel

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

Existing player-timer behavior is preserved: remaining pulses pause while the
character is offline. This is an explicit policy, rather than an accidental
consequence of the old format. Legacy `Evnt` records remain readable and can be
written by setting `LUMINARI_EVENT_PERSISTENCE_FORMAT=legacy` before boot.

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

For a player, the intended result so far is deliberately boring: commands,
round timing, cooldown messages, and copyover connections should behave as they
did. The difference is reliability underneath. Timers cannot keep a deleted
character or room alive, a burst of due timers cannot monopolize the server,
and saved cooldowns now prove they belong to the character before returning
after logout or reboot.

The typed domain-event runtime now exists and owns one sealed main-thread
registry from boot through world teardown. Eight foundational contracts cover
character movement, damage, death, extraction, combat-state change, object
movement, door-state change, and activity transition. A ninth production
contract, `WorldPhenomenon`, carries sensory facts. Registration and handler
lists cannot change after boot; publication is synchronous and depth-first,
with hard nesting and causal-count limits.

Movement, combat-state, and extraction boundaries now publish their
foundational facts to maintain autonomous NPC ownership. Combat mechanics are
still the existing combat system, player commands still enter through the
interpreter, and many heartbeat consumers still run at their old cadence. This
incremental migration avoids running an old and new side-effect path at once.
Other owning systems will add generation-aware resolvers, publishers, and
subscribers one boundary at a time.

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

Walk-to progress, connected PSP regeneration, bardic verses, and player hints
also use scheduler work without synthetic domain facts. One character-owner
event waits for that owner's nearest relevant legacy boundary, invokes the
existing single-character gameplay routine, and then selects the next
boundary. Connected characters own walk/PSP/hint service; active NPC
performers are admitted independently. Dormant NPCs own no character-periodic
event. The registry is capped at 32,768 owners and refills released capacity
from registered active owners without scanning `character_list`.
`perfmon entities` presents the character-owner mode, registry, validation,
capacity, callbacks, and service work on seven labeled rows that remain within
80 columns.

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

- Never free or relink `struct event` directly during execution. Use
  [C.event_cancel()](../../src/dgscript/dg_event.c); the facade safely records
  in-flight cancellation and prevents recurrence from reviving the event.
- Never call [C.event_free_all()](../../src/dgscript/dg_event.c) while processing
  is active; it is guarded, but treat it as shutdown-only.
- For EVENT_ROOM and EVENT_REGION:
  - Always store a heap-allocated copy of VNUMs on attach (do not keep pointers to stack or external memory)
  - On free, copy the VNUM out before freeing the pStruct; then validate real_room/real_region prior to dereferencing world/region_table
- Use the entity clear helpers during extraction, descriptor close, room
  replacement, and region reload. They safely cancel queued and in-flight work.
- sVariables ownership:
  - Always strdup on creation ([C.new_mud_event()](../../src/mud_event.c#L578))
  - Always free on payload free ([C.free_mud_event()](../../src/mud_event.c#L607))

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
- If you store additional state in sVariables, strdup on creation and free on completion
- Avoid touching owner lists directly; rely on attach/free helpers

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
  - Fixed by copying VNUM before freeing and validating indices; see [C.free_mud_event()](../../src/mud_event.c#L607) and [C.free_mud_event()](../../src/mud_event.c#L607)
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
- Character-periodic selection:
  - Default: `LUMINARI_CHARACTER_EVENTS=scheduled`
  - Rollback: `LUMINARI_CHARACTER_EVENTS=legacy`
  - The selection jointly controls walk-to, PSP, bardic verse, and hint work;
    scheduled and heartbeat paths never run together.
- Startup logs one `Event backend initialized:` line naming the effective
  backend.
- `perf event total` and the PERFMON CSV representation include lifecycle,
  delay-distribution, queue-depth, due-batch, and callback-duration aggregates.
- The libevent reactor arms the scheduler's nearest deadline while the ten-Hz
  compatibility heartbeat remains for unmigrated pulse consumers. The select
  driver and legacy event queue remain boot-time rollback options until the
  Phase 11 retirement gate.

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
