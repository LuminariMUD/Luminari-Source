# Casting Visuals System - Testing Guide

## Overview

This document provides a comprehensive testing checklist for the new casting visuals system implemented in LuminariMUD. The system adds school-specific, class-specific, and metamagic-enhanced visual descriptions to spellcasting.

---

## Quick Test Commands

Use these in-game commands to test the casting visuals:

```
cast 'magic missile' <target>     # Test EVOCATION school
cast 'cure light wounds' self     # Test CONJURATION school (healing)
cast 'detect magic'               # Test DIVINATION school
cast 'charm person' <target>      # Test ENCHANTMENT school
cast 'shield'                     # Test ABJURATION school
cast 'invisibility' self          # Test ILLUSION school
cast 'ray of enfeeblement' <tar>  # Test NECROMANCY school
cast 'bull strength' self         # Test TRANSMUTATION school
```

---

## Test Scenarios

### 1. School-Specific Visuals

**Goal**: Verify each spell school displays unique themed messages.

| School | Test Spell | Expected Visual Theme |
|--------|-----------|----------------------|
| ABJURATION | shield, mage armor | White protective shimmer, wards |
| CONJURATION | acid arrow, summon monster | Purple reality rifts, extraplanar |
| DIVINATION | detect magic, identify | Yellow inner light, visions |
| ENCHANTMENT | charm person, sleep | Magenta hypnotic, mesmerizing |
| EVOCATION | magic missile, fireball | Red crackling energy |
| ILLUSION | invisibility, blur | Blue shimmering, reality blur |
| NECROMANCY | chill touch, enervation | Dark shadows, deathly |
| TRANSMUTATION | bull strength, enlarge | Green matter shifting |

**Test Steps**:
1. [ ] Cast at least one spell from each school
2. [ ] Verify start message matches school theme
3. [ ] Verify completion message matches school theme
4. [ ] Check color codes display correctly

### 2. Target Type Variations

**Goal**: Verify messages adapt to different target types.

| Target Type | Test | Expected Variation |
|-------------|------|-------------------|
| SELF | cast 'shield' | Message refers to self |
| CHAR | cast 'cure light' <other> | Message shows target name |
| OBJ | cast 'identify' <object> | Message references object |
| NONE | cast 'detect magic' | Generic room effect |

**Test Steps**:
1. [ ] Cast a spell targeting yourself
2. [ ] Cast a spell targeting another character
3. [ ] Cast a spell targeting an object
4. [ ] Cast an area/no-target spell

### 3. Class-Specific Casting Styles

**Goal**: Verify different classes have distinct casting descriptions.

| Class | Expected Style |
|-------|---------------|
| WIZARD | "traces precise arcane sigils" |
| SORCERER | "channels raw magical energy instinctively" |
| CLERIC | "raises holy symbol" |
| DRUID | "communes with nature" |
| BARD | "weaves magic through melodic incantation" |
| PALADIN | "invokes righteous power" |
| WARLOCK | "draws upon eldritch energies" |
| PSIONICIST | "focuses mind as psionic energy builds" |

**Test Steps**:
1. [ ] Create/use characters of different casting classes
2. [ ] Cast the same spell from different classes
3. [ ] Verify class-specific style appears in start message
4. [ ] Verify school flavor additions are appended

### 4. Metamagic Visual Signatures

**Goal**: Verify metamagic modifiers display unique visual effects.

| Metamagic | Expected Visual |
|-----------|----------------|
| QUICKEN | "With impossible speed" |
| MAXIMIZE | "unnaturally dense energy pulses" |
| HEIGHTEN | "elevated magical intensity radiates" |
| EMPOWER | "crackling excess energy arcs wildly" |
| SILENT | "in eerie silence" |
| STILL | "perfectly motionless" |
| EXTEND | "elongated magical threads weave" |

**Test Steps**:
1. [ ] Cast a quickened spell - verify speed description
2. [ ] Cast an empowered spell - verify energy description
3. [ ] Cast a silent spell - verify silence description
4. [ ] Cast a spell with multiple metamagic - verify combinations

**Progress Display**:
- [ ] Metamagic shows as `[QUICKENED]`, `[EMPOWERED]`, etc.
- [ ] Multiple metamagics display all labels

### 5. Dynamic Progress Descriptions

**Goal**: Verify casting progress shows escalating narrative.

**Test with a multi-round spell (3+ rounds casting time)**

Expected progression (example for EVOCATION):
- Stage 0: "Sparks of raw energy gather at your fingertips..."
- Stage 1: "The energy crackles and grows, barely contained..."
- Stage 2: "Power surges through you, building to dangerous levels..."
- Stage 3: "The arcane force strains against your control!"
- Stage 4: "Destructive power reaches its apex!"

**Test Steps**:
1. [ ] Cast a spell with 3+ round casting time
2. [ ] Observe progress messages each round
3. [ ] Verify messages escalate in intensity
4. [ ] Verify observer messages appear to others in room
5. [ ] Verify progress dots show remaining time

### 6. Environmental Reactions

**Goal**: Verify ambient effects trigger for powerful spells.

| Spell Circle | Intensity | Chance |
|--------------|-----------|--------|
| 1-3 | Subtle | 15% per tick |
| 4-6 | Moderate | 25% per tick |
| 7-9 | Dramatic | 40% per tick |

**Test Steps**:
1. [ ] Cast low-circle spells - may see subtle effects
2. [ ] Cast mid-circle spells - moderate effects more common
3. [ ] Cast high-circle spells - dramatic effects frequent
4. [ ] Verify environmental messages go to room (not caster)
5. [ ] Verify school-themed environmental effects appear

**Toggle Test**:
- [ ] Comment out `CASTING_ENV_EFFECTS_ENABLED` in mud_options.h
- [ ] Recompile and verify no environmental effects appear
- [ ] Re-enable and verify effects return

### 7. Observer Message Tests

**Goal**: Verify observers see appropriate messages based on Spellcraft.

**Test Setup**: Need two characters in same room.

| Observer Spellcraft | Expected |
|--------------------|----------|
| High (success) | Sees actual spell name |
| Low (failure) | Sees obfuscated syllables |

**Test Steps**:
1. [ ] Have observer with high Spellcraft watch casting
2. [ ] Verify observer sees spell name in message
3. [ ] Have observer with low Spellcraft watch casting
4. [ ] Verify observer sees gibberish syllables

### 8. Interruption Scenarios

**Goal**: Verify casting visuals handle interruptions gracefully.

**Test Steps**:
1. [ ] Start casting a multi-round spell
2. [ ] Interrupt with movement - verify abort message
3. [ ] Start casting again
4. [ ] Interrupt with damage - verify abort message
5. [ ] Verify no lingering visual state

### 9. NPC Casting

**Goal**: Verify NPCs cast with appropriate visuals.

**Test Steps**:
1. [ ] Trigger NPC spellcasting (combat or script)
2. [ ] Verify NPC uses school-based messages (no class style)
3. [ ] Verify progress/observer messages work for NPC casters

---

## Performance Verification

### Memory Safety

**Verified by code review**:
- [x] All static buffers use MAX_STRING_LENGTH (49152 bytes)
- [x] All string operations use safe functions (snprintf, strlcat)
- [x] All array accesses have bounds checking
- [x] No dynamic memory allocation in message generation

### Performance Checklist

1. [ ] No noticeable lag during casting
2. [ ] Server CPU usage remains normal during heavy casting
3. [ ] Memory usage stable over extended play session
4. [ ] No crashes or hangs during casting

---

## Known Limitations

1. **Static Buffers**: Functions returning char* use static buffers. Not thread-safe, but MUD is single-threaded so this is acceptable.

2. **Environmental Effects**: Can be disabled via `CASTING_ENV_EFFECTS_ENABLED` if deemed too spammy.

3. **Class Style Coverage**: Not all classes have defined casting styles. Those without styles fall back to school-only messages.

4. **Prestige Classes**: Most prestige classes don't have unique styles and use base class behavior.

---

## Configuration Options

### mud_options.h

```c
/* Enable/disable environmental effects during casting */
#define CASTING_ENV_EFFECTS_ENABLED
```

### Tuning Probabilities

In `casting_visuals.h`:
```c
#define ENV_CHANCE_SUBTLE       15  /* Low spells: 15% per tick */
#define ENV_CHANCE_MODERATE     25  /* Mid spells: 25% per tick */
#define ENV_CHANCE_DRAMATIC     40  /* High spells: 40% per tick */
```

---

## Bug Report Template

If issues are found, document with:

```
**Bug Description**: [What went wrong]
**Steps to Reproduce**:
1. [Step 1]
2. [Step 2]
**Expected Behavior**: [What should happen]
**Actual Behavior**: [What actually happened]
**Character Info**: [Class, level, spell cast]
**Server Log**: [Any relevant log messages]
```

---

## Sign-Off

| Tester | Date | Status |
|--------|------|--------|
| | | |

---

*Testing Guide created: 2025-11-26*
*For: LuminariMUD Casting Visuals System*
