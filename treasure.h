/*/ \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
\
/  Luminari Treasure System, Inspired by D20mud's Treasure System
/  Created By: Zusuk, original d20 code written by Gicker
\
/
\         todo: CP system by Ornir
/
\ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ /*/

#ifndef TREASURE_H
#define TREASURE_H

/* defines */

/* rare grade */
#define RARE_GRADE_NORMAL 0
#define RARE_GRADE_RARE 1
#define RARE_GRADE_LEGENDARY 2
#define RARE_GRADE_MYTHICAL 3

/* added this because the apply_X bonus is capped, stop it before
   it causes problems */
#define RANDOM_BONUS_CAP 127

/* misc-slots for random/determined magic items */
#define TRS_SLOT_FINGER 1
#define TRS_SLOT_NECK 2
#define TRS_SLOT_FEET 3
#define TRS_SLOT_HANDS 4
#define TRS_SLOT_HAND TRS_SLOT_HANDS
#define TRS_SLOT_ABOUT 5
#define TRS_SLOT_WAIST 6
#define TRS_SLOT_WRIST 7
#define TRS_SLOT_HELD 8
#define TRS_SLOT_MONK_GLOVE 9
#define TRS_SLOT_MONK_GLOVES TRS_SLOT_MONK_GLOVE
/* for random miscellaneous items, this is the number of categories */
#define NUM_MISC_CATEGORIES 9 /* match last value above */

/* Ornir CP System Defines, CP = creation points*/
/* we manually add +X to cp_max_bonus in the code */
#define TREASURE_MAX_BONUS 6
#define CRAFT_MAX_BONUS (TREASURE_MAX_BONUS + 1)
#define MIN_LEVEL_FOR_BONUS 5
#define CP_COST(bonus) ((bonus - 1) * 150 + 100)
#define CP_BASE_VALUE(level) ((level - 1 + MIN_LEVEL_FOR_BONUS) * 80.6)

#define CP_TYPE_ARMOR 1
#define CP_TYPE_WEAPON 2
#define CP_TYPE_MISC 3
#define CP_TYPE_AMMO 4
#define CP_TYPE_CRYSTAL 5

/* percentage chance of random treasure drop */
/* DO NOT MAKE OVER 98 (because of happyhour settings) */
#define TREASURE_PERCENT 5

/* array sizes for treasure_const.c */
#define NUM_A_GEMSTONES 27
#define NUM_A_RING_DESCS 3
#define NUM_A_WRIST_DESCS 10
#define NUM_A_NECK_DESCS 14
#define NUM_A_HEAD_DESCS 17
#define NUM_A_HAND_DESCS 4
#define NUM_A_HANDS_DESCS NUM_A_HAND_DESCS
#define NUM_A_MONK_GLOVE_DESCS 4
#define NUM_A_CLOAK_DESCS 5
#define NUM_A_WAIST_DESCS 6
#define NUM_A_BOOT_DESCS 11
#define NUM_A_BOOTS_DESCS NUM_A_BOOT_DESCS
#define NUM_A_BLADE_DESCS 32
#define NUM_A_PIERCING_DESCS 29
#define NUM_A_BLUNT_DESCS 30
#define NUM_A_COLORS 45
#define NUM_A_CRYSTAL_DESCS 18
#define NUM_A_POTION_DESCS 30
#define NUM_A_ARMOR_SPECIAL_DESCS 26
#define NUM_A_AMMO_DESCS 26
#define NUM_A_AMMO_HEAD_DESCS 11
#define NUM_A_ARMOR_CRESTS 39
#define NUM_A_HANDLE_TYPES 6
#define NUM_A_HEAD_TYPES 10

#define GRADE_MUNDANE 1  // 0-5
#define GRADE_MINOR 2    // 5-10
#define GRADE_TYPICAL 3  // 10-15
#define GRADE_MEDIUM 4   // 15-20
#define GRADE_MAJOR 5    // 20-25
#define GRADE_SUPERIOR 6 // 25-30

#define TYPE_POTION 1
#define TYPE_SCROLL 2
#define TYPE_WAND 3
#define TYPE_STAFF 4

/* armor mold vnums (body, helm, sleeves, leggings) */
/*body*/
#if defined(CAMPAIGN_DL)

#define PLATE_BODY 40119
#define HALFPLATE_BODY 40143
#define SPLINT_BODY 40144
#define BREASTPLATE_BODY 40145 /*piecemail armor*/
#define CHAIN_BODY 40146
#define STUD_LEATHER_BODY 40147
#define LEATHER_BODY 40148
#define PADDED_BODY 40149
#define CLOTH_BODY 40150
#define BANDED_BODY 40241
#define SCALE_BODY 40242
#define HIDE_BODY 40243
#define LIGHT_CHAIN_BODY 40244

/*head*/
#define PLATE_HELM 40151
#define HALFPLATE_HELM 40152
#define SPLINT_HELM 40153
#define PIECEPLATE_HELM 40154
#define CHAIN_HELM 40155
#define STUD_LEATHER_HELM 40156
#define LEATHER_HELM 40157
#define PADDED_HELM 40158
#define CLOTH_HELM 40159

#define BANDED_HELM 40245
#define SCALE_HELM 40246
#define HIDE_HELM 40247
#define LIGHT_CHAIN_HELM 40248

/*arms*/
#define PLATE_ARMS 40160
#define HALFPLATE_ARMS 40161
#define SPLINT_ARMS 40162
#define CHAIN_ARMS 40163
#define STUD_LEATHER_ARMS 40164
#define LEATHER_ARMS 40165
#define PADDED_ARMS 40166
#define CLOTH_ARMS 40167

#define BANDED_ARMS 40249
#define PIECEPLATE_ARMS 40250
#define SCALE_ARMS 40251
#define HIDE_ARMS 40252
#define LIGHT_CHAIN_ARMS 40253

/*legs*/
#define PLATE_LEGS 40168
#define HALFPLATE_LEGS 40169
#define SPLINT_LEGS 40170
#define CHAIN_LEGS 40171
#define STUD_LEATHER_LEGS 40172
#define LEATHER_LEGS 40173
#define PADDED_LEGS 40174
#define CLOTH_LEGS 40175

#define BANDED_LEGS 40254
#define PIECEPLATE_LEGS 40255
#define SCALE_LEGS 40256
#define HIDE_LEGS 40257
#define LIGHT_CHAIN_LEGS 40258

/*shields*/
#define SHIELD_MEDIUM 40180
#define SHIELD_LARGE 40181
#define SHIELD_TOWER 40182

/* end armor molds */
#define NUM_ARMOR_MOLDS 37

/* weapon mold vnums */
/* simple */
/* light */
#define DAGGER 40212 // 0
#define MACE 40213
#define SICKLE 40214
/* one handed */
#define CLUB 40215
#define MORNINGSTAR 40216
/* two handed */
#define SPEAR 40217 // 5
#define QUARTERSTAFF 40218
/* martial */
/* light */
#define HANDAXE 40219
#define KUKRI 40220
#define SHORTSWORD 40221
/* one handed */
#define BATTLEAXE 40222 // 10
#define FLAIL 40223
#define LONGSWORD 40224
#define RAPIER 40225
#define SCIMITAR 40226
#define TRIDENT 40227 // 15
#define WARHAMMER 40228
/* two handed */
#define FALCHION 40229
#define GLAIVE 40230
#define GREATAXE 40231
#define GREATCLUB 40232 // 20
#define GREATSWORD 40233
#define HALBERD 40234
#define LANCE 40235
#define SCYTHE 40236
/* exotic */
/* light */
#define KAMA 40237 // 25
/* one handed */
#define BASTARDSWORD 40238
#define DWARVENWARAXE 40239
/* two handed */
#define DIREFLAIL 40240
/* end weapon molds */
#define NUM_WEAPON_MOLDS 29

/* weapon blank object */
#define WEAPON_PROTO 40299
/* armor blank object */
#define ARMOR_PROTO 40298
/* ammo blank object */
#define AMMO_PROTO 40297
#define INSTRUMENT_PROTO 40296

/* misc mold vnums */
#define RING_MOLD 40176 // 0
#define NECKLACE_MOLD 40177
#define BOOTS_MOLD 40178
#define GLOVES_MOLD 40179
#define CLOAK_MOLD 40183
#define BELT_MOLD 40184 // 5
#define WRIST_MOLD 40185
#define HELD_MOLD 40186
#define EYES_MOLD 40249
#define FACE_MOLD 40250
#define EARS_MOLD 40251
#define SHOULDERS_MOLD 40252
/* end misc molds */
#define NUM_MISC_MOLDS 8

/* item prototype for potions/scrolls/wands/staves */
#define ITEM_PROTOTYPE 40210
/* item prototype for crystals */
#define CRYSTAL_PROTOTYPE 40211

#define KENDER_BAUBLE 40447

#else

#define PLATE_BODY 3119
#define HALFPLATE_BODY 3143
#define SPLINT_BODY 3144
#define BREASTPLATE_BODY 3145 /*piecemail armor*/
#define CHAIN_BODY 3146
#define STUD_LEATHER_BODY 3147
#define LEATHER_BODY 3148
#define PADDED_BODY 3149
#define CLOTH_BODY 3150
#define BANDED_BODY 3241
#define SCALE_BODY 3242
#define HIDE_BODY 3243
#define LIGHT_CHAIN_BODY 3244

/*head*/
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

/*arms*/
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

/*legs*/
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

/*shields*/
#define SHIELD_MEDIUM 3180
#define SHIELD_LARGE 3181
#define SHIELD_TOWER 3182

/* end armor molds */
#define NUM_ARMOR_MOLDS 37

/* weapon mold vnums */
/* simple */
/* light */
#define DAGGER 3212 // 0
#define MACE 3213
#define SICKLE 3214
/* one handed */
#define CLUB 3215
#define MORNINGSTAR 3216
/* two handed */
#define SPEAR 3217 // 5
#define QUARTERSTAFF 3218
/* martial */
/* light */
#define HANDAXE 3219
#define KUKRI 3220
#define SHORTSWORD 3221
/* one handed */
#define BATTLEAXE 3222 // 10
#define FLAIL 3223
#define LONGSWORD 3224
#define RAPIER 3225
#define SCIMITAR 3226
#define TRIDENT 3227 // 15
#define WARHAMMER 3228
/* two handed */
#define FALCHION 3229
#define GLAIVE 3230
#define GREATAXE 3231
#define GREATCLUB 3232 // 20
#define GREATSWORD 3233
#define HALBERD 3234
#define LANCE 3235
#define SCYTHE 3236
/* exotic */
/* light */
#define KAMA 3237 // 25
/* one handed */
#define BASTARDSWORD 3238
#define DWARVENWARAXE 3239
/* two handed */
#define DIREFLAIL 3240
/* end weapon molds */
#define NUM_WEAPON_MOLDS 29

/* weapon blank object */
#define WEAPON_PROTO 3299
/* armor blank object */
#define ARMOR_PROTO 3298
/* ammo blank object */
#define AMMO_PROTO 3297

/* misc mold vnums */
#define RING_MOLD 3176 // 0
#define NECKLACE_MOLD 3177
#define BOOTS_MOLD 3178
#define GLOVES_MOLD 3179
#define CLOAK_MOLD 3183
#define BELT_MOLD 3184 // 5
#define WRIST_MOLD 3185
#define HELD_MOLD 3186
/* end misc molds */
#define NUM_MISC_MOLDS 8

/* item prototype for potions/scrolls/wands/staves */
#define ITEM_PROTOTYPE 3210
/* item prototype for crystals */
#define CRYSTAL_PROTOTYPE 3211

#define KENDER_BAUBLE 66700

#endif

#define CUSTOM_TREASURE_MESSAGE_NONE   0
#define CUSTOM_TREASURE_MESSAGE_BORROW 1
#define CUSTOM_TREASURE_MESSAGE_SWINDLE 2
#define CUSTOM_TREASURE_MESSAGE_PERFORM 3
#define CUSTOM_TREASURE_MESSAGE_TRIBUTE 4
#define CUSTOM_TREASURE_MESSAGE_EXTORTION 5
#define CUSTOM_TREASURE_MESSAGE_SCROUNGE 6

/* treasure_const.c - list of constant arrays */
extern const char *gemstones[NUM_A_GEMSTONES + 1];
extern const char *ring_descs[NUM_A_RING_DESCS + 1];
extern const char *wrist_descs[NUM_A_WRIST_DESCS + 1];
extern const char *neck_descs[NUM_A_NECK_DESCS + 1];
extern const char *head_descs[NUM_A_HEAD_DESCS + 1];
extern const char *hands_descs[NUM_A_HANDS_DESCS + 1];
extern const char *monk_glove_descs[NUM_A_MONK_GLOVE_DESCS + 1];
extern const char *cloak_descs[NUM_A_CLOAK_DESCS + 1];
extern const char *waist_descs[NUM_A_WAIST_DESCS + 1];
extern const char *boot_descs[NUM_A_BOOT_DESCS + 1];
extern const char *blade_descs[NUM_A_BLADE_DESCS + 1];
extern const char *piercing_descs[NUM_A_PIERCING_DESCS + 1];
extern const char *blunt_descs[NUM_A_BLUNT_DESCS + 1];
extern const char *colors[NUM_A_COLORS + 1];
extern const char *crystal_descs[NUM_A_CRYSTAL_DESCS + 1];
extern const char *potion_descs[NUM_A_POTION_DESCS + 1];
extern const char *armor_special_descs[NUM_A_ARMOR_SPECIAL_DESCS + 1];
extern const char *armor_crests[NUM_A_ARMOR_CRESTS + 1];
extern const char *handle_types[NUM_A_HANDLE_TYPES + 1];
extern const char *head_types[NUM_A_HEAD_TYPES + 1];
extern const char *ammo_descs[NUM_A_AMMO_DESCS + 1];
extern const char *ammo_head_descs[NUM_A_AMMO_HEAD_DESCS + 1];
extern const char *kender_loot[NUM_KENDER_BAUBLES];

/* treasure.c functions */

/* utility */
/* some spells are not appropriate for expendable items, this simple
 function returns TRUE if the spell is OK, FALSE if not */
bool valid_item_spell(int spellnum);
/* when grouped, determine random recipient from group */
struct char_data *find_treasure_recipient(struct char_data *killer);
/* modify objects bonuses, set value, level, 'say' and deliver */
void cp_modify_object_applies(struct char_data *ch, struct obj_data *obj,
                              int enchantment_grade, int cp_type, int rare_grade, int silent_mode);
// determines whether the character will get treasure or not
void determine_treasure(struct char_data *ch, struct char_data *mob);
// character should get treasure, chooses what awards are given out
// uses:  award_special/expendable (potion/scroll/wand/staff)/weapon/"item"/armor
void award_magic_item(int number, struct char_data *ch, int grade);
/* function that creates a random crystal */
void award_random_crystal(struct char_data *ch, int grade);
// gives away staff/potion/scroll/wand
void award_expendable_item(struct char_data *ch, int grade, int type);
// gives away random magic armor
void award_magic_armor(struct char_data *ch, int grade, int wear_slot);
// gives away random magic weapon
void award_magic_weapon(struct char_data *ch, int grade);
// gives away random armor pieces (outside of body-armor/shield)
void award_misc_magic_item(struct char_data *ch, int category, int grade);
// gives away random ammo
void award_magic_ammo(struct char_data *ch, int grade);
// determines bonus modifiers to apply_value
int random_bonus_value(int apply_value, int grade, int mod);
// take an object, and set its values to an appropriate weapon of 'type'
void set_weapon_object(struct obj_data *obj, int type);
// take an object, and set its values to an appropriate armor of 'type'
void set_armor_object(struct obj_data *obj, int type);
// take an object, and set its values to an appropriate ammo of 'type'
void set_ammo_object(struct obj_data *obj, int type);
// take base material, check for upgrade based on grade
int possible_material_upgrade(int base_mat, int grade);
/* give a random 'misc' magical item to ch */
void give_misc_magic_item(struct char_data *ch, int category, int enchantment, bool silent_mode);
/* select a random valid apply-value */
int random_apply_value(void);
/* called by random_bonus_value(), cp_modify_object_applies(), */
int adjust_bonus_value(int apply_location, int bonus);
/* determine a random armor style */
int get_random_armor_suit_type(void);
/* get an armor piece by armor style and wear location */
int get_armor_piece_by_style(int style, int wear_loc);
/* Give away full suit of random magic armor
 * (includes:  body/head/legs/arms)
 * 1)  determine material
 * 2)  determine rarity
 * 3)  determine Creation Points
 * 4)  determine AC bonus (Always first stat...)
 * 5)  craft description based on object and bonuses */
void award_magic_armor_suit(struct char_data *ch, int grade);
/*  Will return a random healing potion based on level */
int get_random_healing_potion(int spell_level);
/* Will determine whether a feat makes sense on a particular object.
 * For example, having medium armor specialization on a piece of light armor
 */
bool proper_feat(struct obj_data *obj, int feat_num);

/* Procedures for loading mobs with items, rather than simply handing them out
 * automaticalyl after death.  Used with random treasure load resets.
 *
 * The gear loaded will be based on the mob data passed in, and the level of the
 * items should not surpass the level of the mob.
 *
 * This is a work in progress. */
void load_treasure(struct char_data *mob);
int determine_rnd_misc_cat();
int cp_convert_grade_enchantment(int grade);
/* given a level, determine a random appropriate grade */
int quick_grade_check(int level);
int increase_gold(struct char_data *ch, int amt);
int award_random_money(struct char_data *ch, int result);
void award_random_food_item(struct char_data *ch, int result, int type);

// staff tool to load random items
ACMD_DECL(do_loadmagic);
ACMD_DECL(do_loadmagicspecific);
// ACMD_DECL(do_bazaar);

/* special procedures */
SPECIAL_DECL(bazaar);

#define TYPE_POTION 1
#define TYPE_SCROLL 2
#define TYPE_WAND 3
#define TYPE_STAFF 4

#endif /* TREASURE_H */

/*eof*/
