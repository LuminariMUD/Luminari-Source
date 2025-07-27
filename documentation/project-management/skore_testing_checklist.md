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