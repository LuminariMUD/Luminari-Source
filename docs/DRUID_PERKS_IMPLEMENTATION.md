# Druid Perk System Implementation

## Overview
Complete implementation of the Druid perk system with 45 perks across 3 enhancement trees and 4 tiers each, inspired by Dungeons & Dragons Online enhancement system.

## Implementation Date
November 4, 2024

## Files Modified

### src/structs.h
- Added druid perk categories (21-23):
  - `PERK_CATEGORY_NATURES_WARRIOR` (21)
  - `PERK_CATEGORY_SEASONS_HERALD` (22)
  - `PERK_CATEGORY_NATURES_PROTECTOR` (23)
- Updated barbarian perk categories to 24-26
- Added 45 druid perk ID definitions (PERK_DRUID_* constants, IDs 600-644)

### src/perks.h
- Added declaration: `void define_druid_perks(void);`

### src/perks.c
- Updated `perk_category_names` array with druid tree names
- Added `define_druid_perks()` function call in `init_perks()`
- Implemented complete `define_druid_perks()` function with all 45 perk definitions

## Enhancement Trees

### 1. Nature's Warrior (Perks 600-614) - Melee/Wild Shape Focus
**Theme:** Wild shape enhancement, natural weapons, and melee combat

#### Tier 1 (1 point each)
1. **Wild Shape Enhancement I** (600) - +1 attack/damage per rank (5 ranks max)
2. **Natural Armor I** (601) - +1 natural armor per rank (3 ranks max)
3. **Natural Weapons I** (602) - +1d4 weapon damage per rank (3 ranks max)
4. **Primal Instinct I** (603) - +10 HP while wild shaped per rank (3 ranks max)

#### Tier 2 (2 points each)
5. **Wild Shape Enhancement II** (604) - Additional +1 attack/damage per rank (3 ranks max)
6. **Natural Armor II** (605) - Additional +1 natural armor per rank (2 ranks max)
7. **Natural Weapons II** (606) - Critical on 19-20
8. **Improved Wild Shape** (607) - Wild shape as swift action

#### Tier 3 (3 points each)
9. **Wild Shape Enhancement III** (608) - Additional +1 attack/damage per rank (2 ranks max)
10. **Natural Armor III** (609) - Additional +1 natural armor per rank (2 ranks max)
11. **Primal Instinct II** (610) - Additional +15 HP per rank (2 ranks max)
12. **Mighty Wild Shape** (611) - +4 STR and CON in wild shape

#### Tier 4 (4 points each)
13. **Elemental Wild Shape** (612) - When in elemental form gain +3 to attack +6 to damage +3 to natural AC and +50 hp
14. **Primal Avatar** (613) - Extra attack per round when wildshaped, +4 to DEX when wildshaped
15. **Natural Fury** (614) - Critical hits deal triple damage

### 2. Season's Herald (Perks 615-629) - Elemental/Spell Focus
**Theme:** Offensive spellcasting, elemental manipulation, spell power

#### Tier 1 (1 point each)
16. **Spell Power I** (615) - +1 damage per die per rank (5 ranks max)
17. **Nature's Focus I** (616) - +1 spell DC per rank (3 ranks max)
18. **Elemental Manipulation I** (617) - +2d6 elemental damage per rank (3 ranks max)
19. **Efficient Caster** (618) - +1 circle 1,2,3 spells to prepare

#### Tier 2 (2 points each)
20. **Spell Power II** (619) - Additional +1 damage per die per rank (3 ranks max)
21. **Nature's Focus II** (620) - Additional +1 spell DC per rank (2 ranks max)
22. **Elemental Manipulation II** (621) - Additional +3d6 elemental damage per rank (2 ranks max)
23. **Spell Critical** (622) - 5% spell crit chance to deal 1.5x damage

#### Tier 3 (3 points each)
24. **Spell Power III** (623) - Additional +1 damage per die per rank (2 ranks max)
25. **Storm Caller** (624) - Lightning spells have 25% chance to do double damage
26. **Elemental Manipulation III** (625) - Additional +4d6 elemental damage per rank (2 ranks max)
27. **Nature's Wrath** (626) - 10% spell crit chance

#### Tier 4 (4 points each)
28. **Force of Nature** (627) - Spells penetrate all elemental resistances
29. **Elemental Mastery** (628) - Maximize one elemental spell for free with 2 minute cooldown
30. **Nature's Vengeance** (629) - Spell crits deal 2.0x damage

### 3. Nature's Protector (Perks 630-644) - Healing/Companion Focus
**Theme:** Healing, support spells, animal companion enhancement

#### Tier 1 (1 point each)
31. **Healing Spring I** (630) - +3 HP healing per rank (5 ranks max)
32. **Animal Bond I** (631) - Companion +10 HP, +1 attack/damage per rank (3 ranks max)
33. **Natural Remedy I** (632) - +1 HP per tick per rank (3 ranks max)
34. **Nature's Blessing** (633) - Buffs last 25% longer

#### Tier 2 (2 points each)
35. **Healing Spring II** (634) - Additional +4 HP healing per rank (3 ranks max)
36. **Animal Bond II** (635) - Companion +15 HP, +2 attack/damage per rank (2 ranks max)
37. **Natural Remedy II** (636) - Additional +2 HP per tick per rank (2 ranks max)
38. **Companion Enhancement** (637) - Companion +2 natural AC, +10% speed

#### Tier 3 (3 points each)
39. **Healing Spring III** (638) - Additional +5 HP healing per rank (2 ranks max)
40. **Animal Bond III** (639) - Companion +20 HP, +3 attack/damage per rank (2 ranks max)
41. **Rejuvenation** (640) - 15% chance healing cures negative effects
42. **Pack Leader** (641) - Companion +4 all saves

#### Tier 4 (4 points each)
43. **Nature's Guardian** (642) - Auto-heal to 50% max hp when fall below 25% HP, 10 minute cooldown
44. **Vital Surge** (643) - Healing grants 25% healed amount as increase to max hp for 5 rounds
45. **Alpha Companion** (644) - Companion becomes dire with doubled str, con, dex, hp, +5 natural ac, +3 attack and damage and 1 extra attack per round

## Prerequisite Chains

### Nature's Warrior
- Enhancement I → Enhancement II → Enhancement III → Elemental Wild Shape
- Natural Armor I → Natural Armor II → Natural Armor III
- Natural Weapons I → Natural Weapons II → Natural Fury
- Primal Instinct I → Primal Instinct II
- Enhancement II → Mighty Wild Shape → Primal Avatar

### Season's Herald
- Spell Power I → Spell Power II → Spell Power III → Force of Nature
- Nature's Focus I → Nature's Focus II
- Elemental Manipulation I → Elemental Manipulation II → Elemental Manipulation III → Elemental Mastery
- Spell Power I → Spell Critical → Nature's Wrath → Nature's Vengeance
- Elemental Manipulation II → Storm Caller

### Nature's Protector
- Healing Spring I → Healing Spring II → Healing Spring III → Nature's Guardian
- Animal Bond I → Animal Bond II → Animal Bond III → Alpha Companion
- Natural Remedy I → Natural Remedy II
- Animal Bond I → Companion Enhancement → Pack Leader
- Healing Spring II → Rejuvenation → Vital Surge

## Cost Structure
- **Tier 1:** 1 perk point per rank
- **Tier 2:** 2 perk points per rank
- **Tier 3:** 3 perk points per rank
- **Tier 4:** 4 perk points per rank

## Maximum Investment Per Tree
- **Nature's Warrior:** 51 total points (if all perks maxed)
- **Season's Herald:** 51 total points (if all perks maxed)
- **Nature's Protector:** 51 total points (if all perks maxed)

## Design Philosophy
1. **Wild Shape Enhancement:** Nature's Warrior focuses on making druids formidable melee combatants in wild shape
2. **Elemental Power:** Season's Herald emphasizes offensive spellcasting with elemental damage
3. **Support Role:** Nature's Protector enhances healing abilities and animal companion strength
4. **Flexible Builds:** Players can specialize in one tree or hybrid across multiple trees
5. **DDO Inspiration:** Inspired by DDO's enhancement system with similar progression and power scaling

## Implementation Notes
- All perks use `PERK_EFFECT_SPECIAL` type, requiring custom implementation
- Most perks support multiple ranks for gradual power scaling
- Tier 4 capstone perks are typically single-rank powerful abilities
- Prerequisite system ensures logical progression through trees
- All perks associated with `CLASS_DRUID`

## Future Work
Many perks require additional implementation work to fully function:
- Wild shape bonuses need integration with wild shape system
- Spell damage/DC bonuses need spell system hooks
- Animal companion enhancements need companion system updates
- Special effects (chain lightning, auto-heal triggers, etc.) need custom code
- Critical hit mechanics for spells need implementation

## Testing Recommendations
1. Verify perk selection UI shows all three trees correctly
2. Test prerequisite chains prevent out-of-order selection
3. Confirm perk point costs match tier structure
4. Validate multi-rank perks can be taken multiple times
5. Check that perk descriptions display correctly in-game

## Related Systems
- Wild Shape (for Nature's Warrior perks)
- Spell System (for Season's Herald perks)
- Animal Companion System (for Nature's Protector perks)
- Combat System (for damage/attack bonuses)
- Healing System (for healing bonuses)

## Compilation Status
✅ Successfully compiled on November 4, 2024
✅ All 45 perks defined
✅ No compilation errors or warnings
✅ Binary size: 16M
