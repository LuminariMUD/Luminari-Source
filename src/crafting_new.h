#ifndef NEWCRAFT_H
#define NEWCRAFT_H

#include <stdbool.h>
#include <time.h>     // For time_t type
#include "structs.h"  // For struct definitions
#include "utils.h"    // For ACMD_DECL and SPECIAL_DECL macros

// materials used for the new crafting system
#define CRAFT_MAT_NONE                  0
#define CRAFT_MAT_COPPER                1
#define CRAFT_MAT_TIN                   2
#define CRAFT_MAT_BRONZE                3
#define CRAFT_MAT_IRON                  4
#define CRAFT_MAT_COAL                  5
#define CRAFT_MAT_STEEL                 6
#define CRAFT_MAT_COLD_IRON             7
#define CRAFT_MAT_ALCHEMAL_SILVER       8
#define CRAFT_MAT_MITHRIL               9
#define CRAFT_MAT_ADAMANTINE            10
#define CRAFT_MAT_SILVER                11
#define CRAFT_MAT_GOLD                  12
#define CRAFT_MAT_PLATINUM              13
#define CRAFT_MAT_DRAGONMETAL           14
#define CRAFT_MAT_DRAGONSCALE           15
#define CRAFT_MAT_DRAGONBONE            16
#define CRAFT_MAT_LOW_GRADE_HIDE        17
#define CRAFT_MAT_MEDIUM_GRADE_HIDE     18
#define CRAFT_MAT_HIGH_GRADE_HIDE       19
#define CRAFT_MAT_PRISTINE_GRADE_HIDE   20
#define CRAFT_MAT_ASH_WOOD              21
#define CRAFT_MAT_MAPLE_WOOD            22
#define CRAFT_MAT_MAHAGONY_WOOD         23
#define CRAFT_MAT_VALENWOOD             24
#define CRAFT_MAT_IRONWOOD              25
#define CRAFT_MAT_HEMP                  26
#define CRAFT_MAT_WOOL                  27
#define CRAFT_MAT_LINEN                 28
#define CRAFT_MAT_SATIN                 29
#define CRAFT_MAT_SILK                  30
#define CRAFT_MAT_ZINC                  31
#define CRAFT_MAT_COTTON                32
#define CRAFT_MAT_BRASS                 33
#define CRAFT_MAT_FLAX                  34
#define CRAFT_MAT_BONE                  35

#define NUM_CRAFT_MATS                  36
// also set in structs.h

#define CRAFT_GROUP_NONE            0
#define CRAFT_GROUP_HARD_METALS     1
#define CRAFT_GROUP_SOFT_METALS     2
#define CRAFT_GROUP_HIDES           3
#define CRAFT_GROUP_WOOD            4
#define CRAFT_GROUP_CLOTH           5
#define CRAFT_GROUP_REFINING        6
#define CRAFT_GROUP_RESIZING        7

#define NUM_CRAFT_GROUPS            8
// also set in structs.h

#define CRAFT_SKILL_NONE            0
#define CRAFT_SKILL_WEAPONSMITH     1
#define CRAFT_SKILL_ARMORSMITH      2
#define CRAFT_SKILL_JEWELER         3
#define CRAFT_SKILL_TINKER          4
#define CRAFT_SKILL_CARPENTER       5
#define CRAFT_SKILL_TAILOR          6
#define CRAFT_SKILL_BREWING         7

#define NUM_CRAFT_SKILLS            8

#define HARVEST_SKILL_NONE          0
#define HARVEST_SKILL_MINING        1
#define HARVEST_SKILL_HUNTING       2
#define HARVEST_SKILL_FORESTRY      3
#define HARVEST_SKILL_GATHERING     4

#define NUM_HARVEST_SKILLS          5

#define REFINING_SKILL_NONE         0
#define REFINING_SKILL_SMELTING     1
#define REFINING_SKILL_WEAVING      2
#define REFINING_SKILL_TANNING      3
#define REFINING_SKILL_CARVING      4

#define NUM_REFINING_SKILLS         5

#define CRAFT_TYPE_NONE             0
#define CRAFT_TYPE_WEAPON           1
#define CRAFT_TYPE_ARMOR            2
#define CRAFT_TYPE_MISC             3
#define CRAFT_TYPE_INSTRUMENT       4

#define NUM_CRAFT_TYPES             5

#define CRAFT_JEWELRY_NONE          0
#define CRAFT_JEWELRY_RING          1
#define CRAFT_JEWELRY_NECKLACE      2
#define CRAFT_JEWELRY_BRACELET      3
#define CRAFT_JEWELRY_EARRING       4
#define CRAFT_JEWELRY_GLASSES       5
#define CRAFT_MISC_BOOTS            6
#define CRAFT_MISC_GLOVES           7
#define CRAFT_MISC_CLOAK            8
#define CRAFT_MISC_BELT             9
#define CRAFT_MISC_MASK             10
#define CRAFT_MISC_SHOULDERS        11
#define CRAFT_MISC_ANKLET           12

#define NUM_CRAFT_MISC_TYPES        13

#define CRAFT_INSTRUMENT_NONE       0
#define CRAFT_INSTRUMENT_LYRE       1
#define CRAFT_INSTRUMENT_FLUTE      2
#define CRAFT_INSTRUMENT_HORN       3
#define CRAFT_INSTRUMENT_HARP       4
#define CRAFT_INSTRUMENT_DRUM       5
#define CRAFT_INSTRUMENT_MANDOLIN   6

#define NUM_CRAFT_INSTRUMENT_TYPES  7

#define SCMD_NEWCRAFT_CREATE        1
#define SCMD_NEWCRAFT_SURVEY        2
#define SCMD_NEWCRAFT_HARVEST       3
#define SCMD_NEWCRAFT_REFINE        4
#define SCMD_NEWCRAFT_RESIZE        5
#define SCMD_NEWCRAFT_SUPPLYORDER   6
#define SCMD_NEWCRAFT_EQUIPMENT     7

#define NUM_CRAFTING_METHODS        8

// Supply order contract types
#define SUPPLY_CONTRACT_BASIC       1
#define SUPPLY_CONTRACT_RUSH        2
#define SUPPLY_CONTRACT_BULK        3
#define SUPPLY_CONTRACT_QUALITY     4
#define SUPPLY_CONTRACT_PRESTIGE    5
#define SUPPLY_CONTRACT_EVENT       6

#define NUM_SUPPLY_CONTRACT_TYPES   7

// Supply order constants
#define MAX_ACTIVE_CONTRACTS        3
#define MIN_CONTRACT_QUANTITY       3
#define MAX_CONTRACT_QUANTITY       10
#define BASE_CONTRACT_REWARD        100

// Reputation system constants
#define REP_RANK_NOVICE             0
#define REP_RANK_APPRENTICE         1
#define REP_RANK_JOURNEYMAN         2
#define REP_RANK_EXPERT             3
#define REP_RANK_MASTER             4
#define REP_RANK_GRANDMASTER        5

#define NUM_REP_RANKS               6

// Reputation thresholds
#define REP_THRESHOLD_APPRENTICE    100
#define REP_THRESHOLD_JOURNEYMAN    300
#define REP_THRESHOLD_EXPERT        600
#define REP_THRESHOLD_MASTER        1000
#define REP_THRESHOLD_GRANDMASTER   1500

// Quality tier constants
#define QUALITY_TIER_STANDARD       0
#define QUALITY_TIER_SUPERIOR       1
#define QUALITY_TIER_EXCEPTIONAL    2
#define QUALITY_TIER_MASTERWORK     3
#define QUALITY_TIER_LEGENDARY      4

#define NUM_QUALITY_TIERS           5

// Bulk efficiency bonuses
#define BULK_EFFICIENCY_THRESHOLD_1 5   // 10% bonus
#define BULK_EFFICIENCY_THRESHOLD_2 10  // 20% bonus
#define BULK_EFFICIENCY_THRESHOLD_3 15  // 30% bonus

// Contract expiration times (in real-world hours)
#define CONTRACT_EXPIRE_BASIC       72  // 3 days
#define CONTRACT_EXPIRE_RUSH        24  // 1 day
#define CONTRACT_EXPIRE_BULK        96  // 4 days
#define CONTRACT_EXPIRE_QUALITY     48  // 2 days
#define CONTRACT_EXPIRE_PRESTIGE    168 // 1 week
#define CONTRACT_EXPIRE_EVENT       12  // 12 hours

#define CRAFT_SKILL_TYPE_NONE       0
#define CRAFT_SKILL_TYPE_CRAFT      1
#define CRAFT_SKILL_TYPE_HARVEST    2

#define NUM_CRAFT_SKILL_TYPES       3

#define CRAFTING_MOTE_NONE          0
#define CRAFTING_MOTE_AIR           1
#define CRAFTING_MOTE_DARK          2
#define CRAFTING_MOTE_EARTH         3
#define CRAFTING_MOTE_FIRE          4
#define CRAFTING_MOTE_ICE           5
#define CRAFTING_MOTE_LIGHT         6
#define CRAFTING_MOTE_LIGHTNING     7
#define CRAFTING_MOTE_WATER         8

#define NUM_CRAFT_MOTES             9 // Also defined in structs.h

#define MAX_CRAFT_SKILL             30

#define ARMOR_ENHANCEMENT_MOTE CRAFTING_MOTE_WATER
#define WEAPON_ENHANCEMENT_MOTE CRAFTING_MOTE_ICE

bool room_has_harvest_materials(room_rnum room);
int material_grade(int material);
int obj_material_to_craft_material(int material);
bool is_valid_harvesting_sector(int sector);
bool will_room_have_harvest_materials(room_rnum room);
int determine_grade_by_zone_level(int zone_level);
int determine_random_grade(int grade);
void wipe_room_harvest_materials(room_rnum room);
void assign_harvest_materials_to_word(void);
void assign_harvest_materials_to_room(room_rnum room);
int determine_harvest_material_for_room(room_rnum room);
int determine_number_of_harvest_units_for_room(void);
int determine_material_type_by_group_and_grade(int group, int grade);
int determine_random_material_group_by_sector_type(room_rnum sector);
bool is_valid_craft_ability(int ability);
bool is_valid_craft_feat(int feat);
bool is_valid_craft_class(int ch_class, int location);
void reset_current_craft(struct char_data *ch, char *arg2, bool verbose, bool reimburse);
int craft_recipe_by_type(int type);
int craft_misc_type_by_wear_loc(int wear_loc);
bool is_craft_ready(struct char_data *ch, bool verbose);
void set_crafting_variant(struct char_data *ch, char *arg2);
void begin_current_craft(struct char_data *ch);
void craft_create_complete(struct char_data *ch);
void set_craft_item_descs(struct char_data *ch, struct obj_data *obj);
void set_craft_item_affects(struct char_data *ch, struct obj_data *obj);
void create_craft_armor(struct char_data *ch);
void set_craft_item_flags(struct char_data *ch, struct obj_data *obj);
void set_crafting_materials(struct char_data *ch, const char *arg2);
int craft_group_by_material(int material);
void process_crafting_materials(struct char_data *ch, int group, int mat_type, int num_mats, int mat_slot);
void survey_complete(struct char_data *ch);
void sort_materials(void);
int harvesting_skill_by_material(int material);
int crafting_skill_type(int skill);
int craft_skill_level_exp(struct char_data *ch, int level);
void gain_craft_exp(struct char_data *ch, int exp, int abil, bool verbose);
void set_crafting_enhancement(struct char_data *ch, const char *arg2);
int get_level_adjustment_by_apply_and_modifier(int apply, int mod, int btype);
int get_level_adjustment_by_enhancement_bonus(int bonus_amt);
int get_craft_obj_level(struct obj_data *obj, struct char_data *ch);
int craft_motes_required(int location, int modifier, int bonus_type, int enhancement);
int crafting_mote_by_bonus_location(int location, int specific, int bonus_type);
void set_crafting_motes(struct char_data *ch, const char *argument);
void reset_craft_materials(struct char_data *ch, bool verbose, bool reimburse);
int get_craft_project_level(struct char_data *ch);
int get_enhancement_mote_type(struct char_data *ch, int type, int spec);
void show_craft_progress_meter(void);
bool create_craft_skill_check(struct char_data *ch, struct obj_data *obj, int skill, char *method, int exp, int dc);
int get_craft_material_final_level_adjustment(struct char_data *ch);
int craft_material_to_obj_material(int craftmat);
void show_refine_noargs(struct char_data *ch);
bool is_refine_ready(struct char_data *ch, bool verbose);
void craft_refine_complete(struct char_data *ch);
int get_craft_skill_value(struct char_data *ch, int skill_num);
int craft_material_to_obj_material(int material);
void craft_resize_complete(struct char_data *ch);
char *get_supply_order_item_desc(struct char_data *ch);
int determine_supply_order_exp(struct char_data *ch);
void craft_supplyorder_complete(struct char_data *ch);
void reset_crafting_obj(struct char_data *ch);
struct obj_data *find_obj_rnum_in_inventory(struct char_data *ch, obj_rnum obj_rnum);
bool does_craft_apply_type_have_specific_value(int location);
int get_craft_material_by_name(struct char_data *ch, char *arg);
int get_current_craft_project_recipe(struct char_data *ch);
bool remove_supply_order_materials(struct char_data *ch);
void set_supply_order_materials(struct char_data *ch, char *arg2, char *arg3);
int select_random_craft_recipe(void);
bool player_has_supply_order(struct char_data *ch);
void request_new_supply_order(struct char_data *ch);
int num_supply_order_requisitions_to_go(struct char_data *ch);
void start_supply_order(struct char_data *ch);
void show_supply_order_materials(struct char_data *ch, int recipe, int variant);
void show_supply_order(struct char_data *ch);
void reset_supply_order(struct char_data *ch);
int calculate_supply_order_reward(struct char_data *ch);
bool consume_supply_order_materials(struct char_data *ch);
void complete_supply_order(struct char_data *ch);
bool validate_supply_order_materials(struct char_data *ch);
void show_available_contracts(struct char_data *ch);
int generate_contract_quantity(int contract_type);
int generate_contract_reward(int contract_type, int quantity, int difficulty);
int get_player_reputation_rank(struct char_data *ch);
int get_player_reputation_points(struct char_data *ch);
void add_reputation_points(struct char_data *ch, int points);
const char *get_reputation_rank_name(int rank);
int calculate_quality_tier_bonus(struct char_data *ch);
int calculate_bulk_efficiency_bonus(int quantity);
bool is_contract_expired(struct char_data *ch);
void update_contract_expiration(struct char_data *ch, int contract_type);
int get_event_contract_availability(void);
bool is_special_event_active(void);
void generate_prestige_contract(struct supply_contract *contract, struct char_data *ch);
void generate_event_contract(struct supply_contract *contract, struct char_data *ch);
void show_supply_order_cooldowns(struct char_data *ch);
SPECIAL_DECL(new_supply_orders);

struct obj_data *setup_craft_weapon(struct char_data *ch, int w_type);
struct obj_data *setup_craft_misc(struct char_data *ch, int vnum);
struct obj_data *setup_craft_armor(struct char_data *ch, int a_type);
void set_crafting_instrument(struct char_data *ch, char *arg2);
int get_crafting_instrument_dc_modifier(struct char_data *ch);
int get_crafting_instrument_motes(struct char_data *ch, int type, bool get_amount);
void set_craft_instrument_object(struct obj_data *obj, struct char_data *ch);
int craft_instrument_type_to_actual(int type);
struct obj_data *setup_craft_instrument(struct char_data *ch, int a_type);
int get_craft_level_adjust_dc_change(int adjust);
void set_craft_level_adjust(struct char_data *ch, char *arg2);
int craft_misc_spec_to_vnum(int s_type);
int get_craft_wear_loc(struct char_data *ch);
int recipe_skill_to_actual_crafting_skill(int recipe_skill);
bool has_proper_harvesting_tool_equipped(struct char_data *ch);
bool is_wearing_tool_for_crafting_ability(struct char_data *ch, int ability);
bool is_crafting_skill_in_game(int skill);
void show_harvesting_tool_needed(struct char_data *ch);

void newcraft_supplyorder(struct char_data *ch, const char *argument);
void newcraft_equipment(struct char_data *ch, const char *argument);
void newcraft_show_tools(struct char_data *ch, const char *argument);

ACMD_DECL(do_newcraft);
ACMD_DECL(do_setmaterial);
ACMD_DECL(do_list_craft_materials);
ACMD_DECL(do_craftbonuses);
ACMD_DECL(do_craft_score);
ACMD_DECL(do_motes);
ACMD_DECL(do_craft_score_new);

extern int materials_sort_info[NUM_CRAFT_MATS];

// Supply order slot management functions
void initialize_supply_slots(struct char_data *ch);
void cleanup_supply_slots(struct char_data *ch);
void refresh_supply_slots(struct char_data *ch);
bool should_refresh_supply_slots(struct char_data *ch);
void update_supply_slots_for_all_players(void);

// Crafting station requirement functions
bool has_quartermaster_in_room(struct char_data *ch);
int get_required_crafting_station(int skill);
const char *get_crafting_station_name(int skill);
bool has_crafting_station_in_room(struct char_data *ch, int skill);

#endif // NEWCRAFT_H
