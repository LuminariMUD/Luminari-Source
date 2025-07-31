# Stat Distribution on Gear Drops

## Equipment Slot Stat Distributions

This document outlines the possible stat distributions for different equipment slots in the game.

### Primary Equipment Slots

| Equipment Slot | Available Stats | Notes |
|---------------|-----------------|-------|
| **Worn On Finger** (1st) | wisdom, save-will, hit-points, res-fire, res-puncture, res-illusion, res-energy | See "Luminari - Armor Notes" |
| **Worn On Finger** (2nd) | Same as 1st slot | Second ring slot |
| **Worn Around Neck** (1st) | intelligence, save-reflex, res-cold, res-air, res-force, res-mental, res-water | |
| **Worn Around Neck** (2nd) | Same as 1st slot | Second neck slot |
| **Worn On Body** | enhancement only | Slot #5 |
| **Worn On Head** | enhancement only | |
| **Worn On Legs** | enhancement only | |
| **Worn On Feet** | dexterity, movement-points, res-poison | |
| **Worn On Hands** | strength, res-slice, res-disease | |
| **Worn On Arms** | enhancement only | Slot #10 |
| **Worn As Shield** | enhancement only | |
| **Worn About Body** | charisma, res-acid, res-negative | |
| **Worn About Waist** | constitution, res-holy, res-earth | |
| **Worn Around Wrist** (1st) | save-fort, mana-points, res-electricity, res-unholy, res-sound, res-light | |
| **Worn Around Wrist** (2nd) | Same as 1st slot | Slot #15, Second wrist slot |
| **Wielded** | enhancement only | |
| **Held** | intelligence, charisma, hp | Marked with "?" |
| **Wielded Offhand** | enhancement only | |
| **Held Offhand** | intelligence, charisma, hp | |
| **Wielded Twohanded** | enhancement only | Slot #20 |
| **Held Twohanded** | intelligence, charisma, hp | |

## Attribute System Definitions

### Core Attributes
```
#define APPLY_NONE              0
#define APPLY_STR               1   // Strength
#define APPLY_DEX               2   // Dexterity
#define APPLY_INT               3   // Intelligence
#define APPLY_WIS               4   // Wisdom
#define APPLY_CON               5   // Constitution
#define APPLY_CHA               6   // Charisma
```

### Resource Pools
```
#define APPLY_MANA             12   // Mana Points
#define APPLY_HIT              13   // Hit Points
#define APPLY_MOVE             14   // Movement Points
```

### Saving Throws
```
#define APPLY_SAVING_FORT      20   // Fortitude Save
#define APPLY_SAVING_REFL      21   // Reflex Save
#define APPLY_SAVING_WILL      22   // Will Save
```

### Resistances
```
#define APPLY_RES_FIRE         28   // Fire Resistance
#define APPLY_RES_COLD         29   // Cold Resistance
#define APPLY_RES_AIR          30   // Air Resistance
#define APPLY_RES_EARTH        31   // Earth Resistance
#define APPLY_RES_ACID         32   // Acid Resistance
#define APPLY_RES_HOLY         33   // Holy Resistance
#define APPLY_RES_ELECTRIC     34   // Electric Resistance
#define APPLY_RES_UNHOLY       35   // Unholy Resistance
#define APPLY_RES_SLICE        36   // Slicing Resistance
#define APPLY_RES_PUNCTURE     37   // Puncture Resistance
#define APPLY_RES_FORCE        38   // Force Resistance
#define APPLY_RES_SOUND        39   // Sound Resistance
#define APPLY_RES_POISON       40   // Poison Resistance
#define APPLY_RES_DISEASE      41   // Disease Resistance
#define APPLY_RES_NEGATIVE     42   // Negative Resistance
#define APPLY_RES_ILLUSION     43   // Illusion Resistance
#define APPLY_RES_MENTAL       44   // Mental Resistance
#define APPLY_RES_LIGHT        45   // Light Resistance
#define APPLY_RES_ENERGY       46   // Energy Resistance
#define APPLY_RES_WATER        47   // Water Resistance
```

## Game Mechanics

### General Rules

**Attribute Priority:**
- Physical attributes are more valuable than mental attributes
- Priority order: **Strength, Dexterity, Constitution > Intelligence, Wisdom, Charisma**

### Attribute Bonuses

Every two points in a stat provides a +1 bonus to:
- Stat-related checks
- Certain skill checks

#### Specific Attribute Benefits

| Attribute | Bonus per 2 Points |
|-----------|-------------------|
| **Strength** | +1 Hitroll, +1 Damroll, Increased Load Capacity |
| **Constitution** | +30 Hit Points*, +1 Fortitude Save |
| **Dexterity** | +1 AC, +1 Reflex Save |
| **Wisdom** | Spell Slots, +1 Will Save |
| **Intelligence** | Spell Slots, +1 Train per Level |
| **Charisma** | Spell Slots, Better Shop Prices |

*Note: HP bonus may vary based on system

## Equipment Value System

### Rare Equipment Point System
- **Base Value:** 2500 × character level
- **Epic Crafting/Bosses (Level 30):** Up to 100,000 value
- **Alternative Calculation:** 3333 × character level

### Point Values
For simplicity, most stats are valued at **1 point each**, except:
- **Hit Points:** 12 points per HP
- **Movement Points:** 12 points per move

Reference: [D20 SRD Magic Item Values](http://www.d20srd.org/srd/magicItems/creatingMagicItems.htm#tableEstimatingMagicItemGoldPieceValues)

## Practical Stat Caps

| Apply Type | Stat | Preliminary Cap |
|------------|------|-----------------|
| 1 | Strength | 6 |
| 2 | Dexterity | 6 |
| 3 | Constitution | 6 |
| 4 | Wisdom | 6 |
| 5 | Intelligence | 6 |
| 6 | Charisma | 6 |
| 7 | Max Hit Points | 72 |
| 8 | Armor Class | 30 |
| 9 | Hitroll | 6 |
| 10 | Damroll | 6 |
| 11 | Save-Fortitude | 6 |
| 12 | Save-Reflex | 6 |
| 13 | Save-Will | 6 |
| 14 | *Spell Resist | 6 |
| 15 | *Shield | 42 |
| 16 | *Damage Reduction | 6 |
| 17 | *Spell Penetration | 6 |

*Not yet implemented with gear

---

*Last Updated: 03/01/2013*

## Notes
- "Enhancement only" slots can only receive enhancement bonuses, not specific stat bonuses
- Some equipment slots have multiple versions (1st and 2nd) allowing for duplicate items
- The system uses a D20-based mechanics framework