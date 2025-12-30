# Session Specification

**Session ID**: `phase02-session02-vehicle-creation-system`
**Phase**: 02 - Simple Vehicle Support
**Status**: Not Started
**Created**: 2025-12-30

---

## 1. Session Overview

This session implements the vehicle creation, initialization, and persistence system for the Simple Vehicle Support tier. Building directly on the data structures defined in Session 01 (vehicle_data struct, enums, and constants in vessels.h), this session brings vehicles to life by implementing all lifecycle functions, state management, capacity tracking, condition handling, and database persistence.

The vehicle system is designed to support up to 1000 concurrent vehicles with minimal memory footprint (136 bytes per vehicle). Unlike the complex vessel system, vehicles are lightweight land-based transport (carts, wagons, mounts, carriages) that operate using standard room-to-room movement. This session establishes the foundation that all subsequent Phase 02 sessions depend on - movement (Session 03), player commands (Session 04), and vehicle-in-vehicle mechanics (Session 05) all require functional vehicle creation and persistence.

The implementation follows proven patterns from vessels_db.c, including auto-creating database tables at startup, in-memory tracking arrays for fast lookups, and comprehensive NULL checks throughout all paths.

---

## 2. Objectives

1. Implement vehicle lifecycle functions (create, destroy, init) with complete memory management
2. Create global vehicle tracking array supporting up to 1000 concurrent vehicles
3. Implement database schema (vehicle_data table) with auto-create at startup
4. Implement save/load persistence functions following vessels_db.c patterns
5. Integrate boot-time vehicle loading into server startup sequence

---

## 3. Prerequisites

### Required Sessions
- [x] `phase02-session01-vehicle-data-structures` - Provides vehicle_data struct, enums, constants

### Required Tools/Knowledge
- ANSI C90/C89 programming (no C99/C11 features)
- MySQL/MariaDB query patterns (vessels_db.c reference)
- Memory management patterns (CREATE macro, NULL checks)
- CuTest framework for unit testing

### Environment Requirements
- MySQL/MariaDB database connection configured
- GCC or Clang with -Wall -Wextra flags
- Valgrind for memory leak verification
- CuTest framework in unittests/CuTest/

---

## 4. Scope

### In Scope (MVP)
- `vehicle_create()` - Allocate and initialize new vehicle
- `vehicle_destroy()` - Free vehicle and cleanup resources
- `vehicle_init()` - Set default values based on vehicle type
- `vehicle_set_state()`, `vehicle_get_state()` - State machine management
- `vehicle_state_name()`, `vehicle_type_name()` - String conversion utilities
- Capacity functions: add/remove passenger, add/remove weight
- Condition functions: damage, repair, is_operational
- Lookup functions: find_by_id, find_in_room, find_by_obj
- Persistence: save, load, save_all, load_all
- Global vehicle tracking array with 1000 capacity
- Database table creation at startup
- Boot-time vehicle loading integration

### Out of Scope (Deferred)
- Movement functions (vehicle_can_move, vehicle_move) - *Reason: Session 03*
- Player commands (mount, dismount, drive) - *Reason: Session 04*
- Vehicle-in-vehicle mechanics - *Reason: Session 05*
- Display functions (vehicle_show_status) - *Reason: Session 04*
- Builder/OLC integration - *Reason: Future enhancement*

---

## 5. Technical Approach

### Architecture
Create a new `vehicles.c` source file containing all vehicle implementation functions. This file will manage a global array `vehicle_list[MAX_VEHICLES]` for tracking all active vehicles. The implementation follows the established pattern from vessels_db.c where database tables are auto-created at startup and persistence operations use direct MySQL queries.

### Design Patterns
- **Array-based tracking**: Fixed-size array with NULL entries for available slots (proven at 500+ vessels)
- **Factory pattern**: vehicle_create() allocates, initializes, and registers in one operation
- **State machine**: Enum-based state transitions with validation
- **Auto-create tables**: Database schema created at startup if not exists

### Technology Stack
- ANSI C90/C89 with GNU extensions
- MySQL/MariaDB for persistence
- CuTest for unit testing
- Valgrind for memory verification

---

## 6. Deliverables

### Files to Create
| File | Purpose | Est. Lines |
|------|---------|------------|
| `src/vehicles.c` | All vehicle implementation functions | ~600 |
| `unittests/CuTest/test_vehicle_creation.c` | Unit tests for vehicle creation system | ~300 |

### Files to Modify
| File | Changes | Est. Lines |
|------|---------|------------|
| `src/Makefile.in` | Add vehicles.o to build | ~3 |
| `CMakeLists.txt` | Add src/vehicles.c to sources | ~1 |
| `src/db.c` | Call vehicle_load_all() at boot | ~5 |
| `src/comm.c` | Call vehicle_save_all() at shutdown | ~5 |
| `unittests/CuTest/Makefile` | Add test_vehicle_creation to build | ~5 |
| `unittests/CuTest/AllTests.c` | Register vehicle creation test suite | ~5 |

---

## 7. Success Criteria

### Functional Requirements
- [ ] vehicle_create() returns valid vehicle pointer for all 4 types
- [ ] vehicle_destroy() frees memory with no leaks (Valgrind clean)
- [ ] vehicle_init() sets correct defaults per vehicle type
- [ ] State transitions validate legal state changes
- [ ] Passenger/weight capacity enforces limits correctly
- [ ] Condition damage/repair clamps to valid range
- [ ] Lookup functions find vehicles by id, room, and object
- [ ] Database table auto-created at server startup
- [ ] Vehicles persist across server restart
- [ ] Up to 1000 concurrent vehicles supported

### Testing Requirements
- [ ] Unit tests for all lifecycle functions
- [ ] Unit tests for all state management functions
- [ ] Unit tests for all capacity functions
- [ ] Unit tests for all condition functions
- [ ] Unit tests for all lookup functions
- [ ] Integration test for persistence round-trip
- [ ] All tests pass with -Wall -Wextra -Werror
- [ ] Valgrind reports no memory leaks

### Quality Gates
- [ ] All files ASCII-encoded (0-127 characters only)
- [ ] Unix LF line endings
- [ ] Code follows CONVENTIONS.md (2-space indent, Allman braces, /* */ comments)
- [ ] No C99/C11 features (no // comments, no VLAs, no mixed declarations)
- [ ] All functions have documentation comments

---

## 8. Implementation Notes

### Key Considerations
- vehicle_data struct is 136 bytes - tracking array of 1000 uses ~133KB
- Use next_vehicle_id counter for unique ID assignment
- Database vehicle_id should be AUTO_INCREMENT primary key
- All string operations must use snprintf (never sprintf)
- All pointer parameters must be NULL-checked before use

### Potential Challenges
- **ID collision after server restart**: Use database AUTO_INCREMENT, load max_id on boot
  - *Mitigation*: Query MAX(vehicle_id) and set next_vehicle_id = max + 1
- **Memory fragmentation**: Frequent create/destroy cycles
  - *Mitigation*: Track in fixed array, reuse slots from destroyed vehicles
- **Concurrent save operations**: Multiple saves during shutdown
  - *Mitigation*: Single vehicle_save_all() call in shutdown sequence

### Relevant Considerations
- [P01] **Auto-create DB tables at startup**: Use ensure_vehicle_table_exists() pattern from ensure_schedule_table_exists()
- [P01] **Memory-efficient struct design**: vehicle_data is 136 bytes (well under 512 target)
- [P01] **Standalone unit test files**: Create self-contained tests that don't require server running
- [P00] **Don't use C99/C11 features**: All code must be strict ANSI C90/C89

### ASCII Reminder
All output files must use ASCII-only characters (0-127).

---

## 9. Testing Strategy

### Unit Tests
- `test_vehicle_create_cart`: Create cart, verify type and defaults
- `test_vehicle_create_wagon`: Create wagon, verify capacity settings
- `test_vehicle_create_mount`: Create mount, verify speed settings
- `test_vehicle_create_carriage`: Create carriage, verify all fields
- `test_vehicle_create_null_name`: Verify NULL name handling
- `test_vehicle_destroy_null`: Verify NULL pointer handling
- `test_vehicle_destroy_frees_memory`: Valgrind verification
- `test_vehicle_init_sets_defaults`: Verify per-type default values
- `test_vehicle_state_transitions`: Valid and invalid state changes
- `test_vehicle_passenger_limits`: Add/remove within capacity
- `test_vehicle_weight_limits`: Add/remove within weight limit
- `test_vehicle_condition_damage`: Damage clamping to 0
- `test_vehicle_condition_repair`: Repair clamping to max
- `test_vehicle_find_by_id`: Lookup existing and non-existing
- `test_vehicle_find_in_room`: Find vehicle in specific room
- `test_vehicle_tracking_array`: Add/remove from global array

### Integration Tests
- `test_vehicle_save_load_roundtrip`: Save vehicle, reload, verify all fields match
- `test_vehicle_persistence_restart`: Simulate server restart, verify vehicles restored

### Manual Testing
- Start server, verify vehicle_data table created in database
- Create test vehicle via admin command, verify appears in DB
- Restart server, verify vehicle still exists

### Edge Cases
- Create vehicle when array is full (1000 vehicles)
- Destroy vehicle with invalid ID
- Add passenger when at max capacity
- Add weight exceeding max_weight
- Damage vehicle already at 0 condition
- Repair vehicle already at max condition
- Find vehicle in empty room
- Load vehicle with corrupted database record

---

## 10. Dependencies

### External Libraries
- MySQL/MariaDB client library (libmariadb-dev)

### Other Sessions
- **Depends on**: phase02-session01-vehicle-data-structures (provides struct definitions)
- **Depended by**: phase02-session03-vehicle-movement-system, phase02-session04-vehicle-player-commands, phase02-session05-vehicle-in-vehicle-mechanics, phase02-session06-unified-command-interface, phase02-session07-testing-validation

---

## Appendix: Database Schema

```sql
CREATE TABLE IF NOT EXISTS vehicle_data (
  vehicle_id INT AUTO_INCREMENT PRIMARY KEY,
  vehicle_type INT NOT NULL DEFAULT 0,
  vehicle_state INT NOT NULL DEFAULT 0,
  vehicle_name VARCHAR(64) NOT NULL DEFAULT '',
  location INT NOT NULL DEFAULT 0,
  direction INT NOT NULL DEFAULT 0,
  max_passengers INT NOT NULL DEFAULT 0,
  current_passengers INT NOT NULL DEFAULT 0,
  max_weight INT NOT NULL DEFAULT 0,
  current_weight INT NOT NULL DEFAULT 0,
  base_speed INT NOT NULL DEFAULT 0,
  current_speed INT NOT NULL DEFAULT 0,
  terrain_flags INT NOT NULL DEFAULT 0,
  max_condition INT NOT NULL DEFAULT 100,
  vehicle_condition INT NOT NULL DEFAULT 100,
  owner_id BIGINT NOT NULL DEFAULT 0,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  INDEX idx_location (location),
  INDEX idx_owner (owner_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

---

## Appendix: Function Signatures Reference

From vessels.h (Session 01):
```c
/* Lifecycle Functions */
struct vehicle_data *vehicle_create(enum vehicle_type type, const char *name);
void vehicle_destroy(struct vehicle_data *vehicle);
void vehicle_init(struct vehicle_data *vehicle, enum vehicle_type type);

/* State Management Functions */
int vehicle_set_state(struct vehicle_data *vehicle, enum vehicle_state new_state);
enum vehicle_state vehicle_get_state(struct vehicle_data *vehicle);
const char *vehicle_state_name(enum vehicle_state state);
const char *vehicle_type_name(enum vehicle_type type);

/* Capacity Functions */
int vehicle_can_add_passenger(struct vehicle_data *vehicle);
int vehicle_add_passenger(struct vehicle_data *vehicle);
int vehicle_remove_passenger(struct vehicle_data *vehicle);
int vehicle_can_add_weight(struct vehicle_data *vehicle, int weight);
int vehicle_add_weight(struct vehicle_data *vehicle, int weight);
int vehicle_remove_weight(struct vehicle_data *vehicle, int weight);

/* Condition Functions */
int vehicle_damage(struct vehicle_data *vehicle, int amount);
int vehicle_repair(struct vehicle_data *vehicle, int amount);
int vehicle_is_operational(struct vehicle_data *vehicle);

/* Lookup Functions */
struct vehicle_data *vehicle_find_by_id(int id);
struct vehicle_data *vehicle_find_in_room(room_rnum room);
struct vehicle_data *vehicle_find_by_obj(struct obj_data *obj);

/* Persistence Functions */
int vehicle_save(struct vehicle_data *vehicle);
int vehicle_load(int vehicle_id, struct vehicle_data *vehicle);
void vehicle_save_all(void);
void vehicle_load_all(void);
```

---

## Next Steps

Run `/tasks` to generate the implementation task checklist.
