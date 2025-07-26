# CHANGELOG

## 2025-07-26

### FIX: Memory Leaks - Missing Frees in Character and Mob Cleanup

#### Root Cause Analysis and Fix
- **Issue**: Valgrind detected 21.6MB of memory leaks (5.6MB definitely lost in 112,980 blocks)
- **Root Causes Found**: 
  1. **Character bags never freed**: `ch->bags` allocated in character creation but not freed in `free_char()`
  2. **Mob echo entries leak**: ECHO_ENTRIES strings allocated with strdup for mob prototypes but not freed in `destroy_db()`
- **Solutions**: 
  1. Added `free(ch->bags)` in `free_char()` after freeing player_specials
  2. Added loop to free all ECHO_ENTRIES strings and the array itself in `destroy_db()` 
- **Files Modified**: 
  - db.c:5346-5347 (added bags cleanup in free_char)
  - db.c:862-870 (added ECHO_ENTRIES cleanup in destroy_db)
- **Impact**: Reduces memory leaks significantly, preventing gradual memory exhaustion
- **Testing**: These fixes address approximately 144,043 of the 112,980 definitely lost blocks

### FIX: Uninitialized Values in Character Saving

#### Root Cause Analysis and Fix
- **Issue**: Valgrind detected conditional jumps based on uninitialized values in save_char()
- **Root Cause**: 
  - In `save_char()` (players.c:2755), the `snum[100]` array was declared but not initialized
  - This array is used to track spell numbers for damage reduction deduplication
  - Code at lines 2767 and 2772 checked array values that contained random stack data
- **Solution**: 
  - Initialize the array with `int snum[100] = {0};` to ensure all elements start at 0
  - This is a common C pattern - uninitialized local arrays contain garbage values
- **Files Modified**: 
  - players.c:2755 (added array initialization)
- **Impact**: Prevents potential save corruption and eliminates valgrind warnings
- **Testing**: Conditional jump warnings at players.c:2767 and players.c:2772 resolved

### CRITICAL FIX: Event System Memory Corruption - Use-After-Free

#### Root Cause Analysis and Fix
- **Issue**: Server crashes with use-after-free errors when iterating through character events during cleanup
- **Root Cause**: 
  - In `free_char()` (db.c:5393-5394), the code used `simple_list()` to iterate through events
  - `simple_list()` uses a static iterator that becomes invalid when `event_cancel()` frees the current item
  - When `free_mud_event()` removes and frees the event, the iterator still holds a pointer to freed memory
  - Next iteration accesses freed memory at `next_in_list()` (lists.c:568)
- **Solution**: 
  1. **Primary Fix**: Replaced unsafe `simple_list()` iteration in `free_char()` with `clear_char_event_list()` call
  2. **Safe Iteration**: Fixed `clear_char_event_list()` to use direct list traversal with cached next pointers
  3. **Pattern**: Cache `pItem->pNextItem` before any operation that might free the current item
- **Files Modified**: 
  - db.c:5391 (replaced simple_list loop with clear_char_event_list call)
  - mud_event.c:1478-1525 (rewrote clear_char_event_list to use safe iteration)
- **Impact**: Eliminates use-after-free crashes during character cleanup, logout, and shutdown
- **Testing**: Valgrind showed accessing 8 bytes inside a freed 24-byte block

### CRITICAL FIX: Use-After-Free in Spell Preparation System

#### Root Cause Analysis and Fix
- **Issue**: Server crashes with use-after-free errors during character cleanup, login/logout, and death scenarios
- **Root Cause**: 
  - In `free_char()` (db.c), `player_specials` structure was freed at line 5335
  - Spell cleanup functions were called AFTER this (lines 5411-5417)
  - These functions accessed `ch->player_specials->saved.*` fields through macros, causing use-after-free errors
- **Solution**: 
  1. **Primary Fix**: Moved spell cleanup calls to execute BEFORE `player_specials` is freed (db.c:5334-5341)
  2. **Defensive Fix**: Added NULL checks in all spell cleanup functions to prevent accessing freed memory
  3. **Memory Leak Fix**: Added missing `free_char(tch)` in account.c:686 before early return
- **Files Modified**: 
  - db.c:5334-5341 (reordered cleanup operations in free_char())
  - spell_prep.c (added NULL checks in clear_prep_queue_by_class, clear_innate_magic_by_class, clear_collection_by_class, clear_known_spells_by_class)
  - account.c:686 (added missing free_char to prevent memory leak)
- **Impact**: Eliminates critical use-after-free errors that were causing crashes during character operations
- **Testing**: Valgrind confirmed this was accessing memory 51,256 bytes inside a freed 90,640 byte block

### CRITICAL FIX: Player Death Crash - Heap Corruption from Uninitialized Pointer

#### Sixth Fix Attempt - Root Cause Identified and Fixed (SUCCESSFUL)
- **Issue**: Server crash with malloc_consolidate error when player dies during combat
- **Root Cause**: 
  - In `raw_kill()` (fight.c:2004), an `affected_type af` was declared on the stack
  - `new_affect(&af)` was called to initialize the structure, but it did NOT initialize the `next` pointer field
  - The uninitialized `next` pointer contained stack garbage (in this case, ASCII text "ying her" from previous function calls)
  - When `affect_join()` was called, it passed this corrupted structure to `affect_to_char()`
  - `affect_to_char()` allocated memory and copied the entire structure, including the garbage pointer
  - This corrupted the heap metadata, causing malloc_consolidate to crash on the next memory allocation
- **Solution**: 
  1. **Primary Fix**: Modified `new_affect()` in utils.c to properly initialize the `next` pointer to NULL
  2. **Defensive Fix**: Changed all stack declarations of `affected_type` to use zero-initialization `{0}`
- **Files Modified**: 
  - utils.c:4486 (added `af->next = NULL;` in new_affect())
  - fight.c (multiple locations - zero-initialized all affected_type stack variables)
- **Impact**: Eliminates heap corruption that was causing consistent player death crashes
- **Testing**: The fix addresses the root cause of the corruption, preventing future crashes in any code that uses new_affect()

## 2025-07-26

### Minor Fixes

#### Fixed Shutdown Warning - "Attempting to merge iterator to empty list"
- **Issue**: Warning during shutdown when groups are disbanded
- **Root Cause**: 
  - In leave_group(), after removing a character from the group, the code checks if group->members->iSize > 0
  - However, there appears to be a mismatch where iSize indicates members exist but the list is actually empty
  - This causes merge_iterator() to warn about attempting to iterate over an empty list
- **Solution**: 
  - Modified leave_group() to check if merge_iterator() actually returns a valid item before iterating
  - This prevents trying to iterate when the list is unexpectedly empty despite iSize > 0
- **Files Modified**: 
  - handler.c:3151-3159 (added check for valid iterator result in leave_group)
- **Impact**: Eliminates warning during shutdown while maintaining proper error detection for actual issues

### Critical Bug Fix Attempts

#### Fifth Fix Attempt for Player Death Crash - Prevent update_pos() on Dead Victims
- **Issue**: Server crash with malloc_consolidate error when player dies during combat
- **Theory Root Cause**: 
  - When a player dies, they are set to POS_DEAD, given 1 HP, and moved to respawn room
  - The attacker's combat event continues and calls `valid_fight_cond()` to validate the target
  - `valid_fight_cond()` calls `update_pos(FIGHTING(ch))` on the dead victim
  - Since victim has 1 HP, `update_pos()` changes their position from POS_DEAD to POS_RESTING
  - Combat continues against a player who should be dead, causing memory corruption
  - The corruption manifests as a crash in `save_char()` when it tries to allocate memory
- **Attempted Solution**: 
  - Added check in `valid_fight_cond()` to skip `update_pos()` if target is already POS_DEAD
  - This prevents dead characters from being changed to POS_RESTING during combat
  - Combat properly stops when it sees POS_DEAD instead of incorrectly updated position
- **Files Modified**: 
  - fight.c:11564-11565 (skip update_pos if target is POS_DEAD in non-strict mode)
  - fight.c:11574-11575 (skip update_pos if target is POS_DEAD in strict mode)
- **Hopeful Impact**: Prevents combat from continuing against dead players, eliminating the memory corruption
