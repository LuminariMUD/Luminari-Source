# CHANGELOG

## 2025-07-28

### Fixes
  - Fix: Added explicit initialization loop for all queue head/tail pointers
  - Fix: Added `free_action_queue()` and `free_attack_queue()` calls to `free_mobile()`
  - Fix: 
    - Fixed memory leaks in track pruning code in `create_tracks()`
    - Added missing `free()` for trail_data_list structure in `free_trail_data_list()`
    - Added periodic `cleanup_all_trails()` function called once per mud hour
  - Fix:
    - Added proper NULL checks after all `strdup()` calls
    - Fixed error paths to properly free allocated memory before returning
    - Ensured CURL handles are only cleaned up when not using persistent handle
  - Fix: Removed redundant `strdup()` calls when passing strings to `new_mud_event()`
    - Changed `new_mud_event(eCOMBAT_ROUND, ch, strdup("1"))` to `new_mud_event(eCOMBAT_ROUND, ch, "1")`
    - Changed `new_mud_event(eCRIPPLING_CRITICAL, victim, strdup("1"))` to `new_mud_event(eCRIPPLING_CRITICAL, victim, "1")`
  - Fix: Moved action queue cleanup outside of player_specials check in `free_char()`
  - Fix: Fixed MySQL result memory leak in `load_account()`
    - Added missing `mysql_free_result()` call when account is not found in `account.c:394`
  - Fix: Fixed critical uninitialized memory access in DG Event Queue system
    - Added NULL queue safety checks to all queue operations (`queue_enq`, `queue_deq`, `queue_head`, `queue_key`)
    - Added validation in `event_create()` and `event_process()` to ensure event_q is initialized before use
    - Prevents potential crashes from accessing uninitialized queue pointers
  - Fix: Fixed memory leaks in Fight System during combat
    - Added cleanup of combat-related events (`eSMASH_DEFENSE`, `eSTUNNED`) in `stop_fighting()`
    - Fixed duplicate event creation by checking if `eSMASH_DEFENSE` event already exists before creating
    - Prevents memory exhaustion during prolonged combat sessions
  - Fix: Fixed string duplication memory leak in `do_put()` command
    - Added proper `free()` calls for `thecont` and `theobj` strings allocated with `strdup()`
    - Memory leak occurred every time items were put in containers
    - Fixed by freeing strings before all return paths and at function exit
  - Fix: Fixed character save memory leak in `save_char()` function
    - Added `free()` call before overwriting existing character name in account data
    - Memory leak occurred every time a character saved (6 bytes per save)
    - Fixed by freeing old string before assigning new one with `strdup()`
  - Fix: Fixed account character loading issues in `account.c`
    - Added bounds check to prevent buffer overflow when loading more than MAX_CHARS_PER_ACCOUNT characters
    - Fixed potential memory leak in `get_char_account_name()` when multiple rows are returned
    - Prevents crashes and memory corruption during login
  - Fix: Fixed crafting system memory leaks in character loading
    - Added `free()` calls before overwriting craft description strings when loading character data
    - Memory leak occurred because `reset_current_craft()` allocates default strings that were overwritten without freeing
    - Fixed in `players.c` for keywords, short_description, room_description, and ex_description fields
  - Fix: Fixed uninitialized value errors in DG Scripts `process_wait()` function
    - Variables `when`, `hr`, `min`, `ntime`, and `c` were not initialized before use
    - Issue occurred when sscanf failed to parse the "wait until" time format
    - Added proper initialization of all local variables to 0
    - Added missing sscanf check for alternate time format (1430 vs 14:30)
    - Added error handling and logging for invalid time formats in `dg_scripts.c:1851`

