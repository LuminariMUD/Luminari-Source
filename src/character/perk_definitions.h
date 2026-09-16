/**
 * @file perk_definitions.h
 * Internal contract between the perk engine and the perk definition tables.
 *
 * The perk system is split into two translation units with different
 * change profiles:
 *
 *   perks.c             - the engine: purchase rules, rank lookups, stage
 *                         advancement, the perk command, and every gameplay
 *                         accessor.
 *   perk_definitions.c  - the tables: one function per class tree that fills
 *                         perk_list[] with that tree's content.
 *
 * This header is the seam between them and is not part of the perk system's
 * public API. Only perks.c and perk_definitions.c include it; gameplay code
 * includes perks.h instead.
 *
 * Ownership and lifetime
 * ----------------------
 * The definition functions populate the single global perk_list[] array,
 * which is owned by perks.c. They do not allocate, resize, or free the array
 * itself; they write fields into slots the engine has already reset.
 *
 * Each function stores strdup() copies in the name, description, and
 * special_description fields. perk_list[] owns those copies from that point
 * on, and destroy_perks() releases them. A definition function must therefore
 * never store a string literal or a pointer to caller memory in those fields:
 * destroy_perks() would pass it to free(). The only exception is the shared
 * "undefined" sentinel storage that init_perks() installs, which
 * destroy_perks() recognises by address and skips.
 *
 * A definition function must not free the pointer already in a slot. Slots
 * arrive either holding a sentinel (which must not be freed) or holding the
 * previous boot's freed storage, so init_perks() is responsible for clearing
 * the array before any definition function runs.
 *
 * Call order and reentrancy
 * -------------------------
 * init_perks() owns the order in which these run, and it is the only
 * supported caller. Every function is boot-time only and writes shared global
 * state with no locking, so none of them is thread-safe and none may run
 * while the game loop is live. Calling one twice without an intervening
 * destroy_perks() leaks the previous strdup() copies.
 *
 * Nullability and errors
 * ----------------------
 * These functions take no arguments, return nothing, and report no errors.
 * A strdup() failure follows the same out-of-memory policy as the rest of
 * boot. Writing outside perk_list[] is a programming error: every function
 * indexes it with a PERK_* constant, and NUM_PERKS bounds the array.
 */

#ifndef PERK_DEFINITIONS_H
#define PERK_DEFINITIONS_H

#include "perks.h"

/* The perk table itself. perks.c defines it and owns its lifetime; the
 * definition functions below only fill in slots. It is declared here rather
 * than in perks.h so gameplay code keeps reaching perks through the accessor
 * API instead of indexing the array directly. */
extern struct perk_data perk_list[NUM_PERKS];

void define_fighter_perks(void);
void define_wizard_perks(void);
void define_wizard_controller_perks(void);
void define_wizard_versatile_caster_perks(void);
void define_cleric_perks(void);
void define_rogue_perks(void);
void define_ranger_perks(void);
void define_bard_perks(void);
void define_barbarian_perks(void);
void define_monk_perks(void);
void define_paladin_perks(void);
void define_alchemist_perks(void);
void define_psionicist_perks(void);
void define_blackguard_perks(void);
void define_inquisitor_perks(void);

#endif /* PERK_DEFINITIONS_H */
