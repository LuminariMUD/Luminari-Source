# Cleric Domain Master Tree Tier 1-4 Implementation

## Overview
This document describes the implementation of all four tiers of the Cleric Domain Master perk tree. All 19 perks have been defined in the codebase and are ready for integration with the spell system.

**Status:** ✅ FULLY DEFINED IN CODE (v1.5)
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
- **Effect:** +1 bonus domain spell slot per rank (consumed during casting, regenerates 1 per 5 minutes)
- **Implementation:**
  - Defined in `src/perks.c` (line ~1435)
  - Helper function: `get_cleric_bonus_domain_spells()` (line ~4974)
  - Returns total bonus domain spell slots
- **Integration Needed:** Spell casting system - check for bonus slots when casting domain spells
- **Design:** Bonus slots are stored separately and consumed during casting instead of modifying spell preparation. Slots regenerate automatically at 1 per 5 minutes.
- **Testing:** Purchase ranks, cast domain spells, verify bonus slots consumed before main prepared slots, verify regeneration over time

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
- **Effect:** +1 bonus spell slot (any level) per rank (consumed during casting, regenerates 1 per 5 minutes)
- **Implementation:**
  - Defined in `src/perks.c` (line ~1517)
  - Helper function: `get_cleric_bonus_spell_slots()` (line ~4990)
  - Returns total bonus spell slots (any level)
- **Integration Needed:** Spell casting system - check for bonus slots when casting divine spells
- **Design:** Bonus slots are stored separately and consumed during casting instead of modifying spell preparation. Slots regenerate automatically at 1 per 5 minutes.
- **Testing:** Max Bonus Domain Spell I first, then purchase this perk, cast divine spells, verify bonus slots consumed before main prepared slots, verify regeneration over time

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

### Tier 3 Perks (3-4 points each)

#### 1. Domain Focus III (ID: 160)
- **Cost:** 3 points
- **Max Rank:** 1
- **Prerequisite:** Domain Focus II (rank 5)
- **Effect:** +2 to domain spell DC
- **Implementation:**
  - Defined in `src/perks.c` (line ~1570)
  - Helper function: `get_cleric_domain_focus_bonus()` updated (line ~5078)
  - Returns cumulative DC bonus including Tier 1, 2, and 3
- **Stacking:** Stacks with Domain Focus I and II for maximum +7 DC
- **Testing:** Purchase perk, cast domain spells, verify +2 DC increase

#### 2. Divine Spell Power III (ID: 161)
- **Cost:** 3 points per rank
- **Max Rank:** 2
- **Prerequisite:** Divine Spell Power II (rank 5)
- **Effect:** +3 to divine spell damage per rank
- **Implementation:**
  - Defined in `src/perks.c` (line ~1588)
  - Helper function: `get_cleric_divine_spell_power_bonus()` updated (line ~5109)
  - Returns cumulative damage bonus including Tier 1, 2, and 3
- **Stacking:** Stacks with Divine Spell Power I and II for maximum +17 damage
- **Testing:** Purchase ranks, cast offensive divine spells, verify +3/+6 damage increase

#### 3. Bonus Domain Spell III (ID: 162)
- **Cost:** 3 points per rank
- **Max Rank:** 2
- **Prerequisite:** Bonus Domain Spell II (rank 3)
- **Effect:** +1 bonus domain spell slot per rank (regenerates 1 per 5 minutes)
- **Implementation:**
  - Defined in `src/perks.c` (line ~1606)
  - Helper function: `get_cleric_bonus_domain_spells()` updated (line ~5137)
  - Returns cumulative bonus slots including Tier 1, 2, and 3
- **Stacking:** Stacks with Bonus Domain Spell I and II for maximum +10 domain spell slots
- **Regeneration:** Same regeneration system as lower tiers (1 slot per 5 minutes)
- **Testing:** Purchase ranks, cast domain spells, verify bonus slots, test regeneration

#### 4. Divine Metamagic II (ID: 163)
- **Cost:** 4 points
- **Max Rank:** 1
- **Prerequisite:** Divine Metamagic I (rank 1)
- **Effect:** Metamagic feats increase spell level by 2 less
- **Implementation:**
  - Defined in `src/perks.c` (line ~1624)
  - Helper function: `get_cleric_divine_metamagic_reduction()` updated (line ~5220)
  - Returns cumulative reduction including Tier 2 and 3
- **Stacking:** Stacks with Divine Metamagic I for total -3 spell level increase
- **Example:** Empower Spell (+2 levels) becomes +0 levels with both perks
- **Testing:** Apply metamagic feats, verify spell level reduction

#### 5. Greater Turning (ID: 164)
- **Cost:** 3 points
- **Max Rank:** 1
- **Prerequisite:** Turn Undead Enhancement II (rank 5)
- **Effect:** Turn undead affects undead +2 HD levels higher
- **Implementation:**
  - Defined in `src/perks.c` (line ~1642)
  - Helper function: `get_cleric_greater_turning_bonus()` (line ~5256)
  - Returns +2 HD bonus for turn undead
- **Integration Needed:** Turn undead HD calculation
- **Testing:** Use turn undead on higher HD undead, verify extended range

#### 6. Domain Mastery (ID: 165)
- **Cost:** 3 points
- **Max Rank:** 1
- **Prerequisite:** Extended Domain (rank 1)
- **Effect:** Use domain powers +1 additional time per day
- **Implementation:**
  - Defined in `src/perks.c` (line ~1660)
  - Helper function: `get_cleric_domain_mastery_bonus()` (line ~5274)
  - Returns +1 daily use for domain powers
- **Integration Needed:** Domain power usage tracking
- **Testing:** Use domain powers, verify extra daily use

---

### Tier 4 Capstone Perks (5 points each)

#### 1. Divine Channeler (ID: 166)
- **Cost:** 5 points
- **Max Rank:** 1
- **Prerequisite:** Domain Focus III (rank 1)
- **Effect:** Master of divine magic - ALL divine spells gain +3 DC and +10 damage, domain powers can be used 2x per day
- **Implementation:**
  - Defined in `src/perks.c` (line ~1678)
  - Integrated into existing helper functions:
    * `get_cleric_domain_focus_bonus()` - adds +3 DC
    * `get_cleric_divine_spell_power_bonus()` - adds +10 damage
    * `get_cleric_domain_mastery_bonus()` - returns -1 to indicate doubling
- **Stacking:** Fully stacks with all previous perks
  - Maximum DC bonus: +10 (+3+2+5 from tiers + 3 from capstone)
  - Maximum damage bonus: +27 (+5+6+6 from tiers + 10 from capstone)
- **Integration Needed:** Domain power daily use doubling
- **Testing:** Verify all divine spell DCs increase by +3, all damage by +10, domain powers 2x/day

#### 2. Master of the Undead (ID: 167)
- **Cost:** 5 points
- **Max Rank:** 1
- **Prerequisite:** Greater Turning (rank 1)
- **Effect:** Ultimate turning - +5 turn DC, can control turned undead, destroy undead up to 10 HD below cleric level
- **Implementation:**
  - Defined in `src/perks.c` (line ~1696)
  - Helper functions:
    * `get_cleric_master_of_undead_dc_bonus()` (line ~5296) - returns +5 turn DC
    * `has_control_undead()` (line ~5313) - returns TRUE for control ability
    * `get_destroy_undead_threshold()` (line ~5327) - returns 10 HD threshold
- **Stacking:** Stacks with all previous turn undead bonuses
  - Maximum turn DC: +12 (+3+4 from Tier 1-2 + 5 from capstone)
  - HD bonus: +2 from Greater Turning
  - Destroy threshold: 10 HD instead of 3 HD
- **Integration Needed:** Turn undead DC, control mechanics, destroy threshold
- **Testing:** Use turn undead, verify +5 DC, test control on turned undead, destroy high HD undead

---

## Code Changes Summary

### Files Modified

#### 1. `src/structs.h`
**Lines 3168-3180:** Added 11 Tier 1-2 perk ID constants
**Lines 3182-3191:** Added 8 Tier 3-4 perk ID constants
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

/* Domain Master Tree - Tier 3 (160-165) */
#define PERK_CLERIC_DOMAIN_FOCUS_3 160
#define PERK_CLERIC_DIVINE_SPELL_POWER_3 161
#define PERK_CLERIC_SPELL_POINT_RESERVE_3 162
#define PERK_CLERIC_DIVINE_METAMAGIC_2 163
#define PERK_CLERIC_GREATER_TURNING 164
#define PERK_CLERIC_DOMAIN_MASTERY 165

/* Domain Master Tree - Tier 4 (166-167) */
#define PERK_CLERIC_DIVINE_CHANNELER 166
#define PERK_CLERIC_MASTER_OF_UNDEAD 167
```

**Lines 5712-5716:** Added 4 bonus spell slot tracking fields

**Lines 3188-3230:** Updated Rogue perk IDs (109-138) to avoid conflicts

#### 2. `src/perks.h`
**Lines 154-178:** Added 13 function declarations (8 original + 5 new for Tier 3-4)
```c
/* Cleric Domain Master tree perk functions */
bool is_divine_spellcasting_class(int class_num);
int get_cleric_domain_focus_bonus(struct char_data *ch);
int get_cleric_divine_spell_power_bonus(struct char_data *ch);
int get_cleric_bonus_domain_spells(struct char_data *ch);
int get_cleric_bonus_spell_slots(struct char_data *ch);
int get_cleric_turn_undead_enhancement_bonus(struct char_data *ch);
int get_cleric_extended_domain_bonus(struct char_data *ch);
int get_cleric_divine_metamagic_reduction(struct char_data *ch);
bool has_destroy_undead(struct char_data *ch);
int get_cleric_greater_turning_bonus(struct char_data *ch);           /* NEW: Tier 3 */
int get_cleric_domain_mastery_bonus(struct char_data *ch);            /* NEW: Tier 3 */
int get_cleric_master_of_undead_dc_bonus(struct char_data *ch);       /* NEW: Tier 4 */
bool has_control_undead(struct char_data *ch);                        /* NEW: Tier 4 */
int get_destroy_undead_threshold(struct char_data *ch);               /* NEW: Tier 4 */
```

#### 3. `src/perks.c`
**Lines 1393-1617:** Added 11 Tier 1-2 perk definitions in `define_cleric_perks()`
**Lines 1570-1714:** Added 8 Tier 3-4 perk definitions in `define_cleric_perks()`
- All 19 perks defined with proper prerequisites, costs, max ranks
- Effect types and values specified
- Special descriptions added
- Tier 3 perks cost 3-4 points each, require maxed Tier 2 prerequisites
- Tier 4 capstone perks cost 5 points each, require specific Tier 3 perks

**Lines 4928-5062:** Added 8 initial helper functions:
- `is_divine_spellcasting_class()` - Checks if class is divine (Cleric, Druid, Ranger, Paladin, Blackguard, Inquisitor)
- `get_cleric_domain_focus_bonus()` - Calculates total domain spell DC bonus (now includes Tier 3-4)
- `get_cleric_divine_spell_power_bonus()` - Calculates total divine spell damage bonus (now includes Tier 3-4)
- `get_cleric_bonus_domain_spells()` - Calculates bonus domain spell slots (now includes Tier 3)
- `get_cleric_bonus_spell_slots()` - Calculates bonus spell slots (any level)
- `get_cleric_turn_undead_enhancement_bonus()` - Calculates total turn undead DC bonus
- `get_cleric_extended_domain_bonus()` - Returns domain spell duration bonus
- `get_cleric_divine_metamagic_reduction()` - Returns metamagic level reduction (now includes Tier 3)
- `has_destroy_undead()` - Checks if character can destroy weak undead

**Lines 5256-5341:** Added 5 new Tier 3-4 helper functions:
- `get_cleric_greater_turning_bonus()` - Returns +2 HD bonus for turn undead
- `get_cleric_domain_mastery_bonus()` - Returns daily use bonus for domain powers
- `get_cleric_master_of_undead_dc_bonus()` - Returns +5 turn DC bonus from capstone
- `has_control_undead()` - Checks if character can control turned undead
- `get_destroy_undead_threshold()` - Returns HD threshold for instant destruction (3 or 10)

---

## Integration Points

### 1. Spell DC Calculation (✅ IMPLEMENTED)
**Files Modified:** `src/magic.c` (line ~461)
**Function:** `mag_savingthrow_full()`

Domain focus bonus is now applied to domain spells:
```c
challenge += get_spell_dc_bonus(ch);

/* Add domain focus bonus for domain spells */
if (is_domain_spell_of_ch(ch, spellnum))
  challenge += get_cleric_domain_focus_bonus(ch);
```

### 2. Divine Spell Damage (✅ IMPLEMENTED)
**Files Modified:** `src/magic.c` (line ~2734), `src/perks.c` (line ~4924)
**Function:** `mag_damage()`, `is_divine_spellcasting_class()`

Divine spell power bonus is now applied to all divine spell damage:
```c
/* Add divine spell power bonus for divine casters with Domain Master perks */
if (!IS_NPC(ch) && is_divine_spellcasting_class(GET_CASTING_CLASS(ch)))
  dam += get_cleric_divine_spell_power_bonus(ch);
```

Helper function checks if casting class is divine:
```c
bool is_divine_spellcasting_class(int class_num)
{
  switch (class_num)
  {
    case CLASS_CLERIC:
    case CLASS_DRUID:
    case CLASS_RANGER:
    case CLASS_PALADIN:
    case CLASS_BLACKGUARD:
    case CLASS_INQUISITOR:
      return TRUE;
    default:
      return FALSE;
  }
}
```

### 3. Bonus Spell Slots (⚠️ INTEGRATION NEEDED)
**Files Needed:** `src/spell_parser.c` or spell casting system
**Function:** Spell slot consumption during casting

**Design:** Bonus spell slots are tracked separately from prepared spells. During spell casting, the system should:

1. Check if bonus slots are available:
   - For domain spells: Check `get_cleric_bonus_domain_spells(ch)`
   - For any divine spell: Check `get_cleric_bonus_spell_slots(ch)`
2. Consume bonus slot first if available
3. Only consume prepared spell slot if no bonus slots remain

**Suggested Implementation:**
```c
bool consume_spell_slot(struct char_data *ch, int spellnum)
{
  /* Check if we can use a bonus domain spell slot */
  if (is_domain_spell_of_ch(ch, spellnum))
  {
    int bonus_domain_slots = get_cleric_bonus_domain_spells(ch);
    int used_domain_slots = GET_BONUS_DOMAIN_SLOTS_USED(ch); /* Need to track this */
    
    if (used_domain_slots < bonus_domain_slots)
    {
      GET_BONUS_DOMAIN_SLOTS_USED(ch)++;
      return TRUE; /* Used bonus slot, don't consume prepared spell */
    }
  }
  
  /* Check if we can use a bonus spell slot (any level) */
  if (is_divine_spellcasting_class(GET_CASTING_CLASS(ch)))
  {
    int bonus_slots = get_cleric_bonus_spell_slots(ch);
    int used_slots = GET_BONUS_SLOTS_USED(ch); /* Need to track this */
    
    if (used_slots < bonus_slots)
    {
      GET_BONUS_SLOTS_USED(ch)++;
      return TRUE; /* Used bonus slot, don't consume prepared spell */
    }
  }
  
  /* No bonus slots available, consume prepared spell slot */
  return FALSE;
}
```

**Character Data Needed:**
- `GET_BONUS_DOMAIN_SLOTS_USED(ch)` - Track used bonus domain slots
- `GET_BONUS_DOMAIN_REGEN_TIMER(ch)` - Timer for domain slot regeneration (0-4 ticks)
- `GET_BONUS_SLOTS_USED(ch)` - Track used bonus any-level slots
- `GET_BONUS_SLOTS_REGEN_TIMER(ch)` - Timer for any-level slot regeneration (0-4 ticks)

**Regeneration System (✅ IMPLEMENTED):**
- Bonus slots regenerate at a rate of **1 slot per 5 minutes (5 ticks)**
- Implemented in `src/limits.c` in the character update loop (line ~1790)
- Uses regeneration timers that increment each tick
- When `GET_BONUS_DOMAIN_SLOTS_USED(ch) > 0`, timer increments
- When timer reaches 5, regenerates 1 slot and resets timer
- Minimum value is 0 (cannot go negative)
- Full slots available immediately after rest/spell preparation (counters reset to 0)
- **Files Modified:**
  - `src/limits.c` - Added regeneration logic in character update loop
  - `src/structs.h` - Added 4 fields to `player_special_data_saved`
  - `src/utils.h` - Added 4 GET macros for accessing the fields
  - `src/players.c` - Added save/load support (tags: BDsU, BDsT, BSlU, BSlT) and initialization

**Regeneration Implementation:**
```c
/* In src/limits.c - Character update loop */
/* Regenerates 1 slot every 5 ticks (5 minutes) */
if (GET_BONUS_DOMAIN_SLOTS_USED(ch) > 0)
{
  GET_BONUS_DOMAIN_REGEN_TIMER(ch)++;
  if (GET_BONUS_DOMAIN_REGEN_TIMER(ch) >= 5)
  {
    GET_BONUS_DOMAIN_SLOTS_USED(ch)--;
    GET_BONUS_DOMAIN_REGEN_TIMER(ch) = 0;
    send_to_char(ch, "You feel a bonus domain spell slot restore.\r\n");
  }
}
else
{
  GET_BONUS_DOMAIN_REGEN_TIMER(ch) = 0;
}

/* Same logic for GET_BONUS_SLOTS_USED(ch) */
```
```
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
- [x] Domain Focus DC bonus integrated into `mag_savingthrow_full()`
- [x] Divine Spell Power damage bonus integrated into `mag_damage()`
- [x] Divine class detection function `is_divine_spellcasting_class()` implemented

### ⚠️ Integration Testing Needed
- [x] Domain Focus increases domain spell DC correctly
- [x] Divine Spell Power increases offensive spell damage correctly
- [ ] Bonus Domain Spell I: Track and consume bonus domain spell slots during casting
- [ ] Bonus Domain Spell II: Track and consume bonus any-level spell slots during casting
- [ ] Bonus slots regenerate at 1 slot per 5 minutes
- [ ] Bonus slots reset to 0 on rest/spell preparation
- [ ] Regeneration messages display correctly
- [ ] Regeneration stops when slots are fully restored
- [ ] Turn Undead Enhancement increases turn undead DC correctly
- [ ] Extended Domain increases domain spell duration by 5 rounds
- [ ] Divine Metamagic reduces metamagic spell level increase by 1
- [ ] Destroy Undead instantly kills weak undead (3+ HD below level)
- [ ] All bonuses stack properly with Tier 1 and Tier 2 combined
- [ ] Bonuses only apply to clerics (or appropriate divine classes)
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
  - *Regenerates at 1 slot per 5 minutes = 12 slots per hour sustained*
  - *Full restoration in 25 minutes if all 5 slots used*
- **Any-Level Spell Slots:** +3 maximum (flexible spell preparation)
  - *Regenerates at 1 slot per 5 minutes = 12 slots per hour sustained*
  - *Full restoration in 15 minutes if all 3 slots used*
- **Turn Undead DC:** +7 maximum (makes turning much more reliable)
- **Domain Duration:** +5 rounds (25-50% increase typically)
- **Metamagic Level:** -1 level increase (allows higher-level metamagic spells)
- **Destroy Undead:** Instant kill on weak undead (great utility)

**Regeneration Impact:**
- Bonus slots encourage active spell usage without fear of running out
- 5-minute regeneration prevents spell spam but allows steady casting
- Provides sustained advantage in long dungeons or extended combat scenarios
- Does not make clerics overpowered in short encounters (same initial burst)
- Rewards tactical play and spell conservation

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

**v1.5 - October 26, 2025**
- ✅ Implemented Tier 3 perks (IDs 160-165)
  * Domain Focus III: +2 DC (1 rank) - requires Domain Focus II max
  * Divine Spell Power III: +3 damage per rank (2 ranks) - requires Divine Spell Power II max
  * Bonus Domain Spell III: +1 slot per rank (2 ranks) - requires Bonus Domain Spell II max
  * Divine Metamagic II: -2 spell level increase (1 rank) - requires Divine Metamagic I
  * Greater Turning: +2 HD levels for turn undead (1 rank) - requires Turn Undead Enhancement II max
  * Domain Mastery: +1 domain power use per day (1 rank) - requires Extended Domain
- ✅ Implemented Tier 4 capstone perks (IDs 166-167)
  * Divine Channeler: +3 DC, +10 damage, 2x domain powers (5 points) - requires Domain Focus III
  * Master of the Undead: +5 turn DC, control undead, destroy 10 HD undead (5 points) - requires Greater Turning
- ✅ Updated all helper functions to include Tier 3-4 bonuses
- ✅ Added 5 new helper functions:
  * get_cleric_greater_turning_bonus() - returns +2 HD bonus
  * get_cleric_domain_mastery_bonus() - returns daily use bonus
  * get_cleric_master_of_undead_dc_bonus() - returns +5 turn DC
  * has_control_undead() - checks for control ability
  * get_destroy_undead_threshold() - returns HD threshold for destruction
- ✅ Code compiled successfully with no errors

**v1.4 - October 25, 2025**
- **IMPLEMENTED** bonus spell slot regeneration system
- Added 4 new character data fields in `src/structs.h`: bonus_domain_slots_used, bonus_domain_regen_timer, bonus_slots_used, bonus_slots_regen_timer
- Added 4 GET macros in `src/utils.h` for accessing regeneration data
- Implemented regeneration logic in `src/limits.c` character update loop (line ~1790)
- Added save/load support in `src/players.c` (tags: BDsU, BDsT, BSlU, BSlT)
- Regeneration rate: 1 slot per 5 minutes (5 ticks)
- Players receive feedback messages when slots restore
- Code compiled successfully with no errors

**v1.3 - October 26, 2025**
- Updated bonus spell slot system to use regeneration instead of rest-only reset
- Bonus domain slots regenerate at 1 slot per 5 minutes
- Bonus any-level slots regenerate at 1 slot per 5 minutes
- Added suggested regeneration implementation using mud event system
- Updated testing checklist to include regeneration testing
- Clarified that slots still reset to 0 (full) on rest/spell preparation
- Added regeneration messages for player feedback

**v1.2 - October 26, 2025**
- Implemented domain focus DC bonus integration in `mag_savingthrow_full()` (src/magic.c line ~461)
- Implemented divine spell power damage bonus integration in `mag_damage()` (src/magic.c line ~2734)
- Added `is_divine_spellcasting_class()` helper function to check for divine casting classes
- Updated bonus spell slot design: slots are now tracked separately and consumed during casting
- Bonus Domain Spell I: Provides bonus domain spell slots consumed before prepared slots
- Bonus Domain Spell II: Provides bonus any-level spell slots consumed before prepared slots
- Documented required character data fields: GET_BONUS_DOMAIN_SLOTS_USED and GET_BONUS_SLOTS_USED
- Updated integration documentation with detailed implementation approach

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
