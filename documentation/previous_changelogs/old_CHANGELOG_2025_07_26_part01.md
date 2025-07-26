# CHANGELOG

## 2025-07-26

### Critical Fix Attempts

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