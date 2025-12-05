/**************************************************************************
 *  File: mob_spellslots.c                           Part of LuminariMUD *
 *  Usage: Mob spell slot tracking system implementation.                  *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "spells.h"
#include "handler.h"
#include "comm.h"
#include "db.h"
#include "constants.h"
#include "spell_prep.h"
#include "class.h"

/* External functions */
extern int compute_slots_by_circle(struct char_data *ch, int class_num, int circle);

/**
 * Calculate the spell circle for a given spell and class.
 * This is a simplified version based on the formula in spell_prep.c
 * 
 * @param spellnum The spell number
 * @param char_class The class of the caster
 * @return The spell circle (0-9), or -1 if invalid
 */
int get_spell_circle(int spellnum, int char_class)
{
  int spell_circle;
  
  if (spellnum < 0 || spellnum > TOP_SPELL_DEFINE)
    return -1;
    
  if (char_class < 0 || char_class >= NUM_CLASSES)
    return -1;
  
  /* Check if this class can cast this spell at all */
  if (spell_info[spellnum].min_level[char_class] >= LVL_IMMORT)
    return -1;
  
  /* Calculate circle based on class type */
  switch (char_class) {
    case CLASS_WIZARD:
    case CLASS_CLERIC:
    case CLASS_DRUID:
    case CLASS_SORCERER:
      /* Full casters: (level + 1) / 2 */
      spell_circle = (spell_info[spellnum].min_level[char_class] + 1) / 2;
      break;
      
    
    case CLASS_INQUISITOR:
    case CLASS_BARD:
    case CLASS_ALCHEMIST:
    case CLASS_SUMMONER:
      if (spell_info[spellnum].min_level[char_class] <= 1)
        spell_circle = 1;
      if (spell_info[spellnum].min_level[char_class] <= 4)
        spell_circle = 2;
      if (spell_info[spellnum].min_level[char_class] <= 7)
        spell_circle = 3;
      if (spell_info[spellnum].min_level[char_class] <= 10)
        spell_circle = 4;
      if (spell_info[spellnum].min_level[char_class] <= 13)
        spell_circle = 5;
      else
        spell_circle = 6;
      break;
      
    case CLASS_PALADIN:
    case CLASS_RANGER:
    case CLASS_BLACKGUARD:
      if (spell_info[spellnum].min_level[char_class] <= 6)
        spell_circle = 1;
      if (spell_info[spellnum].min_level[char_class] <= 10)
        spell_circle = 2;
      if (spell_info[spellnum].min_level[char_class] <= 12)
        spell_circle = 3;
      else
        spell_circle = 4;
      break;
      
    default:
      /* Use generic formula */
      spell_circle = (spell_info[spellnum].min_level[char_class] + 1) / 2;
      break;
  }
  
  /* Clamp to valid range */
  if (spell_circle < 0)
    spell_circle = 0;
  if (spell_circle > 9)
    spell_circle = 9;
    
  return spell_circle;
}

/**
 * Initialize spell slots for a mob based on its class and level.
 * Called when mob is loaded or reset.
 * 
 * @param ch The mob to initialize spell slots for
 */
void init_mob_spell_slots(struct char_data *ch)
{
  int circle, char_class, mob_level, max_slots;
  int stat_bonus = 0;
  
  if (!ch || !IS_NPC(ch))
    return;
    
  /* Skip initialization if mob has unlimited spell slots */
  if (MOB_FLAGGED(ch, MOB_UNLIMITED_SPELL_SLOTS))
  {
    /* Zero out slots for mobs with unlimited casting */
    for (circle = 0; circle < 10; circle++)
    {
      ch->mob_specials.spell_slots[circle] = 0;
      ch->mob_specials.max_spell_slots[circle] = 0;
    }
    ch->mob_specials.last_slot_regen = 0;
    return;
  }
  
  /* Determine mob's primary casting class */
  char_class = GET_CLASS(ch);
  
  /* If no class or non-caster class, try to infer from spells known */
  if (char_class < 0 || char_class >= NUM_CLASSES)
  {
    /* Default to wizard for generic spellcasters */
    char_class = CLASS_WIZARD;
  }
  
  /* Get mob level (use GET_LEVEL for NPCs, not CLASS_LEVEL) */
  mob_level = GET_LEVEL(ch);
  if (mob_level > LVL_IMMORT - 1)
    mob_level = LVL_IMMORT - 1;
  
  /* Calculate maximum spell slots per circle based on class and level */
  /* NOTE: We manually calculate for mobs because compute_slots_by_circle uses CLASS_LEVEL
   * which is for PC multiclassing and doesn't work properly for NPCs */
  for (circle = 0; circle < 10; circle++)
  {
    max_slots = 0;
    stat_bonus = 0;
    
    /* Get base slots from class table */
    switch (char_class)
    {
      case CLASS_WIZARD:
        stat_bonus = spell_bonus[GET_INT(ch)][circle];
        max_slots = wizard_slots[mob_level][circle];
        break;
      case CLASS_SORCERER:
        stat_bonus = spell_bonus[GET_CHA(ch)][circle];
        max_slots = sorcerer_known[mob_level][circle];
        break;
      case CLASS_CLERIC:
        stat_bonus = spell_bonus[GET_WIS(ch)][circle];
        max_slots = cleric_slots[mob_level][circle];
        break;
      case CLASS_DRUID:
        stat_bonus = spell_bonus[GET_WIS(ch)][circle];
        max_slots = druid_slots[mob_level][circle];
        break;
      case CLASS_BARD:
        stat_bonus = spell_bonus[GET_CHA(ch)][circle];
        max_slots = bard_slots[mob_level][circle];
        break;
      case CLASS_RANGER:
        stat_bonus = spell_bonus[GET_WIS(ch)][circle];
        max_slots = ranger_slots[mob_level][circle];
        break;
      case CLASS_PALADIN:
      case CLASS_BLACKGUARD:
        stat_bonus = spell_bonus[GET_CHA(ch)][circle];
        max_slots = paladin_slots[mob_level][circle];
        break;
      case CLASS_ALCHEMIST:
        stat_bonus = spell_bonus[GET_INT(ch)][circle];
        max_slots = alchemist_slots[mob_level][circle];
        break;
      case CLASS_SUMMONER:
        stat_bonus = spell_bonus[GET_CHA(ch)][circle];
        max_slots = summoner_slots[mob_level][circle];
        break;
      case CLASS_INQUISITOR:
        stat_bonus = spell_bonus[GET_WIS(ch)][circle];
        max_slots = inquisitor_slots[mob_level][circle];
        break;
      default:
        /* Non-caster class */
        max_slots = 0;
        break;
    }
    
    /* Add stat bonus to base slots */
    max_slots += stat_bonus;
    
    /* Ensure non-negative */
    if (max_slots < 0)
      max_slots = 0;
      
    /* Circle 0 (cantrips) should be unlimited or very high */
    if (circle == 0 && max_slots > 0)
      max_slots = 100; /* Effectively unlimited cantrips */
    
    ch->mob_specials.max_spell_slots[circle] = max_slots;
    ch->mob_specials.spell_slots[circle] = max_slots; /* Start fully rested */
  }
  
  /* Initialize regeneration timestamp */
  ch->mob_specials.last_slot_regen = time(0);
}

/**
 * Check if mob has an available spell slot for the given spell.
 * 
 * @param ch The mob to check
 * @param spellnum The spell number to check
 * @return TRUE if slot available, FALSE otherwise
 */
bool has_spell_slot(struct char_data *ch, int spellnum)
{
  int circle, char_class;
  
  if (!ch || !IS_NPC(ch))
    return TRUE; /* PCs handle their own slots */
    
  /* Mobs with unlimited spell slots flag have unlimited casting */
  if (MOB_FLAGGED(ch, MOB_UNLIMITED_SPELL_SLOTS))
    return TRUE;
  
  /* Determine mob's primary casting class */
  char_class = GET_CLASS(ch);
  if (char_class < 0 || char_class >= NUM_CLASSES)
    char_class = CLASS_WIZARD;
  
  /* Get the spell circle */
  circle = get_spell_circle(spellnum, char_class);
  if (circle < 0 || circle > 9)
    return FALSE; /* Invalid spell/circle */
  
  /* Check if slot available */
  return (ch->mob_specials.spell_slots[circle] > 0);
}

/**
 * Consume a spell slot when mob casts a spell.
 * 
 * @param ch The mob casting the spell
 * @param spellnum The spell being cast
 */
void consume_spell_slot(struct char_data *ch, int spellnum)
{
  int circle, char_class;
  
  if (!ch || !IS_NPC(ch))
    return; /* PCs handle their own slots */
    
  /* Mobs with unlimited spell slots flag don't consume slots */
  if (MOB_FLAGGED(ch, MOB_UNLIMITED_SPELL_SLOTS))
    return;
  
  /* Determine mob's primary casting class */
  char_class = GET_CLASS(ch);
  if (char_class < 0 || char_class >= NUM_CLASSES)
    char_class = CLASS_WIZARD;
  
  /* Get the spell circle */
  circle = get_spell_circle(spellnum, char_class);
  if (circle < 0 || circle > 9)
    return; /* Invalid spell/circle */
  
  /* Consume the slot */
  if (ch->mob_specials.spell_slots[circle] > 0)
  {
    ch->mob_specials.spell_slots[circle]--;
    
    /* Optional: Log slot consumption for debugging - DISABLED (too spammy)
    if (ch->mob_specials.spell_slots[circle] == 0)
    {
      log("MOB_SPELLSLOT: %s depleted circle %d slots",
          GET_NAME(ch), circle);
    }
    */
  }
}

/**
 * Check if mob has sufficient spell slots for out-of-combat buffing.
 * Returns TRUE if mob has more than 50% of max slots for the given spell.
 * This prevents mobs from wasting slots on buffs when they're running low.
 * 
 * @param ch The mob to check
 * @param spellnum The spell to check
 * @return TRUE if mob has > 50% slots remaining, FALSE otherwise
 */
bool has_sufficient_slots_for_buff(struct char_data *ch, int spellnum)
{
  int circle, char_class;
  int current_slots, max_slots;
  
  if (!ch || !IS_NPC(ch))
    return TRUE; /* PCs handle their own slot management */
    
  /* Mobs with unlimited spell slots always have sufficient slots */
  if (MOB_FLAGGED(ch, MOB_UNLIMITED_SPELL_SLOTS))
    return TRUE;
  
  /* Determine mob's primary casting class */
  char_class = GET_CLASS(ch);
  if (char_class < 0 || char_class >= NUM_CLASSES)
    char_class = CLASS_WIZARD;
  
  /* Get the spell circle */
  circle = get_spell_circle(spellnum, char_class);
  if (circle < 0 || circle > 9)
    return FALSE; /* Invalid spell/circle */
  
  /* Get current and max slots for this circle */
  current_slots = ch->mob_specials.spell_slots[circle];
  max_slots = ch->mob_specials.max_spell_slots[circle];
  
  /* If max slots is 0 or negative, mob can't cast this circle */
  if (max_slots <= 0)
    return FALSE;
  
  /* Cantrips (circle 0) are always OK since they're effectively unlimited */
  if (circle == 0)
    return TRUE;
  
  /* If max slots is 1, allow buffing if we have at least 1 slot */
  if (max_slots == 1)
    return (current_slots >= 1);
  
  /* Check if we have more than 50% of max slots remaining */
  /* Use integer arithmetic: current * 2 > max means current > max/2 */
  return (current_slots * 2 > max_slots);
}

/**
 * Regenerate one random spell slot for a mob.
 * Called periodically from mobile activity loop.
 * Only regenerates when mob is not in combat.
 * 
 * @param ch The mob to regenerate a slot for
 */
void regenerate_mob_spell_slot(struct char_data *ch)
{
  time_t current_time;
  int circle, depleted_count = 0;
  int depleted_circles[10];
  int chosen_circle;
  
  if (!ch || !IS_NPC(ch))
    return;
    
  /* Skip regeneration for mobs with unlimited spell slots */
  if (MOB_FLAGGED(ch, MOB_UNLIMITED_SPELL_SLOTS))
    return;
  
  /* Don't regenerate while in combat */
  if (FIGHTING(ch))
    return;
  
  /* Check if enough time has elapsed (300 seconds = 5 minutes) */
  current_time = time(0);
  if (current_time - ch->mob_specials.last_slot_regen < 300)
    return;
  
  /* Build list of circles with less than maximum slots */
  for (circle = 0; circle < 10; circle++)
  {
    if (ch->mob_specials.spell_slots[circle] < ch->mob_specials.max_spell_slots[circle])
    {
      depleted_circles[depleted_count] = circle;
      depleted_count++;
    }
  }
  
  /* If no depleted circles, nothing to regenerate */
  if (depleted_count == 0)
    return;
  
  /* Randomly select one depleted circle */
  chosen_circle = depleted_circles[rand_number(0, depleted_count - 1)];
  
  /* Restore one slot to that circle */
  ch->mob_specials.spell_slots[chosen_circle]++;
  
  /* Update regeneration timestamp */
  ch->mob_specials.last_slot_regen = current_time;
  
  /* Optional: Log regeneration for debugging - DISABLED (too spammy)
  log("MOB_SPELLSLOT: %s regenerated circle %d slot (%d/%d)",
      GET_NAME(ch), chosen_circle,
      ch->mob_specials.spell_slots[chosen_circle],
      ch->mob_specials.max_spell_slots[chosen_circle]);
  */
}

/**
 * Display spell slot information for a mob (for admin commands).
 * 
 * @param ch The character viewing the information
 * @param mob The mob to display slot information for
 */
void show_mob_spell_slots(struct char_data *ch, struct char_data *mob)
{
  int circle;
  
  if (!mob || !IS_NPC(mob))
  {
    send_to_char(ch, "That's not a mob.\r\n");
    return;
  }
  
  if (MOB_FLAGGED(mob, MOB_UNLIMITED_SPELL_SLOTS))
  {
    send_to_char(ch, "%s has unlimited spell slots (spell slot system disabled).\r\n",
                 GET_NAME(mob));
    return;
  }
  
  send_to_char(ch, "\tCSpell Slots for %s:\tn\r\n", GET_NAME(mob));
  send_to_char(ch, "Circle   Current / Maximum\r\n");
  send_to_char(ch, "------   -----------------\r\n");
  
  for (circle = 0; circle < 10; circle++)
  {
    if (mob->mob_specials.max_spell_slots[circle] > 0)
    {
      send_to_char(ch, "  %d      %3d / %3d\r\n", circle,
                   mob->mob_specials.spell_slots[circle],
                   mob->mob_specials.max_spell_slots[circle]);
    }
  }
  
  send_to_char(ch, "\r\nLast regeneration: %ld seconds ago\r\n",
               (long)(time(0) - mob->mob_specials.last_slot_regen));
}
