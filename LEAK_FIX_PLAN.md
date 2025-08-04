# LuminariMUD Memory Leak Fix Plan

## Executive Summary

**Total Errors**: 97 errors from 94 contexts
**Memory Leaked**: 7,829 bytes directly + 217,640 bytes indirectly = 225,469 bytes total
**Critical Issues**: 1 SEGFAULT crash, 4 use-after-free errors
**Non-Critical Issues**: 292 memory leaks in various subsystems

### Severity Breakdown:
1. **CRITICAL**: Use-after-free causing SEGFAULT (lines 89-138)
2. **HIGH**: Invalid reads after memory freed (lines 7-88)
3. **MEDIUM**: Memory leaks in get_enclosing_regions() - 182 blocks
4. **LOW**: Memory leaks in output buffers and object loading

## Prioritized Issue List

### 1. **CRITICAL - SEGFAULT in list management** (lines 119-138) ✅ FIXED
- **Location**: `lists.c:572` in `next_in_list()`
- **Cause**: Reading freed memory (0xcdcdcdcdcdcdcddd pattern indicates freed/corrupted memory)
- **Impact**: Server crash
- **Fix Applied**: Modified `extract_char_final()` in handler.c to use safe two-pass event cancellation pattern instead of using `simple_list()` which maintained a static iterator that became invalid when events were cancelled during traversal

### 2. **HIGH - Use-after-free in combat** (lines 7-46, 47-88) ✅ FIXED
- **Location**: `fight.c:7766` in `apply_damage_reduction()`
- **Cause**: Accessing damage reduction structure after it's been freed by affect_from_char()
- **Impact**: Potential crashes during combat
- **Fix Applied**: Modified apply_damage_reduction() to cache spell number and wearoff message before calling affect_from_char(), preventing use of potentially freed dr structure

### 3. **HIGH - Use-after-free in event list** (lines 89-118) ✅ FIXED
- **Location**: `lists.c:568` in `next_in_list()`
- **Cause**: Event cleanup freeing list items still being traversed
- **Impact**: Crashes during character extraction
- **Fix Applied**: Same fix as #1 - the safe two-pass event cancellation pattern in extract_char_final() resolves this issue

### 4. **MEDIUM - Memory leaks in wilderness/regions** (lines 144-277) ✅ FIXED
- **Location**: `mysql.c:807` in `get_enclosing_regions()`
- **Cause**: Allocated memory never freed
- **Impact**: 2,992 bytes leaked across 187 calls
- **Fix Applied**: Added free_region_list() and free_path_list() functions in mysql.c and updated all callers to free the lists after use

### 5. **MEDIUM - Memory leaks in output buffers** (lines 288-394) ✅ FIXED
- **Location**: `comm.c:2223` in `vwrite_to_output()`
- **Cause**: Output buffers not properly freed
- **Impact**: 217,152 bytes leaked
- **Fix Applied**: Modified vwrite_to_output() to check if t->large_outbuf already exists before allocating a new one, preventing memory leak when multiple large buffers are needed in sequence

### 6. **LOW - Memory leaks in object loading** (lines 209-219, 278-287)
- **Location**: `db.c:4321` in `read_object()`
- **Cause**: Objects loaded but not properly cleaned up
- **Impact**: 4,896 bytes leaked

## Root Cause Analysis

### 1. SEGFAULT in list management (CRITICAL)
```c
// lists.c:572 - The crash occurs here
struct mud_event_list *nextp = curr->next;
```
**Problem**: The code is traversing a linked list while items are being freed during traversal. The pattern 0xcdcdcdcdcdcdcddd suggests memory has been freed and overwritten with debug pattern.

**Root Cause**: `extract_char_final()` is calling `event_cancel()` which frees list nodes, but the list traversal in `simple_list()` continues using freed memory.

### 2. Use-after-free in combat (HIGH)
```c
// fight.c:7764-7766
affect_from_char(ch, af);  // Line 7764 frees the affect
// ... some code ...
if (af->something)          // Line 7766 uses freed affect
```
**Problem**: The affect is removed and freed, but the code continues to use the `af` pointer.

**Root Cause**: Missing check or return after `affect_from_char()` call.

### 3. Memory leaks in get_enclosing_regions (MEDIUM)
```c
// mysql.c:807
regions = calloc(1, sizeof(struct region_list));
```
**Problem**: This allocation is never freed.

**Root Cause**: No cleanup function called for region lists, or missing free in the calling functions.

## Specific Fixes

### Fix 1: SEGFAULT in list management (lists.c) ✅ IMPLEMENTED
**Actual Fix Applied in handler.c:2589-2633**
```c
/* Cancel all events associated with this character */
if (ch->events != NULL)
{
  if (ch->events->iSize > 0)
  {
    struct event *pEvent = NULL;
    struct item_data *pItem = NULL;
    struct item_data *pNextItem = NULL;
    struct list_data *temp_list = NULL;

    /* Create a temporary list to hold events that need to be cancelled */
    temp_list = create_list();

    /* First pass: collect all events into temporary list */
    pItem = ch->events->pFirstItem;
    while (pItem)
    {
      pNextItem = pItem->pNextItem;  /* Cache next pointer */
      pEvent = (struct event *)pItem->pContent;
      
      if (pEvent && event_is_queued(pEvent))
        add_to_list(pEvent, temp_list);
        
      pItem = pNextItem;
    }

    /* Second pass: cancel the collected events using safe iteration */
    pItem = temp_list->pFirstItem;
    while (pItem)
    {
      pNextItem = pItem->pNextItem;  /* Cache next pointer before event_cancel */
      pEvent = (struct event *)pItem->pContent;
      
      if (pEvent)
        event_cancel(pEvent);
        
      pItem = pNextItem;
    }

    /* Clean up the temporary list */
    free_list(temp_list);
  }
  free_list(ch->events);
  ch->events = NULL;
}
```

### Fix 2: Use-after-free in combat (fight.c:7764-7766) ✅ IMPLEMENTED
**Actual Fix Applied in fight.c:7764-7773**
```c
/* Cache the spell number and wearoff message before removing the affect,
 * as affect_from_char may free the dr structure if it has APPLY_DR */
int spell_num = dr->spell;
const char *wearoff_msg = get_wearoff(spell_num);

affect_from_char(victim, spell_num);

/* Use cached values instead of dr-> which may be freed */
if (wearoff_msg)
    send_to_char(victim, "%s\r\n", wearoff_msg);
```

### Fix 3: Memory leak in get_enclosing_regions (mysql.c:807) ✅ IMPLEMENTED
```c
// Added cleanup functions in mysql.c:827-848
void free_region_list(struct region_list *regions) {
    struct region_list *temp;
    
    while (regions) {
        temp = regions;
        regions = regions->next;
        free(temp);
    }
}

void free_path_list(struct path_list *paths) {
    struct path_list *temp;
    
    while (paths) {
        temp = paths;
        paths = paths->next;
        free(temp);
    }
}

// Updated all callers to free the lists after use:
// - wilderness.c:412-413 (get_map)
// - wilderness.c:587-588 (get_modified_sector_type)
// - wilderness.c:813-814 (assign_wilderness_room)
// - wilderness.c:1412-1413 (save_wild_map_to_file - first loop)
// - wilderness.c:1673-1674 (save_wild_map_to_file - second loop)
// - desc_engine.c:358 (gen_room_description)
```

### Fix 4: Memory leak in output buffers (comm.c:2223) ✅ IMPLEMENTED
**Actual Fix Applied in comm.c:2215-2231**
```c
/* Check if we already have a large buffer allocated */
if (t->large_outbuf)
{
  /* We already have a large buffer, just use it */
}
else if (bufpool != NULL)
{
  /* if the pool has a buffer in it, grab it */
  t->large_outbuf = bufpool;
  bufpool = bufpool->next;
}
else
{ /* else create a new one */
  CREATE(t->large_outbuf, struct txt_block, 1);
  CREATE(t->large_outbuf->text, char, LARGE_BUFSIZE);
  buf_largecount++;
}
```

### Fix 5: Memory leak in strdup (players.c:1419)
```c
// players.c:1419 - Fix in load_char()
// Find where this strdup'd string should be freed
// Likely missing a free() in free_char() or similar cleanup function
```

## Implementation Plan

### Phase 1: Critical Fixes (Immediate)
1. **Fix SEGFAULT in lists.c** (Fix 1) ✅ COMPLETED
   - Fixed by implementing safe two-pass event cancellation in extract_char_final()
   - Avoided use of simple_list() during event cancellation to prevent iterator corruption
   - Pattern follows the existing clear_char_event_list() implementation

2. **Fix use-after-free in fight.c** (Fix 2) ✅ COMPLETED
   - Fixed by caching spell number and wearoff message before affect_from_char() call
   - Prevents accessing dr structure after it may have been freed by APPLY_DR removal
   - Pattern ensures combat mechanics remain unchanged

### Phase 2: High Priority (Within 24 hours)
3. **Review all affect_remove/affect_from_char calls**
   - Audit codebase for similar patterns
   - Ensure no pointer usage after removal

4. **Fix event list management** ✅ COMPLETED
   - Fixed as part of Phase 1 - the extract_char_final() fix resolves event list management issues
   - The safe two-pass pattern prevents iterator corruption during event cancellation

### Phase 3: Medium Priority (Within 1 week)
5. **Fix region list memory leaks** (Fix 3)
   - Add free_region_list() function
   - Update all callers of get_enclosing_regions()
   - Consider reference counting if regions are shared

6. **Fix output buffer leaks** (Fix 4)
   - Review descriptor cleanup
   - Ensure all paths free buffers

### Phase 4: Low Priority (Within 2 weeks)
7. **Fix object loading leaks**
   - Review objsave_parse_objects_db()
   - Ensure proper cleanup on errors

8. **Fix misc string leaks**
   - Audit all strdup() calls
   - Ensure matching free() calls

## Testing Strategy

1. **Valgrind Testing**:
   ```bash
   valgrind --leak-check=full --show-leak-kinds=all \
            --track-origins=yes --verbose \
            bin/circle -q 4100
   ```

2. **Stress Testing**:
   - Create/delete many characters rapidly
   - Cast damage reduction spells repeatedly
   - Move through wilderness areas extensively
   - Force combat with multiple NPCs

3. **Regression Testing**:
   - Verify combat mechanics unchanged
   - Verify event system still functions
   - Verify wilderness movement works

## Notes for Implementation

1. **C90 Compliance**: All fixes must declare variables at block start
2. **Null Checks**: Always validate pointers before use
3. **Documentation**: Add comments explaining the fixes
4. **Logging**: Add debug logs for memory operations if needed

## Success Metrics

- No SEGFAULTs in 24 hours of runtime
- Memory leaks reduced by 90% (target: < 25KB leaked)
- No use-after-free errors in valgrind
- All combat and movement systems functional

---

*Generated: 2025-08-04*
*Updated: 2025-08-04 - SEGFAULT fix implemented*
*Priority: CRITICAL - Begin with Phase 1 immediately*

## Fix Summary

### Completed Fixes:
1. **CRITICAL SEGFAULT in list management** - Fixed by implementing safe two-pass event cancellation in extract_char_final()
2. **HIGH Use-after-free in event list** - Fixed by the same implementation (issues #1 and #3 had the same root cause)
3. **HIGH Use-after-free in combat (fight.c:7766)** - Fixed by caching values before affect_from_char() call that could free dr structure
4. **MEDIUM Memory leaks in wilderness/regions** - Fixed by adding free_region_list() and free_path_list() functions and updating all callers
5. **MEDIUM Memory leaks in output buffers** - Fixed by checking if large_outbuf already exists before allocating a new one in vwrite_to_output()

### Remaining High Priority:
1. **Review all affect_remove/affect_from_char calls** - Audit needed to prevent similar issues

### Remaining Medium Priority:
None - All medium priority issues have been resolved

### Remaining Low Priority:
1. **Memory leaks in object loading** - Need to review objsave_parse_objects_db() and ensure proper cleanup on errors
2. **Memory leaks in strdup calls** - Need to audit all strdup() calls for matching free() calls