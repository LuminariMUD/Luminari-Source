# Movement Systems Refactoring Strategy

## Overview
The `act.movement.c` file contains 4,418 lines of code handling all character movement, position changes, door/portal mechanics, and related functionality. This document outlines a comprehensive refactoring strategy to modernize the movement system, improve maintainability, reduce complexity, and enhance code organization through a modular architecture.

## Current State Analysis

### File Statistics
- **Current File**: `act.movement.c`
- **Total Lines**: 4,418
- **Core Functions**: ~45 major functions
- **Complexity**: High - multiple responsibilities mixed together
- **Key Issues**:
  - Monolithic functions (do_simple_move is 400+ lines)
  - Mixed responsibilities (movement, doors, positions, mounts)
  - Complex nested conditionals
  - Duplicate validation logic
  - Hard-coded magic numbers
  - Limited error handling
  - Outdated naming convention (`act.` prefix)

## Naming Convention Modernization

### Current Convention Issues
- The `act.` prefix is a legacy from CircleMUD meaning "action/command files"
- Newer systems already use modern names: `trails.c`, `transport.c`, `crafting_new.c`
- The prefix adds no semantic value and creates longer, less readable names

### New File Structure
Transform from monolithic `act.movement.c` to modular `movement_*.c` system:

```
src/
  movement.c                 # Core movement system (simplified do_simple_move)
  movement_validation.c      # All movement checks and restrictions
  movement_position.c        # Position changes (sit/stand/rest/sleep/recline)
  movement_stealth.c         # Sneak/hide mechanics
  movement_mounts.c          # Mount/rider system
  movement_cost.c            # Movement point calculations
  movement_messages.c        # Enter/exit/sneak messages
  movement_doors.c           # Door mechanics and commands
  movement_portals.c         # Portal entry system
  movement_falling.c         # Falling physics and damage
  movement_constants.h       # All constants and configuration
```

### Migration Strategy
```c
// Temporary compatibility header during transition
// act.movement.h (deprecated - remove after migration)
#pragma message("act.movement.h is deprecated, use movement.h")
#include "movement.h"

// Add compatibility defines if needed
#define do_simple_move movement_perform
```

## Top 10 Refactoring Strategies

### 1. Extract Movement Validation Module
**Priority**: HIGH  
**Effort**: Medium  
**Impact**: High

Create `movement_validation.c/h` to consolidate all movement checks:
- Terrain validation (water, flying, climbing, etc.)
- Mount requirements
- Room restrictions (size, flags, zones)
- Character state checks (grappled, entangled, etc.)

**Benefits**:
- Single source of truth for movement rules
- Easier to test and modify
- Reduces do_simple_move() complexity by ~150 lines

**Implementation**:
```c
// movement_validation.h
typedef struct {
    bool can_move;
    char *error_message;
    int movement_cost;
} movement_result_t;

movement_result_t validate_movement(struct char_data *ch, int direction);
bool can_enter_water(struct char_data *ch, room_rnum destination);
bool can_fly_here(struct char_data *ch, room_rnum destination);
bool can_climb_here(struct char_data *ch, room_rnum destination);
```

### 2. Separate Sneak/Stealth System
**Priority**: HIGH  
**Effort**: Medium  
**Impact**: Medium

Extract stealth mechanics to `movement_stealth.c/h`:
- Lines 1490-1750 contain complex sneak detection logic
- Consolidate mounted/unmounted sneak handling
- Centralize sneak vs. listen checks

**Benefits**:
- Simplifies movement message generation
- Makes stealth mechanics testable
- Reduces conditional nesting

**Implementation**:
```c
// movement_stealth.h
bool is_sneaking_successfully(struct char_data *ch, struct char_data *observer);
void generate_sneak_messages(struct char_data *ch, int dir, bool entering);
void handle_mounted_sneak(struct char_data *ch, struct char_data *mount, int dir);
```

### 3. Modularize Message System
**Priority**: MEDIUM  
**Effort**: Low  
**Impact**: Medium

Create `movement_messages.c/h` for all movement-related messages:
- Enter/exit room messages
- Sneak detection messages
- Mount/dismount messages
- Position change messages

**Benefits**:
- Consistent message formatting
- Easier localization
- Reduced code duplication

**Implementation**:
```c
// movement_messages.h
void send_movement_message(struct char_data *ch, int message_type, ...);
void send_enter_message(struct char_data *ch, int from_dir);
void send_exit_message(struct char_data *ch, int to_dir);
```

### 4. Extract Position System
**Priority**: MEDIUM  
**Effort**: Medium  
**Impact**: High

Move position management to `movement_position.c/h`:
- change_position() function (lines 3688-4013)
- All position commands (sit, stand, rest, sleep, recline)
- Position validation logic

**Benefits**:
- Cleaner separation of concerns
- Easier to add new positions
- Simplified combat position handling

**Implementation**:
```c
// movement_position.h
int change_position(struct char_data *ch, int new_position);
bool can_change_position(struct char_data *ch, int new_position);
void handle_position_effects(struct char_data *ch, int old_pos, int new_pos);
```

### 5. Separate Tracks/Trail System
**Priority**: LOW  
**Effort**: Low  
**Impact**: Low

Expand existing `trails.c` with all tracking functionality:
- Move create_tracks() function
- Move cleanup_all_trails() function
- Better encapsulation of trail data structures

**Benefits**:
- Feature isolation
- Easier to disable/enable tracking
- Cleaner memory management

### 6. Consolidate Door/Portal Mechanics
**Priority**: HIGH  
**Effort**: High  
**Impact**: High

Create `movement_doors.c/h` and `movement_portals.c/h` for door and portal logic:
- Extract door commands (lines 2270-2880) to `movement_doors.c`
- Move portal logic from do_enter() (lines 2882-3175) to `movement_portals.c`
- Consolidate key handling in `movement_doors.c`

**Benefits**:
- Separated feature logic
- Easier to extend portal types
- Simplified door mechanics

**Implementation**:
```c
// movement_doors.h
int find_door(struct char_data *ch, const char *type, char *dir, const char *cmdname);
void handle_door_command(struct char_data *ch, int door, int command);
bool has_key(struct char_data *ch, obj_vnum key);

// movement_portals.h
bool process_portal_entry(struct char_data *ch, struct obj_data *portal);
room_rnum calculate_portal_destination(struct obj_data *portal);
```

### 7. Extract Movement Cost Calculations
**Priority**: MEDIUM  
**Effort**: Low  
**Impact**: Medium

Create `movement_cost.c/h` for movement point calculations:
- Terrain costs
- Skill modifiers
- Speed calculations
- Mount effects

**Benefits**:
- Centralized movement economy
- Easier balancing
- Clear cost formulas

**Implementation**:
```c
// movement_cost.h
int calculate_movement_cost(struct char_data *ch, room_rnum from, room_rnum to);
int get_terrain_cost(int sector_type);
int apply_skill_modifiers(struct char_data *ch, int base_cost);
```

### 8. Simplify do_simple_move()
**Priority**: CRITICAL  
**Effort**: High  
**Impact**: Very High

Break the 400+ line function into smaller, focused functions:
- `validate_movement()` - All pre-movement checks
- `apply_movement_effects()` - Movement costs and effects
- `handle_followers()` - Follower movement
- `trigger_room_events()` - Scripts and triggers
- `generate_movement_messages()` - All messaging

**Benefits**:
- Dramatically improved readability
- Easier debugging
- Better testability
- Reduced cognitive load

**Implementation Structure**:
```c
int do_simple_move(struct char_data *ch, int dir, int need_specials_check) {
    movement_result_t validation = validate_movement(ch, dir);
    if (!validation.can_move) {
        send_to_char(ch, validation.error_message);
        return 0;
    }
    
    room_rnum old_room = IN_ROOM(ch);
    room_rnum new_room = calculate_destination(ch, dir);
    
    if (!apply_movement_cost(ch, validation.movement_cost))
        return 0;
    
    generate_exit_messages(ch, dir);
    move_character(ch, old_room, new_room);
    generate_enter_messages(ch, dir);
    
    trigger_movement_events(ch, old_room, new_room, dir);
    handle_room_effects(ch, new_room);
    
    return 1;
}
```

### 9. Create Mount System Module
**Priority**: MEDIUM  
**Effort**: Medium  
**Impact**: Medium

Extract all mount/rider logic to `movement_mounts.c/h`:
- Mount validation
- Mounted movement handling
- Mount/dismount mechanics
- Riding skill checks

**Benefits**:
- Isolated mount system
- Easier mount feature additions
- Cleaner main movement code

**Implementation**:
```c
// movement_mounts.h
bool is_valid_mount(struct char_data *mount);
bool can_mount(struct char_data *ch, struct char_data *mount);
void handle_mounted_movement(struct char_data *ch, int dir);
int get_mount_movement_cost(struct char_data *mount, int base_cost);
```

### 10. Optimize Falling System
**Priority**: LOW  
**Effort**: Medium  
**Impact**: Low

Move falling mechanics to `movement_falling.c/h`:
- event_falling() function
- char_should_fall() logic
- Fall damage calculations
- Safe fall mechanics

**Benefits**:
- Cleaner event handling
- Isolated falling physics
- Easier to extend (gliding, etc.)

## Additional Improvements

### Constants and Magic Numbers
Replace all magic numbers with named constants:
```c
// movement_constants.h
#define BASE_MOVEMENT_SPEED 30
#define MOUNTED_SPEED_BONUS 20
#define FLYING_SPEED 50
#define SWIM_DC_BASE 13
#define CLIMB_DC_BASE 10
#define MOVEMENT_COST_MULTIPLIER 10
```

### Error Handling
Implement comprehensive error handling:
- Add null checks at function entry
- Validate room numbers before access
- Check array bounds
- Add logging for unusual conditions

### Memory Management
Improve trail system memory handling:
- Use memory pools for trail data
- Implement proper cleanup on shutdown
- Add memory leak detection

### Testing Infrastructure
Create unit tests for critical functions:
- Movement validation tests
- Cost calculation tests
- Position change tests
- Message generation tests

### Documentation
Add comprehensive documentation:
- Function headers with parameter descriptions
- Complex algorithm explanations
- Movement system overview document
- Configuration guide

## Implementation Plan

### Phase 0: File Renaming (Day 1)
1. Rename `act.movement.c` to `movement.c`
2. Create compatibility header `act.movement.h`
3. Update all includes across codebase
4. Update Makefile/build system
5. Test compilation and basic functionality

### Phase 0.5: Safe Mechanical Extraction (Week 1) - ZERO RISK APPROACH

This phase involves **ONLY** moving functions between files with **ZERO** logic changes. This is the safest possible refactoring approach.

#### Safety Rules - MANDATORY
1. **NO logic changes** - Not even fixing obvious bugs
2. **NO renaming** - Keep all function names identical
3. **NO signature changes** - Keep all parameters and return types identical
4. **NO optimization** - Leave all inefficiencies as-is
5. **NO reformatting** - Keep indentation and style unchanged
6. **Compile after EVERY function move**
7. **Test after EVERY module extraction**

#### Step-by-Step Mechanical Extraction Process

##### Step 1: Create Module Files (30 minutes)
```bash
# Create empty module files with just headers
touch movement_validation.c movement_validation.h
touch movement_position.c movement_position.h
touch movement_doors.c movement_doors.h
touch movement_cost.c movement_cost.h
touch movement_falling.c movement_falling.h

# Add standard includes to each .c file
# Copy the EXACT same includes from act.movement.c
```

##### Step 2: Extract Helper Functions (1 hour)
**These have minimal dependencies and are safest to move first:**

```c
// Move to movement_validation.c (in this exact order):
- has_boat()           (lines 385-438)
- has_flight()         (lines 441-463)
- has_scuba()          (lines 466-491)
- can_climb()          (lines 494-580)

// Move to movement_cost.c:
- get_speed()          (lines 4142-4221)

// After each function:
make clean && make
./bin/circle  # Test basic movement
```

##### Step 3: Extract Position System (1 hour)
**Self-contained command functions:**

```c
// Move to movement_position.c (in order):
- can_stand()          (lines 3199-3235)
- do_stand()           (lines 3238-3300)  
- do_sit()             (lines 3302-3391)
- do_rest()            (lines 3393-3447)
- do_recline()         (lines 3449-3484)
- do_sleep()           (lines 3486-3518)
- do_wake()            (lines 3520-3560)
- change_position()    (lines 3688-4013)

// Test after extraction:
- Stand/sit/rest/sleep/recline commands
- Combat position changes
```

##### Step 4: Extract Door System (2 hours)
**Related functions that reference each other:**

```c
// Move to movement_doors.c (keep together):
- find_door()          (lines 2270-2377)
- is_evaporating_key() (lines 2383-2424)
- has_key()            (lines 2428-2472)
- extract_key()        (lines 2476-2515)
- do_doorcmd()         (lines 2537-2685)
- ok_pick()            (lines 2687-2776)
- do_gen_door()        (lines 2778-2880)

// Test after extraction:
- Open/close/lock/unlock doors
- Pick locks
- Use keys
```

##### Step 5: Extract Falling System (30 minutes)
```c
// Move to movement_falling.c:
- obj_should_fall()    (lines 195-213)
- char_should_fall()   (lines 217-252)
- event_falling()      (lines 254-381)

// Test after extraction:
- Fall from height
- Flying/levitation prevents falling
```

##### Step 6: Extract Miscellaneous (30 minutes)
```c
// Move to movement_validation.c:
- is_top_of_room_for_singlefile() (lines 152-167)
- get_char_ahead_of_me()          (lines 171-187)

// Move to trails.c (if not campaign-specific):
- create_tracks()      (lines 593-689)
- cleanup_all_trails() (lines 692-745)
```

#### Verification Checklist After Each Extraction
```bash
✓ Compilation successful (no warnings/errors)
✓ Server starts without crashes
✓ Basic movement works (n/s/e/w)
✓ Doors work (open/close/lock/unlock)
✓ Positions work (sit/stand/rest)
✓ Falling works (if applicable)
✓ No memory leaks (quick valgrind test)
```

#### What NOT to Do During Extraction

```c
// DON'T fix bugs (even obvious ones):
if (ch = NULL)  // WRONG but leave it!

// DON'T rename for clarity:
do_simple_move() -> movement_perform()  // NO!

// DON'T optimize:
for(i=0;i<10;i++) 
  for(j=0;j<10;j++)  // Leave nested loops as-is

// DON'T change signatures:
int has_boat(struct char_data *ch, room_rnum going_to)
-> bool has_boat(character *ch, room_t dest)  // NO!

// DON'T add const:
void foo(char *str) -> void foo(const char *str)  // NO!

// DON'T fix memory leaks:
malloc(100);  // Even if never freed, leave it!

// DON'T reorganize includes:
#include "structs.h"  // Keep exact same order
#include "utils.h"
```

#### Function Dependency Map

```
INDEPENDENT (move first):
├── has_boat()
├── has_flight()  
├── has_scuba()
├── can_climb()
├── get_speed()
├── can_stand()
└── position commands (do_sit, etc.)

GROUPED (move together):
├── Door System
│   ├── find_door()
│   ├── has_key()
│   ├── extract_key()
│   └── do_gen_door()
└── Falling System
    ├── char_should_fall()
    └── event_falling()

COMPLEX (move last):
├── do_simple_move() - 400+ lines
├── perform_move() - calls do_simple_move
└── do_enter() - portal logic
```

#### Success Criteria
- ✅ All functions extracted to appropriate modules
- ✅ Zero logic changes (verified by diff)
- ✅ All tests pass
- ✅ No new bugs introduced
- ✅ Code compiles without warnings
- ✅ Server runs identically to before

#### Time Estimate
- Total: 6-8 hours of careful work
- Can be done incrementally over several days
- Each extraction is independently valuable

#### Rollback Plan
```bash
# If anything goes wrong:
git diff act.movement.c  # See what changed
git checkout -- .         # Revert everything
# Or revert specific module:
git checkout -- movement_validation.c
```

This mechanical extraction is **100% safe** because:
- No logic changes mean no new bugs
- Compiler catches any missing declarations
- Each step is independently testable
- Easy to verify with diff
- Can rollback at any point

### Phase 1: Code Improvements (Week 2)
After successful mechanical extraction:
1. Fix identified bugs (marked with TODO)
2. Rename functions for clarity
3. Add const correctness
4. Optimize obvious inefficiencies
5. Add error handling
6. Update function signatures to use bool where appropriate

### Phase 2: Complex Extractions (Week 3-4)
Extract the more complex, interdependent systems:
1. Extract mount system to `movement_mounts.c`
2. Separate sneak mechanics to `movement_stealth.c`
3. Modularize messages to `movement_messages.c`
4. Extract portal logic from do_enter() to `movement_portals.c`

### Phase 3: Core Refactoring (Week 5-6)
1. Simplify do_simple_move() in `movement.c`
2. Break down perform_move() function
3. Reorganize do_enter() and do_leave()
4. Create unified movement pipeline

### Phase 4: Optimization (Week 7)
1. Optimize falling system in `movement_falling.c`
2. Improve trail system (integrate with existing `trails.c`)
3. Add error handling across all modules
4. Performance testing

### Phase 5: Testing & Documentation (Week 8)
1. Create unit tests for each module
2. Integration testing
3. Update documentation
4. Code review
5. Remove compatibility headers

## Expected Benefits

### Maintainability
- 60% reduction in file size
- 40% reduction in function complexity
- Clear module boundaries
- Single responsibility principle

### Performance
- Faster movement validation
- Reduced memory allocation
- Optimized message generation
- Better cache locality

### Extensibility
- Easier to add new movement types
- Simpler feature toggles
- Cleaner plugin points
- Better mod support

## Risk Mitigation

### Backward Compatibility
- Maintain existing command interfaces
- Preserve trigger compatibility
- Test with existing world files
- Gradual migration approach

### Testing Strategy
- Comprehensive regression tests
- Player beta testing
- Automated movement testing
- Performance benchmarking

### Rollback Plan
- Version control all changes
- Feature flag new systems
- Parallel implementation
- Staged deployment

## Success Metrics

- **Code Metrics**:
  - Reduce cyclomatic complexity by 50%
  - Achieve 80% test coverage
  - Reduce average function length to <50 lines

- **Performance Metrics**:
  - 20% reduction in movement processing time
  - 30% reduction in memory usage for trails
  - No increase in CPU usage

- **Quality Metrics**:
  - 50% reduction in movement-related bugs
  - 30% faster feature implementation
  - Improved code review ratings

## File Size Projections

### Current vs. Projected Line Counts
```
Current:
  act.movement.c:           4,418 lines

Projected:
  movement.c:                 800 lines (core logic)
  movement_validation.c:      600 lines
  movement_position.c:        500 lines
  movement_stealth.c:         400 lines
  movement_mounts.c:          350 lines
  movement_doors.c:           450 lines
  movement_portals.c:         300 lines
  movement_messages.c:        250 lines
  movement_cost.c:            200 lines
  movement_falling.c:         250 lines
  movement_constants.h:       100 lines
  -----------------------------------
  Total:                    4,200 lines (5% reduction)
```

The total line count reduction is modest, but the key benefit is **organization and maintainability**, not raw size reduction.

## Conclusion

This refactoring strategy will transform `act.movement.c` from a monolithic file into a well-organized, modular system using modern naming conventions. The phased approach ensures minimal disruption while delivering significant improvements in code quality, maintainability, and performance.

### Key Advantages of Modern Naming
- **Clarity**: `movement_*.c` immediately shows file relationships
- **IDE Support**: Better autocomplete and navigation
- **Future-Proof**: Aligns with modern codebase direction
- **Simplicity**: Removes meaningless `act.` prefix

The investment in refactoring will pay dividends through:
- Faster bug fixes
- Easier feature additions
- Better performance
- Improved developer experience
- Higher code quality
- Modern, maintainable architecture

Priority should be given to the file renaming (Phase 0) and simplifying `do_simple_move()`, as these changes will have the most immediate impact on code maintainability and set the foundation for the modular architecture.