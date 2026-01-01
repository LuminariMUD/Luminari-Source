# Session Specification

**Session ID**: `phase01-session05-npc-pilot-integration`
**Phase**: 01 - Automation Layer
**Status**: Not Started
**Created**: 2025-12-30

---

## 1. Session Overview

This session implements NPC pilot integration for the vessel autopilot system. Building on the complete autopilot foundation from sessions 01-04, this session enables NPC characters (mobs) to serve as vessel pilots, operating vessels autonomously along assigned routes with appropriate behavior and announcements.

NPC pilots are essential for creating immersive ferry services, patrol routes, and transport NPCs that players can interact with. When an NPC pilot is assigned to a vessel, the autopilot system operates automatically without requiring player commands - the pilot "runs" the ship. This creates a foundation for scheduled ferry services and dynamic world content.

The implementation leverages the existing `pilot_mob_vnum` field in `autopilot_data` (added in Session 01) and the `ship_crew_roster` database table for persistence. The session adds player commands to assign/unassign pilots and integrates pilot behavior into the autopilot tick processing.

---

## 2. Objectives

1. Implement `assignpilot` and `unassignpilot` commands with proper validation
2. Add pilot behavior functions for waypoint arrival announcements
3. Integrate pilot control with autopilot tick processing (pilot-controlled vessels auto-navigate)
4. Persist pilot assignments via ship_crew_roster database table

---

## 3. Prerequisites

### Required Sessions
- [x] `phase01-session01-autopilot-data-structures` - provides autopilot_data with pilot_mob_vnum field
- [x] `phase01-session02-waypoint-route-management` - provides route/waypoint management
- [x] `phase01-session03-path-following-logic` - provides path-following and autopilot tick
- [x] `phase01-session04-autopilot-player-commands` - provides autopilot command patterns

### Required Tools/Knowledge
- Understanding of NPC (mob) data structures (`struct char_data`, `IS_NPC()`)
- Existing `ship_crew_roster` table schema and functions in vessels_db.c
- Command registration pattern in interpreter.c

### Environment Requirements
- MySQL/MariaDB database running with ship_crew_roster table
- Working autopilot system from sessions 01-04

---

## 4. Scope

### In Scope (MVP)
- `assignpilot <npc>` command to assign an NPC in the helm room as vessel pilot
- `unassignpilot` command to remove current pilot assignment
- Pilot validation (must be NPC, must be in helm room, vessel must not already have pilot)
- Pilot remains with vessel during travel (prevent leaving helm room)
- Automatic autopilot engagement when pilot is assigned with route
- Pilot announcement messages on waypoint arrivals
- Database persistence for pilot assignments (survives reboot)
- Pilot loading on ship load from database

### Out of Scope (Deferred)
- Complex NPC AI (combat, evasion, danger response) - *Reason: Future enhancement*
- NPC crew beyond pilot role (gunners, engineers) - *Reason: Separate crew system*
- Pilot training/skill requirements - *Reason: Adds complexity without MVP value*
- Dynamic NPC hiring/wages - *Reason: Economy system dependency*
- Pilot personality/varied announcements - *Reason: Polish for later*

---

## 5. Technical Approach

### Architecture
The NPC pilot system extends the existing autopilot infrastructure:
1. `autopilot_data.pilot_mob_vnum` stores the assigned pilot's VNUM (-1 if none)
2. Runtime pilot pointer tracked via mob lookup when needed
3. `ship_crew_roster` table stores persistent pilot assignments with role='pilot'
4. Autopilot tick checks for pilot presence and auto-engages when route is set

### Design Patterns
- **Existing Command Pattern**: Follow `do_autopilot`, `do_setwaypoint` patterns for new commands
- **Database Persistence Pattern**: Use existing `vessel_db_save_crew_member()` for pilot storage
- **Mob Lookup Pattern**: Use `get_char_room()` and VNUM matching for pilot validation

### Technology Stack
- ANSI C90/C89 (project standard)
- MySQL/MariaDB via existing mysql.c functions
- CuTest for unit testing

---

## 6. Deliverables

### Files to Create
| File | Purpose | Est. Lines |
|------|---------|------------|
| `lib/text/help/assignpilot.hlp` | Help file for assignpilot command | ~20 |
| `lib/text/help/unassignpilot.hlp` | Help file for unassignpilot command | ~15 |
| `unittests/CuTest/test_npc_pilot.c` | Unit tests for pilot logic | ~150 |

### Files to Modify
| File | Changes | Est. Lines |
|------|---------|------------|
| `src/vessels_autopilot.c` | Add do_assignpilot, do_unassignpilot, pilot behavior | ~300 |
| `src/vessels.h` | Add pilot function prototypes, PILOT_ROLE constant | ~20 |
| `src/interpreter.c` | Register assignpilot, unassignpilot commands | ~5 |
| `src/vessels_db.c` | Add pilot load/save functions | ~100 |
| `unittests/CuTest/Makefile` | Add test_npc_pilot.c to build | ~5 |

---

## 7. Success Criteria

### Functional Requirements
- [ ] NPCs can be assigned as vessel pilots via `assignpilot <npc>`
- [ ] Pilot assignment persists across server reboots
- [ ] Piloted vessels with routes operate autopilot automatically
- [ ] Pilot announces waypoint arrivals to vessel occupants
- [ ] Pilot can be unassigned via `unassignpilot` command
- [ ] Invalid NPC assignments rejected with clear error message
- [ ] Pilot cannot leave helm room while assigned
- [ ] Only one pilot per vessel allowed

### Testing Requirements
- [ ] Unit tests for pilot assignment validation logic
- [ ] Unit tests for pilot persistence save/load
- [ ] Manual testing: assign pilot, set route, verify auto-navigation
- [ ] Manual testing: reboot server, verify pilot restored

### Quality Gates
- [ ] All files ASCII-encoded (0-127 characters only)
- [ ] Unix LF line endings
- [ ] Code follows ANSI C90 conventions (no // comments, no C99 features)
- [ ] All compiler warnings resolved
- [ ] Valgrind clean (no memory leaks)

---

## 8. Implementation Notes

### Key Considerations
- Use `GET_MOB_VNUM()` for storing pilot identity (survives reextraction)
- Pilot validation must check `IS_NPC()` - only NPCs can be assigned as pilots
- Captain permission check required for assign/unassign (existing `check_vessel_captain()`)
- Pilot announcements use `send_to_ship()` for all vessel occupants

### Potential Challenges
- **NPC movement restriction**: Must prevent pilot from leaving helm room via movement commands
  - Mitigation: Add pilot check in movement validation or use SENTINEL flag
- **Pilot extraction on reboot**: VNUM-based lookup required since char_data pointers invalid
  - Mitigation: Load pilot from ship_crew_roster on ship initialization
- **Race condition on pilot assignment**: Two captains assigning different pilots
  - Mitigation: Atomic check-and-set with error message if already assigned

### Relevant Considerations
- [P00] **Auto-create DB tables at startup**: Pilot data uses existing ship_crew_roster table with role='pilot'
- [P00] **Don't use C99/C11 features**: All new code must be ANSI C90 compliant
- [P00] **Don't skip NULL checks**: Critical for NPC pointer validation

### ASCII Reminder
All output files must use ASCII-only characters (0-127).

---

## 9. Testing Strategy

### Unit Tests
- Test pilot assignment validation (NPC check, helm room check, single pilot check)
- Test pilot VNUM storage and retrieval
- Test pilot persistence save/load cycle

### Integration Tests
- Test pilot assignment triggers autopilot engagement with existing route
- Test pilot announcements during waypoint arrival (mock send_to_ship)

### Manual Testing
- Assign NPC pilot to vessel, verify assignment message
- Set route while pilot assigned, verify auto-navigation starts
- Arrive at waypoint, verify pilot announcement
- Reboot server, verify pilot restored and route continues
- Test invalid cases: non-NPC, NPC not in room, already has pilot

### Edge Cases
- Assign pilot to vessel with no route (should assign but not navigate)
- Remove pilot during active navigation (should stop autopilot)
- Kill assigned pilot NPC (should clear assignment)
- Extract pilot mob (should handle gracefully on reload)

---

## 10. Dependencies

### External Libraries
- MySQL/MariaDB client library (existing dependency)

### Other Sessions
- **Depends on**: phase01-session01 through phase01-session04 (autopilot foundation)
- **Depended by**: phase01-session06 (scheduled routes - pilots announce departures/arrivals)

---

## Next Steps

Run `/tasks` to generate the implementation task checklist.
