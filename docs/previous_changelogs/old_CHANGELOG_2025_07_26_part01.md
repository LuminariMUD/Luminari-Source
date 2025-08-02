# CHANGELOG

## 2025-07-26

### Critical Fix Attempts


### Critical Memory Fix Attempt - Mobile Activity Processing
- **Issue**: Use-after-free errors and extreme CPU usage (935%) in mobile_activity
- **Root Cause**: Characters marked for extraction (MOB_NOTDEADYET) were still being processed
- **Fix Attempted**:
  - Skip mobs with MOB_NOTDEADYET flag at start of mobile_activity loop
  - Add defensive checks after perform_move and hit calls that may extract mob
  - Validate character still exists after operations that trigger scripts
- **Files**: mobact.c

### Critical Memory Fix Attempt - Combat System Character Access After Free
- **Issue**: Use-after-free in fight.c:5043 (last_attacker) and act.item.c:2206 (PRF_FLAGGED)
- **Fix Attempted**:
  - Added character validation loop in damage() to verify last_attacker still valid
  - Added NULL/IS_NPC checks in perform_get_from_container()
  - Clear all last_attacker references in extract_char_final()
- **Files**: fight.c, act.item.c, handler.c

### Critical Memory Fix Attempt - Object Timer Trigger Use-After-Free
- **Issue**: Use-after-free in point_update (limits.c:2116,2118) when timer triggers opurge themselves
- **Fix Attempted**:
  - Changed timer_otrigger() to return int indicating if object was purged
  - Check dg_owner_purged flag and skip object if self-purged
- **Files**: dg_triggers.c, dg_scripts.h, limits.c

### Critical Memory Fix Attempt - DG Script Use-After-Free
- **Issue**: Use-after-free in script_driver (dg_scripts.c:2817) when triggers detach themselves
- **Fix Attempted**:
  - Added !dg_owner_purged check to script execution loop
  - Modified process_detach() to detect and flag self-detachment
- **Files**: dg_scripts.c

### String Memory Management Fix Attempt
- **Issue**: Memory leaks from strdup() calls without corresponding free()
- **Root Cause**: CAP() macro modifies string in-place, but when used with strdup() creates unreferenced temporary strings
- **Fix Attempted**:
  - Fixed memory leak in craft.c:1796 - CAP(strdup(obj->short_description)) now uses temporary variable
  - Fixed memory leak in act.wizard.c:8930 - CAP(strdup(player_table[i].name)) now uses temporary variable
  - Audited string lifecycle management across character and object systems
  - Verified free_char() and free_obj() properly clean up all allocated strings
- **Files**: craft.c, act.wizard.c
- **Impact**: Should reduce "definitely lost" memory reported by valgrind

### Trigger System Memory Investigation
- **Issue**: Suspected memory leaks in trigger allocation/deallocation
- **Investigation**: Comprehensive review of trigger memory management
- **Findings**:
  - Trigger system memory management is correctly implemented
  - All destruction functions properly call extract_script() and free_proto_script()
  - Trigger cmdlists are shared between instances and correctly freed only with prototypes
  - No memory leaks identified in the trigger system
- **Action Taken**: Added clarifying comment to free_trigger() about cmdlist memory model
- **Files**: dg_handler.c (comment update only)

### Craft System Double-Free Fix Attempt
- **Issue**: Use-after-free in free_craft() when iterating through requirements list
- **Root Cause**: Calling next_in_list() after remove_from_list() accessed freed memory
- **Fix Attempted**:
  - Modified free_craft() to save next item pointer before removing current item
  - Changed loop structure to prevent accessing freed list nodes
  - Pattern: Get next_r, then remove/free current r, then set r = next_r
- **Files**: crafts.c (free_craft function, lines 85-106)
- **Impact**: Should eliminate double-free errors in craft system cleanup

### Pet Data SQL Syntax Error Fix Attempt
- **Issue**: SQL syntax error in pet data INSERT statement - double comma causing invalid SQL
- **Root Cause**: Format string in snprintf started with comma when previous code already added one
- **Fix Attempted**:
  - Removed leading comma from query3 format string in players.c:4144
  - Changed from `",'%d','%d'..."` to `"'%d','%d'..."`
- **Files**: players.c (line 4144)
- **Impact**: Should resolve SQL syntax errors when saving pet data to database

### Character Follower Cleanup Fix Attempt
- **Issue**: Use-after-free in destroy_db when calling stop_follower() on freed characters
- **Root Cause**: stop_follower() sends messages to master/followers who may already be freed
- **Fix Attempted**:
  - Split character cleanup into two passes in destroy_db()
  - First pass: Clear all follower/master relationships without sending messages
  - Second pass: Free all characters after relationships are cleared
  - Prevents accessing freed character data during shutdown
- **Files**: db.c (destroy_db function, lines 752-784)
- **Impact**: Should eliminate use-after-free errors during game shutdown

### Overlapping strcpy Fix Attempt
- **Issue**: Source and destination overlap in strcpy causing undefined behavior
- **Root Cause**: find_all_dots() used strcpy(arg, arg + 4) with overlapping memory regions
- **Fix Attempted**:
  - Replaced strcpy with memmove which correctly handles overlapping regions
  - Changed from `strcpy(arg, arg + 4)` to `memmove(arg, arg + 4, strlen(arg + 4) + 1)`
- **Files**: handler.c (find_all_dots function, line 3098)
- **Impact**: Should eliminate undefined behavior when parsing "all.item" commands


#### Use-After-Free in Spell Preparation System (Latest Attempt)
- **Issue**: Segfault when accessing freed spell preparation data during character cleanup
- **Cause**: Functions called on already-freed character memory (valgrind: 0xbbbbbbbbbbbbbbcb pattern)
- **Attempted Fix**: Added NULL checks in all spell cleanup functions
- **Files Modified**: spell_prep.c (lines 158-192), account.c:656

#### Use-After-Free in Spell Preparation System (Earlier Attempt)
- **Issue**: Crashes during character cleanup/logout/death
- **Cause**: player_specials freed before spell cleanup functions called
- **Attempted Fix**: Reordered cleanup operations in free_char()
- **Files Modified**: db.c:5334-5341, spell_prep.c, account.c:686

#### Event System Memory Corruption
- **Issue**: Use-after-free when iterating character events
- **Cause**: simple_list() iterator invalid after event_cancel() frees current item
- **Attempted Fix**: Replaced with clear_char_event_list() using cached next pointers
- **Files Modified**: db.c:5391, mud_event.c:1478-1525

#### Player Death Crash - Heap Corruption (Attempt #6)
- **Issue**: malloc_consolidate error on player death
- **Cause**: Uninitialized next pointer in affected_type contained garbage
- **Attempted Fix**: Initialize next pointer to NULL in new_affect()
- **Files Modified**: utils.c:4486, fight.c (multiple locations)

#### Player Death Crash - Position Update (Attempt #5)
- **Issue**: malloc_consolidate error on player death
- **Theory**: update_pos() changes POS_DEAD to POS_RESTING, causing combat to continue
- **Attempted Fix**: Skip update_pos() if target is POS_DEAD
- **Files Modified**: fight.c:11564-11565, 11574-11575

### Memory Leak Fix Attempts

#### Character and Mob Cleanup
- **Issue**: 21.6MB leaks (5.6MB definitely lost)
- **Attempted Fixes**:
  - Added free(ch->bags) in free_char()
  - Added ECHO_ENTRIES cleanup in destroy_db()
- **Files Modified**: db.c:5346-5347, db.c:862-870

#### Uninitialized Values in save_char()
- **Issue**: Conditional jumps on uninitialized snum[100] array
- **Attempted Fix**: Initialize array with {0}
- **Files Modified**: players.c:2755

### Minor Fix Attempts

#### Shutdown Warning
- **Issue**: "Attempting to merge iterator to empty list" on group disband
- **Attempted Fix**: Check merge_iterator() result before iterating
- **Files Modified**: handler.c:3151-3159

### Memory Leak Fix Attempts (Later Session)

#### Craft Data Memory Leak
- **Issue**: GET_CRAFT(ch).ex_description allocated with strdup() but never freed (7 bytes per occurrence)
- **Cause**: Missing free() calls in reset_current_craft() and free_char()
- **Attempted Fix**: 
  - Added free(GET_CRAFT(ch).ex_description) in reset_current_craft() before setting to NULL
  - Added craft string cleanup in free_char() before freeing player_specials
- **Files Modified**: crafting_new.c:2326,3266, db.c:5353-5360
- **Status**: Unverified - requires testing to confirm fix resolves the issue

#### Quest Command Memory Leak
- **Issue**: boot_the_quests() leaking 15,744 bytes in 656 blocks
- **Cause**: Quest commands created but not attached to quest lists when inner[0] is neither 'I' nor 'O'
- **Attempted Fix**: Added default case in switch statement to free orphaned quest commands
- **Files Modified**: hlquest.c:987-991
- **Status**: Unverified - requires testing to confirm fix resolves the issue

#### Boot-time String Allocations (Reviewed)
- **Issue**: weapon_type[] and race_list[] strings allocated at boot but never freed
- **Files**: assign_wpn_armor.c:922, race.c:186-190
- **Status**: Reviewed (2025-07-26) - these are "still reachable" leaks that persist for game lifetime
- **Note**: These allocations are used throughout the game and freed on program exit
- **Decision**: No fix attempted - intentional persistent allocations

#### Object Save Function Memory Leaks
- **Issue**: objsave_save_obj_record_db_sheath() and objsave_save_obj_record_db_pet() creating temporary objects without freeing in error paths
- **Cause**: Missing extract_obj(temp) calls before early returns on MySQL query failures
- **Attempted Fix**: 
  - Added extract_obj(temp) in objsave_save_obj_record_db_sheath() before error return
  - Added extract_obj(temp) in objsave_save_obj_record_db_pet() before error return
- **Files Modified**: objsave.c:3601, objsave.c:2976
- **Status**: Unverified - requires testing to confirm fix resolves the issue