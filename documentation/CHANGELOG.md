# CHANGELOG

## 2025-01-29
### Performance
- Implemented incremental save system for clans:
  - Added `save_single_clan()` function to save individual clans instead of rewriting entire file
  - Added `mark_clan_modified()` to track which clans need saving
  - Added periodic auto-save every 5 minutes for modified clans
  - Significantly reduces disk I/O for clan operations

### Added
- Enhanced clan rank documentation:
  - Added clear rank system explanation in main clan help
  - Added rank structure display to `clan info` command
  - Shows that lower rank numbers = higher authority (1 = leader)
  - Added note about rank 0 meaning "leader only" for permissions
- Enhanced clan info display to show all fields:
  - Added clan description display
  - Added tax rate, clan hall zone, and war timer
  - Added PK statistics (wins/losses/raids) for clan members
  - Shows all clan data fields that were previously hidden
- Implemented comprehensive clan activity logging system:
  - Added `log_clan_activity()` function to log clan events to files
  - Logs stored in `lib/etc/clan_logs/clan_<vnum>.log`
  - Added logging for: applications, enrollments, promotions, demotions, expulsions, leaving, deposits, withdrawals, awards, alliances, wars
  - Added `clan log [lines]` command to view recent activity (default 20 lines, max 100)
- Added permissions display to clan info:
  - Shows required rank for each clan privilege
  - Only visible to clan members and immortals
  - Clearly indicates which permissions are "Leader Only"
  - Helps players understand clan hierarchy

### Fixed
- Removed commented out code:
  - Removed old `find_clan_by_id()` function that was commented out
  - Removed commented variable declarations in `get_clan_by_name()`
  - Removed commented line in clan_edit.c
  - Cleaned up codebase from obsolete commented code

## 2025-01-29
### Fixed
- Fixed critical clan system bugs:
  - Fixed inverted permission logic in `can_edit_clan()` preventing authorized clan members from editing clans
  - Fixed memory leak in `do_clanleave()` where clan leave code was not being freed
  - Fixed buffer overflow risk in clan name truncation by implementing safe color code handling
  - Fixed null pointer dereference in `do_clantalk()` when checking clan leadership
  - Verified that `free_single_clan_data()` properly frees description (no fix needed)
- Fixed additional clan system issues:
  - Fixed rank boundary check in `do_clanapply()` - now uses `IS_IN_CLAN()` macro instead of incorrect rank check
  - Added duplicate clan name prevention in `do_clancreate()`
  - Added input sanitization for clan names to remove control characters
  - Added save_player_index() call after clan deletion to update all member records
  - Added tax rate validation (0-100%) in `do_clanset()`
  - Added raid counter display to clan info output
  - Fixed missing save_player_index() calls in promote/demote/leave commands
  - Added zone permission validation to `do_clanunclaim()` - implementors can now only unclaim zones they have permission to edit
  - Added null checks for all strdup() calls to prevent crashes on memory allocation failures
  - Added bounds checking for clan VNUM assignment to prevent overflow
### Performance
- Optimized clan lookups:
  - Implemented hash table for O(1) clan VNUM lookups instead of O(n) linear search
  - Added hash table initialization to `load_clans()` and cleanup to `free_clan_list()`
  - Updated `add_clan()` and `remove_clan()` to maintain hash table integrity
- Optimized member counting:
  - Added member count and power caching to reduce O(n) player table iterations
  - Cache is automatically updated when members join/leave or on 5-minute timeout
  - Significantly improves performance with large player bases
### Added
- Implemented clan war timer functionality:
  - Created `update_clans()` function to decrement war timers each MUD hour
  - Wars automatically end when timer reaches 0, notifying online members
  - Added `update_clans()` call to heartbeat in comm.c
- Implemented clan alliance management commands:
  - Added `clan ally <clan>` command to propose/accept/break alliances
  - Added `clan war <clan>` command to declare/end wars
  - Both commands show current allies/enemies when used without arguments
  - Alliances and wars are mutually exclusive (must break alliance before war)
  - Wars have a 48-hour timer and automatically end when expired
  - All clan members are notified when alliances/wars are formed or broken
- Added clan activity tracking:
  - Tracks last activity timestamp for each clan
  - Updates on clantalk, deposits, withdrawals, and zone claims
  - Displays time since last activity in clan info (green < 1 hour, yellow < 1 day, red > 1 day)
  - Persists across reboots via save/load functions
- Added clan member limits:
  - Configurable maximum member limit per clan (default: 50, 0 = unlimited)
  - Enforced during enrollment with clear error messages
  - Displayed in clan info output
  - Persists across reboots via save/load functions
### Removed
- Removed unused clan spells framework (spells[MAX_CLANSPELLS] array and MAX_CLANSPELLS constant)

## 2025-01-28
### Fixed
- Fixed `put all <container>` command not recognizing "all" keyword properly
  - Added missing `find_all_dots()` parsing in `do_put()` function in act.item.c:2009
  - Command was treating "all" as an object name instead of a keyword
  - Now correctly puts all items from inventory into the specified container

