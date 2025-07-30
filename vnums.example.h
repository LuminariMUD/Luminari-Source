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
 * DragonLance Newbie Starting Equipment
 * ************************************************************************ */

/* Wizard Starting Gear */
/*
#define NOOB_GEAR_WIZ_WEAPON        160016
#define NOOB_GEAR_WIZ_BODY          160000
#define NOOB_GEAR_WIZ_HEAD          160001
#define NOOB_GEAR_WIZ_LEGS          160002
#define NOOB_GEAR_WIZ_ARMS          160003
*/

/* Sorcerer Starting Gear */
/*
#define NOOB_GEAR_SORC_WEAPON       160017
#define NOOB_GEAR_SORC_BODY         160000
#define NOOB_GEAR_SORC_HEAD         160001
#define NOOB_GEAR_SORC_LEGS         160002
#define NOOB_GEAR_SORC_ARMS         160003
*/

/* Cleric Starting Gear */
/*
#define NOOB_GEAR_CLER_WEAPON       160026
#define NOOB_GEAR_CLER_BODY         160012
#define NOOB_GEAR_CLER_HEAD         160013
#define NOOB_GEAR_CLER_LEGS         160014
#define NOOB_GEAR_CLER_ARMS         160015
#define NOOB_GEAR_CLER_SHIELD       160034
*/

/* Rogue Starting Gear */
/*
#define NOOB_GEAR_ROG_WEAPON        160023
#define NOOB_GEAR_ROG_BODY          160004
#define NOOB_GEAR_ROG_HEAD          160005
#define NOOB_GEAR_ROG_LEGS          160006
#define NOOB_GEAR_ROG_ARMS          160007
*/

/* Fighter Starting Gear */
/*
#define NOOB_GEAR_FIGHT_WEAPON      160033
#define NOOB_GEAR_FIGHT_BODY        160012
#define NOOB_GEAR_FIGHT_HEAD        160013
#define NOOB_GEAR_FIGHT_LEGS        160014
#define NOOB_GEAR_FIGHT_ARMS        160015
#define NOOB_GEAR_FIGHT_SHIELD      160038
#define NOOB_GEAR_FIGHT_WEAPON2     160037
*/

/* Monk Starting Gear */
/*
#define NOOB_GEAR_MONK_WEAPON       160028
#define NOOB_GEAR_MONK_BODY         160000
#define NOOB_GEAR_MONK_HEAD         160001
#define NOOB_GEAR_MONK_LEGS         160002
#define NOOB_GEAR_MONK_ARMS         160003
*/

/* Druid Starting Gear */
/*
#define NOOB_GEAR_DRUID_WEAPON      160024
#define NOOB_GEAR_DRUID_BODY        160004
#define NOOB_GEAR_DRUID_HEAD        160005
#define NOOB_GEAR_DRUID_LEGS        160006
#define NOOB_GEAR_DRUID_ARMS        160007
*/

/* Berserker Starting Gear */
/*
#define NOOB_GEAR_BERSER_WEAPON     160030
#define NOOB_GEAR_BERSER_BODY       160008
#define NOOB_GEAR_BERSER_HEAD       160009
#define NOOB_GEAR_BERSER_LEGS       160010
#define NOOB_GEAR_BERSER_ARMS       160011
*/

/* Paladin Starting Gear */
/*
#define NOOB_GEAR_PALAD_WEAPON      160031
#define NOOB_GEAR_PALAD_BODY        160012
#define NOOB_GEAR_PALAD_HEAD        160013
#define NOOB_GEAR_PALAD_LEGS        160014
#define NOOB_GEAR_PALAD_ARMS        160015
#define NOOB_GEAR_PALAD_SHIELD      160035
*/

/* Ranger Starting Gear */
/*
#define NOOB_GEAR_RANGER_WEAPON     160029
#define NOOB_GEAR_RANGER_BODY       160008
#define NOOB_GEAR_RANGER_HEAD       160009
#define NOOB_GEAR_RANGER_LEGS       160010
#define NOOB_GEAR_RANGER_ARMS       160011
*/

/* Bard Starting Gear */
/*
#define NOOB_GEAR_BARD_WEAPON       160020
#define NOOB_GEAR_BARD_INSTRUMENT   160022
#define NOOB_GEAR_BARD_BODY         160004
#define NOOB_GEAR_BARD_HEAD         160005
#define NOOB_GEAR_BARD_LEGS         160006
#define NOOB_GEAR_BARD_ARMS         160007
*/

/* Blackguard Starting Gear */
/*
#define NOOB_GEAR_BLKG_WEAPON       160032
#define NOOB_GEAR_BLKG_BODY         160012
#define NOOB_GEAR_BLKG_HEAD         160013
#define NOOB_GEAR_BLKG_LEGS         160014
#define NOOB_GEAR_BLKG_ARMS         160015
#define NOOB_GEAR_BLKG_SHIELD       160036
*/

/* Psionicist Starting Gear */
/*
#define NOOB_GEAR_PSION_WEAPON      160025
#define NOOB_GEAR_PSION_BODY        160000
#define NOOB_GEAR_PSION_HEAD        160001
#define NOOB_GEAR_PSION_LEGS        160002
#define NOOB_GEAR_PSION_ARMS        160003
*/

/* Warlock Starting Gear */
/*
#define NOOB_GEAR_WARLOCK_WEAPON    160021
#define NOOB_GEAR_WARLOCK_BODY      160004
#define NOOB_GEAR_WARLOCK_HEAD      160005
#define NOOB_GEAR_WARLOCK_LEGS      160006
#define NOOB_GEAR_WARLOCK_ARMS      160007
*/

/* Summoner Starting Gear */
/*
#define NOOB_GEAR_SUMMONER_WEAPON   160019
#define NOOB_GEAR_SUMMONER_BODY     160004
#define NOOB_GEAR_SUMMONER_HEAD     160005
#define NOOB_GEAR_SUMMONER_LEGS     160006
#define NOOB_GEAR_SUMMONER_ARMS     160007
*/

/* Inquisitor Starting Gear */
/*
#define NOOB_GEAR_INQUISITOR_WEAPON 160027
#define NOOB_GEAR_INQUISITOR_BODY   160008
#define NOOB_GEAR_INQUISITOR_HEAD   160009
#define NOOB_GEAR_INQUISITOR_LEGS   160010
#define NOOB_GEAR_INQUISITOR_ARMS   160011
*/

/* Alchemist Starting Gear */
/*
#define NOOB_GEAR_ALCHEMIST_WEAPON  160018
#define NOOB_GEAR_ALCHEMIST_BODY    160004
#define NOOB_GEAR_ALCHEMIST_HEAD    160005
#define NOOB_GEAR_ALCHEMIST_LEGS    160006
#define NOOB_GEAR_ALCHEMIST_ARMS    160007
*/

/* ************************************************************************
 * More Campaign-Specific Starting Items
 * ************************************************************************ */

/* 
 * Chronicles of Krynn Campaign Items
 * Uncomment this section for Krynn campaign
 */
/*
#define NOOB_TORCH                  20858
#define NOOB_RATIONS                20804
#define NOOB_WATERSKIN              20803
#define NOOB_BP                     20857
#define NOOB_CRAFTING_KIT           40118
#define NOOB_BOW                    20814
#define NOOB_QUIVER                 20816
#define NOOB_ARROW                  20815
#define NOOB_CRAFT_MAT              16602
#define NOOB_CRAFT_MOLD             16603
#define NOOB_WIZ_NOTE               20850
#define NOOB_WIZ_SPELLBOOK          20812
#define NOOB_LEATHER_SLEEVES        20854
#define NOOB_LEATHER_LEGGINGS       20855
#define NOOB_IRON_MACE              20861
#define NOOB_IRON_SHIELD            20863
#define NOOB_SCALE_MAIL             20807
#define NOOB_STEEL_SCIMITAR         20862
#define NOOB_WOOD_SHIELD            20864
#define NOOB_STUD_LEATHER           20851
#define NOOB_LONG_SWORD             20808
#define NOOB_CLOTH_ROBES            20809
#define NOOB_DAGGER                 20852
#define NOOB_CLOTH_SLEEVES          20865
#define NOOB_CLOTH_PANTS            20866
#define NOOB_DWARF_WARAXE           20806
#define NOOB_DROW_XBOW              20832
#define NOOB_DROW_BOLT              20831
#define NOOB_DROW_POUCH             20833
*/

/* Musical Instruments */
/*
#define LYRE                        20825
#define FLUTE                       20826
#define DRUM                        20827
#define HORN                        20828
#define HARP                        20829
#define MANDOLIN                    20830
*/

/* Special Items */
/*
#define NOOB_HOOPAK                 117
#define NOOB_GEAR_MAP_ONE           2227   
#define NOOB_GEAR_MAP_TWO           6526
*/

/* 
 * LuminariMUD and Faerun Campaign Items
 * Uncomment this section for LuminariMUD and Faerun campaign
 */
#define NOOB_TORCH                  867
#define NOOB_RATIONS                804
#define NOOB_WATERSKIN              803
#define NOOB_BP                     857
#define NOOB_CRAFTING_KIT           3118
#define NOOB_BOW                    814
#define NOOB_QUIVER                 816
#define NOOB_ARROW                  815
#define NOOB_CRAFT_MAT              3135
#define NOOB_CRAFT_MOLD             3176
#define NOOB_WIZ_NOTE               850
#define NOOB_WIZ_SPELLBOOK          812
#define NOOB_LEATHER_SLEEVES        854
#define NOOB_LEATHER_LEGGINGS       855
#define NOOB_IRON_MACE              861
#define NOOB_IRON_SHIELD            863
#define NOOB_SCALE_MAIL             807
#define NOOB_STEEL_SCIMITAR         862
#define NOOB_WOOD_SHIELD            864
#define NOOB_STUD_LEATHER           851
#define NOOB_LONG_SWORD             808
#define NOOB_CLOTH_ROBES            809
#define NOOB_DAGGER                 852
#define NOOB_CLOTH_SLEEVES          865
#define NOOB_CLOTH_PANTS            866
#define NOOB_DWARF_WARAXE           806
#define NOOB_DROW_XBOW              832
#define NOOB_DROW_BOLT              831
#define NOOB_DROW_POUCH             833
#define LYRE                        825
#define FLUTE                       826
#define DRUM                        827
#define HORN                        828
#define HARP                        829
#define MANDOLIN                    830

/* ************************************************************************
 * Crafting System - Armor Molds
 * ************************************************************************ */

/*
 * Chronicles of Krynn Armor Molds
 */

/* Body Armor Molds */
/*
#define PLATE_BODY                  40119
#define HALFPLATE_BODY              40143
#define SPLINT_BODY                 40144
#define BREASTPLATE_BODY            40145
#define CHAIN_BODY                  40146
#define STUD_LEATHER_BODY           40147
#define LEATHER_BODY                40148
#define PADDED_BODY                 40149
#define CLOTH_BODY                  40150
#define BANDED_BODY                 40241
#define SCALE_BODY                  40242
#define HIDE_BODY                   40243
#define LIGHT_CHAIN_BODY            40244
*/

/* Helm Molds */
/*
#define PLATE_HELM                  40151
#define HALFPLATE_HELM              40152
#define SPLINT_HELM                 40153
#define PIECEPLATE_HELM             40154
#define CHAIN_HELM                  40155
#define STUD_LEATHER_HELM           40156
#define LEATHER_HELM                40157
#define PADDED_HELM                 40158
#define CLOTH_HELM                  40159
#define BANDED_HELM                 40245
#define SCALE_HELM                  40246
#define HIDE_HELM                   40247
#define LIGHT_CHAIN_HELM            40248
*/

/* Arm Armor Molds */
/*
#define PLATE_ARMS                  40160
#define HALFPLATE_ARMS              40161
#define SPLINT_ARMS                 40162
#define CHAIN_ARMS                  40163
#define STUD_LEATHER_ARMS           40164
#define LEATHER_ARMS                40165
#define PADDED_ARMS                 40166
#define CLOTH_ARMS                  40167
#define BANDED_ARMS                 40249
#define PIECEPLATE_ARMS             40250
#define SCALE_ARMS                  40251
#define HIDE_ARMS                   40252
#define LIGHT_CHAIN_ARMS            40253
*/

/* Leg Armor Molds */
/*
#define PLATE_LEGS                  40168
#define HALFPLATE_LEGS              40169
#define SPLINT_LEGS                 40170
#define CHAIN_LEGS                  40171
#define STUD_LEATHER_LEGS           40172
#define LEATHER_LEGS                40173
#define PADDED_LEGS                 40174
#define CLOTH_LEGS                  40175
#define BANDED_LEGS                 40254
#define PIECEPLATE_LEGS             40255
#define SCALE_LEGS                  40256
#define HIDE_LEGS                   40257
#define LIGHT_CHAIN_LEGS            40258
*/

/* Shield Molds */
/*
#define SHIELD_MEDIUM               40180
#define SHIELD_LARGE                40181
#define SHIELD_TOWER                40182

#define NUM_ARMOR_MOLDS             37
*/

/* ************************************************************************
 * DragonLance Crafting System - Weapon Molds
 * ************************************************************************ */

/* Weapon Molds */
/*
#define DAGGER                      40212
#define MACE                        40213
#define SICKLE                      40214
#define CLUB                        40215
#define MORNINGSTAR                 40216
#define SPEAR                       40217
#define QUARTERSTAFF                40218
#define HANDAXE                     40219
#define KUKRI                       40220
#define SHORTSWORD                  40221
#define BATTLEAXE                   40222
#define FLAIL                       40223
#define LONGSWORD                   40224
#define RAPIER                      40225
#define SCIMITAR                    40226
#define TRIDENT                     40227
#define WARHAMMER                   40228
#define FALCHION                    40229
#define GLAIVE                      40230
#define GREATAXE                    40231
#define GREATCLUB                   40232
#define GREATSWORD                  40233
#define HALBERD                     40234
#define LANCE                       40235
#define SCYTHE                      40236
#define KAMA                        40237
#define BASTARDSWORD                40238
#define DWARVENWARAXE               40239
#define DIREFLAIL                   40240

#define NUM_WEAPON_MOLDS            29
*/

/* ************************************************************************
 * Crafting System - DragonLance Miscellaneous Items
 * ************************************************************************ */

/* Equipment Molds */
/*
#define RING_MOLD                   40176
#define NECKLACE_MOLD               40177
#define BOOTS_MOLD                  40178
#define GLOVES_MOLD                 40179
#define CLOAK_MOLD                  40183
#define BELT_MOLD                   40184
#define WRIST_MOLD                  40185
#define HELD_MOLD                   40186
#define EYES_MOLD                   40249
#define FACE_MOLD                   40250
#define EARS_MOLD                   40251
#define SHOULDERS_MOLD              40252
#define ANKLET_MOLD                 40253

#define NUM_MISC_MOLDS              8
*/

/* Crafting Prototypes */
/*
#define WEAPON_PROTO                40299
#define ARMOR_PROTO                 40298
#define AMMO_PROTO                  40297
#define INSTRUMENT_PROTO            40296
#define ITEM_PROTOTYPE              40210
#define CRYSTAL_PROTOTYPE           40211
#define KENDER_BAUBLE               40447
*/

/*************************************************************
 * LuminariMUD and Faerun Crafting Molds
 * Uncomment this section for Faerun campaign
 *************************************************************/

/* Armor Molds */
#define PLATE_BODY                  3119
#define HALFPLATE_BODY              3143
#define SPLINT_BODY                 3144
#define BREASTPLATE_BODY            3145
#define CHAIN_BODY                  3146
#define STUD_LEATHER_BODY           3147
#define LEATHER_BODY                3148
#define PADDED_BODY                 3149
#define CLOTH_BODY                  3150
#define BANDED_BODY                 3241
#define SCALE_BODY                  3242
#define HIDE_BODY                   3243
#define LIGHT_CHAIN_BODY            3244
#define PLATE_HELM                  3151
#define HALFPLATE_HELM              3152
#define SPLINT_HELM                 3153
#define PIECEPLATE_HELM             3154
#define CHAIN_HELM                  3155
#define STUD_LEATHER_HELM           3156
#define LEATHER_HELM                3157
#define PADDED_HELM                 3158
#define CLOTH_HELM                  3159
#define BANDED_HELM                 3245
#define SCALE_HELM                  3246
#define HIDE_HELM                   3247
#define LIGHT_CHAIN_HELM            3248
#define PLATE_ARMS                  3160
#define HALFPLATE_ARMS              3161
#define SPLINT_ARMS                 3162
#define CHAIN_ARMS                  3163
#define STUD_LEATHER_ARMS           3164
#define LEATHER_ARMS                3165
#define PADDED_ARMS                 3166
#define CLOTH_ARMS                  3167
#define BANDED_ARMS                 3249
#define PIECEPLATE_ARMS             3250
#define SCALE_ARMS                  3251
#define HIDE_ARMS                   3252
#define LIGHT_CHAIN_ARMS            3253
#define PLATE_LEGS                  3168
#define HALFPLATE_LEGS              3169
#define SPLINT_LEGS                 3170
#define CHAIN_LEGS                  3171
#define STUD_LEATHER_LEGS           3172
#define LEATHER_LEGS                3173
#define PADDED_LEGS                 3174
#define CLOTH_LEGS                  3175
#define BANDED_LEGS                 3254
#define PIECEPLATE_LEGS             3255
#define SCALE_LEGS                  3256
#define HIDE_LEGS                   3257
#define LIGHT_CHAIN_LEGS            3258
#define SHIELD_MEDIUM               3180
#define SHIELD_LARGE                3181
#define SHIELD_TOWER                3182

#define NUM_ARMOR_MOLDS             37

/* Weapon Molds */
#define DAGGER                      3212
#define MACE                        3213
#define SICKLE                      3214
#define CLUB                        3215
#define MORNINGSTAR                 3216
#define SPEAR                       3217
#define QUARTERSTAFF                3218
#define HANDAXE                     3219
#define KUKRI                       3220
#define SHORTSWORD                  3221
#define BATTLEAXE                   3222
#define FLAIL                       3223
#define LONGSWORD                   3224
#define RAPIER                      3225
#define SCIMITAR                    3226
#define TRIDENT                     3227
#define WARHAMMER                   3228
#define FALCHION                    3229
#define GLAIVE                      3230
#define GREATAXE                    3231
#define GREATCLUB                   3232
#define GREATSWORD                  3233
#define HALBERD                     3234
#define LANCE                       3235
#define SCYTHE                      3236
#define KAMA                        3237
#define BASTARDSWORD                3238
#define DWARVENWARAXE               3239
#define DIREFLAIL                   3240

#define NUM_WEAPON_MOLDS            29

/* Prototypes */
#define WEAPON_PROTO                3299
#define ARMOR_PROTO                 3298
#define AMMO_PROTO                  3297
#define ITEM_PROTOTYPE              3210
#define CRYSTAL_PROTOTYPE           3211
/* unfinished */
#define INSTRUMENT_PROTO 40296

/* Miscellaneous Items */
#define RING_MOLD                   3176
#define NECKLACE_MOLD               3177
#define BOOTS_MOLD                  3178
#define GLOVES_MOLD                 3179
#define CLOAK_MOLD                  3183
#define BELT_MOLD                   3184
#define WRIST_MOLD                  3185
#define HELD_MOLD                   3186

/* unfinished */
#define EYES_MOLD                   40249
#define FACE_MOLD                   40250
#define EARS_MOLD                   40251
#define SHOULDERS_MOLD              40252
#define ANKLET_MOLD                 40253

#define NUM_MISC_MOLDS              8

/* unfinished Kender */
#define KENDER_BAUBLE               66700
#define NOOB_HOOPAK                 117

/* End of LuminariMUD and Faerun Crafting Molds */


/**************************
 * Shared Constants
 **************************/

 /* Starting Equipment Constants */
#define NUM_NOOB_ARROWS             40
#define NUM_NOOB_DROW_BOLTS         30
#define NOOB_TELEPORTER             82


#endif /* _VNUMS_H_ */
