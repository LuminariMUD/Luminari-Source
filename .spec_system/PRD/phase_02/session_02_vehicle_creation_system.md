# Session 02: Vehicle Creation System

**Session ID**: `phase02-session02-vehicle-creation-system`
**Status**: Not Started
**Estimated Tasks**: ~15-20
**Estimated Duration**: 2-4 hours

---

## Objective

Implement the vehicle creation, initialization, and registration system including database persistence for vehicle state.

---

## Scope

### In Scope (MVP)
- Vehicle creation function (create_vehicle)
- Vehicle initialization and default values
- Vehicle registration/tracking array or list
- Database table schema for vehicles
- Auto-create tables at startup (matching vessel pattern)
- Vehicle loading from database on boot
- Vehicle saving to database

### Out of Scope
- Movement logic (Session 03)
- Player commands (Session 04)
- Vehicle-in-vehicle loading (Session 05)
- Builder/OLC integration

---

## Prerequisites

- [ ] Session 01 complete (data structures defined)
- [ ] MySQL/MariaDB database available
- [ ] Review db_init.c for table creation patterns

---

## Deliverables

1. create_vehicle() function
2. initialize_vehicle() function
3. Vehicle tracking array/list management
4. Database schema (vehicle_data table)
5. save_vehicle() and load_vehicle() functions
6. Boot-time vehicle loading integration

---

## Success Criteria

- [ ] Vehicles can be created programmatically
- [ ] Vehicle state persists across server restarts
- [ ] Database tables auto-created at startup
- [ ] No memory leaks in creation/destruction
- [ ] Vehicle tracking supports up to 1000 concurrent vehicles
- [ ] Follows existing persistence patterns from vessels_db.c
