# TASK LIST - Coders ToDo List

---

## COMPREHENSIVE AUDIT: award_() SYSTEM

### Executive Summary
The award_() system in Luminari MUD is **severely fragmented and disorganized**. While the intention appears to be centralizing reward functionality in `treasure.c/.h`, the reality is that award functions are scattered across multiple files with inconsistent naming, overlapping functionality, and poor organization.

### Current State Analysis

#### Core Issues Identified:
1. **Fragmented Location**: Award functions exist in at least 6 different files
2. **Inconsistent Naming**: Mix of `award_`, `gain_`, `increase_`, and `get_` prefixes
3. **Overlapping Functionality**: Multiple ways to accomplish the same task
4. **Missing Centralization**: No single point of control for reward distribution
5. **Poor Documentation**: Functions lack consistent documentation standards

#### File Distribution Analysis:

**treasure.c/.h (PRIMARY LOCATION - PARTIALLY ORGANIZED)**
- `award_magic_item()` - Main treasure distribution function
- `award_expendable_item()` - Potions, scrolls, wands, staffs
- `award_magic_armor()` - Armor pieces with specific wear slots
- `award_magic_weapon()` - Weapon generation
- `award_misc_magic_item()` - Trinkets, rings, etc.
- `award_random_crystal()` - Crystal generation
- `award_magic_ammo()` - Ammunition
- `award_random_magic_armor()` - Wrapper for armor distribution
- `award_random_expendible_item()` - Wrapper for expendables
- `award_random_money()` - Gold rewards

**act.wizard.c (ADMIN COMMAND)**
- `do_award()` - Administrative command for manual rewards
- Uses `award_types[]` array from constants.c
- Handles: experience, quest-points, account-experience, gold, bank-gold, skill-points, feats, class-feats, epic-feats, epic-class-feats, ability-boosts

**limits.c (CORE MECHANICS)**
- `gain_exp()` - Primary experience awarding function
- `increase_gold()` - Primary gold manipulation function
- `decrease_gold()` - Gold deduction wrapper

**fight.c (COMBAT REWARDS)**
- `perform_group_gain()` - Group experience distribution
- `solo_gain()` - Solo experience distribution
- Combat damage experience via `gain_exp()` calls

**missions.c (MISSION SYSTEM)**
- `get_mission_reward()` - Calculate mission rewards
- `apply_mission_rewards()` - Apply mission completion rewards
- Handles: credits, standing, reputation, experience

**quest.c (QUEST SYSTEM)**
- Quest completion rewards (gold, quest points, experience)
- Uses `increase_gold()` and direct manipulation
- Happy hour bonuses for quest rewards

**hunts.c (HUNT SYSTEM)**
- `award_hunt_materials()` - Hunt-specific material rewards
- Direct gold and quest point manipulation

**act.item.c (LOOTBOX SYSTEM)**
- Lootbox opening mechanics using treasure.c functions
- Calls various `award_*` functions from treasure.c

#### Function Naming Inconsistencies:

**Award Functions (treasure.c):**
- `award_magic_item()`
- `award_expendable_item()`
- `award_magic_armor()`
- `award_magic_weapon()`
- `award_misc_magic_item()`
- `award_random_crystal()`
- `award_magic_ammo()`
- `award_random_money()`

**Gain Functions (limits.c, fight.c):**
- `gain_exp()`
- `hit_gain()`
- `move_gain()`
- `psp_gain()`

**Increase/Decrease Functions (limits.c):**
- `increase_gold()`
- `decrease_gold()`

**Get Functions (missions.c):**
- `get_mission_reward()`

### Architectural Problems:

#### 1. **Lack of Unified Interface**
- No single function to handle "award player X of type Y"
- Each system implements its own reward logic
- No consistent parameter passing (some use structs, some use primitives)

#### 2. **Inconsistent Error Handling**
- Some functions return values, others don't
- Inconsistent NULL checking
- Mixed error reporting methods

#### 3. **Hardcoded Values**
- Magic numbers scattered throughout functions
- No centralized configuration for reward amounts
- Difficulty adjusting reward balance

#### 4. **Poor Separation of Concerns**
- treasure.c handles both item generation AND distribution
- Combat code directly calls experience functions
- Quest system bypasses central reward mechanisms

### Recommendations for Reorganization:

#### Phase 1: Centralize Core Award Functions
**Target File: `treasure.c/.h`**
1. Move all `gain_exp()` calls to use a central `award_experience()` function
2. Move all `increase_gold()` calls to use a central `award_gold()` function
3. Create unified `award_quest_points()` function
4. Standardize all function signatures and return values

#### Phase 2: Create Unified Award Interface
**New Functions Needed:**
```c
// Central award dispatcher
int award_player(struct char_data *ch, int award_type, int amount, int grade);

// Specific award functions (all in treasure.c)
int award_experience(struct char_data *ch, int amount, int mode);
int award_gold(struct char_data *ch, int amount);
int award_quest_points(struct char_data *ch, int amount);
int award_skill_points(struct char_data *ch, int amount);
int award_bank_gold(struct char_data *ch, int amount);
// ... etc for all award types
```

#### Phase 3: Refactor Calling Code
1. Update all files to use centralized award functions
2. Remove direct manipulation of player stats
3. Ensure all rewards go through the central system

#### Phase 4: Add Configuration System
1. Move hardcoded values to configuration files
2. Add reward scaling based on level/difficulty
3. Implement reward caps and validation

### Priority Actions:

#### CRITICAL (Fix Immediately):
1. **Document Current State**: Create comprehensive function inventory
2. **Standardize Naming**: Choose consistent prefix (recommend `award_`)
3. **Centralize Core Functions**: Move `gain_exp()` and `increase_gold()` logic to treasure.c

#### HIGH (Next Sprint):
1. **Create Unified Interface**: Single entry point for all awards
2. **Refactor Admin Command**: Update `do_award()` to use centralized functions
3. **Add Error Handling**: Consistent return values and error reporting

#### MEDIUM (Future Releases):
1. **Configuration System**: Move hardcoded values to config
2. **Reward Balancing**: Centralized scaling and caps
3. **Audit Trail**: Logging for all reward distributions

### Estimated Effort:
- **Phase 1-2**: 2-3 weeks (high complexity due to dependencies)
- **Phase 3**: 1-2 weeks (systematic refactoring)
- **Phase 4**: 1 week (configuration and testing)

### Risk Assessment:
- **HIGH**: Changes affect core game mechanics
- **MEDIUM**: Extensive testing required across all reward systems
- **LOW**: Well-defined scope with clear success criteria

---

## DETAILED IMPLEMENTATION PLAN: Shop Marking on Map

### Overview
Display shops as $ symbol on ASCII minimap for better visibility. This requires integrating the existing shop system with the ASCII map rendering system.

### Technical Analysis
**Current Systems:**
- ASCII map system in `asciimap.c` with sector-based symbols in `map_info[]` and `world_map_info[]` arrays
- Shop system in `shop.c` with global `shop_index[]` array and `top_shop` counter
- Shops are defined by room vnums in `SHOP_ROOM(shop_nr, room_index)` macro
- Map generation in `MapArea()` function sets `map[x][y] = SECT(room)` for each room

**Key Functions:**
- `MapArea()` - Core map generation, currently only uses room sector type
- `StringMap()` / `CompactStringMap()` - Convert map array to display string
- `ok_shop_room()` - Static function that checks if a specific shop is in a room
- Shop data accessible via `shop_index[]` global array
- Current special sectors: `SECT_EMPTY` (38), `SECT_STRANGE`, `SECT_HERE` (for player location)

### Implementation Plan

#### Phase 1: Add Shop Detection Function
**File:** `asciimap.c`
**Function:** `int room_has_shop(room_vnum room_vnum)`
- Iterate through `shop_index[]` array (0 to `top_shop`)
- For each shop, check all rooms in `SHOP_ROOM(shop_nr, room_index)`
- Return TRUE if room_vnum matches any shop room, FALSE otherwise
- Handle NOWHERE terminator in room lists

#### Phase 2: Define Shop Sector Constant
**File:** `asciimap.c` (with map constants) or `structs.h`
- Add `#define SECT_SHOP (NUM_ROOM_SECTORS + 4)` to avoid conflicts
- Current special values: SECT_EMPTY (37), SECT_STRANGE (38), SECT_HERE (39)
- SECT_SHOP should be 40 to avoid conflicts

#### Phase 3: Add Shop Symbol to Display Arrays
**File:** `asciimap.c`
**Arrays:** `map_info[]` and `world_map_info[]`
- Add new entry: `{SECT_SHOP, "\tc[\ty$\tc]\tn"}` for regular map
- Add new entry: `{SECT_SHOP, "\ty$\tn"}` for world map
- Insert BEFORE the reserved entries (before `{-1, ""}` markers)
- Update array positions carefully to maintain reserved markers

#### Phase 4: Modify Map Generation Logic
**File:** `asciimap.c`
**Function:** `MapArea()`
- After setting base sector, check for special cases in order:
  1. Player location (`SECT_HERE`) - highest priority
  2. Shop location (`SECT_SHOP`) - medium priority  
  3. Normal sector (`SECT(room)`) - default
- Modify the existing logic around line 280-283

### Implementation Details

**Shop Detection Logic:**
```c
int room_has_shop(room_vnum room_vnum) {
  int shop_nr, room_index;

  for (shop_nr = 0; shop_nr <= top_shop; shop_nr++) {
    for (room_index = 0; SHOP_ROOM(shop_nr, room_index) != NOWHERE; room_index++) {
      if (SHOP_ROOM(shop_nr, room_index) == room_vnum) {
        return TRUE;
      }
    }
  }
  return FALSE;
}
```

**MapArea() Modification:**
```c
// Replace existing logic around lines 280-283:
/* marks the room as visited */
if (room == IN_ROOM(ch))
  map[x][y] = SECT_HERE;  // Player location takes highest precedence
else if (room_has_shop(GET_ROOM_VNUM(room)))
  map[x][y] = SECT_SHOP;  // Shop symbol takes precedence over normal sector
else
  map[x][y] = SECT(room); // Normal sector
```

### Testing Strategy
1. **Unit Testing:** Test `room_has_shop()` with known shop rooms and non-shop rooms
2. **Visual Testing:** Visit known shops and verify $ symbol appears on map
3. **Priority Testing:** Ensure player location (&) overrides shop symbol when standing in shop
4. **Edge Cases:** Test with multiple shops in same room, closed shops, room 0
5. **Performance:** Verify no significant lag with large shop counts
6. **Compatibility:** Test both regular and world map modes (`map` and `map world`)

### Potential Issues & Solutions
1. **Performance:** Shop lookup on every room - acceptable for minimap size
2. **Priority:** Player location MUST override shop symbol (already handled)
3. **Multiple Shops:** One room with multiple shops shows single $ (acceptable)
4. **Closed Shops:** Currently shows all shops regardless of hours (feature, not bug)
5. **Color Conflicts:** Yellow $ should be visible on most sector backgrounds
6. **Array Bounds:** Must maintain reserved marker positions in display arrays

### Files to Modify
- `asciimap.c` - Core implementation (shop detection function, MapArea logic, display arrays)
- May need header file for function prototype if used outside asciimap.c

### Estimated Effort
- **Complexity:** Low-Medium (integrating two existing systems)
- **Time:** 2-4 hours implementation + testing
- **Risk:** Low (minimal impact on existing functionality)
- **Dependencies:** Requires understanding of shop system and map rendering

---

## Code Documentation Comprehensive Audit Report

### Executive Summary

The LuminariMUD codebase demonstrates a mixed documentation state with areas of excellence alongside significant gaps. While file headers are consistently formatted and some systems feature exemplary documentation, the overall codebase lacks unified documentation standards and contains critical missing documentation.

### 1. Documentation File Structure

#### Well-Organized Areas:
- **docs/** directory with logical subdirectories:
  - `admin/` - Administrative documentation
  - `development/` - Developer resources
  - `guides/` - Setup and development guides
  - `systems/` - System-specific documentation
  - `testing/` - Testing documentation
  - `project-management/` - Project planning
  - `previous_changelogs/` - Historical changes

#### Documentation Coverage:
- **52 documentation files** found (*.md, *.txt, README*)
- **Key documentation present**: Setup guides, architecture docs, system descriptions
- **Recent updates**: CHANGELOG.md actively maintained (last update: 2025-01-30)

### 2. Critical Documentation Issues

#### Missing Files:
1. **TECHNICAL_DOCUMENTATION_MASTER_INDEX.md** - Referenced in 8 files but doesn't exist
   - README.md:214
   - CLAUDE.md:218
   - CONTRIBUTING.md:29, 319
   - Multiple guide files reference it

#### Outdated References:
- Multiple documentation files reference the missing master index
- Some system documentation may not reflect recent refactoring (based on CHANGELOG entries)

### 3. Code Documentation Analysis

#### Documentation Patterns:

**Excellent Examples:**
- **ai_events.c** - Modern doxygen-style with @file, @author, @brief tags
- **magic.c::compute_spell_res()** - Comprehensive function documentation
- **zone_procs.c** - Well-organized with clear section markers

**Poor Documentation:**
- **crafting_new.c** - 6,154 lines with minimal function documentation
- **spells.c** - Complex implementations lacking explanations
- **db.c** - Core functions without proper documentation
- **oedit.c** - Missing function descriptions

#### File Header Consistency:
✅ **Excellent** - All source files follow standard header format:
```c
/**************************************************************************
 *  File: filename.c                                   Part of LuminariMUD *
 *  Usage: Brief description                                              *
 **************************************************************************/
```

#### Function Documentation:
❌ **Inconsistent** - Mix of styles:
- Some files use doxygen comments (`/**`)
- Others use traditional C comments (`/*`)
- Many functions lack any documentation

### 4. Documentation Quality Metrics

#### By Category:
- **Setup/Installation**: ✅ Good - Comprehensive guides available
- **Architecture**: ✅ Good - Multiple system documents
- **API Reference**: ⚠️ Partial - DEVELOPER_GUIDE_AND_API.md exists but incomplete
- **Code Comments**: ❌ Poor - Inconsistent across codebase
- **Change Tracking**: ✅ Excellent - Detailed CHANGELOG.md

#### Documentation Freshness:
- **Recently Updated**: CHANGELOG.md, memory leak fixes documented
- **Potentially Stale**: References to refactored functions in valgrind logs
- **Active Maintenance**: Evidence of ongoing documentation updates

### 5. Consistency Analysis

#### Documentation vs Code:
✅ **COMBAT_SYSTEM.md** accurately describes `set_fighting()` implementation
✅ **Campaign system** documentation matches code structure
⚠️ **Some function references** in docs may be outdated due to refactoring

#### Cross-Reference Integrity:
❌ **Broken references** to TECHNICAL_DOCUMENTATION_MASTER_INDEX.md
✅ **Internal doc links** generally work within existing files
✅ **Code examples** in documentation appear accurate

### 6. Recommendations

#### Immediate Actions:
1. **Create TECHNICAL_DOCUMENTATION_MASTER_INDEX.md** to fix broken references
2. **Standardize on doxygen** format for all new code documentation
3. **Document critical functions** in core files (db.c, handler.c, comm.c)

#### Short-Term Improvements:
1. **Update stale references** from recent refactoring
2. **Add function documentation** to poorly documented files
3. **Create coding standards document** for documentation requirements

#### Long-Term Goals:
1. **Implement documentation generation** (doxygen configured but underutilized)
2. **Regular documentation audits** aligned with major releases
3. **Integrate documentation** into code review process

### 7. Documentation Strengths

1. **Comprehensive CHANGELOG** - Excellent change tracking
2. **Good file organization** - Logical directory structure
3. **Consistent file headers** - Professional appearance
4. **Active maintenance** - Recent updates show ongoing care
5. **Multiple documentation types** - Guides, references, and examples

### 8. Critical Gaps Summary

1. **Missing master index file** causing broken references
2. **Inconsistent function documentation** standards
3. **Undocumented complex systems** (crafting, spells, database)
4. **No unified documentation standard** enforcement
5. **Limited API documentation** for key functions

### Conclusion

The LuminariMUD documentation is functional but inconsistent. While excellent examples exist (ai_events.c, CHANGELOG.md), the lack of enforced standards results in significant gaps. The project would benefit greatly from establishing and enforcing documentation standards, particularly for function-level documentation in core system files.

**Overall Grade: C+** - Adequate for basic use but needs improvement for professional development standards.

---
