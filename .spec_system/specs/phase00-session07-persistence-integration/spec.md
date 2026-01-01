# Session Specification

**Session ID**: `phase00-session07-persistence-integration`
**Phase**: 00 - Core Vessel System
**Status**: Not Started
**Created**: 2025-12-29

---

## 1. Session Overview

This session wires the persistence layer to save and load vessel state across server restarts. The database functions (save_ship_interior, load_ship_interior, etc.) already exist in vessels_db.c, but they are not currently called from appropriate save and load points in the codebase.

The goal is to integrate these persistence functions into the server lifecycle: calling save functions when ships are created, modified, docked/undocked, and at shutdown; calling load functions during the boot sequence. This ensures that vessel configurations, interior layouts, and docking records survive server restarts.

This is a critical integration session that connects the existing database layer with the runtime vessel system. It follows the pattern established by other LuminariMUD systems (houses, clans) that persist to MySQL.

---

## 2. Objectives

1. Wire save_ship_interior() calls to all appropriate save points (creation, modification, shutdown)
2. Wire load_ship_interior() calls to boot sequence for restoring vessel state on startup
3. Wire docking record persistence (save on dock, end on undock)
4. Verify save/load cycle with no data loss through manual testing

---

## 3. Prerequisites

### Required Sessions
- [x] `phase00-session01-header-cleanup-foundation` - Clean header structure
- [x] `phase00-session02-dynamic-wilderness-room-allocation` - Room allocation
- [x] `phase00-session03-vessel-type-system` - Vessel types defined
- [x] `phase00-session04-phase2-command-registration` - Commands registered
- [x] `phase00-session05-interior-room-generation-wiring` - Interior generation works
- [x] `phase00-session06-interior-movement-implementation` - Interior movement works

### Required Tools/Knowledge
- Understanding of vessels_db.c save/load functions
- Understanding of db.c boot_world() sequence
- Understanding of greyhawk_ships[] global array

### Environment Requirements
- MySQL/MariaDB running and accessible
- Database tables exist (auto-created at startup via init_vessel_system_tables)

---

## 4. Scope

### In Scope (MVP)
- Add save_ship_interior() call on ship interior generation
- Add load_ship_interiors() boot-time function to restore all saved ships
- Wire load_ship_interiors() into db.c boot_world() after room loading
- Add save_ship_interior() to auto-save trigger points
- Add save_all_vessels() function for shutdown
- Wire save_all_vessels() to shutdown sequence
- Wire docking record save/end calls to dock/undock commands
- Basic verification testing

### Out of Scope (Deferred)
- Cargo manifest persistence - *Reason: Stretch goal, requires object persistence*
- Crew roster persistence - *Reason: Stretch goal, requires NPC persistence*
- Performance optimization - *Reason: Premature optimization*
- Data migration utilities - *Reason: No existing data to migrate*

---

## 5. Technical Approach

### Architecture
The persistence layer follows a simple pattern:
1. **Load at boot**: After rooms are loaded in boot_world(), iterate saved ships from DB and restore state
2. **Save on change**: After interior generation or modification, call save_ship_interior()
3. **Save at shutdown**: Before server terminates, save all active vessel states

### Design Patterns
- **Iterator pattern**: Loop through greyhawk_ships[] array for bulk operations
- **Observer pattern**: Save triggers on state changes (implicit, not event-driven)
- **Null check guards**: All functions validate mysql_available and pointers

### Technology Stack
- MySQL/MariaDB via LuminariMUD mysql.c wrapper
- REPLACE INTO for upsert semantics
- Comma-separated VNUM strings for room list serialization

---

## 6. Deliverables

### Files to Create
| File | Purpose | Est. Lines |
|------|---------|------------|
| (none) | All functions already exist in vessels_db.c | - |

### Files to Modify
| File | Changes | Est. Lines |
|------|---------|------------|
| `src/vessels_db.c` | Add load_all_ship_interiors(), save_all_vessels() | ~50 |
| `src/vessels.h` | Declare new functions | ~5 |
| `src/db.c` | Wire load_all_ship_interiors() to boot_world() | ~5 |
| `src/vessels.c` | Add save call after interior generation | ~10 |
| `src/vessels.c` | Wire docking save/end to dock/undock | ~10 |
| `src/comm.c` (or genolc.c) | Wire save_all_vessels() to shutdown | ~5 |

---

## 7. Success Criteria

### Functional Requirements
- [ ] Ships saved to ship_interiors table after interior generation
- [ ] Ships restored from ship_interiors table on server boot
- [ ] Docking records saved when ships dock
- [ ] Docking records marked completed when ships undock
- [ ] Server restart preserves all ship interior configurations

### Testing Requirements
- [ ] Manual test: create ship, restart server, verify ship loads
- [ ] Manual test: dock ships, restart, verify docking state
- [ ] Log messages confirm save/load operations

### Quality Gates
- [ ] All files ASCII-encoded (0-127 characters only)
- [ ] Unix LF line endings
- [ ] Code follows project conventions (C90, 2-space indent, Allman braces)
- [ ] NULL checks on all pointer dereferences
- [ ] mysql_available checked before all DB operations

---

## 8. Implementation Notes

### Key Considerations
- The greyhawk_ships[] array is statically allocated with GREYHAWK_MAXSHIPS slots
- Ships may not have valid data in all slots; must check for valid ship before saving
- Ship ID (ship->id) is the primary key for ship_interiors table
- REPLACE INTO handles both insert and update cases

### Potential Challenges
- **Identifying valid ships**: Need to determine which array slots have real ships (check id or state)
- **Boot sequence timing**: load_all_ship_interiors() must run after rooms are loaded but before players connect
- **Partial data**: If load fails mid-way, some ships may not restore; need error handling

### Relevant Considerations
- **[P00] MySQL/MariaDB required**: All save/load functions already guard with mysql_available check
- **[P00] Auto-create DB tables**: Schema matches runtime code via init_vessel_system_tables()
- **[P00] Don't use C99 features**: Use /* */ comments, declare variables at block start

### ASCII Reminder
All output files must use ASCII-only characters (0-127).

---

## 9. Testing Strategy

### Unit Tests
- Not feasible for DB integration without mocking; rely on manual testing

### Integration Tests
- Create ship via in-game command
- Verify log message "Saved interior configuration for ship X"
- Stop server
- Start server
- Verify log message "Loaded interior configuration for ship X"
- Check ship state matches pre-restart state

### Manual Testing
1. Boot server, note greyhawk_ships array state
2. Create/load a test ship
3. Query ship_interiors table directly: `SELECT * FROM ship_interiors;`
4. Restart server
5. Query again, verify data persists
6. Verify in-game ship state

### Edge Cases
- Empty database (no ships to load)
- Corrupted ship_id in database
- Missing room VNUMs after room number changes
- Server crash (data since last save lost - acceptable)

---

## 10. Dependencies

### External Libraries
- MySQL/MariaDB client library (already linked)

### Other Sessions
- **Depends on**: Sessions 01-06 (all core vessel features)
- **Depended by**: Session 08 (External View and Display Systems), Session 09 (Testing and Validation)

---

## Next Steps

Run `/tasks` to generate the implementation task checklist.
