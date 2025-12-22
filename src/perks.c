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
#include "psionics.h"
#include "fight.h"
#include "act.h"
#include "fight.h"

#include "interpreter.h"
#include "constants.h"
#include "perks.h"
#include "mud_event.h"

/* Forward declarations */
static void define_wizard_controller_perks(void);
static void define_wizard_versatile_caster_perks(void);

/* Global perk database - all defined perks */
struct perk_data perk_list[NUM_PERKS];

/* Perk category names for display */
const char *perk_category_names[] = {
  "Undefined",              /* 0 - PERK_CATEGORY_UNDEFINED */
  "Weapon Specialist",      /* 1 - PERK_CATEGORY_WEAPON_SPECIALIST */
  "Defender",               /* 2 - PERK_CATEGORY_DEFENDER */
  "Tactical Fighter",       /* 3 - PERK_CATEGORY_TACTICAL_FIGHTER */
  "Evoker",                 /* 4 - PERK_CATEGORY_EVOKER */
  "Controller",             /* 5 - PERK_CATEGORY_CONTROLLER */
  "Versatile Caster",       /* 6 - PERK_CATEGORY_VERSATILE_CASTER */
  "Divine Healer",          /* 7 - PERK_CATEGORY_DIVINE_HEALER */
  "Battle Cleric",          /* 8 - PERK_CATEGORY_BATTLE_CLERIC */
  "Domain Master",          /* 9 - PERK_CATEGORY_DOMAIN_MASTER */
  "Assassin",               /* 10 - PERK_CATEGORY_ASSASSIN */
  "Master Thief",           /* 11 - PERK_CATEGORY_MASTER_THIEF */
  "Shadow Scout",           /* 12 - PERK_CATEGORY_SHADOW_SCOUT */
  "Iron Body",              /* 13 - PERK_CATEGORY_IRON_BODY */
  "Perfect Self",           /* 14 - PERK_CATEGORY_PERFECT_SELF */
  "Way of the Open Hand",   /* 15 - PERK_CATEGORY_WAY_OF_THE_OPEN_HAND */
  "Way of the Shadow",      /* 16 - PERK_CATEGORY_WAY_OF_THE_SHADOW */
  "Hunter",                 /* 17 - PERK_CATEGORY_HUNTER */
  "Beast Master",           /* 18 - PERK_CATEGORY_BEAST_MASTER */
  "Wilderness Warrior",     /* 19 - PERK_CATEGORY_WILDERNESS_WARRIOR */
  "Berserker",              /* 19 - PERK_CATEGORY_BERSERKER */
  "Totem Warrior",          /* 20 - PERK_CATEGORY_TOTEM_WARRIOR */
  "Primal Champion",        /* 21 - PERK_CATEGORY_PRIMAL_CHAMPION */
  "Mutagenist",             /* 33 - PERK_CATEGORY_MUTAGENIST */
  "Bomb Craftsman",         /* 34 - PERK_CATEGORY_BOMB_CRAFTSMAN */
  "Extract Master",         /* 35 - PERK_CATEGORY_EXTRACT_MASTER */
  "Telepathic Control",     /* 36 - PERK_CATEGORY_TELEPATHIC_CONTROL */
  "Psychokinetic Arsenal",  /* 37 - PERK_CATEGORY_PSYCHOKINETIC_ARSENAL */
  "Metacreative Genius",    /* 38 - PERK_CATEGORY_METACREATIVE_GENIUS */
  "\n"                      /* Terminator */
};
/* Forward declarations for perk definition functions */
void define_bard_perks(void);

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
    perk_list[i].perk_category = PERK_CATEGORY_UNDEFINED;
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
  /* Define Bard Perks */
  define_bard_perks();
  define_barbarian_perks();
  
  /* Define Monk Perks */
  define_monk_perks();
  
  /* Define Paladin Perks */
  define_paladin_perks();

  /* Define Alchemist Perks */
  define_alchemist_perks();
  void define_alchemist_perks(void);

  /* Define Psionicist Perks */
  define_psionicist_perks();
  
  log("Perks system initialized with %d defined perks.", count_defined_perks());
}
/* Define Psionicist Perks */
void define_psionicist_perks(void)
{
  struct perk_data *perk;

  /*** Telepathic Control - Tier I ***/

  /* Mind Spike I */
  perk = &perk_list[PERK_PSIONICIST_MIND_SPIKE_I];
  perk->id = PERK_PSIONICIST_MIND_SPIKE_I;
  perk->name = strdup("Mind Spike I");
  perk->description = strdup("+1 DC to Telepathy powers.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_TELEPATHIC_CONTROL;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 DC */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Telepathy powers gain +1 DC.");

  /* Suggestion Primer */
  perk = &perk_list[PERK_PSIONICIST_SUGGESTION_PRIMER];
  perk->id = PERK_PSIONICIST_SUGGESTION_PRIMER;
  perk->name = strdup("Suggestion Primer");
  perk->description = strdup("Telepathy debuffs gain +1 round duration on failed save (non-boss).");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_TELEPATHIC_CONTROL;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 round */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Telepathy MAG_AFFECTS powers extend by +1 round.");

  /* Psionic Disruptor I */
  perk = &perk_list[PERK_PSIONICIST_PSIONIC_DISRUPTOR_I];
  perk->id = PERK_PSIONICIST_PSIONIC_DISRUPTOR_I;
  perk->name = strdup("Psionic Disruptor I");
  perk->description = strdup("+1 manifester level vs power resistance with Telepathy powers.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_TELEPATHIC_CONTROL;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 penetration check */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Telepathy powers gain +1 to penetration checks.");

  /* Focus Channeling */
  perk = &perk_list[PERK_PSIONICIST_FOCUS_CHANNELING];
  perk->id = PERK_PSIONICIST_FOCUS_CHANNELING;
  perk->name = strdup("Focus Channeling");
  perk->description = strdup("Regain 1 PSP when a Telepathy power affects or damages at least one target (once per round).");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_TELEPATHIC_CONTROL;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* PSP amount */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Regain 1 PSP once per round on Telepathy power hit.");

  /*** Telepathic Control - Tier II ***/

  /* Mind Spike II */
  perk = &perk_list[PERK_PSIONICIST_MIND_SPIKE_II];
  perk->id = PERK_PSIONICIST_MIND_SPIKE_II;
  perk->name = strdup("Mind Spike II");
  perk->description = strdup("Total +2 Telepathy DCs; Telepathy damage powers add +1 die if augmented by ≥2 PSP.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_TELEPATHIC_CONTROL;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_MIND_SPIKE_I;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* Total +2 DC */
  perk->effect_modifier = 1; /* +1 die on damage */
  perk->special_description = strdup("Telepathy powers gain +2 DC total; damage powers gain +1 die if augmented ≥2 PSP.");

  /* Overwhelm */
  perk = &perk_list[PERK_PSIONICIST_OVERWHELM];
  perk->id = PERK_PSIONICIST_OVERWHELM;
  perk->name = strdup("Overwhelm");
  perk->description = strdup("First Telepathy power each encounter forces targets to save twice, taking the worse (once per combat).");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_TELEPATHIC_CONTROL;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_SUGGESTION_PRIMER;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* Save twice, take worse */
  perk->effect_modifier = 0;
  perk->special_description = strdup("First Telepathy power each encounter: targets save twice, use worse result.");

  /* Psionic Disruptor II */
  perk = &perk_list[PERK_PSIONICIST_PSIONIC_DISRUPTOR_II];
  perk->id = PERK_PSIONICIST_PSIONIC_DISRUPTOR_II;
  perk->name = strdup("Psionic Disruptor II");
  perk->description = strdup("Total +2 manifester level vs power resistance for Telepathy powers.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_TELEPATHIC_CONTROL;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_PSIONIC_DISRUPTOR_I;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* Total +2 manifester level */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Telepathy powers gain +2 manifester level vs PR.");

  /* Linked Menace */
  perk = &perk_list[PERK_PSIONICIST_LINKED_MENACE];
  perk->id = PERK_PSIONICIST_LINKED_MENACE;
  perk->name = strdup("Linked Menace");
  perk->description = strdup("When landing a Telepathy debuff, target takes -2 penalty to AC for 2 rounds.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_TELEPATHIC_CONTROL;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_FOCUS_CHANNELING;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = -2; /* -2 AC */
  perk->effect_modifier = 2; /* 2 round duration */
  perk->special_description = strdup("Telepathy debuffs apply -2 AC for 2 rounds.");
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
  perk->perk_category = PERK_CATEGORY_WEAPON_SPECIALIST;
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
  perk->perk_category = PERK_CATEGORY_WEAPON_SPECIALIST;
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
  perk->perk_category = PERK_CATEGORY_WEAPON_SPECIALIST;
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
  perk->perk_category = PERK_CATEGORY_WEAPON_SPECIALIST;
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
  perk->perk_category = PERK_CATEGORY_WEAPON_SPECIALIST;
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
  perk->perk_category = PERK_CATEGORY_WEAPON_SPECIALIST;
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
  perk->perk_category = PERK_CATEGORY_WEAPON_SPECIALIST;
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
  perk->perk_category = PERK_CATEGORY_WEAPON_SPECIALIST;
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
  perk->perk_category = PERK_CATEGORY_WEAPON_SPECIALIST;
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
  perk->perk_category = PERK_CATEGORY_WEAPON_SPECIALIST;
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
  perk->perk_category = PERK_CATEGORY_WEAPON_SPECIALIST;
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
  perk->perk_category = PERK_CATEGORY_WEAPON_SPECIALIST;
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
  perk->perk_category = PERK_CATEGORY_DEFENDER;
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
  perk->perk_category = PERK_CATEGORY_DEFENDER;
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
  perk->perk_category = PERK_CATEGORY_DEFENDER;
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
  perk->perk_category = PERK_CATEGORY_DEFENDER;
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
  perk->perk_category = PERK_CATEGORY_DEFENDER;
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
  perk->perk_category = PERK_CATEGORY_DEFENDER;
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
  perk->perk_category = PERK_CATEGORY_DEFENDER;
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
  perk->perk_category = PERK_CATEGORY_DEFENDER;
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
  perk->perk_category = PERK_CATEGORY_DEFENDER;
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
  perk->perk_category = PERK_CATEGORY_DEFENDER;
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
  perk->perk_category = PERK_CATEGORY_DEFENDER;
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
  perk->perk_category = PERK_CATEGORY_DEFENDER;
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
  perk->description = strdup("DR 6/-, immunity to knockdown");
  perk->associated_class = CLASS_WARRIOR;
  perk->perk_category = PERK_CATEGORY_DEFENDER;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_FIGHTER_IMPROVED_DAMAGE_REDUCTION;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 6;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Increases DR to 6/- (replaces Defensive Stance DR) and grants immunity to knockdown");
  perk->toggleable = true; /* Can be toggled on/off like Defensive Stance */
  
  /* Last Stand */
  perk = &perk_list[PERK_FIGHTER_LAST_STAND];
  perk->id = PERK_FIGHTER_LAST_STAND;
  perk->name = strdup("Last Stand");
  perk->description = strdup("33% chance when reduced to 0 HP to survive at 1 HP");
  perk->associated_class = CLASS_WARRIOR;
  perk->perk_category = PERK_CATEGORY_DEFENDER;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_FIGHTER_TOUGHNESS_1;
  perk->prerequisite_rank = 5; /* Requires Toughness I at max rank */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("When HP drops to 0 or below, you have a 33% chance to remain at 1 HP instead of dying. Requires max Toughness I and max Resilience.");
  
  /*** TREE 3: TACTICAL FIGHTER - TIER I ***/
  
  /* Combat Reflexes I */
  perk = &perk_list[PERK_FIGHTER_COMBAT_REFLEXES_1];
  perk->id = PERK_FIGHTER_COMBAT_REFLEXES_1;
  perk->name = strdup("Combat Reflexes I");
  perk->description = strdup("+1 attack of opportunity per round per rank (max 3 ranks)");
  perk->associated_class = CLASS_WARRIOR;
  perk->perk_category = PERK_CATEGORY_TACTICAL_FIGHTER;
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
  perk->perk_category = PERK_CATEGORY_TACTICAL_FIGHTER;
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
  perk->perk_category = PERK_CATEGORY_TACTICAL_FIGHTER;
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
  perk->perk_category = PERK_CATEGORY_TACTICAL_FIGHTER;
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
  perk->perk_category = PERK_CATEGORY_TACTICAL_FIGHTER;
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
  perk->perk_category = PERK_CATEGORY_TACTICAL_FIGHTER;
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
  perk->perk_category = PERK_CATEGORY_TACTICAL_FIGHTER;
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
  perk->perk_category = PERK_CATEGORY_TACTICAL_FIGHTER;
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
  perk->perk_category = PERK_CATEGORY_WEAPON_SPECIALIST;
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
  perk->perk_category = PERK_CATEGORY_WEAPON_SPECIALIST;
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
  perk->perk_category = PERK_CATEGORY_WEAPON_SPECIALIST;
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
  perk->perk_category = PERK_CATEGORY_DEFENDER;
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
  perk->perk_category = PERK_CATEGORY_DEFENDER;
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
  perk->perk_category = PERK_CATEGORY_DEFENDER;
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
  perk->perk_category = PERK_CATEGORY_DEFENDER;
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
  perk->perk_category = PERK_CATEGORY_DEFENDER;
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
  perk->perk_category = PERK_CATEGORY_VERSATILE_CASTER;
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
  perk->perk_category = PERK_CATEGORY_VERSATILE_CASTER;
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
  perk->perk_category = PERK_CATEGORY_CONTROLLER;
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
  perk->perk_category = PERK_CATEGORY_EVOKER;
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
  perk->perk_category = PERK_CATEGORY_EVOKER;
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
  perk->perk_category = PERK_CATEGORY_EVOKER;
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
  perk->perk_category = PERK_CATEGORY_EVOKER;
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
  perk->perk_category = PERK_CATEGORY_EVOKER;
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
  perk->perk_category = PERK_CATEGORY_EVOKER;
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
  perk->perk_category = PERK_CATEGORY_EVOKER;
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
  perk->perk_category = PERK_CATEGORY_EVOKER;
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
  perk->perk_category = PERK_CATEGORY_EVOKER;
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
  perk->perk_category = PERK_CATEGORY_EVOKER;
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
  perk->description = strdup("Damage spells have 5% chance to deal 1.5x damage");
  perk->associated_class = CLASS_WIZARD;
  perk->perk_category = PERK_CATEGORY_EVOKER;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_WIZARD_SPELL_POWER_1;
  perk->prerequisite_rank = 3; /* Need at least 3 ranks */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5; /* 5% chance */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Spell Power I (at least 3 ranks). Damaging spells have a 5% chance to critically strike for 1.5x damage.");
  
  /* Maximize Spell */
  perk = &perk_list[PERK_WIZARD_MAXIMIZE_SPELL];
  perk->id = PERK_WIZARD_MAXIMIZE_SPELL;
  perk->name = strdup("Maximize Spell");
  perk->description = strdup("Cast 1 spell with max damage (no level increase), 5min cooldown");
  perk->associated_class = CLASS_WIZARD;
  perk->perk_category = PERK_CATEGORY_EVOKER;
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
  perk->perk_category = PERK_CATEGORY_EVOKER;
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
  perk->perk_category = PERK_CATEGORY_EVOKER;
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
  perk->perk_category = PERK_CATEGORY_EVOKER;
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
  perk->perk_category = PERK_CATEGORY_EVOKER;
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
  perk->perk_category = PERK_CATEGORY_EVOKER;
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
  perk->perk_category = PERK_CATEGORY_EVOKER;
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
  perk->perk_category = PERK_CATEGORY_EVOKER;
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
  perk->perk_category = PERK_CATEGORY_CONTROLLER;
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
  perk->perk_category = PERK_CATEGORY_CONTROLLER;
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
  perk->perk_category = PERK_CATEGORY_CONTROLLER;
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
  perk->perk_category = PERK_CATEGORY_CONTROLLER;
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
  perk->perk_category = PERK_CATEGORY_CONTROLLER;
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
  perk->perk_category = PERK_CATEGORY_CONTROLLER;
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
  perk->perk_category = PERK_CATEGORY_CONTROLLER;
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
  perk->perk_category = PERK_CATEGORY_CONTROLLER;
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
  perk->perk_category = PERK_CATEGORY_CONTROLLER;
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
  perk->perk_category = PERK_CATEGORY_CONTROLLER;
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
  perk->perk_category = PERK_CATEGORY_CONTROLLER;
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
  perk->perk_category = PERK_CATEGORY_CONTROLLER;
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
  perk->perk_category = PERK_CATEGORY_CONTROLLER;
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
  perk->perk_category = PERK_CATEGORY_CONTROLLER;
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
  perk->perk_category = PERK_CATEGORY_CONTROLLER;
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
  perk->perk_category = PERK_CATEGORY_CONTROLLER;
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
  perk->perk_category = PERK_CATEGORY_CONTROLLER;
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
  perk->perk_category = PERK_CATEGORY_CONTROLLER;
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
  perk->perk_category = PERK_CATEGORY_VERSATILE_CASTER;
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
  perk->perk_category = PERK_CATEGORY_VERSATILE_CASTER;
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
  perk->perk_category = PERK_CATEGORY_VERSATILE_CASTER;
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
  perk->perk_category = PERK_CATEGORY_VERSATILE_CASTER;
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
  perk->perk_category = PERK_CATEGORY_VERSATILE_CASTER;
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
  perk->perk_category = PERK_CATEGORY_VERSATILE_CASTER;
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
  perk->perk_category = PERK_CATEGORY_VERSATILE_CASTER;
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
  perk->perk_category = PERK_CATEGORY_VERSATILE_CASTER;
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
  perk->perk_category = PERK_CATEGORY_VERSATILE_CASTER;
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
  perk->perk_category = PERK_CATEGORY_VERSATILE_CASTER;
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
  perk->perk_category = PERK_CATEGORY_VERSATILE_CASTER;
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
  perk->perk_category = PERK_CATEGORY_VERSATILE_CASTER;
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
  perk->perk_category = PERK_CATEGORY_VERSATILE_CASTER;
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
  perk->perk_category = PERK_CATEGORY_VERSATILE_CASTER;
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
  perk->perk_category = PERK_CATEGORY_VERSATILE_CASTER;
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
  perk->perk_category = PERK_CATEGORY_DIVINE_HEALER;
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
  perk->perk_category = PERK_CATEGORY_DIVINE_HEALER;
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
  perk->perk_category = PERK_CATEGORY_DIVINE_HEALER;
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
  perk->perk_category = PERK_CATEGORY_DIVINE_HEALER;
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
  perk->perk_category = PERK_CATEGORY_DIVINE_HEALER;
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
  perk->perk_category = PERK_CATEGORY_DIVINE_HEALER;
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
  perk->perk_category = PERK_CATEGORY_DIVINE_HEALER;
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
  perk->perk_category = PERK_CATEGORY_DIVINE_HEALER;
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
  perk->perk_category = PERK_CATEGORY_DIVINE_HEALER;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_CLERIC_RADIANT_SERVANT_1;
  perk->prerequisite_rank = 2; /* Must have at least 2 ranks of Radiant Servant I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* 2d6 */
  perk->effect_modifier = 6;
  perk->special_description = strdup("Requires Radiant Servant I (at least 2 ranks). Grants the ability to channel positive energy to heal all allies in the room for 2d6 HP. Can be used once per short rest.");

  /*** Telepathic Control - Tier III ***/

  /* Dominion */
  perk = &perk_list[PERK_PSIONICIST_DOMINION];
  perk->id = PERK_PSIONICIST_DOMINION;
  perk->name = strdup("Dominion");
  perk->description = strdup("Total +3 Telepathy DCs; charm/dominate effects gain +2 rounds on failed saves (non-boss).");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_TELEPATHIC_CONTROL;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_MIND_SPIKE_II;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3; /* Total +3 DC */
  perk->effect_modifier = 2; /* +2 rounds on charm/dominate */
  perk->special_description = strdup("Telepathy powers gain +3 DC total; charm/dominate extend +2 rounds (non-boss).");

  /* Psychic Sundering */
  perk = &perk_list[PERK_PSIONICIST_PSYCHIC_SUNDERING];
  perk->id = PERK_PSIONICIST_PSYCHIC_SUNDERING;
  perk->name = strdup("Psychic Sundering");
  perk->description = strdup("Telepathy damage powers make targets vulnerable: +10% damage from all sources for 3 rounds.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_TELEPATHIC_CONTROL;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_OVERWHELM;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10; /* +10% damage taken */
  perk->effect_modifier = 3; /* 3 round duration */
  perk->special_description = strdup("Telepathy damage applies 3-round 10% vulnerability to all damage sources.");

  /* Mental Backlash */
  perk = &perk_list[PERK_PSIONICIST_MENTAL_BACKLASH];
  perk->id = PERK_PSIONICIST_MENTAL_BACKLASH;
  perk->name = strdup("Mental Backlash");
  perk->description = strdup("When a target saves vs your Telepathy, it still takes (5 + 1/2 level) mental damage (no save).");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_TELEPATHIC_CONTROL;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_LINKED_MENACE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5; /* base chip damage */
  perk->effect_modifier = 2; /* scaling uses level/2 */
  perk->special_description = strdup("Successful saves vs your Telepathy still take chip mental damage.");

  /* Piercing Will */
  perk = &perk_list[PERK_PSIONICIST_PIERCING_WILL];
  perk->id = PERK_PSIONICIST_PIERCING_WILL;
  perk->name = strdup("Piercing Will");
  perk->description = strdup("Ignore 5 power resistance when manifesting Telepathy powers (stacks with Disruptor).");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_TELEPATHIC_CONTROL;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_PSIONIC_DISRUPTOR_II;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5; /* ignore 5 PR */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Telepathy powers ignore 5 PR; stacks with Disruptor perks.");

  /*** TELEPATHIC CONTROL - TIER 4 CAPSTONE PERKS (5 points each) ***/

  /* Absolute Geas */
  perk = &perk_list[PERK_PSIONICIST_ABSOLUTE_GEAS];
  perk->id = PERK_PSIONICIST_ABSOLUTE_GEAS;
  perk->name = strdup("Absolute Geas");
  perk->description = strdup("When you manifest a hostile Telepathy power against a target, there is a 10% chance it applies shaken, fatigued, and deafened (3 rounds, Will save negates).");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_TELEPATHIC_CONTROL;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_PIERCING_WILL;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10; /* 10% chance */
  perk->effect_modifier = 3; /* 3 round duration */
  perk->special_description = strdup("Tier 4 Capstone: 10% chance on Telepathy powers applies shaken/fatigued/deafened (Will save negates).");

  /* Hive Commander */
  perk = &perk_list[PERK_PSIONICIST_HIVE_COMMANDER];
  perk->id = PERK_PSIONICIST_HIVE_COMMANDER;
  perk->name = strdup("Hive Commander");
  perk->description = strdup("On successful hostile Telepathy powers, grant yourself +3 DC to further powers vs that target and +2 to-hit for allies (3 rounds).");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_TELEPATHIC_CONTROL;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_PIERCING_WILL;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3; /* +3 DC bonus */
  perk->effect_modifier = 3; /* 3 round duration */
  perk->special_description = strdup("Tier 4 Capstone: Successful Telepathy powers mark target for +3 DC and grant +2 to-hit to allies.");

  /*** PSYCHOKINETIC ARSENAL - TIER 1 PERKS (1-2 points each) ***/

  /* Kinetic Edge I */
  perk = &perk_list[PERK_PSIONICIST_KINETIC_EDGE_I];
  perk->id = PERK_PSIONICIST_KINETIC_EDGE_I;
  perk->name = strdup("Kinetic Edge I");
  perk->description = strdup("+1 damage die on Psychokinesis blasts (energy ray, crystal shard, energy push, concussion blast) if augmented by >=1 PSP.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_PSYCHOKINETIC_ARSENAL;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 damage die */
  perk->effect_modifier = 1; /* requires augmentation >= 1 */
  perk->special_description = strdup("Tier 1: +1 damage die on Psychokinesis blasts when augmented by >=1 PSP.");

  /* Force Screen Adept */
  perk = &perk_list[PERK_PSIONICIST_FORCE_SCREEN_ADEPT];
  perk->id = PERK_PSIONICIST_FORCE_SCREEN_ADEPT;
  perk->name = strdup("Force Screen Adept");
  perk->description = strdup("Inertial armor and force screen grant +1 AC and +10% duration.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_PSYCHOKINETIC_ARSENAL;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 AC */
  perk->effect_modifier = 10; /* +10% duration */
  perk->special_description = strdup("Tier 1: Inertial armor/force screen +1 AC and +10% duration.");

  /* Vector Shove */
  perk = &perk_list[PERK_PSIONICIST_VECTOR_SHOVE];
  perk->id = PERK_PSIONICIST_VECTOR_SHOVE;
  perk->name = strdup("Vector Shove");
  perk->description = strdup("Energy push/telekinetic shoves get +2 to the movement check; on success deal +1 die force damage.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_PSYCHOKINETIC_ARSENAL;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 to movement check */
  perk->effect_modifier = 1; /* +1 die force damage */
  perk->special_description = strdup("Tier 1: Energy push/telekinetic shoves +2 to movement check; +1 die force on success.");

  /* Energy Specialization */
  perk = &perk_list[PERK_PSIONICIST_ENERGY_SPECIALIZATION];
  perk->id = PERK_PSIONICIST_ENERGY_SPECIALIZATION;
  perk->name = strdup("Energy Specialization");
  perk->description = strdup("Choose an energy type (fire, cold, electric, acid, sonic, force). Your Psychokinesis powers of that type gain +1 DC.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_PSYCHOKINETIC_ARSENAL;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 DC */
  perk->effect_modifier = 0; /* chosen energy type stored elsewhere */
  perk->special_description = strdup("Tier 1: Choose energy type; Psychokinesis powers of that type gain +1 DC.");
  
  /*** PSYCHOKINETIC ARSENAL - TIER 2 PERKS (2 points each) ***/

  /* Kinetic Edge II */
  perk = &perk_list[PERK_PSIONICIST_KINETIC_EDGE_II];
  perk->id = PERK_PSIONICIST_KINETIC_EDGE_II;
  perk->name = strdup("Kinetic Edge II");
  perk->description = strdup("Total +2 dice on Psychokinesis blasts when augmented by >=3 PSP; energy burst/concussion blast splash +1 die.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_PSYCHOKINETIC_ARSENAL;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_KINETIC_EDGE_I;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* total +2 dice at >=3 PSP (combined with Tier I) */
  perk->effect_modifier = 3; /* requires augmentation >= 3 */
  perk->special_description = strdup("Tier 2: Psychokinesis blasts reach +2 dice at >=3 PSP; burst/concussion splash +1 die.");

  /* Deflective Screen */
  perk = &perk_list[PERK_PSIONICIST_DEFLECTIVE_SCREEN];
  perk->id = PERK_PSIONICIST_DEFLECTIVE_SCREEN;
  perk->name = strdup("Deflective Screen");
  perk->description = strdup("While force screen or inertial armor is active, gain +2 AC vs ranged and +2 Reflex; first hit each round is reduced by 5 damage.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_PSYCHOKINETIC_ARSENAL;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_FORCE_SCREEN_ADEPT;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 AC vs ranged */
  perk->effect_modifier = 2; /* +2 Reflex; 5 DR on first hit each round (handled in code) */
  perk->special_description = strdup("Tier 2: +2 AC (ranged) and +2 Reflex while shield/armor active; first hit each round -5 damage.");

  /* Accelerated Manifestation */
  perk = &perk_list[PERK_PSIONICIST_ACCELERATED_MANIFEST];
  perk->id = PERK_PSIONICIST_ACCELERATED_MANIFEST;
  perk->name = strdup("Accelerated Manifestation");
  perk->description = strdup("Once per combat, reduce PSP cost of a Psychokinesis power by 2 (min 1) and make it a faster action.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_PSYCHOKINETIC_ARSENAL;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* -2 PSP cost */
  perk->effect_modifier = 1; /* faster action */
  perk->special_description = strdup("Tier 2: Once per combat -2 PSP (min 1) and quickened cast for one Psychokinesis power.");

  /* Energy Retort (Perk) */
  perk = &perk_list[PERK_PSIONICIST_ENERGY_RETORT_PERK];
  perk->id = PERK_PSIONICIST_ENERGY_RETORT_PERK;
  perk->name = strdup("Energy Retort");
  perk->description = strdup("When struck in melee while you have an active Psychokinesis affect (force screen, energy retort, inertial armor), return level-based energy damage (scales with augment).");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_PSYCHOKINETIC_ARSENAL;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Tier 2: Reflect level-based energy damage when hit in melee while shield/armor/retort is active.");

  /*** PSYCHOKINETIC ARSENAL - TIER 3 PERKS (3 points each) ***/

  /* Kinetic Edge III */
  perk = &perk_list[PERK_PSIONICIST_KINETIC_EDGE_III];
  perk->id = PERK_PSIONICIST_KINETIC_EDGE_III;
  perk->name = strdup("Kinetic Edge III");
  perk->description = strdup("Total +3 dice on Psychokinesis blasts; energy ray/energy push gain +2 DC.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_PSYCHOKINETIC_ARSENAL;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_KINETIC_EDGE_II;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3; /* total +3 dice (combined with Tier I and II) */
  perk->effect_modifier = 2; /* +2 DC to energy ray/push */
  perk->special_description = strdup("Tier 3: Psychokinesis blasts reach +3 dice total; energy ray/push +2 DC.");

  /* Gravity Well */
  perk = &perk_list[PERK_PSIONICIST_GRAVITY_WELL];
  perk->id = PERK_PSIONICIST_GRAVITY_WELL;
  perk->name = strdup("Gravity Well");
  perk->description = strdup("Once per combat, create an AoE effect that halves speed and prevents fleeing (Reflex negates each round); lasts 3 rounds.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_PSYCHOKINETIC_ARSENAL;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3; /* 3 round duration */
  perk->effect_modifier = 50; /* 50% speed reduction */
  perk->special_description = strdup("Tier 3: Once/combat AoE gravity field halves speed, prevents fleeing (Reflex negates/round, 3 rounds).");

  /* Force Aegis */
  perk = &perk_list[PERK_PSIONICIST_FORCE_AEGIS];
  perk->id = PERK_PSIONICIST_FORCE_AEGIS;
  perk->name = strdup("Force Aegis");
  perk->description = strdup("+3 AC vs ranged/spells while force screen/inertial armor active; gain temp HP = manifester level on cast.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_PSYCHOKINETIC_ARSENAL;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_DEFLECTIVE_SCREEN;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3; /* +3 AC vs ranged/spells */
  perk->effect_modifier = 1; /* temp HP = manifester level */
  perk->special_description = strdup("Tier 3: +3 AC (ranged/spells) while shield/armor active; gain temp HP = manifester level on cast.");

  /* Kinetic Crush */
  perk = &perk_list[PERK_PSIONICIST_KINETIC_CRUSH];
  perk->id = PERK_PSIONICIST_KINETIC_CRUSH;
  perk->name = strdup("Kinetic Crush");
  perk->description = strdup("Forced-movement powers add prone on failed Reflex; if target collides, take extra force damage = manifester level.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_PSYCHOKINETIC_ARSENAL;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_VECTOR_SHOVE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* prone on failed save */
  perk->effect_modifier = 1; /* collision damage = manifester level */
  perk->special_description = strdup("Tier 3: Forced-movement powers prone on failed Reflex; collision adds force damage = manifester level.");

  /*** PSYCHOKINETIC ARSENAL - TIER 4 PERKS (Capstones - 5 points each) ***/

  /* Singular Impact */
  perk = &perk_list[PERK_PSIONICIST_SINGULAR_IMPACT];
  perk->id = PERK_PSIONICIST_SINGULAR_IMPACT;
  perk->name = strdup("Singular Impact");
  perk->description = strdup("1/day Psychokinesis strike: heavy force damage, auto-bull rush, and stun 1 round (Fort partial: half damage, no stun).");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_PSYCHOKINETIC_ARSENAL;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_KINETIC_EDGE_III;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* 1 round stun */
  perk->effect_modifier = 1; /* 1/day usage */
  perk->special_description = strdup("Tier 4 Capstone: 1/day heavy force strike with auto-bull rush and stun (Fort partial).");

  /* Perfect Deflection */
  perk = &perk_list[PERK_PSIONICIST_PERFECT_DEFLECTION];
  perk->id = PERK_PSIONICIST_PERFECT_DEFLECTION;
  perk->name = strdup("Perfect Deflection");
  perk->description = strdup("1/day reaction: negate one ranged/spell/psionic attack against you and reflect it using your casting stat vs the original attacker.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_PSYCHOKINETIC_ARSENAL;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_FORCE_AEGIS;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* 1/day usage */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Tier 4 Capstone: 1/day reaction negate and reflect one ranged/spell/psionic attack.");

  /*** METACREATIVE GENIUS - TIER 1 PERKS (1-2 points each) ***/

  /* Ectoplasmic Artisan I */
  perk = &perk_list[PERK_PSIONICIST_ECTOPLASMIC_ARTISAN_I];
  perk->id = PERK_PSIONICIST_ECTOPLASMIC_ARTISAN_I;
  perk->name = strdup("Ectoplasmic Artisan I");
  perk->description = strdup("Metacreativity powers cost 1 less PSP (min 1) once per encounter; +10% duration on metacreative buffs.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_METACREATIVE_GENIUS;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* PSP reduction */
  perk->effect_modifier = 10; /* 10% duration bonus */
  perk->special_description = strdup("Tier 1: Metacreativity PSP cost -1 (once/encounter); +10% buff duration.");

  /* Shard Volley */
  perk = &perk_list[PERK_PSIONICIST_SHARD_VOLLEY];
  perk->id = PERK_PSIONICIST_SHARD_VOLLEY;
  perk->name = strdup("Shard Volley");
  perk->description = strdup("Crystal shard gains +1 projectile (additional attack roll) when augmented by ≥2 PSP.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_METACREATIVE_GENIUS;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 projectile */
  perk->effect_modifier = 2; /* requires ≥2 PSP augment */
  perk->special_description = strdup("Tier 1: Crystal shard gains extra projectile when augmented ≥2 PSP.");

  /* Hardened Constructs I */
  perk = &perk_list[PERK_PSIONICIST_HARDENED_CONSTRUCTS_I];
  perk->id = PERK_PSIONICIST_HARDENED_CONSTRUCTS_I;
  perk->name = strdup("Hardened Constructs I");
  perk->description = strdup("Summons/creations gain temp HP = manifester level and +1 AC.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_METACREATIVE_GENIUS;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 AC */
  perk->effect_modifier = 1; /* temp HP = manifester level */
  perk->special_description = strdup("Tier 1: Summons gain temp HP = manifester level and +1 AC.");

  /* Fabricate Focus */
  perk = &perk_list[PERK_PSIONICIST_FABRICATE_FOCUS];
  perk->id = PERK_PSIONICIST_FABRICATE_FOCUS;
  perk->name = strdup("Fabricate Focus");
  perk->description = strdup("Metacreative powers manifest 10% faster.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_METACREATIVE_GENIUS;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10; /* 10% faster manifesting */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Tier 1: Metacreative powers manifest 10% faster.");

  /* Healing Aura I */
  perk = &perk_list[PERK_CLERIC_HEALING_AURA_1];
  perk->id = PERK_CLERIC_HEALING_AURA_1;
  perk->name = strdup("Healing Aura I");
  perk->description = strdup("Allies within 2 rooms regenerate +1 HP per tick");
  perk->associated_class = CLASS_CLERIC;
  perk->perk_category = PERK_CATEGORY_DIVINE_HEALER;
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
  perk->perk_category = PERK_CATEGORY_DIVINE_HEALER;
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
  perk->perk_category = PERK_CATEGORY_DIVINE_HEALER;
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
  perk->perk_category = PERK_CATEGORY_DIVINE_HEALER;
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
  perk->perk_category = PERK_CATEGORY_DIVINE_HEALER;
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
  perk->perk_category = PERK_CATEGORY_DIVINE_HEALER;
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
  perk->perk_category = PERK_CATEGORY_DIVINE_HEALER;
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
  perk->perk_category = PERK_CATEGORY_DIVINE_HEALER;
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
  perk->perk_category = PERK_CATEGORY_BATTLE_CLERIC;
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
  perk->perk_category = PERK_CATEGORY_BATTLE_CLERIC;
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
  perk->perk_category = PERK_CATEGORY_BATTLE_CLERIC;
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
  perk->perk_category = PERK_CATEGORY_BATTLE_CLERIC;
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
  perk->perk_category = PERK_CATEGORY_BATTLE_CLERIC;
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
  perk->perk_category = PERK_CATEGORY_BATTLE_CLERIC;
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
  perk->perk_category = PERK_CATEGORY_BATTLE_CLERIC;
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
  perk->perk_category = PERK_CATEGORY_BATTLE_CLERIC;
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
  perk->perk_category = PERK_CATEGORY_BATTLE_CLERIC;
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
  perk->perk_category = PERK_CATEGORY_BATTLE_CLERIC;
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
  perk->perk_category = PERK_CATEGORY_BATTLE_CLERIC;
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
  perk->perk_category = PERK_CATEGORY_BATTLE_CLERIC;
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
  perk->perk_category = PERK_CATEGORY_BATTLE_CLERIC;
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
  perk->perk_category = PERK_CATEGORY_BATTLE_CLERIC;
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
  perk->perk_category = PERK_CATEGORY_BATTLE_CLERIC;
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
  perk->perk_category = PERK_CATEGORY_BATTLE_CLERIC;
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
  perk->perk_category = PERK_CATEGORY_BATTLE_CLERIC;
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
  perk->perk_category = PERK_CATEGORY_BATTLE_CLERIC;
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
  perk->perk_category = PERK_CATEGORY_BATTLE_CLERIC;
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
  perk->perk_category = PERK_CATEGORY_BATTLE_CLERIC;
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
  perk->perk_category = PERK_CATEGORY_DOMAIN_MASTER;
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
  perk->perk_category = PERK_CATEGORY_DOMAIN_MASTER;
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
  perk->perk_category = PERK_CATEGORY_DOMAIN_MASTER;
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
  perk->perk_category = PERK_CATEGORY_DOMAIN_MASTER;
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
  perk->perk_category = PERK_CATEGORY_DOMAIN_MASTER;
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
  perk->perk_category = PERK_CATEGORY_DOMAIN_MASTER;
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
  perk->perk_category = PERK_CATEGORY_DOMAIN_MASTER;
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
  perk->perk_category = PERK_CATEGORY_DOMAIN_MASTER;
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
  perk->perk_category = PERK_CATEGORY_DOMAIN_MASTER;
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
  perk->perk_category = PERK_CATEGORY_DOMAIN_MASTER;
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
  perk->perk_category = PERK_CATEGORY_DOMAIN_MASTER;
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
  perk->perk_category = PERK_CATEGORY_DOMAIN_MASTER;
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
  perk->perk_category = PERK_CATEGORY_DOMAIN_MASTER;
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
  perk->perk_category = PERK_CATEGORY_DOMAIN_MASTER;
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
  perk->perk_category = PERK_CATEGORY_DOMAIN_MASTER;
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
  perk->perk_category = PERK_CATEGORY_DOMAIN_MASTER;
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
  perk->perk_category = PERK_CATEGORY_DOMAIN_MASTER;
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
  perk->perk_category = PERK_CATEGORY_DOMAIN_MASTER;
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
  perk->perk_category = PERK_CATEGORY_DOMAIN_MASTER;
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
  perk->perk_category = PERK_CATEGORY_ASSASSIN;
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
  perk->perk_category = PERK_CATEGORY_ASSASSIN;
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
  perk->perk_category = PERK_CATEGORY_ASSASSIN;
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
  perk->perk_category = PERK_CATEGORY_ASSASSIN;
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
  perk->perk_category = PERK_CATEGORY_ASSASSIN;
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
  perk->perk_category = PERK_CATEGORY_ASSASSIN;
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
  perk->perk_category = PERK_CATEGORY_ASSASSIN;
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
  perk->perk_category = PERK_CATEGORY_ASSASSIN;
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
  perk->perk_category = PERK_CATEGORY_ASSASSIN;
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
  perk->perk_category = PERK_CATEGORY_ASSASSIN;
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
  perk->perk_category = PERK_CATEGORY_ASSASSIN;
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
  perk->perk_category = PERK_CATEGORY_ASSASSIN;
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
  perk->perk_category = PERK_CATEGORY_ASSASSIN;
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
  perk->perk_category = PERK_CATEGORY_ASSASSIN;
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
  perk->perk_category = PERK_CATEGORY_ASSASSIN;
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
  perk->perk_category = PERK_CATEGORY_ASSASSIN;
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
  perk->perk_category = PERK_CATEGORY_ASSASSIN;
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
  perk->perk_category = PERK_CATEGORY_MASTER_THIEF;
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
  perk->perk_category = PERK_CATEGORY_MASTER_THIEF;
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
  perk->perk_category = PERK_CATEGORY_MASTER_THIEF;
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
  perk->perk_category = PERK_CATEGORY_MASTER_THIEF;
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
  perk->perk_category = PERK_CATEGORY_MASTER_THIEF;
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
  perk->perk_category = PERK_CATEGORY_MASTER_THIEF;
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
  perk->perk_category = PERK_CATEGORY_MASTER_THIEF;
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
  perk->perk_category = PERK_CATEGORY_MASTER_THIEF;
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
  perk->perk_category = PERK_CATEGORY_MASTER_THIEF;
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
  perk->perk_category = PERK_CATEGORY_MASTER_THIEF;
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
  perk->perk_category = PERK_CATEGORY_MASTER_THIEF;
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
  perk->perk_category = PERK_CATEGORY_MASTER_THIEF;
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
  perk->perk_category = PERK_CATEGORY_MASTER_THIEF;
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
  perk->perk_category = PERK_CATEGORY_MASTER_THIEF;
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
  perk->perk_category = PERK_CATEGORY_SHADOW_SCOUT;
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
  perk->perk_category = PERK_CATEGORY_MASTER_THIEF;
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
  perk->perk_category = PERK_CATEGORY_MASTER_THIEF;
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
  perk->perk_category = PERK_CATEGORY_MASTER_THIEF;
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
  perk->perk_category = PERK_CATEGORY_SHADOW_SCOUT;
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
  perk->perk_category = PERK_CATEGORY_SHADOW_SCOUT;
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
  perk->perk_category = PERK_CATEGORY_SHADOW_SCOUT;
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
  perk->perk_category = PERK_CATEGORY_SHADOW_SCOUT;
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
  perk->perk_category = PERK_CATEGORY_SHADOW_SCOUT;
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
  perk->perk_category = PERK_CATEGORY_SHADOW_SCOUT;
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
  perk->perk_category = PERK_CATEGORY_SHADOW_SCOUT;
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
  perk->perk_category = PERK_CATEGORY_SHADOW_SCOUT;
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
  perk->perk_category = PERK_CATEGORY_SHADOW_SCOUT;
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
  perk->perk_category = PERK_CATEGORY_SHADOW_SCOUT;
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
  perk->perk_category = PERK_CATEGORY_SHADOW_SCOUT;
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
  perk->perk_category = PERK_CATEGORY_SHADOW_SCOUT;
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
  perk->perk_category = PERK_CATEGORY_SHADOW_SCOUT;
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
  perk->perk_category = PERK_CATEGORY_SHADOW_SCOUT;
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
  perk->perk_category = PERK_CATEGORY_SHADOW_SCOUT;
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
  perk->perk_category = PERK_CATEGORY_SHADOW_SCOUT;
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
  perk->perk_category = PERK_CATEGORY_SHADOW_SCOUT;
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
  perk->perk_category = PERK_CATEGORY_SHADOW_SCOUT;
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
  perk->perk_category = PERK_CATEGORY_SHADOW_SCOUT;
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
  perk->perk_category = PERK_CATEGORY_HUNTER;
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
  perk->perk_category = PERK_CATEGORY_WILDERNESS_WARRIOR;
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
  perk->perk_category = PERK_CATEGORY_HUNTER;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_WEAPON_TOHIT;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Bonus to hit with bow weapons");
  
  /*** HUNTER TREE - TIER I ***/
  
  /* Archer's Focus I */
  perk = &perk_list[PERK_RANGER_ARCHERS_FOCUS_I];
  perk->id = PERK_RANGER_ARCHERS_FOCUS_I;
  perk->name = strdup("Archer's Focus I");
  perk->description = strdup("+1 to hit with ranged weapons per rank");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_HUNTER;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("+1 to hit with ranged weapons per rank");
  
  /* Steady Aim I */
  perk = &perk_list[PERK_RANGER_STEADY_AIM_I];
  perk->id = PERK_RANGER_STEADY_AIM_I;
  perk->name = strdup("Steady Aim I");
  perk->description = strdup("+1 damage with ranged weapons per rank");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_HUNTER;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("+1 damage with ranged weapons per rank");
  
  /* Quick Draw */
  perk = &perk_list[PERK_RANGER_QUICK_DRAW];
  perk->id = PERK_RANGER_QUICK_DRAW;
  perk->name = strdup("Quick Draw");
  perk->description = strdup("Grants a 5% chance per rank to immediately fire an extra arrow/missile on ranged attacks");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_HUNTER;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10;
  perk->effect_modifier = 0;
  perk->special_description = strdup("On ranged attacks, 5% chance per rank to make one extra immediate shot");
  
  /* Improved Critical: Ranged I */
  perk = &perk_list[PERK_RANGER_IMPROVED_CRITICAL_RANGED_I];
  perk->id = PERK_RANGER_IMPROVED_CRITICAL_RANGED_I;
  perk->name = strdup("Improved Critical: Ranged I");
  perk->description = strdup("+1 critical threat range with ranged weapons (19-20 becomes 18-20)");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_HUNTER;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_CRITICAL_CHANCE;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Expands critical threat range with ranged weapons by 1");
  
  /*** HUNTER TREE - TIER II ***/
  
  /* Archer's Focus II */
  perk = &perk_list[PERK_RANGER_ARCHERS_FOCUS_II];
  perk->id = PERK_RANGER_ARCHERS_FOCUS_II;
  perk->name = strdup("Archer's Focus II");
  perk->description = strdup("Additional +2 to hit with ranged weapons per rank");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_HUNTER;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_RANGER_ARCHERS_FOCUS_I;
  perk->prerequisite_rank = 3; /* Requires Archer's Focus I at max rank */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("+2 to hit with ranged weapons per rank (stacks with Archer's Focus I)");
  
  /* Deadly Aim */
  perk = &perk_list[PERK_RANGER_DEADLY_AIM];
  perk->id = PERK_RANGER_DEADLY_AIM;
  perk->name = strdup("Deadly Aim");
  perk->description = strdup("Arrows/bolts ignore 5 points of DR per rank");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_HUNTER;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_RANGER_STEADY_AIM_I;
  perk->prerequisite_rank = 1; /* Requires Steady Aim I (1 rank) */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Ranged attacks ignore 5 points of damage reduction per rank");
  
  /* Manyshot */
  perk = &perk_list[PERK_RANGER_MANYSHOT];
  perk->id = PERK_RANGER_MANYSHOT;
  perk->name = strdup("Manyshot");
  perk->description = strdup("Once per combat, fire an additional arrow at your target (does not consume ammunition)");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_HUNTER;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_RANGER_QUICK_DRAW;
  perk->prerequisite_rank = 2; /* Requires Quick Draw (2 ranks) */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Special ranged attack ability usable once per combat");
  
  /* Hunter's Mark */
  perk = &perk_list[PERK_RANGER_HUNTERS_MARK];
  perk->id = PERK_RANGER_HUNTERS_MARK;
  perk->name = strdup("Hunter's Mark");
  perk->description = strdup("Mark a target for 5 rounds; +2 to hit and +1d6 damage against marked target");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_HUNTER;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_RANGER_ARCHERS_FOCUS_I;
  perk->prerequisite_rank = 1; /* Requires Archer's Focus I (1 rank) */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Marks target for 5 rounds granting +2 to hit and +1d6 damage");

  /*** HUNTER TREE - TIER III ***/

  /* Improved Manyshot */
  perk = &perk_list[PERK_RANGER_IMPROVED_MANYSHOT];
  perk->id = PERK_RANGER_IMPROVED_MANYSHOT;
  perk->name = strdup("Improved Manyshot");
  perk->description = strdup("Manyshot fires 2 additional arrows and has reduced cooldown");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_HUNTER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_RANGER_MANYSHOT;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 additional arrows */
  perk->effect_modifier = 0; /* cooldown handled in code */
  perk->special_description = strdup("Upgrades Manyshot: +2 arrows, shorter cooldown (approx. twice per combat)");

  /* Sniper */
  perk = &perk_list[PERK_RANGER_SNIPER];
  perk->id = PERK_RANGER_SNIPER;
  perk->name = strdup("Sniper");
  perk->description = strdup("Critical hits with ranged weapons deal +2d6 damage");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_HUNTER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_RANGER_IMPROVED_CRITICAL_RANGED_I;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2d6 on crits */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Ranged weapon critical hits deal +2d6 damage");

  /* Longshot */
  perk = &perk_list[PERK_RANGER_LONGSHOT];
  perk->id = PERK_RANGER_LONGSHOT;
  perk->name = strdup("Longshot");
  perk->description = strdup("Increase effective range by 50%; ignore long range penalties");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_HUNTER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_RANGER_ARCHERS_FOCUS_II;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 50; /* +50% range - future hook */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Future hook: extends ranged bands and removes long-range penalties");

  /* Pinpoint Accuracy */
  perk = &perk_list[PERK_RANGER_PINPOINT_ACCURACY];
  perk->id = PERK_RANGER_PINPOINT_ACCURACY;
  perk->name = strdup("Pinpoint Accuracy");
  perk->description = strdup("Ignore cover and concealment with ranged attacks");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_HUNTER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_RANGER_DEADLY_AIM;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Ranged attacks ignore concealment and cover where applicable");

  /*** HUNTER TREE - TIER IV (CAPSTONES) ***/

  /* Master Archer */
  perk = &perk_list[PERK_RANGER_MASTER_ARCHER];
  perk->id = PERK_RANGER_MASTER_ARCHER;
  perk->name = strdup("Master Archer");
  perk->description = strdup("Ranged crit range becomes 19-20; critical multiplier becomes x4");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_HUNTER;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_RANGER_SNIPER;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Capstone: 19-20 crits and x4 crit multiplier for ranged attacks");

  /* Arrow Storm */
  perk = &perk_list[PERK_RANGER_ARROW_STORM];
  perk->id = PERK_RANGER_ARROW_STORM;
  perk->name = strdup("Arrow Storm");
  perk->description = strdup("Once per day: hit all enemies in the room for 6d6 damage");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_HUNTER;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_RANGER_IMPROVED_MANYSHOT;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 6; /* 6d6 damage */
  perk->effect_modifier = 6;
  perk->special_description = strdup("Capstone active: 'arrowstorm' command to deal 6d6 to all foes in room, 24h cooldown");

  /*** BEAST MASTER TREE - TIER I ***/

  /* Enhanced Companion I */
  perk = &perk_list[PERK_RANGER_ENHANCED_COMPANION_I];
  perk->id = PERK_RANGER_ENHANCED_COMPANION_I;
  perk->name = strdup("Enhanced Companion I");
  perk->description = strdup("Animal companion gains +5 HP and +1 AC per rank");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_BEAST_MASTER;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5; /* +5 HP per rank */
  perk->effect_modifier = 1; /* +1 AC per rank */
  perk->special_description = strdup("Animal companion gains +5 HP and +1 AC per rank");

  /* Pack Tactics I */
  perk = &perk_list[PERK_RANGER_PACK_TACTICS_I];
  perk->id = PERK_RANGER_PACK_TACTICS_I;
  perk->name = strdup("Pack Tactics I");
  perk->description = strdup("You and your animal companion gain +1 to hit when flanking the same enemy per rank");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_BEAST_MASTER;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("+1 to hit when you and your companion attack the same target per rank");

  /* Natural Empathy I */
  perk = &perk_list[PERK_RANGER_NATURAL_EMPATHY_I];
  perk->id = PERK_RANGER_NATURAL_EMPATHY_I;
  perk->name = strdup("Natural Empathy I");
  perk->description = strdup("+2 bonus to all Animal Handling and Animal Empathy checks per rank");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_BEAST_MASTER;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SKILL;
  perk->effect_value = 2; /* +2 per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Enhances animal-related skill checks");

  /* Spell Focus: Conjuration I */
  perk = &perk_list[PERK_RANGER_SPELL_FOCUS_CONJURATION_I];
  perk->id = PERK_RANGER_SPELL_FOCUS_CONJURATION_I;
  perk->name = strdup("Spell Focus: Conjuration I");
  perk->description = strdup("+1 DC to conjuration spells (includes summon spells) per rank");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_BEAST_MASTER;
  perk->cost = 1;
  perk->max_rank = 2;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 DC per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Increases DC of conjuration spells including summons");

  /*** BEAST MASTER TREE - TIER II ***/

  /* Enhanced Companion II */
  perk = &perk_list[PERK_RANGER_ENHANCED_COMPANION_II];
  perk->id = PERK_RANGER_ENHANCED_COMPANION_II;
  perk->name = strdup("Enhanced Companion II");
  perk->description = strdup("Animal companion gains +10 HP, +2 AC, and +2 to hit per rank");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_BEAST_MASTER;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_RANGER_ENHANCED_COMPANION_I;
  perk->prerequisite_rank = 3; /* Requires Enhanced Companion I at max rank */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10; /* +10 HP per rank */
  perk->effect_modifier = 2; /* +2 AC and +2 to-hit per rank */
  perk->special_description = strdup("Companion gains +10 HP, +2 AC, and +2 to hit per rank (stacks with Enhanced Companion I)");

  /* Feral Charge */
  perk = &perk_list[PERK_RANGER_FERAL_CHARGE];
  perk->id = PERK_RANGER_FERAL_CHARGE;
  perk->name = strdup("Feral Charge");
  perk->description = strdup("Animal companion's first attack each combat deals +2d6 damage");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_BEAST_MASTER;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_RANGER_PACK_TACTICS_I;
  perk->prerequisite_rank = 2; /* Requires Pack Tactics I (2 ranks) */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2d6 */
  perk->effect_modifier = 6;
  perk->special_description = strdup("Companion's opening attack in each combat deals +2d6 damage");

  /* Nature's Remedy */
  perk = &perk_list[PERK_RANGER_NATURES_REMEDY];
  perk->id = PERK_RANGER_NATURES_REMEDY;
  perk->name = strdup("Nature's Remedy");
  perk->description = strdup("Healing spells cast by you are 25% more effective per rank");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_BEAST_MASTER;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_RANGER_SPELL_FOCUS_CONJURATION_I;
  perk->prerequisite_rank = 1; /* Requires Spell Focus: Conjuration I (1 rank) */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 25; /* +25% per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Healing spells you cast are 25% more effective per rank");

  /* Shared Spells */
  perk = &perk_list[PERK_RANGER_SHARED_SPELLS];
  perk->id = PERK_RANGER_SHARED_SPELLS;
  perk->name = strdup("Shared Spells");
  perk->description = strdup("Beneficial spells cast on yourself also affect your animal companion");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_BEAST_MASTER;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_RANGER_ENHANCED_COMPANION_I;
  perk->prerequisite_rank = 2; /* Requires Enhanced Companion I (2 ranks) */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Personal buffs automatically extend to your companion");

  /*** BEAST MASTER TREE - TIER III ***/

  /* Alpha Bond */
  perk = &perk_list[PERK_RANGER_ALPHA_BOND];
  perk->id = PERK_RANGER_ALPHA_BOND;
  perk->name = strdup("Alpha Bond");
  perk->description = strdup("Your animal companion gains +3 to all saves and immunity to fear");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_BEAST_MASTER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_RANGER_ENHANCED_COMPANION_II;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3; /* +3 to saves */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Companion gains +3 to all saves and immunity to fear");

  /* Coordinated Attack */
  perk = &perk_list[PERK_RANGER_COORDINATED_ATTACK];
  perk->id = PERK_RANGER_COORDINATED_ATTACK;
  perk->name = strdup("Coordinated Attack");
  perk->description = strdup("When you and your companion attack the same target, both gain +2d4 damage");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_BEAST_MASTER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_RANGER_PACK_TACTICS_I;
  perk->prerequisite_rank = 3; /* Requires Pack Tactics I (max rank) AND Feral Charge (checked separately) */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2d4 */
  perk->effect_modifier = 4;
  perk->special_description = strdup("Requires: Pack Tactics I (max rank) and Feral Charge. You and companion both gain +2d4 damage when attacking same target");

  /* Primal Vigor */
  perk = &perk_list[PERK_RANGER_PRIMAL_VIGOR];
  perk->id = PERK_RANGER_PRIMAL_VIGOR;
  perk->name = strdup("Primal Vigor");
  perk->description = strdup("You and your animal companion regenerate 1 HP per round in combat");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_BEAST_MASTER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_RANGER_NATURES_REMEDY;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* 1 HP per round */
  perk->effect_modifier = 0;
  perk->special_description = strdup("You and your companion regenerate 1 HP per round during combat");

  /* Greater Summons */
  perk = &perk_list[PERK_RANGER_GREATER_SUMMONS];
  perk->id = PERK_RANGER_GREATER_SUMMONS;
  perk->name = strdup("Greater Summons");
  perk->description = strdup("All summoned creatures have +25% HP, +4 to attack rolls and deal +1d6 damage");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_BEAST_MASTER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_RANGER_SPELL_FOCUS_CONJURATION_I;
  perk->prerequisite_rank = 2; /* Requires Spell Focus: Conjuration I (max rank) */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 25; /* +25% HP */
  perk->effect_modifier = 4; /* +4 to attack rolls */
  perk->special_description = strdup("All summoned creatures gain +25% HP, +4 to attack rolls, and +1d6 damage");

  /*** BEAST MASTER TREE - TIER IV (CAPSTONES) ***/

  /* Primal Avatar */
  perk = &perk_list[PERK_RANGER_PRIMAL_AVATAR];
  perk->id = PERK_RANGER_PRIMAL_AVATAR;
  perk->name = strdup("Primal Avatar");
  perk->description = strdup("Your animal companion becomes a Primal Beast: +50 HP, +5 AC, +5 to hit, attacks deal +3d6 damage, immune to mind-affecting");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_BEAST_MASTER;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_RANGER_ALPHA_BOND;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 50; /* +50 HP */
  perk->effect_modifier = 5; /* +5 AC/to-hit, +3d6 damage handled separately */
  perk->special_description = strdup("Capstone: Companion becomes Primal Beast with massive bonuses and mind-affecting immunity");

  /* Nature's Wrath */
  perk = &perk_list[PERK_RANGER_NATURES_WRATH];
  perk->id = PERK_RANGER_NATURES_WRATH;
  perk->name = strdup("Nature's Wrath");
  perk->description = strdup("Once per day: you and your companion gain +4 to all stats, +2d8 damage on all attacks, and fast healing 5 for 10 rounds");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_BEAST_MASTER;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_RANGER_COORDINATED_ATTACK;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4; /* +4 to stats */
  perk->effect_modifier = 10; /* 10 rounds duration */
  perk->special_description = strdup("Capstone active: 'natureswrath' command grants powerful temporary bonuses to you and companion, 24h cooldown");

  /*** WILDERNESS WARRIOR TREE (partial) ***/

  /* Favored Enemy Slayer */
  perk = &perk_list[PERK_RANGER_FAVORED_ENEMY_SLAYER];
  perk->id = PERK_RANGER_FAVORED_ENEMY_SLAYER;
  perk->name = strdup("Favored Enemy Slayer");
  perk->description = strdup("+2 to hit against favored enemies");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_WILDERNESS_WARRIOR;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL; /* handled in combat code */
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Grants +2 to-hit vs favored enemies");

  /* Apex Predator (capstone) */
  perk = &perk_list[PERK_RANGER_APEX_PREDATOR];
  perk->id = PERK_RANGER_APEX_PREDATOR;
  perk->name = strdup("Apex Predator");
  perk->description = strdup("+5 to hit against favored enemies");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_WILDERNESS_WARRIOR;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_RANGER_FAVORED_ENEMY_SLAYER; /* reasonable prerequisite */
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL; /* handled in combat code */
  perk->effect_value = 5;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Capstone: +5 to-hit vs favored enemies");

  /*** WILDERNESS WARRIOR TREE - TIER I ***/

  /* Two-Weapon Focus I */
  perk = &perk_list[PERK_RANGER_TWO_WEAPON_FOCUS_I];
  perk->id = PERK_RANGER_TWO_WEAPON_FOCUS_I;
  perk->name = strdup("Two-Weapon Focus I");
  perk->description = strdup("+1 to attacks when dual wielding, per rank");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_WILDERNESS_WARRIOR;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("+1 to hit when dual wielding per rank");

  /* Dual Strike I */
  perk = &perk_list[PERK_RANGER_DUAL_STRIKE_I];
  perk->id = PERK_RANGER_DUAL_STRIKE_I;
  perk->name = strdup("Dual Strike I");
  perk->description = strdup("+1 damage with off-hand weapon per rank");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_WILDERNESS_WARRIOR;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("+1 damage with off-hand weapon per rank");

  /* Favored Enemy Mastery I */
  perk = &perk_list[PERK_RANGER_FAVORED_ENEMY_MASTERY_I];
  perk->id = PERK_RANGER_FAVORED_ENEMY_MASTERY_I;
  perk->name = strdup("Favored Enemy Mastery I");
  perk->description = strdup("+1 damage vs favored enemies per rank");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_WILDERNESS_WARRIOR;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("+1 damage vs favored enemies per rank");

  /* Ranger Toughness I */
  perk = &perk_list[PERK_RANGER_RANGER_TOUGHNESS_I];
  perk->id = PERK_RANGER_RANGER_TOUGHNESS_I;
  perk->name = strdup("Ranger Toughness I");
  perk->description = strdup("+5 HP per rank");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_WILDERNESS_WARRIOR;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_HP;
  perk->effect_value = 5;
  perk->effect_modifier = 0;
  perk->special_description = strdup("+5 HP per rank");

  /*** WILDERNESS WARRIOR TREE - TIER II ***/

  /* Two-Weapon Focus II */
  perk = &perk_list[PERK_RANGER_TWO_WEAPON_FOCUS_II];
  perk->id = PERK_RANGER_TWO_WEAPON_FOCUS_II;
  perk->name = strdup("Two-Weapon Focus II");
  perk->description = strdup("+1 damage when dual wielding per rank");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_WILDERNESS_WARRIOR;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_RANGER_TWO_WEAPON_FOCUS_I;
  perk->prerequisite_rank = 3;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("+1 damage when dual wielding per rank (stacks with TWF I)");

  /* Wilderness Warrior Two-Weapon Fighting */
  perk = &perk_list[PERK_RANGER_WW_TWO_WEAPON_FIGHTING];
  perk->id = PERK_RANGER_WW_TWO_WEAPON_FIGHTING;
  perk->name = strdup("Wilderness Warrior Two-Weapon Fighting");
  perk->description = strdup("10% chance to gain an additional off-hand attack per round");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_WILDERNESS_WARRIOR;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_RANGER_TWO_WEAPON_FOCUS_I;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10;
  perk->effect_modifier = 0;
  perk->special_description = strdup("10% chance per round for an extra off-hand attack when dual wielding");

  /* Tempest */
  perk = &perk_list[PERK_RANGER_TEMPEST];
  perk->id = PERK_RANGER_TEMPEST;
  perk->name = strdup("Tempest");
  perk->description = strdup("+1 AC dodge bonus when fighting with two weapons per rank");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_WILDERNESS_WARRIOR;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_RANGER_TWO_WEAPON_FOCUS_I;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_AC;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("+1 AC dodge bonus per rank when dual wielding");

  /* Favored Enemy Slayer */
  perk = &perk_list[PERK_RANGER_FAVORED_ENEMY_SLAYER];
  perk->id = PERK_RANGER_FAVORED_ENEMY_SLAYER;
  perk->name = strdup("Favored Enemy Slayer");
  perk->description = strdup("+2 to hit against favored enemies, critical threat range +1 vs favored enemies");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_WILDERNESS_WARRIOR;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_RANGER_FAVORED_ENEMY_MASTERY_I;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 1;
  perk->special_description = strdup("+2 to hit vs favored enemies and +1 critical threat range");

  /*** WILDERNESS WARRIOR TREE - TIER III ***/

  /* Greater Wilderness Warrior Two-Weapon Fighting */
  perk = &perk_list[PERK_RANGER_GREATER_WW_TWO_WEAPON_FIGHTING];
  perk->id = PERK_RANGER_GREATER_WW_TWO_WEAPON_FIGHTING;
  perk->name = strdup("Greater WW Two-Weapon Fighting");
  perk->description = strdup("10% chance to gain a second additional off-hand attack per round");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_WILDERNESS_WARRIOR;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_RANGER_WW_TWO_WEAPON_FIGHTING;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10;
  perk->effect_modifier = 0;
  perk->special_description = strdup("10% chance per round for a second extra off-hand attack");

  /* Whirling Steel */
  perk = &perk_list[PERK_RANGER_WHIRLING_STEEL];
  perk->id = PERK_RANGER_WHIRLING_STEEL;
  perk->name = strdup("Whirling Steel");
  perk->description = strdup("5% chance per hit to make an additional free attack when dual wielding");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_WILDERNESS_WARRIOR;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_RANGER_TWO_WEAPON_FOCUS_II;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5;
  perk->effect_modifier = 0;
  perk->special_description = strdup("5% chance per hit for a free attack when dual wielding");

  /* Deadly Hunter */
  perk = &perk_list[PERK_RANGER_DEADLY_HUNTER];
  perk->id = PERK_RANGER_DEADLY_HUNTER;
  perk->name = strdup("Deadly Hunter");
  perk->description = strdup("Against favored enemies: +2d6 damage and attacks ignore 10 points of DR");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_WILDERNESS_WARRIOR;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_RANGER_FAVORED_ENEMY_SLAYER;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 6;
  perk->special_description = strdup("+2d6 damage and ignore 10 DR vs favored enemies");

  /* Crippling Strike */
  perk = &perk_list[PERK_RANGER_CRIPPLING_STRIKE];
  perk->id = PERK_RANGER_CRIPPLING_STRIKE;
  perk->name = strdup("Crippling Strike");
  perk->description = strdup("Successful melee attacks have a 5% chance to apply slow for 3 rounds");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_WILDERNESS_WARRIOR;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_RANGER_DUAL_STRIKE_I;
  perk->prerequisite_rank = 3;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5;
  perk->effect_modifier = 3;
  perk->special_description = strdup("5% chance to slow target for 3 rounds on melee hit");

  /*** WILDERNESS WARRIOR TREE - TIER IV (CAPSTONES) ***/

  /* Perfect WW Two-Weapon Fighting */
  perk = &perk_list[PERK_RANGER_PERFECT_WW_TWO_WEAPON_FIGHTING];
  perk->id = PERK_RANGER_PERFECT_WW_TWO_WEAPON_FIGHTING;
  perk->name = strdup("Perfect WW Two-Weapon Fighting");
  perk->description = strdup("All weapon attacks gain +2 to hit +4 to damage when dual wielding");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_WILDERNESS_WARRIOR;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_RANGER_GREATER_WW_TWO_WEAPON_FIGHTING;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 4;
  perk->special_description = strdup("+2 to hit, +4 to damage when dual wielding (capstone)");
}

/* Define Bard Perks */
void define_bard_perks(void)
{
  struct perk_data *perk;

  /*** SPELLSINGER TREE - TIER I ***/

  /* Songweaver I */
  perk = &perk_list[PERK_BARD_SONGWEAVER_I];
  perk->id = PERK_BARD_SONGWEAVER_I;
  perk->name = strdup("Songweaver I");
  perk->description = strdup("Bard songs gain +1 effective level per rank for duration and potency");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SPELLSINGER;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Bard songs gain +1 effective level per rank");

  /* Enchanter's Guile I */
  perk = &perk_list[PERK_BARD_ENCHANTERS_GUILE_I];
  perk->id = PERK_BARD_ENCHANTERS_GUILE_I;
  perk->name = strdup("Enchanter's Guile I");
  perk->description = strdup("+1 DC to Enchantment and Illusion spells per rank");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SPELLSINGER;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("+1 DC to Enchantment and Illusion spells per rank");

  /* Resonant Voice I */
  perk = &perk_list[PERK_BARD_RESONANT_VOICE_I];
  perk->id = PERK_BARD_RESONANT_VOICE_I;
  perk->name = strdup("Resonant Voice I");
  perk->description = strdup("Allies under your songs gain +1 competence to saves vs. mind-affecting per rank");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SPELLSINGER;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Allies gain +1 save vs. mind-affecting per rank");

  /* Harmonic Casting */
  perk = &perk_list[PERK_BARD_HARMONIC_CASTING];
  perk->id = PERK_BARD_HARMONIC_CASTING;
  perk->name = strdup("Harmonic Casting");
  perk->description = strdup("Casting a bard spell while maintaining a song has a 50% chance to not consume a performance round");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SPELLSINGER;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 50;
  perk->effect_modifier = 0;
  perk->special_description = strdup("50% chance to save performance round when casting spell during song");

  /*** SPELLSINGER TREE - TIER II ***/

  /* Songweaver II */
  perk = &perk_list[PERK_BARD_SONGWEAVER_II];
  perk->id = PERK_BARD_SONGWEAVER_II;
  perk->name = strdup("Songweaver II");
  perk->description = strdup("Additional +1 effective song level per rank (stacks with Songweaver I)");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SPELLSINGER;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_BARD_SONGWEAVER_I;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Additional +1 effective song level per rank");

  /* Enchanter's Guile II */
  perk = &perk_list[PERK_BARD_ENCHANTERS_GUILE_II];
  perk->id = PERK_BARD_ENCHANTERS_GUILE_II;
  perk->name = strdup("Enchanter's Guile II");
  perk->description = strdup("Additional +1 DC to Enchantment and Illusion spells per rank");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SPELLSINGER;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_BARD_ENCHANTERS_GUILE_I;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Additional +1 DC to Enchantment and Illusion spells per rank");

  /* Crescendo */
  perk = &perk_list[PERK_BARD_CRESCENDO];
  perk->id = PERK_BARD_CRESCENDO;
  perk->name = strdup("Crescendo");
  perk->description = strdup("The first spell you cast after starting a song deals +1d6 sonic damage and has +2 to its save DC");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SPELLSINGER;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_HARMONIC_CASTING;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 6;
  perk->effect_modifier = 2;
  perk->special_description = strdup("First spell after song: +1d6 sonic damage, +2 save DC");

  /* Sustaining Melody */
  perk = &perk_list[PERK_BARD_SUSTAINING_MELODY];
  perk->id = PERK_BARD_SUSTAINING_MELODY;
  perk->name = strdup("Sustaining Melody");
  perk->description = strdup("While a song is active, you have a 20% chance per combat round to recover 1 spell slot only while in combat");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SPELLSINGER;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_SONGWEAVER_I;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 20;
  perk->effect_modifier = 1;
  perk->special_description = strdup("20% chance per round to recover 1 spell slot while performing in combat");

  /* ========================================================================
   * TIER III - SPELLSINGER TREE
   * ======================================================================== */

  /* Master of Motifs */
  perk = &perk_list[PERK_BARD_MASTER_OF_MOTIFS];
  perk->id = PERK_BARD_MASTER_OF_MOTIFS;
  perk->name = strdup("Master of Motifs");
  perk->description = strdup("Maintain up to two distinct bard songs simultaneously (shared performance pool)");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SPELLSINGER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_SUSTAINING_MELODY;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Allows maintaining 2 distinct bard songs simultaneously");

  /* Dirge of Dissonance */
  perk = &perk_list[PERK_BARD_DIRGE_OF_DISSONANCE];
  perk->id = PERK_BARD_DIRGE_OF_DISSONANCE;
  perk->name = strdup("Dirge of Dissonance");
  perk->description = strdup("Enemies in the room take 1d6 sonic damage per round and -2 penalty to concentration checks while your song persists");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SPELLSINGER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_CRESCENDO;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 6; /* 1d6 damage */
  perk->effect_modifier = -2; /* concentration penalty */
  perk->special_description = strdup("Room-wide attrition: 1d6 sonic damage and -2 concentration per round");

  /* Heightened Harmony */
  perk = &perk_list[PERK_BARD_HEIGHTENED_HARMONY];
  perk->id = PERK_BARD_HEIGHTENED_HARMONY;
  perk->name = strdup("Heightened Harmony");
  perk->description = strdup("When you spend metamagic on a bard spell, you gain +5 to your perform skill for one minute");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SPELLSINGER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_ENCHANTERS_GUILE_II;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5; /* perform bonus */
  perk->effect_modifier = 60; /* duration in seconds */
  perk->special_description = strdup("Metamagic grants +5 perform for 1 minute");

  /* Protective Chorus */
  perk = &perk_list[PERK_BARD_PROTECTIVE_CHORUS];
  perk->id = PERK_BARD_PROTECTIVE_CHORUS;
  perk->name = strdup("Protective Chorus");
  perk->description = strdup("Allies under your song gain +2 to saves vs. spells and +2 to AC vs. attacks of opportunity");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SPELLSINGER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_RESONANT_VOICE_I;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* save bonus */
  perk->effect_modifier = 2; /* AC bonus */
  perk->special_description = strdup("Allies gain +2 saves vs. spells and +2 AC vs. AoO");

  /* Tier IV Spellsinger Perks - Capstones */

  /* Spellsong Maestra */
  perk = &perk_list[PERK_BARD_SPELLSONG_MAESTRA];
  perk->id = PERK_BARD_SPELLSONG_MAESTRA;
  perk->name = strdup("Spellsong Maestra");
  perk->description = strdup("While performing, bard spells gain +2 caster level, +2 spell DC, and metamagic on bard spells is free");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SPELLSINGER;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_MASTER_OF_MOTIFS;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* caster level bonus */
  perk->effect_modifier = 2; /* DC bonus */
  perk->special_description = strdup("Capstone: +2 caster, +2 DC on bard spells while performing; free metamagic on bard spells");

  /* Aria of Stasis */
  perk = &perk_list[PERK_BARD_ARIA_OF_STASIS];
  perk->id = PERK_BARD_ARIA_OF_STASIS;
  perk->name = strdup("Aria of Stasis");
  perk->description = strdup("Allies gain +4 to all saves and immunity to slow. Enemies suffer -2 to hit and 10% movement penalty");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SPELLSINGER;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_PROTECTIVE_CHORUS;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4; /* ally save bonus */
  perk->effect_modifier = 2; /* enemy tohit penalty */
  perk->special_description = strdup("Capstone: Allies +4 saves/immune to slow; Enemies -2 hit/10% slow");

  /* Symphonic Resonance */
  perk = &perk_list[PERK_BARD_SYMPHONIC_RESONANCE];
  perk->id = PERK_BARD_SYMPHONIC_RESONANCE;
  perk->name = strdup("Symphonic Resonance");
  perk->description = strdup("Each round while performing, gain 1d6 temp HP (max 30 rounds). Enchantment/Illusion spells in songs daze enemies 1 round in 20 ft");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SPELLSINGER;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_CRESCENDO;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 6; /* temp HP d6 */
  perk->effect_modifier = 1; /* daze duration */
  perk->special_description = strdup("Capstone: 1d6 temp HP/round (max 180 total); Enchantment/Illusion spells daze in 20 ft radius");

  /* Endless Refrain */
  perk = &perk_list[PERK_BARD_ENDLESS_REFRAIN];
  perk->id = PERK_BARD_ENDLESS_REFRAIN;
  perk->name = strdup("Endless Refrain");
  perk->description = strdup("Performance costs nothing and regenerates 1 spell slot per round. Songs last indefinitely until stopped");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SPELLSINGER;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_SUSTAINING_MELODY;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* spell slot regen */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Capstone: Performance is free; regenerate 1 spell slot per round; songs last indefinitely");

  /*** WARCHANTER TREE - TIER I ***/

  /* Battle Hymn I */
  perk = &perk_list[PERK_BARD_BATTLE_HYMN_I];
  perk->id = PERK_BARD_BATTLE_HYMN_I;
  perk->name = strdup("Battle Hymn I");
  perk->description = strdup("Inspire Courage also grants +1 competence to damage per rank");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_WARCHANTER;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Inspire Courage grants +1 competence to damage per rank");

  /* Drummer's Rhythm I */
  perk = &perk_list[PERK_BARD_DRUMMERS_RHYTHM_I];
  perk->id = PERK_BARD_DRUMMERS_RHYTHM_I;
  perk->name = strdup("Drummer's Rhythm I");
  perk->description = strdup("While a song is active, you gain +1 to hit in melee per rank");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_WARCHANTER;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("While performing, gain +1 melee to-hit per rank");

  /* Rallying Cry */
  perk = &perk_list[PERK_BARD_RALLYING_CRY];
  perk->id = PERK_BARD_RALLYING_CRY;
  perk->name = strdup("Rallying Cry");
  perk->description = strdup("Activate to remove the shaken condition from allies and grant +2 morale to saves vs. fear for 5 rounds");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_WARCHANTER;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 5;
  perk->special_description = strdup("Remove shaken condition; grant +2 morale to fear saves for 5 rounds");

  /* Frostbite Refrain I */
  perk = &perk_list[PERK_BARD_FROSTBITE_REFRAIN_I];
  perk->id = PERK_BARD_FROSTBITE_REFRAIN_I;
  perk->name = strdup("Frostbite Refrain I");
  perk->description = strdup("Your melee hits deal +1 cold damage per rank while a song is active; enemies you hit suffer -1 to attack for 1 round on a natural 20");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_WARCHANTER;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = -1;
  perk->special_description = strdup("While performing: melee hits +1 cold damage per rank; nat 20 applies -1 attack debuff for 1 round");

  /*** WARCHANTER TREE - TIER II ***/

  /* Battle Hymn II */
  perk = &perk_list[PERK_BARD_BATTLE_HYMN_II];
  perk->id = PERK_BARD_BATTLE_HYMN_II;
  perk->name = strdup("Battle Hymn II");
  perk->description = strdup("Additional +1 damage granted by Inspire Courage per rank (stacks with Battle Hymn I)");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_WARCHANTER;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_BARD_BATTLE_HYMN_I;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Inspire Courage grants additional +1 competence to damage per rank");

  /* Drummer's Rhythm II */
  perk = &perk_list[PERK_BARD_DRUMMERS_RHYTHM_II];
  perk->id = PERK_BARD_DRUMMERS_RHYTHM_II;
  perk->name = strdup("Drummer's Rhythm II");
  perk->description = strdup("Additional +1 melee to-hit per rank while a martial song is active (stacks with Drummer's Rhythm I)");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_WARCHANTER;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_BARD_DRUMMERS_RHYTHM_I;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("While performing, gain additional +1 melee to-hit per rank");

  /* Warbeat */
  perk = &perk_list[PERK_BARD_WARBEAT];
  perk->id = PERK_BARD_WARBEAT;
  perk->name = strdup("Warbeat");
  perk->description = strdup("On your first turn in combat, make an extra melee attack at your highest bonus; on hit, grant allies +1d4 damage for 2 rounds");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_WARCHANTER;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_RALLYING_CRY;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4;
  perk->effect_modifier = 2;
  perk->special_description = strdup("First turn in combat: extra melee attack; on hit grants allies +1d4 damage for 2 rounds");

  /* Frostbite Refrain II */
  perk = &perk_list[PERK_BARD_FROSTBITE_REFRAIN_II];
  perk->id = PERK_BARD_FROSTBITE_REFRAIN_II;
  perk->name = strdup("Frostbite Refrain II");
  perk->description = strdup("Melee hits deal an additional +1 cold damage per rank; your natural 20 debuff becomes -2 to attack and -1 to AC for 1 round");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_WARCHANTER;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_BARD_FROSTBITE_REFRAIN_I;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = -2;
  perk->special_description = strdup("Melee hits +1 cold damage per rank; nat 20 applies -2 attack and -1 AC debuff for 1 round");

  /*** WARCHANTER TREE - TIER III ***/

  /* Anthem of Fortitude */
  perk = &perk_list[PERK_BARD_ANTHEM_OF_FORTITUDE];
  perk->id = PERK_BARD_ANTHEM_OF_FORTITUDE;
  perk->name = strdup("Anthem of Fortitude");
  perk->description = strdup("Allies under your songs gain +10% max HP and +2 to Fortitude saves");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_WARCHANTER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_BATTLE_HYMN_II;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 10;
  perk->special_description = strdup("While performing: allies gain +10% max HP and +2 to Fortitude saves");

  /* Commanding Cadence */
  perk = &perk_list[PERK_BARD_COMMANDING_CADENCE];
  perk->id = PERK_BARD_COMMANDING_CADENCE;
  perk->name = strdup("Commanding Cadence");
  perk->description = strdup("Enemies you hit in melee must save or be dazed for 1 round (once per target per 5 rounds)");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_WARCHANTER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_WARBEAT;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 5;
  perk->special_description = strdup("On melee hit: enemy must save vs Will or be dazed 1 round (once per target per 5 rounds)");

  /* Steel Serenade */
  perk = &perk_list[PERK_BARD_STEEL_SERENADE];
  perk->id = PERK_BARD_STEEL_SERENADE;
  perk->name = strdup("Steel Serenade");
  perk->description = strdup("While singing, you gain +2 natural AC and 10% physical damage resistance");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_WARCHANTER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_DRUMMERS_RHYTHM_II;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 10;
  perk->special_description = strdup("While performing: +2 natural AC and 10% physical damage resistance");

  /* Banner Verse */
  perk = &perk_list[PERK_BARD_BANNER_VERSE];
  perk->id = PERK_BARD_BANNER_VERSE;
  perk->name = strdup("Banner Verse");
  perk->description = strdup("Plant a musical standard object in the room for 5 rounds; allies in the room gain +2 to hit and +2 to all saves");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_WARCHANTER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_RALLYING_CRY;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 5;
  perk->special_description = strdup("Plant musical banner: allies in room gain +2 to hit and +2 to all saves for 5 rounds");

  /* Warchanter's Dominance - Tier 4 Capstone */
  perk = &perk_list[PERK_BARD_WARCHANTERS_DOMINANCE];
  perk->id = PERK_BARD_WARCHANTERS_DOMINANCE;
  perk->name = strdup("Warchanter's Dominance");
  perk->description = strdup("Inspire Courage now also grants +1 attack and +1 AC; your Warbeat now gives allies an additional +1d4 to damage and +1 to AC");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_WARCHANTER;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_ANTHEM_OF_FORTITUDE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 1;
  perk->special_description = strdup("Capstone: Inspire Courage grants +1 attack/+1 AC; Warbeat grants +1d4 damage/+1 AC to allies");

  /* Winter's War March - Tier 4 Capstone */
  perk = &perk_list[PERK_BARD_WINTERS_WAR_MARCH];
  perk->id = PERK_BARD_WINTERS_WAR_MARCH;
  perk->name = strdup("Winter's War March");
  perk->description = strdup("Perform a devastating martial anthem: deal 4d6 cold damage to all enemies and slow them for 3 rounds (save halves damage and reduces slow to 1 round). Useable at-will.");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_WARCHANTER;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_COMMANDING_CADENCE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4;
  perk->effect_modifier = 3;
  perk->special_description = strdup("Room-wide martial anthem: 4d6 cold damage, enemies slow for 3 rounds (save halves/reduces to 1 round)");

  /*** SWASHBUCKLER TREE - TIER I ***/

  /* Fencer's Footwork I */
  perk = &perk_list[PERK_BARD_FENCERS_FOOTWORK_I];
  perk->id = PERK_BARD_FENCERS_FOOTWORK_I;
  perk->name = strdup("Fencer's Footwork I");
  perk->description = strdup("+1 Dodge AC and +1 Reflex save per rank while wielding a finesse weapon or single one-handed weapon");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SWASHBUCKLER;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 1;
  perk->special_description = strdup("While wielding finesse or single one-handed weapon: +1 Dodge AC and +1 Reflex per rank");

  /* Precise Strike I */
  perk = &perk_list[PERK_BARD_PRECISE_STRIKE_I];
  perk->id = PERK_BARD_PRECISE_STRIKE_I;
  perk->name = strdup("Precise Strike I");
  perk->description = strdup("+1 precision damage per rank with finesse or one-handed piercing/slashing weapons (not multiplied on crits)");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SWASHBUCKLER;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("With finesse or one-handed piercing/slashing: +1 precision damage per rank (not on crit multiplier)");

  /* Riposte Training I */
  perk = &perk_list[PERK_BARD_RIPOSTE_TRAINING_I];
  perk->id = PERK_BARD_RIPOSTE_TRAINING_I;
  perk->name = strdup("Riposte Training I");
  perk->description = strdup("3% chance per rank to make an immediate counterattack after you successfully dodge or parry");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SWASHBUCKLER;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3;
  perk->effect_modifier = 0;
  perk->special_description = strdup("After dodging/parrying: 3% chance per rank to make immediate counterattack");

  /* Flourish */
  perk = &perk_list[PERK_BARD_FLOURISH];
  perk->id = PERK_BARD_FLOURISH;
  perk->name = strdup("Flourish");
  perk->description = strdup("Activate for +2 to hit and +2 AC for 2 rounds; ends if you are knocked prone or grappled. Requires and uses a move action");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SWASHBUCKLER;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 2;
  perk->special_description = strdup("Activate: +2 to hit and +2 AC for 2 rounds (ends if knocked prone/grappled)");

  /*** SWASHBUCKLER TREE - TIER II ***/

  /* Fencer's Footwork II */
  perk = &perk_list[PERK_BARD_FENCERS_FOOTWORK_II];
  perk->id = PERK_BARD_FENCERS_FOOTWORK_II;
  perk->name = strdup("Fencer's Footwork II");
  perk->description = strdup("Additional +1 Dodge AC and +1 Reflex per rank while using a finesse/single weapon");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SWASHBUCKLER;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_BARD_FENCERS_FOOTWORK_I;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 1;
  perk->special_description = strdup("While wielding finesse or single one-handed weapon: +1 Dodge AC and +1 Reflex per rank (stacks with Tier I)");

  /* Precise Strike II */
  perk = &perk_list[PERK_BARD_PRECISE_STRIKE_II];
  perk->id = PERK_BARD_PRECISE_STRIKE_II;
  perk->name = strdup("Precise Strike II");
  perk->description = strdup("Additional +1 precision damage per rank (stacks with Precise Strike I)");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SWASHBUCKLER;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_BARD_PRECISE_STRIKE_I;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("With finesse or one-handed piercing/slashing: +1 precision damage per rank (stacks with Tier I, not on crit multiplier)");

  /* Duelist's Poise */
  perk = &perk_list[PERK_BARD_DUELISTS_POISE];
  perk->id = PERK_BARD_DUELISTS_POISE;
  perk->name = strdup("Duelist's Poise");
  perk->description = strdup("Gain +2 to critical confirmation and +1 critical threat range when using a finesse weapon");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SWASHBUCKLER;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_FLOURISH;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 1;
  perk->special_description = strdup("With finesse weapon: +2 to critical confirmation rolls and +1 critical threat range");

  /* Agile Disengage */
  perk = &perk_list[PERK_BARD_AGILE_DISENGAGE];
  perk->id = PERK_BARD_AGILE_DISENGAGE;
  perk->name = strdup("Agile Disengage");
  perk->description = strdup("On a failed flee attempt, you gain +4 AC for 3 rounds. This bonus ends if you move out of the room you're in");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SWASHBUCKLER;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_FENCERS_FOOTWORK_I;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4;
  perk->effect_modifier = 3;
  perk->special_description = strdup("Failed flee: +4 AC for 3 rounds (ends if you move rooms)");

  /*** SWASHBUCKLER TREE - TIER III ***/

  /* Perfect Tempo */
  perk = &perk_list[PERK_BARD_PERFECT_TEMPO];
  perk->id = PERK_BARD_PERFECT_TEMPO;
  perk->name = strdup("Perfect Tempo");
  perk->description = strdup("If you avoid all melee hits for a full round, your next attack gains +4 to hit and +2d6 precision damage");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SWASHBUCKLER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_DUELISTS_POISE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4;
  perk->effect_modifier = 12;
  perk->special_description = strdup("Avoided all hits this round: +4 to hit and +2d6 precision damage on next attack");

  /* Showstopper */
  perk = &perk_list[PERK_BARD_SHOWSTOPPER];
  perk->id = PERK_BARD_SHOWSTOPPER;
  perk->name = strdup("Showstopper");
  perk->description = strdup("On a confirmed crit, impose -2 to enemy AC and -2 to attack rolls for 2 rounds (once per target per 5 rounds)");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SWASHBUCKLER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_PRECISE_STRIKE_II;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 2;
  perk->special_description = strdup("Critical hit: enemy takes -2 AC and -2 to hit for 2 rounds (once per target/5 min)");

  /* Acrobatic Charge */
  perk = &perk_list[PERK_BARD_ACROBATIC_CHARGE];
  perk->id = PERK_BARD_ACROBATIC_CHARGE;
  perk->name = strdup("Acrobatic Charge");
  perk->description = strdup("You can charge through difficult terrain and around allies; you gain +2 to hit on charges");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SWASHBUCKLER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_AGILE_DISENGAGE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Charge through terrain and allies: +2 to hit on charges");

  /* Feint and Finish */
  perk = &perk_list[PERK_BARD_FEINT_AND_FINISH];
  perk->id = PERK_BARD_FEINT_AND_FINISH;
  perk->name = strdup("Feint and Finish");
  perk->description = strdup("After successfully feinting, your next attack deals +2d6 precision damage and gains +2 to confirm criticals");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SWASHBUCKLER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_RIPOSTE_TRAINING_I;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 12;
  perk->effect_modifier = 2;
  perk->special_description = strdup("After successful feint: +2d6 precision damage and +2 crit confirm on next attack");

  /* Swashbuckler's Supreme Style - TIER 4 CAPSTONE */
  perk = &perk_list[PERK_BARD_SUPREME_STYLE];
  perk->id = PERK_BARD_SUPREME_STYLE;
  perk->name = strdup("Swashbuckler's Supreme Style");
  perk->description = strdup("While wielding a finesse or single one-handed weapon, gain +2 to hit, +2 dodge AC, +2 to crit confirmation, and one additional attack per 3 rounds");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SWASHBUCKLER;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_PERFECT_TEMPO;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 2;
  perk->special_description = strdup("Tier 4 Capstone: +2 to hit, +2 AC, +2 crit confirm, +1 attack per 3 rounds (finesse/1H weapon only)");

  /* Curtain Call - TIER 4 CAPSTONE */
  perk = &perk_list[PERK_BARD_CURTAIN_CALL];
  perk->id = PERK_BARD_CURTAIN_CALL;
  perk->name = strdup("Curtain Call");
  perk->description = strdup("Once per 5 minutes, make a free attack against up to 3 adjacent enemies, each dealing +2d6 precision damage; creatures struck must save or be disoriented for 2 rounds (disadvantage on attacks)");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SWASHBUCKLER;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_SHOWSTOPPER;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 12;
  perk->effect_modifier = 2;
  perk->special_description = strdup("Tier 4 Capstone: Free multi-target attack 1/5min, +2d6 precision damage, targets save vs disoriented");
}

/* Define Alchemist Perks */
void define_alchemist_perks(void)
{
  struct perk_data *perk;

  /*** MUTAGENIST TREE - TIER I ***/

  /* Mutagen I */
  perk = &perk_list[PERK_ALCHEMIST_MUTAGEN_I];
  perk->id = PERK_ALCHEMIST_MUTAGEN_I;
  perk->name = strdup("Mutagen I");
  perk->description = strdup("Your mutagens give +1 to STR, DEX, and CON per rank while active.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_MUTAGENIST;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Applies +1 per rank to STR/DEX/CON only while under your mutagen.");

  /* Hardy Constitution I */
  perk = &perk_list[PERK_ALCHEMIST_HARDY_CONSTITUTION_I];
  perk->id = PERK_ALCHEMIST_HARDY_CONSTITUTION_I;
  perk->name = strdup("Hardy Constitution I");
  perk->description = strdup("When you drink your own mutagen, gain +1 max HP per level per rank while it lasts.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_MUTAGENIST;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Bonus max HP scales with level and persists only during mutagen.");

  /* Alchemical Reflexes */
  perk = &perk_list[PERK_ALCHEMIST_ALCHEMICAL_REFLEXES];
  perk->id = PERK_ALCHEMIST_ALCHEMICAL_REFLEXES;
  perk->name = strdup("Alchemical Reflexes");
  perk->description = strdup("While a mutagen is active: +1 dodge AC and +1 Reflex saves.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_MUTAGENIST;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Applies +1 dodge AC and +1 Reflex while under mutagen.");

  /* Natural Armor */
  perk = &perk_list[PERK_ALCHEMIST_NATURAL_ARMOR];
  perk->id = PERK_ALCHEMIST_NATURAL_ARMOR;
  perk->name = strdup("Natural Armor");
  perk->description = strdup("While a mutagen is active, gain +2 natural armor.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_MUTAGENIST;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Adds +2 natural armor only while under mutagen.");

  /*** MUTAGENIST TREE - TIER II ***/

  /* Mutagen II */
  perk = &perk_list[PERK_ALCHEMIST_MUTAGEN_II];
  perk->id = PERK_ALCHEMIST_MUTAGEN_II;
  perk->name = strdup("Mutagen II");
  perk->description = strdup("Additional +1 to STR/DEX/CON per rank while a mutagen is active.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_MUTAGENIST;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_ALCHEMIST_MUTAGEN_I;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Adds +1 per rank to STR/DEX/CON while under mutagen; stacks with Mutagen I.");

  /* Persistence Mutagen */
  perk = &perk_list[PERK_ALCHEMIST_PERSISTENCE_MUTAGEN];
  perk->id = PERK_ALCHEMIST_PERSISTENCE_MUTAGEN;
  perk->name = strdup("Persistence Mutagen");
  perk->description = strdup("Your mutagens last twice as long.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_MUTAGENIST;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_MUTAGEN_I;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Doubles duration of mutagen and elemental mutagen.");

  /* Infused with Vigor */
  perk = &perk_list[PERK_ALCHEMIST_INFUSED_WITH_VIGOR];
  perk->id = PERK_ALCHEMIST_INFUSED_WITH_VIGOR;
  perk->name = strdup("Infused with Vigor");
  perk->description = strdup("On drinking your mutagen, heal 1d6 + level; also gain fast healing 1 for 10 rounds.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_MUTAGENIST;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_HARDY_CONSTITUTION_I;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Heal on drink: 1d6 + level; Fast Healing 1 for 10 rounds.");

  /* Cellular Adaptation */
  perk = &perk_list[PERK_ALCHEMIST_CELLULAR_ADAPTATION];
  perk->id = PERK_ALCHEMIST_CELLULAR_ADAPTATION;
  perk->name = strdup("Cellular Adaptation");
  perk->description = strdup("While a mutagen is active, gain DR 5/-.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_MUTAGENIST;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_NATURAL_ARMOR;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Grants 5/- damage reduction while mutagen lasts.");

  /*** MUTAGENIST TREE - TIER III ***/

  /* Improved Mutagen */
  perk = &perk_list[PERK_ALCHEMIST_IMPROVED_MUTAGEN];
  perk->id = PERK_ALCHEMIST_IMPROVED_MUTAGEN;
  perk->name = strdup("Improved Mutagen");
  perk->description = strdup("Mutagens grant an additional +4 to the chosen ability.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_MUTAGENIST;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_MUTAGEN_II;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Adds +4 to the primary ability of your mutagen.");

  /* Unstable Mutagen */
  perk = &perk_list[PERK_ALCHEMIST_UNSTABLE_MUTAGEN];
  perk->id = PERK_ALCHEMIST_UNSTABLE_MUTAGEN;
  perk->name = strdup("Unstable Mutagen");
  perk->description = strdup("Toggle: mutagen effects are +50% stronger but may backlash.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_MUTAGENIST;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_CELLULAR_ADAPTATION;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Use a toggle to enable/disable; boosts mutagen effects by 50%.");

  /* Universal Mutagen */
  perk = &perk_list[PERK_ALCHEMIST_UNIVERSAL_MUTAGEN];
  perk->id = PERK_ALCHEMIST_UNIVERSAL_MUTAGEN;
  perk->name = strdup("Universal Mutagen");
  perk->description = strdup("Activate: next mutagen applies highest bonus to all abilities (short duration).");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_MUTAGENIST;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_IMPROVED_MUTAGEN;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Next mutagen: highest ability bonus applies to all; lasts up to ~5 minutes.");

  /* Mutagenic Mastery */
  perk = &perk_list[PERK_ALCHEMIST_MUTAGENIC_MASTERY];
  perk->id = PERK_ALCHEMIST_MUTAGENIC_MASTERY;
  perk->name = strdup("Mutagenic Mastery");
  perk->description = strdup("While mutagen is active, gain +2 to all ability scores.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_MUTAGENIST;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_INFUSED_WITH_VIGOR;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Adds +2 to STR/DEX/CON/INT/WIS/CHA while mutagen lasts.");

  /*** MUTAGENIST TREE - TIER IV CAPSTONES ***/

  /* Perfect Mutagen (Capstone) */
  perk = &perk_list[PERK_ALCHEMIST_PERFECT_MUTAGEN];
  perk->id = PERK_ALCHEMIST_PERFECT_MUTAGEN;
  perk->name = strdup("Perfect Mutagen");
  perk->description = strdup("Mutagens perfected: +4 to chosen ability and +2 to all others; immune to Unstable Mutagen backlash.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_MUTAGENIST;
  perk->cost = 5;
  perk->max_rank = 1;
  /* System supports single prerequisite; enforce Improved Mutagen directly and document both */
  perk->prerequisite_perk = PERK_ALCHEMIST_IMPROVED_MUTAGEN;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Mutagenic Mastery and Improved Mutagen. Grants immunity to Unstable Mutagen backlash; ensures +4 primary and +2 others while mutagen lasts.");

  /* Chimeric Transmutation (Capstone) */
  perk = &perk_list[PERK_ALCHEMIST_CHIMERIC_TRANSMUTATION];
  perk->id = PERK_ALCHEMIST_CHIMERIC_TRANSMUTATION;
  perk->name = strdup("Chimeric Transmutation");
  perk->description = strdup("While under mutagen, unleash a swift-action breath weapon once per combat: 3d6 fire, 3d6 poison, and 3d6 cold.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_MUTAGENIST;
  perk->cost = 5;
  perk->max_rank = 1;
  /* Single prerequisite field; use Universal Mutagen and document both */
  perk->prerequisite_perk = PERK_ALCHEMIST_UNIVERSAL_MUTAGEN;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Universal Mutagen and Unstable Mutagen. Grants a once-per-combat swift breath attack while mutagen is active.");

  /*** BOMB CRAFTSMAN TREE - TIER I ***/

  /* Alchemical Bomb I */
  perk = &perk_list[PERK_ALCHEMIST_ALCHEMICAL_BOMB_I];
  perk->id = PERK_ALCHEMIST_ALCHEMICAL_BOMB_I;
  perk->name = strdup("Alchemical Bomb I");
  perk->description = strdup("Your direct bomb damage increases by +3 per rank.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_BOMB_CRAFTSMAN;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Adds +3 bomb damage per rank to the direct hit component.");

  /* Precise Bombs (perk) */
  perk = &perk_list[PERK_ALCHEMIST_PRECISE_BOMBS_PERK];
  perk->id = PERK_ALCHEMIST_PRECISE_BOMBS_PERK;
  perk->name = strdup("Precise Bombs");
  perk->description = strdup("Gain +3 to hit on bomb ranged touch attacks.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_BOMB_CRAFTSMAN;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Adds +3 to ranged touch attack rolls with bombs.");

  /* Splash Damage */
  perk = &perk_list[PERK_ALCHEMIST_SPLASH_DAMAGE];
  perk->id = PERK_ALCHEMIST_SPLASH_DAMAGE;
  perk->name = strdup("Splash Damage");
  perk->description = strdup("Splash damage from bombs increases by +3 and bomb save DCs increase by +2 per rank.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_BOMB_CRAFTSMAN;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3;
  perk->effect_modifier = 2;
  perk->special_description = strdup("Adds +3 splash damage and +2 save DC per rank to bomb effects.");

  /* Quick Bomb */
  perk = &perk_list[PERK_ALCHEMIST_QUICK_BOMB];
  perk->id = PERK_ALCHEMIST_QUICK_BOMB;
  perk->name = strdup("Quick Bomb");
  perk->description = strdup("10% chance to throw a bomb as a swift action instead of the usual action.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_BOMB_CRAFTSMAN;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10;
  perk->effect_modifier = 0;
  perk->special_description = strdup("10% proc: bomb throw uses a swift action if available.");

  /*** BOMB CRAFTSMAN TREE - TIER II ***/

  /* Alchemical Bomb II */
  perk = &perk_list[PERK_ALCHEMIST_ALCHEMICAL_BOMB_II];
  perk->id = PERK_ALCHEMIST_ALCHEMICAL_BOMB_II;
  perk->name = strdup("Alchemical Bomb II");
  perk->description = strdup("Your bombs deal an additional +3 damage per rank.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_BOMB_CRAFTSMAN;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_ALCHEMIST_ALCHEMICAL_BOMB_I;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Adds +3 damage per rank on top of Alchemical Bomb I.");

  /* Elemental Bomb */
  perk = &perk_list[PERK_ALCHEMIST_ELEMENTAL_BOMB];
  perk->id = PERK_ALCHEMIST_ELEMENTAL_BOMB;
  perk->name = strdup("Elemental Bomb");
  perk->description = strdup("Elemental bombs bypass 10 resistance and deal an extra 1d6 damage.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_BOMB_CRAFTSMAN;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_ALCHEMICAL_BOMB_I;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10; /* resistance bypass */
  perk->effect_modifier = 6; /* extra damage die size marker */
  perk->special_description = strdup("Fire/cold/acid/electric bombs ignore 10 resistance and add 1d6 damage.");

  /* Concussive Bomb */
  perk = &perk_list[PERK_ALCHEMIST_CONCUSSIVE_BOMB];
  perk->id = PERK_ALCHEMIST_CONCUSSIVE_BOMB;
  perk->name = strdup("Concussive Bomb");
  perk->description = strdup("10% chance on hit to knock targets prone; cannot be reversed on you.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_BOMB_CRAFTSMAN;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_SPLASH_DAMAGE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10; /* proc chance */
  perk->effect_modifier = 0;
  perk->special_description = strdup("On hit: 10% chance to knock targets prone without backlash.");

  /* Poison Bomb */
  perk = &perk_list[PERK_ALCHEMIST_POISON_BOMB];
  perk->id = PERK_ALCHEMIST_POISON_BOMB;
  perk->name = strdup("Poison Bomb");
  perk->description = strdup("10% chance on hit to poison targets.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_BOMB_CRAFTSMAN;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_ALCHEMICAL_BOMB_I;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10; /* proc chance */
  perk->effect_modifier = 0;
  perk->special_description = strdup("On hit: 10% chance to inflict poison.");

  /*** BOMB CRAFTSMAN TREE - TIER III ***/

  /* Inferno Bomb */
  perk = &perk_list[PERK_ALCHEMIST_INFERNO_BOMB];
  perk->id = PERK_ALCHEMIST_INFERNO_BOMB;
  perk->name = strdup("Inferno Bomb");
  perk->description = strdup("Your bombs have a 10% chance to deal +2d6 fire damage.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_BOMB_CRAFTSMAN;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_ELEMENTAL_BOMB;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10; /* proc chance */
  perk->effect_modifier = 2; /* 2d6 bonus damage */
  perk->special_description = strdup("10% chance for bombs to deal an additional 2d6 fire damage.");

  /* Cluster Bomb */
  perk = &perk_list[PERK_ALCHEMIST_CLUSTER_BOMB];
  perk->id = PERK_ALCHEMIST_CLUSTER_BOMB;
  perk->name = strdup("Cluster Bomb");
  perk->description = strdup("Your bombs have a 10% chance to become cluster bombs, hitting 3 times at 75% damage each.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_BOMB_CRAFTSMAN;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_ALCHEMICAL_BOMB_II;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10; /* proc chance */
  perk->effect_modifier = 75; /* damage multiplier per hit */
  perk->special_description = strdup("10% chance for bombs to fragment: 3 hits at 75% damage each.");

  /* Calculated Throw */
  perk = &perk_list[PERK_ALCHEMIST_CALCULATED_THROW];
  perk->id = PERK_ALCHEMIST_CALCULATED_THROW;
  perk->name = strdup("Calculated Throw");
  perk->description = strdup("Your bombs are extra precise, making DCs to resist their effects +3 higher.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_BOMB_CRAFTSMAN;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_PRECISE_BOMBS_PERK;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3; /* DC bonus */
  perk->effect_modifier = 0;
  perk->special_description = strdup("All bomb save DCs increased by +3.");

  /* Bomb Mastery */
  perk = &perk_list[PERK_ALCHEMIST_BOMB_MASTERY];
  perk->id = PERK_ALCHEMIST_BOMB_MASTERY;
  perk->name = strdup("Bomb Mastery");
  perk->description = strdup("Bombs now deal +2d6 extra damage.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_BOMB_CRAFTSMAN;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_ALCHEMICAL_BOMB_II;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* 2d6 bonus damage */
  perk->effect_modifier = 6; /* die size */
  perk->special_description = strdup("All bombs deal an additional 2d6 damage.");

  /*** BOMB CRAFTSMAN TREE - TIER IV (CAPSTONES) ***/

  /* Bombardier Savant (Capstone) */
  perk = &perk_list[PERK_ALCHEMIST_BOMBARDIER_SAVANT];
  perk->id = PERK_ALCHEMIST_BOMBARDIER_SAVANT;
  perk->name = strdup("Bombardier Savant");
  perk->description = strdup("Expert bomb thrower. Bombs gain +3 to hit and +6d6 damage. Throw 2 bombs when starting combat.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_BOMB_CRAFTSMAN;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_BOMB_MASTERY;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3; /* +3 to hit */
  perk->effect_modifier = 6; /* +6d6 damage */
  perk->special_description = strdup("Capstone: +3 ranged touch attack, +6d6 damage, dual bomb throw at combat start.");

  /* Volatile Catalyst (Capstone) */
  perk = &perk_list[PERK_ALCHEMIST_VOLATILE_CATALYST];
  perk->id = PERK_ALCHEMIST_VOLATILE_CATALYST;
  perk->name = strdup("Volatile Catalyst");
  perk->description = strdup("Bombs trigger chain reactions. 1% per bomb prepared to throw an additional bomb. Toggleable.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_BOMB_CRAFTSMAN;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_INFERNO_BOMB;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* 1% per bomb */
  perk->effect_modifier = 0;
  perk->toggleable = TRUE;
  perk->special_description = strdup("Capstone: Bombs have 1% chance per bomb prepared to trigger an auto-throw. Requires toggle.");

  /*** EXTRACT MASTER TREE - TIER I ***/

  /* Alchemical Extract I */
  perk = &perk_list[PERK_ALCHEMIST_ALCHEMICAL_EXTRACT_I];
  perk->id = PERK_ALCHEMIST_ALCHEMICAL_EXTRACT_I;
  perk->name = strdup("Alchemical Extract I");
  perk->description = strdup("Your extracts now have a 3% chance per rank to not expend a use.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_EXTRACT_MASTER;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = PERK_UNDEFINED;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3; /* 3% per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Extract bottling: 3% per rank chance extracts don't consume.");

  /* Infusion I */
  perk = &perk_list[PERK_ALCHEMIST_INFUSION_I];
  perk->id = PERK_ALCHEMIST_INFUSION_I;
  perk->name = strdup("Infusion I");
  perk->description = strdup("Your extract saving throw DCs are +1 higher per rank.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_EXTRACT_MASTER;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = PERK_UNDEFINED;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPELL_DC;
  perk->effect_value = 1; /* +1 per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Focused specialization: +1 per rank to extract save DCs.");

  /* Swift Extraction */
  perk = &perk_list[PERK_ALCHEMIST_SWIFT_EXTRACTION];
  perk->id = PERK_ALCHEMIST_SWIFT_EXTRACTION;
  perk->name = strdup("Swift Extraction");
  perk->description = strdup("Extracts take 20% less time to prepare.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_EXTRACT_MASTER;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_UNDEFINED;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 20; /* 20% speed boost */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Crafting speed: Extracts prepared 20% faster.");

  /* Resonant Extract */
  perk = &perk_list[PERK_ALCHEMIST_RESONANT_EXTRACT];
  perk->id = PERK_ALCHEMIST_RESONANT_EXTRACT;
  perk->name = strdup("Resonant Extract");
  perk->description = strdup("Extracts you create gain the resonant property. They have a 5% chance to affect all members in your party.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_EXTRACT_MASTER;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_UNDEFINED;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5; /* 5% chance */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Party synergy: Extracts have 5% chance to affect all party members.");

  /*** EXTRACT MASTER TREE - TIER II ***/

  /* Alchemical Extract II */
  perk = &perk_list[PERK_ALCHEMIST_ALCHEMICAL_EXTRACT_II];
  perk->id = PERK_ALCHEMIST_ALCHEMICAL_EXTRACT_II;
  perk->name = strdup("Alchemical Extract II");
  perk->description = strdup("Additional 3% chance per rank for extracts to not expend a use.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_EXTRACT_MASTER;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_ALCHEMIST_ALCHEMICAL_EXTRACT_I;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3; /* 3% per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Further bottling refinement: +3% per rank non-consumption chance.");

  /* Infusion II */
  perk = &perk_list[PERK_ALCHEMIST_INFUSION_II];
  perk->id = PERK_ALCHEMIST_INFUSION_II;
  perk->name = strdup("Infusion II");
  perk->description = strdup("Your extract saving throw DCs gain another +1 per rank.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_EXTRACT_MASTER;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_ALCHEMIST_INFUSION_I;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPELL_DC;
  perk->effect_value = 1; /* +1 per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Advanced infusion: additional +1 DC per rank.");

  /* Concentrated Essence */
  perk = &perk_list[PERK_ALCHEMIST_CONCENTRATED_ESSENCE];
  perk->id = PERK_ALCHEMIST_CONCENTRATED_ESSENCE;
  perk->name = strdup("Concentrated Essence");
  perk->description = strdup("All extracts have a 20% chance to be empowered when used.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_EXTRACT_MASTER;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_SWIFT_EXTRACTION;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 20; /* 20% chance */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Power concentration: 20% chance to apply Empower to extracts on use.");

  /* Persistent Extraction */
  perk = &perk_list[PERK_ALCHEMIST_PERSISTENT_EXTRACTION];
  perk->id = PERK_ALCHEMIST_PERSISTENT_EXTRACTION;
  perk->name = strdup("Persistent Extraction");
  perk->description = strdup("All extracts have a 20% chance to be extended when used.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_EXTRACT_MASTER;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_RESONANT_EXTRACT;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 20; /* 20% chance */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Duration extension: 20% chance to apply Extend to extracts on use.");

  /*** EXTRACT MASTER TREE - TIER III ***/

  /* Healing Extraction */
  perk = &perk_list[PERK_ALCHEMIST_HEALING_EXTRACTION];
  perk->id = PERK_ALCHEMIST_HEALING_EXTRACTION;
  perk->name = strdup("Healing Extraction");
  perk->description = strdup("All extracts heal you for (level/2) HP when used (minimum 1).");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_EXTRACT_MASTER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_ALCHEMICAL_EXTRACT_II;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Advanced spell bottling: Extract use heals level/2 HP.");

  /* Alchemical Compatibility */
  perk = &perk_list[PERK_ALCHEMIST_ALCHEMICAL_COMPATIBILITY];
  perk->id = PERK_ALCHEMIST_ALCHEMICAL_COMPATIBILITY;
  perk->name = strdup("Alchemical Compatibility");
  perk->description = strdup("Extracts automatically apply to other alchemists in your party when used.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_EXTRACT_MASTER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_CONCENTRATED_ESSENCE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Combination effects: Extracts auto-apply to alchemist allies in party.");

  /* Discovery Extraction */
  perk = &perk_list[PERK_ALCHEMIST_DISCOVERY_EXTRACTION];
  perk->id = PERK_ALCHEMIST_DISCOVERY_EXTRACTION;
  perk->name = strdup("Discovery Extraction");
  perk->description = strdup("10% chance for extracts to grant +1 INT (stacks to +10, 2 min duration, resets on proc).");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_EXTRACT_MASTER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_INFUSION_II;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10; /* 10% chance */
  perk->effect_modifier = 1; /* +1 INT per stack */
  perk->special_description = strdup("Unique effect: 10% proc for stacking INT buff (+1 per proc, max +10, 2 min).");

  /* Master Alchemist */
  perk = &perk_list[PERK_ALCHEMIST_MASTER_ALCHEMIST];
  perk->id = PERK_ALCHEMIST_MASTER_ALCHEMIST;
  perk->name = strdup("Master Alchemist");
  perk->description = strdup("Extracts and bombs have a 10% chance to be maximized.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_EXTRACT_MASTER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_ALCHEMICAL_COMPATIBILITY;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10; /* 10% chance */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Crafting excellence: 10% chance for Maximize on extracts and bombs.");

  /*** EXTRACT MASTER TREE - TIER IV (CAPSTONES) ***/

  /* Eternal Extract (Capstone) */
  perk = &perk_list[PERK_ALCHEMIST_ETERNAL_EXTRACT];
  perk->id = PERK_ALCHEMIST_ETERNAL_EXTRACT;
  perk->name = strdup("Eternal Extract");
  perk->description = strdup("Extracts have a 5% chance to last 1 hour (unless the duration would be higher anyway).");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_EXTRACT_MASTER;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_MASTER_ALCHEMIST;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5; /* 5% chance */
  perk->effect_modifier = 3600; /* 1 hour in seconds */
  perk->special_description = strdup("Ultimate alchemy: Extracts have 5% chance to last 1 hour.");

  /* Quintessential Extraction (Capstone) */
  perk = &perk_list[PERK_ALCHEMIST_QUINTESSENTIAL_EXTRACTION];
  perk->id = PERK_ALCHEMIST_QUINTESSENTIAL_EXTRACTION;
  perk->name = strdup("Quintessential Extraction");
  perk->description = strdup("Using extracts heals 10 HP and increases max HP by 10 for 5 min (stacks to +100 max HP).");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_EXTRACT_MASTER;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_DISCOVERY_EXTRACTION;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10; /* 10 HP heal */
  perk->effect_modifier = 10; /* +10 max HP per stack */
  perk->special_description = strdup("Perfect mastery: Extracts heal 10 HP and grant +10 max HP (5 min, stacks to +100).");
}


/* Alchemist Mutagenist helper implementations */
int get_alchemist_mutagen_i_rank(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  return get_perk_rank(ch, PERK_ALCHEMIST_MUTAGEN_I, CLASS_ALCHEMIST);
}

int get_alchemist_hardy_constitution_hp_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  if (!affected_by_spell(ch, SKILL_MUTAGEN))
    return 0;
  int ranks = get_perk_rank(ch, PERK_ALCHEMIST_HARDY_CONSTITUTION_I, CLASS_ALCHEMIST);
  if (ranks <= 0)
    return 0;
  return GET_LEVEL(ch) * ranks;
}

bool has_alchemist_alchemical_reflexes(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_ALCHEMIST_ALCHEMICAL_REFLEXES) && affected_by_spell(ch, SKILL_MUTAGEN);
}

bool has_alchemist_natural_armor(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_ALCHEMIST_NATURAL_ARMOR) && affected_by_spell(ch, SKILL_MUTAGEN);
}

bool has_alchemist_improved_mutagen(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_ALCHEMIST_IMPROVED_MUTAGEN);
}

bool is_alchemist_unstable_mutagen_on(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_ALCHEMIST_UNSTABLE_MUTAGEN) && is_perk_toggled_on(ch, PERK_ALCHEMIST_UNSTABLE_MUTAGEN);
}

bool is_alchemist_universal_mutagen_ready(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  if (!has_perk(ch, PERK_ALCHEMIST_UNIVERSAL_MUTAGEN))
    return FALSE;
  /* must be toggled and not under cooldown */
  if (!is_perk_toggled_on(ch, PERK_ALCHEMIST_UNIVERSAL_MUTAGEN))
    return FALSE;
  if (char_has_mud_event(ch, eUNIVERSAL_MUTAGEN_COOLDOWN))
    return FALSE;
  return TRUE;
}

int get_alchemist_mutagenic_mastery_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  return has_perk(ch, PERK_ALCHEMIST_MUTAGENIC_MASTERY) ? 2 : 0;
}

int get_alchemist_mutagen_ii_rank(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  return get_perk_rank(ch, PERK_ALCHEMIST_MUTAGEN_II, CLASS_ALCHEMIST);
}

bool has_alchemist_persistence_mutagen(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_ALCHEMIST_PERSISTENCE_MUTAGEN);
}

bool has_alchemist_infused_with_vigor(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_ALCHEMIST_INFUSED_WITH_VIGOR);
}

bool has_alchemist_cellular_adaptation(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_ALCHEMIST_CELLULAR_ADAPTATION) && affected_by_spell(ch, SKILL_MUTAGEN);
}

/* Mutagenist Tier IV helpers (file scope) */
bool has_alchemist_perfect_mutagen(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_ALCHEMIST_PERFECT_MUTAGEN);
}

bool can_use_chimeric_transmutation(struct char_data *ch)
{
  time_t current_time;

  if (!ch || IS_NPC(ch))
    return FALSE;

  /* Must have perk and be under mutagen */
  if (!has_perk(ch, PERK_ALCHEMIST_CHIMERIC_TRANSMUTATION))
    return FALSE;
  if (!affected_by_spell(ch, SKILL_MUTAGEN))
    return FALSE;

  /* Reset flag if 60s have passed since last combat end */
  current_time = time(0);
  if (ch->player_specials->saved.chimeric_breath_last_combat > 0 &&
      (current_time - ch->player_specials->saved.chimeric_breath_last_combat) >= 60)
  {
    ch->player_specials->saved.chimeric_breath_used = FALSE;
  }

  return !ch->player_specials->saved.chimeric_breath_used;
}

void use_chimeric_transmutation(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return;
  ch->player_specials->saved.chimeric_breath_used = TRUE;
}

void update_chimeric_transmutation_combat_end(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return;
  if (!has_perk(ch, PERK_ALCHEMIST_CHIMERIC_TRANSMUTATION))
    return;
  ch->player_specials->saved.chimeric_breath_last_combat = time(0);
}

/* Bomb Craftsman Tier I helpers */
int get_alchemist_bomb_damage_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  int ranks_i = get_perk_rank(ch, PERK_ALCHEMIST_ALCHEMICAL_BOMB_I, CLASS_ALCHEMIST);
  int ranks_ii = get_perk_rank(ch, PERK_ALCHEMIST_ALCHEMICAL_BOMB_II, CLASS_ALCHEMIST);
  return (ranks_i * 3) + (ranks_ii * 3);
}

int get_alchemist_bomb_precision_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  return has_perk(ch, PERK_ALCHEMIST_PRECISE_BOMBS_PERK) ? 3 : 0;
}

int get_alchemist_bomb_splash_damage_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  int ranks = get_perk_rank(ch, PERK_ALCHEMIST_SPLASH_DAMAGE, CLASS_ALCHEMIST);
  return ranks * 3;
}

int get_alchemist_bomb_dc_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  int ranks = get_perk_rank(ch, PERK_ALCHEMIST_SPLASH_DAMAGE, CLASS_ALCHEMIST);
  int calculated_throw = get_alchemist_calculated_throw_dc_bonus(ch);
  return (ranks * 2) + calculated_throw;
}

int get_alchemist_quick_bomb_chance(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  return has_perk(ch, PERK_ALCHEMIST_QUICK_BOMB) ? 10 : 0;
}

/* Bomb Craftsman Tier II helpers */
int get_alchemist_bomb_damage_bonus_tier2(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  int ranks_ii = get_perk_rank(ch, PERK_ALCHEMIST_ALCHEMICAL_BOMB_II, CLASS_ALCHEMIST);
  return ranks_ii * 3;
}

bool has_alchemist_elemental_bomb(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_ALCHEMIST_ELEMENTAL_BOMB);
}

static bool is_elemental_damage_type(int dam_type)
{
  switch (dam_type)
  {
  case DAM_FIRE:
  case DAM_COLD:
  case DAM_ACID:
  case DAM_ELECTRIC:
    return TRUE;
  default:
    return FALSE;
  }
}

int get_alchemist_elemental_bomb_bypass(struct char_data *ch, int dam_type)
{
  if (!has_alchemist_elemental_bomb(ch))
    return 0;
  return is_elemental_damage_type(dam_type) ? 10 : 0;
}

int get_alchemist_elemental_bomb_extra_damage(struct char_data *ch, int dam_type)
{
  if (!has_alchemist_elemental_bomb(ch))
    return 0;
  return is_elemental_damage_type(dam_type) ? dice(1, 6) : 0;
}

bool has_alchemist_concussive_bomb(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_ALCHEMIST_CONCUSSIVE_BOMB);
}

bool has_alchemist_poison_bomb(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_ALCHEMIST_POISON_BOMB);
}

/* Bomb Craftsman Tier III helpers */
bool has_alchemist_inferno_bomb(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_ALCHEMIST_INFERNO_BOMB);
}

bool has_alchemist_cluster_bomb(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_ALCHEMIST_CLUSTER_BOMB);
}

int get_alchemist_calculated_throw_dc_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  return has_perk(ch, PERK_ALCHEMIST_CALCULATED_THROW) ? 3 : 0;
}

bool has_alchemist_bomb_mastery(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_ALCHEMIST_BOMB_MASTERY);
}

int get_alchemist_bomb_mastery_damage_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  return has_perk(ch, PERK_ALCHEMIST_BOMB_MASTERY) ? dice(2, 6) : 0;
}

/* Bomb Craftsman Tier IV helpers */
bool has_alchemist_bombardier_savant(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_ALCHEMIST_BOMBARDIER_SAVANT);
}

int get_bombardier_savant_attack_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  return has_perk(ch, PERK_ALCHEMIST_BOMBARDIER_SAVANT) ? 3 : 0;
}

int get_bombardier_savant_damage_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  return has_perk(ch, PERK_ALCHEMIST_BOMBARDIER_SAVANT) ? dice(6, 6) : 0;
}

bool has_alchemist_volatile_catalyst(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_ALCHEMIST_VOLATILE_CATALYST);
}

bool is_volatile_catalyst_on(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_ALCHEMIST_VOLATILE_CATALYST) && is_perk_toggled_on(ch, PERK_ALCHEMIST_VOLATILE_CATALYST);
}

/* Extract Master Tier I helpers */
int get_alchemist_extract_i_rank(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  return get_perk_rank(ch, PERK_ALCHEMIST_ALCHEMICAL_EXTRACT_I, CLASS_ALCHEMIST);
}

int get_alchemist_extract_ii_rank(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  return get_perk_rank(ch, PERK_ALCHEMIST_ALCHEMICAL_EXTRACT_II, CLASS_ALCHEMIST);
}

int get_alchemist_extract_not_consumed_chance(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  int rank_i = get_alchemist_extract_i_rank(ch);
  int rank_ii = get_alchemist_extract_ii_rank(ch);
  int total_rank = rank_i + rank_ii;
  if (total_rank <= 0)
    return 0;
  return total_rank * 3; /* 3% per rank across both tiers */
}

int get_alchemist_infusion_i_rank(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  return get_perk_rank(ch, PERK_ALCHEMIST_INFUSION_I, CLASS_ALCHEMIST);
}

int get_alchemist_infusion_ii_rank(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  return get_perk_rank(ch, PERK_ALCHEMIST_INFUSION_II, CLASS_ALCHEMIST);
}

int get_alchemist_infusion_dc_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  int total = get_alchemist_infusion_i_rank(ch) + get_alchemist_infusion_ii_rank(ch);
  if (total <= 0)
    return 0;
  return total; /* +1 per rank across both tiers */
}

bool has_alchemist_swift_extraction(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_ALCHEMIST_SWIFT_EXTRACTION);
}

bool has_alchemist_resonant_extract(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_ALCHEMIST_RESONANT_EXTRACT);
}

bool has_alchemist_concentrated_essence(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_ALCHEMIST_CONCENTRATED_ESSENCE);
}

bool has_alchemist_persistent_extraction(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_ALCHEMIST_PERSISTENT_EXTRACTION);
}

/* Extract Master Tier III helpers */
bool has_alchemist_healing_extraction(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_ALCHEMIST_HEALING_EXTRACTION);
}

int get_alchemist_healing_extraction_amount(struct char_data *ch)
{
  if (!ch || IS_NPC(ch) || !has_alchemist_healing_extraction(ch))
    return 0;
  return MAX(1, GET_LEVEL(ch) / 2);
}

bool has_alchemist_alchemical_compatibility(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_ALCHEMIST_ALCHEMICAL_COMPATIBILITY);
}

bool has_alchemist_discovery_extraction(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_ALCHEMIST_DISCOVERY_EXTRACTION);
}

bool has_alchemist_master_alchemist(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_ALCHEMIST_MASTER_ALCHEMIST);
}

/* Extract Master Tier IV helpers */
bool has_alchemist_eternal_extract(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_ALCHEMIST_ETERNAL_EXTRACT);
}

bool has_alchemist_quintessential_extraction(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_ALCHEMIST_QUINTESSENTIAL_EXTRACTION);
}

/* Define Barbarian Perks */
/* Psionicist Telepathic Control Tier I and II helpers */
int get_psionic_telepathy_dc_bonus(struct char_data *ch)
{
  int bonus = 0;
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Tier I: +1 DC */
  if (has_perk(ch, PERK_PSIONICIST_MIND_SPIKE_I))
    bonus += 1;
  
  /* Tier II: additional +1 DC (total +2) */
  if (has_perk(ch, PERK_PSIONICIST_MIND_SPIKE_II))
    bonus += 1;

  /* Tier III: additional +1 DC (total +3) */
  if (has_perk(ch, PERK_PSIONICIST_DOMINION))
    bonus += 1;
  
  return bonus;
}

int get_psionic_telepathy_penetration_bonus(struct char_data *ch)
{
  int bonus = 0;
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Tier I: +1 manifester level */
  if (has_perk(ch, PERK_PSIONICIST_PSIONIC_DISRUPTOR_I))
    bonus += 1;
  
  /* Tier II: additional +1 manifester level (total +2) */
  if (has_perk(ch, PERK_PSIONICIST_PSIONIC_DISRUPTOR_II))
    bonus += 1;
  
  return bonus;
}

bool has_psionic_suggestion_primer(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PSIONICIST_SUGGESTION_PRIMER);
}

bool has_psionic_focus_channeling(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PSIONICIST_FOCUS_CHANNELING);
}

/* Apply Suggestion Primer duration extension
 * Extends target's affect for matching Telepathy power by +1 round (6 ticks)
 */
void apply_psionic_suggestion_primer(struct char_data *ch, struct char_data *vict, int spellnum, int routines_flags)
{
  if (!ch || !vict)
    return;
  if (!has_psionic_suggestion_primer(ch))
    return;
  if (!is_spellnum_psionic(spellnum))
    return;
  if (psionic_powers[spellnum].power_type != TELEPATHY)
    return;
  if (!IS_SET(routines_flags, MAG_AFFECTS))
    return;

  /* Optional: limit duration extension to non-boss targets.
   * Heuristic similar to banishment safeguards: many bosses are uncharmable
   * and have very high max HP. If victim matches that profile, skip. */
  if (IS_NPC(vict) && (MOB_FLAGGED(vict, MOB_NOCHARM) || GET_MAX_HIT(vict) >= 1000))
    return;

  struct affected_type *hjp = NULL;
  for (hjp = vict->affected; hjp; hjp = hjp->next)
  {
    if (hjp->spell == spellnum)
    {
      hjp->duration += 6;
      break;
    }
  }
}

/* Apply Focus Channeling PSP regain with 1/round throttle */
void apply_psionic_focus_channeling(struct char_data *ch)
{
  if (!ch)
    return;
  if (!has_psionic_focus_channeling(ch))
    return;
  if (affected_by_spell(ch, PERK_PSIONICIST_FOCUS_CHANNELING))
    return;

  int before = GET_PSP(ch);
  int after = MIN(GET_MAX_PSP(ch), before + 1);
  GET_PSP(ch) = after;
  if (after > before)
    send_to_char(ch, "\tGYou channel focus and regain 1 PSP.\tn\r\n");

  struct affected_type af; new_affect(&af);
  af.spell = PERK_PSIONICIST_FOCUS_CHANNELING;
  af.duration = 6; /* ~1 round */
  affect_to_char(ch, &af);
}

/* Tier II Telepathic Control Helpers */

/* Check if character has Mind Spike II and meets augment threshold */
bool has_mind_spike_ii_bonus(struct char_data *ch, int augment_spent)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  if (!has_perk(ch, PERK_PSIONICIST_MIND_SPIKE_II))
    return FALSE;
  return augment_spent >= 2;
}

/* Check if character has Overwhelm perk */
bool has_overwhelm(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PSIONICIST_OVERWHELM);
}

/* Check if Overwhelm has been used this combat (track via affect) */
bool overwhelm_used_this_combat(struct char_data *ch)
{
  if (!has_overwhelm(ch))
    return FALSE;
  return affected_by_spell(ch, PERK_PSIONICIST_OVERWHELM);
}

/* Set Overwhelm cooldown for this combat */
void set_overwhelm_cooldown(struct char_data *ch)
{
  if (!has_overwhelm(ch))
    return;
  
  struct affected_type af;
  new_affect(&af);
  af.spell = PERK_PSIONICIST_OVERWHELM;
  af.duration = 60; /* ~10 minutes for a combat encounter */
  affect_to_char(ch, &af);
}

/* Check if character has Linked Menace perk */
bool has_linked_menace(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PSIONICIST_LINKED_MENACE);
}

/* Apply Linked Menace AC penalty to target */
void apply_linked_menace_ac_penalty(struct char_data *vict)
{
  if (!vict)
    return;
  
  struct affected_type af;
  new_affect(&af);
  af.spell = PERK_PSIONICIST_LINKED_MENACE;
  af.location = APPLY_AC;
  af.modifier = 2; /* -2 AC (note: lower AC is worse, so +2 to the AC value) */
  af.duration = 12; /* 2 rounds */
  affect_to_char(vict, &af);
}

/* Tier III Telepathic Control Helpers */

bool has_psionic_dominion(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PSIONICIST_DOMINION);
}

bool has_psychic_sundering(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PSIONICIST_PSYCHIC_SUNDERING);
}

bool has_psionic_mental_backlash(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PSIONICIST_MENTAL_BACKLASH);
}

bool has_psionic_piercing_will(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PSIONICIST_PIERCING_WILL);
}

/* Extend charm/dominate durations by +2 rounds (12 ticks) for Telepathy powers */
void apply_psionic_dominion_extension(struct char_data *ch, struct char_data *vict, int spellnum,
                                      struct affected_type *af_array, int count)
{
  int i = 0;

  if (!ch || !vict || !af_array)
    return;
  if (!has_psionic_dominion(ch))
    return;
  if (!is_spellnum_psionic(spellnum) || psionic_powers[spellnum].power_type != TELEPATHY)
    return;

  /* Skip bosses / uncharmables */
  if (IS_NPC(vict) && (MOB_FLAGGED(vict, MOB_NOCHARM) || GET_MAX_HIT(vict) >= 1000))
    return;

  for (i = 0; i < count; i++)
  {
    if (af_array[i].location == APPLY_NONE && !af_array[i].bitvector[0] &&
        !af_array[i].bitvector[1] && !af_array[i].bitvector[2] && !af_array[i].bitvector[3])
      continue;

    if (IS_SET_AR(af_array[i].bitvector, AFF_CHARM))
    {
      af_array[i].duration += 12; /* +2 rounds */
    }
  }
}

/* Apply 3-round 10% vulnerability marker for Psychic Sundering */
void apply_psychic_sundering_debuff(struct char_data *ch, struct char_data *vict)
{
  if (!ch || !vict)
    return;
  if (!has_psychic_sundering(ch))
    return;

  struct affected_type af;
  new_affect(&af);
  af.spell = PERK_PSIONICIST_PSYCHIC_SUNDERING;
  af.location = APPLY_NONE;
  af.modifier = 0;
  af.duration = 18; /* 3 rounds */
  affect_join(vict, &af, TRUE, FALSE, TRUE, FALSE);
  send_to_char(vict, "\tRYour mind is torn open, leaving you vulnerable!\tn\r\n");
}

/* +5 PR bypass for Telepathy powers */
int get_psionic_piercing_will_bonus(struct char_data *ch)
{
  if (!has_psionic_piercing_will(ch))
    return 0;
  return 5;
}

/* Chip damage amount for Mental Backlash */
int get_psionic_mental_backlash_damage(struct char_data *ch, int level)
{
  if (!has_psionic_mental_backlash(ch))
    return 0;

  int dmg = 5 + (level / 2);
  return MAX(1, dmg);
}

/* ===== TIER IV CAPSTONE HELPERS ===== */

bool has_psionic_absolute_geas(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PSIONICIST_ABSOLUTE_GEAS);
}

bool has_psionic_hive_commander(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PSIONICIST_HIVE_COMMANDER);
}

/* Apply random debuffs (shaken, fatigued, silenced) from Absolute Geas */
void apply_absolute_geas_debuffs(struct char_data *ch, struct char_data *vict, int level)
{
  struct affected_type af;
  int debuff_flags[3] = {AFF_SHAKEN, AFF_FATIGUED, AFF_SILENCED};
  int i;
  const char *debuff_descs[3] = {
    "You feel shaken by the mental assault!",
    "You feel fatigued by the mental assault!",
    "You are silenced by the mental assault!"
  };

  if (!ch || !vict || !has_psionic_absolute_geas(ch))
    return;

  /* 10% chance roll */
  if (rand_number(1, 100) > 10)
    return;

  /* Will save to negate entire effect */
  if (savingthrow(ch, vict, SAVING_WILL, 0, CAST_SPELL, level, NO_SUBSCHOOL))
    return;

  /* Apply all three debuffs for 3 rounds (180 ticks) */
  for (i = 0; i < 3; i++)
  {
    new_affect(&af);
    af.spell = SPELL_ABSOLUTE_GEAS;
    af.duration = 3;
    af.location = APPLY_NONE;
    af.modifier = 0;
    SET_BIT_AR(af.bitvector, debuff_flags[i]);
    affect_to_char(vict, &af);
  }

  send_to_char(vict, "%s\r\n", debuff_descs[rand_number(0, 2)]);
}

/* Mark a target with Hive Commander bonus: +3 DC to future powers, +2 to hit for allies */
void apply_hive_commander_mark(struct char_data *ch, struct char_data *vict)
{
  struct affected_type af;
  struct follow_type *f;

  if (!ch || !vict || !has_psionic_hive_commander(ch))
    return;

  /* Apply +3 DC marker affect to victim */
  new_affect(&af);
  af.spell = SPELL_HIVE_COMMANDER_MARK;
  af.duration = 3;
  af.location = APPLY_NONE;
  af.modifier = 0;
  SET_BIT_AR(af.bitvector, AFF_HIVE_MARKED);
  affect_to_char(vict, &af);

  send_to_char(vict, "You feel marked by the Hive Commander's psychic link!\r\n");
  send_to_char(ch, "%s is now marked by your psychic link!\r\n", GET_NAME(vict));

  /* Grant +2 to-hit to caster */
  new_affect(&af);
  af.spell = SPELL_HIVE_COMMANDER_MARK;
  af.duration = 3;
  af.location = APPLY_HITROLL;
  af.modifier = 2;
  af.bonus_type = BONUS_TYPE_MORALE;
  affect_to_char(ch, &af);

  /* Grant +2 to-hit to leader (if caster is not leader) */
  if (ch->master)
  {
    new_affect(&af);
    af.spell = SPELL_HIVE_COMMANDER_MARK;
    af.duration = 3;
    af.location = APPLY_HITROLL;
    af.modifier = 2;
    af.bonus_type = BONUS_TYPE_MORALE;
    affect_to_char(ch->master, &af);
  }

  /* Grant to all followers in group */
  for (f = ch->followers; f; f = f->next)
  {
    if (f->follower && f->follower->in_room == ch->in_room && !IS_NPC(f->follower))
    {
      new_affect(&af);
      af.spell = SPELL_HIVE_COMMANDER_MARK;
      af.duration = 3;
      af.location = APPLY_HITROLL;
      af.modifier = 2;
      af.bonus_type = BONUS_TYPE_MORALE;
      affect_to_char(f->follower, &af);
    }
  }

  send_to_char(ch, "\tY[Hive Commander] Your allies are bolstered!\tn\r\n");
}

/* ===== PSYCHOKINETIC ARSENAL TIER I HELPERS ===== */

bool has_kinetic_edge_i(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PSIONICIST_KINETIC_EDGE_I);
}

bool has_force_screen_adept(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PSIONICIST_FORCE_SCREEN_ADEPT);
}

bool has_vector_shove(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PSIONICIST_VECTOR_SHOVE);
}

bool has_energy_specialization(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PSIONICIST_ENERGY_SPECIALIZATION);
}

/* Returns bonus damage dice for Kinetic Edge I (+1 die when augmented >= 1 PSP) */
int get_kinetic_edge_bonus(struct char_data *ch)
{
  if (!has_kinetic_edge_i(ch))
    return 0;
  /* Bonus applies when augmented by >= 1 PSP */
  if (GET_AUGMENT_PSP(ch) >= 1)
    return 1; /* +1 damage die */
  return 0;
}

/* Returns AC bonus for Force Screen Adept (+1 AC) */
int get_force_screen_ac_bonus(struct char_data *ch)
{
  if (!has_force_screen_adept(ch))
    return 0;
  return 1; /* +1 AC */
}

/* Returns duration bonus percentage for Force Screen Adept (+10% duration) */
int get_force_screen_duration_bonus(struct char_data *ch)
{
  if (!has_force_screen_adept(ch))
    return 0;
  return 10; /* +10% duration */
}

/* Returns movement check bonus for Vector Shove (+2) */
int get_vector_shove_movement_bonus(struct char_data *ch)
{
  if (!has_vector_shove(ch))
    return 0;
  return 2; /* +2 to movement check */
}

/* Returns damage bonus (dice) for Vector Shove (+1 die force damage) */
int get_vector_shove_damage_bonus(struct char_data *ch)
{
  if (!has_vector_shove(ch))
    return 0;
  return 1; /* +1 die force damage */
}

/* Returns DC bonus for Energy Specialization based on chosen element */
int get_energy_specialization_dc_bonus(struct char_data *ch, int element)
{
  if (!has_energy_specialization(ch))
    return 0;
  
  /* Energy Specialization grants +1 DC to powers of the chosen element type */
  if (element == GET_PSIONIC_ENERGY_TYPE(ch))
    return 1;
  
  return 0;
}

/* ===== Tier 2 Psychokinetic Arsenal helper implementations ===== */

bool has_kinetic_edge_ii(struct char_data *ch)
{
  return has_perk(ch, PERK_PSIONICIST_KINETIC_EDGE_II);
}

/* Returns additional +1 die when augmented >= 3 PSP (total +2 with Tier I) */
int get_kinetic_edge_ii_bonus(struct char_data *ch)
{
  if (!has_kinetic_edge_ii(ch))
    return 0;
  if (GET_AUGMENT_PSP(ch) >= 3)
    return 1;
  return 0;
}

bool has_deflective_screen(struct char_data *ch)
{
  return has_perk(ch, PERK_PSIONICIST_DEFLECTIVE_SCREEN);
}

int get_deflective_screen_ranged_ac_bonus(struct char_data *ch)
{
  return has_deflective_screen(ch) ? 2 : 0;
}

int get_deflective_screen_reflex_bonus(struct char_data *ch)
{
  return has_deflective_screen(ch) ? 2 : 0;
}

int get_deflective_screen_first_hit_dr(struct char_data *ch)
{
  return has_deflective_screen(ch) ? 5 : 0;
}

bool has_accelerated_manifestation(struct char_data *ch)
{
  return has_perk(ch, PERK_PSIONICIST_ACCELERATED_MANIFEST);
}

bool has_energy_retort_perk(struct char_data *ch)
{
  return has_perk(ch, PERK_PSIONICIST_ENERGY_RETORT_PERK);
}

/* Level-based energy retort bonus damage: simple scaling on psionic level and augment */
int get_energy_retort_bonus_damage(struct char_data *victim)
{
  int lvl = GET_PSIONIC_LEVEL(victim);
  int aug = GET_AUGMENT_PSP(victim);
  int ndice = MAX(1, lvl / 5) + (aug >= 1 ? 1 : 0);
  /* use d6s */
  return dice(ndice, 6);
}

/* ===== Tier 3 Psychokinetic Arsenal helper implementations ===== */

bool has_kinetic_edge_iii(struct char_data *ch)
{
  return has_perk(ch, PERK_PSIONICIST_KINETIC_EDGE_III);
}

/* Returns additional +1 die when has tier III (total +3 with Tier I and II) */
int get_kinetic_edge_iii_bonus(struct char_data *ch)
{
  if (!has_kinetic_edge_iii(ch))
    return 0;
  return 1;
}

/* Returns +2 DC bonus for energy ray and energy push powers */
int get_kinetic_edge_iii_dc_bonus(struct char_data *ch, int spellnum)
{
  if (!has_kinetic_edge_iii(ch))
    return 0;
  
  if (spellnum == PSIONIC_ENERGY_RAY || spellnum == PSIONIC_ENERGY_PUSH)
    return 2;
  
  return 0;
}

bool has_gravity_well(struct char_data *ch)
{
  return has_perk(ch, PERK_PSIONICIST_GRAVITY_WELL);
}

bool can_use_gravity_well(struct char_data *ch)
{
  if (!has_gravity_well(ch))
    return FALSE;
  
  /* Check if already used this combat */
  if (char_has_mud_event(ch, eGRAVITY_WELL_USED))
    return FALSE;
  
  return TRUE;
}

void use_gravity_well(struct char_data *ch)
{
  if (!can_use_gravity_well(ch))
    return;
  
  /* Mark as used for this combat - will clear on combat end */
  attach_mud_event(new_mud_event(eGRAVITY_WELL_USED, ch, NULL), 0);
}

bool has_force_aegis(struct char_data *ch)
{
  return has_perk(ch, PERK_PSIONICIST_FORCE_AEGIS);
}

int get_force_aegis_ranged_ac_bonus(struct char_data *ch)
{
  if (!has_force_aegis(ch))
    return 0;
  
  /* Check if force screen or inertial armor is active */
  if (affected_by_spell(ch, PSIONIC_FORCE_SCREEN) || 
      affected_by_spell(ch, PSIONIC_INERTIAL_ARMOR))
    return 3;
  
  return 0;
}

int get_force_aegis_temp_hp_bonus(struct char_data *ch)
{
  if (!has_force_aegis(ch))
    return 0;
  
  return GET_PSIONIC_LEVEL(ch);
}

bool has_kinetic_crush(struct char_data *ch)
{
  return has_perk(ch, PERK_PSIONICIST_KINETIC_CRUSH);
}

bool should_apply_kinetic_crush_prone(struct char_data *ch)
{
  return has_kinetic_crush(ch);
}

int get_kinetic_crush_collision_damage(struct char_data *ch)
{
  if (!has_kinetic_crush(ch))
    return 0;
  
  return GET_PSIONIC_LEVEL(ch);
}

/* ===== Tier 4 Psychokinetic Arsenal helper implementations ===== */

bool has_singular_impact(struct char_data *ch)
{
  return has_perk(ch, PERK_PSIONICIST_SINGULAR_IMPACT);
}

bool can_use_singular_impact(struct char_data *ch)
{
  if (!has_singular_impact(ch))
    return FALSE;
  
  /* Check if already used today */
  if (char_has_mud_event(ch, eSINGULAR_IMPACT_USED))
    return FALSE;
  
  return TRUE;
}

void use_singular_impact(struct char_data *ch, struct char_data *victim)
{
  int dam;
  struct affected_type af;
  
  if (!can_use_singular_impact(ch))
    return;
  
  /* Calculate heavy force damage: scales with manifester level */
  dam = dice(GET_PSIONIC_LEVEL(ch) / 2, 10) + GET_INT(ch);
  
  send_to_char(ch, "\tWYou channel your psychokinetic power into a singular devastating impact!\tn\r\n");
  act("$n \tWunleashes a devastating psychokinetic blast at $N!\tn", FALSE, ch, 0, victim, TO_NOTVICT);
  act("$n \tWunleashes a devastating psychokinetic blast at you!\tn", FALSE, ch, 0, victim, TO_VICT);
  
  /* Apply damage */
  damage(ch, victim, dam, PSIONIC_ENERGY_RAY, DAM_FORCE, FALSE);
  
  /* Auto bull rush - push back */
  if (victim && GET_POS(victim) > POS_DEAD && !MOB_FLAGGED(victim, MOB_NOBASH))
  {
    send_to_char(victim, "The force of the blast hurls you backwards!\r\n");
    change_position(victim, POS_SITTING);
  }
  
  /* Apply stun on failed Fort save (50% chance or save negates) */
  if (victim && GET_POS(victim) > POS_DEAD)
  {
    if (!savingthrow(ch, victim, SAVING_FORT, 0, CAST_INNATE, GET_PSIONIC_LEVEL(ch), PSYCHOKINESIS))
    {
      new_affect(&af);
      af.spell = PSIONIC_ENERGY_STUN; /* Use existing psionic stun spell */
      af.duration = 10; /* 1 round */
      SET_BIT_AR(af.bitvector, AFF_STUN);
      affect_to_char(victim, &af);
      send_to_char(victim, "You are \tRstunned\tn by the impact!\r\n");
    }
    else
    {
      send_to_char(victim, "You manage to resist the stunning effect!\r\n");
    }
  }
  
  /* Mark as used for the day */
  attach_mud_event(new_mud_event(eSINGULAR_IMPACT_USED, ch, NULL), 24 * 60 * PASSES_PER_SEC); /* 24 hours */
}

bool has_perfect_deflection(struct char_data *ch)
{
  return has_perk(ch, PERK_PSIONICIST_PERFECT_DEFLECTION);
}

bool can_use_perfect_deflection(struct char_data *ch)
{
  if (!has_perfect_deflection(ch))
    return FALSE;
  
  /* Check if already used today */
  if (char_has_mud_event(ch, ePERFECT_DEFLECTION_USED))
    return FALSE;
  
  return TRUE;
}

void use_perfect_deflection(struct char_data *ch)
{
  if (!can_use_perfect_deflection(ch))
    return;
  
  /* The actual reflection logic will be handled in the attack code */
  /* This just marks it as used */
  attach_mud_event(new_mud_event(ePERFECT_DEFLECTION_USED, ch, NULL), 24 * 60 * PASSES_PER_SEC); /* 24 hours */
  
  send_to_char(ch, "\tCYou prepare to deflect the next attack against you!\tn\r\n");
}
/* ===== METACREATIVE GENIUS TIER I HELPERS ===== */

bool has_ectoplasmic_artisan_i(struct char_data *ch)
{
  return has_perk(ch, PERK_PSIONICIST_ECTOPLASMIC_ARTISAN_I);
}

bool has_ectoplasmic_artisan_ii(struct char_data *ch)
{
  return has_perk(ch, PERK_PSIONICIST_ECTOPLASMIC_ARTISAN_II);
}

bool can_use_ectoplasmic_artisan_psp_reduction(struct char_data *ch)
{
  if (!has_ectoplasmic_artisan_i(ch) && !has_ectoplasmic_artisan_ii(ch))
    return FALSE;
  
  /* Check if already used this encounter */
  if (char_has_mud_event(ch, eECTOPLASMIC_ARTISAN_USED))
    return FALSE;
  
  return TRUE;
}

void use_ectoplasmic_artisan_psp_reduction(struct char_data *ch)
{
  if (!can_use_ectoplasmic_artisan_psp_reduction(ch))
    return;
  
  /* Mark as used for this encounter - will clear on combat end */
  attach_mud_event(new_mud_event(eECTOPLASMIC_ARTISAN_USED, ch, NULL), 0);
}

int get_ectoplasmic_artisan_psp_reduction(struct char_data *ch)
{
  if (!can_use_ectoplasmic_artisan_psp_reduction(ch))
    return 0;
  
  /* Tier II increases the reduction to -2 PSP */
  if (has_ectoplasmic_artisan_ii(ch))
    return 2;
  return 1; /* -1 PSP cost */
}

int get_ectoplasmic_artisan_duration_bonus(struct char_data *ch)
{
  if (has_ectoplasmic_artisan_ii(ch))
    return 20; /* +20% duration for Tier II */
  if (has_ectoplasmic_artisan_i(ch))
    return 10; /* +10% duration for Tier I */
  return 0;
}

bool has_shard_volley(struct char_data *ch)
{
  return has_perk(ch, PERK_PSIONICIST_SHARD_VOLLEY);
}

bool should_add_extra_shard_projectile(struct char_data *ch, int augment_psp)
{
  if (!has_shard_volley(ch))
    return FALSE;
  
  /* Requires ≥2 PSP augment */
  if (augment_psp < 2)
    return FALSE;
  
  return TRUE;
}

bool has_hardened_constructs_i(struct char_data *ch)
{
  return has_perk(ch, PERK_PSIONICIST_HARDENED_CONSTRUCTS_I);
}

int get_hardened_constructs_temp_hp(struct char_data *ch)
{
  if (!has_hardened_constructs_i(ch))
    return 0;
  
  return GET_PSIONIC_LEVEL(ch);
}

int get_hardened_constructs_ac_bonus(struct char_data *ch)
{
  if (!has_hardened_constructs_i(ch))
    return 0;
  
  return 1; /* +1 AC */
}

bool has_hardened_constructs_ii(struct char_data *ch)
{
  return has_perk(ch, PERK_PSIONICIST_HARDENED_CONSTRUCTS_II);
}

int get_hardened_constructs_ii_ac_bonus(struct char_data *ch)
{
  if (!has_hardened_constructs_ii(ch))
    return 0;
  return 2; /* +2 AC */
}

int get_hardened_constructs_dr_amount(struct char_data *ch)
{
  if (!has_hardened_constructs_ii(ch))
    return 0;
  return 2; /* DR 2/— */
}

bool has_fabricate_focus(struct char_data *ch)
{
  return has_perk(ch, PERK_PSIONICIST_FABRICATE_FOCUS);
}

int get_fabricate_focus_casting_time_reduction(struct char_data *ch)
{
  if (!has_fabricate_focus(ch))
    return 0;
  
  return 10; /* 10% faster */
}

bool has_shardstorm(struct char_data *ch)
{
  return has_perk(ch, PERK_PSIONICIST_SHARDSTORM);
}

bool has_rapid_manifester(struct char_data *ch)
{
  return has_perk(ch, PERK_PSIONICIST_RAPID_MANIFESTER);
}

bool can_use_rapid_manifester(struct char_data *ch)
{
  if (!has_rapid_manifester(ch))
    return FALSE;
  if (char_has_mud_event(ch, eRAPID_MANIFESTER_USED))
    return FALSE;
  return TRUE;
}

void use_rapid_manifester(struct char_data *ch)
{
  if (!can_use_rapid_manifester(ch))
    return;
  /* Mark as used for this encounter */
  attach_mud_event(new_mud_event(eRAPID_MANIFESTER_USED, ch, NULL), 0);
}

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

/* ============================================================================
 * MONK PERK DEFINITIONS - Path of the Iron Fist (Tier 1-2)
 * ============================================================================ */
void define_monk_perks(void)
{
  struct perk_data *perk;
  
  /* ========== TIER 1 PERKS ========== */
  
  /* Improved Unarmed Strike I */
  perk = &perk_list[PERK_MONK_IMPROVED_UNARMED_STRIKE_I];
  perk->id = PERK_MONK_IMPROVED_UNARMED_STRIKE_I;
  perk->name = strdup("Improved Unarmed Strike I");
  perk->description = strdup("+1 to unarmed damage per rank");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_IRON_BODY;
  perk->cost = 1;
  perk->max_rank = 5;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_UNARMED_DAMAGE;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Path of the Iron Fist - Tier 1");
  
  /* Fists of Iron */
  perk = &perk_list[PERK_MONK_FISTS_OF_IRON];
  perk->id = PERK_MONK_FISTS_OF_IRON;
  perk->name = strdup("Fists of Iron");
  perk->description = strdup("Bypasses 2 points of damage reduction");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_IRON_BODY;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Path of the Iron Fist - Tier 1");
  
  /* Lightning Reflexes I */
  perk = &perk_list[PERK_MONK_LIGHTNING_REFLEXES_I];
  perk->id = PERK_MONK_LIGHTNING_REFLEXES_I;
  perk->name = strdup("Lightning Reflexes I");
  perk->description = strdup("+1 to Reflex saves per rank");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_IRON_BODY;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SAVE;
  perk->effect_value = 1;
  perk->effect_modifier = SAVING_REFL;
  perk->special_description = strdup("Path of the Iron Fist - Tier 1");
  
  /* Sweeping Strike */
  perk = &perk_list[PERK_MONK_SWEEPING_STRIKE];
  perk->id = PERK_MONK_SWEEPING_STRIKE;
  perk->name = strdup("Sweeping Strike");
  perk->description = strdup("Trip attempts gain +2 bonus, can trip as part of flurry");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_IRON_BODY;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Path of the Iron Fist - Tier 1");
  
  /* Meditation Focus I */
  perk = &perk_list[PERK_MONK_MEDITATION_FOCUS_I];
  perk->id = PERK_MONK_MEDITATION_FOCUS_I;
  perk->name = strdup("Meditation Focus I");
  perk->description = strdup("+1 GP regen per rank and 1 extra ki point per rank");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_IRON_BODY;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Path of the Iron Fist - Tier 1");
  
  /* ========== TIER 2 PERKS ========== */
  
  /* Improved Unarmed Strike II */
  perk = &perk_list[PERK_MONK_IMPROVED_UNARMED_STRIKE_II];
  perk->id = PERK_MONK_IMPROVED_UNARMED_STRIKE_II;
  perk->name = strdup("Improved Unarmed Strike II");
  perk->description = strdup("+1 unarmed damage per rank, attacks count as lawful");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_IRON_BODY;
  perk->cost = 2;
  perk->max_rank = 3;
  perk->prerequisite_perk = PERK_MONK_IMPROVED_UNARMED_STRIKE_I;
  perk->prerequisite_rank = 5;
  perk->effect_type = PERK_EFFECT_UNARMED_DAMAGE;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Path of the Iron Fist - Tier 2");
  
  /* Stunning Fist Enhancement */
  perk = &perk_list[PERK_MONK_STUNNING_FIST_ENHANCEMENT];
  perk->id = PERK_MONK_STUNNING_FIST_ENHANCEMENT;
  perk->name = strdup("Stunning Fist Enhancement");
  perk->description = strdup("Stunning Fist DC increases by +2, duration +1 round");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_IRON_BODY;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_IMPROVED_UNARMED_STRIKE_I;
  perk->prerequisite_rank = 3;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 1;
  perk->special_description = strdup("Path of the Iron Fist - Tier 2");
  
  /* Improved Critical: Unarmed */
  perk = &perk_list[PERK_MONK_IMPROVED_CRITICAL_UNARMED];
  perk->id = PERK_MONK_IMPROVED_CRITICAL_UNARMED;
  perk->name = strdup("Improved Critical: Unarmed");
  perk->description = strdup("Critical threat range for unarmed attacks increased (19-20)");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_IRON_BODY;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_IMPROVED_UNARMED_STRIKE_I;
  perk->prerequisite_rank = 5;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 19;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Path of the Iron Fist - Tier 2");
  
  /* Tiger Claw */
  perk = &perk_list[PERK_MONK_TIGER_CLAW];
  perk->id = PERK_MONK_TIGER_CLAW;
  perk->name = strdup("Tiger Claw");
  perk->description = strdup("+2 damage on all unarmed attacks, attacks can cause bleeding (1d4 per round for 3 rounds)");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_IRON_BODY;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_IMPROVED_UNARMED_STRIKE_I;
  perk->prerequisite_rank = 3;
  perk->effect_type = PERK_EFFECT_UNARMED_DAMAGE;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Path of the Iron Fist - Tier 2");
  
  /* One With Wood and Stone */
  perk = &perk_list[PERK_MONK_ONE_WITH_WOOD_AND_STONE];
  perk->id = PERK_MONK_ONE_WITH_WOOD_AND_STONE;
  perk->name = strdup("One With Wood and Stone");
  perk->description = strdup("With a quarterstaff or kama, gain +1 AC, +1 to hit and +1 to damage when using them");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_IRON_BODY;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Path of the Iron Fist - Tier 2");
  
  /* Flurry Focus */
  perk = &perk_list[PERK_MONK_FLURRY_FOCUS];
  perk->id = PERK_MONK_FLURRY_FOCUS;
  perk->name = strdup("Flurry Focus");
  perk->description = strdup("Flurry of Blows penalty reduced by 1, 10% chance for extra flurry attack per round");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_IRON_BODY;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_IMPROVED_UNARMED_STRIKE_I;
  perk->prerequisite_rank = 5;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = -1;
  perk->effect_modifier = 10;
  perk->special_description = strdup("Path of the Iron Fist - Tier 2");
  
  /* ====== TIER 3 PERKS ====== */
  
  /* Improved Unarmed Strike III */
  perk = &perk_list[PERK_MONK_IMPROVED_UNARMED_STRIKE_III];
  perk->id = PERK_MONK_IMPROVED_UNARMED_STRIKE_III;
  perk->name = strdup("Improved Unarmed Strike III");
  perk->description = strdup("+2 unarmed damage per rank, attacks count as adamantine");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_IRON_BODY;
  perk->cost = 3;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_MONK_IMPROVED_UNARMED_STRIKE_II;
  perk->prerequisite_rank = 3;
  perk->effect_type = PERK_EFFECT_UNARMED_DAMAGE;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Path of the Iron Fist - Tier 3");
  
  /* Fists of Fury */
  perk = &perk_list[PERK_MONK_FISTS_OF_FURY];
  perk->id = PERK_MONK_FISTS_OF_FURY;
  perk->name = strdup("Fists of Fury");
  perk->description = strdup("Increases Flurry Focus extra attack chance from 10% to 20%");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_IRON_BODY;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_FLURRY_FOCUS;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 20;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Path of the Iron Fist - Tier 3");
  
  /* Crushing Blow */
  perk = &perk_list[PERK_MONK_CRUSHING_BLOW];
  perk->id = PERK_MONK_CRUSHING_BLOW;
  perk->name = strdup("Crushing Blow");
  perk->description = strdup("Use a ki point to make a crushing blow attack dealing +4d6 damage and ignoring 10 DR");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_IRON_BODY;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_TIGER_CLAW;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Path of the Iron Fist - Tier 3");
  
  /* Improved Critical: Unarmed II */
  perk = &perk_list[PERK_MONK_IMPROVED_CRITICAL_UNARMED_II];
  perk->id = PERK_MONK_IMPROVED_CRITICAL_UNARMED_II;
  perk->name = strdup("Improved Critical: Unarmed II");
  perk->description = strdup("Critical threat range for unarmed attacks increased to 18-20");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_IRON_BODY;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_IMPROVED_CRITICAL_UNARMED;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Path of the Iron Fist - Tier 3");
  
  /* Power Strike */
  perk = &perk_list[PERK_MONK_POWER_STRIKE];
  perk->id = PERK_MONK_POWER_STRIKE;
  perk->name = strdup("Power Strike");
  perk->description = strdup("Take -1 to hit for +3 damage on all unarmed attacks per rank");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_IRON_BODY;
  perk->cost = 3;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_MONK_IMPROVED_UNARMED_STRIKE_II;
  perk->prerequisite_rank = 3;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3;
  perk->effect_modifier = -1;
  perk->special_description = strdup("Path of the Iron Fist - Tier 3");
  
  /* ====== TIER 4 CAPSTONE PERKS ====== */
  
  /* Legendary Fist */
  perk = &perk_list[PERK_MONK_LEGENDARY_FIST];
  perk->id = PERK_MONK_LEGENDARY_FIST;
  perk->name = strdup("Legendary Fist");
  perk->description = strdup("Unarmed strikes deal +2d6 damage and critical multiplier is now x3");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_IRON_BODY;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_IMPROVED_UNARMED_STRIKE_III;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Path of the Iron Fist - Tier 4 Capstone");
  
  /* Shattering Strike */
  perk = &perk_list[PERK_MONK_SHATTERING_STRIKE];
  perk->id = PERK_MONK_SHATTERING_STRIKE;
  perk->name = strdup("Shattering Strike");
  perk->description = strdup("Use a ki point to deal +8d8 damage on next unarmed attack");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_IRON_BODY;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_CRUSHING_BLOW;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Path of the Iron Fist - Tier 4 Capstone");

  /* ===== WAY OF THE SHADOW - TIER 1 ===== */

  /* Shadow Step I */
  perk = &perk_list[PERK_MONK_SHADOW_STEP_I];
  perk->id = PERK_MONK_SHADOW_STEP_I;
  perk->name = strdup("Shadow Step I");
  perk->description = strdup("+5 feet movement speed per rank");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_SHADOW;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Shadow - Tier 1");

  /* Improved Hide I */
  perk = &perk_list[PERK_MONK_IMPROVED_HIDE_I];
  perk->id = PERK_MONK_IMPROVED_HIDE_I;
  perk->name = strdup("Improved Hide I");
  perk->description = strdup("+2 to Stealth skill per rank");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_SHADOW;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = ABILITY_STEALTH;
  perk->special_description = strdup("Way of the Shadow - Tier 1");

  /* Acrobatic Defense */
  perk = &perk_list[PERK_MONK_ACROBATIC_DEFENSE];
  perk->id = PERK_MONK_ACROBATIC_DEFENSE;
  perk->name = strdup("Acrobatic Defense");
  perk->description = strdup("+1 dodge AC bonus, +2 to Acrobatics checks");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_SHADOW;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Shadow - Tier 1");

  /* Deadly Precision I */
  perk = &perk_list[PERK_MONK_DEADLY_PRECISION_I];
  perk->id = PERK_MONK_DEADLY_PRECISION_I;
  perk->name = strdup("Deadly Precision I");
  perk->description = strdup("+1d6 sneak attack damage per rank (or +1d6 if no sneak attack)");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_SHADOW;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Shadow - Tier 1");

  /* ===== WAY OF THE SHADOW - TIER 2 ===== */

  /* Shadow Step II */
  perk = &perk_list[PERK_MONK_SHADOW_STEP_II];
  perk->id = PERK_MONK_SHADOW_STEP_II;
  perk->name = strdup("Shadow Step II");
  perk->description = strdup("+10 feet movement speed per rank");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_SHADOW;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_MONK_SHADOW_STEP_I;
  perk->prerequisite_rank = 3;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Shadow - Tier 2");

  /* Vanishing Technique */
  perk = &perk_list[PERK_MONK_VANISHING_TECHNIQUE];
  perk->id = PERK_MONK_VANISHING_TECHNIQUE;
  perk->name = strdup("Vanishing Technique");
  perk->description = strdup("Spend a ki point to cast invisibility on self");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_SHADOW;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_IMPROVED_HIDE_I;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Shadow - Tier 2");

  /* Deadly Precision II */
  perk = &perk_list[PERK_MONK_DEADLY_PRECISION_II];
  perk->id = PERK_MONK_DEADLY_PRECISION_II;
  perk->name = strdup("Deadly Precision II");
  perk->description = strdup("+1d6 sneak attack damage per rank");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_SHADOW;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_MONK_DEADLY_PRECISION_I;
  perk->prerequisite_rank = 3;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Shadow - Tier 2");

  /* Shadow Clone */
  perk = &perk_list[PERK_MONK_SHADOW_CLONE];
  perk->id = PERK_MONK_SHADOW_CLONE;
  perk->name = strdup("Shadow Clone");
  perk->description = strdup("Spend a ki point to cast mirror image");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_SHADOW;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_IMPROVED_HIDE_I;
  perk->prerequisite_rank = 3;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Shadow - Tier 2");

  /* Pressure Point Strike */
  perk = &perk_list[PERK_MONK_PRESSURE_POINT_STRIKE];
  perk->id = PERK_MONK_PRESSURE_POINT_STRIKE;
  perk->name = strdup("Pressure Point Strike");
  perk->description = strdup("Sneak attacks have 5% chance to stun for 1 round (Fortitude save negates)");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_SHADOW;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_DEADLY_PRECISION_I;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Shadow - Tier 2");

  /* Smoke Bomb */
  perk = &perk_list[PERK_MONK_SMOKE_BOMB];
  perk->id = PERK_MONK_SMOKE_BOMB;
  perk->name = strdup("Smoke Bomb");
  perk->description = strdup("Spend a ki point to cast darkness");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_SHADOW;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_VANISHING_TECHNIQUE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Shadow - Tier 2");

  /* ===== WAY OF THE SHADOW - TIER 3 ===== */

  /* Shadow Step III */
  perk = &perk_list[PERK_MONK_SHADOW_STEP_III];
  perk->id = PERK_MONK_SHADOW_STEP_III;
  perk->name = strdup("Shadow Step III");
  perk->description = strdup("+15 feet movement, can use ki point to cast waterwalk and spider climb");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_SHADOW;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_SHADOW_STEP_II;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 15;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Shadow - Tier 3");

  /* Deadly Precision III */
  perk = &perk_list[PERK_MONK_DEADLY_PRECISION_III];
  perk->id = PERK_MONK_DEADLY_PRECISION_III;
  perk->name = strdup("Deadly Precision III");
  perk->description = strdup("+2d6 sneak attack damage, critical hits with sneak attacks deal +3d6 extra damage");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_SHADOW;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_DEADLY_PRECISION_II;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Shadow - Tier 3");

  /* Assassinate */
  perk = &perk_list[PERK_MONK_ASSASSINATE];
  perk->id = PERK_MONK_ASSASSINATE;
  perk->name = strdup("Assassinate");
  perk->description = strdup("Sneak attacks on stunned or paralyzed foes deal extra 4d6 damage");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_SHADOW;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_PRESSURE_POINT_STRIKE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Shadow - Tier 3");

  /* Shadow Fade */
  perk = &perk_list[PERK_MONK_SHADOW_FADE];
  perk->id = PERK_MONK_SHADOW_FADE;
  perk->name = strdup("Shadow Fade");
  perk->description = strdup("20% concealment miss chance at all times, 50% in dim light or darkness");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_SHADOW;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_SHADOW_CLONE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 20;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Shadow - Tier 3");

  /* Blinding Speed */
  perk = &perk_list[PERK_MONK_BLINDING_SPEED];
  perk->id = PERK_MONK_BLINDING_SPEED;
  perk->name = strdup("Blinding Speed");
  perk->description = strdup("Spend a ki point to cast haste");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_SHADOW;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_SHADOW_STEP_II;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Shadow - Tier 3");

  /* ===== WAY OF THE SHADOW - TIER 4 ===== */

  /* Shadow Master */
  perk = &perk_list[PERK_MONK_SHADOW_MASTER];
  perk->id = PERK_MONK_SHADOW_MASTER;
  perk->name = strdup("Shadow Master");
  perk->description = strdup("Gain Hide in Plain Sight ability, +4d6 sneak attack damage, vanishing technique casts greater invisibility");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_SHADOW;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_SHADOW_FADE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Shadow - Tier 4 Capstone");

  /* Void Strike */
  perk = &perk_list[PERK_MONK_VOID_STRIKE];
  perk->id = PERK_MONK_VOID_STRIKE;
  perk->name = strdup("Void Strike");
  perk->description = strdup("Use ki point to make attack that ignores all DR and deals +8d6 force damage");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_SHADOW;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_ASSASSINATE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 8;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Shadow - Tier 4 Capstone");

  /* ===== TREE 3: WAY OF THE FOUR ELEMENTS ===== */
  
  /* TIER 1 PERKS */
  
  /* Elemental Attunement I */
  perk = &perk_list[PERK_MONK_ELEMENTAL_ATTUNEMENT_I];
  perk->id = PERK_MONK_ELEMENTAL_ATTUNEMENT_I;
  perk->name = strdup("Elemental Attunement I");
  perk->description = strdup("+1 to saves vs. elemental damage and 1 DR vs. elemental damage per rank");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_FOUR_ELEMENTS;
  perk->cost = 1;
  perk->max_rank = 5;
  perk->prerequisite_perk = PERK_UNDEFINED;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SAVE;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Four Elements - Tier 1");

  /* Fangs of the Fire Snake */
  perk = &perk_list[PERK_MONK_FANGS_OF_FIRE_SNAKE];
  perk->id = PERK_MONK_FANGS_OF_FIRE_SNAKE;
  perk->name = strdup("Fangs of the Fire Snake");
  perk->description = strdup("Spend a ki point to gain flaming attacks of +1d6 fire damage on unarmed strikes");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_FOUR_ELEMENTS;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_UNDEFINED;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Four Elements - Tier 1");

  /* Water Whip */
  perk = &perk_list[PERK_MONK_WATER_WHIP];
  perk->id = PERK_MONK_WATER_WHIP;
  perk->name = strdup("Water Whip");
  perk->description = strdup("Spend 1 ki point to deal 4d6 water damage and entangle the enemy");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_FOUR_ELEMENTS;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_UNDEFINED;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Four Elements - Tier 1");

  /* Gong of the Summit */
  perk = &perk_list[PERK_MONK_GONG_OF_SUMMIT];
  perk->id = PERK_MONK_GONG_OF_SUMMIT;
  perk->name = strdup("Gong of the Summit");
  perk->description = strdup("Spend 1 ki point to emit thunder wave dealing 4d6 sound damage and deafening the target");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_FOUR_ELEMENTS;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_UNDEFINED;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Four Elements - Tier 1");

  /* Fist of Unbroken Air */
  perk = &perk_list[PERK_MONK_FIST_OF_UNBROKEN_AIR];
  perk->id = PERK_MONK_FIST_OF_UNBROKEN_AIR;
  perk->name = strdup("Fist of Unbroken Air");
  perk->description = strdup("Spend 1 ki point to unleash (2d6+2)xRank force damage AoE, knocking enemies prone (Reflex negates)");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_FOUR_ELEMENTS;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = PERK_UNDEFINED;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 2;
  perk->special_description = strdup("Way of the Four Elements - Tier 1");

  /* Elemental Resistance I */
  perk = &perk_list[PERK_MONK_ELEMENTAL_RESISTANCE_I];
  perk->id = PERK_MONK_ELEMENTAL_RESISTANCE_I;
  perk->name = strdup("Elemental Resistance I");
  perk->description = strdup("Resistance 5 to fire, cold, lightning, and acid per rank");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_FOUR_ELEMENTS;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = PERK_UNDEFINED;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Four Elements - Tier 1");

  /* TIER 2 PERKS */

  /* Elemental Attunement II */
  perk = &perk_list[PERK_MONK_ELEMENTAL_ATTUNEMENT_II];
  perk->id = PERK_MONK_ELEMENTAL_ATTUNEMENT_II;
  perk->name = strdup("Elemental Attunement II");
  perk->description = strdup("+2 to saves vs elemental damage per rank, +1 ki point per rank");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_FOUR_ELEMENTS;
  perk->cost = 2;
  perk->max_rank = 3;
  perk->prerequisite_perk = PERK_MONK_ELEMENTAL_ATTUNEMENT_I;
  perk->prerequisite_rank = 5;
  perk->effect_type = PERK_EFFECT_SAVE;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Four Elements - Tier 2");

  /* Shape the Flowing River */
  perk = &perk_list[PERK_MONK_SHAPE_FLOWING_RIVER];
  perk->id = PERK_MONK_SHAPE_FLOWING_RIVER;
  perk->name = strdup("Shape the Flowing River");
  perk->description = strdup("Spend 2 ki points to cast wall of water (blocks ranged attacks, slows movement through it)");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_FOUR_ELEMENTS;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_WATER_WHIP;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Four Elements - Tier 2");

  /* Sweeping Cinder Strike */
  perk = &perk_list[PERK_MONK_SWEEPING_CINDER_STRIKE];
  perk->id = PERK_MONK_SWEEPING_CINDER_STRIKE;
  perk->name = strdup("Sweeping Cinder Strike");
  perk->description = strdup("Spend 2 ki points to create fire wave in 15-ft cone (3d6 fire damage, ignites flammables)");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_FOUR_ELEMENTS;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_FANGS_OF_FIRE_SNAKE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Four Elements - Tier 2");

  /* Rush of the Gale Spirits */
  perk = &perk_list[PERK_MONK_RUSH_OF_GALE_SPIRITS];
  perk->id = PERK_MONK_RUSH_OF_GALE_SPIRITS;
  perk->name = strdup("Rush of the Gale Spirits");
  perk->description = strdup("Spend 2 ki points to cast gust of wind (knocks flying creatures down, pushes medium creatures back)");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_FOUR_ELEMENTS;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_GONG_OF_SUMMIT;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Four Elements - Tier 2");

  /* Clench of the North Wind */
  perk = &perk_list[PERK_MONK_CLENCH_NORTH_WIND];
  perk->id = PERK_MONK_CLENCH_NORTH_WIND;
  perk->name = strdup("Clench of the North Wind");
  perk->description = strdup("Spend 1 ki point to strike an enemy dealing 2d6 cold damage. Failed reflex save (DC 10 + monk level/2 + WIS) encases them in ice for 2 rounds (paralyzed, immune to cold, DR 5/-). 1 minute cooldown.");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_FOUR_ELEMENTS;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_FIST_OF_UNBROKEN_AIR;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Four Elements - Tier 2");

  /* Elemental Resistance II */
  perk = &perk_list[PERK_MONK_ELEMENTAL_RESISTANCE_II];
  perk->id = PERK_MONK_ELEMENTAL_RESISTANCE_II;
  perk->name = strdup("Elemental Resistance II");
  perk->description = strdup("Increase all elemental resistances by +5, can change element attunement as free action");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_FOUR_ELEMENTS;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_ELEMENTAL_RESISTANCE_I;
  perk->prerequisite_rank = 3;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Four Elements - Tier 2");

  /* Mist Stance */
  perk = &perk_list[PERK_MONK_MIST_STANCE];
  perk->id = PERK_MONK_MIST_STANCE;
  perk->name = strdup("Mist Stance");
  perk->description = strdup("Spend 1 ki point to become gaseous form (fly speed, immune to nonmagical attacks, can pass through small openings)");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_FOUR_ELEMENTS;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_ELEMENTAL_ATTUNEMENT_I;
  perk->prerequisite_rank = 3;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Four Elements - Tier 2");

  /* Swarming Ice Rabbit */
  perk = &perk_list[PERK_MONK_SWARMING_ICE_RABBIT];
  perk->id = PERK_MONK_SWARMING_ICE_RABBIT;
  perk->name = strdup("Swarming Ice Rabbit");
  perk->description = strdup("Spend 1 ki point to summon spectral ice rabbits for a ranged attack (3d6 cold damage, can target adjacent rooms)");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_FOUR_ELEMENTS;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_FIST_OF_UNBROKEN_AIR;
  perk->prerequisite_rank = 3;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Four Elements - Tier 2");

  /* Flowing River */
  perk = &perk_list[PERK_MONK_FLOWING_RIVER];
  perk->id = PERK_MONK_FLOWING_RIVER;
  perk->name = strdup("Flowing River");
  perk->description = strdup("Spend 1 ki point to create water wave AoE (2d6 water damage, extinguishes fire effects)");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_FOUR_ELEMENTS;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_WATER_WHIP;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Four Elements - Tier 2");

  /* ====== TIER 3 PERKS ====== */

  /* Elemental Attunement III */
  perk = &perk_list[PERK_MONK_ELEMENTAL_ATTUNEMENT_III];
  perk->id = PERK_MONK_ELEMENTAL_ATTUNEMENT_III;
  perk->name = strdup("Elemental Attunement III");
  perk->description = strdup("Gain +4 ki points and immunity to one chosen element per rank");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_FOUR_ELEMENTS;
  perk->cost = 3;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_MONK_ELEMENTAL_ATTUNEMENT_II;
  perk->prerequisite_rank = 3;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Four Elements - Tier 3");

  /* Flames of the Phoenix */
  perk = &perk_list[PERK_MONK_FLAMES_OF_PHOENIX];
  perk->id = PERK_MONK_FLAMES_OF_PHOENIX;
  perk->name = strdup("Flames of the Phoenix");
  perk->description = strdup("Spend 2 ki points to unleash an inferno (AoE 8d6 fire damage, Reflex save for half or be set on fire for 2 rounds)");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_FOUR_ELEMENTS;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_SWEEPING_CINDER_STRIKE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Four Elements - Tier 3");

  /* Wave of Rolling Earth */
  perk = &perk_list[PERK_MONK_WAVE_OF_ROLLING_EARTH];
  perk->id = PERK_MONK_WAVE_OF_ROLLING_EARTH;
  perk->name = strdup("Wave of Rolling Earth");
  perk->description = strdup("Use a ki point with the rollingearth command to create an earthquake (30-ft radius, difficult terrain, knocks prone, 4d6 damage)");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_FOUR_ELEMENTS;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_ELEMENTAL_ATTUNEMENT_II;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Four Elements - Tier 3");

  /* Ride the Wind */
  perk = &perk_list[PERK_MONK_RIDE_THE_WIND];
  perk->id = PERK_MONK_RIDE_THE_WIND;
  perk->name = strdup("Ride the Wind");
  perk->description = strdup("Spend 3 ki points to gain fly speed 60 ft for 10 minutes");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_FOUR_ELEMENTS;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_RUSH_OF_GALE_SPIRITS;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Four Elements - Tier 3");

  /* Eternal Mountain Defense */
  perk = &perk_list[PERK_MONK_ETERNAL_MOUNTAIN_DEFENSE];
  perk->id = PERK_MONK_ETERNAL_MOUNTAIN_DEFENSE;
  perk->name = strdup("Eternal Mountain Defense");
  perk->description = strdup("Spend 1 ki point to gain 5/- DR that absorbs up to 100 HP (doesn't stack with stoneskin/ironskin)");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_FOUR_ELEMENTS;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_ELEMENTAL_RESISTANCE_II;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Four Elements - Tier 3");

  /* Fist of Four Thunders */
  perk = &perk_list[PERK_MONK_FIST_OF_FOUR_THUNDERS];
  perk->id = PERK_MONK_FIST_OF_FOUR_THUNDERS;
  perk->name = strdup("Fist of Four Thunders");
  perk->description = strdup("Spend 3 ki points to unleash 4d6 AoE sound damage, then strike a random enemy with 3d10 lightning each round for 3 rounds");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_FOUR_ELEMENTS;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_GONG_OF_SUMMIT;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Four Elements - Tier 3");

  /* River of Hungry Flame */
  perk = &perk_list[PERK_MONK_RIVER_OF_HUNGRY_FLAME];
  perk->id = PERK_MONK_RIVER_OF_HUNGRY_FLAME;
  perk->name = strdup("River of Hungry Flame");
  perk->description = strdup("Spend 4 ki points to create wall of fire (20-ft line, 5d8 fire damage, burns for 3 rounds)");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_FOUR_ELEMENTS;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_FLAMES_OF_PHOENIX;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Four Elements - Tier 3");

  /* ====== TIER 4 PERKS ====== */

  /* Breath of Winter */
  perk = &perk_list[PERK_MONK_BREATH_OF_WINTER];
  perk->id = PERK_MONK_BREATH_OF_WINTER;
  perk->name = strdup("Breath of Winter");
  perk->description = strdup("Spend 5 ki points to cast cone of cold (60-ft cone, 12d6 cold damage, freezes enemies solid for 1 round on failed save)");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_FOUR_ELEMENTS;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_CLENCH_NORTH_WIND;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Four Elements - Tier 4 Capstone");

  /* Elemental Embodiment */
  perk = &perk_list[PERK_MONK_ELEMENTAL_EMBODIMENT];
  perk->id = PERK_MONK_ELEMENTAL_EMBODIMENT;
  perk->name = strdup("Elemental Embodiment");
  perk->description = strdup("Spend 5 ki points to transform into elemental form for 1 minute (choose fire/water/air/earth: gain elemental traits, immunities, and special attacks)");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_FOUR_ELEMENTS;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_ELEMENTAL_ATTUNEMENT_III;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Four Elements - Tier 4 Capstone");

  /* Avatar of the Elements */
  perk = &perk_list[PERK_MONK_AVATAR_OF_ELEMENTS];
  perk->id = PERK_MONK_AVATAR_OF_ELEMENTS;
  perk->name = strdup("Avatar of the Elements");
  perk->description = strdup("All elemental ki abilities cost -2 ki (minimum 1), deal +2d6 damage, immunity to all elemental damage types");
  perk->associated_class = CLASS_MONK;
  perk->perk_category = PERK_CATEGORY_WAY_OF_THE_FOUR_ELEMENTS;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_MONK_ETERNAL_MOUNTAIN_DEFENSE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Way of the Four Elements - Tier 4 Capstone");
}

/* ============================================================================
 * PALADIN PERKS (900-999)
 * ============================================================================ */
void define_paladin_perks(void)
{
  struct perk_data *perk;
  
  /*** SACRED DEFENDER TREE - TIER 1 PERKS (1 point each) ***/
  
  /* Extra Lay on Hands I (Ranks 1-3) */
  perk = &perk_list[PERK_PALADIN_EXTRA_LAY_ON_HANDS_1];
  perk->id = PERK_PALADIN_EXTRA_LAY_ON_HANDS_1;
  perk->name = strdup("Extra Lay on Hands I");
  perk->description = strdup("+1 use of Lay on Hands per day per rank");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_SACRED_DEFENDER;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 use per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Increase your daily uses of Lay on Hands by +1 per rank. Maximum 3 ranks for +3 uses total.");
  
  /* Shield of Faith I (Ranks 1-3) */
  perk = &perk_list[PERK_PALADIN_SHIELD_OF_FAITH_1];
  perk->id = PERK_PALADIN_SHIELD_OF_FAITH_1;
  perk->name = strdup("Shield of Faith I");
  perk->description = strdup("+1 Deflection AC per rank");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_SACRED_DEFENDER;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_AC;
  perk->effect_value = 1; /* +1 AC per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Divine protection grants +1 Deflection bonus to AC per rank. Maximum 3 ranks for +3 AC total.");
  
  /* Bulwark of Defense (Ranks 1-3) */
  perk = &perk_list[PERK_PALADIN_BULWARK_OF_DEFENSE];
  perk->id = PERK_PALADIN_BULWARK_OF_DEFENSE;
  perk->name = strdup("Bulwark of Defense");
  perk->description = strdup("+1 to all saves per rank when wielding shield");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_SACRED_DEFENDER;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 to saves per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("While wielding a shield, gain +1 bonus to all saving throws per rank. Maximum 3 ranks for +3 to saves total.");
  
  /* Defensive Strike */
  perk = &perk_list[PERK_PALADIN_DEFENSIVE_STRIKE];
  perk->id = PERK_PALADIN_DEFENSIVE_STRIKE;
  perk->name = strdup("Defensive Strike");
  perk->description = strdup("Attack that grants +2 AC on hit for 5 rounds");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_SACRED_DEFENDER;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 AC */
  perk->effect_modifier = 5; /* 5 rounds duration */
  perk->special_description = strdup("Gain 'Defensive Strike' ability - Make an attack that, if it hits, grants you +2 AC for 5 rounds. 2 minute cooldown.");
  
  /*** SACRED DEFENDER TREE - TIER 2 PERKS (2 points each) ***/
  
  /* Extra Lay on Hands II (Ranks 1-2) */
  perk = &perk_list[PERK_PALADIN_EXTRA_LAY_ON_HANDS_2];
  perk->id = PERK_PALADIN_EXTRA_LAY_ON_HANDS_2;
  perk->name = strdup("Extra Lay on Hands II");
  perk->description = strdup("Additional +1 use of Lay on Hands per day per rank");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_SACRED_DEFENDER;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_PALADIN_EXTRA_LAY_ON_HANDS_1;
  perk->prerequisite_rank = 3; /* Must have max ranks of Extra Lay on Hands I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 use per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Extra Lay on Hands I (max). Further increase daily Lay on Hands uses by +1 per rank. Maximum 2 ranks for +2 uses (combined +5 with Tier 1).");
  
  /* Shield of Faith II (Ranks 1-2) */
  perk = &perk_list[PERK_PALADIN_SHIELD_OF_FAITH_2];
  perk->id = PERK_PALADIN_SHIELD_OF_FAITH_2;
  perk->name = strdup("Shield of Faith II");
  perk->description = strdup("Additional +1 Deflection AC per rank");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_SACRED_DEFENDER;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_PALADIN_SHIELD_OF_FAITH_1;
  perk->prerequisite_rank = 3; /* Must have max ranks of Shield of Faith I */
  perk->effect_type = PERK_EFFECT_AC;
  perk->effect_value = 1; /* +1 AC per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Shield of Faith I (max). Further enhance divine protection with +1 Deflection AC per rank. Maximum 2 ranks for +2 AC (combined +5 with Tier 1).");
  
  /* Healing Hands (Ranks 1-3) */
  perk = &perk_list[PERK_PALADIN_HEALING_HANDS];
  perk->id = PERK_PALADIN_HEALING_HANDS;
  perk->name = strdup("Healing Hands");
  perk->description = strdup("Lay on Hands heals +10% more per rank");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_SACRED_DEFENDER;
  perk->cost = 2;
  perk->max_rank = 3;
  perk->prerequisite_perk = PERK_PALADIN_EXTRA_LAY_ON_HANDS_1;
  perk->prerequisite_rank = 2; /* Must have at least 2 ranks of Extra Lay on Hands I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10; /* +10% per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Extra Lay on Hands I (at least 2 ranks). Your Lay on Hands ability heals +10% more per rank. Maximum 3 ranks for +30% healing total.");
  
  /* Shield Guardian */
  perk = &perk_list[PERK_PALADIN_SHIELD_GUARDIAN];
  perk->id = PERK_PALADIN_SHIELD_GUARDIAN;
  perk->name = strdup("Shield Guardian");
  perk->description = strdup("Grouped allies gain +2 AC from your shield");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_SACRED_DEFENDER;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PALADIN_BULWARK_OF_DEFENSE;
  perk->prerequisite_rank = 2; /* Must have at least 2 ranks of Bulwark of Defense */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 AC to allies */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Bulwark of Defense (at least 2 ranks). While wielding a shield, grouped allies in your room gain +2 AC bonus from your protective presence.");

  /* ===== TIER 3 PERKS ===== */
  
  /* Aura of Protection */
  perk = &perk_list[PERK_PALADIN_AURA_OF_PROTECTION];
  perk->id = PERK_PALADIN_AURA_OF_PROTECTION;
  perk->name = strdup("Aura of Protection");
  perk->description = strdup("Your Aura of Courage grants +2 to all saves");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_SACRED_DEFENDER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PALADIN_SHIELD_OF_FAITH_2;
  perk->prerequisite_rank = 2; /* Must have at least 2 ranks of Shield of Faith II */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 to all saves for allies */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Shield of Faith II (at least 2 ranks). Allies within your Aura of Courage radius gain +2 to all saving throws.");

  /* Sanctuary */
  perk = &perk_list[PERK_PALADIN_SANCTUARY];
  perk->id = PERK_PALADIN_SANCTUARY;
  perk->name = strdup("Sanctuary");
  perk->description = strdup("Reduce all incoming damage by 10%");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_SACRED_DEFENDER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PALADIN_SHIELD_GUARDIAN;
  perk->prerequisite_rank = 1; /* Must have Shield Guardian */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10; /* 10% damage reduction */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Shield Guardian. You become a beacon of divine safety, reducing all incoming damage by 10%.");

  /* Merciful Touch */
  perk = &perk_list[PERK_PALADIN_MERCIFUL_TOUCH];
  perk->id = PERK_PALADIN_MERCIFUL_TOUCH;
  perk->name = strdup("Merciful Touch");
  perk->description = strdup("Lay on Hands grants +20 HP for 5 rounds");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_SACRED_DEFENDER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PALADIN_HEALING_HANDS;
  perk->prerequisite_rank = 3; /* Must have all 3 ranks of Healing Hands */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 20; /* +20 current and max HP */
  perk->effect_modifier = 5; /* 5 rounds duration */
  perk->special_description = strdup("Requires Healing Hands (at least 3 ranks). Your Lay on Hands also grants +20 to current and maximum hit points for 5 rounds. Does not stack.");

  /* Bastion of Defense */
  perk = &perk_list[PERK_PALADIN_BASTION_OF_DEFENSE];
  perk->id = PERK_PALADIN_BASTION_OF_DEFENSE;
  perk->name = strdup("Bastion of Defense");
  perk->description = strdup("Gain 'Bastion' ability (swift action defensive buff)");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_SACRED_DEFENDER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PALADIN_BULWARK_OF_DEFENSE;
  perk->prerequisite_rank = 3; /* Must have all 3 ranks of Bulwark of Defense */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 20; /* 20 temp HP */
  perk->effect_modifier = 4; /* +4 AC */
  perk->special_description = strdup("Requires Bulwark of Defense (3 ranks) and Shield Guardian. Gain 'Bastion' ability - Swift action: gain 20 temporary HP, +4 AC, and immunity to knockdown for 5 rounds. 5 minute cooldown.");

  /* ===== TIER 4 PERKS ===== */
  
  /* Aura of Life */
  perk = &perk_list[PERK_PALADIN_AURA_OF_LIFE];
  perk->id = PERK_PALADIN_AURA_OF_LIFE;
  perk->name = strdup("Aura of Life");
  perk->description = strdup("Allies in your aura regenerate HP");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_SACRED_DEFENDER;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PALADIN_AURA_OF_PROTECTION;
  perk->prerequisite_rank = 1; /* Must have Aura of Protection */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* 2 HP per round in combat */
  perk->effect_modifier = 5; /* 5 HP per round out of combat */
  perk->special_description = strdup("Requires Aura of Protection. Your divine presence sustains allies within your aura, regenerating 2 HP per round in combat, 5 HP per round out of combat.");

  /* Cleansing Touch */
  perk = &perk_list[PERK_PALADIN_CLEANSING_TOUCH];
  perk->id = PERK_PALADIN_CLEANSING_TOUCH;
  perk->name = strdup("Cleansing Touch");
  perk->description = strdup("Lay on Hands removes affects and is a swift action");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_SACRED_DEFENDER;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PALADIN_MERCIFUL_TOUCH;
  perk->prerequisite_rank = 1; /* Must have Merciful Touch */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* Can remove 1 negative affect */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Merciful Touch. Your Lay on Hands can remove one negative magical affect and can be used as a swift action.");

  /* Divine Sacrifice */
  perk = &perk_list[PERK_PALADIN_DIVINE_SACRIFICE];
  perk->id = PERK_PALADIN_DIVINE_SACRIFICE;
  perk->name = strdup("Divine Sacrifice");
  perk->description = strdup("Take damage meant for allies");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_SACRED_DEFENDER;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PALADIN_BASTION_OF_DEFENSE;
  perk->prerequisite_rank = 1; /* Must have Bastion of Defense */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->effect_modifier = 10; /* 10 minute cooldown */
  perk->special_description = strdup("Requires Bastion of Defense. When an ally within 30 feet would be reduced below 0 HP, you may take the damage instead. Once per 10 minutes.");

  /* DIVINE CHAMPION TREE - Tier 1 Perks */

  /* Spell Focus I */
  perk = &perk_list[PERK_PALADIN_SPELL_FOCUS_1];
  perk->id = PERK_PALADIN_SPELL_FOCUS_1;
  perk->name = strdup("Spell Focus I");
  perk->description = strdup("More potent divine spells");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_DIVINE_CHAMPION;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPELL_DC;
  perk->effect_value = 1; /* +1 to spell save DCs per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("+1 to spell save DCs per rank (+3 at max).");

  /* Turn Undead Mastery I */
  perk = &perk_list[PERK_PALADIN_TURN_UNDEAD_MASTERY_1];
  perk->id = PERK_PALADIN_TURN_UNDEAD_MASTERY_1;
  perk->name = strdup("Turn Undead Mastery I");
  perk->description = strdup("Enhanced turning");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_DIVINE_CHAMPION;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 HD worth of undead per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Turn Undead affects +2 HD worth of undead per rank (+6 at max).");

  /* Divine Grace */
  perk = &perk_list[PERK_PALADIN_DIVINE_GRACE];
  perk->id = PERK_PALADIN_DIVINE_GRACE;
  perk->name = strdup("Divine Grace");
  perk->description = strdup("Divine favor protects you");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_DIVINE_CHAMPION;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SAVE;
  perk->effect_value = 1; /* +1 to all saving throws per rank */
  perk->effect_modifier = -1; /* Applies to all save types */
  perk->special_description = strdup("+1 to all saving throws per rank (+3 at max).");

  /* Radiant Aura */
  perk = &perk_list[PERK_PALADIN_RADIANT_AURA];
  perk->id = PERK_PALADIN_RADIANT_AURA;
  perk->name = strdup("Radiant Aura");
  perk->description = strdup("Emit holy light");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_DIVINE_CHAMPION;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* 1d4 divine damage */
  perk->effect_modifier = 4; /* d4 die */
  perk->special_description = strdup("Gain 'Radiant Aura' ability - Toggle: emit light, undead within 10 feet take 1d4 divine damage per round, costs 1 move per round active.");

  /* DIVINE CHAMPION TREE - Tier 2 Perks */

  /* Spell Focus II */
  perk = &perk_list[PERK_PALADIN_SPELL_FOCUS_2];
  perk->id = PERK_PALADIN_SPELL_FOCUS_2;
  perk->name = strdup("Spell Focus II");
  perk->description = strdup("Master of divine magic");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_DIVINE_CHAMPION;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_PALADIN_SPELL_FOCUS_1;
  perk->prerequisite_rank = 3; /* Must have Spell Focus I (3 ranks) */
  perk->effect_type = PERK_EFFECT_SPELL_DC;
  perk->effect_value = 1; /* +1 to spell save DCs per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Spell Focus I (3 ranks). Additional +1 to spell save DCs per rank (+2 at max).");

  /* Turn Undead Mastery II */
  perk = &perk_list[PERK_PALADIN_TURN_UNDEAD_MASTERY_2];
  perk->id = PERK_PALADIN_TURN_UNDEAD_MASTERY_2;
  perk->name = strdup("Turn Undead Mastery II");
  perk->description = strdup("Devastating turning");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_DIVINE_CHAMPION;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_PALADIN_TURN_UNDEAD_MASTERY_1;
  perk->prerequisite_rank = 3; /* Must have Turn Undead Mastery I (3 ranks) */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3; /* +3 HD worth per rank */
  perk->effect_modifier = 2; /* +2d6 damage per rank */
  perk->special_description = strdup("Requires Turn Undead Mastery I (3 ranks). Turn Undead affects additional +3 HD worth per rank and deals +2d6 damage per rank.");

  /* Quickened Blessing */
  perk = &perk_list[PERK_PALADIN_QUICKENED_BLESSING];
  perk->id = PERK_PALADIN_QUICKENED_BLESSING;
  perk->name = strdup("Quickened Blessing");
  perk->description = strdup("Swift divine blessings");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_DIVINE_CHAMPION;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PALADIN_SPELL_FOCUS_1;
  perk->prerequisite_rank = 2; /* Must have Spell Focus I (at least 2 ranks) */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* Once per day */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Spell Focus I (at least 2 ranks). Once per day, you may cast Bless, Shield of Faith, or Protection from Evil as a swift action.");

  /* Channel Energy I */
  perk = &perk_list[PERK_PALADIN_CHANNEL_ENERGY_1];
  perk->id = PERK_PALADIN_CHANNEL_ENERGY_1;
  perk->name = strdup("Channel Energy I");
  perk->description = strdup("Channel positive energy");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_DIVINE_CHAMPION;
  perk->cost = 2;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* 1d6 per rank */
  perk->effect_modifier = 6; /* d6 die */
  perk->special_description = strdup("Gain 'Channel Energy' ability - Standard action: heal allies or damage undead in 30 foot radius for 1d6 per rank, 2 uses per day.");

  /* ===== TIER 3 PERKS ===== */

  /* Spell Penetration */
  perk = &perk_list[PERK_PALADIN_SPELL_PENETRATION];
  perk->id = PERK_PALADIN_SPELL_PENETRATION;
  perk->name = strdup("Spell Penetration");
  perk->description = strdup("Pierce spell resistance");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_DIVINE_CHAMPION;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PALADIN_SPELL_FOCUS_2;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4; /* +4 to overcome SR */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Spell Focus II (at least 2 ranks). +4 bonus to overcome spell resistance with paladin spells.");

  /* Destroy Undead */
  perk = &perk_list[PERK_PALADIN_DESTROY_UNDEAD];
  perk->id = PERK_PALADIN_DESTROY_UNDEAD;
  perk->name = strdup("Destroy Undead");
  perk->description = strdup("Turn Undead can destroy");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_DIVINE_CHAMPION;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PALADIN_TURN_UNDEAD_MASTERY_2;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Turn Undead Mastery II (at least 2 ranks). When Turn Undead affects undead with less than half your paladin level in HD, they are destroyed instead of turned.");

  /* Channel Energy II */
  perk = &perk_list[PERK_PALADIN_CHANNEL_ENERGY_2];
  perk->id = PERK_PALADIN_CHANNEL_ENERGY_2;
  perk->name = strdup("Channel Energy II");
  perk->description = strdup("Powerful channeling");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_DIVINE_CHAMPION;
  perk->cost = 3;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_PALADIN_CHANNEL_ENERGY_1;
  perk->prerequisite_rank = 3;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2d6 per rank */
  perk->effect_modifier = 2; /* +2 uses per day per rank */
  perk->special_description = strdup("Requires Channel Energy I (3 ranks). Channel Energy heals/damages +2d6 per rank and gains +2 uses per day per rank.");

  /* Aura of Courage Mastery */
  perk = &perk_list[PERK_PALADIN_AURA_OF_COURAGE_MASTERY];
  perk->id = PERK_PALADIN_AURA_OF_COURAGE_MASTERY;
  perk->name = strdup("Aura of Courage Mastery");
  perk->description = strdup("Enhanced fear immunity");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_DIVINE_CHAMPION;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PALADIN_DIVINE_GRACE;
  perk->prerequisite_rank = 3;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4; /* +4 bonus vs mind-affecting */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Divine Grace (3 ranks). Your Aura of Courage grants immunity to fear and charm effects, and grants +4 bonus vs mind-affecting.");

  /* ===== TIER 4 PERKS ===== */

  /* Mass Cure Wounds */
  perk = &perk_list[PERK_PALADIN_MASS_CURE_WOUNDS];
  perk->id = PERK_PALADIN_MASS_CURE_WOUNDS;
  perk->name = strdup("Mass Cure Wounds");
  perk->description = strdup("Healing burst");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_DIVINE_CHAMPION;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PALADIN_CHANNEL_ENERGY_2;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3; /* 3d8 */
  perk->effect_modifier = 8; /* d8 die */
  perk->special_description = strdup("Requires Channel Energy II (at least 2 ranks). Gain 'Mass Cure Wounds' ability - Standard action: heal all allies within 30 feet for 3d8 + CHA modifier, twice per day.");

  /* Holy Avenger */
  perk = &perk_list[PERK_PALADIN_HOLY_AVENGER];
  perk->id = PERK_PALADIN_HOLY_AVENGER;
  perk->name = strdup("Holy Avenger");
  perk->description = strdup("Ultimate spell and turning synergy");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_DIVINE_CHAMPION;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PALADIN_SPELL_PENETRATION;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4; /* +4 caster level */
  perk->effect_modifier = 2; /* +2 DC */
  perk->special_description = strdup("Requires Spell Penetration and Destroy Undead. When you destroy undead with Turn Undead, your next spell cast within 1 round is cast at +4 caster level and has +2 DC.");

  /* Beacon of Hope */
  perk = &perk_list[PERK_PALADIN_BEACON_OF_HOPE];
  perk->id = PERK_PALADIN_BEACON_OF_HOPE;
  perk->name = strdup("Beacon of Hope");
  perk->description = strdup("Inspire allies with divine presence");
  perk->associated_class = CLASS_PALADIN;
  perk->perk_category = PERK_CATEGORY_DIVINE_CHAMPION;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PALADIN_AURA_OF_COURAGE_MASTERY;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5; /* 5 rounds duration */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Requires Aura of Courage Mastery. Gain 'Beacon of Hope' ability - Standard action: all allies within 30 feet gain advantage on saves, immunity to fear, and maximize healing received for 5 rounds, once per day.");
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

/* Get perk category name */
const char *get_perk_category_name(int perk_category)
{
  /* Bounds check */
  if (perk_category < 0 || perk_category > PERK_CATEGORY_PRIMAL_CHAMPION)
    return "Unknown Category";
  
  return perk_category_names[perk_category];
}

/*****************************************************************************
 * STAGE PROGRESSION FUNCTIONS (Step 3)
 * These functions handle the stage-based leveling system where each level
 * is divided into 4 stages, each requiring 25% of the total level XP.
 *****************************************************************************/

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
  
  /* If perk system is disabled, don't process stages */
  if (!CONFIG_PERK_SYSTEM)
  {
    if (perk_points_awarded)
      *perk_points_awarded = 0;
    return FALSE;
  }
  
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

  if (GET_LEVEL(ch) >= LVL_IMMORT && PRF_FLAGGED(ch, PRF_HOLYLIGHT))
    return TRUE;
  
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
  
  /* Special prerequisite check for Coordinated Attack - requires Pack Tactics I at max AND Feral Charge */
  if (perk_id == PERK_RANGER_COORDINATED_ATTACK)
  {
    int feral_charge_rank = get_perk_rank(ch, PERK_RANGER_FERAL_CHARGE, class_id);
    if (feral_charge_rank < 1)
    {
      if (error_msg)
        snprintf(error_msg, error_len, "You must have Feral Charge to purchase Coordinated Attack.");
      return FALSE;
    }
  }

  /* Special prerequisite check for Cluster Bomb - requires Alchemical Bomb II (1 rank) AND Precise Bombs */
  if (perk_id == PERK_ALCHEMIST_CLUSTER_BOMB)
  {
    int precise_bombs_rank = get_perk_rank(ch, PERK_ALCHEMIST_PRECISE_BOMBS_PERK, class_id);
    if (precise_bombs_rank < 1)
    {
      if (error_msg)
        snprintf(error_msg, error_len, "You must have Precise Bombs to purchase Cluster Bomb.");
      return FALSE;
    }
  }

  /* Special prerequisite check for Calculated Throw - requires Precise Bombs AND Quick Bomb */
  if (perk_id == PERK_ALCHEMIST_CALCULATED_THROW)
  {
    int quick_bomb_rank = get_perk_rank(ch, PERK_ALCHEMIST_QUICK_BOMB, class_id);
    if (quick_bomb_rank < 1)
    {
      if (error_msg)
        snprintf(error_msg, error_len, "You must have Quick Bomb to purchase Calculated Throw.");
      return FALSE;
    }
  }

  /* Special prerequisite check for Bomb Mastery - requires Alchemical Bomb II (any) AND (Concussive Bomb OR Poison Bomb) */
  if (perk_id == PERK_ALCHEMIST_BOMB_MASTERY)
  {
    int concussive_rank = get_perk_rank(ch, PERK_ALCHEMIST_CONCUSSIVE_BOMB, class_id);
    int poison_rank = get_perk_rank(ch, PERK_ALCHEMIST_POISON_BOMB, class_id);
    if (concussive_rank < 1 && poison_rank < 1)
    {
      if (error_msg)
        snprintf(error_msg, error_len, "You must have either Concussive Bomb or Poison Bomb to purchase Bomb Mastery.");
      return FALSE;
    }
  }

  /* Special prerequisite check for Bombardier Savant - requires Bomb Mastery AND Calculated Throw */
  if (perk_id == PERK_ALCHEMIST_BOMBARDIER_SAVANT)
  {
    int calculated_throw_rank = get_perk_rank(ch, PERK_ALCHEMIST_CALCULATED_THROW, class_id);
    if (calculated_throw_rank < 1)
    {
      if (error_msg)
        snprintf(error_msg, error_len, "You must have Calculated Throw to purchase Bombardier Savant.");
      return FALSE;
    }
  }

  /* Special prerequisite check for Volatile Catalyst - requires Inferno Bomb AND Cluster Bomb */
  if (perk_id == PERK_ALCHEMIST_VOLATILE_CATALYST)
  {
    int cluster_bomb_rank = get_perk_rank(ch, PERK_ALCHEMIST_CLUSTER_BOMB, class_id);
    if (cluster_bomb_rank < 1)
    {
      if (error_msg)
        snprintf(error_msg, error_len, "You must have Cluster Bomb to purchase Volatile Catalyst.");
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
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Get all HP bonuses from perks */
  bonus += get_perk_bonus(ch, PERK_EFFECT_HP, -1);
  
  /* Wilderness Warrior: Ranger Toughness I */
  bonus += get_ranger_toughness_hp(ch);

  /* Alchemist Mutagenist: Hardy Constitution I (only while mutagen is active) */
  bonus += get_alchemist_hardy_constitution_hp_bonus(ch);
  
  return bonus;
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
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
    
  /* If wielded is NULL, character is unarmed - apply bonus */
  if (!wielded)
    bonus = get_perk_bonus(ch, PERK_EFFECT_WEAPON_DAMAGE, -1);
  else
  {
    /* Check if the weapon is ranged - if so, don't apply bonus */
    if (GET_OBJ_TYPE(wielded) == ITEM_WEAPON || GET_OBJ_TYPE(wielded) == ITEM_FIREWEAPON)
    {
      int weapon_type = GET_OBJ_VAL(wielded, 0);
      if (IS_SET(weapon_list[weapon_type].weaponFlags, WEAPON_FLAG_RANGED))
        return 0; /* No bonus for ranged weapons */
    }
    
    /* Weapon is melee, apply bonus */
    bonus = get_perk_bonus(ch, PERK_EFFECT_WEAPON_DAMAGE, -1);
  }
  
  /* Add Bard Warchanter perks */
  /* Battle Hymn I & II: +1 damage per rank for Inspire Courage recipients */
  bonus += get_bard_battle_hymn_damage_bonus(ch);
  bonus += get_bard_battle_hymn_ii_damage_bonus(ch);
  
  /* Frostbite Refrain I & II: +1 cold damage per rank while performing */
  bonus += get_bard_frostbite_cold_damage(ch);
  bonus += get_bard_frostbite_refrain_ii_cold_damage(ch);
  
  /* Warchanter's Dominance: +1 damage while performing */
  bonus += get_bard_warchanters_dominance_damage_bonus(ch);

  /* Add Bard Swashbuckler perks */
  /* Precise Strike I: +1 precision damage per rank with appropriate weapons */
  bonus += get_bard_precise_strike_i_bonus(ch);
  
  /* Precise Strike II: Additional +1 precision damage per rank with appropriate weapons */
  bonus += get_bard_precise_strike_ii_bonus(ch);
  
  /* Perfect Tempo: +2d6 precision damage on next attack after avoiding all hits */
  bonus += get_bard_perfect_tempo_damage_bonus(ch);
  
  /* Feint and Finish: +2d6 precision damage on next attack after feint */
  bonus += get_bard_feint_and_finish_damage_bonus(ch);
  
  return bonus;
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
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
    
  /* If wielded is NULL, character is unarmed - apply bonus */
  if (!wielded)
  {
    bonus = get_perk_bonus(ch, PERK_EFFECT_WEAPON_TOHIT, -1);
  }
  else
  {
    /* Check if the weapon is ranged - if so, don't apply bonus */
    if (GET_OBJ_TYPE(wielded) == ITEM_WEAPON || GET_OBJ_TYPE(wielded) == ITEM_FIREWEAPON)
    {
      int weapon_type = GET_OBJ_VAL(wielded, 0);
      if (IS_SET(weapon_list[weapon_type].weaponFlags, WEAPON_FLAG_RANGED))
        return 0; /* No bonus for ranged weapons */
    }
    
    /* Weapon is melee, apply bonus */
    bonus = get_perk_bonus(ch, PERK_EFFECT_WEAPON_TOHIT, -1);
  }
  
  /* Add Drummer's Rhythm I & II bonus while performing */
  bonus += get_bard_drummers_rhythm_tohit_bonus(ch);
  bonus += get_bard_drummers_rhythm_ii_tohit_bonus(ch);
  
  /* Add Warchanter's Dominance bonus */
  bonus += get_bard_warchanters_dominance_tohit_bonus(ch);

  /* Add Bard Swashbuckler bonuses */
  /* Flourish: +2 to hit while active */
  bonus += get_bard_flourish_tohit_bonus(ch);
  
  /* Perfect Tempo: +4 to hit on next attack after avoiding all hits */
  bonus += get_bard_perfect_tempo_tohit_bonus(ch);
  
  /* Feint and Finish: +2 to hit bonus (note: actual bonus is crit confirm, but also +2 to-hit from affect) */
  if (is_affected_by_feint_and_finish(ch))
    bonus += 2;
  
  return bonus;
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
  
  /* Bard Swashbuckler: Duelist's Poise - +2 with finesse weapon */
  bonus += get_bard_duelists_poise_crit_confirm_bonus(ch);
  
  /* Bard Swashbuckler: Feint and Finish - +2 to crit confirmation after feint */
  bonus += get_bard_feint_and_finish_crit_confirm_bonus(ch);
  
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
 * Get ranged weapon to-hit bonus from ranger perks.
 * Includes bonuses from Hunter tree perks like Archer's Focus I and II.
 * Only applies to ranged weapon attacks.
 * 
 * @param ch The character
 * @param wielded The wielded weapon (must be ranged)
 * @return Total ranged weapon to-hit bonus
 */
int get_ranger_ranged_tohit_bonus(struct char_data *ch, struct obj_data *wielded)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* If no weapon wielded or weapon is not ranged, return 0 */
  if (!wielded)
    return 0;
    
  if (GET_OBJ_TYPE(wielded) != ITEM_WEAPON && GET_OBJ_TYPE(wielded) != ITEM_FIREWEAPON)
    return 0;
    
  int weapon_type = GET_OBJ_VAL(wielded, 0);
  if (!IS_SET(weapon_list[weapon_type].weaponFlags, WEAPON_FLAG_RANGED))
    return 0; /* Not a ranged weapon */
  
  /* Archer's Focus I: +1 to-hit per rank (max 3 ranks) */
  bonus += get_total_perk_ranks(ch, PERK_RANGER_ARCHERS_FOCUS_I);
  
  /* Archer's Focus II: +1 to-hit per rank (max 2 ranks) */
  bonus += 1 * get_total_perk_ranks(ch, PERK_RANGER_ARCHERS_FOCUS_II);
  
  return bonus;
}

/**
 * Get ranged weapon damage bonus from ranger perks.
 * Includes bonuses from Hunter tree perks like Steady Aim I.
 * Only applies to ranged weapon attacks.
 * 
 * @param ch The character
 * @param wielded The wielded weapon (must be ranged)
 * @return Total ranged weapon damage bonus
 */
int get_ranger_ranged_damage_bonus(struct char_data *ch, struct obj_data *wielded)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* If no weapon wielded or weapon is not ranged, return 0 */
  if (!wielded)
    return 0;
    
  if (GET_OBJ_TYPE(wielded) != ITEM_WEAPON && GET_OBJ_TYPE(wielded) != ITEM_FIREWEAPON)
    return 0;
    
  int weapon_type = GET_OBJ_VAL(wielded, 0);
  if (!IS_SET(weapon_list[weapon_type].weaponFlags, WEAPON_FLAG_RANGED))
    return 0; /* Not a ranged weapon */
  
  /* Steady Aim I: +1 damage per rank (max 3 ranks) */
  bonus += get_total_perk_ranks(ch, PERK_RANGER_STEADY_AIM_I);
  
  return bonus;
}

/**
 * Get DR penetration value from ranger perks.
 * Deadly Aim allows arrows/bolts to ignore damage reduction.
 * 
 * @param ch The character
 * @return Total DR to ignore
 */
int get_ranger_dr_penetration(struct char_data *ch)
{
  int penetration = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Deadly Aim: Ignore 3 points of DR per rank (max 2 ranks = 6 DR ignored) */
  penetration += 3 * get_total_perk_ranks(ch, PERK_RANGER_DEADLY_AIM);
  
  return penetration;
}

/**
 * Get attack speed bonus from ranger perks.
 * Quick Draw reduces the time between ranged attacks.
 * 
 * @param ch The character
 * @return Attack speed bonus percentage (10 = 10% faster)
 */
int get_ranger_attack_speed_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Quick Draw: 10% attack speed per rank (max 3 ranks = 30% faster) */
  bonus += 10 * get_total_perk_ranks(ch, PERK_RANGER_QUICK_DRAW);
  
  return bonus;
}

/**
 * Get Quick Draw proc chance for an extra ranged attack.
 * 5% per rank of Quick Draw.
 * 
 * @param ch The character
 * @return Proc chance percentage (e.g., 10 for 10%)
 */
int get_ranger_quick_draw_proc_chance(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;

  /* Quick Draw: 5% chance per rank to grant an extra ranged attack */
  int ranks = get_total_perk_ranks(ch, PERK_RANGER_QUICK_DRAW);
  return 5 * ranks;
}

/**
 * Get HP bonus for ranger's animal companion from Beast Master perks.
 * 
 * @param ch The ranger (master)
 * @return Total HP bonus for companion
 */
int get_ranger_companion_hp_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Enhanced Companion I: +5 HP per rank */
  bonus += 5 * get_total_perk_ranks(ch, PERK_RANGER_ENHANCED_COMPANION_I);
  
  /* Enhanced Companion II: +10 HP per rank */
  bonus += 10 * get_total_perk_ranks(ch, PERK_RANGER_ENHANCED_COMPANION_II);
  
  /* Primal Avatar: +50 HP */
  if (has_perk(ch, PERK_RANGER_PRIMAL_AVATAR))
    bonus += 50;
  
  return bonus;
}

/**
 * Get AC bonus for ranger's animal companion from Beast Master perks.
 * 
 * @param ch The ranger (master)
 * @return Total AC bonus for companion
 */
int get_ranger_companion_ac_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Enhanced Companion I: +1 AC per rank */
  bonus += get_total_perk_ranks(ch, PERK_RANGER_ENHANCED_COMPANION_I);
  
  /* Enhanced Companion II: +2 AC per rank */
  bonus += 2 * get_total_perk_ranks(ch, PERK_RANGER_ENHANCED_COMPANION_II);
  
  /* Primal Avatar: +5 AC */
  if (has_perk(ch, PERK_RANGER_PRIMAL_AVATAR))
    bonus += 5;
  
  return bonus;
}

/**
 * Get to-hit bonus for ranger's animal companion from Beast Master perks.
 * 
 * @param ch The ranger (master)
 * @return Total to-hit bonus for companion
 */
int get_ranger_companion_tohit_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Enhanced Companion II: +2 to-hit per rank */
  bonus += 2 * get_total_perk_ranks(ch, PERK_RANGER_ENHANCED_COMPANION_II);
  
  /* Primal Avatar: +5 to-hit */
  if (has_perk(ch, PERK_RANGER_PRIMAL_AVATAR))
    bonus += 5;
  
  return bonus;
}

/**
 * Get save bonus for ranger's animal companion from Beast Master perks.
 * 
 * @param ch The ranger (master)
 * @return Total save bonus for companion
 */
int get_ranger_companion_save_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Alpha Bond: +3 to all saves */
  if (has_perk(ch, PERK_RANGER_ALPHA_BOND))
    bonus += 3;
  
  return bonus;
}

/**
 * Check if ranger's companion is immune to fear (Alpha Bond).
 * 
 * @param ch The ranger (master)
 * @return TRUE if companion immune to fear
 */
bool ranger_companion_immune_fear(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_RANGER_ALPHA_BOND);
}

/**
 * Check if ranger's companion is immune to mind-affecting (Primal Avatar).
 * 
 * @param ch The ranger (master)
 * @return TRUE if companion immune to mind-affecting
 */
bool ranger_companion_immune_mind(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_RANGER_PRIMAL_AVATAR);
}

/**
 * Get Pack Tactics to-hit bonus when ranger and companion attack same target.
 * 
 * @param ch The ranger or companion
 * @param master The ranger (if ch is companion)
 * @param victim The target being attacked
 * @return To-hit bonus
 */
int get_pack_tactics_bonus(struct char_data *ch, struct char_data *master, struct char_data *victim)
{
  int bonus = 0;
  struct char_data *ranger = NULL;
  
  if (!ch || !victim)
    return 0;
  
  /* Determine who the ranger is */
  if (!IS_NPC(ch) && CLASS_LEVEL(ch, CLASS_RANGER) > 0)
    ranger = ch;
  else if (master && !IS_NPC(master) && CLASS_LEVEL(master, CLASS_RANGER) > 0)
    ranger = master;
  else
    return 0;
  
  /* Pack Tactics I: +1 per rank when both attacking same target */
  bonus += get_total_perk_ranks(ranger, PERK_RANGER_PACK_TACTICS_I);
  
  return bonus;
}

/**
 * Get Coordinated Attack damage bonus (2d4) when both attack same target.
 * 
 * @param ch The ranger (master)
 * @return Damage in d4s (returns 2 for 2d4)
 */
int get_coordinated_attack_damage(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_perk(ch, PERK_RANGER_COORDINATED_ATTACK))
    return 2; /* 2d4 */
  
  return 0;
}

/**
 * Get Primal Avatar damage bonus for companion attacks.
 * 
 * @param ch The ranger (master)
 * @return Damage in d6s (returns 3 for 3d6)
 */
int get_primal_avatar_damage(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_perk(ch, PERK_RANGER_PRIMAL_AVATAR))
    return 3; /* 3d6 */
  
  return 0;
}

/**
 * Get healing effectiveness bonus from Nature's Remedy.
 * 
 * @param ch The caster
 * @return Percentage bonus (25 = 25% more effective)
 */
int get_natures_remedy_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Nature's Remedy: 25% per rank */
  return 25 * get_total_perk_ranks(ch, PERK_RANGER_NATURES_REMEDY);
}

/**
 * Check if ranger has Primal Vigor (HP regen in combat).
 * 
 * @param ch The ranger
 * @return TRUE if has perk
 */
bool has_primal_vigor(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_RANGER_PRIMAL_VIGOR);
}

/**
 * Check if ranger has Shared Spells perk (Beast Master tree).
 * Allows beneficial spells cast on the ranger to also affect their animal companion.
 * 
 * @param ch The ranger
 * @return TRUE if has perk
 */
bool has_shared_spells(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_RANGER_SHARED_SPELLS);
}

/**
 * Get conjuration spell DC bonus from Spell Focus: Conjuration.
 * 
 * @param ch The caster
 * @return DC bonus
 */
int get_ranger_conjuration_dc_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Spell Focus: Conjuration I: +1 per rank */
  return get_total_perk_ranks(ch, PERK_RANGER_SPELL_FOCUS_CONJURATION_I);
}

/**
 * Get Greater Summons HP bonus for summoned creatures.
 * 
 * @param ch The summoner
 * @return Percentage HP bonus (25 = 25% more HP)
 */
int get_greater_summons_hp_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_perk(ch, PERK_RANGER_GREATER_SUMMONS))
    return 25;
  
  return 0;
}

/**
 * Get Greater Summons damage bonus for summoned creatures.
 * 
 * @param ch The summoner
 * @return Damage in d6s (returns 1 for 1d6)
 */
int get_greater_summons_damage(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_perk(ch, PERK_RANGER_GREATER_SUMMONS))
    return 1; /* 1d6 */
  
  return 0;
}

/**
 * Get Greater Summons attack bonus for summoned creatures.
 * 
 * @param ch The summoner
 * @return Attack bonus (+4 to attack rolls)
 */
int get_greater_summons_attack_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_perk(ch, PERK_RANGER_GREATER_SUMMONS))
    return 4;
  
  return 0;
}

/* === WILDERNESS WARRIOR PERK HELPERS === */

/**
 * Get Two-Weapon Focus to-hit bonus when dual wielding.
 * 
 * @param ch The character
 * @return To-hit bonus
 */
int get_ranger_two_weapon_focus_tohit(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Two-Weapon Focus I: +1 per rank (max 3) */
  bonus += get_total_perk_ranks(ch, PERK_RANGER_TWO_WEAPON_FOCUS_I);
  
  return bonus;
}

/**
 * Get Two-Weapon Focus damage bonus when dual wielding.
 * 
 * @param ch The character
 * @return Damage bonus
 */
int get_ranger_two_weapon_focus_damage(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Two-Weapon Focus II: +1 per rank (max 2) */
  bonus += get_total_perk_ranks(ch, PERK_RANGER_TWO_WEAPON_FOCUS_II);
  
  return bonus;
}

/**
 * Get Dual Strike off-hand damage bonus.
 * 
 * @param ch The character
 * @return Off-hand damage bonus
 */
int get_ranger_dual_strike_offhand(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Dual Strike I: +1 per rank (max 3) */
  bonus += get_total_perk_ranks(ch, PERK_RANGER_DUAL_STRIKE_I);
  
  return bonus;
}

/**
 * Get Favored Enemy Mastery damage bonus vs favored enemies.
 * 
 * @param ch The character
 * @return Damage bonus vs favored enemies
 */
int get_ranger_favored_enemy_mastery_damage(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Favored Enemy Mastery I: +1 per rank (max 3) */
  bonus += get_total_perk_ranks(ch, PERK_RANGER_FAVORED_ENEMY_MASTERY_I);
  
  return bonus;
}

/**
 * Get Ranger Toughness HP bonus.
 * 
 * @param ch The character
 * @return HP bonus
 */
int get_ranger_toughness_hp(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Ranger Toughness I: +5 HP per rank (max 3) */
  bonus += 5 * get_total_perk_ranks(ch, PERK_RANGER_RANGER_TOUGHNESS_I);
  
  return bonus;
}

/**
 * Get Tempest AC bonus when dual wielding.
 * 
 * @param ch The character
 * @return AC dodge bonus
 */
int get_ranger_tempest_ac(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Tempest: +1 AC per rank (max 2) */
  bonus += get_total_perk_ranks(ch, PERK_RANGER_TEMPEST);
  
  return bonus;
}

/**
 * Get Favored Enemy Slayer to-hit bonus vs favored enemies.
 * 
 * @param ch The character
 * @return To-hit bonus
 */
int get_ranger_favored_enemy_slayer_tohit(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Favored Enemy Slayer: +2 to-hit */
  if (has_perk(ch, PERK_RANGER_FAVORED_ENEMY_SLAYER))
    return 2;
  
  return 0;
}

/**
 * Get Favored Enemy Slayer critical threat range bonus vs favored enemies.
 * 
 * @param ch The character
 * @return Critical threat range increase
 */
int get_ranger_favored_enemy_slayer_crit(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Favored Enemy Slayer: +1 crit threat range */
  if (has_perk(ch, PERK_RANGER_FAVORED_ENEMY_SLAYER))
    return 1;
  
  return 0;
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
 * Get Power Strike attack penalty.
 * 
 * @param ch The character
 * @return Attack penalty from Power Strike (-1 per rank)
 */
int get_monk_power_strike_penalty(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!has_perk(ch, PERK_MONK_POWER_STRIKE))
    return 0;
  
  /* Check if power strike is active (value > 0) */
  if (GET_POWER_STRIKE(ch) <= 0)
    return 0;
  
  /* Return penalty based on the power strike value (-1 per rank) */
  return -1 * GET_POWER_STRIKE(ch);
}

/**
 * Get Legendary Fist bonus damage (2d6).
 * 
 * @param ch The character
 * @return Legendary Fist damage dice (0 if no perk)
 */
int get_monk_legendary_fist_damage(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!has_perk(ch, PERK_MONK_LEGENDARY_FIST))
    return 0;
  
  /* 2d6 bonus damage */
  return dice(2, 6);
}

/**
 * Check if monk has Legendary Fist for crit multiplier.
 * 
 * @param ch The character
 * @return TRUE if has Legendary Fist, FALSE otherwise
 */
bool has_monk_legendary_fist(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_LEGENDARY_FIST);
}

/**
 * Check if monk has Crushing Blow perk.
 * 
 * @param ch The character
 * @return TRUE if has Crushing Blow, FALSE otherwise
 */
bool has_monk_crushing_blow(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_CRUSHING_BLOW);
}

/**
 * Check if monk has Shattering Strike perk.
 * 
 * @param ch The character
 * @return TRUE if has Shattering Strike, FALSE otherwise
 */
bool has_monk_shattering_strike(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_SHATTERING_STRIKE);
}

/**
 * Get monk Shadow Step movement bonus
 * 
 * @param ch The character
 * @return Movement speed bonus from Shadow Step I and II
 */
int get_monk_shadow_step_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Shadow Step I: +5 feet per rank (max 3 ranks) */
  bonus += get_perk_rank(ch, PERK_MONK_SHADOW_STEP_I, CLASS_MONK) * 5;
  
  /* Shadow Step II: +10 feet per rank (max 2 ranks) */
  bonus += get_perk_rank(ch, PERK_MONK_SHADOW_STEP_II, CLASS_MONK) * 10;
  
  /* Shadow Step III: +15 feet */
  if (has_perk(ch, PERK_MONK_SHADOW_STEP_III))
    bonus += 15;
  
  return bonus;
}

/**
 * Get monk Improved Hide stealth bonus
 * 
 * @param ch The character
 * @return Stealth skill bonus from Improved Hide I
 */
int get_monk_improved_hide_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* +2 to Stealth per rank (max 3 ranks) */
  return get_perk_rank(ch, PERK_MONK_IMPROVED_HIDE_I, CLASS_MONK) * 2;
}

/**
 * Get monk Acrobatic Defense AC bonus
 * 
 * @param ch The character
 * @return AC bonus from Acrobatic Defense
 */
int get_monk_acrobatic_defense_ac(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* +1 dodge AC if has perk */
  return has_perk(ch, PERK_MONK_ACROBATIC_DEFENSE) ? 1 : 0;
}

/**
 * Get monk Acrobatic Defense acrobatics bonus
 * 
 * @param ch The character
 * @return Acrobatics skill bonus from Acrobatic Defense
 */
int get_monk_acrobatic_defense_skill(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* +2 to Acrobatics if has perk */
  return has_perk(ch, PERK_MONK_ACROBATIC_DEFENSE) ? 2 : 0;
}

/**
 * Get monk Deadly Precision sneak attack bonus dice
 * 
 * @param ch The character
 * @return Number of sneak attack dice from Deadly Precision I and II
 */
int get_monk_deadly_precision_dice(struct char_data *ch)
{
  int dice = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Deadly Precision I: +1d6 per rank (max 3 ranks) */
  dice += get_perk_rank(ch, PERK_MONK_DEADLY_PRECISION_I, CLASS_MONK);
  
  /* Deadly Precision II: +1d6 per rank (max 2 ranks) */
  dice += get_perk_rank(ch, PERK_MONK_DEADLY_PRECISION_II, CLASS_MONK);
  
  /* Deadly Precision III: +2d6 */
  if (has_perk(ch, PERK_MONK_DEADLY_PRECISION_III))
    dice += 2;
  
  /* Shadow Master: +4d6 */
  if (has_perk(ch, PERK_MONK_SHADOW_MASTER))
    dice += 4;
  
  return dice;
}

/**
 * Check if character has Vanishing Technique perk
 * 
 * @param ch The character
 * @return TRUE if has Vanishing Technique, FALSE otherwise
 */
bool has_monk_vanishing_technique(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_VANISHING_TECHNIQUE);
}

/**
 * Check if character has Shadow Clone perk
 * 
 * @param ch The character
 * @return TRUE if has Shadow Clone, FALSE otherwise
 */
bool has_monk_shadow_clone(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_SHADOW_CLONE);
}

/**
 * Check if character has Smoke Bomb perk
 * 
 * @param ch The character
 * @return TRUE if has Smoke Bomb, FALSE otherwise
 */
bool has_monk_smoke_bomb(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_SMOKE_BOMB);
}

/**
 * Check if character has Pressure Point Strike perk
 * 
 * @param ch The character
 * @return TRUE if has Pressure Point Strike, FALSE otherwise
 */
bool has_monk_pressure_point_strike(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_PRESSURE_POINT_STRIKE);
}

/**
 * Check if character has Shadow Step III perk
 * 
 * @param ch The character
 * @return TRUE if has Shadow Step III, FALSE otherwise
 */
bool has_monk_shadow_step_iii(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_SHADOW_STEP_III);
}

/**
 * Get Deadly Precision III sneak attack bonus
 * 
 * @param ch The character
 * @return Bonus sneak attack dice from Deadly Precision III
 */
int get_monk_deadly_precision_iii_dice(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Deadly Precision III: +2d6 */
  if (has_perk(ch, PERK_MONK_DEADLY_PRECISION_III))
    return 2;
  
  return 0;
}

/**
 * Get Deadly Precision III critical sneak attack bonus
 * 
 * @param ch The character
 * @return Bonus sneak attack dice on critical hits
 */
int get_monk_deadly_precision_iii_crit_dice(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Deadly Precision III: +3d6 on crits */
  if (has_perk(ch, PERK_MONK_DEADLY_PRECISION_III))
    return 3;
  
  return 0;
}

/**
 * Check if character has Assassinate perk
 * 
 * @param ch The character
 * @return TRUE if has Assassinate, FALSE otherwise
 */
bool has_monk_assassinate(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_ASSASSINATE);
}

/**
 * Check if character has Shadow Fade perk
 * 
 * @param ch The character
 * @return TRUE if has Shadow Fade, FALSE otherwise
 */
bool has_monk_shadow_fade(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_SHADOW_FADE);
}

/**
 * Check if character has Blinding Speed perk
 * 
 * @param ch The character
 * @return TRUE if has Blinding Speed, FALSE otherwise
 */
bool has_monk_blinding_speed(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_BLINDING_SPEED);
}

/**
 * Check if character has Shadow Master perk
 * 
 * @param ch The character
 * @return TRUE if has Shadow Master, FALSE otherwise
 */
bool has_monk_shadow_master(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_SHADOW_MASTER);
}

/**
 * Check if character has Void Strike perk
 * 
 * @param ch The character
 * @return TRUE if has Void Strike, FALSE otherwise
 */
bool has_monk_void_strike(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_VOID_STRIKE);
}

/**
 * Get Elemental Attunement I rank.
 * 
 * @param ch The character
 * @return Rank of perk (0-5)
 */
int get_monk_elemental_attunement_i_rank(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  return get_perk_rank(ch, PERK_MONK_ELEMENTAL_ATTUNEMENT_I, CLASS_MONK);
}

/**
 * Check if character has Fangs of the Fire Snake perk.
 * 
 * @param ch The character
 * @return TRUE if has perk, FALSE otherwise
 */
bool has_monk_fangs_of_fire_snake(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_FANGS_OF_FIRE_SNAKE);
}

/**
 * Check if character has Water Whip perk.
 * 
 * @param ch The character
 * @return TRUE if has perk, FALSE otherwise
 */
bool has_monk_water_whip(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_WATER_WHIP);
}

/**
 * Check if character has Gong of the Summit perk.
 * 
 * @param ch The character
 * @return TRUE if has perk, FALSE otherwise
 */
bool has_monk_gong_of_summit(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_GONG_OF_SUMMIT);
}

/**
 * Get Fist of Unbroken Air rank.
 * 
 * @param ch The character
 * @return Rank of perk (0-3)
 */
int get_monk_fist_of_unbroken_air_rank(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  return get_perk_rank(ch, PERK_MONK_FIST_OF_UNBROKEN_AIR, CLASS_MONK);
}

/**
 * Get Elemental Resistance I rank.
 * 
 * @param ch The character
 * @return Rank of perk (0-3)
 */
int get_monk_elemental_resistance_i_rank(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  return get_perk_rank(ch, PERK_MONK_ELEMENTAL_RESISTANCE_I, CLASS_MONK);
}

/**
 * Get Elemental Attunement II rank.
 * 
 * @param ch The character
 * @return Rank of perk (0-3)
 */
int get_monk_elemental_attunement_ii_rank(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  return get_perk_rank(ch, PERK_MONK_ELEMENTAL_ATTUNEMENT_II, CLASS_MONK);
}

/**
 * Get Elemental Attunement III rank.
 * 
 * @param ch The character
 * @return Rank of perk (0-2)
 */
int get_monk_elemental_attunement_iii_rank(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  return get_perk_rank(ch, PERK_MONK_ELEMENTAL_ATTUNEMENT_III, CLASS_MONK);
}

/**
 * Check if character has Shape the Flowing River perk.
 * 
 * @param ch The character
 * @return TRUE if has perk, FALSE otherwise
 */
bool has_monk_shape_flowing_river(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_SHAPE_FLOWING_RIVER);
}

/**
 * Check if character has Sweeping Cinder Strike perk.
 * 
 * @param ch The character
 * @return TRUE if has perk, FALSE otherwise
 */
bool has_monk_sweeping_cinder_strike(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_SWEEPING_CINDER_STRIKE);
}

/**
 * Check if character has Rush of the Gale Spirits perk.
 * 
 * @param ch The character
 * @return TRUE if has perk, FALSE otherwise
 */
bool has_monk_rush_of_gale_spirits(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_RUSH_OF_GALE_SPIRITS);
}

/**
 * Check if character has Clench of the North Wind perk.
 * 
 * @param ch The character
 * @return TRUE if has perk, FALSE otherwise
 */
bool has_monk_clench_north_wind(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_CLENCH_NORTH_WIND);
}

/**
 * Check if character has Elemental Resistance II perk.
 * 
 * @param ch The character
 * @return TRUE if has perk, FALSE otherwise
 */
bool has_monk_elemental_resistance_ii(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_ELEMENTAL_RESISTANCE_II);
}

/**
 * Check if character has Mist Stance perk.
 * 
 * @param ch The character
 * @return TRUE if has perk, FALSE otherwise
 */
bool has_monk_mist_stance(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_MIST_STANCE);
}

/**
 * Check if character has Swarming Ice Rabbit perk.
 * 
 * @param ch The character
 * @return TRUE if has perk, FALSE otherwise
 */
bool has_monk_swarming_ice_rabbit(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_SWARMING_ICE_RABBIT);
}

bool has_monk_flames_of_phoenix(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_FLAMES_OF_PHOENIX);
}

bool has_monk_wave_of_rolling_earth(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_WAVE_OF_ROLLING_EARTH);
}

bool has_monk_ride_the_wind(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_RIDE_THE_WIND);
}

bool has_monk_eternal_mountain_defense(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_ETERNAL_MOUNTAIN_DEFENSE);
}

bool has_monk_fist_of_four_thunders(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_FIST_OF_FOUR_THUNDERS);
}

bool has_monk_river_of_hungry_flame(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_RIVER_OF_HUNGRY_FLAME);
}

bool has_monk_breath_of_winter(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_BREATH_OF_WINTER);
}

bool has_monk_elemental_embodiment(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_ELEMENTAL_EMBODIMENT);
}

bool has_monk_avatar_of_elements(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_AVATAR_OF_ELEMENTS);
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
    dice = MAX(2, dice);
  
  /* Paladin Channel Energy I: Add 1d6 per rank */
  dice += get_paladin_channel_energy_dice(ch);
  
  /* Paladin Channel Energy II: Add 2d6 per rank */
  dice += get_paladin_channel_energy_2_dice(ch);
  
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
  
  /* Paladin Destroy Undead: less than half paladin level in HD */
  if (has_paladin_destroy_undead(ch))
  {
    int paladin_level = CLASS_LEVEL(ch, CLASS_PALADIN);
    return (paladin_level / 2);
  }
  
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
  int count, i, j;
  struct perk_data *perk;
  struct char_perk_data *char_perk;
  int category;
  int category_perks[NUM_PERKS];
  int category_count;
  char cat_out[100];
  
  count = get_class_perks(class_id, perk_ids, NUM_PERKS);
  
  if (count == 0)
  {
    send_to_char(ch, "No perks available for %s.\r\n", class_names[class_id]);
    return;
  }
  
  send_to_char(ch, "\tc%s Perks\tn\r\n", class_names[class_id]);
  send_to_char(ch, "Available Perk Points: \tY%d\tn\r\n\r\n", get_perk_points(ch, class_id));
  
  /* Two column header */
  send_to_char(ch, "\tW%-4s  %-32s %s/%s  %-4s %-32s %s/%s\tn\r\n", 
               "ID", "Name", "Rnk", "Max", "ID", "Name", "Rnk", "Max");
  send_to_char(ch, "--------------------------------------------------------------------------------------------\r\n");
  send_to_char(ch, "\tW* = Available to purchase\tn\r\n\r\n");
  
  /* Loop through each possible category and display perks for that category */
  for (category = 0; category <= PERK_CATEGORY_PRIMAL_CHAMPION; category++)
  {
    /* Build list of perks in this category for this class */
    category_count = 0;
    for (i = 0; i < count; i++)
    {
      perk = get_perk_by_id(perk_ids[i]);
      if (perk && perk->perk_category == category)
      {
        category_perks[category_count++] = perk_ids[i];
      }
    }
    
    /* Skip this category if no perks */
    if (category_count == 0)
      continue;
    
    /* Debug output to syslog */
    log("PERK DEBUG: Displaying category %d (%s) with %d perks for class %s", 
        category, get_perk_category_name(category), category_count, class_names[class_id]);
    
    /* Sort perks within this category alphabetically by name */
    qsort(category_perks, category_count, sizeof(int), compare_perks_by_name);
    
    /* Display centered category header (skip for UNDEFINED category) */
    if (category != PERK_CATEGORY_UNDEFINED)
    {
      const char *category_name = get_perk_category_name(category);
      snprintf(cat_out, sizeof(cat_out), "\tC%s\tn", category_name);
      text_line(ch, cat_out, 95, '-', '-');
    }
    
    /* Display perks in this category in two columns */
    for (j = 0; j < category_count; j += 2)
    {
      /* Left column */
      perk = get_perk_by_id(category_perks[j]);
      if (!perk)
        continue;
        
      char_perk = find_char_perk(ch, category_perks[j], class_id);
      int current_rank = char_perk ? char_perk->current_rank : 0;
      bool can_purchase_left = meets_prerequisites_not_purchased(ch, category_perks[j], class_id);
      
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
      
      snprintf(left_col, sizeof(left_col), "%-4d %-35.35s\tn %s%3d\tn/%s%-3d\tn",
               perk->id,
               perk_name_left,
               current_rank > 0 ? "\tG" : "",
               current_rank,
               current_rank >= perk->max_rank ? "\tY" : "",
               perk->max_rank);
      
      /* Right column (if exists) */
      if (j + 1 < category_count)
      {
        perk = get_perk_by_id(category_perks[j + 1]);
        if (perk)
        {
          char_perk = find_char_perk(ch, category_perks[j + 1], class_id);
          current_rank = char_perk ? char_perk->current_rank : 0;
          bool can_purchase_right = meets_prerequisites_not_purchased(ch, category_perks[j + 1], class_id);
          
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
          
          send_to_char(ch, "%s %-4d %-35.35s\tn %s%3d\tn/%s%-3d\tn\r\n",
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
    
    /* Add spacing between categories */
    send_to_char(ch, "\r\n");
  }
  
  send_to_char(ch, "Use '\tcperk info <id>\tn' to see details about a perk.\r\n");
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
  
  /* Check if perk system is enabled */
  if (!CONFIG_PERK_SYSTEM)
  {
    send_to_char(ch, "The perk system is currently disabled.\r\n");
    return;
  }
  
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
  if (!CONFIG_PERK_SYSTEM)
  {
    send_to_char(ch, "The perk system is disabled.\r\n");
    return;
  }
  
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
  time_t current_time;
  
  if (!ch || IS_NPC(ch))
    return;
  
  if (!has_perk(ch, PERK_WIZARD_SPELL_SHIELD))
    return;
  
  /* Check if spell shield is on cooldown (2 minutes = 120 seconds) */
  current_time = time(0);
  if (GET_SPELL_SHIELD_COOLDOWN(ch) > current_time)
    return; /* Still on cooldown, don't activate */
  
  /* Activate the spell shield */
  GET_SPELL_SHIELD_TIMER(ch) = 1;
  
  /* Set cooldown to 2 minutes from now */
  GET_SPELL_SHIELD_COOLDOWN(ch) = current_time + 120;
  
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

/* ============================================================================
 * MONK PERK HELPER FUNCTIONS
 * ============================================================================ */

/**
 * Get total unarmed damage bonus from monk perks.
 * 
 * @param ch The character
 * @return Total bonus damage for unarmed attacks
 */
int get_monk_unarmed_damage_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Improved Unarmed Strike I: +1 per rank, max 5 */
  if (has_perk(ch, PERK_MONK_IMPROVED_UNARMED_STRIKE_I))
    bonus += get_perk_rank(ch, PERK_MONK_IMPROVED_UNARMED_STRIKE_I, CLASS_MONK);
  
  /* Improved Unarmed Strike II: +1 per rank, max 3 */
  if (has_perk(ch, PERK_MONK_IMPROVED_UNARMED_STRIKE_II))
    bonus += get_perk_rank(ch, PERK_MONK_IMPROVED_UNARMED_STRIKE_II, CLASS_MONK);
  
  /* Improved Unarmed Strike III: +2 per rank, max 2 */
  if (has_perk(ch, PERK_MONK_IMPROVED_UNARMED_STRIKE_III))
    bonus += get_perk_rank(ch, PERK_MONK_IMPROVED_UNARMED_STRIKE_III, CLASS_MONK) * 2;
  
  /* Tiger Claw: +2 damage */
  if (has_perk(ch, PERK_MONK_TIGER_CLAW))
    bonus += 2;
  
  /* Power Strike: +2 damage per rank when active */
  if (has_perk(ch, PERK_MONK_POWER_STRIKE) && GET_POWER_STRIKE(ch) > 0)
    bonus += GET_POWER_STRIKE(ch) * 3;
  
  return bonus;
}

/**
 * Get DR bypass amount from monk perks (also returns the crushing blow bypass).
 * 
 * @param ch The character
 * @return Amount of DR bypassed
 */
int get_monk_dr_bypass(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Fists of Iron: Bypass 2 DR */
  if (has_perk(ch, PERK_MONK_FISTS_OF_IRON))
    return 2;
  
  /* Note: Crushing Blow (10 DR bypass) is handled separately as it requires a stunning fist use */
  
  return 0;
}

/**
 * Get Reflex save bonus from monk perks.
 * 
 * @param ch The character
 * @return Reflex save bonus
 */
int get_monk_reflex_save_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Lightning Reflexes I: +1 per rank, max 3 */
  if (has_perk(ch, PERK_MONK_LIGHTNING_REFLEXES_I))
    bonus += get_perk_rank(ch, PERK_MONK_LIGHTNING_REFLEXES_I, CLASS_MONK);
  
  return bonus;
}

/**
 * Get trip bonus from monk perks.
 * 
 * @param ch The character
 * @return Trip attempt bonus
 */
int get_monk_trip_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Sweeping Strike: +2 trip bonus */
  if (has_perk(ch, PERK_MONK_SWEEPING_STRIKE))
    return 2;
  
  return 0;
}

/**
 * Check if monk can trip during flurry of blows.
 * 
 * @param ch The character
 * @return TRUE if can trip during flurry, FALSE otherwise
 */
bool can_monk_trip_during_flurry(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_SWEEPING_STRIKE);
}

/**
 * Get HP regeneration bonus from monk perks.
 * 
 * @param ch The character
 * @return HP regeneration bonus per tick
 */
int get_monk_hp_regen_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Meditation Focus I: +1 per rank, max 3 */
  if (has_perk(ch, PERK_MONK_MEDITATION_FOCUS_I))
    bonus += get_perk_rank(ch, PERK_MONK_MEDITATION_FOCUS_I, CLASS_MONK);
  
  return bonus;
}

/**
 * Get bonus ki points from monk perks.
 * 
 * @param ch The character
 * @return Bonus ki points
 */
int get_monk_stunning_fist_bonus_uses(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Meditation Focus I: +1 per rank, max 3 */
  if (has_perk(ch, PERK_MONK_MEDITATION_FOCUS_I))
    bonus += get_perk_rank(ch, PERK_MONK_MEDITATION_FOCUS_I, CLASS_MONK);
  
  return bonus;
}

/**
 * Get stunning fist DC bonus from monk perks.
 * 
 * @param ch The character
 * @return DC bonus for stunning fist
 */
int get_monk_stunning_fist_dc_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Stunning Fist Enhancement: +2 DC */
  if (has_perk(ch, PERK_MONK_STUNNING_FIST_ENHANCEMENT))
    return 2;
  
  return 0;
}

/**
 * Get stunning fist duration bonus from monk perks.
 * 
 * @param ch The character
 * @return Duration bonus in rounds
 */
int get_monk_stunning_fist_duration_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Stunning Fist Enhancement: +1 round */
  if (has_perk(ch, PERK_MONK_STUNNING_FIST_ENHANCEMENT))
    return 1;
  
  return 0;
}

/**
 * Get unarmed critical threat range from monk perks.
 * 
 * @param ch The character
 * @return Critical threat range minimum (20 by default, 19 with perk, 18 with perk II)
 */
int get_monk_unarmed_crit_range(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 20;
  
  /* Improved Critical: Unarmed II - 18-20 */
  if (has_perk(ch, PERK_MONK_IMPROVED_CRITICAL_UNARMED_II))
    return 18;
  
  /* Improved Critical: Unarmed - 19-20 */
  if (has_perk(ch, PERK_MONK_IMPROVED_CRITICAL_UNARMED))
    return 19;
  
  return 20;
}

/**
 * Check if monk can apply bleeding from Tiger Claw.
 * 
 * @param ch The character
 * @return TRUE if has Tiger Claw, FALSE otherwise
 */
bool has_monk_tiger_claw_bleed(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_TIGER_CLAW);
}

/**
 * Check if monk can use quarterstaff/kama with monk abilities.
 * 
 * @param ch The character
 * @return TRUE if has One With Wood and Stone, FALSE otherwise
 */
bool can_use_monk_weapons_with_abilities(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_MONK_ONE_WITH_WOOD_AND_STONE);
}

/**
 * Get AC bonus when wielding monk weapons (quarterstaff/kama).
 * 
 * @param ch The character
 * @param weapon The weapon being checked
 * @return AC bonus (1 if monk weapon, 0 otherwise)
 */
int get_monk_weapon_ac_bonus(struct char_data *ch, struct obj_data *weapon)
{
  if (!ch || IS_NPC(ch) || !weapon)
    return 0;
  
  if (!has_perk(ch, PERK_MONK_ONE_WITH_WOOD_AND_STONE))
    return 0;
  
  /* Check if weapon is quarterstaff or kama */
  int weapon_type = GET_OBJ_VAL(weapon, 0);
  if (weapon_type == WEAPON_TYPE_QUARTERSTAFF || weapon_type == WEAPON_TYPE_KAMA)
    return 1;
  
  return 0;
}

/**
 * Get attack bonus when wielding monk weapons (quarterstaff/kama).
 * 
 * @param ch The character
 * @param weapon The weapon being checked
 * @return Attack bonus (1 if monk weapon, 0 otherwise)
 */
int get_monk_weapon_attack_bonus(struct char_data *ch, struct obj_data *weapon)
{
  if (!ch || IS_NPC(ch) || !weapon)
    return 0;
  
  if (!has_perk(ch, PERK_MONK_ONE_WITH_WOOD_AND_STONE))
    return 0;
  
  /* Check if weapon is quarterstaff or kama */
  int weapon_type = GET_OBJ_VAL(weapon, 0);
  if (weapon_type == WEAPON_TYPE_QUARTERSTAFF || weapon_type == WEAPON_TYPE_KAMA)
    return 1;
  
  return 0;
}

/**
 * Get damage bonus when wielding monk weapons (quarterstaff/kama).
 * 
 * @param ch The character
 * @param weapon The weapon being checked
 * @return Damage bonus (1 if monk weapon, 0 otherwise)
 */
int get_monk_weapon_damage_bonus(struct char_data *ch, struct obj_data *weapon)
{
  if (!ch || IS_NPC(ch) || !weapon)
    return 0;
  
  if (!has_perk(ch, PERK_MONK_ONE_WITH_WOOD_AND_STONE))
    return 0;
  
  /* Check if weapon is quarterstaff or kama */
  int weapon_type = GET_OBJ_VAL(weapon, 0);
  if (weapon_type == WEAPON_TYPE_QUARTERSTAFF || weapon_type == WEAPON_TYPE_KAMA)
    return 1;
  
  return 0;
}

/**
 * Get flurry of blows penalty reduction from monk perks.
 * 
 * @param ch The character
 * @return Penalty reduction (negative number to reduce penalty)
 */
int get_monk_flurry_penalty_reduction(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Flurry Focus: -1 to flurry penalty */
  if (has_perk(ch, PERK_MONK_FLURRY_FOCUS))
    return -1;
  
  return 0;
}

/**
 * Check if monk gets extra flurry attack this round.
 * 10% chance with Flurry Focus, 20% with Fists of Fury.
 * 
 * @param ch The character
 * @return TRUE if extra attack triggered, FALSE otherwise
 */
bool check_monk_extra_flurry_attack(struct char_data *ch)
{
  int chance = 0;
  
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  if (!has_perk(ch, PERK_MONK_FLURRY_FOCUS))
    return FALSE;
  
  /* Fists of Fury increases chance from 10% to 20% */
  if (has_perk(ch, PERK_MONK_FISTS_OF_FURY))
    chance = 20;
  else
    chance = 10;
  
  return (rand_number(1, 100) <= chance);
}

/* =============================================================== */
/* DRUID PERK HELPER FUNCTIONS - STUBS (NOT YET IMPLEMENTED)      */
/* =============================================================== */

int get_druid_wild_shape_attack_bonus(struct char_data *ch) { return 0; }
int get_druid_wild_shape_damage_bonus(struct char_data *ch) { return 0; }
int get_druid_natural_armor_bonus(struct char_data *ch) { return 0; }
int get_druid_wild_shape_hp_bonus(struct char_data *ch) { return 0; }
int get_druid_natural_weapons_damage_dice(struct char_data *ch) { return 0; }
bool has_druid_natural_weapons_improved_crit(struct char_data *ch) { return FALSE; }
int get_druid_elemental_attack_bonus(struct char_data *ch) { return 0; }
int get_druid_elemental_damage_bonus(struct char_data *ch) { return 0; }
int get_druid_elemental_armor_bonus(struct char_data *ch) { return 0; }
int get_druid_elemental_hp_bonus(struct char_data *ch) { return 0; }
bool has_druid_primal_avatar(struct char_data *ch) { return FALSE; }
bool has_druid_natural_fury(struct char_data *ch) { return FALSE; }
int get_druid_spell_power_bonus(struct char_data *ch) { return 0; }
int get_druid_spell_dc_bonus(struct char_data *ch) { return 0; }
int get_druid_elemental_damage_dice(struct char_data *ch) { return 0; }
bool check_druid_spell_critical(struct char_data *ch) { return FALSE; }
float get_druid_spell_critical_multiplier(struct char_data *ch) { return 1.0; }
int get_druid_bonus_spell_slots(struct char_data *ch) { return 0; }
bool has_druid_force_of_nature(struct char_data *ch) { return FALSE; }
bool has_druid_storm_caller(struct char_data *ch) { return FALSE; }
bool has_druid_elemental_mastery(struct char_data *ch) { return FALSE; }

/* =============================================================== */
/* BERSERKER PERK HELPER FUNCTIONS - STUBS (NOT YET IMPLEMENTED)  */
/* =============================================================== */

int get_berserker_power_attack_bonus(struct char_data *ch) { return 0; }
int get_berserker_rage_damage_bonus(struct char_data *ch) { return 0; }
int get_berserker_critical_bonus(struct char_data *ch) { return 0; }
bool has_berserker_cleaving_strikes(struct char_data *ch) { return FALSE; }
int get_berserker_cleave_bonus(struct char_data *ch) { return 0; }
bool has_berserker_blood_frenzy(struct char_data *ch) { return FALSE; }
int get_berserker_devastating_critical_dice(struct char_data *ch) { return 0; }
int get_berserker_power_attack_mastery_3_bonus(struct char_data *ch) { return 0; }
bool has_berserker_overwhelming_force(struct char_data *ch) { return FALSE; }
int get_berserker_crimson_rage_bonus(struct char_data *ch) { return 0; }
bool has_berserker_carnage(struct char_data *ch) { return FALSE; }
bool has_berserker_frenzied_berserker(struct char_data *ch) { return FALSE; }
bool has_berserker_relentless_assault(struct char_data *ch) { return FALSE; }
bool has_berserker_death_from_above(struct char_data *ch) { return FALSE; }
int get_berserker_thick_skin_bonus(struct char_data *ch) { return 0; }
int get_berserker_damage_reduction(struct char_data *ch) { return 0; }
int get_berserker_elemental_resistance(struct char_data *ch) { return 0; }
bool has_berserker_hardy(struct char_data *ch) { return FALSE; }
int get_berserker_savage_defiance_dr(struct char_data *ch) { return 0; }
int get_berserker_damage_reduction_3(struct char_data *ch) { return 0; }
int get_berserker_spell_resistance(struct char_data *ch) { return 0; }
bool has_berserker_pain_tolerance(struct char_data *ch) { return FALSE; }
bool has_berserker_deathless_frenzy(struct char_data *ch) { return FALSE; }
int get_berserker_unstoppable_dr(struct char_data *ch) { return 0; }
bool has_berserker_indomitable_will(struct char_data *ch) { return FALSE; }
bool has_berserker_raging_defender(struct char_data *ch) { return FALSE; }
int get_berserker_fleet_of_foot_bonus(struct char_data *ch) { return 0; }
int get_berserker_intimidating_presence_bonus(struct char_data *ch) { return 0; }
int get_berserker_intimidating_presence_morale_penalty(struct char_data *ch) { return 0; }
bool has_berserker_mighty_leap(struct char_data *ch) { return FALSE; }
int get_berserker_mighty_leap_bonus(struct char_data *ch) { return 0; }
int get_berserker_thick_headed_bonus(struct char_data *ch) { return 0; }
bool has_berserker_sprint(struct char_data *ch) { return FALSE; }
bool has_berserker_intimidating_presence_2(struct char_data *ch) { return FALSE; }
int get_berserker_crippling_blow_chance(struct char_data *ch) { return 0; }
bool has_berserker_reckless_abandon(struct char_data *ch) { return FALSE; }
bool has_berserker_blinding_rage(struct char_data *ch) { return FALSE; }
bool has_berserker_stunning_blow(struct char_data *ch) { return FALSE; }
bool has_berserker_uncanny_dodge_mastery(struct char_data *ch) { return FALSE; }
int get_berserker_uncanny_dodge_perception_bonus(struct char_data *ch) { return 0; }
int get_berserker_uncanny_dodge_ac_bonus(struct char_data *ch) { return 0; }
bool has_berserker_savage_charge(struct char_data *ch) { return FALSE; }
bool has_berserker_war_cry(struct char_data *ch) { return FALSE; }
bool has_berserker_earthshaker(struct char_data *ch) { return FALSE; }

/* =============================================================== */
/* PALADIN PERK HELPER FUNCTIONS - KNIGHT OF THE CHALICE         */
/* =============================================================== */

/**
 * Get damage bonus from Holy Weapon perks against evil creatures.
 * 
 * @param ch The character
 * @param victim The target (to check if evil)
 * @return Damage bonus
 */
int get_paladin_holy_weapon_damage_bonus(struct char_data *ch, struct char_data *victim)
{
  if (!ch || IS_NPC(ch) || !victim)
    return 0;
  
  if (!IS_EVIL(victim))
    return 0;
  
  int bonus = 0;
  
  /* Holy Weapon I: +2 per rank, max 3 ranks = +6 */
  if (has_perk(ch, PERK_PALADIN_HOLY_WEAPON_1))
    bonus += get_perk_rank(ch, PERK_PALADIN_HOLY_WEAPON_1, CLASS_PALADIN) * 2;
  
  /* Holy Weapon II: +2 per rank, max 2 ranks = +4 */
  if (has_perk(ch, PERK_PALADIN_HOLY_WEAPON_2))
    bonus += get_perk_rank(ch, PERK_PALADIN_HOLY_WEAPON_2, CLASS_PALADIN) * 2;
  
  return bonus;
}

/**
 * Get AC bonus from Sacred Defender perks.
 * 
 * @param ch The character
 * @return AC bonus
 */
int get_paladin_sacred_defender_ac_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Sacred Defender: +1 AC per rank when wielding weapon and shield */
  if (!has_perk(ch, PERK_PALADIN_SACRED_DEFENDER))
    return 0;
  
  /* Check if wielding shield */
  struct obj_data *shield = GET_EQ(ch, WEAR_SHIELD);
  if (!shield || GET_OBJ_TYPE(shield) != ITEM_ARMOR)
    return 0;
  
  /* Check if wielding weapon */
  struct obj_data *weapon = GET_EQ(ch, WEAR_WIELD_1);
  if (!weapon || GET_OBJ_TYPE(weapon) != ITEM_WEAPON)
    return 0;
  
  return get_perk_rank(ch, PERK_PALADIN_SACRED_DEFENDER, CLASS_PALADIN);
}

/**
 * Get bonus smite damage dice from Improved Smite perk.
 * 
 * @param ch The character
 * @return Number of bonus d6 dice
 */
int get_paladin_improved_smite_dice(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!has_perk(ch, PERK_PALADIN_IMPROVED_SMITE))
    return 0;
  
  return get_perk_rank(ch, PERK_PALADIN_IMPROVED_SMITE, CLASS_PALADIN);
}

/**
 * Check if character has Faithful Strike perk.
 */
bool has_paladin_faithful_strike(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PALADIN_FAITHFUL_STRIKE);
}

/**
 * Check if character has Holy Blade perk.
 */
bool has_paladin_holy_blade(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PALADIN_HOLY_BLADE);
}

/**
 * Check if character has Divine Might perk.
 */
bool has_paladin_divine_might(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PALADIN_DIVINE_MIGHT);
}

/**
 * Check if character has Exorcism of the Slain perk.
 */
bool has_paladin_exorcism_of_the_slain(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PALADIN_EXORCISM_OF_THE_SLAIN);
}

/**
 * Check if character has Holy Sword perk.
 */
bool has_paladin_holy_sword(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PALADIN_HOLY_SWORD);
}

/**
 * Check if character has Zealous Smite perk.
 */
bool has_paladin_zealous_smite(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PALADIN_ZEALOUS_SMITE);
}

/**
 * Check if character has Blinding Smite perk.
 */
bool has_paladin_blinding_smite(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PALADIN_BLINDING_SMITE);
}

/**
 * Check if character has Overwhelming Smite perk.
 */
bool has_paladin_overwhelming_smite(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PALADIN_OVERWHELMING_SMITE);
}

/**
 * Check if character has Sacred Vengeance perk.
 */
bool has_paladin_sacred_vengeance(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PALADIN_SACRED_VENGEANCE);
}

/* =============================================================== */
/* PALADIN PERK HELPER FUNCTIONS - SACRED DEFENDER                */
/* =============================================================== */

/**
 * Get total extra Lay on Hands uses from perks.
 * 
 * @param ch The character
 * @return Total bonus uses
 */
int get_paladin_extra_lay_on_hands(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  int bonus = 0;
  
  /* Extra Lay on Hands I: +1 per rank, max 3 ranks = +3 */
  if (has_perk(ch, PERK_PALADIN_EXTRA_LAY_ON_HANDS_1))
    bonus += get_perk_rank(ch, PERK_PALADIN_EXTRA_LAY_ON_HANDS_1, CLASS_PALADIN);
  
  /* Extra Lay on Hands II: +1 per rank, max 2 ranks = +2 */
  if (has_perk(ch, PERK_PALADIN_EXTRA_LAY_ON_HANDS_2))
    bonus += get_perk_rank(ch, PERK_PALADIN_EXTRA_LAY_ON_HANDS_2, CLASS_PALADIN);
  
  return bonus;
}

/**
 * Get AC bonus from Shield of Faith perks.
 * 
 * @param ch The character
 * @return AC bonus
 */
int get_paladin_shield_of_faith_ac_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  int bonus = 0;
  
  /* Shield of Faith I: +1 per rank, max 3 ranks = +3 */
  if (has_perk(ch, PERK_PALADIN_SHIELD_OF_FAITH_1))
    bonus += get_perk_rank(ch, PERK_PALADIN_SHIELD_OF_FAITH_1, CLASS_PALADIN);
  
  /* Shield of Faith II: +1 per rank, max 2 ranks = +2 */
  if (has_perk(ch, PERK_PALADIN_SHIELD_OF_FAITH_2))
    bonus += get_perk_rank(ch, PERK_PALADIN_SHIELD_OF_FAITH_2, CLASS_PALADIN);
  
  return bonus;
}

/**
 * Get saves bonus from Bulwark of Defense when wielding a shield.
 * 
 * @param ch The character
 * @return Saves bonus
 */
int get_paladin_bulwark_saves_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!has_perk(ch, PERK_PALADIN_BULWARK_OF_DEFENSE))
    return 0;
  
  /* Check if wielding shield */
  struct obj_data *shield = GET_EQ(ch, WEAR_SHIELD);
  if (!shield || GET_OBJ_TYPE(shield) != ITEM_ARMOR)
    return 0;
  
  return get_perk_rank(ch, PERK_PALADIN_BULWARK_OF_DEFENSE, CLASS_PALADIN);
}

/**
 * Get healing percentage bonus from Healing Hands perk.
 * 
 * @param ch The character
 * @return Healing percentage bonus (10 per rank)
 */
int get_paladin_healing_hands_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!has_perk(ch, PERK_PALADIN_HEALING_HANDS))
    return 0;
  
  return get_perk_rank(ch, PERK_PALADIN_HEALING_HANDS, CLASS_PALADIN) * 10;
}

/**
 * Check if character has Defensive Strike perk.
 */
bool has_paladin_defensive_strike(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PALADIN_DEFENSIVE_STRIKE);
}

/**
 * Check if character has Shield Guardian perk.
 */
bool has_paladin_shield_guardian(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PALADIN_SHIELD_GUARDIAN);
}

/**
 * Check if character has Aura of Protection perk.
 */
bool has_paladin_aura_of_protection(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PALADIN_AURA_OF_PROTECTION);
}

/**
 * Get Sanctuary damage reduction percentage.
 * Returns 10 if character has the perk, 0 otherwise.
 */
int get_paladin_sanctuary_reduction(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  if (has_perk(ch, PERK_PALADIN_SANCTUARY))
    return 10; /* 10% damage reduction */
  return 0;
}

/**
 * Check if character has Merciful Touch perk.
 */
bool has_paladin_merciful_touch(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PALADIN_MERCIFUL_TOUCH);
}

/**
 * Check if character has Bastion of Defense perk.
 */
bool has_paladin_bastion_of_defense(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PALADIN_BASTION_OF_DEFENSE);
}

/**
 * Check if character has Aura of Life perk.
 */
bool has_paladin_aura_of_life(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PALADIN_AURA_OF_LIFE);
}

/**
 * Check if character has Cleansing Touch perk.
 */
bool has_paladin_cleansing_touch(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PALADIN_CLEANSING_TOUCH);
}

/**
 * Check if character has Divine Sacrifice perk.
 */
bool has_paladin_divine_sacrifice(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PALADIN_DIVINE_SACRIFICE);
}

/**
 * Get total Spell Focus bonus from Divine Champion perks.
 * Returns +1 per rank of Spell Focus I and II.
 */
int get_paladin_spell_focus_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
    
  bonus += get_perk_rank(ch, PERK_PALADIN_SPELL_FOCUS_1, CLASS_PALADIN);
  bonus += get_perk_rank(ch, PERK_PALADIN_SPELL_FOCUS_2, CLASS_PALADIN);
  
  return bonus;
}

/**
 * Get Turn Undead HD bonus from Divine Champion perks.
 * Turn Undead Mastery I: +2 HD per rank
 * Turn Undead Mastery II: +3 HD per rank
 */
int get_paladin_turn_undead_hd_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
    
  bonus += get_perk_rank(ch, PERK_PALADIN_TURN_UNDEAD_MASTERY_1, CLASS_PALADIN) * 2;
  bonus += get_perk_rank(ch, PERK_PALADIN_TURN_UNDEAD_MASTERY_2, CLASS_PALADIN) * 3;
  
  return bonus;
}

/**
 * Get Turn Undead damage bonus from Divine Champion perks.
 * Turn Undead Mastery II: +2d6 damage per rank
 */
int get_paladin_turn_undead_damage_bonus(struct char_data *ch)
{
  int bonus_dice = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
    
  bonus_dice = get_perk_rank(ch, PERK_PALADIN_TURN_UNDEAD_MASTERY_2, CLASS_PALADIN) * 2;
  
  return bonus_dice;
}

/**
 * Get Divine Grace save bonus from Divine Champion perks.
 * Returns +1 per rank to all saves.
 */
int get_paladin_divine_grace_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
    
  return get_perk_rank(ch, PERK_PALADIN_DIVINE_GRACE, CLASS_PALADIN);
}

/**
 * Check if character has Radiant Aura perk.
 */
bool has_paladin_radiant_aura(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PALADIN_RADIANT_AURA);
}

/**
 * Check if character has Quickened Blessing perk.
 */
bool has_paladin_quickened_blessing(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PALADIN_QUICKENED_BLESSING);
}

/**
 * Get Channel Energy dice count.
 * Returns 1d6 per rank of Channel Energy I.
 */
int get_paladin_channel_energy_dice(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
    
  return get_perk_rank(ch, PERK_PALADIN_CHANNEL_ENERGY_1, CLASS_PALADIN);
}

/**
 * Get Channel Energy daily uses.
 * Base: 2 uses per day
 * Channel Energy II adds +2 uses per rank
 */
int get_paladin_channel_energy_uses(struct char_data *ch)
{
  int uses = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
    
  if (!has_perk(ch, PERK_PALADIN_CHANNEL_ENERGY_1))
    return 0;
    
  uses = 2; /* Base 2 uses per day */
  
  /* Add Channel Energy II bonus uses */
  uses += get_paladin_channel_energy_2_uses(ch);
  
  return uses;
}

bool is_quickened_blessing_spell(int spellnum)
{
  switch (spellnum)
  {
    case SPELL_BLESS:
    case SPELL_CURE_LIGHT:
    case SPELL_CURE_MODERATE:
    case SPELL_CURE_SERIOUS:
    case SPELL_CURE_CRITIC:
    case SPELL_PROT_FROM_EVIL:
      return TRUE;
    default:
      return FALSE;
  }
}

/* ===== TIER 3 HELPER FUNCTIONS ===== */

/**
 * Check if character has Spell Penetration perk.
 */
bool has_paladin_spell_penetration(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PALADIN_SPELL_PENETRATION);
}

/**
 * Check if character has Destroy Undead perk.
 */
bool has_paladin_destroy_undead(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PALADIN_DESTROY_UNDEAD);
}

/**
 * Get Channel Energy II bonus dice.
 * Returns +2d6 per rank.
 */
int get_paladin_channel_energy_2_dice(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
    
  return get_perk_rank(ch, PERK_PALADIN_CHANNEL_ENERGY_2, CLASS_PALADIN) * 2;
}

/**
 * Get Channel Energy II bonus uses per day.
 * Returns +2 uses per rank.
 */
int get_paladin_channel_energy_2_uses(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
    
  return get_perk_rank(ch, PERK_PALADIN_CHANNEL_ENERGY_2, CLASS_PALADIN) * 2;
}

/**
 * Check if character has Aura of Courage Mastery perk.
 */
bool has_paladin_aura_of_courage_mastery(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PALADIN_AURA_OF_COURAGE_MASTERY);
}

/* ===== TIER 4 HELPER FUNCTIONS ===== */

/**
 * Check if character has Mass Cure Wounds perk.
 */
bool has_paladin_mass_cure_wounds(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PALADIN_MASS_CURE_WOUNDS);
}

/**
 * Check if character has Holy Avenger perk.
 */
bool has_paladin_holy_avenger(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PALADIN_HOLY_AVENGER);
}

/**
 * Check if character has Beacon of Hope perk.
 */
bool has_paladin_beacon_of_hope(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  return has_perk(ch, PERK_PALADIN_BEACON_OF_HOPE);
}


/******************************************************************************
 * Bard Spellsinger Tree Perk Functions
 ******************************************************************************/

/**
 * Get Enchanter's Guile DC bonus for Enchantment and Illusion spells.
 * 
 * @param ch The character
 * @return Total DC bonus for Enchantment/Illusion spells
 */
int get_bard_enchanters_guile_dc_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Enchanter's Guile I: +1 DC per rank */
  bonus += get_perk_rank(ch, PERK_BARD_ENCHANTERS_GUILE_I, CLASS_BARD);
  
  /* Enchanter's Guile II: +1 additional DC per rank */
  bonus += get_perk_rank(ch, PERK_BARD_ENCHANTERS_GUILE_II, CLASS_BARD);
  
  return bonus;
}

/**
 * Get Songweaver song level bonus for bardic performance scaling.
 * 
 * @param ch The character
 * @return Total effective song level bonus
 */
int get_bard_songweaver_level_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Songweaver I: +1 effective song level per rank */
  bonus += get_perk_rank(ch, PERK_BARD_SONGWEAVER_I, CLASS_BARD);
  
  /* Songweaver II: +1 additional effective song level per rank */
  bonus += get_perk_rank(ch, PERK_BARD_SONGWEAVER_II, CLASS_BARD);
  
  return bonus;
}

/**
 * Get Resonant Voice save bonus for allies under bard songs.
 * 
 * @param ch The character
 * @return Total save bonus vs. mind-affecting for allies
 */
int get_bard_resonant_voice_save_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Resonant Voice I: +1 competence to saves per rank */
  bonus += get_perk_rank(ch, PERK_BARD_RESONANT_VOICE_I, CLASS_BARD);
  
  return bonus;
}

/**
 * Check if character has Harmonic Casting perk.
 * 
 * @param ch The character
 * @return TRUE if has Harmonic Casting, FALSE otherwise
 */
bool has_bard_harmonic_casting(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_HARMONIC_CASTING);
}
/**
 * Get Songweaver II song level bonus (Tier 2).
 * 
 * @param ch The character
 * @return Additional effective song level bonus from Tier 2
 */
int get_bard_songweaver_ii_level_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Songweaver II: +1 additional effective song level per rank */
  bonus += get_perk_rank(ch, PERK_BARD_SONGWEAVER_II, CLASS_BARD);
  
  return bonus;
}

/**
 * Get Enchanter's Guile II DC bonus (Tier 2).
 * 
 * @param ch The character
 * @return Additional DC bonus for Enchantment/Illusion spells from Tier 2
 */
int get_bard_enchanters_guile_ii_dc_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Enchanter's Guile II: +1 additional DC per rank */
  bonus += get_perk_rank(ch, PERK_BARD_ENCHANTERS_GUILE_II, CLASS_BARD);
  
  return bonus;
}

/**
 * Check if character has Crescendo perk.
 * 
 * @param ch The character
 * @return TRUE if has Crescendo, FALSE otherwise
 */
bool has_bard_crescendo(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_CRESCENDO);
}

/**
 * Get sonic damage dice value for Crescendo.
 * 
 * @param ch The character
 * @return Number of d6 dice to roll for sonic damage (1 if has perk, 0 otherwise)
 */
int get_bard_crescendo_sonic_damage(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_bard_crescendo(ch))
    return 1; /* +1d6 sonic damage */
  
  return 0;
}

/**
 * Get save DC bonus for Crescendo.
 * 
 * @param ch The character
 * @return DC bonus (+2 if has perk, 0 otherwise)
 */
int get_bard_crescendo_dc_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_bard_crescendo(ch))
    return 2; /* +2 to save DC */
  
  return 0;
}

/**
 * Check if character has Sustaining Melody perk.
 * 
 * @param ch The character
 * @return TRUE if has Sustaining Melody, FALSE otherwise
 */
bool has_bard_sustaining_melody(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_SUSTAINING_MELODY);
}

/* ============================================================================
 * TIER III SPELLSINGER PERK FUNCTIONS
 * ============================================================================ */

/**
 * Check if character has Master of Motifs perk.
 * Allows maintaining two distinct bard songs simultaneously.
 * 
 * @param ch The character
 * @return TRUE if has Master of Motifs, FALSE otherwise
 */
bool has_bard_master_of_motifs(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_MASTER_OF_MOTIFS);
}

/**
 * Check if character has Dirge of Dissonance perk.
 * Causes enemies in the room to take sonic damage and concentration penalties.
 * 
 * @param ch The character
 * @return TRUE if has Dirge of Dissonance, FALSE otherwise
 */
bool has_bard_dirge_of_dissonance(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_DIRGE_OF_DISSONANCE);
}

/**
 * Get sonic damage dice value for Dirge of Dissonance.
 * 
 * @param ch The character
 * @return Number of d6 dice to roll for sonic damage per round (1 if has perk, 0 otherwise)
 */
int get_bard_dirge_sonic_damage(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_bard_dirge_of_dissonance(ch))
    return 1; /* 1d6 sonic damage per round */
  
  return 0;
}

/**
 * Get concentration penalty for Dirge of Dissonance.
 * 
 * @param ch The character
 * @return Concentration check penalty (-2 if has perk, 0 otherwise)
 */
int get_bard_dirge_concentration_penalty(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_bard_dirge_of_dissonance(ch))
    return -2; /* -2 to concentration checks */
  
  return 0;
}

/**
 * Check if character has Heightened Harmony perk.
 * Grants perform skill bonus when using metamagic on bard spells.
 * 
 * @param ch The character
 * @return TRUE if has Heightened Harmony, FALSE otherwise
 */
bool has_bard_heightened_harmony(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_HEIGHTENED_HARMONY);
}

/**
 * Get perform skill bonus from Heightened Harmony.
 * 
 * @param ch The character
 * @return Perform skill bonus (+5 if has perk and buff is active, 0 otherwise)
 */
int get_bard_heightened_harmony_perform_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_bard_heightened_harmony(ch) && affected_by_spell(ch, PERK_BARD_HEIGHTENED_HARMONY))
    return 5; /* +5 to perform skill */
  
  return 0;
}

/**
 * Check if character has Protective Chorus perk.
 * Grants defensive bonuses to allies under bard songs.
 * 
 * @param ch The character
 * @return TRUE if has Protective Chorus, FALSE otherwise
 */
bool has_bard_protective_chorus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_PROTECTIVE_CHORUS);
}

/**
 * Get save bonus from Protective Chorus.
 * 
 * @param ch The character
 * @return Save bonus vs. spells (+2 if has perk, 0 otherwise)
 */
int get_bard_protective_chorus_save_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_bard_protective_chorus(ch))
    return 2; /* +2 to saves vs. spells */
  
  return 0;
}

/**
 * Get AC bonus from Protective Chorus.
 * 
 * @param ch The character
 * @return AC bonus vs. attacks of opportunity (+2 if has perk, 0 otherwise)
 */
int get_bard_protective_chorus_ac_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_bard_protective_chorus(ch))
    return 2; /* +2 AC vs. attacks of opportunity */
  
  return 0;
}

/* ============================================================================
 * TIER IV SPELLSINGER PERK FUNCTIONS - CAPSTONES
 * ============================================================================ */

/**
 * Check if character has Spellsong Maestra perk.
 * Grants +2 caster level and DC to bard spells while performing.
 * 
 * @param ch The character
 * @return TRUE if has Spellsong Maestra, FALSE otherwise
 */
bool has_bard_spellsong_maestra(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_SPELLSONG_MAESTRA);
}

/**
 * Get caster level bonus from Spellsong Maestra.
 * Only applies while performing.
 * 
 * @param ch The character
 * @return Caster level bonus (+2 if performing with perk, 0 otherwise)
 */
int get_bard_spellsong_maestra_caster_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_bard_spellsong_maestra(ch) && IS_PERFORMING(ch))
    return 2; /* +2 caster level */
  
  return 0;
}

/**
 * Get spell DC bonus from Spellsong Maestra.
 * Only applies to bard spells while performing.
 * 
 * @param ch The character
 * @return Spell DC bonus (+2 if performing with perk, 0 otherwise)
 */
int get_bard_spellsong_maestra_dc_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_bard_spellsong_maestra(ch) && IS_PERFORMING(ch))
    return 2; /* +2 spell DC */
  
  return 0;
}

/**
 * Check if Spellsong Maestra allows free metamagic on bard spells.
 * When active during performance, metamagic on bard spells costs nothing.
 * 
 * @param ch The character
 * @return TRUE if metamagic is free on bard spells, FALSE otherwise
 */
bool has_bard_spellsong_maestra_metamagic_free(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  if (has_bard_spellsong_maestra(ch) && IS_PERFORMING(ch))
    return TRUE; /* Metamagic on bard spells is free while performing */
  
  return FALSE;
}

/**
 * Check if character has Aria of Stasis perk.
 * Grants save bonuses and movement penalties to foes.
 * 
 * @param ch The character
 * @return TRUE if has Aria of Stasis, FALSE otherwise
 */
bool has_bard_aria_of_stasis(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_ARIA_OF_STASIS);
}

/**
 * Get save bonus from Aria of Stasis for allies.
 * Applies +4 to all saves.
 * 
 * @param ch The character
 * @return Save bonus (+4 if has perk, 0 otherwise)
 */
int get_bard_aria_stasis_ally_saves_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_bard_aria_of_stasis(ch))
    return 4; /* +4 to all saves */
  
  return 0;
}

/**
 * Get to-hit penalty from Aria of Stasis for enemies.
 * Applies -2 to hit penalty.
 * 
 * @param ch The character
 * @return To-hit penalty (-2 if has perk, 0 otherwise)
 */
int get_bard_aria_stasis_enemy_tohit_penalty(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_bard_aria_of_stasis(ch))
    return -2; /* -2 to hit */
  
  return 0;
}

/**
 * Get movement speed penalty from Aria of Stasis for enemies.
 * Returns percentage reduction (10).
 * 
 * @param ch The character
 * @return Movement penalty percentage (10 if has perk, 0 otherwise)
 */
int get_bard_aria_stasis_movement_penalty(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_bard_aria_of_stasis(ch))
    return 10; /* 10% movement speed reduction */
  
  return 0;
}

/**
 * Check if character has Symphonic Resonance perk.
 * Grants temp HP and daze effect on certain spells.
 * 
 * @param ch The character
 * @return TRUE if has Symphonic Resonance, FALSE otherwise
 */
bool has_bard_symphonic_resonance(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_SYMPHONIC_RESONANCE);
}

/**
 * Get temporary HP from Symphonic Resonance per round.
 * Returns 1d6.
 * 
 * @param ch The character
 * @return Number of d6 dice for temp HP (1 if has perk, 0 otherwise)
 */
int get_bard_symphonic_resonance_temp_hp(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_bard_symphonic_resonance(ch))
    return 1; /* 1d6 temp HP per round */
  
  return 0;
}

/**
 * Get daze duration from Symphonic Resonance.
 * Returns number of rounds for daze effect.
 * 
 * @param ch The character
 * @return Daze duration in rounds (1 if has perk, 0 otherwise)
 */
int get_bard_symphonic_resonance_daze_duration(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_bard_symphonic_resonance(ch))
    return 1; /* 1 round daze */
  
  return 0;
}

/**
 * Get daze range from Symphonic Resonance in feet.
 * 
 * @param ch The character
 * @return Daze range in feet (20 if has perk, 0 otherwise)
 */
int get_bard_symphonic_resonance_daze_range(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_bard_symphonic_resonance(ch))
    return 20; /* 20 feet range */
  
  return 0;
}

/**
 * Check if character has Endless Refrain perk.
 * Makes performance free and regenerates spell slots.
 * 
 * @param ch The character
 * @return TRUE if has Endless Refrain, FALSE otherwise
 */
bool has_bard_endless_refrain(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_ENDLESS_REFRAIN);
}

/**
 * Get spell slot regeneration from Endless Refrain.
 * Returns number of spell slots regenerated per round.
 * 
 * @param ch The character
 * @return Spell slots regenerated per round (1 if has perk, 0 otherwise)
 */
int get_bard_endless_refrain_slot_regen(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (has_bard_endless_refrain(ch))
    return 1; /* Regenerate 1 spell slot per round */
  
  return 0;
}

/**
 * Check if Endless Refrain allows performance to continue without consuming resources.
 * When active, performance costs nothing and doesn't consume rounds.
 * 
 * @param ch The character
 * @return FALSE if Endless Refrain is active (don't consume), TRUE otherwise
 */
bool should_endless_refrain_consume_performance(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return TRUE;
  
  if (has_bard_endless_refrain(ch) && IS_PERFORMING(ch))
    return FALSE; /* Don't consume performance */
  
  return TRUE; /* Normal bards consume performance */
}
/* ============================================================================
 * WARCHANTER TREE PERK FUNCTIONS
 * ============================================================================ */

/**
 * Get damage bonus from Battle Hymn I.
 * Provides +1 competence damage per rank to Inspire Courage recipients.
 * 
 * @param ch The character
 * @return Damage bonus per rank (cumulative)
 */
int get_bard_battle_hymn_damage_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Battle Hymn I: +1 competence to damage per rank */
  bonus += get_perk_rank(ch, PERK_BARD_BATTLE_HYMN_I, CLASS_BARD);
  
  return bonus;
}

/**
 * Get to-hit bonus from Drummer's Rhythm I.
 * Provides +1 to-hit per rank while performing.
 * 
 * @param ch The character
 * @return To-hit bonus per rank (cumulative)
 */
int get_bard_drummers_rhythm_tohit_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!IS_PERFORMING(ch))
    return 0;
  
  /* Drummer's Rhythm I: +1 melee to-hit per rank while performing */
  bonus += get_perk_rank(ch, PERK_BARD_DRUMMERS_RHYTHM_I, CLASS_BARD);
  
  return bonus;
}

/**
 * Check if character has Rallying Cry perk.
 * Allows removing shaken condition and granting fear save bonuses to allies.
 * 
 * @param ch The character
 * @return TRUE if has Rallying Cry, FALSE otherwise
 */
bool has_bard_rallying_cry_perk(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_RALLYING_CRY);
}

/**
 * Get fear save bonus from Rallying Cry effect.
 * Returns morale bonus to saves vs. fear.
 * 
 * @param ch The character
 * @return +2 if affected by Rallying Cry, 0 otherwise
 */
int get_bard_rallying_cry_fear_save_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Check if character is affected by AFFECT_RALLYING_CRY */
  if (affected_by_spell(ch, AFFECT_RALLYING_CRY))
    return 2; /* +2 morale to fear saves */
  
  return 0;
}

/**
 * Check if character has Frostbite Refrain I perk.
 * Adds cold damage to melee hits and applies debuff on natural 20.
 * 
 * @param ch The character
 * @return TRUE if has Frostbite Refrain I, FALSE otherwise
 */
bool has_bard_frostbite_refrain(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_FROSTBITE_REFRAIN_I);
}

/**
 * Get cold damage bonus from Frostbite Refrain I.
 * Returns +1 cold damage per rank while performing.
 * 
 * @param ch The character
 * @return Cold damage bonus per rank (cumulative)
 */
int get_bard_frostbite_cold_damage(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!IS_PERFORMING(ch))
    return 0;
  
  if (!has_bard_frostbite_refrain(ch))
    return 0;
  
  /* Frostbite Refrain I: +1 cold damage per rank while performing */
  bonus += get_perk_rank(ch, PERK_BARD_FROSTBITE_REFRAIN_I, CLASS_BARD);
  
  return bonus;
}

/**
 * Get natural 20 debuff modifier from Frostbite Refrain I.
 * Returns the to-hit penalty applied on natural 20 hit.
 * 
 * @param ch The character
 * @return -1 to attack (as specified in effect_modifier)
 */
int get_bard_frostbite_natural_20_debuff(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!IS_PERFORMING(ch))
    return 0;
  
  if (!has_bard_frostbite_refrain(ch))
    return 0;
  
  /* Frostbite Refrain I: natural 20 applies -1 to attack debuff */
  return -1;
}

/* ============================================================================
 * WARCHANTER TREE TIER 2 PERK FUNCTIONS
 * ============================================================================ */

/**
 * Get additional damage bonus from Battle Hymn II.
 * Provides additional +1 competence damage per rank to Inspire Courage recipients (stacks with Tier 1).
 * 
 * @param ch The character
 * @return Additional damage bonus per rank (cumulative)
 */
int get_bard_battle_hymn_ii_damage_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  /* Battle Hymn II: Additional +1 competence to damage per rank */
  bonus += get_perk_rank(ch, PERK_BARD_BATTLE_HYMN_II, CLASS_BARD);
  
  return bonus;
}

/**
 * Get additional to-hit bonus from Drummer's Rhythm II.
 * Provides additional +1 to-hit per rank while performing (stacks with Tier 1).
 * 
 * @param ch The character
 * @return Additional to-hit bonus per rank (cumulative)
 */
int get_bard_drummers_rhythm_ii_tohit_bonus(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!IS_PERFORMING(ch))
    return 0;
  
  /* Drummer's Rhythm II: Additional +1 melee to-hit per rank while performing */
  bonus += get_perk_rank(ch, PERK_BARD_DRUMMERS_RHYTHM_II, CLASS_BARD);
  
  return bonus;
}

/**
 * Check if character has Warbeat perk.
 * Grants first-turn-in-combat extra melee attack and ally damage buff on hit.
 * 
 * @param ch The character
 * @return TRUE if has Warbeat, FALSE otherwise
 */
bool has_bard_warbeat(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_WARBEAT);
}

/**
 * Get ally damage bonus from Warbeat perk.
 * Returns the number of d4 dice for the damage bonus to allies.
 * 
 * @param ch The character
 * @return 1 (for 1d4 damage bonus), 0 if not applicable
 */
int get_bard_warbeat_ally_damage_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!has_bard_warbeat(ch))
    return 0;
  
  /* Warbeat: On hit, grant allies +1d4 damage for 2 rounds */
  return 1; /* 1d4 */
}

/**
 * Check if character has Frostbite Refrain II perk.
 * Adds enhanced cold damage to melee hits with upgraded natural 20 debuff.
 * 
 * @param ch The character
 * @return TRUE if has Frostbite Refrain II, FALSE otherwise
 */
bool has_bard_frostbite_refrain_ii(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_FROSTBITE_REFRAIN_II);
}

/**
 * Get cold damage bonus from Frostbite Refrain II.
 * Returns additional +1 cold damage per rank while performing (stacks with Tier 1).
 * 
 * @param ch The character
 * @return Cold damage bonus per rank (cumulative)
 */
int get_bard_frostbite_refrain_ii_cold_damage(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!IS_PERFORMING(ch))
    return 0;
  
  if (!has_bard_frostbite_refrain_ii(ch))
    return 0;
  
  /* Frostbite Refrain II: Additional +1 cold damage per rank while performing */
  bonus += get_perk_rank(ch, PERK_BARD_FROSTBITE_REFRAIN_II, CLASS_BARD);
  
  return bonus;
}

/**
 * Get natural 20 to-hit debuff from Frostbite Refrain II.
 * Returns the enhanced to-attack penalty applied on natural 20 hit.
 * 
 * @param ch The character
 * @return -2 to attack (upgraded from Tier 1's -1)
 */
int get_bard_frostbite_refrain_ii_natural_20_debuff_attack(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!IS_PERFORMING(ch))
    return 0;
  
  if (!has_bard_frostbite_refrain_ii(ch))
    return 0;
  
  /* Frostbite Refrain II: natural 20 applies -2 to attack debuff (upgraded from Tier 1) */
  return -2;
}

/**
 * Get natural 20 AC debuff from Frostbite Refrain II.
 * Returns the AC penalty applied on natural 20 hit (new in Tier 2).
 * 
 * @param ch The character
 * @return -1 to AC for 1 round
 */
int get_bard_frostbite_refrain_ii_natural_20_debuff_ac(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!IS_PERFORMING(ch))
    return 0;
  
  if (!has_bard_frostbite_refrain_ii(ch))
    return 0;
  
  /* Frostbite Refrain II: natural 20 also applies -1 to AC debuff (new effect) */
  return -1;
}

/* ============================================================================
 * WARCHANTER TREE TIER 3 PERK FUNCTIONS
 * ============================================================================ */

/**
 * Check if character has Anthem of Fortitude perk.
 * Grants allies +10% max HP and +2 to Fortitude saves while performing.
 * 
 * @param ch The character
 * @return TRUE if has Anthem of Fortitude, FALSE otherwise
 */
bool has_bard_anthem_of_fortitude(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  if (!IS_PERFORMING(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_ANTHEM_OF_FORTITUDE);
}

/**
 * Get max HP bonus from Anthem of Fortitude for allies.
 * Returns the percentage of max HP bonus.
 * 
 * @param ch The character
 * @return 10 (for 10% max HP bonus)
 */
int get_bard_anthem_fortitude_hp_bonus(struct char_data *ch)
{
  if (!has_bard_anthem_of_fortitude(ch))
    return 0;
  
  /* Anthem of Fortitude: +10% max HP */
  return 10;
}

/**
 * Get Fortitude save bonus from Anthem of Fortitude for allies.
 * Returns the morale bonus to Fortitude saves.
 * 
 * @param ch The character
 * @return +2 to Fortitude saves
 */
int get_bard_anthem_fortitude_save_bonus(struct char_data *ch)
{
  if (!has_bard_anthem_of_fortitude(ch))
    return 0;
  
  /* Anthem of Fortitude: +2 to Fortitude saves */
  return 2;
}

/**
 * Check if character has Commanding Cadence perk.
 * Enemies hit in melee must save or be dazed.
 * 
 * @param ch The character
 * @return TRUE if has Commanding Cadence, FALSE otherwise
 */
bool has_bard_commanding_cadence(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_COMMANDING_CADENCE);
}

/**
 * Get daze chance (save DC) from Commanding Cadence.
 * Returns the Will save DC that enemies must beat to avoid daze.
 * 
 * @param ch The character
 * @return 1 (flag that daze should be applied if save fails)
 */
int get_bard_commanding_cadence_daze_chance(struct char_data *ch)
{
  if (!has_bard_commanding_cadence(ch))
    return 0;
  
  /* Commanding Cadence: 1 indicates daze should be applied */
  return 1;
}

/**
 * Check if character has Steel Serenade perk.
 * Grants +2 natural AC and 10% physical damage resistance while performing.
 * 
 * @param ch The character
 * @return TRUE if has Steel Serenade, FALSE otherwise
 */
bool has_bard_steel_serenade(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  if (!IS_PERFORMING(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_STEEL_SERENADE);
}

/**
 * Get natural AC bonus from Steel Serenade.
 * Returns the AC improvement while performing.
 * 
 * @param ch The character
 * @return +2 AC (note: lower is better in this system)
 */
int get_bard_steel_serenade_ac_bonus(struct char_data *ch)
{
  if (!has_bard_steel_serenade(ch))
    return 0;
  
  /* Steel Serenade: +2 natural AC (2 point improvement) */
  return 2;
}

/**
 * Get physical damage resistance from Steel Serenade.
 * Returns the percentage resistance to physical damage types.
 * 
 * @param ch The character
 * @return 10 (for 10% physical damage resistance)
 */
int get_bard_steel_serenade_damage_resistance(struct char_data *ch)
{
  if (!has_bard_steel_serenade(ch))
    return 0;
  
  /* Steel Serenade: 10% physical damage resistance */
  return 10;
}

/**
 * Check if character has Banner Verse perk.
 * Plants a musical standard that grants allies bonuses.
 * 
 * @param ch The character
 * @return TRUE if has Banner Verse, FALSE otherwise
 */
bool has_bard_banner_verse(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_BANNER_VERSE);
}

/**
 * Get to-hit bonus from Banner Verse for allies in the room.
 * Returns the competence bonus to hit.
 * 
 * @param ch The character
 * @return +2 to hit
 */
int get_bard_banner_verse_tohit_bonus(struct char_data *ch)
{
  if (!has_bard_banner_verse(ch))
    return 0;
  
  /* Banner Verse: +2 to hit in the room */
  return 2;
}

/**
 * Get save bonus from Banner Verse for allies in the room.
 * Returns the morale bonus to all saves.
 * 
 * @param ch The character
 * @return +2 to all saves
 */
int get_bard_banner_verse_save_bonus(struct char_data *ch)
{
  if (!has_bard_banner_verse(ch))
    return 0;
  
  /* Banner Verse: +2 to all saves in the room */
  return 2;
}

/* ============================================================================
 * BARD WARCHANTER TIER 4 HELPER FUNCTIONS
 * ============================================================================ */

/**
 * Check if character has Warchanter's Dominance perk.
 * Capstone that enhances Inspire Courage and Warbeat effects.
 * 
 * @param ch The character
 * @return TRUE if has perk, FALSE otherwise
 */
bool has_bard_warchanters_dominance(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  if (!IS_PERFORMING(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_WARCHANTERS_DOMINANCE);
}

/**
 * Get to-hit bonus from Warchanter's Dominance.
 * Inspire Courage grants +1 additional attack bonus.
 * 
 * @param ch The character
 * @return +1 to hit while performing
 */
int get_bard_warchanters_dominance_tohit_bonus(struct char_data *ch)
{
  if (!has_bard_warchanters_dominance(ch))
    return 0;
  
  /* Warchanter's Dominance: +1 to hit from Inspire Courage enhancement */
  return 1;
}

/**
 * Get AC bonus from Warchanter's Dominance.
 * Inspire Courage grants +1 additional AC.
 * 
 * @param ch The character
 * @return +1 AC while performing (negative value for AC system)
 */
int get_bard_warchanters_dominance_ac_bonus(struct char_data *ch)
{
  if (!has_bard_warchanters_dominance(ch))
    return 0;
  
  /* Warchanter's Dominance: +1 AC from Inspire Courage enhancement */
  return 1;
}

/**
 * Get damage bonus from Warchanter's Dominance.
 * Enhance damage output on first attack (Warbeat enhancement).
 * 
 * @param ch The character
 * @return +1 to damage
 */
int get_bard_warchanters_dominance_damage_bonus(struct char_data *ch)
{
  if (!has_bard_warchanters_dominance(ch))
    return 0;
  
  /* Warchanter's Dominance: +1 to damage via Warbeat enhancement */
  return 1;
}

/**
 * Check if character has Winter's War March perk.
 * Room-wide martial anthem that damages and slows enemies.
 * 
 * @param ch The character
 * @return TRUE if has perk, FALSE otherwise
 */
bool has_bard_winters_war_march(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  if (!IS_PERFORMING(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_WINTERS_WAR_MARCH);
}

/**
 * Get damage from Winter's War March ability.
 * Returns number of dice for 4d6 damage.
 * 
 * @param ch The character
 * @return 4 (for 4d6 cold damage)
 */
int get_bard_winters_war_march_damage(struct char_data *ch)
{
  if (!has_bard_winters_war_march(ch))
    return 0;
  
  /* Winter's War March: 4d6 cold damage */
  return 4;
}

/* ============================================================================
 * SWASHBUCKLER TREE PERK FUNCTIONS
 * ============================================================================ */

/**
 * Check if character has Fencer's Footwork I perk.
 * Grants +1 dodge AC and +1 reflex save per rank with finesse/single weapon.
 * 
 * @param ch The character
 * @return TRUE if has Fencer's Footwork I, FALSE otherwise
 */
bool has_bard_fencers_footwork_i(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_FENCERS_FOOTWORK_I);
}

/**
 * Get AC dodge bonus from Fencer's Footwork I.
 * Returns +1 dodge AC per rank while wielding finesse or single one-handed weapon.
 * 
 * @param ch The character
 * @return Dodge AC bonus per rank (cumulative)
 */
int get_bard_fencers_footwork_ac_bonus(struct char_data *ch)
{
  struct obj_data *wielded;
  int bonus = 0;
  int weapon_type;
  bool is_finesse = FALSE;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!has_bard_fencers_footwork_i(ch))
    return 0;
  
  /* Check if wielding a finesse weapon or single one-handed weapon */
  wielded = GET_EQ(ch, WEAR_WIELD_1);
  if (!wielded)
    return 0;
  
  if (GET_OBJ_TYPE(wielded) != ITEM_WEAPON)
    return 0;
  
  weapon_type = GET_OBJ_VAL(wielded, 0);
  
  /* Check if finesse weapon: size < wielder OR has WEAPON_FLAG_BALANCED */
  if (GET_OBJ_SIZE(wielded) < GET_SIZE(ch))
    is_finesse = TRUE;
  if (IS_SET(weapon_list[weapon_type].weaponFlags, WEAPON_FLAG_BALANCED))
    is_finesse = TRUE;
  
  /* Grant bonus if finesse weapon */
  if (is_finesse)
  {
    bonus = get_perk_rank(ch, PERK_BARD_FENCERS_FOOTWORK_I, CLASS_BARD);
    return bonus;
  }
  
  /* Check if single one-handed weapon (no offhand weapon) */
  if (!GET_EQ(ch, WEAR_WIELD_OFFHAND))
  {
    int size = GET_OBJ_SIZE(wielded);
    if (size <= SIZE_SMALL || size == SIZE_MEDIUM)
    {
      bonus = get_perk_rank(ch, PERK_BARD_FENCERS_FOOTWORK_I, CLASS_BARD);
      return bonus;
    }
  }
  
  return 0;
}

/**
 * Get Reflex save bonus from Fencer's Footwork I.
 * Returns +1 reflex save per rank while wielding finesse/single weapon.
 * 
 * @param ch The character
 * @return Reflex save bonus per rank (cumulative)
 */
int get_bard_fencers_footwork_reflex_bonus(struct char_data *ch)
{
  struct obj_data *wielded;
  int bonus = 0;
  int weapon_type;
  bool is_finesse = FALSE;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!has_bard_fencers_footwork_i(ch))
    return 0;
  
  /* Check if wielding a finesse weapon or single one-handed weapon */
  wielded = GET_EQ(ch, WEAR_WIELD_1);
  if (!wielded)
    return 0;
  
  if (GET_OBJ_TYPE(wielded) != ITEM_WEAPON)
    return 0;
  
  weapon_type = GET_OBJ_VAL(wielded, 0);
  
  /* Check if finesse weapon: size < wielder OR has WEAPON_FLAG_BALANCED */
  if (GET_OBJ_SIZE(wielded) < GET_SIZE(ch))
    is_finesse = TRUE;
  if (IS_SET(weapon_list[weapon_type].weaponFlags, WEAPON_FLAG_BALANCED))
    is_finesse = TRUE;
  
  /* Grant bonus if finesse weapon */
  if (is_finesse)
  {
    bonus = get_perk_rank(ch, PERK_BARD_FENCERS_FOOTWORK_I, CLASS_BARD);
    return bonus;
  }
  
  /* Check if single one-handed weapon (no offhand weapon) */
  if (!GET_EQ(ch, WEAR_WIELD_OFFHAND))
  {
    int size = GET_OBJ_SIZE(wielded);
    if (size <= SIZE_SMALL || size == SIZE_MEDIUM)
    {
      bonus = get_perk_rank(ch, PERK_BARD_FENCERS_FOOTWORK_I, CLASS_BARD);
      return bonus;
    }
  }
  
  return 0;
}

/**
 * Check if character has Precise Strike I perk.
 * Grants +1 precision damage per rank with finesse/one-handed piercing/slashing.
 * 
 * @param ch The character
 * @return TRUE if has Precise Strike I, FALSE otherwise
 */
bool has_bard_precise_strike_i(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_PRECISE_STRIKE_I);
}

/**
 * Get precision damage bonus from Precise Strike I.
 * Returns +1 precision damage per rank with finesse or one-handed piercing/slashing.
 * 
 * @param ch The character
 * @return Precision damage bonus per rank (not multiplied on crits)
 */
int get_bard_precise_strike_i_bonus(struct char_data *ch)
{
  struct obj_data *wielded;
  int bonus = 0;
  int weapon_type;
  int damage_type;
  bool is_finesse = FALSE;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!has_bard_precise_strike_i(ch))
    return 0;
  
  /* Check if wielding appropriate weapon */
  wielded = GET_EQ(ch, WEAR_WIELD_1);
  if (!wielded || GET_OBJ_TYPE(wielded) != ITEM_WEAPON)
    return 0;
  
  weapon_type = GET_OBJ_VAL(wielded, 0);
  damage_type = GET_OBJ_VAL(wielded, 3);
  
  /* Check if finesse weapon: size < wielder OR has WEAPON_FLAG_BALANCED */
  if (GET_OBJ_SIZE(wielded) < GET_SIZE(ch))
    is_finesse = TRUE;
  if (IS_SET(weapon_list[weapon_type].weaponFlags, WEAPON_FLAG_BALANCED))
    is_finesse = TRUE;
  
  /* Check if finesse weapon with correct damage type */
  if (is_finesse)
  {
    /* Check if piercing or slashing */
    if (damage_type == DAMAGE_TYPE_PIERCING || damage_type == DAMAGE_TYPE_SLASHING)
    {
      bonus = get_perk_rank(ch, PERK_BARD_PRECISE_STRIKE_I, CLASS_BARD);
      return bonus;
    }
  }
  
  /* Check if single one-handed weapon with correct damage type */
  if (!GET_EQ(ch, WEAR_WIELD_OFFHAND))
  {
    int size = GET_OBJ_SIZE(wielded);
    if ((size <= SIZE_SMALL || size == SIZE_MEDIUM) &&
        (damage_type == DAMAGE_TYPE_PIERCING || damage_type == DAMAGE_TYPE_SLASHING))
    {
      bonus = get_perk_rank(ch, PERK_BARD_PRECISE_STRIKE_I, CLASS_BARD);
      return bonus;
    }
  }
  
  return 0;
}

/**
 * Check if character has Riposte Training I perk.
 * Grants 3% chance per rank to counterattack after dodging/parrying.
 * 
 * @param ch The character
 * @return TRUE if has Riposte Training I, FALSE otherwise
 */
bool has_bard_riposte_training_i(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_RIPOSTE_TRAINING_I);
}

/**
 * Get riposte chance from Riposte Training I.
 * Returns 3% per rank chance to counterattack.
 * 
 * @param ch The character
 * @return Riposte chance percentage per rank (3% per rank)
 */
int get_bard_riposte_training_i_chance(struct char_data *ch)
{
  int bonus = 0;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!has_bard_riposte_training_i(ch))
    return 0;
  
  /* Riposte Training I: 3% per rank */
  bonus = 3 * get_perk_rank(ch, PERK_BARD_RIPOSTE_TRAINING_I, CLASS_BARD);
  
  return bonus;
}

/**
 * SWASHBUCKLER TREE TIER 2 PERK FUNCTIONS
 */

/**
 * Check if character has Fencer's Footwork II perk.
 * Grants additional +1 Dodge AC and +1 Reflex per rank.
 * 
 * @param ch The character
 * @return TRUE if has Fencer's Footwork II, FALSE otherwise
 */
bool has_bard_fencers_footwork_ii(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_FENCERS_FOOTWORK_II);
}

/**
 * Get Fencer's Footwork II AC bonus.
 * Returns +1 Dodge AC per rank when wielding finesse or single one-handed weapon.
 * Stacks with Tier I bonus.
 * 
 * @param ch The character
 * @return AC bonus (0-2)
 */
int get_bard_fencers_footwork_ii_ac_bonus(struct char_data *ch)
{
  struct obj_data *wielded;
  int bonus = 0;
  int weapon_type;
  bool is_finesse = FALSE;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!has_bard_fencers_footwork_ii(ch))
    return 0;
  
  /* Check if wielding a finesse weapon or single one-handed weapon */
  wielded = GET_EQ(ch, WEAR_WIELD_1);
  if (!wielded)
    return 0;
  
  if (GET_OBJ_TYPE(wielded) != ITEM_WEAPON)
    return 0;
  
  weapon_type = GET_OBJ_VAL(wielded, 0);
  
  /* Check if finesse weapon: size < wielder OR has WEAPON_FLAG_BALANCED */
  if (GET_OBJ_SIZE(wielded) < GET_SIZE(ch))
    is_finesse = TRUE;
  if (IS_SET(weapon_list[weapon_type].weaponFlags, WEAPON_FLAG_BALANCED))
    is_finesse = TRUE;
  
  /* Grant bonus if finesse weapon */
  if (is_finesse)
  {
    bonus = get_perk_rank(ch, PERK_BARD_FENCERS_FOOTWORK_II, CLASS_BARD);
    return bonus;
  }
  
  /* Check if single one-handed weapon (no offhand weapon) */
  if (!GET_EQ(ch, WEAR_WIELD_OFFHAND))
  {
    int size = GET_OBJ_SIZE(wielded);
    if (size <= SIZE_SMALL || size == SIZE_MEDIUM)
    {
      bonus = get_perk_rank(ch, PERK_BARD_FENCERS_FOOTWORK_II, CLASS_BARD);
      return bonus;
    }
  }
  
  return 0;
}

/**
 * Get Fencer's Footwork II Reflex save bonus.
 * Returns +1 Reflex per rank when wielding finesse or single one-handed weapon.
 * Stacks with Tier I bonus.
 * 
 * @param ch The character
 * @return Reflex save bonus (0-2)
 */
int get_bard_fencers_footwork_ii_reflex_bonus(struct char_data *ch)
{
  struct obj_data *wielded;
  int bonus = 0;
  int weapon_type;
  bool is_finesse = FALSE;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!has_bard_fencers_footwork_ii(ch))
    return 0;
  
  /* Check if wielding a finesse weapon or single one-handed weapon */
  wielded = GET_EQ(ch, WEAR_WIELD_1);
  if (!wielded)
    return 0;
  
  if (GET_OBJ_TYPE(wielded) != ITEM_WEAPON)
    return 0;
  
  weapon_type = GET_OBJ_VAL(wielded, 0);
  
  /* Check if finesse weapon: size < wielder OR has WEAPON_FLAG_BALANCED */
  if (GET_OBJ_SIZE(wielded) < GET_SIZE(ch))
    is_finesse = TRUE;
  if (IS_SET(weapon_list[weapon_type].weaponFlags, WEAPON_FLAG_BALANCED))
    is_finesse = TRUE;
  
  /* Grant bonus if finesse weapon */
  if (is_finesse)
  {
    bonus = get_perk_rank(ch, PERK_BARD_FENCERS_FOOTWORK_II, CLASS_BARD);
    return bonus;
  }
  
  /* Check if single one-handed weapon (no offhand weapon) */
  if (!GET_EQ(ch, WEAR_WIELD_OFFHAND))
  {
    int size = GET_OBJ_SIZE(wielded);
    if (size <= SIZE_SMALL || size == SIZE_MEDIUM)
    {
      bonus = get_perk_rank(ch, PERK_BARD_FENCERS_FOOTWORK_II, CLASS_BARD);
      return bonus;
    }
  }
  
  return 0;
}

/**
 * Check if character has Precise Strike II perk.
 * Grants additional +1 precision damage per rank.
 * 
 * @param ch The character
 * @return TRUE if has Precise Strike II, FALSE otherwise
 */
bool has_bard_precise_strike_ii(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_PRECISE_STRIKE_II);
}

/**
 * Get Precise Strike II precision damage bonus.
 * Returns +1 precision damage per rank with finesse or one-handed piercing/slashing weapons.
 * Stacks with Tier I bonus, not multiplied on crits.
 * 
 * @param ch The character
 * @return Precision damage bonus (0-2)
 */
int get_bard_precise_strike_ii_bonus(struct char_data *ch)
{
  struct obj_data *wielded;
  int bonus = 0;
  int weapon_type;
  int damage_type;
  bool is_finesse = FALSE;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!has_bard_precise_strike_ii(ch))
    return 0;
  
  /* Check if wielding appropriate weapon */
  wielded = GET_EQ(ch, WEAR_WIELD_1);
  if (!wielded || GET_OBJ_TYPE(wielded) != ITEM_WEAPON)
    return 0;
  
  weapon_type = GET_OBJ_VAL(wielded, 0);
  damage_type = GET_OBJ_VAL(wielded, 3);
  
  /* Check if finesse weapon: size < wielder OR has WEAPON_FLAG_BALANCED */
  if (GET_OBJ_SIZE(wielded) < GET_SIZE(ch))
    is_finesse = TRUE;
  if (IS_SET(weapon_list[weapon_type].weaponFlags, WEAPON_FLAG_BALANCED))
    is_finesse = TRUE;
  
  /* Check if finesse weapon with correct damage type */
  if (is_finesse)
  {
    /* Check if piercing or slashing */
    if (damage_type == DAMAGE_TYPE_PIERCING || damage_type == DAMAGE_TYPE_SLASHING)
    {
      bonus = get_perk_rank(ch, PERK_BARD_PRECISE_STRIKE_II, CLASS_BARD);
      return bonus;
    }
  }
  
  /* Check if single one-handed weapon with correct damage type */
  if (!GET_EQ(ch, WEAR_WIELD_OFFHAND))
  {
    int size = GET_OBJ_SIZE(wielded);
    if ((size <= SIZE_SMALL || size == SIZE_MEDIUM) &&
        (damage_type == DAMAGE_TYPE_PIERCING || damage_type == DAMAGE_TYPE_SLASHING))
    {
      bonus = get_perk_rank(ch, PERK_BARD_PRECISE_STRIKE_II, CLASS_BARD);
      return bonus;
    }
  }
  
  return 0;
}

/**
 * Check if character has Duelist's Poise perk.
 * Grants +2 to critical confirmation and +1 critical threat range with finesse weapon.
 * 
 * @param ch The character
 * @return TRUE if has Duelist's Poise, FALSE otherwise
 */
bool has_bard_duelists_poise(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_DUELISTS_POISE);
}

/**
 * Get Duelist's Poise critical confirmation bonus.
 * Returns +2 to critical confirmation rolls when using a finesse weapon.
 * 
 * @param ch The character
 * @return Critical confirmation bonus (0 or 2)
 */
int get_bard_duelists_poise_crit_confirm_bonus(struct char_data *ch)
{
  struct obj_data *wielded;
  int weapon_type;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!has_bard_duelists_poise(ch))
    return 0;
  
  /* Check if wielding a finesse weapon */
  wielded = GET_EQ(ch, WEAR_WIELD_1);
  if (!wielded || GET_OBJ_TYPE(wielded) != ITEM_WEAPON)
    return 0;
  
  weapon_type = GET_OBJ_VAL(wielded, 0);
  
  /* Check if finesse weapon: size < wielder OR has WEAPON_FLAG_BALANCED */
  if (GET_OBJ_SIZE(wielded) < GET_SIZE(ch))
    return 2;
  if (IS_SET(weapon_list[weapon_type].weaponFlags, WEAPON_FLAG_BALANCED))
    return 2;
  
  return 0;
}

/**
 * Get Duelist's Poise critical threat range bonus.
 * Returns +1 to critical threat range when using a finesse weapon.
 * 
 * @param ch The character
 * @return Critical threat range bonus (0 or 1)
 */
int get_bard_duelists_poise_threat_range_bonus(struct char_data *ch)
{
  struct obj_data *wielded;
  int weapon_type;
  
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!has_bard_duelists_poise(ch))
    return 0;
  
  /* Check if wielding a finesse weapon */
  wielded = GET_EQ(ch, WEAR_WIELD_1);
  if (!wielded || GET_OBJ_TYPE(wielded) != ITEM_WEAPON)
    return 0;
  
  weapon_type = GET_OBJ_VAL(wielded, 0);
  
  /* Check if finesse weapon: size < wielder OR has WEAPON_FLAG_BALANCED */
  if (GET_OBJ_SIZE(wielded) < GET_SIZE(ch))
    return 1;
  if (IS_SET(weapon_list[weapon_type].weaponFlags, WEAPON_FLAG_BALANCED))
    return 1;
  
  return 0;
}

/**
 * Check if character has Agile Disengage perk.
 * Grants +4 AC for 3 rounds on failed flee (ends if moving rooms).
 * 
 * @param ch The character
 * @return TRUE if has Agile Disengage, FALSE otherwise
 */
bool has_bard_agile_disengage(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_AGILE_DISENGAGE);
}

/**
 * Check if character is affected by Agile Disengage buff.
 * 
 * @param ch The character
 * @return TRUE if affected by Agile Disengage, FALSE otherwise
 */
bool is_affected_by_agile_disengage(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  /* Check for AFFECT_BARD_AGILE_DISENGAGE affect */
  return affected_by_spell(ch, AFFECT_BARD_AGILE_DISENGAGE);
}

/**
 * Get Agile Disengage AC bonus.
 * Returns +4 AC if affected by Agile Disengage buff.
 * 
 * @param ch The character
 * @return AC bonus (0 or 4)
 */
int get_bard_agile_disengage_ac_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!is_affected_by_agile_disengage(ch))
    return 0;
  
  return 4;
}

/**
 * SWASHBUCKLER TREE TIER 3 PERK FUNCTIONS
 */

/**
 * Check if character has Perfect Tempo perk.
 * Grants +4 to hit and +2d6 damage on next attack if you avoid all melee hits for a round.
 * 
 * @param ch The character
 * @return TRUE if has Perfect Tempo, FALSE otherwise
 */
bool has_bard_perfect_tempo(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_PERFECT_TEMPO);
}

/**
 * Check if character is affected by Perfect Tempo buff.
 * 
 * @param ch The character
 * @return TRUE if affected by Perfect Tempo, FALSE otherwise
 */
bool is_affected_by_perfect_tempo(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return affected_by_spell(ch, AFFECT_BARD_PERFECT_TEMPO);
}

/**
 * Get Perfect Tempo to-hit bonus.
 * Returns +4 to hit when Perfect Tempo is active.
 * 
 * @param ch The character
 * @return To-hit bonus (0 or 4)
 */
int get_bard_perfect_tempo_tohit_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!is_affected_by_perfect_tempo(ch))
    return 0;
  
  return 4;
}

/**
 * Get Perfect Tempo precision damage bonus.
 * Returns 2d6 precision damage when Perfect Tempo is active.
 * 
 * @param ch The character
 * @return Precision damage bonus (0 or 2d6)
 */
int get_bard_perfect_tempo_damage_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!is_affected_by_perfect_tempo(ch))
    return 0;
  
  /* 2d6 damage */
  return dice(2, 6);
}

/**
 * Check if character has Showstopper perk.
 * Imposes AC and attack penalties on critical hit.
 * 
 * @param ch The character
 * @return TRUE if has Showstopper, FALSE otherwise
 */
bool has_bard_showstopper(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_SHOWSTOPPER);
}

/**
 * Check if character has Acrobatic Charge perk.
 * Allows charging through terrain and allies with +2 to hit.
 * 
 * @param ch The character
 * @return TRUE if has Acrobatic Charge, FALSE otherwise
 */
bool has_bard_acrobatic_charge(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_ACROBATIC_CHARGE);
}

/**
 * Get Acrobatic Charge to-hit bonus on charges.
 * Returns +2 to hit when using Acrobatic Charge ability.
 * 
 * @param ch The character
 * @return To-hit bonus (0 or 2)
 */
int get_bard_acrobatic_charge_tohit_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!has_bard_acrobatic_charge(ch))
    return 0;
  
  return 2;
}

/**
 * Check if character has Feint and Finish perk.
 * Grants +2d6 damage and +2 crit confirm on next attack after feint.
 * 
 * @param ch The character
 * @return TRUE if has Feint and Finish, FALSE otherwise
 */
bool has_bard_feint_and_finish(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_FEINT_AND_FINISH);
}

/**
 * Check if character is affected by Feint and Finish buff.
 * 
 * @param ch The character
 * @return TRUE if affected by Feint and Finish, FALSE otherwise
 */
bool is_affected_by_feint_and_finish(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return affected_by_spell(ch, AFFECT_BARD_FEINT_AND_FINISH);
}

/**
 * Get Feint and Finish precision damage bonus.
 * Returns +2d6 precision damage after successful feint.
 * 
 * @param ch The character
 * @return Precision damage bonus (0 or 2d6)
 */
int get_bard_feint_and_finish_damage_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!is_affected_by_feint_and_finish(ch))
    return 0;
  
  /* 2d6 damage */
  return dice(2, 6);
}

/**
 * Get Feint and Finish critical confirmation bonus.
 * Returns +2 to critical confirmation after successful feint.
 * 
 * @param ch The character
 * @return Crit confirmation bonus (0 or 2)
 */
int get_bard_feint_and_finish_crit_confirm_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!is_affected_by_feint_and_finish(ch))
    return 0;
  
  return 2;
}

/**
 * Check if character has Flourish perk.
 * Allows +2 to hit and +2 AC for 2 rounds.
 * 
 * @param ch The character
 * @return TRUE if has Flourish, FALSE otherwise
 */
bool has_bard_flourish_perk(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_FLOURISH);
}

/**
 * Check if character is currently under Flourish effect.
 * 
 * @param ch The character
 * @return TRUE if affected by Flourish, FALSE otherwise
 */
bool is_affected_by_flourish(struct char_data *ch)
{
  if (!ch)
    return FALSE;
  
  /* Check for AFFECT_BARD_FLOURISH affect */
  return affected_by_spell(ch, AFFECT_BARD_FLOURISH);
}

/**
 * Get Flourish bonus to hit.
 * Returns +2 to hit while Flourish is active.
 * 
 * @param ch The character
 * @return +2 to hit bonus, 0 if not active
 */
int get_bard_flourish_tohit_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!is_affected_by_flourish(ch))
    return 0;
  
  return 2;
}

/**
 * Get Flourish bonus to AC.
 * Returns +2 to AC while Flourish is active.
 * 
 * @param ch The character
 * @return -2 (for AC system), 0 if not active
 */
int get_bard_flourish_ac_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!is_affected_by_flourish(ch))
    return 0;
  
  return 2;
}

/* ============================================================================
 * TIER 4 SWASHBUCKLER PERK FUNCTIONS (Capstone Perks)
 * ============================================================================ */

/**
 * Check if character has Swashbuckler's Supreme Style perk.
 * Grants +2 to hit, +2 dodge AC, +2 crit confirm, +1 attack per 3 rounds with finesse/1H weapons.
 * 
 * @param ch The character
 * @return TRUE if has Supreme Style, FALSE otherwise
 */
bool has_bard_supreme_style(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_SUPREME_STYLE);
}

/**
 * Check if character is affected by Supreme Style buff.
 * 
 * @param ch The character
 * @return TRUE if affected by Supreme Style, FALSE otherwise
 */
bool is_affected_by_supreme_style(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return affected_by_spell(ch, AFFECT_BARD_SUPREME_STYLE);
}

/**
 * Get Supreme Style to-hit bonus.
 * Returns +2 while wielding finesse or single one-handed weapon.
 * 
 * @param ch The character
 * @return To-hit bonus (0 or 2)
 */
int get_bard_supreme_style_tohit_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!has_bard_supreme_style(ch))
    return 0;
  
  /* TODO: Verify wielding appropriate weapon type (finesse or single 1H) */
  if (is_affected_by_supreme_style(ch))
    return 2;
  
  return 0;
}

/**
 * Get Supreme Style AC bonus (dodge).
 * Returns +2 dodge AC while wielding finesse or single one-handed weapon.
 * 
 * @param ch The character
 * @return AC bonus (0 or 2 for dodge AC)
 */
int get_bard_supreme_style_ac_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!has_bard_supreme_style(ch))
    return 0;
  
  /* TODO: Verify wielding appropriate weapon type (finesse or single 1H) */
  if (is_affected_by_supreme_style(ch))
    return 2;
  
  return 0;
}

/**
 * Get Supreme Style critical confirmation bonus.
 * Returns +2 to critical confirmation.
 * 
 * @param ch The character
 * @return Crit confirmation bonus (0 or 2)
 */
int get_bard_supreme_style_crit_confirm_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!has_bard_supreme_style(ch))
    return 0;
  
  /* TODO: Verify wielding appropriate weapon type (finesse or single 1H) */
  if (is_affected_by_supreme_style(ch))
    return 2;
  
  return 0;
}

/**
 * Check if character has Curtain Call perk.
 * Multi-target free attack, 1/5 min, +2d6 precision damage, targets save vs disoriented.
 * 
 * @param ch The character
 * @return TRUE if has Curtain Call, FALSE otherwise
 */
bool has_bard_curtain_call(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return has_perk(ch, PERK_BARD_CURTAIN_CALL);
}

/**
 * Check if character is affected by Curtain Call buff (during active use).
 * 
 * @param ch The character
 * @return TRUE if affected by Curtain Call, FALSE otherwise
 */
bool is_affected_by_curtain_call(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return affected_by_spell(ch, AFFECT_BARD_CURTAIN_CALL);
}

/**
 * Check if character is disoriented by Curtain Call.
 * Disoriented creatures have disadvantage on attacks for 2 rounds.
 * 
 * @param ch The character
 * @return TRUE if disoriented, FALSE otherwise
 */
bool is_affected_by_curtain_call_disoriented(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;
  
  return affected_by_spell(ch, AFFECT_BARD_CURTAIN_CALL_DISORIENTED);
}

/**
 * Get Curtain Call precision damage bonus.
 * Returns +2d6 precision damage per affected target.
 * 
 * @param ch The character
 * @return Precision damage bonus (0 or 2d6)
 */
int get_bard_curtain_call_damage_bonus(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;
  
  if (!is_affected_by_curtain_call(ch))
    return 0;
  
  /* 2d6 precision damage */
  return dice(2, 6);
}