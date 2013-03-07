/* 
 * File:   craft.h
 * Author: Zusuk
 *
 * Created on March 7th, 2013
 */

#ifndef CRAFT_H
#define	CRAFT_H

//DEFINES//
/* number of mats needed to complete a supply order */
#define SUPPLYORDER_MATS    3
/* craft_pattern_vnums() determines pattern vnums, but you */
/* can modify the list of patterns here (i.e. molds) */
#define PATTERN_UPPER       30299
#define PATTERN_LOWER       30200
/* regardless of object weight, minimum needed mats to create an object */
#define MIN_MATS            5
/* regardless of object weight, minimum needed mats to create an object
   for those with 'elf-crafting' feat */
#define MIN_ELF_MATS        2
/* Amount of material needed: [mold weight divided by weight_factor] */
#define WEIGHT_FACTOR       50
/* Max level of a crystal in determining bonus */
#define CRYSTAL_CAP         (LVL_IMMORT-1)
/* Crystal bonus division factor, ex. level 30 = +6 bonus (factor of 5) */
#define BONUS_FACTOR        6
/* Maximum crit rolls you can get on crafting */
#define MAX_CRAFT_CRIT      3
#define AUTOCQUEST_VNUM     30084  /* set your autoquest object here */
#define AUTOCQUEST_MAKENUM  5  /* how many objects needed to craft */
/* for resizing weapons, what increment for size change in damage? */
#define WEAPON_RESIZE_INC   2
// end DEFINES //

/* item defines, by vnum */
#define HARVESTING_NODE       811
#define STEEL_MATERIAL        3121
#define LEATHER_MATERIAL      3122
#define BRONZE_MATERIAL       3125
#define IRON_MATERIAL         3126
#define BURLAP_MATERIAL       3127
#define COTTON_MATERIAL       3128
#define HEMP_MATERIAL         3129
#define WOOL_MATERIAL         3130
#define SATIN_MATERIAL        3131
#define SILK_MATERIAL         3132
#define VELVET_MATERIAL       3133
#define WOOD_MATERIAL         3134
#define COPPER_MATERIAL       3135
#define SILVER_MATERIAL       3136
#define GOLD_MATERIAL         3137

#define ONYX_MATERIAL         3187
#define OBSIDIAN_MATERIAL     3188
#define COLD_IRON_MATERIAL    3189
#define MITHRIL_MATERIAL      3190
#define RUBY_MATERIAL         3191
#define SAPPHIRE_MATERIAL     3192
#define ADAMANTINE_MATERIAL   3193
#define PLATINUM_MATERIAL     3194
#define DIAMOND_MATERIAL      3195
#define EMERALD_MATERIAL      3196


/* end item defines */

#endif	/* CRAFT_H */

