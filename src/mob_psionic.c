/**************************************************************************
 *  File: mob_psionic.c                               Part of LuminariMUD *
 *  Usage: Mobile psionic power functions                                 *
 *                                                                         *
 *  All rights reserved.  See license for complete information.           *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "spells.h"
#include "constants.h"
#include "psionics.h"
#include "mob_utils.h"
#include "mob_spells.h"  /* for OFFENSIVE_AOE_SPELLS define */
#include "mob_psionic.h"

/* Define for loop safety */
#define MAX_LOOPS 100

/* External data arrays */
extern int valid_aoe_spell[OFFENSIVE_AOE_SPELLS];

void npc_psionic_powerup(struct char_data *ch)
{

  int powernum = 0, i = 0;
  bool found = false;
  int buff_count = 0;

  if (!ch)
    return;

  if (MOB_FLAGGED(ch, MOB_NOCLASS))
    return;
  
  /* Safety check: Only psionicists should use psionic powers */
  if (GET_CLASS(ch) != CLASS_PSIONICIST)
  {
    return;
  }
  
  /* Check buff saturation - count existing psionic buffs */
  if (affected_by_spell(ch, PSIONIC_INERTIAL_ARMOR)) buff_count++;
  if (affected_by_spell(ch, PSIONIC_FORCE_SCREEN)) buff_count++;
  if (affected_by_spell(ch, PSIONIC_FORTIFY)) buff_count++;
  if (affected_by_spell(ch, PSIONIC_BIOFEEDBACK)) buff_count++;
  if (affected_by_spell(ch, PSIONIC_BODY_EQUILIBRIUM)) buff_count++;
  if (affected_by_spell(ch, PSIONIC_TOWER_OF_IRON_WILL)) buff_count++;
  if (affected_by_spell(ch, PSIONIC_OAK_BODY)) buff_count++;
  if (affected_by_spell(ch, PSIONIC_BODY_OF_IRON)) buff_count++;
  
  /* If already well-buffed (4+ defensive powers), reduce buff frequency */
  if (buff_count >= 4) {
    if (rand_number(0, 3)) /* 75% chance to skip buffing */
      return;
  }

  // we'll max out at 20 tries just to avoid any potential infinite loops
  while (!found && i < 20)
  {
    powernum = rand_number(PSIONIC_POWER_START, PSIONIC_POWER_END);
    i++;
    if (valid_psionic_spellup_power(powernum))
    {
      if (spell_info[powernum].min_level[CLASS_PSIONICIST] <= GET_LEVEL(ch))
        found = true;
    }
  }

  /* Use direct power manifestation instead of command parsing */
  manifest_power(ch, ch, powernum, GET_LEVEL(ch) / 2);
}

void npc_offensive_powers(struct char_data *ch)
{
  bool found = false;
  int powernum = 0;
  struct char_data *tch = NULL;
  int level, use_aoe = 0, loop_counter = 0, i = 0;

  if (!ch)
    return;

  if (MOB_FLAGGED(ch, MOB_NOCLASS))
    return;
  
  /* Safety check: Only psionicists should use psionic powers */
  if (GET_CLASS(ch) != CLASS_PSIONICIST)
  {
    return;
  }

  /* capping */
  if (GET_LEVEL(ch) >= LVL_IMMORT)
    level = LVL_IMMORT - 1;
  else
    level = GET_LEVEL(ch);

  /* 25% of spellup instead of offensive spell */
  if (!rand_number(0, 3))
  {
    npc_psionic_powerup(ch);
    return;
  }

  if (!(tch = npc_find_target(ch, &use_aoe)))
    return;
  
  /* random offensive power */
  if (use_aoe >= 2)
  {
    do
    {
      powernum = valid_aoe_spell[rand_number(0, OFFENSIVE_AOE_SPELLS - 1)];
      loop_counter++;
      if (loop_counter >= (MAX_LOOPS / 2))
        break;
    } while (level < spell_info[powernum].min_level[CLASS_PSIONICIST]);

    if (loop_counter < (MAX_LOOPS / 2) && powernum != -1)
    {
      // found a power, manifest it
      manifest_power(ch, tch, powernum, GET_LEVEL(ch) / 2);
      return;
    }
  }

  // we'll max out at 20 tries just to avoid any potential infinite loops
  while (!found && i < 20)
  {
    powernum = rand_number(PSIONIC_POWER_START, PSIONIC_POWER_END);
    i++;
    if (valid_psionic_combat_power(powernum))
    {
      if (spell_info[powernum].min_level[CLASS_PSIONICIST] <= GET_LEVEL(ch))
        found = true;
    }
  }

  /* Use direct power manifestation instead of command parsing */
  manifest_power(ch, tch, powernum, GET_LEVEL(ch) / 2);
}

bool valid_psionic_spellup_power(int powernum)
{
  switch (powernum)
  {
    case PSIONIC_FORCE_SCREEN:
    case PSIONIC_FORTIFY:
    case PSIONIC_INERTIAL_ARMOR:
    case PSIONIC_INEVITABLE_STRIKE:
    case PSIONIC_DEFENSIVE_PRECOGNITION:
    case PSIONIC_OFFENSIVE_PRECOGNITION:
    case PSIONIC_OFFENSIVE_PRESCIENCE:
    case PSIONIC_VIGOR:
    case PSIONIC_BIOFEEDBACK:
    case PSIONIC_BODY_EQUILIBRIUM:
    case PSIONIC_CONCEALING_AMORPHA:
    case PSIONIC_ELFSIGHT:
    case PSIONIC_THOUGHT_SHIELD:
    case PSIONIC_BODY_ADJUSTMENT:
    case PSIONIC_ENDORPHIN_SURGE:
    case PSIONIC_ERADICATE_INVISIBILITY:
    case PSIONIC_HEIGHTENED_VISION:
    case PSIONIC_MENTAL_BARRIER:
    case PSIONIC_SHARPENED_EDGE:
    case PSIONIC_UBIQUITUS_VISION:
    case PSIONIC_EMPATHIC_FEEDBACK:
    case PSIONIC_ENERGY_ADAPTATION:
    case PSIONIC_INTELLECT_FORTRESS:
    case PSIONIC_SLIP_THE_BONDS:
    case PSIONIC_POWER_RESISTANCE:
    case PSIONIC_TOWER_OF_IRON_WILL:
    case PSIONIC_SUSTAINED_FLIGHT:
    case PSIONIC_OAK_BODY:
    case PSIONIC_BODY_OF_IRON:
    case PSIONIC_SHADOW_BODY:
      return true;
  }
  return false;
}

bool valid_psionic_combat_power(int powernum)
{
  switch (powernum)
  {
    case PSIONIC_CRYSTAL_SHARD:
    case PSIONIC_DECELERATION:
    case PSIONIC_DEMORALIZE:
    case PSIONIC_ENERGY_RAY:
    case PSIONIC_MIND_THRUST:
    case PSIONIC_CONCUSSION_BLAST:
    case PSIONIC_ENERGY_PUSH:
    case PSIONIC_ENERGY_STUN:
    case PSIONIC_INFLICT_PAIN:
    case PSIONIC_MENTAL_DISRUPTION:
    case PSIONIC_RECALL_AGONY:
    case PSIONIC_SWARM_OF_CRYSTALS:
    case PSIONIC_BODY_ADJUSTMENT:
    case PSIONIC_CONCUSSIVE_ONSLAUGHT:
    case PSIONIC_DISPEL_PSIONICS:
    case PSIONIC_ENERGY_BURST:
    case PSIONIC_ENERGY_RETORT:    
    case PSIONIC_MIND_TRAP:
    case PSIONIC_PSIONIC_BLAST:
    case PSIONIC_DEADLY_FEAR:
    case PSIONIC_DEATH_URGE:        
    case PSIONIC_INCITE_PASSION:    
    case PSIONIC_MOMENT_OF_TERROR:
    case PSIONIC_POWER_LEECH:    
    case PSIONIC_WITHER:
    case PSIONIC_ADAPT_BODY:
    case PSIONIC_PIERCE_VEIL:    
    case PSIONIC_PSYCHIC_CRUSH:
    case PSIONIC_SHATTER_MIND_BLANK:
    case PSIONIC_SHRAPNEL_BURST:    
    case PSIONIC_UPHEAVAL:
    case PSIONIC_BREATH_OF_THE_BLACK_DRAGON:
    case PSIONIC_BRUTALIZE_WOUNDS:
    case PSIONIC_DISINTEGRATION:
    case PSIONIC_BARRED_MIND:
    case PSIONIC_COSMIC_AWARENESS:
    case PSIONIC_ENERGY_CONVERSION:
    case PSIONIC_ENERGY_WAVE:
    case PSIONIC_EVADE_BURST:
    case PSIONIC_ULTRABLAST:
    case PSIONIC_RECALL_DEATH:
    case PSIONIC_APOPSI:
    case PSIONIC_ASSIMILATE:
    case PSIONIC_TIMELESS_BODY:
    case PSIONIC_BARRED_MIND_PERSONAL:
    case PSIONIC_TRUE_METABOLISM:
      return true;
  }
  return false;
}