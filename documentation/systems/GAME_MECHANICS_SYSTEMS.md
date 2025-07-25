# LuminariMUD Game Mechanics Systems

## Overview

LuminariMUD implements a comprehensive Pathfinder/D&D 3.5-based game mechanics system featuring character classes, races, skills, feats, spells, and combat mechanics. The system provides authentic tabletop RPG mechanics adapted for the MUD environment with extensive customization and progression options.

## Character Creation System

### Race System

#### Core Races
```c
// Race definitions in race.c
struct race_data {
  char *name;                    // Race name
  char *abbrev;                  // Abbreviation
  int ability_mods[NUM_ABILITIES]; // Ability score modifiers
  int size;                      // Character size
  int favored_class;             // Favored class
  bitvector_t racial_traits;     // Special racial abilities
  int languages[MAX_LANGUAGES];  // Known languages
};

// Example: Human race
race_list[RACE_HUMAN] = {
  .name = "Human",
  .abbrev = "Hum",
  .ability_mods = {0, 0, 0, 0, 0, 0}, // No racial modifiers
  .size = SIZE_MEDIUM,
  .favored_class = CLASS_UNDEFINED,    // Any class
  .racial_traits = RACIAL_EXTRA_FEAT | RACIAL_EXTRA_SKILL_POINTS,
  .languages = {LANG_COMMON, -1}
};
```

#### Racial Abilities Implementation
```c
// Apply racial bonuses during character creation
void apply_racial_bonuses(struct char_data *ch) {
  int race = GET_RACE(ch);
  
  // Apply ability score modifiers
  for (int i = 0; i < NUM_ABILITIES; i++) {
    GET_REAL_ABILITY(ch, i) += race_list[race].ability_mods[i];
  }
  
  // Apply special racial traits
  if (IS_SET(race_list[race].racial_traits, RACIAL_DARKVISION)) {
    SET_BIT(AFF_FLAGS(ch), AFF_INFRAVISION);
  }
  
  if (IS_SET(race_list[race].racial_traits, RACIAL_EXTRA_FEAT)) {
    GET_FEAT_POINTS(ch) += 1;
  }
  
  // Set racial languages
  for (int i = 0; race_list[race].languages[i] != -1; i++) {
    SET_BIT(GET_LANGUAGES(ch), race_list[race].languages[i]);
  }
}
```

### Class System

#### Class Structure
```c
struct class_data {
  char *name;                    // Class name
  char *abbrev;                  // Abbreviation
  int hit_die;                   // Hit die size (d6, d8, d10, d12)
  int skill_points;              // Skill points per level
  int bab_type;                  // Base attack bonus progression
  int fort_save;                 // Fortitude save progression
  int ref_save;                  // Reflex save progression
  int will_save;                 // Will save progression
  int spellcaster_type;          // Spellcasting ability
  int spell_stat;                // Primary spellcasting ability
  bitvector_t class_feats;       // Automatic class feats
  int class_skills[MAX_SKILLS];  // Class skill list
};

// Example: Fighter class
class_list[CLASS_FIGHTER] = {
  .name = "Fighter",
  .abbrev = "Fig",
  .hit_die = 10,
  .skill_points = 2,
  .bab_type = BAB_HIGH,          // Full BAB progression
  .fort_save = SAVE_HIGH,        // Good fortitude save
  .ref_save = SAVE_LOW,          // Poor reflex save
  .will_save = SAVE_LOW,         // Poor will save
  .spellcaster_type = SPELL_TYPE_NONE,
  .spell_stat = ABILITY_NONE,
  .class_feats = CFEAT_WEAPON_PROFICIENCY_MARTIAL,
  .class_skills = {SKILL_CLIMB, SKILL_INTIMIDATE, SKILL_JUMP, -1}
};
```

#### Level Advancement
```c
void advance_level(struct char_data *ch) {
  int class = GET_CLASS(ch);
  int new_level = GET_LEVEL(ch) + 1;
  int hp_gain, skill_points;
  
  // Calculate hit point gain
  hp_gain = dice(1, class_list[class].hit_die);
  if (GET_CON_BONUS(ch) > 0) {
    hp_gain += GET_CON_BONUS(ch);
  }
  
  // Minimum 1 HP per level
  if (hp_gain < 1) hp_gain = 1;
  
  GET_MAX_HIT(ch) += hp_gain;
  GET_HIT(ch) += hp_gain;
  
  // Calculate skill points
  skill_points = class_list[class].skill_points + GET_INT_BONUS(ch);
  if (skill_points < 1) skill_points = 1;
  
  // Humans get extra skill point
  if (GET_RACE(ch) == RACE_HUMAN) {
    skill_points++;
  }
  
  GET_SKILL_POINTS(ch) += skill_points;
  
  // Update saves and BAB
  update_char_saves(ch);
  update_char_bab(ch);
  
  // Grant class features
  grant_class_features(ch, new_level);
  
  GET_LEVEL(ch) = new_level;
  GET_EXP(ch) = level_exp(new_level);
  
  send_to_char(ch, "You have advanced to level %d!\r\n", new_level);
  send_to_char(ch, "You gain %d hit points and %d skill points.\r\n", 
               hp_gain, skill_points);
}
```

## Skill System

### Skill Structure
```c
struct skill_data {
  char *name;                    // Skill name
  int ability;                   // Key ability
  bool trainable;                // Can be trained by anyone
  bool armor_check;              // Subject to armor check penalty
  char *description;             // Skill description
};

// Example skills
skill_list[SKILL_CLIMB] = {
  .name = "Climb",
  .ability = ABILITY_STR,
  .trainable = TRUE,
  .armor_check = TRUE,
  .description = "Scale walls and cliffs"
};

skill_list[SKILL_SPELLCRAFT] = {
  .name = "Spellcraft",
  .ability = ABILITY_INT,
  .trainable = FALSE,            // Class skill only
  .armor_check = FALSE,
  .description = "Identify and understand magical effects"
};
```

### Skill Checks
```c
int skill_check(struct char_data *ch, int skill, int difficulty) {
  int roll, total, ranks, ability_mod, misc_mod = 0;
  
  // Base d20 roll
  roll = dice(1, 20);
  
  // Get skill ranks
  ranks = GET_SKILL(ch, skill);
  
  // Get ability modifier
  ability_mod = GET_ABILITY_MOD(ch, skill_list[skill].ability);
  
  // Check for armor check penalty
  if (skill_list[skill].armor_check) {
    misc_mod += GET_ARMOR_CHECK_PENALTY(ch);
  }
  
  // Calculate total
  total = roll + ranks + ability_mod + misc_mod;
  
  // Check for natural 1 or 20
  if (roll == 1) {
    return SKILL_CHECK_CRITICAL_FAILURE;
  } else if (roll == 20) {
    return SKILL_CHECK_CRITICAL_SUCCESS;
  }
  
  // Compare to difficulty
  if (total >= difficulty) {
    return SKILL_CHECK_SUCCESS;
  } else {
    return SKILL_CHECK_FAILURE;
  }
}
```

### Skill Training
```c
ACMD(do_practice) {
  struct char_data *trainer = NULL;
  int skill, cost;
  char arg[MAX_INPUT_LENGTH];
  
  one_argument(argument, arg);
  
  if (!*arg) {
    list_skills(ch);
    return;
  }
  
  // Find trainer in room
  for (trainer = world[IN_ROOM(ch)].people; trainer; trainer = trainer->next_in_room) {
    if (IS_NPC(trainer) && MOB_FLAGGED(trainer, MOB_TRAINER)) {
      break;
    }
  }
  
  if (!trainer) {
    send_to_char(ch, "There is no trainer here.\r\n");
    return;
  }
  
  skill = find_skill_num(arg);
  if (skill < 1) {
    send_to_char(ch, "That is not a valid skill.\r\n");
    return;
  }
  
  // Check if skill can be trained
  if (!can_train_skill(ch, skill)) {
    send_to_char(ch, "You cannot train that skill.\r\n");
    return;
  }
  
  // Calculate training cost
  cost = skill_training_cost(ch, skill);
  
  if (GET_SKILL_POINTS(ch) < cost) {
    send_to_char(ch, "You don't have enough skill points.\r\n");
    return;
  }
  
  // Train the skill
  GET_SKILL(ch, skill)++;
  GET_SKILL_POINTS(ch) -= cost;
  
  send_to_char(ch, "You train your %s skill.\r\n", skill_list[skill].name);
  act("$n practices $s skills with the trainer.", TRUE, ch, 0, 0, TO_ROOM);
}
```

## Feat System

### Feat Structure
```c
struct feat_data {
  char *name;                    // Feat name
  bool in_game;                  // Available in game
  bool can_learn;                // Can be learned normally
  bool can_stack;                // Can be taken multiple times
  char *prerequisites;           // Prerequisite description
  char *description;             // Feat description
  int feat_type;                 // Combat, metamagic, etc.
};

// Example feats
feat_list[FEAT_POWER_ATTACK] = {
  .name = "Power Attack",
  .in_game = TRUE,
  .can_learn = TRUE,
  .can_stack = FALSE,
  .prerequisites = "Str 13, base attack bonus +1",
  .description = "Trade attack bonus for damage bonus",
  .feat_type = FEAT_TYPE_COMBAT
};

feat_list[FEAT_WEAPON_FOCUS] = {
  .name = "Weapon Focus",
  .in_game = TRUE,
  .can_learn = TRUE,
  .can_stack = TRUE,             // Can take for different weapons
  .prerequisites = "Proficiency with weapon, base attack bonus +1",
  .description = "+1 attack bonus with chosen weapon",
  .feat_type = FEAT_TYPE_COMBAT
};
```

### Feat Prerequisites
```c
bool meets_feat_prereqs(struct char_data *ch, int feat) {
  switch (feat) {
    case FEAT_POWER_ATTACK:
      if (GET_STR(ch) < 13) return FALSE;
      if (GET_BAB(ch) < 1) return FALSE;
      break;
      
    case FEAT_WEAPON_FOCUS:
      if (GET_BAB(ch) < 1) return FALSE;
      // Additional weapon proficiency check would go here
      break;
      
    case FEAT_COMBAT_EXPERTISE:
      if (GET_INT(ch) < 13) return FALSE;
      break;
      
    case FEAT_WHIRLWIND_ATTACK:
      if (!HAS_FEAT(ch, FEAT_COMBAT_EXPERTISE)) return FALSE;
      if (!HAS_FEAT(ch, FEAT_DODGE)) return FALSE;
      if (!HAS_FEAT(ch, FEAT_MOBILITY)) return FALSE;
      if (!HAS_FEAT(ch, FEAT_SPRING_ATTACK)) return FALSE;
      if (GET_DEX(ch) < 13) return FALSE;
      if (GET_INT(ch) < 13) return FALSE;
      if (GET_BAB(ch) < 4) return FALSE;
      break;
  }
  
  return TRUE;
}
```

### Feat Effects
```c
// Apply feat bonuses during combat
int apply_feat_bonuses(struct char_data *ch, int bonus_type) {
  int bonus = 0;
  
  switch (bonus_type) {
    case BONUS_TYPE_ATTACK:
      if (HAS_FEAT(ch, FEAT_WEAPON_FOCUS)) {
        // Check if using focused weapon
        struct obj_data *weapon = GET_EQ(ch, WEAR_WIELD);
        if (weapon && is_focused_weapon(ch, weapon)) {
          bonus += 1;
        }
      }
      
      if (HAS_FEAT(ch, FEAT_WEAPON_SPECIALIZATION)) {
        struct obj_data *weapon = GET_EQ(ch, WEAR_WIELD);
        if (weapon && is_specialized_weapon(ch, weapon)) {
          bonus += 2; // Weapon Specialization gives damage, not attack
        }
      }
      break;
      
    case BONUS_TYPE_DAMAGE:
      if (HAS_FEAT(ch, FEAT_WEAPON_SPECIALIZATION)) {
        struct obj_data *weapon = GET_EQ(ch, WEAR_WIELD);
        if (weapon && is_specialized_weapon(ch, weapon)) {
          bonus += 2;
        }
      }
      break;
      
    case BONUS_TYPE_AC:
      if (HAS_FEAT(ch, FEAT_DODGE)) {
        bonus += 1;
      }
      break;
  }
  
  return bonus;
}
```

## Spell System

### Spell Structure
```c
struct spell_info_type {
  byte min_level[NUM_CLASSES];   // Minimum level for each class
  int mana_min;                  // Minimum mana cost
  int mana_max;                  // Maximum mana cost
  int mana_change;               // Mana cost change per level
  int min_position;              // Minimum position to cast
  int targets;                   // Valid targets
  int violent;                   // Violent spell flag
  int routines;                  // Spell effect routines
  char *name;                    // Spell name
  char *wear_off_msg;            // Message when spell ends
  int comp_flags;                // Component requirements
  int school;                    // School of magic
  int domain;                    // Divine domain
  int cast_time;                 // Casting time
};

// Example spell
spell_info[SPELL_MAGIC_MISSILE] = {
  .min_level = {1, LVL_IMMORT, 1, LVL_IMMORT, LVL_IMMORT, LVL_IMMORT},
  .mana_min = 15,
  .mana_max = 15,
  .mana_change = 0,
  .min_position = POS_FIGHTING,
  .targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT,
  .violent = TRUE,
  .routines = MAG_DAMAGE,
  .name = "magic missile",
  .wear_off_msg = NULL,
  .comp_flags = COMP_VERBAL | COMP_SOMATIC,
  .school = SCHOOL_EVOCATION,
  .domain = DOMAIN_UNDEFINED,
  .cast_time = 1
};
```

### Spell Preparation System
```c
// Prepare spells (for prepared casters like wizards)
ACMD(do_prepare) {
  int spell, slot_level;
  char spell_name[MAX_INPUT_LENGTH], slot_arg[MAX_INPUT_LENGTH];
  
  two_arguments(argument, spell_name, slot_arg);
  
  if (!*spell_name) {
    show_prepared_spells(ch);
    return;
  }
  
  spell = find_skill_num(spell_name);
  if (spell < 1 || spell > MAX_SPELLS) {
    send_to_char(ch, "That is not a valid spell.\r\n");
    return;
  }
  
  // Check if character knows the spell
  if (GET_SKILL(ch, spell) == 0) {
    send_to_char(ch, "You don't know that spell.\r\n");
    return;
  }
  
  slot_level = atoi(slot_arg);
  if (slot_level < 1 || slot_level > 9) {
    send_to_char(ch, "Invalid spell slot level.\r\n");
    return;
  }
  
  // Check if character has available slots
  if (GET_SPELL_SLOTS(ch, slot_level) <= GET_PREPARED_SPELLS(ch, slot_level)) {
    send_to_char(ch, "You have no available level %d spell slots.\r\n", slot_level);
    return;
  }
  
  // Prepare the spell
  add_prepared_spell(ch, spell, slot_level);
  send_to_char(ch, "You prepare %s in a level %d slot.\r\n", 
               spell_info[spell].name, slot_level);
}
```

### Spell Effects
```c
// Magic missile spell implementation
ASPELL(spell_magic_missile) {
  int dam, missiles, i;
  
  if (victim == NULL || ch == NULL) return;
  
  // Calculate number of missiles (1 + 1 per 2 caster levels, max 5)
  missiles = MIN(5, 1 + (GET_LEVEL(ch) / 2));
  
  for (i = 0; i < missiles; i++) {
    // Each missile does 1d4+1 damage
    dam = dice(1, 4) + 1;
    
    // Magic missiles always hit
    damage(ch, victim, dam, SPELL_MAGIC_MISSILE);
    
    if (DEAD(victim)) break;
  }
  
  act("$n points at $N and launches glowing missiles!", 
      FALSE, ch, 0, victim, TO_NOTVICT);
  act("You point at $N and launch glowing missiles!", 
      FALSE, ch, 0, victim, TO_CHAR);
  act("$n points at you and launches glowing missiles!", 
      FALSE, ch, 0, victim, TO_VICT);
}
```

## Combat Mechanics

### Attack Resolution
```c
int perform_attack(struct char_data *ch, struct char_data *victim, 
                   struct obj_data *weapon, int attack_number) {
  int attack_roll, damage_roll, ac, bab, attack_bonus = 0;
  
  // Calculate base attack bonus
  bab = GET_BAB(ch);
  
  // Apply iterative attack penalties
  attack_bonus = bab - (attack_number * 5);
  
  // Apply ability modifiers
  if (weapon && GET_OBJ_TYPE(weapon) == ITEM_WEAPON) {
    if (IS_RANGED_WEAPON(weapon)) {
      attack_bonus += GET_DEX_BONUS(ch);
    } else {
      attack_bonus += GET_STR_BONUS(ch);
    }
  } else {
    // Unarmed attack
    attack_bonus += GET_STR_BONUS(ch);
  }
  
  // Apply feat bonuses
  attack_bonus += apply_feat_bonuses(ch, BONUS_TYPE_ATTACK);
  
  // Apply magic bonuses
  attack_bonus += apply_magic_bonuses(ch, BONUS_TYPE_ATTACK);
  
  // Roll attack
  attack_roll = dice(1, 20) + attack_bonus;
  
  // Get target AC
  ac = GET_AC(victim);
  
  // Check for hit
  if (attack_roll >= ac || attack_roll == 20) {
    // Calculate damage
    damage_roll = calculate_damage(ch, victim, weapon);
    
    // Apply damage
    damage(ch, victim, damage_roll, TYPE_HIT);
    
    return TRUE; // Hit
  } else {
    // Miss
    act("$n misses $N with $s attack.", FALSE, ch, 0, victim, TO_NOTVICT);
    act("You miss $N with your attack.", FALSE, ch, 0, victim, TO_CHAR);
    act("$n misses you with $s attack.", FALSE, ch, 0, victim, TO_VICT);
    
    return FALSE; // Miss
  }
}
```

### Damage Calculation
```c
int calculate_damage(struct char_data *ch, struct char_data *victim, 
                     struct obj_data *weapon) {
  int damage = 0, str_bonus;
  
  if (weapon && GET_OBJ_TYPE(weapon) == ITEM_WEAPON) {
    // Weapon damage
    damage = dice(GET_OBJ_VAL(weapon, 1), GET_OBJ_VAL(weapon, 2));
    
    // Add strength bonus
    if (IS_RANGED_WEAPON(weapon)) {
      // Ranged weapons don't add STR (except composite bows)
      if (GET_OBJ_VAL(weapon, 3) == WEAPON_TYPE_COMPOSITE_BOW) {
        str_bonus = MIN(GET_STR_BONUS(ch), GET_OBJ_VAL(weapon, 4));
        damage += str_bonus;
      }
    } else {
      // Melee weapons add STR bonus
      str_bonus = GET_STR_BONUS(ch);
      
      // Two-handed weapons get 1.5x STR bonus
      if (IS_TWO_HANDED_WEAPON(weapon)) {
        damage += (str_bonus * 3) / 2;
      } else {
        damage += str_bonus;
      }
    }
  } else {
    // Unarmed damage (1d3 + STR bonus)
    damage = dice(1, 3) + GET_STR_BONUS(ch);
  }
  
  // Apply feat bonuses
  damage += apply_feat_bonuses(ch, BONUS_TYPE_DAMAGE);
  
  // Apply magic bonuses
  damage += apply_magic_bonuses(ch, BONUS_TYPE_DAMAGE);
  
  // Minimum 1 damage
  if (damage < 1) damage = 1;
  
  return damage;
}
```

### Armor Class Calculation
```c
int calculate_ac(struct char_data *ch) {
  int ac = 10; // Base AC
  struct obj_data *armor, *shield;
  
  // Add DEX bonus (limited by armor)
  int dex_bonus = GET_DEX_BONUS(ch);
  armor = GET_EQ(ch, WEAR_BODY);
  
  if (armor && GET_OBJ_TYPE(armor) == ITEM_ARMOR) {
    int max_dex = GET_OBJ_VAL(armor, 2);
    if (max_dex > 0) {
      dex_bonus = MIN(dex_bonus, max_dex);
    }
    
    // Add armor bonus
    ac += GET_OBJ_VAL(armor, 0);
  }
  
  ac += dex_bonus;
  
  // Add shield bonus
  shield = GET_EQ(ch, WEAR_SHIELD);
  if (shield && GET_OBJ_TYPE(shield) == ITEM_ARMOR) {
    ac += GET_OBJ_VAL(shield, 0);
  }
  
  // Add natural armor bonus
  ac += GET_NATURAL_ARMOR(ch);
  
  // Add deflection bonuses (from spells/magic items)
  ac += GET_DEFLECTION_BONUS(ch);
  
  // Add feat bonuses
  ac += apply_feat_bonuses(ch, BONUS_TYPE_AC);
  
  return ac;
}
```

## Saving Throws

### Save Calculation
```c
int calculate_save(struct char_data *ch, int save_type) {
  int base_save = 0, ability_mod = 0, total;
  
  // Get base save from class
  switch (save_type) {
    case SAVE_FORT:
      base_save = GET_SAVE_BASE(ch, SAVE_FORT);
      ability_mod = GET_CON_BONUS(ch);
      break;
    case SAVE_REFLEX:
      base_save = GET_SAVE_BASE(ch, SAVE_REFLEX);
      ability_mod = GET_DEX_BONUS(ch);
      break;
    case SAVE_WILL:
      base_save = GET_SAVE_BASE(ch, SAVE_WILL);
      ability_mod = GET_WIS_BONUS(ch);
      break;
  }
  
  total = base_save + ability_mod;
  
  // Add resistance bonuses (from spells/items)
  total += GET_RESISTANCE_BONUS(ch, save_type);
  
  // Add feat bonuses
  if (HAS_FEAT(ch, FEAT_GREAT_FORTITUDE) && save_type == SAVE_FORT) {
    total += 2;
  }
  if (HAS_FEAT(ch, FEAT_LIGHTNING_REFLEXES) && save_type == SAVE_REFLEX) {
    total += 2;
  }
  if (HAS_FEAT(ch, FEAT_IRON_WILL) && save_type == SAVE_WILL) {
    total += 2;
  }
  
  return total;
}
```

### Save Execution
```c
bool make_saving_throw(struct char_data *ch, int save_type, int difficulty) {
  int roll, save_bonus, total;
  
  roll = dice(1, 20);
  save_bonus = calculate_save(ch, save_type);
  total = roll + save_bonus;
  
  // Natural 1 always fails
  if (roll == 1) {
    return FALSE;
  }
  
  // Natural 20 always succeeds
  if (roll == 20) {
    return TRUE;
  }
  
  return (total >= difficulty);
}
```

---

*This documentation covers the core game mechanics systems. For specific implementation details and advanced mechanics, refer to the individual source files and the [Developer Guide](DEVELOPER_GUIDE_AND_API.md).*
