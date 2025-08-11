# Movement System Mechanical Refactor - CLEAN EXECUTION PLAN

## Critical Lessons Learned from Failed Attempt

### What Went Wrong
1. **Commenting out functions while they're still being called** - This is idiotic. Functions must remain active until ALL references are updated.
2. **Mixed approaches** - Using both `/* */` comments and `#if 0` blocks created a mess
3. **Not handling function dependencies** - Static functions in movement.c are called by other functions in the same file
4. **Header file confusion** - ACMD vs ACMD_DECL mixup shows lack of understanding of the macro system
5. **Trying to do too much at once** - Should have tested compilation after EVERY SINGLE CHANGE

### The Core Problem
**YOU CANNOT JUST MOVE FUNCTIONS WITHOUT UNDERSTANDING THEIR DEPENDENCIES**

## The CORRECT Approach - Phase by Phase

### Phase 1: Simple File Rename (ONLY RENAME, NOTHING ELSE)
```bash
# Step 1: Rename the file
git mv src/act.movement.c src/movement.c

# Step 2: Update Makefile.am
sed -i 's/act\.movement\.c/movement.c/g' Makefile.am

# Step 3: Update the file header comment
sed -i '2s/act\.movement\.c/movement.c/' src/movement.c

# Step 4: TEST COMPILATION
make clean && make

# Step 5: Commit if successful
git add -A && git commit -m "Rename act.movement.c to movement.c"
```

**STOP HERE AND TEST**

### Phase 2: Create Module Structure (EMPTY FILES ONLY)

```bash
# Create empty module files
touch src/movement_validation.{c,h}
touch src/movement_cost.{c,h}
touch src/movement_position.{c,h}
touch src/movement_doors.{c,h}
touch src/movement_falling.{c,h}

# Add to Makefile.am (BUT DON'T BUILD YET)
# Just add the filenames, we'll populate them later
```

### Phase 3: Understand Function Dependencies

**BEFORE MOVING ANY FUNCTION, MAP ITS DEPENDENCIES:**

#### Category A: TRULY INDEPENDENT Functions (Move First)
These functions don't call any other functions in movement.c:
- `get_speed()` - Only uses external functions
- `is_evaporating_key()` - Simple check function
- `has_key()` - Simple inventory check

#### Category B: SEMI-INDEPENDENT Functions (Move Second)
These call only external functions or Category A functions:
- `has_boat()` - Calls external `is_flying()`
- `has_flight()` - Calls external `is_flying()`
- `has_scuba()` - Only external calls
- `can_climb()` - Calls `has_flight()` from Category B

#### Category C: INTERDEPENDENT Functions (Move as Groups)
These must be moved together:
- Door system group:
  - `find_door()` (static)
  - `do_doorcmd()` (static)  
  - `ok_pick()` (static)
  - `do_gen_door()` - Calls all above
  
- Position system group:
  - `can_stand()`
  - `change_position()` 
  - `do_stand()` - Calls `change_position()`
  - `do_sit()` - Calls `change_position()`
  - `do_rest()` - Calls `change_position()`
  - `do_sleep()` - Calls `change_position()`
  - `do_recline()` - Calls `change_position()`
  - `do_wake()` - Calls `change_position()`

#### Category D: COMPLEX Functions (DON'T MOVE YET)
- `do_simple_move()` - Calls EVERYTHING
- `perform_move()` - Calls do_simple_move
- `do_move()` - Calls perform_move
- `do_enter()` - Complex portal logic
- `do_leave()` - Complex logic

### Phase 4: The SMART Way to Extract Functions

#### Step 1: Create a Compatibility Layer

```c
// movement_common.h - Temporary compatibility header
#ifndef MOVEMENT_COMMON_H
#define MOVEMENT_COMMON_H

// During transition, this allows functions to be in different files
// but still find each other

// From movement_validation.c
int has_boat(struct char_data *ch, room_rnum going_to);
int has_flight(struct char_data *ch);
// ... etc

#endif
```

#### Step 2: Use CONDITIONAL COMPILATION Intelligently

```c
// In movement.c
#ifndef MOVEMENT_VALIDATION_SEPARATE
// Original function here
int has_boat(...) { 
  // implementation
}
#endif

// In movement_validation.c
#define MOVEMENT_VALIDATION_SEPARATE
#include "movement_common.h"
// Same function here
int has_boat(...) {
  // EXACT SAME implementation
}
```

#### Step 3: Switch One Module at a Time

```makefile
# In Makefile.am, add compilation flags to use the new file
# This way you can switch between old and new WITHOUT touching the code
AM_CFLAGS += -DMOVEMENT_VALIDATION_SEPARATE
```

### Phase 5: Correct Order of Extraction

#### Round 1: Extract TRULY INDEPENDENT functions
1. Copy `get_speed()` to `movement_cost.c`
2. Add `-DMOVEMENT_COST_SEPARATE` to Makefile
3. Compile and test
4. If successful, remove from movement.c

#### Round 2: Extract SEMI-INDEPENDENT functions
1. Copy validation functions to `movement_validation.c`
2. Ensure they can find each other via headers
3. Add `-DMOVEMENT_VALIDATION_SEPARATE`
4. Compile and test
5. If successful, remove from movement.c

#### Round 3: Extract GROUPED functions
1. Copy entire door system group to `movement_doors.c`
2. Make static functions non-static in the new file
3. Add proper headers
4. Test extensively

### Phase 6: The Nuclear Option (If All Else Fails)

```c
// movement.c
#include "movement_implementation.c"

// movement_implementation.c (not compiled separately)
// Contains all the actual code

// Then gradually split movement_implementation.c into:
#include "movement_validation_impl.c"
#include "movement_doors_impl.c"
// etc.
```

## Testing Strategy

### After EVERY Single Change:
```bash
make clean && make
if [ $? -eq 0 ]; then
    echo "SUCCESS - Continue"
    git add -A && git commit -m "Working state"
else
    echo "FAILED - Revert"
    git reset --hard HEAD
fi
```

### Integration Test Points:
1. After file rename - Full compile
2. After adding empty modules - Full compile  
3. After each function extraction - Full compile
4. After each module completion - Full compile + run server
5. After all extractions - Full test suite

## What NOT to Do (Learning from Failures)

1. **DON'T** comment out functions that are still being called
2. **DON'T** mix comment styles (`/* */` with `#if 0`)
3. **DON'T** forget about static function dependencies
4. **DON'T** try to move 10 functions at once
5. **DON'T** assume the build system will figure it out
6. **DON'T** change function signatures during extraction
7. **DON'T** forget to test after every change

## The Golden Rules

1. **One change at a time**
2. **Test after every change**
3. **Commit working states**
4. **Map dependencies first**
5. **Don't break what works**
6. **If it gets messy, reset and start over**

## Realistic Timeline

- Phase 1 (Rename): 10 minutes
- Phase 2 (Structure): 10 minutes  
- Phase 3 (Analysis): 30 minutes
- Phase 4 (Setup): 20 minutes
- Phase 5 (Extraction): 2-3 hours (WITH TESTING)
- Phase 6 (Cleanup): 30 minutes

**Total: 4 hours of CAREFUL work**

## Success Criteria

- [ ] All files compile without warnings
- [ ] Server starts without errors
- [ ] Movement commands work (n/s/e/w)
- [ ] Doors work (open/close/lock/unlock)
- [ ] Position commands work (sit/stand/rest/sleep)
- [ ] No memory leaks
- [ ] Code is MORE organized, not less
- [ ] Can still revert if needed

## Emergency Rollback Plan

```bash
# If everything goes to hell
git reset --hard HEAD
git clean -fd
# Start over with a better plan
```

## Final Words

**MECHANICAL REFACTORING MEANS NO LOGIC CHANGES**

If you're changing logic, you're not doing mechanical refactoring. You're doing a rewrite. Pick one.

**TEST EVERY FUCKING CHANGE**

Not every 10 changes. Not every file. EVERY. SINGLE. CHANGE.

**UNDERSTAND BEFORE MOVING**

If you don't understand why a function is static, or what calls it, DON'T MOVE IT YET.

**SMALL STEPS WIN**

Better to take 100 small successful steps than 1 big failed leap.