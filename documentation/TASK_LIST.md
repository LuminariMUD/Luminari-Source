# TASK LIST - Coders ToDo List

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

## User Interface Improvements

### Shop Marking on Map
- **Description**: Display shops as $ symbol on ASCII minimap for better visibility
- **Submitted by**: Rinne (Level 12)
- **Status**: Map system exists but lacks shop symbols - good QoL improvement

### Keyring Wearable Item
- **Description**: Make keyrings a wearable item (belt slot) that allows keys to be used while stored
- **Submitted by**: Rinne (Level 13), Eldek (Level 12)
- **Status**: Belt slot exists, key system exists, but no keyring container - excellent QoL improvement

### In-Character Communication
- **Description**: Telepathy item system in unique slot, restringable and craftable for IC communication
- **Submitted by**: Rinne (Level 13)
- **Status**: Would enhance roleplay, unique slot system exists

### Charmee Affects Display
- **Description**: Command to check what spells/buffs are active on charmees
- **Submitted by**: Plixid (Level 8)
- **Status**: Charmee system exists but lacks affect visibility - good utility feature

### In Wilderness Only: Location Coordinates on Prompt
- **Description**: Add room coordinates to prompt display
- **Submitted by**: Age (Level 10)
- **Status**: Coordinate system exists but not in prompt - useful for navigation

### Help in Character Creation
- **Description**: Enable help files during character creation process
- **Submitted by**: Zusuk (Level 34)
- **Status**: Help system exists but not in chargen - would help new players

### Defensive Stance Notification
- **Description**: Alert when stalwart defender's defensive stance expires
- **Submitted by**: Brondo (Level 30)
- **Status**: Class-specific QoL improvement for defensive stance users

### Compare Command
- **Description**: Add command to compare weapons and armor stats
- **Submitted by**: Darowin (Level 3)
- **Status**: Equipment system exists - very useful utility command

## Crafting System

### Stop/Cancel Crafting Command
- **Description**: Command to interrupt crafting process
- **Submitted by**: Dagmar (Level 1)
- **Status**: Crafting system exists but lacks interruption - important QoL

### Early Area Crafting Molds
- **Description**: Add armor and cloth molds to Ashenport crafting store
- **Submitted by**: Mendev (Level 20)
- **Status**: Content addition for early game crafting accessibility

### Restring Enhancements
- **Description**: Allow modification of description and material in addition to name
- **Submitted by**: Metvagen (Level 19)
- **Status**: Restring exists but limited - would enhance customization

### Periodic Crafting Bonuses
- **Description**: Crafters gain XP when their crafted items are used in combat
- **Submitted by**: Gicker (Level 34), originally by Tollymore
- **Status**: Interesting crafting incentive system

### Crafting Missions from Bazaar
- **Description**: Create crafting missions when players order from Bazaar, paying quest points
- **Submitted by**: Tollymore (Level 8)
- **Status**: Mission system exists - would integrate crafting with economy

### Crafting Recipe Adventure Loop
- **Description**: Recipe system with key components from spawned crafting nodes with guardians
- **Submitted by**: Tollymore (Level 8)
- **Status**: Would add adventure element to crafting

### Weapon Type Conversion
- **Description**: Crafting method to change weapon form without losing enchantments
- **Submitted by**: Murdoch (Level 13)
- **Status**: Useful for weapon customization without losing progress

## Combat & Class Features

### Class Feats Display for Wizards
- **Description**: Show wizard bonus feats every 5 levels in class feats display
- **Submitted by**: Ortallus (Level 4)
- **Status**: Class system exists - good information display improvement

### Berserker Acrobatics
- **Description**: Add acrobatics as class skill for berserkers (Pathfinder alignment)
- **Submitted by**: Mendev (Level 23)
- **Status**: Class skill system exists - reasonable class enhancement

### Naming Companions
- **Description**: Optional command to assign names to followers/summons/companions
- **Submitted by**: Valafar (Level 9)
- **Status**: Companion system exists - good roleplay enhancement

### Greater Feint Feat
- **Description**: New feat allowing feint as move action
- **Submitted by**: Yure (Level 3)
- **Status**: Feat system exists - reasonable combat feat addition

### Zombie Bite Attacks
- **Description**: Add bite and occasional blunt attacks to animated dead zombies
- **Submitted by**: Metvagen (Level 12)
- **Status**: Undead system exists - would make zombies more interesting

### Rogue Poison Creation
- **Description**: Weak, long-lasting poison creation for rogues without expensive materials
- **Submitted by**: Serul (Level 26)
- **Status**: Poison system exists - good class-specific utility

### Summon Shadow Teamwork
- **Description**: Grant shadow any teamwork feats the caster has
- **Submitted by**: Raiko (Level 15)
- **Status**: Teamwork feat system exists - logical enhancement

### Non-Druid Wildshape Forms
- **Description**: Allow viewing wildshape forms for polymorph self spell
- **Submitted by**: Ilzude (Level 23)
- **Status**: Wildshape system exists - good information display

### Healing Wave Spell
- **Description**: Multi-round healing spell (100-200 HP per round for 3-5 rounds)
- **Submitted by**: Melow (Level 30)
- **Status**: Spell system exists - interesting healing mechanic

### Damage Display Toggle
- **Description**: Toggle to view damage from other players/pets
- **Submitted by**: Brondo (Level 30)
- **Status**: Combat system exists - useful information toggle

### Careful With Pets Default
- **Description**: Make "careful with pets" on by default
- **Submitted by**: Malicor (Level 23)
- **Status**: Pet system exists - good default setting change

### Circle/Backstab on Blind/Paralyzed
- **Description**: Allow circle when enemy is blind/paralyzed even if tanking
- **Submitted by**: Murdoch (Level 30)
- **Status**: Combat system exists - reasonable tactical enhancement

### Class-Specific AI Casting
- **Description**: Mob casting tables by class for more realistic spellcasting
- **Submitted by**: Dudris (Level 31)
- **Status**: AI system exists - would improve mob intelligence

### Circle Combat Initiation
- **Description**: Allow circle command to engage combat when someone else is tanking
- **Submitted by**: Badase (Level 30)
- **Status**: Combat system exists - tactical improvement

### Buffself Stand Command
- **Description**: Auto-stand when using "buffself perform" command
- **Submitted by**: Lyllian (Level 7)
- **Status**: Performance system exists - good QoL improvement

## Game Mechanics

### Food/Water Rest Bonus
- **Description**: HP recovery bonus while resting with food/water
- **Submitted by**: Mendev (Level 17)
- **Status**: Rest system exists - logical survival mechanic

### Temple Association
- **Description**: Temple affiliation system filling God slot, providing quest chains
- **Submitted by**: Arvaunshae (Level 17)
- **Status**: Religion system exists - would enhance roleplay

### PSP on GUI
- **Description**: Add psionic power points to GUI display
- **Submitted by**: Zusuk (Level 34)
- **Status**: Psionics exist, GUI exists - good information display

### Auto-Sacrifice Toggle
- **Description**: Auto-grab items when sacrificing corpses
- **Submitted by**: Metvagen (Level 20)
- **Status**: Sacrifice system exists - useful automation

### Message Speed Control
- **Description**: Slow down NPC message delivery for readability
- **Submitted by**: Delax (Level 1)
- **Status**: Communication system exists - accessibility improvement

### Screen Reader Support
- **Description**: Better integration with screen reader detection
- **Submitted by**: Harlon (Level 3)
- **Status**: Accessibility improvement - important for disabled players

### Unlockable Feats List
- **Description**: Show locked feats and their requirements
- **Submitted by**: Dasvel (Level 28)
- **Status**: Feat system exists - good information display

### Independent Feat Changes
- **Description**: Allow feat changes without full respec
- **Submitted by**: Yaran (Level 9)
- **Status**: Feat system exists - good QoL improvement

### Read During Crafting
- **Description**: Allow reading news/motd while crafting
- **Submitted by**: Kormundrad (Level 3)
- **Status**: Both systems exist - good multitasking feature

### Store Food/Drink
- **Description**: Add food/drink enhancements to store feature
- **Submitted by**: Brondo (Level 30)
- **Status**: Store system exists - logical enhancement

### Poison Vial Cleanup
- **Description**: Empty poison vials should disappear after last use
- **Submitted by**: Badase (Level 30)
- **Status**: Poison system exists - good cleanup mechanic

### Locate Object Filter
- **Description**: Exclude items in player houses from locate object
- **Submitted by**: Diel (Level 30)
- **Status**: Locate system exists - prevents house snooping

### Mission Location Mapping
- **Description**: Only create missions to mapped locations
- **Submitted by**: Tanaka (Level 13)
- **Status**: Mission system exists - prevents impossible missions

### Save Command Hint
- **Description**: Add save command instruction to tutorial/hints
- **Submitted by**: Fish (Level 1)
- **Status**: Tutorial system exists - important for new players

### Quest Item Drop Command
- **Description**: Command for quest mobs to drop inventory items without killing
- **Submitted by**: Arithon (Level 30)
- **Status**: Quest system exists - useful for quest design

### Feat Point Documentation
- **Description**: Explain different types of feat points and their uses
- **Submitted by**: Neurrone (Level 21)
- **Status**: Feat system exists - important documentation improvement

### Polymorph Self Clarification
- **Description**: Help file should mention no race feats when polymorphed
- **Submitted by**: Neurrone (Level 22)
- **Status**: Polymorph system exists - documentation improvement

### Shifter Dragon Buffs
- **Description**: Increase dragon form breath weapon and spell damage significantly
- **Submitted by**: Ogoun (Level 30)
- **Status**: Shifter class exists - balance improvement for underused forms

## Quality of Life

### Unlock Door Message
- **Description**: Change "*click*" to "You unlock the door"
- **Submitted by**: Delax (Level 1)
- **Status**: Door system exists - simple message improvement

### Colorful OK Messages
- **Description**: Replace generic "okay" with more colorful responses
- **Submitted by**: Zusuk (Level 34)
- **Status**: Message system exists - flavor improvement

### Harvesting Depletion Message
- **Description**: Show "depleted" at end of harvest process instead of warning
- **Submitted by**: Variel (Level 2)
- **Status**: Harvesting system exists - better user feedback

### Beginning Journey for Blind
- **Description**: Add coordinates of half-orc camp to quest description
- **Submitted by**: Ozrim (Level 3)
- **Status**: Quest system exists - accessibility improvement

### Psionic Power Conservation
- **Description**: Don't consume PSP when power can't be manifested
- **Submitted by**: Fyre (Level 3)
- **Status**: Psionics exist - prevents resource waste

### Intimidate Duration Info
- **Description**: Add duration information to intimidate help file
- **Submitted by**: Mendev (Level 10)
- **Status**: Intimidate exists - documentation improvement

### Staff Helper for Queues
- **Description**: Add field to categorize bug/typo/idea submissions by type
- **Submitted by**: Jordan (Level 31)
- **Status**: Bug system exists - staff workflow improvement

### Spell Class/Circle Info
- **Description**: Add class and circle information to spell help files
- **Submitted by**: Murdoch (Level 30)
- **Status**: Spell system exists - important information display

### Gear Splitting System
- **Description**: Dice roll or bidding system for fair loot distribution
- **Submitted by**: Lamix (Level 30)
- **Status**: Group system exists - would reduce loot disputes

### Mosswood Elder Directions
- **Description**: Have "ask elder ashenport" give directions to Ashenport
- **Submitted by**: Mddljeu (Level 2)
- **Status**: NPC system exists - specific content improvement

## New Spells & Abilities

### Dimension Door
- **Description**: Implement dimension door spell
- **Submitted by**: Gargar (Level 22)
- **Status**: Spell system exists - classic D&D spell addition

### Dispel Invisibility
- **Description**: Spell to remove invisibility from equipment
- **Submitted by**: Melaw (Level 30)
- **Status**: Invisibility system exists - useful counter-spell

### Area Wall Spells
- **Description**: Prismatic cube or force cube area denial spells
- **Submitted by**: Melaw (Level 30)
- **Status**: Spell system exists - tactical area control spells

### Healer Class Unlock
- **Description**: Prestige class with healing bonuses and faster memorization
- **Submitted by**: Melaw (Level 30)
- **Status**: Class system exists - specialized healing class

## NPC & World Interaction

### Ashenport Tourist Quests
- **Description**: Small escort missions for lost tourists in city
- **Submitted by**: Metvagen (Level 12)
- **Status**: Quest system exists - good low-level content

### Smoking System
- **Description**: Immersive smoking system for roleplay
- **Submitted by**: Arvaunshae (Level 17)
- **Status**: Item system exists - roleplay enhancement

## Accessibility

### Symbol Alternatives
- **Description**: Use letters/words instead of punctuation for screen readers
- **Submitted by**: Dranulous (Level 2)
- **Status**: Display system exists - important accessibility improvement

### Plane Travel Effect
- **Description**: Add humorous message for maintained velocity after recall from falling
- **Submitted by**: Iliri (Level 26)
- **Status**: Recall system exists - fun flavor addition

