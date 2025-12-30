# Session 06: Scheduled Route System

**Session ID**: `phase01-session06-scheduled-route-system`
**Status**: Not Started
**Estimated Tasks**: ~15-18
**Estimated Duration**: 2-4 hours

---

## Objective

Implement timer-based route scheduling that allows vessels to operate on fixed schedules, enabling ferry services, patrol routes, and other timed operations.

---

## Scope

### In Scope (MVP)
- Schedule data structure (start times, intervals)
- Database table for route schedules
- `setschedule <route> <interval>` command
- `clearschedule` command
- `showschedule` command
- Timer integration with game loop
- Automatic route start at scheduled times
- Schedule persistence across reboots
- Basic schedule status display

### Out of Scope
- Complex calendar-based scheduling
- Multiple schedules per vessel
- Schedule conflicts/collision detection
- Dynamic schedule adjustment based on conditions

---

## Prerequisites

- [ ] Sessions 01-05 complete
- [ ] Understanding of game timer/event system
- [ ] Understanding of mud time vs real time

---

## Deliverables

1. Schedule data structure in vessels.h
2. `ship_schedules` database table
3. Schedule management functions
4. Timer hook for schedule checks
5. Command handlers for schedule management
6. Commands registered in interpreter.c
7. Schedule persistence (save/load)
8. Help file entries

---

## Success Criteria

- [ ] Schedules can be created with interval timing
- [ ] Vessel starts route automatically at scheduled time
- [ ] Schedule persists across server restarts
- [ ] Schedule status visible via command
- [ ] Schedules can be cleared/disabled
- [ ] Timer integration does not impact performance
- [ ] Scheduled vessels with pilots announce departures
- [ ] Unit tests verify schedule timing logic
