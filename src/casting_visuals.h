/**
 * @file casting_visuals.h
 * @brief Casting visual effects system header.
 *
 * This file provides the foundation for school-specific, class-specific,
 * and metamagic-enhanced casting visual messages. It centralizes all
 * casting visual infrastructure to keep spell_parser.c cleaner and
 * facilitate future customization.
 *
 * Part of the LuminariMUD Casting Visuals Improvement project.
 *
 * @see casting_visuals.c for implementation
 * @see docs/project-management-zusuk/casting-visuals/ for design docs
 */

#ifndef CASTING_VISUALS_H
#define CASTING_VISUALS_H

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * Target type constants for visual message lookup.
 * These define the context in which a spell is being cast,
 * affecting which message variant to display.
 */
#define CAST_TARGET_SELF      0  /* Caster targeting themselves */
#define CAST_TARGET_CHAR      1  /* Caster targeting another character */
#define CAST_TARGET_OBJ       2  /* Caster targeting an object */
#define CAST_TARGET_NONE      3  /* Caster with no specific target */
#define NUM_CAST_TARGET_TYPES 4

/*
 * Message type constants for visual phases.
 * These define whether the message is for spell start or completion.
 */
#define CAST_MSG_START    0
#define CAST_MSG_COMPLETE 1
#define NUM_CAST_MSG_TYPES 2

/*
 * Progress stage constants for escalating descriptions.
 * Most spells take 1-5 rounds, so we define 5 progress stages.
 */
#define CAST_PROGRESS_STAGES 5

/**
 * Gets the spell school index for array lookup.
 *
 * This function retrieves the school of magic for a given spell number
 * and validates it for safe array access. Returns NOSCHOOL (0) for
 * invalid spell numbers or spells without a defined school.
 *
 * @param spellnum The spell number to look up.
 * @return The school index (0-8), safe for array indexing.
 */
int get_spell_school_index(int spellnum);

/**
 * Gets the school name for a given school index.
 *
 * Returns a human-readable school name suitable for debug output
 * and logging. This wraps the existing school_names_specific array
 * with bounds checking.
 *
 * @param school The school index (0-8).
 * @return Pointer to static string with school name.
 */
const char *get_casting_school_name(int school);

/**
 * Gets the start message for a spell based on school and target type.
 *
 * Returns an appropriate message to display when a caster begins casting
 * a spell. The message varies based on the spell's school of magic and
 * what type of target is being affected.
 *
 * @param school The school index from get_spell_school_index().
 * @param target_type One of CAST_TARGET_* constants.
 * @return Pointer to static message string, or NULL if not available.
 *
 * @note Currently returns NULL as placeholder. Will be populated in Phase 2.
 */
const char *get_school_start_msg(int school, int target_type);

/**
 * Gets the completion message for a spell based on school and target type.
 *
 * Returns an appropriate message to display when a caster completes casting
 * a spell. The message varies based on the spell's school of magic and
 * what type of target is being affected.
 *
 * @param school The school index from get_spell_school_index().
 * @param target_type One of CAST_TARGET_* constants.
 * @return Pointer to static message string, or NULL if not available.
 *
 * @note Currently returns NULL as placeholder. Will be populated in Phase 2.
 */
const char *get_school_complete_msg(int school, int target_type);

/**
 * Determines the target type constant from spell parameters.
 *
 * Given the casting context (target character, target object), determines
 * the appropriate CAST_TARGET_* constant for message lookup.
 *
 * @param ch The caster.
 * @param tch Target character (may be NULL).
 * @param tobj Target object (may be NULL).
 * @return One of CAST_TARGET_* constants.
 */
int determine_cast_target_type(struct char_data *ch, struct char_data *tch,
                               struct obj_data *tobj);

/**
 * Initializes the casting visuals system.
 *
 * Called during mud boot to set up any runtime data structures
 * needed by the casting visuals system. Currently a no-op but
 * reserved for future expansion.
 */
void init_casting_visuals(void);

/*
 * ==========================================================================
 * Phase 5: Dynamic Escalating Progress Descriptions
 * ==========================================================================
 */

/**
 * Calculates the progress stage based on elapsed casting time.
 *
 * The stage represents how far along the casting has progressed,
 * used to select appropriately escalating narrative messages.
 *
 * @param time_max The original total casting time.
 * @param time_remaining The remaining casting time.
 * @return Progress stage (0 to CAST_PROGRESS_STAGES-1).
 */
int calculate_casting_progress_stage(int time_max, int time_remaining);

/**
 * Gets the caster progress message for a spell based on school and stage.
 *
 * Returns a first-person message to display to the caster during casting
 * based on how far along the casting is.
 *
 * @param school The school index from get_spell_school_index().
 * @param stage The progress stage (0 to CAST_PROGRESS_STAGES-1).
 * @return Pointer to static message string, or NULL if not available.
 */
const char *get_school_progress_msg(int school, int stage);

/**
 * Gets the observer progress message for a spell based on school and stage.
 *
 * Returns a third-person message to display to observers in the room
 * during casting based on how far along the casting is.
 *
 * @param school The school index from get_spell_school_index().
 * @param stage The progress stage (0 to CAST_PROGRESS_STAGES-1).
 * @return Pointer to static message string with $n format codes, or NULL.
 */
const char *get_school_progress_observer_msg(int school, int stage);

/**
 * Gets the class-specific casting style description.
 *
 * Returns a string describing how a character of a particular class
 * performs their casting gestures and style. This is used to create
 * the class-flavored portion of casting messages.
 *
 * @param class_num The class index (CLASS_WIZARD, CLASS_CLERIC, etc.).
 * @return Pointer to static string with class style, or NULL if not a caster.
 */
const char *get_class_casting_style(int class_num);

/**
 * Builds a combined class+school start message for casting.
 *
 * Combines the class-specific casting style with the school-specific
 * visual effects to create a rich, layered casting description.
 * Falls back to school-only messages for NPCs or unknown classes.
 *
 * @param ch The caster.
 * @param spellnum The spell being cast.
 * @param target_type One of CAST_TARGET_* constants.
 * @param casting_class The class used for casting (CASTING_CLASS(ch)).
 * @return Pointer to static buffer with combined message, never NULL.
 *
 * @note Returns a pointer to a static buffer. Not thread-safe.
 *       Contents may change on subsequent calls.
 */
const char *build_class_start_msg(struct char_data *ch, int spellnum,
                                  int target_type, int casting_class);

/**
 * Checks if a class has a specific casting style defined.
 *
 * Used to determine whether to use class+school combined messages
 * or fall back to school-only messages.
 *
 * @param class_num The class index.
 * @return TRUE if class has defined casting style, FALSE otherwise.
 */
int has_class_casting_style(int class_num);

/*
 * ==========================================================================
 * Phase 4: Metamagic Visual Signatures
 * ==========================================================================
 */

/**
 * Number of metamagic types with visual descriptions.
 * Matches the bit-flag metamagics defined in spells.h:
 * QUICKEN(0), MAXIMIZE(1), HEIGHTEN(2), ARCANE_ADEPT(3),
 * EMPOWER(4), SILENT(5), STILL(6), EXTEND(7)
 */
#define NUM_METAMAGIC_VISUALS 8

/**
 * Indices for metamagic visual lookup.
 * These match the bit positions in the metamagic flags.
 */
#define META_VIS_QUICKEN   0
#define META_VIS_MAXIMIZE  1
#define META_VIS_HEIGHTEN  2
#define META_VIS_ARCANE    3
#define META_VIS_EMPOWER   4
#define META_VIS_SILENT    5
#define META_VIS_STILL     6
#define META_VIS_EXTEND    7

/**
 * Gets the visual descriptor for a single metamagic type.
 *
 * Returns a descriptive prefix string for one metamagic modifier.
 * Used internally by build_metamagic_prefix().
 *
 * @param meta_index The metamagic index (0-7, matching bit position).
 * @return Pointer to static string with visual description, or NULL.
 */
const char *get_metamagic_visual(int meta_index);

/**
 * Builds a combined prefix string for all active metamagic modifiers.
 *
 * Examines the metamagic bitfield and concatenates visual descriptors
 * for each active metamagic, creating a dramatic prefix for casting
 * messages.
 *
 * Example outputs:
 * - Quickened only: "With impossible speed, "
 * - Quickened + Empowered: "With impossible speed, crackling with excess energy, "
 * - Silent + Still: "In eerie silence, perfectly motionless, "
 *
 * @param metamagic The metamagic bitfield (CASTING_METAMAGIC(ch)).
 * @return Pointer to static buffer with combined prefix, or empty string.
 *
 * @note Returns a pointer to a static buffer. Not thread-safe.
 *       Contents may change on subsequent calls.
 */
const char *build_metamagic_prefix(int metamagic);

/**
 * Builds the metamagic descriptor for event_casting() progress display.
 *
 * Creates a thematic description for the ongoing casting progress,
 * incorporating active metamagic modifiers. This replaces the simple
 * "quickened empowered" text with atmospheric descriptions.
 *
 * @param ch The caster.
 * @param spellnum The spell being cast.
 * @param metamagic The metamagic bitfield.
 * @return Pointer to static buffer with progress description.
 */
const char *build_metamagic_progress_desc(struct char_data *ch, int spellnum,
                                          int metamagic);

/**
 * Checks if any visual metamagic modifiers are active.
 *
 * Returns TRUE if any metamagic flags that have visual effects are set.
 * ARCANE_ADEPT is excluded as it's an internal modifier without visible effect.
 *
 * @param metamagic The metamagic bitfield.
 * @return TRUE if any visual metamagic is active, FALSE otherwise.
 */
int has_visual_metamagic(int metamagic);

/*
 * ==========================================================================
 * Phase 6: Environmental Reactions
 * ==========================================================================
 */

/**
 * Intensity levels for environmental reactions.
 * These determine which message set to use based on spell power.
 */
#define ENV_INTENSITY_NONE      0   /* No environmental effect */
#define ENV_INTENSITY_SUBTLE    1   /* Circles 1-3: minor ambient effects */
#define ENV_INTENSITY_MODERATE  2   /* Circles 4-6: noticeable effects */
#define ENV_INTENSITY_DRAMATIC  3   /* Circles 7-9: powerful effects */
#define NUM_ENV_INTENSITIES     4

/**
 * Number of message variants per intensity level.
 * Having multiple variants prevents repetitive messages.
 */
#define ENV_MSG_VARIANTS        3

/**
 * Base chance percentages for environmental effects per spell tick.
 * These can be tuned for balance without code changes.
 */
#define ENV_CHANCE_SUBTLE       15  /* 15% chance per tick for low spells */
#define ENV_CHANCE_MODERATE     25  /* 25% chance per tick for mid spells */
#define ENV_CHANCE_DRAMATIC     40  /* 40% chance per tick for high spells */

/**
 * Determines the intensity level of environmental reactions for a spell.
 *
 * Maps spell circle to an intensity level:
 * - Circles 1-3 (cantrips to 3rd level): Subtle
 * - Circles 4-6 (4th to 6th level): Moderate
 * - Circles 7-9 (7th to 9th level): Dramatic
 *
 * @param spell_circle The spell's circle (1-9).
 * @return One of ENV_INTENSITY_* constants.
 */
int get_env_intensity_for_circle(int spell_circle);

/**
 * Gets a generic environmental reaction message.
 *
 * Returns a random message from the generic pool for the given intensity.
 * Generic messages are school-agnostic and work for any spell.
 *
 * @param intensity One of ENV_INTENSITY_* constants (not NONE).
 * @return Pointer to static message string, or NULL if intensity is invalid.
 */
const char *get_generic_env_reaction(int intensity);

/**
 * Gets a school-specific environmental reaction message.
 *
 * Returns a random message themed to the spell's school of magic.
 * Falls back to generic messages if school has no specific effects.
 *
 * @param school The school index from get_spell_school_index().
 * @param intensity One of ENV_INTENSITY_* constants (not NONE).
 * @return Pointer to static message string, or NULL if not available.
 */
const char *get_school_env_reaction(int school, int intensity);

/**
 * Determines if an environmental reaction should trigger this tick.
 *
 * Uses probability-based triggering scaled by intensity level.
 * Higher intensity (more powerful spells) triggers more often.
 *
 * @param intensity One of ENV_INTENSITY_* constants.
 * @return TRUE if an effect should trigger, FALSE otherwise.
 */
int should_trigger_env_reaction(int intensity);

/**
 * Sends an environmental reaction message to a room.
 *
 * This is the main entry point for the environmental reaction system.
 * It determines intensity from spell circle, checks probability,
 * selects an appropriate message (school-specific or generic),
 * and sends it to everyone in the room.
 *
 * The caster is excluded from seeing the message to avoid spam.
 *
 * @param ch The caster (used to get room and exclude from message).
 * @param spell_circle The circle of the spell being cast.
 * @param school The school index from get_spell_school_index().
 * @return TRUE if a message was sent, FALSE otherwise.
 */
int send_env_reaction(struct char_data *ch, int spell_circle, int school);

#ifdef __cplusplus
}
#endif

#endif /* CASTING_VISUALS_H */
