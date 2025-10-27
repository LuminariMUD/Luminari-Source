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

/* Undefine NUM_ABILITIES before including spells.h to avoid redefinition warning */
#undef NUM_ABILITIES

#include "spells.h"

/* External function from class.c */
extern int level_exp(struct char_data *ch, int level);
#include "interpreter.h"
#include "constants.h"
#include "perks.h"

/* Forward declarations */
static void define_wizard_controller_perks(void);
static void define_wizard_versatile_caster_perks(void);

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
  
  /* Define Wizard Controller Perks */
  define_wizard_controller_perks();
  
  /* Define Wizard Versatile Caster Perks */
  define_wizard_versatile_caster_perks();
  
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
  perk->cost = 2;
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
  perk->cost = 3;
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
  perk->cost = 4;
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
  
  /*** TREE 3: TACTICAL FIGHTER - TIER I ***/
  
  /* Combat Reflexes I */
  perk = &perk_list[PERK_FIGHTER_COMBAT_REFLEXES_1];
  perk->id = PERK_FIGHTER_COMBAT_REFLEXES_1;
  perk->name = strdup("Combat Reflexes I");
  perk->description = strdup("+1 attack of opportunity per round per rank (max 3 ranks)");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("+1 attack of opportunity per round per rank");
  
  /* Improved Initiative I */
  perk = &perk_list[PERK_FIGHTER_IMPROVED_INITIATIVE_1];
  perk->id = PERK_FIGHTER_IMPROVED_INITIATIVE_1;
  perk->name = strdup("Improved Initiative I");
  perk->description = strdup("+2 initiative per rank (max 3 ranks)");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("+2 to initiative per rank");
  
  /* Mobility I */
  perk = &perk_list[PERK_FIGHTER_MOBILITY_1];
  perk->id = PERK_FIGHTER_MOBILITY_1;
  perk->name = strdup("Mobility I");
  perk->description = strdup("+2 AC vs attacks of opportunity per rank (max 2 ranks)");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 1;
  perk->max_rank = 2;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("+2 AC vs attacks of opportunity per rank");
  
  /*** TREE 3: TACTICAL FIGHTER - TIER II ***/
  
  /* Combat Reflexes II */
  perk = &perk_list[PERK_FIGHTER_COMBAT_REFLEXES_2];
  perk->id = PERK_FIGHTER_COMBAT_REFLEXES_2;
  perk->name = strdup("Combat Reflexes II");
  perk->description = strdup("Additional +1 attack of opportunity per round per rank (max 2 ranks)");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_FIGHTER_COMBAT_REFLEXES_1;
  perk->prerequisite_rank = 3; /* Requires Combat Reflexes I at max rank */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Combat Reflexes I at max rank (3)");
  
  /* Improved Trip */
  perk = &perk_list[PERK_FIGHTER_IMPROVED_TRIP];
  perk->id = PERK_FIGHTER_IMPROVED_TRIP;
  perk->name = strdup("Improved Trip");
  perk->description = strdup("+4 to trip attempts, enemies provoke AoO when standing");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_FIGHTER_COMBAT_REFLEXES_1;
  perk->prerequisite_rank = 1; /* Requires at least 1 rank of Combat Reflexes I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4;
  perk->effect_modifier = 0;
  perk->special_description = strdup("+4 bonus to trip combat maneuvers, enemies provoke AoO when standing up");
  
  /* Improved Disarm */
  perk = &perk_list[PERK_FIGHTER_IMPROVED_DISARM];
  perk->id = PERK_FIGHTER_IMPROVED_DISARM;
  perk->name = strdup("Improved Disarm");
  perk->description = strdup("+4 to disarm attempts, no AoO provoked");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4;
  perk->effect_modifier = 0;
  perk->special_description = strdup("+4 bonus to disarm combat maneuvers, does not provoke attacks of opportunity");
  
  /* Improved Sunder */
  perk = &perk_list[PERK_FIGHTER_IMPROVED_SUNDER];
  perk->id = PERK_FIGHTER_IMPROVED_SUNDER;
  perk->name = strdup("Improved Sunder");
  perk->description = strdup("+4 to sunder attempts, deal full damage to objects");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4;
  perk->effect_modifier = 0;
  perk->special_description = strdup("+4 bonus to sunder combat maneuvers, deal full damage to objects");
  
  /* Spring Attack */
  perk = &perk_list[PERK_FIGHTER_SPRING_ATTACK];
  perk->id = PERK_FIGHTER_SPRING_ATTACK;
  perk->name = strdup("Spring Attack");
  perk->description = strdup("Can move before and after attacking");
  perk->associated_class = CLASS_WARRIOR;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_FIGHTER_MOBILITY_1;
  perk->prerequisite_rank = 1; /* Requires at least 1 rank of Mobility I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Allows moving before and after attacking without provoking AoO");
  
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
  
  /*** EVOKER TREE - TIER 1 PERKS (1 point each) ***/
  
  /* Spell Power I */
  perk = &perk_list[PERK_WIZARD_SPELL_POWER_1];
  perk->id = PERK_WIZARD_SPELL_POWER_1;
  perk->name = strdup("Spell Power I");
  perk->description = strdup("+1 spell damage per rank");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 1;
  perk->max_rank = 5;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Increases damage dealt by damaging spells by +1 per rank. Can be taken 5 times for +5 damage total.");
  
  /* Energy Affinity: Fire */
  perk = &perk_list[PERK_WIZARD_ENERGY_AFFINITY_FIRE];
  perk->id = PERK_WIZARD_ENERGY_AFFINITY_FIRE;
  perk->name = strdup("Energy Affinity: Fire");
  perk->description = strdup("Fire spells deal +10% damage");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10; /* +10% */
  perk->effect_modifier = 0;
  perk->special_description = strdup("All fire damage spells deal 10% additional damage.");
  
  /* Energy Affinity: Cold */
  perk = &perk_list[PERK_WIZARD_ENERGY_AFFINITY_COLD];
  perk->id = PERK_WIZARD_ENERGY_AFFINITY_COLD;
  perk->name = strdup("Energy Affinity: Cold");
  perk->description = strdup("Cold spells deal +10% damage");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10; /* +10% */
  perk->effect_modifier = 0;
  perk->special_description = strdup("All cold damage spells deal 10% additional damage.");
  
  /* Energy Affinity: Lightning */
  perk = &perk_list[PERK_WIZARD_ENERGY_AFFINITY_LIGHTNING];
  perk->id = PERK_WIZARD_ENERGY_AFFINITY_LIGHTNING;
  perk->name = strdup("Energy Affinity: Lightning");
  perk->description = strdup("Lightning spells deal +10% damage");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10; /* +10% */
  perk->effect_modifier = 0;
  perk->special_description = strdup("All lightning/electricity damage spells deal 10% additional damage.");
  
  /* Spell Penetration I */
  perk = &perk_list[PERK_WIZARD_SPELL_PENETRATION_1];
  perk->id = PERK_WIZARD_SPELL_PENETRATION_1;
  perk->name = strdup("Spell Penetration I");
  perk->description = strdup("+2 to overcome spell resistance per rank");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Bonus to overcome spell resistance. Can be taken 3 times for +6 total.");
  
  /*** EVOKER TREE - TIER 2 PERKS (2 points each) ***/
  
  /* Spell Power II */
  perk = &perk_list[PERK_WIZARD_SPELL_POWER_2];
  perk->id = PERK_WIZARD_SPELL_POWER_2;
  perk->name = strdup("Spell Power II");
  perk->description = strdup("Additional +1 spell damage per rank");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 2;
  perk->max_rank = 3;
  perk->prerequisite_perk = PERK_WIZARD_SPELL_POWER_1;
  perk->prerequisite_rank = 5; /* Must max Spell Power I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Spell Power I (max). Further increases damage by +1 per rank. Can be taken 3 times for +8 total with Spell Power I.");
  
  /* Focused Element: Fire */
  perk = &perk_list[PERK_WIZARD_FOCUSED_ELEMENT_FIRE];
  perk->id = PERK_WIZARD_FOCUSED_ELEMENT_FIRE;
  perk->name = strdup("Focused Element: Fire");
  perk->description = strdup("Fire spells deal +20% damage total, +1 spell DC");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_WIZARD_ENERGY_AFFINITY_FIRE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 20; /* +20% total (replaces +10% from affinity) */
  perk->effect_modifier = 1; /* +1 spell DC */
  perk->special_description = strdup("Requires Energy Affinity: Fire. Fire spells deal +20% total damage and have +1 DC.");
  
  /* Focused Element: Cold */
  perk = &perk_list[PERK_WIZARD_FOCUSED_ELEMENT_COLD];
  perk->id = PERK_WIZARD_FOCUSED_ELEMENT_COLD;
  perk->name = strdup("Focused Element: Cold");
  perk->description = strdup("Cold spells deal +20% damage total, +1 spell DC");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_WIZARD_ENERGY_AFFINITY_COLD;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 20; /* +20% total (replaces +10% from affinity) */
  perk->effect_modifier = 1; /* +1 spell DC */
  perk->special_description = strdup("Requires Energy Affinity: Cold. Cold spells deal +20% total damage and have +1 DC.");
  
  /* Focused Element: Lightning */
  perk = &perk_list[PERK_WIZARD_FOCUSED_ELEMENT_LIGHTNING];
  perk->id = PERK_WIZARD_FOCUSED_ELEMENT_LIGHTNING;
  perk->name = strdup("Focused Element: Lightning");
  perk->description = strdup("Lightning spells deal +20% damage total, +1 spell DC");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_WIZARD_ENERGY_AFFINITY_LIGHTNING;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 20; /* +20% total (replaces +10% from affinity) */
  perk->effect_modifier = 1; /* +1 spell DC */
  perk->special_description = strdup("Requires Energy Affinity: Lightning. Lightning spells deal +20% total damage and have +1 DC.");
  
  /* Spell Critical I */
  perk = &perk_list[PERK_WIZARD_SPELL_CRITICAL_1];
  perk->id = PERK_WIZARD_SPELL_CRITICAL_1;
  perk->name = strdup("Spell Critical I");
  perk->description = strdup("Damage spells have 5% chance to deal double damage");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_WIZARD_SPELL_POWER_1;
  perk->prerequisite_rank = 3; /* Need at least 3 ranks */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5; /* 5% chance */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Spell Power I (at least 3 ranks). Damaging spells have a 5% chance to critically strike for double damage.");
  
  /* Maximize Spell */
  perk = &perk_list[PERK_WIZARD_MAXIMIZE_SPELL];
  perk->id = PERK_WIZARD_MAXIMIZE_SPELL;
  perk->name = strdup("Maximize Spell");
  perk->description = strdup("Cast 1 spell with max damage (no level increase), 5min cooldown");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_WIZARD_SPELL_POWER_1;
  perk->prerequisite_rank = 5; /* Must max Spell Power I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Spell Power I (max). Once every 5 minutes, you can cast a spell with the Maximize metamagic effect (maximum damage dice) without increasing the spell level. Use 'metamagic maximize' before casting.");

  /*** WIZARD EVOKER TREE - TIER 3 PERKS (3-4 points each) ***/
  
  /* Spell Power III */
  perk = &perk_list[PERK_WIZARD_SPELL_POWER_3];
  perk->id = PERK_WIZARD_SPELL_POWER_3;
  perk->name = strdup("Spell Power III");
  perk->description = strdup("+2 spell damage per rank");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 3;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_WIZARD_SPELL_POWER_2;
  perk->prerequisite_rank = 3; /* Must max Spell Power II */
  perk->effect_type = PERK_EFFECT_SPELL_DAMAGE;
  perk->effect_value = 2; /* +2 per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Spell Power II (max). Your mastery of evocation continues to grow.");
  
  /* Master of Elements */
  perk = &perk_list[PERK_WIZARD_MASTER_OF_ELEMENTS];
  perk->id = PERK_WIZARD_MASTER_OF_ELEMENTS;
  perk->name = strdup("Master of Elements");
  perk->description = strdup("All elemental damage +15%, can change energy type of spells. Use 'masterofelements' command.");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = 0; /* Special: requires any two Focused Element perks */
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 15; /* +15% to all elemental damage */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires any two Focused Element perks. You gain mastery over elemental forces, increasing all elemental spell damage by 15% and gaining the ability to change the energy type of spells.");
  
  /* Spell Critical II */
  perk = &perk_list[PERK_WIZARD_SPELL_CRITICAL_2];
  perk->id = PERK_WIZARD_SPELL_CRITICAL_2;
  perk->name = strdup("Spell Critical II");
  perk->description = strdup("Critical chance increases to 10%, critical damage x2.5");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_WIZARD_SPELL_CRITICAL_1;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10; /* 10% chance */
  perk->effect_modifier = 250; /* 2.5x damage stored as 250 (will divide by 100) */
  perk->special_description = strdup("Requires Spell Critical I. Your critical strike chance increases to 10% and critical spells deal 2.5x damage instead of 1.5x.");
  
  /* Empower Spell */
  perk = &perk_list[PERK_WIZARD_EMPOWER_SPELL];
  perk->id = PERK_WIZARD_EMPOWER_SPELL;
  perk->name = strdup("Empower Spell");
  perk->description = strdup("Cast 1 spell with +50% damage (no level increase), 5min cooldown");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_WIZARD_MAXIMIZE_SPELL;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 50; /* +50% damage */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Maximize Spell. Once every 5 minutes, you can cast a spell with the Empower metamagic effect (+50% damage) without increasing the spell level. Use 'metamagic empower' before casting.");
  
  /* Spell Penetration II */
  perk = &perk_list[PERK_WIZARD_SPELL_PENETRATION_2];
  perk->id = PERK_WIZARD_SPELL_PENETRATION_2;
  perk->name = strdup("Spell Penetration II");
  perk->description = strdup("+4 to overcome spell resistance");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_WIZARD_SPELL_PENETRATION_1;
  perk->prerequisite_rank = 3; /* Must max Spell Penetration I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Spell Penetration I (max). Your spells become even more effective at piercing magical defenses.");
  
  /*** WIZARD EVOKER TREE - TIER 4 CAPSTONE PERKS (5+ points each) ***/
  
  /* Arcane Annihilation */
  perk = &perk_list[PERK_WIZARD_ARCANE_ANNIHILATION];
  perk->id = PERK_WIZARD_ARCANE_ANNIHILATION;
  perk->name = strdup("Arcane Annihilation");
  perk->description = strdup("Damage spells deal +3d6 damage, +2 spell DC");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_WIZARD_SPELL_POWER_3;
  perk->prerequisite_rank = 2; /* Must max Spell Power III + need Master of Elements */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3; /* 3d6 dice */
  perk->effect_modifier = 2; /* +2 DC */
  perk->special_description = strdup("Requires Spell Power III (max) and Master of Elements. Your evocation spells reach devastating power, dealing an additional 3d6 damage and increasing spell DCs by 2.");
  
  /* Overwhelming Magic */
  perk = &perk_list[PERK_WIZARD_OVERWHELMING_MAGIC];
  perk->id = PERK_WIZARD_OVERWHELMING_MAGIC;
  perk->name = strdup("Overwhelming Magic");
  perk->description = strdup("Spell resistance reduced by 10 against your spells");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_WIZARD_SPELL_PENETRATION_2;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Spell Penetration II. Your spells are so potent that enemies' spell resistance is effectively reduced by 10 against your magic.");
}

/* Define Wizard Controller Perks */
static void define_wizard_controller_perks(void)
{
  struct perk_data *perk;
  
  /*** CONTROLLER TREE - TIER 1 PERKS ***/
  
  /* Spell Focus (Enchantment) I */
  perk = &perk_list[PERK_WIZARD_SPELL_FOCUS_ENCHANTMENT_1];
  perk->id = PERK_WIZARD_SPELL_FOCUS_ENCHANTMENT_1;
  perk->name = strdup("Spell Focus (Enchantment) I");
  perk->description = strdup("+1 DC to enchantment spells per rank");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 1;
  perk->max_rank = 5;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPELL_DC;
  perk->effect_value = 1;
  
  /* Spell Focus (Enchantment) II */
  perk = &perk_list[PERK_WIZARD_SPELL_FOCUS_ENCHANTMENT_2];
  perk->id = PERK_WIZARD_SPELL_FOCUS_ENCHANTMENT_2;
  perk->name = strdup("Spell Focus (Enchantment) II");
  perk->description = strdup("+1 DC to enchantment spells per rank");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 1;
  perk->max_rank = 5;
  perk->prerequisite_perk = PERK_WIZARD_SPELL_FOCUS_ENCHANTMENT_1;
  perk->prerequisite_rank = 5;
  perk->effect_type = PERK_EFFECT_SPELL_DC;
  perk->effect_value = 1;
  
  /* Spell Focus (Enchantment) III */
  perk = &perk_list[PERK_WIZARD_SPELL_FOCUS_ENCHANTMENT_3];
  perk->id = PERK_WIZARD_SPELL_FOCUS_ENCHANTMENT_3;
  perk->name = strdup("Spell Focus (Enchantment) III");
  perk->description = strdup("+1 DC to enchantment spells per rank");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 1;
  perk->max_rank = 5;
  perk->prerequisite_perk = PERK_WIZARD_SPELL_FOCUS_ENCHANTMENT_2;
  perk->prerequisite_rank = 5;
  perk->effect_type = PERK_EFFECT_SPELL_DC;
  perk->effect_value = 1;
  
  /* Spell Focus (Enchantment) IV */
  perk = &perk_list[PERK_WIZARD_SPELL_FOCUS_ENCHANTMENT_4];
  perk->id = PERK_WIZARD_SPELL_FOCUS_ENCHANTMENT_4;
  perk->name = strdup("Spell Focus (Enchantment) IV");
  perk->description = strdup("+1 DC to enchantment spells per rank");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 2;
  perk->max_rank = 5;
  perk->prerequisite_perk = PERK_WIZARD_SPELL_FOCUS_ENCHANTMENT_3;
  perk->prerequisite_rank = 5;
  perk->effect_type = PERK_EFFECT_SPELL_DC;
  perk->effect_value = 1;
  
  /* Spell Focus (Enchantment) V */
  perk = &perk_list[PERK_WIZARD_SPELL_FOCUS_ENCHANTMENT_5];
  perk->id = PERK_WIZARD_SPELL_FOCUS_ENCHANTMENT_5;
  perk->name = strdup("Spell Focus (Enchantment) V");
  perk->description = strdup("+1 DC to enchantment spells per rank");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 2;
  perk->max_rank = 5;
  perk->prerequisite_perk = PERK_WIZARD_SPELL_FOCUS_ENCHANTMENT_4;
  perk->prerequisite_rank = 5;
  perk->effect_type = PERK_EFFECT_SPELL_DC;
  perk->effect_value = 1;
  
  /* Extend Spell */
  perk = &perk_list[PERK_WIZARD_EXTEND_SPELL];
  perk->id = PERK_WIZARD_EXTEND_SPELL;
  perk->name = strdup("Extend Spell");
  perk->description = strdup("Buff spell durations are increased by 5% per rank (minimum +1 round)");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 1;
  perk->max_rank = 10;
  perk->prerequisite_perk = PERK_WIZARD_SPELL_FOCUS_ENCHANTMENT_1;
  perk->prerequisite_rank = 3;
  perk->effect_type = PERK_EFFECT_SPELL_DURATION;
  perk->effect_value = 5;
  
  /*** CONTROLLER TREE - TIER 2 PERKS ***/
  
  /* Greater Spell Focus (Enchantment) I */
  perk = &perk_list[PERK_WIZARD_GREATER_SPELL_FOCUS_ENCHANTMENT_1];
  perk->id = PERK_WIZARD_GREATER_SPELL_FOCUS_ENCHANTMENT_1;
  perk->name = strdup("Greater Spell Focus (Enchantment) I");
  perk->description = strdup("+2 DC to enchantment spells per rank");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 2;
  perk->max_rank = 3;
  perk->prerequisite_perk = PERK_WIZARD_SPELL_FOCUS_ENCHANTMENT_5;
  perk->prerequisite_rank = 5;
  perk->effect_type = PERK_EFFECT_SPELL_DC;
  perk->effect_value = 2;
  
  /* Greater Spell Focus (Enchantment) II */
  perk = &perk_list[PERK_WIZARD_GREATER_SPELL_FOCUS_ENCHANTMENT_2];
  perk->id = PERK_WIZARD_GREATER_SPELL_FOCUS_ENCHANTMENT_2;
  perk->name = strdup("Greater Spell Focus (Enchantment) II");
  perk->description = strdup("+2 DC to enchantment spells per rank");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 2;
  perk->max_rank = 3;
  perk->prerequisite_perk = PERK_WIZARD_GREATER_SPELL_FOCUS_ENCHANTMENT_1;
  perk->prerequisite_rank = 3;
  perk->effect_type = PERK_EFFECT_SPELL_DC;
  perk->effect_value = 2;
  
  /* Greater Spell Focus (Enchantment) III */
  perk = &perk_list[PERK_WIZARD_GREATER_SPELL_FOCUS_ENCHANTMENT_3];
  perk->id = PERK_WIZARD_GREATER_SPELL_FOCUS_ENCHANTMENT_3;
  perk->name = strdup("Greater Spell Focus (Enchantment) III");
  perk->description = strdup("+2 DC to enchantment spells per rank");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 3;
  perk->max_rank = 3;
  perk->prerequisite_perk = PERK_WIZARD_GREATER_SPELL_FOCUS_ENCHANTMENT_2;
  perk->prerequisite_rank = 3;
  perk->effect_type = PERK_EFFECT_SPELL_DC;
  perk->effect_value = 2;
  
  /* Persistent Spell */
  perk = &perk_list[PERK_WIZARD_PERSISTENT_SPELL];
  perk->id = PERK_WIZARD_PERSISTENT_SPELL;
  perk->name = strdup("Persistent Spell");
  perk->description = strdup("Activate to force target to save twice vs next spell (2 uses, regenerates 1 per 5 min). Use 'persistentspell' command.");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_WIZARD_SPELL_FOCUS_ENCHANTMENT_3;
  perk->prerequisite_rank = 3;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  
  /* Split Enchantment */
  perk = &perk_list[PERK_WIZARD_SPLIT_ENCHANTMENT];
  perk->id = PERK_WIZARD_SPLIT_ENCHANTMENT;
  perk->name = strdup("Split Enchantment");
  perk->description = strdup("Activate to make next enchantment spell affect all enemies in room (5 min cooldown). Use 'splitenchantment' command.");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_WIZARD_GREATER_SPELL_FOCUS_ENCHANTMENT_1;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;

  /*** WIZARD CONTROLLER TREE - TIER 3 PERKS ***/

  /* Extended Spell III */
  perk = &perk_list[PERK_WIZARD_EXTENDED_SPELL_3];
  perk->id = PERK_WIZARD_EXTENDED_SPELL_3;
  perk->name = strdup("Extended Spell III");
  perk->description = strdup("+5 rounds duration");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_WIZARD_EXTENDED_SPELL_2;
  perk->prerequisite_rank = 2; /* Must have Extended Spell II maxed */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5;
  perk->special_description = strdup("Requires Extended Spell II (max). Your spell durations are extended by an additional 5 rounds.");

  /* Master Enchanter */
  perk = &perk_list[PERK_WIZARD_MASTER_ENCHANTER];
  perk->id = PERK_WIZARD_MASTER_ENCHANTER;
  perk->name = strdup("Master Enchanter");
  perk->description = strdup("Enchantment DC +3 (total +5), duration doubled");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_WIZARD_GREATER_SPELL_FOCUS_ENCHANTMENT_1;
  perk->prerequisite_rank = 1; /* Must have Greater Spell Focus: Enchantment */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3; /* +3 DC */
  perk->special_description = strdup("Requires Greater Spell Focus: Enchantment. You become a true master of enchantment magic. Enchantment spell DCs increase by +3 (total +5), and all enchantment spell durations are doubled.");

  /* Master Illusionist */
  perk = &perk_list[PERK_WIZARD_MASTER_ILLUSIONIST];
  perk->id = PERK_WIZARD_MASTER_ILLUSIONIST;
  perk->name = strdup("Master Illusionist");
  perk->description = strdup("Illusion DC +3 (total +5), enemies get -4 to disbelief");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_WIZARD_SPELL_FOCUS_ENCHANTMENT_3; /* Using enchantment focus as proxy for illusion */
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3; /* +3 DC */
  perk->special_description = strdup("You become a true master of illusion magic. Illusion spell DCs increase by +3 (total +5), and enemies get -4 to disbelief checks.");

  /* Master Transmuter */
  perk = &perk_list[PERK_WIZARD_MASTER_TRANSMUTER];
  perk->id = PERK_WIZARD_MASTER_TRANSMUTER;
  perk->name = strdup("Master Transmuter");
  perk->description = strdup("Transmutation DC +3 (total +5), duration increased 50%");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_WIZARD_SPELL_FOCUS_ENCHANTMENT_4; /* Using enchantment focus as proxy for transmutation */
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3; /* +3 DC */
  perk->special_description = strdup("You become a true master of transmutation magic. Transmutation spell DCs increase by +3 (total +5), and all transmutation spell durations are increased by 50%.");

  /* Spell Mastery */
  perk = &perk_list[PERK_WIZARD_SPELL_MASTERY];
  perk->id = PERK_WIZARD_SPELL_MASTERY;
  perk->name = strdup("Spell Mastery");
  perk->description = strdup("+2 to all spell DCs");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_WIZARD_SPELL_FOCUS_ENCHANTMENT_5; /* Using mental clarity proxy */
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 DC */
  perk->special_description = strdup("Your mastery of magic grants +2 to all spell DCs.");

  /*** WIZARD CONTROLLER TREE - TIER 4 CAPSTONE PERKS ***/

  /* Archmage of Control */
  perk = &perk_list[PERK_WIZARD_ARCHMAGE_OF_CONTROL];
  perk->id = PERK_WIZARD_ARCHMAGE_OF_CONTROL;
  perk->name = strdup("Archmage of Control");
  perk->description = strdup("Control spells last 50% longer, +5 DC vs charm/confuse/daze/sleep");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_WIZARD_EXTENDED_SPELL_3;
  perk->prerequisite_rank = 1; /* Must have Extended Spell III */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5; /* +5 DC */
  perk->special_description = strdup("Requires Extended Spell III and Spell Mastery. You become an archmage of control magic. All spells that apply charm, confusion, daze, or sleep effects have their duration extended by 50% and gain +5 to their save DCs.");

  /* Irresistible Magic */
  perk = &perk_list[PERK_WIZARD_IRRESISTIBLE_MAGIC];
  perk->id = PERK_WIZARD_IRRESISTIBLE_MAGIC;
  perk->name = strdup("Irresistible Magic");
  perk->description = strdup("Once per 5 minutes, cast spell that cannot be resisted");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_WIZARD_PERSISTENT_SPELL;
  perk->prerequisite_rank = 1; /* Must have Persistent Spell */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->special_description = strdup("Requires Persistent Spell and Spell Mastery. Once every 5 minutes, you can use the 'irresistablemagic' command to make your next spell automatically succeed (no saving throw allowed).");
}

/*****************************************************************************
 * Wizard Versatile Caster Tree Perk Definitions
 *****************************************************************************/

void define_wizard_versatile_caster_perks(void)
{
  struct perk_data *perk;
  
  /*** VERSATILE CASTER TREE - TIER 1 PERKS ***/
  
  /* Spell Focus I - 2% chance per rank to not expend spell slot */
  perk = &perk_list[PERK_WIZARD_SPELL_FOCUS_I];
  perk->id = PERK_WIZARD_SPELL_FOCUS_I;
  perk->name = strdup("Spell Focus I");
  perk->description = strdup("2% chance per rank that spells don't expend a slot when cast");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 1;
  perk->max_rank = 5;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* 2% per rank */
  perk->special_description = strdup("When you cast a spell, there is a 2% chance per rank (max 10% at rank 5) that the spell slot is not expended.");
  
  /* Quick Cast I */
  perk = &perk_list[PERK_WIZARD_QUICK_CAST_I];
  perk->id = PERK_WIZARD_QUICK_CAST_I;
  perk->name = strdup("Quick Cast I");
  perk->description = strdup("Use quicken metamagic without spell circle increase once per 5 minutes");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->special_description = strdup("Once every 5 minutes, you can apply quicken metamagic to a spell without increasing its spell circle requirement.");
  
  /* Arcane Knowledge I */
  perk = &perk_list[PERK_WIZARD_ARCANE_KNOWLEDGE_I];
  perk->id = PERK_WIZARD_ARCANE_KNOWLEDGE_I;
  perk->name = strdup("Arcane Knowledge I");
  perk->description = strdup("+2 to spellcraft skill per rank");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SKILL;
  perk->effect_value = 2; /* +2 per rank */
  perk->special_description = strdup("Increases your spellcraft skill by +2 per rank.");
  
  /* Combat Casting I */
  perk = &perk_list[PERK_WIZARD_COMBAT_CASTING_I];
  perk->id = PERK_WIZARD_COMBAT_CASTING_I;
  perk->name = strdup("Combat Casting I");
  perk->description = strdup("+2 concentration when in combat per rank");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 per rank when in combat */
  perk->special_description = strdup("Increases your concentration skill by +2 per rank when you are in combat.");
  
  /*** VERSATILE CASTER TREE - TIER 2 PERKS ***/
  
  /* Spell Focus II - Additional 2% chance per rank */
  perk = &perk_list[PERK_WIZARD_SPELL_FOCUS_II];
  perk->id = PERK_WIZARD_SPELL_FOCUS_II;
  perk->name = strdup("Spell Focus II");
  perk->description = strdup("Additional 2% chance per rank that spells don't expend slots");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 2;
  perk->max_rank = 3;
  perk->prerequisite_perk = PERK_WIZARD_SPELL_FOCUS_I;
  perk->prerequisite_rank = 5; /* Must max Spell Focus I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* 2% per rank */
  perk->special_description = strdup("Requires Spell Focus I (max). Increases the chance by an additional 2% per rank (total 16% at max ranks of both).");
  
  /* Quick Cast II */
  perk = &perk_list[PERK_WIZARD_QUICK_CAST_II];
  perk->id = PERK_WIZARD_QUICK_CAST_II;
  perk->name = strdup("Quick Cast II");
  perk->description = strdup("Can cast one spell per combat as instant action");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_WIZARD_QUICK_CAST_I;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->special_description = strdup("Requires Quick Cast I. Once per combat, you can cast a spell as an instant action without using the quicken metamagic.");
  
  /* Spell Recall */
  perk = &perk_list[PERK_WIZARD_SPELL_RECALL];
  perk->id = PERK_WIZARD_SPELL_RECALL;
  perk->name = strdup("Spell Recall");
  perk->description = strdup("Restore a spell slot once per day");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_WIZARD_SPELL_FOCUS_I;
  perk->prerequisite_rank = 3; /* Must have at least 3 ranks */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->special_description = strdup("Requires Spell Focus I (at least 3 ranks). Once per day, use the 'spellrecall' command to restore a spell slot. For spontaneous casters, restores a random slot. For preparation casters, randomly advances a spell currently being memorized.");
  
  /* Metamagic Master I */
  perk = &perk_list[PERK_WIZARD_METAMAGIC_MASTER_I];
  perk->id = PERK_WIZARD_METAMAGIC_MASTER_I;
  perk->name = strdup("Metamagic Master I");
  perk->description = strdup("Metamagic reduces spell circle increase by 1");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* -1 circle reduction */
  perk->special_description = strdup("When using metamagic feats, reduce the spell circle increase by 1 (minimum 0). Limited to 2 uses per 5 minutes. Use 'cooldowns' to check remaining uses.");
  
  /* Defensive Casting */
  perk = &perk_list[PERK_WIZARD_DEFENSIVE_CASTING];
  perk->id = PERK_WIZARD_DEFENSIVE_CASTING;
  perk->name = strdup("Defensive Casting");
  perk->description = strdup("+4 AC when casting spells");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_WIZARD_COMBAT_CASTING_I;
  perk->prerequisite_rank = 2; /* Must have at least 2 ranks */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4; /* +4 AC */
  perk->special_description = strdup("Requires Combat Casting I (at least 2 ranks). You gain +4 AC when casting spells, lasting until your next turn.");

  /*** VERSATILE CASTER TREE - TIER 3 PERKS (3-4 points each) ***/
  
  perk = &perk_list[PERK_WIZARD_SPELL_FOCUS_III];
  perk->id = PERK_WIZARD_SPELL_FOCUS_III;
  perk->name = strdup("Spell Focus III");
  perk->description = strdup("2% chance per rank spell doesn't consume slot");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 3;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_WIZARD_SPELL_FOCUS_II;
  perk->prerequisite_rank = 3; /* Must max out Spell Focus II */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2% per rank */
  perk->special_description = strdup("Requires Spell Focus II (max). Additional 2% chance per rank that spells don't consume prepared slots (total up to 16% with all ranks).");

  perk = &perk_list[PERK_WIZARD_METAMAGIC_MASTER_II];
  perk->id = PERK_WIZARD_METAMAGIC_MASTER_II;
  perk->name = strdup("Metamagic Master II");
  perk->description = strdup("Metamagic spell circle increase reduced by 1");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_WIZARD_METAMAGIC_MASTER_I;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = -1; /* -1 to metamagic circle cost */
  perk->special_description = strdup("Requires Metamagic Master I. Metamagic feats increase spell circle by 1 less (stacks with Metamagic Master I for total -2). Limited to 2 uses per 5 minutes. Use 'cooldowns' to check remaining uses.");

  perk = &perk_list[PERK_WIZARD_ARCANE_RECOVERY];
  perk->id = PERK_WIZARD_ARCANE_RECOVERY;
  perk->name = strdup("Arcane Recovery");
  perk->description = strdup("Reduce spell preparation time");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_WIZARD_SPELL_RECALL;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->special_description = strdup("Requires Spell Recall. Once per day, reduce spell preparation/slot recovery time by 50%. Use 'arcane-recovery' command.");

  perk = &perk_list[PERK_WIZARD_SPELL_SHIELD];
  perk->id = PERK_WIZARD_SPELL_SHIELD;
  perk->name = strdup("Spell Shield");
  perk->description = strdup("10 DR + 4 AC when attacked first");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_WIZARD_DEFENSIVE_CASTING;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10; /* 10 DR */
  perk->special_description = strdup("Requires Defensive Casting. When an enemy initiates combat against you, automatically activate a spell shield granting 10 DR and +4 AC for 1 round.");

  /*** VERSATILE CASTER TREE - TIER 4 CAPSTONE PERKS (5 points each) ***/
  
  perk = &perk_list[PERK_WIZARD_ARCHMAGES_POWER];
  perk->id = PERK_WIZARD_ARCHMAGES_POWER;
  perk->name = strdup("Archmage's Power");
  perk->description = strdup("+5% free spell chance, -1 metamagic cost");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_WIZARD_SPELL_FOCUS_III;
  perk->prerequisite_rank = 2; /* Must max out Spell Focus III */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5; /* +5% free spell chance */
  perk->special_description = strdup("Requires Spell Focus III (max) and Metamagic Master II. Additional 5% chance spells don't consume slots, and metamagic reduces spell circle by 1 additional level. Limited to 2 uses per 5 minutes. Use 'cooldowns' to check remaining uses.");

  perk = &perk_list[PERK_WIZARD_ARCANE_SUPREMACY];
  perk->id = PERK_WIZARD_ARCANE_SUPREMACY;
  perk->name = strdup("Arcane Supremacy");
  perk->description = strdup("+2 spell DCs, +2 caster level, +2 spell damage");
  perk->associated_class = CLASS_WIZARD;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_WIZARD_ARCANE_KNOWLEDGE_I;
  perk->prerequisite_rank = 3; /* Must max out Arcane Knowledge I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 to various stats */
  perk->special_description = strdup("Requires Arcane Knowledge I (max) and Metamagic Master II. Gain +2 to all spell DCs, +2 effective caster level, and +2 spell damage.");
}

/* Define Cleric Perks */
void define_cleric_perks(void)
{
  struct perk_data *perk;
  
  /*** DIVINE HEALER TREE - TIER 1 PERKS (1 point each) ***/
  
  /* Healing Power I */
  perk = &perk_list[PERK_CLERIC_HEALING_POWER_1];
  perk->id = PERK_CLERIC_HEALING_POWER_1;
  perk->name = strdup("Healing Power I");
  perk->description = strdup("Healing spells restore +2 HP per rank");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 1;
  perk->max_rank = 5;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 HP per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Increases healing done by all healing spells by +2 HP per rank. Can be taken 5 times for +10 HP total.");
  
  /* Radiant Servant I */
  perk = &perk_list[PERK_CLERIC_RADIANT_SERVANT_1];
  perk->id = PERK_CLERIC_RADIANT_SERVANT_1;
  perk->name = strdup("Radiant Servant I");
  perk->description = strdup("+1 to positive energy damage/healing per rank");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 positive energy per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Increases positive energy effects (healing, turn undead, etc.) by +1 per rank. Can be taken 3 times for +3 total.");
  
  /* Efficient Healing */
  perk = &perk_list[PERK_CLERIC_EFFICIENT_HEALING];
  perk->id = PERK_CLERIC_EFFICIENT_HEALING;
  perk->name = strdup("Efficient Healing");
  perk->description = strdup("Cast 1 additional healing spell per rest period");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* 1 extra healing spell */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Allows you to prepare and cast one additional healing spell per rest period, bypassing normal spell slot limits for healing spells only.");
  
  /* Preserve Life */
  perk = &perk_list[PERK_CLERIC_PRESERVE_LIFE];
  perk->id = PERK_CLERIC_PRESERVE_LIFE;
  perk->name = strdup("Preserve Life");
  perk->description = strdup("+5 HP to healing spells per rank when target below 50% HP");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5; /* +5 HP per rank when target below 50% */
  perk->effect_modifier = 0;
  perk->special_description = strdup("When healing a target that is below 50% of their maximum HP, healing is increased by +5 HP per rank. Can be taken 3 times for +15 HP total.");

  /*** DIVINE HEALER TREE - TIER 2 PERKS (2 points each) ***/
  
  /* Healing Power II */
  perk = &perk_list[PERK_CLERIC_HEALING_POWER_2];
  perk->id = PERK_CLERIC_HEALING_POWER_2;
  perk->name = strdup("Healing Power II");
  perk->description = strdup("Additional +3 HP healing per rank");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 2;
  perk->max_rank = 3;
  perk->prerequisite_perk = PERK_CLERIC_HEALING_POWER_1;
  perk->prerequisite_rank = 5; /* Must have max ranks of Healing Power I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3; /* +3 HP per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Healing Power I (max). Increases healing done by all healing spells by additional +3 HP per rank. Can be taken 3 times for +9 HP total.");
  
  /* Radiant Servant II */
  perk = &perk_list[PERK_CLERIC_RADIANT_SERVANT_2];
  perk->id = PERK_CLERIC_RADIANT_SERVANT_2;
  perk->name = strdup("Radiant Servant II");
  perk->description = strdup("Additional +2 positive energy per rank");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_CLERIC_RADIANT_SERVANT_1;
  perk->prerequisite_rank = 3; /* Must have max ranks of Radiant Servant I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 positive energy per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Radiant Servant I (max). Increases positive energy effects by additional +2 per rank. Can be taken 2 times for +4 total.");
  
  /* Mass Healing Focus */
  perk = &perk_list[PERK_CLERIC_MASS_HEALING_FOCUS];
  perk->id = PERK_CLERIC_MASS_HEALING_FOCUS;
  perk->name = strdup("Mass Healing Focus");
  perk->description = strdup("Mass healing spells affect +2 additional targets");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_HEALING_POWER_1;
  perk->prerequisite_rank = 3; /* Must have at least 3 ranks of Healing Power I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 targets */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Healing Power I (at least 3 ranks). Mass healing spells can affect 2 additional targets beyond the normal limit.");
  
  /* Empowered Healing I */
  perk = &perk_list[PERK_CLERIC_EMPOWERED_HEALING_1];
  perk->id = PERK_CLERIC_EMPOWERED_HEALING_1;
  perk->name = strdup("Empowered Healing I");
  perk->description = strdup("Healing spells have 10% chance to heal for 150%");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_HEALING_POWER_1;
  perk->prerequisite_rank = 3; /* Must have at least 3 ranks of Healing Power I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10; /* 10% chance */
  perk->effect_modifier = 150; /* 150% healing */
  perk->special_description = strdup("Requires Healing Power I (at least 3 ranks). Your healing spells have a 10% chance to be empowered, healing for 150% of their normal amount.");
  
  /* Channel Energy: Heal */
  perk = &perk_list[PERK_CLERIC_CHANNEL_ENERGY_HEAL];
  perk->id = PERK_CLERIC_CHANNEL_ENERGY_HEAL;
  perk->name = strdup("Channel Energy: Heal");
  perk->description = strdup("Channel positive energy heals 2d6 HP to all allies in room");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_RADIANT_SERVANT_1;
  perk->prerequisite_rank = 2; /* Must have at least 2 ranks of Radiant Servant I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* 2d6 */
  perk->effect_modifier = 6;
  perk->special_description = strdup("Requires Radiant Servant I (at least 2 ranks). Grants the ability to channel positive energy to heal all allies in the room for 2d6 HP. Can be used once per short rest.");
  
  /* Healing Aura I */
  perk = &perk_list[PERK_CLERIC_HEALING_AURA_1];
  perk->id = PERK_CLERIC_HEALING_AURA_1;
  perk->name = strdup("Healing Aura I");
  perk->description = strdup("Allies within 2 rooms regenerate +1 HP per tick");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_RADIANT_SERVANT_1;
  perk->prerequisite_rank = 3; /* Must have max ranks of Radiant Servant I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 HP per tick */
  perk->effect_modifier = 2; /* 2 room range */
  perk->special_description = strdup("Requires Radiant Servant I (max). Your presence radiates healing energy. All allies within 2 rooms of you regenerate an additional +1 HP per regeneration tick.");
  
  /*** DIVINE HEALER TREE - TIER 3 PERKS (3-4 points each) ***/
  
  /* Healing Power III (Rank 1-2) */
  perk = &perk_list[PERK_CLERIC_HEALING_POWER_3];
  perk->id = PERK_CLERIC_HEALING_POWER_3;
  perk->name = strdup("Healing Power III");
  perk->description = strdup("Additional +5 HP per rank to all healing spells");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 3;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_CLERIC_HEALING_POWER_2;
  perk->prerequisite_rank = 3; /* Must have max ranks of Healing Power II */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5; /* +5 HP per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Healing Power II (max). Increases all healing spells by +5 HP per rank. Maximum 2 ranks for +10 HP total.");
  
  /* Empowered Healing II */
  perk = &perk_list[PERK_CLERIC_EMPOWERED_HEALING_2];
  perk->id = PERK_CLERIC_EMPOWERED_HEALING_2;
  perk->name = strdup("Empowered Healing II");
  perk->description = strdup("Critical healing: 20% chance for 200% healing");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_EMPOWERED_HEALING_1;
  perk->prerequisite_rank = 1; /* Must have Empowered Healing I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 20; /* 20% chance */
  perk->effect_modifier = 200; /* 200% healing */
  perk->special_description = strdup("Requires Empowered Healing I. Critical healing chance increases to 20% and heals for 200% (double) instead of 150%.");
  
  /* Channel Energy: Greater Heal */
  perk = &perk_list[PERK_CLERIC_CHANNEL_ENERGY_GREATER_HEAL];
  perk->id = PERK_CLERIC_CHANNEL_ENERGY_GREATER_HEAL;
  perk->name = strdup("Channel Energy: Greater Heal");
  perk->description = strdup("Channel healing increases to 4d6 HP");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_CHANNEL_ENERGY_HEAL;
  perk->prerequisite_rank = 1; /* Must have Channel Energy: Heal */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4; /* 4d6 */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Channel Energy: Heal. Your channel positive energy now heals for 4d6 HP instead of 2d6.");
  
  /* Healing Aura II */
  perk = &perk_list[PERK_CLERIC_HEALING_AURA_2];
  perk->id = PERK_CLERIC_HEALING_AURA_2;
  perk->name = strdup("Healing Aura II");
  perk->description = strdup("Healing aura +2 HP/tick, range 3 rooms");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_HEALING_AURA_1;
  perk->prerequisite_rank = 1; /* Must have Healing Aura I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 HP per tick */
  perk->effect_modifier = 3; /* 3 room range */
  perk->special_description = strdup("Requires Healing Aura I. Your healing aura increases to +2 HP per tick and extends to 3 rooms away.");
  
  /* Restorative Touch */
  perk = &perk_list[PERK_CLERIC_RESTORATIVE_TOUCH];
  perk->id = PERK_CLERIC_RESTORATIVE_TOUCH;
  perk->name = strdup("Restorative Touch");
  perk->description = strdup("Healing spells can remove one negative condition");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_HEALING_POWER_2;
  perk->prerequisite_rank = 2; /* Must have at least 2 ranks of Healing Power II */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Healing Power II (at least 2 ranks). When you cast a healing spell, it can remove one negative condition from the target (poison, disease, curse, blindness, or paralysis).");
  
  /*** DIVINE HEALER TREE - TIER 4 CAPSTONES (5 points each) ***/
  
  /* Divine Radiance */
  perk = &perk_list[PERK_CLERIC_DIVINE_RADIANCE];
  perk->id = PERK_CLERIC_DIVINE_RADIANCE;
  perk->name = strdup("Divine Radiance");
  perk->description = strdup("All healing +20 HP, channel 6d6, aura +3 HP/tick");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_CHANNEL_ENERGY_GREATER_HEAL;
  perk->prerequisite_rank = 1; /* Must have Channel Energy: Greater Heal */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 20; /* +20 HP to all healing */
  perk->effect_modifier = 6; /* Channel becomes 6d6 */
  perk->special_description = strdup("Requires Healing Power III (max) and Channel Energy: Greater Heal. Capstone: All healing spells gain +20 HP, channel energy heals for 6d6 HP, and healing aura grants +3 HP per tick.");
  
  /* Beacon of Hope */
  perk = &perk_list[PERK_CLERIC_BEACON_OF_HOPE];
  perk->id = PERK_CLERIC_BEACON_OF_HOPE;
  perk->name = strdup("Beacon of Hope");
  perk->description = strdup("1/day: Fully heal allies, grant +4 saves for 10 rounds");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_HEALING_AURA_2;
  perk->prerequisite_rank = 1; /* Must have Healing Aura II */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* Once per day */
  perk->effect_modifier = 4; /* +4 to saves */
  perk->special_description = strdup("Requires Healing Aura II and Mass Healing Focus. Capstone: Once per day, you can fully heal all allies in the area and grant them +4 to all saving throws for 10 rounds.");
  
  /*** BATTLE CLERIC TREE - TIER 1 PERKS (1 point each) ***/
  
  /* Divine Favor I (Rank 1-3) */
  perk = &perk_list[PERK_CLERIC_DIVINE_FAVOR_1];
  perk->id = PERK_CLERIC_DIVINE_FAVOR_1;
  perk->name = strdup("Divine Favor I");
  perk->description = strdup("+1 to hit per rank");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_WEAPON_TOHIT;
  perk->effect_value = 1; /* +1 per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Increases your attack bonus by +1 per rank. Maximum 3 ranks for +3 total.");
  
  /* Holy Weapon I (Rank 1-3) */
  perk = &perk_list[PERK_CLERIC_HOLY_WEAPON_1];
  perk->id = PERK_CLERIC_HOLY_WEAPON_1;
  perk->name = strdup("Holy Weapon I");
  perk->description = strdup("Melee attacks deal +2 holy damage per rank");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 holy damage per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Your melee attacks deal +2 holy damage per rank. Maximum 3 ranks for +6 holy damage total.");
  
  /* Armor of Faith I (Rank 1-3) */
  perk = &perk_list[PERK_CLERIC_ARMOR_OF_FAITH_1];
  perk->id = PERK_CLERIC_ARMOR_OF_FAITH_1;
  perk->name = strdup("Armor of Faith I");
  perk->description = strdup("+1 AC per rank");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_AC;
  perk->effect_value = 1; /* +1 AC per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Divine protection grants +1 AC per rank. Maximum 3 ranks for +3 AC total.");
  
  /* Battle Blessing */
  perk = &perk_list[PERK_CLERIC_BATTLE_BLESSING];
  perk->id = PERK_CLERIC_BATTLE_BLESSING;
  perk->name = strdup("Battle Blessing");
  perk->description = strdup("Can cast self-buff spells as swift action");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("You can cast self-targeting buff spells as a swift action instead of a standard action, allowing you to buff and attack in the same round.");
  
  /* Smite Evil I */
  perk = &perk_list[PERK_CLERIC_SMITE_EVIL_1];
  perk->id = PERK_CLERIC_SMITE_EVIL_1;
  perk->name = strdup("Smite Evil I");
  perk->description = strdup("Once per combat: +2d6 damage to evil enemies");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* 2d6 damage */
  perk->effect_modifier = 1; /* Once per combat */
  perk->special_description = strdup("Once per combat, you can channel divine fury to deal +2d6 holy damage against an evil-aligned enemy.");
  
  /*** BATTLE CLERIC TREE - TIER 2 PERKS (2 points each) ***/
  
  /* Divine Favor II (Rank 1-2) */
  perk = &perk_list[PERK_CLERIC_DIVINE_FAVOR_2];
  perk->id = PERK_CLERIC_DIVINE_FAVOR_2;
  perk->name = strdup("Divine Favor II");
  perk->description = strdup("Additional +1 to hit per rank");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_CLERIC_DIVINE_FAVOR_1;
  perk->prerequisite_rank = 3; /* Must have max ranks of Divine Favor I */
  perk->effect_type = PERK_EFFECT_WEAPON_TOHIT;
  perk->effect_value = 1; /* +1 per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Divine Favor I (max). Increases your attack bonus by an additional +1 per rank. Maximum 2 ranks for +2 total (combined +5 with Tier 1).");
  
  /* Holy Weapon II (Rank 1-2) */
  perk = &perk_list[PERK_CLERIC_HOLY_WEAPON_2];
  perk->id = PERK_CLERIC_HOLY_WEAPON_2;
  perk->name = strdup("Holy Weapon II");
  perk->description = strdup("Additional +3 holy damage per rank");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_CLERIC_HOLY_WEAPON_1;
  perk->prerequisite_rank = 3; /* Must have max ranks of Holy Weapon I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3; /* +3 holy damage per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Holy Weapon I (max). Your melee attacks deal an additional +3 holy damage per rank. Maximum 2 ranks for +6 holy damage (combined +12 with Tier 1).");
  
  /* Armor of Faith II (Rank 1-2) */
  perk = &perk_list[PERK_CLERIC_ARMOR_OF_FAITH_2];
  perk->id = PERK_CLERIC_ARMOR_OF_FAITH_2;
  perk->name = strdup("Armor of Faith II");
  perk->description = strdup("Additional +1 AC per rank");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_CLERIC_ARMOR_OF_FAITH_1;
  perk->prerequisite_rank = 3; /* Must have max ranks of Armor of Faith I */
  perk->effect_type = PERK_EFFECT_AC;
  perk->effect_value = 1; /* +1 AC per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Armor of Faith I (max). Divine protection grants an additional +1 AC per rank. Maximum 2 ranks for +2 AC (combined +5 with Tier 1).");
  
  /* Smite Evil II */
  perk = &perk_list[PERK_CLERIC_SMITE_EVIL_2];
  perk->id = PERK_CLERIC_SMITE_EVIL_2;
  perk->name = strdup("Smite Evil II");
  perk->description = strdup("+4d6 damage to evil, usable 2/combat");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_SMITE_EVIL_1;
  perk->prerequisite_rank = 1; /* Must have Smite Evil I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4; /* 4d6 damage */
  perk->effect_modifier = 2; /* Twice per combat */
  perk->special_description = strdup("Requires Smite Evil I. Smite damage increases to +4d6 holy damage and can be used twice per combat.");
  
  /* Divine Power */
  perk = &perk_list[PERK_CLERIC_DIVINE_POWER];
  perk->id = PERK_CLERIC_DIVINE_POWER;
  perk->name = strdup("Divine Power");
  perk->description = strdup("+2 to melee damage rolls");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_DIVINE_FAVOR_1;
  perk->prerequisite_rank = 2; /* Must have at least 2 ranks of Divine Favor I */
  perk->effect_type = PERK_EFFECT_WEAPON_DAMAGE;
  perk->effect_value = 2; /* +2 damage */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Divine Favor I (at least 2 ranks). You channel divine power into your melee attacks, gaining +2 to all melee damage rolls.");
  
  /* Channel Energy: Harm */
  perk = &perk_list[PERK_CLERIC_CHANNEL_ENERGY_HARM];
  perk->id = PERK_CLERIC_CHANNEL_ENERGY_HARM;
  perk->name = strdup("Channel Energy: Harm");
  perk->description = strdup("Channel negative energy: 2d6 damage to enemies");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* 2d6 damage */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Grants the ability to channel negative energy to harm living enemies in the room for 2d6 damage. Can be used in conjunction with the existing channel energy ability.");
  
  /* Spiritual Weapon */
  perk = &perk_list[PERK_CLERIC_SPIRITUAL_WEAPON];
  perk->id = PERK_CLERIC_SPIRITUAL_WEAPON;
  perk->name = strdup("Spiritual Weapon");
  perk->description = strdup("Summon spiritual weapon that attacks for 5 rounds");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_HOLY_WEAPON_1;
  perk->prerequisite_rank = 2; /* Must have at least 2 ranks of Holy Weapon I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5; /* Lasts 5 rounds */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Holy Weapon I (at least 2 ranks). You can summon a spiritual weapon that attacks your enemies independently for 5 rounds. The weapon uses your base attack bonus and deals weapon damage.");

  /*** Battle Cleric Tree - Tier 3 Perks ***/

  /* Divine Favor III */
  perk = &perk_list[PERK_CLERIC_DIVINE_FAVOR_3];
  perk->id = PERK_CLERIC_DIVINE_FAVOR_3;
  perk->name = strdup("Divine Favor III");
  perk->description = strdup("+2 to hit with melee weapons");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_DIVINE_FAVOR_2;
  perk->prerequisite_rank = 2; /* Must have Divine Favor II maxed */
  perk->effect_type = PERK_EFFECT_WEAPON_TOHIT;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Divine Favor II (max). Divine favor continues to strengthen your attacks, granting an additional +2 to hit with melee weapons.");

  /* Holy Weapon III */
  perk = &perk_list[PERK_CLERIC_HOLY_WEAPON_3];
  perk->id = PERK_CLERIC_HOLY_WEAPON_3;
  perk->name = strdup("Holy Weapon III");
  perk->description = strdup("+5 holy damage, weapons count as holy");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_HOLY_WEAPON_2;
  perk->prerequisite_rank = 2; /* Must have Holy Weapon II maxed */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5; /* +5 holy damage */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Holy Weapon II (max). Your weapons become truly sanctified, dealing an additional +5 holy damage and counting as holy for purposes of overcoming damage reduction.");

  /* Armor of Faith III */
  perk = &perk_list[PERK_CLERIC_ARMOR_OF_FAITH_3];
  perk->id = PERK_CLERIC_ARMOR_OF_FAITH_3;
  perk->name = strdup("Armor of Faith III");
  perk->description = strdup("+2 AC and +2 to all saves");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_ARMOR_OF_FAITH_2;
  perk->prerequisite_rank = 2; /* Must have Armor of Faith II maxed */
  perk->effect_type = PERK_EFFECT_AC;
  perk->effect_value = 2; /* +2 AC */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Armor of Faith II (max). Divine protection reaches its peak, granting +2 AC and +2 to all saving throws.");

  /* Smite Evil III */
  perk = &perk_list[PERK_CLERIC_SMITE_EVIL_3];
  perk->id = PERK_CLERIC_SMITE_EVIL_3;
  perk->name = strdup("Smite Evil III");
  perk->description = strdup("Smite deals +6d6 damage, usable 3/combat");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_SMITE_EVIL_2;
  perk->prerequisite_rank = 1; /* Must have Smite Evil II */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 6; /* 6d6 damage */
  perk->effect_modifier = 3; /* 3 uses per combat */
  perk->special_description = strdup("Requires Smite Evil II. Your smite becomes even more powerful, dealing 6d6 holy damage and usable 3 times per combat.");

  /* Channel Energy: Greater Harm */
  perk = &perk_list[PERK_CLERIC_CHANNEL_ENERGY_GREATER_HARM];
  perk->id = PERK_CLERIC_CHANNEL_ENERGY_GREATER_HARM;
  perk->name = strdup("Channel Energy: Greater Harm");
  perk->description = strdup("Channel harm damage increases to 4d6");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_CHANNEL_ENERGY_HARM;
  perk->prerequisite_rank = 1; /* Must have Channel Energy: Harm */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4; /* Increases channel level bonus */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Channel Energy: Harm. Your negative energy channeling becomes more potent, dealing 4d6 damage instead of 2d6.");

  /* Righteous Fury */
  perk = &perk_list[PERK_CLERIC_RIGHTEOUS_FURY];
  perk->id = PERK_CLERIC_RIGHTEOUS_FURY;
  perk->name = strdup("Righteous Fury");
  perk->description = strdup("Critical hits deal +2d6 holy damage");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_HOLY_WEAPON_2;
  perk->prerequisite_rank = 1; /* Must have at least 1 rank of Holy Weapon II */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* 2d6 damage */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Holy Weapon II (at least 1 rank). When you score a critical hit with a melee weapon, divine wrath adds an extra 2d6 holy damage.");

  /*** Battle Cleric Tree - Tier 4 Capstone Perks ***/

  /* Avatar of War */
  perk = &perk_list[PERK_CLERIC_AVATAR_OF_WAR];
  perk->id = PERK_CLERIC_AVATAR_OF_WAR;
  perk->name = strdup("Avatar of War");
  perk->description = strdup("+3 hit/AC, +10 damage for 10 rounds, 1/day");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_DIVINE_FAVOR_3;
  perk->prerequisite_rank = 1; /* Must have Divine Favor III */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10; /* Duration in rounds */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Divine Favor III and Armor of Faith III. Transform into an avatar of divine war, gaining +3 to hit, +3 AC, and +10 to damage for 10 rounds. Usable once per day.");

  /* Divine Wrath */
  perk = &perk_list[PERK_CLERIC_DIVINE_WRATH];
  perk->id = PERK_CLERIC_DIVINE_WRATH;
  perk->name = strdup("Divine Wrath");
  perk->description = strdup("Smite +10d6, channel harm 6d6, holy damage +10");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_SMITE_EVIL_3;
  perk->prerequisite_rank = 1; /* Must have Smite Evil III */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10; /* Various bonuses */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Smite Evil III and Holy Weapon III. Divine wrath flows through your attacks. Smite Evil deals +10d6 damage, Channel Harm deals 6d6, and all holy damage from Holy Weapon is increased by +10.");
  
  /*** DOMAIN MASTER TREE - TIER 1 PERKS (1 point each) ***/
  
  /* Domain Focus I */
  perk = &perk_list[PERK_CLERIC_DOMAIN_FOCUS_1];
  perk->id = PERK_CLERIC_DOMAIN_FOCUS_1;
  perk->name = strdup("Domain Focus I");
  perk->description = strdup("+1 to domain spell DC per rank");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 DC per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Increases the save DC of spells from your chosen domains by +1 per rank. Can be taken 3 times for +3 DC total.");
  
  /* Divine Spell Power I */
  perk = &perk_list[PERK_CLERIC_DIVINE_SPELL_POWER_1];
  perk->id = PERK_CLERIC_DIVINE_SPELL_POWER_1;
  perk->name = strdup("Divine Spell Power I");
  perk->description = strdup("+1 to divine spell damage per rank");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 1;
  perk->max_rank = 5;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 damage per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Increases damage dealt by all divine offensive spells by +1 per rank. Can be taken 5 times for +5 damage total.");
  
  /* Bonus Domain Spell I */
  perk = &perk_list[PERK_CLERIC_SPELL_POINT_RESERVE_1];
  perk->id = PERK_CLERIC_SPELL_POINT_RESERVE_1;
  perk->name = strdup("Bonus Domain Spell I");
  perk->description = strdup("Prepare +1 additional domain spell per rank");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 1;
  perk->max_rank = 5;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 domain spell per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Allows you to prepare one additional domain spell per rank when you prepare spells. Can be taken 5 times for +5 domain spell slots total.");
  
  /* Turn Undead Enhancement I */
  perk = &perk_list[PERK_CLERIC_TURN_UNDEAD_ENHANCEMENT_1];
  perk->id = PERK_CLERIC_TURN_UNDEAD_ENHANCEMENT_1;
  perk->name = strdup("Turn Undead Enhancement I");
  perk->description = strdup("+1 to turn undead DC per rank");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 turn undead DC per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Increases the save DC for your turn undead ability by +1 per rank. Can be taken 3 times for +3 DC total.");
  
  /*** DOMAIN MASTER TREE - TIER 2 PERKS (2 points each) ***/
  
  /* Domain Focus II */
  perk = &perk_list[PERK_CLERIC_DOMAIN_FOCUS_2];
  perk->id = PERK_CLERIC_DOMAIN_FOCUS_2;
  perk->name = strdup("Domain Focus II");
  perk->description = strdup("Additional +1 domain spell DC per rank");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_CLERIC_DOMAIN_FOCUS_1;
  perk->prerequisite_rank = 3; /* Must max Domain Focus I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 DC per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Domain Focus I (max). Further increases domain spell DC by +1 per rank. Can be taken 2 times for +5 DC total with Domain Focus I.");
  
  /* Divine Spell Power II */
  perk = &perk_list[PERK_CLERIC_DIVINE_SPELL_POWER_2];
  perk->id = PERK_CLERIC_DIVINE_SPELL_POWER_2;
  perk->name = strdup("Divine Spell Power II");
  perk->description = strdup("Additional +2 divine spell damage per rank");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 2;
  perk->max_rank = 3;
  perk->prerequisite_perk = PERK_CLERIC_DIVINE_SPELL_POWER_1;
  perk->prerequisite_rank = 5; /* Must max Divine Spell Power I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 damage per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Divine Spell Power I (max). Further increases divine spell damage by +2 per rank. Can be taken 3 times for +11 damage total with Divine Spell Power I.");
  
  /* Bonus Domain Spell II */
  perk = &perk_list[PERK_CLERIC_SPELL_POINT_RESERVE_2];
  perk->id = PERK_CLERIC_SPELL_POINT_RESERVE_2;
  perk->name = strdup("Bonus Domain Spell II");
  perk->description = strdup("Prepare +1 additional spell of any level per rank");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 2;
  perk->max_rank = 3;
  perk->prerequisite_perk = PERK_CLERIC_SPELL_POINT_RESERVE_1;
  perk->prerequisite_rank = 5; /* Must max Bonus Domain Spell I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 spell of any level per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Bonus Domain Spell I (max). Allows you to prepare one additional spell of any level per rank. Can be taken 3 times for +8 total spell slots with Bonus Domain Spell I.");
  
  /* Turn Undead Enhancement II */
  perk = &perk_list[PERK_CLERIC_TURN_UNDEAD_ENHANCEMENT_2];
  perk->id = PERK_CLERIC_TURN_UNDEAD_ENHANCEMENT_2;
  perk->name = strdup("Turn Undead Enhancement II");
  perk->description = strdup("Additional +2 turn undead DC per rank");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_CLERIC_TURN_UNDEAD_ENHANCEMENT_1;
  perk->prerequisite_rank = 3; /* Must max Turn Undead Enhancement I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 turn undead DC per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Turn Undead Enhancement I (max). Further increases turn undead DC by +2 per rank. Can be taken 2 times for +7 DC total with Turn Undead Enhancement I.");
  
  /* Extended Domain */
  perk = &perk_list[PERK_CLERIC_EXTENDED_DOMAIN];
  perk->id = PERK_CLERIC_EXTENDED_DOMAIN;
  perk->name = strdup("Extended Domain");
  perk->description = strdup("Domain spell duration +5 rounds");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_DOMAIN_FOCUS_1;
  perk->prerequisite_rank = 2; /* Must have Domain Focus I at least 2 ranks */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5; /* +5 rounds duration */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Domain Focus I (at least 2 ranks). Domain spells have their duration extended by +5 rounds.");
  
  /* Divine Metamagic I */
  perk = &perk_list[PERK_CLERIC_DIVINE_METAMAGIC_1];
  perk->id = PERK_CLERIC_DIVINE_METAMAGIC_1;
  perk->name = strdup("Divine Metamagic I");
  perk->description = strdup("Apply metamagic to divine spells without increasing spell level");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_SPELL_POINT_RESERVE_1;
  perk->prerequisite_rank = 3; /* Must have Bonus Domain Spell I at least 3 ranks */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* Reduce metamagic level increase by 1 */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Bonus Domain Spell I (at least 3 ranks). When applying metamagic feats to divine spells, the effective spell level increase is reduced by 1 (minimum +0). For example, Empower Spell normally increases spell level by +2, but with this perk it only increases by +1.");
  
  /* Destroy Undead */
  perk = &perk_list[PERK_CLERIC_DESTROY_UNDEAD];
  perk->id = PERK_CLERIC_DESTROY_UNDEAD;
  perk->name = strdup("Destroy Undead");
  perk->description = strdup("Turn undead can destroy weak undead instantly");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_TURN_UNDEAD_ENHANCEMENT_1;
  perk->prerequisite_rank = 3; /* Must max Turn Undead Enhancement I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* Enable destroy undead */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Turn Undead Enhancement I (max). When you successfully turn undead, weak undead (3 HD or less below your cleric level) are instantly destroyed instead of fleeing.");

  /*** DOMAIN MASTER TREE - TIER 3 PERKS (3-4 points each) ***/
  
  /* Domain Focus III */
  perk = &perk_list[PERK_CLERIC_DOMAIN_FOCUS_3];
  perk->id = PERK_CLERIC_DOMAIN_FOCUS_3;
  perk->name = strdup("Domain Focus III");
  perk->description = strdup("+2 DC for domain spells");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_DOMAIN_FOCUS_2;
  perk->prerequisite_rank = 5; /* Must max Domain Focus II */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 DC for domain spells */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Domain Focus II (max). Grants +2 DC to all domain spell saving throws. Stacks with Domain Focus I and II.");
  
  /* Divine Spell Power III (Rank 1-2) */
  perk = &perk_list[PERK_CLERIC_DIVINE_SPELL_POWER_3];
  perk->id = PERK_CLERIC_DIVINE_SPELL_POWER_3;
  perk->name = strdup("Divine Spell Power III");
  perk->description = strdup("+3 damage to divine spells per rank");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 3;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_CLERIC_DIVINE_SPELL_POWER_2;
  perk->prerequisite_rank = 5; /* Must max Divine Spell Power II */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3; /* +3 damage per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Divine Spell Power II (max). Grants +3 bonus damage to all divine spells per rank. Can be taken 2 times for +6 total. Stacks with Divine Spell Power I and II.");
  
  /* Bonus Domain Spell III (Rank 1-2) */
  perk = &perk_list[PERK_CLERIC_SPELL_POINT_RESERVE_3];
  perk->id = PERK_CLERIC_SPELL_POINT_RESERVE_3;
  perk->name = strdup("Bonus Domain Spell III");
  perk->description = strdup("+1 bonus domain spell per rank");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 3;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_CLERIC_SPELL_POINT_RESERVE_2;
  perk->prerequisite_rank = 3; /* Must max Bonus Domain Spell II */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 spell slot per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Bonus Domain Spell II (max). Grants +1 bonus domain spell slot of any level per rank. Can be taken 2 times. Regenerates 1 slot per 5 minutes.");
  
  /* Divine Metamagic II */
  perk = &perk_list[PERK_CLERIC_DIVINE_METAMAGIC_2];
  perk->id = PERK_CLERIC_DIVINE_METAMAGIC_2;
  perk->name = strdup("Divine Metamagic II");
  perk->description = strdup("Metamagic feats increase spell level by 2 less");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_DIVINE_METAMAGIC_1;
  perk->prerequisite_rank = 1; /* Must have Divine Metamagic I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* -2 spell level increase */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Divine Metamagic I. When applying metamagic feats to your divine spells, the spell level increase is reduced by 2. Stacks with Divine Metamagic I for -3 total.");
  
  /* Greater Turning */
  perk = &perk_list[PERK_CLERIC_GREATER_TURNING];
  perk->id = PERK_CLERIC_GREATER_TURNING;
  perk->name = strdup("Greater Turning");
  perk->description = strdup("Turn undead affects undead +2 HD levels higher");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_TURN_UNDEAD_ENHANCEMENT_2;
  perk->prerequisite_rank = 5; /* Must max Turn Undead Enhancement II */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 HD levels */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Turn Undead Enhancement II (max). Your turn undead ability can affect undead up to 2 HD levels higher than normal.");
  
  /* Domain Mastery */
  perk = &perk_list[PERK_CLERIC_DOMAIN_MASTERY];
  perk->id = PERK_CLERIC_DOMAIN_MASTERY;
  perk->name = strdup("Domain Mastery");
  perk->description = strdup("Use domain powers +1 additional time per day");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_EXTENDED_DOMAIN;
  perk->prerequisite_rank = 1; /* Must have Extended Domain */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 daily use */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Extended Domain. You can use your domain powers one additional time per day.");

  /*** DOMAIN MASTER TREE - TIER 4 CAPSTONE PERKS (5 points each) ***/
  
  /* Divine Channeler */
  perk = &perk_list[PERK_CLERIC_DIVINE_CHANNELER];
  perk->id = PERK_CLERIC_DIVINE_CHANNELER;
  perk->name = strdup("Divine Channeler");
  perk->description = strdup("Master of divine magic: +3 DC, +10 damage, domain powers 2x/day");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_DOMAIN_FOCUS_3;
  perk->prerequisite_rank = 1; /* Must have Domain Focus III */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3; /* +3 DC bonus */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Domain Focus III. CAPSTONE: All divine spells gain +3 DC and +10 damage. Domain powers can be used twice as often per day.");
  
  /* Master of the Undead */
  perk = &perk_list[PERK_CLERIC_MASTER_OF_UNDEAD];
  perk->id = PERK_CLERIC_MASTER_OF_UNDEAD;
  perk->name = strdup("Master of the Undead");
  perk->description = strdup("Ultimate turning: +5 turn DC, control undead, destroy up to 10 HD");
  perk->associated_class = CLASS_CLERIC;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_GREATER_TURNING;
  perk->prerequisite_rank = 1; /* Must have Greater Turning */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5; /* +5 turn DC */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Greater Turning. CAPSTONE: Turn undead DC +5, can control turned undead, destroy undead up to 10 HD below your cleric level.");
}

/* Define Rogue Perks */
void define_rogue_perks(void)
{
  struct perk_data *perk;
  
  /*** ASSASSIN TREE - TIER 1 PERKS (1 point each) ***/
  
  /* Sneak Attack I (Rank 1-5) */
  perk = &perk_list[PERK_ROGUE_SNEAK_ATTACK_1];
  perk->id = PERK_ROGUE_SNEAK_ATTACK_1;
  perk->name = strdup("Sneak Attack I");
  perk->description = strdup("+1d6 sneak attack damage per rank");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 1;
  perk->max_rank = 5;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Adds 1d6 sneak attack damage per rank. Can be taken 5 times for +5d6 total.");
  
  /* Vital Strike */
  perk = &perk_list[PERK_ROGUE_VITAL_STRIKE];
  perk->id = PERK_ROGUE_VITAL_STRIKE;
  perk->name = strdup("Vital Strike");
  perk->description = strdup("+2 to confirm critical hits");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Grants +2 bonus to critical hit confirmation rolls.");
  
  /* Deadly Aim I (Rank 1-3) */
  perk = &perk_list[PERK_ROGUE_DEADLY_AIM_1];
  perk->id = PERK_ROGUE_DEADLY_AIM_1;
  perk->name = strdup("Deadly Aim I");
  perk->description = strdup("+1 damage with ranged sneak attacks per rank");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Adds +1 damage to ranged sneak attacks per rank. Can be taken 3 times.");
  
  /* Opportunist I */
  perk = &perk_list[PERK_ROGUE_OPPORTUNIST_1];
  perk->id = PERK_ROGUE_OPPORTUNIST_1;
  perk->name = strdup("Opportunist I");
  perk->description = strdup("+1 attack of opportunity per round");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Grants one additional attack of opportunity per round.");
  
  /*** ASSASSIN TREE - TIER 2 PERKS (2 points each) ***/
  
  /* Sneak Attack II (Rank 1-3) */
  perk = &perk_list[PERK_ROGUE_SNEAK_ATTACK_2];
  perk->id = PERK_ROGUE_SNEAK_ATTACK_2;
  perk->name = strdup("Sneak Attack II");
  perk->description = strdup("Additional +1d6 sneak attack damage per rank");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 2;
  perk->max_rank = 3;
  perk->prerequisite_perk = PERK_ROGUE_SNEAK_ATTACK_1;
  perk->prerequisite_rank = 5; /* Must have max rank (5) of Sneak Attack I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Sneak Attack I at max rank. Adds additional 1d6 sneak attack damage per rank.");
  
  /* Improved Vital Strike */
  perk = &perk_list[PERK_ROGUE_IMPROVED_VITAL_STRIKE];
  perk->id = PERK_ROGUE_IMPROVED_VITAL_STRIKE;
  perk->name = strdup("Improved Vital Strike");
  perk->description = strdup("Additional +2 to confirm criticals (+4 total)");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_VITAL_STRIKE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Vital Strike. Grants additional +2 bonus to critical confirmation (+4 total).");
  
  /* Assassinate I */
  perk = &perk_list[PERK_ROGUE_ASSASSINATE_1];
  perk->id = PERK_ROGUE_ASSASSINATE_1;
  perk->name = strdup("Assassinate I");
  perk->description = strdup("Sneak attacks from stealth deal +2d6 damage");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_SNEAK_ATTACK_1;
  perk->prerequisite_rank = 3; /* Must have at least 3 ranks of Sneak Attack I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Sneak Attack I (at least 3 ranks). Attacks from stealth deal +2d6 additional damage.");
  
  /* Deadly Aim II (Rank 1-2) */
  perk = &perk_list[PERK_ROGUE_DEADLY_AIM_2];
  perk->id = PERK_ROGUE_DEADLY_AIM_2;
  perk->name = strdup("Deadly Aim II");
  perk->description = strdup("Additional +2 damage with ranged sneak attacks per rank");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_ROGUE_DEADLY_AIM_1;
  perk->prerequisite_rank = 3; /* Must have max rank (3) of Deadly Aim I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Deadly Aim I at max rank. Adds +2 damage to ranged sneak attacks per rank.");
  
  /* Crippling Strike */
  perk = &perk_list[PERK_ROGUE_CRIPPLING_STRIKE];
  perk->id = PERK_ROGUE_CRIPPLING_STRIKE;
  perk->name = strdup("Crippling Strike");
  perk->description = strdup("Sneak attacks reduce target movement by 50% for 3 rounds");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_SNEAK_ATTACK_1;
  perk->prerequisite_rank = 3; /* Must have at least 3 ranks of Sneak Attack I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 50; /* 50% movement reduction */
  perk->effect_modifier = 3; /* Duration: 3 rounds */
  perk->special_description = strdup("Requires Sneak Attack I (at least 3 ranks). Sneak attacks apply movement speed reduction.");
  
  /* Bleeding Attack */
  perk = &perk_list[PERK_ROGUE_BLEEDING_ATTACK];
  perk->id = PERK_ROGUE_BLEEDING_ATTACK;
  perk->name = strdup("Bleeding Attack");
  perk->description = strdup("Sneak attacks cause target to bleed for 1d6 damage per round (5 rounds)");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_SNEAK_ATTACK_1;
  perk->prerequisite_rank = 3; /* Must have at least 3 ranks of Sneak Attack I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* 1d6 damage per round */
  perk->effect_modifier = 5; /* Duration: 5 rounds */
  perk->special_description = strdup("Requires Sneak Attack I (at least 3 ranks). Sneak attacks apply bleeding damage over time.");
  
  /*** ASSASSIN TREE - TIER 3 PERKS (3-4 points each) ***/
  
  /* Sneak Attack III (Rank 1-2) */
  perk = &perk_list[PERK_ROGUE_SNEAK_ATTACK_3];
  perk->id = PERK_ROGUE_SNEAK_ATTACK_3;
  perk->name = strdup("Sneak Attack III");
  perk->description = strdup("Additional +2d6 sneak attack damage per rank");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 3;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_ROGUE_SNEAK_ATTACK_2;
  perk->prerequisite_rank = 3; /* Must have max rank (3) of Sneak Attack II */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2d6 per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Sneak Attack II at max rank. Adds +2d6 sneak attack damage per rank (total +4d6).");
  
  /* Assassinate II */
  perk = &perk_list[PERK_ROGUE_ASSASSINATE_2];
  perk->id = PERK_ROGUE_ASSASSINATE_2;
  perk->name = strdup("Assassinate II");
  perk->description = strdup("Stealth sneak attacks deal +4d6 damage (total +6d6)");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_ASSASSINATE_1;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4; /* Additional +4d6 (for +6d6 total with Assassinate I) */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Assassinate I. Attacks from stealth deal +4d6 additional damage (+6d6 total).");
  
  /* Critical Precision */
  perk = &perk_list[PERK_ROGUE_CRITICAL_PRECISION];
  perk->id = PERK_ROGUE_CRITICAL_PRECISION;
  perk->name = strdup("Critical Precision");
  perk->description = strdup("Critical hits deal +2d6 precision damage");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_IMPROVED_VITAL_STRIKE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2d6 on critical hits */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Improved Vital Strike. Critical hits deal +2d6 precision damage.");
  
  /* Opportunist II */
  perk = &perk_list[PERK_ROGUE_OPPORTUNIST_2];
  perk->id = PERK_ROGUE_OPPORTUNIST_2;
  perk->name = strdup("Opportunist II");
  perk->description = strdup("Attacks of opportunity are automatically sneak attacks");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_OPPORTUNIST_1;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Opportunist I. All attacks of opportunity automatically count as sneak attacks.");
  
  /* Death Attack */
  perk = &perk_list[PERK_ROGUE_DEATH_ATTACK];
  perk->id = PERK_ROGUE_DEATH_ATTACK;
  perk->name = strdup("Death Attack");
  perk->description = strdup("Gives +2 to backstab dc modifiers to apply death attack. Requires the death attack assassin class ability.");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_ASSASSINATE_1;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Assassinate I and Sneak Attack II (at least 2 ranks). Study a target for 1 round, then your next sneak attack forces a Fortitude save or the target dies.");
  
  /*** ASSASSIN TREE - TIER 4 CAPSTONE PERKS (5 points each) ***/
  
  /* Master Assassin */
  perk = &perk_list[PERK_ROGUE_MASTER_ASSASSIN];
  perk->id = PERK_ROGUE_MASTER_ASSASSIN;
  perk->name = strdup("Master Assassin");
  perk->description = strdup("All sneak attacks +5d6, critical threat range +1, sneak from any position");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_SNEAK_ATTACK_3;
  perk->prerequisite_rank = 2; /* Must have max rank (2) of Sneak Attack III */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5; /* +5d6 sneak attack damage */
  perk->effect_modifier = 1; /* +1 critical threat range */
  perk->special_description = strdup("Requires Sneak Attack III at max rank and Assassinate II. All sneak attacks gain +5d6 damage, critical threat range increases by 1, and you can sneak attack from any position.");
  
  /* Perfect Kill */
  perk = &perk_list[PERK_ROGUE_PERFECT_KILL];
  perk->id = PERK_ROGUE_PERFECT_KILL;
  perk->name = strdup("Perfect Kill");
  perk->description = strdup("Once per combat, declare one attack auto-critical with max damage");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_CRITICAL_PRECISION;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Critical Precision and Death Attack. Once per combat, you can declare one attack to be an automatic critical hit with maximum damage.");
  
  /*** MASTER THIEF TREE - TIER 1 PERKS (1 point each) ***/
  
  /* Skill Mastery I */
  perk = &perk_list[PERK_ROGUE_SKILL_MASTERY_1];
  perk->id = PERK_ROGUE_SKILL_MASTERY_1;
  perk->name = strdup("Skill Mastery I");
  perk->description = strdup("+2 to all rogue skills per rank");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 1;
  perk->max_rank = 5;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SKILL;
  perk->effect_value = 2; /* +2 per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Grants +2 to stealth, sleight of hand, perception, disable device, and pick locks per rank.");
  
  /* Trapfinding Expert I */
  perk = &perk_list[PERK_ROGUE_TRAPFINDING_EXPERT_1];
  perk->id = PERK_ROGUE_TRAPFINDING_EXPERT_1;
  perk->name = strdup("Trapfinding Expert I");
  perk->description = strdup("+3 to find and disable traps per rank");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SKILL;
  perk->effect_value = 3; /* +3 per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Grants +3 bonus to perception for finding traps and disable device per rank.");
  
  /* Fast Hands I */
  perk = &perk_list[PERK_ROGUE_FAST_HANDS_1];
  perk->id = PERK_ROGUE_FAST_HANDS_1;
  perk->name = strdup("Fast Hands I");
  perk->description = strdup("+2 to sleight of hand and pick locks per rank");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SKILL;
  perk->effect_value = 2; /* +2 per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Grants +2 bonus to sleight of hand and pick locks per rank.");
  
  /* Evasion Training */
  perk = &perk_list[PERK_ROGUE_EVASION_TRAINING];
  perk->id = PERK_ROGUE_EVASION_TRAINING;
  perk->name = strdup("Evasion Training");
  perk->description = strdup("+3 to saves vs area effects, half damage on successful save");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SAVE;
  perk->effect_value = 3; /* +3 to saves vs area effects */
  perk->effect_modifier = -1; /* Applies to all save types */
  perk->special_description = strdup("+3 bonus to saving throws against area of effect spells. When you succeed on a Reflex save against an area effect attack, you take half damage instead of full damage.");
  
  /*** MASTER THIEF TREE - TIER 2 PERKS (2 points each) ***/
  
  /* Skill Mastery II */
  perk = &perk_list[PERK_ROGUE_SKILL_MASTERY_2];
  perk->id = PERK_ROGUE_SKILL_MASTERY_2;
  perk->name = strdup("Skill Mastery II");
  perk->description = strdup("Additional +3 to all rogue skills per rank");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 2;
  perk->max_rank = 3;
  perk->prerequisite_perk = PERK_ROGUE_SKILL_MASTERY_1;
  perk->prerequisite_rank = 5; /* Must have max rank (5) of Skill Mastery I */
  perk->effect_type = PERK_EFFECT_SKILL;
  perk->effect_value = 3; /* +3 per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Skill Mastery I at max rank. Grants additional +3 to all rogue skills per rank.");
  
  /* Trapfinding Expert II */
  perk = &perk_list[PERK_ROGUE_TRAPFINDING_EXPERT_2];
  perk->id = PERK_ROGUE_TRAPFINDING_EXPERT_2;
  perk->name = strdup("Trapfinding Expert II");
  perk->description = strdup("Additional +4 to find/disable traps per rank");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_ROGUE_TRAPFINDING_EXPERT_1;
  perk->prerequisite_rank = 3; /* Must have max rank (3) of Trapfinding Expert I */
  perk->effect_type = PERK_EFFECT_SKILL;
  perk->effect_value = 4; /* +4 per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Trapfinding Expert I at max rank. Grants additional +4 to find and disable traps per rank.");
  
  /* Fast Hands II */
  perk = &perk_list[PERK_ROGUE_FAST_HANDS_2];
  perk->id = PERK_ROGUE_FAST_HANDS_2;
  perk->name = strdup("Fast Hands II");
  perk->description = strdup("Additional +3 to sleight of hand and locks per rank");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_ROGUE_FAST_HANDS_1;
  perk->prerequisite_rank = 3; /* Must have max rank (3) of Fast Hands I */
  perk->effect_type = PERK_EFFECT_SKILL;
  perk->effect_value = 3; /* +3 per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Fast Hands I at max rank. Grants additional +3 to sleight of hand and pick locks per rank.");
  
  /* Improved Evasion */
  perk = &perk_list[PERK_ROGUE_IMPROVED_EVASION];
  perk->id = PERK_ROGUE_IMPROVED_EVASION;
  perk->name = strdup("Improved Evasion");
  perk->description = strdup("+3 to saves vs area effects, half damage on fail, no damage on success");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_EVASION_TRAINING;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SAVE;
  perk->effect_value = 3; /* +3 to saves vs area effects */
  perk->effect_modifier = -1; /* Applies to all save types */
  perk->special_description = strdup("Requires Evasion Training. +3 bonus to saving throws against area of effect spells. Take no damage on successful Reflex save, half damage on failed save against area effects.");
  
  /* Trap Sense I */
  perk = &perk_list[PERK_ROGUE_TRAP_SENSE_1];
  perk->id = PERK_ROGUE_TRAP_SENSE_1;
  perk->name = strdup("Trap Sense I");
  perk->description = strdup("+2 to saves vs traps, +2 AC vs trap attacks per rank");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_ROGUE_TRAPFINDING_EXPERT_1;
  perk->prerequisite_rank = 2; /* Must have at least 2 ranks of Trapfinding Expert I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 saves vs traps per rank */
  perk->effect_modifier = 2; /* +2 AC vs trap attacks per rank */
  perk->special_description = strdup("Requires Trapfinding Expert I (at least 2 ranks). Grants +2 to saves vs traps and +2 AC vs trap attacks per rank.");
  
  /* Resiliency */
  perk = &perk_list[PERK_ROGUE_RESILIENCY];
  perk->id = PERK_ROGUE_RESILIENCY;
  perk->name = strdup("Resiliency");
  perk->description = strdup("Once per day, stay at 1 HP when reduced to 0 or less");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_EVASION_TRAINING;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Evasion Training. Once per day, when reduced to 0 or fewer hit points, you automatically stabilize at 1 HP.");
  
  /* Trap Scavenger */
  perk = &perk_list[PERK_ROGUE_TRAP_SCAVENGER];
  perk->id = PERK_ROGUE_TRAP_SCAVENGER;
  perk->name = strdup("Trap Scavenger");
  perk->description = strdup("Can salvage components from disabled traps");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_TRAPFINDING_EXPERT_1;
  perk->prerequisite_rank = 3; /* Must have max rank (3) of Trapfinding Expert I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Trapfinding Expert I at max rank. When you successfully disable a trap, you can salvage crafting components from it.");
  
  /*** MASTER THIEF TREE - TIER 3 PERKS (3 points each) ***/
  
  /* Skill Mastery III */
  perk = &perk_list[PERK_ROGUE_SKILL_MASTERY_3];
  perk->id = PERK_ROGUE_SKILL_MASTERY_3;
  perk->name = strdup("Skill Mastery III");
  perk->description = strdup("Additional +4 to all rogue skills per rank");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 3;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_ROGUE_SKILL_MASTERY_2;
  perk->prerequisite_rank = 3; /* Must have max rank (3) of Skill Mastery II */
  perk->effect_type = PERK_EFFECT_SKILL;
  perk->effect_value = 4; /* +4 per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Skill Mastery II at max rank. Grants additional +4 to all rogue skills per rank.");
  
  /* Trapfinding Expert III */
  perk = &perk_list[PERK_ROGUE_TRAPFINDING_EXPERT_3];
  perk->id = PERK_ROGUE_TRAPFINDING_EXPERT_3;
  perk->name = strdup("Trapfinding Expert III");
  perk->description = strdup("Additional +5 to find/disable traps per rank");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 3;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_ROGUE_TRAPFINDING_EXPERT_2;
  perk->prerequisite_rank = 2; /* Must have max rank (2) of Trapfinding Expert II */
  perk->effect_type = PERK_EFFECT_SKILL;
  perk->effect_value = 5; /* +5 per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Trapfinding Expert II at max rank. Grants additional +5 to find and disable traps per rank.");
  
  /* Fast Hands III */
  perk = &perk_list[PERK_ROGUE_FAST_HANDS_3];
  perk->id = PERK_ROGUE_FAST_HANDS_3;
  perk->name = strdup("Fast Hands III");
  perk->description = strdup("Additional +4 to sleight of hand and locks per rank");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 3;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_ROGUE_FAST_HANDS_2;
  perk->prerequisite_rank = 2; /* Must have max rank (2) of Fast Hands II */
  perk->effect_type = PERK_EFFECT_SKILL;
  perk->effect_value = 4; /* +4 per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Fast Hands II at max rank. Grants additional +4 to sleight of hand and pick locks per rank.");
  
  /* Shadow Step */
  perk = &perk_list[PERK_ROGUE_SHADOW_STEP];
  perk->id = PERK_ROGUE_SHADOW_STEP;
  perk->name = strdup("Shadow Step");
  perk->description = strdup("+5 to stealth, can hide while being observed");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_SKILL_MASTERY_2;
  perk->prerequisite_rank = 2; /* Must have at least 2 ranks of Skill Mastery II */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5; /* +5 to stealth */
  perk->effect_modifier = 1; /* Can hide while observed */
  perk->special_description = strdup("Requires Skill Mastery II (at least 2 ranks). Grants +5 bonus to stealth and allows you to attempt to hide even while being observed.");
  
  /* Trap Sense II */
  perk = &perk_list[PERK_ROGUE_TRAP_SENSE_2];
  perk->id = PERK_ROGUE_TRAP_SENSE_2;
  perk->name = strdup("Trap Sense II");
  perk->description = strdup("Additional +3 to saves vs traps, +3 AC vs trap attacks per rank");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 3;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_ROGUE_TRAP_SENSE_1;
  perk->prerequisite_rank = 2; /* Must have max rank (2) of Trap Sense I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3; /* +3 saves vs traps per rank */
  perk->effect_modifier = 3; /* +3 AC vs trap attacks per rank */
  perk->special_description = strdup("Requires Trap Sense I at max rank. Grants additional +3 to saves vs traps and +3 AC vs trap attacks per rank.");
  
  /*** MASTER THIEF TREE - TIER 4 PERKS (4 points each) ***/
  
  /* Master Thief Capstone */
  perk = &perk_list[PERK_ROGUE_MASTER_THIEF_CAPSTONE];
  perk->id = PERK_ROGUE_MASTER_THIEF_CAPSTONE;
  perk->name = strdup("Master Thief");
  perk->description = strdup("+10 to all rogue skills, take 10 on any rogue skill check");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_SKILL_MASTERY_3;
  perk->prerequisite_rank = 2; /* Must have max rank (2) of Skill Mastery III */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10; /* +10 to all rogue skills */
  perk->effect_modifier = 1; /* Can take 10 on rogue skills */
  perk->special_description = strdup("Requires Skill Mastery III at max rank. The ultimate expression of rogue mastery. Grants +10 to all rogue skills and allows you to 'take 10' on any rogue skill check, even in combat or under pressure.");
  
  /* Legendary Reflexes */
  perk = &perk_list[PERK_ROGUE_LEGENDARY_REFLEXES];
  perk->id = PERK_ROGUE_LEGENDARY_REFLEXES;
  perk->name = strdup("Legendary Reflexes");
  perk->description = strdup("+5 to all saves, take no damage from area effects on successful save");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_IMPROVED_EVASION;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5; /* +5 to all saves */
  perk->effect_modifier = 1; /* Take no damage from area effects on successful save */
  perk->special_description = strdup("Requires Improved Evasion. Your reflexes have reached legendary status. Grants +5 to all saving throws and you take no damage from area effects on successful saves (even Fortitude/Will saves), and only half damage on failed saves.");
  
  /*** SHADOW SCOUT TREE - TIER 1 PERKS (1 point each) ***/
  
  /* Stealth Mastery I */
  perk = &perk_list[PERK_ROGUE_STEALTH_MASTERY_1];
  perk->id = PERK_ROGUE_STEALTH_MASTERY_1;
  perk->name = strdup("Stealth Mastery I");
  perk->description = strdup("+3 to stealth per rank");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 1;
  perk->max_rank = 5;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SKILL;
  perk->effect_value = 3; /* +3 per rank */
  perk->effect_modifier = ABILITY_STEALTH;
  perk->special_description = strdup("Grants +3 bonus to stealth per rank.");
  
  /* Fleet of Foot I */
  perk = &perk_list[PERK_ROGUE_FLEET_OF_FOOT_1];
  perk->id = PERK_ROGUE_FLEET_OF_FOOT_1;
  perk->name = strdup("Fleet of Foot I");
  perk->description = strdup("+5 movement speed per rank");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5; /* +5 movement per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Grants +5 movement speed per rank.");
  
  /* Awareness I */
  perk = &perk_list[PERK_ROGUE_AWARENESS_1];
  perk->id = PERK_ROGUE_AWARENESS_1;
  perk->name = strdup("Awareness I");
  perk->description = strdup("+3 to perception and search per rank");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SKILL;
  perk->effect_value = 3; /* +3 per rank */
  perk->effect_modifier = ABILITY_PERCEPTION; /* Also applies to search */
  perk->special_description = strdup("Grants +3 bonus to perception and search per rank.");
  
  /* Light Step */
  perk = &perk_list[PERK_ROGUE_LIGHT_STEP];
  perk->id = PERK_ROGUE_LIGHT_STEP;
  perk->name = strdup("Light Step");
  perk->description = strdup("Don't trigger movement-based traps or floor detection");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Your footsteps are so light that you don't trigger movement-based traps or pressure plates.");
  
  /*** SHADOW SCOUT TREE - TIER 2 PERKS (2 points each) ***/
  
  /* Stealth Mastery II */
  perk = &perk_list[PERK_ROGUE_STEALTH_MASTERY_2];
  perk->id = PERK_ROGUE_STEALTH_MASTERY_2;
  perk->name = strdup("Stealth Mastery II");
  perk->description = strdup("Additional +4 to stealth per rank");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 2;
  perk->max_rank = 3;
  perk->prerequisite_perk = PERK_ROGUE_STEALTH_MASTERY_1;
  perk->prerequisite_rank = 5; /* Must have max rank (5) of Stealth Mastery I */
  perk->effect_type = PERK_EFFECT_SKILL;
  perk->effect_value = 4; /* +4 per rank */
  perk->effect_modifier = ABILITY_STEALTH;
  perk->special_description = strdup("Requires Stealth Mastery I at max rank. Grants additional +4 to stealth per rank.");
  
  /* Fleet of Foot II */
  perk = &perk_list[PERK_ROGUE_FLEET_OF_FOOT_2];
  perk->id = PERK_ROGUE_FLEET_OF_FOOT_2;
  perk->name = strdup("Fleet of Foot II");
  perk->description = strdup("Additional +10 movement speed per rank");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_ROGUE_FLEET_OF_FOOT_1;
  perk->prerequisite_rank = 3; /* Must have max rank (3) of Fleet of Foot I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10; /* +10 movement per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Fleet of Foot I at max rank. Grants additional +10 movement speed per rank.");
  
  /* Awareness II */
  perk = &perk_list[PERK_ROGUE_AWARENESS_2];
  perk->id = PERK_ROGUE_AWARENESS_2;
  perk->name = strdup("Awareness II");
  perk->description = strdup("Additional +4 to perception/search per rank");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_ROGUE_AWARENESS_1;
  perk->prerequisite_rank = 3; /* Must have max rank (3) of Awareness I */
  perk->effect_type = PERK_EFFECT_SKILL;
  perk->effect_value = 4; /* +4 per rank */
  perk->effect_modifier = ABILITY_PERCEPTION;
  perk->special_description = strdup("Requires Awareness I at max rank. Grants additional +4 to perception/search per rank.");
  
  /* Hide in Plain Sight */
  perk = &perk_list[PERK_ROGUE_HIDE_IN_PLAIN_SIGHT];
  perk->id = PERK_ROGUE_HIDE_IN_PLAIN_SIGHT;
  perk->name = strdup("Hide in Plain Sight");
  perk->description = strdup("Can attempt to hide even while being observed");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_STEALTH_MASTERY_1;
  perk->prerequisite_rank = 3; /* Must have at least 3 ranks of Stealth Mastery I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Stealth Mastery I (at least 3 ranks). You can attempt to hide even while being observed, though this is more difficult than normal.");
  
  /* Shadow Step (Teleport) */
  perk = &perk_list[PERK_ROGUE_SHADOW_STEP_TELEPORT];
  perk->id = PERK_ROGUE_SHADOW_STEP_TELEPORT;
  perk->name = strdup("Shadow Step");
  perk->description = strdup("Can move through shadows to teleport short distances (10 ft)");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_STEALTH_MASTERY_1;
  perk->prerequisite_rank = 3; /* Must have at least 3 ranks of Stealth Mastery I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10; /* 10 ft teleport range */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Stealth Mastery I (at least 3 ranks). You can use your bonus action to teleport up to 10 feet to an unoccupied space you can see that is in dim light or darkness.");
  
  /* Uncanny Dodge I */
  perk = &perk_list[PERK_ROGUE_UNCANNY_DODGE_1];
  perk->id = PERK_ROGUE_UNCANNY_DODGE_1;
  perk->name = strdup("Uncanny Dodge I");
  perk->description = strdup("Cannot be caught flat-footed, retain DEX bonus to AC");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_AWARENESS_1;
  perk->prerequisite_rank = 2; /* Must have at least 2 ranks of Awareness I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Awareness I (at least 2 ranks). You cannot be caught flat-footed and always retain your Dexterity bonus to AC, even when surprised.");
  
  /* Acrobatics I */
  perk = &perk_list[PERK_ROGUE_ACROBATICS_1];
  perk->id = PERK_ROGUE_ACROBATICS_1;
  perk->name = strdup("Acrobatics I");
  perk->description = strdup("+3 to acrobatics, +1 AC per rank");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_ROGUE_FLEET_OF_FOOT_1;
  perk->prerequisite_rank = 2; /* Must have at least 2 ranks of Fleet of Foot I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3; /* +3 to acrobatics per rank */
  perk->effect_modifier = 1; /* +1 AC per rank */
  perk->special_description = strdup("Requires Fleet of Foot I (at least 2 ranks). Grants +3 to acrobatics and +1 AC per rank.");

  /*** SHADOW SCOUT TREE - TIER 3 PERKS (3-4 points each) ***/

  /* Stealth Mastery III */
  perk = &perk_list[PERK_ROGUE_STEALTH_MASTERY_3];
  perk->id = PERK_ROGUE_STEALTH_MASTERY_3;
  perk->name = strdup("Stealth Mastery III");
  perk->description = strdup("Additional +5 to stealth per rank");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 3;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_ROGUE_STEALTH_MASTERY_2;
  perk->prerequisite_rank = 3; /* Must have max ranks of Stealth Mastery II */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5; /* +5 to stealth per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Stealth Mastery II (max). Grants an additional +5 to stealth per rank.");

  /* Fleet of Foot III */
  perk = &perk_list[PERK_ROGUE_FLEET_OF_FOOT_3];
  perk->id = PERK_ROGUE_FLEET_OF_FOOT_3;
  perk->name = strdup("Fleet of Foot III");
  perk->description = strdup("+15 movement speed, can disengage as free action");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_FLEET_OF_FOOT_2;
  perk->prerequisite_rank = 2; /* Must have max ranks of Fleet of Foot II */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 15; /* +15 movement speed */
  perk->effect_modifier = 1; /* Disengage as free action flag */
  perk->special_description = strdup("Requires Fleet of Foot II (max). Grants +15 movement speed and allows you to disengage from combat as a free action.");

  /* Awareness III */
  perk = &perk_list[PERK_ROGUE_AWARENESS_3];
  perk->id = PERK_ROGUE_AWARENESS_3;
  perk->name = strdup("Awareness III");
  perk->description = strdup("+5 to perception/search, blindsense 10 ft");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_AWARENESS_2;
  perk->prerequisite_rank = 2; /* Must have max ranks of Awareness II */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5; /* +5 to perception/search */
  perk->effect_modifier = 10; /* Blindsense 10 ft */
  perk->special_description = strdup("Requires Awareness II (max). Grants +5 to perception and search skills, and provides blindsense within 10 feet.");

  /* Uncanny Dodge II */
  perk = &perk_list[PERK_ROGUE_UNCANNY_DODGE_2];
  perk->id = PERK_ROGUE_UNCANNY_DODGE_2;
  perk->name = strdup("Uncanny Dodge II");
  perk->description = strdup("Cannot be flanked, +2 AC vs attacks of opportunity");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_UNCANNY_DODGE_1;
  perk->prerequisite_rank = 1; /* Must have Uncanny Dodge I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 AC vs attacks of opportunity */
  perk->effect_modifier = 1; /* Cannot be flanked flag */
  perk->special_description = strdup("Requires Uncanny Dodge I. You cannot be flanked and gain +2 AC against attacks of opportunity.");

  /* Acrobatics II */
  perk = &perk_list[PERK_ROGUE_ACROBATICS_2];
  perk->id = PERK_ROGUE_ACROBATICS_2;
  perk->name = strdup("Acrobatics II");
  perk->description = strdup("+5 to acrobatics, +2 AC");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_ACROBATICS_1;
  perk->prerequisite_rank = 2; /* Must have max ranks of Acrobatics I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5; /* +5 to acrobatics */
  perk->effect_modifier = 2; /* +2 AC */
  perk->special_description = strdup("Requires Acrobatics I (max). Grants +5 to acrobatics and +2 AC.");

  /* Vanish */
  perk = &perk_list[PERK_ROGUE_VANISH];
  perk->id = PERK_ROGUE_VANISH;
  perk->name = strdup("Vanish");
  perk->description = strdup("Once per combat, become invisible for 3 rounds");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_HIDE_IN_PLAIN_SIGHT;
  perk->prerequisite_rank = 1; /* Must have Hide in Plain Sight */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3; /* 3 rounds of invisibility */
  perk->effect_modifier = 1; /* Once per combat flag */
  perk->special_description = strdup("Requires Hide in Plain Sight. Once per combat, you can vanish, becoming invisible for 3 rounds.");

  /*** SHADOW SCOUT TREE - TIER 4 PERKS (5 points each) ***/

  /* Shadow Master */
  perk = &perk_list[PERK_ROGUE_SHADOW_MASTER];
  perk->id = PERK_ROGUE_SHADOW_MASTER;
  perk->name = strdup("Shadow Master");
  perk->description = strdup("Perfect stealth, auto-hide after attacks, +20 to stealth");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_STEALTH_MASTERY_3;
  perk->prerequisite_rank = 2; /* Must have max ranks of Stealth Mastery III AND Hide in Plain Sight */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 20; /* +20 to stealth */
  perk->effect_modifier = 1; /* Auto-hide after attacks flag */
  perk->special_description = strdup("Requires Stealth Mastery III (max) and Hide in Plain Sight. You achieve perfect stealth mastery, automatically hiding after attacks and gaining +20 to stealth.");

  /* Ghost */
  perk = &perk_list[PERK_ROGUE_GHOST];
  perk->id = PERK_ROGUE_GHOST;
  perk->name = strdup("Ghost");
  perk->description = strdup("+30 movement, immune to attacks of opportunity, pass through enemies");
  perk->associated_class = CLASS_ROGUE;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_FLEET_OF_FOOT_3;
  perk->prerequisite_rank = 1; /* Must have Fleet of Foot III AND Uncanny Dodge II */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 30; /* +30 movement speed */
  perk->effect_modifier = 1; /* Immune to AoO and pass through enemies flag */
  perk->special_description = strdup("Requires Fleet of Foot III and Uncanny Dodge II. You move like a ghost, gaining +30 movement speed, immunity to attacks of opportunity, and the ability to pass through enemies.");
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
  
  /* Master Thief Skill Mastery perks apply to all rogue skills */
  switch (skill_num)
  {
    case ABILITY_STEALTH:
    case ABILITY_SLEIGHT_OF_HAND:
    case ABILITY_PERCEPTION:
    case ABILITY_DISABLE_DEVICE:
      bonus += get_perk_skill_mastery_bonus(ch);
      break;
  }
  
  /* Trapfinding Expert perks apply to perception (finding traps) and disable device */
  switch (skill_num)
  {
    case ABILITY_PERCEPTION:
    case ABILITY_DISABLE_DEVICE:
      bonus += get_perk_trapfinding_bonus(ch);
      break;
  }
  
  /* Fast Hands perks apply to sleight of hand and disable device (pick locks) */
  switch (skill_num)
  {
    case ABILITY_SLEIGHT_OF_HAND:
    case ABILITY_DISABLE_DEVICE:
      bonus += get_perk_fast_hands_bonus(ch);
      break;
  }
  
  /* Shadow Scout Stealth Mastery perks apply to stealth */
  if (skill_num == ABILITY_STEALTH)
  {
    bonus += get_perk_stealth_mastery_bonus(ch);
  }
  
  /* Shadow Scout Awareness perks apply to perception (and search) */
  if (skill_num == ABILITY_PERCEPTION)
  {
    bonus += get_perk_awareness_bonus(ch);
  }
  
  /* Shadow Scout Acrobatics perks apply to acrobatics */
  if (skill_num == ABILITY_ACROBATICS)
  {
    bonus += get_perk_acrobatics_bonus(ch);
  }
  
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

/**
 * Get total sneak attack dice bonus from perks.
 * 
 * @param ch The character
 * @return Total bonus sneak attack dice (e.g., 8 means +8d6)
 */
int get_perk_sneak_attack_dice(struct char_data *ch)
{
  int total = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Sneak Attack I: +1d6 per rank, max 5 ranks */
  total += get_total_perk_ranks(ch, PERK_ROGUE_SNEAK_ATTACK_1);
  
  /* Sneak Attack II: +1d6 per rank, max 3 ranks */
  total += get_total_perk_ranks(ch, PERK_ROGUE_SNEAK_ATTACK_2);
  
  /* Sneak Attack III: +2d6 per rank, max 2 ranks */
  total += 2 * get_total_perk_ranks(ch, PERK_ROGUE_SNEAK_ATTACK_3);
  
  /* Master Assassin: +5d6 */
  if (has_perk(ch, PERK_ROGUE_MASTER_ASSASSIN))
    total += 5;
  
  return total;
}

/**
 * Get critical confirmation bonus from perks.
 * 
 * @param ch The character
 * @return Total bonus to critical confirmation rolls
 */
int get_perk_critical_confirmation_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Vital Strike: +2 */
  if (has_perk(ch, PERK_ROGUE_VITAL_STRIKE))
    bonus += 2;
  
  /* Improved Vital Strike: +2 more (+4 total) */
  if (has_perk(ch, PERK_ROGUE_IMPROVED_VITAL_STRIKE))
    bonus += 2;
  
  return bonus;
}

/**
 * Get ranged sneak attack damage bonus from perks.
 * 
 * @param ch The character
 * @return Total flat damage bonus to ranged sneak attacks
 */
int get_perk_ranged_sneak_attack_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Deadly Aim I: +1 per rank, max 3 ranks */
  bonus += get_total_perk_ranks(ch, PERK_ROGUE_DEADLY_AIM_1);
  
  /* Deadly Aim II: +2 per rank, max 2 ranks */
  bonus += 2 * get_total_perk_ranks(ch, PERK_ROGUE_DEADLY_AIM_2);
  
  return bonus;
}

/**
 * Get assassinate bonus damage from stealth.
 * 
 * @param ch The character
 * @return Bonus sneak attack dice when attacking from stealth
 */
int get_perk_assassinate_bonus(struct char_data *ch)
{
  int total = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Assassinate I: +2d6 when attacking from stealth */
  if (has_perk(ch, PERK_ROGUE_ASSASSINATE_1))
    total += 2;
  
  /* Assassinate II: +4d6 additional (total +6d6) */
  if (has_perk(ch, PERK_ROGUE_ASSASSINATE_2))
    total += 4;
  
  return total;
}

/**
 * Get additional attacks of opportunity from perks.
 * 
 * @param ch The character
 * @return Number of additional AoOs per round
 */
int get_perk_aoo_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Opportunist I: +1 AoO */
  if (has_perk(ch, PERK_ROGUE_OPPORTUNIST_1))
    bonus += 1;
  
  return bonus;
}

/**
 * Get critical precision bonus damage on critical hits.
 * 
 * @param ch The character
 * @return Bonus precision damage dice on critical hits
 */
int get_perk_critical_precision_damage(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Critical Precision: +2d6 precision damage on crits */
  if (has_perk(ch, PERK_ROGUE_CRITICAL_PRECISION))
    return 2;
  
  return 0;
}

/**
 * Check if Master Assassin allows sneak attack from any position.
 * 
 * @param ch The character
 * @return TRUE if Master Assassin is active
 */
bool has_master_assassin(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_ROGUE_MASTER_ASSASSIN);
}

/**
 * Get Master Assassin critical threat range bonus.
 * 
 * @param ch The character
 * @return Critical threat range increase
 */
int get_master_assassin_crit_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Master Assassin: +1 critical threat range */
  if (has_perk(ch, PERK_ROGUE_MASTER_ASSASSIN))
    return 1;
  
  return 0;
}

/**
 * Check if Perfect Kill is available for use this combat.
 * Perfect Kill can be used once per combat, and resets 1 minute after combat ends.
 * 
 * @param ch The character
 * @return TRUE if Perfect Kill is available
 */
bool can_use_perfect_kill(struct char_data *ch)
{
  time_t current_time;
  
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  /* Must have the perk */
  if (!has_perk(ch, PERK_ROGUE_PERFECT_KILL))
    return FALSE;
  
  /* Check if 60 seconds have passed since last combat */
  current_time = time(0);
  if (ch->player_specials->saved.perfect_kill_last_combat > 0 &&
      (current_time - ch->player_specials->saved.perfect_kill_last_combat) >= 60)
  {
    /* Reset the flag after cooldown */
    ch->player_specials->saved.perfect_kill_used = FALSE;
  }
  
  /* Can use if not used yet this combat cycle */
  return !ch->player_specials->saved.perfect_kill_used;
}

/**
 * Mark Perfect Kill as used for this combat.
 * 
 * @param ch The character
 */
void use_perfect_kill(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return;
  
  ch->player_specials->saved.perfect_kill_used = TRUE;
}

/**
 * Update the last combat timestamp when combat ends.
 * This should be called when a character leaves combat.
 * 
 * @param ch The character
 */
void update_perfect_kill_combat_end(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return;
  
  /* Only update if they have the perk */
  if (!has_perk(ch, PERK_ROGUE_PERFECT_KILL))
    return;
  
  ch->player_specials->saved.perfect_kill_last_combat = time(0);
}

/**
 * Get total skill bonus from Master Thief perks for rogue skills.
 * 
 * @param ch The character
 * @return Total bonus to all rogue skills
 */
int get_perk_skill_mastery_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Skill Mastery I: +2 per rank, max 5 ranks */
  bonus += 2 * get_total_perk_ranks(ch, PERK_ROGUE_SKILL_MASTERY_1);
  
  /* Skill Mastery II: +3 per rank, max 3 ranks */
  bonus += 3 * get_total_perk_ranks(ch, PERK_ROGUE_SKILL_MASTERY_2);
  
  /* Skill Mastery III: +4 per rank, max 2 ranks */
  bonus += 4 * get_total_perk_ranks(ch, PERK_ROGUE_SKILL_MASTERY_3);
  
  /* Master Thief Capstone: +10 */
  if (has_perk(ch, PERK_ROGUE_MASTER_THIEF_CAPSTONE))
    bonus += 10;
  
  return bonus;
}

/**
 * Get trapfinding bonus from Master Thief perks.
 * 
 * @param ch The character
 * @return Total bonus to find and disable traps
 */
int get_perk_trapfinding_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Trapfinding Expert I: +3 per rank, max 3 ranks */
  bonus += 3 * get_total_perk_ranks(ch, PERK_ROGUE_TRAPFINDING_EXPERT_1);
  
  /* Trapfinding Expert II: +4 per rank, max 2 ranks */
  bonus += 4 * get_total_perk_ranks(ch, PERK_ROGUE_TRAPFINDING_EXPERT_2);
  
  /* Trapfinding Expert III: +5 per rank, max 2 ranks */
  bonus += 5 * get_total_perk_ranks(ch, PERK_ROGUE_TRAPFINDING_EXPERT_3);
  
  return bonus;
}

/**
 * Get Fast Hands bonus from Master Thief perks.
 * 
 * @param ch The character
 * @return Total bonus to sleight of hand and pick locks
 */
int get_perk_fast_hands_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Fast Hands I: +2 per rank, max 3 ranks */
  bonus += 2 * get_total_perk_ranks(ch, PERK_ROGUE_FAST_HANDS_1);
  
  /* Fast Hands II: +3 per rank, max 2 ranks */
  bonus += 3 * get_total_perk_ranks(ch, PERK_ROGUE_FAST_HANDS_2);
  
  /* Fast Hands III: +4 per rank, max 2 ranks */
  bonus += 4 * get_total_perk_ranks(ch, PERK_ROGUE_FAST_HANDS_3);
  
  return bonus;
}

/**
 * Get Trap Sense bonus to saves and AC vs traps.
 * 
 * @param ch The character
 * @return Total bonus to saves and AC vs traps
 */
int get_perk_trap_sense_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Trap Sense I: +2 per rank, max 2 ranks */
  bonus += 2 * get_total_perk_ranks(ch, PERK_ROGUE_TRAP_SENSE_1);
  
  /* Trap Sense II: +3 per rank, max 2 ranks */
  bonus += 3 * get_total_perk_ranks(ch, PERK_ROGUE_TRAP_SENSE_2);
  
  return bonus;
}

/**
 * Check if character has Shadow Step ability.
 * Shadow Step allows hiding while being observed.
 * 
 * @param ch The character
 * @return TRUE if character has Shadow Step
 */
bool has_shadow_step(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_ROGUE_SHADOW_STEP);
}

/**
 * Get Shadow Step stealth bonus.
 * 
 * @param ch The character
 * @return Bonus to stealth from Shadow Step (+5)
 */
int get_shadow_step_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_perk(ch, PERK_ROGUE_SHADOW_STEP))
    return 5;
  
  return 0;
}

/**
 * Check if character can "take 10" on rogue skill checks.
 * Master Thief capstone allows taking 10 even under pressure.
 * 
 * @param ch The character
 * @return TRUE if character can take 10 on rogue skills
 */
bool can_take_10_on_rogue_skills(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_ROGUE_MASTER_THIEF_CAPSTONE);
}

/**
 * Check if character has Legendary Reflexes.
 * 
 * @param ch The character
 * @return TRUE if character has Legendary Reflexes
 */
bool has_legendary_reflexes(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_ROGUE_LEGENDARY_REFLEXES);
}

/**
 * Get Legendary Reflexes save bonus.
 * 
 * @param ch The character
 * @return Bonus to all saves from Legendary Reflexes (+5)
 */
int get_legendary_reflexes_save_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_perk(ch, PERK_ROGUE_LEGENDARY_REFLEXES))
    return 5;
  
  return 0;
}

/**
 * Get stealth mastery bonus from Shadow Scout perks.
 * 
 * @param ch The character
 * @return Total bonus to stealth
 */
int get_perk_stealth_mastery_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Stealth Mastery I: +3 per rank, max 5 ranks */
  bonus += 3 * get_total_perk_ranks(ch, PERK_ROGUE_STEALTH_MASTERY_1);
  
  /* Stealth Mastery II: +4 per rank, max 3 ranks */
  bonus += 4 * get_total_perk_ranks(ch, PERK_ROGUE_STEALTH_MASTERY_2);
  
  /* Stealth Mastery III: +5 per rank, max 2 ranks */
  bonus += 5 * get_total_perk_ranks(ch, PERK_ROGUE_STEALTH_MASTERY_3);
  
  /* Shadow Master: +20 to stealth */
  if (has_perk(ch, PERK_ROGUE_SHADOW_MASTER))
    bonus += 20;
  
  return bonus;
}

/**
 * Get fleet of foot movement speed bonus.
 * 
 * @param ch The character
 * @return Total movement speed bonus
 */
int get_perk_fleet_of_foot_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Fleet of Foot I: +5 per rank, max 3 ranks */
  bonus += 5 * get_total_perk_ranks(ch, PERK_ROGUE_FLEET_OF_FOOT_1);
  
  /* Fleet of Foot II: +10 per rank, max 2 ranks */
  bonus += 10 * get_total_perk_ranks(ch, PERK_ROGUE_FLEET_OF_FOOT_2);
  
  /* Fleet of Foot III: +15 */
  if (has_perk(ch, PERK_ROGUE_FLEET_OF_FOOT_3))
    bonus += 15;
  
  /* Ghost: +30 movement speed */
  if (has_perk(ch, PERK_ROGUE_GHOST))
    bonus += 30;
  
  return bonus;
}

/**
 * Get awareness bonus to perception and search.
 * 
 * @param ch The character
 * @return Total bonus to perception/search
 */
int get_perk_awareness_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Awareness I: +3 per rank, max 3 ranks */
  bonus += 3 * get_total_perk_ranks(ch, PERK_ROGUE_AWARENESS_1);
  
  /* Awareness II: +4 per rank, max 2 ranks */
  bonus += 4 * get_total_perk_ranks(ch, PERK_ROGUE_AWARENESS_2);
  
  /* Awareness III: +5 */
  if (has_perk(ch, PERK_ROGUE_AWARENESS_3))
    bonus += 5;
  
  return bonus;
}

/**
 * Check if character has Light Step.
 * Light Step prevents triggering movement-based traps.
 * 
 * @param ch The character
 * @return TRUE if character has Light Step
 */
bool has_light_step(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_ROGUE_LIGHT_STEP);
}

/**
 * Check if character has Hide in Plain Sight.
 * Allows hiding while being observed.
 * 
 * @param ch The character
 * @return TRUE if character has Hide in Plain Sight
 */
bool has_hide_in_plain_sight(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_ROGUE_HIDE_IN_PLAIN_SIGHT);
}

/**
 * Check if character has Shadow Step teleport ability.
 * 
 * @param ch The character
 * @return TRUE if character has Shadow Step teleport
 */
bool has_shadow_step_teleport(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_ROGUE_SHADOW_STEP_TELEPORT);
}

/**
 * Get Shadow Step teleport range.
 * 
 * @param ch The character
 * @return Teleport range in feet (10)
 */
int get_shadow_step_range(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_perk(ch, PERK_ROGUE_SHADOW_STEP_TELEPORT))
    return 10;
  
  return 0;
}

/**
 * Check if character has Uncanny Dodge.
 * Cannot be caught flat-footed.
 * 
 * @param ch The character
 * @return TRUE if character has Uncanny Dodge
 */
bool has_uncanny_dodge(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_ROGUE_UNCANNY_DODGE_1);
}

/**
 * Get acrobatics bonus from perks.
 * 
 * @param ch The character
 * @return Total bonus to acrobatics
 */
int get_perk_acrobatics_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Acrobatics I: +3 per rank, max 2 ranks */
  bonus += 3 * get_total_perk_ranks(ch, PERK_ROGUE_ACROBATICS_1);
  
  /* Acrobatics II: +5 */
  if (has_perk(ch, PERK_ROGUE_ACROBATICS_2))
    bonus += 5;
  
  return bonus;
}

/**
 * Get AC bonus from acrobatics perks.
 * 
 * @param ch The character
 * @return AC bonus from acrobatics
 */
int get_perk_acrobatics_ac_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Acrobatics I: +1 AC per rank, max 2 ranks */
  bonus += get_total_perk_ranks(ch, PERK_ROGUE_ACROBATICS_1);
  
  /* Acrobatics II: +2 AC */
  if (has_perk(ch, PERK_ROGUE_ACROBATICS_2))
    bonus += 2;
  
  return bonus;
}

/**
 * Check if character has Awareness III (grants blindsense).
 * 
 * @param ch The character
 * @return TRUE if character has Awareness III
 */
bool has_awareness_3(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_ROGUE_AWARENESS_3);
}

/**
 * Get blindsense range from Awareness III.
 * 
 * @param ch The character
 * @return Blindsense range in feet (0 if no blindsense)
 */
int get_perk_blindsense_range(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_perk(ch, PERK_ROGUE_AWARENESS_3))
    return 10; /* 10 feet blindsense */
  
  return 0;
}

/**
 * Check if character has Uncanny Dodge II.
 * Cannot be flanked and gains AC bonus vs AoO.
 * 
 * @param ch The character
 * @return TRUE if character has Uncanny Dodge II
 */
bool has_uncanny_dodge_2(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_ROGUE_UNCANNY_DODGE_2);
}

/**
 * Get AC bonus vs attacks of opportunity from Uncanny Dodge II.
 * 
 * @param ch The character
 * @return AC bonus vs AoO
 */
int get_uncanny_dodge_aoo_ac_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_perk(ch, PERK_ROGUE_UNCANNY_DODGE_2))
    return 2;
  
  return 0;
}

/**
 * Check if character can disengage as a free action (Fleet of Foot III).
 * 
 * @param ch The character
 * @return TRUE if character can disengage as free action
 */
bool can_disengage_free_action(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_ROGUE_FLEET_OF_FOOT_3);
}

/**
 * Check if character has Vanish ability.
 * 
 * @param ch The character
 * @return TRUE if character has Vanish
 */
bool has_vanish(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_ROGUE_VANISH);
}

/**
 * Check if character has Shadow Master.
 * Auto-hide after attacks and perfect stealth.
 * 
 * @param ch The character
 * @return TRUE if character has Shadow Master
 */
bool has_shadow_master(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_ROGUE_SHADOW_MASTER);
}

/**
 * Check if character has Ghost perk.
 * Immune to AoO and can pass through enemies.
 * 
 * @param ch The character
 * @return TRUE if character has Ghost
 */
bool has_ghost(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_ROGUE_GHOST);
}

/*****************************************************************************
 * Cleric Perk Helper Functions
 *****************************************************************************/

/**
 * Get healing power bonus from Divine Healer perks.
 * 
 * @param ch The character
 * @return Total bonus HP to healing spells
 */
int get_cleric_healing_power_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Healing Power I: +2 per rank, max 5 ranks */
  bonus += 2 * get_total_perk_ranks(ch, PERK_CLERIC_HEALING_POWER_1);
  
  /* Healing Power II: +3 per rank, max 3 ranks */
  bonus += 3 * get_total_perk_ranks(ch, PERK_CLERIC_HEALING_POWER_2);
  
  /* Healing Power III: +5 per rank, max 2 ranks */
  bonus += 5 * get_total_perk_ranks(ch, PERK_CLERIC_HEALING_POWER_3);
  
  /* Divine Radiance: +20 HP capstone */
  if (has_perk(ch, PERK_CLERIC_DIVINE_RADIANCE))
    bonus += 20;
  
  return bonus;
}

/**
 * Get radiant servant bonus to positive energy.
 * 
 * @param ch The character
 * @return Total bonus to positive energy effects
 */
int get_cleric_radiant_servant_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Radiant Servant I: +1 per rank, max 3 ranks */
  bonus += get_total_perk_ranks(ch, PERK_CLERIC_RADIANT_SERVANT_1);
  
  /* Radiant Servant II: +2 per rank, max 2 ranks */
  bonus += 2 * get_total_perk_ranks(ch, PERK_CLERIC_RADIANT_SERVANT_2);
  
  return bonus;
}

/**
 * Check if character has Efficient Healing perk.
 * Allows one extra healing spell per rest.
 * 
 * @param ch The character
 * @return TRUE if character has Efficient Healing
 */
bool has_efficient_healing(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_CLERIC_EFFICIENT_HEALING);
}

/**
 * Get preserve life bonus when target is below 50% HP.
 * 
 * @param ch The caster
 * @param target The healing target
 * @return Bonus HP when target below 50%, 0 otherwise
 */
int get_preserve_life_bonus(struct char_data *ch, struct char_data *target)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch) || !target)
    return 0;
  
  /* Only applies if target is below 50% HP */
  if (GET_HIT(target) >= (GET_MAX_HIT(target) / 2))
    return 0;
  
  /* Preserve Life: +5 HP per rank, max 3 ranks */
  bonus = 5 * get_total_perk_ranks(ch, PERK_CLERIC_PRESERVE_LIFE);
  
  return bonus;
}

/**
 * Get mass healing focus bonus targets.
 * 
 * @param ch The character
 * @return Number of additional targets for mass healing spells
 */
int get_mass_healing_focus_targets(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_perk(ch, PERK_CLERIC_MASS_HEALING_FOCUS))
    return 2;
  
  return 0;
}

/**
 * Check if healing spell is empowered (critical heal).
 * 
 * @param ch The character
 * @return TRUE if this heal should be empowered (10% or 20% chance)
 */
bool is_healing_empowered(struct char_data *ch)
{
  int chance = 0;
  
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  /* Empowered Healing II: 20% chance */
  if (has_perk(ch, PERK_CLERIC_EMPOWERED_HEALING_2))
    chance = 20;
  /* Empowered Healing I: 10% chance */
  else if (has_perk(ch, PERK_CLERIC_EMPOWERED_HEALING_1))
    chance = 10;
  else
    return FALSE;
  
  return (rand_number(1, 100) <= chance);
}

/**
 * Get empowered healing multiplier.
 * 
 * @param ch The character
 * @return 200 for tier 2, 150 for tier 1, 100 for none
 */
int get_empowered_healing_multiplier(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 100;
  
  /* Empowered Healing II: 200% (double) */
  if (has_perk(ch, PERK_CLERIC_EMPOWERED_HEALING_2))
    return 200;
  
  /* Empowered Healing I: 150% */
  if (has_perk(ch, PERK_CLERIC_EMPOWERED_HEALING_1))
    return 150;
  
  return 100;
}

/**
 * Check if character has Channel Energy: Heal ability.
 * 
 * @param ch The character
 * @return TRUE if character can channel energy to heal
 */
bool has_channel_energy_heal(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_CLERIC_CHANNEL_ENERGY_HEAL);
}

/**
 * Check if character has Healing Aura.
 * 
 * @param ch The character
 * @return TRUE if character has healing aura (any tier)
 */
bool has_healing_aura(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return (has_perk(ch, PERK_CLERIC_HEALING_AURA_1) || 
          has_perk(ch, PERK_CLERIC_HEALING_AURA_2));
}

/**
 * Get healing aura HP bonus per tick.
 * 
 * @param ch The character with the aura
 * @return HP bonus per regen tick
 */
int get_healing_aura_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Divine Radiance capstone: +3 HP/tick */
  if (has_perk(ch, PERK_CLERIC_DIVINE_RADIANCE))
    return 3;
  
  /* Healing Aura II: +2 HP/tick */
  if (has_perk(ch, PERK_CLERIC_HEALING_AURA_2))
    return 2;
  
  /* Healing Aura I: +1 HP/tick */
  if (has_perk(ch, PERK_CLERIC_HEALING_AURA_1))
    return 1;
  
  return 0;
}

/**
 * Get healing aura range in rooms.
 * 
 * @param ch The character with the aura
 * @return Range in rooms
 */
int get_healing_aura_range(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Healing Aura II or Divine Radiance: 3 room range */
  if (has_perk(ch, PERK_CLERIC_HEALING_AURA_2) || 
      has_perk(ch, PERK_CLERIC_DIVINE_RADIANCE))
    return 3;
  
  /* Healing Aura I: 2 room range */
  if (has_perk(ch, PERK_CLERIC_HEALING_AURA_1))
    return 2;
  
  return 0;
}

/**
 * Get channel energy dice from perks.
 * 
 * @param ch The character
 * @return Number of d6 dice for channel energy
 */
int get_channel_energy_dice(struct char_data *ch)
{
  int dice = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Base from level */
  dice = (compute_channel_energy_level(ch) + 1) / 2;
  
  /* Divine Radiance capstone: 6d6 */
  if (has_perk(ch, PERK_CLERIC_DIVINE_RADIANCE))
    return 6;
  
  /* Channel Energy: Greater Heal: 4d6 */
  if (has_perk(ch, PERK_CLERIC_CHANNEL_ENERGY_GREATER_HEAL))
    return 4;
  
  /* Channel Energy: Heal: 2d6 minimum */
  if (has_perk(ch, PERK_CLERIC_CHANNEL_ENERGY_HEAL))
    return MAX(2, dice);
  
  return dice;
}

/**
 * Check if character has Restorative Touch.
 * 
 * @param ch The character
 * @return TRUE if character has restorative touch
 */
bool has_restorative_touch(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_CLERIC_RESTORATIVE_TOUCH);
}

/**
 * Check if character has Divine Radiance.
 * 
 * @param ch The character
 * @return TRUE if character has divine radiance capstone
 */
bool has_divine_radiance(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_CLERIC_DIVINE_RADIANCE);
}

/**
 * Check if character has Beacon of Hope.
 * 
 * @param ch The character
 * @return TRUE if character has beacon of hope capstone
 */
bool has_beacon_of_hope(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_CLERIC_BEACON_OF_HOPE);
}

/*****************************************************************************
 * Battle Cleric Perk Helper Functions
 *****************************************************************************/

/**
 * Get divine favor to-hit bonus from Battle Cleric perks.
 * 
 * @param ch The character
 * @return Total bonus to attack rolls
 */
int get_cleric_divine_favor_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Divine Favor I: +1 per rank, max 3 ranks */
  bonus += get_total_perk_ranks(ch, PERK_CLERIC_DIVINE_FAVOR_1);
  
  /* Divine Favor II: +1 per rank, max 2 ranks */
  bonus += get_total_perk_ranks(ch, PERK_CLERIC_DIVINE_FAVOR_2);
  
  return bonus;
}

/**
 * Get holy weapon damage bonus from Battle Cleric perks.
 * 
 * @param ch The character
 * @return Total holy damage bonus to melee attacks
 */
int get_cleric_holy_weapon_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Holy Weapon I: +2 per rank, max 3 ranks */
  bonus += 2 * get_total_perk_ranks(ch, PERK_CLERIC_HOLY_WEAPON_1);
  
  /* Holy Weapon II: +3 per rank, max 2 ranks */
  bonus += 3 * get_total_perk_ranks(ch, PERK_CLERIC_HOLY_WEAPON_2);
  
  return bonus;
}

/**
 * Get armor of faith AC bonus from Battle Cleric perks.
 * 
 * @param ch The character
 * @return Total AC bonus
 */
int get_cleric_armor_of_faith_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Armor of Faith I: +1 per rank, max 3 ranks */
  bonus += get_total_perk_ranks(ch, PERK_CLERIC_ARMOR_OF_FAITH_1);
  
  /* Armor of Faith II: +1 per rank, max 2 ranks */
  bonus += get_total_perk_ranks(ch, PERK_CLERIC_ARMOR_OF_FAITH_2);
  
  /* Armor of Faith III: +2 AC */
  if (has_perk(ch, PERK_CLERIC_ARMOR_OF_FAITH_3))
    bonus += 2;
  
  /* Avatar of War: +3 AC when active */
  if (affected_by_spell(ch, PERK_CLERIC_AVATAR_OF_WAR))
    bonus += 3;
  
  return bonus;
}

/**
 * Get Armor of Faith III save bonus (+2 to all saves).
 * 
 * @param ch The character
 * @return Save bonus
 */
int get_cleric_armor_of_faith_save_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_perk(ch, PERK_CLERIC_ARMOR_OF_FAITH_3))
    return 2;
  
  return 0;
}

/**
 * Check if character has Righteous Fury perk.
 * 
 * @param ch The character
 * @return TRUE if has perk, FALSE otherwise
 */
bool has_righteous_fury(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_CLERIC_RIGHTEOUS_FURY);
}

/**
 * Check if character has Divine Wrath capstone.
 * 
 * @param ch The character
 * @return TRUE if has perk, FALSE otherwise
 */
bool has_divine_wrath(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_CLERIC_DIVINE_WRATH);
}

/**
 * Get total holy weapon damage bonus including Divine Wrath.
 * 
 * @param ch The character
 * @return Total holy damage bonus
 */
int get_cleric_total_holy_weapon_bonus(struct char_data *ch)
{
  int bonus = get_cleric_holy_weapon_bonus(ch);
  
  if (!ch || IS_NPC(ch))
    return bonus;
  
  /* Holy Weapon III: +5 holy damage */
  if (has_perk(ch, PERK_CLERIC_HOLY_WEAPON_3))
    bonus += 5;
  
  /* Divine Wrath: +10 holy damage */
  if (has_divine_wrath(ch))
    bonus += 10;
  
  /* Avatar of War: +10 damage when active */
  if (affected_by_spell(ch, PERK_CLERIC_AVATAR_OF_WAR))
    bonus += 10;
  
  return bonus;
}

/**
 * Get total divine favor to-hit bonus including tier 3.
 * 
 * @param ch The character
 * @return Total to-hit bonus
 */
int get_cleric_total_divine_favor_bonus(struct char_data *ch)
{
  int bonus = get_cleric_divine_favor_bonus(ch);
  
  if (!ch || IS_NPC(ch))
    return bonus;
  
  /* Divine Favor III: +2 to hit */
  if (has_perk(ch, PERK_CLERIC_DIVINE_FAVOR_3))
    bonus += 2;
  
  /* Avatar of War: +3 to hit when active */
  if (affected_by_spell(ch, PERK_CLERIC_AVATAR_OF_WAR))
    bonus += 3;
  
  return bonus;
}

/**
 * Check if character has Battle Blessing perk.
 * 
 * @param ch The character
 * @return TRUE if character can cast self-buffs as swift action
 */
bool has_battle_blessing(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_CLERIC_BATTLE_BLESSING);
}

/**
 * Get smite evil damage dice.
 * 
 * @param ch The character
 * @return Number of d6 dice for smite damage (2d6 or 4d6)
 */
int get_cleric_smite_evil_dice(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Divine Wrath: 10d6 damage */
  if (has_divine_wrath(ch))
    return 10;
  
  /* Smite Evil III: 6d6 damage */
  if (has_perk(ch, PERK_CLERIC_SMITE_EVIL_3))
    return 6;
  
  /* Smite Evil II: 4d6 damage */
  if (has_perk(ch, PERK_CLERIC_SMITE_EVIL_2))
    return 4;
  
  /* Smite Evil I: 2d6 damage */
  if (has_perk(ch, PERK_CLERIC_SMITE_EVIL_1))
    return 2;
  
  return 0;
}

/**
 * Get number of smites available per combat.
 * 
 * @param ch The character
 * @return Number of smites (1, 2, or 3)
 */
int get_cleric_smite_evil_uses(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Smite Evil III: 3 uses per combat */
  if (has_perk(ch, PERK_CLERIC_SMITE_EVIL_3))
    return 3;
  
  /* Smite Evil II: 2 uses per combat */
  if (has_perk(ch, PERK_CLERIC_SMITE_EVIL_2))
    return 2;
  
  /* Smite Evil I: 1 use per combat */
  if (has_perk(ch, PERK_CLERIC_SMITE_EVIL_1))
    return 1;
  
  return 0;
}

/**
 * Get divine power damage bonus.
 * 
 * @param ch The character
 * @return Damage bonus to melee attacks
 */
int get_cleric_divine_power_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_perk(ch, PERK_CLERIC_DIVINE_POWER))
    return 2;
  
  return 0;
}

/**
 * Check if character has Channel Energy: Harm.
 * 
 * @param ch The character
 * @return TRUE if character can channel negative energy
 */
bool has_channel_energy_harm(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_CLERIC_CHANNEL_ENERGY_HARM);
}

/**
 * Check if character has Spiritual Weapon perk.
 * 
 * @param ch The character
 * @return TRUE if character can summon spiritual weapon
 */
bool has_spiritual_weapon(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_CLERIC_SPIRITUAL_WEAPON);
}

/**
 * Check if the given class is a divine spellcasting class.
 * Divine classes are: Cleric, Druid, Ranger, Paladin, Blackguard, Inquisitor
 * 
 * @param class_num The class number to check
 * @return TRUE if divine class, FALSE otherwise
 */
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

/**
 * Get domain focus DC bonus from Domain Master perks.
 * 
 * @param ch The character
 * @return Total DC bonus for domain spells
 */
int get_cleric_domain_focus_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Domain Focus I: +1 per rank, max 3 ranks */
  bonus += get_total_perk_ranks(ch, PERK_CLERIC_DOMAIN_FOCUS_1);
  
  /* Domain Focus II: +1 per rank, max 2 ranks */
  bonus += get_total_perk_ranks(ch, PERK_CLERIC_DOMAIN_FOCUS_2);
  
  /* Domain Focus III: +2 DC (1 rank) */
  if (has_perk(ch, PERK_CLERIC_DOMAIN_FOCUS_3))
    bonus += 2;
  
  /* Divine Channeler: +3 DC (capstone) */
  if (has_perk(ch, PERK_CLERIC_DIVINE_CHANNELER))
    bonus += 3;
  
  return bonus;
}

/**
 * Get divine spell power damage bonus from Domain Master perks.
 * 
 * @param ch The character
 * @return Total damage bonus for divine offensive spells
 */
int get_cleric_divine_spell_power_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Divine Spell Power I: +1 per rank, max 5 ranks */
  bonus += get_total_perk_ranks(ch, PERK_CLERIC_DIVINE_SPELL_POWER_1);
  
  /* Divine Spell Power II: +2 per rank, max 3 ranks */
  bonus += 2 * get_total_perk_ranks(ch, PERK_CLERIC_DIVINE_SPELL_POWER_2);
  
  /* Divine Spell Power III: +3 per rank, max 2 ranks */
  bonus += 3 * get_total_perk_ranks(ch, PERK_CLERIC_DIVINE_SPELL_POWER_3);
  
  /* Divine Channeler: +10 damage (capstone) */
  if (has_perk(ch, PERK_CLERIC_DIVINE_CHANNELER))
    bonus += 10;
  
  return bonus;
}

/**
 * Get bonus domain spell slots from Domain Master perks.
 * 
 * @param ch The character
 * @return Total bonus domain spell slots
 */
int get_cleric_bonus_domain_spells(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Bonus Domain Spell I: +1 per rank, max 5 ranks */
  bonus += get_total_perk_ranks(ch, PERK_CLERIC_SPELL_POINT_RESERVE_1);
  
  /* Bonus Domain Spell II: +1 per rank, max 3 ranks */
  bonus += get_total_perk_ranks(ch, PERK_CLERIC_SPELL_POINT_RESERVE_2);
  
  /* Bonus Domain Spell III: +1 per rank, max 2 ranks */
  bonus += get_total_perk_ranks(ch, PERK_CLERIC_SPELL_POINT_RESERVE_3);
  
  return bonus;
}

/**
 * Get bonus spell slots (any level) from Domain Master perks.
 * 
 * @param ch The character
 * @return Total bonus spell slots of any level
 */
int get_cleric_bonus_spell_slots(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Bonus Domain Spell II: +1 per rank, max 3 ranks */
  bonus += get_total_perk_ranks(ch, PERK_CLERIC_SPELL_POINT_RESERVE_2);
  
  return bonus;
}

/**
 * Get turn undead enhancement DC bonus from Domain Master perks.
 * 
 * @param ch The character
 * @return Total DC bonus for turn undead
 */
int get_cleric_turn_undead_enhancement_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Turn Undead Enhancement I: +1 per rank, max 3 ranks */
  bonus += get_total_perk_ranks(ch, PERK_CLERIC_TURN_UNDEAD_ENHANCEMENT_1);
  
  /* Turn Undead Enhancement II: +2 per rank, max 2 ranks */
  bonus += 2 * get_total_perk_ranks(ch, PERK_CLERIC_TURN_UNDEAD_ENHANCEMENT_2);
  
  return bonus;
}

/**
 * Get extended domain duration bonus.
 * 
 * @param ch The character
 * @return Duration bonus in rounds (5 if has perk, 0 otherwise)
 */
int get_cleric_extended_domain_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_perk(ch, PERK_CLERIC_EXTENDED_DOMAIN))
    return 5;
  
  return 0;
}

/**
 * Get divine metamagic spell level reduction.
 * 
 * @param ch The character
 * @return Spell level increase reduction when applying metamagic
 */
int get_cleric_divine_metamagic_reduction(struct char_data *ch)
{
  int reduction = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Divine Metamagic I: -1 spell level increase */
  if (has_perk(ch, PERK_CLERIC_DIVINE_METAMAGIC_1))
    reduction += 1;
  
  /* Divine Metamagic II: -2 spell level increase */
  if (has_perk(ch, PERK_CLERIC_DIVINE_METAMAGIC_2))
    reduction += 2;
  
  return reduction;
}

/**
 * Check if character can destroy undead with turn undead.
 * 
 * @param ch The character
 * @return TRUE if weak undead are destroyed instead of turned
 */
bool has_destroy_undead(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_CLERIC_DESTROY_UNDEAD);
}

/**
 * Get greater turning HD bonus for turn undead.
 * 
 * @param ch The character
 * @return HD levels bonus for affecting higher HD undead
 */
int get_cleric_greater_turning_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Greater Turning: +2 HD levels */
  if (has_perk(ch, PERK_CLERIC_GREATER_TURNING))
    return 2;
  
  return 0;
}

/**
 * Get domain mastery bonus uses per day.
 * 
 * @param ch The character
 * @return Additional daily uses for domain powers
 */
int get_cleric_domain_mastery_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Domain Mastery: +1 daily use */
  if (has_perk(ch, PERK_CLERIC_DOMAIN_MASTERY))
    bonus += 1;
  
  /* Divine Channeler: 2x daily uses (doubles all domain power uses) */
  if (has_perk(ch, PERK_CLERIC_DIVINE_CHANNELER))
    bonus = -1; /* Special value to indicate doubling */
  
  return bonus;
}

/**
 * Get turn undead DC bonus from Master of Undead.
 * 
 * @param ch The character
 * @return Turn undead DC bonus
 */
int get_cleric_master_of_undead_dc_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Master of the Undead: +5 turn DC */
  if (has_perk(ch, PERK_CLERIC_MASTER_OF_UNDEAD))
    return 5;
  
  return 0;
}

/**
 * Check if character can control turned undead.
 * 
 * @param ch The character
 * @return TRUE if can control turned undead
 */
bool has_control_undead(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_CLERIC_MASTER_OF_UNDEAD);
}

/**
 * Get destroy undead HD threshold.
 * 
 * @param ch The character
 * @return HD below cleric level at which undead are destroyed
 */
int get_destroy_undead_threshold(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Destroy Undead: 3 HD or less below cleric level */
  if (has_perk(ch, PERK_CLERIC_DESTROY_UNDEAD))
    return 3;
  
  /* Master of the Undead: 10 HD or less below cleric level */
  if (has_perk(ch, PERK_CLERIC_MASTER_OF_UNDEAD))
    return 10;
  
  return 0;
}

/******************************************************************************
 * Wizard Controller Tree Perk Functions
 ******************************************************************************/

/**
 * Get DC bonus from Spell Focus (Enchantment) perks.
 * 
 * @param ch The character
 * @return Total DC bonus for enchantment spells
 */
int get_enchantment_spell_dc_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Spell Focus I-V: +1 per rank */
  bonus += get_perk_rank(ch, PERK_WIZARD_SPELL_FOCUS_ENCHANTMENT_1, CLASS_WIZARD);
  bonus += get_perk_rank(ch, PERK_WIZARD_SPELL_FOCUS_ENCHANTMENT_2, CLASS_WIZARD);
  bonus += get_perk_rank(ch, PERK_WIZARD_SPELL_FOCUS_ENCHANTMENT_3, CLASS_WIZARD);
  bonus += get_perk_rank(ch, PERK_WIZARD_SPELL_FOCUS_ENCHANTMENT_4, CLASS_WIZARD);
  bonus += get_perk_rank(ch, PERK_WIZARD_SPELL_FOCUS_ENCHANTMENT_5, CLASS_WIZARD);
  
  /* Greater Spell Focus I-III: +2 per rank */
  bonus += get_perk_rank(ch, PERK_WIZARD_GREATER_SPELL_FOCUS_ENCHANTMENT_1, CLASS_WIZARD) * 2;
  bonus += get_perk_rank(ch, PERK_WIZARD_GREATER_SPELL_FOCUS_ENCHANTMENT_2, CLASS_WIZARD) * 2;
  bonus += get_perk_rank(ch, PERK_WIZARD_GREATER_SPELL_FOCUS_ENCHANTMENT_3, CLASS_WIZARD) * 2;
  
  return bonus;
}

/**
 * Get duration bonus from Extend Spell perk for buff spells.
 * Only affects non-violent spells that can target self.
 * 
 * @param ch The character
 * @param spellnum The spell number
 * @return Duration multiplier percentage (150 for +50%, 100 for normal)
 */
int get_extend_spell_bonus(struct char_data *ch, int spellnum)
{
  if (!ch || IS_NPC(ch))
    return 100;
  
  if (!has_perk(ch, PERK_WIZARD_EXTEND_SPELL))
    return 100;
  
  /* Only extend buff spells (non-violent, can target self) */
  if (spell_info[spellnum].violent || IS_SET(spell_info[spellnum].targets, TAR_NOT_SELF))
    return 100;
  
  /* Calculate bonus: 5% per rank */
  int ranks = get_perk_rank(ch, PERK_WIZARD_EXTEND_SPELL, CLASS_WIZARD);
  return 100 + (ranks * 5); /* +5% per rank */
}

/**
 * Check if Persistent Spell use is available (has charges).
 * Regenerates charges if cooldown has expired.
 * 
 * @param ch The character
 * @return TRUE if available, FALSE if no charges
 */
bool can_use_persistent_spell_perk(struct char_data *ch)
{
  time_t current_time;
  
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  if (!has_perk(ch, PERK_WIZARD_PERSISTENT_SPELL))
    return FALSE;
  
  current_time = time(0);
  
  /* Initialize charges if this is the first time using the perk */
  if (ch->player_specials->saved.persistent_spell_uses == 0 &&
      ch->player_specials->saved.persistent_spell_cooldown == 0)
  {
    ch->player_specials->saved.persistent_spell_uses = 2;
  }
  
  /* Regenerate charges if cooldown expired and we're below max */
  while (ch->player_specials->saved.persistent_spell_uses < 2 &&
         ch->player_specials->saved.persistent_spell_cooldown > 0 &&
         ch->player_specials->saved.persistent_spell_cooldown <= current_time)
  {
    ch->player_specials->saved.persistent_spell_uses++;
    
    /* If we're still below max, set next regeneration time */
    if (ch->player_specials->saved.persistent_spell_uses < 2)
      ch->player_specials->saved.persistent_spell_cooldown = current_time + 300;
    else
      ch->player_specials->saved.persistent_spell_cooldown = 0; /* All charges available */
  }
  
  /* Check if we have charges available */
  return (ch->player_specials->saved.persistent_spell_uses > 0);
}

/**
 * Use one charge of Persistent Spell perk and activate the effect.
 * 
 * @param ch The character
 */
void use_persistent_spell_perk(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return;
  
  /* Consume one charge */
  if (ch->player_specials->saved.persistent_spell_uses > 0)
    ch->player_specials->saved.persistent_spell_uses--;
  
  /* If we just used our last charge or went below max, start regeneration timer */
  if (ch->player_specials->saved.persistent_spell_uses < 2)
  {
    /* Set cooldown: 5 minutes = 300 seconds */
    ch->player_specials->saved.persistent_spell_cooldown = time(0) + 300;
  }
  
  /* Activate persistent spell flag */
  ch->player_specials->saved.persistent_spell_active = TRUE;
}

/**
 * Check if persistent spell effect is currently active.
 * 
 * @param ch The character
 * @return TRUE if active, FALSE otherwise
 */
bool is_persistent_spell_active(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return ch->player_specials->saved.persistent_spell_active;
}

/**
 * Clear the persistent spell active flag after use.
 * 
 * @param ch The character
 */
void clear_persistent_spell_active(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return;
  
  ch->player_specials->saved.persistent_spell_active = FALSE;
}

/**
 * Check if Split Enchantment is available (not on cooldown).
 * 
 * @param ch The character
 * @return TRUE if available, FALSE if on cooldown
 */
bool can_use_split_enchantment_perk(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  if (!has_perk(ch, PERK_WIZARD_SPLIT_ENCHANTMENT))
    return FALSE;
  
  /* Check cooldown */
  if (ch->player_specials->saved.split_enchantment_cooldown > time(0))
    return FALSE;
  
  return TRUE;
}

/**
 * Activate the Split Enchantment perk cooldown (5 minutes).
 * 
 * @param ch The character
 */
void use_split_enchantment_perk(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return;
  
  /* Set cooldown: 5 minutes = 300 seconds */
  ch->player_specials->saved.split_enchantment_cooldown = time(0) + 300;
}

/**
 * Activate defensive casting AC bonus.
 * Sets a timer for 1 round of +4 dodge AC after casting a spell.
 * 
 * @param ch The character
 */
void activate_defensive_casting(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return;
  
  if (!has_perk(ch, PERK_WIZARD_DEFENSIVE_CASTING))
    return;
  
  /* Set timer for 1 round (will be decremented in limits.c or combat) */
  ch->player_specials->saved.defensive_casting_timer = 1;
}

/**
 * Check if defensive casting AC bonus is active.
 * 
 * @param ch The character
 * @return TRUE if defensive casting bonus is active
 */
bool has_defensive_casting_active(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return (ch->player_specials->saved.defensive_casting_timer > 0);
}

/**
 * Get the AC bonus from defensive casting perk.
 * 
 * @param ch The character
 * @return AC bonus (4 if active, 0 otherwise)
 */
int get_defensive_casting_ac_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!has_defensive_casting_active(ch))
    return 0;
  
  return 4; /* +4 dodge AC */
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

/*****************************************************************************
 * Wizard Evoker Perk Helper Functions
 *****************************************************************************/

/**
 * Get total spell power damage bonus from Spell Power I, II, and III.
 * 
 * @param ch The character
 * @return Total spell damage bonus
 */
int get_wizard_spell_power_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Spell Power I: +1 per rank, max 5 ranks */
  bonus += get_total_perk_ranks(ch, PERK_WIZARD_SPELL_POWER_1);
  
  /* Spell Power II: +1 per rank, max 3 ranks */
  bonus += get_total_perk_ranks(ch, PERK_WIZARD_SPELL_POWER_2);
  
  /* Spell Power III: +2 per rank, max 2 ranks */
  bonus += 2 * get_total_perk_ranks(ch, PERK_WIZARD_SPELL_POWER_3);
  
  return bonus;
}

/**
 * Check if Master of Elements perk should override the spell's damage type.
 * Converts fire, cold, and lightning damage to the preferred element.
 * Displays messages when conversion occurs.
 * 
 * @param ch The character casting the spell
 * @param dam_type The original damage type
 * @return The damage type to use (either original or preferred element)
 */
int get_master_of_elements_override(struct char_data *ch, int dam_type)
{
  int preferred_type;
  char buf[MAX_STRING_LENGTH];
  extern const char *damtypes[];
  
  if (!ch || IS_NPC(ch))
    return dam_type;
  
  if (!HAS_FEAT(ch, PERK_WIZARD_MASTER_OF_ELEMENTS))
    return dam_type;
  
  preferred_type = GET_MASTER_OF_ELEMENTS_TYPE(ch);
  
  /* No preference set */
  if (preferred_type == 0)
    return dam_type;
  
  /* Only convert fire, cold, and lightning damage */
  if (dam_type == DAM_FIRE || dam_type == DAM_COLD || dam_type == DAM_ELECTRIC)
  {
    /* Already the preferred type, no conversion needed */
    if (dam_type == preferred_type)
      return dam_type;
    
    /* Convert and display message */
    snprintf(buf, sizeof(buf), "\tC[Master of Elements: Converting %s to %s]\tn", 
             damtypes[dam_type], damtypes[preferred_type]);
    send_to_char(ch, "%s\r\n", buf);
    
    snprintf(buf, sizeof(buf), "\tC[Master of Elements: $n converts the spell's %s energy to %s]\tn", 
             damtypes[dam_type], damtypes[preferred_type]);
    act(buf, FALSE, ch, 0, 0, TO_ROOM);
    
    return preferred_type;
  }
  
  return dam_type;
}

/**
 * Get elemental damage percentage bonus for a specific damage type.
 * 
 * @param ch The character
 * @param dam_type The damage type (DAM_FIRE, DAM_COLD, DAM_ELECTRIC)
 * @return Percentage bonus (10, 20, or 35 with Master of Elements)
 */
int get_wizard_elemental_damage_bonus(struct char_data *ch, int dam_type)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  switch (dam_type)
  {
    case DAM_FIRE:
    case DAM_COLD:
    case DAM_ELECTRIC:
      /* Focused Element gives +20% (replaces +10% from affinity) */
      if ((dam_type == DAM_FIRE && has_perk(ch, PERK_WIZARD_FOCUSED_ELEMENT_FIRE)) ||
          (dam_type == DAM_COLD && has_perk(ch, PERK_WIZARD_FOCUSED_ELEMENT_COLD)) ||
          (dam_type == DAM_ELECTRIC && has_perk(ch, PERK_WIZARD_FOCUSED_ELEMENT_LIGHTNING)))
      {
        bonus = 20;
      }
      /* Energy Affinity gives +10% */
      else if ((dam_type == DAM_FIRE && has_perk(ch, PERK_WIZARD_ENERGY_AFFINITY_FIRE)) ||
               (dam_type == DAM_COLD && has_perk(ch, PERK_WIZARD_ENERGY_AFFINITY_COLD)) ||
               (dam_type == DAM_ELECTRIC && has_perk(ch, PERK_WIZARD_ENERGY_AFFINITY_LIGHTNING)))
      {
        bonus = 10;
      }
      
      /* Master of Elements adds +15% to all elemental damage */
      if (has_perk(ch, PERK_WIZARD_MASTER_OF_ELEMENTS))
      {
        bonus += 15;
      }
      break;
  }
  
  return bonus;
}

/**
 * Get elemental spell DC bonus for a specific damage type.
 * 
 * @param ch The character
 * @param dam_type The damage type
 * @return DC bonus (0 or 1)
 */
int get_wizard_elemental_dc_bonus(struct char_data *ch, int dam_type)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Focused Element perks give +1 DC */
  switch (dam_type)
  {
    case DAM_FIRE:
      if (has_perk(ch, PERK_WIZARD_FOCUSED_ELEMENT_FIRE))
        return 1;
      break;
      
    case DAM_COLD:
      if (has_perk(ch, PERK_WIZARD_FOCUSED_ELEMENT_COLD))
        return 1;
      break;
      
    case DAM_ELECTRIC:
      if (has_perk(ch, PERK_WIZARD_FOCUSED_ELEMENT_LIGHTNING))
        return 1;
      break;
  }
  
  return 0;
}

/**
 * Get spell penetration bonus from Spell Penetration I, II, and Overwhelming Magic.
 * 
 * @param ch The character
 * @return Spell penetration bonus
 */
int get_wizard_spell_penetration_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Spell Penetration I: +2 per rank, max 3 ranks = +6 */
  bonus += 2 * get_total_perk_ranks(ch, PERK_WIZARD_SPELL_PENETRATION_1);
  
  /* Spell Penetration II: +4 */
  if (has_perk(ch, PERK_WIZARD_SPELL_PENETRATION_2))
    bonus += 4;
  
  /* Overwhelming Magic: +10 (effective SR reduction) */
  if (has_perk(ch, PERK_WIZARD_OVERWHELMING_MAGIC))
    bonus += 10;
  
  return bonus;
}

/**
 * Check if character has any Spell Critical perk.
 * 
 * @param ch The character
 * @return TRUE if has any spell critical perk, FALSE otherwise
 */
bool has_wizard_spell_critical(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return (has_perk(ch, PERK_WIZARD_SPELL_CRITICAL_1) || 
          has_perk(ch, PERK_WIZARD_SPELL_CRITICAL_2));
}

/**
 * Get spell critical chance percentage.
 * 
 * @param ch The character
 * @return Critical chance (5 for tier 1, 10 for tier 2, 0 for none)
 */
int get_wizard_spell_critical_chance(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Spell Critical II: 10% chance */
  if (has_perk(ch, PERK_WIZARD_SPELL_CRITICAL_2))
    return 10;
  
  /* Spell Critical I: 5% chance */
  if (has_perk(ch, PERK_WIZARD_SPELL_CRITICAL_1))
    return 5;
  
  return 0;
}

/**
 * Get spell critical damage multiplier.
 * 
 * @param ch The character
 * @return Damage multiplier as percentage (250 for 2.5x, 150 for 1.5x, 100 for none)
 */
int get_wizard_spell_critical_multiplier(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 100;
  
  /* Spell Critical II: 2.5x damage (250%) */
  if (has_perk(ch, PERK_WIZARD_SPELL_CRITICAL_2))
    return 250;
  
  /* Spell Critical I: 1.5x damage (150%) */
  if (has_perk(ch, PERK_WIZARD_SPELL_CRITICAL_1))
    return 150;
  
  return 100;
}

/**
 * Check if Maximize Spell free use is available (not on cooldown).
 * 
 * @param ch The character
 * @return TRUE if available, FALSE if on cooldown
 */
bool can_use_maximize_spell_perk(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  if (!has_perk(ch, PERK_WIZARD_MAXIMIZE_SPELL))
    return FALSE;
  
  /* Check cooldown */
  if (ch->player_specials->saved.maximize_spell_cooldown > time(0))
    return FALSE;
  
  return TRUE;
}

/**
 * Activate the Maximize Spell perk cooldown (5 minutes).
 * 
 * @param ch The character
 */
void use_maximize_spell_perk(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return;
  
  /* Set cooldown: 5 minutes = 300 seconds */
  ch->player_specials->saved.maximize_spell_cooldown = time(0) + 300;
}

/**
 * Check if Empower Spell free use is available (has charges).
 * Regenerates charges if cooldown has expired.
 * Initializes charges to 2 if not yet initialized.
 * 
 * @param ch The character
 * @return TRUE if available, FALSE if no charges
 */
bool can_use_empower_spell_perk(struct char_data *ch)
{
  time_t current_time;
  
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  if (!has_perk(ch, PERK_WIZARD_EMPOWER_SPELL))
    return FALSE;
  
  current_time = time(0);
  
  /* Initialize charges if this is the first time using the perk */
  if (ch->player_specials->saved.empower_spell_uses == 0 &&
      ch->player_specials->saved.empower_spell_cooldown == 0)
  {
    ch->player_specials->saved.empower_spell_uses = 2;
  }
  
  /* Regenerate charges if cooldown expired and we're below max */
  while (ch->player_specials->saved.empower_spell_uses < 2 &&
         ch->player_specials->saved.empower_spell_cooldown > 0 &&
         ch->player_specials->saved.empower_spell_cooldown <= current_time)
  {
    ch->player_specials->saved.empower_spell_uses++;
    
    /* If we're still below max, set next regeneration time */
    if (ch->player_specials->saved.empower_spell_uses < 2)
      ch->player_specials->saved.empower_spell_cooldown = current_time + 300;
    else
      ch->player_specials->saved.empower_spell_cooldown = 0; /* All charges available */
  }
  
  /* Check if we have charges available */
  return (ch->player_specials->saved.empower_spell_uses > 0);
}

/**
 * Use one charge of Empower Spell perk and start cooldown if needed.
 * 
 * @param ch The character
 */
void use_empower_spell_perk(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return;
  
  /* Consume one charge */
  if (ch->player_specials->saved.empower_spell_uses > 0)
    ch->player_specials->saved.empower_spell_uses--;
  
  /* If we just used our last charge or went below max, start regeneration timer */
  if (ch->player_specials->saved.empower_spell_uses < 2)
  {
    /* Set cooldown: 5 minutes = 300 seconds */
    ch->player_specials->saved.empower_spell_cooldown = time(0) + 300;
  }
}

/**
 * Get bonus damage dice from Arcane Annihilation.
 * 
 * @param ch The character
 * @return Number of d6 bonus dice (3 if has perk, 0 otherwise)
 */
int get_arcane_annihilation_bonus_dice(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_perk(ch, PERK_WIZARD_ARCANE_ANNIHILATION))
    return 3;
  
  return 0;
}

/**
 * Get DC bonus from Arcane Annihilation.
 * 
 * @param ch The character
 * @return DC bonus (+2 if has perk, 0 otherwise)
 */
int get_arcane_annihilation_dc_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_perk(ch, PERK_WIZARD_ARCANE_ANNIHILATION))
    return 2;
  
  return 0;
}

/*****************************************************************************
 * Wizard Controller Perk Helper Functions
 *****************************************************************************/

/**
 * Get spell duration multiplier for Master Enchanter perk.
 * Returns 2.0 if character has the perk, 1.0 otherwise.
 * 
 * @param ch The character
 * @return Duration multiplier (2.0 or 1.0)
 */
float get_master_enchanter_duration_multiplier(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 1.0;
  
  if (has_perk(ch, PERK_WIZARD_MASTER_ENCHANTER))
    return 2.0;
  
  return 1.0;
}

/**
 * Get spell duration multiplier for Master Transmuter perk.
 * Returns 1.5 if character has the perk, 1.0 otherwise.
 * 
 * @param ch The character
 * @return Duration multiplier (1.5 or 1.0)
 */
float get_master_transmuter_duration_multiplier(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 1.0;
  
  if (has_perk(ch, PERK_WIZARD_MASTER_TRANSMUTER))
    return 1.5;
  
  return 1.0;
}

/**
 * Get spell duration multiplier for Archmage of Control perk.
 * Applies to charm, confuse, daze, and sleep effects.
 * Returns 1.5 if character has the perk, 1.0 otherwise.
 * 
 * @param ch The character
 * @return Duration multiplier (1.5 or 1.0)
 */
float get_archmage_control_duration_multiplier(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 1.0;
  
  if (has_perk(ch, PERK_WIZARD_ARCHMAGE_OF_CONTROL))
    return 1.5;
  
  return 1.0;
}

/**
 * Get DC bonus for control spells from Archmage of Control.
 * Applies to charm, confuse, daze, and sleep effects.
 * 
 * @param ch The character
 * @return DC bonus (+5 if has perk, 0 otherwise)
 */
int get_archmage_control_dc_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_perk(ch, PERK_WIZARD_ARCHMAGE_OF_CONTROL))
    return 5;
  
  return 0;
}

/**
 * Get DC bonus for enchantment spells from Master Enchanter.
 * 
 * @param ch The character
 * @return DC bonus (+3 if has perk, 0 otherwise)
 */
int get_master_enchanter_dc_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_perk(ch, PERK_WIZARD_MASTER_ENCHANTER))
    return 3;
  
  return 0;
}

/**
 * Get DC bonus for transmutation spells from Master Transmuter.
 * 
 * @param ch The character
 * @return DC bonus (+3 if has perk, 0 otherwise)
 */
int get_master_transmuter_dc_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_perk(ch, PERK_WIZARD_MASTER_TRANSMUTER))
    return 3;
  
  return 0;
}

/**
 * Get DC bonus for illusion spells from Master Illusionist.
 * 
 * @param ch The character
 * @return DC bonus (+3 if has perk, 0 otherwise)
 */
int get_master_illusionist_dc_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_perk(ch, PERK_WIZARD_MASTER_ILLUSIONIST))
    return 3;
  
  return 0;
}

/**
 * Get DC bonus from Spell Mastery perk.
 * Applies to all spells.
 * 
 * @param ch The character
 * @return DC bonus (+2 if has perk, 0 otherwise)
 */
int get_spell_mastery_dc_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_perk(ch, PERK_WIZARD_SPELL_MASTERY))
    return 2;
  
  return 0;
}

/*****************************************************************************
 * Wizard Versatile Caster Perk Helper Functions
 *****************************************************************************/

/**
 * Get concentration bonus when in combat from Combat Casting I perk.
 * 
 * @param ch The character
 * @return Concentration bonus (+2 per rank when in combat, 0 otherwise)
 */
int get_combat_casting_concentration_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Only applies when character is in combat */
  if (!FIGHTING(ch))
    return 0;
  
  /* +2 per rank, max 3 ranks = +6 */
  return 2 * get_total_perk_ranks(ch, PERK_WIZARD_COMBAT_CASTING_I);
}

/**
 * Check if character can use spell recall (has perk and cooldown expired).
 * 
 * @param ch The character
 * @return TRUE if available, FALSE otherwise
 */
bool can_use_spell_recall(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  if (!has_perk(ch, PERK_WIZARD_SPELL_RECALL))
    return FALSE;
  
  /* Check daily cooldown */
  if (GET_SPELL_RECALL_COOLDOWN(ch) > 0)
    return FALSE;
  
  return TRUE;
}

/**
 * Get metamagic spell circle reduction from Metamagic Master I.
 * Randomly reduces metamagic circle increase by 1.
 * 
 * @param ch The character
 * @return 1 if reduction applies (50% chance), 0 otherwise
 */
int get_metamagic_master_reduction(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!has_perk(ch, PERK_WIZARD_METAMAGIC_MASTER_I))
    return 0;
  
  /* 50% chance to reduce circle increase by 1 */
  if (rand_number(0, 1))
    return 1;
  
  return 0;
}

/**
 * Get spellcraft skill bonus from Arcane Knowledge I.
 * 
 * @param ch The character
 * @return Spellcraft bonus (+2 per rank)
 */
int get_arcane_knowledge_spellcraft_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* +2 per rank, max 3 ranks = +6 */
  return 2 * get_total_perk_ranks(ch, PERK_WIZARD_ARCANE_KNOWLEDGE_I);
}

/**
 * Check if spell slot should be preserved due to Spell Focus perks.
 * Spell Focus I: 2% chance per rank (max 10% at 5 ranks)
 * Spell Focus II: Additional 2% per rank (max 6% at 3 ranks)
 * Total max: 16% chance at full ranks
 * 
 * @param ch The character
 * @return TRUE if spell slot should be preserved, FALSE otherwise
 */
bool check_spell_slot_preservation(struct char_data *ch)
{
  int chance = 0;
  
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  /* Spell Focus I: 2% per rank, max 5 ranks = 10% */
  chance += 2 * get_total_perk_ranks(ch, PERK_WIZARD_SPELL_FOCUS_I);
  
  /* Spell Focus II: 2% per rank, max 3 ranks = 6% */
  chance += 2 * get_total_perk_ranks(ch, PERK_WIZARD_SPELL_FOCUS_II);
  
  /* Spell Focus III: 2% per rank, max 2 ranks = 4% */
  chance += 2 * get_total_perk_ranks(ch, PERK_WIZARD_SPELL_FOCUS_III);
  
  /* Archmage's Power: +5% */
  if (has_perk(ch, PERK_WIZARD_ARCHMAGES_POWER))
    chance += 5;
  
  /* Roll percentage check */
  if (chance > 0 && rand_number(1, 100) <= chance)
  {
    return TRUE;
  }
  
  return FALSE;
}


/**
 * Activate spell shield effect (10 DR + 4 AC for 1 round).
 * Triggered when enemy initiates combat against the wizard.
 * 
 * @param ch The character
 */
void activate_spell_shield(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return;
  
  if (!has_perk(ch, PERK_WIZARD_SPELL_SHIELD))
    return;
  
  GET_SPELL_SHIELD_TIMER(ch) = 1;
  send_to_char(ch, "\tCAn arcane shield springs into being around you!\tn\r\n");
}

/**
 * Check if spell shield is active.
 * 
 * @param ch The character
 * @return TRUE if active, FALSE otherwise
 */
bool has_spell_shield_active(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return GET_SPELL_SHIELD_TIMER(ch) > 0;
}

/**
 * Get AC bonus from spell shield.
 * 
 * @param ch The character
 * @return AC bonus (4 if active, 0 otherwise)
 */
int get_spell_shield_ac_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_spell_shield_active(ch))
    return 4;
  
  return 0;
}

/**
 * Get DR from spell shield.
 * 
 * @param ch The character
 * @return DR value (10 if active, 0 otherwise)
 */
int get_spell_shield_dr(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_spell_shield_active(ch))
    return 10;
  
  return 0;
}

/**
 * Get metamagic circle cost reduction from perks.
 * Metamagic Master I (-1) + Metamagic Master II (-1) + Archmage's Power (-1).
 * Limited to 2 uses per 5 minutes, regenerates like Empower Spell.
 * 
 * @param ch The character
 * @return Circle cost reduction (negative number), or 0 if no uses available
 */
int get_metamagic_circle_reduction(struct char_data *ch)
{
  int reduction = 0;
  int max_reduction = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Check if character has any metamagic reduction perks */
  if (!has_perk(ch, PERK_WIZARD_METAMAGIC_MASTER_I) && 
      !has_perk(ch, PERK_WIZARD_METAMAGIC_MASTER_II) &&
      !has_perk(ch, PERK_WIZARD_ARCHMAGES_POWER))
    return 0;
  
  /* Regenerate charges if needed (same system as Empower Spell) */
  if (ch->player_specials->saved.metamagic_reduction_uses < 2)
  {
    time_t now = time(0);
    if (ch->player_specials->saved.metamagic_reduction_cooldown <= now)
    {
      /* Grant one use back */
      ch->player_specials->saved.metamagic_reduction_uses++;
      
      /* If not at max, set cooldown for next regeneration (5 minutes) */
      if (ch->player_specials->saved.metamagic_reduction_uses < 2)
      {
        ch->player_specials->saved.metamagic_reduction_cooldown = now + 300; /* 5 minutes */
      }
    }
  }
  
  /* Check if we have uses available */
  if (ch->player_specials->saved.metamagic_reduction_uses <= 0)
    return 0;
  
  /* Calculate maximum possible reduction based on perks */
  if (has_perk(ch, PERK_WIZARD_METAMAGIC_MASTER_I))
    max_reduction -= 1;
  
  if (has_perk(ch, PERK_WIZARD_METAMAGIC_MASTER_II))
    max_reduction -= 1;
  
  if (has_perk(ch, PERK_WIZARD_ARCHMAGES_POWER))
    max_reduction -= 1;
  
  /* Only consume a use if we're actually going to reduce cost */
  if (max_reduction < 0)
  {
    reduction = max_reduction;
    /* Note: Actual use consumption happens in use_metamagic_reduction() */
  }
  
  return reduction;
}

/**
 * Check if character can use metamagic reduction.
 * Regenerates uses automatically if cooldown has expired.
 * 
 * @param ch The character
 * @return TRUE if uses available, FALSE otherwise
 */
bool can_use_metamagic_reduction(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  /* Must have at least one perk */
  if (!has_perk(ch, PERK_WIZARD_METAMAGIC_MASTER_I) && 
      !has_perk(ch, PERK_WIZARD_METAMAGIC_MASTER_II) &&
      !has_perk(ch, PERK_WIZARD_ARCHMAGES_POWER))
    return FALSE;
  
  /* Regenerate charges if needed */
  if (ch->player_specials->saved.metamagic_reduction_uses < 2)
  {
    time_t now = time(0);
    if (ch->player_specials->saved.metamagic_reduction_cooldown <= now)
    {
      ch->player_specials->saved.metamagic_reduction_uses++;
      
      if (ch->player_specials->saved.metamagic_reduction_uses < 2)
      {
        ch->player_specials->saved.metamagic_reduction_cooldown = now + 300;
      }
    }
  }
  
  return (ch->player_specials->saved.metamagic_reduction_uses > 0);
}

/**
 * Consume one metamagic reduction use.
 * Called after successfully reducing metamagic cost.
 * 
 * @param ch The character
 */
void use_metamagic_reduction(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return;
  
  if (ch->player_specials->saved.metamagic_reduction_uses <= 0)
    return;
  
  ch->player_specials->saved.metamagic_reduction_uses--;
  
  /* If we just used our last charge, start the cooldown */
  if (ch->player_specials->saved.metamagic_reduction_uses == 0)
  {
    ch->player_specials->saved.metamagic_reduction_cooldown = time(0) + 300; /* 5 minutes */
  }
  /* If we have 1 use left and cooldown isn't set, set it */
  else if (ch->player_specials->saved.metamagic_reduction_uses == 1 && 
           ch->player_specials->saved.metamagic_reduction_cooldown <= time(0))
  {
    ch->player_specials->saved.metamagic_reduction_cooldown = time(0) + 300;
  }
}

/**
 * Get spell DC bonus from Arcane Supremacy.
 * 
 * @param ch The character
 * @return DC bonus (2 if has perk, 0 otherwise)
 */
int get_arcane_supremacy_dc_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_perk(ch, PERK_WIZARD_ARCANE_SUPREMACY))
    return 2;
  
  return 0;
}

/**
 * Get caster level bonus from Arcane Supremacy.
 * 
 * @param ch The character
 * @return Caster level bonus (2 if has perk, 0 otherwise)
 */
int get_arcane_supremacy_caster_level_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_perk(ch, PERK_WIZARD_ARCANE_SUPREMACY))
    return 2;
  
  return 0;
}

/**
 * Get spell damage bonus from Arcane Supremacy.
 * 
 * @param ch The character
 * @return Damage bonus (2 if has perk, 0 otherwise)
 */
int get_arcane_supremacy_damage_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_perk(ch, PERK_WIZARD_ARCANE_SUPREMACY))
    return 2;
  
  return 0;
}
