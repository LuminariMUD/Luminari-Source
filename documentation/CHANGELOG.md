# CHANGELOG

## 2025-07-28

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

