/**
 * @file vnums.example.h
 * @brief Virtual Number (VNUM) definitions for game objects, mobs, and equipment
 *
 * This file contains all virtual number definitions used throughout the codebase.
 * It is HIGHLY recommended to record all vnums here with #defines so that you
 * can easily change them later if needed.
 *
 * Copy this file to vnums.h and customize for your MUD configuration.
 */

#ifndef _VNUMS_H_
#define _VNUMS_H_

/* ************************************************************************
 * LuminariMUD Starting Items
 * ************************************************************************ */
#define NOOB_TORCH 867
#define NOOB_RATIONS 804
#define NOOB_WATERSKIN 803
#define NOOB_BP 857
#define NOOB_CRAFTING_KIT 3118
#define NOOB_BOW 814
#define NOOB_QUIVER 816
#define NOOB_ARROW 815
#define NOOB_CRAFT_MAT 3135
#define NOOB_CRAFT_MOLD 3176
#define NOOB_WIZ_NOTE 850
#define NOOB_WIZ_SPELLBOOK 812
#define NOOB_LEATHER_SLEEVES 854
#define NOOB_LEATHER_LEGGINGS 855
#define NOOB_IRON_MACE 861
#define NOOB_IRON_SHIELD 863
#define NOOB_SCALE_MAIL 807
#define NOOB_STEEL_SCIMITAR 862
#define NOOB_WOOD_SHIELD 864
#define NOOB_STUD_LEATHER 851
#define NOOB_LONG_SWORD 808
#define NOOB_CLOTH_ROBES 809
#define NOOB_DAGGER 852
#define NOOB_CLOTH_SLEEVES 865
#define NOOB_CLOTH_PANTS 866
#define NOOB_DWARF_WARAXE 806
#define NOOB_DROW_XBOW 832
#define NOOB_DROW_BOLT 831
#define NOOB_DROW_POUCH 833
#define LYRE 825
#define FLUTE 826
#define DRUM 827
#define HORN 828
#define HARP 829
#define MANDOLIN 830

/* ************************************************************************
 * LuminariMUD Crafting Molds
 * ************************************************************************ */

/* Armor Molds */
#define PLATE_BODY 3119
#define HALFPLATE_BODY 3143
#define SPLINT_BODY 3144
#define BREASTPLATE_BODY 3145
#define CHAIN_BODY 3146
#define STUD_LEATHER_BODY 3147
#define LEATHER_BODY 3148
#define PADDED_BODY 3149
#define CLOTH_BODY 3150
#define BANDED_BODY 3241
#define SCALE_BODY 3242
#define HIDE_BODY 3243
#define LIGHT_CHAIN_BODY 3244
#define PLATE_HELM 3151
#define HALFPLATE_HELM 3152
#define SPLINT_HELM 3153
#define PIECEPLATE_HELM 3154
#define CHAIN_HELM 3155
#define STUD_LEATHER_HELM 3156
#define LEATHER_HELM 3157
#define PADDED_HELM 3158
#define CLOTH_HELM 3159
#define BANDED_HELM 3245
#define SCALE_HELM 3246
#define HIDE_HELM 3247
#define LIGHT_CHAIN_HELM 3248
#define PLATE_ARMS 3160
#define HALFPLATE_ARMS 3161
#define SPLINT_ARMS 3162
#define CHAIN_ARMS 3163
#define STUD_LEATHER_ARMS 3164
#define LEATHER_ARMS 3165
#define PADDED_ARMS 3166
#define CLOTH_ARMS 3167
#define BANDED_ARMS 3249
#define PIECEPLATE_ARMS 3250
#define SCALE_ARMS 3251
#define HIDE_ARMS 3252
#define LIGHT_CHAIN_ARMS 3253
#define PLATE_LEGS 3168
#define HALFPLATE_LEGS 3169
#define SPLINT_LEGS 3170
#define CHAIN_LEGS 3171
#define STUD_LEATHER_LEGS 3172
#define LEATHER_LEGS 3173
#define PADDED_LEGS 3174
#define CLOTH_LEGS 3175
#define BANDED_LEGS 3254
#define PIECEPLATE_LEGS 3255
#define SCALE_LEGS 3256
#define HIDE_LEGS 3257
#define LIGHT_CHAIN_LEGS 3258
#define SHIELD_MEDIUM 3180
#define SHIELD_LARGE 3181
#define SHIELD_TOWER 3182

#define NUM_ARMOR_MOLDS 37

/* Weapon Molds */
#define DAGGER 3212
#define MACE 3213
#define SICKLE 3214
#define CLUB 3215
#define MORNINGSTAR 3216
#define SPEAR 3217
#define QUARTERSTAFF 3218
#define HANDAXE 3219
#define KUKRI 3220
#define SHORTSWORD 3221
#define BATTLEAXE 3222
#define FLAIL 3223
#define LONGSWORD 3224
#define RAPIER 3225
#define SCIMITAR 3226
#define TRIDENT 3227
#define WARHAMMER 3228
#define FALCHION 3229
#define GLAIVE 3230
#define GREATAXE 3231
#define GREATCLUB 3232
#define GREATSWORD 3233
#define HALBERD 3234
#define LANCE 3235
#define SCYTHE 3236
#define KAMA 3237
#define BASTARDSWORD 3238
#define DWARVENWARAXE 3239
#define DIREFLAIL 3240

#define NUM_WEAPON_MOLDS 29

/* Prototypes */
#define WEAPON_PROTO 3299
#define ARMOR_PROTO 3298
#define AMMO_PROTO 3297
#define ITEM_PROTOTYPE 3210
#define CRYSTAL_PROTOTYPE 3211
/* unfinished */
#define INSTRUMENT_PROTO 40296

/* Miscellaneous Items */
#define RING_MOLD 3176
#define NECKLACE_MOLD 3177
#define BOOTS_MOLD 3178
#define GLOVES_MOLD 3179
#define CLOAK_MOLD 3183
#define BELT_MOLD 3184
#define WRIST_MOLD 3185
#define HELD_MOLD 3186

/* unfinished */
#define EYES_MOLD 40249
#define FACE_MOLD 40250
#define EARS_MOLD 40251
#define SHOULDERS_MOLD 40252
#define ANKLET_MOLD 40253

#define NUM_MISC_MOLDS 8

/* unfinished Kender */
#define KENDER_BAUBLE 66700
#define NOOB_HOOPAK 117

/* End of LuminariMUD crafting molds */


/**************************
 * Shared Constants
 **************************/

/* Starting Equipment Constants */
#define NUM_NOOB_ARROWS 40
#define NUM_NOOB_DROW_BOLTS 30
#define NOOB_TELEPORTER 82


// Starting Harvesting Tools
// #define NOOB_CRAFTING_TAILORING         160040
// #define NOOB_CRAFTING_ALCHEMY           160045
// #define NOOB_CRAFTING_ARMORSMITHING     160046
// #define NOOB_CRAFTING_WEAPONSMITHING    160048
// #define NOOB_CRAFTING_JEWELCRAFTING     160047
// #define NOOB_HARVESTING_MINING          160044
// #define NOOB_HARVESTING_HUNTING         160043
// #define NOOB_HARVESTING_FORESTRY        160042
// #define NOOB_HARVESTING_GATHERING       160041

/* Golem VNUMs */
/* Wood Golems - Small to Huge */
#define GOLEM_WOOD_SMALL 16500
#define GOLEM_WOOD_MEDIUM 16501
#define GOLEM_WOOD_LARGE 16502
#define GOLEM_WOOD_HUGE 16503

/* Stone Golems - Small to Huge */
#define GOLEM_STONE_SMALL 16504
#define GOLEM_STONE_MEDIUM 16505
#define GOLEM_STONE_LARGE 16506
#define GOLEM_STONE_HUGE 16507

/* Iron Golems - Small to Huge */
#define GOLEM_IRON_SMALL 16508
#define GOLEM_IRON_MEDIUM 16509
#define GOLEM_IRON_LARGE 16510
#define GOLEM_IRON_HUGE 16511

#endif /* _VNUMS_H_ */
