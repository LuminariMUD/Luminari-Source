# CHANGELOG

## [Unreleased] - 2025-01-24

### Fixed

#### Database Schema Fixes (July 24, 2025)
- **Fixed missing 'idnum' column errors** - Added `idnum` column to `house_data` and `player_save_objs` tables in both production (`luminari_mudprod`) and development (`luminari_muddev`) databases. Column type: `int(10) unsigned`, default value: 0, with indexes added. This resolves:
  - 10 boot errors when loading house data
  - Player item save failures on login
  - Database compatibility issues that were blocking core game functionality

#### NPC Access Violations (July 24, 2025)
- **Fixed NPCs accessing player preference flags** - Added IS_NPC() checks in act.informative.c to prevent NPCs from accessing PRF_FLAGGED player data. This fixes:
  - Lines 845-847: Commented out PRF_NON_ROLEPLAYER check in NPC-only display block
  - Lines 903-906: Commented out PRF_NON_ROLEPLAYER check in non-fighting NPC block  
  - Line 1064: Added !IS_NPC() check before PRF_FLAGGED access
  - Line 4494: Added !IS_NPC() check in user listing
  - Eliminates "Mob using '((i)->player_specials->saved.pref)' at act.informative.c:845" errors

#### Combat System Fixes (July 24, 2025)
- **Fixed combat targeting dead/corpse validation** - Modified damage() function in fight.c to handle the race condition when creatures attempt to damage corpses. This fixes:
  - Lines 4971-4976: Removed error log and redundant die() call when attempting to damage a corpse
  - Added stop_fighting() call to ensure attackers stop targeting corpses
  - Eliminates "Attempt to damage corpse" errors for creatures like spiders, crows, and crickets attacking their own corpses
  - This was a normal race condition where combat continued briefly after death before raw_kill() could clear fighting status

#### Compilation Fixes (July 24, 2025)
- **Fixed compilation errors in act.item.c and oasis.c** - Fixed incorrect variable references:
  - act.item.c:5375: Changed `!IS_NPC(ch)` to `!IS_NPC(i->character)` in auc_send_to_all()
  - oasis.c:63: Changed `!IS_NPC(ch)` to `!IS_NPC(d->character)` in clear_screen()

#### Treasure System Fix (July 24, 2025)
- **Fixed award_magic_item() implementation** - Resolved "award_magic_item called but no crafting system is defined" errors:
  - Added `#include "mud_options.h"` to treasure.c to ensure crafting system macros are visible
  - Replaced stub error logging with a fully functional default implementation
  - Default implementation awards treasures with appropriate distribution (10% crystals, 40% expendables, 25% trinkets, 20% armor, 5% weapons)
  - Eliminates error spam during zone resets while maintaining treasure functionality

#### Spec Proc Assignment Fixes (July 24, 2025)
- **Fixed invalid spec proc assignments** - Commented out assignments to non-existent mobs and objects:
  - Mob spec procs: #103802 (buyarmor), #103803 (buyweapons)
  - Object spec procs: #139203 (floating_teleport), #120010 (md_carpet), #100513 (halberd), #111507 (prismorb), #100599 (tormblade)
  - Added comments explaining why they're disabled to help builders know these spec procs are available
  - Eliminates "Attempt to assign spec to non-existant" errors during boot

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

