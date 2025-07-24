# CHANGELOG

## [Unreleased] - 2025-01-24

### Fixed

#### Database Schema Fixes (July 24, 2025)
- **Fixed missing 'idnum' column errors** - Added `idnum` column to `house_data` and `player_save_objs` tables in both production (`luminari_mudprod`) and development (`luminari_muddev`) databases. Column type: `int(10) unsigned`, default value: 0, with indexes added. This resolves:
  - 10 boot errors when loading house data
  - Player item save failures on login
  - Database compatibility issues that were blocking core game functionality

## [Previous] - 2025-01-23

### Fixed

#### High Priority Fixes
- **Fixed PRF_FLAGGED mob access crash** - Added IS_NPC check before PRF_FLAGGED usage in fight.c:11669. This was causing crashes every 2 seconds during combat when NPCs tried to access player-specific data.
- **Fixed objects in NOWHERE executing scripts** - Added checks in `timer_otrigger()` and `random_otrigger()` in dg_triggers.c to prevent script execution for objects in NOWHERE that aren't carried, worn, or inside other objects.
- **Fixed database compatibility issues** - Added fallback queries and error handling for missing database columns and tables:
  - `pet_save_objs` table: Changed fatal errors to graceful handling when table doesn't exist
  - `pet_data.intel` column: Removed references to non-existent column from INSERT queries
  - `PLAYER_DATA.character_info` column: Added fallback UPDATE queries without the column

### Technical Details
- Modified files:
  - `fight.c` - Line 11669: Added `!IS_NPC(ch) &&` check
  - `dg_triggers.c` - Added NOWHERE checks in timer_otrigger() and random_otrigger()
  - `objsave.c` - Modified pet_load_objs() and pet object saving to handle missing table; fixed compilation warning by returning NULL instead of void
  - `players.c` - Fixed pet_data INSERT queries and character_info UPDATE
  - `act.wizard.c` - Modified SELECT queries to use empty string for missing character_info column

#### Low Priority Fix
- **Fixed award_magic_item crafting system configuration** - Added USE_OLD_CRAFTING_SYSTEM configuration option to mud_options.h and mud_options.example.h. This eliminates the "award_magic_item called but no crafting system is defined" log spam and enables magic item awards to actually function.

### Added
- **campaign.example.h** - Added example campaign configuration file to repository. Users can now copy this to campaign.h instead of creating from scratch. Includes options for CAMPAIGN_DL (Dragonlance) and CAMPAIGN_FR (Forgotten Realms).

### Notes
- These fixes should significantly reduce error spam in the logs
- Most remaining issues are world file problems (missing triggers, invalid zone commands, etc.) rather than code bugs
- The code already handles most world file issues gracefully by logging errors and continuing operation
- Magic items will now be properly awarded when treasure is generated

