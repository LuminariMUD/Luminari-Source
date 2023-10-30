/*
 * File:   craft.h
 * Author: Zusuk
 *
 * Created on March 7th, 2013
 */

#ifndef CRAFT_H
#define CRAFT_H

//DEFINES//
/* number of mats needed to complete a supply order */
#define SUPPLYORDER_MATS 3
/* craft_pattern_vnums() determines pattern vnums, but you */
/* can modify the list of patterns here (i.e. molds) */
#define PATTERN_UPPER 30299
#define PATTERN_LOWER 30200
/* regardless of object weight, minimum needed mats to create an object */
#define MIN_MATS 5
/* regardless of object weight, minimum needed mats to create an object
   for those with 'elf-crafting' feat */
#define MIN_ELF_MATS 2
/* Amount of material needed: [mold weight divided by weight_factor] */
#define WEIGHT_FACTOR 50
/* Max level of a crystal in determining bonus */
#define CRYSTAL_CAP (LVL_IMMORT - 1)
/* Crystal bonus division factor, ex. level 30 = +6 bonus (factor of 5) */
#define BONUS_FACTOR 5
/* Maximum crit rolls you can get on crafting */
#define MAX_CRAFT_CRIT 3
#define AUTOCQUEST_VNUM 30084 /* set your autoquest object here */
#define AUTOCQUEST_MAKENUM 5  /* how many objects needed to craft */
/* for resizing weapons, what increment for size change in damage? */
#define WEAPON_RESIZE_INC 2
/* the maximun harvesting-nodes allowed is number of rooms divided by this value */
#define NODE_CAP_FACTOR 17
// end DEFINES //

#define CREATE_MODE_CREATE 1
#define CREATE_MODE_CHECK 2

/* item defines, by vnum */
#if defined (CAMPAIGN_DL)

#define MAGICAL_ESSENCE 40248

#define HARVESTING_NODE 20811
#define STEEL_MATERIAL 40121
#define LEATHER_MATERIAL 40122 // this matches low quality leather below
#define BRONZE_MATERIAL 40125
#define IRON_MATERIAL 40126
#define BURLAP_MATERIAL 40127
#define COTTON_MATERIAL 40128
#define HEMP_MATERIAL 40129
#define WOOL_MATERIAL 40130
#define SATIN_MATERIAL 40131
#define SILK_MATERIAL 40132
#define VELVET_MATERIAL 40133
#define WOOD_MATERIAL 40134
#define COPPER_MATERIAL 40135
#define SILVER_MATERIAL 40136
#define GOLD_MATERIAL 40137

#define ONYX_MATERIAL 40187
#define OBSIDIAN_MATERIAL 40188
#define COLD_IRON_MATERIAL 40189
#define MITHRIL_MATERIAL 40190
#define RUBY_MATERIAL 40191
#define SAPPHIRE_MATERIAL 40192
#define ADAMANTINE_MATERIAL 40193
#define PLATINUM_MATERIAL 40194
#define DIAMOND_MATERIAL 40195
#define EMERALD_MATERIAL 40196

#define ALDERWOOD_MATERIAL 40197
#define FOS_BIRD_MATERIAL 40198   // fossilized bird egg
#define FOS_WYVERN_MATERIAL 40199 // fossilized wyvern egg
#define FOS_DRAGON_MATERIAL 40200 // fossilized dragon egg
#define YEW_MATERIAL 40201
#define OAK_MATERIAL 40202
#define DARKWOOD_MATERIAL 40203
#define LEATHER_LQ_MATERIAL 40122 // low quality
#define LEATHER_MQ_MATERIAL 40205 // medium quality
#define LEATHER_HQ_MATERIAL 40206 // high quality
#define FOS_LIZARD_MATERIAL 40207 // fossilized giant lizard egg
#define ALCHEMAL_SILVER_MATERIAL 40208
#define DRAGONHIDE_MATERIAL 40209

#else

#define MAGICAL_ESSENCE 3248

#define HARVESTING_NODE 811
#define STEEL_MATERIAL 3121
#define LEATHER_MATERIAL 3122 // this matches low quality leather below
#define BRONZE_MATERIAL 3125
#define IRON_MATERIAL 3126
#define BURLAP_MATERIAL 3127
#define COTTON_MATERIAL 3128
#define HEMP_MATERIAL 3129
#define WOOL_MATERIAL 3130
#define SATIN_MATERIAL 3131
#define SILK_MATERIAL 3132
#define VELVET_MATERIAL 3133
#define WOOD_MATERIAL 3134
#define COPPER_MATERIAL 3135
#define SILVER_MATERIAL 3136
#define GOLD_MATERIAL 3137

#define ONYX_MATERIAL 3187
#define OBSIDIAN_MATERIAL 3188
#define COLD_IRON_MATERIAL 3189
#define MITHRIL_MATERIAL 3190
#define RUBY_MATERIAL 3191
#define SAPPHIRE_MATERIAL 3192
#define ADAMANTINE_MATERIAL 3193
#define PLATINUM_MATERIAL 3194
#define DIAMOND_MATERIAL 3195
#define EMERALD_MATERIAL 3196

#define ALDERWOOD_MATERIAL 3197
#define FOS_BIRD_MATERIAL 3198   // fossilized bird egg
#define FOS_WYVERN_MATERIAL 3199 // fossilized wyvern egg
#define FOS_DRAGON_MATERIAL 3200 // fossilized dragon egg
#define YEW_MATERIAL 3201
#define OAK_MATERIAL 3202
#define DARKWOOD_MATERIAL 3203
#define LEATHER_LQ_MATERIAL 3122 // low quality
#define LEATHER_MQ_MATERIAL 3205 // medium quality
#define LEATHER_HQ_MATERIAL 3206 // high quality
#define FOS_LIZARD_MATERIAL 3207 // fossilized giant lizard egg
#define ALCHEMAL_SILVER_MATERIAL 3208
#define DRAGONHIDE_MATERIAL 3209

#endif

#define IS_CLOTH(mat) (mat == MATERIAL_COTTON || mat == MATERIAL_SILK ||  \
                       mat == MATERIAL_SATIN || mat == MATERIAL_VELVET || \
                       mat == MATERIAL_WOOL || mat == MATERIAL_HEMP ||    \
                       mat == MATERIAL_BURLAP || mat == MATERIAL_DRAGONHIDE)
#define IS_LEATHER(mat) (mat == MATERIAL_LEATHER || \
                         mat == MATERIAL_DRAGONHIDE)
#define IS_WOOD(mat) (mat == MATERIAL_WOOD || mat == MATERIAL_DARKWOOD || mat == MATERIAL_DRAGONSCALE || mat == MATERIAL_DRAGONBONE)
#define IS_HARD_METAL(mat) (mat == MATERIAL_STEEL ||           \
                            mat == MATERIAL_BRONZE ||          \
                            mat == MATERIAL_IRON ||            \
                            mat == MATERIAL_ALCHEMAL_SILVER || \
                            mat == MATERIAL_COLD_IRON ||       \
                            mat == MATERIAL_MITHRIL ||         \
                            mat == MATERIAL_ADAMANTINE || \
                            mat == MATERIAL_DRAGONSCALE || \
                            mat == MATERIAL_DRAGONBONE)
#define IS_PRECIOUS_METAL(mat) (mat == MATERIAL_SILVER || \
                                mat == MATERIAL_GOLD ||   \
                                mat == MATERIAL_COPPER || \
                                mat == MATERIAL_PLATINUM)
#define SCMD_CRAFT_UNDF 0
#define SCMD_BREW 1
#define SCMD_CRAFT 2 // SCMD_CREATE
#define SCMD_FLETCH 3
#define SCMD_KNIT 4
#define SCMD_MINE 5
#define SCMD_DISENCHANT 6
#define SCMD_SYNTHESIZE 7
#define SCMD_HUNT 8
#define SCMD_FOREST 9
#define SCMD_DIVIDE 10
#define SCMD_RESIZE 11
#define SCMD_AUGMENT 12
#define SCMD_SUPPLYORDER 13
#define SCMD_CONVERT 14
#define SCMD_RESTRING 15
#define SCMD_WAND_MAKING 16
#define SCMD_STAFF_MAKING 17
/**craft_type in constants.c**/
#define NUM_CRAFT 18
/***/

/* autocraft */
#define GET_AUTOCQUEST_VNUM(ch) CHECK_PLAYER_SPECIAL((ch), \
                                                     ((ch)->player_specials->saved.autocquest_vnum))
#define GET_AUTOCQUEST_MAKENUM(ch) CHECK_PLAYER_SPECIAL((ch), \
                                                        ((ch)->player_specials->saved.autocquest_makenum))
#define GET_AUTOCQUEST_QP(ch) CHECK_PLAYER_SPECIAL((ch), \
                                                   ((ch)->player_specials->saved.autocquest_qp))
#define GET_AUTOCQUEST_EXP(ch) CHECK_PLAYER_SPECIAL((ch), \
                                                    ((ch)->player_specials->saved.autocquest_exp))
#define GET_AUTOCQUEST_GOLD(ch) CHECK_PLAYER_SPECIAL((ch), \
                                                     ((ch)->player_specials->saved.autocquest_gold))
#define GET_AUTOCQUEST_DESC(ch) CHECK_PLAYER_SPECIAL((ch), \
                                                     ((ch)->player_specials->saved.autocquest_desc))
#define GET_AUTOCQUEST_MATERIAL(ch) CHECK_PLAYER_SPECIAL((ch), \
                                                         ((ch)->player_specials->saved.autocquest_material))

#define GET_CRAFTING_TYPE(ch) ((ch)->char_specials.crafting_type)
#define GET_CRAFTING_TICKS(ch) ((ch)->char_specials.crafting_ticks)
#define GET_CRAFTING_OBJ(ch) ((ch)->char_specials.crafting_object)
#define GET_CRAFTING_REPEAT(ch) ((ch)->char_specials.crafting_repeat)
#define GET_CRAFTING_BONUS(ch) ((ch)->char_specials.crafting_bonus)

/* end item defines */

/* external functions */

void reset_harvesting_rooms(void);
extern int assembly_skills[NUM_CRAFT];
void reset_acraft(struct char_data *ch);

/* end external functions */

/* command functions */
ACMD_DECL(do_harvest);
ACMD_DECL(do_disenchant);
/* end command functions */

int get_mysql_supply_orders_available(struct char_data *ch);
void put_mysql_supply_orders_available(struct char_data *ch, int avail);

#endif /* CRAFT_H */
