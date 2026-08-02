/**************************************************************************
 *  File: moon_bonus_spells.c                      Part of LuminariMUD *
 *  Usage: Moon-based bonus spell slot system implementation               *
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
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "moon_bonus_spells.h"

/* Externals */
extern struct weather_data weather_info;

/* Define for moon bonus spell regeneration - one spell per 5 minutes */
#define MOON_BONUS_REGEN_TICKS (5 * 60 * 10) /* 5 minutes in ticks (assuming 10 ticks per second) */

/**
 * Initialize moon bonus spells for a character based on their alignment
 * and the current moon phases
 */
void init_moon_bonus_spells(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return;

  /* Check if arcane moon phases are enabled in config */
  if (!CONFIG_ARCANE_MOON_PHASES)
  {
    ch->player_specials->saved.moon_bonus_spells = 0;
    ch->player_specials->saved.moon_bonus_spells_used = 0;
    ch->player_specials->saved.moon_bonus_regen_timer = 0;
    return;
  }

  if (!ch->player_specials || !ch->player_specials)
    return;

  /* Only arcane casters get moon bonus spells */
  if (!is_arcane_caster(ch))
  {
    ch->player_specials->saved.moon_bonus_spells = 0;
    ch->player_specials->saved.moon_bonus_spells_used = 0;
    ch->player_specials->saved.moon_bonus_regen_timer = 0;
    return;
  }

  /* Initialize based on alignment and moon phase */
  update_moon_bonus_spells(ch);

  /* Reset usage and timer */
  ch->player_specials->saved.moon_bonus_spells_used = 0;
  ch->player_specials->saved.moon_bonus_regen_timer = MOON_BONUS_REGEN_TICKS;
}

/**
 * Update the moon bonus spells based on current moon phases
 * Called when moon phases change
 */
void update_moon_bonus_spells(struct char_data *ch)
{
  int max_bonus_spells = 0;
  int current_used = 0;
  int alignment_bonus = 0;

  if (!ch || IS_NPC(ch))
    return;

  /* Check if arcane moon phases are enabled in config */
  if (!CONFIG_ARCANE_MOON_PHASES)
  {
    ch->player_specials->saved.moon_bonus_spells = 0;
    ch->player_specials->saved.moon_bonus_spells_used = 0;
    return;
  }

  if (!ch->player_specials || !ch->player_specials)
    return;

  /* Only arcane casters get moon bonus spells */
  if (!is_arcane_caster(ch))
  {
    ch->player_specials->saved.moon_bonus_spells = 0;
    return;
  }

  /* Store current usage to apply to new maximum if needed */
  current_used = ch->player_specials->saved.moon_bonus_spells_used;

  /* Determine alignment and get corresponding bonus spells */
  if (IS_GOOD(ch))
    alignment_bonus = weather_info.moons.solinari_sp;
  else if (IS_NEUTRAL(ch))
    alignment_bonus = weather_info.moons.lunitari_sp;
  else
    alignment_bonus = weather_info.moons.nuitari_sp;

  max_bonus_spells = alignment_bonus;

  /* Update the maximum */
  ch->player_specials->saved.moon_bonus_spells = max_bonus_spells;

  /* If the new maximum is lower than what's currently used, cap the usage */
  if (current_used > max_bonus_spells)
  {
    ch->player_specials->saved.moon_bonus_spells_used = max_bonus_spells;
  }
}

/**
 * Get the current moon bonus spells available for a character
 * This is the maximum allowed based on their alignment and moon phase
 */
int get_max_moon_bonus_spells(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;

  if (!ch->player_specials || !ch->player_specials)
    return 0;

  if (!is_arcane_caster(ch))
    return 0;

  return ch->player_specials->saved.moon_bonus_spells;
}

/**
 * Get the number of moon bonus spells currently available (not yet used)
 */
int get_available_moon_bonus_spells(struct char_data *ch)
{
  int max_spells;
  int used_spells;

  if (!ch || IS_NPC(ch))
    return 0;

  if (!ch->player_specials || !ch->player_specials)
    return 0;

  max_spells = ch->player_specials->saved.moon_bonus_spells;
  used_spells = ch->player_specials->saved.moon_bonus_spells_used;

  return MAX(0, max_spells - used_spells);
}

/**
 * Get the number of moon bonus spells currently in use
 */
int get_used_moon_bonus_spells(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return 0;

  if (!ch->player_specials || !ch->player_specials)
    return 0;

  return ch->player_specials->saved.moon_bonus_spells_used;
}

/**
 * Check if a character has available moon bonus spells
 */
bool has_moon_bonus_spells(struct char_data *ch)
{
  if (!CONFIG_ARCANE_MOON_PHASES)
    return FALSE;

  return get_available_moon_bonus_spells(ch) > 0;
}

/**
 * Consume a moon bonus spell for a character
 */
bool use_moon_bonus_spell(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;

  if (!ch->player_specials || !ch->player_specials)
    return FALSE;

  if (!has_moon_bonus_spells(ch))
    return FALSE;

  /* Increment the used counter */
  ch->player_specials->saved.moon_bonus_spells_used++;

  return TRUE;
}

/**
 * Regenerate one moon bonus spell if the timer has elapsed
 * Should be called once per tick/heartbeat
 */
void regenerate_moon_bonus_spell(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return;

  if (!ch->player_specials || !ch->player_specials)
    return;

  if (!is_arcane_caster(ch))
    return;

  /* Check if we're at max already */
  if (ch->player_specials->saved.moon_bonus_spells_used <= 0)
    return;

  /* Decrement the timer */
  ch->player_specials->saved.moon_bonus_regen_timer--;

  /* Check if it's time to regenerate */
  if (ch->player_specials->saved.moon_bonus_regen_timer <= 0)
  {
    /* Regenerate one spell slot */
    ch->player_specials->saved.moon_bonus_spells_used--;

    /* Reset the timer */
    ch->player_specials->saved.moon_bonus_regen_timer = MOON_BONUS_REGEN_TICKS;
  }
}

/**
 * Reset moon bonus spells when a character levels up or resets
 */
void reset_moon_bonus_spells(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return;

  if (!ch->player_specials || !ch->player_specials)
    return;

  /* Reset usage and timer when leveling */
  ch->player_specials->saved.moon_bonus_spells_used = 0;
  ch->player_specials->saved.moon_bonus_regen_timer = MOON_BONUS_REGEN_TICKS;
}

/**
 * Helper function to check if a character is an arcane caster
 * Includes: Wizard, Sorcerer, Bard
 */
bool is_arcane_caster(struct char_data *ch)
{
  if (!ch)
    return FALSE;

  if (ARCANE_LEVEL(ch) > 0)
    return TRUE;

  return FALSE;
}
