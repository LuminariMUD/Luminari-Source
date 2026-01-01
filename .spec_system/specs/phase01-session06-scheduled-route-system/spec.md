# Session Specification

**Session ID**: `phase01-session06-scheduled-route-system`
**Phase**: 01 - Automation Layer
**Status**: Not Started
**Created**: 2025-12-30

---

## 1. Session Overview

This session implements timer-based route scheduling for the vessel automation system, enabling vessels to operate on fixed schedules like ferry services, patrol routes, and other timed operations. Building on the complete autopilot foundation from Sessions 01-05 (data structures, waypoint management, path-following, player commands, and NPC pilot integration), this session adds the final automation capability: autonomous time-triggered route execution.

The scheduled route system allows administrators to configure vessels that automatically begin their assigned routes at specific intervals measured in MUD hours. When a schedule triggers, the vessel's autopilot engages and follows the assigned route. Combined with NPC pilots from Session 05, scheduled vessels can provide immersive ferry services with departure announcements and passenger interactions without requiring player intervention.

This is the penultimate session of Phase 01, completing the automation feature set before the final testing and validation session. The schedule system represents the culmination of the autopilot subsystem, transforming vessels from manually-operated craft into fully autonomous transportation services.

---

## 2. Objectives

1. Implement schedule data structure supporting interval-based timing in MUD hours
2. Create `ship_schedules` database table with persistence across server restarts
3. Implement schedule management commands (setschedule, clearschedule, showschedule)
4. Integrate schedule timer checks with game loop heartbeat for automatic route triggering

---

## 3. Prerequisites

### Required Sessions
- [x] `phase01-session01-autopilot-data-structures` - Provides autopilot_data, waypoint, route structures
- [x] `phase01-session02-waypoint-route-management` - Provides route creation and persistence
- [x] `phase01-session03-path-following-logic` - Provides autopilot_tick() and navigation
- [x] `phase01-session04-autopilot-player-commands` - Provides command registration pattern
- [x] `phase01-session05-npc-pilot-integration` - Provides pilot announcements for departures

### Required Tools/Knowledge
- Understanding of MUD time system (SECS_PER_MUD_HOUR, time_info structure)
- Game loop heartbeat integration pattern (comm.c)
- MySQL table creation and CRUD operations
- Command registration in interpreter.c

### Environment Requirements
- MySQL/MariaDB database running and accessible
- Vessel system enabled with VESSELS_ENABLE_GREYHAWK
- Test vessel with route and NPC pilot for integration testing

---

## 4. Scope

### In Scope (MVP)
- Schedule data structure with route_id, interval (MUD hours), next_departure fields
- `ship_schedules` database table for persistence
- `setschedule <route> <interval>` command to configure scheduling
- `clearschedule` command to remove vessel's schedule
- `showschedule` command to display current schedule status
- Timer hook in heartbeat checking schedules each MUD hour tick
- Automatic route start when scheduled time arrives
- Schedule loading during boot (follows load_all_routes pattern)
- Schedule saving on modification
- Departure announcements via existing NPC pilot system

### Out of Scope (Deferred)
- Complex calendar-based scheduling (weekdays, holidays) - *Reason: MVP uses simple interval timing*
- Multiple schedules per vessel - *Reason: Single active schedule sufficient for MVP*
- Schedule conflicts/collision detection - *Reason: Not needed for basic operation*
- Dynamic schedule adjustment based on conditions - *Reason: Future enhancement*
- Schedule editing (must clear and recreate) - *Reason: Simplifies implementation*

---

## 5. Technical Approach

### Architecture
The schedule system extends the existing autopilot_data structure with schedule-related fields stored in a separate database table for clean separation. A schedule timer function runs once per MUD hour tick (checked in heartbeat), iterating through active schedules and triggering route starts when the next_departure time has passed.

```
+------------------+     +------------------+     +------------------+
| ship_schedules   | --> | autopilot_data   | --> | autopilot_start  |
| (DB persistence) |     | (schedule fields)|     | (route trigger)  |
+------------------+     +------------------+     +------------------+
        ^                        |                        |
        |                        v                        v
+------------------+     +------------------+     +------------------+
| schedule_tick()  | <-- | heartbeat()      |     | pilot_announce   |
| (timer check)    |     | (MUD hour pulse) |     | (departure msg)  |
+------------------+     +------------------+     +------------------+
```

### Design Patterns
- **Timer pattern**: Check schedules at MUD hour intervals (existing heartbeat pattern)
- **Persistence pattern**: Follow load_all_routes/save_all_routes pattern for schedules
- **Command pattern**: ACMD handlers matching existing autopilot commands

### Technology Stack
- ANSI C90/C89 (matching codebase standard)
- MySQL/MariaDB for schedule persistence
- CuTest for unit testing

---

## 6. Deliverables

### Files to Create
| File | Purpose | Est. Lines |
|------|---------|------------|
| `sql/schedule_tables.sql` | ship_schedules table definition | ~30 |
| `lib/text/help/schedule.hlp` | Help entries for schedule commands | ~50 |

### Files to Modify
| File | Changes | Est. Lines |
|------|---------|------------|
| `src/vessels.h` | Add schedule constants, struct, function prototypes | ~40 |
| `src/vessels_autopilot.c` | Add schedule functions and command handlers | ~350 |
| `src/vessels_db.c` | Add schedule persistence functions | ~120 |
| `src/interpreter.c` | Register setschedule, clearschedule, showschedule | ~10 |
| `src/comm.c` | Add schedule_tick() call in heartbeat | ~5 |
| `unittests/CuTest/test_schedule.c` | Unit tests for schedule logic | ~150 |
| `unittests/CuTest/Makefile` | Add schedule test compilation | ~5 |

---

## 7. Success Criteria

### Functional Requirements
- [ ] Schedules can be created with `setschedule <route> <interval>`
- [ ] Vessel starts route automatically when scheduled time arrives
- [ ] Schedule persists across server restarts
- [ ] Schedule status visible via `showschedule` command
- [ ] Schedules can be cleared/disabled with `clearschedule`
- [ ] Timer integration does not impact performance (< 1ms overhead)
- [ ] Scheduled vessels with pilots announce departures
- [ ] Invalid schedule parameters are rejected with clear error messages

### Testing Requirements
- [ ] Unit tests written and passing for schedule_tick() logic
- [ ] Unit tests for schedule_create/save/load functions
- [ ] Manual testing completed with ferry service scenario
- [ ] Edge cases tested (no route, route deleted, pilot missing)

### Quality Gates
- [ ] All files ASCII-encoded (0-127 characters only)
- [ ] Unix LF line endings
- [ ] Code follows project conventions (CONVENTIONS.md)
- [ ] No compiler warnings with -Wall -Wextra
- [ ] Valgrind clean (no memory leaks)

---

## 8. Implementation Notes

### Key Considerations
- Schedule timing uses MUD hours, not real-time (SECS_PER_MUD_HOUR = 75 real seconds)
- next_departure stored as mud_time value for accurate comparison
- Schedule check runs once per MUD hour tick to minimize overhead
- Vessel must be docked and have valid route for schedule to trigger
- Schedule is vessel-specific (stored via ship_id in autopilot_data)

### Potential Challenges
- **Timer precision**: MUD hour tick may drift slightly; use >= comparison not ==
- **Server restarts**: Calculate next_departure on load based on current time and interval
- **Route deletion**: If assigned route is deleted, schedule becomes invalid (handle gracefully)
- **Concurrent schedules**: If schedule triggers while vessel already traveling, ignore until complete

### Relevant Considerations
- **[Architecture]** Per-vessel type mapping still uses placeholder - schedules should work with any vessel type (not type-specific)
- **[Lesson]** Auto-create DB tables at startup - ship_schedules should follow this pattern via ensure_table_exists()
- **[Lesson]** Use #defines for constants, not hardcoded values - apply to schedule interval limits

### ASCII Reminder
All output files must use ASCII-only characters (0-127). No smart quotes, em-dashes, or special Unicode characters.

---

## 9. Testing Strategy

### Unit Tests
- schedule_create(): Valid/invalid parameters, route validation
- schedule_save(): Database persistence, field accuracy
- schedule_load(): Boot-time loading, next_departure calculation
- schedule_tick(): Trigger detection, state transitions
- schedule_clear(): Cleanup, database removal

### Integration Tests
- Full lifecycle: create schedule -> wait for trigger -> route starts -> completion -> reschedule
- Persistence: create schedule -> restart server -> verify schedule restored
- NPC pilot: schedule with pilot -> trigger -> verify departure announcement

### Manual Testing
- Create test vessel with route "FerryRoute" and NPC pilot
- `setschedule FerryRoute 2` (depart every 2 MUD hours)
- `showschedule` - verify schedule displayed
- Wait for departure, verify autopilot engages and pilot announces
- `clearschedule` - verify schedule removed
- Restart server, verify schedule (if any) persists

### Edge Cases
- Schedule with non-existent route (reject with error)
- Schedule with interval < 1 or > 24 (reject with bounds error)
- Schedule trigger while vessel already traveling (skip until complete)
- Vessel at sea when schedule time arrives (reschedule for next interval)
- Route deleted after schedule created (clear schedule, log warning)

---

## 10. Dependencies

### External Libraries
- MySQL/MariaDB: Schedule persistence (existing dependency)
- CuTest: Unit testing (existing dependency)

### Other Sessions
- **Depends on**: Sessions 01-05 (autopilot infrastructure)
- **Depended by**: Session 07 (testing/validation will exercise schedules)

---

## Next Steps

Run `/tasks` to generate the implementation task checklist.
