# Session 07: Persistence Integration

**Session ID**: `phase00-session07-persistence-integration`
**Status**: Not Started
**Estimated Tasks**: ~18-22
**Estimated Duration**: 3-4 hours

---

## Objective

Wire the persistence layer to save and load vessel state, including ship interiors, docking records, cargo manifests, and crew rosters.

---

## Scope

### In Scope (MVP)
- Wire save_ship_interior() to appropriate save points
- Wire load_ship_interior() to boot/load sequence
- Wire docking record persistence (ship_docking table)
- Test save/load cycle for ship state
- Test persistence across server restarts
- Handle data migration for existing ships (if any)

### Stretch Goals
- Wire cargo manifest persistence (ship_cargo_manifest table)
- Wire crew roster persistence (ship_crew_roster table)

### Out of Scope
- Adding new persistence features
- Performance optimization
- Data compression

---

## Prerequisites

- [ ] Sessions 01-06 completed
- [ ] Database tables exist (auto-created at startup)
- [ ] Understanding of vessels_db.c
- [ ] Understanding of ship_interiors, ship_docking tables

---

## Deliverables

1. Ship interiors persist to database
2. Ship interiors restored on server restart
3. Docking records persist correctly
4. No data loss on normal shutdown

---

## Success Criteria

- [ ] Ship interiors saved to ship_interiors table
- [ ] Ship interiors loaded on boot
- [ ] Active docking records persist in ship_docking
- [ ] Completed dockings have proper status
- [ ] Foreign key relationships maintained
- [ ] Server restart preserves all ship state
- [ ] No orphaned records
- [ ] CASCADE DELETE works correctly

---

## Technical Notes

### Database Tables (from PRD Section 6)

| Table | Primary Key | Purpose |
|-------|-------------|---------|
| ship_interiors | ship_id VARCHAR(8) | Vessel configuration, room data |
| ship_docking | dock_id INT AUTO_INCREMENT | Active/historical docking records |
| ship_cargo_manifest | manifest_id INT AUTO_INCREMENT | Cargo tracking (FK to ship_interiors) |
| ship_crew_roster | crew_id INT AUTO_INCREMENT | NPC crew assignments (FK to ship_interiors) |

### Persistence Functions

Expected in vessels_db.c:
- save_ship_interior(ship)
- load_ship_interior(ship_id)
- save_docking_record(dock)
- load_active_dockings()

### Save Points

Persistence should trigger:
- On ship creation
- On interior modification
- On docking/undocking
- On server shutdown
- Periodically (auto-save)

### Load Points

Persistence should load:
- At boot time
- When ship enters world
- On demand (if lazy loading)

### Foreign Key Consideration

From PRD Troubleshooting:
> ship_interiors record must exist first (parent table) before inserting cargo/crew records.

### Files to Modify

- src/vessels_db.c - Ensure save/load functions exist
- src/vessels.c - Wire save calls
- src/db.c - Wire load calls to boot sequence
