# LuminariMUD Combat System

## Overview

LuminariMUD implements a sophisticated combat system inspired by Pathfinder/D&D 3.5/d20 rules. The system uses an **event-driven architecture** for combat rounds, multiple attack types, complex damage calculations, and tactical combat maneuvers. Combat features range from basic melee attacks to advanced techniques including grappling, combat modes, special abilities, and evolution-based attacks.

**Key Architecture Features:**
- Event-driven combat rounds (not pulse-based)
- Action-based system with standard/move/swift actions
- Condensed combat mode for streamlined output
- Multiple simultaneous combat tracking
- Comprehensive damage reduction and resistance systems

## Core Combat Architecture

### 1. Combat Initialization

Combat initialization is handled through the damage system. When damage is dealt between two characters, the system automatically establishes combat relationships:

```c
// From damage() function in fight.c
if (victim != ch) {
    /* Only auto engage if both parties are unengaged. */
    if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL) && 
        (FIGHTING(victim) == NULL) && !is_wall_spell(w_type))
        set_fighting(ch, victim);
    
    if (GET_POS(victim) > POS_STUNNED && (FIGHTING(victim) == NULL) && 
        !is_wall_spell(w_type))
        set_fighting(victim, ch);
    
    victim->last_attacker = ch;
}
```

**Note:** The `set_fighting()` function implementation details are encapsulated, but it manages:
- Combat list management (linked list of combatants)
- Initiative tracking
- Combat event scheduling
- Position updates

### 2. Initiative System

The initiative system determines combat order:

```c
// Function declaration from fight.h
int get_initiative_modifier(struct char_data *ch);
```

**Initiative Features:**
- d20 roll + modifiers (DEX bonus, feats, etc.)
- Combat list maintains initiative order
- Event-driven timing based on initiative
- Multiple combat phases for complex action resolution

**Known Modifiers:**
- Dexterity bonus
- Improved Initiative feat (+4)
- Class-specific bonuses
- Size modifiers
- Equipment effects

### 3. Combat Round System

Combat uses an event-driven system with multiple phases:

```c
// perform_violence() is the core combat execution function
void perform_violence(struct char_data *ch, int phase) {
    // Handle confused/feared states
    if (AFF_FLAGGED(ch, AFF_CONFUSED)) { /* confusion logic */ }
    if (AFF_FLAGGED(ch, AFF_FEAR)) { /* fear logic */ }
    
    // Group auto-assist logic
    if (GROUP(ch)) { /* auto-assist code */ }
    
    // Main combat execution
    if (!IS_CASTING(ch) && !AFF_FLAGGED(ch, AFF_TOTAL_DEFENSE) && 
        !(AFF_FLAGGED(ch, AFF_GRAPPLED) && /* grapple restrictions */)) {
        // Execute attack routine
        perform_attacks(ch, NORMAL_ATTACK_ROUTINE, phase);
        
        // Handle cleave attacks
        if (phase == 1 && HAS_FEAT(ch, FEAT_CLEAVE))
            handle_cleave(ch);
    }
}
```

**Combat Phases:**
- PHASE_0: Full round actions
- PHASE_1: Primary attacks
- PHASE_2: Off-hand attacks
- PHASE_3: Haste/extra attacks

## Attack System

### 1. Attack Types

The system supports numerous attack types:

```c
// Core attack types
#define ATTACK_TYPE_PRIMARY       0    // Primary hand weapon
#define ATTACK_TYPE_OFFHAND       1    // Off-hand weapon  
#define ATTACK_TYPE_RANGED        2    // Ranged weapon
#define ATTACK_TYPE_UNARMED       3    // Unarmed combat
#define ATTACK_TYPE_TWOHAND       4    // Two-handed weapon
#define ATTACK_TYPE_BOMB_TOSS     5    // Alchemist bombs
#define ATTACK_TYPE_PRIMARY_SNEAK 6    // Sneak attack primary
#define ATTACK_TYPE_OFFHAND_SNEAK 7    // Sneak attack off-hand

// Evolution-based attacks (Eidolon/Summoner system)
#define ATTACK_TYPE_PRIMARY_EVO_BITE     12
#define ATTACK_TYPE_PRIMARY_EVO_CLAWS    13
#define ATTACK_TYPE_PRIMARY_EVO_HOOVES   14
#define ATTACK_TYPE_PRIMARY_EVO_PINCERS  15
#define ATTACK_TYPE_PRIMARY_EVO_STING    16
#define ATTACK_TYPE_PRIMARY_EVO_TAIL_SLAP 17
#define ATTACK_TYPE_PRIMARY_EVO_TENTACLE 18
#define ATTACK_TYPE_PRIMARY_EVO_WING_BUFFET 19
#define ATTACK_TYPE_PRIMARY_EVO_GORE     20
#define ATTACK_TYPE_PRIMARY_EVO_RAKE     21
#define ATTACK_TYPE_PRIMARY_EVO_REND     22
#define ATTACK_TYPE_PRIMARY_EVO_TRAMPLE  23
```

### 2. Attack Resolution

Attack resolution is handled through multiple functions:

```c
// Main attack functions from fight.h
int attack_roll(struct char_data *ch, struct char_data *victim, 
                int attack_type, int is_touch, int attack_number);
int attack_roll_with_critical(struct char_data *ch, struct char_data *victim, 
                              int attack_type, int is_touch, int attack_number, 
                              int threat_range);
int compute_attack_bonus(struct char_data *ch, struct char_data *victim, 
                        int attack_type);
int compute_attack_bonus_full(struct char_data *ch, struct char_data *victim, 
                             int attack_type, bool display);
```

**Attack Resolution Process:**
1. Calculate attack bonus (BAB + modifiers)
2. Calculate target AC (base 10 + modifiers)
3. Roll d20 + attack bonus
4. Compare to AC for hit/miss
5. Check for critical threats and confirmation

### 3. Attack Bonus Calculation

Attack bonuses combine multiple factors (actual implementation in fight.c):

**Base Components:**
- Base Attack Bonus (BAB) from class levels
- Ability modifiers:
  - STR for melee attacks
  - DEX for ranged attacks
  - DEX for finesse/agile weapons
- Size modifiers

**Equipment Bonuses:**
- Weapon enhancement bonuses
- Magic weapon properties
- Agile weapon property (use DEX instead of STR)

**Feat Bonuses:**
- Weapon Focus (+1)
- Greater Weapon Focus (+1)
- Point Blank Shot (+1 ranged within same room)
- Various class-specific feats

**Situational Modifiers:**
- Flanking (+2)
- Dual wielding penalties:
  - Primary: -6 (or -4 with light weapon)
  - Off-hand: -10 (or -8 with light weapon)  
  - Reduced by Two-Weapon Fighting feats
- Combat modes (Power Attack, Combat Expertise)
- Conditions (prone, grappled, etc.)

### 4. Armor Class Calculation

Armor Class is calculated with multiple modes:

```c
// AC calculation modes from fight.h
#define MODE_ARMOR_CLASS_NORMAL 0
#define MODE_ARMOR_CLASS_COMBAT_MANEUVER_DEFENSE 1
#define MODE_ARMOR_CLASS_PENALTIES 2
#define MODE_ARMOR_CLASS_DISPLAY 3
#define MODE_ARMOR_CLASS_TOUCH 4

int compute_armor_class(struct char_data *attacker, struct char_data *ch, 
                       int is_touch, int mode);
```

**AC Components:**
- Base AC: 10
- Armor bonus (not vs touch)
- Shield bonus (not vs touch)
- Natural armor (not vs touch)
- Dexterity bonus (if not denied)
- Size modifiers
- Deflection bonus (always applies)
- Dodge bonus (lost when flat-footed)
- Enhancement bonuses
- Insight bonuses
- Sacred/Profane bonuses

**Special Conditions:**
- Flat-footed: Lose DEX and dodge bonuses
- Touch attacks: Ignore armor, shield, natural
- Combat Expertise mode: Trade attack for AC
- Fighting defensively: +2 AC, -4 attack
- Total defense: +4 AC, no attacks

## Damage System

### 1. Damage Calculation

Damage calculation is complex and involves multiple functions:

```c
int compute_hit_damage(struct char_data *ch, struct char_data *victim,
                      int w_type, int diceroll, int mode, 
                      bool is_critical, int attack_type, int dam_type);

int compute_damage_bonus(struct char_data *ch, struct char_data *vict,
                        struct obj_data *wielded, int w_type, int mod, 
                        int mode, int attack_type);
```

**Base Damage Components:**
- Weapon damage dice (e.g., 1d8 for longsword)
- Ability modifiers:
  - Full STR for primary
  - 1.5x STR for two-handed
  - 0.5x STR for off-hand
  - STR penalty (but not bonus) for ranged
  - Special: Composite bows add limited STR
- Enhancement bonuses
- Critical multipliers (x2, x3, x4)

**Bonus Damage Sources:**
- Damroll (from gear/spells)
- Feat bonuses (Weapon Specialization, etc.)
- Class features (Favored Enemy, Smite, etc.)
- Rage bonuses
- Sneak attack dice
- Elemental damage (flaming, frost, etc.)
- Evolution bonuses (Eidolon system)

### 2. Damage Processing (`damage()`)

The central damage function in fight.c handles all damage:

```c
int damage(struct char_data *ch, struct char_data *victim, int dam,
          int w_type, int dam_type, int offhand);
```

**Damage Processing Steps:**
1. **Validation**: Check valid targets, protected mobs, peaceful rooms
2. **Combat Initialization**: Auto-engage combatants if not fighting
3. **Damage Reduction**: Apply damage_handling() for resistances
4. **Apply Damage**: Subtract from hit points
5. **Update Status**: Call update_pos() for condition changes
6. **Death Handling**: Process death if HP <= 0

**Special Cases:**
- Self-damage (TYPE_SUFFERING)
- Pet protection (PRF_CAREFUL_PET)
- Immortal protection
- Arena combat
- Mission mob restrictions

### 3. Damage Reduction and Resistance

The damage_handling() function implements comprehensive mitigation:

**Avoidance Mechanics:**
- **Concealment** (20-50%, can exceed with Vanish)
  - Displacement (50%)
  - Blur/Blinking (20%)
  - Invisibility (50% if can't see)
  - Shadow Blend evolution
- **Racial Abilities**
  - Trelux dodge (25% chance)
  - Stalwart Defender (10% with Last Word)

**Damage Reduction:**
- **DR Types**: DR X/material (e.g., DR 10/magic)
- **Sources**: 
  - Class features (Barbarian, etc.)
  - Racial (Warforged, etc.)
  - Spells (Stoneskin)
  - Equipment (Adamantine armor)
- **Maximum**: 20 for players

**Energy Resistance/Immunity:**
- **Types**: Fire, Cold, Acid, Electric, Sonic
- **Resistance**: Flat reduction (e.g., Resist 10)
- **Percentage**: % based reduction
- **Immunity**: 100% reduction
- **Vulnerability**: Negative resistance (extra damage)

**Special Defenses:**
- Incorporeal (50% from non-magical)
- Defensive Roll (avoid death 1/day)
- Inertial Barrier (PSP absorption)
- Warding effects

## Combat Maneuvers

### 1. Grappling System (grapple.c)

The grappling system is fully implemented with detailed mechanics:

```c
// Core grapple functions
void set_grapple(struct char_data *ch, struct char_data *vict);
void clear_grapple(struct char_data *ch, struct char_data *vict);
bool valid_grapple_cond(struct char_data *ch);
void perform_grapple(struct char_data *ch, char *argument);
```

**Grapple Mechanics:**
- Both participants gain AFF_GRAPPLED
- Grappler has dominant position (GRAPPLE_TARGET)
- Victim is grappled (GRAPPLE_ATTACKER)
- Actions while grappled:
  - **Move**: Half speed with opponent
  - **Damage**: Unarmed/light weapon damage
  - **Pin**: Apply pinned condition
  - **Tie Up**: Bind with ropes (DC 20 + CMB)
  - **Escape**: Break free or reverse grapple

**Restrictions:**
- Need free hands (-4 without two free)
- Limited to light weapons or unarmed
- Provokes AoO without Improved Grapple

### 2. Other Combat Maneuvers

**Implemented Maneuvers:**
- **Trip** (act.offensive.c): Knock prone
- **Bash** (act.offensive.c): Shield bash
- **Charge** (act.offensive.c): +2 attack, -2 AC
- **Bull Rush**: Push opponent
- **Disarm**: Remove weapon
- **Sunder**: Damage equipment

**CMB/CMD System:**
- CMB = BAB + STR + size + misc
- CMD = 10 + BAB + STR + DEX + size + misc
- d20 + CMB vs CMD for success

### 2. Attacks of Opportunity

Attacks of opportunity provide battlefield control:

```c
int attack_of_opportunity(struct char_data *ch, struct char_data *victim,
                         int penalty);
void attacks_of_opportunity(struct char_data *victim, int penalty);
void teamwork_attacks_of_opportunity(struct char_data *victim, int penalty, 
                                   int featnum);
```

**AoO Limits:**
- Normal: 1 per round
- Combat Reflexes: 1 + DEX bonus per round
- Cannot make while flat-footed (unless Combat Reflexes)
- Cannot make while grappled/entangled

**Common AoO Triggers:**
- Movement out of threatened area
- Casting spells (unless defensive)
- Ranged attacks in melee
- Standing from prone
- Retrieving items
- Combat maneuvers (without Improved feats)
- Unarmed attacks (without Improved Unarmed)

**Teamwork AoOs:**
- Coordinated attacks with allies
- Requires shared teamwork feats
- Additional tactical options

## Special Combat Abilities

### 1. Critical Hits

Critical hit system with threat ranges and confirmation:

**Critical Mechanics:**
- **Threat Range**: Usually 20, improved by:
  - Keen weapons (doubles range)
  - Improved Critical feat (doubles range)
  - Weapon properties (18-20, 19-20)
- **Confirmation**: Second attack roll to confirm
- **Multipliers**: x2, x3, or x4 damage
- **Special Effects**:
  - Exploit Weakness (Alchemist)
  - Shadow Blind (Shadow Dancer)
  - Spell Critical (automatic spell proc)

**Weapon Critical Properties:**
- Longsword: 19-20/x2
- Rapier: 18-20/x2
- Greataxe: 20/x3
- Scythe: 20/x4

### 2. Sneak Attack

Sneak attack provides precision damage:

**Sneak Attack Conditions:**
- Target is flanked
- Target is flat-footed
- Target denied DEX bonus
- Attacker is invisible/hidden
- Target is blinded

**Damage Progression:**
- Rogue: +1d6 per 2 levels
- Ninja: Similar progression
- Arcane Trickster: Stacks with base
- Slayer: Limited progression

**Restrictions:**
- Must be within 30 feet (ranged)
- Target must have discernible anatomy
- Immune: Constructs, Oozes, some Undead
- Precision damage - not multiplied on crits

### 3. Special Attack Abilities

**Implemented Special Attacks:**

**Monk Abilities:**
- **Stunning Fist**: Save or stunned 1 round
- **Quivering Palm**: Death attack, Fort save
- **Flurry of Blows**: Extra attacks, -2 penalty

**Ranger/Arcane Archer:**
- **Death Arrow**: Instant death ranged attack
- **Imbued Arrows**: Spell-storing ammunition

**Rage Powers (Berserker):**
- **Surprise Accuracy**: Auto-hit next attack
- **Powerful Blow**: Extra damage
- **Come and Get Me**: Enemies get +4 to hit/damage

**Alchemist:**
- **Bomb Toss**: Splash damage attacks
- **Exploit Weakness**: Analyze for weak points

**Evolution Attacks (Eidolon):**
- Multiple natural attacks
- Special properties (grab, poison, bleed)
- Combination attacks (rend, rake)

## Combat States and Conditions

### 1. Position States

Character positions affect combat capabilities:

```c
// Position constants
#define POS_DEAD       0    // Character is dead
#define POS_MORTALLYW  1    // Mortally wounded (-10 to -6 hp)
#define POS_INCAP      2    // Incapacitated (-5 to -3 hp)
#define POS_STUNNED    3    // Stunned (-2 to -1 hp)
#define POS_SLEEPING   4    // Sleeping
#define POS_RESTING    5    // Resting
#define POS_SITTING    6    // Sitting
#define POS_FIGHTING   7    // Fighting
#define POS_STANDING   8    // Standing
```

### 2. Combat Conditions

**Major Combat Conditions:**

**Movement/Position:**
- **Prone**: -4 melee attack, +4 AC vs ranged, -4 AC vs melee
- **Grappled**: -2 attack/AC, no move, limited actions
- **Pinned**: More severe than grappled, nearly helpless
- **Entangled**: -2 attack, -4 DEX, half speed

**Awareness:**
- **Flat-footed**: No DEX to AC, no AoOs, vulnerable to sneak
- **Blinded**: -2 AC, lose DEX to AC, 50% miss in melee
- **Invisible**: +2 attack, deny opponent DEX

**Mental States:**
- **Stunned**: Drop items, -2 AC, lose DEX, no actions
- **Confused**: Random actions, may attack allies
- **Fear**: Shaken (-2), Frightened (flee), Panicked (drop items)
- **Paralyzed**: Helpless, coup de grace possible

**Status Effects:**
- **Sickened**: -2 attack/damage/saves/skills
- **Nauseated**: Move action only
- **Exhausted**: -6 STR/DEX, half speed
- **Fatigued**: -2 STR/DEX, no run/charge

## Combat Modes (combat_modes.c)

### Mode Groups

Combat modes are organized into exclusive groups:

**Group 1 (Modify Attacks):**
- **Power Attack**: Trade attack for damage
- **Combat Expertise**: Trade attack for AC
- **Spellbattle**: Caster combat mode
- **Total Defense**: +4 AC, no attacks

**Group 2 (Add Attacks):**
- **Dual Wield**: Use two weapons
- **Flurry of Blows**: Monk rapid attacks
- **Rapid Shot**: Extra ranged attack

**Group 3 (Modify Casting):**
- **Counterspell**: Ready to counter
- **Defensive Casting**: Avoid AoOs

**No Group:**
- **Whirlwind Attack**: Attack all adjacent
- **Deadly Aim**: Ranged power attack

## Performance Features

### 1. Condensed Combat Mode

Streamlines combat output for reduced spam:
- Accumulates attack/damage info
- Single-line combat summaries
- Configurable via PRF_CONDENSED
- Tracked via CNDNSD() structure

### 2. Combat Optimizations

- **Event-driven**: No pulse-based overhead
- **Linked list**: Efficient combat_list management  
- **Cached values**: Reuse calculations
- **Phase system**: Distributed action resolution
- **Lazy evaluation**: On-demand computation

### 3. Balance Mechanisms

**Action Economy:**
- Standard/Move/Swift action system
- Attack progression limits
- Daily ability uses
- Cooldown timers

**Resource Management:**
- Rage rounds
- Ki points  
- Spell slots
- Power points (psionics)

**Risk vs Reward:**
- Power Attack: -attack/+damage
- Reckless Attack: enemies +4 to hit
- Charge: +2 attack/-2 AC
- Total Defense: No attacks allowed

This combat system provides deep tactical gameplay while maintaining performance suitable for real-time MUD combat with potentially hundreds of simultaneous battles.
