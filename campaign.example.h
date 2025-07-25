/**
 * @file campaign.example.h
 * @brief Example campaign-specific configuration for LuminariMUD
 * 
 * This file contains example campaign-specific settings and customizations.
 * It is excluded from version control as it contains local configuration.
 * 
 * IMPORTANT: Copy this file to campaign.h and remove the .example extension.
 * Then uncomment the appropriate campaign define for your MUD.
 * 
 * Available campaigns:
 * - CAMPAIGN_DL: DragonLance (Chronicles of Krynn)
 * - CAMPAIGN_FR: Forgotten Realms (Faerun)
 * - Default: LuminariMUD (if no campaign is defined)
 */

#ifndef _CAMPAIGN_H_
#define _CAMPAIGN_H_

/* ************************************************************************
 * Campaign Selection
 * 
 * Define ONE of the following campaigns, OR leave ALL commented out to
 * use the default LuminariMUD campaign settings.
 * ************************************************************************ */

/**
 * CAMPAIGN_DL - DragonLance Campaign (Chronicles of Krynn)
 * 
 * Enables DragonLance-specific features including:
 * - Krynn-specific races (Kender, etc.)
 * - DragonLance classes and prestige classes
 * - Krynn deities and domains
 * - DragonLance-specific areas and content
 */
/* #define CAMPAIGN_DL */

/**
 * CAMPAIGN_FR - Forgotten Realms Campaign (Faerun)
 * 
 * Enables Forgotten Realms-specific features including:
 * - Faerun-specific races
 * - Forgotten Realms deities and domains
 * - FR-specific prestige classes
 * - Faerun areas and content
 */
/* #define CAMPAIGN_FR */

/* ************************************************************************
 * Campaign-Specific Settings
 * 
 * Add any additional campaign-specific configuration below.
 * These might include:
 * - Custom race/class restrictions
 * - Modified game mechanics
 * - Campaign-specific constants
 * - Feature toggles
 * ************************************************************************ */

/* Add your campaign-specific defines here */

#endif /* _CAMPAIGN_H_ */