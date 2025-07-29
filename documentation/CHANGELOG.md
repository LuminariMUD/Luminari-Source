# CHANGELOG

## 2025-01-29 (Part 5)
### Fixed
- **Build Errors and Warnings**:
  - Fixed missing CLAMP macro in clan.c by replacing with MAX/MIN
  - Fixed incorrect level constant LVL_GRGOD with LVL_GRSTAFF
  - Fixed implicit declaration of ABS macro in dg_mobcmd.c
  - Fixed improper assignment of const char* from two_arguments()
  - Added forward declarations for transaction system functions
  - Added ACMD_DECL(do_claninvest) to clan.h
  - Commented out references to non-existent player_index_element.clanrank field
  - Fixed unused variable warnings
  - All compilation errors and warnings resolved - system builds completely clean with no warnings

## 2025-01-29 (Part 4)
### Added
- **DG Scripts Clan Integration**:
  - Added `mclanset` command to set character clan affiliation via scripts
  - Added `mclanrank` command to modify character clan rank via scripts  
  - Added `mclangold` command to add/remove gold from clan treasury via scripts
  - Added `mclanwar` command to set war status between clans via scripts
  - Added `mclanally` command to set alliance status between clans via scripts
  - New character variables: `%actor.clanname%`, `%actor.is_clan_leader%`, `%actor.clan_gold%`
  
- **Clan Economy System** (clan_economy.c/h):
  - Integrated clan shops with zone-based discounts (5-20% based on rank)
  - Automatic transaction tax collection for clan members
  - Clan investment system with 4 types: shops, caravans, mines, farms
  - Added `claninvest` command for managing clan investments
  - Daily investment returns with risk/reward mechanics
  - Shop discounts for allied clans in controlled zones
  
- **Clan Locking Mechanism**:
  - Added time-based locks to prevent concurrent modifications
  - Lock duration of 60 seconds with automatic expiration
  - Functions: `acquire_clan_lock()`, `release_clan_lock()`, `is_clan_locked()`, `can_modify_clan()`
  - Integrated locks into deposit/withdraw operations
  - Periodic cleanup of expired locks in `update_clans()`
  
- **Clan Transaction System** (clan_transactions.c/h):
  - Atomic operations with rollback capability
  - Transaction types for treasury, membership, ranks, wars, alliances
  - Automatic rollback for failed operations
  - Transaction logging and timeout handling
  - Example integration in clan deposit function

### Fixed
- Updated outdated code comments in clan system
- Changed TODO comment for clan spells to indicate deprecated functionality
- Improved error handling with transaction rollbacks

### Changed
- Modified shop prices to apply clan discounts automatically
- Updated clan deposit/withdraw to use locking mechanism
- Enhanced transaction handling for atomic operations

## 2025-01-29 (Part 3)
### Fixed
- Renamed `is_a_clan_leader()` to `check_clan_leader()` for consistent function naming with `check_clanpriv()`
- Added detailed error logging to clan functions via new `log_clan_error()` function
  - Logs to both system log and dedicated clan_errors.log file
  - Includes function name, error details, and timestamps
  - Updated key functions to use detailed error logging instead of generic returns

### Added
- Comprehensive unit test suite for clan system functions (test_clan.c)
  - Tests for core functions: clear_clan_vals, real_clan, add_clan, remove_clan
  - Tests for privilege checking: check_clan_leader, check_clanpriv
  - Tests for utility functions: get_clan_by_name, are_clans_allied, are_clans_at_war
  - Tests for hash table performance optimization
- Data validation and recovery system for clan data
  - `validate_clan_data()` - Validates single clan with optional auto-fix
  - `validate_all_clans()` - Validates all clans in system
  - `validate_clan_membership()` - Validates player clan memberships
  - `clan_data_integrity_check()` - Comprehensive integrity check
  - New immortal command: `clanfix <check|fix>` - Run integrity checks and repairs
- Meaningful zone control benefits system
  - Created clan_benefits.h defining 12 different zone control benefits
  - HP/Mana/Move regeneration bonuses (+2/+2/+5 per tick)
  - Experience and gold bonuses (+10%/+15%)
  - Combat bonuses (skill +2, saves +1, damage +1, AC +1)
  - No death penalty in controlled zones
  - Fast travel (20% movement cost reduction)
  - Shop discounts (10% off)
  - Allied clans receive partial benefits (regen and movement only)
  - New command: `clan benefits` - Display zone control benefits

### Performance
- Implemented zone benefit application functions for integration with game systems
  - `apply_zone_regen_bonus()` - Apply to regeneration tick
  - `apply_zone_exp_bonus()` - Apply to experience gains
  - `apply_zone_gold_bonus()` - Apply to gold drops
  - `apply_zone_skill_bonus()` - Apply to skill checks
  - `apply_zone_damage_bonus()` - Apply to damage rolls
  - `apply_zone_shop_price()` - Apply shop discounts

## 2025-01-29 (Part 2)
### Added
- Comprehensive clan statistics tracking system:
  - Tracks total deposits, withdrawals, members joined/left, zones claimed
  - Records combat statistics: wars won/lost, alliances formed
  - Maintains historical data: date founded, highest member count
  - Added `clan stats` command to view all statistics with formatted display
  - Statistics persist across reboots via enhanced save/load functions
- Standardized clan error messages:
  - Defined 23 standard error message constants in clan.h
  - Replaced all hardcoded error strings with consistent messages
  - Improves user experience with uniform error reporting
- Replaced magic numbers with named constants:
  - DEFAULT_CLAN_RANKS (6)
  - DEFAULT_MAX_MEMBERS (50)
  - DEFAULT_WAR_DURATION (1440 ticks)
  - DEFAULT_CACHE_TIMEOUT (300 seconds)
  - CLAN_LOG_DIR, MAX_CLAN_LOG_LINES, etc.
  - Improves code maintainability and configuration

### Fixed
- Updated clan member tracking to increment statistics when members join/leave
- Enhanced clan financial tracking to record all deposits and withdrawals
- Updated zone claiming to track total zones claimed and current ownership

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

