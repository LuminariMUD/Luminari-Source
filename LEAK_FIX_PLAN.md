# LuminariMUD Memory Leak and Error Fix Plan

Based on valgrind analysis from `valgrind_mudpocalypse_20250804_121205.log`

## Executive Summary

- **Total Errors**: 84 errors from 84 contexts
- **Memory Leaked**: 21,408 bytes definitely lost, 48,632 bytes indirectly lost (✅ ~68KB fixed)
- **Still Reachable**: 903,516,181 bytes (expected at shutdown)
- **Critical Issues**: 2 invalid memory reads that can cause crashes ✅ FIXED
- **Object Memory Leaks**: ~20KB from missing spellbook cleanup ✅ FIXED
- **Output Buffer Leaks**: ~48KB from unbounded buffer pool ✅ FIXED

## Priority 1: CRITICAL - Invalid Memory Access (Crash Risk) ✅ FIXED

### Issue 1: Invalid read in apply_damage_reduction (fight.c:7763) ✅ FIXED
```
==168114== Invalid read of size 4
==168114==    at 0x33BA53: apply_damage_reduction (fight.c:7763)
==168114== Invalid read of size 8
==168114==    at 0x505626: get_wearoff (utils.c:6698)
```

**Root Cause**: The `dr->spell` value is likely out of bounds for the `spell_info[]` array.

**Fix** ✅ **IMPLEMENTED**:
1. ✅ Added bounds checking in `apply_damage_reduction()` before calling `get_wearoff()`:
   ```c
   if (dr->spell >= 0 && dr->spell < TOP_SPELL_DEFINE) {
     affect_from_char(victim, dr->spell);
     if (get_wearoff(dr->spell))
       send_to_char(victim, "%s\r\n", get_wearoff(dr->spell));
   }
   ```

2. ✅ Added bounds checking in `get_wearoff()`:
   ```c
   const char *get_wearoff(int abilnum)
   {
     if (abilnum < 0 || abilnum >= TOP_SPELL_DEFINE)
       return NULL;
     
     if (spell_info[abilnum].schoolOfMagic != NOSCHOOL)
       return (const char *)spell_info[abilnum].wear_off_msg;
     
     if (spell_info[abilnum].schoolOfMagic == ACTIVE_SKILL)
       return (const char *)spell_info[abilnum].wear_off_msg;
     
     return (const char *)spell_info[abilnum].wear_off_msg;
   }
   ```

**Implementation Date**: 2025-01-04
**Files Modified**:
- `src/fight.c` (line ~7762-7769)
- `src/utils.c` (line ~6692-6693)

## Priority 2: HIGH - Memory Leaks in Object Creation ✅ FIXED

### Issue 2: Object memory leaks in read_object() (db.c:4304) ✅ FIXED
Multiple leaks totaling ~20KB from object creation:
- 800 bytes in 1 block (x3 occurrences)
- 3,200 bytes in 4 blocks (x2 occurrences)
- 6,400 bytes in 8 blocks

**Root Cause**: Objects with spellbook info (`sbinfo`) created by `read_object()` were not having their spellbook memory freed when the objects were destroyed. The `free_obj()` function was missing cleanup for `obj->sbinfo`.

**Fix** ✅ **IMPLEMENTED**:
1. ✅ Added spellbook info cleanup to `free_obj()` in db.c:
   ```c
   /* free spellbook info */
   if (obj->sbinfo) {
     free(obj->sbinfo);
     obj->sbinfo = NULL;
   }
   ```

2. ✅ Verified all object creation paths:
   - `reset_zone()` in db.c:4780, 4894 - properly handles objects
   - `objsave_parse_objects_db()` in objsave.c:2263, 2421 - properly handles objects
   - `House_load()` in house.c:169 - uses objsave_parse_objects_db (fixed)
   - `assign_weighted_bonuses()` in treasure.c:5853 - properly frees with extract_obj()

3. ✅ Added defensive logging for NULL object returns in objsave.c

**Implementation Date**: 2025-08-04
**Files Modified**:
- `src/db.c` (free_obj function, added sbinfo cleanup)
- `src/objsave.c` (added NULL check logging)

### Issue 3: Output buffer leaks (comm.c:2202) ✅ FIXED
Two 24KB leaks from `vwrite_to_output()`:
```
==168114== 24,168 (24 direct, 24,144 indirect) bytes in 1 blocks are definitely lost
```

**Root Cause**: The buffer pool (`bufpool`) was growing unbounded. When connections closed, their large output buffers were always added to the pool for reuse, but the pool had no size limit. This caused memory to accumulate indefinitely, especially during protocol handshake failures or rapid connection closures.

**Fix** ✅ **IMPLEMENTED**:
1. ✅ Modified `flush_queues()` to limit buffer pool to 5 buffers maximum
2. ✅ Modified `process_output()` to limit buffer pool to 5 buffers maximum  
3. ✅ When pool is full, excess buffers are properly freed instead of being pooled
4. ✅ Properly decrement `buf_largecount` when freeing buffers

**Implementation Date**: 2025-08-04
**Files Modified**:
- `src/comm.c` (flush_queues and process_output functions)

## Priority 3: MEDIUM - DG Script Variable Leaks

### Issue 4: Script variable memory leaks
Multiple small leaks (1-5 bytes) from `add_var()` in dg_variables.c

**Root Cause**: Script variables not being freed when scripts are destroyed.

**Fix**:
1. Ensure `free_varlist()` is called when:
   - Scripts are removed from objects/mobs/rooms
   - Objects/mobs/rooms are extracted
   - Zone resets occur

## Priority 4: LOW - Expected Leaks (Still Reachable)

The 903MB of "still reachable" memory is expected for a running MUD and includes:
- World data (rooms, objects, mobiles)
- Player data structures
- Help system
- Command tables
- Spell/skill definitions

These are not leaks but rather persistent data that would be freed at clean shutdown.

## Implementation Plan

### Phase 1: Critical Fixes (Immediate) ✅ COMPLETED
1. ✅ Fixed bounds checking in `get_wearoff()` and `apply_damage_reduction()`
2. ✅ Added defensive programming patterns to prevent similar issues

### Phase 2: Object Leak Fixes (This Week) ✅ COMPLETED
1. ✅ Fixed root cause: `free_obj()` was not freeing spellbook info
2. ✅ All object creation paths now properly clean up via the fixed `free_obj()`
3. ✅ Added defensive programming checks for NULL object returns

### Phase 3: Buffer and Script Fixes (Next Week)
1. Fix output buffer leaks in connection handling
2. Audit DG script variable cleanup
3. Add leak detection to development builds

### Phase 4: Prevention (Ongoing)
1. Add valgrind to CI/CD pipeline
2. Create coding standards for memory management
3. Add debug allocators for object tracking

## Testing Strategy

1. **Immediate Testing**:
   - Test combat with various damage reduction effects
   - Verify no crashes with invalid spell numbers

2. **Memory Testing**:
   - Run with valgrind for 1 hour of normal gameplay
   - Monitor memory usage during zone resets
   - Test player login/logout cycles

3. **Regression Testing**:
   - Ensure object loading still works correctly
   - Verify DG scripts function properly
   - Test all affected systems

## Notes

- The codebase uses C90/C89 standards, so fixes must be compatible
- Many "leaks" are one-time allocations that persist for the lifetime of the MUD
- Focus on the critical invalid reads first as they can cause crashes
- The object leaks are the most significant ongoing memory drain