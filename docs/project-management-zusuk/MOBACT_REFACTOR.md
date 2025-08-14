# MOBACT.C Mechanical Refactoring Plan

## Overview
This document outlines a safe, mechanical refactoring of `mobact.c` (2086 lines) into logical, maintainable modules without changing any functionality.

## Goals
- Split monolithic file into focused modules
- Maintain 100% backward compatibility
- Improve compilation times
- Create clear module boundaries
- Enable easier maintenance

## Current State Analysis

### File Statistics
- **Current file:** `src/mobact.c`
- **Lines:** 2086
- **Functions:** ~40 functions
- **Major components:** 
  - Mobile activity loop
  - Memory management
  - Spell/power casting
  - Class-specific behaviors
  - Racial behaviors
  - Utility functions

### Logical Components Identified

| Component | Lines | Functions | Dependencies |
|-----------|-------|-----------|--------------|
| Memory Management | 568-683 | 4 | Minimal - only structs.h |
| Utility Functions | 265-510 | 5 | Memory functions |
| Spell Data | 42-261 | Arrays | None (data only) |
| Spell Casting | 1091-1588 | 3 | Spell data, utilities |
| Class Behaviors | 800-1089 | 8 | Utilities, spells |
| Racial Behaviors | 740-797 | 1 | Utilities |
| Psionic Functions | 1364-1471, 1987-2079 | 4 | Psionics.h |
| Main Activity Loop | 1593-1985 | 1 | All above |

## Proposed New Structure

### File Organization
```
src/
├── mob_act.c        (Main mobile activity loop - ~400 lines)
├── mob_act.h        (Main coordinator header)
├── mob_memory.c     (Memory management - ~115 lines)
├── mob_memory.h     
├── mob_utils.c      (Utility functions - ~245 lines)
├── mob_utils.h      
├── mob_spells.c     (Spell casting & data - ~720 lines)
├── mob_spells.h     
├── mob_class.c      (Class behaviors - ~290 lines)
├── mob_class.h      
├── mob_race.c       (Racial behaviors - ~60 lines)
├── mob_race.h       
├── mob_psionic.c    (Psionic functions - ~200 lines)
└── mob_psionic.h    
```

### Module Dependencies
```
mob_memory   --> (standalone)
     ^
     |
mob_utils    --> mob_memory
     ^
     |
mob_race     --> mob_utils
mob_psionic  --> mob_utils
mob_spells   --> mob_utils
     ^
     |
mob_class    --> mob_utils, mob_spells
     ^
     |
mob_act      --> (all modules)
```

## Implementation Plan

### Phase 1: Header Creation (No Code Movement)
**Goal:** Create all header files with proper declarations
**Risk:** Zero - only adding new files

1. **mob_memory.h**
   ```c
   - is_in_memory()
   - remember()
   - forget()
   - clearMemory()
   ```

2. **mob_utils.h**
   ```c
   - npc_find_target()
   - npc_switch_opponents()
   - npc_rescue()
   - move_on_path()
   - mobile_echos()
   - can_continue()
   ```

3. **mob_race.h**
   ```c
   - npc_racial_behave()
   ```

4. **mob_psionic.h**
   ```c
   - npc_psionic_powerup()
   - npc_offensive_powers()
   - valid_psionic_spellup_power()
   - valid_psionic_combat_power()
   ```

5. **mob_class.h**
   ```c
   - npc_class_behave()
   - npc_monk_behave()
   - npc_rogue_behave()
   - npc_bard_behave()
   - npc_warrior_behave()
   - npc_ranger_behave()
   - npc_paladin_behave()
   - npc_berserker_behave()
   ```

6. **mob_spells.h**
   ```c
   - npc_spellup()
   - npc_offensive_spells()
   - npc_assigned_spells()
   - mob_knows_assigned_spells()
   - npc_ability_behave()
   - External arrays declarations
   ```

### Phase 2: Module Extraction Order
**Goal:** Extract one module at a time, test after each

#### Step 1: Extract mob_memory.c
- **Lines to move:** 568-683
- **Functions:** 4 memory functions
- **Risk:** Low - self-contained
- **Test:** Compile, verify mob aggro memory

#### Step 2: Extract mob_utils.c  
- **Lines to move:** 265-510, 686-710
- **Functions:** 6 utility functions
- **Risk:** Low - used by others but straightforward
- **Test:** Compile, verify mob movement/rescue

#### Step 3: Extract mob_race.c
- **Lines to move:** 740-797
- **Functions:** 1 function
- **Risk:** Low - simple extraction
- **Test:** Compile, verify racial abilities

#### Step 4: Extract mob_psionic.c
- **Lines to move:** 1364-1471, 1987-2079
- **Functions:** 4 functions
- **Risk:** Low - separate system
- **Test:** Compile, verify psionic mobs

#### Step 5: Extract mob_class.c
- **Lines to move:** 800-1089, 712-735
- **Functions:** 9 functions
- **Risk:** Medium - interdependent functions
- **Test:** Compile, verify each class behavior

#### Step 6: Extract mob_spells.c
- **Lines to move:** 42-261, 1091-1588
- **Functions:** 5 functions + data arrays
- **Risk:** Medium - large extraction
- **Test:** Compile, verify mob casting

#### Step 7: Clean up mob_act.c
- **Remaining lines:** ~1593-1985 + includes
- **Risk:** Low - just cleanup
- **Test:** Full regression test

### Phase 3: Build System Updates

#### Makefile.am Changes
```makefile
# Remove:
src/mobact.c \

# Add:
src/mob_act.c \
src/mob_memory.c \
src/mob_utils.c \
src/mob_spells.c \
src/mob_class.c \
src/mob_race.c \
src/mob_psionic.c \
```

#### CMakeLists.txt Changes
```cmake
# Remove:
src/mobact.c

# Add:
src/mob_act.c
src/mob_memory.c
src/mob_utils.c
src/mob_spells.c
src/mob_class.c
src/mob_race.c
src/mob_psionic.c
```

## Safety Protocols

### For Each Extraction Step:
1. **Copy, don't cut** - Leave original code in place initially
2. **Create new file** with proper includes
3. **Compile** - Must compile without errors
4. **Comment out original** - Use `#if 0` blocks
5. **Compile again** - Verify still works
6. **Test functionality** - Run specific tests
7. **Remove commented code** - Only after verification
8. **Commit** - Clear commit message for each step

### Compilation Test Commands
```bash
# After each step:
autoreconf -fiv && ./configure && make -j20

# Or for CMake:
cmake --build build/ -j20
```

### Rollback Strategy
- Each step is a separate git commit
- Can revert individual steps if issues arise
- Keep original mobact.c until fully verified

## Testing Checklist

### After Each Module Extraction:
- [ ] Code compiles without warnings
- [ ] Server starts successfully
- [ ] No undefined symbol errors

### Final Integration Tests:
- [ ] Mobs move randomly when appropriate
- [ ] Mobs follow paths correctly
- [ ] Mobs engage in combat
- [ ] Mobs cast spells (caster types)
- [ ] Mobs use class abilities
- [ ] Mobs use racial abilities
- [ ] Mob memory system works (remembers attackers)
- [ ] Helper mobs assist
- [ ] Guard mobs protect citizens
- [ ] Scavenger mobs pick up items
- [ ] Mobs return to default positions

## Success Criteria
- Zero functionality changes
- All existing behavior preserved
- Improved code organization
- Faster incremental compilation
- Each module under 800 lines
- Clear module boundaries

## Risk Mitigation
- **Incremental approach** - One module at a time
- **Extensive testing** - After each step
- **Version control** - Commit after each successful extraction
- **Backup plan** - Can revert to original mobact.c
- **Compilation checks** - Both build systems must work

## Timeline Estimate
- Phase 1 (Headers): 30 minutes
- Phase 2 (Extraction): 2-3 hours
- Phase 3 (Build/Test): 30 minutes
- Total: ~3-4 hours for safe completion

## Notes
- This is a mechanical refactoring - no logic changes
- Preserve all comments and formatting
- Maintain ANSI C90 compatibility
- Do not add new features during refactoring
- Focus on code organization only

## Post-Refactoring Opportunities
Once successfully split:
- Can optimize individual modules
- Easier to add new mob behaviors
- Simpler to debug specific systems
- Better code reuse possibilities
- Cleaner interfaces between systems