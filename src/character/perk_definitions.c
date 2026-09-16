/**
 * @file perk_definitions.c
 * Perk tables - the content of every class perk tree.
 *
 * Each function here fills perk_list[] with one class tree's perks. The
 * bodies are data written as field assignments, not algorithms: adding or
 * retuning a perk changes only this file, and the perk engine in perks.c
 * stays untouched.
 *
 * perk_definitions.h documents the ownership, lifetime, call-order, and
 * thread-safety rules these functions must follow. The short version: run
 * only from init_perks(), store strdup() copies in the three string fields,
 * and never free what is already in a slot.
 */

#include "conf.h"
#include "core/sysdep.h"
#include "core/structs.h"

#include "core/utils.h"
#include "core/interpreter.h"
#include "class.h"

#include "magic/spells.h"
#include "magic/domains_schools.h"
#include "perks.h"
#include "perk_definitions.h"

/* Define Blackguard Perks (Tier 1–2 for Tyranny & Fear tree) */
void define_blackguard_perks(void)
{
  struct perk_data *perk;

  /* Tier 1: Dread Presence */
  perk = &perk_list[PERK_BLACKGUARD_DREAD_PRESENCE];
  perk->id = PERK_BLACKGUARD_DREAD_PRESENCE;
  perk->name = strdup("Dread Presence");
  perk->description = strdup("Aura imposes extra penalties to fear saves; +2 intimidate.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_TYRANNY_AND_FEAR;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 intimidate */
  perk->special_description =
      strdup("Intimidate +2; foes in your aura suffer extra fear save penalties.");

  /* Tier 1: Intimidating Smite */
  perk = &perk_list[PERK_BLACKGUARD_INTIMIDATING_SMITE];
  perk->id = PERK_BLACKGUARD_INTIMIDATING_SMITE;
  perk->name = strdup("Intimidating Smite");
  perk->description = strdup("Smite Good applies shaken on hit; Will save negates.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_TYRANNY_AND_FEAR;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->special_description = strdup("On smite good hits, target must save or be shaken.");

  /* Tier 1: Cruel Edge */
  perk = &perk_list[PERK_BLACKGUARD_CRUEL_EDGE];
  perk->id = PERK_BLACKGUARD_CRUEL_EDGE;
  perk->name = strdup("Cruel Edge");
  perk->description = strdup("Bonus damage vs frightened/shaken foes; minor temp hp on kill.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_TYRANNY_AND_FEAR;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* damage bonus scaling handled in code */
  perk->special_description = strdup("Gain bonus damage vs fearful foes; small temp hp on kill.");

  /* Tier 1: Command the Weak */
  perk = &perk_list[PERK_BLACKGUARD_COMMAND_THE_WEAK];
  perk->id = PERK_BLACKGUARD_COMMAND_THE_WEAK;
  perk->name = strdup("Command the Weak");
  perk->description = strdup("Demoralize faster: move action; 1/enc swift. Longer duration.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_TYRANNY_AND_FEAR;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->special_description =
      strdup("Demoralize as move; once per encounter as swift; extended duration.");

  /* Tier 2: Aura of Cowardice (perk) */
  perk = &perk_list[PERK_BLACKGUARD_AURA_OF_COWARDICE_PERK];
  perk->id = PERK_BLACKGUARD_AURA_OF_COWARDICE_PERK;
  perk->name = strdup("Aura of Cowardice");
  perk->description =
      strdup("Enhance aura: extra penalties on fear saves; suppress more immunity.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_TYRANNY_AND_FEAR;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_DREAD_PRESENCE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* extra penalty scaling handled */
  perk->special_description = strdup("Enemies in your aura take additional fear save penalties.");

  /* Tier 2: Terror Tactics */
  perk = &perk_list[PERK_BLACKGUARD_TERROR_TACTICS];
  perk->id = PERK_BLACKGUARD_TERROR_TACTICS;
  perk->name = strdup("Terror Tactics");
  perk->description = strdup("Strong intimidate splashes to nearby foes on high roll.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_TYRANNY_AND_FEAR;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_COMMAND_THE_WEAK;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->special_description =
      strdup("Intimidate can splash to adjacent enemies when exceeding DC.");

  /* Tier 2: Black Seraph Step */
  perk = &perk_list[PERK_BLACKGUARD_BLACK_SERAPH_STEP];
  perk->id = PERK_BLACKGUARD_BLACK_SERAPH_STEP;
  perk->name = strdup("Black Seraph Step");
  perk->description = strdup("On forced move/knockdown/trip, auto-demoralize nearby foe.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_TYRANNY_AND_FEAR;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_DREAD_PRESENCE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->special_description = strdup("Auto-demoralize trigger on positional disruption.");

  /* Tier 2: Nightmarish Visage */
  perk = &perk_list[PERK_BLACKGUARD_NIGHTMARISH_VISAGE];
  perk->id = PERK_BLACKGUARD_NIGHTMARISH_VISAGE;
  perk->name = strdup("Nightmarish Visage");
  perk->description = strdup("Intimidate becomes more menacing; can affect multiple foes.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_TYRANNY_AND_FEAR;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_DREAD_PRESENCE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->special_description = strdup("AoE intimidate capabilities; improved visuals.");

  /* Tier 3: Paralyzing Dread */
  perk = &perk_list[PERK_BLACKGUARD_PARALYZING_DREAD];
  perk->id = PERK_BLACKGUARD_PARALYZING_DREAD;
  perk->name = strdup("Paralyzing Dread");
  perk->description =
      strdup("Failed fear saves escalate: shaken -> frightened; big fail -> cower.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_TYRANNY_AND_FEAR;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_AURA_OF_COWARDICE_PERK;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->special_description =
      strdup("Shaken foes who fail saves become frightened; critical failures cower.");

  /* Tier 3: Despair Harvest */
  perk = &perk_list[PERK_BLACKGUARD_DESPAIR_HARVEST];
  perk->id = PERK_BLACKGUARD_DESPAIR_HARVEST;
  perk->name = strdup("Despair Harvest");
  perk->description = strdup("Gain temp hp when foes fail fear saves; capped per round.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_TYRANNY_AND_FEAR;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_AURA_OF_COWARDICE_PERK;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->special_description =
      strdup("Harvest temporary hit points from enemy fear; limited per round.");

  /* Tier 3: Shackles of Awe */
  perk = &perk_list[PERK_BLACKGUARD_SHACKLES_OF_AWE];
  perk->id = PERK_BLACKGUARD_SHACKLES_OF_AWE;
  perk->name = strdup("Shackles of Awe");
  perk->description = strdup("Fear effects reduce speed and attack bonus of afflicted enemies.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_TYRANNY_AND_FEAR;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_AURA_OF_COWARDICE_PERK;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->special_description = strdup("Fearful foes suffer speed and attack penalties.");

  /* Tier 3: Profane Dominion */
  perk = &perk_list[PERK_BLACKGUARD_PROFANE_DOMINION];
  perk->id = PERK_BLACKGUARD_PROFANE_DOMINION;
  perk->name = strdup("Profane Dominion");
  perk->description = strdup("Feared foes take periodic profane damage each round.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_TYRANNY_AND_FEAR;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_CRUEL_EDGE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->special_description = strdup("Enemies suffering fear effects take ongoing profane damage.");

  /* Tier 4: Sovereign of Terror */
  perk = &perk_list[PERK_BLACKGUARD_SOVEREIGN_OF_TERROR];
  perk->id = PERK_BLACKGUARD_SOVEREIGN_OF_TERROR;
  perk->name = strdup("Sovereign of Terror");
  perk->description = strdup("Aura escalates fear each round (cap cower); immunity -> resistance.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_TYRANNY_AND_FEAR;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_PARALYZING_DREAD;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->special_description =
      strdup("Fear immunity becomes resistance; aura escalates fear per round.");

  /* Tier 4: Midnight Edict */
  perk = &perk_list[PERK_BLACKGUARD_MIDNIGHT_EDICT];
  perk->id = PERK_BLACKGUARD_MIDNIGHT_EDICT;
  perk->name = strdup("Midnight Edict");
  perk->description = strdup("1/day: All in aura save vs mass fright/panic; fail = stagger too.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_TYRANNY_AND_FEAR;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_PARALYZING_DREAD;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->special_description = strdup("Once per day AoE mass fear with stagger on failure.");

  /**************************************************************************
   * TREE B: PROFANE MIGHT - Tier 1 & 2
   **************************************************************************/

  /* Tier 1: Vile Strike */
  perk = &perk_list[PERK_BLACKGUARD_VILE_STRIKE];
  perk->id = PERK_BLACKGUARD_VILE_STRIKE;
  perk->name = strdup("Vile Strike");
  perk->description = strdup("Melee attacks deal bonus profane damage; extra vs good/holy foes.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_PROFANE_MIGHT;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 base profane damage per rank */
  perk->special_description = strdup("+1 profane damage per rank; double vs good alignment.");

  /* Tier 1: Cruel Momentum */
  perk = &perk_list[PERK_BLACKGUARD_CRUEL_MOMENTUM];
  perk->id = PERK_BLACKGUARD_CRUEL_MOMENTUM;
  perk->name = strdup("Cruel Momentum");
  perk->description = strdup("On kill or critical hit, gain stacking damage buff.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_PROFANE_MIGHT;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 damage per stack */
  perk->special_description =
      strdup("Gain +2 damage stacking buff on kill/crit; lasts 10 seconds, max 3 stacks.");

  /* Tier 1: Dark Channel */
  perk = &perk_list[PERK_BLACKGUARD_DARK_CHANNEL];
  perk->id = PERK_BLACKGUARD_DARK_CHANNEL;
  perk->name = strdup("Dark Channel");
  perk->description = strdup("Smite Good adds extra damage die and bypasses DR/good.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_PROFANE_MIGHT;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* adds 1d6 to smite */
  perk->special_description = strdup("Smite Good deals +1d6 damage and bypasses DR/good.");

  /* Tier 1: Brutal Oath */
  perk = &perk_list[PERK_BLACKGUARD_BRUTAL_OATH];
  perk->id = PERK_BLACKGUARD_BRUTAL_OATH;
  perk->name = strdup("Brutal Oath");
  perk->description =
      strdup("Choose favored foe creature type; gain bonus to-hit and damage vs that type.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_PROFANE_MIGHT;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 hit/damage vs favored type */
  perk->special_description =
      strdup("Choose favored foe creature type (use 'brutalfoe' command); +2 hit/damage vs chosen "
             "type. Works like ranger favored enemies.");

  /* Tier 2: Ravaging Smite */
  perk = &perk_list[PERK_BLACKGUARD_RAVAGING_SMITE];
  perk->id = PERK_BLACKGUARD_RAVAGING_SMITE;
  perk->name = strdup("Ravaging Smite");
  perk->description = strdup("Smite Good inflicts bleeding damage over time.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_PROFANE_MIGHT;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_DARK_CHANNEL;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* bleeding damage handled by code */
  perk->special_description =
      strdup("Smite Good causes ongoing bleeding damage (1d6 per round, 5 rounds).");

  /* Tier 2: Profane Weapon Bond */
  perk = &perk_list[PERK_BLACKGUARD_PROFANE_WEAPON_BOND];
  perk->id = PERK_BLACKGUARD_PROFANE_WEAPON_BOND;
  perk->name = strdup("Profane Weapon Bond");
  perk->description =
      strdup("Activate buff: weapon gains magic, evil alignment, and on-hit rider.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_PROFANE_MIGHT;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_VILE_STRIKE;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->special_description = strdup("Use 'profanebond' command; 1/enc, 1 minute duration, weapon "
                                     "is magic/evil, applies weakness on hit.");

  /* Tier 2: Relentless Assault */
  perk = &perk_list[PERK_BLACKGUARD_RELENTLESS_ASSAULT];
  perk->id = PERK_BLACKGUARD_RELENTLESS_ASSAULT;
  perk->name = strdup("Relentless Assault");
  perk->description = strdup("Extra attack after charge or killing blow; once per round.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_PROFANE_MIGHT;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_CRUEL_MOMENTUM;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->special_description =
      strdup("Gain free attack after charge or killing blow; 1/round limit.");

  /* Tier 2: Sanguine Barrier */
  perk = &perk_list[PERK_BLACKGUARD_SANGUINE_BARRIER];
  perk->id = PERK_BLACKGUARD_SANGUINE_BARRIER;
  perk->name = strdup("Sanguine Barrier");
  perk->description = strdup("Portion of damage dealt converts to temporary hit points.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_PROFANE_MIGHT;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_VILE_STRIKE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 20; /* 20% conversion rate */
  perk->special_description = strdup("20% of damage dealt becomes temp hp; max 5*level per round.");

  /* Tier 3: Doom Cleave */
  perk = &perk_list[PERK_BLACKGUARD_DOOM_CLEAVE];
  perk->id = PERK_BLACKGUARD_DOOM_CLEAVE;
  perk->name = strdup("Doom Cleave");
  perk->description = strdup("On kill, gain free attack on another foe in the room.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_PROFANE_MIGHT;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_SANGUINE_BARRIER;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->special_description =
      strdup("On kill, make free attack on another foe fighting you or your party.");

  /* Tier 3: Soul Rend */
  perk = &perk_list[PERK_BLACKGUARD_SOUL_REND];
  perk->id = PERK_BLACKGUARD_SOUL_REND;
  perk->name = strdup("Soul Rend");
  perk->description = strdup("Extra dice vs good outsiders/undead; may suppress resist.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_PROFANE_MIGHT;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_RELENTLESS_ASSAULT;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2d6 extra */
  perk->special_description =
      strdup("Attacks vs good outsiders/undead deal +2d6 damage; may suppress their resist.");

  /* Tier 3: Blackened Precision */
  perk = &perk_list[PERK_BLACKGUARD_BLACKENED_PRECISION];
  perk->id = PERK_BLACKGUARD_BLACKENED_PRECISION;
  perk->name = strdup("Blackened Precision");
  perk->description = strdup("Increased critical range or crit multiplier while bonded.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_PROFANE_MIGHT;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_PROFANE_WEAPON_BOND;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->special_description =
      strdup("While Profane Weapon Bond active: threat range +1 and crit mult +1.");

  /* Tier 3: Unholy Blitz */
  perk = &perk_list[PERK_BLACKGUARD_UNHOLY_BLITZ];
  perk->id = PERK_BLACKGUARD_UNHOLY_BLITZ;
  perk->name = strdup("Unholy Blitz");
  perk->description = strdup("Brief haste-like burst after smite hit with limited uses.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_PROFANE_MIGHT;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_DARK_CHANNEL;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->special_description =
      strdup("After smite hit: gain extra attack/movement for 1 round, 2/encounter.");

  /* Tier 4: Avatar of Profanity (Capstone) */
  perk = &perk_list[PERK_BLACKGUARD_AVATAR_OF_PROFANITY];
  perk->id = PERK_BLACKGUARD_AVATAR_OF_PROFANITY;
  perk->name = strdup("Avatar of Profanity");
  perk->description = strdup(
      "Long-cooldown self-buff: big profane damage, DR/—, resist, and auto-bypass alignment DR.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_PROFANE_MIGHT;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_DOOM_CLEAVE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->special_description = strdup("Capstone active: 'profaneavatar' command for +profane "
                                     "dmg/DR/resist/DR bypass, 1 hour cooldown.");

  /* Tier 4: Cataclysmic Smite (Capstone) */
  perk = &perk_list[PERK_BLACKGUARD_CATACLYSMIC_SMITE];
  perk->id = PERK_BLACKGUARD_CATACLYSMIC_SMITE;
  perk->name = strdup("Cataclysmic Smite");
  perk->description =
      strdup("1/day smite detonates in dark burst: AoE damage + save vs sickened/staggered.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_PROFANE_MIGHT;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_SOUL_REND;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->special_description = strdup(
      "Capstone active: smite can detonate in dark burst, 1/day. AoE damage + debuff riders.");

  /**************************************************************************
   * TREE C: UNHOLY RESILIENCE - Tier 1 & 2 defined elsewhere
   * TREE C: UNHOLY RESILIENCE - Tier 3 & 4
   **************************************************************************/


  /**************************************************************************
   * TREE C: UNHOLY RESILIENCE - Tier 1 & 2
   **************************************************************************/

  /* Tier 1: Profane Fortitude */
  perk = &perk_list[PERK_BLACKGUARD_PROFANE_FORTITUDE];
  perk->id = PERK_BLACKGUARD_PROFANE_FORTITUDE;
  perk->name = strdup("Profane Fortitude");
  perk->description = strdup("Gain profane bonus to saves vs holy/good magic.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_UNHOLY_RESILIENCE;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 profane bonus */
  perk->special_description =
      strdup("+2 profane bonus on saves vs good-aligned casters or holy effects.");

  /* Tier 1: Dark Aegis */
  perk = &perk_list[PERK_BLACKGUARD_DARK_AEGIS];
  perk->id = PERK_BLACKGUARD_DARK_AEGIS;
  perk->name = strdup("Dark Aegis");
  perk->description = strdup("Gain a small DR/— while not flat-footed; scales lightly with level.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_UNHOLY_RESILIENCE;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->special_description = strdup("+1 DR/— plus small scaling while not flat-footed.");

  /* Tier 1: Graveborn Vigor */
  perk = &perk_list[PERK_BLACKGUARD_GRAVEBORN_VIGOR];
  perk->id = PERK_BLACKGUARD_GRAVEBORN_VIGOR;
  perk->name = strdup("Graveborn Vigor");
  perk->description =
      strdup("When bloodied, gain a burst of profane vitality (temp hp) once per cooldown.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_UNHOLY_RESILIENCE;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->special_description =
      strdup("Trigger at 50% HP or lower: gain temp hp; 5-minute cooldown.");

  /* Tier 1: Sinister Recovery */
  perk = &perk_list[PERK_BLACKGUARD_SINISTER_RECOVERY];
  perk->id = PERK_BLACKGUARD_SINISTER_RECOVERY;
  perk->name = strdup("Sinister Recovery");
  perk->description =
      strdup("Channel profane energy to heal yourself; nearby good foes take half.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_UNHOLY_RESILIENCE;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->special_description = strdup("Command 'sinisterrecovery'; heals you for level-based "
                                     "amount, deals half to nearby good foes. Daily cooldown.");

  /* Tier 2: Aura of Desecration */
  perk = &perk_list[PERK_BLACKGUARD_AURA_OF_DESECRATION];
  perk->id = PERK_BLACKGUARD_AURA_OF_DESECRATION;
  perk->name = strdup("Aura of Desecration");
  perk->description = strdup(
      "Enemy healing in your presence is blighted; allies’ negative energy effects bite harder.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_UNHOLY_RESILIENCE;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_PROFANE_FORTITUDE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->special_description = strdup(
      "Enemy healing reduced near you; your negative energy heals/damage are slightly empowered.");

  /* Tier 2: Fell Ward */
  perk = &perk_list[PERK_BLACKGUARD_FELL_WARD];
  perk->id = PERK_BLACKGUARD_FELL_WARD;
  perk->name = strdup("Fell Ward");
  perk->description = strdup("After being targeted by divine magic, your next save is bolstered.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_UNHOLY_RESILIENCE;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_PROFANE_FORTITUDE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->special_description = strdup("+2 saves vs divine spellcasters.");

  /* Tier 2: Defiant Hide */
  perk = &perk_list[PERK_BLACKGUARD_DEFIANT_HIDE];
  perk->id = PERK_BLACKGUARD_DEFIANT_HIDE;
  perk->name = strdup("Defiant Hide");
  perk->description = strdup("Gain DR versus good foes and smiting weapons.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_UNHOLY_RESILIENCE;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_DARK_AEGIS;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->special_description = strdup("Additional DR vs good-aligned enemies or smiting attacks.");

  /* Tier 2: Shade Step */
  perk = &perk_list[PERK_BLACKGUARD_SHADE_STEP];
  perk->id = PERK_BLACKGUARD_SHADE_STEP;
  perk->name = strdup("Shade Step");
  perk->description = strdup("Slip through shadow, gaining brief evasion and repositioning.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_UNHOLY_RESILIENCE;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_GRAVEBORN_VIGOR;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->special_description =
      strdup("Command 'shadestep'; swift action, grants brief blur/concealment and positions you "
             "defensively. 1-minute cooldown.");


  /* Tier 3: Soul Carapace */
  perk = &perk_list[PERK_BLACKGUARD_SOUL_CARAPACE];
  perk->id = PERK_BLACKGUARD_SOUL_CARAPACE;
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_UNHOLY_RESILIENCE;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_DEFIANT_HIDE;
  perk->prerequisite_rank = 1;
  perk->name = strdup("Soul Carapace");
  perk->description =
      strdup("Portion of incoming damage converts to temporary HP (per-round cap).");
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->special_description =
      strdup("Convert ~15% incoming damage to temp HP; capped per round by level.");

  /* Tier 3: Warding Malice */
  perk = &perk_list[PERK_BLACKGUARD_WARDING_MALICE];
  perk->id = PERK_BLACKGUARD_WARDING_MALICE;
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_UNHOLY_RESILIENCE;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_FELL_WARD;
  perk->prerequisite_rank = 1;
  perk->name = strdup("Warding Malice");
  perk->description =
      strdup("Enemies suffer penalties to caster level checks vs your wards and saves.");
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->special_description = strdup("Apply -2 penalty to spell penetration checks against you.");

  /* Tier 3: Blackguard's Reprisal */
  perk = &perk_list[PERK_BLACKGUARD_BLACKGUARDS_REPRISAL];
  perk->id = PERK_BLACKGUARD_BLACKGUARDS_REPRISAL;
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_UNHOLY_RESILIENCE;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_FELL_WARD;
  perk->prerequisite_rank = 1;
  perk->name = strdup("Blackguard's Reprisal");
  perk->description = strdup("After you save vs a spell, your next attack gains bonus damage.");
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->special_description =
      strdup("On successful spell save: next attack +CHA and level-based bonus damage.");


  /* Tier 3: Necrotic Regeneration */
  perk = &perk_list[PERK_BLACKGUARD_NECROTIC_REGENERATION];
  perk->id = PERK_BLACKGUARD_NECROTIC_REGENERATION;
  perk->name = strdup("Necrotic Regeneration");
  perk->description = strdup("Gain fast healing 2 while below 50% HP.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_UNHOLY_RESILIENCE;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_GRAVEBORN_VIGOR;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* Fast Healing 2 */
  perk->special_description =
      strdup("While below 50% HP, gain fast healing 2 (heals 2 HP per round).");

  /* Tier 3: Unholy Fortification */
  perk = &perk_list[PERK_BLACKGUARD_UNHOLY_FORTIFICATION];
  perk->id = PERK_BLACKGUARD_UNHOLY_FORTIFICATION;
  perk->name = strdup("Unholy Fortification");
  perk->description =
      strdup("Immune to critical hits and sneak attacks from good-aligned attackers.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_UNHOLY_RESILIENCE;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_DARK_AEGIS;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->special_description =
      strdup("Good-aligned attackers cannot land critical hits or sneak attacks against you.");

  /* Tier 3: Blasphemous Warding */
  perk = &perk_list[PERK_BLACKGUARD_BLASPHEMOUS_WARDING];
  perk->id = PERK_BLACKGUARD_BLASPHEMOUS_WARDING;
  perk->name = strdup("Blasphemous Warding");
  perk->description =
      strdup("Gain bonus spell resistance vs divine spells and good-aligned casters.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_UNHOLY_RESILIENCE;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_FELL_WARD;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5; /* +5 SR vs divine spells */
  perk->special_description =
      strdup("Gain +5 spell resistance vs divine spells and spells from good-aligned casters.");

  /* Tier 3: Resilient Corruption */
  perk = &perk_list[PERK_BLACKGUARD_RESILIENT_CORRUPTION];
  perk->id = PERK_BLACKGUARD_RESILIENT_CORRUPTION;
  perk->name = strdup("Resilient Corruption");
  perk->description =
      strdup("Gain stacking +1 DR each time damaged, max 5 stacks, resets out of combat.");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_UNHOLY_RESILIENCE;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_DEFIANT_HIDE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 DR per stack */
  perk->special_description =
      strdup("Each time you take damage, gain +1 DR (max 5 stacks). Resets when combat ends.");

  /* Tier 4: Undying Vigor (Capstone) */
  perk = &perk_list[PERK_BLACKGUARD_UNDYING_VIGOR];
  perk->id = PERK_BLACKGUARD_UNDYING_VIGOR;
  perk->name = strdup("Undying Vigor");
  perk->description = strdup("Once per day, survive a killing blow with 1 HP (auto-triggers).");
  perk->associated_class = CLASS_BLACKGUARD;
  perk->perk_category = PERK_CATEGORY_UNHOLY_RESILIENCE;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BLACKGUARD_NECROTIC_REGENERATION;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->special_description = strdup("Capstone: Once per day, if reduced to 0 or fewer HP, survive "
                                     "with 1 HP instead. Triggers automatically.");
}

/* Define Inquisitor Perks - Judgment & Spellcasting (Tier 1) */
void define_inquisitor_perks(void)
{
  struct perk_data *perk;

  /**************************************************************************
   * TREE 1: JUDGMENT & SPELLCASTING - Tier 1
   **************************************************************************/

  /* Tier 1: Empowered Judgment (3 ranks, 1 point each) */
  perk = &perk_list[PERK_INQUISITOR_EMPOWERED_JUDGMENT];
  perk->id = PERK_INQUISITOR_EMPOWERED_JUDGMENT;
  perk->name = strdup("Empowered Judgment");
  perk->description = strdup(
      "Your judgment abilities become more potent. Increase judgment bonuses by +1 per rank.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_JUDGMENT_SPELLCASTING;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 bonus per rank */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Rank 1-3: +1 to judgment bonuses per rank. At rank 3, can "
                                     "maintain two judgments for 1d4 rounds once per encounter.");
  perk->toggleable = false;

  /* Tier 1: Swift Spellcaster (1 rank, 1 point) */
  perk = &perk_list[PERK_INQUISITOR_SWIFT_SPELLCASTER];
  perk->id = PERK_INQUISITOR_SWIFT_SPELLCASTER;
  perk->name = strdup("Swift Spellcaster");
  perk->description = strdup("Channel divine magic with practiced efficiency.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_JUDGMENT_SPELLCASTING;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 to concentration */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Reduce casting time of inquisitor spells by one step once "
                                     "per encounter. +2 to concentration checks.");
  perk->toggleable = false;

  /* Tier 1: Spell Focus: Divination (2 ranks, 1 point each) */
  perk = &perk_list[PERK_INQUISITOR_SPELL_FOCUS_DIVINATION];
  perk->id = PERK_INQUISITOR_SPELL_FOCUS_DIVINATION;
  perk->name = strdup("Spell Focus: Divination");
  perk->description = strdup("Your divination spells pierce through deception.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_JUDGMENT_SPELLCASTING;
  perk->cost = 1;
  perk->max_rank = 2;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPELL_DC;
  perk->effect_value = 1;             /* +1 DC per rank */
  perk->effect_modifier = ABJURATION; /* Will use for divination school */
  perk->special_description =
      strdup("Rank 1-2: Increase DC of divination spells by +1 per rank. At rank 2, gain one "
             "additional divination spell slot per spell level.");
  perk->toggleable = false;

  /* Tier 1: Judgment Recovery (1 rank, 1 point) */
  perk = &perk_list[PERK_INQUISITOR_JUDGMENT_RECOVERY];
  perk->id = PERK_INQUISITOR_JUDGMENT_RECOVERY;
  perk->name = strdup("Judgment Recovery");
  perk->description = strdup("Your judgment powers refresh more quickly.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_JUDGMENT_SPELLCASTING;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Regain one use of judgment ability when you reduce an enemy "
                                     "to 0 hit points or below. Once per encounter.");
  perk->toggleable = false;

  /**************************************************************************
   * TREE 1: JUDGMENT & SPELLCASTING - Tier 2
   **************************************************************************/

  /* Tier 2: Enhanced Bane (4 ranks, 2 points each) */
  perk = &perk_list[PERK_INQUISITOR_ENHANCED_BANE];
  perk->id = PERK_INQUISITOR_ENHANCED_BANE;
  perk->name = strdup("Enhanced Bane");
  perk->description = strdup("Your weapon gains the bane property against your judged target.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_JUDGMENT_SPELLCASTING;
  perk->cost = 2;
  perk->max_rank = 4;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 damage per rank, +1 attack per 2 ranks */
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("Add +1 damage per rank and +1 to attack rolls per 2 ranks against judged target. At "
             "rank 4, bane extends to all creatures of same type.");
  perk->toggleable = false;

  /* Tier 2: Divine Resilience (1 rank, 2 points) */
  perk = &perk_list[PERK_INQUISITOR_DIVINE_RESILIENCE];
  perk->id = PERK_INQUISITOR_DIVINE_RESILIENCE;
  perk->name = strdup("Divine Resilience");
  perk->description = strdup("Channel divine energy to bolster yourself when activating judgment.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_JUDGMENT_SPELLCASTING;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("When you activate judgment, gain temporary hit points equal "
                                     "to inquisitor level + Wisdom modifier.");
  perk->toggleable = false;

  /* Tier 2: Spell Penetration (3 ranks, 2 points each) */
  perk = &perk_list[PERK_INQUISITOR_SPELL_PENETRATION];
  perk->id = PERK_INQUISITOR_SPELL_PENETRATION;
  perk->name = strdup("Spell Penetration");
  perk->description = strdup("Your spells overcome magical defenses more easily.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_JUDGMENT_SPELLCASTING;
  perk->cost = 2;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 per rank to spell penetration */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Gain +1 per rank to checks to overcome spell resistance. At "
                                     "rank 3, ignore the first 5 points of spell resistance.");
  perk->toggleable = false;

  /* Tier 2: Persistent Judgment (1 rank, 2 points) */
  perk = &perk_list[PERK_INQUISITOR_PERSISTENT_JUDGMENT];
  perk->id = PERK_INQUISITOR_PERSISTENT_JUDGMENT;
  perk->name = strdup("Persistent Judgment");
  perk->description = strdup("When a judgment is toggled off, its effect persists for 5 rounds.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_JUDGMENT_SPELLCASTING;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5; /* 5 rounds duration */
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("When toggling off a judgment, the judgment bonus persists as a morale bonus for 5 "
             "rounds. 20 round cooldown per judgment type.");
  perk->toggleable = false;

  /**************************************************************************
   * TREE 1: JUDGMENT & SPELLCASTING - Tier 3
   **************************************************************************/

  /* Tier 3: Greater Judgment (1 rank, 3 points) */
  perk = &perk_list[PERK_INQUISITOR_GREATER_JUDGMENT];
  perk->id = PERK_INQUISITOR_GREATER_JUDGMENT;
  perk->name = strdup("Greater Judgment");
  perk->description = strdup("Your judgment abilities reach their full potential.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_JUDGMENT_SPELLCASTING;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* Bonuses are doubled */
  perk->effect_modifier = 0;
  perk->special_description = strdup(
      "Choose one judgment type whose bonuses are doubled. Change selection after long rest.");
  perk->toggleable = false;

  /* Tier 3: Spell Metamastery (1 rank, 3 points) */
  perk = &perk_list[PERK_INQUISITOR_SPELL_METAMASTERY];
  perk->id = PERK_INQUISITOR_SPELL_METAMASTERY;
  perk->name = strdup("Spell Metamastery");
  perk->description = strdup("Apply metamagic effects with minimal effort.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_JUDGMENT_SPELLCASTING;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 level adjustment max */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Once per encounter, apply a known metamagic feat without "
                                     "increasing casting time or slot level (max +2 adjustment).");
  perk->toggleable = false;

  /* Tier 3: Righteous Strike (2 ranks, 3 points each) */
  perk = &perk_list[PERK_INQUISITOR_RIGHTEOUS_STRIKE];
  perk->id = PERK_INQUISITOR_RIGHTEOUS_STRIKE;
  perk->name = strdup("Righteous Strike");
  perk->description = strdup("Channel spell energy through your weapon.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_JUDGMENT_SPELLCASTING;
  perk->cost = 3;
  perk->max_rank = 2;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* 2d6 per rank */
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("After casting an inquisitor spell, next melee attack within 1 round deals 2d6 damage "
             "per rank. Damage type matches spell.");
  perk->toggleable = false;

  /* Tier 3: Versatile Judgment (1 rank, 3 points) */
  perk = &perk_list[PERK_INQUISITOR_VERSATILE_JUDGMENT];
  perk->id = PERK_INQUISITOR_VERSATILE_JUDGMENT;
  perk->name = strdup("Versatile Judgment");
  perk->description = strdup("Switch between judgments with practiced ease.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_JUDGMENT_SPELLCASTING;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 judgment use */
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("Change active judgment as swift action. Gain +1 daily judgment use.");
  perk->toggleable = false;

  /* Tier 4: Judgment Mastery (1 rank, 4 points) */
  perk = &perk_list[PERK_INQUISITOR_JUDGMENT_MASTERY];
  perk->id = PERK_INQUISITOR_JUDGMENT_MASTERY;
  perk->name = strdup("Judgment Mastery");
  perk->description = strdup("Maintain three judgments and extend their linger.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_JUDGMENT_SPELLCASTING;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3;    /* target minimum concurrent judgments */
  perk->effect_modifier = 2; /* linger rounds */
  perk->special_description = strdup("Maintain up to three simultaneous judgments; judgments "
                                     "linger 2 rounds after you stop focusing.");
  perk->toggleable = false;

  /* Tier 4: Divine Spellstrike (1 rank, 4 points) */
  perk = &perk_list[PERK_INQUISITOR_DIVINE_SPELLSTRIKE];
  perk->id = PERK_INQUISITOR_DIVINE_SPELLSTRIKE;
  perk->name = strdup("Divine Spellstrike");
  perk->description = strdup("Once per day, cast an inquisitor spell as a swift action and follow "
                             "with an immediate full attack.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_JUDGMENT_SPELLCASTING;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("Once per day, cast an inquisitor spell as a swift action and immediately make a full "
             "attack; spell resolves on hit as touch.");
  perk->toggleable = false;

  /* Tier 4: Inexorable Judgment (1 rank, 4 points) */
  perk = &perk_list[PERK_INQUISITOR_INEXORABLE_JUDGMENT];
  perk->id = PERK_INQUISITOR_INEXORABLE_JUDGMENT;
  perk->name = strdup("Inexorable Judgment");
  perk->description = strdup("Once per day, smite with irresistible judgment.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_JUDGMENT_SPELLCASTING;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("Once per day, force a Will save (DC 10 + 1/2 level + Wis) or deal 1d6/level untyped "
             "damage; judgment bonuses ignore immunities.");
  perk->toggleable = false;

  /* Tier 4: Supreme Spellcasting (1 rank, 4 points) */
  perk = &perk_list[PERK_INQUISITOR_SUPREME_SPELLCASTING];
  perk->id = PERK_INQUISITOR_SUPREME_SPELLCASTING;
  perk->name = strdup("Supreme Spellcasting");
  perk->description = strdup("Once per day, cast an inquisitor spell without expending a slot.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_JUDGMENT_SPELLCASTING;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Gain an extra slot per spell level and once per day cast any "
                                     "inquisitor spell you know for free.");
  perk->toggleable = false;

  /**************************************************************************
   * TREE 2: HUNTER'S ARSENAL - Tier 1
   **************************************************************************/

  /* Tier 1: Studied Target (4 ranks, 1 point each) */
  perk = &perk_list[PERK_INQUISITOR_STUDIED_TARGET];
  perk->id = PERK_INQUISITOR_STUDIED_TARGET;
  perk->name = strdup("Studied Target");
  perk->description = strdup("Mark a target for focused hunting, gaining bonuses against it.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_HUNTERS_ARSENAL;
  perk->cost = 1;
  perk->max_rank = 4;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 per rank to attack/damage/skills vs studied target */
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("Use a move action to study one creature within 60 feet; gain +1 per rank to attack, "
             "damage, and relevant skill checks against that target (one target at a time).");
  perk->toggleable = false;

  /* Tier 1: Favored Terrain (1 rank, 1 point) */
  perk = &perk_list[PERK_INQUISITOR_FAVORED_TERRAIN];
  perk->id = PERK_INQUISITOR_FAVORED_TERRAIN;
  perk->name = strdup("Favored Terrain");
  perk->description = strdup("Master fighting in a chosen environment.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_HUNTERS_ARSENAL;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 initiative/stealth in chosen terrain */
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("Choose a favored terrain; gain +2 to initiative and Stealth checks in that terrain. "
             "Can change the selection once per real week.");
  perk->toggleable = false;

  /* Tier 1: Hunter's Precision (2 ranks, 1 point each) */
  perk = &perk_list[PERK_INQUISITOR_HUNTERS_PRECISION];
  perk->id = PERK_INQUISITOR_HUNTERS_PRECISION;
  perk->name = strdup("Hunter's Precision");
  perk->description = strdup("Reroll damage with lethal accuracy.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_HUNTERS_ARSENAL;
  perk->cost = 1;
  perk->max_rank = 2;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5; /* 5% chance per rank to reroll damage */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Gain a 5%% chance per rank to reroll a damage roll and keep "
                                     "the higher result. Applies to weapon and spell damage.");
  perk->toggleable = false;

  /* Tier 1: Track and Hunt (1 rank, 1 point) */
  perk = &perk_list[PERK_INQUISITOR_TRACK_AND_HUNT];
  perk->id = PERK_INQUISITOR_TRACK_AND_HUNT;
  perk->name = strdup("Track and Hunt");
  perk->description = strdup("Double down on your quarry's trail.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_HUNTERS_ARSENAL;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* double Survival modifier while tracking */
  perk->effect_modifier = 0;
  perk->special_description = strdup(
      "Double your Nature modifier when tracking creatures; pairs well with Favored Terrain.");
  perk->toggleable = false;

  /**************************************************************************
   * TREE 2: HUNTER'S ARSENAL - Tier 2
   **************************************************************************/

  /* Tier 2: Favored Enemy Enhancement (4 ranks, 2 points each) */
  perk = &perk_list[PERK_INQUISITOR_FAVORED_ENEMY_ENHANCEMENT];
  perk->id = PERK_INQUISITOR_FAVORED_ENEMY_ENHANCEMENT;
  perk->name = strdup("Favored Enemy Enhancement");
  perk->description = strdup("Specialize in hunting specific creature types.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_HUNTERS_ARSENAL;
  perk->cost = 2;
  perk->max_rank = 4;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;    /* +2 per rank to attack/damage */
  perk->effect_modifier = 1; /* +1 per rank to AC */
  perk->special_description = strdup(
      "Choose a creature type each time you take this perk. Gain +2 per rank to attack and damage "
      "rolls against that type. Add +1 per rank to AC against attacks from that type.");
  perk->toggleable = false;

  /* Tier 2: Ambush Predator (1 rank, 2 points) */
  perk = &perk_list[PERK_INQUISITOR_AMBUSH_PREDATOR];
  perk->id = PERK_INQUISITOR_AMBUSH_PREDATOR;
  perk->name = strdup("Ambush Predator");
  perk->description = strdup("Strike from concealment with devastating effect.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_HUNTERS_ARSENAL;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3; /* 3d6 damage */
  perk->effect_modifier = 6;
  perk->special_description =
      strdup("If you attack a creature that hasn't acted yet in combat or doesn't know you're "
             "there, deal an additional 3d6 damage on your first attack.");
  perk->toggleable = false;

  /* Tier 2: Terrain Mastery (3 ranks, 2 points each) */
  perk = &perk_list[PERK_INQUISITOR_TERRAIN_MASTERY];
  perk->id = PERK_INQUISITOR_TERRAIN_MASTERY;
  perk->name = strdup("Terrain Mastery");
  perk->description =
      strdup("Gain additional favored terrains and enhanced abilities within them.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_HUNTERS_ARSENAL;
  perk->cost = 2;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 per rank to Survival */
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("Choose one new favored terrain per rank. In favored terrains, you leave no trail "
             "unless you choose to, and gain +2 per rank to Nature checks.");
  perk->toggleable = false;

  /* Tier 2: Hunter's Endurance (1 rank, 2 points) */
  perk = &perk_list[PERK_INQUISITOR_HUNTERS_ENDURANCE];
  perk->id = PERK_INQUISITOR_HUNTERS_ENDURANCE;
  perk->name = strdup("Hunter's Endurance");
  perk->description = strdup("Your body is conditioned for long pursuits.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_HUNTERS_ARSENAL;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;    /* +2 move regeneration per round */
  perk->effect_modifier = 5; /* 5% chance to remove fatigue */
  perk->special_description = strdup("Gain +2 move regeneration per round. 5% chance per round to "
                                     "remove any AFF_FATIGUE effect on the player.");
  perk->toggleable = false;

  /**************************************************************************
   * TREE 2: HUNTER'S ARSENAL - Tier 3
   **************************************************************************/

  /* Tier 3: Deadly Aim (3 ranks, 3 points each) */
  perk = &perk_list[PERK_INQUISITOR_DEADLY_AIM];
  perk->id = PERK_INQUISITOR_DEADLY_AIM;
  perk->name = strdup("Deadly Aim");
  perk->description = strdup("Trade accuracy for devastating power with a swift action.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_HUNTERS_ARSENAL;
  perk->cost = 3;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3;     /* +3 damage per rank */
  perk->effect_modifier = -1; /* -1 to hit while active */
  perk->special_description = strdup("As a swift action, take -1 to attacks to gain +3 damage per "
                                     "rank on all attacks until end of turn (melee and ranged).");
  perk->toggleable = false;

  /**************************************************************************
     * TREE 2: HUNTER'S ARSENAL - Tier 4
     **************************************************************************/

  /* Tier 4: Supreme Hunter (1 rank, 4 points) */
  perk = &perk_list[PERK_INQUISITOR_SUPREME_HUNTER];
  perk->id = PERK_INQUISITOR_SUPREME_HUNTER;
  perk->name = strdup("Supreme Hunter");
  perk->description = strdup("Become the ultimate predator.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_HUNTERS_ARSENAL;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4; /* +4 to Studied Target */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Your Studied Target bonus increases by +4. Time to study is "
                                     "halved (swift action instead of move action).");

  perk = &perk_list[PERK_INQUISITOR_LEGENDARY_TRACKER];
  perk->id = PERK_INQUISITOR_LEGENDARY_TRACKER;
  perk->name = strdup("Legendary Tracker");
  perk->description = strdup("Track any creature at any distance.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_HUNTERS_ARSENAL;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("You can track creatures at extreme distances. Maximum "
                                     "tracking distance: 50 + (Nature skill * 10) rooms.");

  perk = &perk_list[PERK_INQUISITOR_INSTANT_DEATH];
  perk->id = PERK_INQUISITOR_INSTANT_DEATH;
  perk->name = strdup("Instant Death");
  perk->description = strdup("Deliver a killing strike with perfect precision.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_HUNTERS_ARSENAL;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3; /* 3% chance */
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("3% chance when you hit with an attack against a studied target to force a Fortitude "
             "save (DC 10 + half level + Wisdom modifier). On failure, target takes +15d6 damage; "
             "on success, it takes +8d6 damage.");

  perk = &perk_list[PERK_INQUISITOR_PERFECT_PREDATOR];
  perk->id = PERK_INQUISITOR_PERFECT_PREDATOR;
  perk->name = strdup("Perfect Predator");
  perk->description = strdup("Achieve mastery over the hunt.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_HUNTERS_ARSENAL;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4; /* +4 favored enemy bonus */
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("All your favored enemy bonuses increase by +4. In your favored terrains, you gain "
             "true sight and can see perfectly in all lighting conditions.");

  perk = &perk_list[PERK_INQUISITOR_MASTER_TRACKER];
  perk->id = PERK_INQUISITOR_MASTER_TRACKER;
  perk->name = strdup("Master Tracker");
  perk->description = strdup("Track prey across any trail and sense nearby quarries.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_HUNTERS_ARSENAL;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup(
      "Ignore mundane tracking limitations; sense when your studied quarry enters your area.");
  perk->toggleable = false;

  /* Tier 3: Wilderness Stride (2 ranks, 3 points each) */
  perk = &perk_list[PERK_INQUISITOR_WILDERNESS_STRIDE];
  perk->id = PERK_INQUISITOR_WILDERNESS_STRIDE;
  perk->name = strdup("Wilderness Stride");
  perk->description = strdup("Move through difficult terrain with supernatural ease.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_HUNTERS_ARSENAL;
  perk->cost = 3;
  perk->max_rank = 2;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Ignore difficult terrain outdoors; at rank 2, magical "
                                     "difficult terrain slows you only slightly.");
  perk->toggleable = false;

  /* Tier 3: Prey's Weakness (1 rank, 3 points) */
  perk = &perk_list[PERK_INQUISITOR_PREYS_WEAKNESS];
  perk->id = PERK_INQUISITOR_PREYS_WEAKNESS;
  perk->name = strdup("Prey's Weakness");
  perk->description = strdup("Exploit what you have learned about your quarry's defenses.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_HUNTERS_ARSENAL;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 50; /* ignore half of target DR */
  perk->effect_modifier = 0;
  perk->special_description = strdup("When you study a foe, you pierce its defenses, ignoring half "
                                     "of its damage reduction.");
  perk->toggleable = false;

  /* Investigation & Perception Tree - Tier 1: Keen Senses (4 ranks, 1 point each) */
  perk = &perk_list[PERK_INQUISITOR_KEEN_SENSES];
  perk->id = PERK_INQUISITOR_KEEN_SENSES;
  perk->name = strdup("Keen Senses");
  perk->description = strdup("Your perceptive abilities are unmatched.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_INVESTIGATION_PERCEPTION;
  perk->cost = 1;
  perk->max_rank = 4;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 per rank to Perception */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Gain +2 per rank to Perception checks. At rank 4, you gain "
                                     "darkvision and can see through magical darkness.");
  perk->toggleable = false;

  /* Investigation & Perception Tree - Tier 1: Read Intentions (1 rank, 1 point) */
  perk = &perk_list[PERK_INQUISITOR_READ_INTENTIONS];
  perk->id = PERK_INQUISITOR_READ_INTENTIONS;
  perk->name = strdup("Read Intentions");
  perk->description = strdup("Sense aggressive creatures nearby.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_INVESTIGATION_PERCEPTION;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup(
      "Sense aggressive mobs one room away. This provides awareness of hostile creatures nearby.");
  perk->toggleable = false;

  /* Investigation & Perception Tree - Tier 1: Lore Master (3 ranks, 1 point each) */
  perk = &perk_list[PERK_INQUISITOR_LORE_MASTER];
  perk->id = PERK_INQUISITOR_LORE_MASTER;
  perk->name = strdup("Lore Master");
  perk->description = strdup("Your knowledge is encyclopedic.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_INVESTIGATION_PERCEPTION;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 per rank to knowledge skills */
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("Gain +1 per rank to Arcana, Wisdom, Nature, and History checks.");
  perk->toggleable = false;

  /* Investigation & Perception Tree - Tier 1: Detect Magic (1 rank, 1 point) */
  perk = &perk_list[PERK_INQUISITOR_DETECT_MAGIC_NATURAL];
  perk->id = PERK_INQUISITOR_DETECT_MAGIC_NATURAL;
  perk->name = strdup("Detect Magic");
  perk->description = strdup("Perceive magical auras naturally.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_INVESTIGATION_PERCEPTION;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("You can cast Detect Magic at will without expending spell slots or components.");
  perk->toggleable = false;

  /* Investigation & Perception Tree - Tier 2: Discern Lies (3 ranks, 2 points each) */
  perk = &perk_list[PERK_INQUISITOR_DISCERN_LIES];
  perk->id = PERK_INQUISITOR_DISCERN_LIES;
  perk->name = strdup("Discern Lies");
  perk->description = strdup("Pierce through deception with divine insight.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_INVESTIGATION_PERCEPTION;
  perk->cost = 2;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SKILL;
  perk->effect_value = 2; /* +2 per rank to opposed Bluff checks */
  perk->effect_modifier = 0;
  perk->special_description = strdup(
      "Gain +2 per rank to Insight checks. At rank 3, you can cast Detect Alignment at will.");
  perk->toggleable = false;

  /* Investigation & Perception Tree - Tier 2: Monster Knowledge (1 rank, 2 points) */
  perk = &perk_list[PERK_INQUISITOR_MONSTER_KNOWLEDGE];
  perk->id = PERK_INQUISITOR_MONSTER_KNOWLEDGE;
  perk->name = strdup("Monster Knowledge");
  perk->description = strdup("Identify creatures with enhanced insight.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_INVESTIGATION_PERCEPTION;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup(
      "Add your Wisdom modifier in addition to Intelligence when making Knowledge checks to lore "
      "creatures. When you successfully lore a creature, you also see all effects they are under.");
  perk->toggleable = false;

  /* Investigation & Perception Tree - Tier 2: Scent of Magic (1 rank, 2 points) */
  perk = &perk_list[PERK_INQUISITOR_SCENT_OF_MAGIC];
  perk->id = PERK_INQUISITOR_SCENT_OF_MAGIC;
  perk->name = strdup("Scent of Magic");
  perk->description = strdup("Detect magical auras with precision.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_INVESTIGATION_PERCEPTION;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("You can identify the school of magic of any affect a "
                                     "creature is under when you lore it or simply look at it.");
  perk->toggleable = false;

  /* Investigation & Perception Tree - Tier 2: Investigator's Eye (2 ranks, 2 points each) */
  perk = &perk_list[PERK_INQUISITOR_INVESTIGATORS_EYE];
  perk->id = PERK_INQUISITOR_INVESTIGATORS_EYE;
  perk->name = strdup("Investigator's Eye");
  perk->description = strdup("Notice details others miss.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_INVESTIGATION_PERCEPTION;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SKILL;
  perk->effect_value = 3; /* +3 per rank to Search and Detect Trap checks */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Gain +3 per rank to Search and Detect Trap checks.");
  perk->toggleable = false;

  /* Investigation & Perception Tree - Tier 3: True Seeing (2 ranks, 3 points each) */
  perk = &perk_list[PERK_INQUISITOR_TRUE_SEEING];
  perk->id = PERK_INQUISITOR_TRUE_SEEING;
  perk->name = strdup("True Seeing");
  perk->description = strdup("See through illusions and deception.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_INVESTIGATION_PERCEPTION;
  perk->cost = 3;
  perk->max_rank = 2;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SAVE;
  perk->effect_value = 4; /* +4 to saves vs illusions at rank 1 */
  perk->effect_modifier = 0;
  perk->special_description = strdup(
      "Rank 1: +4 to saving throws against illusions, cast Detect Invisibility once per day. "
      "Rank 2: Cast True Seeing once per day.");
  perk->toggleable = false;

  /* Investigation & Perception Tree - Tier 3: Telepathic Bond (1 rank, 3 points) */
  perk = &perk_list[PERK_INQUISITOR_TELEPATHIC_BOND];
  perk->id = PERK_INQUISITOR_TELEPATHIC_BOND;
  perk->name = strdup("Telepathic Bond");
  perk->description = strdup("Form mental connections with your allies.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_INVESTIGATION_PERCEPTION;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 to attacks and AC when fighting with allies */
  perk->effect_modifier = 0;
  perk->special_description = strdup("All party members gain +1 to attack rolls and armor class "
                                     "when fighting with allies in the same room.");
  perk->toggleable = false;

  /* Investigation & Perception Tree - Tier 3: Aura Reading (1 rank, 3 points) */
  perk = &perk_list[PERK_INQUISITOR_AURA_READING];
  perk->id = PERK_INQUISITOR_AURA_READING;
  perk->name = strdup("Aura Reading");
  perk->description = strdup("Read the spiritual aura of creatures around you.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_INVESTIGATION_PERCEPTION;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Cast Sense Life and Detect Alignment once per day each.");
  perk->toggleable = false;

  /* Investigation & Perception Tree - Tier 3: Perfect Recall (1 rank, 3 points) */
  perk = &perk_list[PERK_INQUISITOR_PERFECT_RECALL];
  perk->id = PERK_INQUISITOR_PERFECT_RECALL;
  perk->name = strdup("Perfect Recall");
  perk->description = strdup("Your memory becomes eidetic.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_INVESTIGATION_PERCEPTION;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SKILL;
  perk->effect_value = 4; /* +4 to all knowledge skills */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Gain +4 to all Knowledge skills (Arcana, Religion, History, "
                                     "Nature). Learn 3 new languages.");
  perk->toggleable = false;

  /* Investigation & Perception Tree - Tier 4: Master Tactician (1 rank, 4 points) */
  perk = &perk_list[PERK_INQUISITOR_MASTER_TACTICIAN];
  perk->id = PERK_INQUISITOR_MASTER_TACTICIAN;
  perk->name = strdup("Master Tactician");
  perk->description = strdup("Achieve perfect tactical awareness.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_INVESTIGATION_PERCEPTION;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("You can use Battlefield Commander as a free action once every 5 minutes. All allies "
             "within 60 feet gain +2 to initiative and can't be flanked.");
  perk->toggleable = false;

  /* Investigation & Perception Tree - Tier 4: Legendary Resilience (1 rank, 4 points) */
  perk = &perk_list[PERK_INQUISITOR_LEGENDARY_RESILIENCE];
  perk->id = PERK_INQUISITOR_LEGENDARY_RESILIENCE;
  perk->name = strdup("Legendary Resilience");
  perk->description = strdup("Your resilience becomes supernatural.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_INVESTIGATION_PERCEPTION;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5; /* 5/- DR */
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("Gain damage reduction 5/—. Immune to ability damage, ability drain, and energy "
             "drain. 10% chance to automatically succeed on a saving throw (5 minute cooldown).");
  perk->toggleable = false;

  /* Investigation & Perception Tree - Tier 4: Perfect Adaptation (1 rank, 4 points) */
  perk = &perk_list[PERK_INQUISITOR_PERFECT_ADAPTATION];
  perk->id = PERK_INQUISITOR_PERFECT_ADAPTATION;
  perk->name = strdup("Perfect Adaptation");
  perk->description = strdup("Adapt to any threat instantly.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_INVESTIGATION_PERCEPTION;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("Every 5 minutes you can choose to gain one of the following for 30 seconds: immunity "
             "to one energy type, immunity to blindness/deafness/paralysis, spell resistance equal "
             "to 10 + your level, or fast healing 5.");
  perk->toggleable = false;

  /* Investigation & Perception Tree - Tier 4: Inquisitor's Supremacy (1 rank, 4 points) */
  perk = &perk_list[PERK_INQUISITOR_SUPREMACY];
  perk->id = PERK_INQUISITOR_SUPREMACY;
  perk->name = strdup("Inquisitor's Supremacy");
  perk->description = strdup("Achieve the pinnacle of inquisitorial power.");
  perk->associated_class = CLASS_INQUISITOR;
  perk->perk_category = PERK_CATEGORY_INVESTIGATION_PERCEPTION;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 to ability score and spell DC */
  perk->effect_modifier = 0;
  perk->special_description = strdup(
      "Gain +2 to the ability score of your choice. All spell and power DCs increase by +2.");
  perk->toggleable = false;
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
  perk->description = strdup("Regain 1 PSP when a Telepathy power affects or damages at least one "
                             "target (once per round).");
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
  perk->description =
      strdup("Total +2 Telepathy DCs; Telepathy damage powers add +1 die if augmented by ≥2 PSP.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_TELEPATHIC_CONTROL;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_MIND_SPIKE_I;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;    /* Total +2 DC */
  perk->effect_modifier = 1; /* +1 die on damage */
  perk->special_description =
      strdup("Telepathy powers gain +2 DC total; damage powers gain +1 die if augmented ≥2 PSP.");

  /* Overwhelm */
  perk = &perk_list[PERK_PSIONICIST_OVERWHELM];
  perk->id = PERK_PSIONICIST_OVERWHELM;
  perk->name = strdup("Overwhelm");
  perk->description = strdup("First Telepathy power each encounter forces targets to save twice, "
                             "taking the worse (once per combat).");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_TELEPATHIC_CONTROL;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_SUGGESTION_PRIMER;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* Save twice, take worse */
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("First Telepathy power each encounter: targets save twice, use worse result.");

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
  perk->description =
      strdup("When landing a Telepathy debuff, target takes -2 penalty to AC for 2 rounds.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_TELEPATHIC_CONTROL;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_FOCUS_CHANNELING;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = -2;   /* -2 AC */
  perk->effect_modifier = 2; /* 2 round duration */
  perk->special_description = strdup("Telepathy debuffs apply -2 AC for 2 rounds.");

  /*** METACREATIVE GENIUS - TIER II PERKS ***/

  /* Ectoplasmic Artisan II */
  perk = &perk_list[PERK_PSIONICIST_ECTOPLASMIC_ARTISAN_II];
  perk->id = PERK_PSIONICIST_ECTOPLASMIC_ARTISAN_II;
  perk->name = strdup("Ectoplasmic Artisan II");
  perk->description =
      strdup("Total –2 PSP (min 1) once per encounter; +20% duration on metacreative buffs.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_METACREATIVE_GENIUS;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_ECTOPLASMIC_ARTISAN_I;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;     /* Total PSP reduction */
  perk->effect_modifier = 20; /* 20% duration bonus */
  perk->special_description =
      strdup("Tier 2: Metacreativity PSP cost -2 (once/encounter); +20% buff duration.");

  /* Shardstorm */
  perk = &perk_list[PERK_PSIONICIST_SHARDSTORM];
  perk->id = PERK_PSIONICIST_SHARDSTORM;
  perk->name = strdup("Shardstorm");
  perk->description =
      strdup("Crystal shard converts to AoE when augmented ≥4 PSP; applies bleed rider on hit.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_METACREATIVE_GENIUS;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_SHARD_VOLLEY;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4; /* Requires ≥4 PSP */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Tier 2: Crystal shard becomes room-wide AoE when augmented "
                                     "≥4 PSP; applies bleed on Fort fail.");

  /* Hardened Constructs II */
  perk = &perk_list[PERK_PSIONICIST_HARDENED_CONSTRUCTS_II];
  perk->id = PERK_PSIONICIST_HARDENED_CONSTRUCTS_II;
  perk->name = strdup("Hardened Constructs II");
  perk->description =
      strdup("Summons/creations gain +2 AC, DR 2/—, and attacks count as magic for DR.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_METACREATIVE_GENIUS;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_HARDENED_CONSTRUCTS_I;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2 AC, DR 2 */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Tier 2: Summons gain +2 AC, DR 2/—, and magic attacks.");

  /* Rapid Manifester */
  perk = &perk_list[PERK_PSIONICIST_RAPID_MANIFESTER];
  perk->id = PERK_PSIONICIST_RAPID_MANIFESTER;
  perk->name = strdup("Rapid Manifester");
  perk->description =
      strdup("Once per encounter reduce action time of a metacreative power by one step.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_METACREATIVE_GENIUS;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_FABRICATE_FOCUS;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* One step faster */
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("Tier 2: Once per encounter manifest one Metacreative power one action step faster.");

  /*** METACREATIVE GENIUS - TIER III PERKS ***/

  /* Ectoplasmic Artisan III */
  perk = &perk_list[PERK_PSIONICIST_ECTOPLASMIC_ARTISAN_III];
  perk->id = PERK_PSIONICIST_ECTOPLASMIC_ARTISAN_III;
  perk->name = strdup("Ectoplasmic Artisan III");
  perk->description = strdup(
      "Total –3 PSP (min 1) once per encounter; +30% duration on metacreative buffs and walls.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_METACREATIVE_GENIUS;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_ECTOPLASMIC_ARTISAN_II;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3;     /* Total PSP reduction */
  perk->effect_modifier = 30; /* 30% duration bonus */
  perk->special_description =
      strdup("Tier 3: Metacreativity PSP cost -3 (once/encounter); +30% buff/wall duration.");

  /* Empowered Creation */
  perk = &perk_list[PERK_PSIONICIST_EMPOWERED_CREATION];
  perk->id = PERK_PSIONICIST_EMPOWERED_CREATION;
  perk->name = strdup("Empowered Creation");
  perk->description = strdup("Metacreativity damage powers add +2 dice if augmented by ≥4 PSP.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_METACREATIVE_GENIUS;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_SHARDSTORM;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;    /* +2 dice */
  perk->effect_modifier = 4; /* Requires ≥4 PSP */
  perk->special_description = strdup("Tier 3: Metacreativity damage powers (shrapnel burst, razor "
                                     "storm) gain +2 dice when augmented ≥4 PSP.");

  /* Construct Commander */
  perk = &perk_list[PERK_PSIONICIST_CONSTRUCT_COMMANDER];
  perk->id = PERK_PSIONICIST_CONSTRUCT_COMMANDER;
  perk->name = strdup("Construct Commander");
  perk->description = strdup("Summons gain +1 attack and +10% movement; shambler gains taunt pulse "
                             "(1/round, small radius).");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_METACREATIVE_GENIUS;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_HARDENED_CONSTRUCTS_II;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;     /* +1 attack */
  perk->effect_modifier = 10; /* +10% movement */
  perk->special_description =
      strdup("Tier 3: Summons gain +1 to hit and +10% movement speed; shambler gains taunt pulse.");

  /* Self-Forged */
  perk = &perk_list[PERK_PSIONICIST_SELF_FORGED];
  perk->id = PERK_PSIONICIST_SELF_FORGED;
  perk->name = strdup("Self-Forged");
  perk->description = strdup("When manifesting a metacreative power, gain temp HP = 1/2 manifester "
                             "level (stacks up to manifester level).");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_METACREATIVE_GENIUS;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_RAPID_MANIFESTER;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* 1/2 manifester level */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Tier 3: Gain temp HP = 1/2 manifester level when manifesting "
                                     "Metacreative powers (stacks to manifester level max).");

  /*** METACREATIVE GENIUS - TIER IV (CAPSTONES) ***/

  /* Astral Juggernaut */
  perk = &perk_list[PERK_PSIONICIST_ASTRAL_JUGGERNAUT];
  perk->id = PERK_PSIONICIST_ASTRAL_JUGGERNAUT;
  perk->name = strdup("Astral Juggernaut");
  perk->description = strdup("1/day summon a Large construct with reach, taunt, and force slam "
                             "that scales on manifester level.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_METACREATIVE_GENIUS;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_CONSTRUCT_COMMANDER;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* Once per day */
  perk->effect_modifier = 0;
  perk->special_description = strdup("Tier 4 Capstone: Once per day summon a Large construct with "
                                     "reach, taunt, and force slam; scales with manifester level.");

  /* Perfect Fabricator */
  perk = &perk_list[PERK_PSIONICIST_PERFECT_FABRICATOR];
  perk->id = PERK_PSIONICIST_PERFECT_FABRICATOR;
  perk->name = strdup("Perfect Fabricator");
  perk->description =
      strdup("1/day manifest a metacreative power for free (0 PSP) as a swift action; conjured "
             "gear/construct counts as masterwork/magical for 1 hour.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_METACREATIVE_GENIUS;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_SELF_FORGED;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* Once per day */
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("Tier 4 Capstone: Once per day manifest a Metacreative power for free (0 PSP) as a "
             "swift action; counts as masterwork/magical for 1 hour.");
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
  perk->special_description =
      strdup("Requires max rank Weapon Specialization I; can be taken up to 3 times");

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
  perk->special_description =
      strdup("Grants immunity to fear effects and +2 bonus to saves vs mind-affecting");

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
  perk->special_description =
      strdup("Increases DR to 6/- (replaces Defensive Stance DR) and grants immunity to knockdown");
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
  perk->special_description =
      strdup("When HP drops to 0 or below, you have a 33% chance to remain at 1 HP instead of "
             "dying. Requires max Toughness I and max Resilience.");

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
  perk->description =
      strdup("Additional +1 attack of opportunity per round per rank (max 2 ranks)");
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
  perk->special_description =
      strdup("+4 bonus to trip combat maneuvers, enemies provoke AoO when standing up");

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
  perk->special_description =
      strdup("+4 bonus to disarm combat maneuvers, does not provoke attacks of opportunity");

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
  perk->special_description =
      strdup("+4 bonus to sunder combat maneuvers, deal full damage to objects");

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
  perk->special_description =
      strdup("Allows moving before and after attacking without provoking AoO");

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
  perk->special_description = strdup("Increases damage dealt by damaging spells by +1 per rank. "
                                     "Can be taken 5 times for +5 damage total.");

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
  perk->special_description =
      strdup("All lightning/electricity damage spells deal 10% additional damage.");

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
  perk->special_description =
      strdup("Bonus to overcome spell resistance. Can be taken 3 times for +6 total.");

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
  perk->special_description =
      strdup("Requires Spell Power I (max). Further increases damage by +1 per rank. Can be taken "
             "3 times for +8 total with Spell Power I.");

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
  perk->effect_value = 20;   /* +20% total (replaces +10% from affinity) */
  perk->effect_modifier = 1; /* +1 spell DC */
  perk->special_description =
      strdup("Requires Energy Affinity: Fire. Fire spells deal +20% total damage and have +1 DC.");

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
  perk->effect_value = 20;   /* +20% total (replaces +10% from affinity) */
  perk->effect_modifier = 1; /* +1 spell DC */
  perk->special_description =
      strdup("Requires Energy Affinity: Cold. Cold spells deal +20% total damage and have +1 DC.");

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
  perk->effect_value = 20;   /* +20% total (replaces +10% from affinity) */
  perk->effect_modifier = 1; /* +1 spell DC */
  perk->special_description = strdup("Requires Energy Affinity: Lightning. Lightning spells deal "
                                     "+20% total damage and have +1 DC.");

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
  perk->special_description = strdup("Requires Spell Power I (at least 3 ranks). Damaging spells "
                                     "have a 5% chance to critically strike for 1.5x damage.");

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
  perk->special_description =
      strdup("Requires Spell Power I (max). Once every 5 minutes, you can cast a spell with the "
             "Maximize metamagic effect (maximum damage dice) without increasing the spell level. "
             "Use 'metamagic maximize' before casting.");

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
  perk->special_description =
      strdup("Requires Spell Power II (max). Your mastery of evocation continues to grow.");

  /* Master of Elements */
  perk = &perk_list[PERK_WIZARD_MASTER_OF_ELEMENTS];
  perk->id = PERK_WIZARD_MASTER_OF_ELEMENTS;
  perk->name = strdup("Master of Elements");
  perk->description = strdup("All elemental damage +15%, can change energy type of spells. Use "
                             "'masterofelements' command.");
  perk->associated_class = CLASS_WIZARD;
  perk->perk_category = PERK_CATEGORY_EVOKER;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = 0; /* Special: requires any two Focused Element perks */
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 15; /* +15% to all elemental damage */
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("Requires any two Focused Element perks. You gain mastery over elemental forces, "
             "increasing all elemental spell damage by 15% and gaining the ability to change the "
             "energy type of spells.");

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
  perk->effect_value = 10;     /* 10% chance */
  perk->effect_modifier = 250; /* 2.5x damage stored as 250 (will divide by 100) */
  perk->special_description =
      strdup("Requires Spell Critical I. Your critical strike chance increases to 10% and critical "
             "spells deal 2.5x damage instead of 1.5x.");

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
  perk->special_description =
      strdup("Requires Maximize Spell. Once every 5 minutes, you can cast a spell with the Empower "
             "metamagic effect (+50% damage) without increasing the spell level. Use 'metamagic "
             "empower' before casting.");

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
  perk->special_description = strdup("Requires Spell Penetration I (max). Your spells become even "
                                     "more effective at piercing magical defenses.");

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
  perk->effect_value = 3;    /* 3d6 dice */
  perk->effect_modifier = 2; /* +2 DC */
  perk->special_description =
      strdup("Requires Spell Power III (max) and Master of Elements. Your evocation spells reach "
             "devastating power, dealing an additional 3d6 damage and increasing spell DCs by 2.");

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
  perk->special_description =
      strdup("Requires Spell Penetration II. Your spells are so potent that enemies' spell "
             "resistance is effectively reduced by 10 against your magic.");
}

/* Define Wizard Controller Perks */
void define_wizard_controller_perks(void)
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
  perk->description =
      strdup("Buff spell durations are increased by 5% per rank (minimum +1 round)");
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
  perk->description = strdup("Activate to force target to save twice vs next spell (2 uses, "
                             "regenerates 1 per 5 min). Use 'persistentspell' command.");
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
  perk->description = strdup("Activate to make next enchantment spell affect all enemies in room "
                             "(5 min cooldown). Use 'splitenchantment' command.");
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
  perk->special_description = strdup("Requires Extended Spell II (max). Your spell durations are "
                                     "extended by an additional 5 rounds.");

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
  perk->special_description =
      strdup("Requires Greater Spell Focus: Enchantment. You become a true master of enchantment "
             "magic. Enchantment spell DCs increase by +3 (total +5), and all enchantment spell "
             "durations are doubled.");

  /* Master Illusionist */
  perk = &perk_list[PERK_WIZARD_MASTER_ILLUSIONIST];
  perk->id = PERK_WIZARD_MASTER_ILLUSIONIST;
  perk->name = strdup("Master Illusionist");
  perk->description = strdup("Illusion DC +3 (total +5), enemies get -4 to disbelief");
  perk->associated_class = CLASS_WIZARD;
  perk->perk_category = PERK_CATEGORY_CONTROLLER;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk =
      PERK_WIZARD_SPELL_FOCUS_ENCHANTMENT_3; /* Using enchantment focus as proxy for illusion */
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3; /* +3 DC */
  perk->special_description =
      strdup("You become a true master of illusion magic. Illusion spell DCs increase by +3 (total "
             "+5), and enemies get -4 to disbelief checks.");

  /* Master Transmuter */
  perk = &perk_list[PERK_WIZARD_MASTER_TRANSMUTER];
  perk->id = PERK_WIZARD_MASTER_TRANSMUTER;
  perk->name = strdup("Master Transmuter");
  perk->description = strdup("Transmutation DC +3 (total +5), duration increased 50%");
  perk->associated_class = CLASS_WIZARD;
  perk->perk_category = PERK_CATEGORY_CONTROLLER;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk =
      PERK_WIZARD_SPELL_FOCUS_ENCHANTMENT_4; /* Using enchantment focus as proxy for transmutation */
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3; /* +3 DC */
  perk->special_description =
      strdup("You become a true master of transmutation magic. Transmutation spell DCs increase by "
             "+3 (total +5), and all transmutation spell durations are increased by 50%.");

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
  perk->special_description =
      strdup("Requires Extended Spell III and Spell Mastery. You become an archmage of control "
             "magic. All spells that apply charm, confusion, daze, or sleep effects have their "
             "duration extended by 50% and gain +5 to their save DCs.");

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
  perk->special_description =
      strdup("Requires Persistent Spell and Spell Mastery. Once every 5 minutes, you can use the "
             "'irresistablemagic' command to make your next spell automatically succeed (no saving "
             "throw allowed).");
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
  perk->special_description = strdup("When you cast a spell, there is a 2% chance per rank (max "
                                     "10% at rank 5) that the spell slot is not expended.");

  /* Quick Cast I */
  perk = &perk_list[PERK_WIZARD_QUICK_CAST_I];
  perk->id = PERK_WIZARD_QUICK_CAST_I;
  perk->name = strdup("Quick Cast I");
  perk->description =
      strdup("Use quicken metamagic without spell circle increase once per 5 minutes");
  perk->associated_class = CLASS_WIZARD;
  perk->perk_category = PERK_CATEGORY_VERSATILE_CASTER;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->special_description = strdup("Once every 5 minutes, you can apply quicken metamagic to a "
                                     "spell without increasing its spell circle requirement.");

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
  perk->special_description =
      strdup("Increases your concentration skill by +2 per rank when you are in combat.");

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
  perk->special_description = strdup("Requires Spell Focus I (max). Increases the chance by an "
                                     "additional 2% per rank (total 16% at max ranks of both).");

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
  perk->special_description = strdup("Requires Quick Cast I. Once per combat, you can cast a spell "
                                     "as an instant action without using the quicken metamagic.");

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
  perk->special_description =
      strdup("Requires Spell Focus I (at least 3 ranks). Once per day, use the 'spellrecall' "
             "command to restore a spell slot. For spontaneous casters, restores a random slot. "
             "For preparation casters, randomly advances a spell currently being memorized.");

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
  perk->special_description =
      strdup("When using metamagic feats, reduce the spell circle increase by 1 (minimum 0). "
             "Limited to 2 uses per 5 minutes. Use 'cooldowns' to check remaining uses.");

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
  perk->special_description = strdup("Requires Combat Casting I (at least 2 ranks). You gain +4 AC "
                                     "when casting spells, lasting until your next turn.");

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
  perk->special_description =
      strdup("Requires Spell Focus II (max). Additional 2% chance per rank that spells don't "
             "consume prepared slots (total up to 16% with all ranks).");

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
  perk->special_description =
      strdup("Requires Metamagic Master I. Metamagic feats increase spell circle by 1 less (stacks "
             "with Metamagic Master I for total -2). Limited to 2 uses per 5 minutes. Use "
             "'cooldowns' to check remaining uses.");

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
  perk->special_description =
      strdup("Requires Spell Recall. Once per day, reduce spell preparation/slot recovery time by "
             "50%. Use 'arcane-recovery' command.");

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
  perk->special_description =
      strdup("Requires Defensive Casting. When an enemy initiates combat against you, "
             "automatically activate a spell shield granting 10 DR and +4 AC for 1 round.");

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
  perk->special_description =
      strdup("Requires Spell Focus III (max) and Metamagic Master II. Additional 5% chance spells "
             "don't consume slots, and metamagic reduces spell circle by 1 additional level. "
             "Limited to 2 uses per 5 minutes. Use 'cooldowns' to check remaining uses.");

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
  perk->special_description =
      strdup("Requires Arcane Knowledge I (max) and Metamagic Master II. Gain +2 to all spell DCs, "
             "+2 effective caster level, and +2 spell damage.");
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
  perk->special_description = strdup("Increases healing done by all healing spells by +2 HP per "
                                     "rank. Can be taken 5 times for +10 HP total.");

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
  perk->special_description = strdup("Increases positive energy effects (healing, turn undead, "
                                     "etc.) by +1 per rank. Can be taken 3 times for +3 total.");

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
  perk->special_description =
      strdup("Allows you to prepare and cast one additional healing spell per rest period, "
             "bypassing normal spell slot limits for healing spells only.");

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
  perk->special_description =
      strdup("When healing a target that is below 50% of their maximum HP, healing is increased by "
             "+5 HP per rank. Can be taken 3 times for +15 HP total.");

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
  perk->special_description =
      strdup("Requires Healing Power I (max). Increases healing done by all healing spells by "
             "additional +3 HP per rank. Can be taken 3 times for +9 HP total.");

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
  perk->special_description =
      strdup("Requires Radiant Servant I (max). Increases positive energy effects by additional +2 "
             "per rank. Can be taken 2 times for +4 total.");

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
  perk->special_description =
      strdup("Requires Healing Power I (at least 3 ranks). Mass healing spells can affect 2 "
             "additional targets beyond the normal limit.");

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
  perk->effect_value = 10;     /* 10% chance */
  perk->effect_modifier = 150; /* 150% healing */
  perk->special_description =
      strdup("Requires Healing Power I (at least 3 ranks). Your healing spells have a 10% chance "
             "to be empowered, healing for 150% of their normal amount.");

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
  perk->special_description = strdup(
      "Requires Radiant Servant I (at least 2 ranks). Grants the ability to channel positive "
      "energy to heal all allies in the room for 2d6 HP. Can be used once per short rest.");

  /*** Telepathic Control - Tier III ***/

  /* Dominion */
  perk = &perk_list[PERK_PSIONICIST_DOMINION];
  perk->id = PERK_PSIONICIST_DOMINION;
  perk->name = strdup("Dominion");
  perk->description = strdup(
      "Total +3 Telepathy DCs; charm/dominate effects gain +2 rounds on failed saves (non-boss).");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_TELEPATHIC_CONTROL;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_MIND_SPIKE_II;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3;    /* Total +3 DC */
  perk->effect_modifier = 2; /* +2 rounds on charm/dominate */
  perk->special_description =
      strdup("Telepathy powers gain +3 DC total; charm/dominate extend +2 rounds (non-boss).");

  /* Psychic Sundering */
  perk = &perk_list[PERK_PSIONICIST_PSYCHIC_SUNDERING];
  perk->id = PERK_PSIONICIST_PSYCHIC_SUNDERING;
  perk->name = strdup("Psychic Sundering");
  perk->description = strdup("Telepathy damage powers make targets vulnerable: +10% damage from "
                             "all sources for 3 rounds.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_TELEPATHIC_CONTROL;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_OVERWHELM;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10;   /* +10% damage taken */
  perk->effect_modifier = 3; /* 3 round duration */
  perk->special_description =
      strdup("Telepathy damage applies 3-round 10% vulnerability to all damage sources.");

  /* Mental Backlash */
  perk = &perk_list[PERK_PSIONICIST_MENTAL_BACKLASH];
  perk->id = PERK_PSIONICIST_MENTAL_BACKLASH;
  perk->name = strdup("Mental Backlash");
  perk->description = strdup("When a target saves vs your Telepathy, it still takes (5 + 1/2 "
                             "level) mental damage (no save).");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_TELEPATHIC_CONTROL;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_LINKED_MENACE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5;    /* base chip damage */
  perk->effect_modifier = 2; /* scaling uses level/2 */
  perk->special_description =
      strdup("Successful saves vs your Telepathy still take chip mental damage.");

  /* Piercing Will */
  perk = &perk_list[PERK_PSIONICIST_PIERCING_WILL];
  perk->id = PERK_PSIONICIST_PIERCING_WILL;
  perk->name = strdup("Piercing Will");
  perk->description = strdup(
      "Ignore 5 power resistance when manifesting Telepathy powers (stacks with Disruptor).");
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
  perk->description =
      strdup("When you manifest a hostile Telepathy power against a target, there is a 10% chance "
             "it applies shaken, fatigued, and deafened (3 rounds, Will save negates).");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_TELEPATHIC_CONTROL;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_PIERCING_WILL;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10;   /* 10% chance */
  perk->effect_modifier = 3; /* 3 round duration */
  perk->special_description = strdup("Tier 4 Capstone: 10% chance on Telepathy powers applies "
                                     "shaken/fatigued/deafened (Will save negates).");

  /* Hive Commander */
  perk = &perk_list[PERK_PSIONICIST_HIVE_COMMANDER];
  perk->id = PERK_PSIONICIST_HIVE_COMMANDER;
  perk->name = strdup("Hive Commander");
  perk->description = strdup("On successful hostile Telepathy powers, grant yourself +3 DC to "
                             "further powers vs that target and +2 to-hit for allies (3 rounds).");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_TELEPATHIC_CONTROL;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_PIERCING_WILL;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3;    /* +3 DC bonus */
  perk->effect_modifier = 3; /* 3 round duration */
  perk->special_description = strdup("Tier 4 Capstone: Successful Telepathy powers mark target for "
                                     "+3 DC and grant +2 to-hit to allies.");

  /*** PSYCHOKINETIC ARSENAL - TIER 1 PERKS (1-2 points each) ***/

  /* Kinetic Edge I */
  perk = &perk_list[PERK_PSIONICIST_KINETIC_EDGE_I];
  perk->id = PERK_PSIONICIST_KINETIC_EDGE_I;
  perk->name = strdup("Kinetic Edge I");
  perk->description = strdup("+1 damage die on Psychokinesis blasts (energy ray, crystal shard, "
                             "energy push, concussion blast) if augmented by >=1 PSP.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_PSYCHOKINETIC_ARSENAL;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;    /* +1 damage die */
  perk->effect_modifier = 1; /* requires augmentation >= 1 */
  perk->special_description =
      strdup("Tier 1: +1 damage die on Psychokinesis blasts when augmented by >=1 PSP.");

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
  perk->effect_value = 1;     /* +1 AC */
  perk->effect_modifier = 10; /* +10% duration */
  perk->special_description =
      strdup("Tier 1: Inertial armor/force screen +1 AC and +10% duration.");

  /* Vector Shove */
  perk = &perk_list[PERK_PSIONICIST_VECTOR_SHOVE];
  perk->id = PERK_PSIONICIST_VECTOR_SHOVE;
  perk->name = strdup("Vector Shove");
  perk->description = strdup("Energy push/telekinetic shoves get +2 to the movement check; on "
                             "success deal +1 die force damage.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_PSYCHOKINETIC_ARSENAL;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;    /* +2 to movement check */
  perk->effect_modifier = 1; /* +1 die force damage */
  perk->special_description = strdup(
      "Tier 1: Energy push/telekinetic shoves +2 to movement check; +1 die force on success.");

  /* Energy Specialization */
  perk = &perk_list[PERK_PSIONICIST_ENERGY_SPECIALIZATION];
  perk->id = PERK_PSIONICIST_ENERGY_SPECIALIZATION;
  perk->name = strdup("Energy Specialization");
  perk->description = strdup("Choose an energy type (fire, cold, electric, acid, sonic, force). "
                             "Your Psychokinesis powers of that type gain +1 DC.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_PSYCHOKINETIC_ARSENAL;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;    /* +1 DC */
  perk->effect_modifier = 0; /* chosen energy type stored elsewhere */
  perk->special_description =
      strdup("Tier 1: Choose energy type; Psychokinesis powers of that type gain +1 DC.");

  /*** PSYCHOKINETIC ARSENAL - TIER 2 PERKS (2 points each) ***/

  /* Kinetic Edge II */
  perk = &perk_list[PERK_PSIONICIST_KINETIC_EDGE_II];
  perk->id = PERK_PSIONICIST_KINETIC_EDGE_II;
  perk->name = strdup("Kinetic Edge II");
  perk->description = strdup("Total +2 dice on Psychokinesis blasts when augmented by >=3 PSP; "
                             "energy burst/concussion blast splash +1 die.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_PSYCHOKINETIC_ARSENAL;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_KINETIC_EDGE_I;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;    /* total +2 dice at >=3 PSP (combined with Tier I) */
  perk->effect_modifier = 3; /* requires augmentation >= 3 */
  perk->special_description = strdup(
      "Tier 2: Psychokinesis blasts reach +2 dice at >=3 PSP; burst/concussion splash +1 die.");

  /* Deflective Screen */
  perk = &perk_list[PERK_PSIONICIST_DEFLECTIVE_SCREEN];
  perk->id = PERK_PSIONICIST_DEFLECTIVE_SCREEN;
  perk->name = strdup("Deflective Screen");
  perk->description = strdup("While force screen or inertial armor is active, gain +2 AC vs ranged "
                             "and +2 Reflex; first hit each round is reduced by 5 damage.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_PSYCHOKINETIC_ARSENAL;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_FORCE_SCREEN_ADEPT;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;    /* +2 AC vs ranged */
  perk->effect_modifier = 2; /* +2 Reflex; 5 DR on first hit each round (handled in code) */
  perk->special_description = strdup("Tier 2: +2 AC (ranged) and +2 Reflex while shield/armor "
                                     "active; first hit each round -5 damage.");

  /* Accelerated Manifestation */
  perk = &perk_list[PERK_PSIONICIST_ACCELERATED_MANIFEST];
  perk->id = PERK_PSIONICIST_ACCELERATED_MANIFEST;
  perk->name = strdup("Accelerated Manifestation");
  perk->description = strdup("Once per combat, reduce PSP cost of a Psychokinesis power by 2 (min "
                             "1) and make it a faster action.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_PSYCHOKINETIC_ARSENAL;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;    /* -2 PSP cost */
  perk->effect_modifier = 1; /* faster action */
  perk->special_description = strdup(
      "Tier 2: Once per combat -2 PSP (min 1) and quickened cast for one Psychokinesis power.");

  /* Energy Retort (Perk) */
  perk = &perk_list[PERK_PSIONICIST_ENERGY_RETORT_PERK];
  perk->id = PERK_PSIONICIST_ENERGY_RETORT_PERK;
  perk->name = strdup("Energy Retort");
  perk->description = strdup(
      "When struck in melee while you have an active Psychokinesis affect (force screen, energy "
      "retort, inertial armor), return level-based energy damage (scales with augment).");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_PSYCHOKINETIC_ARSENAL;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Tier 2: Reflect level-based energy damage when hit in melee "
                                     "while shield/armor/retort is active.");

  /*** PSYCHOKINETIC ARSENAL - TIER 3 PERKS (3 points each) ***/

  /* Kinetic Edge III */
  perk = &perk_list[PERK_PSIONICIST_KINETIC_EDGE_III];
  perk->id = PERK_PSIONICIST_KINETIC_EDGE_III;
  perk->name = strdup("Kinetic Edge III");
  perk->description =
      strdup("Total +3 dice on Psychokinesis blasts; energy ray/energy push gain +2 DC.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_PSYCHOKINETIC_ARSENAL;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_KINETIC_EDGE_II;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3;    /* total +3 dice (combined with Tier I and II) */
  perk->effect_modifier = 2; /* +2 DC to energy ray/push */
  perk->special_description =
      strdup("Tier 3: Psychokinesis blasts reach +3 dice total; energy ray/push +2 DC.");

  /* Gravity Well */
  perk = &perk_list[PERK_PSIONICIST_GRAVITY_WELL];
  perk->id = PERK_PSIONICIST_GRAVITY_WELL;
  perk->name = strdup("Gravity Well");
  perk->description = strdup("Once per combat, create an AoE effect that halves speed and prevents "
                             "fleeing (Reflex negates each round); lasts 3 rounds.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_PSYCHOKINETIC_ARSENAL;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3;     /* 3 round duration */
  perk->effect_modifier = 50; /* 50% speed reduction */
  perk->special_description = strdup("Tier 3: Once/combat AoE gravity field halves speed, prevents "
                                     "fleeing (Reflex negates/round, 3 rounds).");

  /* Force Aegis */
  perk = &perk_list[PERK_PSIONICIST_FORCE_AEGIS];
  perk->id = PERK_PSIONICIST_FORCE_AEGIS;
  perk->name = strdup("Force Aegis");
  perk->description = strdup("+3 AC vs ranged/spells while force screen/inertial armor active; "
                             "gain temp HP = manifester level on cast.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_PSYCHOKINETIC_ARSENAL;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_DEFLECTIVE_SCREEN;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3;    /* +3 AC vs ranged/spells */
  perk->effect_modifier = 1; /* temp HP = manifester level */
  perk->special_description = strdup("Tier 3: +3 AC (ranged/spells) while shield/armor active; "
                                     "gain temp HP = manifester level on cast.");

  /* Kinetic Crush */
  perk = &perk_list[PERK_PSIONICIST_KINETIC_CRUSH];
  perk->id = PERK_PSIONICIST_KINETIC_CRUSH;
  perk->name = strdup("Kinetic Crush");
  perk->description = strdup("Forced-movement powers add prone on failed Reflex; if target "
                             "collides, take extra force damage = manifester level.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_PSYCHOKINETIC_ARSENAL;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_VECTOR_SHOVE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;    /* prone on failed save */
  perk->effect_modifier = 1; /* collision damage = manifester level */
  perk->special_description = strdup("Tier 3: Forced-movement powers prone on failed Reflex; "
                                     "collision adds force damage = manifester level.");

  /*** PSYCHOKINETIC ARSENAL - TIER 4 PERKS (Capstones - 5 points each) ***/

  /* Singular Impact */
  perk = &perk_list[PERK_PSIONICIST_SINGULAR_IMPACT];
  perk->id = PERK_PSIONICIST_SINGULAR_IMPACT;
  perk->name = strdup("Singular Impact");
  perk->description = strdup("1/day Psychokinesis strike: heavy force damage, auto-bull rush, and "
                             "stun 1 round (Fort partial: half damage, no stun).");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_PSYCHOKINETIC_ARSENAL;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_KINETIC_EDGE_III;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;    /* 1 round stun */
  perk->effect_modifier = 1; /* 1/day usage */
  perk->special_description = strdup(
      "Tier 4 Capstone: 1/day heavy force strike with auto-bull rush and stun (Fort partial).");

  /* Perfect Deflection */
  perk = &perk_list[PERK_PSIONICIST_PERFECT_DEFLECTION];
  perk->id = PERK_PSIONICIST_PERFECT_DEFLECTION;
  perk->name = strdup("Perfect Deflection");
  perk->description = strdup("1/day reaction: negate one ranged/spell/psionic attack against you "
                             "and reflect it using your casting stat vs the original attacker.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_PSYCHOKINETIC_ARSENAL;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_PSIONICIST_FORCE_AEGIS;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* 1/day usage */
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("Tier 4 Capstone: 1/day reaction negate and reflect one ranged/spell/psionic attack.");

  /*** METACREATIVE GENIUS - TIER 1 PERKS (1-2 points each) ***/

  /* Ectoplasmic Artisan I */
  perk = &perk_list[PERK_PSIONICIST_ECTOPLASMIC_ARTISAN_I];
  perk->id = PERK_PSIONICIST_ECTOPLASMIC_ARTISAN_I;
  perk->name = strdup("Ectoplasmic Artisan I");
  perk->description = strdup("Metacreativity powers cost 1 less PSP (min 1) once per encounter; "
                             "+10% duration on metacreative buffs.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_METACREATIVE_GENIUS;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;     /* PSP reduction */
  perk->effect_modifier = 10; /* 10% duration bonus */
  perk->special_description =
      strdup("Tier 1: Metacreativity PSP cost -1 (once/encounter); +10% buff duration.");

  /* Shard Volley */
  perk = &perk_list[PERK_PSIONICIST_SHARD_VOLLEY];
  perk->id = PERK_PSIONICIST_SHARD_VOLLEY;
  perk->name = strdup("Shard Volley");
  perk->description = strdup(
      "Crystal shard gains +1 projectile (additional attack roll) when augmented by ≥2 PSP.");
  perk->associated_class = CLASS_PSIONICIST;
  perk->perk_category = PERK_CATEGORY_METACREATIVE_GENIUS;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;    /* +1 projectile */
  perk->effect_modifier = 2; /* requires ≥2 PSP augment */
  perk->special_description =
      strdup("Tier 1: Crystal shard gains extra projectile when augmented ≥2 PSP.");

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
  perk->effect_value = 1;    /* +1 AC */
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
  perk->effect_value = 1;    /* +1 HP per tick */
  perk->effect_modifier = 2; /* 2 room range */
  perk->special_description =
      strdup("Requires Radiant Servant I (max). Your presence radiates healing energy. All allies "
             "within 2 rooms of you regenerate an additional +1 HP per regeneration tick.");

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
  perk->special_description = strdup("Requires Healing Power II (max). Increases all healing "
                                     "spells by +5 HP per rank. Maximum 2 ranks for +10 HP total.");

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
  perk->effect_value = 20;     /* 20% chance */
  perk->effect_modifier = 200; /* 200% healing */
  perk->special_description =
      strdup("Requires Empowered Healing I. Critical healing chance increases to 20% and heals for "
             "200% (double) instead of 150%.");

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
  perk->special_description = strdup("Requires Channel Energy: Heal. Your channel positive energy "
                                     "now heals for 4d6 HP instead of 2d6.");

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
  perk->effect_value = 2;    /* +2 HP per tick */
  perk->effect_modifier = 3; /* 3 room range */
  perk->special_description = strdup("Requires Healing Aura I. Your healing aura increases to +2 "
                                     "HP per tick and extends to 3 rooms away.");

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
  perk->special_description = strdup(
      "Requires Healing Power II (at least 2 ranks). When you cast a healing spell, it can remove "
      "one negative condition from the target (poison, disease, curse, blindness, or paralysis).");

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
  perk->effect_value = 20;   /* +20 HP to all healing */
  perk->effect_modifier = 6; /* Channel becomes 6d6 */
  perk->special_description =
      strdup("Requires Healing Power III (max) and Channel Energy: Greater Heal. Capstone: All "
             "healing spells gain +20 HP, channel energy heals for 6d6 HP, and healing aura grants "
             "+3 HP per tick.");

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
  perk->effect_value = 1;    /* Once per day */
  perk->effect_modifier = 4; /* +4 to saves */
  perk->special_description = strdup(
      "Requires Healing Aura II and Mass Healing Focus. Capstone: Once per day, you can fully heal "
      "all allies in the area and grant them +4 to all saving throws for 10 rounds.");

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
  perk->special_description =
      strdup("Increases your attack bonus by +1 per rank. Maximum 3 ranks for +3 total.");

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
  perk->special_description = strdup(
      "Your melee attacks deal +2 holy damage per rank. Maximum 3 ranks for +6 holy damage total.");

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
  perk->special_description =
      strdup("Divine protection grants +1 AC per rank. Maximum 3 ranks for +3 AC total.");

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
  perk->special_description =
      strdup("You can cast self-targeting buff spells as a swift action instead of a standard "
             "action, allowing you to buff and attack in the same round.");

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
  perk->effect_value = 2;    /* 2d6 damage */
  perk->effect_modifier = 1; /* Once per combat */
  perk->special_description = strdup("Once per combat, you can channel divine fury to deal +2d6 "
                                     "holy damage against an evil-aligned enemy.");

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
  perk->special_description =
      strdup("Requires Divine Favor I (max). Increases your attack bonus by an additional +1 per "
             "rank. Maximum 2 ranks for +2 total (combined +5 with Tier 1).");

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
  perk->special_description =
      strdup("Requires Holy Weapon I (max). Your melee attacks deal an additional +3 holy damage "
             "per rank. Maximum 2 ranks for +6 holy damage (combined +12 with Tier 1).");

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
  perk->special_description =
      strdup("Requires Armor of Faith I (max). Divine protection grants an additional +1 AC per "
             "rank. Maximum 2 ranks for +2 AC (combined +5 with Tier 1).");

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
  perk->effect_value = 4;    /* 4d6 damage */
  perk->effect_modifier = 2; /* Twice per combat */
  perk->special_description = strdup("Requires Smite Evil I. Smite damage increases to +4d6 holy "
                                     "damage and can be used twice per combat.");

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
  perk->special_description =
      strdup("Requires Divine Favor I (at least 2 ranks). You channel divine power into your melee "
             "attacks, gaining +2 to all melee damage rolls.");

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
  perk->special_description =
      strdup("Grants the ability to channel negative energy to harm living enemies in the room for "
             "2d6 damage. Can be used in conjunction with the existing channel energy ability.");

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
  perk->special_description =
      strdup("Requires Holy Weapon I (at least 2 ranks). You can summon a spiritual weapon that "
             "attacks your enemies independently for 5 rounds. The weapon uses your base attack "
             "bonus and deals weapon damage.");

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
  perk->special_description =
      strdup("Requires Divine Favor II (max). Divine favor continues to strengthen your attacks, "
             "granting an additional +2 to hit with melee weapons.");

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
  perk->special_description = strdup(
      "Requires Holy Weapon II (max). Your weapons become truly sanctified, dealing an additional "
      "+5 holy damage and counting as holy for purposes of overcoming damage reduction.");

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
  perk->special_description = strdup("Requires Armor of Faith II (max). Divine protection reaches "
                                     "its peak, granting +2 AC and +2 to all saving throws.");

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
  perk->effect_value = 6;    /* 6d6 damage */
  perk->effect_modifier = 3; /* 3 uses per combat */
  perk->special_description =
      strdup("Requires Smite Evil II. Your smite becomes even more powerful, dealing 6d6 holy "
             "damage and usable 3 times per combat.");

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
  perk->special_description =
      strdup("Requires Channel Energy: Harm. Your negative energy channeling becomes more potent, "
             "dealing 4d6 damage instead of 2d6.");

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
  perk->special_description =
      strdup("Requires Holy Weapon II (at least 1 rank). When you score a critical hit with a "
             "melee weapon, divine wrath adds an extra 2d6 holy damage.");

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
  perk->special_description = strdup(
      "Requires Divine Favor III and Armor of Faith III. Transform into an avatar of divine war, "
      "gaining +3 to hit, +3 AC, and +10 to damage for 10 rounds. Usable once per day.");

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
  perk->special_description =
      strdup("Requires Smite Evil III and Holy Weapon III. Divine wrath flows through your "
             "attacks. Smite Evil deals +10d6 damage, Channel Harm deals 6d6, and all holy damage "
             "from Holy Weapon is increased by +10.");

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
  perk->special_description = strdup("Increases the save DC of spells from your chosen domains by "
                                     "+1 per rank. Can be taken 3 times for +3 DC total.");

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
  perk->special_description = strdup("Increases damage dealt by all divine offensive spells by +1 "
                                     "per rank. Can be taken 5 times for +5 damage total.");

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
  perk->special_description =
      strdup("Allows you to prepare one additional domain spell per rank when you prepare spells. "
             "Can be taken 5 times for +5 domain spell slots total.");

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
  perk->special_description = strdup("Increases the save DC for your turn undead ability by +1 per "
                                     "rank. Can be taken 3 times for +3 DC total.");

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
  perk->special_description =
      strdup("Requires Domain Focus I (max). Further increases domain spell DC by +1 per rank. Can "
             "be taken 2 times for +5 DC total with Domain Focus I.");

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
  perk->special_description =
      strdup("Requires Divine Spell Power I (max). Further increases divine spell damage by +2 per "
             "rank. Can be taken 3 times for +11 damage total with Divine Spell Power I.");

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
  perk->special_description = strdup(
      "Requires Bonus Domain Spell I (max). Allows you to prepare one additional spell of any "
      "level per rank. Can be taken 3 times for +8 total spell slots with Bonus Domain Spell I.");

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
  perk->special_description =
      strdup("Requires Turn Undead Enhancement I (max). Further increases turn undead DC by +2 per "
             "rank. Can be taken 2 times for +7 DC total with Turn Undead Enhancement I.");

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
  perk->special_description = strdup("Requires Domain Focus I (at least 2 ranks). Domain spells "
                                     "have their duration extended by +5 rounds.");

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
  perk->special_description =
      strdup("Requires Bonus Domain Spell I (at least 3 ranks). When applying metamagic feats to "
             "divine spells, the effective spell level increase is reduced by 1 (minimum +0). For "
             "example, Empower Spell normally increases spell level by +2, but with this perk it "
             "only increases by +1.");

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
  perk->special_description = strdup(
      "Requires Turn Undead Enhancement I (max). When you successfully turn undead, weak undead (3 "
      "HD or less below your cleric level) are instantly destroyed instead of fleeing.");

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
  perk->special_description = strdup("Requires Domain Focus II (max). Grants +2 DC to all domain "
                                     "spell saving throws. Stacks with Domain Focus I and II.");

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
  perk->special_description = strdup(
      "Requires Divine Spell Power II (max). Grants +3 bonus damage to all divine spells per rank. "
      "Can be taken 2 times for +6 total. Stacks with Divine Spell Power I and II.");

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
  perk->special_description =
      strdup("Requires Bonus Domain Spell II (max). Grants +1 bonus domain spell slot of any level "
             "per rank. Can be taken 2 times. Regenerates 1 slot per 5 minutes.");

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
  perk->special_description = strdup(
      "Requires Divine Metamagic I. When applying metamagic feats to your divine spells, the spell "
      "level increase is reduced by 2. Stacks with Divine Metamagic I for -3 total.");

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
  perk->special_description =
      strdup("Requires Turn Undead Enhancement II (max). Your turn undead ability can affect "
             "undead up to 2 HD levels higher than normal.");

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
  perk->special_description = strdup(
      "Requires Extended Domain. You can use your domain powers one additional time per day.");

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
  perk->special_description =
      strdup("Requires Domain Focus III. CAPSTONE: All divine spells gain +3 DC and +10 damage. "
             "Domain powers can be used twice as often per day.");

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
  perk->special_description =
      strdup("Requires Greater Turning. CAPSTONE: Turn undead DC +5, can control turned undead, "
             "destroy undead up to 10 HD below your cleric level.");
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
  perk->special_description =
      strdup("Adds 1d6 sneak attack damage per rank. Can be taken 5 times for +5d6 total.");

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
  perk->special_description =
      strdup("Adds +1 damage to ranged sneak attacks per rank. Can be taken 3 times.");

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
  perk->special_description = strdup(
      "Requires Sneak Attack I at max rank. Adds additional 1d6 sneak attack damage per rank.");

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
  perk->special_description = strdup(
      "Requires Vital Strike. Grants additional +2 bonus to critical confirmation (+4 total).");

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
  perk->special_description = strdup("Requires Sneak Attack I (at least 3 ranks). Attacks from "
                                     "stealth deal +2d6 additional damage.");

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
  perk->special_description =
      strdup("Requires Deadly Aim I at max rank. Adds +2 damage to ranged sneak attacks per rank.");

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
  perk->effect_value = 50;   /* 50% movement reduction */
  perk->effect_modifier = 3; /* Duration: 3 rounds */
  perk->special_description = strdup(
      "Requires Sneak Attack I (at least 3 ranks). Sneak attacks apply movement speed reduction.");

  /* Bleeding Attack */
  perk = &perk_list[PERK_ROGUE_BLEEDING_ATTACK];
  perk->id = PERK_ROGUE_BLEEDING_ATTACK;
  perk->name = strdup("Bleeding Attack");
  perk->description =
      strdup("Sneak attacks cause target to bleed for 1d6 damage per round (5 rounds)");
  perk->associated_class = CLASS_ROGUE;
  perk->perk_category = PERK_CATEGORY_ASSASSIN;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_SNEAK_ATTACK_1;
  perk->prerequisite_rank = 3; /* Must have at least 3 ranks of Sneak Attack I */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;    /* 1d6 damage per round */
  perk->effect_modifier = 5; /* Duration: 5 rounds */
  perk->special_description = strdup(
      "Requires Sneak Attack I (at least 3 ranks). Sneak attacks apply bleeding damage over time.");

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
  perk->special_description = strdup(
      "Requires Sneak Attack II at max rank. Adds +2d6 sneak attack damage per rank (total +4d6).");

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
  perk->special_description = strdup(
      "Requires Assassinate I. Attacks from stealth deal +4d6 additional damage (+6d6 total).");

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
  perk->special_description =
      strdup("Requires Improved Vital Strike. Critical hits deal +2d6 precision damage.");

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
  perk->special_description = strdup(
      "Requires Opportunist I. All attacks of opportunity automatically count as sneak attacks.");

  /* Death Attack */
  perk = &perk_list[PERK_ROGUE_DEATH_ATTACK];
  perk->id = PERK_ROGUE_DEATH_ATTACK;
  perk->name = strdup("Death Attack");
  perk->description = strdup("Gives +2 to backstab dc modifiers to apply death attack. Requires "
                             "the death attack assassin class ability.");
  perk->associated_class = CLASS_ROGUE;
  perk->perk_category = PERK_CATEGORY_ASSASSIN;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_ASSASSINATE_1;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("Requires Assassinate I and Sneak Attack II (at least 2 ranks). Study a target for 1 "
             "round, then your next sneak attack forces a Fortitude save or the target dies.");

  /*** ASSASSIN TREE - TIER 4 CAPSTONE PERKS (5 points each) ***/

  /* Master Assassin */
  perk = &perk_list[PERK_ROGUE_MASTER_ASSASSIN];
  perk->id = PERK_ROGUE_MASTER_ASSASSIN;
  perk->name = strdup("Master Assassin");
  perk->description =
      strdup("All sneak attacks +5d6, critical threat range +1, sneak from any position");
  perk->associated_class = CLASS_ROGUE;
  perk->perk_category = PERK_CATEGORY_ASSASSIN;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_SNEAK_ATTACK_3;
  perk->prerequisite_rank = 2; /* Must have max rank (2) of Sneak Attack III */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5;    /* +5d6 sneak attack damage */
  perk->effect_modifier = 1; /* +1 critical threat range */
  perk->special_description = strdup(
      "Requires Sneak Attack III at max rank and Assassinate II. All sneak attacks gain +5d6 "
      "damage, critical threat range increases by 1, and you can sneak attack from any position.");

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
  perk->special_description =
      strdup("Requires Critical Precision and Death Attack. Once per combat, you can declare one "
             "attack to be an automatic critical hit with maximum damage.");

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
  perk->special_description = strdup("Grants +2 to stealth, sleight of hand, perception, disable "
                                     "device, and pick locks per rank.");

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
  perk->special_description =
      strdup("Grants +3 bonus to perception for finding traps and disable device per rank.");

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
  perk->effect_value = 3;     /* +3 to saves vs area effects */
  perk->effect_modifier = -1; /* Applies to all save types */
  perk->special_description = strdup(
      "+3 bonus to saving throws against area of effect spells. When you succeed on a Reflex save "
      "against an area effect attack, you take half damage instead of full damage.");

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
  perk->special_description = strdup(
      "Requires Skill Mastery I at max rank. Grants additional +3 to all rogue skills per rank.");

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
  perk->special_description = strdup("Requires Trapfinding Expert I at max rank. Grants additional "
                                     "+4 to find and disable traps per rank.");

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
  perk->special_description = strdup("Requires Fast Hands I at max rank. Grants additional +3 to "
                                     "sleight of hand and pick locks per rank.");

  /* Improved Evasion */
  perk = &perk_list[PERK_ROGUE_IMPROVED_EVASION];
  perk->id = PERK_ROGUE_IMPROVED_EVASION;
  perk->name = strdup("Improved Evasion");
  perk->description =
      strdup("+3 to saves vs area effects, half damage on fail, no damage on success");
  perk->associated_class = CLASS_ROGUE;
  perk->perk_category = PERK_CATEGORY_MASTER_THIEF;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_EVASION_TRAINING;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SAVE;
  perk->effect_value = 3;     /* +3 to saves vs area effects */
  perk->effect_modifier = -1; /* Applies to all save types */
  perk->special_description = strdup(
      "Requires Evasion Training. +3 bonus to saving throws against area of effect spells. Take no "
      "damage on successful Reflex save, half damage on failed save against area effects.");

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
  perk->effect_value = 2;    /* +2 saves vs traps per rank */
  perk->effect_modifier = 2; /* +2 AC vs trap attacks per rank */
  perk->special_description = strdup("Requires Trapfinding Expert I (at least 2 ranks). Grants +2 "
                                     "to saves vs traps and +2 AC vs trap attacks per rank.");

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
  perk->special_description = strdup("Requires Evasion Training. Once per day, when reduced to 0 "
                                     "or fewer hit points, you automatically stabilize at 1 HP.");

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
  perk->special_description =
      strdup("Requires Trapfinding Expert I at max rank. When you successfully disable a trap, you "
             "can salvage crafting components from it.");

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
  perk->special_description = strdup(
      "Requires Skill Mastery II at max rank. Grants additional +4 to all rogue skills per rank.");

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
  perk->special_description = strdup("Requires Trapfinding Expert II at max rank. Grants "
                                     "additional +5 to find and disable traps per rank.");

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
  perk->special_description = strdup("Requires Fast Hands II at max rank. Grants additional +4 to "
                                     "sleight of hand and pick locks per rank.");

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
  perk->effect_value = 5;    /* +5 to stealth */
  perk->effect_modifier = 1; /* Can hide while observed */
  perk->special_description =
      strdup("Requires Skill Mastery II (at least 2 ranks). Grants +5 bonus to stealth and allows "
             "you to attempt to hide even while being observed.");

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
  perk->effect_value = 3;    /* +3 saves vs traps per rank */
  perk->effect_modifier = 3; /* +3 AC vs trap attacks per rank */
  perk->special_description = strdup("Requires Trap Sense I at max rank. Grants additional +3 to "
                                     "saves vs traps and +3 AC vs trap attacks per rank.");

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
  perk->effect_value = 10;   /* +10 to all rogue skills */
  perk->effect_modifier = 1; /* Can take 10 on rogue skills */
  perk->special_description =
      strdup("Requires Skill Mastery III at max rank. The ultimate expression of rogue mastery. "
             "Grants +10 to all rogue skills and allows you to 'take 10' on any rogue skill check, "
             "even in combat or under pressure.");

  /* Legendary Reflexes */
  perk = &perk_list[PERK_ROGUE_LEGENDARY_REFLEXES];
  perk->id = PERK_ROGUE_LEGENDARY_REFLEXES;
  perk->name = strdup("Legendary Reflexes");
  perk->description =
      strdup("+5 to all saves, take no damage from area effects on successful save");
  perk->associated_class = CLASS_ROGUE;
  perk->perk_category = PERK_CATEGORY_MASTER_THIEF;
  perk->cost = 4;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_IMPROVED_EVASION;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5;    /* +5 to all saves */
  perk->effect_modifier = 1; /* Take no damage from area effects on successful save */
  perk->special_description =
      strdup("Requires Improved Evasion. Your reflexes have reached legendary status. Grants +5 to "
             "all saving throws and you take no damage from area effects on successful saves (even "
             "Fortitude/Will saves), and only half damage on failed saves.");

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
  perk->effect_value = 3;                     /* +3 per rank */
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
  perk->special_description = strdup("Your footsteps are so light that you don't trigger "
                                     "movement-based traps or pressure plates.");

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
  perk->special_description =
      strdup("Requires Stealth Mastery I at max rank. Grants additional +4 to stealth per rank.");

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
  perk->special_description = strdup(
      "Requires Fleet of Foot I at max rank. Grants additional +10 movement speed per rank.");

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
  perk->special_description = strdup(
      "Requires Awareness I at max rank. Grants additional +4 to perception/search per rank.");

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
  perk->special_description =
      strdup("Requires Stealth Mastery I (at least 3 ranks). You can attempt to hide even while "
             "being observed, though this is more difficult than normal.");

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
  perk->special_description = strdup(
      "Requires Stealth Mastery I (at least 3 ranks). You can use your bonus action to teleport up "
      "to 10 feet to an unoccupied space you can see that is in dim light or darkness.");

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
  perk->special_description =
      strdup("Requires Awareness I (at least 2 ranks). You cannot be caught flat-footed and always "
             "retain your Dexterity bonus to AC, even when surprised.");

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
  perk->effect_value = 3;    /* +3 to acrobatics per rank */
  perk->effect_modifier = 1; /* +1 AC per rank */
  perk->special_description = strdup(
      "Requires Fleet of Foot I (at least 2 ranks). Grants +3 to acrobatics and +1 AC per rank.");

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
  perk->special_description =
      strdup("Requires Stealth Mastery II (max). Grants an additional +5 to stealth per rank.");

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
  perk->effect_value = 15;   /* +15 movement speed */
  perk->effect_modifier = 1; /* Disengage as free action flag */
  perk->special_description = strdup("Requires Fleet of Foot II (max). Grants +15 movement speed "
                                     "and allows you to disengage from combat as a free action.");

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
  perk->effect_value = 5;     /* +5 to perception/search */
  perk->effect_modifier = 10; /* Blindsense 10 ft */
  perk->special_description = strdup("Requires Awareness II (max). Grants +5 to perception and "
                                     "search skills, and provides blindsense within 10 feet.");

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
  perk->effect_value = 2;    /* +2 AC vs attacks of opportunity */
  perk->effect_modifier = 1; /* Cannot be flanked flag */
  perk->special_description = strdup("Requires Uncanny Dodge I. You cannot be flanked and gain +2 "
                                     "AC against attacks of opportunity.");

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
  perk->effect_value = 5;    /* +5 to acrobatics */
  perk->effect_modifier = 2; /* +2 AC */
  perk->special_description =
      strdup("Requires Acrobatics I (max). Grants +5 to acrobatics and +2 AC.");

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
  perk->effect_value = 3;    /* 3 rounds of invisibility */
  perk->effect_modifier = 1; /* Once per combat flag */
  perk->special_description = strdup("Requires Hide in Plain Sight. Once per combat, you can "
                                     "vanish, becoming invisible for 3 rounds.");

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
  perk->prerequisite_rank =
      2; /* Must have max ranks of Stealth Mastery III AND Hide in Plain Sight */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 20;   /* +20 to stealth */
  perk->effect_modifier = 1; /* Auto-hide after attacks flag */
  perk->special_description =
      strdup("Requires Stealth Mastery III (max) and Hide in Plain Sight. You achieve perfect "
             "stealth mastery, automatically hiding after attacks and gaining +20 to stealth.");

  /* Ghost */
  perk = &perk_list[PERK_ROGUE_GHOST];
  perk->id = PERK_ROGUE_GHOST;
  perk->name = strdup("Ghost");
  perk->description =
      strdup("+30 movement, immune to attacks of opportunity, pass through enemies");
  perk->associated_class = CLASS_ROGUE;
  perk->perk_category = PERK_CATEGORY_SHADOW_SCOUT;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ROGUE_FLEET_OF_FOOT_3;
  perk->prerequisite_rank = 1; /* Must have Fleet of Foot III AND Uncanny Dodge II */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 30;   /* +30 movement speed */
  perk->effect_modifier = 1; /* Immune to AoO and pass through enemies flag */
  perk->special_description =
      strdup("Requires Fleet of Foot III and Uncanny Dodge II. You move like a ghost, gaining +30 "
             "movement speed, immunity to attacks of opportunity, and the ability to pass through "
             "enemies.");
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
  perk->description = strdup(
      "Grants a 5% chance per rank to immediately fire an extra arrow/missile on ranged attacks");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_HUNTER;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10;
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("On ranged attacks, 5% chance per rank to make one extra immediate shot");

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
  perk->special_description =
      strdup("+2 to hit with ranged weapons per rank (stacks with Archer's Focus I)");

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
  perk->description = strdup(
      "Once per combat, fire an additional arrow at your target (does not consume ammunition)");
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
  perk->description =
      strdup("Mark a target for 5 rounds; +2 to hit and +1d6 damage against marked target");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_HUNTER;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_RANGER_ARCHERS_FOCUS_I;
  perk->prerequisite_rank = 1; /* Requires Archer's Focus I (1 rank) */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("Marks target for 5 rounds granting +2 to hit and +1d6 damage");

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
  perk->effect_value = 2;    /* +2 additional arrows */
  perk->effect_modifier = 0; /* cooldown handled in code */
  perk->special_description =
      strdup("Upgrades Manyshot: +2 arrows, shorter cooldown (approx. twice per combat)");

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
  perk->special_description =
      strdup("Future hook: extends ranged bands and removes long-range penalties");

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
  perk->special_description =
      strdup("Ranged attacks ignore concealment and cover where applicable");

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
  perk->special_description =
      strdup("Capstone: 19-20 crits and x4 crit multiplier for ranged attacks");

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
  perk->special_description =
      strdup("Capstone active: 'arrowstorm' command to deal 6d6 to all foes in room, 24h cooldown");

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
  perk->effect_value = 5;    /* +5 HP per rank */
  perk->effect_modifier = 1; /* +1 AC per rank */
  perk->special_description = strdup("Animal companion gains +5 HP and +1 AC per rank");

  /* Pack Tactics I */
  perk = &perk_list[PERK_RANGER_PACK_TACTICS_I];
  perk->id = PERK_RANGER_PACK_TACTICS_I;
  perk->name = strdup("Pack Tactics I");
  perk->description =
      strdup("You and your animal companion gain +1 to hit when flanking the same enemy per rank");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_BEAST_MASTER;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* +1 per rank */
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("+1 to hit when you and your companion attack the same target per rank");

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
  perk->effect_value = 10;   /* +10 HP per rank */
  perk->effect_modifier = 2; /* +2 AC and +2 to-hit per rank */
  perk->special_description = strdup(
      "Companion gains +10 HP, +2 AC, and +2 to hit per rank (stacks with Enhanced Companion I)");

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
  perk->description =
      strdup("Beneficial spells cast on yourself also affect your animal companion");
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
  perk->description =
      strdup("When you and your companion attack the same target, both gain +2d4 damage");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_BEAST_MASTER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_RANGER_PACK_TACTICS_I;
  perk->prerequisite_rank =
      3; /* Requires Pack Tactics I (max rank) AND Feral Charge (checked separately) */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2; /* +2d4 */
  perk->effect_modifier = 4;
  perk->special_description =
      strdup("Requires: Pack Tactics I (max rank) and Feral Charge. You and companion both gain "
             "+2d4 damage when attacking same target");

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
  perk->special_description =
      strdup("You and your companion regenerate 1 HP per round during combat");

  /* Greater Summons */
  perk = &perk_list[PERK_RANGER_GREATER_SUMMONS];
  perk->id = PERK_RANGER_GREATER_SUMMONS;
  perk->name = strdup("Greater Summons");
  perk->description =
      strdup("All summoned creatures have +25% HP, +4 to attack rolls and deal +1d6 damage");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_BEAST_MASTER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_RANGER_SPELL_FOCUS_CONJURATION_I;
  perk->prerequisite_rank = 2; /* Requires Spell Focus: Conjuration I (max rank) */
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 25;   /* +25% HP */
  perk->effect_modifier = 4; /* +4 to attack rolls */
  perk->special_description =
      strdup("All summoned creatures gain +25% HP, +4 to attack rolls, and +1d6 damage");

  /*** BEAST MASTER TREE - TIER IV (CAPSTONES) ***/

  /* Primal Avatar */
  perk = &perk_list[PERK_RANGER_PRIMAL_AVATAR];
  perk->id = PERK_RANGER_PRIMAL_AVATAR;
  perk->name = strdup("Primal Avatar");
  perk->description = strdup("Your animal companion becomes a Primal Beast: +50 HP, +5 AC, +5 to "
                             "hit, attacks deal +3d6 damage, immune to mind-affecting");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_BEAST_MASTER;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_RANGER_ALPHA_BOND;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 50;   /* +50 HP */
  perk->effect_modifier = 5; /* +5 AC/to-hit, +3d6 damage handled separately */
  perk->special_description = strdup(
      "Capstone: Companion becomes Primal Beast with massive bonuses and mind-affecting immunity");

  /* Nature's Wrath */
  perk = &perk_list[PERK_RANGER_NATURES_WRATH];
  perk->id = PERK_RANGER_NATURES_WRATH;
  perk->name = strdup("Nature's Wrath");
  perk->description = strdup("Once per day: you and your companion gain +4 to all stats, +2d8 "
                             "damage on all attacks, and fast healing 5 for 10 rounds");
  perk->associated_class = CLASS_RANGER;
  perk->perk_category = PERK_CATEGORY_BEAST_MASTER;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_RANGER_COORDINATED_ATTACK;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4;     /* +4 to stats */
  perk->effect_modifier = 10; /* 10 rounds duration */
  perk->special_description = strdup("Capstone active: 'natureswrath' command grants powerful "
                                     "temporary bonuses to you and companion, 24h cooldown");

  /*** WILDERNESS WARRIOR TREE (partial) ***/

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
  perk->special_description =
      strdup("10% chance per round for an extra off-hand attack when dual wielding");

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
  perk->description =
      strdup("+2 to hit against favored enemies, critical threat range +1 vs favored enemies");
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
  perk->description =
      strdup("5% chance per hit to make an additional free attack when dual wielding");
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
  perk->description =
      strdup("Against favored enemies: +2d6 damage and attacks ignore 10 points of DR");
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
  perk->description =
      strdup("Successful melee attacks have a 5% chance to apply slow for 3 rounds");
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
  perk->description = strdup("Bard songs gain +1 affect round and +1 effectiveness per rank");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SPELLSINGER;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("Bard songs gain +1 affect round and +1 effectiveness per rank");

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
  perk->description = strdup("Group members affected by your songs gain +1 competence to Will "
                             "saves against mind-affecting effects per rank");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SPELLSINGER;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("Song recipients gain +1 Will vs. mind-affecting effects per rank");

  /* Harmonic Casting */
  perk = &perk_list[PERK_BARD_HARMONIC_CASTING];
  perk->id = PERK_BARD_HARMONIC_CASTING;
  perk->name = strdup("Harmonic Casting");
  perk->description = strdup("Casting a bard spell no longer interrupts your active performances");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SPELLSINGER;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Bard spells do not interrupt active performances");

  /*** SPELLSINGER TREE - TIER II ***/

  /* Songweaver II */
  perk = &perk_list[PERK_BARD_SONGWEAVER_II];
  perk->id = PERK_BARD_SONGWEAVER_II;
  perk->name = strdup("Songweaver II");
  perk->description =
      strdup("Additional +1 affect round and +1 effectiveness per rank (stacks with Songweaver I)");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SPELLSINGER;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_BARD_SONGWEAVER_I;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("Additional +1 affect round and +1 effectiveness per rank");

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
  perk->special_description =
      strdup("Additional +1 DC to Enchantment and Illusion spells per rank");

  /* Crescendo */
  perk = &perk_list[PERK_BARD_CRESCENDO];
  perk->id = PERK_BARD_CRESCENDO;
  perk->name = strdup("Crescendo");
  perk->description = strdup("The first Bard spell you cast while performing after starting a "
                             "song gains +2 save DC and deals +1d6 sonic damage once to each "
                             "target it damages");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SPELLSINGER;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_HARMONIC_CASTING;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 6;
  perk->effect_modifier = 2;
  perk->special_description =
      strdup("First Bard spell after starting a song: +2 save DC and +1d6 sonic per target");

  /* Sustaining Melody */
  perk = &perk_list[PERK_BARD_SUSTAINING_MELODY];
  perk->id = PERK_BARD_SUSTAINING_MELODY;
  perk->name = strdup("Sustaining Melody");
  perk->description = strdup("While performing in combat, each five-second pulse has a 20% chance "
                             "to recover one expended Bard spell slot");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SPELLSINGER;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_SONGWEAVER_I;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 20;
  perk->effect_modifier = 1;
  perk->special_description = strdup(
      "20% chance per five-second combat pulse to recover one Bard spell slot while performing");

  /* ========================================================================
   * TIER III - SPELLSINGER TREE
   * ======================================================================== */

  /* Master of Motifs */
  perk = &perk_list[PERK_BARD_MASTER_OF_MOTIFS];
  perk->id = PERK_BARD_MASTER_OF_MOTIFS;
  perk->name = strdup("Master of Motifs");
  perk->description = strdup("Maintain up to two distinct bard songs simultaneously");
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
  perk->description = strdup("While performing, enemies in the room suffer -2 to concentration "
                             "checks and take 1d6 sonic damage on each eleven-second verse");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SPELLSINGER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_CRESCENDO;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 6;     /* 1d6 damage */
  perk->effect_modifier = -2; /* concentration penalty */
  perk->special_description =
      strdup("Enemies: -2 concentration; 1d6 sonic damage each eleven-second verse");

  /* Heightened Harmony */
  perk = &perk_list[PERK_BARD_HEIGHTENED_HARMONY];
  perk->id = PERK_BARD_HEIGHTENED_HARMONY;
  perk->name = strdup("Heightened Harmony");
  perk->description = strdup(
      "When you spend metamagic on a bard spell, you gain +5 to your perform skill for one minute");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SPELLSINGER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_ENCHANTERS_GUILE_II;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5;     /* perform bonus */
  perk->effect_modifier = 60; /* duration in seconds */
  perk->special_description = strdup("Metamagic grants +5 perform for 1 minute");

  /* Protective Chorus */
  perk = &perk_list[PERK_BARD_PROTECTIVE_CHORUS];
  perk->id = PERK_BARD_PROTECTIVE_CHORUS;
  perk->name = strdup("Protective Chorus");
  perk->description = strdup("While performing, you and grouped allies in the room gain +2 to "
                             "saves vs. spells and +2 AC vs. attacks of opportunity");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SPELLSINGER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_RESONANT_VOICE_I;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;    /* save bonus */
  perk->effect_modifier = 2; /* AC bonus */
  perk->special_description =
      strdup("Active grouped aura: +2 saves vs. spells and +2 AC vs. attacks of opportunity");

  /* Tier IV Spellsinger Perks - Capstones */

  /* Spellsong Maestra */
  perk = &perk_list[PERK_BARD_SPELLSONG_MAESTRA];
  perk->id = PERK_BARD_SPELLSONG_MAESTRA;
  perk->name = strdup("Spellsong Maestra");
  perk->description = strdup("While performing, Bard spells gain +2 caster level and +2 spell DC, "
                             "and metamagic adds no spell-circle surcharge");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SPELLSINGER;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_MASTER_OF_MOTIFS;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;    /* caster level bonus */
  perk->effect_modifier = 2; /* DC bonus */
  perk->special_description = strdup("Capstone: +2 caster level and +2 DC on Bard spells while "
                                     "performing; no metamagic circle surcharge");

  /* Aria of Stasis */
  perk = &perk_list[PERK_BARD_ARIA_OF_STASIS];
  perk->id = PERK_BARD_ARIA_OF_STASIS;
  perk->name = strdup("Aria of Stasis");
  perk->description = strdup("While performing, you and grouped allies in the room gain +4 to all "
                             "saves and slow immunity; other creatures in the room suffer -2 to "
                             "hit and 10% lower movement speed");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SPELLSINGER;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_PROTECTIVE_CHORUS;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4;    /* ally save bonus */
  perk->effect_modifier = 2; /* enemy tohit penalty */
  perk->special_description = strdup(
      "Active grouped aura: allies +4 saves/slow immunity; others -2 hit/10% movement speed");

  /* Symphonic Resonance */
  perk = &perk_list[PERK_BARD_SYMPHONIC_RESONANCE];
  perk->id = PERK_BARD_SYMPHONIC_RESONANCE;
  perk->name = strdup("Symphonic Resonance");
  perk->description = strdup("Each eleven-second verse while performing grants 1d6 temporary HP, "
                             "capped at 30 above maximum HP. After a successful Enchantment or "
                             "Illusion Bard spell, valid room enemies that fail a Will save are "
                             "dazed for 1 round");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SPELLSINGER;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_CRESCENDO;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 6;    /* temp HP d6 */
  perk->effect_modifier = 1; /* daze duration */
  perk->special_description = strdup("Capstone: 1d6 temporary HP per verse (cap 30); successful "
                                     "Enchantment/Illusion Bard spells can daze room enemies");

  /* Endless Refrain */
  perk = &perk_list[PERK_BARD_ENDLESS_REFRAIN];
  perk->id = PERK_BARD_ENDLESS_REFRAIN;
  perk->name = strdup("Endless Refrain");
  perk->description = strdup("On each eleven-second verse while performing, recover one expended "
                             "Bard spell slot");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SPELLSINGER;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_SUSTAINING_MELODY;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1; /* spell slot regen */
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("Capstone: recover one expended Bard spell slot per eleven-second verse");

  /*** WARCHANTER TREE - TIER I ***/

  /* Battle Hymn I */
  perk = &perk_list[PERK_BARD_BATTLE_HYMN_I];
  perk->id = PERK_BARD_BATTLE_HYMN_I;
  perk->name = strdup("Battle Hymn I");
  perk->description =
      strdup("Song of Heroism grants +1 competence to damage per rank to its recipients");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_WARCHANTER;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("Song of Heroism recipients gain +1 competence damage per rank");

  /* Drummer's Rhythm I */
  perk = &perk_list[PERK_BARD_DRUMMERS_RHYTHM_I];
  perk->id = PERK_BARD_DRUMMERS_RHYTHM_I;
  perk->name = strdup("Drummer's Rhythm I");
  perk->description = strdup("While performing, you gain +1 to hit in melee per rank");
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
  perk->description = strdup("As a swift action, remove shaken from yourself and grouped allies "
                             "in the room; you and those allies gain +1 to hit, +2 to Will saves, "
                             "and +5 movement speed for 5 rounds");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_WARCHANTER;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 5;
  perk->special_description =
      strdup("Swift group rally: remove shaken; +1 hit, +2 Will, +5 movement speed for 5 rounds");

  /* Frostbite Refrain I */
  perk = &perk_list[PERK_BARD_FROSTBITE_REFRAIN_I];
  perk->id = PERK_BARD_FROSTBITE_REFRAIN_I;
  perk->name = strdup("Frostbite Refrain I");
  perk->description = strdup("While performing, your melee hits deal +1 cold damage per rank; a "
                             "natural 20 gives the target -1 to attack for 1 round");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_WARCHANTER;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = -1;
  perk->special_description = strdup("While performing: melee hits +1 cold damage per rank; nat 20 "
                                     "applies -1 attack debuff for 1 round");

  /*** WARCHANTER TREE - TIER II ***/

  /* Battle Hymn II */
  perk = &perk_list[PERK_BARD_BATTLE_HYMN_II];
  perk->id = PERK_BARD_BATTLE_HYMN_II;
  perk->name = strdup("Battle Hymn II");
  perk->description = strdup("Song of Heroism grants an additional +1 competence damage per rank "
                             "to its recipients (stacks with Battle Hymn I)");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_WARCHANTER;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_BARD_BATTLE_HYMN_I;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("Song of Heroism recipients gain another +1 competence damage per rank");

  /* Drummer's Rhythm II */
  perk = &perk_list[PERK_BARD_DRUMMERS_RHYTHM_II];
  perk->id = PERK_BARD_DRUMMERS_RHYTHM_II;
  perk->name = strdup("Drummer's Rhythm II");
  perk->description = strdup("While performing, gain an additional +1 melee to-hit per rank "
                             "(stacks with Drummer's Rhythm I)");
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
  perk->description = strdup("While performing, make an extra melee attack at your highest bonus "
                             "on your first turn in combat; on hit, you and grouped allies in the "
                             "room gain +1d4 damage for 2 rounds");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_WARCHANTER;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_RALLYING_CRY;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4;
  perk->effect_modifier = 2;
  perk->special_description = strdup("While performing, first combat turn: extra melee attack; on "
                                     "hit grouped room allies gain +1d4 damage for 2 rounds");

  /* Frostbite Refrain II */
  perk = &perk_list[PERK_BARD_FROSTBITE_REFRAIN_II];
  perk->id = PERK_BARD_FROSTBITE_REFRAIN_II;
  perk->name = strdup("Frostbite Refrain II");
  perk->description = strdup("While performing, melee hits deal an additional +1 cold damage per "
                             "rank; your natural 20 debuff becomes -2 to attack and -1 to AC for "
                             "1 round");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_WARCHANTER;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_BARD_FROSTBITE_REFRAIN_I;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = -2;
  perk->special_description =
      strdup("While performing: melee hits +1 cold per rank; natural 20 gives -2 attack/-1 AC");

  /*** WARCHANTER TREE - TIER III ***/

  /* Anthem of Fortitude */
  perk = &perk_list[PERK_BARD_ANTHEM_OF_FORTITUDE];
  perk->id = PERK_BARD_ANTHEM_OF_FORTITUDE;
  perk->name = strdup("Anthem of Fortitude");
  perk->description = strdup("While performing, you and grouped allies in the room gain +10% "
                             "maximum HP and +2 to Fortitude saves");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_WARCHANTER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_BATTLE_HYMN_II;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 10;
  perk->special_description = strdup("Active grouped aura: +10% maximum HP and +2 Fortitude saves");

  /* Commanding Cadence */
  perk = &perk_list[PERK_BARD_COMMANDING_CADENCE];
  perk->id = PERK_BARD_COMMANDING_CADENCE;
  perk->name = strdup("Commanding Cadence");
  perk->description = strdup("While performing, enemies you hit in melee must make a Will save or "
                             "be dazed for 1 round (once per target per 5 rounds)");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_WARCHANTER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_WARBEAT;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 5;
  perk->special_description = strdup("While performing, melee hit: target must make a Will save or "
                                     "be dazed 1 round; 5-round per-target recovery");

  /* Steel Serenade */
  perk = &perk_list[PERK_BARD_STEEL_SERENADE];
  perk->id = PERK_BARD_STEEL_SERENADE;
  perk->name = strdup("Steel Serenade");
  perk->description =
      strdup("While performing, you gain +2 natural AC and 10% physical damage resistance");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_WARCHANTER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_DRUMMERS_RHYTHM_II;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 10;
  perk->special_description =
      strdup("While performing: +2 natural AC and 10% physical damage resistance");

  /* Banner Verse */
  perk = &perk_list[PERK_BARD_BANNER_VERSE];
  perk->id = PERK_BARD_BANNER_VERSE;
  perk->name = strdup("Banner Verse");
  perk->description = strdup("While performing, you and grouped allies in the room gain +2 to hit "
                             "and +2 to all saves");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_WARCHANTER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_RALLYING_CRY;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("Active performance aura: grouped room allies gain +2 to hit and +2 to all saves");

  /* Warchanter's Dominance - Tier 4 Capstone */
  perk = &perk_list[PERK_BARD_WARCHANTERS_DOMINANCE];
  perk->id = PERK_BARD_WARCHANTERS_DOMINANCE;
  perk->name = strdup("Warchanter's Dominance");
  perk->description = strdup("Song of Heroism now also grants +1 attack and +1 AC; your Warbeat "
                             "gives its recipients an additional +1d4 damage and +1 AC");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_WARCHANTER;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_ANTHEM_OF_FORTITUDE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 1;
  perk->special_description = strdup("Capstone: Song of Heroism grants +1 attack/+1 AC; Warbeat "
                                     "grants another +1d4 damage/+1 AC");

  /* Winter's War March - Tier 4 Capstone */
  perk = &perk_list[PERK_BARD_WINTERS_WAR_MARCH];
  perk->id = PERK_BARD_WINTERS_WAR_MARCH;
  perk->name = strdup("Winter's War March");
  perk->description = strdup("On each eleven-second verse while performing, valid enemies in the "
                             "room take 4d6 cold damage and are slowed for 3 rounds; a successful "
                             "Fortitude save halves damage and reduces the slow to 1 round");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_WARCHANTER;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_COMMANDING_CADENCE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4;
  perk->effect_modifier = 3;
  perk->special_description = strdup("Each verse: room enemies take 4d6 cold and slow 3 rounds; "
                                     "Fortitude save halves damage and slow duration becomes 1");

  /*** SWASHBUCKLER TREE - TIER I ***/

  /* Fencer's Footwork I */
  perk = &perk_list[PERK_BARD_FENCERS_FOOTWORK_I];
  perk->id = PERK_BARD_FENCERS_FOOTWORK_I;
  perk->name = strdup("Fencer's Footwork I");
  perk->description = strdup("+1 Dodge AC and +1 Reflex save per rank while wielding a finesse "
                             "weapon or single one-handed weapon");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SWASHBUCKLER;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 1;
  perk->special_description = strdup(
      "While wielding finesse or single one-handed weapon: +1 Dodge AC and +1 Reflex per rank");

  /* Precise Strike I */
  perk = &perk_list[PERK_BARD_PRECISE_STRIKE_I];
  perk->id = PERK_BARD_PRECISE_STRIKE_I;
  perk->name = strdup("Precise Strike I");
  perk->description = strdup("+1 precision damage per rank with finesse or one-handed "
                             "piercing/slashing weapons (not multiplied on crits)");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SWASHBUCKLER;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description = strdup("With finesse or one-handed piercing/slashing: +1 precision "
                                     "damage per rank (not on crit multiplier)");

  /* Riposte Training I */
  perk = &perk_list[PERK_BARD_RIPOSTE_TRAINING_I];
  perk->id = PERK_BARD_RIPOSTE_TRAINING_I;
  perk->name = strdup("Riposte Training I");
  perk->description = strdup("3% chance per rank to make an immediate counterattack after you "
                             "successfully dodge or parry");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SWASHBUCKLER;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3;
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("After dodging/parrying: 3% chance per rank to make immediate counterattack");

  /* Flourish */
  perk = &perk_list[PERK_BARD_FLOURISH];
  perk->id = PERK_BARD_FLOURISH;
  perk->name = strdup("Flourish");
  perk->description = strdup("Activate for +2 to hit and +2 AC for 2 rounds; ends if you are "
                             "knocked prone or grappled. Requires and uses a move action");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SWASHBUCKLER;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 2;
  perk->special_description =
      strdup("Activate: +2 to hit and +2 AC for 2 rounds (ends if knocked prone/grappled)");

  /*** SWASHBUCKLER TREE - TIER II ***/

  /* Fencer's Footwork II */
  perk = &perk_list[PERK_BARD_FENCERS_FOOTWORK_II];
  perk->id = PERK_BARD_FENCERS_FOOTWORK_II;
  perk->name = strdup("Fencer's Footwork II");
  perk->description =
      strdup("Additional +1 Dodge AC and +1 Reflex per rank while using a finesse/single weapon");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SWASHBUCKLER;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_BARD_FENCERS_FOOTWORK_I;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 1;
  perk->special_description = strdup("While wielding finesse or single one-handed weapon: +1 Dodge "
                                     "AC and +1 Reflex per rank (stacks with Tier I)");

  /* Precise Strike II */
  perk = &perk_list[PERK_BARD_PRECISE_STRIKE_II];
  perk->id = PERK_BARD_PRECISE_STRIKE_II;
  perk->name = strdup("Precise Strike II");
  perk->description =
      strdup("Additional +1 precision damage per rank (stacks with Precise Strike I)");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SWASHBUCKLER;
  perk->cost = 2;
  perk->max_rank = 2;
  perk->prerequisite_perk = PERK_BARD_PRECISE_STRIKE_I;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("With finesse or one-handed piercing/slashing: +1 precision damage per rank (stacks "
             "with Tier I, not on crit multiplier)");

  /* Duelist's Poise */
  perk = &perk_list[PERK_BARD_DUELISTS_POISE];
  perk->id = PERK_BARD_DUELISTS_POISE;
  perk->name = strdup("Duelist's Poise");
  perk->description = strdup(
      "Gain +2 to critical confirmation and +1 critical threat range when using a finesse weapon");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SWASHBUCKLER;
  perk->cost = 2;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_FLOURISH;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 1;
  perk->special_description =
      strdup("With finesse weapon: +2 to critical confirmation rolls and +1 critical threat range");

  /* Agile Disengage */
  perk = &perk_list[PERK_BARD_AGILE_DISENGAGE];
  perk->id = PERK_BARD_AGILE_DISENGAGE;
  perk->name = strdup("Agile Disengage");
  perk->description = strdup("On a failed flee attempt, you gain +4 AC for 3 rounds. This bonus "
                             "ends if you move out of the room you're in");
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
  perk->description = strdup("If you avoid all melee hits for a full round, your next attack gains "
                             "+4 to hit and +2d6 precision damage");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SWASHBUCKLER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_DUELISTS_POISE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 4;
  perk->effect_modifier = 12;
  perk->special_description =
      strdup("Avoided all hits this round: +4 to hit and +2d6 precision damage on next attack");

  /* Showstopper */
  perk = &perk_list[PERK_BARD_SHOWSTOPPER];
  perk->id = PERK_BARD_SHOWSTOPPER;
  perk->name = strdup("Showstopper");
  perk->description = strdup("On a confirmed crit, impose -2 to enemy AC and -2 to attack rolls "
                             "for 2 rounds (once per target per 5 rounds)");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SWASHBUCKLER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_PRECISE_STRIKE_II;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 2;
  perk->special_description =
      strdup("Critical hit: enemy takes -2 AC and -2 to hit for 2 rounds (once per target/5 min)");

  /* Acrobatic Charge */
  perk = &perk_list[PERK_BARD_ACROBATIC_CHARGE];
  perk->id = PERK_BARD_ACROBATIC_CHARGE;
  perk->name = strdup("Acrobatic Charge");
  perk->description = strdup(
      "You can charge through difficult terrain and around allies; you gain +2 to hit on charges");
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
  perk->description = strdup("After successfully feinting, your next attack deals +2d6 precision "
                             "damage and gains +2 to confirm criticals");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SWASHBUCKLER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_RIPOSTE_TRAINING_I;
  perk->prerequisite_rank = 2;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 12;
  perk->effect_modifier = 2;
  perk->special_description =
      strdup("After successful feint: +2d6 precision damage and +2 crit confirm on next attack");

  /* Swashbuckler's Supreme Style - TIER 4 CAPSTONE */
  perk = &perk_list[PERK_BARD_SUPREME_STYLE];
  perk->id = PERK_BARD_SUPREME_STYLE;
  perk->name = strdup("Swashbuckler's Supreme Style");
  perk->description =
      strdup("While wielding a finesse or single one-handed weapon, gain +2 to hit, +2 dodge AC, "
             "+2 to crit confirmation, and one additional attack per 3 rounds");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SWASHBUCKLER;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_PERFECT_TEMPO;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 2;
  perk->effect_modifier = 2;
  perk->special_description = strdup("Tier 4 Capstone: +2 to hit, +2 AC, +2 crit confirm, +1 "
                                     "attack per 3 rounds (finesse/1H weapon only)");

  /* Curtain Call - TIER 4 CAPSTONE */
  perk = &perk_list[PERK_BARD_CURTAIN_CALL];
  perk->id = PERK_BARD_CURTAIN_CALL;
  perk->name = strdup("Curtain Call");
  perk->description = strdup("Once per 5 minutes, make a free attack against up to 3 adjacent "
                             "enemies, each dealing +2d6 precision damage; creatures struck must "
                             "save or be disoriented for 2 rounds (disadvantage on attacks)");
  perk->associated_class = CLASS_BARD;
  perk->perk_category = PERK_CATEGORY_SWASHBUCKLER;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_BARD_SHOWSTOPPER;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 12;
  perk->effect_modifier = 2;
  perk->special_description = strdup("Tier 4 Capstone: Free multi-target attack 1/5min, +2d6 "
                                     "precision damage, targets save vs disoriented");
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
  perk->special_description =
      strdup("Applies +1 per rank to STR/DEX/CON only while under your mutagen.");

  /* Hardy Constitution I */
  perk = &perk_list[PERK_ALCHEMIST_HARDY_CONSTITUTION_I];
  perk->id = PERK_ALCHEMIST_HARDY_CONSTITUTION_I;
  perk->name = strdup("Hardy Constitution I");
  perk->description =
      strdup("When you drink your own mutagen, gain +1 max HP per level per rank while it lasts.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_MUTAGENIST;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 1;
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("Bonus max HP scales with level and persists only during mutagen.");

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
  perk->special_description =
      strdup("Adds +1 per rank to STR/DEX/CON while under mutagen; stacks with Mutagen I.");

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
  perk->description =
      strdup("On drinking your mutagen, heal 1d6 + level; also gain fast healing 1 for 10 rounds.");
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
  perk->special_description =
      strdup("Use a toggle to enable/disable; boosts mutagen effects by 50%.");

  /* Universal Mutagen */
  perk = &perk_list[PERK_ALCHEMIST_UNIVERSAL_MUTAGEN];
  perk->id = PERK_ALCHEMIST_UNIVERSAL_MUTAGEN;
  perk->name = strdup("Universal Mutagen");
  perk->description =
      strdup("Activate: next mutagen applies highest bonus to all abilities (short duration).");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_MUTAGENIST;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_IMPROVED_MUTAGEN;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("Next mutagen: highest ability bonus applies to all; lasts up to ~5 minutes.");

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
  perk->description = strdup("Mutagens perfected: +4 to chosen ability and +2 to all others; "
                             "immune to Unstable Mutagen backlash.");
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
  perk->special_description =
      strdup("Requires Mutagenic Mastery and Improved Mutagen. Grants immunity to Unstable Mutagen "
             "backlash; ensures +4 primary and +2 others while mutagen lasts.");

  /* Chimeric Transmutation (Capstone) */
  perk = &perk_list[PERK_ALCHEMIST_CHIMERIC_TRANSMUTATION];
  perk->id = PERK_ALCHEMIST_CHIMERIC_TRANSMUTATION;
  perk->name = strdup("Chimeric Transmutation");
  perk->description = strdup("While under mutagen, unleash a swift-action breath weapon once per "
                             "combat: 3d6 fire, 3d6 poison, and 3d6 cold.");
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
  perk->special_description =
      strdup("Requires Universal Mutagen and Unstable Mutagen. Grants a once-per-combat swift "
             "breath attack while mutagen is active.");

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
  perk->description =
      strdup("Splash damage from bombs increases by +3 and bomb save DCs increase by +2 per rank.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_BOMB_CRAFTSMAN;
  perk->cost = 1;
  perk->max_rank = 3;
  perk->prerequisite_perk = -1;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3;
  perk->effect_modifier = 2;
  perk->special_description =
      strdup("Adds +3 splash damage and +2 save DC per rank to bomb effects.");

  /* Quick Bomb */
  perk = &perk_list[PERK_ALCHEMIST_QUICK_BOMB];
  perk->id = PERK_ALCHEMIST_QUICK_BOMB;
  perk->name = strdup("Quick Bomb");
  perk->description =
      strdup("10% chance to throw a bomb as a swift action instead of the usual action.");
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
  perk->effect_value = 10;   /* resistance bypass */
  perk->effect_modifier = 6; /* extra damage die size marker */
  perk->special_description =
      strdup("Fire/cold/acid/electric bombs ignore 10 resistance and add 1d6 damage.");

  /* Concussive Bomb */
  perk = &perk_list[PERK_ALCHEMIST_CONCUSSIVE_BOMB];
  perk->id = PERK_ALCHEMIST_CONCUSSIVE_BOMB;
  perk->name = strdup("Concussive Bomb");
  perk->description =
      strdup("10% chance on hit to knock targets prone; cannot be reversed on you.");
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
  perk->effect_value = 10;   /* proc chance */
  perk->effect_modifier = 2; /* 2d6 bonus damage */
  perk->special_description = strdup("10% chance for bombs to deal an additional 2d6 fire damage.");

  /* Cluster Bomb */
  perk = &perk_list[PERK_ALCHEMIST_CLUSTER_BOMB];
  perk->id = PERK_ALCHEMIST_CLUSTER_BOMB;
  perk->name = strdup("Cluster Bomb");
  perk->description = strdup(
      "Your bombs have a 10% chance to become cluster bombs, hitting 3 times at 75% damage each.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_BOMB_CRAFTSMAN;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_ALCHEMICAL_BOMB_II;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10;    /* proc chance */
  perk->effect_modifier = 75; /* damage multiplier per hit */
  perk->special_description =
      strdup("10% chance for bombs to fragment: 3 hits at 75% damage each.");

  /* Calculated Throw */
  perk = &perk_list[PERK_ALCHEMIST_CALCULATED_THROW];
  perk->id = PERK_ALCHEMIST_CALCULATED_THROW;
  perk->name = strdup("Calculated Throw");
  perk->description =
      strdup("Your bombs are extra precise, making DCs to resist their effects +3 higher.");
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
  perk->effect_value = 2;    /* 2d6 bonus damage */
  perk->effect_modifier = 6; /* die size */
  perk->special_description = strdup("All bombs deal an additional 2d6 damage.");

  /*** BOMB CRAFTSMAN TREE - TIER IV (CAPSTONES) ***/

  /* Bombardier Savant (Capstone) */
  perk = &perk_list[PERK_ALCHEMIST_BOMBARDIER_SAVANT];
  perk->id = PERK_ALCHEMIST_BOMBARDIER_SAVANT;
  perk->name = strdup("Bombardier Savant");
  perk->description = strdup("Expert bomb thrower. Bombs gain +3 to hit and +6d6 damage. Throw 2 "
                             "bombs when starting combat.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_BOMB_CRAFTSMAN;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_BOMB_MASTERY;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 3;    /* +3 to hit */
  perk->effect_modifier = 6; /* +6d6 damage */
  perk->special_description =
      strdup("Capstone: +3 ranged touch attack, +6d6 damage, dual bomb throw at combat start.");

  /* Volatile Catalyst (Capstone) */
  perk = &perk_list[PERK_ALCHEMIST_VOLATILE_CATALYST];
  perk->id = PERK_ALCHEMIST_VOLATILE_CATALYST;
  perk->name = strdup("Volatile Catalyst");
  perk->description = strdup("Bombs trigger chain reactions. 1% per bomb prepared to throw an "
                             "additional bomb. Toggleable.");
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
  perk->special_description = strdup("Capstone: Bombs have 1% chance per bomb prepared to trigger "
                                     "an auto-throw. Requires toggle.");

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
  perk->special_description =
      strdup("Extract bottling: 3% per rank chance extracts don't consume.");

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
  perk->description = strdup("Extracts you create gain the resonant property. They have a 5% "
                             "chance to affect all members in your party.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_EXTRACT_MASTER;
  perk->cost = 1;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_UNDEFINED;
  perk->prerequisite_rank = 0;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5; /* 5% chance */
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("Party synergy: Extracts have 5% chance to affect all party members.");

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
  perk->special_description =
      strdup("Further bottling refinement: +3% per rank non-consumption chance.");

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
  perk->special_description =
      strdup("Power concentration: 20% chance to apply Empower to extracts on use.");

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
  perk->special_description =
      strdup("Duration extension: 20% chance to apply Extend to extracts on use.");

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
  perk->description =
      strdup("Extracts automatically apply to other alchemists in your party when used.");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_EXTRACT_MASTER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_CONCENTRATED_ESSENCE;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 0;
  perk->effect_modifier = 0;
  perk->special_description =
      strdup("Combination effects: Extracts auto-apply to alchemist allies in party.");

  /* Discovery Extraction */
  perk = &perk_list[PERK_ALCHEMIST_DISCOVERY_EXTRACTION];
  perk->id = PERK_ALCHEMIST_DISCOVERY_EXTRACTION;
  perk->name = strdup("Discovery Extraction");
  perk->description = strdup(
      "10% chance for extracts to grant +1 INT (stacks to +10, 2 min duration, resets on proc).");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_EXTRACT_MASTER;
  perk->cost = 3;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_INFUSION_II;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10;   /* 10% chance */
  perk->effect_modifier = 1; /* +1 INT per stack */
  perk->special_description =
      strdup("Unique effect: 10% proc for stacking INT buff (+1 per proc, max +10, 2 min).");

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
  perk->special_description =
      strdup("Crafting excellence: 10% chance for Maximize on extracts and bombs.");

  /*** EXTRACT MASTER TREE - TIER IV (CAPSTONES) ***/

  /* Eternal Extract (Capstone) */
  perk = &perk_list[PERK_ALCHEMIST_ETERNAL_EXTRACT];
  perk->id = PERK_ALCHEMIST_ETERNAL_EXTRACT;
  perk->name = strdup("Eternal Extract");
  perk->description = strdup(
      "Extracts have a 5% chance to last 1 hour (unless the duration would be higher anyway).");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_EXTRACT_MASTER;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_MASTER_ALCHEMIST;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 5;       /* 5% chance */
  perk->effect_modifier = 3600; /* 1 hour in seconds */
  perk->special_description = strdup("Ultimate alchemy: Extracts have 5% chance to last 1 hour.");

  /* Quintessential Extraction (Capstone) */
  perk = &perk_list[PERK_ALCHEMIST_QUINTESSENTIAL_EXTRACTION];
  perk->id = PERK_ALCHEMIST_QUINTESSENTIAL_EXTRACTION;
  perk->name = strdup("Quintessential Extraction");
  perk->description = strdup(
      "Using extracts heals 10 HP and increases max HP by 10 for 5 min (stacks to +100 max HP).");
  perk->associated_class = CLASS_ALCHEMIST;
  perk->perk_category = PERK_CATEGORY_EXTRACT_MASTER;
  perk->cost = 5;
  perk->max_rank = 1;
  perk->prerequisite_perk = PERK_ALCHEMIST_DISCOVERY_EXTRACTION;
  perk->prerequisite_rank = 1;
  perk->effect_type = PERK_EFFECT_SPECIAL;
  perk->effect_value = 10;    /* 10 HP heal */
  perk->effect_modifier = 10; /* +10 max HP per stack */
  perk->special_description =
      strdup("Perfect mastery: Extracts heal 10 HP and grant +10 max HP (5 min, stacks to +100).");
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
  perk->description = strdup(
      "+2 damage on all unarmed attacks, attacks can cause bleeding (1d4 per round for 3 rounds)");
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
  perk->description =
      strdup("With a quarterstaff or kama, gain +1 AC, +1 to hit and +1 to damage when using them");
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
  perk->description =
      strdup("Flurry of Blows penalty reduced by 1, 10% chance for extra flurry attack per round");
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
  perk->description = strdup(
      "Use a ki point to make a crushing blow attack dealing +4d6 damage and ignoring 10 DR");
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
  perk->description =
      strdup("Sneak attacks have 5% chance to stun for 1 round (Fortitude save negates)");
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
  perk->description =
      strdup("+15 feet movement, can use ki point to cast waterwalk and spider climb");
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
  perk->description =
      strdup("+2d6 sneak attack damage, critical hits with sneak attacks deal +3d6 extra damage");
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
  perk->description =
      strdup("20% concealment miss chance at all times, 50% in dim light or darkness");
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
  perk->description = strdup("Gain Hide in Plain Sight ability, +4d6 sneak attack damage, "
                             "vanishing technique casts greater invisibility");
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
  perk->description =
      strdup("Use ki point to make attack that ignores all DR and deals +8d6 force damage");
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
  perk->description =
      strdup("+1 to saves vs. elemental damage and 1 DR vs. elemental damage per rank");
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
  perk->description =
      strdup("Spend a ki point to gain flaming attacks of +1d6 fire damage on unarmed strikes");
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
  perk->description = strdup(
      "Spend 1 ki point to emit thunder wave dealing 4d6 sound damage and deafening the target");
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
  perk->description = strdup("Spend 1 ki point to unleash (2d6+2)xRank force damage AoE, knocking "
                             "enemies prone (Reflex negates)");
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
  perk->description = strdup(
      "Spend 2 ki points to cast wall of water (blocks ranged attacks, slows movement through it)");
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
  perk->description = strdup(
      "Spend 2 ki points to create fire wave in 15-ft cone (3d6 fire damage, ignites flammables)");
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
  perk->description = strdup("Spend 2 ki points to cast gust of wind (knocks flying creatures "
                             "down, pushes medium creatures back)");
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
  perk->description = strdup("Spend 1 ki point to strike an enemy dealing 2d6 cold damage. Failed "
                             "reflex save (DC 10 + monk level/2 + WIS) encases them in ice for 2 "
                             "rounds (paralyzed, immune to cold, DR 5/-). 1 minute cooldown.");
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
  perk->description = strdup(
      "Increase all elemental resistances by +5, can change element attunement as free action");
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
  perk->description = strdup("Spend 1 ki point to become gaseous form (fly speed, immune to "
                             "nonmagical attacks, can pass through small openings)");
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
  perk->description = strdup("Spend 1 ki point to summon spectral ice rabbits for a ranged attack "
                             "(3d6 cold damage, can target adjacent rooms)");
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
  perk->description = strdup(
      "Spend 1 ki point to create water wave AoE (2d6 water damage, extinguishes fire effects)");
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
  perk->description = strdup("Spend 2 ki points to unleash an inferno (AoE 8d6 fire damage, Reflex "
                             "save for half or be set on fire for 2 rounds)");
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
  perk->description = strdup("Use a ki point with the rollingearth command to create an earthquake "
                             "(30-ft radius, difficult terrain, knocks prone, 4d6 damage)");
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
  perk->description = strdup("Spend 1 ki point to gain 5/- DR that absorbs up to 100 HP (doesn't "
                             "stack with stoneskin/ironskin)");
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
  perk->description = strdup("Spend 3 ki points to unleash 4d6 AoE sound damage, then strike a "
                             "random enemy with 3d10 lightning each round for 3 rounds");
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
  perk->description = strdup(
      "Spend 4 ki points to create wall of fire (20-ft line, 5d8 fire damage, burns for 3 rounds)");
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
  perk->description = strdup("Spend 5 ki points to cast cone of cold (60-ft cone, 12d6 cold "
                             "damage, freezes enemies solid for 1 round on failed save)");
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
  perk->description =
      strdup("Spend 5 ki points to transform into elemental form for 1 minute (choose "
             "fire/water/air/earth: gain elemental traits, immunities, and special attacks)");
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
  perk->description = strdup("All elemental ki abilities cost -2 ki (minimum 1), deal +2d6 damage, "
                             "immunity to all elemental damage types");
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
  perk->special_description = strdup("Increase your daily uses of Lay on Hands by +1 per rank. "
                                     "Maximum 3 ranks for +3 uses total.");

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
  perk->special_description = strdup("Divine protection grants +1 Deflection bonus to AC per rank. "
                                     "Maximum 3 ranks for +3 AC total.");

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
  perk->special_description = strdup("While wielding a shield, gain +1 bonus to all saving throws "
                                     "per rank. Maximum 3 ranks for +3 to saves total.");

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
  perk->effect_value = 2;    /* +2 AC */
  perk->effect_modifier = 5; /* 5 rounds duration */
  perk->special_description = strdup("Gain 'Defensive Strike' ability - Make an attack that, if it "
                                     "hits, grants you +2 AC for 5 rounds. 2 minute cooldown.");

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
  perk->special_description =
      strdup("Requires Extra Lay on Hands I (max). Further increase daily Lay on Hands uses by +1 "
             "per rank. Maximum 2 ranks for +2 uses (combined +5 with Tier 1).");

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
  perk->special_description =
      strdup("Requires Shield of Faith I (max). Further enhance divine protection with +1 "
             "Deflection AC per rank. Maximum 2 ranks for +2 AC (combined +5 with Tier 1).");

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
  perk->special_description =
      strdup("Requires Extra Lay on Hands I (at least 2 ranks). Your Lay on Hands ability heals "
             "+10% more per rank. Maximum 3 ranks for +30% healing total.");

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
  perk->special_description =
      strdup("Requires Bulwark of Defense (at least 2 ranks). While wielding a shield, grouped "
             "allies in your room gain +2 AC bonus from your protective presence.");

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
  perk->special_description =
      strdup("Requires Shield of Faith II (at least 2 ranks). Allies within your Aura of Courage "
             "radius gain +2 to all saving throws.");

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
  perk->special_description = strdup("Requires Shield Guardian. You become a beacon of divine "
                                     "safety, reducing all incoming damage by 10%.");

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
  perk->effect_value = 20;   /* +20 current and max HP */
  perk->effect_modifier = 5; /* 5 rounds duration */
  perk->special_description =
      strdup("Requires Healing Hands (at least 3 ranks). Your Lay on Hands also grants +20 to "
             "current and maximum hit points for 5 rounds. Does not stack.");

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
  perk->effect_value = 20;   /* 20 temp HP */
  perk->effect_modifier = 4; /* +4 AC */
  perk->special_description =
      strdup("Requires Bulwark of Defense (3 ranks) and Shield Guardian. Gain 'Bastion' ability - "
             "Swift action: gain 20 temporary HP, +4 AC, and immunity to knockdown for 5 rounds. 5 "
             "minute cooldown.");

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
  perk->effect_value = 2;    /* 2 HP per round in combat */
  perk->effect_modifier = 5; /* 5 HP per round out of combat */
  perk->special_description =
      strdup("Requires Aura of Protection. Your divine presence sustains allies within your aura, "
             "regenerating 2 HP per round in combat, 5 HP per round out of combat.");

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
  perk->special_description = strdup("Requires Merciful Touch. Your Lay on Hands can remove one "
                                     "negative magical affect and can be used as a swift action.");

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
  perk->special_description =
      strdup("Requires Bastion of Defense. When an ally within 30 feet would be reduced below 0 "
             "HP, you may take the damage instead. Once per 10 minutes.");

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
  perk->special_description =
      strdup("Turn Undead affects +2 HD worth of undead per rank (+6 at max).");

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
  perk->effect_value = 1;     /* +1 to all saving throws per rank */
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
  perk->effect_value = 1;    /* 1d4 divine damage */
  perk->effect_modifier = 4; /* d4 die */
  perk->special_description =
      strdup("Gain 'Radiant Aura' ability - Toggle: emit light, undead within 10 feet take 1d4 "
             "divine damage per round, costs 1 move per round active.");

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
  perk->special_description = strdup(
      "Requires Spell Focus I (3 ranks). Additional +1 to spell save DCs per rank (+2 at max).");

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
  perk->effect_value = 3;    /* +3 HD worth per rank */
  perk->effect_modifier = 2; /* +2d6 damage per rank */
  perk->special_description =
      strdup("Requires Turn Undead Mastery I (3 ranks). Turn Undead affects additional +3 HD worth "
             "per rank and deals +2d6 damage per rank.");

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
  perk->special_description =
      strdup("Requires Spell Focus I (at least 2 ranks). Once per day, you may cast Bless, Shield "
             "of Faith, or Protection from Evil as a swift action.");

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
  perk->effect_value = 1;    /* 1d6 per rank */
  perk->effect_modifier = 6; /* d6 die */
  perk->special_description =
      strdup("Gain 'Channel Energy' ability - Standard action: heal allies or damage undead in 30 "
             "foot radius for 1d6 per rank, 2 uses per day.");

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
  perk->special_description = strdup("Requires Spell Focus II (at least 2 ranks). +4 bonus to "
                                     "overcome spell resistance with paladin spells.");

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
  perk->special_description =
      strdup("Requires Turn Undead Mastery II (at least 2 ranks). When Turn Undead affects undead "
             "with less than half your paladin level in HD, they are destroyed instead of turned.");

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
  perk->effect_value = 2;    /* +2d6 per rank */
  perk->effect_modifier = 2; /* +2 uses per day per rank */
  perk->special_description =
      strdup("Requires Channel Energy I (3 ranks). Channel Energy heals/damages +2d6 per rank and "
             "gains +2 uses per day per rank.");

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
  perk->special_description =
      strdup("Requires Divine Grace (3 ranks). Your Aura of Courage grants immunity to fear and "
             "charm effects, and grants +4 bonus vs mind-affecting.");

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
  perk->effect_value = 3;    /* 3d8 */
  perk->effect_modifier = 8; /* d8 die */
  perk->special_description = strdup(
      "Requires Channel Energy II (at least 2 ranks). Gain 'Mass Cure Wounds' ability - Standard "
      "action: heal all allies within 30 feet for 3d8 + CHA modifier, twice per day.");

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
  perk->effect_value = 4;    /* +4 caster level */
  perk->effect_modifier = 2; /* +2 DC */
  perk->special_description = strdup(
      "Requires Spell Penetration and Destroy Undead. When you destroy undead with Turn Undead, "
      "your next spell cast within 1 round is cast at +4 caster level and has +2 DC.");

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
  perk->special_description =
      strdup("Requires Aura of Courage Mastery. Gain 'Beacon of Hope' ability - Standard action: "
             "all allies within 30 feet gain advantage on saves, immunity to fear, and maximize "
             "healing received for 5 rounds, once per day.");
}
