# Session Specification

**Session ID**: `phase02-session01-vehicle-data-structures`
**Phase**: 02 - Simple Vehicle Support
**Status**: Not Started
**Created**: 2025-12-30

---

## 1. Session Overview

This session establishes the foundational data structures for the Simple Vehicle Support system (Phase 02). Unlike the complex vessel system (ships, airships, submarines) implemented in Phases 00-01, this phase targets lightweight land-based transportation: carts, wagons, mounts, and carriages following the CWG (CircleMUD with Graphical support) pattern.

The vehicle system will be implemented as a simpler, memory-efficient alternative to vessels, designed for high concurrency support. Vehicles differ from vessels in that they do not have multi-room interiors, do not navigate the wilderness coordinate system, and operate using standard room-to-room movement on roads and overland terrain.

This session defines all enums, constants, and the core `vehicle_data` structure that subsequent sessions will use for vehicle creation (Session 02), movement (Session 03), player commands (Session 04), vehicle-in-vehicle mechanics (Session 05), and the unified command interface (Session 06).

---

## 2. Objectives

1. Define vehicle type enum with minimum 4 types (cart, wagon, mount, carriage)
2. Create memory-efficient vehicle_data structure under 512 bytes
3. Define vehicle state enum for lifecycle management
4. Establish vehicle terrain capability flags for land vehicles
5. Define capacity, weight limit, and speed constants

---

## 3. Prerequisites

### Required Sessions
- [x] `phase00-session01` through `phase00-session09` - Core Vessel System complete
- [x] `phase01-session01` through `phase01-session07` - Automation Layer complete

### Required Tools/Knowledge
- Understanding of ANSI C90/C89 struct design patterns
- Familiarity with vessels.h organization for integration patterns
- Memory-efficient struct design (reference: autopilot_data at 48 bytes)

### Environment Requirements
- GCC or Clang compiler with -Wall -Wextra
- valgrind for memory validation
- CuTest framework for unit testing

---

## 4. Scope

### In Scope (MVP)
- Vehicle type enum (VEHICLE_CART, VEHICLE_WAGON, VEHICLE_MOUNT, VEHICLE_CARRIAGE)
- Vehicle state enum (VEHICLE_STATE_IDLE, VEHICLE_STATE_MOVING, VEHICLE_STATE_LOADED)
- Core vehicle_data structure with essential fields only
- Vehicle terrain capability flags (roads, plains, forest, hills, mountains)
- Vehicle capacity constants (passengers, weight limits)
- Speed constants per vehicle type
- Header file organization (extend vehicles.h with vehicle section)
- Comprehensive documentation comments for all structures

### Out of Scope (Deferred)
- Vehicle creation/initialization logic - *Reason: Session 02 scope*
- Movement implementation - *Reason: Session 03 scope*
- Player commands (mount, dismount, hitch, drive) - *Reason: Session 04 scope*
- Database persistence schemas - *Reason: Session 02 scope*
- Vehicle-in-vehicle mechanics - *Reason: Session 05 scope*
- Integration with existing object system - *Reason: Session 02 scope*

---

## 5. Technical Approach

### Architecture
Extend the existing `vessels.h` header with a new clearly-delimited section for vehicles. This maintains consistency with the codebase organization and allows future unification under a single transport abstraction. The vehicle_data struct will be standalone (not derived from greyhawk_ship_data) to keep it lightweight.

### Design Patterns
- **Separation of concerns**: Vehicle data structures are distinct from vessel structures
- **Composition over inheritance**: No struct inheritance; flat, simple data layout
- **Flag-based capabilities**: Bitfield flags for terrain traversal capabilities
- **Enum-based state machine**: Clear state enumeration for lifecycle management

### Technology Stack
- ANSI C90/C89 with GNU extensions
- Header-only definitions (no .c file for this session)
- Integration point: vessels.h

---

## 6. Deliverables

### Files to Create
| File | Purpose | Est. Lines |
|------|---------|------------|
| (none) | All definitions added to existing header | - |

### Files to Modify
| File | Changes | Est. Lines |
|------|---------|------------|
| `src/vessels.h` | Add VEHICLE SYSTEM section with enums, constants, and struct | ~150 |
| `unittests/CuTest/test_vehicles.c` | Unit tests for struct size validation | ~80 |
| `unittests/CuTest/Makefile` | Add test_vehicles to build | ~5 |
| `unittests/CuTest/AllTests.c` | Register vehicle test suite | ~5 |

---

## 7. Success Criteria

### Functional Requirements
- [ ] All 4+ vehicle types defined (cart, wagon, mount, carriage)
- [ ] vehicle_data struct compiles without errors or warnings
- [ ] Struct size validated at <512 bytes via sizeof() test
- [ ] No duplicate definitions (all constants defined once)
- [ ] All terrain flags defined and documented
- [ ] All capacity/weight constants defined

### Testing Requirements
- [ ] Unit tests written for struct size validation
- [ ] Unit tests verify enum value uniqueness
- [ ] Compile test passes with -Wall -Wextra -Werror
- [ ] Valgrind clean on test runner

### Quality Gates
- [ ] All files ASCII-encoded (0-127 characters only)
- [ ] Unix LF line endings
- [ ] Code follows CONVENTIONS.md (2-space indent, Allman braces, /* */ comments)
- [ ] No C99/C11 features (no // comments, no VLAs, no mixed declarations)

---

## 8. Implementation Notes

### Key Considerations
- Vehicle enum values must not conflict with existing VESSEL_TYPE_* or vessel_class values
- Vehicle state enum must be distinct from VESSEL_STATE_* constants
- Struct must remain under 512 bytes to support high concurrency (target: 256-384 bytes)
- Forward declarations may be needed for function prototypes in later sessions

### Potential Challenges
- **VNUM conflicts**: Ensure VEHICLE_* constants don't overlap with existing item types
  - *Mitigation*: Start vehicle enums at offset 100 from vessel enums
- **Namespace pollution**: Many similar names between vehicles and vessels
  - *Mitigation*: Prefix all vehicle items with VEHICLE_ not VESSEL_
- **Struct bloat**: Easy to add too many fields
  - *Mitigation*: Only essential fields for MVP; defer optional fields

### Relevant Considerations
- [P01] **Duplicate struct definitions in vessels.h**: Must avoid repeating this pattern. All vehicle definitions consolidated in single section.
- [P01] **Memory-efficient autopilot struct (48 bytes)**: Use similar design approach - minimal fields, proper type sizing.
- [P00] **Don't use C99/C11 features**: Strictly ANSI C90/C89 compliance. No // comments, no inline without __inline__.

### ASCII Reminder
All output files must use ASCII-only characters (0-127).

---

## 9. Testing Strategy

### Unit Tests
- `test_vehicle_struct_size`: Verify sizeof(vehicle_data) < 512
- `test_vehicle_type_enum_values`: Verify no duplicate enum values
- `test_vehicle_state_enum_values`: Verify state enum integrity
- `test_vehicle_terrain_flags`: Verify flag bits are unique powers of 2

### Integration Tests
- Compile vessels.h with -Wall -Wextra -Werror
- Verify no naming conflicts with existing codebase via grep

### Manual Testing
- Include vessels.h in a test file and instantiate vehicle_data
- Verify all fields can be assigned and read

### Edge Cases
- Empty/zero-initialized vehicle_data struct
- Maximum values for capacity fields
- All terrain flags set simultaneously

---

## 10. Dependencies

### External Libraries
- None (pure C header definitions)

### Other Sessions
- **Depends on**: phase00-session09, phase01-session07 (foundation complete)
- **Depended by**: phase02-session02 (creation), phase02-session03 (movement), phase02-session04 (commands), phase02-session05 (vehicle-in-vehicle), phase02-session06 (unified interface), phase02-session07 (testing)

---

## Appendix: Proposed Structure Layout

```c
/* Vehicle Types (land-based transport) */
enum vehicle_type
{
  VEHICLE_NONE = 0,
  VEHICLE_CART,      /* 1-2 passengers, low capacity */
  VEHICLE_WAGON,     /* 4-6 passengers, high capacity */
  VEHICLE_MOUNT,     /* 1 rider, fast movement */
  VEHICLE_CARRIAGE   /* 2-4 passengers, enclosed */
};

/* Vehicle States */
enum vehicle_state
{
  VSTATE_IDLE = 0,   /* Stationary, not in use */
  VSTATE_MOVING,     /* Currently traveling */
  VSTATE_LOADED,     /* Carrying cargo/passengers */
  VSTATE_HITCHED,    /* Attached to another vehicle */
  VSTATE_DAMAGED     /* Broken, needs repair */
};

/* Vehicle Terrain Flags (bitfield) */
#define VTERRAIN_ROAD     (1 << 0)  /* Paved roads */
#define VTERRAIN_PLAINS   (1 << 1)  /* Open grassland */
#define VTERRAIN_FOREST   (1 << 2)  /* Light forest */
#define VTERRAIN_HILLS    (1 << 3)  /* Hilly terrain */
#define VTERRAIN_MOUNTAIN (1 << 4)  /* Mountain paths */
#define VTERRAIN_DESERT   (1 << 5)  /* Desert/sand */
#define VTERRAIN_SWAMP    (1 << 6)  /* Wetlands */

/* Core vehicle structure - target <512 bytes */
struct vehicle_data
{
  /* Identity */
  int id;                    /* Unique vehicle ID */
  enum vehicle_type type;    /* Vehicle classification */
  enum vehicle_state state;  /* Current state */
  char name[64];             /* Vehicle name/description */

  /* Location */
  room_rnum location;        /* Current room */
  int direction;             /* Facing direction */

  /* Capacity */
  int max_passengers;        /* Maximum passenger count */
  int current_passengers;    /* Current passenger count */
  int max_weight;            /* Weight capacity in pounds */
  int current_weight;        /* Current load weight */

  /* Movement */
  int base_speed;            /* Base movement speed */
  int current_speed;         /* Modified speed */
  int terrain_flags;         /* VTERRAIN_* bitfield */

  /* Condition */
  int max_condition;         /* Maximum durability */
  int condition;             /* Current durability */

  /* Ownership */
  long owner_id;             /* Player ID of owner */

  /* Object reference */
  struct obj_data *obj;      /* Associated object */
};
```

---

## Next Steps

Run `/tasks` to generate the implementation task checklist.
