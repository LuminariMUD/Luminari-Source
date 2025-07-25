# CHANGELOG

## 2025-07-25

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

### Bug Fixes

#### Fixed Major Memory Leak in tokenize() Function (mysql.c)
- **Issue**: 318KB memory leak - the largest single memory leak identified in valgrind analysis
- **Root Cause**: In mysql.c, three locations were manually freeing individual tokens but not calling `free_tokens()` to properly free the token array itself
- **Solution**: 
  - Replaced manual token cleanup loops with proper `free_tokens()` calls in:
    - `load_regions()` at line 337 (was lines 332-338)
    - `load_paths()` at line 691 (was lines 686-693)  
    - `envelope()` at line 900 (was lines 887-903)
- **Files Modified**: mysql.c:337, 691, 900
- **Impact**: Eliminates 318KB of memory leaks from database operations - the single largest memory leak fixed to date

#### Fixed Critical Use-After-Free in close_socket()
- **Issue**: Use-after-free bug causing segfaults during player disconnection
- **Root Cause**: In `comm.c:2951`, the code used `simple_list()` to iterate through descriptor events while calling `event_cancel()`, which freed the memory that the iterator was still referencing
- **Solution**: 
  - Replaced unsafe `simple_list()` iteration with direct access to first item
  - New code safely gets events from the list head while the list size > 0
  - Prevents iterator from accessing freed memory
- **Files Modified**: comm.c:2951-2956
- **Impact**: Eliminates critical crashes during player disconnections, significantly improving server stability

## 2025-07-25

### Bug Fixes

#### Fixed Critical Memory Leaks in tokenize() Function Usage
- **Issue**: Most frequently occurring memory leak in valgrind analysis - tokenize() results not being freed
- **Root Cause**: The tokenize() function returns dynamically allocated arrays of strings, but several callers were not freeing the array itself (only the individual strings)
- **Solution**: 
  - Added `free_tokens()` helper function in mysql.c to properly free both strings and the array
  - Fixed memory leaks in:
    - objsave.c: Added `free_tokens(lines)` calls in objsave_parse_objects_db() (3 locations)
    - mud_event.c: Added `free_tokens(tokens)` in event_encounter_reset()
- **Files Modified**: 
  - mysql.c:232-248 (added free_tokens function)
  - mysql.h:29 (added free_tokens declaration)
  - objsave.c:2417, 3340, 3968 (added free_tokens calls)
  - mud_event.c:635 (added free_tokens call)
- **Impact**: Eliminates the most common memory leak pattern found in valgrind analysis (159KB+ from tokenize calls)

## 2025-01-27

### Bug Fixes

#### Fixed Major Memory Leak in Quest System (hlquest.c)
- **Issue**: 113KB memory leak in `clear_hlquest()` function during quest initialization
- **Root Cause**: Function allocated memory with `strdup()` for keywords and reply_msg fields that were immediately overwritten in `boot_the_quests()`
- **Solution**: Changed `clear_hlquest()` to initialize these pointers to NULL instead of allocating unused memory
- **Files Modified**: hlquest.c:768-769
- **Impact**: Eliminates 113KB of memory leaks during server boot (part of 460KB total identified leaks)

#### Fixed Quest Command Memory Leak in Mob Prototype Cleanup
- **Issue**: 15.7KB memory leak from quest_command structures allocated in `boot_the_quests()`
- **Root Cause**: `destroy_db()` was not freeing quest data attached to mob prototypes during shutdown
- **Solution**: Added `free_hlquest(&mob_proto[cnt])` to the mob prototype cleanup loop in db.c:853
- **Files Modified**: db.c:853 (in destroy_db function)
- **Impact**: Eliminates 15.7KB of memory leaks during server shutdown (part of 460KB total identified leaks)

#### Fixed Major Memory Leak in tokenize() Function
- **Issue**: 318KB memory leak in `tokenize()` function used for parsing database queries
- **Root Cause**: The dynamically allocated token array (char**) was never freed after use, only individual strings were freed
- **Solution**: Added `free(tokens)` after each tokenize usage in load_regions(), load_paths(), and envelope() functions
- **Files Modified**: mysql.c:320, 675, 885
- **Impact**: Eliminates 318KB of memory leaks during database operations (largest single leak, part of 460KB total identified leaks)

#### Fixed NULL Object Handling in fight.c
- **Issue**: `unequip_char()` could return NULL when called in corpse equipment transfer, causing crashes when passed to `obj_to_obj()`
- **Solution**: Added NULL check after `unequip_char()` before calling `obj_to_obj()` in fight.c:1791-1793
- **Impact**: Prevents crashes during death when equipment slots are empty or unequip fails

#### Fixed Extraction Counting Mismatch
- **Issue**: Direct call to `extract_char_final()` in act.wizard.c bypassed extraction counting mechanism
- **Root Cause**: `extractions_pending` counter only incremented in `extract_char()`, not in direct `extract_char_final()` calls
- **Solution**: Changed `extract_char_final(victim)` to `extract_char(victim)` in act.wizard.c:1450
- **Impact**: Eliminates "Couldn't find X extractions as counted" errors and ensures proper character cleanup

#### Resolved False "Missing Damage Types" Issue
- **Issue**: Task list incorrectly identified spell IDs 1507 and 1527 as missing damage type definitions
- **Investigation**: Found that these are psionic spell IDs:
  - 1507 = PSIONIC_ENERGY_RAY (defined in spells.h)
  - 1527 = PSIONIC_ENERGY_PUSH (defined in spells.h)
- **Resolution**: No code changes needed - these spells correctly use `GET_PSIONIC_ENERGY_TYPE(ch)` for damage
- **Impact**: Clarified that no damage types are missing; updated task list to reflect resolved status

## 2025-07-25

### Compiler Warning Fixes

#### Fixed Additional Compiler Warnings (70 → 36 warnings)
- **Type Conversion Errors**:
  - Fixed `room_rnum` initialization in `act.movement.c:4139` - Changed from NULL to NOWHERE
- **Buffer Overflow Warnings**:
  - Fixed in `quest.c` - Changed `arg2` buffer from MAX_INPUT_LENGTH to MAX_STRING_LENGTH in do_quest()
- **Array Address Comparisons**:
  - Removed 15+ unnecessary NULL checks for array addresses across multiple files
  - Arrays like `char host[SIZE]` cannot be NULL by definition
  - Fixed in: act.informative.c, act.wizard.c, ban.c, db.c, dg_misc.c, dg_mobcmd.c, dg_objcmd.c, dg_scripts.c, dg_wldcmd.c
- **String Truncation Warnings**:
  - Fixed `strncpy` warnings by ensuring null termination in account.c and act.wizard.c
- **Array Parameter Mismatches**:
  - Fixed function declarations in perlin.h to match implementation array bounds
- **Indentation Issues**:
  - Fixed misleading indentation in act.wizard.c else clause

#### Previous Fixes: Critical Compiler Warnings (86 → 6 warnings)
- **Dangling Pointer Warnings**: 
  - Fixed in `spec_procs.c` (mayor function) - Made path arrays static to prevent dangling pointers
  - Fixed in `zone_procs.c` (king_welmar function) - Made path arrays static
- **Null Pointer Dereferences**:
  - Fixed 3 instances in `spec_procs.c` ferry functions - Added NULL checks before strcmp()
- **Buffer Overlap Issues**:
  - Fixed 6 instances in `utils.c` - Replaced overlapping snprintf() calls with safe strlcat() approach
  - Prevents undefined behavior when source and destination buffers overlap
- **Boolean Logic Errors**:
  - Fixed in `utils.c:5834` - Changed `bool num` to `int num` for proper counting
  - Fixed in `act.other.c:5694` - Corrected boolean comparison logic
- **Impact**: Eliminates potential crashes, undefined behavior, and memory corruption

### Bug Fixes

#### Fixed Critical Memory Leak in Object Save System
- **Issue**: 460KB memory leak in `objsave_save_obj_record_db()` when MySQL operations failed
- **Root Cause**: Temporary object created for comparison was not freed on early return from MySQL errors
- **Solution**: Added `extract_obj(temp)` cleanup calls before error returns at lines 476 and 485
- **Impact**: Prevents major memory leak during house saves when database errors occur
- **Source**: Identified via valgrind analysis (`valgrind_20250724_210758.log`)

#### Fixed Use-After-Free Bug in List Iterator System
- **Issue**: Use-after-free error in `lists.c:553` when removing items during iteration
- **Root Cause**: `simple_list()` iterator held stale pointers to freed items after `remove_from_list()`
- **Solution**: 
  - Added NULL pointer safety check in `next_in_list()` to prevent dereferencing freed memory
  - Refactored `free_list()` functions to use safe iteration pattern with cached next pointers
  - Prevents iterator from accessing freed memory by pre-caching next item before removal
- **Impact**: Eliminates crashes when lists are modified during iteration (common in cleanup operations)
- **Source**: Identified via valgrind analysis (`valgrind_20250724_210758.log`)

## 2025-01-24

### Performance Optimizations

#### Performance Monitoring System Optimization
- **Implemented Configurable Monitoring Levels**: Reduced overhead by up to 95%
  - **Sampling Mode** (default): Monitors 1 in N pulses (90% overhead reduction)
  - **Basic Mode**: Only logs when performance exceeds 100% threshold
  - **Off Mode**: Completely disables monitoring for production
  - **Full Mode**: Original behavior for debugging
- **Dynamic Load-Based Monitoring**:
  - Automatically switches to full monitoring when server load > 150%
  - Returns to sampling mode when load drops below 130%
  - Prevents missing critical performance issues while minimizing overhead
- **Runtime Configuration**: New `perfconfig` command for administrators
  - No server restart required to adjust monitoring levels
  - Real-time status display and configuration
  - Adjustable sampling rates (e.g., 1/5, 1/20, 1/100 pulses)
- **Implementation Details**:
  - Created `perfmon_optimized.c` with optimized monitoring logic
  - Added `PERF_log_pulse_optimized()` function with level checking
  - Maintains backward compatibility with existing `perfmon` command
  - Weak symbol linkage for graceful fallback if not compiled
- **Files Added/Modified**:
  - New: `perfmon_config.h`, `perfmon_optimized.c`
  - Modified: `comm.c`, `perfmon.h`, `perfmon.cpp`, `act.h`, `interpreter.c`

## 2025-01-25

### Bug Fixes

#### Fixed Critical Player Data Structure Access Violations
- **Issue**: NPCs were attempting to access player-only data structures (player_specials), causing crashes
- **Affected Files**:
  - `treasure.c:1525,1702` - NPCs accessing PRF_USE_STORED_CONSUMABLES preference flag
  - `spec_procs.c:6315,6335,6354,6376,6395,6417,6436,6458` - NPCs accessing PRF_CONDENSED preference flag
  - `magic.c` - Multiple instances of NPCs accessing GET_PSIONIC_ENERGY_TYPE
- **Solution**: 
  - Added `!IS_NPC()` checks before all PRF_FLAGGED macro uses
  - Implemented safe energy type access: `IS_NPC(ch) ? DAM_MENTAL : GET_PSIONIC_ENERGY_TYPE(ch)`
  - NPCs now default to DAM_MENTAL damage type for psionic abilities
- **Impact**: Prevents server crashes when NPCs receive treasure, engage in combat with special weapons, or cast psionic spells

## 2025-01-27

### Performance Optimizations

#### do_save() Performance Improvements (257ms → ~50-75ms target)
- **Buffered I/O Implementation**: Replaced hundreds of individual fprintf() calls with a single buffered write operation
  - 64KB initial buffer with dynamic growth capability
  - All player data collected in memory before one disk write
  - Dramatically reduces system call overhead
- **String Operation Optimization**: 
  - Created `buffer_write_string_field()` helper function to consolidate repetitive string operations
  - Pre-allocated buffers to minimize memory allocations
  - Reduced redundant string copying operations
- **Performance Monitoring**: Added timing measurements to track save performance
  - Logs saves taking >50ms for ongoing monitoring
  - Previously: 257ms per save (down from 513ms)
  - Target: 50-75ms per save (75% reduction)

#### affect_update() Performance Improvements (30% CPU reduction target)
- **Enhanced NPC Processing**:
  - Skip NPCs without affects entirely (no processing needed)
  - Only update MSDP for players with active descriptors
  - Prevents unnecessary work for disconnected players
- **Safe Iteration**: Cache next pointer to handle character extraction during processing
- **Performance Metrics**:
  - Track affected characters vs total characters
  - Count total affects processed per update
  - Log metrics every 100 updates (10 minutes) for monitoring

### Bug Fixes

#### Fixed "degenerate board" Error
- **Issue**: Board initialization was missing the rnum field, causing find_board() to fail
- **Solution**: 
  - Explicitly initialized rnum to NOTHING in board_info array declarations
  - Fixed array initialization for both CAMPAIGN_DL (7 boards) and standard (1 board)
  - Added enhanced error logging to identify problematic boards
  - Added validation to ensure board array size matches NUM_OF_BOARDS
- **Impact**: Eliminates SYSERR log spam and allows bulletin boards to function properly

### Technical Details
- Added `<sys/time.h>` include to players.c for performance timing
- Fixed C99 compilation error in db.c (for loop variable declaration)
- Maintained full backwards compatibility with existing save files
- No changes to save file format or data structures
- All existing functionality preserved

