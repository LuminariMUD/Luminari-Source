# CHANGELOG

## 2025-07-28 (Latest)

### SQL Injection Security Fixes
- **Comprehensive SQL Injection Prevention**: Fixed SQL injection vulnerabilities across the entire codebase
  - **craft.c**: Fixed player name escaping in get_avail_supply_orders() and put_mysql_supply_orders_available()
  - **new_mail.c**: Fixed multiple injection points in mail_list(), mail_read(), mail_delete(), and new_mail_alert() - properly escaping player names and mail IDs
  - **act.wizard.c**: Fixed account name escaping in do_accountpw() for both SELECT and UPDATE queries
  - **modify.c**: Added TODO for future clan name escaping when clans are implemented, fixed mail_deleted insert
  - **players.c**: Fixed player name escaping in load_pet_data(), load_eidolon_data(), save_eidolon_data(), and save_char() MySQL update
  - **account.c**: Fixed character name escaping in display_account_menu()
  - **act.item.c**: Fixed character name escaping in loot chest queries (SELECT, DELETE, INSERT)
  - **mysql.c**: Fixed player name escaping in write_who_to_mysql()
  - **hedit.c**: Fixed help tag and keyword escaping in hedit_save_to_disk()
- **Best Practices**: All user-provided strings now properly escaped using mysql_escape_string_alloc() before SQL queries

## 2025-07-28

### MySQL Thread Safety and Security Improvements
- **Thread Safety**: Added mutex protection for global MySQL connections (conn, conn2, conn3) to prevent race conditions
  - Added pthread_mutex_t declarations for each connection in mysql.h
  - Implemented thread-safe wrappers mysql_query_safe() and mysql_store_result_safe() with automatic mutex selection
  - Added MYSQL_LOCK/MYSQL_UNLOCK macros for consistent mutex handling
- **Error Handling Policy**: Established consistent MySQL error handling approach
  - Added MYSQL_ERROR_CRITICAL macro for startup/loading errors (exits program)
  - Added MYSQL_ERROR_RUNTIME macro for runtime errors (logs and continues)
  - Documented policy: Critical errors during startup exit, runtime errors log and return
- **SQL Injection Prevention**: Added mysql_escape_string_alloc() helper function
  - Thread-safe wrapper around mysql_real_escape_string() with automatic memory allocation
  - Fixed SQL injection vulnerability in templates.c levelinfo_search() - now properly escapes character names
  - Identified additional queries needing escaping in: craft.c, new_mail.c, act.wizard.c, modify.c, players.c, account.c

## 2025-07-28

### Copyover System Critical Fixes
- **Fixed Silent Failures**: Added comprehensive error handling throughout copyover process
  - **chdir() Error Handling**: Now checks return value and notifies players on failure (act.wizard.c:6120)
  - **Binary Path Validation**: Added access() check before execl() to verify binary exists (act.wizard.c:6139)
  - **Write Verification**: All fprintf() operations now check return values (act.wizard.c:5903,6058,6080)
  - **Atomic File Writing**: Implemented temp file + rename pattern to prevent partial writes (act.wizard.c:5884,6129)
  - **Player Notifications**: Players now receive clear error messages when copyover fails
  - **Race Condition Fix**: Moved file deletion to after all data is read (comm.c:561)
- **Graceful Failure**: Game now continues running instead of exiting when copyover fails
- **Error Logging**: Enhanced error messages with errno details for debugging

### Copyover System Comprehensive Improvements
- **Enhanced Error Logging**: Replaced all perror() calls with proper log() functions throughout copyover system
  - Added detailed error messages with errno information for all failure points
  - Added success logging for player restoration and file operations
  - Enhanced recovery logging showing player count and connection states
- **Descriptor State Validation**: Improved handling of non-playing connections
  - Added comprehensive state logging when dropping connections
  - Provide appropriate messages based on connection state (menu, OLC, etc.)
  - Added validation for room validity before saving players
  - Added player count tracking and reporting
- **Resource Cleanup**: Added proper resource cleanup before execl()
  - Close and flush log files to ensure all data is written
  - Close mother descriptor socket
  - Disconnect database connections
  - Flush stdout/stderr streams
- **Pre-Copyover Environment Validation**: Added validate_copyover_environment() function
  - Verify current working directory is in lib/
  - Check binary exists and is executable before attempting copyover
  - Test file creation permissions in current directory
  - Validate database connection is active
  - All checks performed before any destructive operations
- **Copyover State Tracking**: Implemented state machine to prevent race conditions
  - Added copyover_state enum (NONE, PREPARING, WRITING, EXECUTING, FAILED)
  - Prevents multiple simultaneous copyover attempts
  - Proper state cleanup on all error paths
  - Added get_copyover_state_string() diagnostic function

### Combat System Fixes
- **Autoblast in Safe Rooms**: Fixed eldritch blast spam in peaceful rooms during score command - added display mode checks in perform_attacks() (fight.c:11742-11756)
- **Autoblast Feat Requirements**: Added proper feat validation for autoblast toggle in prefedit and combat execution (prefedit.c:1067, fight.c:11726)

### UI Fixes
- **Prefedit Menu Display**: Fixed inconsistent spacing for AutoCollect option - added missing space before bracket to match other menu items (prefedit.c:359)

### MySQL Resource Management Fixes
- **templates.c**: Removed 17 incorrect mysql_close() calls on global connection that were breaking database connectivity
- **templates.c**: Changed all mysql_use_result() to mysql_store_result() for proper result set handling
- **mysql.c**: Fixed disconnect functions to not call mysql_library_end() (should only be called once at program end)
- **mysql.c**: Added cleanup_mysql_library() function for proper shutdown sequence
- **mysql.h**: Added cleanup_mysql_library() declaration
- **Transaction Handling**: Fixed missing rollback on errors in house.c and objsave.c - all transactions now properly rollback on failure
- **house.c**: Added mysql_query(conn, "rollback;") calls on all error paths during transaction (lines 250-299)
- **objsave.c**: Fixed transaction handling in Crash_crashsave(), Crash_idlesave(), and Crash_rentsave() - added rollback calls and proper file cleanup
- **account.c**: Removed redundant NULL check before mysql_free_result() (line 455)

### Critical Fixes

#### Memory Access & Crash Prevention
- **Spell Casting System**: Fixed invalid memory access in `castingCheckOk()` - validates target still exists before accessing (spell_parser.c:1405)
- **Player Save System**: Fixed uninitialized values in `create_entry()` preventing save file corruption (players.c:2920,2925)
- **Player Index Operations**: Fixed uninitialized memory access in `build_player_index()` and `save_player_index()` (players.c:275)
- **DG Event Queue**: Added NULL queue safety checks to prevent crashes from uninitialized pointers
- **Corpse Money Bug**: Fixed NULL container crash in fight.c:1735 - money from incorporeal creatures now drops to room only
- **Metamagic Exploit**: Fixed spell stacking exploit - players can no longer cast metamagic spells using regular spell slots

#### Major Memory Leaks
- **DG Scripts Lookup Table**: Fixed unbounded growth - added proper removal from static bucket heads and shutdown cleanup
- **AI Service**: Added missing `shutdown_ai_service()` call to free cache, config, rate limiter, and CURL handles
- **Event System**: Fixed redundant `strdup()` calls in `new_mud_event()` usage across multiple files
- **Track System**: Added proper cleanup in `destroy_db()` - fixes 6 bytes leaked per movement at shutdown

### File Handle Leaks
- **zmalloc.c:75**: Fixed debug log file reopening without checking if already open
- **comm.c:3741**: Added proper logfile cleanup before reassignment
- **act.wizard.c**: Fixed missing `fclose()` in `list_llog_entries()` (line 2639) and `find_llog_entry()` (line 2453)

### String Memory Leaks

#### Object/Room String Operations
- **Wilderness**: Fixed room name leaks in wilderness.c (673,698) and desc_engine.c (145,149)
- **Crafting**: Fixed string leaks in craft.c during object restringing and mold creation
- **Item Enhancement**: Fixed leaks in `spec_procs.c` masterwork/magical name functions
- **Room Commands**: Fixed leaks in `do_setroomname()` and `do_setroomdesc()`

#### Character/Communication
- **Corpse System**: Added missing `free()` for char_sdesc field in genobj.c
- **Character Descriptions**: Changed to static buffers in char_descs.c functions
- **Communication**: Fixed `buf3` leak in whisper/ask commands (act.comm.do_spec_comm.c)
- **Speech Triggers**: Fixed `strdup()` leaks in trigger processing (act.comm.c:59,158,234)

#### System Operations
- **Account Loading**: Fixed leaks in `load_account()` - now frees name/email/character_names
- **Character Saving**: Fixed leak in `save_char()` when overwriting account character names
- **Crafting Loads**: Fixed craft description string overwrites without freeing in players.c
- **MySQL Operations**: Added missing `mysql_free_result()` in account.c:394

### Safety Improvements
- **Circular References**: Added detection in `obj_to_obj()` to prevent infinite loops
- **Bounds Checking**: Added `VALID_ROOM_RNUM()` and `GET_ROOM()` macros for safe array access
- **Descriptor Cleanup**: Added NULL check for `d->events` in `close_socket()`
- **Account Loading**: Added bounds check for MAX_CHARS_PER_ACCOUNT

### Bug Fixes
- **Object Save Chain**: Fixed premature exit in `objsave_parse_objects()` cleanup loop
- **DG Scripts Wait**: Fixed uninitialized variables in `process_wait()` time parsing
- **Combat Events**: Fixed duplicate `eSMASH_DEFENSE` event creation and added cleanup in `stop_fighting()`
- **Put Command**: Fixed `strdup()` leaks for `thecont` and `theobj` strings
- **Action Queues**: Moved cleanup outside player_specials check in `free_char()`
- **OLC Scripts**: Fixed proto list leak in `dg_olc_script_copy()`
- **Template System**: Removed unnecessary `strdup()` in levelinfo_search calls
- **Forge Command**: Fixed forge_as_signature leak in backgrounds.c:826

