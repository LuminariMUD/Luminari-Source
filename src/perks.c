/**
 * @file perks.c
 * Perks System - Character progression through stage-based leveling
 * 
 * This system provides additional customization between levels through
 * a staged advancement system similar to DDO's enhancement trees.
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"

/* Workaround for class.h using NUM_ABILITIES instead of MAX_ABILITIES */
#ifndef NUM_ABILITIES
#define NUM_ABILITIES MAX_ABILITIES
#endif

#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "class.h"
#include "perks.h"
#include "assign_wpn_armor.h"

/* External function from class.c */
extern int level_exp(struct char_data *ch, int level);
#include "interpreter.h"
#include "constants.h"
#include "perks.h"

/* Global perk database - all defined perks */
struct perk_data perk_list[NUM_PERKS];

/* Initialize the perk system - called at boot */
void init_perks(void)
{
  int i;
  
  /* Clear all perks */
  for (i = 0; i < NUM_PERKS; i++)
  {
    perk_list[i].id = PERK_UNDEFINED;
    perk_list[i].name = strdup("Undefined");
    perk_list[i].description = strdup("This perk has not been defined.");
    perk_list[i].associated_class = CLASS_UNDEFINED;
    perk_list[i].cost = 0;
    perk_list[i].max_rank = 0;
    perk_list[i].prerequisite_perk = -1;
    perk_list[i].prerequisite_rank = 0;
    perk_list[i].effect_type = PERK_EFFECT_NONE;
    perk_list[i].effect_value = 0;
    perk_list[i].effect_modifier = 0;
    perk_list[i].special_description = strdup("");
    perk_list[i].toggleable = false;
  }
  
  /* Define Fighter Perks */
  define_fighter_perks();
  
  /* Define Wizard Perks */
  define_wizard_perks();
  
  /* Define Cleric Perks */
  define_cleric_perks();
  
  /* Define Rogue Perks */
  define_rogue_perks();
  
  /* Define Ranger Perks */
  define_ranger_perks();
  
  /* Define Barbarian Perks */
  define_barbarian_perks();
  
  log("Perks system initialized with %d defined perks.", count_defined_perks());
}

/* Count how many perks are actually defined */
int count_defined_perks(void)
{
  int i, count = 0;
  
  for (i = 0; i < NUM_PERKS; i++)
  {
    if (perk_list[i].id != PERK_UNDEFINED)
      count++;
  }
  
  return count;
}

/* Define Fighter Perks */
void define_fighter_perks(void)
{
  struct perk_data *perk;
  
  /*** TIER I PERKS (Tree 1: Weapon Specialist) ***/
  
  /* Weapon Focus I */
  perk = &perk_list[PERK_FIGHTER_WEAPON_FOCUS_1];
  perk->id = PERK_FIGHTER_WEAPON_FOCUS_1;
  perk->name = strdup("Weapon Focus I");
  perk->description = strdup("+1 to hit with all weapons");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_WEAPON_TOHIT;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("");
  
  /* Power Attack Training */
  perk = &perk_list[PERK_FIGHTER_POWER_ATTACK_TRAINING];
  perk->id = PERK_FIGHTER_POWER_ATTACK_TRAINING;
  perk->name = strdup("Power Attack Training");
  perk->description = strdup("+2 damage when using power attack, -1 to hit penalty instead of -2");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Improves power attack: +2 damage, reduces to-hit penalty to -1");
  
  /* Critical Awareness I */
  perk = &perk_list[PERK_FIGHTER_CRITICAL_AWARENESS_1];
  perk->id = PERK_FIGHTER_CRITICAL_AWARENESS_1;
  perk->name = strdup("Critical Awareness I");
  perk->description = strdup("+1 to critical confirmation rolls");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Bonus to critical hit confirmation rolls");
  
  /*** TIER II PERKS (Tree 1: Weapon Specialist) ***/
  
  /* Weapon Focus II */
  perk = &perk_list[PERK_FIGHTER_WEAPON_FOCUS_2];
  perk->id = PERK_FIGHTER_WEAPON_FOCUS_2;
  perk->name = strdup("Weapon Focus II");
  perk->description = strdup("Additional +1 to hit with all weapons (+2 total with Focus I)");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_FIGHTER_WEAPON_FOCUS_1;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_WEAPON_TOHIT;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("");
  
  /* Weapon Specialization I - Tier 2 version (multi-rank) */
  perk = &perk_list[PERK_FIGHTER_WEAPON_SPECIALIZATION_1];
  perk->id = PERK_FIGHTER_WEAPON_SPECIALIZATION_1;
  perk->name = strdup("Weapon Specialization I");
  perk->description = strdup("+1 damage per rank with all weapons");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 2;
  perk->max_rank = 3;
  perk->prerequisite_perk = PERK_FIGHTER_WEAPON_FOCUS_1;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_WEAPON_DAMAGE;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Can be taken up to 3 times for cumulative effect");
  
  /* Cleaving Strike */
  perk = &perk_list[PERK_FIGHTER_CLEAVING_STRIKE];
  perk->id = PERK_FIGHTER_CLEAVING_STRIKE;
  perk->name = strdup("Cleaving Strike");
  perk->description = strdup("If you kill an enemy, immediately make one additional attack");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_FIGHTER_POWER_ATTACK_TRAINING;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Extra attack on kill (similar to Cleave feat)");
  
  /* Critical Awareness II */
  perk = &perk_list[PERK_FIGHTER_CRITICAL_AWARENESS_2];
  perk->id = PERK_FIGHTER_CRITICAL_AWARENESS_2;
  perk->name = strdup("Critical Awareness II");
  perk->description = strdup("Additional +1 to critical confirmation rolls (+2 total)");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_FIGHTER_CRITICAL_AWARENESS_1;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Bonus to critical hit confirmation rolls");
  
  /* Improved Critical Threat */
  perk = &perk_list[PERK_FIGHTER_IMPROVED_CRITICAL_THREAT];
  perk->id = PERK_FIGHTER_IMPROVED_CRITICAL_THREAT;
  perk->name = strdup("Improved Critical Threat");
  perk->description = strdup("+1 to critical threat range (19-20 becomes 18-20, etc.)");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_FIGHTER_CRITICAL_AWARENESS_1;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Expands critical threat range by 1");
  
  /*** TIER III PERKS (Tree 1: Weapon Specialist) ***/
  
  /* Weapon Focus III */
  perk = &perk_list[PERK_FIGHTER_WEAPON_FOCUS_3];
  perk->id = PERK_FIGHTER_WEAPON_FOCUS_3;
  perk->name = strdup("Weapon Focus III");
  perk->description = strdup("Additional +1 to hit with all weapons (+3 total)");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_FIGHTER_WEAPON_FOCUS_2;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_WEAPON_TOHIT;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("");
  
  /* Weapon Specialization II - Tier 3 version (multi-rank) */
  perk = &perk_list[PERK_FIGHTER_WEAPON_SPECIALIZATION_2];
  perk->id = PERK_FIGHTER_WEAPON_SPECIALIZATION_2;
  perk->name = strdup("Weapon Specialization II");
  perk->description = strdup("Additional +1 damage per rank with all weapons");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 3;
  perk->max_rank = 3;
  perk->prerequisite_perk = PERK_FIGHTER_WEAPON_SPECIALIZATION_1;
  perk->prerequisite_rank = 3;
  perk->effect_type = PERK_EFFECT_WEAPON_DAMAGE;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires max rank Weapon Specialization I; can be taken up to 3 times");
  
  /* Great Cleave */
  perk = &perk_list[PERK_FIGHTER_GREAT_CLEAVE];
  perk->id = PERK_FIGHTER_GREAT_CLEAVE;
  perk->name = strdup("Great Cleave");
  perk->description = strdup("Cleaving Strike works on any kill, not just first");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_FIGHTER_CLEAVING_STRIKE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Unlimited cleave attacks on kills (like Great Cleave feat)");
  
  /* Devastating Critical */
  perk = &perk_list[PERK_FIGHTER_DEVASTATING_CRITICAL];
  perk->id = PERK_FIGHTER_DEVASTATING_CRITICAL;
  perk->name = strdup("Devastating Critical");
  perk->description = strdup("Critical hits deal +1d6 additional damage");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_FIGHTER_IMPROVED_CRITICAL_THREAT;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Extra 1d6 damage on critical hits");
  
  /*** TREE 2: DEFENDER - TIER I ***/
  
  /* Armor Training I */
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
  
  /* Toughness I */
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
  
  /* Resilience */
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
  
  /*** TREE 2: DEFENDER - TIER II ***/
  
  /* Armor Training II */
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
  
  /* Shield Mastery I */
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
  
  /* Defensive Stance */
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
  perk->toggleable = true; /* Can be toggled on/off */
  
  /* Iron Will */
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
  
  /* Lightning Reflexes */
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
  
  /*** TREE 2: DEFENDER - TIER III ***/
  
  /* Armor Training III */
  perk = &perk_list[PERK_FIGHTER_ARMOR_TRAINING_3];
  perk->id = PERK_FIGHTER_ARMOR_TRAINING_3;
  perk->name = strdup("Armor Training III");
  perk->description = strdup("+2 AC bonus");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_FIGHTER_ARMOR_TRAINING_2;
  perk->prerequisite_rank = 2; /* Requires Armor Training II at max rank */
  perk->effect_type = PERK_EFFECT_AC;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Armor Training II at max rank (2)");
  
  /* Shield Mastery II */
  perk = &perk_list[PERK_FIGHTER_SHIELD_MASTERY_2];
  perk->id = PERK_FIGHTER_SHIELD_MASTERY_2;
  perk->name = strdup("Shield Mastery II");
  perk->description = strdup("+2 AC with shield (+4 total with Shield Mastery I)");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_FIGHTER_SHIELD_MASTERY_1;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("+2 additional AC bonus when wielding a shield");
  
  /* Improved Damage Reduction */
  perk = &perk_list[PERK_FIGHTER_IMPROVED_DAMAGE_REDUCTION];
  perk->id = PERK_FIGHTER_IMPROVED_DAMAGE_REDUCTION;
  perk->name = strdup("Improved Damage Reduction");
  perk->description = strdup("Damage reduction increases to 4/- when Defensive Stance is active");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_FIGHTER_DEFENSIVE_STANCE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Increases Defensive Stance DR from 2/- to 4/-");
  
  /* Stalwart */
  perk = &perk_list[PERK_FIGHTER_STALWART];
  perk->id = PERK_FIGHTER_STALWART;
  perk->name = strdup("Stalwart");
  perk->description = strdup("Immune to fear, +2 bonus vs mind-affecting effects");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_FIGHTER_IRON_WILL;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Grants immunity to fear effects and +2 bonus to saves vs mind-affecting");
  
  /*** TREE 2: DEFENDER - TIER IV ***/
  
  /* Immovable Object */
  perk = &perk_list[PERK_FIGHTER_IMMOVABLE_OBJECT];
  perk->id = PERK_FIGHTER_IMMOVABLE_OBJECT;
  perk->name = strdup("Immovable Object");
  perk->description = strdup("DR 6/-, immunity to knockdown and bull rush");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_FIGHTER_IMPROVED_DAMAGE_REDUCTION;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 6;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Increases DR to 6/- (replaces Defensive Stance DR) and grants immunity to knockdown and bull rush");
  perk->toggleable = true; /* Can be toggled on/off like Defensive Stance */
  
  /* Last Stand */
  perk = &perk_list[PERK_FIGHTER_LAST_STAND];
  perk->id = PERK_FIGHTER_LAST_STAND;
  perk->name = strdup("Last Stand");
  perk->description = strdup("Once per day, when reduced to 0 HP, stay at 1 HP for 5 rounds");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_FIGHTER_TOUGHNESS_1;
  perk->prerequisite_rank = 5; /* Requires Toughness I at max rank */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Once per day, when HP drops to 0 or below, remain at 1 HP for 5 rounds. Requires max Toughness I and max Resilience.");
  
  /*** OLDER PERKS (to be reorganized into tiers) ***/
  
  /* Weapon Specialization I */
  perk = &perk_list[PERK_FIGHTER_WEAPON_SPEC_1];
  perk->id = PERK_FIGHTER_WEAPON_SPEC_1;
  perk->name = strdup("Weapon Specialization I");
  perk->description = strdup("+1 damage with all melee weapons");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_WEAPON_DAMAGE;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("");
  
  /* Weapon Specialization II */
  perk = &perk_list[PERK_FIGHTER_WEAPON_SPEC_2];
  perk->id = PERK_FIGHTER_WEAPON_SPEC_2;
  perk->name = strdup("Weapon Specialization II");
  perk->description = strdup("+2 damage with all melee weapons");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_FIGHTER_WEAPON_SPEC_1;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_WEAPON_DAMAGE;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("");
  
  /* Weapon Specialization III */
  perk = &perk_list[PERK_FIGHTER_WEAPON_SPEC_3];
  perk->id = PERK_FIGHTER_WEAPON_SPEC_3;
  perk->name = strdup("Weapon Specialization III");
  perk->description = strdup("+3 damage with all melee weapons");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_FIGHTER_WEAPON_SPEC_2;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_WEAPON_DAMAGE;
  perk->effect_value = 3;
  perk->effect_modifier = 0;
  perk->special_description = strdup("");
  
  /* Armor Mastery I */
  perk = &perk_list[PERK_FIGHTER_ARMOR_MASTERY_1];
  perk->id = PERK_FIGHTER_ARMOR_MASTERY_1;
  perk->name = strdup("Armor Mastery I");
  perk->description = strdup("Reduce armor check penalty by 1");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Reduces armor check penalties");
  
  /* Armor Mastery II */
  perk = &perk_list[PERK_FIGHTER_ARMOR_MASTERY_2];
  perk->id = PERK_FIGHTER_ARMOR_MASTERY_2;
  perk->name = strdup("Armor Mastery II");
  perk->description = strdup("Reduce armor check penalty by 2");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_FIGHTER_ARMOR_MASTERY_1;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Reduces armor check penalties");
  
  /* Armor Mastery III */
  perk = &perk_list[PERK_FIGHTER_ARMOR_MASTERY_3];
  perk->id = PERK_FIGHTER_ARMOR_MASTERY_3;
  perk->name = strdup("Armor Mastery III");
  perk->description = strdup("Reduce armor check penalty by 3");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_FIGHTER_ARMOR_MASTERY_2;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Reduces armor check penalties");
  
  /* Toughness */
  perk = &perk_list[PERK_FIGHTER_TOUGHNESS];
  perk->id = PERK_FIGHTER_TOUGHNESS;
  perk->name = strdup("Toughness");
  perk->description = strdup("+5 HP per rank");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 1;
  perk->max_rank = 5;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_HP;
  perk->effect_value = 5;
  perk->effect_modifier = 0;
  perk->special_description = strdup("");
  
  /* Physical Resistance I */
  perk = &perk_list[PERK_FIGHTER_PHYSICAL_RESISTANCE_1];
  perk->id = PERK_FIGHTER_PHYSICAL_RESISTANCE_1;
  perk->name = strdup("Physical Resistance I");
  perk->description = strdup("+1 to Fortitude saves");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SAVE;
  perk->effect_value = 1;
  perk->effect_modifier = APPLY_SAVING_FORT;
  perk->special_description = strdup("");
}

/* Define Wizard Perks */
void define_wizard_perks(void)
{
  struct perk_data *perk;
  
  /* Spell Focus I */
  perk = &perk_list[PERK_WIZARD_SPELL_FOCUS_1];
  perk->id = PERK_WIZARD_SPELL_FOCUS_1;
  perk->name = strdup("Spell Focus Enhancement I");
  perk->description = strdup("+1 DC to chosen spell school");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPELL_DC;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Choose spell school when taking this perk");
  
  /* Arcane Augmentation */
  perk = &perk_list[PERK_WIZARD_ARCANE_AUGMENTATION];
  perk->id = PERK_WIZARD_ARCANE_AUGMENTATION;
  perk->name = strdup("Arcane Augmentation");
  perk->description = strdup("+5 spell points per rank");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 1;
  perk->max_rank = 5;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPELL_POINTS;
  perk->effect_value = 5;
  perk->effect_modifier = 0;
  perk->special_description = strdup("");
  
  /* Extended Spell I */
  perk = &perk_list[PERK_WIZARD_EXTENDED_SPELL_1];
  perk->id = PERK_WIZARD_EXTENDED_SPELL_1;
  perk->name = strdup("Extended Spell Enhancement I");
  perk->description = strdup("+10% spell duration");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPELL_DURATION;
  perk->effect_value = 10;
  perk->effect_modifier = 0;
  perk->special_description = strdup("");
  
  /* Potent Magic I */
  perk = &perk_list[PERK_WIZARD_POTENT_MAGIC_1];
  perk->id = PERK_WIZARD_POTENT_MAGIC_1;
  perk->name = strdup("Potent Magic I");
  perk->description = strdup("+1 to spell penetration checks");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Bonus to spell penetration checks");
}

/* Define Cleric Perks */
void define_cleric_perks(void)
{
  struct perk_data *perk;
  
  /* Healing Amplification */
  perk = &perk_list[PERK_CLERIC_HEALING_AMP];
  perk->id = PERK_CLERIC_HEALING_AMP;
  perk->name = strdup("Healing Amplification");
  perk->description = strdup("+10% healing spell effectiveness per rank");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 1;
  perk->max_rank = 5;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Increases healing done by healing spells");
  
  /* Empowered Healing I */
  perk = &perk_list[PERK_CLERIC_EMPOWERED_HEALING_1];
  perk->id = PERK_CLERIC_EMPOWERED_HEALING_1;
  perk->name = strdup("Empowered Healing I");
  perk->description = strdup("Heal an additional 1d6 HP with healing spells");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 6;
  perk->special_description = strdup("Adds 1d6 to healing spell rolls");
  
  /* Toughness */
  perk = &perk_list[PERK_CLERIC_TOUGHNESS];
  perk->id = PERK_CLERIC_TOUGHNESS;
  perk->name = strdup("Toughness");
  perk->description = strdup("+5 HP per rank");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 1;
  perk->max_rank = 5;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_HP;
  perk->effect_value = 5;
  perk->effect_modifier = 0;
  perk->special_description = strdup("");
}

/* Define Rogue Perks */
void define_rogue_perks(void)
{
  struct perk_data *perk;
  
  /* Sneak Attack Enhancement */
  perk = &perk_list[PERK_ROGUE_SNEAK_ATTACK];
  perk->id = PERK_ROGUE_SNEAK_ATTACK;
  perk->name = strdup("Sneak Attack Enhancement");
  perk->description = strdup("+1d6 sneak attack damage per rank");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 1;
  perk->max_rank = 5;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 6;
  perk->special_description = strdup("Increases sneak attack damage dice");
  
  /* Deadly Precision I */
  perk = &perk_list[PERK_ROGUE_DEADLY_PRECISION_1];
  perk->id = PERK_ROGUE_DEADLY_PRECISION_1;
  perk->name = strdup("Deadly Precision I");
  perk->description = strdup("Critical hits with sneak attack deal +1d6 damage");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 6;
  perk->special_description = strdup("Extra damage on critical sneak attacks");
  
  /* Improved Flanking I */
  perk = &perk_list[PERK_ROGUE_IMPROVED_FLANKING_1];
  perk->id = PERK_ROGUE_IMPROVED_FLANKING_1;
  perk->name = strdup("Improved Flanking I");
  perk->description = strdup("+1 to hit when flanking");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Bonus to attack when flanking enemies");
}

/* Define Ranger Perks */
void define_ranger_perks(void)
{
  struct perk_data *perk;
  
  /* Favored Enemy Enhancement I */
  perk = &perk_list[PERK_RANGER_FAVORED_ENEMY_1];
  perk->id = PERK_RANGER_FAVORED_ENEMY_1;
  perk->name = strdup("Favored Enemy Enhancement I");
  perk->description = strdup("+1 damage vs. favored enemy");
  perk->associated_class = CLASS_RANGER;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Extra damage against favored enemies");
  
  /* Toughness */
  perk = &perk_list[PERK_RANGER_TOUGHNESS];
  perk->id = PERK_RANGER_TOUGHNESS;
  perk->name = strdup("Toughness");
  perk->description = strdup("+5 HP per rank");
  perk->associated_class = CLASS_RANGER;
  perk->cost = 1;
  perk->max_rank = 5;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_HP;
  perk->effect_value = 5;
  perk->effect_modifier = 0;
  perk->special_description = strdup("");
  
  /* Bow Mastery I */
  perk = &perk_list[PERK_RANGER_BOW_MASTERY_1];
  perk->id = PERK_RANGER_BOW_MASTERY_1;
  perk->name = strdup("Bow Mastery I");
  perk->description = strdup("+1 to hit with bows");
  perk->associated_class = CLASS_RANGER;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_WEAPON_TOHIT;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Bonus to hit with bow weapons");
}

/* Define Barbarian Perks */
void define_barbarian_perks(void)
{
  struct perk_data *perk;
  
  /* Rage Enhancement */
  perk = &perk_list[PERK_BARBARIAN_RAGE_ENHANCEMENT];
  perk->id = PERK_BARBARIAN_RAGE_ENHANCEMENT;
  perk->name = strdup("Rage Enhancement");
  perk->description = strdup("+1 to Strength and Constitution while raging per rank");
  perk->associated_class = CLASS_BERSERKER;
  perk->cost = 1;
  perk->max_rank = 5;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Enhances rage bonuses");
  
  /* Extended Rage I */
  perk = &perk_list[PERK_BARBARIAN_EXTENDED_RAGE_1];
  perk->id = PERK_BARBARIAN_EXTENDED_RAGE_1;
  perk->name = strdup("Extended Rage I");
  perk->description = strdup("Rage lasts +2 rounds longer");
  perk->associated_class = CLASS_BERSERKER;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Increases rage duration");
  
  /* Toughness */
  perk = &perk_list[PERK_BARBARIAN_TOUGHNESS];
  perk->id = PERK_BARBARIAN_TOUGHNESS;
  perk->name = strdup("Toughness");
  perk->description = strdup("+5 HP per rank");
  perk->associated_class = CLASS_BERSERKER;
  perk->cost = 1;
  perk->max_rank = 5;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_HP;
  perk->effect_value = 5;
  perk->effect_modifier = 0;
  perk->special_description = strdup("");
}

/* Lookup functions */

/* Get perk by ID */
struct perk_data *get_perk_by_id(int perk_id)
{
  if (perk_id < 0 || perk_id >= NUM_PERKS)
    return NULL;
  
  if (perk_list[perk_id].id == PERK_UNDEFINED)
    return NULL;
  
  return &perk_list[perk_id];
}

/* Get all perks for a specific class */
int get_class_perks(int class_id, int *perk_ids, int max_perks)
{
  int i, count = 0;
  
  for (i = 0; i < NUM_PERKS && count < max_perks; i++)
  {
    if (perk_list[i].id != PERK_UNDEFINED && 
        perk_list[i].associated_class == class_id)
    {
      perk_ids[count++] = i;
    }
  }
  
  return count;
}

/* Check if a perk exists */
bool perk_exists(int perk_id)
{
  if (perk_id < 0 || perk_id >= NUM_PERKS)
    return FALSE;
  
  return (perk_list[perk_id].id != PERK_UNDEFINED);
}

/* Get perk name */
const char *get_perk_name(int perk_id)
{
  struct perk_data *perk = get_perk_by_id(perk_id);
  
  if (!perk)
    return "Unknown Perk";
  
  return perk->name;
}

/* Get perk description */
const char *get_perk_description(int perk_id)
{
  struct perk_data *perk = get_perk_by_id(perk_id);
  
  if (!perk)
    return "This perk does not exist.";
  
  return perk->description;
}

/*****************************************************************************
 * STAGE PROGRESSION FUNCTIONS (Step 3)
 * These functions handle the stage-based leveling system where each level
 * is divided into 4 stages, each requiring 25% of the total level XP.
 *****************************************************************************/

/* External function from class.c */
extern int level_exp(struct char_data *ch, int level);

/**
 * Initialize stage data for a new character or character without stage data.
 * Sets them to stage 0 with 0 stage XP so they can advance through stages 1-3
 * and receive perk points as they gain XP.
 */
void init_stage_data(struct char_data *ch)
{
  if (!ch)
    return;
  
  ch->player_specials->saved.stage_info.current_stage = 0;
  ch->player_specials->saved.stage_info.stage_exp = 0;
  ch->player_specials->saved.stage_info.exp_to_next_stage = calculate_stage_xp_needed(ch);
}

/**
 * Update stage data after XP is gained.
 * Recalculates the XP needed to reach the next stage.
 */
void update_stage_data(struct char_data *ch)
{
  if (!ch)
    return;
  
  ch->player_specials->saved.stage_info.exp_to_next_stage = calculate_stage_xp_needed(ch);
}

/**
 * Calculate how much XP is needed to advance to the next stage.
 * Each stage requires 25% of the total XP needed for the level.
 * 
 * @param ch The character
 * @return XP needed for next stage, or 0 if at max level
 */
int calculate_stage_xp_needed(struct char_data *ch)
{
  int current_level, next_level_xp, current_level_xp, xp_for_level, stage_xp;
  int current_stage;
  
  if (!ch)
    return 0;
  
  current_level = GET_LEVEL(ch);
  current_stage = ch->player_specials->saved.stage_info.current_stage;
  
  /* At max level or stage 4 (ready to level) */
  if (current_level >= LVL_IMMORT || current_stage >= STAGES_PER_LEVEL)
    return 0;
  
  /* Calculate total XP needed for this level */
  next_level_xp = level_exp(ch, current_level + 1);
  current_level_xp = level_exp(ch, current_level);
  xp_for_level = next_level_xp - current_level_xp;
  
  /* Each stage is 25% of the level */
  stage_xp = xp_for_level / STAGES_PER_LEVEL;
  
  return stage_xp;
}

/**
 * Check if character has enough XP to advance to the next stage.
 * Awards perk points for stages 1-3, and sets ready-to-level flag for stage 4.
 * Perk points are awarded to GET_CLASS(ch) (the last class gained).
 * 
 * @param ch The character
 * @param perk_points_awarded Output parameter - set to 1 if points awarded, 0 otherwise
 * @return TRUE if character advanced a stage, FALSE otherwise
 */
bool check_stage_advancement(struct char_data *ch, int *perk_points_awarded)
{
  int current_level_xp, stage_xp_needed, stages_gained = 0;
  int current_stage;
  bool advanced = FALSE;
  
  if (!ch || IS_NPC(ch))
  {
    if (perk_points_awarded)
      *perk_points_awarded = 0;
    return FALSE;
  }
  
  /* Initialize perk points awarded */
  if (perk_points_awarded)
    *perk_points_awarded = 0;
  
  /* Get current stage info */
  current_stage = ch->player_specials->saved.stage_info.current_stage;
  
  /* Already at stage 4 (ready to level) */
  if (current_stage >= STAGES_PER_LEVEL)
    return FALSE;
  
  /* Calculate how much XP we have within this level */
  current_level_xp = level_exp(ch, GET_LEVEL(ch));
  stage_xp_needed = calculate_stage_xp_needed(ch);
  
  /* Check if we have enough XP to advance stages */
  while (current_stage < STAGES_PER_LEVEL && 
         GET_EXP(ch) >= (current_level_xp + (stage_xp_needed * current_stage)))
  {
    current_stage++;
    stages_gained++;
    advanced = TRUE;
    
    /* Award perk points for stages 1-3 to the last class gained */
    if (current_stage < STAGES_PER_LEVEL)
    {
      int award_class = GET_CLASS(ch);
      if (award_class < 0 || award_class >= NUM_CLASSES)
      {
        send_to_char(ch, "\tR[DEBUG] Invalid GET_CLASS: %d (should be 0-%d)\tn\r\n", 
                    award_class, NUM_CLASSES-1);
      }
      award_stage_perk_points(ch, award_class);
      if (perk_points_awarded)
        (*perk_points_awarded)++;
    }
  }
  
  /* Update character's stage data */
  if (advanced)
  {
    ch->player_specials->saved.stage_info.current_stage = current_stage;
    
    if (current_stage >= STAGES_PER_LEVEL)
    {
      /* Stage 4 reached - ready to level */
      send_to_char(ch, "\tYYou have completed all stages for this level!\tn\r\n");
      send_to_char(ch, "\tYType 'gain' to advance to the next level.\tn\r\n");
    }
    else
    {
      /* Advanced to stage 1, 2, or 3 */
      if (stages_gained > 1)
        send_to_char(ch, "\tYYou have advanced %d stages! (Now at stage %d/4)\tn\r\n", 
                    stages_gained, current_stage);
      else
        send_to_char(ch, "\tYYou have advanced to stage %d/4!\tn\r\n", current_stage);
    }
    
    update_stage_data(ch);
  }
  
  return advanced;
}

/**
 * Award perk point(s) to the character for advancing a stage.
 * Points are awarded to the class that is being leveled.
 * 
 * @param ch The character
 * @param class_id The class that is gaining the level
 */
void award_stage_perk_points(struct char_data *ch, int class_id)
{
  if (!ch || IS_NPC(ch))
    return;
  
  if (class_id < 0 || class_id >= NUM_CLASSES)
  {
    send_to_char(ch, "\tRERROR: Invalid class ID %d in award_stage_perk_points!\tn\r\n", class_id);
    return;
  }
  
  /* Award 1 perk point per stage (stages 1-3 only) */
  ch->player_specials->saved.perk_points[class_id]++;
  
  send_to_char(ch, "\tGYou gain 1 perk point for your %s class! (Total: %d)\tn\r\n",
              class_names[class_id], ch->player_specials->saved.perk_points[class_id]);
}

/*****************************************************************************
 * PERK POINT TRACKING FUNCTIONS (Step 4)
 * These functions manage the class-specific perk point pools.
 *****************************************************************************/

/**
 * Get the number of unspent perk points for a specific class.
 * 
 * @param ch The character
 * @param class_id The class to check
 * @return Number of unspent perk points, or 0 if invalid
 */
int get_perk_points(struct char_data *ch, int class_id)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (class_id < 0 || class_id >= NUM_CLASSES)
    return 0;
  
  return ch->player_specials->saved.perk_points[class_id];
}

/**
 * Spend perk points from a class's pool.
 * Returns TRUE if successful, FALSE if not enough points.
 * 
 * @param ch The character
 * @param class_id The class to spend points from
 * @param amount Number of points to spend
 * @return TRUE if successful, FALSE if insufficient points
 */
bool spend_perk_points(struct char_data *ch, int class_id, int amount)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  if (class_id < 0 || class_id >= NUM_CLASSES)
    return FALSE;
  
  if (amount < 0)
    return FALSE;
  
  /* Check if character has enough points */
  if (ch->player_specials->saved.perk_points[class_id] < amount)
    return FALSE;
  
  /* Spend the points */
  ch->player_specials->saved.perk_points[class_id] -= amount;
  
  return TRUE;
}

/**
 * Add perk points to a class's pool.
 * Used for refunds or admin adjustments.
 * 
 * @param ch The character
 * @param class_id The class to add points to
 * @param amount Number of points to add
 */
void add_perk_points(struct char_data *ch, int class_id, int amount)
{
  if (!ch || IS_NPC(ch))
    return;
  
  if (class_id < 0 || class_id >= NUM_CLASSES)
    return;
  
  if (amount < 0)
    return;
  
  ch->player_specials->saved.perk_points[class_id] += amount;
}

/**
 * Get the total number of unspent perk points across all classes.
 * 
 * @param ch The character
 * @return Total unspent perk points
 */
int get_total_perk_points(struct char_data *ch)
{
  int i, total = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  for (i = 0; i < NUM_CLASSES; i++)
  {
    total += ch->player_specials->saved.perk_points[i];
  }
  
  return total;
}

/**
 * Display all perk points for a character (used in score/status commands).
 * Shows points for each class the character has levels in.
 * 
 * @param ch The character to display points for
 */
void display_perk_points(struct char_data *ch)
{
  int i, points, has_points = FALSE;
  
  if (!ch || IS_NPC(ch))
    return;
  
  send_to_char(ch, "\tWPerk Points:\tn\r\n");
  
  /* Display points for each class the character has levels in */
  for (i = 0; i < NUM_CLASSES; i++)
  {
    /* Only show classes with levels or unspent points */
    if (CLASS_LEVEL(ch, i) > 0 || ch->player_specials->saved.perk_points[i] > 0)
    {
      points = ch->player_specials->saved.perk_points[i];
      send_to_char(ch, "  %-20s: %s%d\tn point%s\r\n", 
                  class_list[i].name,
                  points > 0 ? "\tG" : "\tD",
                  points,
                  points == 1 ? "" : "s");
      has_points = TRUE;
    }
  }
  
  if (!has_points)
  {
    send_to_char(ch, "  You have no perk points yet.\r\n");
  }
}

/*****************************************************************************
 * PERK PURCHASE/MANAGEMENT FUNCTIONS (Step 5)
 * These functions handle purchasing, checking, and managing character perks.
 *****************************************************************************/

/**
 * Check if a character has purchased a specific perk (in any class).
 * 
 * @param ch The character
 * @param perk_id The perk ID to check
 * @return TRUE if character has the perk, FALSE otherwise
 */
bool has_perk(struct char_data *ch, int perk_id)
{
  struct char_perk_data *perk;
  
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  for (perk = ch->player_specials->saved.perks; perk; perk = perk->next)
  {
    if (perk->perk_id == perk_id)
      return TRUE;
  }
  
  return FALSE;
}

/**
 * Check if a perk toggle bit is set for a character.
 * 
 * @param ch The character
 * @param perk_id The perk ID to check (0-255)
 * @return TRUE if toggled on, FALSE if toggled off
 */
bool is_perk_toggled_on(struct char_data *ch, int perk_id)
{
  int byte_index, bit_index;
  
  if (!ch || IS_NPC(ch))
    return FALSE;
    
  if (perk_id < 0 || perk_id >= 256)
    return FALSE;
  
  byte_index = perk_id / 8;
  bit_index = perk_id % 8;
  
  return (ch->player_specials->saved.perk_toggles[byte_index] & (1 << bit_index)) != 0;
}

/**
 * Set a perk toggle bit for a character.
 * 
 * @param ch The character
 * @param perk_id The perk ID to set (0-255)
 * @param state TRUE to toggle on, FALSE to toggle off
 */
void set_perk_toggle(struct char_data *ch, int perk_id, bool state)
{
  int byte_index, bit_index;
  
  if (!ch || IS_NPC(ch))
    return;
    
  if (perk_id < 0 || perk_id >= 256)
    return;
  
  byte_index = perk_id / 8;
  bit_index = perk_id % 8;
  
  if (state)
    ch->player_specials->saved.perk_toggles[byte_index] |= (1 << bit_index);
  else
    ch->player_specials->saved.perk_toggles[byte_index] &= ~(1 << bit_index);
}

/**
 * Check if a character has a perk AND it's active (not toggled off).
 * For non-toggleable perks, this is the same as has_perk().
 * For toggleable perks, checks both ownership and toggle state.
 * 
 * @param ch The character
 * @param perk_id The perk ID to check
 * @return TRUE if character has the perk and it's active, FALSE otherwise
 */
bool has_perk_active(struct char_data *ch, int perk_id)
{
  struct perk_data *perk_def;
  
  if (!has_perk(ch, perk_id))
    return FALSE;
  
  perk_def = get_perk_by_id(perk_id);
  if (!perk_def)
    return FALSE;
  
  /* Non-toggleable perks are always active */
  if (!perk_def->toggleable)
    return TRUE;
  
  /* Toggleable perks require the toggle to be on */
  return is_perk_toggled_on(ch, perk_id);
}

/**
 * Get the rank of a perk for a specific class.
 * Returns 0 if the character doesn't have the perk for that class.
 * 
 * @param ch The character
 * @param perk_id The perk ID to check
 * @param class_id The class to check for
 * @return Current rank (0 if not purchased)
 */
int get_perk_rank(struct char_data *ch, int perk_id, int class_id)
{
  struct char_perk_data *perk;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (class_id < 0 || class_id >= NUM_CLASSES)
    return 0;
  
  perk = find_char_perk(ch, perk_id, class_id);
  
  return perk ? perk->current_rank : 0;
}

/**
 * Get the total rank of a perk across all classes.
 * Useful for perks that stack between classes.
 * 
 * @param ch The character
 * @param perk_id The perk ID to check
 * @return Total ranks across all classes
 */
int get_total_perk_ranks(struct char_data *ch, int perk_id)
{
  struct char_perk_data *perk;
  int total_ranks = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  for (perk = ch->player_specials->saved.perks; perk; perk = perk->next)
  {
    if (perk->perk_id == perk_id)
      total_ranks += perk->current_rank;
  }
  
  return total_ranks;
}

/**
 * Find a character's perk entry for a specific perk and class.
 * 
 * @param ch The character
 * @param perk_id The perk ID to find
 * @param class_id The class to find it for
 * @return Pointer to char_perk_data or NULL if not found
 */
struct char_perk_data *find_char_perk(struct char_data *ch, int perk_id, int class_id)
{
  struct char_perk_data *perk;
  
  if (!ch || IS_NPC(ch))
    return NULL;
  
  if (class_id < 0 || class_id >= NUM_CLASSES)
    return NULL;
  
  for (perk = ch->player_specials->saved.perks; perk; perk = perk->next)
  {
    if (perk->perk_id == perk_id && perk->perk_class == class_id)
      return perk;
  }
  
  return NULL;
}

/**
 * Check if a character can purchase a perk.
 * Validates prerequisites, class requirements, rank limits, and point availability.
 * 
 * @param ch The character
 * @param perk_id The perk to check
 * @param class_id The class to purchase it for
 * @param error_msg Buffer to store error message (can be NULL)
 * @param error_len Size of error message buffer
 * @return TRUE if can purchase, FALSE otherwise
 */
bool can_purchase_perk(struct char_data *ch, int perk_id, int class_id, char *error_msg, size_t error_len)
{
  struct perk_data *perk;
  struct char_perk_data *char_perk;
  int current_rank;
  
  /* Clear error message */
  if (error_msg && error_len > 0)
    *error_msg = '\0';
  
  /* Basic validation */
  if (!ch || IS_NPC(ch))
  {
    if (error_msg)
      snprintf(error_msg, error_len, "Invalid character.");
    return FALSE;
  }
  
  if (class_id < 0 || class_id >= NUM_CLASSES)
  {
    if (error_msg)
      snprintf(error_msg, error_len, "Invalid class.");
    return FALSE;
  }
  
  /* Get perk data */
  perk = get_perk_by_id(perk_id);
  if (!perk)
  {
    if (error_msg)
      snprintf(error_msg, error_len, "Invalid perk ID.");
    return FALSE;
  }
  
  /* Check if perk belongs to this class */
  if (perk->associated_class != class_id)
  {
    if (error_msg)
      snprintf(error_msg, error_len, "This perk does not belong to the %s class.", 
              class_list[class_id].name);
    return FALSE;
  }
  
  /* Check if character has levels in this class */
  if (CLASS_LEVEL(ch, class_id) <= 0)
  {
    if (error_msg)
      snprintf(error_msg, error_len, "You must have at least one level in %s to purchase this perk.",
              class_list[class_id].name);
    return FALSE;
  }
  
  /* Check current rank */
  char_perk = find_char_perk(ch, perk_id, class_id);
  current_rank = char_perk ? char_perk->current_rank : 0;
  
  /* Check if at max rank */
  if (current_rank >= perk->max_rank)
  {
    if (error_msg)
      snprintf(error_msg, error_len, "You already have the maximum rank (%d) in this perk.",
              perk->max_rank);
    return FALSE;
  }
  
  /* Check perk point cost */
  if (get_perk_points(ch, class_id) < perk->cost)
  {
    if (error_msg)
      snprintf(error_msg, error_len, "You need %d perk point%s (%d available) to purchase this.",
              perk->cost, perk->cost == 1 ? "" : "s", get_perk_points(ch, class_id));
    return FALSE;
  }
  
  /* Check prerequisite (if any) */
  if (perk->prerequisite_perk != PERK_UNDEFINED && perk->prerequisite_perk >= 0)
  {
    int prereq_id = perk->prerequisite_perk;
    struct perk_data *prereq_perk = get_perk_by_id(prereq_id);
    
    if (prereq_perk)
    {
      int prereq_rank = get_perk_rank(ch, prereq_id, class_id);
      
      /* Check if character has the prerequisite perk at required rank */
      if (prereq_rank < perk->prerequisite_rank)
      {
        if (error_msg)
        {
          if (perk->prerequisite_rank > 1)
            snprintf(error_msg, error_len, "You must first purchase: %s (Rank %d)",
                    prereq_perk->name, perk->prerequisite_rank);
          else
            snprintf(error_msg, error_len, "You must first purchase: %s", prereq_perk->name);
        }
        return FALSE;
      }
    }
  }
  
  /* Special prerequisite check for Last Stand - requires both Toughness I and Resilience at max */
  if (perk_id == PERK_FIGHTER_LAST_STAND)
  {
    int resilience_rank = get_perk_rank(ch, PERK_FIGHTER_RESILIENCE, class_id);
    if (resilience_rank < 3) /* Resilience max rank is 3 */
    {
      if (error_msg)
        snprintf(error_msg, error_len, "You must have Resilience at max rank (3) to purchase Last Stand.");
      return FALSE;
    }
  }
  
  /* Special prerequisite check for Immovable Object - also requires Armor Training III */
  if (perk_id == PERK_FIGHTER_IMMOVABLE_OBJECT)
  {
    int armor_training_3_rank = get_perk_rank(ch, PERK_FIGHTER_ARMOR_TRAINING_3, class_id);
    if (armor_training_3_rank < 1)
    {
      if (error_msg)
        snprintf(error_msg, error_len, "You must have Armor Training III to purchase Immovable Object.");
      return FALSE;
    }
  }
  
  /* All checks passed */
  return TRUE;
}

/**
 * Purchase a perk for a character.
 * Deducts perk points and adds the perk to the character's list.
 * 
 * @param ch The character
 * @param perk_id The perk to purchase
 * @param class_id The class to purchase it for
 * @return TRUE if successful, FALSE otherwise
 */
bool purchase_perk(struct char_data *ch, int perk_id, int class_id)
{
  struct perk_data *perk;
  struct char_perk_data *char_perk;
  char error_msg[MAX_STRING_LENGTH];
  
  /* Validate purchase */
  if (!can_purchase_perk(ch, perk_id, class_id, error_msg, sizeof(error_msg)))
  {
    send_to_char(ch, "%s\r\n", error_msg);
    return FALSE;
  }
  
  perk = get_perk_by_id(perk_id);
  if (!perk) /* Should never happen after validation */
    return FALSE;
  
  /* Try to spend the perk points */
  if (!spend_perk_points(ch, class_id, perk->cost))
  {
    send_to_char(ch, "Failed to spend perk points.\r\n");
    return FALSE;
  }
  
  /* Find or create the character perk entry */
  char_perk = find_char_perk(ch, perk_id, class_id);
  
  if (char_perk)
  {
    /* Increase rank of existing perk */
    char_perk->current_rank++;
    send_to_char(ch, "\tGYou have increased %s to rank %d/%d!\tn\r\n",
                perk->name, char_perk->current_rank, perk->max_rank);
  }
  else
  {
    /* Add new perk */
    add_char_perk(ch, perk_id, class_id);
    send_to_char(ch, "\tGYou have learned: %s (Rank 1/%d)!\tn\r\n",
                perk->name, perk->max_rank);
  }
  
  /* Save character */
  save_char(ch, 0);
  
  return TRUE;
}

/**
 * Add a perk to a character's perk list.
 * Creates a new char_perk_data entry with rank 1.
 * 
 * @param ch The character
 * @param perk_id The perk ID to add
 * @param class_id The class to add it for
 */
void add_char_perk(struct char_data *ch, int perk_id, int class_id)
{
  struct char_perk_data *new_perk;
  
  if (!ch || IS_NPC(ch))
    return;
  
  if (class_id < 0 || class_id >= NUM_CLASSES)
    return;
  
  /* Check if already exists */
  if (find_char_perk(ch, perk_id, class_id))
    return;
  
  /* Create new perk entry */
  CREATE(new_perk, struct char_perk_data, 1);
  new_perk->perk_id = perk_id;
  new_perk->perk_class = class_id;
  new_perk->current_rank = 1;
  
  /* Add to front of list */
  new_perk->next = ch->player_specials->saved.perks;
  ch->player_specials->saved.perks = new_perk;
  
  /* If toggleable, default to toggled ON */
  struct perk_data *perk_def = get_perk_by_id(perk_id);
  if (perk_def && perk_def->toggleable)
  {
    set_perk_toggle(ch, perk_id, true);
  }
}

/* Old remove_char_perk moved to refund section below */

/**
 * Count the total number of perks a character has purchased.
 * 
 * @param ch The character
 * @return Total number of perk entries
 */
int count_char_perks(struct char_data *ch)
{
  struct char_perk_data *perk;
  int count = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  for (perk = ch->player_specials->saved.perks; perk; perk = perk->next)
    count++;
  
  return count;
}

/*****************************************************************************
 * PERK EFFECTS SYSTEM (Step 6)
 * These functions apply perk bonuses to character statistics.
 * Called from various game systems (combat, saves, skills, etc.)
 *****************************************************************************/

/**
 * Apply all perk effects to a character.
 * This is called when a character's stats need to be recalculated.
 * Note: For most effects, we use getter functions called at calculation time.
 * This function handles permanent modifications (like max HP from Toughness).
 * 
 * @param ch The character
 */
void apply_all_perk_effects(struct char_data *ch)
{
  struct char_perk_data *char_perk;
  struct perk_data *perk;
  int hp_bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return;
  
  /* Calculate HP bonuses from perks like Toughness */
  for (char_perk = ch->player_specials->saved.perks; char_perk; char_perk = char_perk->next)
  {
    perk = get_perk_by_id(char_perk->perk_id);
    if (!perk)
      continue;
    
    /* Apply HP bonuses */
    if (perk->effect_type == PERK_EFFECT_HP)
    {
      hp_bonus += perk->effect_value * char_perk->current_rank;
    }
  }
  
  /* Note: HP bonus is applied via get_perk_hp_bonus() in limits.c */
  /* Other effects are applied via their respective getter functions */
}

/**
 * Get the total bonus from perks for a specific effect type and modifier.
 * This is a generic function that sums all matching perk effects.
 * 
 * @param ch The character
 * @param effect_type The type of effect (PERK_EFFECT_*)
 * @param effect_modifier Additional modifier (skill num, save type, etc.) or -1 for any
 * @return Total bonus from all matching perks
 */
int get_perk_bonus(struct char_data *ch, int effect_type, int effect_modifier)
{
  struct char_perk_data *char_perk;
  struct perk_data *perk;
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  for (char_perk = ch->player_specials->saved.perks; char_perk; char_perk = char_perk->next)
  {
    perk = get_perk_by_id(char_perk->perk_id);
    if (!perk)
      continue;
    
    /* Check if this perk matches the effect type */
    if (perk->effect_type != effect_type)
      continue;
    
    /* Check if modifier matches (if specified) */
    if (effect_modifier >= 0 && perk->effect_modifier != effect_modifier)
      continue;
    
    /* Add the bonus (effect_value * current_rank) */
    bonus += perk->effect_value * char_perk->current_rank;
  }
  
  return bonus;
}

/**
 * Get HP bonus from all perks.
 * Used by hit point calculation functions.
 * 
 * @param ch The character
 * @return Total HP bonus
 */
int get_perk_hp_bonus(struct char_data *ch)
{
  return get_perk_bonus(ch, PERK_EFFECT_HP, -1);
}

/**
 * Get spell points bonus from all perks.
 * Used by spell point calculation functions.
 * 
 * @param ch The character
 * @return Total spell points bonus
 */
int get_perk_spell_points_bonus(struct char_data *ch)
{
  return get_perk_bonus(ch, PERK_EFFECT_SPELL_POINTS, -1);
}

/**
 * Get AC bonus from all perks.
 * Used by armor class calculation.
 * 
 * @param ch The character
 * @return Total AC bonus
 */
int get_perk_ac_bonus(struct char_data *ch)
{
  return get_perk_bonus(ch, PERK_EFFECT_AC, -1);
}

/**
 * Get saving throw bonus from perks for a specific save type.
 * 
 * @param ch The character
 * @param save_type The save type (SAVING_FORT, SAVING_REFL, SAVING_WILL)
 * @return Total save bonus
 */
int get_perk_save_bonus(struct char_data *ch, int save_type)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Get bonuses for this specific save type */
  bonus += get_perk_bonus(ch, PERK_EFFECT_SAVE, save_type);
  
  /* Also add universal save bonuses (effect_modifier = -1) */
  bonus += get_perk_bonus(ch, PERK_EFFECT_SAVE, -1);
  
  return bonus;
}

/**
 * Get skill bonus from perks for a specific skill.
 * 
 * @param ch The character
 * @param skill_num The skill number
 * @return Total skill bonus
 */
int get_perk_skill_bonus(struct char_data *ch, int skill_num)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Get bonuses for this specific skill */
  bonus += get_perk_bonus(ch, PERK_EFFECT_SKILL, skill_num);
  
  /* Also add universal skill bonuses (effect_modifier = -1) */
  bonus += get_perk_bonus(ch, PERK_EFFECT_SKILL, -1);
  
  return bonus;
}

/**
 * Get weapon damage bonus from perks for melee attacks.
 * Only applies if character is unarmed or wielding a melee weapon (not ranged).
 * 
 * @param ch The character
 * @param wielded The wielded weapon (can be NULL for unarmed)
 * @return Total weapon damage bonus
 */
int get_perk_weapon_damage_bonus(struct char_data *ch, struct obj_data *wielded)
{
  if (!ch || IS_NPC(ch))
    return 0;
    
  /* If wielded is NULL, character is unarmed - apply bonus */
  if (!wielded)
    return get_perk_bonus(ch, PERK_EFFECT_WEAPON_DAMAGE, -1);
  
  /* Check if the weapon is ranged - if so, don't apply bonus */
  if (GET_OBJ_TYPE(wielded) == ITEM_WEAPON || GET_OBJ_TYPE(wielded) == ITEM_FIREWEAPON)
  {
    int weapon_type = GET_OBJ_VAL(wielded, 0);
    if (IS_SET(weapon_list[weapon_type].weaponFlags, WEAPON_FLAG_RANGED))
      return 0; /* No bonus for ranged weapons */
  }
  
  /* Weapon is melee, apply bonus */
  return get_perk_bonus(ch, PERK_EFFECT_WEAPON_DAMAGE, -1);
}

/**
 * Get weapon to-hit bonus from perks for melee attacks.
 * Only applies if character is unarmed or wielding a melee weapon (not ranged).
 * 
 * @param ch The character
 * @param wielded The wielded weapon (can be NULL for unarmed)
 * @return Total weapon to-hit bonus
 */
int get_perk_weapon_tohit_bonus(struct char_data *ch, struct obj_data *wielded)
{
  if (!ch || IS_NPC(ch))
    return 0;
    
  /* If wielded is NULL, character is unarmed - apply bonus */
  if (!wielded)
    return get_perk_bonus(ch, PERK_EFFECT_WEAPON_TOHIT, -1);
  
  /* Check if the weapon is ranged - if so, don't apply bonus */
  if (GET_OBJ_TYPE(wielded) == ITEM_WEAPON || GET_OBJ_TYPE(wielded) == ITEM_FIREWEAPON)
  {
    int weapon_type = GET_OBJ_VAL(wielded, 0);
    if (IS_SET(weapon_list[weapon_type].weaponFlags, WEAPON_FLAG_RANGED))
      return 0; /* No bonus for ranged weapons */
  }
  
  /* Weapon is melee, apply bonus */
  return get_perk_bonus(ch, PERK_EFFECT_WEAPON_TOHIT, -1);
}

/*****************************************************************************
 * Step 7: Perk OLC Interface - Player Commands
 *****************************************************************************/

/* Save type names for display */
static const char *save_type_names[] = {
  "Fortitude",
  "Reflex", 
  "Will",
  "Poison",
  "Death"
};

/**
 * Display a single perk's details to the player.
 * 
 * @param ch The character viewing the perk
 * @param perk The perk to display
 * @param char_perk The character's current rank in this perk (can be NULL)
 */
void display_perk_details(struct char_data *ch, struct perk_data *perk, struct char_perk_data *char_perk)
{
  int current_rank = char_perk ? char_perk->current_rank : 0;
  int class_id = perk->associated_class;
  
  send_to_char(ch, "\tc%s\tn\r\n", perk->name);
  send_to_char(ch, "Class: \tW%s\tn\r\n", class_names[class_id]);
  send_to_char(ch, "Cost: \tY%d\tn perk point%s per rank\r\n", perk->cost, perk->cost != 1 ? "s" : "");
  send_to_char(ch, "Max Ranks: \tC%d\tn\r\n", perk->max_rank);
  send_to_char(ch, "Current Rank: \tG%d\tn\r\n", current_rank);
  
  if (perk->prerequisite_perk >= 0)
  {
    struct perk_data *prereq = get_perk_by_id(perk->prerequisite_perk);
    if (prereq)
      send_to_char(ch, "Prerequisite: \tR%s\tn (Rank %d)\r\n", prereq->name, perk->prerequisite_rank);
  }
  
  send_to_char(ch, "\r\nDescription:\r\n%s\r\n", perk->description);
  
  /* Show toggleable status if applicable */
  if (perk->toggleable)
  {
    send_to_char(ch, "\r\n\tYThis perk can be toggled on/off.\tn\r\n");
    if (char_perk)
    {
      bool is_on = is_perk_toggled_on(ch, perk->id);
      send_to_char(ch, "Current status: %s%s\tn\r\n", 
                   is_on ? "\tG" : "\tr",
                   is_on ? "ON" : "OFF");
      send_to_char(ch, "Use '\tcperk toggle %d\tn' to toggle this perk.\r\n", perk->id);
    }
  }
  
  /* Show effect - only if there is one */
  if (perk->effect_type != PERK_EFFECT_NONE)
  {
    send_to_char(ch, "\r\nEffect: ");
    switch (perk->effect_type)
    {
      case PERK_EFFECT_HP:
        send_to_char(ch, "\tG+%d\tn Hit Points per rank\r\n", perk->effect_value);
        break;
      case PERK_EFFECT_SPELL_POINTS:
        send_to_char(ch, "\tG+%d\tn Spell Points per rank\r\n", perk->effect_value);
        break;
      case PERK_EFFECT_AC:
        send_to_char(ch, "\tG+%d\tn Armor Class per rank\r\n", perk->effect_value);
        break;
      case PERK_EFFECT_ABILITY_SCORE:
        if (perk->effect_modifier >= 0 && perk->effect_modifier < 6)
          send_to_char(ch, "\tG+%d\tn to %s per rank\r\n", perk->effect_value,
                       ability_score_names[perk->effect_modifier]);
        break;
      case PERK_EFFECT_SAVE:
        if (perk->effect_modifier == -1)
          send_to_char(ch, "\tG+%d\tn to all saves per rank\r\n", perk->effect_value);
        else if (perk->effect_modifier >= 0 && perk->effect_modifier < NUM_OF_SAVING_THROWS)
          send_to_char(ch, "\tG+%d\tn to %s save per rank\r\n", perk->effect_value, 
                       save_type_names[perk->effect_modifier]);
        break;
      case PERK_EFFECT_SKILL:
        if (perk->effect_modifier == -1)
          send_to_char(ch, "\tG+%d\tn to all skills per rank\r\n", perk->effect_value);
        else if (perk->effect_modifier >= 0 && perk->effect_modifier < NUM_ABILITIES)
          send_to_char(ch, "\tG+%d\tn to %s per rank\r\n", perk->effect_value,
                       ability_names[perk->effect_modifier]);
        break;
      case PERK_EFFECT_WEAPON_DAMAGE:
        send_to_char(ch, "\tG+%d\tn to melee weapon damage per rank\r\n", perk->effect_value);
        break;
      case PERK_EFFECT_WEAPON_TOHIT:
        send_to_char(ch, "\tG+%d\tn to melee weapon to-hit per rank\r\n", perk->effect_value);
        break;
      case PERK_EFFECT_SPELL_DC:
        send_to_char(ch, "\tG+%d\tn to spell save DC per rank\r\n", perk->effect_value);
        break;
      case PERK_EFFECT_SPELL_DAMAGE:
        send_to_char(ch, "\tG+%d\tn to spell damage per rank\r\n", perk->effect_value);
        break;
      case PERK_EFFECT_SPELL_DURATION:
        send_to_char(ch, "\tG+%d\tn rounds to spell duration per rank\r\n", perk->effect_value);
        break;
      case PERK_EFFECT_CASTER_LEVEL:
        send_to_char(ch, "\tG+%d\tn to effective caster level per rank\r\n", perk->effect_value);
        break;
      case PERK_EFFECT_DAMAGE_REDUCTION:
        send_to_char(ch, "\tG%d/-\tn damage reduction per rank\r\n", perk->effect_value);
        break;
      case PERK_EFFECT_SPELL_RESISTANCE:
        send_to_char(ch, "\tG+%d\tn spell resistance per rank\r\n", perk->effect_value);
        break;
      case PERK_EFFECT_CRITICAL_MULT:
        send_to_char(ch, "\tG+%d\tn to critical multiplier per rank\r\n", perk->effect_value);
        break;
      case PERK_EFFECT_CRITICAL_CHANCE:
        send_to_char(ch, "\tG+%d\tn to critical threat range per rank\r\n", perk->effect_value);
        break;
      case PERK_EFFECT_SPECIAL:
        if (perk->special_description && *perk->special_description)
          send_to_char(ch, "%s\r\n", perk->special_description);
        else
          send_to_char(ch, "Special effect (see description)\r\n");
        break;
      default:
        /* Don't display anything for unknown effects */
        break;
    }
  }
  
  /* Show if purchasable */
  if (current_rank < perk->max_rank)
  {
    char error_msg[256];
    if (can_purchase_perk(ch, perk->id, class_id, error_msg, sizeof(error_msg)))
    {
      send_to_char(ch, "\r\n\tGYou can purchase this perk!\tn\r\n");
      send_to_char(ch, "Use: \tcperk buy %d\tn\r\n", perk->id);
    }
    else
    {
      send_to_char(ch, "\r\n\tRCannot purchase: %s\tn\r\n", error_msg);
    }
  }
  else
  {
    send_to_char(ch, "\r\n\tYYou have maximum ranks in this perk.\tn\r\n");
  }
}

/**
 * Comparison function for sorting perks alphabetically by name.
 */
static int compare_perks_by_name(const void *a, const void *b)
{
  int perk_id_a = *(const int *)a;
  int perk_id_b = *(const int *)b;
  struct perk_data *perk_a = get_perk_by_id(perk_id_a);
  struct perk_data *perk_b = get_perk_by_id(perk_id_b);
  
  if (!perk_a || !perk_b)
    return 0;
  
  return strcasecmp(perk_a->name, perk_b->name);
}

/**
 * Check if character meets prerequisites for a perk (but hasn't purchased it).
 * This is used to highlight perks that are available to purchase.
 * 
 * @param ch The character
 * @param perk_id The perk to check
 * @param class_id The class 
 * @return TRUE if prerequisites are met but perk not yet purchased
 */
static bool meets_prerequisites_not_purchased(struct char_data *ch, int perk_id, int class_id)
{
  struct perk_data *perk;
  char error_msg[256];
  
  /* If they already have the perk at max rank, don't highlight */
  int current_rank = get_perk_rank(ch, perk_id, class_id);
  perk = get_perk_by_id(perk_id);
  if (perk && current_rank >= perk->max_rank)
    return FALSE;
  
  /* Check if they can purchase it (meets all requirements) */
  return can_purchase_perk(ch, perk_id, class_id, error_msg, sizeof(error_msg));
}

/**
 * List all perks available for a specific class.
 * 
 * @param ch The character viewing perks
 * @param class_id The class to show perks for
 */
void list_perks_for_class(struct char_data *ch, int class_id)
{
  int perk_ids[NUM_PERKS];
  int count, i;
  struct perk_data *perk;
  struct char_perk_data *char_perk;
  
  count = get_class_perks(class_id, perk_ids, NUM_PERKS);
  
  if (count == 0)
  {
    send_to_char(ch, "No perks available for %s.\r\n", class_names[class_id]);
    return;
  }
  
  /* Sort perks alphabetically by name */
  qsort(perk_ids, count, sizeof(int), compare_perks_by_name);
  
  send_to_char(ch, "\tc%s Perks\tn\r\n", class_names[class_id]);
  send_to_char(ch, "Available Perk Points: \tY%d\tn\r\n\r\n", get_perk_points(ch, class_id));
  
  /* Two column header */
  send_to_char(ch, "\tW%-3s %-35s %s/%s  %-3s %-35s %s/%s\tn\r\n", 
               "ID", "Name", "Rnk", "Max", "ID", "Name", "Rnk", "Max");
  send_to_char(ch, "--- ----------------------------------- --- ---  --- ----------------------------------- --- ---\r\n");
  send_to_char(ch, "\tW* = Available to purchase\tn\r\n\r\n");
  
  /* Display in two columns */
  for (i = 0; i < count; i += 2)
  {
    /* Left column */
    perk = get_perk_by_id(perk_ids[i]);
    if (!perk)
      continue;
      
    char_perk = find_char_perk(ch, perk_ids[i], class_id);
    int current_rank = char_perk ? char_perk->current_rank : 0;
    bool can_purchase_left = meets_prerequisites_not_purchased(ch, perk_ids[i], class_id);
    
    char left_col[200];
    char perk_name_left[60];
    if (can_purchase_left)
    {
      /* Space + asterisk (1 char) + name (33 chars) = 35 visible total */
      snprintf(perk_name_left, sizeof(perk_name_left), " \tC*%-33.33s\tn", perk->name);
    }
    else
    {
      snprintf(perk_name_left, sizeof(perk_name_left), "\tn %-33.33s\tn", perk->name);
    }
    
    snprintf(left_col, sizeof(left_col), "%-3d %-35.35s\tn %s%3d\tn/%s%-3d\tn",
             perk->id,
             perk_name_left,
             current_rank > 0 ? "\tG" : "",
             current_rank,
             current_rank >= perk->max_rank ? "\tY" : "",
             perk->max_rank);
    
    /* Right column (if exists) */
    if (i + 1 < count)
    {
      perk = get_perk_by_id(perk_ids[i + 1]);
      if (perk)
      {
        char_perk = find_char_perk(ch, perk_ids[i + 1], class_id);
        current_rank = char_perk ? char_perk->current_rank : 0;
        bool can_purchase_right = meets_prerequisites_not_purchased(ch, perk_ids[i + 1], class_id);
        
        char perk_name_right[60];
        if (can_purchase_right)
        {
          /* Space + asterisk (1 char) + name (33 chars) = 35 visible total */
          snprintf(perk_name_right, sizeof(perk_name_right), " \tC*%-33.33s\tn  ", perk->name);
        }
        else
        {
          snprintf(perk_name_right, sizeof(perk_name_right), " \tn%-33.33s\tn", perk->name);
        }
        
        send_to_char(ch, "%s %-3d %-35.35s\tn %s%3d\tn/%s%-3d\tn\r\n",
                     left_col,
                     perk->id,
                     perk_name_right,
                     current_rank > 0 ? "\tG" : "",
                     current_rank,
                     current_rank >= perk->max_rank ? "\tY" : "",
                     perk->max_rank);
      }
      else
      {
        send_to_char(ch, "%s\r\n", left_col);
      }
    }
    else
    {
      send_to_char(ch, "%s\r\n", left_col);
    }
  }
  
  send_to_char(ch, "\r\nUse '\tcperk info <id>\tn' to see details about a perk.\r\n");
  send_to_char(ch, "Use '\tcperk buy <id>\tn' to purchase a perk.\r\n");
}

/**
 * List all perks the character currently has.
 * 
 * @param ch The character
 */
void list_my_perks(struct char_data *ch)
{
  struct char_perk_data *char_perk;
  struct perk_data *perk;
  int count = 0;
  
  send_to_char(ch, "\tcYour Purchased Perks\tn\r\n\r\n");
  send_to_char(ch, "\tW%-30s %-15s %-6s %-8s\tn\r\n", "Name", "Class", "Rank", "Toggle");
  send_to_char(ch, "------------------------------ --------------- ------ --------\r\n");
  
  for (char_perk = ch->player_specials->saved.perks; char_perk; char_perk = char_perk->next)
  {
    perk = get_perk_by_id(char_perk->perk_id);
    if (!perk)
      continue;
    
    /* Show toggle status for toggleable perks */
    char toggle_str[20];
    if (perk->toggleable)
    {
      if (is_perk_toggled_on(ch, char_perk->perk_id))
        snprintf(toggle_str, sizeof(toggle_str), "\tG%-8s\tn", "ON");
      else
        snprintf(toggle_str, sizeof(toggle_str), "\tr%-8s\tn", "OFF");
    }
    else
    {
      snprintf(toggle_str, sizeof(toggle_str), "%-8s", "-");
    }
      
    send_to_char(ch, "%-30s %-15s \tG%-6d\tn %s\r\n",
                 perk->name,
                 class_names[perk->associated_class],
                 char_perk->current_rank,
                 toggle_str);
    count++;
  }
  
  if (count == 0)
  {
    send_to_char(ch, "You have not purchased any perks yet.\r\n");
  }
  
  send_to_char(ch, "\r\nTotal perks purchased: %d\r\n", count);
  send_to_char(ch, "Use 'perk toggle <name>' to toggle toggleable perks on/off.\r\n");
}

/**
 * Main perk command - handles viewing and purchasing perks.
 * 
 * Syntax:
 *   perk                    - Show perks for current class
 *   perk <class>            - Show perks for specific class
 *   perk info <id>          - Show details for a specific perk
 *   perk buy <id>           - Purchase a perk
 *   perk list               - Show all classes with perks
 */
ACMD(do_perk)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int class_id, perk_id;
  struct perk_data *perk;
  struct char_perk_data *char_perk;
  char error_msg[256];
  
  if (IS_NPC(ch))
  {
    send_to_char(ch, "NPCs cannot use perks.\r\n");
    return;
  }
  
  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
  
  /* No arguments - show perks for primary class */
  if (!*arg1)
  {
    /* Find first class with available perks or levels */
    int found_class = -1;
    int total_classes = 0;
    
    for (class_id = 0; class_id < NUM_CLASSES; class_id++)
    {
      if (CLASS_LEVEL(ch, class_id) > 0)
      {
        total_classes++;
        if (found_class == -1)
          found_class = class_id;
      }
    }
    
    if (found_class == -1)
    {
      send_to_char(ch, "You don't have any class levels yet.\r\n");
      return;
    }
    
    list_perks_for_class(ch, found_class);
    
    /* If multiple classes, suggest using perk list */
    if (total_classes > 1)
    {
      send_to_char(ch, "\r\n\tcYou have levels in multiple classes.\tn\r\n");
      send_to_char(ch, "Use '\tcperk list\tn' to see all your classes and perk points.\r\n");
      send_to_char(ch, "Use '\tcperk <class>\tn' to view perks for a specific class.\r\n");
    }
    
    return;
  }
  
  /* perk list - show all available classes */
  if (!strcmp(arg1, "list"))
  {
    send_to_char(ch, "\tcAvailable Perk Classes\tn\r\n\r\n");
    send_to_char(ch, "Your classes and available perk points:\r\n");
    
    for (class_id = 0; class_id < NUM_CLASSES; class_id++)
    {
      if (CLASS_LEVEL(ch, class_id) > 0)
      {
        int points = get_perk_points(ch, class_id);
        send_to_char(ch, "  %-15s (Level %2d): \tY%d\tn perk point%s\r\n",
                     class_names[class_id],
                     CLASS_LEVEL(ch, class_id),
                     points,
                     points != 1 ? "s" : "");
      }
    }
    
    send_to_char(ch, "\r\nUse '\tcperk <class>\tn' to view perks for a specific class.\r\n");
    return;
  }
  
  /* perk info <id> - show perk details */
  if (!strcmp(arg1, "info"))
  {
    if (!*arg2)
    {
      send_to_char(ch, "Usage: perk info <perk id>\r\n");
      return;
    }
    
    perk_id = atoi(arg2);
    perk = get_perk_by_id(perk_id);
    
    if (!perk)
    {
      send_to_char(ch, "Invalid perk ID.\r\n");
      return;
    }
    
    char_perk = find_char_perk(ch, perk_id, perk->associated_class);
    display_perk_details(ch, perk, char_perk);
    return;
  }
  
  /* perk buy <id> - purchase a perk */
  if (!strcmp(arg1, "buy") || !strcmp(arg1, "purchase"))
  {
    if (!*arg2)
    {
      send_to_char(ch, "Usage: perk buy <perk id>\r\n");
      return;
    }
    
    perk_id = atoi(arg2);
    perk = get_perk_by_id(perk_id);
    
    if (!perk)
    {
      send_to_char(ch, "Invalid perk ID.\r\n");
      return;
    }
    
    class_id = perk->associated_class;
    
    /* Check if can purchase */
    if (!can_purchase_perk(ch, perk_id, class_id, error_msg, sizeof(error_msg)))
    {
      send_to_char(ch, "\tRCannot purchase perk: %s\tn\r\n", error_msg);
      return;
    }
    
    /* Purchase the perk */
    if (purchase_perk(ch, perk_id, class_id))
    {
      char_perk = find_char_perk(ch, perk_id, class_id);
      send_to_char(ch, "\tGYou have purchased rank %d of '%s'!\tn\r\n",
                   char_perk->current_rank, perk->name);
      send_to_char(ch, "Remaining perk points for %s: \tY%d\tn\r\n",
                   class_names[class_id], get_perk_points(ch, class_id));
      
      /* Recalculate stats to apply new perk */
      affect_total(ch);
    }
    else
    {
      send_to_char(ch, "\tRFailed to purchase perk.\tn\r\n");
    }
    return;
  }
  
  /* perk toggle [name] - toggle a perk on/off */
  if (!strcmp(arg1, "toggle"))
  {
    /* No perk specified - show list of toggleable perks */
    if (!*arg2)
    {
      struct char_perk_data *char_perk;
      struct perk_data *perk_def;
      int count = 0;
      
      send_to_char(ch, "\tcYour Toggleable Perks\tn\r\n\r\n");
      send_to_char(ch, "\tW%-4s %-30s %-8s\tn\r\n", "ID", "Name", "Status");
      send_to_char(ch, "---- ------------------------------ --------\r\n");
      
      for (char_perk = ch->player_specials->saved.perks; char_perk; char_perk = char_perk->next)
      {
        perk_def = get_perk_by_id(char_perk->perk_id);
        if (!perk_def || !perk_def->toggleable)
          continue;
        
        send_to_char(ch, "%-4d %-30s %s%-8s\tn\r\n",
                     char_perk->perk_id,
                     perk_def->name,
                     is_perk_toggled_on(ch, char_perk->perk_id) ? "\tG" : "\tr",
                     is_perk_toggled_on(ch, char_perk->perk_id) ? "ON" : "OFF");
        count++;
      }
      
      if (count == 0)
      {
        send_to_char(ch, "You have no toggleable perks.\r\n");
      }
      
      send_to_char(ch, "\r\nUse 'perk toggle <name>' to toggle a perk on or off.\r\n");
      return;
    }
    
    /* Toggle the specified perk */
    int perk_id = -1;
    struct perk_data *perk_def;
    bool current_state, new_state;
    
    /* Try to find the perk by number first */
    if (is_number(arg2))
    {
      perk_id = atoi(arg2);
      if (perk_id < 0 || perk_id >= NUM_PERKS)
      {
        send_to_char(ch, "Invalid perk number.\r\n");
        return;
      }
    }
    else
    {
      /* Search by name */
      int i;
      for (i = 0; i < NUM_PERKS; i++)
      {
        if (perk_list[i].id == PERK_UNDEFINED)
          continue;
        if (is_abbrev(arg2, perk_list[i].name))
        {
          perk_id = i;
          break;
        }
      }
      
      if (perk_id == -1)
      {
        send_to_char(ch, "No such perk found.\r\n");
        return;
      }
    }
    
    /* Check if character has this perk */
    if (!has_perk(ch, perk_id))
    {
      send_to_char(ch, "You don't have that perk.\r\n");
      return;
    }
    
    /* Get perk definition */
    perk_def = get_perk_by_id(perk_id);
    if (!perk_def)
    {
      send_to_char(ch, "Error: Invalid perk.\r\n");
      return;
    }
    
    /* Check if toggleable */
    if (!perk_def->toggleable)
    {
      send_to_char(ch, "The perk '%s' cannot be toggled on or off.\r\n", perk_def->name);
      return;
    }
    
    /* Toggle it */
    current_state = is_perk_toggled_on(ch, perk_id);
    new_state = !current_state;
    set_perk_toggle(ch, perk_id, new_state);
    
    send_to_char(ch, "You have toggled '%s' %s%s%s.\r\n", 
                 perk_def->name,
                 new_state ? "\tG" : "\tr",
                 new_state ? "ON" : "OFF",
                 "\tn");
    
    if (new_state)
      send_to_char(ch, "The perk's effects are now active.\r\n");
    else
      send_to_char(ch, "The perk's effects are now inactive.\r\n");
    
    /* Save character */
    save_char(ch, 0);
    return;
  }
  
  /* Try to parse as a class name */
  class_id = parse_class_long(arg1);
  
  if (class_id == CLASS_UNDEFINED)
  {
    send_to_char(ch, "Unknown class or command.\r\n");
    send_to_char(ch, "Usage:\r\n");
    send_to_char(ch, "  perk               - Show perks for your current class\r\n");
    send_to_char(ch, "  perk list          - Show all your classes\r\n");
    send_to_char(ch, "  perk <class>       - Show perks for a specific class\r\n");
    send_to_char(ch, "  perk info <id>     - Show details for a perk\r\n");
    send_to_char(ch, "  perk buy <id>      - Purchase a perk\r\n");
    send_to_char(ch, "  perk toggle [name] - Toggle a perk on/off (or list toggleable perks)\r\n");
    return;
  }
  
  /* Check if character has this class */
  if (CLASS_LEVEL(ch, class_id) == 0)
  {
    send_to_char(ch, "You don't have any levels in %s.\r\n", class_names[class_id]);
    return;
  }
  
  list_perks_for_class(ch, class_id);
}

/**
 * Command to show character's purchased perks.
 * 
 * Syntax: myperks
 */
ACMD(do_myperks)
{
  if (IS_NPC(ch))
  {
    send_to_char(ch, "NPCs cannot use perks.\r\n");
    return;
  }
  
  list_my_perks(ch);
}

/*****************************************************************************
 * PERK REFUND/RESET FUNCTIONS (Step 10)
 * These functions handle refunding perks during respec.
 *****************************************************************************/ 

/**
 * Remove a specific perk from a character.
 * Refunds the perk points to the class pool.
 * 
 * @param ch The character
 * @param perk_id The perk to remove
 * @param class_id The class the perk was purchased for
 * @return TRUE if removed, FALSE if not found
 */
bool remove_char_perk(struct char_data *ch, int perk_id, int class_id)
{
  struct char_perk_data *perk, *prev = NULL;
  struct perk_data *perk_def;
  
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  /* Find and remove the perk from the linked list */
  for (perk = ch->player_specials->saved.perks; perk != NULL; prev = perk, perk = perk->next)
  {
    if (perk->perk_id == perk_id && perk->perk_class == class_id)
    {
      /* Found it - remove from list */
      if (prev)
        prev->next = perk->next;
      else
        ch->player_specials->saved.perks = perk->next;
      
      /* Refund perk points */
      perk_def = get_perk_by_id(perk_id);
      if (perk_def)
      {
        int points_to_refund = perk_def->cost * perk->current_rank;
        add_perk_points(ch, class_id, points_to_refund);
      }
      
      /* Free memory */
      free(perk);
      
      return TRUE;
    }
  }
  
  return FALSE;
}

/**
 * Remove all perks for a specific class.
 * Refunds all perk points to the class pool.
 * 
 * @param ch The character
 * @param class_id The class to remove perks for
 */
void remove_class_perks(struct char_data *ch, int class_id)
{
  struct char_perk_data *perk, *next_perk, *prev = NULL;
  struct perk_data *perk_def;
  int total_refunded = 0;
  
  if (!ch || IS_NPC(ch))
    return;
  
  perk = ch->player_specials->saved.perks;
  
  while (perk != NULL)
  {
    next_perk = perk->next;
    
    if (perk->perk_class == class_id)
    {
      /* Remove from list */
      if (prev)
        prev->next = next_perk;
      else
        ch->player_specials->saved.perks = next_perk;
      
      /* Calculate refund */
      perk_def = get_perk_by_id(perk->perk_id);
      if (perk_def)
      {
        total_refunded += perk_def->cost * perk->current_rank;
      }
      
      /* Free memory */
      free(perk);
    }
    else
    {
      prev = perk;
    }
    
    perk = next_perk;
  }
  
  /* Refund all points */
  if (total_refunded > 0)
  {
    add_perk_points(ch, class_id, total_refunded);
  }
}

/**
 * Remove ALL perks from a character.
 * Used during full respec - doesn't refund points since they'll be reset anyway.
 * 
 * @param ch The character
 */
void remove_all_perks(struct char_data *ch)
{
  struct char_perk_data *perk, *next_perk;
  
  if (!ch || IS_NPC(ch))
    return;
  
  perk = ch->player_specials->saved.perks;
  
  while (perk != NULL)
  {
    next_perk = perk->next;
    free(perk);
    perk = next_perk;
  }
  
  ch->player_specials->saved.perks = NULL;
  
  /* Clear all perk toggles when removing all perks */
  memset(ch->player_specials->saved.perk_toggles, 0, 32);
}

/**
 * Reset all perk points to zero for all classes.
 * Used during respec to clear out all points.
 * 
 * @param ch The character
 */
void reset_all_perk_points(struct char_data *ch)
{
  int i;
  
  if (!ch || IS_NPC(ch))
    return;
  
  for (i = 0; i < NUM_CLASSES; i++)
  {
    ch->player_specials->saved.perk_points[i] = 0;
  }
}

