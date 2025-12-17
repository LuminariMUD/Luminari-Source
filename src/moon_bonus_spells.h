/**************************************************************************
 *  File: moon_bonus_spells.h                      Part of LuminariMUD *
 *  Usage: Moon-based bonus spell slot system declarations                *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

#ifndef MOON_BONUS_SPELLS_H
#define MOON_BONUS_SPELLS_H

/* Function declarations for moon bonus spell system */

/**
 * Initialize moon bonus spells for a character based on their alignment
 * and the current moon phases
 * @param ch The character to initialize
 */
void init_moon_bonus_spells(struct char_data *ch);

/**
 * Update the moon bonus spells based on current moon phases
 * Called when moon phases change
 * @param ch The character to update
 */
void update_moon_bonus_spells(struct char_data *ch);

/**
 * Get the current moon bonus spells available for a character
 * This is the maximum allowed based on their alignment and moon phase
 * @param ch The character
 * @return The maximum number of moon bonus spells
 */
int get_max_moon_bonus_spells(struct char_data *ch);

/**
 * Check if a character has available moon bonus spells
 * @param ch The character
 * @return TRUE if they have at least one available, FALSE otherwise
 */
bool has_moon_bonus_spells(struct char_data *ch);

/**
 * Consume a moon bonus spell for a character
 * @param ch The character
 * @return TRUE if a bonus spell was successfully used, FALSE if none available
 */
bool use_moon_bonus_spell(struct char_data *ch);

/**
 * Regenerate one moon bonus spell if the timer has elapsed
 * Should be called once per tick/heartbeat
 * @param ch The character
 */
void regenerate_moon_bonus_spell(struct char_data *ch);

/**
 * Reset moon bonus spells when a character levels up or resets
 * @param ch The character
 */
void reset_moon_bonus_spells(struct char_data *ch);

/**
 * Get the number of moon bonus spells currently available (not yet used)
 * @param ch The character
 * @return The number of unused moon bonus spells
 */
int get_available_moon_bonus_spells(struct char_data *ch);

/**
 * Get the number of moon bonus spells currently in use
 * @param ch The character
 * @return The number of used moon bonus spells
 */
int get_used_moon_bonus_spells(struct char_data *ch);

/**
 * Check if a character is an arcane caster
 * @param ch The character
 * @return TRUE if they are an arcane caster, FALSE otherwise
 */
bool is_arcane_caster(struct char_data *ch);

/**
 * @brief Displays information about the moons to a character.
 * 
 * This function provides the character with details about the current state
 * of the moons, potentially used for determining moon-based bonus spells or
 * other lunar-related game mechanics.
 * 
 * @param ch Pointer to the character data structure who is looking at the moons.
 *           Must not be NULL.
 * 
 * @return void This function does not return a value.
 */
void look_at_moons(struct char_data *ch);

#endif /* MOON_BONUS_SPELLS_H */
