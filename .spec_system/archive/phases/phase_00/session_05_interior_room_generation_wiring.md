# Session 05: Interior Room Generation Wiring

**Session ID**: `phase00-session05-interior-room-generation-wiring`
**Status**: Not Started
**Estimated Tasks**: ~18-22
**Estimated Duration**: 3-4 hours

---

## Objective

Wire the existing generate_ship_interior() function to ship creation/loading, and optionally connect it to the database room templates instead of hard-coded values.

---

## Scope

### In Scope (MVP)
- Identify ship creation/loading entry points
- Call generate_ship_interior() at appropriate times
- Connect room generation to vessel type
- Verify interior rooms created correctly
- Test room VNUM allocation (30000-40019 range)
- Ensure rooms persist across vessel operations

### Stretch Goals
- Replace hard-coded room templates with DB lookup
- Query ship_room_templates table for room definitions

### Out of Scope
- Interior movement (Session 06)
- Room persistence to database (Session 07)
- New room template types

---

## Prerequisites

- [ ] Session 01-03 completed
- [ ] Session 04 completed (commands registered)
- [ ] Understanding of vessels_rooms.c
- [ ] DB tables created (ship_room_templates)

---

## Deliverables

1. Interior rooms generated when ship is created/loaded
2. Rooms use correct VNUM range
3. Room types match vessel configuration
4. ship_rooms command shows generated rooms

---

## Success Criteria

- [ ] generate_ship_interior() called on ship creation
- [ ] generate_ship_interior() called on ship load (if persisted)
- [ ] Interior rooms created in VNUM range 30000-40019
- [ ] Room count matches vessel type expectations
- [ ] ship_rooms command lists all interior rooms
- [ ] No VNUM conflicts with builder zones
- [ ] Build compiles cleanly

---

## Technical Notes

### Interior VNUM Formula (from PRD)

```
Formula: 30000 + (ship->shipnum * MAX_SHIP_ROOMS) + room_index
Range:   30000 - 40019 (reserved, do not use for builder zones)
```

### Room Templates

Currently 10 hard-coded templates in vessels_rooms.c:27-104.
Database has 19 templates in ship_room_templates table:

- Control: bridge, helm
- Quarters: quarters_captain, quarters_crew, quarters_officer
- Cargo: cargo_main, cargo_secure
- Engineering: engineering, weapons, armory
- Common: mess_hall, galley, infirmary
- Connectivity: corridor, deck_main, deck_lower
- Special: airlock, observation, brig

### Key Functions

- generate_ship_interior() in vessels_rooms.c
- Ship creation point (needs identification)
- Ship loading point (needs identification)

### Files to Modify

- src/vessels.c or src/vessels_rooms.c - Wire generation calls
- Possibly src/db.c - If ship loading happens there
- src/vessels_rooms.c - Optionally add DB template lookup
