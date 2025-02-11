#ifndef NEWCRAFT_H
#define NEWCRAFT_H

// materials used for the new crafting system
#define CRAFT_MAT_NONE                  0
#define CRAFT_MAT_COPPER                1
#define CRAFT_MAT_TIN                   2
#define CRAFT_MAT_BRONZE                3
#define CRAFT_MAT_IRON                  4
#define CRAFT_MAT_COAL                  5
#define CRAFT_MAT_STEEL                 6
#define CRAFT_MAT_COLD_IRON             7
#define CRAFT_MAT_ALCHEMICAL_SILVER     8
#define CRAFT_MAT_MITHRIL               9
#define CRAFT_MAT_ADAMANTITE            10
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

#define NUM_CRAFT_MATS                  31
// also set in structs.h

#define CRAFT_GROUP_NONE            0
#define CRAFT_GROUP_HARD_METALS     1
#define CRAFT_GROUP_SOFT_METALS     2
#define CRAFT_GROUP_HIDES           3
#define CRAFT_GROUP_WOOD            4
#define CRAFT_GROUP_CLOTH           5

#define NUM_CRAFT_GROUPS            6

#define CRAFT_SKILL_NONE            0
#define CRAFT_SKILL_WEAPONSMITH     1
#define CRAFT_SKILL_ARMORSMITH      2
#define CRAFT_SKILL_JEWELER         3
#define CRAFT_SKILL_TINKER          4
#define CRAFT_SKILL_CARPENTER       5
#define CRAFT_SKILL_TAILOR          6

#define NUM_CRAFT_SKILLS            7

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

struct crafting_data_info
{

    struct obj_data *crafting_obj;

};

bool room_has_harvest_materials(room_rnum room);
int material_grade(int material);
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

ACMD_DECL(do_craft_survey);


#endif // NEWCRAFT_H
