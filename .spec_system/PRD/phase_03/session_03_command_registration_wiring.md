# Session 03: Command Registration & Wiring

**Session ID**: `phase03-session03-command-registration-wiring`
**Status**: Not Started
**Estimated Tasks**: ~15-18
**Estimated Duration**: 2-3 hours

---

## Objective

Register all Phase 2 vessel commands in interpreter.c and wire up interior generation to be called at appropriate runtime points.

---

## Scope

### In Scope (MVP)

- Register `dock` command in interpreter.c
- Register `undock` command in interpreter.c
- Register `board_hostile` command in interpreter.c
- Register `look_outside` command in interpreter.c
- Register `ship_rooms` command in interpreter.c
- Wire generate_ship_interior() to ship creation/loading
- Wire persistence calls (save/load interiors)
- Connect look_outside to get_weather() and wilderness view
- Verify all command handlers are properly linked
- Test each registered command

### Out of Scope

- New command implementation
- Tactical display (future enhancement)
- Contact detection system (future enhancement)
- Ship combat commands

---

## Prerequisites

- [ ] Session 02 complete (interior movement working)
- [ ] do_dock, do_undock handlers exist in vessels_docking.c
- [ ] generate_ship_interior() implemented in vessels_rooms.c

---

## Deliverables

1. All 5 Phase 2 commands registered in interpreter.c
2. Interior generation wired to ship creation
3. Persistence calls wired to runtime
4. look_outside connected to weather/wilderness systems
5. Unit tests for command registration
6. Manual testing verification

---

## Success Criteria

- [ ] All Phase 2 commands recognized by parser
- [ ] `dock` command creates gangway connections
- [ ] `undock` command removes docking connections
- [ ] `board_hostile` initiates forced boarding
- [ ] `look_outside` shows exterior weather and terrain
- [ ] `ship_rooms` lists interior rooms
- [ ] Interiors auto-generated when ships are created
- [ ] No compilation warnings
