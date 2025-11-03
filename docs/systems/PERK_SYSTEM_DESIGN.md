# Perk System Design Document

## Overview
The perk system provides class-specific character customization through purchasable enhancements. Players earn 3 perk points per level (stages 1-3 of each level), allowing them to specialize their characters beyond base class features.

## Perk Point Calculation

### Level Progression
- Each level has 5 stages (0-4)
- Stage 0: Initial stage after gaining a level (no perk point)
- Stages 1-3: Each awards 1 perk point (3 per level)
- Stage 4: Ready to level up (no perk point)

### Maximum Perk Points by Class

| Class Type | Max Level | Total Perk Points | Design Target |
|------------|-----------|-------------------|---------------|
| Base Classes (max -1) | 30 | 90 | 120+ perks worth |
| Prestige 20 Level | 20 | 60 | 80+ perks worth |
| Prestige 10 Level | 10 | 30 | 40+ perks worth |

### Class Max Levels Reference

**Base Classes (30 levels = 90 perk points)**
- Wizard, Cleric, Rogue, Warrior, Monk, Druid, Berserker, Sorcerer, Paladin
- Blackguard, Ranger, Bard, Psionicist, Alchemist, Inquisitor, Summoner
- Assassin, Necromancer, Warlock, Artificer

**Prestige Classes - 20 Levels (60 perk points)**
- Knight of Solamnia, Knight of the Lily, Knight of the Thorn, Knight of the Skull
- Dragonrider, Shifter

**Prestige Classes - 10 Levels (30 perk points)**
- Weapon Master, Arcane Archer, Arcane Shadow, Eldritch Knight, Spellsword
- Sacred Fist, Stalwart Defender, Shadowdancer, Duelist, Mystic Theurge

## Perk Design Philosophy

### Inspiration Sources
- D&D Online Enhancement Trees
- Pathfinder Feat System
- D&D 5e Feat System
- Class-specific mechanics that enhance core abilities

### Design Principles

1. **Meaningful Choices**: Each perk should provide tangible benefits
2. **Build Diversity**: Multiple viable paths within each class
3. **Synergy**: Perks should work together to create builds
4. **Balance**: No single perk should be mandatory or overpowering
5. **Theme**: Perks should fit the class fantasy and role

### Perk Categories by Role

#### Combat Classes (Warrior, Monk, Berserker, Weapon Master, etc.)
- Weapon specialization (damage, accuracy, critical hits)
- Defensive capabilities (AC, DR, saves)
- Combat maneuvers (special attacks, positioning)
- Durability (HP, regeneration, stamina)

#### Divine Casters (Cleric, Druid, Paladin, Inquisitor)
- Healing enhancements
- Channel energy improvements
- Domain/sphere power boosts
- Turn undead upgrades
- Defensive buffs for party

#### Arcane Casters (Wizard, Sorcerer, Warlock, Necromancer)
- Spell DC increases
- Spell damage boosts
- Metamagic enhancements
- School specialization
- Spell point efficiency

#### Skill-Based (Rogue, Bard, Ranger, Assassin)
- Skill bonuses
- Stealth improvements
- Sneak attack enhancements
- Skill versatility
- Special abilities (evasion, uncanny dodge)

#### Hybrid Classes (Paladin, Ranger, Bard, Eldritch Knight, etc.)
- Balance between martial and casting
- Unique class features
- Party support abilities
- Versatility enhancements

## Perk Tiers

### Tier 1: Entry Level (Cost 1)
- Basic improvements to class abilities
- Small numerical bonuses (+1-2)
- Prerequisite: None
- Examples: +2 HP, +1 to a save, +1 to a skill

### Tier 2: Intermediate (Cost 2)
- Moderate improvements
- Conditional bonuses
- Prerequisite: May require Tier 1 perks
- Examples: +1 spell DC, +1d6 damage on critical, improved class feature

### Tier 3: Advanced (Cost 3-4)
- Significant improvements
- New abilities or major enhancements
- Prerequisite: Usually requires lower tier perks
- Examples: Extra attack, special abilities, major spell improvements

### Tier 4: Capstone (Cost 5+)
- Powerful unique abilities
- Build-defining features
- Prerequisite: High level requirement, multiple prerequisites
- Examples: Signature abilities, major combat advantages

## Perk Trees by Class

### Example: Warrior Perk Trees

#### Tree 1: Weapon Specialist (40 points possible)
1. **Weapon Focus I** (1 pt) - +1 to hit with chosen weapon
2. **Weapon Focus II** (1 pt, req: Focus I) - +2 to hit total
3. **Weapon Specialization I** (2 pts, req: Focus II) - +2 damage
4. **Weapon Specialization II** (2 pts, req: Spec I) - +4 damage total
5. **Greater Weapon Focus** (3 pts, req: Focus II) - +3 to hit total
6. **Greater Weapon Specialization** (3 pts, req: Spec II) - +6 damage total
7. **Weapon Master** (5 pts, req: Greater of both) - Critical threat range +1

#### Tree 2: Defender (40 points possible)
1. **Armor Training I** (1 pt) - +1 AC
2. **Armor Training II** (1 pt, req: Training I) - +2 AC total
3. **Shield Mastery I** (2 pts) - +2 AC with shield
4. **Shield Mastery II** (2 pts, req: Mastery I) - +4 AC with shield total
5. **Defensive Stance** (3 pts) - Damage reduction 2/-
6. **Improved Damage Reduction** (3 pts, req: Stance) - DR 4/-
7. **Immovable Object** (5 pts, req: Improved DR) - DR 6/-, immunity to knockdown

#### Tree 3: Tactical Fighter (40 points possible)
1. **Combat Reflexes** (1 pt) - +1 attack of opportunity per round
2. **Improved Trip** (2 pts) - +4 to trip attempts
3. **Improved Disarm** (2 pts) - +4 to disarm attempts
4. **Mobility** (2 pts) - +4 AC vs attacks of opportunity
5. **Spring Attack** (3 pts, req: Mobility) - Move-attack-move action
6. **Whirlwind Attack** (4 pts) - Attack all adjacent enemies
7. **Perfect Strike** (5 pts) - Once per combat, automatic critical hit

### Additional Design Notes

Each class should have:
- 3-4 major perk trees (35-45 points each)
- Mix of passive and active abilities
- Clear progression paths
- Enough variety for different playstyles

Total perks per class:
- Base classes: 120-150 points worth of perks
- 20-level prestige: 80-100 points worth
- 10-level prestige: 40-50 points worth

## Perk Effects System

### Effect Types
```
PERK_EFFECT_NONE              - No mechanical effect (purely flavor)
PERK_EFFECT_HP                - Increases max HP
PERK_EFFECT_SPELL_POINTS      - Increases max spell points
PERK_EFFECT_ABILITY_SCORE     - Modifies STR/DEX/CON/INT/WIS/CHA
PERK_EFFECT_SAVE              - Modifies saving throws
PERK_EFFECT_AC                - Modifies armor class
PERK_EFFECT_SKILL             - Modifies skill ranks/bonuses
PERK_EFFECT_WEAPON_DAMAGE     - Adds damage to weapons
PERK_EFFECT_WEAPON_TOHIT      - Adds to-hit bonus
PERK_EFFECT_SPELL_DC          - Increases spell save DC
PERK_EFFECT_SPELL_DAMAGE      - Increases spell damage
PERK_EFFECT_SPELL_DURATION    - Increases spell duration
PERK_EFFECT_CASTER_LEVEL      - Increases effective caster level
PERK_EFFECT_DAMAGE_REDUCTION  - Adds damage reduction
PERK_EFFECT_SPELL_RESISTANCE  - Adds spell resistance
PERK_EFFECT_CRITICAL_MULT     - Increases critical multiplier
PERK_EFFECT_CRITICAL_CHANCE   - Increases critical threat range
PERK_EFFECT_SPECIAL           - Special/unique effect requiring code
```

## Implementation Priority

### Phase 1: Core Combat Classes
1. Warrior - 3 trees (Weapon, Defender, Tactical)
2. Rogue - 3 trees (Assassin, Thief, Scout)
3. Cleric - 3 trees (Healer, Battle Cleric, Domain Focus)
4. Wizard - 3 trees (Evoker, Controller, Versatile Caster)

### Phase 2: Popular Classes
5. Monk - 3 trees (Martial Artist, Ki Master, Defensive)
6. Ranger - 3 trees (Archer, Dual Wield, Beast Master)
7. Paladin - 3 trees (Holy Warrior, Defender, Smite)
8. Sorcerer - 3 trees (Blaster, Battlefield Control, Social)

### Phase 3: Remaining Base Classes
9. Druid - 3 trees (Wildshape, Caster, Natural Bond)
10. Bard - 3 trees (Support, Performer, Skill Master)
11. And so on...

### Phase 4: Prestige Classes
- Focus on prestige classes after base classes are complete
- Prestige perks should enhance prestige class features

## Database Schema

Perks are stored in the `perks` table:
```sql
CREATE TABLE perks (
  id INT PRIMARY KEY,
  name VARCHAR(100),
  description TEXT,
  associated_class INT,
  cost INT,
  max_rank INT,
  prerequisite_perk INT,
  prerequisite_rank INT,
  effect_type INT,
  effect_value INT,
  effect_modifier INT,
  special_description TEXT
);
```

Character perks are stored in player files and tracked in memory.

## Quality Assurance

### Testing Checklist
- [ ] Perk points awarded correctly on level up
- [ ] Perk points display in score command
- [ ] Perks list shows correct class name
- [ ] Perk info displays correctly
- [ ] Perk purchase works and deducts points
- [ ] Perk effects apply to character
- [ ] Prerequisites enforced properly
- [ ] Multi-class characters have separate perk pools
- [ ] Respec functionality works correctly

## Future Enhancements

1. **Perk Respec** - Allow players to reset perks (cost gold/special item)
2. **Perk Trees UI** - Visual representation of perk trees
3. **Perk Sets** - Bonuses for taking certain perk combinations
4. **Racial Perks** - Race-specific perks
5. **Epic Perks** - Special perks for level 30+ characters
6. **Perk Search** - Search perks by effect or keyword
7. **Perk Recommendations** - Suggest perks based on build

## References

- D&D Online Enhancement System
- Pathfinder Feats and Traits
- D&D 5e Feats
- World of Warcraft Talent Trees (Classic)
- Path of Exile Passive Tree

---
**Document Version:** 1.0  
**Last Updated:** October 22, 2025  
**Author:** Zusuk & AI Assistant
