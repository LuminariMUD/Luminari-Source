# Luminari MUD Golem Crafting and Management System

## Overview

The golem system allows wizards and sorcerers to craft magical constructs that serve as followers. Golems are intelligent constructs immune to healing magic, fatigue, and mind-affecting spells. They can only be healed through a specialized repair command using crafted materials.

## Golem Types

### Wood Golems
- **Material Requirements:** 50-200 units of Ash Wood + 10-40 units of Metal
- **Difficulty:** Base DC 15 (easiest to construct)
- **Characteristics:** Fast, agile, lower durability
- **Sizes:** Small (16500), Medium (16501), Large (16502), Huge (16503)

### Stone Golems
- **Material Requirements:** 50-200 units of Stone + 10-40 units of Metal
- **Difficulty:** Base DC 25 (moderate construction difficulty)
- **Characteristics:** Durable, moderate speed, balanced stats
- **Sizes:** Small (16504), Medium (16505), Large (16506), Huge (16507)

### Iron Golems
- **Material Requirements:** 50-200 units of Iron + 10-40 units of Metal
- **Difficulty:** Base DC 35 (most difficult to construct)
- **Characteristics:** Extremely durable, strong, slower movement
- **Sizes:** Small (16508), Medium (16509), Large (16510), Huge (16511)

## VNUMs

| Type | Small | Medium | Large | Huge |
|------|-------|--------|-------|------|
| Wood | 16500 | 16501 | 16502 | 16503 |
| Stone | 16504 | 16505 | 16506 | 16507 |
| Iron | 16508 | 16509 | 16510 | 16511 |

## Feats Required

- **FEAT_CONSTRUCT_WOOD_GOLEM:** Required for level 15 Wizards/Sorcerers
- **FEAT_CONSTRUCT_STONE_GOLEM:** Required for level 25 Wizards/Sorcerers
- **FEAT_CONSTRUCT_IRON_GOLEM:** Required for level 35 Wizards/Sorcerers

## Creating a Golem

### Command Syntax
```
craft create golem
craft golem type (wood|stone|iron)
craft golem size (small|medium|large|huge)
craft golem show
craft golem start
```

### Steps
1. Use `craft create golem` or navigate to golem crafting
2. Set type with `craft golem type wood` (or stone/iron)
3. Set size with `craft golem size medium` (or small/large/huge)
4. Review requirements with `craft golem show`
5. Craft with `craft golem start`

### Crafting Time
Base 60 seconds + (20 seconds × size tier)
- Small: ~80 seconds
- Medium: ~100 seconds
- Large: ~120 seconds
- Huge: ~140 seconds

### Skill Check
- **Skill Used:** Arcana
- **DC:** Base DC varies by type + (5 × size tier)
  - Wood Small: DC 15, Medium: 20, Large: 25, Huge: 30
  - Stone Small: DC 25, Medium: 30, Large: 35, Huge: 40
  - Iron Small: DC 35, Medium: 40, Large: 45, Huge: 50

### Material Costs

#### Wood Golems (per size tier multiplier)
- Small: 50 Ash Wood + 10 Metal
- Medium: 100 Ash Wood + 20 Metal
- Large: 150 Ash Wood + 30 Metal
- Huge: 200 Ash Wood + 40 Metal

#### Stone Golems (per size tier multiplier)
- Small: 50 Stone + 10 Metal
- Medium: 100 Stone + 20 Metal
- Large: 150 Stone + 30 Metal
- Huge: 200 Stone + 40 Metal

#### Iron Golems (per size tier multiplier)
- Small: 50 Iron + 10 Metal
- Medium: 100 Iron + 20 Metal
- Large: 150 Iron + 30 Metal
- Huge: 200 Iron + 40 Metal

### Mote Requirements
All golem types require motes from each of the 9 crafting mote types:
- Small: 5 motes of each type
- Medium: 6 motes of each type (1.2× multiplier)
- Large: 7 motes of each type (1.4× multiplier)
- Huge: 8 motes of each type (1.6× multiplier)

## Managing Golems

### Follower Limit
- Players can have **1 active golem** at a time
- Golems count as a special follower type separate from mercs/pets/summons
- Only one golem per player can be active

### Destroying a Golem
```
destroygolem <golem name>
```
- Dismantles the golem and recovers **50% of original crafting materials**
- Materials are returned to player's storage

### Golem Death
- When a golem is killed in combat, **25% of crafting materials** are dropped in the room
- Full material recovery is possible if all items are collected

## Repairing Golems

### Command Syntax
```
golemrepair <golem name>
```

### Requirements
- Must be **out of combat** (neither player nor golem fighting)
- Golem must be in the **same room** as the player
- Golem must have **missing health points**
- Player must have **sufficient crafting materials**

### Repair Process
1. Player makes an **Arcana skill check** against DC (same as construction DC)
2. On success:
   - All missing HP is restored
   - Materials cost = (missing_hp_percent / 10) × material_cost_per_10%
   - Materials are consumed from player's storage
3. On failure:
   - Materials are **still consumed** (no refund)
   - Golem is **not healed**

### Material Costs for Repair
Cost scales with missing HP percentage. For example:
- 50% missing HP on Medium Wood Golem: 5 Ash Wood (half of 10 per 10%)
- 100% missing HP on Medium Wood Golem: 10 Ash Wood (full cost)
- 25% missing HP on Large Stone Golem: 7.5 Stone (rounded)

## Golem Immunities

### Healing Spells
- **Immune to all magical healing** (cure light wounds, heal, etc.)
- Only the repair command can heal golems
- Cannot benefit from healing auras or spells

### Natural Regeneration
- **No natural HP regeneration** from resting
- **No PSP regeneration** from food/drink effects
- **No movement point regeneration** from food/drink effects
- **Cannot become fatigued**

### Mind-Affecting Effects
- **Immune to charm spells** (charm person, dominate person, etc.)
- **Immune to fear effects** (fear, cause fear, etc.)
- **Immune to enchantment magic** that affects the mind
- Messages notify casters of the construct's immunity

### Construct Nature
- Golems are treated as **RACE_TYPE_CONSTRUCT**
- They gain all benefits of construct type immunities
- Some abilities may have reduced effectiveness against constructs

## Technical Implementation

### Code Files Modified
- `src/structs.h` - MOB_GOLEM flag (103), crafting data structure
- `src/crafting_new.c` - Golem crafting logic, helper functions
- `src/crafting_new.h` - Function declarations
- `src/act.other.c` - destroygolem and golemrepair commands
- `src/act.item.c` - Conditional auto-pickup for materials
- `src/act.h` - Command declarations
- `src/interpreter.c` - Command registration
- `src/utils.c` - can_add_follower, immunity checks, regen functions
- `src/utils.h` - IS_GOLEM macro, immunity checks
- `src/magic.c` - Healing spell immunity
- `src/limits.c` - Regeneration immunity
- `src/fight.c` - Material drops on death
- `src/class.c` - Golem feats for Wizard/Sorcerer

### Macros

```c
#define IS_GOLEM(ch) (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_GOLEM))
#define IS_CONSTRUCT(ch) (GET_RACE(ch) == RACE_TYPE_CONSTRUCT)
#define GET_CRAFT_MAT(ch, mat) ((ch)->player_specials->saved.craft_materials[mat])
```

### Key Functions

#### Golem Creation
- `craft_golem_complete()` - Instantiates golem mob
- `begin_golem_craft()` - Validates and starts crafting process
- `get_golem_vnum()` - Returns VNUM for type/size
- `has_golem_follower()` - Checks if player already has golem

#### Golem Management
- `do_destroygolem()` - Destroys golem, recovers materials
- `do_golemrepair()` - Repairs golem with Arcana check
- `can_repair_golem()` - Validates repair eligibility
- `recover_golem_materials()` - Returns materials to player

#### Immunities
- `is_immune_mind_affecting()` - Checks golem mind immunity
- `mag_points()` - Blocks healing on golems
- `regen_hps()` - Returns 0 HP regen for golems
- `get_fast_healing_amount()` - Returns 0 for golems
- `get_hp/psp/mv_regen_amount()` - Returns 0 for golems
- `regen_update()` - Strips fatigue, sets move to max

#### Material Calculations
- `get_golem_base_dc()` - Construction/repair DC
- `get_golem_material_requirements()` - Material cost
- `get_golem_mote_requirements()` - Mote cost
- `get_golem_repair_material_cost()` - Repair material cost
- `get_golem_repair_dc()` - Repair DC (same as construction)

## Balancing Notes

### Follower Slot
- Golems occupy one dedicated follower slot
- Does not conflict with other pets/mercs/summons
- Only one golem active per player

### Material Scaling
- Material costs increase significantly with size
- Huge golems cost 4× more materials than small ones
- Encourages variety in golem usage

### Difficulty Curve
- Wood golems accessible to mid-level casters (15+)
- Stone golems require significant investment (25+)
- Iron golems are end-game craftables (35+)

### Repair Economics
- Repair costs are steep but fair
- 10% of crafting cost per 10% healing maintains progression
- Materials are consumed even on failure (risk/reward)

## Future Enhancement Ideas

1. **Golem Leveling:** Track golem experience, improve stats
2. **Golem Customization:** Enchantments, special abilities per type
3. **Golem Upgrades:** Ability to enhance existing golems
4. **Golem Recovery:** Ability to reclaim materials from destroyed golems
5. **Passive Abilities:** Each golem type gains unique passive effects
6. **Combat AI:** Golem-specific combat tactics and strategies
7. **Golem Equipment:** Allow golems to wear certain magical items
8. **Golem Familiars:** Tiny golem variants as alternatives to regular familiars

## Testing Checklist

- [ ] Golem crafting with correct materials
- [ ] Skill check accuracy (Arcana-based)
- [ ] Material consumption on success/failure
- [ ] Golem mob spawning with correct VNUM
- [ ] Follower limit enforcement (1 golem max)
- [ ] Destroy command material recovery (50%)
- [ ] Death material drops (25%)
- [ ] Repair command material validation
- [ ] Repair skill check mechanics
- [ ] Healing spell immunity messages
- [ ] Natural regeneration immunity
- [ ] Mind-affecting spell immunity
- [ ] Fatigue immunity
- [ ] Movement cost immunity
- [ ] Race type setting to RACE_TYPE_CONSTRUCT
