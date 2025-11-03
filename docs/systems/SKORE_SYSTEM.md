# SKORE System Documentation

## Overview
Enhanced character display system with visual formatting, color coding, and customization options.

## SKORE Command
**Usage:** `skore [section]`
- Base command shows full character information
- Section views: `combat`, `magic`, `stats`

**Features:**
- Progress bars with health-based colors (green>yellow>orange>red)
- Class-themed colors and borders
- Race symbols
- 8 information panels: Identity, Vitals, Experience, Abilities, Combat, Magic, Wealth, Equipment
- 6 color themes for different preferences and accessibility needs
- Context-aware section reordering (combat mode prioritizes combat stats)
- Active spell effects display with duration progress bars

**Customization:** Use `scoreconfig` to adjust display

## Color Themes

The SKORE system includes six color themes to suit different preferences and accessibility needs:

1. **Enhanced** (default) - Full color gradient with rich colors
2. **Classic** - Minimal coloring for a traditional MUD experience
3. **Minimal** - Simple two-tone coloring
4. **High Contrast** - Bold distinct colors for improved visibility
5. **Dark** - Muted color palette for reduced eye strain
6. **Colorblind** - Carefully selected colors avoiding red-green combinations

## SCORECONFIG Command
**Usage:** `scoreconfig [option] [value]`

**Options:**
- `width <80|120|160>` - Display width
- `theme <enhanced|classic|minimal|highcontrast|dark|colorblind>` - Color theme
- `density <full|compact|minimal>` - Information shown
- `classic <on|off>` - Use classic score format
- `colors <on|off>` - Enable/disable colors
- `borders <on|off>` - Class-themed borders
- `symbols <on|off>` - Race symbols
- `reset` - Restore defaults

Preferences auto-save.

## Context Detection

The SKORE system automatically detects your current activity context and reorders sections accordingly:

- **Combat Context**: When fighting, combat stats appear first, followed by vitals and abilities
- **Shopping Context**: When in shops, wealth and equipment sections are prioritized
- **Exploring Context**: When sneaking/hiding, vitals and abilities come first
- **Roleplay Context**: When PRF_RP flag is set, maintains default order
- **Normal Context**: Default section ordering

## Active Effects Display

The magic section now includes active spell effects with visual duration indicators:

- **Progress Bars**: Shows remaining duration as a filled bar
- **Color Coding**: 
  - Green: More than 1 hour remaining
  - Yellow: 10-60 minutes remaining
  - Red: Less than 10 minutes remaining
- **Special States**:
  - Permanent effects show full white bar
  - Expiring effects show warning indicator
- **Display Format**: Shows spell number and duration in minutes

## Test Plan Summary

### 1. Basic Functionality
- Verify `skore` displays all 8 sections correctly
- Test section views: `skore combat/magic/stats`
- Confirm classic fallback: `scoreconfig classic on`

### 2. Configuration Tests
- Width: Test 80/120/160 character layouts
- Themes: Test enhanced/classic/minimal/highcontrast/dark/colorblind colors
- Density: Test full/compact/minimal information
- Toggles: Test colors/borders/symbols on/off
- Reset: Verify `scoreconfig reset` restores defaults

### 3. Character Tests
- Classes: Warrior, Wizard, Cleric, Rogue, Multiclass
- Levels: 1, 10, 20, Immortal
- Races: All available races with symbols

### 4. Equipment Tests
- Display: Verify equipment section shows correct items
- Counter: Test equipped item count accuracy

### 5. Progress Bars
- Health bars: Test color transitions (green→yellow→orange→red)
- Experience bar: Verify XP progress display

### 6. Error Handling
- Invalid inputs: Test bad scoreconfig values
- NPC usage: Verify NPCs can't use scoreconfig

### 7. Performance
- Speed: Target <10ms render time
- Memory: No leaks (valgrind test)

### 8. Client Compatibility
- Test clients: telnet, MUSHclient, Mudlet, TinTin++
- Verify ASCII rendering and color support

## Test Requirements

### Test Characters
- L1 Human Warrior, L10 Elf Wizard, L20 Dwarf Cleric
- Multiclass, Immortal, Psionic characters

### Manual Tasks Required
1. Add help entries via `hedit skore` and `hedit scoreconfig`
2. Run valgrind memory tests
3. Test client compatibility
4. Performance profiling (<10ms target)

### Known Limitations
- No equipment durability (not in codebase)
- Preferences use binary saves (not MySQL)

## Testing Scripts

### Valgrind Test (test_skore_valgrind.sh)
```bash
#!/bin/bash
# Memory leak testing for SKORE system
valgrind --leak-check=full --show-leak-kinds=all \
         --track-origins=yes --log-file=valgrind_skore.log \
         ../bin/circle < test_commands.txt
```

### Performance Test (test_skore_performance.sh)
```bash
#!/bin/bash
# Performance testing - 100 iterations, target <10ms
time for i in {1..100}; do echo "skore" | ../bin/circle; done
```

## Client Testing Guide

### Test Clients
1. **Telnet**: Basic ASCII/ANSI support
2. **MUSHclient**: Windows, full color support
3. **Mudlet**: Cross-platform, UTF-8 encoding
4. **TinTin++**: Unix/Linux, script compatibility

### Quick Test Sequence
```bash
skore
skore combat/magic/stats
scoreconfig width 80/120/160
scoreconfig theme enhanced/classic/minimal/highcontrast/dark/colorblind
scoreconfig density full/compact/minimal
scoreconfig colors off/on
scoreconfig borders off/on
scoreconfig symbols off/on
scoreconfig reset
```

## Testing Checklist

### Setup
1. Compile: `make clean && make`
2. Add help entries: `hedit skore` and `hedit scoreconfig`

### Critical Tests
1. **Memory**: `valgrind --leak-check=full ../bin/circle`
2. **Performance**: 100x execution <10ms each
3. **Functionality**: All skore variants display correctly
4. **Configuration**: All scoreconfig options work and persist
5. **Characters**: Test various classes/levels/races
6. **Clients**: Test telnet/MUSHclient/Mudlet/TinTin++
7. **Edge Cases**: NPCs, empty equipment, critical health

### Success Criteria
- No crashes/leaks
- <10ms performance
- Settings persist
- Client compatibility
- Help entries work