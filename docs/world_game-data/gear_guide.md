# Combined Gear Stat Distribution

Sources merged:
- [docs/world_game-data/current_gear_stats_distribution.md](docs/world_game-data/current_gear_stats_distribution.md)
- [docs/world_game-data/stats-by-location-markdown.md](docs/world_game-data/stats-by-location-markdown.md)
- [docs/world_game-data/stat_distribute_on_gear_drop.md](docs/world_game-data/stat_distribute_on_gear_drop.md)

Document dates in sources:
- Stats by Wear Location: February 23, 2022
- Stat Distribution on Gear Drops: Last Updated: 03/01/2013
- Current Gear Stats Distribution: undated (tabular skeleton)

Normalization notes:
- Slot naming standardized: Finger, Neck, Wrist, Hold/Held, Feet, Hands, About, Waist, Head, Body, Arms, Legs, Shield, Wield, Wield Offhand, Wield Twohanded, Held Offhand, Held Twohanded, Light, Badge, Face, Ear.
- Stat naming standardized:
  - Primary attributes: Strength, Dexterity, Constitution, Intelligence, Wisdom, Charisma.
  - Resources: Hit Points, Movement Points, Mana Points, Psionic Points.
  - Saving throws: Fortitude Save, Reflex Save, Willpower Save.
  - Resistances: Resist Fire, Cold, Air, Earth, Acid, Holy, Electric, Unholy, Slice, Puncture, Force, Sound, Poison, Disease, Negative, Illusion, Mental, Light, Energy, Water.
- “Enhancement only” indicates no specific stat bonuses, only enhancement/special/proc effects.

Additional context incorporated from Armor Information:
- Piecemeal armor system governs AC contribution by protective slots (Head, Body, Arms, Legs).
- Object values (VAL0..VAL4) govern AC and enhancement settings.
- Builder-set fields (armor-type, material, enhancement, special abilities, custom cost) and auto-assigned fields (AC, wear pos, material, cost, max Dex, check penalty, ASF, weight, don/remove times, speeds).
- AC rating distribution across protective slots and shields, plus weight distribution tables.
- Enhancement averaging across worn gear, and stacking rules via “bonus types”.
- Random treasure/crafted gear bonus rules and caps integration with stat points.

---

## Master Mapping: Stats by Slot (Union of all sources, duplicates removed)

- FINGER (Ring)
  - Wisdom
  - Hit Points
  - Willpower Save
  - Resist Energy
  - Resist Fire
  - Resist Illusion
  - Resist Puncture

- NECK (Necklace/Amulet)
  - Intelligence
  - Reflex Save
  - Resist Air
  - Resist Cold
  - Resist Force
  - Resist Mental
  - Resist Water

- WRIST (Bracelet/Bracer)
  - Fortitude Save
  - Resist Electric
  - Resist Light
  - Resist Sound
  - Resist Unholy
  - Psionic Points
  - Mana Points
  - Note: Psionic Points (per 2022 doc); Mana Points (per 2013 doc). See “Contradictions and Variances”.

- HOLD / HELD (One-handed Held Item)
  - Intelligence
  - Charisma
  - Hit Points

- FEET (Boots)
  - Dexterity
  - Movement Points
  - Resist Poison

- HANDS (Gloves)
  - Strength
  - Resist Disease
  - Resist Slice

- ABOUT (Cloak/Cape)
  - Charisma
  - Resist Acid
  - Resist Negative

- WAIST (Belt)
  - Constitution
  - Resist Earth
  - Resist Holy

- BODY (Armor/Robe)
  - Enhancement only

- HEAD (Helmet/Crown)
  - Enhancement only

- ARMS (Armguards)
  - Enhancement only

- LEGS (Greaves/Pants)
  - Enhancement only

- SHIELD
  - Enhancement only

- WIELD (Weapon)
  - Enhancement only

- WIELD OFFHAND
  - Enhancement only

- WIELD TWOHANDED
  - Enhancement only

- HELD OFFHAND
  - Intelligence
  - Charisma
  - Hit Points

- HELD TWOHANDED
  - Intelligence
  - Charisma
  - Hit Points
  - Note: 2013 doc lists stats; 2022 doc lists “2H HOLD” under enhancement-only. See “Contradictions and Variances”.

- LIGHT (Light Source)
  - Enhancement only (per 2022 doc)

- BADGE (Badge/Emblem)
  - Enhancement only (per 2022 doc)

- FACE (Mask/Visor)
  - Enhancement only (per 2022 doc)

- EAR (Earring)
  - Enhancement only (per 2022 doc)

---

## Quick Reference Summaries (Deduplicated)

- Primary Attributes
  - Strength: HANDS
  - Dexterity: FEET
  - Constitution: WAIST
  - Intelligence: NECK, HOLD/HELD (incl. Held Offhand, Held Twohanded)
  - Wisdom: FINGER
  - Charisma: HOLD/HELD (incl. Held Offhand, Held Twohanded), ABOUT

- Resource Pools
  - Hit Points: FINGER, HOLD/HELD (incl. Held Offhand, Held Twohanded)
  - Movement Points: FEET
  - Psionic Points: WRIST (per 2022 doc)
  - Mana Points: WRIST (per 2013 doc)

- Saving Throws
  - Fortitude Save: WRIST
  - Reflex Save: NECK
  - Willpower Save: FINGER

- Elemental Resistances
  - Resist Fire: FINGER
  - Resist Cold: NECK
  - Resist Electric: WRIST
  - Resist Acid: ABOUT
  - Resist Air: NECK
  - Resist Earth: WAIST
  - Resist Water: NECK

- Physical Resistances
  - Resist Slice: HANDS
  - Resist Puncture: FINGER
  - Resist Force: NECK

- Magical/Other Resistances
  - Resist Holy: WAIST
  - Resist Unholy: WRIST
  - Resist Energy: FINGER
  - Resist Light: WRIST
  - Resist Illusion: FINGER
  - Resist Mental: NECK
  - Resist Sound: WRIST
  - Resist Negative: ABOUT
  - Resist Poison: FEET
  - Resist Disease: HANDS

- Enhancement-Only Slots (all sources, union)
  - HEAD, BODY, ARMS, LEGS
  - WIELD, WIELD OFFHAND, WIELD TWOHANDED
  - SHIELD
  - LIGHT, BADGE, FACE, EAR
  - 2H HOLD (Two-Handed Held Item) — per 2022 doc only; conflicts with 2013 doc which lists stats on Held Twohanded

---

## Piecemeal Armor System (from armor_information.md)

Reference: http://www.d20pfsrd.com/gamemastering/other-rules/piecemeal-armor

### Object Values (VAL fields)
- Value 0: Armor Class apply (AC) — not settable
- Value 1: Armor-type (or swapped with Value 2) — settable
- Value 2: unused/unspecified
- Value 3: unused/unspecified
- Value 4: Enhancement bonus — settable

### Builder-Set Fields
- Armor-type
- Material (if changing from base material)
- Enhancement bonus
- Special abilities on armor (not currently implemented)
- Custom cost (if changing from base cost)

### Auto-Assigned by Armor Struct
- Armor class (based on material, armor-type, and wear location)
- Wear position
- Base material
- Cost
- Maximum Dex bonus
- Armor check penalty
- Arcane spell failure chance
- Weight
- Don-time (optional)
- Remove-time (optional)
- Speed (30 ft) (optional)
- Speed (20 ft) (optional)

Note: The armor type struct must have armor bonus organized by wear location (determines the AC of body, arms, and legs).

---

### Armor Types: Protective Slots & AC Ratings
Number of protective slots with example ratings (Full Plate total 8.0):

| # | Slot | Full Plate (8.0) |
|---|------|-------------------|
| 1 | Head | 1.5               |
| 2 | Body | 3.5               |
| 3 | Arms | 1.5               |
| 4 | Legs | 1.5               |

### Armor Slots AC Distribution (percent-based scaling example)
| Slot          | 70  | 60  | 50  | 40  | 30  | 20  | 10  |
|---------------|-----|-----|-----|-----|-----|-----|-----|
| Head (0.1875) | 1.3 | 1.1 | 0.9 | 0.7 | 0.5 | 0.3 | 0.1 |
| Body (0.4375) | 3.1 | 2.7 | 2.3 | 1.9 | 1.5 | 1.1 | 0.7 |
| Arms (0.1875) | 1.3 | 1.1 | 0.9 | 0.7 | 0.5 | 0.3 | 0.1 |
| Legs (0.1875) | 1.3 | 1.1 | 0.9 | 0.7 | 0.5 | 0.3 | 0.1 |

### Shields
| Shield  | AC  | Chk | Dex |
|---------|-----|-----|-----|
| Buckler | 0.5 | -1  | 0   |
| Medium  | 1.0 | -2  | 0   |
| Heavy   | 2.0 | -3  | 0   |
| Tower   | 4.0 | -10 | 2   |

- Slots not listed above are supplemental and should not exceed the above numbers.

### Weight Distribution (lbs)
| Slot          | 50 | 45 | 40 | 25 | 15 | 10 |
|---------------|----|----|----|----|----|----|
| Head (0.1875) | 9  | 8  | 7  | 4  | 2  | 1  |
| Body (0.4375) | 23 | 21 | 19 | 13 | 9  | 7  |
| Arms (0.1875) | 9  | 8  | 7  | 4  | 2  | 1  |
| Legs (0.1875) | 9  | 8  | 7  | 4  | 2  | 1  |

### Object Creation Logic
- All wear-location-based modifications are applied on object creation.
- VAL0 is set to the appropriate percent of the Armor Bonus of the selected armor type.

### Enhancement Bonus Averaging
- Stacking four gear enhancement bonuses is problematic.
- Use average enhancement bonus across all worn gear instead.

### Bonus Types and Stacking
- Use “bonus types” (d20 rules) to prevent unintended stacking across multiple slots.
- Typed bonuses do not stack (highest applies). Untyped and Dodge stack.

Example:
- Typeless stacking problem:
  - Boots: +2 AC, Ring of Protection #1: +3 AC, Ring of Protection #2: +2 AC -> +7
- Typed solution:
  - Rings = deflection, Boots = dodge -> +3 (deflection highest) +2 (dodge) = +5

Implementation:
- affected_type struct adds field: bonus_type (int); values via #define.
- While applying affects, only apply the largest affect per bonus type.

### Bonuses: Random Treasure Drops
Rules:
- Armor/weapons (body, legs, arms, helm, shield, wield):
  - Enhancement bonus = level / 6 (max 5) + rarity (max 7)
- Misc pieces:
  - Various bonuses, same evaluation as armor except:
    - +Hit Points or +Movement Points = 12 points per point (max 96)

### Bonuses: In-Game and Crafted Gear
- In-game gear follows similar logic to random treasure.
- Crafted gear TBD; may reach Epic+ with rare components.

---

## Slot Inventory and Notes From “Current Gear Stats Distribution” (Structure)

The “Current Gear Stats Distribution” sheet defines the complete slot list and indicates that some are “Enhancement, Special-Ability, Proc ONLY”:
- Slots listed: Finger, Neck, Body, Head, Legs, Arms, Hands, Feet, Shield, About, Waist, Wrist, Wield, Held, Wield Offhand, Held Offhand, Wield Twohand, Held Twohand.
- Note: “Body, Head, Legs, Arms, Shield, Wielded Slots are Enhancement, Special-Ability, Proc ONLY.”
- This aligns with enhancement-only classification for Body, Head, Legs, Arms, Shield, Wield/Wield Offhand/Wield Twohanded.
- Held/Held Offhand/Held Twohanded are not listed as enhancement-only in that sheet, allowing stat bonuses, consistent with the 2013 doc, and in partial conflict with the 2022 doc for “2H HOLD.”

---

## Attribute System Definitions (from 2013 document)

Core Attributes
```
#define APPLY_NONE              0
#define APPLY_STR               1   // Strength
#define APPLY_DEX               2   // Dexterity
#define APPLY_INT               3   // Intelligence
#define APPLY_WIS               4   // Wisdom
#define APPLY_CON               5   // Constitution
#define APPLY_CHA               6   // Charisma
```

Resource Pools
```
#define APPLY_MANA             12   // Mana Points
#define APPLY_HIT              13   // Hit Points
#define APPLY_MOVE             14   // Movement Points
```

Saving Throws
```
#define APPLY_SAVING_FORT      20   // Fortitude Save
#define APPLY_SAVING_REFL      21   // Reflex Save
#define APPLY_SAVING_WILL      22   // Will Save
```

Resistances
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

---

## Game Mechanics (from 2013 document)

General Rules — Attribute Priority
- Physical attributes are more valuable than mental attributes
- Priority: Strength, Dexterity, Constitution > Intelligence, Wisdom, Charisma

Attribute Bonuses (every +2 in a stat)
- Applies +1 to related checks and certain skill checks

Specific Attribute Benefits
- Strength: +1 Hitroll, +1 Damroll, Increased Load Capacity
- Constitution: +30 Hit Points*, +1 Fortitude Save
- Dexterity: +1 AC, +1 Reflex Save
- Wisdom: Spell Slots, +1 Will Save
- Intelligence: Spell Slots, +1 Train per Level
- Charisma: Spell Slots, Better Shop Prices
- *HP bonus may vary based on system

Equipment Value System — Rare Equipment Point System
- Base Value: 2500 × character level
- Epic Crafting/Bosses (Level 30): Up to 100,000 value
- Alternative Calculation: 3333 × character level
- Reference: http://www.d20srd.org/srd/magicItems/creatingMagicItems.htm#tableEstimatingMagicItemGoldPieceValues

Practical Stat Caps (preliminary)
- 1 Strength 6
- 2 Dexterity 6
- 3 Constitution 6
- 4 Wisdom 6
- 5 Intelligence 6
- 6 Charisma 6
- 7 Max Hit Points 72
- 8 Armor Class 30
- 9 Hitroll 6
- 10 Damroll 6
- 11 Save-Fortitude 6
- 12 Save-Reflex 6
- 13 Save-Will 6
- 14 Spell Resist 6*
- 15 Shield 42*
- 16 Damage Reduction 6*
- 17 Spell Penetration 6*
- *Not yet implemented with gear

---

## Contradictions and Variances

1) Held Twohanded vs 2H Hold
- 2022 “Stats by Wear Location”: Lists “2H HOLD (Two-Handed Held Item)” in Enhancement-Only, implying no specific stats.
- 2013 “Stat Distribution on Gear Drops”: “Held Twohanded” lists Intelligence, Charisma, Hit Points.
- Impact: Contradictory classification for two-handed held items. This document preserves both claims; master mapping shows Held Twohanded with stats but flags this note.

2) Wrist Resource Stat: Psionic Points vs Mana Points
- 2022 doc: WRIST includes Psionic Points.
- 2013 doc: WRIST includes Mana Points.
- Overlap: Both agree on Fortitude Save, Resist Electric, Resist Light, Resist Sound, Resist Unholy. Resource differs.
- Impact: Variant implementations or terminology divergence. Both retained in master mapping and flagged here.

3) Enhancement-only scope for Held categories
- “Current Gear Stats Distribution” note states “Body, Head, Legs, Arms, Shield, Wielded Slots are Enhancement, Special-Ability, Proc ONLY,” which does not include Held/Held Twohanded in the enhancement-only list.
- 2022 doc includes “2H HOLD” among enhancement-only, creating an inconsistency specifically for the two-handed held variant.
- Impact: Reinforces Contradiction (1).

No other direct contradictions found; differences like separate slots (Held, Held Offhand, Held Twohanded) in 2013 vs single HOLD in 2022 are treated as granularity expansions rather than conflicts.

---

## Original “Current Gear Stats Distribution” Table (Structure Preserved)

Note: The original file provided a table layout with empty stat cells and a note about enhancement-only slots. The structure is preserved here for reference and future population. The master mapping above contains the consolidated allocations.

| Slot | Strength | Dexterity | Constitution | Intelligence | Wisdom | Charisma | HPs | MVs | Mana | Will | Reflex | Fortitude | R. Fire | R. Cold | R. Air | R. Earth | R. Acid | R. Holy | R. Electric | R. Unholy | R. Slice | R. Puncture | R. Force | R. Sound | R. Poison | R. Disease | R. Negative | R. Illusion | R. Mental | R. Light | R. Energy | R. Water |
|:-----|:---------|:----------|:-------------|:-------------|:-------|:---------|:----|:----|:-----|:-----|:-------|:----------|:--------|:--------|:-------|:---------|:--------|:--------|:------------|:----------|:---------|:------------|:---------|:---------|:----------|:-----------|:------------|:------------|:----------|:---------|:----------|:---------|
| Finger |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| Neck |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| Body |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| Head |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| Legs |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| Arms |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| Hands |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| Feet |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| Shield |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| About |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| Waist |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| Wrist |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| Wield |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| Held |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| Wield Offhand |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| Held Offhand |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| Wield Twohand |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| Held Twohand |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |

Footer note from the original sheet:
- Body, Head, Legs, Arms, Shield, Wielded Slots are Enhancement, Special-Ability, Proc ONLY.

---

## Design and Balance Notes (from 2022 document)

- Stat Distribution Philosophy:
  - Jewelry slots (Finger, Neck, Wrist) have the most stat options.
  - Armor pieces (Head, Body, Arms, Legs) are enhancement-only.
  - Weapons and held items vary in their stat availability.

- Balance Considerations:
  - Each primary attribute appears on only 1-2 slot types.
  - Resistances are distributed across multiple slots to encourage equipment diversity.
  - Critical stats like Hit Points are limited to specific slots.

- Equipment Strategy:
  - Players must choose equipment combinations to maximize desired stats.
  - No single slot provides all resistances or attributes.
  - Enhancement-only slots focus on base armor/weapon improvements.

---

## Cross-References
- More info referenced by armor_information.md: “current stat distribution on random drops” corresponds to the mappings in this combined document and stat_distribute_on_gear_drop.md.

End of combined document.