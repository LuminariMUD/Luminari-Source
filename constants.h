/**
* @file constants.h                         LuminariMUD
* Declares the global constants defined in constants.c.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* All rights reserved.  See license for complete information.
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
*/
#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_

/* under construction -zusuk
extern const char *npc_race_short[];
extern const char *race_family_abbrevs[];
 */
extern const char *shape_types[MAX_PC_SUBRACES + 1];
extern const char *morph_to_char[NUM_RACE_TYPES + 1];
extern const char *morph_to_room[NUM_RACE_TYPES + 1];
extern const char *shape_to_room[MAX_PC_SUBRACES + 1];
extern const char *shape_to_char[MAX_PC_SUBRACES + 1];
extern const char *npc_race_menu;
extern const char *npc_subrace_types[NUM_SUB_RACES + 1];
extern const char *npc_subrace_abbrevs[NUM_SUB_RACES + 1];
extern const char *race_family_abbrevs[NUM_RACE_TYPES + 1];
extern const char *race_family_short[NUM_RACE_TYPES + 1];
extern const char *race_family_types[NUM_RACE_TYPES + 1];
extern const char *race_family_types_plural[NUM_RACE_TYPES + 1];
extern const char *class_names[NUM_CLASSES + 1];
extern const char *alignment_names_nocolor[NUM_ALIGNMENTS + 1];
extern const char *attack_hit_types[];
extern const char *instrument_names[MAX_INSTRUMENTS + 1];
extern const char *random_male_names[NUM_MALE_NAMES + 1];
extern const char *random_female_names[NUM_FEMALE_NAMES + 1];
extern const char * random_surnames[NUM_SURNAMES+1];
extern const char *armor_suit_types[NUM_SPEC_ARMOR_SUIT_TYPES + 1];
// extern const char *spec_armor_type[NUM_SPEC_ARMOR_TYPES + 1];
extern const char *ammo_types[NUM_AMMO_TYPES + 1];
extern const char *weapon_head_types[NUM_WEAPON_HEAD_TYPES + 1];
extern const char *weapon_handle_types[NUM_WEAPON_HANDLE_TYPES + 1];
extern const char *sizes[NUM_SIZES + 1];
extern const char *weapon_family[NUM_WEAPON_FAMILIES + 1];
extern const char *weapon_damage_types[NUM_WEAPON_DAMAGE_TYPES + 1];
extern const char *weapon_flags[NUM_WEAPON_FLAGS + 1];
// extern const int *valid_bonus_types[NUM_APPLIES][NUM_BONUS_TYPES];
extern const char *bonus_types[];
extern const char *dr_damtypes[];
extern const char *damtypes[];
extern const char *trap_type[MAX_TRAP_TYPES + 1];
extern const char *trap_effects[MAX_TRAP_EFFECTS + 1];
extern const char *ranged_weapons[NUM_RANGED_WEAPONS + 1];
extern const char *ranged_missiles[NUM_RANGED_MISSILES + 1];
extern const char *alignment_names[];
extern const char *portal_types[];
extern const char *craft_type[];
extern const char *admin_level_names[]; // for imp prefix
extern const char *item_profs[];
extern const char *material_name[];
extern const char *size_names[];
extern const int size_modifiers[];
extern const int size_modifiers_inverse[];
extern const int grapple_size_modifiers[];
extern const char *room_affections[];
extern const char * const luminari_version;
extern const char * const luminari_build;
extern const char *damtype_display[];
extern const char *dirs[];
extern const char *autoexits[];
extern const char *room_bits[];
extern const char *zone_bits[];
extern const char *exit_bits[];
extern const char *sector_types[];
extern const char *sector_types_readable[];
extern const char *genders[];
extern const char *position_types[];
extern const char *player_bits[];
extern const char *action_bits[];
extern const char *preference_bits[];
extern const char *affected_bits[];
extern const char *affected_bit_descs[];
extern const char *connected_types[];
extern const char *wear_where[];
extern const char *equipment_types[];
extern const char *item_types[];
extern const char *wear_bits[];
extern const char *extra_bits[];
extern const char *apply_types[];
extern const char *container_bits[];
extern const char *drinks[];
extern const char *drinknames[];
extern const char *color_liquid[];
extern const char *fullness[];
extern const char *weekdays[];
extern const char *month_name[];
extern const char *spell_schools[];
extern const char *spell_schools_lower[];
extern int spell_bonus[STAT_CAP + 1][NUM_CIRCLES + 1];
extern const struct str_app_type str_app[];
extern const struct dex_skill_type dex_app_skill[];
extern const struct dex_app_type dex_app[];
extern const struct con_app_type con_app[];
extern const struct int_app_type int_app[];
extern const struct wis_app_type wis_app[];
extern const struct cha_app_type cha_app[];
extern int lore_app[];
extern int rev_dir[];
extern int movement_loss[];
extern int drink_aff[][3];
extern const char *trig_types[];
extern const char *otrig_types[];
extern const char *wtrig_types[];
extern const char *history_types[];
extern const char *ibt_bits[];
extern const char *feat_types[];
extern const char *ability_names[];
extern size_t room_bits_count;
extern size_t action_bits_count;
extern size_t affected_bits_count;
extern size_t extra_bits_count;
extern size_t wear_bits_count;
extern const int druid_slots[LVL_IMPL + 1][NUM_CIRCLES + 1];
extern const int cleric_slots[LVL_IMPL + 1][NUM_CIRCLES + 1];
extern const int paladin_slots[LVL_IMPL + 1][NUM_CIRCLES + 1];
extern const int ranger_slots[LVL_IMPL + 1][NUM_CIRCLES + 1];
extern const int bard_known[LVL_IMPL + 1][NUM_CIRCLES + 1];
extern const int sorcerer_known[LVL_IMPL + 1][NUM_CIRCLES + 1];
extern const int bard_slots[LVL_IMPL + 1][NUM_CIRCLES + 1];
extern const int sorcerer_slots[LVL_IMPL + 1][NUM_CIRCLES + 1];
extern const int wizard_slots[LVL_IMPL + 1][NUM_CIRCLES + 1];
extern const int alchemist_slots[LVL_IMPL + 1][NUM_CIRCLES + 1];
// extern const char *spell_prep_dictation[NUM_CASTERS][4];
extern const char *spell_consign_dict[NUM_CLASSES][4];
extern const char *spell_prep_dict[NUM_CLASSES][4];
extern const int draconic_heritage_energy_types[NUM_DRACONIC_HERITAGE_TYPES + 1];
extern const char *draconic_heritage_names[NUM_DRACONIC_HERITAGE_TYPES + 1];
extern const char *bloodline_names[];
extern const char *do_cast_types[][6];
extern const char *ability_score_names[6];
extern const char * const faction_names[];
extern const char * const faction_names_lwr[];
extern const char *award_types[];
extern const char *paladin_mercy_descriptions[];
extern const char *paladin_mercies[];
extern const int paladin_mercy_levels[];
extern const char *fiendish_boons[];
extern const int fiendish_boon_levels[];
extern const char *fiendish_boon_descriptions[];
extern const int fiendish_boon_slots[];
extern const char *blackguard_cruelties[];
extern const char *blackguard_cruelty_descriptions[];
extern const int blackguard_cruelty_levels[];
extern const int blackguard_cruelty_affect_types[];

/* NewCraft */
extern const char *craft_flags[];
extern const char *requirement_flags[];

#endif /* _CONSTANTS_H_ */
