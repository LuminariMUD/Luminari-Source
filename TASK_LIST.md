# Task List for LuminariMUD Code Fixes

## High Priority Issues

### 1. PRF_FLAGGED Mob Access Issue - FIXED ✓
- **Error**: `SYSERR: Mob using '((ch)->player_specials->saved.pref)' at fight.c:11669.`
- **Frequency**: Extremely high (occurs every 2 seconds during combat)
- **Location**: fight.c:11669
- **Problem**: Code is trying to access player_specials on NPCs
- **Solution**: Added IS_NPC check before PRF_FLAGGED usage
- **Status**: FIXED - Added `!IS_NPC(ch) &&` check at line 11669

### 2. Objects in NOWHERE Executing Scripts - FIXED ✓
- **Error**: `SCRIPT ERROR: Obj (VNum):: oecho called by object in NOWHERE`
- **Affected Objects**:
  - a burning cluster of pines (VNum 11865)
  - a large webbed cocoon (VNum 1920)
  - a little green mound of earth (VNum 11870)
  - the corpse of a ghostly girl (VNum 11869)
  - a black egg (VNum 11862)
  - the corpse of the vile toad (VNum 11858)
  - the corpse of the taildove (VNum 11857)
  - the corpse of the red trout (VNum 11856)
  - a Goethite staff alight with flame (VNum 2779)
  - a red apple (VNum 501)
  - an ice cream (VNum 90)
- **Status**: FIXED - Added NOWHERE checks in dg_triggers.c
- **Solution**: Added checks in `timer_otrigger()` and `random_otrigger()` to prevent script execution for objects in NOWHERE

### 3. Database Schema Mismatches - FIXED ✓
- **Missing Tables**:
  - `luminari_muddev.pet_save_objs` - Table doesn't exist
- **Missing Columns**:
  - `house_data.idnum` - Unknown column (already had fallback in objsave.c)
  - `player_save_objs.idnum` - Unknown column (already had fallback in objsave.c)
  - `pet_data.intel` - Unknown column
  - `PLAYER_DATA.character_info` - Unknown column
- **Status**: FIXED - Added compatibility code for all missing columns
- **Solution**: 
  - pet_save_objs: Changed fatal errors to graceful handling
  - pet_data.intel: Removed column from INSERT queries
  - character_info: Added fallback UPDATE queries

## Medium Priority Issues (World File Issues - Not Code Bugs)

### 4. Missing Triggers - WORLD FILE ISSUE
- **Error**: `SYSERR: dg_read_trigger: Trigger vnum #XXXX asked for but non-existant!`
- **Missing Trigger VNums**:
  - 2315 (room)
  - 2314, 2316 (mob: the Dark Knight)
  - 2310 (mob: a giant mother spider)
  - 2313 (mob: a bat-like creature)
  - 2308 (Object: a jet black pearl)
  - 2311 (Object: a large stone chest)
  - 2317 (Object: Helm of Brilliance)
- **Status**: World file issue - code handles gracefully
- **Solution**: Either create the missing triggers or remove references from objects/mobs

### 5. Invalid Zone Commands - WORLD FILE ISSUE
- **Errors**:
  - `SYSERR: zone file: Invalid vnum 15802, cmd disabled` (zone #158, line 8)
  - `SYSERR: zone file: invalid equipment pos number` (zone #1481, line 31)
- **Status**: World file issue - code disables invalid commands
- **Solution**: Fix zone files to reference valid vnums and equipment positions

### 6. Missing Spec Procs - WORLD FILE ISSUE
- **Missing Mob Specs**: 103802, 103803
- **Missing Object Specs**: 139203, 120010, 100513, 111507, 100599
- **Status**: World file issue - code handles gracefully
- **Solution**: Either create the spec procs or remove assignments

### 7. Quests Without Questmasters - WORLD FILE ISSUE
- **Affected Quest VNums**: 0, 2011, 20309, 20315-20325, 102412-102415, 128102
- **Status**: World file issue
- **Solution**: Assign questmasters to these quests or mark them as special quests

### 8. Mob Function Calls - CODE HANDLES GRACEFULLY
- **Error**: `Attempting to call non-existing mob function`
- **Affected Mobs**:
  - Brother Spire (#200103)
  - Jakur the tanner (#125913)
  - Adoril (#21605)
- **Status**: Code already removes MOB_SPEC flag when function doesn't exist
- **Solution**: Check mob special procedures and fix function references

## Low Priority Issues

### 9. Award Magic Item System - FIXED ✓
- **Error**: `award_magic_item called but no crafting system is defined`
- **Status**: FIXED - Added crafting system configuration to mud_options.h and mud_options.example.h
- **Solution**: Added USE_OLD_CRAFTING_SYSTEM as default configuration option

### 10. Missing Object References - WORLD FILE ISSUE
- **Error**: `Object (V) 19216 does not exist in database`
- **Status**: World file issue
- **Solution**: Create object or remove references

## Implementation Status

### Phase 1: Critical Fixes (COMPLETED) ✓
1. ✓ Fixed PRF_FLAGGED usage in fight.c:11669
2. ✓ Enhanced object NOWHERE checks to prevent all script execution
3. ✓ Added database compatibility for critical missing columns

### Phase 2: Stability Improvements (NOTED)
1. Missing triggers - World file issue, code handles gracefully
2. Zone file errors - World file issue, code disables invalid commands
3. Spec procs - World file issue, code handles gracefully

### Phase 3: Content Fixes (WORLD FILE WORK NEEDED)
1. Assign questmasters to orphaned quests
2. Fix mob function references (code already handles gracefully)
3. ✓ Define crafting system (FIXED - USE_OLD_CRAFTING_SYSTEM now defined by default)
4. Create missing objects

## Testing Requirements
- ✓ Monitor logs for reduction in error frequency
- ✓ Test combat with NPCs to ensure no PRF_FLAGGED crashes
- ✓ Verify object scripts don't execute in NOWHERE
- ✓ Confirm database operations work with missing columns

## Notes
- The PRF_FLAGGED issue in fight.c has been fixed - this should dramatically reduce error spam
- Database compatibility code added for dev environment with incomplete schema
- Object script errors have been fixed by adding NOWHERE checks
- Most remaining issues are world file problems, not code bugs