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
#define TRS_SLOT_SHOULDERS 10
#define TRS_SLOT_EYES 11
#define TRS_SLOT_FACE 12
#define TRS_SLOT_EARS 13
#define TRS_SLOT_ANKLET 14
#define TRS_SLOT_INSTRUMENT 15
/* for random miscellaneous items, this is the number of categories */
#define NUM_MISC_CATEGORIES 16 /* match last value above */

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
#define NUM_A_ANKLET_DESCS 3
#define NUM_A_FACE_DESCS 3
#define NUM_A_SHOULDER_DESCS 3
#define NUM_A_EYES_DESCS 4
#define NUM_A_EARS_DESCS 7

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
extern const char *ankle_descs[NUM_A_ANKLET_DESCS + 1];
extern const char *face_descs[NUM_A_FACE_DESCS + 1];
extern const char *shoulder_descs[NUM_A_SHOULDER_DESCS + 1];
extern const char *eyes_descs[NUM_A_EYES_DESCS + 1];
extern const char *ears_descs[NUM_A_EARS_DESCS + 1];

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

bool is_resist_magic_apply(int bonus);
bool is_resist_physical_apply(int bonus);
bool is_spell_slot_apply(int bonus);
bool is_bonus_valid_for_item_type(int bonus, int item_type);
bool is_bonus_valid_for_where_slot(int bonus, int wear_slot);
bool is_ability_score_apply(int bonus);
int get_gear_bonus_amount_by_level(int bonus, int olevel);
bool highlight_apply_by_obj(struct obj_data *obj, int offset);
int get_suggested_enhancement_bonus(int olevel, bool boss_mob);
void assign_random_bonuses(struct obj_data *obj, int olevel, int num_bonuses);
bool does_obj_have_wear_slots(struct obj_data *obj);
void assign_a_random_apply_to_slot(struct obj_data *obj, int olevel, int i);
void erase_bonus_info_for_slot(struct obj_data *obj, int slot);
int choose_random_apply_type(void);
int get_first_wear_slot(struct obj_data *obj);
int get_apply_no_repeat_category(int apply);
bool is_valid_spell_circle_for_class(int circle, int ch_class);

// in assign_wpn_armor.c
int get_wear_location_by_armor_type(int type);

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
