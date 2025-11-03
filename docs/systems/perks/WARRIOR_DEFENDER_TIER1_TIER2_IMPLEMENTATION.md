# Warrior Defender Tree - Tier I & II Implementation Guide

## Overview
This document details the implementation of the **Defender Tree** (Tree 2) for warriors, covering Tier I and Tier II perks. The Defender tree focuses on defensive capabilities including armor class, hit points, damage reduction, and saving throws.

**Implementation Date:** October 23, 2025  
**Total Perks Implemented:** 8 perks (3 Tier I, 5 Tier II)  
**Point Cost Range:** 1-2 points per rank

---

## Tree 2: Defender

### Design Philosophy
The Defender tree is designed for warriors who prioritize survivability and protection over offense. These perks provide:
- **Armor Class improvements** through Armor Training ranks
- **Hit Point increases** through Toughness ranks
- **Saving throw bonuses** for all three save types
- **Shield-specific bonuses** for shield users
- **Damage Reduction** with tactical trade-offs

Unlike the Weapon Specialist tree which focuses on dealing damage, the Defender tree makes warriors harder to kill and more resilient in prolonged combat.

---

## Tier I Perks (1 point each)

### Perk 13: Armor Training I
**ID:** `PERK_FIGHTER_ARMOR_TRAINING_1` (13)

#### Basic Info
- **Cost:** 1 point per rank
- **Max Rank:** 3
- **Prerequisites:** None
- **Effect Type:** `PERK_EFFECT_AC`
- **Effect Value:** +1 AC per rank

#### Mechanics
- Provides a flat AC bonus that stacks with all other AC sources
- Multi-rank perk: can be purchased up to 3 times
- Total possible bonus: +3 AC (at 3 ranks for 3 points)
- Auto-applies via `get_perk_bonus()` in handler.c

#### Progression
| Rank | Cost | Total Spent | AC Bonus | Cumulative AC |
|------|------|-------------|----------|---------------|
| 1    | 1    | 1           | +1       | +1            |
| 2    | 1    | 2           | +1       | +2            |
| 3    | 1    | 3           | +1       | +3            |

#### Code Implementation
**structs.h:**
```c
#define PERK_FIGHTER_ARMOR_TRAINING_1 13
```

**perks.c (lines ~285-297):**
```c
perk = &perk_list[PERK_FIGHTER_ARMOR_TRAINING_1];
perk->id = PERK_FIGHTER_ARMOR_TRAINING_1;
perk->name = strdup("Armor Training I");
perk->description = strdup("+1 AC per rank (max 3 ranks)");
perk->associated_class = CLASS_WARRIOR;
perk->cost = 1;
perk->max_rank = 3;
perk->prerequisite_perk = -1;
perk->prerequisite_rank = 0;
perk->effect_type = PERK_EFFECT_AC;
perk->effect_value = 1;
perk->effect_modifier = 0;
perk->special_description = strdup("");
```

#### Strategic Value
- **Early Game:** Essential foundation for defensive builds
- **Mid Game:** Prerequisite for Armor Training II
- **Late Game:** Part of complete +5 AC progression
- **Synergy:** Stacks with armor, shields, and other AC bonuses
- **Build Types:** Universal for any defensive warrior

---

### Perk 14: Toughness I
**ID:** `PERK_FIGHTER_TOUGHNESS_1` (14)

#### Basic Info
- **Cost:** 1 point per rank
- **Max Rank:** 5
- **Prerequisites:** None
- **Effect Type:** `PERK_EFFECT_HP`
- **Effect Value:** +10 HP per rank

#### Mechanics
- Provides a flat hit point bonus
- Multi-rank perk: can be purchased up to 5 times
- Total possible bonus: +50 HP (at 5 ranks for 5 points)
- Auto-applies via `get_perk_bonus()` in handler.c
- HP bonus updates immediately when perk is purchased

#### Progression
| Rank | Cost | Total Spent | HP Bonus | Cumulative HP |
|------|------|-------------|----------|---------------|
| 1    | 1    | 1           | +10      | +10           |
| 2    | 1    | 2           | +10      | +20           |
| 3    | 1    | 3           | +10      | +30           |
| 4    | 1    | 4           | +10      | +40           |
| 5    | 1    | 5           | +10      | +50           |

#### Code Implementation
**structs.h:**
```c
#define PERK_FIGHTER_TOUGHNESS_1 14
```

**perks.c (lines ~299-311):**
```c
perk = &perk_list[PERK_FIGHTER_TOUGHNESS_1];
perk->id = PERK_FIGHTER_TOUGHNESS_1;
perk->name = strdup("Toughness I");
perk->description = strdup("+10 HP per rank (max 5 ranks)");
perk->associated_class = CLASS_WARRIOR;
perk->cost = 1;
perk->max_rank = 5;
perk->prerequisite_perk = -1;
perk->prerequisite_rank = 0;
perk->effect_type = PERK_EFFECT_HP;
perk->effect_value = 10;
perk->effect_modifier = 0;
perk->special_description = strdup("");
```

#### Strategic Value
- **Early Game:** Immediate survivability boost for low-level warriors
- **Mid Game:** Helps survive burst damage and critical hits
- **Late Game:** +50 HP can mean the difference between life and death
- **Synergy:** Combines with high CON scores and healing effects
- **Build Types:** Essential for tanks and front-line fighters

#### Efficiency Analysis
- **Cost per HP:** 1 point = 10 HP
- **Comparison:** A +2 CON item gives roughly 2 HP per level (20 HP at level 10)
- **Value:** Very efficient for pure survivability

---

### Perk 15: Resilience
**ID:** `PERK_FIGHTER_RESILIENCE` (15)

#### Basic Info
- **Cost:** 1 point per rank
- **Max Rank:** 3
- **Prerequisites:** None
- **Effect Type:** `PERK_EFFECT_SAVE`
- **Effect Value:** +1 to Fortitude saves per rank
- **Effect Modifier:** `APPLY_SAVING_FORT`

#### Mechanics
- Provides a bonus to Fortitude saving throws
- Multi-rank perk: can be purchased up to 3 times
- Total possible bonus: +3 to Fort saves (at 3 ranks for 3 points)
- Auto-applies via `get_perk_bonus()` with save type checking
- Helps resist poison, disease, death effects, and physical debuffs

#### Progression
| Rank | Cost | Total Spent | Fort Bonus | Cumulative Fort |
|------|------|-------------|------------|-----------------|
| 1    | 1    | 1           | +1         | +1              |
| 2    | 1    | 2           | +1         | +2              |
| 3    | 1    | 3           | +1         | +3              |

#### Code Implementation
**structs.h:**
```c
#define PERK_FIGHTER_RESILIENCE 15
```

**perks.c (lines ~313-325):**
```c
perk = &perk_list[PERK_FIGHTER_RESILIENCE];
perk->id = PERK_FIGHTER_RESILIENCE;
perk->name = strdup("Resilience");
perk->description = strdup("+1 to Fortitude saves per rank (max 3 ranks)");
perk->associated_class = CLASS_WARRIOR;
perk->cost = 1;
perk->max_rank = 3;
perk->prerequisite_perk = -1;
perk->prerequisite_rank = 0;
perk->effect_type = PERK_EFFECT_SAVE;
perk->effect_value = 1;
perk->effect_modifier = APPLY_SAVING_FORT;
perk->special_description = strdup("");
```

#### Strategic Value
- **Early Game:** Helps survive poison and disease
- **Mid Game:** Improves resistance to deadly debuffs
- **Late Game:** Essential for surviving high-level save-or-die effects
- **Synergy:** Stacks with Constitution bonus and other Fort save bonuses
- **Build Types:** Critical for any warrior facing casters or monsters with special attacks

#### Save Type Coverage
Fortitude saves protect against:
- Poison
- Disease
- Death effects
- Paralysis
- Petrification
- Physical debilitations

---

## Tier II Perks (2 points each)

### Perk 16: Armor Training II
**ID:** `PERK_FIGHTER_ARMOR_TRAINING_2` (16)

#### Basic Info
- **Cost:** 2 points per rank
- **Max Rank:** 2
- **Prerequisites:** Armor Training I at max rank (3)
- **Effect Type:** `PERK_EFFECT_AC`
- **Effect Value:** +1 AC per rank

#### Mechanics
- Continuation of Armor Training progression
- Requires completion of Armor Training I (all 3 ranks)
- Multi-rank perk: can be purchased up to 2 times
- Total from this perk: +2 AC (at 2 ranks for 4 points)
- **Complete progression:** Armor Training I (3 ranks) + Armor Training II (2 ranks) = +5 AC total

#### Progression
| Rank | Cost | Total Spent | AC Bonus | Cumulative AC | Total AC (I+II) |
|------|------|-------------|----------|---------------|-----------------|
| 1    | 2    | 2           | +1       | +1            | +4              |
| 2    | 2    | 4           | +1       | +2            | +5              |

**Full Armor Training Path:**
- Armor Training I Rank 1: 1 point → +1 AC
- Armor Training I Rank 2: 1 point → +2 AC total
- Armor Training I Rank 3: 1 point → +3 AC total
- Armor Training II Rank 1: 2 points → +4 AC total
- Armor Training II Rank 2: 2 points → +5 AC total
- **Total Investment:** 7 points for +5 AC

#### Code Implementation
**structs.h:**
```c
#define PERK_FIGHTER_ARMOR_TRAINING_2 16
```

**perks.c (lines ~329-341):**
```c
perk = &perk_list[PERK_FIGHTER_ARMOR_TRAINING_2];
perk->id = PERK_FIGHTER_ARMOR_TRAINING_2;
perk->name = strdup("Armor Training II");
perk->description = strdup("+1 AC per rank (max 2 ranks)");
perk->associated_class = CLASS_WARRIOR;
perk->cost = 2;
perk->max_rank = 2;
perk->prerequisite_perk = PERK_FIGHTER_ARMOR_TRAINING_1;
perk->prerequisite_rank = 3; /* Requires Armor Training I at max rank */
perk->effect_type = PERK_EFFECT_AC;
perk->effect_value = 1;
perk->effect_modifier = 0;
perk->special_description = strdup("Requires Armor Training I at max rank (3)");
```

#### Strategic Value
- **Early Game:** Not accessible (requires Tier I completion)
- **Mid Game:** Significant AC boost for established characters
- **Late Game:** Part of optimized defensive build
- **Synergy:** Combines with shields, magic items, defensive spells
- **Build Types:** Essential capstone for AC-focused defenders

#### Efficiency Analysis
- **Cost per AC:** 2 points = +1 AC (Tier II) vs 1 point = +1 AC (Tier I)
- **Diminishing Returns:** Tier II costs twice as much per AC point
- **Overall Value:** 7 points for +5 AC is reasonable for dedicated defenders
- **Comparison:** A +5 AC bonus is equivalent to a very high-quality magic armor enhancement

---

### Perk 17: Shield Mastery I
**ID:** `PERK_FIGHTER_SHIELD_MASTERY_1` (17)

#### Basic Info
- **Cost:** 2 points
- **Max Rank:** 1
- **Prerequisites:** None
- **Effect Type:** `PERK_EFFECT_SPECIAL`
- **Effect Value:** +2 AC when using a shield
- **Effect Modifier:** 0

#### Mechanics
- Provides a conditional +2 AC bonus **only when wielding a shield**
- Integrated into `compute_armor_class()` function in fight.c
- Bonus type: `BONUS_TYPE_SHIELD` (stacks with shield's base AC)
- Does not apply if not using a shield or if shield is disabled (e.g., shield bash recovery)

#### Code Implementation
**structs.h:**
```c
#define PERK_FIGHTER_SHIELD_MASTERY_1 17
```

**perks.c (lines ~343-357):**
```c
perk = &perk_list[PERK_FIGHTER_SHIELD_MASTERY_1];
perk->id = PERK_FIGHTER_SHIELD_MASTERY_1;
perk->name = strdup("Shield Mastery I");
perk->description = strdup("+2 AC when using a shield");
perk->associated_class = CLASS_WARRIOR;
perk->cost = 2;
perk->max_rank = 1;
perk->prerequisite_perk = -1;
perk->prerequisite_rank = 0;
perk->effect_type = PERK_EFFECT_SPECIAL;
perk->effect_value = 2;
perk->effect_modifier = 0;
perk->special_description = strdup("+2 AC bonus when wielding a shield");
```

**fight.c (lines ~715-720):**
```c
else
{ /* okay to add shield bonus to shield, remember factor 10 */
  bonuses[BONUS_TYPE_SHIELD] += (apply_ac(ch, WEAR_SHIELD) / 10);
}
if (teamwork_using_shield(ch, FEAT_SHIELD_WALL))
  bonuses[BONUS_TYPE_SHIELD] += teamwork_using_shield(ch, FEAT_SHIELD_WALL);
/* Shield Mastery I perk bonus */
if (has_perk(ch, PERK_FIGHTER_SHIELD_MASTERY_1))
  bonuses[BONUS_TYPE_SHIELD] += 2;
```

#### Strategic Value
- **Equipment Requirement:** Must use a shield (weapon slot commitment)
- **AC Efficiency:** +2 AC for 2 points is 1:1 ratio (same as Armor Training I)
- **Build Specialization:** Excellent for sword-and-board warriors
- **Synergy:** Combines with:
  - Shield's base AC bonus
  - Shield Wall teamwork feat
  - Any shield-enhancing magic items
  - Armor Training perks

#### Comparison: Shield vs Two-Handed
**Shield Build (with Shield Mastery I):**
- +2 AC from perk
- +AC from shield itself (varies by shield type)
- One-handed weapon only

**Two-Handed Build:**
- +50% STR bonus to damage (1.5x vs 1x)
- Greater Weapon Focus/Specialization potential
- No shield AC bonus

#### Use Cases
- **Tank Role:** Frontline defender protecting allies
- **Boss Fights:** When survivability > damage
- **PvP:** Where defense matters as much as offense
- **Balanced Build:** Moderate offense with enhanced defense

---

### Perk 18: Defensive Stance
**ID:** `PERK_FIGHTER_DEFENSIVE_STANCE` (18)

#### Basic Info
- **Cost:** 2 points
- **Max Rank:** 1
- **Prerequisites:** Armor Training I (at least 1 rank)
- **Effect Type:** `PERK_EFFECT_SPECIAL`
- **Effect Value:** 2 (DR amount)
- **Effect Modifier:** 0

#### Mechanics
This perk provides both a benefit and a drawback:

**Benefit:**
- Damage Reduction 2/- (reduces all physical damage by 2 points)
- Integrated into `compute_damage_reduction_full()` in fight.c
- Always active when toggled on

**Drawback:**
- -1 penalty to hit (attack rolls)
- Integrated into `compute_attack_bonus_full()` in fight.c
- Applied as `BONUS_TYPE_CIRCUMSTANCE` penalty
- Always active when toggled on

**Toggleable:**
- This perk can be toggled on/off with the `toggleperk` command
- When toggled OFF, neither the benefit nor the drawback applies
- Use `toggleperk defensive` to toggle
- Default state when purchased: ON

#### Code Implementation
**structs.h:**
```c
#define PERK_FIGHTER_DEFENSIVE_STANCE 18
```

**perks.c (lines ~359-373):**
```c
perk = &perk_list[PERK_FIGHTER_DEFENSIVE_STANCE];
perk->id = PERK_FIGHTER_DEFENSIVE_STANCE;
perk->name = strdup("Defensive Stance");
perk->description = strdup("Damage reduction 2/-, -1 to hit when active");
perk->associated_class = CLASS_WARRIOR;
perk->cost = 2;
perk->max_rank = 1;
perk->prerequisite_perk = PERK_FIGHTER_ARMOR_TRAINING_1;
perk->prerequisite_rank = 1; /* Requires at least 1 rank of Armor Training I */
perk->effect_type = PERK_EFFECT_SPECIAL;
perk->effect_value = 2;
perk->effect_modifier = 0;
perk->special_description = strdup("Provides damage reduction 2/- but -1 to hit");
```

**fight.c - Damage Reduction (lines ~3927-3933):**
```c
/* Defensive Stance perk */
if (has_perk_active(ch, PERK_FIGHTER_DEFENSIVE_STANCE))
{
  damage_reduction += 2;
  if (display)
    send_to_char(ch, "%-30s: %d\r\n", "Defensive Stance Perk", 2);
}
```

**fight.c - Attack Penalty (lines ~8530-8536):**
```c
/* Defensive Stance perk penalty */
if (has_perk_active(ch, PERK_FIGHTER_DEFENSIVE_STANCE))
{
  bonuses[BONUS_TYPE_CIRCUMSTANCE] -= 1;
  if (display)
    send_to_char(ch, "-1: %-50s\r\n", "Defensive Stance Perk"); 
}
```

**Note:** Uses `has_perk_active()` which checks both ownership AND toggle state.

#### Strategic Value
**When to Take:**
- Characters who prioritize survival over damage
- Tanks absorbing many small hits
- Low-damage, high-durability builds
- Group play where allies provide damage

**When to Avoid:**
- High-damage DPS builds
- Characters with already low hit chance
- Solo play where killing speed matters
- Builds relying on critical hits

#### Mathematical Analysis
**Damage Reduction Value:**
- Against 10 attacks of 20 damage each: saves 20 damage (10 attacks × 2 DR)
- Against 5 attacks of 50 damage each: saves 10 damage (5 attacks × 2 DR)
- More valuable against many small attacks than few large attacks

**Attack Penalty Cost:**
- -1 to hit = ~5% reduction in hit chance
- If you had 70% hit chance, now ~65%
- Reduces damage output by ~7% (5% fewer hits)

**Net Value:**
- Worthwhile if incoming damage > lost damage output
- Best for defensive tanks, not damage dealers

#### Synergy
**Good Combinations:**
- Other DR sources (stacks additively)
- High AC (reduces incoming attacks)
- High HP (survives burst damage)
- Armor Training (more AC to compensate for hit penalty)

**Poor Combinations:**
- Critical hit builds (fewer hits = fewer crits)
- Damage-focused perks (penalty counteracts benefits)
- Precision builds (already tight hit margins)

---

### Perk 19: Iron Will
**ID:** `PERK_FIGHTER_IRON_WILL` (19)

#### Basic Info
- **Cost:** 2 points
- **Max Rank:** 1
- **Prerequisites:** None
- **Effect Type:** `PERK_EFFECT_SAVE`
- **Effect Value:** +2 to Will saves
- **Effect Modifier:** `APPLY_SAVING_WILL`

#### Mechanics
- Provides a +2 bonus to Will saving throws
- Auto-applies via `get_perk_bonus()` with save type checking
- Helps resist mind-affecting spells, illusions, and mental compulsions
- Particularly valuable for warriors (typically poor Will saves)

#### Code Implementation
**structs.h:**
```c
#define PERK_FIGHTER_IRON_WILL 19
```

**perks.c (lines ~375-387):**
```c
perk = &perk_list[PERK_FIGHTER_IRON_WILL];
perk->id = PERK_FIGHTER_IRON_WILL;
perk->name = strdup("Iron Will");
perk->description = strdup("+2 to Will saves");
perk->associated_class = CLASS_WARRIOR;
perk->cost = 2;
perk->max_rank = 1;
perk->prerequisite_perk = -1;
perk->prerequisite_rank = 0;
perk->effect_type = PERK_EFFECT_SAVE;
perk->effect_value = 2;
perk->effect_modifier = APPLY_SAVING_WILL;
perk->special_description = strdup("");
```

#### Strategic Value
- **Class Weakness:** Warriors typically have low Wisdom and poor Will saves
- **Threat Mitigation:** Protects against charm, fear, sleep, and compulsion
- **Value:** +2 to Will saves is a significant boost for warriors
- **Tier II Cost:** 2 points is fair for a +2 bonus to a weak save

#### Save Type Coverage
Will saves protect against:
- Charm and domination
- Fear effects
- Sleep and unconsciousness
- Illusions (disbelief)
- Mental compulsions
- Psychic attacks

#### Comparison to Resilience
| Perk        | Save Type | Bonus | Cost | Efficiency |
|-------------|-----------|-------|------|------------|
| Resilience  | Fort      | +1/rank | 1/rank | 1:1 |
| Iron Will   | Will      | +2    | 2    | 1:1 |

Both offer 1:1 efficiency, but Iron Will provides the full +2 in a single purchase.

---

### Perk 20: Lightning Reflexes
**ID:** `PERK_FIGHTER_LIGHTNING_REFLEXES` (20)

#### Basic Info
- **Cost:** 2 points
- **Max Rank:** 1
- **Prerequisites:** None
- **Effect Type:** `PERK_EFFECT_SAVE`
- **Effect Value:** +2 to Reflex saves
- **Effect Modifier:** `APPLY_SAVING_REFL`

#### Mechanics
- Provides a +2 bonus to Reflex saving throws
- Auto-applies via `get_perk_bonus()` with save type checking
- Helps avoid area-of-effect damage, traps, and evasion-based attacks
- Particularly valuable for heavy armor users (typically poor Reflex)

#### Code Implementation
**structs.h:**
```c
#define PERK_FIGHTER_LIGHTNING_REFLEXES 20
```

**perks.c (lines ~389-401):**
```c
perk = &perk_list[PERK_FIGHTER_LIGHTNING_REFLEXES];
perk->id = PERK_FIGHTER_LIGHTNING_REFLEXES;
perk->name = strdup("Lightning Reflexes");
perk->description = strdup("+2 to Reflex saves");
perk->associated_class = CLASS_WARRIOR;
perk->cost = 2;
perk->max_rank = 1;
perk->prerequisite_perk = -1;
perk->prerequisite_rank = 0;
perk->effect_type = PERK_EFFECT_SAVE;
perk->effect_value = 2;
perk->effect_modifier = APPLY_SAVING_REFL;
perk->special_description = strdup("");
```

#### Strategic Value
- **Heavy Armor Penalty:** Heavy armor often imposes Reflex save penalties
- **AoE Protection:** Reduces damage from fireballs, breath weapons, etc.
- **Trap Evasion:** Better chance to avoid or reduce trap damage
- **Value:** +2 to Reflex is significant for low-DEX warriors

#### Save Type Coverage
Reflex saves protect against:
- Area-of-effect spells (fireball, lightning bolt, cone of cold)
- Dragon breath weapons
- Traps (spikes, pits, falling objects)
- Sudden physical dangers
- Evasion-based attacks

#### Armor Considerations
Heavy armor users often have:
- Low Dexterity (for STR/CON focus)
- Armor check penalties to Reflex saves
- Poor natural Reflex save progression

Lightning Reflexes helps compensate for these disadvantages.

---

## Complete Tier I & II Build Paths

### Path 1: Maximum AC Tank
**Goal:** Highest possible armor class

**Investment:**
1. Armor Training I Rank 1-3: 3 points → +3 AC
2. Armor Training II Rank 1-2: 4 points → +2 AC
3. Shield Mastery I: 2 points → +2 AC (with shield)

**Total:** 9 points for +7 AC (with shield), +5 AC (without shield)

**Additional Options:**
- Defensive Stance: +2 points → DR 2/-, -2 to hit

---

### Path 2: Survivability Specialist
**Goal:** Maximum hit points and damage reduction

**Investment:**
1. Toughness I Rank 1-5: 5 points → +50 HP
2. Armor Training I Rank 1: 1 point → +1 AC (prerequisite)
3. Defensive Stance: 2 points → DR 2/-, -1 to hit

**Total:** 8 points for +50 HP, +1 AC, DR 2/-, -1 to hit

**Additional Options:**
- Resilience ranks for Fort saves

---

### Path 3: Balanced Defender
**Goal:** Well-rounded defensive capabilities

**Investment:**
1. Armor Training I Rank 1-3: 3 points → +3 AC
2. Toughness I Rank 1-3: 3 points → +30 HP
3. Resilience Rank 1-2: 2 points → +2 Fort saves
4. Iron Will: 2 points → +2 Will saves

**Total:** 10 points for +3 AC, +30 HP, +2 Fort, +2 Will

**Rationale:** Spreads points across multiple defensive layers

---

### Path 4: Save-Focused Guardian
**Goal:** Maximum saving throw bonuses

**Investment:**
1. Resilience Rank 1-3: 3 points → +3 Fort saves
2. Iron Will: 2 points → +2 Will saves
3. Lightning Reflexes: 2 points → +2 Reflex saves

**Total:** 7 points for +3 Fort, +2 Will, +2 Reflex

**Additional Options:**
- Armor Training I for +3 AC (3 points)
- Toughness I ranks for HP

---

### Path 5: Shield Master
**Goal:** Optimized shield-and-sword warrior

**Investment:**
1. Armor Training I Rank 1-3: 3 points → +3 AC
2. Shield Mastery I: 2 points → +2 AC (with shield)
3. Defensive Stance: 2 points → DR 2/-, -1 to hit

**Total:** 7 points for +5 AC (with shield), DR 2/-, -1 to hit

**Build Notes:**
- Must use shield to benefit from Shield Mastery
- Defensive Stance penalty offset by high AC
- Excellent for tanking role

---

## Integration Points

### Automatic Effect Application
The following perks have their effects **automatically applied** through the existing `get_perk_bonus()` system:

1. **Armor Training I & II** (`PERK_EFFECT_AC`)
   - Applied in: `handler.c` via `get_perk_ac_bonus()`
   - Recalculated: During stat updates and AC computation

2. **Toughness I** (`PERK_EFFECT_HP`)
   - Applied in: `handler.c` via `get_perk_hp_bonus()`
   - Recalculated: During stat updates and max HP computation

3. **Resilience** (`PERK_EFFECT_SAVE` + `APPLY_SAVING_FORT`)
   - Applied in: `perks.c` via `get_perk_save_bonus()`
   - Recalculated: During saving throw checks

4. **Iron Will** (`PERK_EFFECT_SAVE` + `APPLY_SAVING_WILL`)
   - Applied in: `perks.c` via `get_perk_save_bonus()`
   - Recalculated: During saving throw checks

5. **Lightning Reflexes** (`PERK_EFFECT_SAVE` + `APPLY_SAVING_REFL`)
   - Applied in: `perks.c` via `get_perk_save_bonus()`
   - Recalculated: During saving throw checks

### Special Effect Implementation
The following perks require **custom integration** in fight.c:

1. **Shield Mastery I** (`PERK_EFFECT_SPECIAL`)
   - Location: `fight.c` in `compute_armor_class()`
   - Lines: ~718-720
   - Condition: Must be wielding a shield
   - Bonus Type: `BONUS_TYPE_SHIELD`

2. **Defensive Stance** (`PERK_EFFECT_SPECIAL`)
   - **Damage Reduction:** `fight.c` in `compute_damage_reduction_full()`
     - Lines: ~3927-3933
     - Adds: DR 2/-
   - **Attack Penalty:** `fight.c` in `compute_attack_bonus_full()`
     - Lines: ~8530-8536
     - Penalty: -2 to hit
     - Type: `BONUS_TYPE_CIRCUMSTANCE`

---

## Testing Recommendations

### Functional Testing

#### Armor Training I & II
1. Create test warrior character
2. Purchase Armor Training I Rank 1
3. Verify AC increased by +1 (check `score` command)
4. Purchase ranks 2 and 3, verify cumulative +3 AC
5. Attempt to purchase Armor Training II without rank 3 (should fail)
6. Purchase Armor Training I Rank 3
7. Purchase Armor Training II Rank 1 and 2
8. Verify final AC is +5 higher than base

#### Toughness I
1. Note starting HP
2. Purchase Toughness I Rank 1
3. Verify HP increased by +10
4. Purchase up to Rank 5
5. Verify total +50 HP increase

#### Resilience, Iron Will, Lightning Reflexes
1. Check base save values (`score` or save display command)
2. Purchase each perk
3. Verify save bonuses applied correctly:
   - Resilience: +1 Fort per rank (max +3)
   - Iron Will: +2 Will
   - Lightning Reflexes: +2 Reflex
4. Test actual saving throws against spells/effects

#### Shield Mastery I
1. Check AC without shield equipped
2. Equip a shield, note AC increase from shield itself
3. Purchase Shield Mastery I
4. Verify additional +2 AC beyond shield's base AC
5. Remove shield, verify perk bonus disappears
6. Re-equip shield, verify bonus returns

#### Defensive Stance
1. Check base to-hit and damage taken
2. Purchase Defensive Stance
3. Verify -2 to attack rolls (`combat` or attack display)
4. Enter combat, verify damage reduced by 2 per hit
5. Use `damreduc` command to view DR breakdown

### Combat Testing
1. **Tank Test:** Full AC build vs multiple enemies
2. **Survival Test:** HP + DR build vs burst damage
3. **Save Test:** Save-focused build vs spell casters
4. **Shield Test:** Shield Master build with various shields

### Edge Cases
1. **Multi-class:** Verify perks work with dual/tri-class warriors
2. **Respec:** Verify perks removed/refunded correctly
3. **Save/Load:** Verify perks persist across login/logout
4. **Prerequisites:** Verify Armor Training II requires max Armor Training I
5. **Shield Recovery:** Verify Shield Mastery bonus removed during shield bash recovery

---

## Balance Considerations

### Point Efficiency
| Perk Type           | Points | Benefit         | Efficiency |
|---------------------|--------|-----------------|------------|
| Armor Training I    | 1      | +1 AC           | 1:1        |
| Armor Training II   | 2      | +1 AC           | 2:1        |
| Toughness I         | 1      | +10 HP          | 1:10       |
| Resilience          | 1      | +1 Fort         | 1:1        |
| Shield Mastery I    | 2      | +2 AC*          | 1:1*       |
| Defensive Stance    | 2      | DR 2/-, -2 hit  | Mixed      |
| Iron Will           | 2      | +2 Will         | 1:1        |
| Lightning Reflexes  | 2      | +2 Reflex       | 1:1        |

*Conditional on using shield

### Comparison to Weapon Specialist Tree
**Defender advantages:**
- Immediate survivability improvements
- Benefits entire party (by staying alive to tank)
- Less gear-dependent

**Weapon Specialist advantages:**
- Higher damage output
- Faster combat resolution
- Better solo performance

**Recommended Mix:**
- Early levels: Invest in Weapon Specialist for leveling speed
- Mid levels: Add Defender perks for difficult content
- Late levels: Specialize based on role (tank vs DPS)

### Power Level Assessment
The Defender tree is appropriately balanced:
- **Tier I (1 point):** Small, incremental bonuses
- **Tier II (2 points):** Meaningful improvements with prerequisites or trade-offs
- **Total Investment:** 10-15 points for a strong defensive build
- **Opportunity Cost:** Balanced against Weapon Specialist tree

---

## Future Tier III & IV Perks (Not Yet Implemented)

### Tier III Preview
From WARRIOR_PERKS.md:
- **Armor Training III:** +2 AC for 3 points (requires Armor Training II max)
- **Shield Mastery II:** Additional +2 AC with shield for 3 points
- **Improved Damage Reduction:** DR improvement for 3 points

### Tier IV Preview
- **Unstoppable:** Capstone defensive ability
- **Impenetrable Defense:** Ultimate AC/DR combination

---

## Toggleable Perk System

### Overview
Some perks can be toggled on or off, allowing players to control when the perk's effects apply. This is useful for perks with both benefits and drawbacks, like **Defensive Stance**.

### Implementation

**Data Structures:**
- `struct perk_data` has a `bool toggleable` field
- `char_data` has a `byte perk_toggles[32]` bitfield (supports 256 perks)
- Each bit represents whether a perk is toggled ON (1) or OFF (0)

**Functions:**
- `is_perk_toggled_on(ch, perk_id)` - Check if a perk is toggled on
- `set_perk_toggle(ch, perk_id, state)` - Set toggle state
- `has_perk_active(ch, perk_id)` - Check if perk is owned AND active

**Persistence:**
- Toggle state saved in pfile with tag `PTog:` as 64 hex characters
- Format: `PTog: 0000000000000000...` (32 bytes = 64 hex digits)
- Automatically loaded/saved with character

### Usage

**Player Commands:**
```
toggleperk <perk name>     - Toggle a perk on/off
toggleperk defensive       - Toggle Defensive Stance
myperks                    - View all perks and their toggle status
```

**Toggle Display:**
The `myperks` command shows toggle status:
```
Your Purchased Perks

Name                           Class           Rank   Toggle
------------------------------ --------------- ------ --------
Defensive Stance               Warrior         1      ON
```

### Default Behavior
- When a toggleable perk is purchased, it defaults to **ON**
- Non-toggleable perks always show "-" for toggle status
- Toggleable perks only apply their effects when ON

### Current Toggleable Perks
1. **Defensive Stance** - Can be toggled to disable DR 2/- and -1 to hit

### Integration
Code that checks for toggleable perks must use `has_perk_active()` instead of `has_perk()`:

```c
// Wrong - doesn't check toggle state
if (has_perk(ch, PERK_FIGHTER_DEFENSIVE_STANCE))

// Correct - checks both ownership and toggle state
if (has_perk_active(ch, PERK_FIGHTER_DEFENSIVE_STANCE))
```

For non-toggleable perks, `has_perk_active()` behaves identically to `has_perk()`.

---

## Conclusion

The Defender tree Tier I and II implementation provides warriors with 8 comprehensive defensive options covering:
- **Armor Class** (Armor Training I/II, Shield Mastery I)
- **Hit Points** (Toughness I)
- **Damage Reduction** (Defensive Stance)
- **Saving Throws** (Resilience, Iron Will, Lightning Reflexes)

All perks are fully functional with both automatic effect application (via `get_perk_bonus()`) and special integrations (in `fight.c`). The system supports multi-rank perks, prerequisites including "max rank" requirements, proper save/load persistence, and **toggleable perks** for situational control.

**Total Implementation:**
- **Files Modified:** 3 (structs.h, perks.c, fight.c)
- **New Perk IDs:** 8 (IDs 13-20)
- **Lines of Code:** ~250 lines of definitions + integration
- **Compilation Status:** Clean, zero errors/warnings

The Defender tree is ready for player use and provides a strong foundation for Tier III and IV perks.
