# CHANGELOG

## 2025-07-25

### Critical Bug Fixes

#### Fixed String Overlap Error in half_chop() Function
- **Issue**: Undefined behavior due to overlapping memory regions in strcpy() call
- **Root Cause**: 
  - In helpers.c:47, `half_chop()` used `strcpy(arg2, temp)` to copy the remainder of the string
  - Many callers pass the same buffer as both the input string and arg2 (e.g., `half_chop(buf2, buf, buf2)`)
  - When temp points into the original string and arg2 is the same buffer, strcpy() has overlapping source/destination
  - This violates strcpy()'s requirement that memory regions must not overlap, causing undefined behavior
- **Solution**: 
  - Replaced `strcpy(arg2, temp)` with `memmove(arg2, temp, strlen(temp) + 1)`
  - memmove() is specifically designed to handle overlapping memory regions safely
  - The +1 ensures the null terminator is properly copied
- **Files Modified**: 
  - helpers.c:47 (half_chop function)
- **Impact**: Eliminates undefined behavior and potential crashes/corruption in command parsing throughout the MUD

#### Fixed Memory Leak in Crafting System reset_current_craft()
- **Issue**: Memory leak when resetting craft descriptions - 24 bytes lost per reset
- **Root Cause**: 
  - In crafting_new.c:2319-2321, the function allocated new strings with strdup() for keywords, short_description, and room_description
  - The old strings were not freed before allocating new ones, causing a memory leak
  - Each call to reset_current_craft() would leak 3 strings ("not set" = 8 bytes each including null terminator)
- **Solution**: 
  - Added checks to free existing strings before allocating new ones
  - Free GET_CRAFT(ch).keywords, short_description, and room_description if they exist
  - Then allocate new strings with strdup() as before
- **Files Modified**: 
  - crafting_new.c:2319-2330 (reset_current_craft function)
- **Impact**: Eliminates memory leaks in the crafting system, preventing memory accumulation during gameplay

#### Fixed Critical Memory Leak in Object Special Abilities System
- **Issue**: Major memory leak in object special abilities - 48+ bytes per object leaked during zone resets
- **Root Cause**: 
  - In `read_object()` at db.c:4025, special abilities were allocated via CREATE() and linked to objects
  - Special abilities included dynamically allocated command_word strings via strdup()
  - When objects were destroyed via `extract_obj()` and `free_obj()`, special abilities were never freed
  - During zone resets (which happen periodically), this caused continuous memory accumulation
- **Solution**: 
  - Created `free_obj_special_abilities()` function to properly free the entire linked list
  - Added call to this function in `free_obj()` for regular object cleanup
  - Added call during shutdown when obj_proto array is cleaned up
  - Function properly frees both the special ability structures and any strdup'd command words
- **Files Modified**: 
  - db.c:5425-5436 (new free_obj_special_abilities function)
  - db.c:5458-5459 (added cleanup in free_obj)
  - db.c:822-824 (added cleanup in obj_proto destruction)
  - db.h:339 (added function declaration)
- **Impact**: Eliminates one of the most significant memory leaks identified in valgrind analysis, preventing memory exhaustion in long-running servers

#### Fixed Critical SEGFAULT in Spell Preparation System
- **Issue**: SEGFAULT crash when clearing spell preparation queues due to use-after-free errors
- **Root Cause**: 
  - In spell_prep.c, functions `clear_prep_queue_by_class()`, `clear_innate_magic_by_class()`, `clear_collection_by_class()`, and `clear_known_spells_by_class()` used unsafe do-while loops
  - After freeing a node with `free(tmp)`, the code continued to access the freed memory in the loop condition
  - This caused reading from address 0xbbbbbbbbbbbbbbcb (freed memory pattern) resulting in SEGFAULT
- **Solution**: 
  - Refactored all four functions to use safe iteration pattern with cached next pointers
  - Changed from do-while loops to while loops that cache the next pointer before freeing
  - Explicitly set the list head to NULL after clearing all nodes
- **Files Modified**: 
  - spell_prep.c:88-100 (clear_prep_queue_by_class)
  - spell_prep.c:101-113 (clear_innate_magic_by_class)
  - spell_prep.c:114-126 (clear_collection_by_class)
  - spell_prep.c:127-139 (clear_known_spells_by_class)
- **Impact**: Eliminates critical crashes during character logout and spell system cleanup, significantly improving server stability

#### Fixed Memory Leaks in String Handling Functions
- **Issue**: Multiple memory leaks in string handling functions causing gradual memory consumption
- **Root Causes and Solutions**: 
  1. **handler.c:134 (isname)** - strdup() allocated memory was not freed on early return
     - Added free(strlist) before returning 0 when isname_tok fails
     - Prevents 5-7 byte leaks on every failed name match
  2. **spell_parser.c:3055 (spello)** - Spell wear-off messages leaked on spell reinitialization
     - Added checks to free previous wear_off_msg before allocating new ones
     - Prevents ~250 instances of 30-40 byte leaks
     - Only frees dynamically allocated strings, not static constants
- **Files Modified**: 
  - handler.c:139-143 (isname function)
  - spell_parser.c:3051-3068 (spello function)
- **Impact**: Eliminates recurring memory leaks in frequently called functions, reducing memory consumption in long-running servers

#### Fixed Critical Use-After-Free in Character Cleanup (free_char)
- **Issue**: Use-after-free crash when freeing characters with active affects during cleanup
- **Root Cause**: 
  - In db.c:5364-5365, `free_char()` removes all affects by calling `affect_remove()` in a loop
  - Each `affect_remove()` call triggers `affect_total()` which calls `affect_total_plus()`
  - During character cleanup, parts of the character structure may already be freed or inconsistent
  - This caused reading from freed memory (18KB inside freed 90KB block) leading to crashes
- **Solution**: 
  - Created new function `affect_remove_no_total()` that removes affects without recalculating totals
  - This function performs all the same cleanup as `affect_remove()` but skips the `affect_total()` call
  - Modified `free_char()` to use `affect_remove_no_total()` during cleanup
- **Files Modified**: 
  - handler.c:1146-1191 (new affect_remove_no_total function)
  - handler.h:23 (added function declaration)
  - db.c:5365 (changed to use affect_remove_no_total)
- **Impact**: Eliminates critical crashes during character deletion, logout, and mob cleanup, significantly improving server stability

#### Fixed Use-After-Free in Event System
- **Issue**: Use-after-free error in event system causing crashes when events were cancelled during iteration
- **Root Cause**: 
  - List iterators held pointers to freed memory when events were cancelled during traversal
  - Functions like `event_cancel_specific()`, `clear_char_event_list()`, and related functions modified lists while iterating
  - When an event was freed, iterators could access the freed memory (8 bytes inside freed 24-byte block)
- **Solution**: 
  - Modified `event_cancel_specific()` to use `simple_list()` instead of iterators for safer iteration
  - Refactored all `clear_*_event_list()` functions to use two-pass approach:
    - First pass: Collect events to cancel into temporary list
    - Second pass: Cancel collected events from temporary list
  - Fixed `change_event_duration()` and `change_event_svariables()` to copy data before cancelling old events
- **Files Modified**: 
  - mud_event.c: event_cancel_specific(), clear_char_event_list(), clear_room_event_list(), clear_region_event_list()
  - mud_event.c: change_event_duration(), change_event_svariables()
- **Impact**: Eliminates crashes from use-after-free errors in the event system, significantly improving server stability

#### Fixed Uninitialized Memory Access in String Reading Functions
- **Issue**: 60 conditional jump errors in valgrind caused by accessing uninitialized memory in fread_string() and fread_clean_string()
- **Root Cause**: 
  - When processing empty strings or strings with only newlines, the code would decrement a pointer before the start of the array
  - Line 4874 in fread_string() had incorrect condition `point == 0` comparing pointer to integer
  - Both functions lacked bounds checking when removing trailing newlines/carriage returns
- **Solution**: 
  - Added bounds check `if (point > tmp)` before decrementing pointer
  - Added condition `(point >= tmp)` in the for loop to prevent underflow
  - Added safety check `if (point >= tmp)` before dereferencing pointer
  - Fixed the incorrect `point == 0` comparison in fread_string()
- **Files Modified**: 
  - db.c:4874-4893 (fread_string function)
  - db.c:4941-4959 (fread_clean_string function)
- **Impact**: Eliminates 60 valgrind errors related to conditional jumps on uninitialized values, improving IBT file loading reliability

#### Fixed "(null)" Race Display in Character Creation
- **Issue**: New players saw "(null)" entries in race selection during character creation, severely impacting first impressions
- **Root Cause**: RACE_GOBLIN and RACE_HOBGOBLIN were only implemented in FR campaign section, not default campaign:
  - Race constants RACE_GOBLIN=26 and RACE_HOBGOBLIN=27 are defined in structs.h
  - But their add_race() implementations were only in the FR campaign section of race.c
  - Default campaign had no implementations for slots 26 & 27, showing as "(null)"
- **Solution**: Copied RACE_GOBLIN and RACE_HOBGOBLIN implementations to default campaign section
- **Files Modified**: race.c:3599-3689 (added Goblin and Hobgoblin race implementations for default campaign)
- **Impact**: Goblin and Hobgoblin races now properly display in default campaign character creation

#### Fixed Critical Double-Free Bug in objsave.c
- **Issue**: Server crash (SIGABRT) when loading house data, with corruption in free_tokens()
- **Root Cause**: The object parsing code in objsave.c was calling `free(*line)` to free individual token strings during parsing, but these strings belonged to the tokenize() array and were freed again by `free_tokens()` at the end, causing a double-free
- **Memory Corruption Pattern**:
  - During parsing, `free(*line)` freed the token strings
  - Memory allocator reused the freed memory for other allocations
  - New allocations wrote pointer values into the old string memory
  - When `free_tokens()` ran, it tried to free these corrupted pointers, causing crash
- **Solution**: 
  - Removed all `free(*line)` calls in parsing loops (lines 2416, 3352, 3976, etc.)
  - Added comments explaining that token strings must not be freed individually
  - Only `free_tokens()` should be called at the end to properly clean up the entire array
- **Files Modified**: 
  - objsave.c: Multiple locations where `free(*line)` was removed
- **Impact**: Eliminates crashes when loading house data, particularly house #24828 which triggered the bug consistently

#### Fixed Critical Memory Allocation Failures in tokenize() Function
- **Issue**: Server crash (SIGABRT) when loading house data due to unchecked memory allocation failures
- **Root Cause**: The `tokenize()` function in mysql.c had no error handling for memory allocation failures:
  - `malloc()` at line 211 could return NULL
  - `realloc()` at lines 218 and 226 could return NULL and leak memory
  - `strdup()` at lines 208 and 220 could return NULL
  - When any allocation failed, NULL or invalid pointers were stored in the array, causing crashes in `free_tokens()`
- **Solution**: 
  - Added comprehensive NULL checks after every memory allocation
  - Implemented proper cleanup on allocation failures to prevent memory leaks
  - Return NULL on error to allow graceful error handling by callers
  - Added detailed error logging to help diagnose memory issues
- **Caller Updates**: Updated all tokenize() callers to handle NULL returns:
  - `objsave.c`: 3 locations (objsave_parse_objects_db, pet_load_objs, objsave_parse_objects_db_sheath)
  - `mud_event.c`: 1 location (event_countdown)
  - `mysql.c`: 3 locations (load_regions, load_paths, envelope)
- **Files Modified**: 
  - mysql.c:206-272 (tokenize function)
  - objsave.c:2126-2132, 3057-3062, 3673-3678
  - mud_event.c:583-587
  - mysql.c:368-375, 729-739, 943-948
- **Impact**: Prevents server crashes from memory allocation failures, provides graceful degradation under low memory conditions
- **Additional Fix**: Fixed C90 compatibility - declared all variables at beginning of blocks (no C99-style declarations)

#### Fixed Critical tokenize() NULL Termination Crash
- **Issue**: Server crash (SIGABRT) when loading house data during boot
- **Root Cause**: The `tokenize()` function in mysql.c:215-226 was improperly NULL-terminating arrays by incrementing count after adding NULL, causing `free_tokens()` to read uninitialized memory and attempt to free garbage pointers
- **Solution**: 
  - Changed loop from `while(1)` to `while(tok)` to only process valid tokens
  - Properly NULL-terminate array after loop without incrementing count
  - Ensure adequate space for NULL terminator with bounds checking
- **Files Modified**: mysql.c:215-229
- **Impact**: Eliminates deterministic crash during server boot when loading house #24828, restoring server stability

### Compiler Warning Fixes

#### Fixed All Remaining Compiler Warnings (32 warnings eliminated)
- **Format Truncation Warnings**:
  - `act.informative.c`: Increased buffer sizes for keyword1 (100→128) and dex_max (10→20)
  - `act.wizard.c`: Increased tmp_buf size from 1024 to 8192 to handle large format strings
  - `char_descs.c`: Increased final buffer from 256 to 512 to handle concatenated strings
  - `fight.c`: Increased buf size from 10 to 20 for integer formatting
  - `limits.c`: Increased buf size from 200 to 256 for spell name formatting
  - `roleplay.c`: Increased buf2 size from 100 to 200 for roleplay info formatting
  - `utils.c`: Increased temp_buf (200→256) and line_buf (200→256) for HP calculations
- **String Operation Warnings**:
  - `act.item.c`: Replaced strncat with memcpy to avoid compiler warning about length dependency
  - `ban.c`: Fixed strncpy truncation by reserving space for null terminator
  - `db.c`: Added null termination after strncpy in two locations (zone names and object descriptions)
  - `genolc.c` & `genwld.c`: Added null termination after strncpy for room descriptions
- **Address Comparison Warnings**:
  - `act.wizard.c`, `class.c`, `magic.c`, `spells.c`: Removed redundant NULL checks for array addresses (AFF_FLAGS)
  - `players.c`: Removed redundant NULL check for host array member
  - Arrays cannot be NULL by definition, making these checks unnecessary
- **Impact**: Clean compilation with zero warnings, improving code quality and maintainability
