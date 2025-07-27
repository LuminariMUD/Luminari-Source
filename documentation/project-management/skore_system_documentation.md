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
- Class-themed decorative borders (Phase 2.1 - NEW!)
- Race symbols for quick identification (Phase 2.2 - NEW!)
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
- Toggle class-themed borders on/off (NEW!)
- Toggle race symbols on/off (NEW!)
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

**borders <on|off>** (NEW!)
Toggles class-themed decorative borders:
- on:  Display borders matching your primary class
- off: No borders (default)

**symbols <on|off>** (NEW!)
Toggles race symbols in the identity panel:
- on:  Show race-specific symbols
- off: No symbols (default)

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
- Class-themed decorative borders (NEW!)
- Race symbols for identification (NEW!)
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

## Remaining Manual Tasks

These tasks require in-game access or physical testing and cannot be automated:

### Help System
- **SKORE help entry**: Must be added via `hedit` command in-game
  - Complete help text available in this document (lines 11-98)
  - Command: `hedit skore`
- **SCORECONFIG help entry**: Must be added via `hedit` command in-game  
  - Complete help text available in this document (lines 102-174)
  - Command: `hedit scoreconfig`

### Testing Requirements
- **Valgrind Testing**: Run the provided script `test_skore_valgrind.sh` on a system with the compiled MUD
- **Client Compatibility**: Test with real MUD clients (MUSHclient, Mudlet, TinTin++, telnet)
- **Performance Testing**: Run the provided script `test_skore_performance.sh` 
- **Screen Reader Testing**: Requires JAWS/NVDA software and testing environment

### Known Limitations
- Equipment durability display not implemented (no durability system in codebase)
- MySQL integration uses binary saves instead of database storage for preferences

## Next Steps

After successful testing:
1. Gather user feedback on display preferences
2. Consider implementing Phase 2 features
3. Add more sophisticated equipment condition tracking
4. Implement additional color themes based on user requests

---

## Testing Scripts and Guides

### Valgrind Memory Testing Script

Save this as `test_skore_valgrind.sh`:

```bash
#!/bin/bash
# Valgrind memory testing script for SKORE/SCORECONFIG commands
# This script automates testing the enhanced score display for memory leaks

echo "=== SKORE/SCORECONFIG Valgrind Memory Testing ==="
echo "This script will test the skore and scoreconfig commands for memory leaks"
echo

# Set the path to your MUD binary
MUD_BINARY="../bin/circle"

# Check if binary exists
if [ ! -f "$MUD_BINARY" ]; then
    echo "ERROR: MUD binary not found at $MUD_BINARY"
    echo "Please compile the MUD first with: make"
    exit 1
fi

# Check if valgrind is installed
if ! command -v valgrind &> /dev/null; then
    echo "ERROR: Valgrind is not installed"
    echo "Install with: sudo apt-get install valgrind"
    exit 1
fi

# Create a test script that will be piped to the MUD
# This simulates a player logging in and using skore commands
cat > test_commands.txt << 'EOF'
1
testplayer
testpass
testpass
y
skore
skore combat
skore magic
skore stats
scoreconfig
scoreconfig width 80
skore
scoreconfig width 120
skore
scoreconfig width 160
skore
scoreconfig theme enhanced
skore
scoreconfig theme classic
skore
scoreconfig theme minimal
skore
scoreconfig density full
skore
scoreconfig density compact
skore
scoreconfig density minimal
skore
scoreconfig colors off
skore
scoreconfig colors on
skore
scoreconfig classic on
score
scoreconfig classic off
skore
scoreconfig reset
scoreconfig
skore
quit
y
EOF

echo "Running Valgrind memory leak detection..."
echo "This may take a few minutes..."
echo

# Run valgrind with leak checking
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         --log-file=valgrind_skore_test.log \
         $MUD_BINARY < test_commands.txt

echo
echo "Valgrind test complete!"
echo "Results saved to: valgrind_skore_test.log"
echo

# Extract summary from log
echo "=== MEMORY LEAK SUMMARY ==="
grep -A 10 "LEAK SUMMARY" valgrind_skore_test.log || echo "No leak summary found"

echo
echo "=== ERROR SUMMARY ==="
grep "ERROR SUMMARY" valgrind_skore_test.log || echo "No error summary found"

echo
echo "To view full results: cat valgrind_skore_test.log"
echo "To search for skore-related issues: grep -i skore valgrind_skore_test.log"

# Clean up
rm -f test_commands.txt

echo
echo "Test complete!"
```

### Performance Testing Script

Save this as `test_skore_performance.sh`:

```bash
#!/bin/bash
# Performance testing script for SKORE command
# Tests response time and concurrent execution

echo "=== SKORE Performance Testing Script ==="
echo "This script tests the performance of the skore command"
echo

# Configuration
MUD_BINARY="../bin/circle"
NUM_ITERATIONS=100
NUM_CONCURRENT=10

# Check if binary exists
if [ ! -f "$MUD_BINARY" ]; then
    echo "ERROR: MUD binary not found at $MUD_BINARY"
    echo "Please compile the MUD first with: make"
    exit 1
fi

# Function to generate test commands
generate_test_commands() {
    local num=$1
    local file=$2
    
    cat > $file << EOF
1
testuser$num
testpass
testpass
y
EOF
    
    # Add multiple skore commands
    for i in $(seq 1 $NUM_ITERATIONS); do
        echo "skore" >> $file
        echo "skore combat" >> $file
        echo "skore magic" >> $file
        echo "skore stats" >> $file
    done
    
    echo "quit" >> $file
    echo "y" >> $file
}

echo "Preparing test commands..."

# Create directory for test files
mkdir -p performance_test_tmp

# Test 1: Single user, multiple iterations
echo
echo "=== TEST 1: Single User Performance ==="
echo "Executing skore command $NUM_ITERATIONS times..."

generate_test_commands 1 performance_test_tmp/single_user_test.txt

# Run with timing
START_TIME=$(date +%s.%N)
timeout 60 $MUD_BINARY < performance_test_tmp/single_user_test.txt > performance_test_tmp/single_output.log 2>&1
END_TIME=$(date +%s.%N)

DURATION=$(echo "$END_TIME - $START_TIME" | bc)
AVG_TIME=$(echo "scale=3; $DURATION / $NUM_ITERATIONS" | bc)

echo "Total time: ${DURATION} seconds"
echo "Average time per skore command: ${AVG_TIME} seconds"
echo "Target: <0.010 seconds (10ms)"

if (( $(echo "$AVG_TIME < 0.010" | bc -l) )); then
    echo "✓ PASS: Performance meets target"
else
    echo "✗ FAIL: Performance below target"
fi

# Test 2: Concurrent users
echo
echo "=== TEST 2: Concurrent User Performance ==="
echo "Simulating $NUM_CONCURRENT concurrent users..."

# Generate test files for concurrent users
for i in $(seq 1 $NUM_CONCURRENT); do
    generate_test_commands $i performance_test_tmp/user${i}_test.txt &
done
wait

# Run concurrent tests
START_TIME=$(date +%s.%N)

for i in $(seq 1 $NUM_CONCURRENT); do
    timeout 60 $MUD_BINARY < performance_test_tmp/user${i}_test.txt > performance_test_tmp/user${i}_output.log 2>&1 &
done

# Wait for all to complete
wait

END_TIME=$(date +%s.%N)
DURATION=$(echo "$END_TIME - $START_TIME" | bc)

echo "Total time for $NUM_CONCURRENT concurrent users: ${DURATION} seconds"

# Test 3: Memory usage check
echo
echo "=== TEST 3: Memory Usage ==="
echo "Checking memory usage during skore execution..."

# Create memory test script
cat > performance_test_tmp/memory_test.txt << EOF
1
memtest
testpass
testpass
y
EOF

# Add many skore commands
for i in $(seq 1 1000); do
    echo "skore" >> performance_test_tmp/memory_test.txt
done

echo "quit" >> performance_test_tmp/memory_test.txt
echo "y" >> performance_test_tmp/memory_test.txt

# Run with memory monitoring
echo "Starting memory monitoring..."
/usr/bin/time -v timeout 60 $MUD_BINARY < performance_test_tmp/memory_test.txt > performance_test_tmp/memory_output.log 2> performance_test_tmp/memory_stats.txt

# Extract memory stats
if [ -f performance_test_tmp/memory_stats.txt ]; then
    echo "Memory statistics:"
    grep -E "Maximum resident set size|Minor|Major" performance_test_tmp/memory_stats.txt
fi

# Check perfmon output if available
echo
echo "=== TEST 4: Perfmon Integration ==="
echo "Checking perfmon statistics..."

# Look for perfmon output in logs
if grep -q "PERF_PROF" performance_test_tmp/single_output.log 2>/dev/null; then
    echo "Perfmon data found:"
    grep "PERF_PROF.*skore" performance_test_tmp/single_output.log | tail -10
else
    echo "No perfmon data found (perfmon may not be enabled)"
fi

# Summary
echo
echo "=== PERFORMANCE TEST SUMMARY ==="
echo "1. Single user average response: ${AVG_TIME} seconds"
echo "2. Concurrent test completed in: ${DURATION} seconds"
echo "3. Memory usage data saved to: performance_test_tmp/memory_stats.txt"
echo "4. Full logs saved in: performance_test_tmp/"

# Cleanup option
echo
read -p "Clean up test files? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    rm -rf performance_test_tmp
    echo "Test files cleaned up"
else
    echo "Test files preserved in performance_test_tmp/"
fi

echo
echo "Performance testing complete!"
```

### Client Compatibility Testing Guide

#### Test Clients

##### 1. Raw Telnet
**Platform:** Any OS with telnet client
**Connection:** `telnet [server] [port]`

**Test Steps:**
1. Connect using telnet
2. Log in with test character
3. Execute all skore test commands
4. Verify:
   - ASCII characters display correctly
   - Progress bars render properly
   - Colors work (if supported by terminal)
   - No strange character artifacts

##### 2. MUSHclient
**Platform:** Windows
**Download:** http://www.gammon.com.au/mushclient/

**Test Steps:**
1. Create new world connection
2. Configure ANSI color support
3. Test with different font sizes
4. Execute all skore commands
5. Verify:
   - Colors display correctly
   - Progress bars align properly
   - Wide displays (120/160) work
   - Copy/paste works correctly

##### 3. Mudlet
**Platform:** Windows/Mac/Linux
**Download:** https://www.mudlet.org/

**Test Steps:**
1. Create new profile
2. Enable UTF-8 encoding
3. Test with different window sizes
4. Execute all skore commands
5. Verify:
   - Color themes work
   - Progress bars render correctly
   - Screen reader mode works
   - Triggers don't interfere

##### 4. TinTin++
**Platform:** Unix/Linux/Mac
**Installation:** `apt-get install tintin++`

**Test Steps:**
1. Connect with: `#session luminari [server] [port]`
2. Test with different terminal sizes
3. Execute all skore commands
4. Verify:
   - ANSI colors work
   - Layout adjusts to terminal width
   - Special characters display correctly
   - Logging captures output properly

#### Test Commands Checklist

Execute these commands in each client:

```
# Basic display test
skore

# Section-specific views
skore combat
skore magic  
skore stats

# Width configurations
scoreconfig width 80
skore
scoreconfig width 120
skore
scoreconfig width 160
skore

# Theme configurations
scoreconfig theme enhanced
skore
scoreconfig theme classic
skore
scoreconfig theme minimal
skore

# Density configurations
scoreconfig density full
skore
scoreconfig density compact
skore
scoreconfig density minimal
skore

# Color toggle
scoreconfig colors off
skore
scoreconfig colors on
skore

# Classic mode toggle
scoreconfig classic on
score
scoreconfig classic off
skore

# Reset test
scoreconfig reset
scoreconfig
skore
```

#### What to Check

**Visual Elements:**
- [ ] Progress bars display correctly: `[████████░░] 80%`
- [ ] Color coding works (green/yellow/red for health)
- [ ] Class-based colors display properly
- [ ] Section headers are aligned
- [ ] No character corruption or artifacts

**Functionality:**
- [ ] All commands execute without errors
- [ ] Configuration changes take effect immediately
- [ ] Preferences persist across sessions
- [ ] Display adapts to configured width
- [ ] Information density settings work

**Performance:**
- [ ] Commands respond quickly (<1 second)
- [ ] No lag or freezing
- [ ] Scrolling is smooth
- [ ] Copy/paste works properly

---

# SKORE System Testing Checklist

## Overview
This checklist provides a quick reference for testing the SKORE system. For detailed test procedures, refer to `skore_system_documentation.md` (lines 178-444).

## Pre-Testing Setup

### 1. Compilation Check
- [ ] Run `make clean && make`
- [ ] Verify no compilation errors or warnings
- [ ] Confirm binary created at `../bin/circle`

### 2. Help Entry Setup (User Action Required)
- [ ] Log in as an immortal character
- [ ] Use `hedit` command to add SKORE help entry (text in `skore_system_documentation.md` lines 11-98)
- [ ] Use `hedit` command to add SCORECONFIG help entry (text in `skore_system_documentation.md` lines 102-174)
- [ ] Save help entries and verify with `help skore` and `help scoreconfig`

## Critical Testing Tasks

### 1. Memory Testing with Valgrind
```bash
# Run from the bin directory
valgrind --leak-check=full --show-leak-kinds=all ../bin/circle

# While running, execute these commands multiple times:
# - skore
# - skore combat
# - skore magic
# - skore stats
# - scoreconfig
# - scoreconfig width 120
# - scoreconfig reset

# Check valgrind output for:
# - Memory leaks
# - Invalid reads/writes
# - Uninitialized values
```

### 2. Performance Testing
- [ ] Execute `skore` command 100 times rapidly
- [ ] Monitor server performance (should remain responsive)
- [ ] Check perfmon logs for render times (target: <10ms)
- [ ] Test with high-level character with many items equipped

### 3. Basic Functionality Tests
- [ ] Test `skore` displays all sections correctly
- [ ] Test `skore combat` shows detailed combat stats
- [ ] Test `skore magic` shows spell slots and magic info
- [ ] Test `skore stats` shows ability scores and saves
- [ ] Verify progress bars display with correct colors
- [ ] Confirm class-based color themes work

### 4. Configuration Tests
- [ ] Test `scoreconfig` shows current settings
- [ ] Test `scoreconfig width 80/120/160` changes display width
- [ ] Test `scoreconfig theme enhanced/classic/minimal` changes colors
- [ ] Test `scoreconfig density full/compact/minimal` changes info shown
- [ ] Test `scoreconfig classic on` switches to classic score
- [ ] Test `scoreconfig colors off` disables colors
- [ ] Test `scoreconfig borders on/off` toggles class borders (NEW!)
- [ ] Test `scoreconfig symbols on/off` toggles race symbols (NEW!)
- [ ] Test `scoreconfig reset` restores defaults
- [ ] Log out and back in - verify settings persist

### 5. Character Type Tests
Test with different character types:
- [ ] Level 1 new character
- [ ] Level 10 mid-level character
- [ ] Level 20+ high-level character
- [ ] Immortal character
- [ ] Warrior (martial class)
- [ ] Wizard (arcane caster with spell slots)
- [ ] Cleric (divine caster with spell slots)
- [ ] Rogue (skill-based class)
- [ ] Psionic character (for PSP bar testing)
- [ ] Multiclass character

### 6. Client Compatibility Tests
Test display in different MUD clients:
- [ ] Raw telnet connection
- [ ] MUSHclient
- [ ] Mudlet
- [ ] TinTin++
- [ ] Web-based client (if available)

### 7. Edge Case Tests
- [ ] NPC usage (should fail gracefully)
- [ ] Character with no equipment
- [ ] Character with all equipment slots filled
- [ ] Character at 1 HP (critical health)
- [ ] Character with 0 movement points
- [ ] Invalid scoreconfig arguments

## Quick Test Commands

```bash
# Basic display test
skore
skore combat
skore magic
skore stats

# Configuration test sequence
scoreconfig
scoreconfig width 120
skore
scoreconfig theme minimal
skore
scoreconfig density compact
skore
scoreconfig classic on
skore
scoreconfig reset
scoreconfig

# Stress test (run multiple times)
skore; skore combat; skore magic; skore stats
```

## Success Criteria
- [ ] No crashes or errors during testing
- [ ] All displays render correctly
- [ ] Performance meets <10ms target
- [ ] No memory leaks detected
- [ ] Settings persist across sessions
- [ ] Works on all tested clients
- [ ] Progress bars show correct colors
- [ ] Help entries display properly

## Known Issues/Limitations
1. Equipment durability not implemented (no durability system in codebase)
2. Some clients may not support all color features
3. Terminal width detection varies by client

## Next Steps After Testing
1. Document any issues found
2. Gather user feedback on display preferences
3. Consider Phase 2 features based on user response
4. Monitor performance in production environment