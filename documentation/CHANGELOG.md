# CHANGELOG

## 2025-07-27

### Critical Bug Fixes
- **Fixed use-after-free in DG Script triggers** (dg_triggers.c)
  - Issue: `greet_mtrigger()` was accessing freed memory when iterating through triggers after `script_driver()` call
  - Root cause: `script_driver()` can free the script and its triggers, invalidating the loop iterator
  - Fix: Cache `t->next` before calling `script_driver()` to ensure safe iteration
  - Impact: Prevents potential crashes during NPC greet trigger execution

- **Fixed potential use-after-free in weather system** (weather.c)
  - Issue: `reset_dailies()` was iterating through character_list without caching next pointer
  - Root cause: `send_to_char()` could trigger character extraction during iteration
  - Fix: Cache `ch->next` before operations that could remove characters
  - Impact: Prevents crashes during daily reset operations (every 6 game hours)

- **Fixed object use-after-free during combat looting** (act.item.c)
  - Issue: Money objects were being accessed after extraction in `perform_get_from_container()` and `perform_get_from_room()`
  - Root cause: `get_check_money()` extracts money objects, but code continued to use the freed object pointer
  - Fix: Check if object is money before extraction, skip further object operations if it was money
  - Impact: Prevents crashes during corpse looting and picking up money

- **Fixed invalid read during shutdown** (db.c)
  - Issue: Craft list traversal was corrupted during game shutdown
  - Root cause: Using `simple_list()` while removing items corrupts its static iterator
  - Fix: Use proper iterator pattern instead of `simple_list()` during list cleanup
  - Impact: Prevents crashes during game shutdown when cleaning up craft data

- **Fixed uninitialized memory in event queue** (dg_event.c)
  - Issue: Queue element pointers not explicitly initialized, causing valgrind warnings
  - Root cause: Although `CREATE` uses `calloc()`, valgrind still reported uninitialized values
  - Fix: Explicitly set `prev` and `next` pointers to NULL in `queue_enq()`
  - Impact: Eliminates valgrind warnings about uninitialized memory in event system

- **Fixed multiple character list use-after-free bugs** (quest.c, dg_scripts.c)
  - Issue: Several functions iterating character_list without caching next pointer
  - Locations: `check_timed_quests()`, `check_trigger()`, `check_time_triggers()`
  - Root cause: Functions that send messages or execute scripts could extract characters
  - Fix: Cache `ch->next` before operations that could remove characters
  - Impact: Prevents crashes during quest timeouts and script trigger execution

- **Fixed hlquest parser bug causing 500+ false errors** (hlquest.c)
  - Issue: "Invalid quest command type 'S' in quest file" logged 500+ times during boot
  - Root cause: 'S' was used both as QUEST_COMMAND_CAST_SPELL and as a loop terminator
  - Details: Parser would create CAST_SPELL command then try to process 'S' as direction (I/O)
  - Fix: Check if we've hit the terminator 'S' before processing command direction
  - Impact: Eliminates 500+ false error messages during server startup
