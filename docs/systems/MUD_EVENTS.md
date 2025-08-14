# LuminariMUD Event Systems

This document explains the complete timing and event infrastructure used by LuminariMUD, covering the base discrete-event queue (“DG event system”) and the higher-level, entity-scoped “MUD event” layer built on top of it.

Core source files:
- [src/dg_event.h](src/dg_event.h)
- [src/dg_event.c](src/dg_event.c)
- [src/mud_event.h](src/mud_event.h)
- [src/mud_event.c](src/mud_event.c)
- [src/mud_event_list.c](src/mud_event_list.c)

Key entry points (clickable declarations):
- [C.EVENTFUNC()](src/dg_event.h:28): standard signature for all event functions
- [C.event_create()](src/dg_event.c:61): schedule an event
- [C.event_process()](src/dg_event.c:249): run due events every pulse
- [C.event_cancel()](src/dg_event.c:133): cancel a queued or in-flight event safely
- [C.cleanup_event_obj()](src/dg_event.c:217): free event payloads (MUD or generic)
- [C.event_time()](src/dg_event.c:357): remaining pulses until an event fires
- [C.event_free_all()](src/dg_event.c:374): bulk free of all events (shutdown/reset)
- [C.attach_mud_event()](src/mud_event.c:437): attach a MUD event to an entity and queue it
- [C.free_mud_event()](src/mud_event.c:607): remove from entity lists and free payload
- [C.new_mud_event()](src/mud_event.c:579): allocate a MUD event payload
- [C.init_events()](src/mud_event.c:66): initialize global world event list
- [C.event_countdown()](src/mud_event.c:75): generic "countdown" handler with special cases
- [C.event_daily_use_cooldown()](src/mud_event.c:284): unified daily-use recovery logic
- [C.change_event_duration()](src/mud_event.c:1216): recreate a specific event with new duration
- [C.change_event_svariables()](src/mud_event.c:1267): recreate a specific event with new sVariables
- [C.event_cancel_specific()](src/mud_event.c:947): cancel a specific event by ID for a character
- [C.mud_event_index[]](src/mud_event_list.c:46): registry mapping of IDs to handlers, types, and metadata

## 1. Architecture Overview

- The event system is layered:
  - Base queue + processing: [src/dg_event.c](src/dg_event.c) and [src/dg_event.h](src/dg_event.h)
  - Higher-level MUD events with entity-scoped lists and safety/memory semantics: [src/mud_event.c](src/mud_event.c) and [src/mud_event.h](src/mud_event.h)
  - Table-driven registry: [src/mud_event_list.c](src/mud_event_list.c) binds event IDs to functions, types, messages, and feat metadata
- Time model:
  - The game runs on “pulses” (tick frequency). Many helpers express real-life seconds using a macro RL_SEC which multiplies by PASSES_PER_SEC (10 ticks/sec), as discussed in [C.event_daily_use_cooldown()](src/mud_event.c:284)
  - Events return the number of pulses until they should run again; returning 0 means “do not reschedule”

## 2. Base Queue (DG Event System)

### 2.1 Data Structures and Signatures

- Event function signature: [C.EVENTFUNC()](src/dg_event.h:28)
- Event structure fields: see [src/dg_event.h](src/dg_event.h) (struct event contains func, event_obj, q_el, isMudEvent)
- Priority queue (multi-bucket) internals: [src/dg_event.h](src/dg_event.h) and [src/dg_event.c](src/dg_event.c)
  - Buckets reduce enqueue costs by distributing events based on (key % NUM_EVENT_QUEUES)

### 2.2 Lifecycle (Base)

- Create/schedule: [C.event_create()](src/dg_event.c:61)
  - Ensures a minimum delay of 1 pulse
  - Returns a heap-allocated struct event whose payload is event_obj (type-specific)
- Process every pulse: [C.event_process()](src/dg_event.c:249)
  - Dequeues due events by current pulse
  - Sets event->q_el = NULL to mark "currently processing"
  - Calls the event's function; if it returns > 0, re-enqueues with that delay; otherwise frees
  - For MUD events, it invokes [C.free_mud_event()](src/mud_event.c:607) if event_obj still present
- Cancel: [C.event_cancel()](src/dg_event.c:133)
  - If q_el is NULL, the event is being processed; it does not free the event structure (prevents double-free)
  - For MUD events, calls [C.cleanup_event_obj()](src/dg_event.c:217) which delegates to [C.free_mud_event()](src/mud_event.c:607)
- Query remaining pulses: [C.event_time()](src/dg_event.c:357)

### 2.3 Safety Guards (Base)

- Double-free prevention:
  - In cancel: detection via q_el == NULL; see comments at [C.event_cancel()](src/dg_event.c:133)
  - In process: only free mud_event payload if not already nulled by cancel; see [C.event_process()](src/dg_event.c:249)
- Global reentrancy guard:
  - Flag processing_events used to disallow bulk frees during active processing; see [C.event_process()](src/dg_event.c:249), [C.event_free_all()](src/dg_event.c:374), and queue_free function

## 3. MUD Event Layer

The MUD layer adds:
- Entity-scoped lists (character, object, room, region, world)
- Central registry of event metadata (names, messages, feat linkage)
- Memory ownership rules for attached data (especially VNUM copies for rooms/regions)
- Utility helpers for querying, clearing, and modifying events

### 3.1 Event IDs, Types, and Registry

- Event identifiers live in an enum: [src/mud_event.h](src/mud_event.h) (see event_id enum)
- Type tags identify which entity owns the event: EVENT_WORLD, EVENT_DESC, EVENT_CHAR, EVENT_ROOM, EVENT_REGION, EVENT_OBJECT (see [src/mud_event.h](src/mud_event.h))
- Registry table: [C.mud_event_index[]](src/mud_event_list.c:46)
  - Each row defines: event_name, handler func, type, completion_msg, recovery_msg, feat_num, daily_uses
  - Example rows span cooldowns, ongoing abilities, spell effects, and daily-use mechanics

### 3.2 Creating and Attaching MUD Events

- Allocate payload: [C.new_mud_event()](src/mud_event.c:579)
  - Duplicates sVariables string if provided (ownership sits with the MUD event)
- Attach and schedule: [C.attach_mud_event()](src/mud_event.c:437)
  - Internally calls [C.event_create()](src/dg_event.c:61) with handler from registry
  - Adds the struct event pointer to the owner’s event list (ch->events, obj->events, room->events, region->events, or world_events)
  - Special memory handling:
    - For EVENT_ROOM: copies the room VNUM into newly allocated memory and stores that pointer in pStruct; validates room existence; see attach switch case at [C.attach_mud_event()](src/mud_event.c:437)
    - For EVENT_REGION: same pattern for region VNUM; see [C.attach_mud_event()](src/mud_event.c:437)
  - Macro helper: [C.NEW_EVENT()](src/mud_event.h:27) wraps allocation + attach

### 3.3 Processing and Completion (Common Handlers)

- Generic countdown handler: [C.event_countdown()](src/mud_event.c:75)
  - Emits standard messages defined in registry (completion_msg) if applicable
  - Handles special cases via switch on event_id:
    - eDARKNESS (room): removes ROOM_DARK and sends message; see [C.event_countdown()](src/mud_event.c:75)
    - ePURGEMOB: extracts character from world; see [C.event_countdown()](src/mud_event.c:75)
    - eQUEST_COMPLETE: parses quest vnum and calls complete_quest; see [C.event_countdown()](src/mud_event.c:75)
    - eENCOUNTER_REG_RESET (region): tokenizes encounter rooms, repositions them to random valid coords, then reschedules itself with “return 60 RL_SEC”; see [C.event_countdown()](src/mud_event.c:75)
- Daily-use unified handler: [C.event_daily_use_cooldown()](src/mud_event.c:284)
  - Reads “uses:N” from sVariables and decrements per completion
  - Computes reschedule cooldown using either:
    - Table-provided daily_uses (non-feat abilities)
    - Character feat-derived daily uses via get_daily_uses
  - Carefully handles overflow and division by zero; see math and guards at [C.event_daily_use_cooldown()](src/mud_event.c:284)

### 3.4 Freeing MUD Events

- Main payload cleanup: [C.free_mud_event()](src/mud_event.c:607)
  - Removes the event from the owning entity list, with post-cleanup to free empty lists
  - Room/Region safety:
    - Rooms: pStruct stores a heap copy of room_vnum; copy it before free, compute room_rnum, free pStruct, and only touch world array if room still exists; see [C.free_mud_event()](src/mud_event.c:607)
    - Regions: same for region_vnum; copy before free, validate against table, remove safely; see [C.free_mud_event()](src/mud_event.c:607)
  - Frees sVariables if present
  - Nulls the event’s event_obj to avoid accidental reuse

### 3.5 Entity-Scoped Query Helpers

- Characters: [C.char_has_mud_event()](src/mud_event.c:764)
- Rooms: [C.room_has_mud_event()](src/mud_event.c:799)
- Objects: [C.obj_has_mud_event()](src/mud_event.c:831)
- Regions: [C.region_has_mud_event()](src/mud_event.c:863)
- World (global list): [C.world_has_mud_event()](src/mud_event.c:905)

### 3.6 Clearing All Events for an Entity

Robust patterns are used to avoid iterator invalidation and double-free:

- Characters: two-pass staging approach; see [C.clear_char_event_list()](src/mud_event.c:998)
  - Pass 1 copies only queued events into a temporary list (events currently executing are not queued)
  - Pass 2 cancels each staged event
- Rooms: same two-pass technique using simple_list iterator; see [C.clear_room_event_list()](src/mud_event.c:1072)
- Regions: process-first-until-empty pattern to avoid holding stale pointers; see [C.clear_region_event_list()](src/mud_event.c:1120)

### 3.7 Modifying Existing Events

- Change duration: [C.change_event_duration()](src/mud_event.c:1216)
  - Finds the event, duplicates its sVariables, creates a new event with new time, cancels old, attaches new
- Change sVariables: [C.change_event_svariables()](src/mud_event.c:1267)
  - Captures remaining time, creates new event with new sVariables, cancels old, reattaches with preserved time
- Cancel a specific char event by ID: [C.event_cancel_specific()](src/mud_event.c:947)
  - Uses event_is_queued guard before calling cancel

## 4. Table-Driven Registry (mud_event_index)

- The registry lives in [C.mud_event_index[]](src/mud_event_list.c:46)
- Each entry contains:
  - Name and event handler function (EVENTFUNC)
  - Event type (which owner list to attach to)
  - Optional messages: completion_msg (one-shot countdown) and recovery_msg (daily-use)
  - Feat association: feat_num, or FEAT_UNDEFINED if non-feat
  - Non-feat daily_uses for items or abilities without feats
- Special cases use message fields for UX, but core logic sits in the handler implementations
- Addition of new rows enables features without spreading logic across multiple files

## 5. Important Safety and Memory Practices

- Never free or modify the struct event directly during execution:
  - [C.event_process()](src/dg_event.c:249) marks in-flight events via q_el = NULL
  - [C.event_cancel()](src/dg_event.c:133) detects in-flight events and avoids double-free
- Never call [C.event_free_all()](src/dg_event.c:374) while processing is active; it is guarded, but treat it as shutdown-only
- For EVENT_ROOM and EVENT_REGION:
  - Always store a heap-allocated copy of VNUMs on attach (do not keep pointers to stack or external memory)
  - On free, copy the VNUM out before freeing the pStruct; then validate real_room/real_region prior to dereferencing world/region_table
- Only cancel queued events:
  - Guard with [C.event_is_queued()](src/dg_event.h:110) before calling [C.event_cancel()](src/dg_event.c:133)
- sVariables ownership:
  - Always strdup on creation ([C.new_mud_event()](src/mud_event.c:578))
  - Always free on payload free ([C.free_mud_event()](src/mud_event.c:607))

## 6. Timing Semantics and Conversions

- Pulses are internal ticks; event functions return pulses to reschedule
- Conversion helpers:
  - RL_SEC multiplies by PASSES_PER_SEC (10 ticks/sec), so “X RL_SEC” equals X seconds × 10
  - e.g., “return 60 RL_SEC;” means run again in 60 seconds × 10 ticks = 600 pulses; see [C.event_countdown()](src/mud_event.c:75)
- Daily-use math safeguards in [C.event_daily_use_cooldown()](src/mud_event.c:284):
  - Use long math to avoid overflow
  - Clamp to sane maximums (e.g., 1 real day)
  - Guard against division by zero and negative counts

## 7. Typical Patterns and Examples

- Attach a simple countdown to a character:
  - Use [C.NEW_EVENT()](src/mud_event.h:27) with an event_id mapped to [C.event_countdown()](src/mud_event.c:75) in the registry
- Start a daily-use recovery cycle:
  - Create a MUD event with sVariables “uses:N” and handler [C.event_daily_use_cooldown()](src/mud_event.c:284); it will decrement and reschedule until uses exhaust
- Room-based timed effects:
  - Attach EVENT_ROOM events using the VNUM value; the attach logic will copy and validate the room
  - eDARKNESS removal happens via countdown special case; see [C.event_countdown()](src/mud_event.c:75)
- Region encounter reset loop:
  - eENCOUNTER_REG_RESET repositions encounter rooms at randomized valid coordinates per region data and reschedules itself; see [C.event_countdown()](src/mud_event.c:75)

## 8. How to Add a New Event

1) Choose an ID:
- Add a new enumerator to event_id in [src/mud_event.h](src/mud_event.h)

2) Declare handler prototype:
- Ensure there is an EVENTFUNC prototype like [C.EVENTFUNC()](src/mud_event.h:252) for your function

3) Implement the handler:
- Implement long my_event_handler(void *event_obj) using [C.EVENTFUNC()](src/dg_event.h:28) semantics
- Return value:
  - > 0: number of pulses until reschedule
  - 0: do not reschedule, event completes

4) Register in the table:
- Add a row to [C.mud_event_index[]](src/mud_event_list.c:46) with:
  - name, your handler function, owner type (EVENT_CHAR/ROOM/etc.), optional messages, feat association or daily_uses

5) Attach it:
- Use [C.NEW_EVENT()](src/mud_event.h:27) or explicit [C.new_mud_event()](src/mud_event.c:579) + [C.attach_mud_event()](src/mud_event.c:437)
- For ROOM/REGION, pass a pointer to the VNUM value; attach will copy it into owned memory

6) Manage messages (optional):
- completion_msg for countdowns
- recovery_msg for daily-use recoveries

7) Validate memory and lifecycle:
- If you store additional state in sVariables, strdup on creation and free on completion
- Avoid touching owner lists directly; rely on attach/free helpers

8) Test cancel and rescheduling:
- Ensure [C.event_is_queued()](src/dg_event.h:110) guards before cancel calls
- If your handler sometimes needs to loop, return the next delay explicitly

## 9. Utility and Query APIs (MUD Layer)

- Initialize global world event list: [C.init_events()](src/mud_event.c:66)
- Query event presence:
  - Characters: [C.char_has_mud_event()](src/mud_event.c:764)
  - Rooms: [C.room_has_mud_event()](src/mud_event.c:799)
  - Objects: [C.obj_has_mud_event()](src/mud_event.c:831)
  - Regions: [C.region_has_mud_event()](src/mud_event.c:863)
  - World: [C.world_has_mud_event()](src/mud_event.c:905)
- Cancel specific by ID: [C.event_cancel_specific()](src/mud_event.c:947)
- Change duration: [C.change_event_duration()](src/mud_event.c:1216)
- Change sVariables: [C.change_event_svariables()](src/mud_event.c:1267)

## 10. Common Pitfalls and Defenses

- Double-free during execution:
  - Handled by q_el==NULL guard in [C.event_cancel()](src/dg_event.c:133) and checks in [C.event_process()](src/dg_event.c:249)
- Free-then-use in Region/Room cleanup:
  - Fixed by copying VNUM before freeing and validating indices; see [C.free_mud_event()](src/mud_event.c:607) and [C.free_mud_event()](src/mud_event.c:607)
- Modifying lists during iteration:
  - Use two-pass staging for character/room clears: [C.clear_char_event_list()](src/mud_event.c:998), [C.clear_room_event_list()](src/mud_event.c:1072)
  - Use process-first-until-empty for regions: [C.clear_region_event_list()](src/mud_event.c:1120)
- Overflows and div-by-zero in cooldown math:
  - Guarded and clamped in [C.event_daily_use_cooldown()](src/mud_event.c:284)
- Attaching to invalid rooms/regions:
  - Attach validates via real_room/real_region and cancels safely if invalid; see [C.attach_mud_event()](src/mud_event.c:437)

## 11. Selected Behavior Details

- eSTRUGGLE completion message only if character is grappled; see [C.event_countdown()](src/mud_event.c:75)
- ePURGEMOB extracts the character (NPC purge); see [C.event_countdown()](src/mud_event.c:75)
- eCOLLECT_DELAY triggers perform_collect; see [C.event_countdown()](src/mud_event.c:75)
- eSPELLBATTLE clears a flag; see [C.event_countdown()](src/mud_event.c:75)
- eENCOUNTER_REG_RESET flow:
  - Tokenize VNUM list, skip occupied rooms, attempt placement with valid sector compatibility, update coords, refresh wilderness lists, and reschedule; see [C.event_countdown()](src/mud_event.c:75)

## 12. Operational Notes

- World events:
  - Global list is created in [C.init_events()](src/mud_event.c:66) and defined at [src/mud_event.c](src/mud_event.c:58)
  - Query via [C.world_has_mud_event()](src/mud_event.c:905)
- Descriptor events:
  - Protocol detection is a descriptor-level event (“Protocol”) in the registry mapped to [C.get_protocols()](src/mud_event_list.c:18)
- Iteration helpers:
  - simple_list and merge_iterator are used to safely traverse owner lists where appropriate (see various query functions)

## 13. Quick Reference: Add a Daily-Use Ability

- Registry row: handler [C.event_daily_use_cooldown()](src/mud_event.c:284), type EVENT_CHAR (or EVENT_OBJECT), recovery_msg set, feat_num set (or daily_uses if non-feat)
- On use:
  - Create sVariables "uses:N" where N is the number of recoveries remaining for this “chain”
  - Attach via [C.NEW_EVENT()](src/mud_event.h:27) with initial delay of ((SECS_PER_MUD_DAY / uses_per_day) RL_SEC)
- On completion:
  - The handler decrements uses and either reschedules or ends, emitting messages as configured

## 14. Logging and Diagnostics

- System errors and warnings are logged (e.g., invalid room/region, tokenize failures, cooldown clamp warnings)
- If you see warnings about cancel during execution or queue_free during processing, audit call sites for correct usage

## 15. Checklist Before Merging New Event Code

- Event ID added to enum in [src/mud_event.h](src/mud_event.h)
- Handler implemented with [C.EVENTFUNC()](src/dg_event.h:28)
- Row added to [C.mud_event_index[]](src/mud_event_list.c:46) with correct type and messages
- Attach paths validated for entity type (and VNUM copying for room/region)
- sVariables allocation and free verified
- Cancel paths guarded with [C.event_is_queued()](src/dg_event.h:110)
- Reschedule returns are in pulses or using RL_SEC as appropriate
- Negative/zero divisions guarded, long math used for multiplications
- Unit/system tests cover attach, process, reschedule, cancel, and free flows