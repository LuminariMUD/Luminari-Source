# Cleric Domain Master Tree Tier 1-2 Implementation

## Overview
This document describes the implementation of the first two tiers of the Cleric Domain Master perk tree. All 11 perks have been defined in the codebase and are ready for integration with the spell system.

**Status:** ✅ FULLY DEFINED IN CODE (v1.0)
**Integration Needed:** Spell system integration for DC bonuses, damage bonuses, and spell point increases

---

## Implemented Perks

### Tier 1 Perks (1 point each)

#### 1. Domain Focus I (ID: 98)
- **Cost:** 1 point per rank
- **Max Rank:** 3
- **Effect:** +1 to domain spell DC per rank
- **Implementation:** 
  - Defined in `src/perks.c` (line ~1395)
  - Helper function: `get_cleric_domain_focus_bonus()` (line ~4928)
  - Returns total DC bonus from Tier 1 & 2 perks
- **Integration Needed:** Spell DC calculation for domain spells
- **Testing:** Purchase ranks, cast domain spells, verify DC increase

#### 2. Divine Spell Power I (ID: 99)
- **Cost:** 1 point per rank
- **Max Rank:** 5
- **Effect:** +1 to divine spell damage per rank
- **Implementation:**
  - Defined in `src/perks.c` (line ~1415)
  - Helper function: `get_cleric_divine_spell_power_bonus()` (line ~4951)
  - Returns total damage bonus from Tier 1 & 2 perks
- **Integration Needed:** Damage calculation for divine offensive spells
- **Testing:** Purchase ranks, cast offensive spells (flame strike, etc.), verify damage increase

#### 3. Bonus Domain Spell I (ID: 100)
- **Cost:** 1 point per rank
- **Max Rank:** 5
- **Effect:** Prepare +1 additional domain spell per rank
- **Implementation:**
  - Defined in `src/perks.c` (line ~1435)
  - Helper function: `get_cleric_bonus_domain_spells()` (line ~4974)
  - Returns total bonus domain spell slots
- **Integration Needed:** Spell preparation system for domain spells
- **Testing:** Purchase ranks, prepare spells, verify additional domain spell slots

#### 4. Turn Undead Enhancement I (ID: 101)
- **Cost:** 1 point per rank
- **Max Rank:** 3
- **Effect:** +1 to turn undead DC per rank
- **Implementation:**
  - Defined in `src/perks.c` (line ~1455)
  - Helper function: `get_cleric_turn_undead_enhancement_bonus()` (line ~4997)
  - Returns total turn undead DC bonus from Tier 1 & 2 perks
- **Integration Needed:** Turn undead DC calculation
- **Testing:** Purchase ranks, use turn undead, verify DC increase

---

### Tier 2 Perks (2 points each)

#### 5. Domain Focus II (ID: 102)
- **Cost:** 2 points per rank
- **Max Rank:** 2
- **Prerequisite:** Domain Focus I (rank 3)
- **Effect:** Additional +1 domain spell DC per rank
- **Implementation:**
  - Defined in `src/perks.c` (line ~1477)
  - Integrated in `get_cleric_domain_focus_bonus()` function
  - Stacks with Domain Focus I for +5 DC total
- **Testing:** Max Domain Focus I first, then purchase this perk

#### 6. Divine Spell Power II (ID: 103)
- **Cost:** 2 points per rank
- **Max Rank:** 3
- **Prerequisite:** Divine Spell Power I (rank 5)
- **Effect:** Additional +2 divine spell damage per rank
- **Implementation:**
  - Defined in `src/perks.c` (line ~1497)
  - Integrated in `get_cleric_divine_spell_power_bonus()` function
  - Stacks with Divine Spell Power I for +11 damage total
- **Testing:** Max Divine Spell Power I first, then purchase this perk

#### 7. Bonus Domain Spell II (ID: 104)
- **Cost:** 2 points per rank
- **Max Rank:** 3
- **Prerequisite:** Bonus Domain Spell I (rank 5)
- **Effect:** Prepare +1 additional spell of any level per rank
- **Implementation:**
  - Defined in `src/perks.c` (line ~1517)
  - Helper function: `get_cleric_bonus_spell_slots()` (line ~4990)
  - Returns total bonus spell slots (any level)
- **Testing:** Max Bonus Domain Spell I first, then purchase this perk

#### 8. Turn Undead Enhancement II (ID: 105)
- **Cost:** 2 points per rank
- **Max Rank:** 2
- **Prerequisite:** Turn Undead Enhancement I (rank 3)
- **Effect:** Additional +2 turn undead DC per rank
- **Implementation:**
  - Defined in `src/perks.c` (line ~1537)
  - Integrated in `get_cleric_turn_undead_enhancement_bonus()` function
  - Stacks with Turn Undead Enhancement I for +7 DC total
- **Testing:** Max Turn Undead Enhancement I first, then purchase this perk

#### 9. Extended Domain (ID: 106)
- **Cost:** 2 points
- **Max Rank:** 1
- **Prerequisite:** Domain Focus I (at least 2 ranks)
- **Effect:** Domain spell duration +5 rounds
- **Implementation:**
  - Defined in `src/perks.c` (line ~1557)
  - Helper function: `get_cleric_extended_domain_bonus()` (line ~5020)
  - Returns 5 rounds if perk is active
- **Integration Needed:** Domain spell duration calculation
- **Testing:** Cast domain spells, verify duration increased by 5 rounds

#### 10. Divine Metamagic I (ID: 107)
- **Cost:** 2 points
- **Max Rank:** 1
- **Prerequisite:** Bonus Domain Spell I (at least 3 ranks)
- **Effect:** Apply metamagic to divine spells with -1 spell level increase
- **Implementation:**
  - Defined in `src/perks.c` (line ~1577)
  - Helper function: `get_cleric_divine_metamagic_reduction()` (line ~5020)
  - Returns 1 if perk is active (reduces metamagic level increase by 1)
- **Integration Needed:** Metamagic spell level calculation
- **Testing:** Apply metamagic to divine spells, verify reduced level increase (e.g., Empower Spell normally +2 levels, with this perk +1 level)

#### 11. Destroy Undead (ID: 108)
- **Cost:** 2 points
- **Max Rank:** 1
- **Prerequisite:** Turn Undead Enhancement I (rank 3)
- **Effect:** Turn undead destroys weak undead (3+ HD below cleric level) instantly
- **Implementation:**
  - Defined in `src/perks.c` (line ~1597)
  - Helper function: `has_destroy_undead()` (line ~5050)
  - Returns TRUE if perk is active
- **Integration Needed:** Turn undead effect system
- **Testing:** Use turn undead on weak undead, verify instant destruction

---

## Code Changes Summary

### Files Modified

#### 1. `src/structs.h`
**Lines 3168-3180:** Added 11 new perk ID constants
```c
/* Domain Master Tree - Tier 1 Perks (98-101) */
#define PERK_CLERIC_DOMAIN_FOCUS_1 98
#define PERK_CLERIC_DIVINE_SPELL_POWER_1 99
#define PERK_CLERIC_SPELL_POINT_RESERVE_1 100
#define PERK_CLERIC_TURN_UNDEAD_ENHANCEMENT_1 101

/* Domain Master Tree - Tier 2 Perks (102-108) */
#define PERK_CLERIC_DOMAIN_FOCUS_2 102
#define PERK_CLERIC_DIVINE_SPELL_POWER_2 103
#define PERK_CLERIC_SPELL_POINT_RESERVE_2 104
#define PERK_CLERIC_TURN_UNDEAD_ENHANCEMENT_2 105
#define PERK_CLERIC_EXTENDED_DOMAIN 106
#define PERK_CLERIC_DIVINE_METAMAGIC_1 107
#define PERK_CLERIC_DESTROY_UNDEAD 108
```

**Lines 3188-3230:** Updated Rogue perk IDs (109-138) to avoid conflicts

#### 2. `src/perks.h`
**Lines 154-161:** Added 7 new function declarations
```c
/* Cleric Domain Master tree perk functions */
int get_cleric_domain_focus_bonus(struct char_data *ch);
int get_cleric_divine_spell_power_bonus(struct char_data *ch);
int get_cleric_bonus_domain_spells(struct char_data *ch);
int get_cleric_bonus_spell_slots(struct char_data *ch);
int get_cleric_turn_undead_enhancement_bonus(struct char_data *ch);
int get_cleric_extended_domain_bonus(struct char_data *ch);
int get_cleric_divine_metamagic_reduction(struct char_data *ch);
bool has_destroy_undead(struct char_data *ch);
```

#### 3. `src/perks.c`
**Lines 1393-1617:** Added 11 new perk definitions in `define_cleric_perks()`
- All perks defined with proper prerequisites, costs, max ranks
- Effect types and values specified
- Special descriptions added

**Lines 4974-5010:** Added 7 new helper functions:
- `get_cleric_domain_focus_bonus()` - Calculates total domain spell DC bonus
- `get_cleric_divine_spell_power_bonus()` - Calculates total divine spell damage bonus
- `get_cleric_bonus_domain_spells()` - Calculates bonus domain spell slots
- `get_cleric_bonus_spell_slots()` - Calculates bonus spell slots (any level)
- `get_cleric_turn_undead_enhancement_bonus()` - Calculates total turn undead DC bonus
- `get_cleric_extended_domain_bonus()` - Returns domain spell duration bonus
- `get_cleric_divine_metamagic_reduction()` - Returns metamagic level reduction
- `has_destroy_undead()` - Checks if character can destroy weak undead

---

## Integration Points

### 1. Spell DC Calculation
**Files Needed:** `src/magic.c`, `src/spells.c`
**Function:** Spell save DC calculation functions

Add domain focus bonus to domain spells:
```c
int calculate_spell_dc(struct char_data *ch, int spell_num)
{
  int dc = 10 + spell_level + ability_modifier;
  
  /* Add domain focus bonus if it's a domain spell */
  if (is_domain_spell(ch, spell_num))
    dc += get_cleric_domain_focus_bonus(ch);
  
  return dc;
}
```

### 2. Divine Spell Damage
**Files Needed:** `src/magic.c`, `src/spells.c`
**Function:** Spell damage calculation

Add divine spell power bonus to offensive divine spells:
```c
int calculate_spell_damage(struct char_data *ch, int spell_num)
{
  int damage = base_damage;
  
  /* Add divine spell power bonus for clerics */
  if (GET_CLASS(ch) == CLASS_CLERIC)
    damage += get_cleric_divine_spell_power_bonus(ch);
  
  return damage;
}
```

### 3. Spell Preparation
**Files Needed:** `src/spell_prep.c`, `src/class.c`
**Function:** Spell preparation system

Add bonus spell slots:
```c
int get_bonus_spell_slots(struct char_data *ch, int spell_level, bool domain_only)
{
  int bonus = 0;
  
  /* Add bonus domain spell slots for clerics */
  if (GET_CLASS(ch) == CLASS_CLERIC && domain_only)
    bonus += get_cleric_bonus_domain_spells(ch);
  
  /* Add bonus spell slots (any level) for clerics */
  if (GET_CLASS(ch) == CLASS_CLERIC && !domain_only)
    bonus += get_cleric_bonus_spell_slots(ch);
  
  return bonus;
}
```

### 4. Turn Undead DC
**Files Needed:** `src/spec_abilities.c` (or wherever turn undead is implemented)
**Function:** Turn undead save DC

Add turn undead enhancement bonus:
```c
int calculate_turn_undead_dc(struct char_data *ch)
{
  int dc = 10 + (GET_LEVEL(ch) / 2) + GET_CHA_BONUS(ch);
  
  /* Add turn undead enhancement bonus */
  dc += get_cleric_turn_undead_enhancement_bonus(ch);
  
  return dc;
}
```

### 5. Domain Spell Duration
**Files Needed:** `src/magic.c`, `src/spells.c`
**Function:** Spell duration calculation

Add extended domain bonus:
```c
int calculate_spell_duration(struct char_data *ch, int spell_num)
{
  int duration = base_duration;
  
  /* Add extended domain bonus if it's a domain spell */
  if (is_domain_spell(ch, spell_num))
    duration += get_cleric_extended_domain_bonus(ch);
  
  return duration;
}
```

### 6. Metamagic Spell Level
**Files Needed:** `src/spell_parser.c`, `src/metamagic.c`
**Function:** Metamagic spell level calculation

Add divine metamagic level reduction:
```c
int calculate_metamagic_level_increase(struct char_data *ch, int spell_num, int metamagic_flags)
{
  int level_increase = base_metamagic_level_increase(metamagic_flags);
  
  /* Reduce level increase for divine metamagic */
  if (GET_CLASS(ch) == CLASS_CLERIC)
    level_increase -= get_cleric_divine_metamagic_reduction(ch);
  
  return MAX(0, level_increase); /* Minimum 0 */
}
```

### 7. Destroy Undead
**Files Needed:** `src/spec_abilities.c`
**Function:** Turn undead effect application

Check for destroy undead:
```c
void apply_turn_undead(struct char_data *ch, struct char_data *victim)
{
  int hd_difference = GET_LEVEL(ch) - GET_LEVEL(victim);
  
  /* Check if undead should be destroyed */
  if (has_destroy_undead(ch) && hd_difference >= 3)
  {
    send_to_char(ch, "Your holy power destroys the undead!\r\n");
    die(victim, ch);
    return;
  }
  
  /* Otherwise, apply fear/flee effect */
  apply_fear(victim);
}
```

---

## Purchase Flow

### Example: Building a Domain Master Cleric

**Level 3 Cleric (3 perk points available):**
```
perk purchase 98  (Domain Focus I, rank 1) - Costs 1 point
perk purchase 99  (Divine Spell Power I, rank 1) - Costs 1 point
perk purchase 100 (Bonus Domain Spell I, rank 1) - Costs 1 point
```
Result: +1 domain DC, +1 spell damage, +1 domain spell slot

**Level 6 Cleric (6 more points = 9 total):**
```
perk purchase 98  (Domain Focus I, rank 2) - Costs 1 point
perk purchase 98  (Domain Focus I, rank 3) - Costs 1 point
perk purchase 99  (Divine Spell Power I, rank 2) - Costs 1 point
perk purchase 99  (Divine Spell Power I, rank 3) - Costs 1 point
perk purchase 100 (Bonus Domain Spell I, rank 2) - Costs 1 point
perk purchase 101 (Turn Undead Enhancement I, rank 1) - Costs 1 point
```
Result: +3 domain DC, +3 spell damage, +2 domain spell slots, +1 turn undead DC

**Level 12 Cleric (18 more points = 27 total):**
```
perk purchase 99  (Divine Spell Power I, rank 4) - Costs 1 point
perk purchase 99  (Divine Spell Power I, rank 5) - Costs 1 point
perk purchase 100 (Bonus Domain Spell I, rank 3-5) - Costs 3 points
perk purchase 101 (Turn Undead Enhancement I, rank 2-3) - Costs 2 points
perk purchase 102 (Domain Focus II, rank 1-2) - Costs 4 points
perk purchase 103 (Divine Spell Power II, rank 1-3) - Costs 6 points
perk purchase 104 (Bonus Domain Spell II, rank 1-3) - Costs 6 points
perk purchase 106 (Extended Domain) - Costs 2 points
perk purchase 108 (Destroy Undead) - Costs 2 points
```
Result: +5 domain DC, +11 spell damage, +5 domain spell slots, +3 any-level spell slots, +3 turn undead DC, +5 rounds domain duration, destroy weak undead

---

## Testing Checklist

### ✅ Completed
- [x] Code compiles without errors
- [x] Perk definitions load on boot
- [x] Perks show in `perk list` command
- [x] Prerequisite checking works (Tier 2 requires maxed Tier 1)
- [x] Multi-rank perks can be purchased incrementally
- [x] Perk info displays correct information
- [x] Helper functions return correct values

### ⚠️ Integration Testing Needed
- [ ] Domain Focus increases domain spell DC correctly
- [ ] Divine Spell Power increases offensive spell damage correctly
- [ ] Bonus Domain Spell I grants additional domain spell slots correctly
- [ ] Bonus Domain Spell II grants additional spell slots (any level) correctly
- [ ] Turn Undead Enhancement increases turn undead DC correctly
- [ ] Extended Domain increases domain spell duration by 5 rounds
- [ ] Divine Metamagic reduces metamagic spell level increase by 1
- [ ] Destroy Undead instantly kills weak undead (3+ HD below level)
- [ ] All bonuses stack properly with Tier 1 and Tier 2 combined
- [ ] Bonuses only apply to clerics
- [ ] Domain spells are correctly identified for bonuses

---

## Balancing Notes

### Point Investment
- **Tier 1 Total:** 16 points maximum (3+5+5+3)
- **Tier 2 Total:** 21 points maximum (4+6+6+4+2+2+2)
- **Combined T1+T2:** 37 points for full investment
- **Level Requirement:** ~Level 12-13 to max both tiers (3 points per level)

### Power Level
- **Domain Spell DC:** +5 maximum (very powerful for save-or-die spells)
- **Spell Damage:** +11 maximum (significant but not overwhelming)
- **Domain Spell Slots:** +5 maximum (extends domain spell usage significantly)
- **Any-Level Spell Slots:** +3 maximum (flexible spell preparation)
- **Turn Undead DC:** +7 maximum (makes turning much more reliable)
- **Domain Duration:** +5 rounds (25-50% increase typically)
- **Metamagic Level:** -1 level increase (allows higher-level metamagic spells)
- **Destroy Undead:** Instant kill on weak undead (great utility)

### Comparison to Other Trees
- **Divine Healer:** Focuses on healing output and party support
- **Battle Cleric:** Focuses on personal combat effectiveness
- **Domain Master:** Focuses on spell versatility and divine magic enhancement
- All three trees are roughly equal in power but serve different playstyles

---

## Future Expansion

### Tier 3 Perks (3-4 points each) - TODO
- Domain Focus III: +2 DC
- Divine Spell Power III: +3 damage per rank (2 ranks)
- Bonus Domain Spell III: +1 domain spell per rank (2 ranks)
- Divine Metamagic II: -2 spell level increase
- Greater Turning: Turn undead affects +2 HD levels
- Domain Mastery: Use domain powers one additional time per day

### Tier 4 Capstone Perks (5 points each) - TODO
- Divine Channeler: All divine spells +3 DC, +10 damage, domain powers usable 2x more per day
- Master of the Undead: Turn undead DC +5, can control turned undead, destroy up to 10 HD undead

---

## Version History

**v1.1 - October 26, 2025**
- Reworked spell point perks to use spell preparation system instead
- Changed "Spell Point Reserve I/II" to "Bonus Domain Spell I/II"
- Bonus Domain Spell I: +1 domain spell slot per rank (max 5)
- Bonus Domain Spell II: +1 any-level spell slot per rank (max 3)
- Changed "Divine Metamagic I" to reduce spell level increase by 1 instead of spell point cost
- Updated helper functions to match spell preparation system
- Updated documentation to reflect spell preparation integration

**v1.0 - October 25, 2025**
- Initial implementation of Domain Master Tree Tiers 1-2
- 11 perks fully defined in code
- 7 helper functions implemented
- All code compiled successfully
- Integration with spell system pending

---

## Credits

- **Design:** Based on CLERIC_PERKS.md specification
- **Implementation:** Domain Master Tree Tiers 1-2
- **Codebase:** LuminariMUD
- **Compilation Status:** ✅ SUCCESS (October 25, 2025)
