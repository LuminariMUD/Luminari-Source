/**
 * @file utils.h                              Part of LuminariMUD
 * Utility macros and prototypes of utility functions.
 *
 * Part of the core tbaMUD source code distribution, which is a derivative
 * of, and continuation of, CircleMUD.
 *
 * All rights reserved.  See license for complete information.
 * Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
 * CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
 *
 * @todo Merge structs, random and other very generic functions and macros into
 * the utils module.
 * @todo Take more mud specific functions and function prototypes (follower
 * functions, move functions, char_from_furniture) out of utils and declare /
 * define elsewhere.
 */
//#include "race.h"

#ifndef _UTILS_H_ /* Begin header file protection */
#define _UTILS_H_

#include "db.h"      /* for config_info */
#include "structs.h" /* for sbyte */
#include "helpers.h" /* for UPPER */
#include "perfmon.h"

#define FLAG(n) (1 << (n))

/** Definition of the action command, for the do_ series of in game functions.
 * This macro is placed here (for now) because it's too general of a macro
 * to be first defined in interpreter.h. The reason for using a macro is
 * to allow for easier addition of parameters to the otherwise generic and
 * static function structure. */
#define ACMD_DECL(name) \
  void name(struct char_data *ch, const char *argument, int cmd, int subcmd)

#define ACMD(name)                                                                             \
  static void impl_##name##_(struct char_data *ch, const char *argument, int cmd, int subcmd); \
  void name(struct char_data *ch, const char *argument, int cmd, int subcmd)                   \
  {                                                                                            \
    PERF_PROF_ENTER(pr_, #name);                                                               \
    impl_##name##_(ch, argument, cmd, subcmd);                                                 \
    PERF_PROF_EXIT(pr_);                                                                       \
  }                                                                                            \
  static void impl_##name##_(struct char_data *ch, const char *argument, int cmd, int subcmd)

/* "unsafe" version of ACMD. Commands that still require non const argument due to using
   unsafe operations on argument */
#define ACMDU(name)                                                                      \
  static void impl_##name##_(struct char_data *ch, char *argument, int cmd, int subcmd); \
  void name(struct char_data *ch, const char *argument, int cmd, int subcmd)             \
  {                                                                                      \
    PERF_PROF_ENTER(pr_, #name);                                                         \
    if (!argument)                                                                       \
    {                                                                                    \
      impl_##name##_(ch, NULL, cmd, subcmd);                                             \
    }                                                                                    \
    else                                                                                 \
    {                                                                                    \
      char arg_buf[MAX_INPUT_LENGTH];                                                    \
      strlcpy(arg_buf, argument, sizeof(arg_buf));                                       \
      impl_##name##_(ch, arg_buf, cmd, subcmd);                                          \
    }                                                                                    \
    PERF_PROF_EXIT(pr_);                                                                 \
  }                                                                                      \
  static void impl_##name##_(struct char_data *ch, char *argument, int cmd, int subcmd)

/** Definition of the helper function for checking if a command can be used.
 *  If show_error is set, an error message will be sent to the user. Otherwise, it
 *  just returns the status code.
 * Returns:
 *   0 if everything is ok
 *   1 if the character lacks prerequsites
 *   2 if the character would normally be able to use the command, but temporarily can't.
 */
#define ACMDCHECK(name) \
  int name(struct char_data *ch, bool show_error)
#define ACMD_ERRORMSG(error) \
  if (show_error == true)    \
    send_to_char(ch, error);

/* external declarations and prototypes */

/** direct all mlog() references to basic_mud_log() function. */
#define log basic_mud_log

/** Standard line size, used for many string limits. */
#define READ_SIZE 512

/* Public functions made available from utils.c. Documentation for all functions
 * are made available with the function definition. */
#define isspace_ignoretabs(c) ((c) != '\t' && isspace(c))

bool is_incorporeal(struct char_data *ch);
bool is_spell_restoreable(int spell);
bool check_poison_resist(struct char_data *ch, struct char_data *victim, int casttype, int level);
int get_poison_save_mod(struct char_data *ch, struct char_data *victim);
int is_immune_to_crits(struct char_data *attacker, struct char_data *target);
sbyte is_immune_fear(struct char_data *ch, struct char_data *victim, sbyte display);
sbyte is_immune_mind_affecting(struct char_data *ch, struct char_data *victim, sbyte display);
sbyte is_immune_charm(struct char_data *ch, struct char_data *victim, sbyte display);
sbyte is_immune_death_magic(struct char_data *ch, struct char_data *victim, sbyte display);
void remove_fear_affects(struct char_data *ch, sbyte display);
bool has_aura_of_good(struct char_data *ch);
bool has_aura_of_evil(struct char_data *ch);
bool has_one_thought(struct char_data *ch);
bool group_member_affected_by_spell(struct char_data *ch, int spellnum);
void gui_combat_wrap_open(struct char_data *ch);
void gui_combat_wrap_notvict_open(struct char_data *ch, struct char_data *vict_obj);
void gui_combat_wrap_close(struct char_data *ch);
void gui_combat_wrap_notvict_close(struct char_data *ch, struct char_data *vict_obj);
void gui_room_desc_wrap_open(struct char_data *ch);
void gui_room_desc_wrap_close(struct char_data *ch);
int BAB(struct char_data *ch);
bool is_dragon_rider_mount(struct char_data *ch);
bool is_riding_dragon_mount(struct char_data *ch);
bool is_poison_spell(int spell);
bool valid_vampire_cloak_apply(int type);
bool is_valid_ability_number(int num);
int get_region_language(int region);
const char *get_region_info(int region);
int get_vampire_cloak_bonus(int level, int type);
bool can_silence(struct char_data *ch);
bool can_daze(struct char_data *ch);
bool is_random_chest_in_room(room_rnum rrnum);
int get_random_chest_item_level(int level);
int get_chest_contents_type(void);
bool is_wearing_metal(struct char_data *ch);
bool has_aura_of_terror(struct char_data *ch);
int get_random_chest_dc(int level);
bool has_blindsense(struct char_data *ch);
int number_of_chests_per_zone(int num_zone_rooms);
void place_random_chest(room_rnum rrnum, int level, int search_dc, int pick_dc, int trap_chance);
bool can_place_random_chest_in_room(room_rnum rrnum, int num_zone_rooms, int num_chests);
int get_default_spell_weapon(struct char_data *ch);
bool can_study_known_spells(struct char_data *ch);
bool can_study_known_psionics(struct char_data *ch);
int compute_bonus_caster_level(struct char_data *ch, int class);
int compute_arcane_level(struct char_data *ch);
bool can_npc_command(struct char_data *ch);
int compute_divine_level(struct char_data *ch);
bool compute_has_combat_feat(struct char_data *ch, int cfeat, int weapon);
int compute_dexterity_bonus(struct char_data *ch);
int compute_strength_bonus(struct char_data *ch);
int compute_constitution_bonus(struct char_data *ch);
int compute_intelligence_bonus(struct char_data *ch);
int compute_wisdom_bonus(struct char_data *ch);
int compute_charisma_bonus(struct char_data *ch);
bool is_in_hometown(struct char_data *ch);
int damage_type_to_resistance_type(int type);
int stats_point_left(struct char_data *ch);
int smite_evil_target_type(struct char_data *ch);
int smite_good_target_type(struct char_data *ch);
int comp_total_stat_points(struct char_data *ch);
int compute_channel_energy_level(struct char_data *ch);
bool affected_by_aura_of_cowardice(struct char_data *ch);
bool affected_by_aura_of_sin(struct char_data *ch);
bool affected_by_aura_of_faith(struct char_data *ch);
bool affected_by_aura_of_depravity(struct char_data *ch);
bool affected_by_aura_of_righteousness(struct char_data *ch);
bool is_fear_spell(int spellnum);
bool is_grouped_with_soldier(struct char_data *ch);
bool is_crafting_skill(int skillnum);
bool is_selectable_region(int region);
int get_knowledge_skill_from_creature_type(int race_type);
bool has_fortune_of_many_bonus(struct char_data *ch);
bool has_authoritative_bonus(struct char_data *ch);
bool can_add_follower(struct char_data *ch, int mob_vnum);
bool can_add_follower_by_flag(struct char_data *ch, int flag);
char *apply_types_lowercase(int apply_type);
bool can_learn_blackguard_cruelty(struct char_data *ch, int mercy);
bool can_speak_language(struct char_data *ch, int language);
int num_blackguard_cruelties_known(struct char_data *ch);
sbyte has_blackguard_cruelties_unchosen(struct char_data *ch);
sbyte has_blackguard_cruelties_unchosen_study(struct char_data *ch);
bool affected_by_aura_of_cowardice(struct char_data *ch);
bool affected_by_aura_of_despair(struct char_data *ch);
bool has_aura_of_courage(struct char_data *ch);
bool pvp_ok_single(struct char_data *ch, bool display);
int comp_cha_cost(struct char_data *ch, int number);
int comp_base_cha(struct char_data *ch);
int comp_wis_cost(struct char_data *ch, int number);
int comp_base_wis(struct char_data *ch);
int comp_inte_cost(struct char_data *ch, int number);
int comp_base_inte(struct char_data *ch);
int comp_con_cost(struct char_data *ch, int number);
int comp_base_con(struct char_data *ch);
int comp_str_cost(struct char_data *ch, int number);
int comp_base_str(struct char_data *ch);
int comp_dex_cost(struct char_data *ch, int number);
int comp_base_dex(struct char_data *ch);
int compute_damage_reduction_full(struct char_data *ch, int dam_type, bool display);
bool is_spell_or_spell_like(int type);
bool can_act(struct char_data *ch);
int vampire_last_feeding_adjustment(struct char_data *ch);
bool can_dam_be_resisted(int type);
bool is_road_room(room_rnum room, int type);
void AoEDamageRoom(struct char_data *ch, int dam, int spellnum, int dam_type);
void dismiss_all_followers(struct char_data *ch);
bool push_attempt(struct char_data *ch, struct char_data *vict, bool display);
void remove_any_spell_with_aff_flag(struct char_data *ch, struct char_data *vict, int aff_flag, bool display);
bool can_learn_paladin_mercy(struct char_data *ch, int mercy);
int num_paladin_mercies_known(struct char_data *ch);
sbyte has_paladin_mercies_unchosen(struct char_data *ch);
sbyte has_paladin_mercies_unchosen_study(struct char_data *ch);
void calculate_max_hp(struct char_data *ch, bool display);
int compute_level_domain_spell_is_granted(int domain, int spell);
int compute_current_size(struct char_data *ch);
int  get_bonus_from_liquid_type(int liquid);
room_vnum what_vnum_is_in_this_direction(room_rnum room_origin, int direction);
int convert_alignment(int align);
void set_alignment(struct char_data *ch, int alignment);
bool is_spellcasting_class(int class_name);
int get_spellcasting_class(struct char_data *ch);
bool valid_pet_name(char *name);
bool is_retainer_in_room(struct char_data *ch);
struct char_data *get_retainer_from_room(struct char_data *ch);
int count_spellcasting_classes(struct char_data *ch);
void auto_sort_obj(struct char_data *ch, struct obj_data *obj);
void auto_store_obj(struct char_data *ch, struct obj_data *obj);
int get_bag_number_by_obj_type(struct obj_data *obj);
const char *get_align_by_num_cnd(int align);
bool char_pets_to_char_loc(struct char_data *ch);
const char *get_align_by_num(int align);
int d20(struct char_data *ch);
void manifest_mastermind_power(struct char_data *ch);
bool can_hear_sneaking(struct char_data *ch, struct char_data *vict);
bool can_see_hidden(struct char_data *ch, struct char_data *vict);
int skill_check(struct char_data *ch, int skill, int dc);
bool has_sage_mob_bonus(struct char_data *ch);
int skill_roll(struct char_data *ch, int skillnum);
void increase_skill(struct char_data *ch, int skillnum);
bool ok_call_mob_vnum(int mob_num);
int convert_material_vnum(int obj_vnum);
void basic_mud_log(const char *format, ...) __attribute__((format(printf, 1, 2)));
void basic_mud_vlog(const char *format, va_list args);
int touch(const char *path);
void mudlog(int type, int level, int file, const char *str, ...) __attribute__((format(printf, 4, 5)));
int rand_number(int from, int to);
bool is_in_water(struct char_data *ch);
float rand_float(float from, float to);
bool do_not_list_spell(int spellnum);
void set_x_y_coords(int start, int *x, int *y, int *room);
bool is_paladin_mount(struct char_data *ch, struct char_data *victim);
char *randstring(int length);
int combat_skill_roll(struct char_data *ch, int skillnum);
int dice(int number, int size);
int base_augment_psp_allowed(struct char_data *ch);
int min_dice(int num, int size, int min);
bool has_epic_power(struct char_data *ch, int powernum);
size_t sprintbit(bitvector_t vektor, const char *names[], char *result, size_t reslen);
size_t sprinttype(int type, const char *names[], char *result, size_t reslen);
bool is_flying(struct char_data *ch);
bool can_flee_speed(struct char_data *ch);
void proc_d20_round(void);
bool can_fly(struct char_data *ch);
int get_first_spellcasting_classes(struct char_data *ch);
void sprintbitarray(int bitvector[], const char *names[], int maxar, char *result);
int get_line(FILE *fl, char *buf);
int get_filename(char *filename, size_t fbufsize, int mode, const char *orig_name);
const char *get_wearoff(int abilnum);
time_t mud_time_to_secs(struct time_info_data *now);
int get_smite_evil_level(struct char_data *ch);
bool has_dr_affect(struct char_data *ch, int spell);
int get_smite_good_level(struct char_data *ch);
struct time_info_data *age(struct char_data *ch);
int num_pc_in_room(struct room_data *room);
void core_dump_real(const char *who, int line);
int count_color_chars(char *string);
bool char_has_infra(struct char_data *ch);
bool char_has_ultra(struct char_data *ch);
bool room_is_dark(room_rnum room);
bool room_is_daylit(room_rnum room);
bool can_naturally_stealthy(struct char_data *ch);
bool can_one_with_shadows(struct char_data *ch);
int get_party_size_same_room(struct char_data *ch);
int get_apply_type_gear_mod(struct char_data *ch, int apply);
int get_fast_healing_amount(struct char_data *ch);
int get_hp_regen_amount(struct char_data *ch);
int get_avg_party_level_same_room(struct char_data *ch);
int get_max_party_level_same_room(struct char_data *ch);
int levenshtein_distance(const char *s1, const char *s2);
struct time_info_data *real_time_passed(time_t t2, time_t t1);
struct time_info_data *mud_time_passed(time_t t2, time_t t1);
void prune_crlf(char *txt);
void column_list(struct char_data *ch, int num_cols, const char **list, int list_length, bool show_nums);
int get_flag_by_name(const char *flag_list[], char *flag_name);
int file_head(FILE *file, char *buf, size_t bufsize, int lines_to_read);
int file_tail(FILE *file, char *buf, size_t bufsize, int lines_to_read);
size_t file_sizeof(FILE *file);
int file_numlines(FILE *file);
int leadership_exp_multiplier(struct char_data *ch);
void clear_misc_cooldowns(struct char_data *ch);
IDXTYPE atoidx(const char *str_to_conv);
char *strfrmt(char *str, int w, int h, int justify, int hpad, int vpad);
const char *strpaste(const char *str1, const char *str2, const char *joiner);
struct char_data *is_playing(char *vict_name);
char *add_commas(long X);
bool is_monk_weapon(struct obj_data *obj);
bool can_mastermind_power(struct char_data *ch, int spellnum);
bool is_room_in_sunlight(room_rnum room);
int get_encumbrance_mod(struct char_data *ch);
bool is_covered(struct char_data *ch);
void new_affect(struct affected_type *af);
void free_affect(struct affected_type *af);
int get_class_by_name(char *classname);
int can_carry_weight_limit(struct char_data *ch);
int get_race_by_name(char *racename);
int get_subrace_by_name(char *racename);
char *convert_from_tabs(char *string);
int count_non_protocol_chars(const char *str);
const char *a_or_an(const char *string);
bool is_immaterial(struct char_data *ch);
bool is_fav_enemy_of(struct char_data *ch, int race);
int compute_arcana_golem_level(struct char_data *ch);
bool process_iron_golem_immunity(struct char_data *ch, struct char_data *victim, int element, int dam);
int count_follower_by_type(struct char_data *ch, int mob_flag);
int specific_follower_count(struct char_data *ch, mob_vnum mvnum);
int color_count(char *bufptr);
bool using_monk_gloves(struct char_data *ch);
int num_obj_in_obj(struct obj_data *obj);
bool ultra_blind(struct char_data *ch, room_rnum room_number);
bool is_room_outdoors(room_rnum room_number);
bool is_outdoors(struct char_data *ch);
bool is_in_wilderness(struct char_data *ch);
bool is_crafting_kit(struct obj_data *kit);
int get_evolution_appearance_save_bonus(struct char_data *ch);
void set_mob_grouping(struct char_data *ch);
int find_armor_type(int specType);
int add_draconic_claws_elemental_damage(struct char_data *ch, struct char_data *victim);
int calculate_cp(struct obj_data *obj);
bool paralysis_immunity(struct char_data *ch);
bool sleep_immunity(struct char_data *ch);
int get_levelup_sorcerer_bloodline_type(struct char_data *ch);
void do_study_spell_help(struct char_data *ch, int spellnum);
int get_daily_uses(struct char_data *ch, int featnum);
int start_daily_use_cooldown(struct char_data *ch, int featnum);
int daily_uses_remaining(struct char_data *ch, int featnum);
void bubbleSort(char arr[][MAX_STRING_LENGTH], int n);
int daily_item_specab_uses_remaining(struct obj_data *obj, int specab);
int start_item_specab_daily_use_cooldown(struct obj_data *obj, int specab);
bool pvp_ok(struct char_data *ch, struct char_data *target, bool display);
bool is_pc_idnum_in_room(struct char_data *ch, long int idnum);
int is_player_grouped(struct char_data *target, struct char_data *group);
int find_ability_num_by_name(char *name);
bool is_grouped_with_dragon(struct char_data *ch);
bool has_bite_attack(struct char_data *ch);
bool power_resistance(struct char_data *ch, struct char_data *victim, int modifier);
int get_power_penetrate_mod(struct char_data *ch);
int get_power_resist_mod(struct char_data *ch);
bool is_spellnum_psionic(int spellnum);
void absorb_energy_conversion(struct char_data *ch, int dam_type, int dam);
int countlines(char *filename);
bool can_blind(struct char_data *ch);
bool can_deafen(struct char_data *ch);
bool can_disease(struct char_data *ch);
bool can_poison(struct char_data *ch);
bool can_stun(struct char_data *ch);
bool can_confuse(struct char_data *ch);
bool has_psionic_body_form_active(struct char_data *ch);
bool can_spell_be_revoked(int spellnum);
void remove_locked_door_flags(room_rnum room, int door);
int is_spell_or_power(int spellnum);
sbyte isSpecialFeat(int feat);
sbyte isRacialFeat(int feat);
int hands_needed_full(struct char_data *ch, struct obj_data *obj, int use_feats);
int warlock_spell_type(int spellnum);
int get_number_of_spellcasting_classes(struct char_data *ch);
struct char_data * get_mob_follower(struct char_data *ch, int mob_type);
void send_combat_roll_info(struct char_data *ch, const char *messg, ...);
bool show_combat_roll(struct char_data *ch);
struct obj_data *get_char_bag(struct char_data *ch, int bagnum);
int get_psp_regen_amount(struct char_data *ch);
int get_mv_regen_amount(struct char_data *ch);

/* ASCII output formatting */
char *line_string(int length, char first, char second);
const char *text_line_string(const char *text, int length, char first, char second);
void draw_line(struct char_data *ch, int length, char first, char second);
void text_line(struct char_data *ch, const char *text, int length, char first, char second);

/* Saving Throws */
int savingthrow(struct char_data *ch, int save, int modifier, int dc);

/* Feats */
int get_feat_value(struct char_data *ch, int featnum);

/* Public functions made available form weather.c */
void weather_and_time(int mode);

/** Creates a core dump for diagnostic purposes, but will keep (if it can)
 * the mud running after the core has been dumped. Call this in the place
 * of calling core_dump_real. */
#define core_dump() core_dump_real(__FILE__, __LINE__)

/* Only provide our versions if one isn't in the C library. These macro names
 * will be defined by sysdep.h if a strcasecmp or stricmp exists. */
#ifndef str_cmp
int str_cmp(const char *arg1, const char *arg2);
#endif
#ifndef strn_cmp
int strn_cmp(const char *arg1, const char *arg2, int n);
#endif

size_t strlcat(char *buf, const char *src, size_t bufsz);

/* random functions in random.c */
void circle_srandom(unsigned long initial_seed);
unsigned long circle_random(void);

/* undefine MAX and MIN so that our functions are used instead */
#ifdef MAX
#undef MAX
#endif

#ifdef MIN
#undef MIN
#endif

int MAX(int a, int b);
int MIN(int a, int b);
float FLOATMAX(float a, float b);
float FLOATMIN(float a, float b);
char *CAP(char *txt);
char *UNCAP(char *txt);

/* Followers */
int num_followers_charmed(struct char_data *ch);
void die_follower(struct char_data *ch);
void add_follower(struct char_data *ch, struct char_data *leader);
void stop_follower(struct char_data *ch);
void stop_follower_engine(struct char_data *ch);
bool circle_follow(struct char_data *ch, struct char_data *victim);
bool is_grouped_in_room(struct char_data *ch);

/* in act.informative.c */
void look_at_room(struct char_data *ch, int mode);
void add_history(struct char_data *ch, const char *msg, int type);
void look_at_room_number(struct char_data *ch, int ignore_brief,
                         long room_number);
/* in spec_procs.c but connected to act.informative.c */
void ship_lookout(struct char_data *ch);

/* in act.movmement.c */
int do_simple_move(struct char_data *ch, int dir, int following);
int perform_move(struct char_data *ch, int dir, int following);

/* in class.c */
void advance_level(struct char_data *ch, int class);

void char_from_furniture(struct char_data *ch);

/*****************/
/* start defines */
/*****************/

/** What ch is currently sitting on. */
#define SITTING(ch) ((ch)->char_specials.furniture)

/** Who is sitting next to ch, if anyone. */
#define NEXT_SITTING(ch) ((ch)->char_specials.next_in_furniture)

/** Who is sitting on this obj */
#define OBJ_SAT_IN_BY(obj) ((obj)->sitting_here)

/* various constants */

/* defines for mudlog() */
#define OFF 0 /**< Receive no mudlog messages. */
#define BRF 1 /**< Receive only the most important mudlog messages. */
#define NRM 2 /**< Receive the standard mudlog messages. */
#define CMP 3 /**< Receive every mudlog message. */

/* get_filename() types of files to open */
#define CRASH_FILE 0       /**< Open up a player crash save file */
#define ETEXT_FILE 1       /**< ???? */
#define SCRIPT_VARS_FILE 2 /**< Reference to a global variable file. */
#define PLR_FILE 3         /**< The standard player file */

#define MAX_FILES 4 /**< Max number of files types vailable */

/* breadth-first searching for graph function (tracking, etc) */
#define BFS_ERROR (-1)         /**< Error in the search. */
#define BFS_ALREADY_THERE (-2) /**< Area traversed already. */
#define BFS_NO_PATH (-3)       /**< No path through here. */

/** Number of real life seconds per mud hour.
 * @todo The definitions based on SECS_PER_MUD_HOUR should be configurable.
 * See act.informative.c and utils.c for other places to change. */
#define SECS_PER_MUD_HOUR 75

/** Real life seconds in one mud day.
 * Current calculation = 30 real life minutes. */
#define SECS_PER_MUD_DAY (24 * SECS_PER_MUD_HOUR)

/** Real life seconds per mud month.
 * Current calculation = 17.5 real life hours */
#define SECS_PER_MUD_MONTH (35 * SECS_PER_MUD_DAY)

/** Real life seconds per mud month.
 * Current calculation ~= 12.4 real life days */
#define SECS_PER_MUD_YEAR (17 * SECS_PER_MUD_MONTH)

/** The number of seconds in a real minute. */
#define SECS_PER_REAL_MIN 60

/** The number of seconds in a real hour. */
#define SECS_PER_REAL_HOUR (60 * SECS_PER_REAL_MIN)

/** The number of seconds in a real day. */
#define SECS_PER_REAL_DAY (24 * SECS_PER_REAL_HOUR)

/** The number of seconds in a real year. */
#define SECS_PER_REAL_YEAR (365 * SECS_PER_REAL_DAY)

/* integer utils */
#define URANGE(a, b, c) ((b) < (a) ? (a) : ((b) > (c) ? (c) : (b)))

/* Various string utils. */
/** If a is not null, FALSE or '0', return "YES"; if it is, return "NO" */
#define YESNO(a) ((a) ? "YES" : "NO")

/** If a is not null, FALSE or '0', return "ON"; if it is, return "OFF" */
#define ONOFF(a) ((a) ? "ON" : "OFF")

/** If ch is equal to either a newline or a carriage return, return 1,
 * else 0.
 * @todo Recommend using the ? operator for clarity. */
#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r')

/** If string begins a vowel (upper or lower case), return "an"; else return
 * "a". */
#define AN(string) (strchr("aeiouAEIOU", *string) ? "an" : "a")

/** A calloc based memory allocation macro.
 * @param result Pointer to created memory.
 * @param type The type of memory (int, struct char_data, etc.).
 * @param number How many of type to make. */
#define CREATE(result, type, number)                                             \
  do                                                                             \
  {                                                                              \
    if ((number) * sizeof(type) <= 0)                                            \
      log("SYSERR: Zero bytes or less requested at %s:%d.", __FILE__, __LINE__); \
    if (!((result) = (type *)calloc((number), sizeof(type))))                    \
    {                                                                            \
      perror("SYSERR: malloc failure");                                          \
      abort();                                                                   \
    }                                                                            \
  } while (0)

/** A realloc based memory reallocation macro. Reminder: realloc can reduce
 * the size of an array as well as increase it.
 * @param result Pointer to created memory.
 * @param type The type of memory (int, struct char_data, etc.).
 * @param number How many of type to make. */
#define RECREATE(result, type, number)                                    \
  do                                                                      \
  {                                                                       \
    if (!((result) = (type *)realloc((result), sizeof(type) * (number)))) \
    {                                                                     \
      perror("SYSERR: realloc failure");                                  \
      abort();                                                            \
    }                                                                     \
  } while (0)

/** Remove an item from a linked list and reset the links.
 * If item is at the list head, change the head, else traverse the
 * list looking for the item before the one to be removed.
 * @pre Requires that a variable 'temp' be declared as the same type as the
 * list to be manipulated.
 * @post List pointers are correctly reset and item is no longer in the list.
 * item can now be changed, removed, etc independently from the list it was in.
 * @param item Pointer to item to remove from the list.
 * @param head Pointer to the head of the linked list.
 * @param next The variable name pointing to the next in the list.
 * */
#define REMOVE_FROM_LIST(item, head, next) \
  if ((item) == (head))                    \
    head = (item)->next;                   \
  else                                     \
  {                                        \
    temp = head;                           \
    while (temp && (temp->next != (item))) \
      temp = temp->next;                   \
    if (temp)                              \
      temp->next = (item)->next;           \
  }

/* Connect 'link' to the end of a double-linked list
 * The new item becomes the last in the linked list, and the last
 * pointer is updated.
 * @param link  Pointer to item to remove from the list.
 * @param first Pointer to the first item of the linked list.
 * @param last  Pointer to the last item of the linked list.
 * @param next  The variable name pointing to the next in the list.
 * @param prev  The variable name pointing to the previous in the list.
 * */
#define LINK(link, first, last, next, prev) \
  do                                        \
  {                                         \
    if (!(first))                           \
      (first) = (link);                     \
    else                                    \
      (last)->next = (link);                \
    (link)->next = NULL;                    \
    (link)->prev = (last);                  \
    (last) = (link);                        \
  } while (0)

/* Remove 'link' from a double-linked list
 * @post  item is removed from the list, but remains in memory, and must
   be free'd after unlinking.
 * @param link  Pointer to item to remove from the list.
 * @param first Pointer to the first item of the linked list.
 * @param last  Pointer to the last item of the linked list.
 * @param next  The variable name pointing to the next in the list.
 * @param prev  The variable name pointing to the previous in the list.
 * */
#define UNLINK(link, first, last, next, prev) \
  do                                          \
  {                                           \
    if (!(link)->prev)                        \
      (first) = (link)->next;                 \
    else                                      \
      (link)->prev->next = (link)->next;      \
    if (!(link)->next)                        \
      (last) = (link)->prev;                  \
    else                                      \
      (link)->next->prev = (link)->prev;      \
  } while (0)

/* Free a pointer, and log if it was NULL
 * @param point The pointer to be free'd.
 * */
#define DISPOSE(point)                                               \
  do                                                                 \
  {                                                                  \
    if (!(point))                                                    \
    {                                                                \
      log("SYSERR: Freeing null pointer %s:%d", __FILE__, __LINE__); \
    }                                                                \
    else                                                             \
      free(point);                                                   \
    point = NULL;                                                    \
  } while (0)

/* String Utils */
/* Allocate memory for a string, and return a pointer
 * @param point The string to be copied.
 * */
#define STRALLOC(point) (strdup(point))

/* Free allocated memory for a string
 * @param point The string to be free'd.
 * */
#define STRFREE(point) DISPOSE(point)

/* basic bitvector utils */

/** Return the bitarray field number x is in. */
#define Q_FIELD(x) ((int)(x) / 32)

/** Return the bit to set in a bitarray field. */
#define Q_BIT(x) (1 << ((x) % 32))

/** 1 if bit is set in the bitarray represented by var, 0 if not. */
#define IS_SET_AR(var, bit) ((var)[Q_FIELD(bit)] & Q_BIT(bit))

/** Set a specific bit in the bitarray represented by var to 1. */
#define SET_BIT_AR(var, bit) ((var)[Q_FIELD(bit)] |= Q_BIT(bit))

/** Unset a specific bit in the bitarray represented by var to 0. */
#define REMOVE_BIT_AR(var, bit) ((var)[Q_FIELD(bit)] &= ~Q_BIT(bit))

/** If bit is on in bitarray var, turn it off; if it is off, turn it on. */
#define TOGGLE_BIT_AR(var, bit) ((var)[Q_FIELD(bit)] = (var)[Q_FIELD(bit)] ^ Q_BIT(bit))

/* Older, stock tbaMUD bit settings. */

/** 1 if bit is set in flag, 0 if it is not set. */
#define IS_SET(flag, bit) ((flag) & (bit))

/** Set a specific bit in var to 1. */
#define SET_BIT(var, bit) ((var) |= (bit))

/** Set a specific bit bit in var to 0. */
#define REMOVE_BIT(var, bit) ((var) &= ~(bit))

/** If bit in var is off, turn it on; if it is on, turn it off. */
#define TOGGLE_BIT(var, bit) ((var) ^= (bit))

/* Accessing player specific data structures on a mobile is a very bad thing
 * to do.  Consider that changing these variables for a single mob will change
 * it for every other single mob in the game.  If we didn't specifically check
 * for it, 'wimpy' would be an extremely bad thing for a mob to do, as an
 * example.  If you really couldn't care less, change this to a '#if 0'. */
#if 1
/** Warn if accessing player_specials on a mob.
 * @todo Subtle bug in the var reporting, but works well for now. */
#define CHECK_PLAYER_SPECIAL(ch, var) \
  (*(((ch)->player_specials == &dummy_mob) ? (log("SYSERR: Mob using '" #var "' at %s:%d.", __FILE__, __LINE__), &(var)) : &(var)))
#else
#define CHECK_PLAYER_SPECIAL(ch, var) (var)
#endif

/** The act flags on a mob. Synonomous with PLR_FLAGS. */
#define MOB_FLAGS(ch) ((ch)->char_specials.saved.act)

/** Player flags on a PC. Synonomous with MOB_FLAGS. */
#define PLR_FLAGS(ch) ((ch)->char_specials.saved.act)

/** Preference flags on a player (not to be used on mobs). */
#define PRF_FLAGS(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.pref))

/** Affect flags on the NPC or PC. */
#define AFF_FLAGS(ch) ((ch)->char_specials.saved.affected_by)

/** Room flags.
 * @param loc The real room number. */
#define ROOM_FLAGS(loc) (world[(loc)].room_flags)

// room affections
#define ROOM_AFFECTIONS(loc) (world[(loc)].room_affections)
#define ROOM_AFFECTED(loc, aff) (IS_SET(ROOM_AFFECTIONS(loc), (aff)))

/** Zone flags.
 * @param rnum The real zone number. */
#define ZONE_FLAGS(rnum) (zone_table[(rnum)].zone_flags)

/** Zone minimum level restriction.
 * @param rnum The real zone number. */
#define ZONE_MINLVL(rnum) (zone_table[(rnum)].min_level)

/** Zone maximum level restriction.
 * @param rnum The real zone number. */
#define ZONE_MAXLVL(rnum) (zone_table[(rnum)].max_level)

/** References the routine element for a spell. Currently unused. */
#define SPELL_ROUTINES(spl) (spell_info[spl].routines)

/* IS_MOB() acts as a VALID_MOB_RNUM()-like function.*/
/** 1 if the character has the NPC bit set, 0 if the character does not.
 * Used to prevents NPCs and mobs from doing things they shouldn't, even
 * when mobs are possessed or charmed by a player. */
#define IS_NPC(ch) (IS_SET_AR(MOB_FLAGS(ch), MOB_ISNPC))

/** 1 if the character is a real NPC, 0 if the character is not. */
#define IS_MOB(ch) (IS_NPC(ch) && GET_MOB_RNUM(ch) <= top_of_mobt && \
                    GET_MOB_RNUM(ch) != NOBODY)

/** 1 if ch is flagged an NPC and flag is set in the act bitarray, 0 if not. */
#define MOB_FLAGGED(ch, flag) (IS_NPC(ch) && IS_SET_AR(MOB_FLAGS(ch), (flag)))
#define MOB_CAN_FIGHT(ch) (!MOB_FLAGGED(ch, MOB_NOFIGHT))
#define IS_FAMILIAR(ch) (MOB_FLAGGED(ch, MOB_C_FAMILIAR) && \
                         AFF_FLAGGED(ch, AFF_CHARM) &&      \
                         ch->master)
#define IS_PAL_MOUNT(ch) (MOB_FLAGGED(ch, MOB_C_MOUNT) && \
                          AFF_FLAGGED(ch, AFF_CHARM) &&   \
                          ch->master)
#define IS_BKG_MOUNT(ch) (MOB_FLAGGED(ch, MOB_C_MOUNT) && \
                          AFF_FLAGGED(ch, AFF_CHARM) &&   \
                          ch->master)
#define IS_COMPANION(ch) (MOB_FLAGGED(ch, MOB_C_ANIMAL) && \
                          AFF_FLAGGED(ch, AFF_CHARM) &&    \
                          ch->master)

/** 1 if ch is not flagged an NPC and flag is set in the act bitarray, 0 if
 * not. */
#define PLR_FLAGGED(ch, flag) (!IS_NPC(ch) && IS_SET_AR(PLR_FLAGS(ch), (flag)))

/** 1 if flag is set in the affect bitarray, 0 if not. */
#define AFF_FLAGGED(ch, flag) (IS_SET_AR(AFF_FLAGS(ch), (flag)))

/** 1 if flag is set in the preferences bitarray, 0 if not. */
#define PRF_FLAGGED(ch, flag) (IS_SET_AR(PRF_FLAGS(ch), (flag)))

/** 1 if flag is set in the room of loc, 0 if not. */
#define ROOM_FLAGGED(loc, flag) (IS_SET_AR(ROOM_FLAGS(loc), (flag)))

/** 1 if flag is set in the zone of rnum, 0 if not. */
#define ZONE_FLAGGED(rnum, flag) (IS_SET_AR(zone_table[(rnum)].zone_flags, (flag)))

/** 1 if flag is set in the exit, 0 if not. */
#define EXIT_FLAGGED(exit, flag) (IS_SET((exit)->exit_info, (flag)))

/** 1 if flag is set in the affects bitarray of obj, 0 if not. */
#define OBJAFF_FLAGGED(obj, flag) (IS_SET_AR(GET_OBJ_AFFECT(obj), (flag)))

/** 1 if flag is set in the element of obj value, 0 if not. */
#define OBJVAL_FLAGGED(obj, flag) (IS_SET(GET_OBJ_VAL((obj), 1), (flag)))

/** 1 if flag is set in the wear bits of obj, 0 if not. */
#define OBJWEAR_FLAGGED(obj, flag) (IS_SET_AR(GET_OBJ_WEAR(obj), (flag)))

/** 1 if flag is set in the extra bits of obj, 0 if not. */
#define OBJ_FLAGGED(obj, flag) (IS_SET_AR(GET_OBJ_EXTRA(obj), (flag)))

#define SET_OBJ_FLAG(obj, flag) (SET_BIT_AR(GET_OBJ_EXTRA(obj), (flag)))
#define REMOVE_OBJ_FLAG(obj, flag) (REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), (flag)))

/** 1 if spl has a flag set in routines, 0 if not. */
#define HAS_SPELL_ROUTINE(spl, flag) (IS_SET(SPELL_ROUTINES(spl), (flag)))

/** IS_AFFECTED for backwards compatibility */
#define IS_AFFECTED(ch, skill) (AFF_FLAGGED((ch), (skill)))

/** Toggle flag in ch PLR_FLAGS' turns on if off, or off if on. */
#define PLR_TOG_CHK(ch, flag) ((TOGGLE_BIT_AR(PLR_FLAGS(ch), (flag))) & Q_BIT(flag))

/** Toggle flag in ch PRF_FLAGS; turns on if off, or off if on. */
#define PRF_TOG_CHK(ch, flag) ((TOGGLE_BIT_AR(PRF_FLAGS(ch), (flag))) & Q_BIT(flag))

/** Checks to see if a PC or NPC is dead. */
#define DEAD(ch) (PLR_FLAGGED((ch), PLR_NOTDEADYET) || MOB_FLAGGED((ch), MOB_NOTDEADYET))

// used for the character short description system
#define GET_PC_DESCRIPTOR_1(ch) (ch->player_specials->saved.sdesc_descriptor_1)
#define GET_PC_DESCRIPTOR_2(ch) (ch->player_specials->saved.sdesc_descriptor_2)
#define GET_PC_ADJECTIVE_1(ch) (ch->player_specials->saved.sdesc_adjective_1)
#define GET_PC_ADJECTIVE_2(ch) (ch->player_specials->saved.sdesc_adjective_2)

/* room utils */

/** Return the sector type for the room. If there is no sector type, return
 * SECT_INSIDE. */
#define SECT(room) (VALID_ROOM_RNUM(room) ? world[(room)].sector_type : SECT_INSIDE)

/** Return the zone number for this room */
#define GET_ROOM_ZONE(room) (VALID_ROOM_RNUM(room) ? world[(room)].zone : NOWHERE)

/** TRUE if the room has no light, FALSE if not. */
#define IS_DARK(room) room_is_dark((room))

/** TRUE if the room has light, FALSE if not. */
#define IS_LIGHT(room) (!IS_DARK(room))

/** TRUE if the room is day-lit, FALSE if not. */
#define IS_DAYLIT(room) room_is_daylit((room))

/** 1 if this is a valid room number, 0 if not. */
#define VALID_ROOM_RNUM(rnum) ((rnum) != NOWHERE && (rnum) <= top_of_world)

/** The room number if this is a valid room, NOWHERE if it is not */
#define GET_ROOM_VNUM(rnum) \
  ((room_vnum)(VALID_ROOM_RNUM(rnum) ? world[(rnum)].number : NOWHERE))

/** Pointer to the room function, NULL if there is not one. */
#define GET_ROOM_SPEC(room) \
  (VALID_ROOM_RNUM(room) ? world[(room)].func : NULL)

/* char utils */

/** What room is PC/NPC in? */
#define IN_ROOM(ch) ((ch)->in_room)

/** What room was PC/NPC previously in? */
#define GET_WAS_IN(ch) ((ch)->was_in_room)

/** What WILDERNESS coordinates is the player at? */
#define X_LOC(ch) ((ch)->coords[0])
#define Y_LOC(ch) ((ch)->coords[1])

/** How old is PC/NPC, at last recorded time? */
#define GET_AGE(ch) (age(ch)->year)

/** Account name. */
#define GET_ACCOUNT_NAME(ch) ((ch)->player_specials->saved.account_name)
#define GET_ACCEXP_DESC(ch) ((ch)->desc->account->experience)

/** Name of PC. */
#define GET_PC_NAME(ch) ((ch)->player.name)

/** Name of PC or short_descr of NPC. */
#define GET_NAME(ch) (IS_NPC(ch) ? (ch)->player.short_descr : GET_PC_NAME(ch))

/** Title of PC */
#define GET_TITLE(ch) ((ch)->player.title)
#define GET_IMM_TITLE(ch) ((ch)->player.imm_title)

// level
#define GET_LEVEL(ch) ((ch)->player.level)
#define CLASS_LEVEL(ch, class) (ch->player_specials->saved.class_level[class])

#define IS_EPIC_LEVEL(ch) (GET_LEVEL(ch) > 20)
#define IS_EPIC(ch) (IS_EPIC_LEVEL(ch))

#define TOTAL_STAT_POINTS(ch) ((GET_REAL_RACE(ch) == RACE_HUMAN || GET_REAL_RACE(ch) == DL_RACE_HUMAN) ? 34 : \
                              (GET_REAL_RACE(ch) == RACE_HALF_ELF || GET_REAL_RACE(ch) == DL_RACE_HALF_ELF) ? 32 :30)
#define MAX_POINTS_IN_A_STAT 10
#define BASE_STAT 8

#define SPELLBATTLE(ch) ((ch)->char_specials.saved.spec_abil[AG_SPELLBATTLE])
#define DIVINE_LEVEL(ch) (compute_divine_level(ch))
#define ARCANE_LEVEL(ch) (compute_arcane_level(ch))
#define MAGIC_LEVEL(ch) ARCANE_LEVEL(ch)
#define ALCHEMIST_LEVEL(ch) (CLASS_LEVEL(ch, CLASS_ALCHEMIST))
#define CASTER_LEVEL(ch) (MIN(IS_NPC(ch) ? GET_LEVEL(ch) : (GET_LEVEL(ch) > 30) ? GET_LEVEL(ch) : DIVINE_LEVEL(ch) + \
                          MAGIC_LEVEL(ch) + GET_WARLOCK_LEVEL(ch) + ALCHEMIST_LEVEL(ch) - \
                          (compute_arcana_golem_level(ch)), LVL_IMMORT - 1))
#define IS_SPELLCASTER(ch) (CASTER_LEVEL(ch) > 0)
#define IS_MEM_BASED_CASTER(ch) ((CLASS_LEVEL(ch, CLASS_WIZARD) > 0))
#define GET_SHIFTER_ABILITY_CAST_LEVEL(ch) (CLASS_LEVEL(ch, CLASS_SHIFTER) + CLASS_LEVEL(ch, CLASS_DRUID))
#define GET_WARLOCK_LEVEL(ch) (GET_LEVEL(ch) > LVL_IMMORT ? GET_LEVEL(ch) : CLASS_LEVEL(ch, CLASS_WARLOCK))
#define GET_SUMMONER_LEVEL(ch) ((GET_LEVEL(ch) > LVL_IMMORT || IS_NPC(ch)) ? GET_LEVEL(ch) : CLASS_LEVEL(ch, CLASS_SUMMONER))
#define GET_CALL_EIDOLON_LEVEL(ch) ((GET_LEVEL(ch) > LVL_IMMORT || IS_NPC(ch)) ? GET_LEVEL(ch) : (CLASS_LEVEL(ch, CLASS_SUMMONER) + CLASS_LEVEL(ch, CLASS_NECROMANCER)))
#define GET_PSIONIC_LEVEL(ch) (((IS_NPC(ch) && GET_CLASS(ch) == CLASS_PSIONICIST) || GET_LEVEL(ch) >= LVL_IMMORT) ? GET_LEVEL(ch) : CLASS_LEVEL(ch, CLASS_PSIONICIST))
#define IS_PSIONIC(ch) (GET_PSIONIC_LEVEL(ch) > 0 || (IS_NPC(ch) && GET_CLASS(ch) == CLASS_PSIONICIST))
#define PSIONIC_LEVEL(ch) (MIN(IS_NPC(ch) ? GET_LEVEL(ch) : CLASS_LEVEL(ch, CLASS_PSIONICIST), LVL_IMMORT - 1))
#define IS_SPELLCASTER_CLASS(c) (c == CLASS_WIZARD || c == CLASS_CLERIC || c == CLASS_SORCERER || c == CLASS_DRUID || c == CLASS_WARLOCK || \
                                 c == CLASS_PALADIN || c == CLASS_ALCHEMIST || c == CLASS_RANGER || c == CLASS_BARD || c == CLASS_INQUISITOR || \
                                 c == CLASS_SUMMONER)

/* Password of PC. */
#define GET_PASSWD(ch) ((ch)->player.passwd)

/** The player file position of PC. */
#define GET_PFILEPOS(ch) ((ch)->pfilepos)

/** Gets the level of a player even if the player is switched.
 * @todo Make this the definition of GET_LEVEL. */
#define GET_REAL_LEVEL(ch) \
  (ch->desc && ch->desc->original ? GET_LEVEL(ch->desc->original) : GET_LEVEL(ch))

// player class is really defined by CLASS_LEVEL now - zusuk
#define GET_CLASS(ch) ((ch)->player.chclass)

// churches homeland-port
#define GET_CHURCH(ch) ((ch)->player_specials->saved.church)

/* disguise related macros */
#define GET_DISGUISE_RACE(ch) ((ch)->char_specials.saved.disguise_race)
#define IS_MORPHED(ch) (ch->player_specials->saved.morphed)
/** Race of ch. */
#define GET_REAL_RACE(ch) ((ch)->player.race)
#define SUBRACE(ch) ((ch)->player.pc_subrace)
#define GET_NPC_RACE(ch) (IS_NPC(ch) ? (ch)->player.race : RACE_UNDEFINED)

#define GET_RACE(ch) ((GET_DISGUISE_RACE(ch)) ? GET_DISGUISE_RACE(ch) : GET_REAL_RACE(ch))
#define RACE_ABBR(ch) (IS_NPC(ch) ? race_family_abbrevs[GET_RACE(ch)] : IS_WILDSHAPED(ch) ? race_list[GET_DISGUISE_RACE(ch)].abbrev_color \
                                                                                          : ((IS_MORPHED(ch) ? race_list[IS_MORPHED(ch)].abbrev_color : (GET_DISGUISE_RACE(ch) ? race_list[GET_DISGUISE_RACE(ch)].abbrev_color : race_list[GET_RACE(ch)].abbrev_color))))
/*#define RACE_ABBR(ch)  (IS_NPC(ch) ? race_family_abbrevs[GET_RACE(ch)] : IS_MORPHED(ch) ? \
  race_family_abbrevs[IS_MORPHED(ch)] : (GET_DISGUISE_RACE(ch)) ? \
  race_list[GET_DISGUISE_RACE(ch)].abbrev : race_list[GET_RACE(ch)].abbrev)*/
#define RACE_ABBR_REAL(ch) (race_list[GET_REAL_RACE(ch)].abbrev)

/* wildshape */
#define IS_WILDSHAPED(ch) (AFF_FLAGGED(ch, AFF_WILD_SHAPE) && GET_DISGUISE_RACE(ch))

/** Height of ch. */
#define GET_HEIGHT(ch) ((ch)->player.height)

/** Weight of ch. */
#define GET_WEIGHT(ch) ((ch)->player.weight)

/** Sex of ch. */
#define GET_SEX(ch) ((ch)->player.sex)

#define GET_CH_AGE(ch) (ch->player_specials->saved.character_age)

/* absolute stat cap */
#define STAT_CAP 50

/** Current strength of ch. */
#define GET_REAL_STR(ch) ((ch)->real_abils.str)
#define GET_DISGUISE_STR(ch) ((ch)->disguise_abils.str)
#define GET_STR(ch) ((AFF_FLAGGED(ch, AFF_WILD_SHAPE) && GET_DISGUISE_RACE(ch)) ? GET_DISGUISE_STR(ch) + (ch)->aff_abils.str : (ch)->aff_abils.str)
#define GET_STR_BONUS(ch) (compute_strength_bonus(ch))
/** Current strength modifer of ch, not in use (from stock circle) */
#define GET_ADD(ch) ((ch)->aff_abils.str_add)

/** Current dexterity of ch. */
#define GET_REAL_DEX(ch) ((ch)->real_abils.dex)
#define GET_DISGUISE_DEX(ch) ((ch)->disguise_abils.dex)
#define GET_DEX(ch) ((AFF_FLAGGED(ch, AFF_WILD_SHAPE) && GET_DISGUISE_RACE(ch)) ? GET_DISGUISE_DEX(ch) + (ch)->aff_abils.dex : (ch)->aff_abils.dex)
#define GET_DEX_BONUS(ch) (compute_dexterity_bonus(ch))

/** Current constitution of ch. */
#define GET_REAL_CON(ch) ((ch)->real_abils.con)
#define GET_DISGUISE_CON(ch) ((ch)->disguise_abils.con)
#define GET_CON(ch) ((AFF_FLAGGED(ch, AFF_WILD_SHAPE) && GET_DISGUISE_RACE(ch)) ? GET_DISGUISE_CON(ch) + (ch)->aff_abils.con : (ch)->aff_abils.con)
#define GET_CON_BONUS(ch) (compute_constitution_bonus(ch))

/** Current intelligence of ch. */
#define GET_REAL_INT(ch) ((ch)->real_abils.intel)
#define GET_INT(ch) ((ch)->aff_abils.intel)
#define GET_INT_BONUS(ch) (((ch)->aff_abils.intel - 10) / 2)
#define GET_REAL_INT_BONUS(ch) (compute_intelligence_bonus(ch))

/** Current wisdom of ch. */
#define GET_REAL_WIS(ch) ((ch)->real_abils.wis)
#define GET_WIS(ch) ((ch)->aff_abils.wis)
#define GET_WIS_BONUS(ch) (compute_wisdom_bonus(ch))

/** Current charisma of ch. */
#define GET_REAL_CHA(ch) ((ch)->real_abils.cha)
#define GET_CHA(ch) ((ch)->aff_abils.cha)
#define GET_CHA_BONUS(ch) (compute_charisma_bonus(ch))

/** Experience points of ch. */
#define GET_EXP(ch) ((ch)->points.exp)
/** Armor class of ch. */
/* Note that this system is basically inspired by d20, but by a factor of
   10.  So naked AC = 10 in d20, or in our system 100 */
#define GET_DISGUISE_AC(ch) ((ch)->points.disguise_armor)
#define GET_REAL_AC(ch) ((ch)->real_points.armor)
#define GET_AC(ch) ((AFF_FLAGGED(ch, AFF_WILD_SHAPE) && GET_DISGUISE_RACE(ch)) ? GET_DISGUISE_AC(ch) + (ch)->points.armor : (ch)->points.armor)
/** Current hit points (health) of ch. */
#define GET_HIT(ch) ((ch)->points.hit)
/** Maximum hit points of ch. */
#define GET_REAL_MAX_HIT(ch) ((ch)->real_points.max_hit)
#define GET_MAX_HIT(ch) ((ch)->points.max_hit)
/** Current move points (stamina) of ch. */
#define GET_MOVE(ch) ((ch)->points.move)
/** Maximum move points (stamina) of ch. */
#define GET_REAL_MAX_MOVE(ch) ((ch)->real_points.max_move)
#define GET_MAX_MOVE(ch) ((ch)->points.max_move)
/** Current psp points (magic) of ch. */
#define GET_PSP(ch) ((ch)->points.psp)
/** Maximum psp points (magic) of ch. */
#define GET_REAL_MAX_PSP(ch) ((ch)->real_points.max_psp)
#define GET_MAX_PSP(ch) ((ch)->points.max_psp)
#define GET_AUGMENT_PSP(ch) ((ch)->player_specials->augment_psp)
// Regen Rates
#define GET_HP_REGEN(ch) (ch->char_specials.saved.hp_regen)
#define GET_MV_REGEN(ch) (ch->char_specials.saved.mv_regen)
#define GET_PSP_REGEN(ch) (ch->char_specials.saved.psp_regen)
#define GET_ENCUMBRANCE_MOD(ch) (ch->char_specials.saved.encumbrance_mod)
#define GET_FAST_HEALING_MOD(ch) (ch->char_specials.saved.fast_healing_mod)
#define GET_INITIATIVE_MOD(ch) (ch->char_specials.saved.initiative_mod)
/** Gold on ch. */
#define GET_GOLD(ch) ((ch)->points.gold)
/** Gold in bank of ch. */
#define GET_BANK_GOLD(ch) ((ch)->points.bank_gold)
/** Current to-hit roll modifier for ch. */
#define GET_REAL_HITROLL(ch) ((ch)->real_points.hitroll)
#define GET_HITROLL(ch) ((ch)->points.hitroll)
/** Current damage roll modifier for ch. */
#define GET_REAL_DAMROLL(ch) ((ch)->real_points.damroll)
#define GET_DAMROLL(ch) ((ch)->points.damroll)
/** Current spell resistance modifier for ch. */
#define GET_REAL_SPELL_RES(ch) ((ch)->real_points.spell_res)
#define GET_SPELL_RES(ch) ((ch)->points.spell_res)
// size
#define GET_REAL_SIZE(ch) ((ch)->real_points.size)
#define GET_SIZE(ch) (compute_current_size(ch))
/* resistances to dam_types */
#define GET_REAL_RESISTANCES(ch, type) ((ch)->real_points.resistances[type])
#define GET_RESISTANCES(ch, type) ((ch)->points.resistances[type])
/** Saving throw i for character ch. */
#define GET_REAL_SAVE(ch, i) ((ch)->real_points.apply_saving_throw[i])
#define GET_SAVE(ch, i) ((ch)->points.apply_saving_throw[i])

/* Damage reduction structure for character ch */
#define GET_DR(ch) ((ch)->char_specials.saved.damage_reduction)
#define GET_DR_MOD(ch) ((ch)->char_specials.saved.damage_reduction_mod)

// ***  char_specials (there are others spread about utils.h file) *** //
#define GET_ELDRITCH_SHAPE(ch) ((ch)->char_specials.saved.eldritch_shape)
#define GET_ELDRITCH_ESSENCE(ch) ((ch)->char_specials.saved.eldritch_essence)
#define VITAL_STRIKING(ch) ((ch)->player_specials->saved.vital_strike)
/** Current position (standing, sitting) of ch. */
#define GET_POS(ch) ((ch)->char_specials.position)
/** Timer  */
#define TIMER(ch) ((ch)->char_specials.timer)
/** Weight carried by ch. */
#define IS_CARRYING_W(ch) ((ch)->char_specials.carry_weight)
/** Number of items carried by ch. */
#define IS_CARRYING_N(ch) ((ch)->char_specials.carry_items)
/** ch's Initiative */
#define GET_INITIATIVE(ch) ((ch)->char_specials.initiative)
/** Who or what ch is fighting. */
#define FIGHTING(ch) ((ch)->char_specials.fighting)
/** Who or what the ch is hunting. */
#define HUNTING(ch) ((ch)->char_specials.hunting)
/** Who is ch guarding? */
#define GUARDING(ch) ((ch)->char_specials.guarding)
/** Is ch firing a missile weapon? */
#define FIRING(ch) ((ch)->char_specials.firing)
/** is ch auto-eldritch-blasting? */
#define BLASTING(ch) ((ch)->char_specials.blasting)
/** Condensed Combat */
#define CNDNSD(ch) ((ch)->char_specials.condensed_combat)

#define GET_NODAZE_COOLDOWN(ch) (ch->char_specials.daze_cooldown)

/* Mode data  */
/* Power attack level and Combat expertise level are stored in the same place*/
#define COMBAT_MODE_VALUE(ch) ((ch)->char_specials.mode_value)

#define POWER_ATTACK(ch) ((ch)->char_specials.mode_value)
#define COMBAT_EXPERTISE(ch) ((ch)->char_specials.mode_value)
#define GET_DC_BONUS(ch) ((ch)->player_specials->dc_bonus)

/** Unique ID of ch. */
#define GET_IDNUM(ch) ((ch)->char_specials.saved.idnum)
/** Returns contents of id field from x. */
#define GET_ID(x) ((x)->id)

/* we changed the alignment system on luminarimud, but built it on top
   of the stock one */
/*
 * Lawful Good  0
 * Neutral Good  1
 * Chaotic Good  2
 * Lawful Neutral  3
 * True Neutral  4
 * Chaotic Neutral  5
 * Lawful Evil  6
 * Neutral Evil  7
 * Chaotic Evil  8
 * #define NUM_ALIGNMENTS 9
 */
/** Alignment value for ch. */
#define GET_ALIGNMENT(ch) ((ch)->char_specials.saved.alignment)
#define GET_FACTION(ch) ((ch)->player_specials->saved.faction)

/* Casting time */
#define IS_CASTING(ch) ((ch)->char_specials.isCasting)
#define CASTING_TIME(ch) ((ch)->char_specials.castingTime)
#define CASTING_TCH(ch) ((ch)->char_specials.castingTCH)
#define CASTING_TOBJ(ch) ((ch)->char_specials.castingTOBJ)
#define CASTING_SPELLNUM(ch) ((ch)->char_specials.castingSpellnum)
#define CASTING_METAMAGIC(ch) ((ch)->char_specials.castingMetamagic)
#define CASTING_CLASS(ch) ((ch)->char_specials.castingClass)

/* this is an array of variables associated with bardic performance */
#define GET_PERFORMANCE_VAR(ch, var) (ch->char_specials.performance_vars[var])
#define IS_PERFORMING(ch) GET_PERFORMANCE_VAR(ch, 0)
#define GET_PERFORMING(ch) GET_PERFORMANCE_VAR(ch, 1)

// spell preparation queue and collection (prepared spells))
/* this refers to items in the list of spells the ch is trying to prepare */
#define PREPARATION_QUEUE(ch, slot, cc) (ch->player_specials->saved.prep_queue[slot][cc])
/* this refers to preparation-time in a list that parallels the preparation_queue
    OLD system, this can be phased out */
#define PREP_TIME(ch, slot, cc) (ch->player_specials->saved.prep_queue[slot][cc].prep_time)
/* this refers to items in the list of spells the ch already has prepared (collection) */
#define PREPARED_SPELLS(ch, slot, cc) (ch->player_specials->saved.collection[slot][cc])
#define SPELL_COLLECTION_OLD(ch, slot, cc) PREPARED_SPELLS(ch, slot, cc)
/* given struct entry, this is the appropriate class for this spell in relation to queue/collection */
#define PREP_CLASS(ch, slot, cc) (ch->player_specials->saved.prep_queue[slot][cc].ch_class)
/* bitvector of metamagic affecting this spell */
#define PREP_METAMAGIC(ch, slot, cc) (ch->player_specials->saved.prep_queue[slot][cc].metamagic)

/* boolean indicating whether someone is in the process of preparation of a spell or not */
#define IS_PREPARING(ch, cc) ((ch)->char_specials.is_preparing[cc])

//  spells / skills
#define GET_ALIGNMENT(ch) ((ch)->char_specials.saved.alignment)

/* ranger favored enemy array */
#define GET_FAVORED_ENEMY(ch, slot) ((ch)->player_specials->saved.favored_enemy[slot])

//  our spec_abil values
#define GET_SPEC_ABIL(ch, slot) ((ch)->char_specials.saved.spec_abil[slot])
//  better macros for spec abils
#define IS_FAV_ENEMY_OF(ch, race) (is_fav_enemy_of(ch, race))
#define GET_ANIMAL_COMPANION(ch) ((ch)->char_specials.saved.spec_abil[CALLCOMPANION])
#define GET_FAMILIAR(ch) ((ch)->char_specials.saved.spec_abil[CALLFAMILIAR])
#define GET_MOUNT(ch) ((ch)->char_specials.saved.spec_abil[CALLMOUNT])
#define GET_SPELL_MANTLE(ch) ((ch)->char_specials.saved.spec_abil[SPELL_MANTLE])
#define IS_SORC_LEARNED(ch) ((ch)->char_specials.saved.spec_abil[SORC_KNOWN])
#define IS_DRUID_LEARNED(ch) ((ch)->char_specials.saved.spec_abil[DRUID_KNOWN])
#define IS_BARD_LEARNED(ch) ((ch)->char_specials.saved.spec_abil[BARD_KNOWN])
#define IS_RANG_LEARNED(ch) ((ch)->char_specials.saved.spec_abil[RANG_KNOWN])
#define IS_WIZ_LEARNED(ch) ((ch)->char_specials.saved.spec_abil[WIZ_KNOWN])
/* incendiary cloud spell */
#define INCENDIARY(ch) ((ch)->char_specials.saved.spec_abil[INCEND])
#define SONG_AFF_VAL(ch) ((ch)->char_specials.saved.spec_abil[SONG_AFF])
#define GET_SHAPECHANGES(ch) ((ch)->char_specials.saved.spec_abil[SHAPECHANGES])
/* creeping doom spell */
#define DOOM(ch) ((ch)->char_specials.saved.spec_abil[C_DOOM])
#define TENACIOUS_PLAGUE(ch) ((ch)->char_specials.saved.spec_abil[C_TENACIOUS_PLAGUE])
/* how many bursts of cloudkill left */
#define CLOUDKILL(ch) ((ch)->char_specials.saved.spec_abil[CLOUD_K])
/*trelux applypoison variables*/
#define TRLX_PSN_VAL(ch) ((ch)->char_specials.saved.spec_abil[TRLX_PSN_SPELL_VAL])
#define TRLX_PSN_LVL(ch) ((ch)->char_specials.saved.spec_abil[TRLX_PSN_SPELL_LVL])
#define TRLX_PSN_HIT(ch) ((ch)->char_specials.saved.spec_abil[TRLX_PSN_SPELL_HIT])
/* moved SPELLBATTLE up near caster_level */

#define GET_WARDING(ch, slot) ((ch)->char_specials.saved.warding[slot])
#define GET_IMAGES(ch) ((ch)->char_specials.saved.warding[MIRROR])
#define GET_STONESKIN(ch) ((ch)->char_specials.saved.warding[STONESKIN])
#define GET_LOST_XP(ch) ((ch)->char_specials.saved.warding[STORED_XP])
#define GET_WARD_POINTS(ch) (GET_STONESKIN((ch)))
#define TOTAL_DEFENSE(ch) ((ch)->char_specials.totalDefense)
#define MOUNTED_BLOCKS_LEFT(ch) ((ch)->char_specials.mounted_blocks_left)
#define DEFLECT_ARROWS_LEFT(ch) ((ch)->char_specials.deflect_arrows_left)
#define GET_SALVATION_NAME(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->salvation_name))
#define GET_SALVATION_ROOM(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->salvation_room))

/** Return condition i (DRUNK, HUNGER, THIRST) of ch. */
#define GET_COND(ch, i) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.conditions[(i)]))

/** The room to load player ch into. */
#define GET_LOADROOM(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.load_room))

// practices, training and boost sessions remaining
#define GET_PRACTICES(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.spells_to_learn))
#define GET_TRAINS(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.abilities_to_learn))
#define GET_BOOSTS(ch) CHECK_PLAYER_SPECIAL((ch), \
                                            ((ch)->player_specials->saved.boosts))

/* special spell macros */
#define IS_EPIC_SPELL(spellnum) (spellnum == SPELL_MUMMY_DUST || spellnum == SPELL_DRAGON_KNIGHT ||                                   \
                                 spellnum == SPELL_GREATER_RUIN || spellnum == SPELL_HELLBALL || spellnum == SPELL_EPIC_MAGE_ARMOR || \
                                 spellnum == SPELL_EPIC_WARDING)

/* domain macros */
#define GET_1ST_DOMAIN(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.domain_1))
#define GET_2ND_DOMAIN(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.domain_2))
#define HAS_DOMAIN(ch, domain) (GET_1ST_DOMAIN(ch) == domain || GET_2ND_DOMAIN(ch) == domain)
/* macro for determining the level you get a spell, added to support
 domain granted-spells */
/* this will return 99 if the 'domain' doesn't grant the 'spell' */
#define LEVEL_DOMAIN_GRANTS_SPELL(domain, spell) (compute_level_domain_spell_is_granted(domain, spell))
#define MIN_SPELL_LVL(spell, chclass, chdomain) (MIN((spell_info[spell].min_level[chclass]), (spell_info[spell].domain[chdomain])))
/* wizard school of magic specialty */
#define GET_SPECIALTY_SCHOOL(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.specialty_school))
#define IS_SPECIALTY_SCHOOL(ch, spellnum) (IS_NPC(ch) ? 0 : GET_SPECIALTY_SCHOOL(ch) == spell_info[spellnum].schoolOfMagic)

#define GET_1ST_RESTRICTED_SCHOOL(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.restricted_school_1))
#define GET_2ND_RESTRICTED_SCHOOL(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.restricted_school_2))
#define IS_RESTRICTED_SCHOOL(ch, i) (GET_1ST_RESTRICTED_SCHOOL(ch) == i || \
                                     GET_2ND_RESTRICTED_SCHOOL(ch) == i)

/** Current invisibility level of ch. */
#define GET_INVIS_LEV(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.invis_level))

/** Current wimpy level of ch. */
#define GET_WIMP_LEV(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.wimp_level))

/** Current freeze level (god command) inflicted upon ch. */
#define GET_FREEZE_LEV(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.freeze_level))

/** Current number of bad password attempts at logon. */
#define GET_BAD_PWS(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.bad_pws))

/** Not used?
 * @deprecated Should not be used, as the talks field has been removed. */
#define GET_TALK(ch, i) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.talks[i]))

/** The poofin string for the ch. */
#define POOFIN(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofin))

/** The poofout string for the ch. */
#define POOFOUT(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofout))

/** The OLC zoon permission for ch.
 * @deprecated Currently unused? */
#define GET_OLC_ZONE(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.olc_zone))

/** Currently unused?
 * @deprecated Currently unused? */
#define GET_LAST_OLC_TARG(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_olc_targ))

/** Currently unused?
 * @deprecated Currently unused? */
#define GET_LAST_OLC_MODE(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_olc_mode))

/** Retrieve command aliases for ch. */
#define GET_ALIASES(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->aliases))

/** Who ch last spoke to with the 'tell' command. */
#define GET_LAST_TELL(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_tell))

/** Get unique session id for ch. */
#define GET_PREF(ch) ((ch)->pref)

/** The number of ticks until the player can perform more diplomacy. */
#define GET_DIPTIMER(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->diplomacy_wait))

/** Get host name or ip of ch. */
#define GET_HOST(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->host))
#define GET_LAST_MOTD(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.lastmotd))
#define GET_LAST_NEWS(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.lastnews))

/** Get channel history i for ch. */
#define GET_HISTORY(ch, i) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.comm_hist[i]))

/** Return the page length (height) for ch. */
#define GET_PAGE_LENGTH(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.page_length))

/** Return the page width for ch */
#define GET_SCREEN_WIDTH(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.screen_width))

/** staff todo lists */
#define GET_TODO(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.todo_list))

/* Autoquests data */
/** Return the number of questpoints ch has. */
#define GET_QUESTPOINTS(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.questpoints))

/** Return the current quest that a player has assigned */
#define GET_QUEST(ch, index) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.current_quest[index]))
//#define GET_QUEST(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.current_quest))

/** Number of goals completed for this quest. */
#define GET_QUEST_COUNTER(ch, index) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.quest_counter[index]))

/** Time remaining to complete the quest ch is currently on. */
#define GET_QUEST_TIME(ch, index) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.quest_time[index]))
//#define GET_QUEST_TIME(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.quest_time))

/** The number of quests completed by ch. */
#define GET_NUM_QUESTS(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.num_completed_quests))

/** The type of quest ch is currently participating in. */
#define GET_QUEST_TYPE(ch, index) (real_quest(GET_QUEST(ch, index)) != NOTHING ? aquest_table[real_quest(GET_QUEST(ch, index))].type : AQ_UNDEFINED)

/* staff ran events data */
#define STAFFRAN_PVAR(ch, variable) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.staff_ran_events[variable]))

/**** Clans *****/
/** Return the vnum of the clan that the player belongs to */
#define GET_CLAN(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.clan))
/** Return the player's rank within their clan */
#define GET_CLANRANK(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.clanrank))
/** Return the current clanpoints that a player has acquired */
#define GET_CLANPOINTS(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.clanpoints))

/** The current skill level of ch for skill i. */
#define GET_SKILL(ch, i) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.skills[i]))
#define GET_SPELL(ch, i) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.spells[i]))

/** Copy the current skill level i of ch to pct. */
#define SET_SKILL(ch, i, pct)                                                 \
  do                                                                          \
  {                                                                           \
    CHECK_PLAYER_SPECIAL((ch), (ch)->player_specials->saved.skills[i]) = pct; \
  } while (0)

/** retrieves the sorcerer bloodline of the player character */
#define GET_SORC_BLOODLINE(ch) (get_sorcerer_bloodline_type(ch))
/** retrieves the sorcerer bloodline subtype, for example dragon color for draconic bloodline */
#define GET_BLOODLINE_SUBTYPE(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.sorcerer_bloodline_subtype))

/** The current trained level of ch for ability i. */
#define GET_ABILITY(ch, i) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.abilities[i]))

/** Copy the current ability level i of ch to pct. */
#define SET_ABILITY(ch, i, pct)                                                  \
  do                                                                             \
  {                                                                              \
    CHECK_PLAYER_SPECIAL((ch), (ch)->player_specials->saved.abilities[i]) = pct; \
  } while (0)

/* Levelup - data storage for study command. */
#define LEVELUP(ch) (ch->player_specials->levelup)

/* Feats */
/*#define MOB_FEATS(ch)           ((ch)->char_specials.saved.feats[i])*/
#define MOB_HAS_FEAT(ch, i) ((ch)->char_specials.mob_feats[i])
#define MOB_SET_FEAT(ch, i, j) ((ch)->char_specials.mob_feats[i] = j)

#define GET_FEAT_POINTS(ch) (ch->player_specials->saved.feat_points)
#define GET_EPIC_FEAT_POINTS(ch) (ch->player_specials->saved.epic_feat_points)
#define GET_CLASS_FEATS(ch, cl) (ch->player_specials->saved.class_feat_points[cl])
#define GET_EPIC_CLASS_FEATS(ch, cl) (ch->player_specials->saved.epic_class_feat_points[cl])

#define IS_EPIC_FEAT(featnum) (feat_list[featnum].epic == TRUE)
#define IS_SPELL_CIRCLE_FEAT(featnum) ((FEAT_BARD_1ST_CIRCLE <= featnum && featnum <= FEAT_BARD_EPIC_SPELL) || (FEAT_CLERIC_1ST_CIRCLE <= featnum && featnum <= FEAT_CLERIC_EPIC_SPELL) || (FEAT_DRUID_1ST_CIRCLE <= featnum && featnum <= FEAT_DRUID_EPIC_SPELL) || (FEAT_PALADIN_1ST_CIRCLE <= featnum && featnum <= FEAT_PALADIN_4TH_CIRCLE) || (FEAT_RANGER_1ST_CIRCLE <= featnum && featnum <= FEAT_RANGER_4TH_CIRCLE) || (FEAT_SORCERER_1ST_CIRCLE <= featnum && featnum <= FEAT_SORCERER_EPIC_SPELL) || (FEAT_WIZARD_1ST_CIRCLE <= featnum && featnum <= FEAT_WIZARD_EPIC_SPELL))

#define HAS_REAL_FEAT(ch, i) ((ch)->char_specials.saved.feats[i])
#define HAS_FEAT(ch, i) (get_feat_value((ch), i))
#define SET_FEAT(ch, i, j) ((ch)->char_specials.saved.feats[i] = j)
//#define HAS_COMBAT_FEAT(ch,i,j) ( IS_SET_AR((ch)->char_specials.saved.combat_feats[i], j) )
#define HAS_COMBAT_FEAT(ch, i, j) ((compute_has_combat_feat((ch), (i), (j))))
#define SET_COMBAT_FEAT(ch, i, j) (SET_BIT_AR((ch)->char_specials.saved.combat_feats[(i)], (j)))
#define HAS_SCHOOL_FEAT(ch, i, j) (IS_SET((ch)->char_specials.saved.school_feats[(i)], (1 << (j))))
#define SET_SCHOOL_FEAT(ch, i, j) (SET_BIT((ch)->char_specials.saved.school_feats[(i)], (1 << (j))))
#define HAS_SKILL_FEAT(ch, i, j) ((ch)->player_specials->saved.skill_focus[i][j])
#define SET_SKILL_FEAT(ch, i, j) (((ch)->player_specials->saved.skill_focus[i][j]) ? (ch)->player_specials->saved.skill_focus[i][j] = FALSE : (ch)->player_specials->saved.skill_focus[i][j] = TRUE)
#define GET_SKILL_FEAT(ch, i, j) ((ch)->player_specials->saved.skill_focus[i][j])

/* Macros to check LEVELUP feats. */
#define HAS_LEVELUP_FEAT(ch, i) (has_feat_requirement_check((ch), i))
#define SET_LEVELUP_FEAT(ch, i, j) (LEVELUP(ch)->feats[i] = j)
#define HAS_LEVELUP_COMBAT_FEAT(ch, i, j) ((i == -1) ? 0 : IS_SET_AR(LEVELUP(ch)->combat_feats[i], j))
#define SET_LEVELUP_COMBAT_FEAT(ch, i, j) (SET_BIT_AR(LEVELUP(ch)->combat_feats[(i)], (j)))
#define HAS_LEVELUP_SCHOOL_FEAT(ch, i, j) (IS_SET(LEVELUP(ch)->school_feats[(i)], (1 << (j))))
#define SET_LEVELUP_SCHOOL_FEAT(ch, i, j) (SET_BIT(LEVELUP(ch)->school_feats[(i)], (1 << (j))))
#define HAS_LEVELUP_SKILL_FEAT(ch, i, j) (LEVELUP(ch)->skill_focus[i][j])
#define SET_LEVELUP_SKILL_FEAT(ch, i, j) (LEVELUP(ch)->skill_focus[i][j] = TRUE)
/*#define SET_LEVELUP_SKILL_FEAT(ch,i,j)  ((LEVELUP(ch)->skill_focus[i][j]) ? \
                LEVELUP(ch)->skill_focus[i][j] = FALSE : \
                LEVELUP(ch)->skill_focus[i][j] = TRUE)*/

#define GET_LEVELUP_FEAT_POINTS(ch) (LEVELUP(ch)->feat_points)
#define GET_LEVELUP_EPIC_FEAT_POINTS(ch) (LEVELUP(ch)->epic_feat_points)
#define GET_LEVELUP_CLASS_FEATS(ch) (LEVELUP(ch)->class_feat_points)
#define GET_LEVELUP_TEAMWORK_FEATS(ch) (LEVELUP(ch)->teamwork_feat_points)
#define GET_LEVELUP_EPIC_CLASS_FEATS(ch) (LEVELUP(ch)->epic_class_feat_points)
#define GET_LEVELUP_SKILL_POINTS(ch) (LEVELUP(ch)->trains)
#define GET_LEVELUP_BOOSTS(ch) (LEVELUP(ch)->num_boosts)
#define GET_LEVELUP_BOOST_STATS(ch, stat) (LEVELUP(ch)->boosts[stat])
#define GET_LEVELUP_SKILL(ch, skill_num) (LEVELUP(ch)->skills[skill_num])
#define GET_LEVELUP_ABILITY(ch, skill_num) (GET_ABILITY(ch, skill_num) + GET_LEVELUP_SKILL(ch, skill_num))

/* MACRO to get a weapon's type. */
#define GET_WEAPON_TYPE(obj) ((GET_OBJ_TYPE(obj) == ITEM_WEAPON) || (GET_OBJ_TYPE(obj) == ITEM_FIREWEAPON) ? GET_OBJ_VAL(obj, 0) : 0)
#define IS_LIGHT_WEAPON_TYPE(type) (IS_SET(weapon_list[type].weaponFlags, WEAPON_FLAG_LIGHT))
#define HAS_WEAPON_FLAG(obj, flag) ((GET_OBJ_TYPE(obj) == ITEM_WEAPON) || (GET_OBJ_TYPE(obj) == ITEM_FIREWEAPON) ? IS_SET(weapon_list[GET_WEAPON_TYPE(obj)].weaponFlags, flag) : 0)
#define HAS_DAMAGE_TYPE(obj, flag)  (GET_OBJ_TYPE(obj) == ITEM_WEAPON) || (GET_OBJ_TYPE(obj) == ITEM_FIREWEAPON) ? IS_SET(weapon_list[GET_WEAPON_TYPE(obj)].damageTypes, flag) : 0)
#define GET_ENHANCEMENT_BONUS(obj) (((GET_OBJ_TYPE(obj) == ITEM_WEAPON) || (GET_OBJ_TYPE(obj) == ITEM_FIREWEAPON) || (GET_OBJ_TYPE(obj) == ITEM_ARMOR) || (GET_OBJ_TYPE(obj) == ITEM_MISSILE)) ? GET_OBJ_VAL(obj, 4) : 0)
#define IS_WEAPON_SHARP(obj) (GET_OBJ_TYPE(obj) == ITEM_WEAPON &&                                       \
                              (weapon_list[GET_WEAPON_TYPE(obj)].damageTypes == DAMAGE_TYPE_PIERCING || \
                               weapon_list[GET_WEAPON_TYPE(obj)].damageTypes == DAMAGE_TYPE_SLASHING))
#define GET_HOLY_WEAPON_TYPE(ch) (ch->player_specials->saved.holy_weapon_type)

/* armor related macro's */
#define GET_ARMOR_TYPE(obj) ((GET_OBJ_TYPE(obj) == ITEM_ARMOR) ? GET_OBJ_VAL(obj, 1) : SPEC_ARMOR_TYPE_UNDEFINED)
#define GET_ARMOR_TYPE_PROF(obj) ((GET_OBJ_TYPE(obj) == ITEM_ARMOR) ? armor_list[GET_OBJ_VAL(obj, 1)].armorType : ARMOR_TYPE_NONE)

#define GET_OUTFIT_OBJ(ch) (ch->player_specials->outfit_obj)
#define GET_OUTFIT_DESC(ch) (ch->player_specials->outfit_desc)
#define GET_OUTFIT_TYPE(ch) (ch->player_specials->outfit_type)
#define GET_OUTFIT_CONFIRM(ch) (ch->player_specials->outfit_confirmation)

#define IS_SHIELD(type) (type == SPEC_ARMOR_TYPE_BUCKLER || type == SPEC_ARMOR_TYPE_SMALL_SHIELD || \
                         type == SPEC_ARMOR_TYPE_LARGE_SHIELD || type == SPEC_ARMOR_TYPE_TOWER_SHIELD)

/* MACROS for the study system */
#define CAN_STUDY_FEATS(ch) ((((GET_LEVELUP_FEAT_POINTS(ch) +                  \
                                          GET_LEVELUP_CLASS_FEATS(ch) +      \
                                          GET_LEVELUP_TEAMWORK_FEATS(ch) +   \
                                          GET_LEVELUP_EPIC_FEAT_POINTS(ch) + \
                                          GET_LEVELUP_EPIC_CLASS_FEATS(ch)) > \
                                      0)                                      \
                                  ? 1                                        \
                                  : 0))
#define CAN_STUDY_SKILLS(ch) (GET_LEVELUP_SKILL_POINTS(ch))
#define CAN_STUDY_BOOSTS(ch) (GET_LEVELUP_BOOSTS(ch) > 0)
#define HAS_SET_STATS_STUDY(ch) (ch->player_specials->saved.have_stats_been_set_study)

#define CAN_SET_STATS(ch) (GET_LEVEL(ch) <= 1)

#define CAN_SET_DOMAIN(ch) (CLASS_LEVEL(ch, CLASS_CLERIC) == 1 || CLASS_LEVEL(ch, CLASS_INQUISITOR) == 1)
#define CAN_SET_SCHOOL(ch) (CLASS_LEVEL(ch, CLASS_WIZARD) == 1)
#define CAN_SET_S_BLOODLINE(ch) (CLASS_LEVEL(ch, CLASS_SORCERER) >= 1 && GET_SORC_BLOODLINE(ch) == 0 && get_levelup_sorcerer_bloodline_type(ch) == 0)
#define CAN_STUDY_CLASS_FEATS(ch) (CAN_STUDY_FEATS(ch) || (GET_LEVELUP_CLASS_FEATS(ch) +                  \
                                                                       GET_LEVELUP_EPIC_CLASS_FEATS(ch) > \
                                                                   0                                      \
                                                               ? 1                                        \
                                                               : 0))

#define CAN_STUDY_KNOWN_SPELLS(ch) (can_study_known_spells(ch))
#define CAN_STUDY_KNOWN_PSIONICS(ch) (can_study_known_psionics(ch))

#define CAN_STUDY_FAMILIAR(ch) (HAS_FEAT(ch, FEAT_SUMMON_FAMILIAR) ? 1 : 0)
#define CAN_STUDY_COMPANION(ch) (HAS_FEAT(ch, FEAT_ANIMAL_COMPANION) ? 1 : 0)
#define CAN_STUDY_FAVORED_ENEMY(ch) (HAS_FEAT(ch, FEAT_FAVORED_ENEMY_AVAILABLE) ? 1 : 0)
/* study - setting preferred caster class, for prestige classes such as arcane archer */
#define CAN_SET_P_CASTER(ch) (1)
//#define CAN_SET_P_DIVINE(ch)  (CLASS_LEVEL(ch, CLASS_CLERIC) || CLASS_LEVEL(ch, CLASS_DRUID))
#define GET_PREFERRED_ARCANE(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.preferred_arcane))
#define GET_PREFERRED_DIVINE(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.preferred_divine))
#define BONUS_CASTER_LEVEL(ch, class) (compute_bonus_caster_level(ch, class))

// Eldritch knight spell critical ability
#define HAS_ELDRITCH_SPELL_CRIT(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->has_eldritch_knight_spell_critical))
// Eldritch knight levels are added to warrior levels when determining qualification for certain warrior-only feats
#define WARRIOR_LEVELS(ch) (CLASS_LEVEL(ch, CLASS_WARRIOR) + CLASS_LEVEL(ch, CLASS_ELDRITCH_KNIGHT) + (CLASS_LEVEL(ch, CLASS_SPELLSWORD) / 2) + \
                            CLASS_LEVEL(ch, CLASS_KNIGHT_OF_THE_CROWN) + CLASS_LEVEL(ch, CLASS_KNIGHT_OF_THE_LILY))

/* Attacks of Opportunity (AOO) */
#define GET_TOTAL_AOO(ch) (ch->char_specials.attacks_of_opportunity)

/** The player's default sector type when buildwalking */
#define GET_BUILDWALK_SECTOR(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->buildwalk_sector))
#define GET_BUILDWALK_FLAGS(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->buildwalk_flags))
#define GET_BUILDWALK_NAME(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->buildwalk_name))
#define GET_BUILDWALK_DESC(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->buildwalk_desc))

/** Get obj worn in position i on ch. */
#define GET_EQ(ch, i) ((ch)->equipment[i])

/* Free hand, means you can't have a shield, 2h weapon or dual wielding */
#define HAS_FREE_HAND(ch) (!GET_EQ(ch, WEAR_SHIELD) && !GET_EQ(ch, WEAR_WIELD_2H) && !GET_EQ(ch, WEAR_WIELD_OFFHAND))

/* ranged-combat:  missiles */
#define MISSILE_ID(obj) ((obj)->missile_id)

// weapon spells
#define HAS_SPELLS(obj) ((obj)->has_spells)
#define GET_WEAPON_SPELL(obj, i) ((obj)->wpn_spells[i].spellnum)
#define GET_WEAPON_SPELL_LVL(obj, i) ((obj)->wpn_spells[i].level ? (obj)->wpn_spells[i].level : (LVL_IMMORT / 2))
#define GET_WEAPON_SPELL_PCT(obj, i) ((obj)->wpn_spells[i].percent)
#define GET_WEAPON_SPELL_AGG(obj, i) ((obj)->wpn_spells[i].inCombat)

// weapon channel spells for spellsword class
#define GET_WEAPON_CHANNEL_SPELL(obj, i) ((obj)->channel_spells[i].spellnum)
#define GET_WEAPON_CHANNEL_SPELL_LVL(obj, i) ((obj)->channel_spells[i].level ? (obj)->channel_spells[i].level : (LVL_IMMORT / 2))
#define GET_WEAPON_CHANNEL_SPELL_PCT(obj, i) ((obj)->channel_spells[i].percent)
#define GET_WEAPON_CHANNEL_SPELL_AGG(obj, i) ((obj)->channel_spells[i].inCombat)
#define GET_WEAPON_CHANNEL_SPELL_USES(obj, i) ((obj)->channel_spells[i].uses_left)

/** If ch is a mob, return the special function, else return NULL. */
#define GET_MOB_SPEC(ch) (IS_MOB(ch) ? mob_index[(ch)->nr].func : NULL)

/** Get the real number of the mob instance. */
#define GET_MOB_RNUM(mob) ((mob)->nr)

/** If mob is a mob, return the virtual number of it. */
#define GET_MOB_VNUM(mob) (IS_MOB(mob) ? mob_index[GET_MOB_RNUM(mob)].vnum : NOBODY)

#define MOB_KNOWS_SPELL(mob, spellnum)  ((mob)->mob_specials.spells_known[spellnum])

/** Return the default position of ch. */
#define GET_DEFAULT_POS(ch) ((ch)->mob_specials.default_pos)
/** Return the memory of ch. */
#define MEMORY(ch) ((ch)->mob_specials.memory)
/** Return the anger/frustration level of ch */
#define GET_ANGER(ch) ((ch)->mob_specials.frustration_level)
/** Subrace Types of MOB **/
#define GET_SUBRACE(ch, i) ((ch)->mob_specials.subrace[i])
/** MOB number of damage dice for attacks **/
#define GET_DAMNODICE(ch) ((ch)->mob_specials.damnodice)
/** MOB size of damage dice for attacks **/
#define GET_DAMSIZEDICE(ch) ((ch)->mob_specials.damsizedice)
/** MOB attack type (slash, bite, etc) **/
#define GET_ATTACK_TYPE(ch) ((ch)->mob_specials.attack_type)

/* mobile special data for echo */
#define ECHO_IS_ZONE(mob) ((mob)->mob_specials.echo_is_zone)
#define ECHO_FREQ(mob) ((mob)->mob_specials.echo_frequency)
#define ECHO_COUNT(mob) ((mob)->mob_specials.echo_count)
#define ECHO_ENTRIES(mob) ((mob)->mob_specials.echo_entries)
#define ECHO_SEQUENTIAL(mob) ((mob)->mob_specials.echo_sequential)
#define CURRENT_ECHO(mob) ((mob)->mob_specials.current_echo)

/* path utilities for mobiles (patrols) */
#define PATH_INDEX(mob) ((mob)->mob_specials.path_index)
#define PATH_DELAY(mob) ((mob)->mob_specials.path_delay)
#define PATH_RESET(mob) ((mob)->mob_specials.path_reset)
#define GET_PATH(mob, x) ((mob)->mob_specials.path[x])
#define PATH_SIZE(mob) ((mob)->mob_specials.path_size)

/* mobile load room */
#define GET_MOB_LOADROOM(ch) ((ch)->mob_specials.loadroom)

/* a (generally) boolean macro that marks whether a proc fired, general use is
 for zone-procs */
#define PROC_FIRED(ch) ((ch)->mob_specials.proc_fired)

/**********************************************/
/*** functions / definese for handling pets ***/
#define NPC_MODE_DISPLAY 0
#define NPC_MODE_FLAG 1
#define NPC_MODE_SPECIFIC 2
#define NPC_MODE_COUNT 3
#define NPC_MODE_SPARE 4
#define IS_PET(ch) (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && ch->master)
int check_npc_followers(struct char_data *ch, int mode, int variable);
/**********************************************/

/** Has Subrace will check the (3) arrays if subrace is there **/
#define HAS_SUBRACE(ch, i) (GET_SUBRACE(ch, 0) == i || \
                            GET_SUBRACE(ch, 1) == i || \
                            GET_SUBRACE(ch, 2) == i)

/** Return the equivalent strength of ch if ch has level 18 strength. */
#define STRENGTH_APPLY_INDEX(ch)                                                        \
  (((GET_ADD(ch) == 0) || (GET_STR(ch) != 18)) ? GET_STR(ch) : (GET_ADD(ch) <= 50) ? 26 \
                                                                                   : ((GET_ADD(ch) <= 75) ? 27 : ((GET_ADD(ch) <= 90) ? 28 : ((GET_ADD(ch) <= 99) ? 29 : 30))))

// returns effectve strength score for determining max carry weight
#define GET_CARRY_STRENGTH(ch)  (GET_STR(ch) + get_encumbrance_mod(ch) + (HAS_FEAT(ch, FEAT_ENCUMBERED_RESILIENCE) ? 2 : 0))

/** Return how much weight ch can carry. */
#define CAN_CARRY_W(ch) (can_carry_weight_limit(ch))

/** Return how many items ch can carry.
 *  Increased this by 5 - Ornir */
#if defined(CAMPAIGN_DL)
#define CAN_CARRY_N(ch) (1000)
#else
#define CAN_CARRY_N(ch) (20 + (GET_DEX(ch) >> 1) + (GET_LEVEL(ch) >> 1))
#endif

/** Return whether or not ch is awake. */
#define AWAKE(ch) (GET_POS(ch) > POS_SLEEPING)

// Mount
#define RIDING(ch) ((ch)->char_specials.riding)
#define RIDDEN_BY(ch) ((ch)->char_specials.ridden_by)

#define IS_HOLY(room) (ROOM_AFFECTED(room, RAFF_HOLY) && !ROOM_AFFECTED(room, RAFF_UNHOLY))
#define IS_UNHOLY(room) (ROOM_AFFECTED(room, RAFF_UNHOLY) && !ROOM_AFFECTED(room, RAFF_HOLY))

/* dnd type of alignments */
#define IS_LG(ch) (GET_ALIGNMENT(ch) >= 800)
#define IS_NG(ch) (GET_ALIGNMENT(ch) >= 575 && GET_ALIGNMENT(ch) < 800)
#define IS_CG(ch) (GET_ALIGNMENT(ch) >= 350 && GET_ALIGNMENT(ch) < 575)
#define IS_LN(ch) (GET_ALIGNMENT(ch) >= 125 && GET_ALIGNMENT(ch) < 350)
#define IS_TN(ch) (GET_ALIGNMENT(ch) < 125 && GET_ALIGNMENT(ch) > -125)
#define IS_CN(ch) (GET_ALIGNMENT(ch) <= -125 && GET_ALIGNMENT(ch) > -350)
#define IS_LE(ch) (GET_ALIGNMENT(ch) <= -350 && GET_ALIGNMENT(ch) > -575)
#define IS_NE(ch) (GET_ALIGNMENT(ch) <= -575 && GET_ALIGNMENT(ch) > -800)
#define IS_CE(ch) (GET_ALIGNMENT(ch) <= -800)

#define ETHOS_LAWFUL 1000
#define ETHOS_NEUTRAL 0
#define ETHOS_CHAOTIC -1000
#define ALIGNMENT_GOOD 1000
#define ALIGNMENT_NEUTRAL 0
#define ALIGNMENT_EVIL -1000

#define GET_ALIGN_ABBREV(e, a) (e > 250 ? (a > 250 ? "LG" : (a < -250 ? "LE" : "LN")) : (e < -250 ? (a > 250 ? "CG" : (a < -250 ? "CE" : "CN")) : ((a > 250 ? "NG" : (a < -250 ? "NE" : "TN")))))

#define GET_ALIGN_STRING(e, a) (e > 250 ? (a > 250 ? "Lawful Good" : (a < -250 ? "Lawful Evil" : "Lawful Neutral")) : (e < -250 ? (a > 250 ? "Chaotic Good" : (a < -250 ? "Chaotic Evil" : "Chaotic Neutral")) : ((a > 250 ? "Neutral Good" : (a < -250 ? "Neutral Evil" : "True Neutral")))))

#define GET_DEITY(ch) (ch->player_specials->saved.deity)
#define FIXED_BAB(ch) (ch->player_specials->saved.fixed_bab)
int NUM_ATTACKS_BAB(struct char_data *ch);
int BAB_NEW(struct char_data *ch);
int ACTUAL_BAB(struct char_data *ch);

/** Defines if ch is good. */
//#define IS_GOOD(ch) (GET_ALIGNMENT(ch) >= 350) // old system
#define IS_GOOD(ch) (IS_LG(ch) || IS_NG(ch) || IS_CG(ch))

/** Defines if ch is evil. */
//#define IS_EVIL(ch) (GET_ALIGNMENT(ch) <= -350) // old system
#define IS_EVIL(ch) (IS_LE(ch) || IS_NE(ch) || IS_CE(ch))

/** Defines if ch is neither good nor evil. */
// #define IS_NEUTRAL(ch) (!IS_GOOD(ch) && !IS_EVIL(ch)) // old system
#define IS_NEUTRAL(ch) (IS_LN(ch) || IS_TN(ch) || IS_CN(ch))
#define IS_NEUTRAL_ETHOS(ch) (IS_NG(ch) || IS_TN(ch) || IS_NE(ch))
#define IS_NEUTRAL_ANY(ch) (IS_NEUTRAL(ch) || IS_NEUTRAL_ETHOS(ch))

#define IS_LAWFUL(ch) (IS_LG(ch) || IS_LN(ch) || IS_LE(ch))
#define IS_CHAOTIC(ch) (IS_CG(ch) || IS_CN(ch) || IS_CE(ch))

/** Old wait state function.
 * @deprecated Use GET_WAIT_STATE */
#define WAIT_STATE(ch, cycle)     \
  do                              \
  {                               \
    GET_WAIT_STATE(ch) = (cycle); \
  } while (0)

/** Old check wait.
 * @deprecated Use GET_WAIT_STATE */
#define CHECK_WAIT(ch) ((ch)->wait > 0)

/** Old mob wait check.
 * @deprecated Use GET_WAIT_STATE */
#define GET_MOB_WAIT(ch) GET_WAIT_STATE(ch)

/** Use this macro to check the wait state of ch. */
#define GET_WAIT_STATE(ch) ((ch)->wait)

/* Descriptor-based utils. */
/** Connected state of d. */
#define STATE(d) ((d)->connected)

/** Defines whether d is using an OLC or not. */
#define IS_IN_OLC(d) (((STATE(d) >= FIRST_OLC_STATE) && (STATE(d) <= LAST_OLC_STATE)) || STATE(d) == CON_IEDIT)

/** Defines whether d is playing or not. */
#define IS_PLAYING(d) (IS_IN_OLC(d) || STATE(d) == CON_PLAYING)

/** Defines if it is ok to send a message to ch. */
#define SENDOK(ch) (((ch)->desc || SCRIPT_CHECK((ch), MTRIG_ACT)) && \
                    (to_sleeping || AWAKE(ch)) &&                    \
                    !PLR_FLAGGED((ch), PLR_WRITING))

/*#define SENDOK(ch)    (((ch)->desc || SCRIPT_CHECK((ch), MTRIG_ACT)) && \
                      (to_sleeping || AWAKE(ch)))
 */
/* deaf flag maybe isn't a good idea to have here */
/*
#define SENDOK(ch)	(((ch)->desc || SCRIPT_CHECK((ch), MTRIG_ACT)) && \
               (to_sleeping || AWAKE(ch)) && \
               !PLR_FLAGGED((ch), PLR_WRITING) && !AFF_FLAGGED(ch, AFF_DEAF))
 */

/* object utils */
/** Check for NOWHERE or the top array index? If using unsigned types, the top
 * array index will catch everything. If using signed types, NOTHING will
 * catch the majority of bad accesses. */
#define VALID_OBJ_RNUM(obj) (GET_OBJ_RNUM(obj) <= top_of_objt && \
                             GET_OBJ_RNUM(obj) != NOTHING)

/** from homeland, used for object specs/procs **/
#define GET_OBJ_SPECTIMER(obj, val) ((obj)->obj_flags.spec_timer[(val)])

/** Level of obj. */
#define GET_OBJ_LEVEL(obj) ((obj)->obj_flags.level)

/** Permanent affects on obj. */
#define GET_OBJ_PERM(obj) ((obj)->obj_flags.bitvector)

/** Object material of object **/
#define GET_OBJ_MATERIAL(obj) ((obj)->obj_flags.material)

/** Type of obj. */
#define GET_OBJ_TYPE(obj) ((obj)->obj_flags.type_flag)

// proficiency of item
#define GET_OBJ_PROF(obj) ((obj)->obj_flags.prof_flag)

/** Cost of obj. */
#define GET_OBJ_COST(obj) ((obj)->obj_flags.cost)

/** Cost per day to rent obj, if rent is turned on. */
#define GET_OBJ_RENT(obj) ((obj)->obj_flags.cost_per_day)

/** Affect flags on obj. */
#define GET_OBJ_AFFECT(obj) ((obj)->obj_flags.bitvector)

/** Extra flags bit array on obj. */
#define GET_OBJ_EXTRA(obj) ((obj)->obj_flags.extra_flags)

/** Extra flags field bit array field i on obj. */
#define GET_OBJ_EXTRA_AR(obj, i) ((obj)->obj_flags.extra_flags[(i)])

/** Wear flags on obj. */
#define GET_OBJ_WEAR(obj) ((obj)->obj_flags.wear_flags)

/** Return value val for obj. */
#define GET_OBJ_VAL(obj, val) ((obj)->obj_flags.value[(val)])

/** Object size */
#define GET_OBJ_SIZE(obj) ((obj)->obj_flags.size)

/** Object (spellbook) # of pages */
#define GET_OBJ_PAGES(obj) ((obj)->obj_flags.spellbook_pages)
/** Object (spellbook) spellnum at given location */
#define GET_OBJ_SB_NUM(obj, loc) ((obj)->obj_flags.spellbook_spellnum[loc])

/** Weight of obj. */
#define GET_OBJ_WEIGHT(obj) ((obj)->obj_flags.weight)

/** Current timer of obj. */
#define GET_OBJ_TIMER(obj) ((obj)->obj_flags.timer)

/** Real number of obj instance. */
#define GET_OBJ_RNUM(obj) ((obj)->item_number)

/** Virtual number of obj, or NOTHING if not a real obj. */
#define GET_OBJ_VNUM(obj) (VALID_OBJ_RNUM(obj) ? obj_index[GET_OBJ_RNUM(obj)].vnum : NOTHING)

/** Special function attached to obj, or NULL if nothing attached. */
#define GET_OBJ_SPEC(obj) (VALID_OBJ_RNUM(obj) ? obj_index[GET_OBJ_RNUM(obj)].func : NULL)

/* bound objects - only usable by a designated player */
#define GET_OBJ_BOUND_ID(obj) ((obj)->obj_flags.bound_id)

/* i_sort determines how it is sorted in inventory */
#define GET_OBJ_SORT(obj) ((obj)->obj_flags.i_sort)
#define GET_BAG_NAME(ch, bagnum)  (ch->player_specials->saved.bag_names[bagnum])

/** Defines if an obj is a corpse. */
#define IS_CORPSE(obj) (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && \
                        GET_OBJ_VAL((obj), 3) == 1)

/** Defines if an obj is a corpse. */
#define IS_DECAYING_PORTAL(obj) (GET_OBJ_TYPE(obj) == ITEM_PORTAL && \
                                 OBJ_FLAGGED(obj, ITEM_DECAY))

/** Can the obj be worn on body part? */
#define CAN_WEAR(obj, part) OBJWEAR_FLAGGED((obj), (part))

/** Return short description of obj. */
#define GET_OBJ_SHORT(obj) ((obj)->short_description)

/* Compound utilities and other macros. */
/** Used to compute version. To see if the code running is newer than 3.0pl13,
 * you would use: if _LUMINARIMUD > LUMINARIMUD_VERSION(3,0,13) */
#define LUMINARIMUD_VERSION(major, minor, patchlevel) \
  (((major) << 16) + ((minor) << 8) + (patchlevel))

/** Figures out possessive pronoun for ch. */
#define HSHR(ch) (GET_SEX(ch) ? (GET_SEX(ch) == SEX_MALE ? "his" : "her") : "its")

/** Figures out third person, singular pronoun for ch. */
#define HSSH(ch) (GET_SEX(ch) ? (GET_SEX(ch) == SEX_MALE ? "he" : "she") : "it")

/** Figures out third person, objective pronoun for ch. */
#define HMHR(ch) (GET_SEX(ch) ? (GET_SEX(ch) == SEX_MALE ? "him" : "her") : "it")

/** "An" or "A" for object (uppercased) */
#define ANA(obj) (strchr("aeiouAEIOU", *(obj)->name) ? "An" : "A")

/** "an" or "a" for object (lowercased) */
#define SANA(obj) (strchr("aeiouAEIOU", *(obj)->name) ? "an" : "a")

/** "an" or "a" for text (lowercased) */
#define TANA(obj) (strchr("aeiouAEIOU", *(obj)) ? "an" : "a")

#define ULTRA_BLIND(ch, room) (ultra_blind(ch, room))

// moved this here for connection between vision macros -zusuk
#define CAN_SEE_IN_DARK(ch) \
  (char_has_ultra(ch) || has_blindsense(ch) || (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_HOLYLIGHT)) || \
  (char_has_infra(ch) && OUTSIDE(ch)))
#define CAN_INFRA_IN_DARK(ch) \
  (char_has_infra(ch) || (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_HOLYLIGHT)))

/* Various macros building up to CAN_SEE */
/** Defines if ch can see in general in the dark. */

/** Defines if there is enough light for sub to see in. */
#define LIGHT_OK(sub) ((!AFF_FLAGGED(sub, AFF_BLIND) || has_blindsense(sub)) && \
                       (IS_LIGHT(IN_ROOM(sub)) || CAN_SEE_IN_DARK(sub) ||                  \
                        GET_LEVEL(sub) >= LVL_IMMORT))
#define INFRA_OK(sub) (!AFF_FLAGGED(sub, AFF_BLIND) &&                      \
                       (IS_LIGHT(IN_ROOM(sub)) || CAN_INFRA_IN_DARK(sub) || \
                        GET_LEVEL(sub) >= LVL_IMMORT))

/** Defines if sub character can see the invisible obj character.
 *  returns FALSE if sub cannot see obj
 *  returns TRUE if sub can see obj
 */
#define INVIS_OK(sub, obj)                                                       \
  ((!AFF_FLAGGED((obj), AFF_INVISIBLE) || (AFF_FLAGGED(sub, AFF_DETECT_INVIS) || \
                                           AFF_FLAGGED(sub, AFF_TRUE_SIGHT) ||   \
                                           HAS_FEAT(sub, FEAT_TRUE_SIGHT))) &&   \
                                           (can_see_hidden(sub, obj)))

/** Defines if sub character can see obj character, assuming mortal only
 * settings. */
#define MORT_CAN_SEE(sub, obj) (LIGHT_OK(sub) && INVIS_OK(sub, obj))
#define MORT_CAN_INFRA(sub, obj) (INFRA_OK(sub) && INVIS_OK(sub, obj))

/** Defines if sub character can see obj character, assuming immortal
 * and mortal settings. */
#define IMM_CAN_SEE(sub, obj) \
  (MORT_CAN_SEE(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED(sub, PRF_HOLYLIGHT)))
#define IMM_CAN_INFRA(sub, obj) \
  (MORT_CAN_INFRA(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED(sub, PRF_HOLYLIGHT)))

/** Is obj character the same as sub character? */
#define SELF(sub, obj) ((sub) == (obj))

/** Can sub character see obj character? */
#define CAN_SEE(sub, obj) (SELF(sub, obj) ||                                                       \
                           ((GET_REAL_LEVEL(sub) >= (IS_NPC(obj) ? FALSE : GET_INVIS_LEV(obj))) && \
                            IMM_CAN_SEE(sub, obj)))
#define CAN_INFRA(sub, obj) (SELF(sub, obj) ||                                                       \
                             ((GET_REAL_LEVEL(sub) >= (IS_NPC(obj) ? FALSE : GET_INVIS_LEV(obj))) && \
                              IMM_CAN_INFRA(sub, obj)))

/* End of CAN_SEE */

/* Object vision handling */

/** Can the sub character see the obj if it is invisible? */
#define INVIS_OK_OBJ(sub, obj) \
  (!OBJ_FLAGGED((obj), ITEM_INVISIBLE) || AFF_FLAGGED((sub), AFF_DETECT_INVIS) || AFF_FLAGGED((sub), AFF_TRUE_SIGHT))

/** Is anyone carrying this object and if so, are they visible? */
#define CAN_SEE_OBJ_CARRIER(sub, obj)                     \
  ((!obj->carried_by || CAN_SEE(sub, obj->carried_by)) && \
   (!obj->worn_by || CAN_SEE(sub, obj->worn_by)))
#define CAN_INFRA_OBJ_CARRIER(sub, obj)                     \
  ((!obj->carried_by || CAN_INFRA(sub, obj->carried_by)) && \
   (!obj->worn_by || CAN_INFRA(sub, obj->worn_by)))
#define IS_TREASURE_CHEST_HIDDEN(obj)  (GET_OBJ_TYPE(obj) == ITEM_TREASURE_CHEST && GET_OBJ_VAL(obj, 3) > 0)

/** Can sub character see the obj, using mortal only checks? */
#define MORT_CAN_SEE_OBJ(sub, obj) \
  (LIGHT_OK(sub) && INVIS_OK_OBJ(sub, obj) && CAN_SEE_OBJ_CARRIER(sub, obj) && !IS_TREASURE_CHEST_HIDDEN(obj))
#define MORT_CAN_INFRA_OBJ(sub, obj) \
  (INFRA_OK(sub) && INVIS_OK_OBJ(sub, obj) && CAN_INFRA_OBJ_CARRIER(sub, obj))

/** Can sub character see the obj, using mortal and immortal checks? */
#define CAN_SEE_OBJ(sub, obj)                                                           \
  (MORT_CAN_SEE_OBJ(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED((sub), PRF_HOLYLIGHT)) || \
   (!AFF_FLAGGED(sub, AFF_BLIND) && OBJ_FLAGGED(obj, ITEM_GLOW)))
#define CAN_INFRA_OBJ(sub, obj) \
  (MORT_CAN_INFRA_OBJ(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED((sub), PRF_HOLYLIGHT)))

/** Can ch carry obj? */
#define CAN_CARRY_OBJ(ch, obj)                                       \
  (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) <= CAN_CARRY_W(ch)) && \
   ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))

/** Can ch pick up obj? */
#define CAN_GET_OBJ(ch, obj)                                        \
  (CAN_WEAR((obj), ITEM_WEAR_TAKE) && CAN_CARRY_OBJ((ch), (obj)) && \
   CAN_SEE_OBJ((ch), (obj)))

/** If vict can see ch, return ch name, else return "someone". */
#define PERS(ch, vict)                                                    \
  (!CAN_SEE(vict, ch) ? "someone" : !GET_DISGUISE_RACE(ch) ? GET_NAME(ch) \
                                                           : race_list[GET_DISGUISE_RACE(ch)].name)

/** If vict can see obj, return obj short description, else return
 * "something". */
#define OBJS(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? (obj)->short_description : "something")

/** If vict can see obj, return obj name, else return "something". */
#define OBJN(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? fname((obj)->name) : "something")

/** Does direction door exist in the same room as ch? */
#define EXIT(ch, door) (world[IN_ROOM(ch)].dir_option[door])

/** Does direction door exist in the same room as obj? */
#define EXIT_OBJ(obj, door) (world[obj->in_room].dir_option[door])

/** Does room number have direction num? */
#define W_EXIT(room, num) (world[(room)].dir_option[(num)])

/** Does room pointer have direction option num? */
#define R_EXIT(room, num) ((room)->dir_option[(num)])

#define _2ND_EXIT(ch, door) (world[EXIT(ch, door)->to_room].dir_option[door])
#define _3RD_EXIT(ch, door) (world[_2ND_EXIT(ch, door)->to_room].dir_option[door])

/** Can ch walk through direction door. */
#define CAN_GO(ch, door) (EXIT(ch, door) &&                       \
                          (EXIT(ch, door)->to_room != NOWHERE) && \
                          !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))

/** True total number of directions available to move in. */
#ifdef CAMPAIGN_FR
#define DIR_COUNT ((CONFIG_DIAGONAL_DIRS) ? 12 : 6)
#else
#define DIR_COUNT ((CONFIG_DIAGONAL_DIRS) ? 10 : 6)
#endif

/* Returns TRUE if the direction is a diagonal one */
#define IS_DIAGONAL(dir) (((dir) == NORTHWEST) || ((dir) == NORTHEAST) || \
                          ((dir) == SOUTHEAST) || ((dir) == SOUTHWEST))

/* is this room an "arena" room? 138600-138699 */
#define ARENA_VNUM_START 138600
#define ARENA_VNUM_END 138699
#define CONFIG_ARENA_DEATH 138610
#define IS_ARENA(rnum) ((world[rnum].number >= ARENA_VNUM_START) && \
                        (world[rnum].number <= ARENA_VNUM_END))
#define IN_ARENA(ch) (IS_ARENA(IN_ROOM(ch)))

/* handy macros for dealing with class_list[] */
#define CLSLIST_NAME(classnum) (class_list[classnum].name)
#define CLSLIST_ABBRV(classnum) (class_list[classnum].abbrev)
#define CLSLIST_CLRABBRV(classnum) (class_list[classnum].colored_abbrev)
#define CLSLIST_MENU(classnum) (class_list[classnum].menu_name)
#define CLSLIST_MAXLVL(classnum) ((class_list[classnum].max_level == -1) ? (LVL_IMMORT - 1) : (class_list[classnum].max_level))
#define CLSLIST_LOCK(classnum) (class_list[classnum].locked_class)
#define CLSLIST_PRESTIGE(classnum) (class_list[classnum].prestige_class)
#define CLSLIST_BAB(classnum) (class_list[classnum].base_attack_bonus)
#define CLSLIST_HPS(classnum) (class_list[classnum].hit_dice)
#define CLSLIST_PSP(classnum) (class_list[classnum].psp_gain)
#define CLSLIST_MVS(classnum) (class_list[classnum].move_gain)
#define CLSLIST_TRAINS(classnum) (class_list[classnum].trains_gain)
#define CLSLIST_INGAME(classnum) (class_list[classnum].in_game)
#define CLSLIST_COST(classnum) (class_list[classnum].unlock_cost)
#define CLSLIST_EFEATP(classnum) (class_list[classnum].epic_feat_progression)
#define CLSLIST_ATTRIBUTE(classnum) (class_list[classnum].primary_attribute)
#define CLSLIST_DESCRIP(classnum) (class_list[classnum].descrip)
#define CLSLIST_SAVES(classnum, savenum) (class_list[classnum].preferred_saves[savenum])
#define CLSLIST_ABIL(classnum, abilnum) (class_list[classnum].class_abil[abilnum])
#define CLSLIST_TITLE(classnum, titlenum) (class_list[classnum].titles[titlenum])

/* macros for dealing with sorcerer draconic bloodlines */
#define DRCHRTLIST_NAME(drac_heritage) (draconic_heritage_names[drac_heritage])
#define DRCHRT_ENERGY_TYPE(drac_heritage) (damtypes[draconic_heritage_energy_types[drac_heritage]])
/* macros for dealing with sorcerer arcane bloodlines */
#define NEW_ARCANA_SLOT(ch, i) CHECK_PLAYER_SPECIAL(ch, (ch->player_specials->saved.new_arcana_circles[i]))
#define APOTHEOSIS_SLOTS(ch) ((ch)->player_specials->arcane_apotheosis_slots)

/** Return the class abbreviation for ch. */
#define CLASS_ABBR(ch) (CLSLIST_ABBRV((int)GET_CLASS(ch)))
#define CLASS_LEVEL_ABBR(ch, class) (IS_NPC(ch) ? CLASS_ABBR(ch) : CLSLIST_ABBRV(class))

// quick macro to see if someone is an immortal or not - Bakarus
#define IS_IMMORTAL(ch) (GET_LEVEL(ch) > LVL_IMMORT)

// these have changed since multi-class, you are classified as CLASS_x if you
// got any levels in it - zusuk
//#define IS_PSION(ch)	     (CLASS_LEVEL(ch, CLASS_PSIONICIST))
//#define IS_PSY_WARR(ch)	     (CLASS_LEVEL(ch, CLASS_PSY_WARR))
//#define IS_SOULKNIFE(ch)	     (CLASS_LEVEL(ch, CLASS_SOULKNIFE))
//#define IS_WILDER(ch)	     (CLASS_LEVEL(ch, CLASS_WILDER))
//#define IS_PSI(ch) (IS_PSION(ch) || IS_PSY_WARR(ch) || IS_SOULKNIFE(ch) || IS_WILDER(ch))
#define IS_WIZARD(ch) (CLASS_LEVEL(ch, CLASS_WIZARD))
#define IS_SORCERER(ch) (CLASS_LEVEL(ch, CLASS_SORCERER))
#define IS_BARD(ch) (CLASS_LEVEL(ch, CLASS_BARD))
#define IS_CLERIC(ch) (CLASS_LEVEL(ch, CLASS_CLERIC))
#define IS_INQUISITOR(ch) (CLASS_LEVEL(ch, CLASS_INQUISITOR))
#define IS_DRUID(ch) (CLASS_LEVEL(ch, CLASS_DRUID))
#define IS_ROGUE(ch) (CLASS_LEVEL(ch, CLASS_ROGUE))
#define IS_ROGUE_TYPE(ch) (CLASS_LEVEL(ch, CLASS_ROGUE) + CLASS_LEVEL(ch, CLASS_DUELIST) + CLASS_LEVEL(ch, CLASS_SHADOW_DANCER) + CLASS_LEVEL(ch, CLASS_ASSASSIN) + \
                           CLASS_LEVEL(ch, CLASS_ARCANE_SHADOW) + CLASS_LEVEL(ch, CLASS_RANGER) + CLASS_LEVEL(ch, CLASS_BARD))
#define IS_PSI_TYPE(ch) (CLASS_LEVEL(ch, CLASS_PSIONICIST)) /* for expansion! */
#define IS_WARRIOR(ch) (CLASS_LEVEL(ch, CLASS_WARRIOR))
#define IS_WEAPONMASTER(ch) (CLASS_LEVEL(ch, CLASS_WEAPON_MASTER))
#define IS_STALWARTDEFENDER(ch) (CLASS_LEVEL(ch, CLASS_STALWART_DEFENDER))
#define IS_DUELIST(ch) (CLASS_LEVEL(ch, CLASS_DUELIST))
//#define IS_SHADOW_DANCER(ch)		(CLASS_LEVEL(ch, CLASS_SHADOW_DANCER))
//#define IS_ASSASSIN(ch)		(CLASS_LEVEL(ch, CLASS_ASSASSIN))
#define IS_MYSTICTHEURGE(ch) (CLASS_LEVEL(ch, CLASS_MYSTIC_THEURGE))
#define IS_ARCANE_ARCHER(ch) (CLASS_LEVEL(ch, CLASS_ARCANE_ARCHER))
#define IS_ARCANE_SHADOW(ch) (CLASS_LEVEL(ch, CLASS_ARCANE_SHADOW))
#define IS_NECROMANCER(ch) (CLASS_LEVEL(ch, CLASS_NECROMANCER))
#define IS_ELDRITCH_KNIGHT(ch) (CLASS_LEVEL(ch, CLASS_ELDRITCH_KNIGHT))
#define IS_SACRED_FIST(ch) (CLASS_LEVEL(ch, CLASS_SACRED_FIST))
#define IS_SHIFTER(ch) (CLASS_LEVEL(ch, CLASS_SHIFTER))
#define IS_MONK(ch) (CLASS_LEVEL(ch, CLASS_MONK))
#define MONK_TYPE(ch) (CLASS_LEVEL(ch, CLASS_MONK) + CLASS_LEVEL(ch, CLASS_SACRED_FIST))
#define IS_BERSERKER(ch) (CLASS_LEVEL(ch, CLASS_BERSERKER))
#define IS_PALADIN(ch) (CLASS_LEVEL(ch, CLASS_PALADIN))
#define IS_RANGER(ch) (CLASS_LEVEL(ch, CLASS_RANGER))
#define IS_ALCHEMIST(ch) (CLASS_LEVEL(ch, CLASS_ALCHEMIST))
#define IS_BLACKGUARD(ch) (CLASS_LEVEL(ch, CLASS_BLACKGUARD))
#define IS_SUMMONER(ch) (CLASS_LEVEL(ch, CLASS_SUMMONER))
#define IS_KNIGHT_OF_THE_CROWN(ch) (CLASS_LEVEL(ch, CLASS_KNIGHT_OF_THE_CROWN))
#define IS_KNIGHT_OF_THE_SWORD(ch) (CLASS_LEVEL(ch, CLASS_KNIGHT_OF_THE_SWORD))
#define IS_KNIGHT_OF_THE_ROSE(ch) (CLASS_LEVEL(ch, CLASS_KNIGHT_OF_THE_ROSE))
#define IS_KNIGHT_OF_THE_THORN(ch) (CLASS_LEVEL(ch, CLASS_KNIGHT_OF_THE_THORN))
#define IS_KNIGHT_OF_THE_SKULL(ch) (CLASS_LEVEL(ch, CLASS_KNIGHT_OF_THE_SKULL))
#define IS_KNIGHT_OF_THE_LILY(ch) (CLASS_LEVEL(ch, CLASS_KNIGHT_OF_THE_LILY))
#define IS_DRAGONRIDER(ch) (CLASS_LEVEL(ch, CLASS_DRAGONRIDER))

#define IS_CASTER(ch) (GET_LEVEL(ch) >= LVL_IMMORT ||                                                          \
                       IS_CLERIC(ch) || IS_WIZARD(ch) || IS_DRUID(ch) || IS_SORCERER(ch) || IS_PALADIN(ch) ||  \
                       IS_RANGER(ch) || IS_BARD(ch) || IS_ALCHEMIST(ch) || IS_ARCANE_ARCHER(ch) ||             \
                       IS_MYSTICTHEURGE(ch) || IS_ARCANE_SHADOW(ch) || IS_SACRED_FIST(ch) || IS_SHIFTER(ch) || \
                       IS_ELDRITCH_KNIGHT(ch) || IS_BLACKGUARD(ch) || IS_INQUISITOR(ch) || IS_SUMMONER(ch) || \
                       IS_NECROMANCER(ch) || IS_KNIGHT_OF_THE_SWORD(ch) || IS_KNIGHT_OF_THE_ROSE(ch) || \
                       IS_KNIGHT_OF_THE_THORN(ch) || IS_KNIGHT_OF_THE_SKULL(ch))

#define IS_FIGHTER(ch) (CLASS_LEVEL(ch, CLASS_WARRIOR) || CLASS_LEVEL(ch, CLASS_WEAPON_MASTER) ||     \
                        CLASS_LEVEL(ch, CLASS_STALWART_DEFENDER) || CLASS_LEVEL(ch, CLASS_DUELIST) || \
                        CLASS_LEVEL(ch, CLASS_BERSERKER) || CLASS_LEVEL(ch, CLASS_PALADIN) ||         \
                        CLASS_LEVEL(ch, CLASS_RANGER))

#define IS_NPC_CASTER(ch) (GET_CLASS(ch) == CLASS_CLERIC ||          \
                           GET_CLASS(ch) == CLASS_WIZARD ||          \
                           GET_CLASS(ch) == CLASS_DRUID ||           \
                           GET_CLASS(ch) == CLASS_SORCERER ||        \
                           GET_CLASS(ch) == CLASS_PALADIN ||         \
                           GET_CLASS(ch) == CLASS_RANGER ||          \
                           GET_CLASS(ch) == CLASS_ALCHEMIST ||       \
                           GET_CLASS(ch) == CLASS_MYSTIC_THEURGE ||  \
                           GET_CLASS(ch) == CLASS_ARCANE_ARCHER ||   \
                           GET_CLASS(ch) == CLASS_ARCANE_SHADOW ||   \
                           GET_CLASS(ch) == CLASS_NECROMANCER ||   \
                           GET_CLASS(ch) == CLASS_ELDRITCH_KNIGHT || \
                           GET_CLASS(ch) == CLASS_SACRED_FIST ||     \
                           GET_CLASS(ch) == CLASS_SHIFTER ||         \
                           GET_CLASS(ch) == CLASS_INQUISITOR ||      \
                           GET_CLASS(ch) == CLASS_KNIGHT_OF_THE_SWORD || \
                           GET_CLASS(ch) == CLASS_KNIGHT_OF_THE_ROSE || \
                           GET_CLASS(ch) == CLASS_KNIGHT_OF_THE_THORN || \
                           GET_CLASS(ch) == CLASS_KNIGHT_OF_THE_SKULL || \
                           GET_CLASS(ch) == CLASS_BARD)

#define GET_CASTING_CLASS(ch) (ch->player_specials->casting_class)

#define NECROMANCER_CAST_TYPE(ch) (CHECK_PLAYER_SPECIAL(ch, (ch->player_specials->saved.necromancer_bonus_levels)))

/* 1 if ch is race, 0 if not */
#define IS_HUMAN(ch) (!IS_NPC(ch) && \
                      (GET_RACE(ch) == RACE_HUMAN || GET_RACE(ch) == DL_RACE_HUMAN))
#define IS_ELF(ch) (!IS_NPC(ch) && \
                    (GET_RACE(ch) == RACE_ELF || GET_RACE(ch) == RACE_WILD_ELF || GET_RACE(ch) == RACE_HIGH_ELF))
#define IS_DWARF(ch) (!IS_NPC(ch) && \
                      (GET_RACE(ch) == RACE_DWARF || GET_RACE(ch) == RACE_DUERGAR_DWARF || GET_RACE(ch) == RACE_GOLD_DWARF || GET_RACE(ch) == RACE_CRYSTAL_DWARF))
#define IS_HALF_TROLL(ch) (!IS_NPC(ch) && \
                           (GET_RACE(ch) == RACE_HALF_TROLL))
#define IS_CRYSTAL_DWARF(ch) (!IS_NPC(ch) && \
                              (GET_RACE(ch) == RACE_CRYSTAL_DWARF))
#define IS_TRELUX(ch) (!IS_NPC(ch) && \
                       (GET_RACE(ch) == RACE_TRELUX))
#define IS_LICH(ch) (!IS_NPC(ch) && \
                     (GET_RACE(ch) == RACE_LICH))
#define IS_HALFLING(ch) (!IS_NPC(ch) && \
                         (GET_RACE(ch) == RACE_HALFLING || GET_RACE(ch) == RACE_STOUT_HALFLING))
#define IS_H_ELF(ch) (!IS_NPC(ch) && \
                      (GET_RACE(ch) == RACE_H_ELF || GET_RACE(ch) == RACE_HALF_DROW))
#define IS_H_ORC(ch) (!IS_NPC(ch) && \
                      (GET_RACE(ch) == RACE_H_ORC))
#define IS_GNOME(ch) (!IS_NPC(ch) && \
                      (GET_RACE(ch) == RACE_GNOME || GET_RACE(ch) == RACE_FOREST_GNOME))
#define IS_ARCANA_GOLEM(ch) (!IS_NPC(ch) && \
                             (GET_RACE(ch) == RACE_ARCANA_GOLEM))
#define IS_ARCANE_GOLEM(ch) (!IS_NPC(ch) && \
                             (GET_RACE(ch) == RACE_ARCANA_GOLEM))
#define IS_DROW(ch) (!IS_NPC(ch) && \
                     (GET_RACE(ch) == RACE_DROW))
#define IS_DROW_ELF(ch) (!IS_NPC(ch) && \
                         (GET_RACE(ch) == RACE_DROW))
#define IS_DARK_ELF(ch) (!IS_NPC(ch) && \
                         (GET_RACE(ch) == RACE_DROW))
#define IS_DUERGAR(ch) (!IS_NPC(ch) && \
                        (GET_RACE(ch) == RACE_DUERGAR))
#define IS_GRAY_DWARF(ch) (!IS_NPC(ch) && \
                           (GET_RACE(ch) == RACE_DUERGAR))
#define IS_DARK_DWARF(ch) (!IS_NPC(ch) && \
                           (GET_RACE(ch) == RACE_DUERGAR))
#if defined(CAMPAIGN_DL)
#define IS_GOBLINOID(ch) ((IS_NPC(ch) && (GET_SUBRACE(ch, 0) == SUBRACE_GOBLINOID || GET_SUBRACE(ch, 1) == SUBRACE_GOBLINOID || \
                          GET_SUBRACE(ch, 2) == SUBRACE_GOBLINOID)) || (!IS_NPC(ch) && (GET_RACE(ch) == DL_RACE_GOBLIN || GET_RACE(ch) == DL_RACE_HOBGOBLIN)))
#else
#define IS_GOBLINOID(ch) ((IS_NPC(ch) && (GET_SUBRACE(ch, 0) == SUBRACE_GOBLINOID || GET_SUBRACE(ch, 1) == SUBRACE_GOBLINOID || \
                          GET_SUBRACE(ch, 2) == SUBRACE_GOBLINOID)) || (!IS_NPC(ch) && (GET_RACE(ch) == RACE_GOBLIN || GET_RACE(ch) == RACE_HOBGOBLIN)))
#endif

// backwards compatibility for old circlemud code snippets
#define SEND_TO_Q(buf, desc) (write_to_output(desc, "%s", buf))

// language stuff
#define SPEAKING(ch) (ch->player_specials->saved.speaking)
#define CAN_SPEAK(ch, i) (can_speak_language(ch, i))
#define GET_REGION(ch) (ch->player_specials->saved.region)
#define GET_LANG(ch)     ((ch)->player_specials->saved.speaking)

#define HIGH_ELF_CANTRIP(ch)	(ch->player_specials->saved.high_elf_cantrip)
#define CAN_CHOOSE_HIGH_ELF_CANTRIP(ch) (HIGH_ELF_CANTRIP(ch) || (GET_RACE(d->character) != RACE_HIGH_ELF && GET_RACE(d->character) != DL_RACE_SILVANESTI_ELF))
#define GET_RACIAL_MAGIC(ch, slot) (ch->player_specials->saved.racial_magic[slot])
#define GET_RACIAL_COOLDOWN(ch, slot) (ch->player_specials->saved.racial_cooldown[slot])
#define GET_DRAGONBORN_ANCESTRY(ch) (ch->player_specials->saved.dragonborn_draconic_ancestry)
#define CAN_CHOOSE_DRAGONBORN_ANCESTRY(ch) (GET_DRAGONBORN_ANCESTRY(ch) || GET_RACE(d->character) != RACE_DRAGONBORN)
#define GET_PRIMORDIAL_MAGIC(ch, slot) (ch->player_specials->saved.primordial_magic[slot])
#define GET_PRIMORDIAL_COOLDOWN(ch, slot) (ch->player_specials->saved.primordial_cooldown[slot])

// shifter forms
#define IS_IRON_GOLEM(ch) (!IS_NPC(ch) && (GET_RACE(ch) == RACE_IRON_GOLEM || GET_DISGUISE_RACE(ch) == RACE_IRON_GOLEM))
#define IS_PIXIE(ch) (!IS_NPC(ch) && (GET_RACE(ch) == RACE_PIXIE || GET_DISGUISE_RACE(ch) == RACE_PIXIE))
#define IS_EFREETI(ch) (!IS_NPC(ch) && (GET_RACE(ch) == RACE_EFREETI || GET_DISGUISE_RACE(ch) == RACE_EFREETI))
#define IS_MANTICORE(ch) (!IS_NPC(ch) && (GET_RACE(ch) == RACE_MANTICORE || GET_DISGUISE_RACE(ch) == RACE_MANTICORE))

// IS_race for various morph/shapechange equivalent of npc races
#define IS_DRAGON(ch) ((IS_NPC(ch) && GET_RACE(ch) == RACE_TYPE_DRAGON) || \
                       (!IS_NPC(ch) && IS_MORPHED(ch) == RACE_TYPE_DRAGON))
#define IS_ANIMAL(ch) ((IS_NPC(ch) && GET_RACE(ch) == RACE_TYPE_ANIMAL) || \
                       (!IS_NPC(ch) && IS_MORPHED(ch) == RACE_TYPE_ANIMAL))
#define IS_UNDEAD(ch) ((IS_NPC(ch) && GET_RACE(ch) == RACE_TYPE_UNDEAD) || \
                       IS_LICH(ch) || IS_VAMPIRE(ch) ||                    \
                       (!IS_NPC(ch) && IS_MORPHED(ch) == RACE_TYPE_UNDEAD) || \
                       HAS_EVOLUTION(ch, EVOLUTION_UNDEAD_APPEARANCE))
#define IS_ELEMENTAL(ch) ((IS_NPC(ch) && GET_RACE(ch) == RACE_TYPE_ELEMENTAL) || \
                          (!IS_NPC(ch) && IS_MORPHED(ch) == RACE_TYPE_ELEMENTAL))
#define IS_PLANT(ch) ((IS_NPC(ch) && GET_RACE(ch) == RACE_TYPE_PLANT) || \
                      (!IS_NPC(ch) && IS_MORPHED(ch) == RACE_TYPE_PLANT))
#define IS_OOZE(ch) ((IS_NPC(ch) && GET_RACE(ch) == RACE_TYPE_OOZE) || \
                     (!IS_NPC(ch) && IS_MORPHED(ch) == RACE_TYPE_OOZE))
#define IS_CONSTRUCT(ch) ((IS_NPC(ch) && GET_RACE(ch) == RACE_TYPE_CONSTRUCT) ||    \
                          (!IS_NPC(ch) && IS_MORPHED(ch) == RACE_TYPE_CONSTRUCT) || \
                          (IS_IRON_GOLEM(ch)))
#define IS_OUTSIDER(ch) ((IS_NPC(ch) && GET_RACE(ch) == RACE_TYPE_OUTSIDER) || \
                         IS_ELEMENTAL(ch) || IS_EFREETI(ch) ||                 \
                         (!IS_NPC(ch) && IS_MORPHED(ch) == RACE_TYPE_OUTSIDER) || \
                         affected_by_spell(ch, SPELL_PLANAR_HEALING) || \
                         affected_by_spell(ch, SPELL_GREATER_PLANAR_HEALING))
#define IS_HUMANOID(ch) ((IS_NPC(ch) && GET_RACE(ch) == RACE_TYPE_HUMANOID) ||    \
                         (!IS_NPC(ch) && IS_MORPHED(ch) == RACE_TYPE_HUMANOID) || \
                         (!IS_NPC(ch) && !IS_MORPHED(ch)))
#define IS_DRACONIAN(ch) (IS_NPC(ch) && (GET_RACE(ch) == DL_RACE_BAAZ_DRACONIAN || GET_RACE(ch) == DL_RACE_BOZAK_DRACONIAN || \
                                         GET_RACE(ch) == DL_RACE_KAPAK_DRACONIAN || GET_RACE(ch) == DL_RACE_SIVAK_DRACONIAN || \
                                         GET_RACE(ch) == DL_RACE_AURAK_DRACONIAN))
#define IS_LIVING(ch) (!IS_UNDEAD(ch) && !IS_CONSTRUCT(ch))
#define IS_VAMPIRE(ch) ((!IS_NPC(ch) && GET_RACE(ch) == RACE_VAMPIRE) ||         \
                        (IS_NPC(ch) && (GET_SUBRACE(ch, 0) == SUBRACE_VAMPIRE || \
                                        GET_SUBRACE(ch, 1) == SUBRACE_VAMPIRE || GET_SUBRACE(ch, 2) == SUBRACE_VAMPIRE)))

#define IS_POWERFUL_BEING(ch) ((ch && IS_NPC(ch) && GET_LEVEL(ch) >= LVL_IMMORT))

    bool can_blood_drain_target(struct char_data *ch, struct char_data *vict);
#define IN_SUNLIGHT(ch) (is_room_in_sunlight(IN_ROOM(ch)))
#define IN_MOVING_WATER(ch) (IN_ROOM(ch) != NOWHERE && world[IN_ROOM(ch)].sector_type == SECT_RIVER)
#define CAN_USE_VAMPIRE_ABILITY(ch) (!IN_SUNLIGHT(ch) && !IN_MOVING_WATER(ch))
#define TIME_SINCE_LAST_FEEDING(ch) (ch->player_specials->saved.time_since_last_feeding)

#define IS_SENTIENT(ch) (IS_HUMANOID(ch) || (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_SENTIENT)))

#define GET_SETCLOAK_TIMER(ch) (ch->player_specials->saved.setcloak_timer)

#define PIXIE_DUST_USES(ch) (ch->player_specials->saved.pixie_dust_uses)
#define PIXIE_DUST_TIMER(ch) (ch->player_specials->saved.pixie_dust_timer)
#define PIXIE_DUST_USES_PER_DAY(ch) (GET_CHA(ch) + 4)

#define EFREETI_MAGIC_USES(ch) (ch->player_specials->saved.efreeti_magic_uses)
#define EFREETI_MAGIC_TIMER(ch) (ch->player_specials->saved.efreeti_magic_timer)
#define EFREETI_MAGIC_USES_PER_DAY 10

#define LAUGHING_TOUCH_USES(ch) (ch->player_specials->saved.laughing_touch_uses)
#define LAUGHING_TOUCH_TIMER(ch) (ch->player_specials->saved.laughing_touch_timer)
#define LAUGHING_TOUCH_USES_PER_DAY(ch) (3 + GET_CHA_BONUS(ch))

#define FLEETING_GLANCE_USES(ch) (ch->player_specials->saved.fleeting_glance_uses)
#define FLEETING_GLANCE_TIMER(ch) (ch->player_specials->saved.fleeting_glance_timer)
#define FLEETING_GLANCE_USES_PER_DAY 3

#define FEY_SHADOW_WALK_USES(ch) (ch->player_specials->saved.fey_shadow_walk_uses)
#define FEY_SHADOW_WALK_TIMER(ch) (ch->player_specials->saved.fey_shadow_walk_timer)
#define FEY_SHADOW_WALK_USES_PER_DAY 1

#define DRAGON_MAGIC_USES(ch) (ch->player_specials->saved.dragon_magic_uses)
#define DRAGON_MAGIC_TIMER(ch) (ch->player_specials->saved.dragon_magic_timer)
#define DRAGON_MAGIC_USES_PER_DAY 10

#define GRAVE_TOUCH_USES(ch) (ch->player_specials->saved.grave_touch_uses)
#define GRAVE_TOUCH_TIMER(ch) (ch->player_specials->saved.grave_touch_timer)
#define GRAVE_TOUCH_USES_PER_DAY(ch) (3 + GET_CHA_BONUS(ch))

#define GRASP_OF_THE_DEAD_USES(ch) (ch->player_specials->saved.grasp_of_the_dead_uses)
#define GRASP_OF_THE_DEAD_TIMER(ch) (ch->player_specials->saved.grasp_of_the_dead_timer)
#define GRASP_OF_THE_DEAD_USES_PER_DAY(ch) (CLASS_LEVEL(ch, CLASS_SORCERER) >= 20 ? 3 : (CLASS_LEVEL(ch, CLASS_SORCERER) >= 17 ? 2 : 1))

#define INCORPOREAL_FORM_USES(ch) (ch->player_specials->saved.incorporeal_form_uses)
#define INCORPOREAL_FORM_TIMER(ch) (ch->player_specials->saved.incorporeal_form_timer)
#define INCORPOREAL_FORM_USES_PER_DAY(ch) (CLASS_LEVEL(ch, CLASS_SORCERER) / 3)

#define HAS_PERFORMED_DEMORALIZING_STRIKE(ch) (ch->char_specials.has_performed_demoralizing_strike)

/* IS_ for other special situations */
#define IS_INCORPOREAL(ch) (is_incorporeal(ch))

#define IS_IMMUNE_CRITS(ch, vict) (is_immune_to_crits(ch, vict))

#define IS_FRIGHTENED(ch) (AFF_FLAGGED(ch, AFF_FEAR) || AFF_FLAGGED(ch, AFF_SHAKEN))

#define KNOWS_MERCY(ch, i) (ch->player_specials->saved.paladin_mercies[i])
#define KNOWS_CRUELTY(ch, i) (ch->player_specials->saved.blackguard_cruelties[i])
#define FIENDISH_BOON_ACTIVE(ch, i) (IS_SET(ch->player_specials->saved.fiendish_boons, FLAG(i)))
#define SET_FIENDISH_BOON(ch, i) (SET_BIT(ch->player_specials->saved.fiendish_boons, FLAG(i)))
#define REMOVE_FIENDISH_BOON(ch, i) (REMOVE_BIT(ch->player_specials->saved.fiendish_boons, FLAG(i)))

/** Defines if ch is outdoors or not. */
#define OUTDOORS(ch) (is_outdoors(ch))
#define IN_WATER(ch)   (is_in_water(ch))
#define IN_WILDERNESS(ch)   (is_in_wilderness(ch))
#define ROOM_OUTDOORS(room) (is_room_outdoors(room))
#define OUTSIDE(ch) (is_outdoors(ch))
#define ROOM_OUTSIDE(room) (is_room_outdoors(room))
#define IS_SHADOW_CONDITIONS(ch) (ch && IN_ROOM(ch) != NOWHERE &&                                                                              \
                                  ((!OUTSIDE(ch) && !ROOM_AFFECTED(IN_ROOM(ch), RAFF_LIGHT) && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_MAGICLIGHT)) || \
                                   (OUTSIDE(ch) && weather_info.sunlight != SUN_LIGHT)))

/** A little more specific macro than above **/
#define IN_NATURE(ch) (world[IN_ROOM(ch)].sector_type == SECT_FIELD ||         \
                       world[IN_ROOM(ch)].sector_type == SECT_FOREST ||        \
                       world[IN_ROOM(ch)].sector_type == SECT_HILLS ||         \
                       world[IN_ROOM(ch)].sector_type == SECT_MOUNTAIN ||      \
                       world[IN_ROOM(ch)].sector_type == SECT_WATER_SWIM ||    \
                       world[IN_ROOM(ch)].sector_type == SECT_WATER_NOSWIM ||  \
                       world[IN_ROOM(ch)].sector_type == SECT_DESERT ||        \
                       world[IN_ROOM(ch)].sector_type == SECT_UD_WILD ||       \
                       world[IN_ROOM(ch)].sector_type == SECT_HIGH_MOUNTAIN || \
                       world[IN_ROOM(ch)].sector_type == SECT_UD_WATER ||      \
                       world[IN_ROOM(ch)].sector_type == SECT_UD_NOSWIM ||     \
                       world[IN_ROOM(ch)].sector_type == SECT_UD_NOGROUND ||   \
                       world[IN_ROOM(ch)].sector_type == SECT_LAVA ||          \
                       world[IN_ROOM(ch)].sector_type == SECT_FLYING ||        \
                       world[IN_ROOM(ch)].sector_type == SECT_MARSHLAND)

/* Group related defines */
#define GROUP(ch) (ch->group)
#define GROUP_LEADER(group) (group->leader)
#define GROUP_FLAGS(group) (group->group_flags)

/**********************/
/* Happy-hour defines */
#define IS_HAPPYQP (happy_data.qp_rate > 0)
#define IS_HAPPYEXP (happy_data.exp_rate > 0)
#define IS_HAPPYGOLD (happy_data.gold_rate > 0)
#define IS_HAPPYTREASURE (happy_data.treasure_rate > 0)

#define HAPPY_EXP happy_data.exp_rate
#define HAPPY_GOLD happy_data.gold_rate
#define HAPPY_QP happy_data.qp_rate
#define HAPPY_TREASURE happy_data.treasure_rate

#define HAPPY_TIME happy_data.ticks_left

#define IS_HAPPYHOUR ((IS_HAPPYEXP || IS_HAPPYGOLD || IS_HAPPYQP || \
                       IS_HAPPYTREASURE) &&                         \
                      (HAPPY_TIME > 0))
/**********************/

/***************************/
/* Staff Ran Event Defines */
#define IS_STAFF_EVENT (staffevent_data.event_num >= 0 && staffevent_data.ticks_left > 0)

#define STAFF_EVENT_NUM staffevent_data.event_num
#define STAFF_EVENT_TIME staffevent_data.ticks_left
#define STAFF_EVENT_DELAY staffevent_data.delay

/**********************/

/* OS compatibility */
#ifndef NULL
/** Just in case NULL is not defined. */
#define NULL (void *)0
#endif

#if !defined(YES)
/** In case YES is not defined. */
#define YES 1
#endif

#if !defined(NO)
/** In case NO is not defined. */
#define NO 0
#endif

/* defines for fseek */
#ifndef SEEK_SET
/** define for fseek */
#define SEEK_SET 0
/** define for fseek */
#define SEEK_CUR 1
/** define for fseek */
#define SEEK_END 2
#endif

/* NOCRYPT can be defined by an implementor manually in sysdep.h. CIRCLE_CRYPT
 * is a variable that the 'configure' script automatically sets when it
 * determines whether or not the system is capable of encrypting. */
#if defined(NOCRYPT) || !defined(CIRCLE_CRYPT)
/** When crypt is not defined. (NOTE: Player passwords will be plain text.) */
#define CRYPT(a, b) (a)
#else
/** When crypt is defined. Player passwords stored encrypted. */
#define CRYPT(a, b) ((char *)crypt((a), (b)))
#endif

/* Config macros */

/** Pointer to the config file. */
#define CONFIG_CONFFILE config_info.CONFFILE

/** Player killing allowed or not? */
#define CONFIG_PK_ALLOWED config_info.play.pk_allowed
/** Player thieving allowed or not? */
#define CONFIG_PT_ALLOWED config_info.play.pt_allowed
/** What level to use the shout command? */
#define CONFIG_LEVEL_CAN_SHOUT config_info.play.level_can_shout
/** How many move points does holler cost? */
#define CONFIG_HOLLER_MOVE_COST config_info.play.holler_move_cost
/** How many characters can fit in a room marked as tunnel? */
#define CONFIG_TUNNEL_SIZE config_info.play.tunnel_size
/** What is the max experience that can be gained at once? */
#define CONFIG_MAX_EXP_GAIN config_info.play.max_exp_gain
/** What is the max experience that can be lost at once? */
#define CONFIG_MAX_EXP_LOSS config_info.play.max_exp_loss
/** How long will npc corpses last before decomposing? */
#define CONFIG_MAX_NPC_CORPSE_TIME config_info.play.max_npc_corpse_time
/** How long will pc corpses last before decomposing? */
#define CONFIG_MAX_PC_CORPSE_TIME config_info.play.max_pc_corpse_time
/** How long can a pc be idled before being pulled into the void? */
#define CONFIG_IDLE_VOID config_info.play.idle_void
/** How long until the idle pc is force rented? */
#define CONFIG_IDLE_RENT_TIME config_info.play.idle_rent_time
/** What level and above is immune to idle outs? */
#define CONFIG_IDLE_MAX_LEVEL config_info.play.idle_max_level
/** Are death traps dumps? */
#define CONFIG_DTS_ARE_DUMPS config_info.play.dts_are_dumps
/** Should items crated with the load command be placed on ground or
 * in the creator's inventory? */
#define CONFIG_LOAD_INVENTORY config_info.play.load_into_inventory
/** Get the track through doors setting. */
#define CONFIG_TRACK_T_DOORS config_info.play.track_through_doors
/** Get the permission to level up from mortal to immortal. */
#define CONFIG_NO_MORT_TO_IMMORT config_info.play.no_mort_to_immort
/** Get the 'OK' message. */
#define CONFIG_OK config_info.play.OK
/** Get the NOPERSON message. */
#define CONFIG_NOPERSON config_info.play.NOPERSON
/** Get the NOEFFECT message. */
#define CONFIG_NOEFFECT config_info.play.NOEFFECT
/** Get the display closed doors setting. */
#define CONFIG_DISP_CLOSED_DOORS config_info.play.disp_closed_doors
/** Get the diagonal directions setting. */
#define CONFIG_DIAGONAL_DIRS config_info.play.diagonal_dirs

/* Map/Automap options */
#define CONFIG_MAP config_info.play.map_option
#define CONFIG_MAP_SIZE config_info.play.map_size
#define CONFIG_MINIMAP_SIZE config_info.play.minimap_size

/* DG Script Options */
#define CONFIG_SCRIPT_PLAYERS config_info.play.script_players
/* Zone Claim Options */
#define CONFIG_MIN_POP_TO_CLAIM config_info.play.min_pop_to_claim

/* Crash Saves */
/** Get free rent setting. */
#define CONFIG_FREE_RENT config_info.csd.free_rent
/** Get max number of objects to save. */
#define CONFIG_MAX_OBJ_SAVE config_info.csd.max_obj_save
/** Get minimum cost to rent. */
#define CONFIG_MIN_RENT_COST config_info.csd.min_rent_cost
/** Get the auto save setting. */
#define CONFIG_AUTO_SAVE config_info.csd.auto_save
/** Get the auto save frequency. */
#define CONFIG_AUTOSAVE_TIME config_info.csd.autosave_time
/** Get the length of time to hold crash files. */
#define CONFIG_CRASH_TIMEOUT config_info.csd.crash_file_timeout
/** Get legnth of time to hold rent files. */
#define CONFIG_RENT_TIMEOUT config_info.csd.rent_file_timeout

/* Room Numbers */
/** Get the mortal start room. */
#define NUM_MORT_START_ROOMS 2 /* a mechanic to enable multiple start rooms */
#define CONFIG_MORTAL_START config_info.room_nums.mortal_start_room
/** Get the immortal start room. */
#define CONFIG_IMMORTAL_START config_info.room_nums.immort_start_room
/** Get the frozen character start room. */
#define CONFIG_FROZEN_START config_info.room_nums.frozen_start_room
/** Get the 1st donation room. */
#define CONFIG_DON_ROOM_1 config_info.room_nums.donation_room_1
/** Get the second donation room. */
#define CONFIG_DON_ROOM_2 config_info.room_nums.donation_room_2
/** Ge the third dontation room. */
#define CONFIG_DON_ROOM_3 config_info.room_nums.donation_room_3

/* Game Operation */
/** Get the default mud connection port. */
#define CONFIG_DFLT_PORT config_info.operation.DFLT_PORT
#define CONFIG_DFLT_DEV_PORT 4001
/** Get the default mud ip address. */
#define CONFIG_DFLT_IP config_info.operation.DFLT_IP
/** Get the max number of players allowed. */
#define CONFIG_MAX_PLAYING config_info.operation.max_playing
/** Get the max filesize allowed. */
#define CONFIG_MAX_FILESIZE config_info.operation.max_filesize
/** Get the max bad password attempts. */
#define CONFIG_MAX_BAD_PWS config_info.operation.max_bad_pws
/** Get the siteok setting. */
#define CONFIG_SITEOK_ALL config_info.operation.siteok_everyone
/** Get the auto-save-to-disk settings for OLC. */
#define CONFIG_OLC_SAVE config_info.operation.auto_save_olc
/** Get the ability to use aedit or not. */
#define CONFIG_NEW_SOCIALS config_info.operation.use_new_socials
/** Get the setting to resolve IPs or not. */
#define CONFIG_NS_IS_SLOW config_info.operation.nameserver_is_slow
/** Default data directory. */
#define CONFIG_DFLT_DIR config_info.operation.DFLT_DIR
/** Where is the default log file? */
#define CONFIG_LOGNAME config_info.operation.LOGNAME
/** Get the text displayed in the opening menu. */
#define CONFIG_MENU config_info.operation.MENU
/** Get the standard welcome message. */
#define CONFIG_WELC_MESSG config_info.operation.WELC_MESSG
/** Get the standard new character message. */
#define CONFIG_START_MESSG config_info.operation.START_MESSG
/** Should medit show the advnaced stats menu? */
#define CONFIG_MEDIT_ADVANCED config_info.operation.medit_advanced
/** Does "bug resolve" autosave ? */
#define CONFIG_IBT_AUTOSAVE config_info.operation.ibt_autosave
/** Use the protocol negotiation system? */
#define CONFIG_PROTOCOL_NEGOTIATION config_info.operation.protocol_negotiation
/** Use the special character in comm channels? */
#define CONFIG_SPECIAL_IN_COMM config_info.operation.special_in_comm
/** Activate debug mode? */
#define CONFIG_DEBUG_MODE config_info.operation.debug_mode

/* Autowiz */
/** Use autowiz or not? */
#define CONFIG_USE_AUTOWIZ config_info.autowiz.use_autowiz
/** What is the minimum level character to put on the wizlist? */
#define CONFIG_MIN_WIZLIST_LEV config_info.autowiz.min_wizlist_lev

/* Hour hour stuff */
/** chance for happy hour to occur autmatically each rl hour */
#define CONFIG_HAPPY_HOUR_CHANCE config_info.happy_hour.chance
/** Percent increase of quest points during automated happy hour */
#define CONFIG_HAPPY_HOUR_QP config_info.happy_hour.qp
/** Percent increase of experience during automated happy hour */
#define CONFIG_HAPPY_HOUR_EXP config_info.happy_hour.exp
/** Percent increase of gold during automated happy hour */
#define CONFIG_HAPPY_HOUR_GOLD config_info.happy_hour.gold
/** Percent increase of chance for random treasure during automated happy hour */
#define CONFIG_HAPPY_HOUR_TREASURE config_info.happy_hour.treasure

/* Player config data stuff! */
/** Percent increase on psionic power damage */
#define CONFIG_PSIONIC_DAMAGE config_info.player_config.psionic_power_damage_bonus
/** Percent increase on divine spell damage */
#define CONFIG_DIVINE_DAMAGE config_info.player_config.divine_spell_damage_bonus
/** Percent increase on arcane spell damage */
#define CONFIG_ARCANE_DAMAGE config_info.player_config.arcane_spell_damage_bonus
/** Extra hit points awarded to players each level */
#define CONFIG_EXTRA_PLAYER_HP_PER_LEVEL config_info.player_config.extra_hp_per_level
/** Extra movement points awarded to players each level */
#define CONFIG_EXTRA_PLAYER_MV_PER_LEVEL config_info.player_config.extra_mv_per_level
/** this is the maximum ac a player can have */
#define CONFIG_PLAYER_AC_CAP config_info.player_config.armor_class_cap
/** This is the maximum difference between player and mob level to gain exp */
#define CONFIG_EXP_LEVEL_DIFFERENCE config_info.player_config.group_level_difference_restriction
/** This is the percentage of level 1-10 player summons hit points compared to normal */
#define CONFIG_SUMMON_LEVEL_1_10_HP config_info.player_config.level_1_10_summon_hp
/** This is the percentage of level 1-10 player summons hit and dam rolls compared to normal */
#define CONFIG_SUMMON_LEVEL_1_10_HIT_DAM config_info.player_config.level_1_10_summon_hit_and_dam
/** This is the percentage of level 1-10 player summons ac compared to normal */
#define CONFIG_SUMMON_LEVEL_1_10_AC config_info.player_config.level_1_10_summon_ac
/** This is the percentage of level 11-20 player summons hit points compared to normal */
#define CONFIG_SUMMON_LEVEL_11_20_HP config_info.player_config.level_11_20_summon_hp
/** This is the percentage of level 11-20 player summons hit and dam rolls compared to normal */
#define CONFIG_SUMMON_LEVEL_11_20_HIT_DAM config_info.player_config.level_11_20_summon_hit_and_dam
/** This is the percentage of level 11-20 player summons ac compared to normal */
#define CONFIG_SUMMON_LEVEL_11_20_AC config_info.player_config.level_11_20_summon_ac
/** This is the percentage of level 21-30 player summons hit points compared to normal */
#define CONFIG_SUMMON_LEVEL_21_30_HP config_info.player_config.level_21_30_summon_hp
/** This is the percentage of level 21-30 player summons hit and dam rolls compared to normal */
#define CONFIG_SUMMON_LEVEL_21_30_HIT_DAM config_info.player_config.level_21_30_summon_hit_and_dam
/** This is the percentage of level 21-30 player summons ac compared to normal */
#define CONFIG_SUMMON_LEVEL_21_30_AC config_info.player_config.level_21_30_summon_ac
/** This is the percentage value compared to normal for psionic psp regeneration */
#define CONFIG_PSIONIC_PREP_TIME config_info.player_config.psionic_mem_times
/** This is the percentage value compared to normal for divine spell prep times */
#define CONFIG_DIVINE_PREP_TIME config_info.player_config.divine_mem_times
/** This is the percentage value compared to normal for arcane spell prep times */
#define CONFIG_ARCANE_PREP_TIME config_info.player_config.arcane_mem_times
/** This is the percentage value compared to normal for alchemy concoction prep times */
#define CONFIG_ALCHEMY_PREP_TIME config_info.player_config.alchemy_mem_times
/** This is the modified percentage of experience lost when a player dies */
#define CONFIG_DEATH_EXP_LOSS config_info.player_config.death_exp_loss_penalty

/* Action queues */
#define GET_QUEUE(ch) ((ch)->char_specials.action_queue)
#define GET_ATTACK_QUEUE(ch) ((ch)->char_specials.attack_queue)
#define GET_BAB(ch) (BAB(ch))

/* Bonus Types */
#define BONUS_TYPE_STACKS(bonus_type) ((bonus_type == BONUS_TYPE_DODGE) || (bonus_type == BONUS_TYPE_CIRCUMSTANCE) || \
                                       (bonus_type == BONUS_TYPE_UNDEFINED) || (bonus_type == BONUS_TYPE_UNIVERSAL))

/* GUI MSDP Related Defines */
#define GUI_CMBT_OPEN(ch) (gui_combat_wrap_open(ch))
#define GUI_CMBT_CLOSE(ch) (gui_combat_wrap_close(ch))
#define GUI_CMBT_NOTVICT_OPEN(ch, vict) (gui_combat_wrap_notvict_open(ch, vict))
#define GUI_CMBT_NOTVICT_CLOSE(ch, vict) (gui_combat_wrap_notvict_close(ch, vict))
#define GUI_RDSC_OPEN(ch) (gui_room_desc_wrap_open(ch))
#define GUI_RDSC_CLOSE(ch) (gui_room_desc_wrap_close(ch))

#ifdef CAMPAING_FR
#define CRAFTING_CRYSTAL "shard of abeir"
#elif defined(CAMPAIGN_DL)
#define CRAFTING_CRYSTAL "greygem shard"
#else
#define CRAFTING_CRYSTAL  "arcanite crystal"
#endif

// LootBoxes / Treasure Chests

#define LOOTBOX_LEVEL(obj) (GET_OBJ_VAL(obj, 0))
#define LOOTBOX_TYPE(obj) (GET_OBJ_VAL(obj, 1))

// Psionic related stuff
#define GET_PSIONIC_ENERGY_TYPE(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.psionic_energy_type))

#define BRUTALIZE_WOUNDS_SAVE_SUCCESS 2
#define BRUTALIZE_WOUNDS_SAVE_FAIL 1

// Misc combat stuff
#define GET_TEMP_ATTACK_ROLL_BONUS(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->temp_attack_roll_bonus))

#define ARENA_START 138600
#define ARENA_END 138608

#define STORED_POTIONS(ch, snum) (ch->player_specials->saved.potions[snum])
#define STORED_SCROLLS(ch, snum) (ch->player_specials->saved.scrolls[snum])
#define STORED_WANDS(ch, snum) (ch->player_specials->saved.wands[snum])
#define STORED_STAVES(ch, snum) (ch->player_specials->saved.staves[snum])

// Assassin feats and functionality
#define GET_MARK_ROUNDS(ch) (ch->player_specials->mark_rounds)
#define GET_MARK(ch) (ch->player_specials->mark_target)
#define GET_MARK_HIT_BONUS(ch) (ch->player_specials->death_attack_hit_bonus)
#define GET_MARK_DAM_BONUS(ch) (ch->player_specials->death_attack_dam_bonus)
    bool is_marked_target(struct char_data *ch, struct char_data *vict);
void apply_assassin_backstab_bonuses(struct char_data *ch, struct char_data *vict);

// Inquisitor Stuff
#define GET_JUDGEMENT_TARGET(ch) (ch->player_specials->judgement_target)
#define IS_JUDGEMENT_ACTIVE(ch, i) (ch->player_specials->saved.judgement_enabled[i])
#define GET_SLAYER_JUDGEMENT(ch) (ch->player_specials->saved.slayer_judgement)
#define GET_BANE_TARGET_TYPE(ch) (ch->player_specials->saved.bane_enemy_type)
bool is_judgement_possible(struct char_data *ch, struct char_data *t, int type);
int judgement_bonus_type(struct char_data *ch);
int get_judgement_bonus(struct char_data *ch, int type);
int has_teamwork_feat(struct char_data *ch, int featnum);
int num_fighting(struct char_data *ch);
int teamwork_using_shield(struct char_data *ch, int featnum);
int teamwork_best_stealth(struct char_data *ch, int featnum);
int teamwork_best_d20(struct char_data *ch, int featnum);
int count_teamwork_feats_available(struct char_data *ch);
#define GET_TEAMWORK_FEAT_POINTS(ch) (count_teamwork_feats_available(ch))

// Walkto functionality
#define GET_WALKTO_LOC(ch) (ch->player_specials->walkto_location)

// Buff self
#define GET_BUFF(ch, i, j)        (ch->player_specials->saved.buff_abilities[i][j])
#define GET_CURRENT_BUFF_SLOT(ch) (ch->player_specials->buff_slot)
#define GET_BUFF_TIMER(ch)        (ch->player_specials->buff_timer)
#define IS_BUFFING(ch)            (ch->player_specials->is_buffing)
#define GET_BUFF_TARGET(ch)       (ch->player_specials->buff_target)

// summoners
int char_has_evolution(struct char_data *ch, int evo);
void save_eidolon_descs(struct char_data *ch);
void set_eidolon_descs(struct char_data *ch);
#define HAS_EVOLUTION(ch, i)      (char_has_evolution(ch, i))
#define HAS_REAL_EVOLUTION(ch, i) ((ch)->char_specials.saved.eidolon_evolutions[i])
#define HAS_TEMP_EVOLUTION(ch, i) ((ch)->char_specials.temporary_eidolon_evolutions[i])
#define KNOWS_EVOLUTION(ch, i) ((ch)->char_specials.saved.known_evolutions[i])
#define GET_EIDOLON_BASE_FORM(ch) ((ch)->char_specials.saved.eidolon_base_form)
#define GET_EIDOLON_SHORT_DESCRIPTION(ch) (ch->player.eidolon_shortdescription)
#define GET_EIDOLON_LONG_DESCRIPTION(ch) (ch->player.eidolon_longdescription)
#define CALL_EIDOLON_COOLDOWN(ch) (ch->player_specials->saved.call_eidolon_cooldown)
#define MERGE_FORMS_TIMER(ch)  (ch->player_specials->saved.merge_forms_timer)

// misc
#define GET_CONSECUTIVE_HITS(ch)  ((ch)->char_specials.consecutive_hits)
#define GET_PUSHED_TIMER(ch)  ((ch)->char_specials.has_been_pushed)
#define GET_SICKENING_AURA_TIMER(ch)  ((ch)->char_specials.sickening_aura_timer)
#define GET_FRIGHTFUL_PRESENCE_TIMER(ch)  ((ch)->char_specials.frightful_presence_timer)
bool has_reach(struct char_data *ch);

#define WEAPON_SPELL_PROC(ch) (ch->player.weaponSpellProc)

#define IS_DRAGONHIDE(material) (material == MATERIAL_DRAGONHIDE)
#define IS_DRAGONSCALE(material) (material == MATERIAL_DRAGONSCALE)
#define IS_DRAGONBONE(material) (material == MATERIAL_DRAGONBONE)
#define IS_DRAGON_CRAFT_MATERIAL(material) (IS_DRAGONHIDE(material) || IS_DRAGONSCALE(material) || IS_DRAGONBONE(material))

#define GET_KAPAK_SALIVA_HEALING_COOLDOWN(ch) (ch->char_specials.saved.kapak_healing_cooldown)

#define IS_OBJ_CONSUMABLE(obj)  (GET_OBJ_TYPE(obj) == ITEM_POTION || GET_OBJ_TYPE(obj) == ITEM_SCROLL || \
                                 GET_OBJ_TYPE(obj) == ITEM_WAND || GET_OBJ_TYPE(obj) == ITEM_STAFF)

#define GET_WEAPON_TOUCH_SPELL(ch)  (ch->player_specials->weapon_touch_spell)
#define GET_TOUCH_SPELL_QUEUED(ch)  (ch->player_specials->touch_spell_queued)

#define GET_FORETELL_USES(ch)       (ch->char_specials.foretell_uses)

#define GET_FIGHT_TO_THE_DEATH_COOLDOWN(ch) (ch->player_specials->saved.fight_to_the_death_cooldown)

#define GET_DRAGON_BOND_TYPE(ch)  (ch->player_specials->saved.dragon_bond_type)
#define GET_DRAGON_RIDER_DRAGON_TYPE(ch)  (ch->player_specials->saved.dragon_rider_dragon_type)

#define HAS_DRAGON_BOND_ABIL(ch, level, type)  (is_riding_dragon_mount(ch) && GET_DRAGON_BOND_TYPE(ch) == type && \
                                                HAS_FEAT(ch, FEAT_RIDERS_BOND) && (CLASS_LEVEL(ch, CLASS_DRAGONRIDER) >= level))

#define HAS_ACTIVATED_SPELLS(obj) (obj->activate_spell[0] <= 0 ? FALSE : TRUE)

#define GET_BACKGROUND(ch)  (ch->player_specials->saved.background_type)
#define HAS_BACKGROUND(ch, i)  (GET_LEVEL(ch) >= LVL_IMMORT ? TRUE : ch->player_specials->saved.background_type == i)

#define GET_HOMETOWN(ch)    (ch->player_specials->saved.hometown)

#define GET_FORAGE_COOLDOWN(ch) (ch->player_specials->saved.forage_cooldown)
#define GET_RETAINER_COOLDOWN(ch) (ch->player_specials->saved.retainer_cooldown)

#define GET_SAGE_MOB_VNUM(ch)   (ch->char_specials.sage_mob_vnum)

#define GET_RETAINER_MAIL_RECIPIENT(ch) (ch->player_specials->retainer_mail_recipient)

#endif /* _UTILS_H_ */

/*EOF*/
