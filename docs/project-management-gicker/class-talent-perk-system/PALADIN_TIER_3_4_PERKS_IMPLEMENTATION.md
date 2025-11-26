# Knight of the Chalice Paladin Perks - Tier 3 & 4 Implementation

## Overview
This document describes the implementation of Tier 3 and Tier 4 perks for the Knight of the Chalice paladin perk tree. These perks enhance smite evil, holy blade, and provide reactive combat bonuses.

---

## Tier 3 Perks (3 points each)

### 1. Divine Might
**Cost:** 3 perk points  
**Prerequisite:** Tier 2 Mighty Smite  

**Description:** As a swift action, you can add your Charisma bonus to all melee damage rolls for 1 minute. Cooldown: 5 minutes.

**Implementation:**
- **Command:** `divinemight`
- **Action Type:** Swift action (doesn't consume main action)
- **Effect:** Applies SKILL_DIVINE_MIGHT affect for 10 rounds (1 minute)
  - APPLY_DAMROLL bonus equal to CHA modifier
  - BONUS_TYPE_SACRED (stacks with other bonuses)
- **Cooldown:** 5 minutes via daily use cooldown system (eDIVINE_MIGHT event)
- **Files Modified:**
  - `src/act.offensive.c`: `do_divine_might()` command implementation
  - `src/act.h`: Command declaration
  - `src/interpreter.c`: Command registration
  - `src/spell_parser.c`: spello() entry for wear-off message

**Usage:**
```
divinemight
```

---

### 2. Exorcism of the Slain
**Cost:** 3 perk points  
**Prerequisite:** Tier 2 Mighty Smite  

**Description:** When you use Smite Evil against evil outsiders, devils, or demons, you deal an additional +4d6 damage.

**Implementation:**
- **Trigger:** Automatic when smiting evil creatures
- **Damage:** +4d6 dice damage added to smite calculation
- **Target Detection:** Currently uses IS_EVIL() check
  - TODO: Add proper demon/devil/outsider subrace detection when race system supports it
- **Files Modified:**
  - `src/fight.c`: Damage bonus applied in smite damage calculation (lines 6632-6665)

**Notes:**
- Damage bonus is displayed in combat messages
- Currently applies to all evil creatures until proper subrace system is implemented
- TODO comment added for future refinement

---

### 3. Holy Sword
**Cost:** 3 perk points  
**Prerequisite:** Tier 2 Holy Blade  

**Description:** Your Holy Blade ability is enhanced:
- Enhancement bonus increases from +2 to +4
- Deals an additional +2d6 holy damage against evil creatures

**Implementation:**
- **Upgrade to Holy Blade:**
  - Enhancement bonus: +4 (was +2)
  - Holy damage: +2d6 against evil targets
- **Trigger:** Automatic when Holy Blade is active and attacking evil creatures
- **Stacking:** Works alongside base Holy Blade and Holy Weapon effects
- **Files Modified:**
  - `src/act.offensive.c`: Modified `do_holy_blade()` to check for perk and apply +4 enhancement
  - `src/fight.c`: Added +2d6 holy damage calculation (lines 6571-6589)

**Notes:**
- Seamlessly upgrades existing Holy Blade ability
- Enhancement bonus automatically changes based on perk ownership
- Holy damage displays separately in combat log

---

### 4. Zealous Smite
**Cost:** 3 perk points  
**Prerequisite:** Tier 3 Divine Might or Exorcism of the Slain  

**Description:** When you kill an evil creature with Smite Evil active, you regain one use of Smite Evil. Cooldown: 5 minutes.

**Implementation:**
- **Trigger:** Automatic when killing an evil creature while smiting
- **Effect:** Restores 1 Smite Evil use by decrementing the cooldown event counter
- **Message:** "Your righteous fury is restored by slaying [victim]!"
- **Cooldown:** 5 minutes (handled by event system)
- **Files Modified:**
  - `src/fight.c`: Added logic in `die()` function before `raw_kill()` (lines 2536-2560)

**Technical Details:**
- Checks for active SKILL_SMITE_EVIL affect
- Accesses eSMITE_EVIL cooldown event
- Decrements the "uses" counter in event sVariables
- Only triggers if victim is evil and dies

---

## Tier 4 Perks (4 points each)

### 5. Blinding Smite
**Cost:** 4 perk points  
**Prerequisite:** Tier 3 Divine Might or Exorcism of the Slain  

**Description:** When you strike an evil creature with Smite Evil active, the target must make a Will save (DC = 10 + paladin level + CHA modifier) or be blinded for 2 rounds.

**Implementation:**
- **Trigger:** Automatic on successful smite evil hits against evil creatures
- **Save:** Will save (DC = 10 + paladin level + CHA modifier)
- **Effect:** Blindness (AFF_BLIND) for 2 rounds via SPELL_BLINDNESS affect
- **Immunity Check:** Uses `can_blind()` to respect blind immunity
- **Files Modified:**
  - `src/fight.c`: Added in hit() function after damage is applied (lines 12001-12020)

**Messages:**
- To attacker: "Your righteous strike blinds [victim] with holy radiance!"
- To victim: "[attacker]'s smite blinds you with holy radiance!"
- To room: "[attacker]'s smite blinds [victim] with holy radiance!"

---

### 6. Overwhelming Smite
**Cost:** 4 perk points  
**Prerequisite:** Tier 3 Zealous Smite  

**Description:** When you score a critical hit with Smite Evil active against an evil creature, the target must make a Fortitude save (DC = 10 + paladin level + STR modifier) or be knocked prone.

**Implementation:**
- **Trigger:** Automatic on critical hits while smiting evil creatures
- **Save:** Fortitude save (DC = 10 + paladin level + STR modifier)
- **Effect:** Knockdown/prone via `perform_knockdown()`
- **Respects:** All knockdown immunity checks (NOBASH, size limits, etc.)
- **Files Modified:**
  - `src/fight.c`: Added in hit() function after damage, checks `is_critical` (lines 12022-12037)

**Messages:**
- To attacker: "Your devastating smite knocks [victim] to the ground!"
- To victim: "[attacker]'s devastating smite knocks you to the ground!"
- To room: "[attacker]'s devastating smite knocks [victim] to the ground!"

**Technical Details:**
- Uses existing `perform_knockdown()` function
- Respects all normal knockdown mechanics
- No counterattack allowed (can_counter = false)
- Full display messages (display = true)

---

### 7. Sacred Vengeance
**Cost:** 4 perk points  
**Prerequisite:** Tier 3 Holy Sword  

**Description:** When an ally in your group drops below 25% HP or is killed, you gain +4 to hit and +8 to damage for 3 rounds. Cooldown: 5 minutes per paladin.

**Implementation:**
- **Trigger:** Automatic when group member's HP drops below 25% or dies
- **Range:** Must be in same room as injured/killed ally
- **Effect:** Applies SKILL_SACRED_VENGEANCE affect for 3 rounds
  - APPLY_HITROLL: +4 (BONUS_TYPE_SACRED)
  - APPLY_DAMROLL: +8 (BONUS_TYPE_SACRED)
- **Cooldown:** Enforced by affect existence (won't reapply if already active)
- **Files Modified:**
  - `src/fight.c`: Added in `damage()` function after GET_HIT() is reduced (lines 5654-5698)
  - `src/spell_parser.c`: spello() entry for wear-off message

**Messages:**
- To paladin: "Your fury ignites as [ally] falls! You gain +4 to hit and +8 to damage!"
- To room: "[paladin]'s eyes blaze with righteous fury!"

**Technical Details:**
- Checks all group members via `GROUP(victim)->members` iterator
- Calculates HP percentage: (current HP * 100) / max HP
- Triggers on <25% HP or death (HP <= 0)
- Won't trigger on self-damage
- Won't reapply if affect already active (natural cooldown)

---

## Perk Constants

All perks are defined in `src/structs.h`:

```c
#define PERK_PALADIN_DIVINE_MIGHT 908
#define PERK_PALADIN_EXORCISM_OF_THE_SLAIN 909
#define PERK_PALADIN_HOLY_SWORD 910
#define PERK_PALADIN_ZEALOUS_SMITE 911
#define PERK_PALADIN_BLINDING_SMITE 912
#define PERK_PALADIN_OVERWHELMING_SMITE 913
#define PERK_PALADIN_SACRED_VENGEANCE 914
```

## Skill Constants

New skill affects defined in `src/spells.h`:

```c
#define SKILL_DIVINE_MIGHT 2227
#define SKILL_SACRED_VENGEANCE 2228
#define NUM_SKILLS 2229  /* Updated from 2227 */
```

## Helper Functions

All perks have corresponding helper functions in `src/perks.c` and `src/perks.h`:

```c
bool has_paladin_divine_might(struct char_data *ch);
bool has_paladin_exorcism_of_the_slain(struct char_data *ch);
bool has_paladin_holy_sword(struct char_data *ch);
bool has_paladin_zealous_smite(struct char_data *ch);
bool has_paladin_blinding_smite(struct char_data *ch);
bool has_paladin_overwhelming_smite(struct char_data *ch);
bool has_paladin_sacred_vengeance(struct char_data *ch);
```

---

## Testing Checklist

### Divine Might
- [ ] Command accessible with swift action
- [ ] CHA bonus correctly applied to damage
- [ ] Duration lasts 10 rounds
- [ ] Cooldown prevents reuse for 5 minutes
- [ ] Wear-off message displays correctly

### Exorcism of the Slain
- [ ] +4d6 damage applied when smiting evil
- [ ] Damage bonus displays in combat log
- [ ] Only triggers with Smite Evil active
- [ ] Future: Test with proper demon/devil/outsider subtypes

### Holy Sword
- [ ] Holy Blade shows +4 enhancement (not +2)
- [ ] +2d6 holy damage applied vs evil
- [ ] Damage bonus displays separately
- [ ] Works only when Holy Blade is active

### Zealous Smite
- [ ] Smite use restored when killing with smite
- [ ] Message displays correctly
- [ ] Only triggers on evil creature deaths
- [ ] Cooldown tracked properly

### Blinding Smite
- [ ] Blindness applied on smite hits vs evil
- [ ] Will save DC calculated correctly
- [ ] 2 round duration
- [ ] Respects blind immunity
- [ ] Messages display correctly

### Overwhelming Smite
- [ ] Knockdown only on critical smite hits
- [ ] Fort save DC calculated correctly
- [ ] Respects knockdown immunity
- [ ] Messages display correctly
- [ ] Works with all knockdown mechanics

### Sacred Vengeance
- [ ] Triggers when ally drops below 25% HP
- [ ] Triggers when ally dies
- [ ] +4 hit and +8 damage applied
- [ ] 3 round duration
- [ ] Only affects paladins in same room
- [ ] Won't reapply if already active
- [ ] Wear-off message displays correctly

---

## Future Enhancements

### Exorcism of the Slain
- **TODO:** Implement proper demon/devil/outsider subrace detection
- Current implementation uses IS_EVIL() as a temporary measure
- When race system supports SUBRACE_DEMON, SUBRACE_DEVIL, SUBRACE_OUTSIDER:
  - Update condition in `src/fight.c` lines 6632-6665
  - Replace IS_EVIL(vict) check with proper subrace checks
  - Example: `(GET_SUBRACE(vict) == SUBRACE_DEMON || GET_SUBRACE(vict) == SUBRACE_DEVIL || ...)`

### Sacred Vengeance Cooldown
- Current implementation uses affect existence as natural cooldown
- Could add explicit 5-minute cooldown via event system if desired
- Current behavior: Won't reapply until the 3-round affect expires

---

## Compilation Status

✅ **Successfully Compiled**
- No errors
- No critical warnings
- All functions properly linked
- Binary generated: `/home/krynn/code/bin/circle`

---

## Files Modified Summary

1. **src/structs.h** - Added 7 perk constants (908-914)
2. **src/spells.h** - Added 2 skill constants (2227-2228), updated NUM_SKILLS
3. **src/perks.c** - Added 7 perk definitions and 7 helper functions
4. **src/perks.h** - Added 7 function declarations
5. **src/act.offensive.c** - Modified Holy Blade, added Divine Might command
6. **src/fight.c** - Added Holy Sword damage, Exorcism damage, Zealous Smite restore, Blinding Smite, Overwhelming Smite, Sacred Vengeance
7. **src/act.h** - Added Divine Might command declaration
8. **src/interpreter.c** - Registered divinemight and holyblade commands
9. **src/spell_parser.c** - Added spello() entries for Divine Might and Sacred Vengeance

---

## Integration Notes

### Combat Flow
1. **Pre-attack:** Divine Might can be activated (swift action)
2. **On hit:** Blinding Smite checks, Overwhelming Smite checks (if crit)
3. **Damage calculation:** Holy Sword bonus, Exorcism bonus added
4. **On kill:** Zealous Smite restores smite use
5. **Ally damage:** Sacred Vengeance triggers for nearby paladins

### Smite Evil Synergy
All Tier 3-4 perks synergize with Smite Evil:
- Divine Might adds CHA to damage during smite
- Exorcism adds +4d6 when smiting
- Holy Sword adds +2d6 holy damage when smiting evil
- Zealous Smite restores smite on kills
- Blinding Smite adds blind on smite hits
- Overwhelming Smite adds knockdown on crit smites
- Sacred Vengeance triggers when allies fall (often during tough fights where smites are used)

### Balance Considerations
- All perks require significant perk point investment (3-4 points each)
- Tier 3 perks require Tier 2 prerequisites
- Tier 4 perks require Tier 3 prerequisites
- Cooldowns prevent spam (Divine Might, Zealous Smite)
- Saving throws provide counterplay (Blinding Smite, Overwhelming Smite)
- Sacred Vengeance requires group play and has limited duration

---

## Author
Implementation completed for Knight of the Chalice paladin perk tree expansion.

**Date:** 2024  
**Version:** 1.0  
**Status:** Complete - Ready for testing
