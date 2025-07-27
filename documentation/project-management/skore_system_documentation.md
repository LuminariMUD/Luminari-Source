# Enhanced Score Display Documentation & Test Plan

## Overview
This document contains the complete documentation for the enhanced score display system including:
- SKORE command help documentation
- SCORECONFIG command help documentation  
- Comprehensive test plan for Phase 1 MVP implementation

---

## SKORE Command Documentation

### Usage: skore [section]

The skore command displays an enhanced version of your character information
with improved visual formatting, color coding, and organized information panels.

You can also view specific sections in detail:
- `skore combat` - Detailed combat statistics
- `skore magic` - Detailed magic and psionic information  
- `skore stats` - Detailed ability scores and saves

### FEATURES:

**Visual Enhancements:**
- Progress bars for Hit Points, Movement, and PSP
- Health-based color coding (green=healthy, yellow=wounded, red=critical)
- Class-based color themes for different character classes
- Organized information panels with clear sections

**Information Sections:**
- Character Identity: Name, race, class, alignment, age, size
- Vitals & Condition: HP, Movement, PSP, physical stats, carrying capacity
- Experience & Progression: Experience points, level progress, caster levels
- Ability Scores & Saves: All six abilities with bonuses and saving throws
- Combat Statistics: BAB, attacks, AC, damage reduction, weapon information
- Magic & Psionics: Spell bonuses, spell slots by class, psionic information
- Wealth & Achievements: Gold, bank, quest points, crafting jobs
- Equipment Status: Key equipment and item counts (full density mode)

**Customization:**
The skore display can be customized using the 'scoreconfig' command:
- Adjust display width (80, 120, or 160 characters)
- Change color themes (enhanced, classic, minimal)
- Control information density (full, compact, minimal)
- Toggle colors on/off
- Switch back to classic score format

**Progress Bars:**
Visual progress bars show current/maximum values:
[████████░░] 80%  - 80% of maximum
Colors change based on health:
- Green: 75%+ (healthy)
- Yellow: 50-74% (wounded)
- Orange: 25-49% (badly wounded)  
- Red: 0-24% (critical)

**Class Colors:**
Different character classes use themed colors:
- Blue: Arcane casters (Wizard, Sorcerer, Bard)
- Green: Divine casters (Cleric, Druid, Paladin, Ranger)
- Red: Martial classes (Warrior, Berserker, Monk)
- Magenta: Skill-based classes (Rogue)
- Cyan: Other classes

### EXAMPLES:

```
skore
  Shows your full enhanced character information

skore combat
  Shows detailed combat statistics

skore magic
  Shows detailed magic and psionic information

skore stats
  Shows detailed ability scores and saves

scoreconfig classic on
  Switches to classic score display

scoreconfig width 120
  Uses wider display format
```

### RELATED COMMANDS:

- score       - Classic character information display
- scoreconfig - Customize the enhanced display
- equipment   - Detailed equipment listing
- affects     - Current spell effects

The enhanced score display provides improved visual hierarchy and better
information organization while maintaining compatibility with all MUD clients.

See also: SCORE, SCORECONFIG, EQUIPMENT, AFFECTS

---

## SCORECONFIG Command Documentation

### Usage: scoreconfig [option] [value]

The scoreconfig command allows you to customize your enhanced score display.
When used without arguments, it shows your current configuration.

### OPTIONS:

**width <80|120|160>**
Sets the display width for your score. Choose from:
- 80:  Standard width (default)
- 120: Wide display for larger terminals
- 160: Extra wide display

**theme <enhanced|classic|minimal>**
Sets the color theme for your score display:
- enhanced: Full color theme with class-based colors (default)
- classic:  Traditional color scheme
- minimal:  Reduced color usage

**density <full|compact|minimal>**
Controls how much information is displayed:
- full:    All available information (default)
- compact: Condensed layout with key information
- minimal: Only essential statistics

**classic <on|off>**
Toggles between enhanced and classic score display:
- on:  Use the original score command format
- off: Use the enhanced skore display (default)

**colors <on|off>**
Enables or disables color in the score display:
- on:  Show colors (default)
- off: Plain text display

**reset**
Resets all score display settings to their defaults.

### EXAMPLES:

```
scoreconfig width 120
  Sets display width to 120 characters

scoreconfig theme minimal
  Uses minimal color theme

scoreconfig classic on
  Switches to classic score display

scoreconfig reset
  Resets all settings to defaults
```

### RELATED COMMANDS:

- score  - Shows character information (classic format)
- skore  - Shows enhanced character information
- toggle - Other display preferences

The enhanced score display includes:
- Visual progress bars for HP, Movement, and PSP
- Health-based color coding
- Class-based color themes
- Organized information panels
- Equipment status display
- Enhanced visual hierarchy

Your preferences are automatically saved when changed.

See also: SCORE, SKORE, TOGGLE

---

## Test Plan

## Test Categories

### 1. Basic Functionality Tests

#### Test 1.1: Enhanced Score Display
**Objective:** Verify the enhanced score display renders correctly
**Steps:**
1. Log in as a test character
2. Execute `skore` command
3. Verify all sections display:
   - Character Identity
   - Vitals & Condition
   - Experience & Progression
   - Ability Scores & Saves
   - Combat Statistics
   - Magic & Psionics (if applicable)
   - Wealth & Achievements
   - Equipment Status (full density only)

**Expected Result:** All sections display with proper formatting, colors, and progress bars

#### Test 1.2: Classic Score Fallback
**Objective:** Verify classic score preference works
**Steps:**
1. Execute `scoreconfig classic on`
2. Execute `skore` command
3. Verify classic score display is shown instead

**Expected Result:** Classic score display is used when preference is enabled

### 2. Configuration System Tests

#### Test 2.1: Width Configuration
**Objective:** Test responsive width handling
**Steps:**
1. Execute `scoreconfig width 80`
2. Execute `skore` and verify 80-character layout
3. Execute `scoreconfig width 120`
4. Execute `skore` and verify 120-character layout
5. Execute `scoreconfig width 160`
6. Execute `skore` and verify 160-character layout

**Expected Result:** Display adapts to configured width

#### Test 2.2: Color Theme Configuration
**Objective:** Test color theme options
**Steps:**
1. Execute `scoreconfig theme enhanced`
2. Execute `skore` and verify enhanced colors
3. Execute `scoreconfig theme classic`
4. Execute `skore` and verify classic colors
5. Execute `scoreconfig theme minimal`
6. Execute `skore` and verify minimal colors

**Expected Result:** Color themes change appropriately

#### Test 2.3: Information Density Configuration
**Objective:** Test information density options
**Steps:**
1. Execute `scoreconfig density full`
2. Execute `skore` and verify all sections shown
3. Execute `scoreconfig density compact`
4. Execute `skore` and verify compact layout
5. Execute `scoreconfig density minimal`
6. Execute `skore` and verify minimal information

**Expected Result:** Information density affects what sections are displayed

#### Test 2.4: Color Toggle
**Objective:** Test color on/off functionality
**Steps:**
1. Execute `scoreconfig colors off`
2. Execute `skore` and verify no colors
3. Execute `scoreconfig colors on`
4. Execute `skore` and verify colors restored

**Expected Result:** Colors can be toggled on/off

#### Test 2.5: Configuration Reset
**Objective:** Test reset to defaults
**Steps:**
1. Change multiple settings
2. Execute `scoreconfig reset`
3. Execute `scoreconfig` to view settings
4. Verify all settings are at defaults

**Expected Result:** All settings reset to default values

### 3. Character Type Tests

#### Test 3.1: Different Classes
**Objective:** Verify display works for different character classes
**Test Characters:**
- Warrior (martial class)
- Wizard (arcane caster)
- Cleric (divine caster)
- Rogue (skill-based)
- Multiclass character

**Expected Result:** Class-specific colors and information display correctly

#### Test 3.2: Different Levels
**Objective:** Test display at various character levels
**Test Levels:**
- Level 1 (new character)
- Level 10 (mid-level)
- Level 20 (high level)
- Immortal level

**Expected Result:** Level-appropriate information displays correctly

#### Test 3.3: Different Races
**Objective:** Verify race information displays correctly
**Test Races:**
- Human
- Elf
- Dwarf
- Other available races

**Expected Result:** Race information displays correctly in identity panel

### 4. Equipment Tests

#### Test 4.1: Equipment Display
**Objective:** Test equipment status section
**Steps:**
1. Equip various items (weapon, armor, shield)
2. Execute `skore` with full density
3. Verify equipment section shows equipped items
4. Remove equipment and verify updates

**Expected Result:** Equipment section accurately reflects equipped items

#### Test 4.2: Equipment Count
**Objective:** Test equipped items counter
**Steps:**
1. Start with no equipment
2. Gradually equip items
3. Verify counter increases correctly
4. Remove items and verify counter decreases

**Expected Result:** Equipment counter is accurate

### 5. Progress Bar Tests

#### Test 5.1: Health Progress Bars
**Objective:** Test HP/Movement/PSP progress bars
**Steps:**
1. Test with full health/movement/PSP
2. Reduce health and verify color changes
3. Test with critical health levels
4. Test PSP bars for psionic characters

**Expected Result:** Progress bars display correctly with appropriate colors

#### Test 5.2: Experience Progress Bar
**Objective:** Test experience progress display
**Steps:**
1. Test with character needing experience
2. Gain experience and verify progress updates
3. Test with immortal character (no exp needed)

**Expected Result:** Experience progress displays correctly

### 6. Error Handling Tests

#### Test 6.1: Invalid Configuration Values
**Objective:** Test error handling for invalid inputs
**Steps:**
1. Execute `scoreconfig width 50` (invalid)
2. Execute `scoreconfig theme invalid` (invalid)
3. Execute `scoreconfig density invalid` (invalid)

**Expected Result:** Appropriate error messages displayed

#### Test 6.2: NPC Usage
**Objective:** Verify NPCs cannot use scoreconfig
**Steps:**
1. Switch to NPC
2. Execute `scoreconfig`

**Expected Result:** Error message about NPCs not being able to configure

### 7. Performance Tests

#### Test 7.1: Rendering Speed
**Objective:** Verify score renders quickly
**Steps:**
1. Execute `skore` multiple times
2. Measure rendering time
3. Test with complex characters (high level, lots of equipment)

**Expected Result:** Renders in <50ms for 95% of requests

#### Test 7.2: Memory Usage
**Objective:** Verify no memory leaks
**Steps:**
1. Execute `skore` repeatedly
2. Monitor memory usage
3. Check for memory leaks

**Expected Result:** No memory leaks detected

### 8. Compatibility Tests

#### Test 8.1: Different MUD Clients
**Objective:** Test across different clients
**Test Clients:**
- Basic telnet
- MUSHclient
- Mudlet
- TinTin++

**Expected Result:** Display works correctly across all clients

#### Test 8.2: Terminal Width Detection
**Objective:** Test automatic width detection
**Steps:**
1. Change terminal width
2. Execute `skore`
3. Verify display adapts appropriately

**Expected Result:** Display adapts to terminal width when possible

## Test Data Requirements

### Test Characters Needed:
1. Level 1 Human Warrior
2. Level 10 Elf Wizard
3. Level 20 Dwarf Cleric
4. Multiclass Rogue/Fighter
5. Immortal character
6. Psionic character (for PSP testing)

### Test Equipment Needed:
1. Various weapons (different types)
2. Armor pieces (different slots)
3. Shields
4. Accessories (rings, amulets, etc.)

## Success Criteria

The enhanced score system passes testing if:
1. All display sections render correctly
2. All configuration options work as expected
3. Performance meets requirements (<50ms render time)
4. No crashes or errors occur
5. Display is compatible across different clients
6. Preferences are saved and loaded correctly
7. Equipment status displays accurately
8. Progress bars function correctly with appropriate colors

## Known Limitations

1. Equipment durability system not yet implemented
2. Advanced customization features (Phase 2+) not included
3. Some color themes may need refinement based on user feedback

## Next Steps

After successful testing:
1. Gather user feedback on display preferences
2. Consider implementing Phase 2 features
3. Add more sophisticated equipment condition tracking
4. Implement additional color themes based on user requests
