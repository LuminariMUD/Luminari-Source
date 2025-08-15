# Shop System Issues - Investigation Report

## Problem Description
After recent automated memory leak fixes, shops are displaying no goods until the room is purged and the zone is reloaded.

## UPDATE: Root Cause Identified and Fix Attempted

### Root Cause
**Location**: `src/db.c:1317-1322`

The boot sequence cleanup code was incorrectly deleting ALL objects with `in_room == NOWHERE` without checking if they were actually orphaned. This deleted all shopkeeper inventory because:

1. When objects are given to mobs via the 'G' zone command, `obj_to_char()` is called
2. `obj_to_char()` correctly sets `in_room = NOWHERE` (src/handler.c:1609) because the object is carried, not in a room
3. The cleanup code at src/db.c:1317-1322 was extracting ALL objects with `in_room == NOWHERE`
4. This deleted all shopkeeper inventory thinking it was orphaned

### Functions Involved
1. **`boot_db()`** in src/db.c:1304-1334 - Contains the buggy cleanup code
2. **`reset_zone()`** in src/db.c:4699-5462 - Executes 'G' commands to give items to shopkeepers
3. **`obj_to_char()`** in src/handler.c:1598-1613 - Correctly sets in_room = NOWHERE for carried objects

### Fix Applied
Modified the cleanup code to only extract truly orphaned objects by checking all attachment points:
```c
if (j->in_room == NOWHERE && 
    j->carried_by == NULL && 
    j->worn_by == NULL && 
    j->in_obj == NULL)
{
    // Only NOW is it truly orphaned
    extract_obj(j);
}
```

### Testing Required
1. Restart the MUD server
2. Check if shops now have inventory immediately after boot
3. Verify no regression in other systems
4. Monitor for any new orphaned object warnings in logs

## Investigation Summary

### Key Components Analyzed
1. **shop.c** - Main shop system implementation
2. **db.c** - Zone reset and object loading
3. **act.wizard.c** - Purge and zone reset commands
4. **Shop initialization** (`boot_the_shops`, `assign_the_shopkeepers`)
5. **Shop keeper special procedure** (`shop_keeper`)
6. **Zone reset process** (`reset_zone`)

### How Shops Work
1. **Shop Data Structure**: Shops have a `producing` list of items they can create unlimited copies of
2. **Initial Population**: Shops DO NOT automatically populate with their producing items at boot
3. **Inventory Management**: 
   - Zone reset 'G' commands give items to shopkeepers
   - `sort_keeper_objs` manages inventory sorting
   - `SHOP_SORT` counter tracks sorted vs unsorted items
4. **Producing Items**: When a producing item is sold, it's extracted rather than moved to keeper

## Potential Root Causes

### Theory 1: Zone Reset State Race Condition (MOST LIKELY)
**Location**: `src/db.c:4711-4718`

The code has zone reset state tracking to prevent race conditions:
```c
if (zone_table[zone].reset_state == ZONE_RESET_ACTIVE) {
    log("SYSERR: Zone %d already resetting - possible race condition detected!");
    return;
}
```

**Problem**: If a memory leak fix freed zone reset data prematurely or changed timing, the zone reset might be:
- Exiting early due to false positive race detection
- Not properly clearing the reset state after completion
- Being called multiple times during boot, causing early exits

### Theory 2: Object Creation Failure in Zone Reset
**Location**: `src/db.c:4925-4956` (G command handling)

The 'G' command gives objects to mobs. Recent bounds checking might cause:
- Silent failures when creating objects for shopkeepers
- Objects being created but not properly attached to keeper
- Validation failures preventing object creation

### Theory 3: Shop Keeper Inventory Cleanup Issue
**Location**: `src/shop.c:895-921` (`sort_keeper_objs`)

The sort function reorganizes keeper inventory. Potential issues:
- Memory leak fix might have freed objects still referenced
- Double-free causing objects to disappear
- Incorrect sorting leaving objects in limbo

### Theory 4: Shop Initialization Order Problem
**Location**: `src/shop.c:1500-1530` (`assign_the_shopkeepers`)

Shop keepers are assigned after shops are loaded. Issues might include:
- Shop keeper prototype modifications not persisting
- Gold amount (100,000) being set but inventory not loading
- Special procedure assignment interfering with object loading

### Theory 5: Object Extraction During Load
**Location**: `src/db.c:1307-1322`

After zone reset, there's cleanup code that removes objects in NOWHERE:
```c
if (j->in_room == NOWHERE) {
    extract_obj(j);
    continue;
}
```

**Problem**: Shop keeper inventory might temporarily be in NOWHERE state during loading

## Why Purge + Reload Works

When you purge and reload:
1. **Purge**: Removes all objects and mobs from the room
2. **Zone Reset**: Re-runs all zone commands including:
   - Loading fresh shopkeeper mob (M command)
   - Giving items to shopkeeper (G commands)
3. **Fresh State**: No existing inventory to sort/manage, clean slate

This suggests the problem is with:
- Initial loading sequence at boot time
- State management during first zone reset
- Timing issues with multiple resets during boot

## Recommended Fixes

### Priority 1: Check Zone Reset State Management
```c
// In src/db.c, after reset_zone completes:
void reset_zone(zone_rnum zone) {
    // ... existing code ...
    
    // CRITICAL: Clear reset state when done
    zone_table[zone].reset_state = ZONE_RESET_IDLE;
    zone_table[zone].reset_end = time(0);
}
```

### Priority 2: Verify Shop Keeper Object Loading
Add debug logging to track:
1. When shopkeepers are created
2. When objects are given to them via G commands
3. State of keeper->carrying after zone reset

### Priority 3: Check Boot Sequence
The boot sequence runs reset_zone twice:
1. Line 1289: Initial reset of all zones
2. Line 1326: Second reset after cleanup

This double-reset might be causing issues if reset state isn't properly managed.

## Testing Recommendations

1. **Add Debug Logging**:
   - Log when shops are assigned keepers
   - Log G command execution for shop keepers
   - Log keeper inventory count after zone reset

2. **Test Single Zone**:
   - Boot with mini-mud mode
   - Reset single shop zone
   - Check keeper inventory immediately

3. **Compare Working vs Broken**:
   - Save keeper inventory state after successful purge/reload
   - Compare with initial boot state
   - Identify missing objects

## Memory Leak Fix Suspects

Look for recent changes to:
1. `free()` calls in shop.c (lines 1818-1845)
2. `extract_obj()` calls during zone reset
3. Object list management in `obj_to_char()`
4. Zone command queue management
5. Reset queue (result_q) handling

## Conclusion

The issue appears to be a timing or state management problem during initial boot/reset, where shopkeeper inventory isn't being properly populated. The fact that purge+reload fixes it suggests the zone reset mechanism works, but something in the initial boot sequence or state management is preventing proper inventory loading.

The most likely culprit is improper zone reset state management or premature object cleanup during the boot sequence's double zone reset.