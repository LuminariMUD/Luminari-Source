# CHANGELOG

## [2025-07-24] - Fixed Search Functions Priority Issue

### Summary
Fixed a critical bug in multiple search functions where abbreviation matching would incorrectly take precedence over exact matches. This affected commands like `feat info dragon mount` which would fail to find valid entries.

### Problem Description
The search functions were checking for abbreviations in the same pass as exact matches. When searching for "dragon mount", the function would find "dragon mount boost" as an abbreviation match first. Since "dragon mount boost" had `in_game = FALSE`, it would return "Could not find that feat" without ever checking for the exact match "dragon mount" (FEAT_DRAGON_BOND).

### Root Cause
All affected search functions followed a flawed pattern:
1. They would check `is_abbrev()` first in the search loop
2. Finding "dragon mount boost" as an abbreviation of "dragon mount"
3. Returning immediately without checking for exact matches later in the list

### Files Modified

#### 1. `feats.c` - Fixed `find_feat_num()`
- **Line**: 8325
- **Change**: Reorganized search into 3 phases: exact match, word-by-word, then abbreviation
- **Result**: "dragon mount" now correctly finds FEAT_DRAGON_BOND

#### 2. `evolutions.c` - Fixed `find_evolution_num()`
- **Line**: 1518
- **Change**: Same 3-phase search reorganization
- **Result**: Prevents similar issues with evolution names

#### 3. `spell_parser.c` - Fixed `find_skill_num()` and `find_ability_num()`
- **Lines**: 386 (find_skill_num), 431 (find_ability_num)
- **Change**: Same 3-phase search reorganization for both functions
- **Result**: Prevents similar issues with skill and ability names

#### 4. `alchemy.c` - Fixed `find_discovery_num()`
- **Line**: 3163
- **Change**: Same 3-phase search reorganization
- **Result**: Prevents similar issues with discovery names

### Technical Implementation
Each function now uses a 3-phase search approach:
```c
/* PHASE 1: Check for exact match first (case-insensitive) */
for (index = 1; index < MAX_ITEMS; index++) {
    if (!strcasecmp(name, item_list[index].name))
        return (index);
}

/* PHASE 2: Try word-by-word matching for multi-word names */
// ... existing word-by-word logic ...

/* PHASE 3: Finally try abbreviation matching as fallback */
for (index = 1; index < MAX_ITEMS; index++) {
    if (is_abbrev(name, item_list[index].name))
        return (index);
}
```

### Impact
- Exact matches now always take priority over abbreviations
- Maintains backward compatibility - all previously working searches continue to work
- Fixes the specific issue where "dragon mount" failed to find FEAT_DRAGON_BOND
- Prevents similar issues across all search functions

### Testing Verification
After the fix:
- `feat info dragon mount` → Finds FEAT_DRAGON_BOND (exact match)
- `feat info dragon mount boost` → Finds FEAT_DRAGON_MOUNT_BOOST (exact match)
- `feat info drag` → Still finds first feat starting with "drag" (abbreviation)
- Similar improvements for evolution, skill, ability, and discovery searches

## [2025-07-23] - Enhanced DG Scripts Safety and Room Validation

### Summary
Fixed additional segmentation faults in the DG Scripts system by adding comprehensive bounds checking for room access. Corrected the VALID_ROOM_RNUM macro to prevent array out-of-bounds access.

### Files Modified

#### 1. `dg_variables.c` - Additional Script Safety Fixes
- **Problem**: Segmentation fault at address 0xE809A60018 when scripts accessed room data
- **Root Cause**: Objects in NOWHERE executing scripts with `%random.char%` or accessing `.room` property
- **Changes**:
  - Lines 626: Added VALID_ROOM_RNUM check for MOB_TRIGGER random.char access
  - Lines 641: Added VALID_ROOM_RNUM check for OBJ_TRIGGER random.char access  
  - Lines 687: Added bounds checking for random.dir access
  - Lines 1927: Added bounds checking for obj.room property access
- **Result**: Scripts no longer crash when accessing room data from invalid locations

#### 2. `utils.h` - VALID_ROOM_RNUM Macro Fix
- **Line 868**: Fixed macro from `<= top_of_world` to `< top_of_world`
- **Problem**: Array bounds check was off by one (arrays are 0-indexed)
- **Result**: Prevents potential buffer overrun when accessing world array

#### 3. `class.c` - Compilation Fix
- **Line 2321**: Removed unused `int x;` variable declaration
- **Line 2348**: Added `int x;` declaration inside `USE_CONTAINER_OBJECTS` block where it's actually used
- **Problem**: Variable was declared at function scope but only used in conditional compilation block
- **Result**: Eliminates unused variable warning

#### 4. `cpbin2dev.sh` - Script Enhancement
- **Lines 4,6**: Added quotes around echo strings for consistency
- **Lines 11-13**: Added reminder message for changing directories after deployment

### Technical Details
- **Error Path**: heartbeat() → script_trigger_check() → random_otrigger() → script_driver() → process_if() → eval_expr() → var_subst() → find_replacement()
- The crash occurred when find_replacement() tried to access world[invalid_room].people
- All room access now uses VALID_ROOM_RNUM macro for consistent validation
- The fixes handle cases where obj_room() returns NOWHERE or invalid positive numbers

### Impact
- Prevents crashes from scripts on objects/mobs in invalid locations
- Improves overall server stability during script execution
- Maintains compatibility with existing scripts and content

## [2025-07-23] - Database Compatibility and Crash Fixes

### Summary
Fixed multiple critical issues affecting development server compatibility and stability. Added graceful handling for missing database columns and fixed a segmentation fault in the DG Scripts system.

### Files Modified

#### 1. `objsave.c` - Database Column Compatibility
- **Problem**: Server crashed with "Unknown column 'idnum' in 'field list'" errors when database schema was incomplete
- **Changes**:
  - Lines 2033-2049: Added fallback query for `player_save_objs` without idnum column
  - Lines 2056-2073: Added fallback query for `house_data` without idnum column  
  - Lines 2098-2122: Added logic to handle different query types based on which columns exist
  - Line 2020: Added `loading_house_data` variable to track query type
- **Result**: Server now gracefully handles missing idnum columns instead of crashing

#### 2. `dg_variables.c` - Script Crash Fix
- **Problem**: Segmentation fault when objects/mobs in NOWHERE tried to access `%random.char%`
- **Error**: "SCRIPT ERROR: Obj (a burning cluster of pines, VNum 11865):: oecho called by object in NOWHERE"
- **Changes**:
  - Lines 623-633: Added safety check for MOB_TRIGGER to verify room is valid
  - Lines 634-645: Added safety check for OBJ_TRIGGER to verify room is valid
- **Result**: Scripts no longer crash when entities are in invalid locations

#### 3. `mysql.c` - Connection Logging
- **Line 128**: Added success message when MySQL connection is established
- **Result**: Easier debugging of database connection issues

#### 4. `cpbin2dev.sh` - Development Deployment Enhancement
- **Lines 7-10**: Added 5-second delay and automatic server startup after deployment
- **Result**: Streamlined development deployment process

### Technical Details
- The database compatibility fixes use a fallback pattern: try with full schema first, then retry with reduced schema
- Room validity checks prevent accessing `world[NOWHERE].people` which causes segfaults
- All changes maintain full backward compatibility with production servers

### Impact
- Development servers can now run without complete database schemas
- Objects and mobs in invalid locations no longer crash the server
- Database connection issues are easier to diagnose
- Development deployment is more automated

## [2025-07-23] - Project Cleanup

### Summary
Removed obsolete and unnecessary files to clean up the project directory. These files were either for legacy platforms, unused build configurations, or IDE-specific settings.

### Files Removed

#### 1. Legacy/Obsolete Files
- **`SCOPTIONS`** - Configuration file for SAS/C compiler on Amiga platform (obsolete since 1990s)
- **`newdepend.sh`** - Simple script that just deleted and restored the depend file from git
- **`htmlh-head`** and **`htmlh-tail`** - Unused HTML template fragments

#### 2. Backup Makefiles
- **`Makefile.2`**, **`Makefile.3`**, **`Makefile.4`** - Alternative Makefile versions with different compiler flags
- **`Makefile2`** - Another backup Makefile
- **Note**: Kept `Makefile.in` as it's the template used by the configure script

#### 3. Documentation Files
- **`dox_withGraphs.doxyfile`** - Doxygen configuration with graph generation enabled
- **`dox_noGraphs.doxyfile`** - Doxygen configuration without graphs
- **Note**: Both referenced outdated project name "tbaMUD" instead of "LuminariMUD"

#### 4. IDE Configuration
- **`nbproject/`** directory - NetBeans IDE project configuration files
  - Not needed for command-line builds
  - Only useful if developing with NetBeans IDE

#### 5. Dependency File Backups
- **`depend.4`** and **`depend.bak`** - Backup copies of the dependency file

### Other Changes
- Regenerated `depend` file using `gcc -MM *.c > depend` to ensure up-to-date dependency tracking

### Impact
These removals have no impact on the build process or functionality. The project still builds successfully with `make all` and is cleaner without these obsolete files.

## [2025-07-23] - Build System Fixes

### Summary
Fixed multiple compilation errors preventing the project from building successfully. These included syntax errors, duplicate definitions, missing constants, and unresolved dependencies.

### Files Modified

#### 1. `class.c`
- **Line**: 2324
- **Error**: `struct *quiver = NULL, *pouch = NULL, *bp = NULL;` - missing type name
- **Fix**: Changed to `struct obj_data *quiver = NULL, *pouch = NULL, *bp = NULL;`

#### 2. `transport.c`
- **Lines**: 224-334
- **Error**: Duplicate definition of `walkto_landmarks` array
- **Fix**: Commented out the second definition using `#if 0 ... #endif`

#### 3. `structs.h`
- **Line**: 973 (after `#define REGION_ZAKHARA`)
- **Error**: `REGION_OUTER_PLANES` was undefined for non-DL campaigns
- **Fix**: Added `#define REGION_OUTER_PLANES 36` and updated `NUM_REGIONS` to 37

#### 4. `constants.c`
- **Line**: 5767 (in regions array)
- **Error**: Missing "Outer Planes" entry causing static assertion failure
- **Fix**: Added `"Outer Planes",` before the terminating `"\n"`

#### 5. `wilderness.c`
- **Multiple locations**
- **Initial Error**: Missing `gd.h` header file (GD graphics library)
- **Initial Approach**: Attempted to disable GD functionality
- **Final Fix**: Reverted changes after discovering libgd-dev was already installed
  - Fixed syntax errors from incomplete reversion (removed extra closing braces on lines 1237 and 1294)
  - GD library functionality remains intact
  - Map generation features (`save_map_to_file()` and `save_noise_to_file()`) are fully functional

#### 6. `Makefile`
- **Line**: 31
- **No changes needed** - `-lgd` remains in LIBS since the library is installed

#### 7. `treasure.c`
- **Line**: 919 (after `#endif`)
- **Error**: `award_magic_item()` undefined when neither `USE_NEW_CRAFTING_SYSTEM` nor `USE_OLD_CRAFTING_SYSTEM` is defined
- **Fix**: Added default stub implementation:
  ```c
  #else
  /* Default implementation when no crafting system is defined */
  void award_magic_item(int number, struct char_data *ch, int grade)
  {
    /* Simple stub implementation - just log a message */
    log("award_magic_item called but no crafting system is defined");
  }
  #endif
  ```

### Build Process
After these fixes, the project builds successfully with `make all`, creating:
- Main game executable: `../bin/circle`
- All utility programs in `../bin/`

### Notes
- The GD library (libgd-dev) is required and installed - map generation features are fully functional
- The crafting system needs to be properly configured by defining either `USE_NEW_CRAFTING_SYSTEM` or `USE_OLD_CRAFTING_SYSTEM`
- Several warnings remain but don't prevent successful compilation

## [2025-07-23] - Bug Fix: Multi-word Search Functions

### Summary
Fixed a critical bug in multiple search functions that prevented proper matching of multi-word names. The bug affected commands like `feat info dragon mount` which would incorrectly fail to find valid entries.

### Problem Description
The search functions were using word-by-word matching but only verified that all search words were consumed, not that all target name words were also consumed. This caused searches like "dragon" to incorrectly match both "dragon mount" and "dragon link", leading to failed searches when they should have succeeded.

### Files Modified

#### 1. `feats.c`
- **Function**: `find_feat_num()` 
- **Line**: 8347
- **Change**: Modified condition from `if (ok && !*first2)` to `if (ok && !*first2 && !*first)`
- **Note**: This fix was already present in the codebase

#### 2. `evolutions.c`
- **Function**: `find_evolution_num()`
- **Line**: 1540 (originally reported as 1251)
- **Change**: Modified condition from `if (ok && !*first2)` to `if (ok && !*first2 && !*first)`

#### 3. `spell_parser.c`
- **Function**: `find_skill_num()`
- **Line**: 409 (originally reported as 170)
- **Change**: Modified condition from `if (ok && !*first2)` to `if (ok && !*first2 && !*first)`
- **Function**: `find_ability_num()`
- **Line**: 442 (originally reported as 237)
- **Change**: Modified condition from `if (ok && !*first2)` to `if (ok && !*first2 && !*first)`

#### 4. `alchemy.c`
- **Function**: `find_discovery_num()`
- **Line**: 3185 (originally reported as 889)
- **Change**: Modified condition from `if (ok && !*first2)` to `if (ok && !*first2 && !*first)`
- **Function**: `find_grand_discovery_num()`
- **Line**: 3272 (originally reported as 1022)
- **Change**: Modified condition from `if (ok && !*first2)` to `if (ok && !*first2 && !*first)`

### Technical Details
The fix ensures that both the search term AND the target name are fully consumed before declaring a match. The additional `&& !*first` check prevents partial matches where the search term is a subset of the target name.

### Testing Notes
After this fix:
- `feat info dragon mount` will correctly find FEAT_DRAGON_BOND
- `feat info dragon link` will correctly find FEAT_DRAGON_LINK
- `feat info dragon` will NOT match multi-word feats (this is the intended behavior)
- Partial matches like `feat info drag mount` will still work correctly
- The same logic applies to evolution, skill, ability, and discovery searches

### Build Setup Notes
For developers setting up local builds:

1. **Required Header Files**: The following files must be created locally (they are in .gitignore):
   - `campaign.h` - Campaign-specific configuration
   - `vnums.h` - Copy from `vnums.example.h`
   - `mud_options.h` - Copy from `mud_options.example.h`

2. **Creating Required Files**:
   ```bash
   # Create campaign.h with minimal content
   echo '/* Campaign-specific configuration */' > campaign.h
   
   # Copy example files
   cp vnums.example.h vnums.h
   cp mud_options.example.h mud_options.h
   ```

3. **Compilation**: All modified files compile successfully with the standard project flags.

### Related Issues
- Original issue: "feat info dragon mount" command not working
- Root cause: Word-matching algorithm bug in search functions
- Impact: Affected feat, evolution, skill, ability, and discovery information commands

### Backwards Compatibility
This fix maintains backwards compatibility. All previously working searches will continue to work. The fix only prevents incorrect matches that were bugs, not features.

### Update: Fix Failed
The attempted fix did not resolve the issue. The `feat info dragon mount` command still returns "Could not find that feat". Further investigation needed.