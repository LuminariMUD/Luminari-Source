/*
 * File:   treasure.h
 * Author: Zusuk (ported from d20mud)
 *
 * Created:  03/10/2013
 */

#ifndef TREASURE_H
#define	TREASURE_H


/* treasure_const.c - list of constant arrays */
extern const char *gemstones[];
extern const char *ring_descs[];
extern const char *wrist_descs[];
extern const char *neck_descs[];
extern const char *head_descs[];
extern const char *hands_descs[];
extern const char *cloak_descs[];
extern const char *waist_descs[];
extern const char *boot_descs[];
extern const char *blade_descs[];
extern const char *piercing_descs[];
extern const char *blunt_descs[];
extern const char *colors[];
extern const char *crystal_descs[];
extern const char *potion_descs[];
extern const char *armor_special_descs[];
extern const char *armor_crests[];
extern const char *handle_types[];
extern const char *head_types[];

/* defines */

/* Ornir CP System Defines, CP = creation points*/
/* we manually add +X to cp_max_bonus in the code */
#define TREASURE_MAX_BONUS   5
#define MIN_LEVEL_FOR_BONUS  10
#define CP_COST(bonus)       ((bonus - 1) * 150 + 100)
#define CP_BASE_VALUE(level) ((level - 1 + MIN_LEVEL_FOR_BONUS) * 80.6)

#define CP_TYPE_ARMOR        1
#define CP_TYPE_WEAPON       2
#define CP_TYPE_MISC         3

/* percentage chance of random treasure drop */
/* DO NOT MAKE OVER 98 (because of happyhour settings) */
#define TREASURE_PERCENT 10

/* array sizes for treasure_const.c */
#define NUM_A_GEMSTONES       26
#define NUM_A_RING_DESCS      2
#define NUM_A_WRIST_DESCS     9
#define NUM_A_NECK_DESCS      13
#define NUM_A_HAND_DESCS      3
#define NUM_A_CLOAK_DESCS     4
#define NUM_A_WAIST_DESCS     5
#define NUM_A_BOOT_DESCS      10
#define NUM_A_BLADE_DESCS     31
#define NUM_A_PIERCING_DESCS  28
#define NUM_A_BLUNT_DESCS     29
#define NUM_A_COLORS          44
#define NUM_A_CRYSTAL_DESCS   17
#define NUM_A_POTION_DESCS    29
#define NUM_A_ARMOR_SPECIAL_DESCS    25
#define NUM_A_ARMOR_CRESTS    38
#define NUM_A_HANDLE_TYPES    5
#define NUM_A_HEAD_TYPES      9

#define GRADE_MUNDANE 1
#define GRADE_MINOR   2
#define GRADE_MEDIUM  3
#define GRADE_MAJOR   4

#define TYPE_POTION 1
#define TYPE_SCROLL 2
#define TYPE_WAND   3
#define TYPE_STAFF  4

/* armor mold vnums (body, helm, sleeves, leggings) */
/*body*/
#define PLATE_BODY            3119  //0
#define HALFPLATE_BODY        3143
#define SPLINT_BODY           3144
#define BREASTPLATE_BODY      3145 /*piecemail armor*/
#define CHAIN_BODY            3146
#define STUD_LEATHER_BODY     3147  //5
#define LEATHER_BODY          3148
#define PADDED_BODY           3149
#define CLOTH_BODY            3150
/*head*/
#define PLATE_HELM            3151
#define HALFPLATE_HELM        3152  //10
#define SPLINT_HELM           3153
#define PIECEPLATE_HELM       3154
#define CHAIN_HELM            3155
#define STUD_LEATHER_HELM     3156
#define LEATHER_HELM          3157  //15
#define PADDED_HELM           3158
#define CLOTH_HELM            3159
/*arms*/
#define PLATE_ARMS            3160
#define HALFPLATE_ARMS        3161
#define SPLINT_ARMS           3162  //20
#define CHAIN_ARMS            3163
#define STUD_LEATHER_ARMS     3164
#define LEATHER_ARMS          3165
#define PADDED_ARMS           3166
#define CLOTH_ARMS            3167  //25
/*legs*/
#define PLATE_LEGS            3168
#define HALFPLATE_LEGS        3169
#define SPLINT_LEGS           3170
#define CHAIN_LEGS            3171
#define STUD_LEATHER_LEGS     3172  //30
#define LEATHER_LEGS          3173
#define PADDED_LEGS           3174
#define CLOTH_LEGS            3175
/*shields*/
#define SHIELD_MEDIUM         3180
#define SHIELD_LARGE          3181  //35
#define SHIELD_TOWER          3182
/* end armor molds */
#define NUM_ARMOR_MOLDS 37

/* weapon mold vnums */
/* simple */
/* light */
#define DAGGER                3212  //0
#define MACE                  3213
#define SICKLE                3214
/* one handed */
#define CLUB                  3215
#define MORNINGSTAR           3216
/* two handed */
#define SPEAR                 3217  //5
#define QUARTERSTAFF          3218
/* martial */
/* light */
#define HANDAXE               3219
#define KUKRI                 3220
#define SHORTSWORD            3221
/* one handed */
#define BATTLEAXE             3222  //10
#define FLAIL                 3223
#define LONGSWORD             3224
#define RAPIER                3225
#define SCIMITAR              3226
#define TRIDENT               3227  //15
#define WARHAMMER             3228
/* two handed */
#define FALCHION              3229
#define GLAIVE                3230
#define GREATAXE              3231
#define GREATCLUB             3232  //20
#define GREATSWORD            3233
#define HALBERD               3234
#define LANCE                 3235
#define SCYTHE                3236
/* exotic */
/* light */
#define KAMA                  3237  //25
/* one handed */
#define BASTARDSWORD          3238
#define DWARVENWARAXE         3239
/* two handed */
#define DIREFLAIL             3240
/* end weapon molds */
#define NUM_WEAPON_MOLDS 29


/* misc mold vnums */
#define RING_MOLD        3176  //0
#define NECKLACE_MOLD    3177
#define BOOTS_MOLD       3178
#define GLOVES_MOLD      3179
#define CLOAK_MOLD       3183
#define BELT_MOLD        3184  //5
#define WRIST_MOLD       3185
#define HELD_MOLD        3186
/* end misc molds */
#define NUM_MISC_MOLDS 8


/* item prototype for potions/scrolls/wands/staves */
#define ITEM_PROTOTYPE        3210
/* item prototype for crystals */
#define CRYSTAL_PROTOTYPE     3211

/* treasure.c functions */

/* utility */
/* some spells are not appropriate for expendable items, this simple
 function returns TRUE if the spell is OK, FALSE if not */
bool valid_item_spell(int spellnum);
/* when grouped, determine random recipient from group */
struct char_data *find_treasure_recipient(struct char_data *killer);

// determines whether the character will get treasure or not
void determine_treasure(struct char_data *ch, struct char_data *mob);
// character should get treasure, chooses what awards are given out
// uses:  award_special/expendable (potion/scroll/wand/staff)/weapon/"item"/armor
void award_magic_item(int number, struct char_data *ch, int level, int grade);
/* function that creates a random crystal */
void award_random_crystal(struct char_data *ch, int level);
// gives away staff/potion/scroll/wand
void award_expendable_item(struct char_data *ch, int grade, int type);
// gives away random magic armor
void award_magic_armor(struct char_data *ch, int grade, int moblevel, int wear_slot);
// gives away random magic weapon
void award_magic_weapon(struct char_data *ch, int grade, int moblevel);
// gives away random armor pieces (outside of body-armor/shield)
void award_misc_magic_item(struct char_data *ch, int grade, int moblevel);
// determines bonus modifiers to apply_value
int random_bonus_value(int apply_value, int level, int mod);

/* Procedures for loading mobs with items, rather than simply handing them out
 * automaticalyl after death.  Used with random treasure load resets.
 *
 * The gear loaded will be based on the mob data passed in, and the level of the
 * items should not surpass the level of the mob.
 *
 * This is a work in progress. */
void load_treasure(struct char_data *mob);


// staff tool to load random items
ACMD(do_loadmagic);
ACMD(do_loadmagicspecific);


#endif	/* TREASURE_H */

