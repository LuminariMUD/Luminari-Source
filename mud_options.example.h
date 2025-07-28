/**
 * @file mud_options.example.h
 * @brief MUD configuration options and compile-time settings
 * 
 * This file contains various MUD options that must be set in the code.
 * 
 * IMPORTANT: Make a copy of this file and name it mud_options.h, then change
 * the options to suit your MUD's needs. Please ensure you read the comment
 * associated with each option, as some configurations, if not done correctly,
 * can cause your game to crash.
 * 
 * This file controls compile-time options that affect game behavior, including:
 * - Container system configuration
 * - Newbie equipment selection
 * - Crafting system selection
 * - Available races
 */

#ifndef _MUD_OPTIONS_H_
#define _MUD_OPTIONS_H_

/* ************************************************************************
 * Container System Configuration
 * 
 * WARNING: ONE AND ONLY ONE of the following two options MUST be defined.
 * Defining both or neither will cause compilation errors.
 * ************************************************************************ */

/**
 * USE_CONTAINER_OBJECTS - Traditional container object system
 * 
 * If defined, the game will use container objects. This is NOT recommended
 * as the container system has a bug that causes contents to be deleted.
 * This bug has not been fixed yet.
 * 
 * Status: NOT RECOMMENDED - Known bugs
 */
#define USE_CONTAINER_OBJECTS

/**
 * USE_VIRTUAL_BAGS_ONLY - Virtual bag container system
 * 
 * If defined, the game will only use virtual bags for containers. This is
 * the recommended option for most MUDs. Instead of using container objects,
 * everything will be contained in virtual bags.
 * 
 * See HELP BAGS in game for more information.
 * 
 * Status: RECOMMENDED - Stable and bug-free
 */
/* #define USE_VIRTUAL_BAGS_ONLY */

/* ************************************************************************
 * Newbie Equipment Configuration
 * 
 * WARNING: ONE AND ONLY ONE of the following two options MUST be defined.
 * Defining both or neither will cause compilation errors.
 * ************************************************************************ */

/**
 * USE_NEW_NOOB_GEAR - Chronicles of Krynn newbie equipment
 * 
 * Uses the new newbie gear created in Chronicles of Krynn.
 * This gear is more balanced and fits the Krynn campaign setting.
 */
/*#define USE_NEW_NOOB_GEAR */

/**
 * USE_OLD_NOOB_GEAR - LuminariMUD legacy newbie equipment
 * 
 * Uses the old newbie gear created in LuminariMUD.
 * This gear is for backward compatibility with existing areas.
 */
#define USE_OLD_NOOB_GEAR

/* ************************************************************************
 * Crafting System Configuration
 * 
 * WARNING: ONE AND ONLY ONE of the following two options MUST be defined.
 * Defining both or neither will cause compilation errors.
 * 
 * The crafting system determines how magic items are awarded in the game.
 * ************************************************************************ */

/**
 * USE_OLD_CRAFTING_SYSTEM - Traditional magic item award system
 * 
 * This is the simpler, more stable option and is recommended for most MUDs.
 * Provides basic crafting functionality with proven stability.
 * 
 * Status: RECOMMENDED - Stable and well-tested
 */
#define USE_OLD_CRAFTING_SYSTEM

/**
 * USE_NEW_CRAFTING_SYSTEM - Advanced crafting system
 * 
 * Provides more advanced crafting features including:
 * - More complex recipes
 * - Additional crafting skills
 * - Enhanced customization options
 * 
 * Status: EXPERIMENTAL - May be less stable
 */
/* #define USE_NEW_CRAFTING_SYSTEM */

/* ************************************************************************
 * Available Races Configuration
 * 
 * Define which races are available for player selection.
 * Comment out any races you don't want available in your MUD.
 * ************************************************************************ */

/**
 * RACE_ALLOW_KENDER - Enable Kender race
 * 
 * Kender are a unique race from the Dragonlance setting, known for their
 * curiosity, fearlessness, and "borrowing" habits.
 */
#define RACE_ALLOW_KENDER

/**
 * RACE_ALLOW_DROW - Enable Drow Elf race
 * 
 * Drow are dark elves who typically dwell in the Underdark. They have
 * unique abilities including innate magic and superior darkvision.
 */
#define RACE_ALLOW_DROW

/**
 * RACE_ALLOW_VAMPIRE - Enable Vampire race
 * 
 * Vampires are undead creatures with special powers and limitations.
 * This is an advanced race that may require special handling.
 */
#define RACE_ALLOW_VAMPIRE

/**
 * WalkTo - Enabling either:
 *   Landmarks - walk across the world (non-wilderness system)
 *   City landmarks - walk within a city (wilderness system)
 * 
 * You should only have one enabled!
 */

/* for non-wilderness system uncomment this and make sure to comment out the other*/
// #define USE_WALKTO_LANDMARKS
/* Otherwise if you want to use city landmarks only, uncomment the following line and make sure to comment out above */
#define USE_CITY_LANDMARKS_ONLY

#endif /* _MUD_OPTIONS_H_ */