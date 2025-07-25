# LuminariMUD Combat System

## Overview

LuminariMUD implements a sophisticated combat system based on Pathfinder/D&D 3.5 rules, featuring turn-based combat with initiative, multiple attack types, complex damage calculations, and tactical combat maneuvers. The system handles everything from basic melee attacks to advanced combat techniques and special abilities.

## Core Combat Architecture

### 1. Combat Initialization (`set_fighting()`)

Combat begins when characters engage each other:

```c
bool set_fighting(struct char_data *ch, struct char_data *vict) {
    // Prevent self-targeting and duplicate combat states
    if (ch == vict || FIGHTING(ch)) return FALSE;
    
    // Prevent multiple combat events
    if (char_has_mud_event(ch, eCOMBAT_ROUND)) return FALSE;
    
    // Roll initiative for combat order
    GET_INITIATIVE(ch) = roll_initiative(ch);
    
    // Add to combat list with initiative-based ordering
    if (combat_list == NULL) {
        ch->next_fighting = combat_list;
        combat_list = ch;
    } else {
        // Insert based on initiative order
        struct char_data *current = combat_list;
        struct char_data *previous = NULL;
        
        while (current && GET_INITIATIVE(current) >= GET_INITIATIVE(ch)) {
            previous = current;
            current = current->next_fighting;
        }
        
        if (previous == NULL) {
            ch->next_fighting = combat_list;
            combat_list = ch;
        } else {
            ch->next_fighting = current;
            previous->next_fighting = ch;
        }
    }
    
    // Set fighting relationship
    FIGHTING(ch) = vict;
    GET_POS(ch) = POS_FIGHTING;
    
    // Start combat round event system
    int delay = (10 - GET_INITIATIVE(ch)) * PULSE_VIOLENCE / 10;
    attach_mud_event(new_mud_event(eCOMBAT_ROUND, ch, strdup("1")), delay);
    
    return TRUE;
}
```

### 2. Initiative System

Initiative determines combat order and action timing:

```c
int roll_initiative(struct char_data *ch) {
    int initiative = dice(1, 20) + get_initiative_modifier(ch);
    return MAX(1, MIN(initiative, 20));
}

int get_initiative_modifier(struct char_data *ch) {
    int modifier = GET_DEX_BONUS(ch);
    
    // Feat bonuses
    if (HAS_FEAT(ch, FEAT_IMPROVED_INITIATIVE))
        modifier += 4;
    
    // Class bonuses
    if (GET_CLASS(ch) == CLASS_RANGER)
        modifier += 2;
    
    // Size modifiers
    modifier += size_modifiers[GET_SIZE(ch)].initiative;
    
    return modifier;
}
```

**Initiative Order:**
- Higher initiative acts first
- Ties broken by dexterity modifier
- Initiative determines delay before first action
- Combat rounds proceed in initiative order

### 3. Combat Round System

Combat uses an event-driven round system:

```c
// Combat round event handler
EVENTFUNC(event_combat_round) {
    struct char_data *ch = (struct char_data *)event_obj;
    int phase = atoi((char *)info);
    
    if (!ch || !FIGHTING(ch)) {
        return 0; // End combat
    }
    
    // Perform character's actions for this phase
    perform_violence(ch, phase);
    
    // Schedule next combat round
    attach_mud_event(new_mud_event(eCOMBAT_ROUND, ch, strdup("1")), 
                     PULSE_VIOLENCE);
    
    return 0;
}

void perform_violence(struct char_data *ch, int phase) {
    // Check if character can act
    if (!valid_fight_cond(ch, FALSE)) return;
    
    // Perform attacks based on character's attack routine
    perform_attacks(ch, MODE_NORMAL_HIT, phase);
    
    // Handle special abilities and reactions
    process_combat_specials(ch);
}
```

## Attack System

### 1. Attack Types

The system supports multiple attack types:

```c
// Attack type constants
#define ATTACK_TYPE_PRIMARY       0    // Primary hand weapon
#define ATTACK_TYPE_OFFHAND       1    // Off-hand weapon  
#define ATTACK_TYPE_RANGED        2    // Ranged weapon
#define ATTACK_TYPE_UNARMED       3    // Unarmed combat
#define ATTACK_TYPE_TWOHAND       4    // Two-handed weapon
#define ATTACK_TYPE_BOMB_TOSS     5    // Alchemist bombs
#define ATTACK_TYPE_PRIMARY_SNEAK 6    // Sneak attack primary
#define ATTACK_TYPE_OFFHAND_SNEAK 7    // Sneak attack off-hand
```

### 2. Attack Resolution (`attack_roll()`)

Attack success is determined by comparing attack roll to armor class:

```c
int attack_roll(struct char_data *ch, struct char_data *victim, 
                int attack_type, int is_touch, int attack_number) {
    
    // Calculate attack bonus
    int attack_bonus = compute_attack_bonus(ch, victim, attack_type);
    
    // Calculate target's armor class
    int victim_ac = compute_armor_class(ch, victim, is_touch, 
                                       MODE_ARMOR_CLASS_NORMAL);
    
    // Roll d20 and apply modifiers
    int diceroll = d20(ch);
    int result = (attack_bonus + diceroll) - victim_ac;
    
    return result; // Positive = hit, negative = miss
}
```

### 3. Attack Bonus Calculation

Attack bonuses combine multiple factors:

```c
int compute_attack_bonus(struct char_data *ch, struct char_data *victim, 
                        int attack_type) {
    int bonus = 0;
    
    // Base attack bonus from class levels
    bonus += GET_BAB(ch);
    
    // Ability modifier (STR for melee, DEX for ranged)
    if (attack_type == ATTACK_TYPE_RANGED) {
        bonus += GET_DEX_BONUS(ch);
    } else {
        bonus += GET_STR_BONUS(ch);
    }
    
    // Size modifier
    bonus += size_modifiers[GET_SIZE(ch)].attack;
    
    // Weapon enhancement bonus
    struct obj_data *weapon = get_wielded(ch, attack_type);
    if (weapon) {
        bonus += GET_OBJ_VAL(weapon, 4); // Enhancement bonus
    }
    
    // Feat bonuses
    if (HAS_FEAT(ch, FEAT_WEAPON_FOCUS) && weapon_focus_applies(ch, weapon))
        bonus += 1;
    if (HAS_FEAT(ch, FEAT_GREATER_WEAPON_FOCUS) && weapon_focus_applies(ch, weapon))
        bonus += 1;
    
    // Situational modifiers
    if (is_flanked(ch, victim))
        bonus += 2; // Flanking bonus
    
    // Dual wielding penalties
    if (is_dual_wielding(ch)) {
        bonus += dual_wielding_penalty(ch, 
                 (attack_type == ATTACK_TYPE_OFFHAND));
    }
    
    return bonus;
}
```

### 4. Armor Class Calculation

Armor Class determines how difficult a character is to hit:

```c
int compute_armor_class(struct char_data *attacker, struct char_data *ch, 
                       int is_touch, int mode) {
    int ac = 10; // Base AC
    
    if (!is_touch) {
        // Armor bonus
        ac += GET_AC_ARMOR(ch);
        
        // Shield bonus  
        ac += GET_AC_SHIELD(ch);
        
        // Natural armor
        ac += GET_AC_NATURAL(ch);
    }
    
    // Dexterity bonus (if applicable)
    if (has_dex_bonus_to_ac(attacker, ch)) {
        ac += GET_DEX_BONUS(ch);
    }
    
    // Size modifier
    ac += size_modifiers[GET_SIZE(ch)].ac;
    
    // Deflection bonus (always applies)
    ac += GET_AC_DEFLECTION(ch);
    
    // Dodge bonus (if not flat-footed)
    if (!AFF_FLAGGED(ch, AFF_FLAT_FOOTED)) {
        ac += GET_AC_DODGE(ch);
    }
    
    // Enhancement bonuses
    ac += GET_AC_ENHANCEMENT(ch);
    
    return ac;
}
```

## Damage System

### 1. Damage Calculation (`compute_hit_damage()`)

Damage combines weapon damage, ability modifiers, and various bonuses:

```c
int compute_hit_damage(struct char_data *ch, struct char_data *victim,
                      int w_type, int diceroll, int mode, 
                      bool is_critical, int attack_type, int dam_type) {
    int dam = 0;
    struct obj_data *weapon = get_wielded(ch, attack_type);
    
    // Base weapon damage
    if (weapon) {
        int num_dice = GET_OBJ_VAL(weapon, 1);
        int die_size = GET_OBJ_VAL(weapon, 2);
        dam = dice(num_dice, die_size);
        
        // Critical hit multiplier
        if (is_critical) {
            int crit_mult = GET_OBJ_VAL(weapon, 3);
            dam *= crit_mult;
        }
    } else {
        // Unarmed damage
        int dice_one, dice_two;
        compute_barehand_dam_dice(ch, &dice_one, &dice_two);
        dam = dice(dice_one, dice_two);
    }
    
    // Ability modifier
    int ability_mod = (attack_type == ATTACK_TYPE_RANGED) ? 
                      GET_DEX_BONUS(ch) : GET_STR_BONUS(ch);
    
    // Two-handed weapons get 1.5x STR bonus
    if (attack_type == ATTACK_TYPE_TWOHAND) {
        ability_mod = (GET_STR_BONUS(ch) * 3) / 2;
    }
    // Off-hand weapons get 0.5x STR bonus
    else if (attack_type == ATTACK_TYPE_OFFHAND) {
        ability_mod = GET_STR_BONUS(ch) / 2;
    }
    
    dam += ability_mod;
    
    // Enhancement bonus
    if (weapon) {
        dam += GET_OBJ_VAL(weapon, 4);
    }
    
    // Feat bonuses
    dam += compute_damage_bonus(ch, victim, weapon, attack_type, 
                               ability_mod, mode, attack_type);
    
    return MAX(1, dam); // Minimum 1 damage
}
```

### 2. Damage Processing (`damage()`)

All damage goes through the central damage function:

```c
int damage(struct char_data *ch, struct char_data *victim, int dam,
          int w_type, int dam_type, int offhand) {

    // Validate parameters
    if (!ch || !victim || dam < 0) return 0;

    // Process damage reductions and resistances
    dam = damage_handling(ch, victim, dam, w_type, dam_type);
    if (dam <= 0) return 0;

    // Apply damage to hit points
    GET_HIT(victim) -= dam;

    // Update position based on hit points
    update_pos(victim);

    // Handle death
    if (GET_POS(victim) == POS_DEAD) {
        if (ch != victim) {
            die(victim, ch);
        } else {
            die(victim, NULL);
        }
        return -1; // Victim died
    }

    // Trigger combat if not already fighting
    if (!FIGHTING(victim) && ch != victim) {
        set_fighting(victim, ch);
    }

    return dam;
}
```

### 3. Damage Reduction and Resistance

The system implements comprehensive damage mitigation:

```c
int damage_handling(struct char_data *ch, struct char_data *victim,
                   int dam, int attacktype, int dam_type) {

    // Damage reduction (DR)
    int dr_reduction = apply_damage_reduction(ch, victim,
                                            get_wielded(ch, ATTACK_TYPE_PRIMARY),
                                            dam, FALSE);
    dam -= dr_reduction;

    // Energy resistance
    int resistance = compute_damtype_reduction(victim, dam_type);
    dam -= resistance;

    // Energy absorption (converts damage to healing)
    int absorption = compute_energy_absorb(victim, dam_type);
    if (absorption > 0) {
        GET_HIT(victim) += absorption;
        dam = 0;
    }

    // Defensive abilities
    if (HAS_FEAT(victim, FEAT_DEFENSIVE_ROLL) &&
        (GET_HIT(victim) - dam) <= 0 &&
        !char_has_mud_event(victim, eD_ROLL)) {
        // Defensive roll avoids lethal damage
        attach_mud_event(new_mud_event(eD_ROLL, victim, NULL),
                        (2 * SECS_PER_MUD_DAY));
        return 0;
    }

    return MAX(0, dam);
}
```

## Combat Maneuvers

### 1. Combat Maneuver System

Combat maneuvers provide tactical options beyond basic attacks:

```c
int combat_maneuver_check(struct char_data *ch, struct char_data *vict,
                         int combat_maneuver_type, int attacker_bonus) {

    int attack_roll = d20(ch);
    int cm_bonus = attacker_bonus;
    int cm_defense = 0;

    // Calculate Combat Maneuver Bonus (CMB)
    cm_bonus += compute_cmb(ch, combat_maneuver_type);

    // Calculate Combat Maneuver Defense (CMD)
    cm_defense = compute_cmd(vict, combat_maneuver_type);

    // Apply situational modifiers
    switch (combat_maneuver_type) {
        case COMBAT_MANEUVER_TYPE_REVERSAL:
            cm_defense += 5; // Grappler gets bonus to maintain
            break;
        case COMBAT_MANEUVER_TYPE_BULL_RUSH:
            if (GET_SIZE(ch) > GET_SIZE(vict))
                cm_bonus += (GET_SIZE(ch) - GET_SIZE(vict)) * 4;
            break;
        case COMBAT_MANEUVER_TYPE_TRIP:
            if (GET_SIZE(ch) > GET_SIZE(vict))
                cm_bonus += (GET_SIZE(ch) - GET_SIZE(vict)) * 2;
            break;
    }

    int result = cm_bonus - cm_defense;

    // Natural 20 is always success, natural 1 is always failure
    if (attack_roll == 20) {
        return (result > 1) ? result : 1;
    } else if (attack_roll == 1) {
        return (result < 0) ? result : 0;
    }

    return result;
}
```

**Combat Maneuver Types:**
- **Bull Rush:** Push opponent backward
- **Disarm:** Remove opponent's weapon
- **Grapple:** Grab and hold opponent
- **Overrun:** Move through opponent's space
- **Sunder:** Destroy opponent's equipment
- **Trip:** Knock opponent prone

### 2. Attacks of Opportunity

The system implements attacks of opportunity for tactical positioning:

```c
int attack_of_opportunity(struct char_data *ch, struct char_data *victim,
                         int penalty) {

    // Check if character can make AoO
    if (AFF_FLAGGED(ch, AFF_FLAT_FOOTED) &&
        !HAS_FEAT(ch, FEAT_COMBAT_REFLEXES))
        return 0;

    if (AFF_FLAGGED(ch, AFF_GRAPPLED) || AFF_FLAGGED(ch, AFF_ENTANGLED))
        return 0;

    // Check AoO limit
    int max_aoo = HAS_FEAT(ch, FEAT_COMBAT_REFLEXES) ?
                  GET_DEX_BONUS(ch) : 1;

    if (GET_TOTAL_AOO(ch) < max_aoo) {
        GET_TOTAL_AOO(ch)++;
        return hit(ch, victim, TYPE_ATTACK_OF_OPPORTUNITY,
                  DAM_RESERVED_DBC, penalty, FALSE);
    }

    return 0;
}
```

**AoO Triggers:**
- Moving out of threatened squares
- Casting spells in combat
- Using ranged weapons in melee
- Standing up from prone
- Retrieving dropped items

## Special Combat Abilities

### 1. Critical Hits

Critical hits deal extra damage and may have special effects:

```c
bool is_critical_hit(struct char_data *ch, struct obj_data *weapon,
                    int diceroll, int attack_bonus, int victim_ac) {

    int threat_range = weapon ? GET_OBJ_VAL(weapon, 3) : 20;

    // Check if roll threatens critical
    if (diceroll < threat_range) return FALSE;

    // Confirm critical hit with second attack roll
    int confirm_roll = d20(ch) + attack_bonus;
    return (confirm_roll >= victim_ac);
}
```

### 2. Sneak Attack

Sneak attacks deal extra damage when conditions are met:

```c
int calculate_sneak_attack_damage(struct char_data *ch, struct char_data *victim) {

    if (!HAS_FEAT(ch, FEAT_SNEAK_ATTACK)) return 0;

    // Check sneak attack conditions
    if (!is_flanked(ch, victim) &&
        !AFF_FLAGGED(victim, AFF_FLAT_FOOTED) &&
        has_dex_bonus_to_ac(ch, victim)) {
        return 0;
    }

    // Calculate sneak attack dice
    int sneak_dice = HAS_FEAT(ch, FEAT_SNEAK_ATTACK);
    return dice(sneak_dice, 6);
}
```

### 3. Spell Combat Integration

The combat system integrates with the spell system:

```c
void cast_spell_in_combat(struct char_data *ch, struct char_data *victim,
                         int spellnum) {

    // Trigger attacks of opportunity
    attacks_of_opportunity(ch, 0);

    // Concentration check if damaged
    if (GET_HIT(ch) < GET_MAX_HIT(ch)) {
        int dc = 10 + spell_info[spellnum].min_level[GET_CLASS(ch)];
        if (!concentration_check(ch, dc)) {
            send_to_char(ch, "You lose concentration!\r\n");
            return;
        }
    }

    // Cast the spell
    call_magic(ch, victim, NULL, spellnum,
              GET_LEVEL(ch), CAST_SPELL);
}
```

## Combat States and Conditions

### 1. Position States

Character positions affect combat capabilities:

```c
// Position constants
#define POS_DEAD       0    // Character is dead
#define POS_MORTALLYW  1    // Mortally wounded
#define POS_INCAP      2    // Incapacitated
#define POS_STUNNED    3    // Stunned
#define POS_SLEEPING   4    // Sleeping
#define POS_RESTING    5    // Resting
#define POS_SITTING    6    // Sitting
#define POS_FIGHTING   7    // Fighting
#define POS_STANDING   8    // Standing

void update_pos(struct char_data *victim) {
    if (GET_HIT(victim) > 0) {
        GET_POS(victim) = POS_STANDING;
    } else if (GET_HIT(victim) <= -11) {
        GET_POS(victim) = POS_DEAD;
    } else if (GET_HIT(victim) <= -6) {
        GET_POS(victim) = POS_MORTALLYW;
    } else if (GET_HIT(victim) <= -3) {
        GET_POS(victim) = POS_INCAP;
    } else {
        GET_POS(victim) = POS_STUNNED;
    }
}
```

### 2. Combat Conditions

Various conditions affect combat performance:

**Condition Effects:**
- **Flat-footed:** Lose dex bonus to AC, can't make AoOs
- **Flanked:** -2 AC against flanking attackers
- **Grappled:** Limited actions, -4 to most rolls
- **Prone:** -4 to melee attacks, +4 AC vs ranged
- **Stunned:** Lose dex bonus, can't act
- **Entangled:** -2 attack, -4 dex, move at half speed

## Performance and Balance

### 1. Combat Optimization

The combat system includes several optimizations:

- **Event-driven rounds:** Reduces CPU overhead
- **Cached calculations:** Attack bonuses computed once per round
- **Efficient data structures:** Combat list for active participants
- **Lazy evaluation:** Complex calculations only when needed

### 2. Balance Mechanisms

- **Action economy:** Limits on attacks per round
- **Resource management:** Spell slots, special abilities
- **Tactical positioning:** Flanking, attacks of opportunity
- **Risk/reward:** High-damage abilities with drawbacks

This combat system provides deep tactical gameplay while maintaining the fast-paced action expected in a MUD environment, with careful attention to both mechanical complexity and performance optimization.
```
