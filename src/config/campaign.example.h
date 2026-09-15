/**
 * @file campaign.example.h
 * @brief Local Luminari configuration example
 *
 * This file contains local settings and customizations. The copied campaign.h
 * file is excluded from version control because installations may customize it.
 *
 * IMPORTANT: Copy this file to campaign.h and remove the .example extension.
 */

#ifndef _CAMPAIGN_H_
#define _CAMPAIGN_H_

/* ************************************************************************
 * Local Settings
 *
 * Add any additional installation-specific configuration below.
 * These might include:
 * - Custom race/class restrictions
 * - Modified game mechanics
 * - Installation-specific constants
 * - Feature toggles
 * ************************************************************************ */

/* Add your installation-specific defines here. */

/* ************************************************************************
 * Phase 4.75: Wilderness Materials Integration
 *
 * Luminari wilderness material system configuration
 * ************************************************************************ */

#define ENABLE_WILDERNESS_MATERIALS 1
#define WILDERNESS_MATERIAL_THEME "luminari"
#define ENABLE_WILDERNESS_CRAFTING_INTEGRATION 1
#define ENABLE_ENHANCED_WILDERNESS_RECIPES 1
#define PRESERVE_WILDERNESS_MATERIAL_HIERARCHY 1
#define WILDERNESS_RESOURCE_DEPLETION_SYSTEM 1
#define ENABLE_DYNAMIC_RESOURCE_DESCRIPTIONS 1
#define RESOURCE_DESCRIPTION_DETAIL_LEVEL 3
#define ECOLOGICAL_NARRATIVE_DEPTH 2

#endif /* _CAMPAIGN_H_ */
