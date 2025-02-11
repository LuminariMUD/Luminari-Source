// New LuminariMUD Crafting System by Steve Squires aka Gicker


#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "mysql.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "interpreter.h"
#include "constants.h"
#include "handler.h"
#include "db.h"
#include "craft.h"
#include "spells.h"
#include "mud_event.h"
#include "modify.h" // for parse_at()
#include "treasure.h"
#include "mudlim.h"
#include "spec_procs.h" /* For compute_ability() */
#include "item.h"
#include "quest.h"
#include "assign_wpn_armor.h"
#include "genolc.h"
#include "crafting_new.h"


void assign_harvest_materials_to_word(void)
{
    ssize_t cnt = 0;

    for (cnt = 0; cnt <= top_of_world; cnt++)
    {
        // erase all harvest materials
        wipe_room_harvest_materials(cnt);
        // check valid sector type
        if (!is_valid_harvesting_sector(world[cnt].sector_type))
            continue;
        // check random chance
        if (!will_room_have_harvest_materials(cnt))
            continue;
        // assign materials
        assign_harvest_materials_to_room(cnt);
    }
}

void assign_harvest_materials_to_room(room_rnum room)
{
    if (room == NOWHERE) return;

    int material_type = determine_harvest_material_for_room(room);
   
    if (material_type != CRAFT_MAT_NONE)
    {
        world[room].harvest_material = material_type;
        world[room].harvest_material_amount = determine_number_of_harvest_units_for_room();
    }
}

int determine_harvest_material_for_room(room_rnum room)
{
    if (room == NOWHERE) return 0;

    int zone_low = zone_table[world[room].zone].min_level;
    int zone_high = zone_table[world[room].zone].min_level;
    int zone_level = MIN(30, MAX(1, ((zone_high - zone_low) / 2) + zone_low));
    int grade = determine_grade_by_zone_level(zone_level);
    int group = determine_random_material_group_by_sector_type(world[room].sector_type);

    grade = determine_random_grade(grade);

    int material = determine_material_type_by_group_and_grade(group, grade);

    return material;

}

int determine_material_type_by_group_and_grade(int group, int grade)
{
    switch (group)
    {
        case CRAFT_GROUP_HARD_METALS:
            switch (grade)
            {
                case 1: case 2: return CRAFT_MAT_TIN;
                case 3: case 4: dice(1, 2) == 1 ? CRAFT_MAT_IRON : CRAFT_MAT_COAL;
                case 5: dice(1, 2) == 1 ? CRAFT_MAT_MITHRIL : CRAFT_MAT_ADAMANTITE;
            }
            break;
        case CRAFT_GROUP_SOFT_METALS:
            switch (grade)
            {
                case 1: case 2: return CRAFT_MAT_COPPER;
                case 3: return CRAFT_MAT_SILVER;
                case 4: return CRAFT_MAT_GOLD;
                case 5: return CRAFT_MAT_PLATINUM;
            }
            break;
        case CRAFT_GROUP_WOOD:
            switch (grade)
            {
                case 1: return CRAFT_MAT_ASH_WOOD; 
                case 2: return CRAFT_MAT_MAPLE_WOOD;
                case 3: return CRAFT_MAT_MAHAGONY_WOOD;
                case 4: return CRAFT_MAT_VALENWOOD;
                case 5: return CRAFT_MAT_IRONWOOD;
            }
            break;
        case CRAFT_GROUP_HIDES:
            switch (grade)
            {
                case 1: return CRAFT_MAT_LOW_GRADE_HIDE;
                case 2: return CRAFT_MAT_MEDIUM_GRADE_HIDE;
                case 3: return CRAFT_MAT_HIGH_GRADE_HIDE;
                case 4: case 5: return CRAFT_MAT_PRISTINE_GRADE_HIDE;
            }
            break;
        case CRAFT_GROUP_CLOTH:
            switch (grade)
            {
                case 1: return CRAFT_MAT_HEMP;
                case 2: return CRAFT_MAT_LINEN;
                case 3: return CRAFT_MAT_WOOL;
                case 4: case 5: return CRAFT_MAT_SILK;
            }
            break;
    }
    return CRAFT_MAT_NONE;
}

int determine_random_material_group_by_sector_type(room_rnum sector)
{
    int chance = dice(1, 100);

    switch (sector)
    {
        case SECT_FIELD:
            if (chance <= 75)
                return CRAFT_GROUP_CLOTH;
            else
                return CRAFT_GROUP_HIDES;

        case SECT_FOREST:
        case SECT_TAIGA:
            if (chance <= 75)
                return CRAFT_GROUP_WOOD;
            else
                return CRAFT_GROUP_HIDES;

        case SECT_HILLS:
        case SECT_UD_WILD:
            if (chance <= 25)
                return CRAFT_GROUP_CLOTH;
            else if (chance <= 50)
                return CRAFT_GROUP_HIDES;
            else if (chance <= 75)
                return CRAFT_GROUP_HARD_METALS;
            else
                return CRAFT_GROUP_SOFT_METALS;

        case SECT_MOUNTAIN:
            if (chance <= 20)
                return CRAFT_GROUP_HIDES;
            else if (chance <= 60)
                return CRAFT_GROUP_HARD_METALS;
            else
                return CRAFT_GROUP_SOFT_METALS;

        case SECT_HIGH_MOUNTAIN:
            if (chance <= 50)
                return CRAFT_GROUP_HARD_METALS;
            else
                return CRAFT_GROUP_SOFT_METALS;
        
        case SECT_DESERT:
            if (chance <= 25)
                return CRAFT_GROUP_CLOTH;
            else
                return CRAFT_GROUP_HIDES;
        
        case SECT_MARSHLAND:
            if (chance <= 25)
                return CRAFT_GROUP_CLOTH;
            else if (chance <= 50)
                return CRAFT_GROUP_WOOD;
            else
                return CRAFT_GROUP_HIDES;

        case SECT_CAVE:
            if (chance <= 50)
                return CRAFT_GROUP_HARD_METALS;
            else
                return CRAFT_GROUP_SOFT_METALS;

        case SECT_JUNGLE:
            if (chance <= 25)
                return CRAFT_GROUP_CLOTH;
            else if (chance <= 50)
                return CRAFT_GROUP_HIDES;
            else
                return CRAFT_GROUP_WOOD;

        case SECT_TUNDRA:
            if (chance <= 60)
                return CRAFT_GROUP_HIDES;
            else if (chance <= 80)
                return CRAFT_GROUP_HARD_METALS;
            else
                return CRAFT_GROUP_SOFT_METALS;
        
        // case SECT_BEACH:
        // case SECT_RIVER:
        // case SECT_UD_WATER:
        // case SECT_OCEAN:
        // case SECT_WATER_SWIM:
        // case SECT_WATER_NOSWIM:
        // case SECT_UNDERWATER:     
    }

    return CRAFT_GROUP_NONE;
}

int determine_random_grade(int grade)
{
    int chance = dice(1, 100);

    switch (grade)
    {
        case 1:
            return 1;
        case 2:
            if (chance <= 66)
                return 1;
            else
                return 2;
        case 3:
            if (chance <= 50)
                return 1;
            else if (chance <= 85)
                return 2;
            else
                return 3;
        case 4:
            if (chance <= 30)
                return 1;
            else if (chance <= 60)
                return 2;
            else if (chance <= 85)
                return 3;
            else
                return 4;
        case 5:
            if (chance <= 25)
                return 1;
            else if (chance <= 50)
                return 2;
            else if (chance <= 70)
                return 3;
            else if (chance <= 85)
                return 4;
            else
                return 5;
    }
    return 1;
}

int determine_grade_by_zone_level(int zone_level)
{
    if (zone_level <= 6)
        return 1;
    else if (zone_level <= 12)
        return 2;
    else if (zone_level <= 18)
        return 3;
    else if (zone_level <= 24)
        return 4;
    else
        return 5;
}

int determine_number_of_harvest_units_for_room(void)
{
    int base = 5;
    int extra = 16;

    return (base - 1 + dice(1, extra));
}

bool will_room_have_harvest_materials(room_rnum room)
{

    if (room == NOWHERE)
        return false;

    if (ROOM_FLAGGED(room, ROOM_HARVEST_NODE))
        return true;

    int chance = 20;

    if (dice(1, 100) <= chance)
        return true;

    return false;
}

bool is_valid_harvesting_sector(int sector)
{
    switch (sector)
    {
        case SECT_FIELD:
        case SECT_FOREST:
        case SECT_HILLS:
        case SECT_MOUNTAIN:
        case SECT_DESERT:
        case SECT_MARSHLAND:
        case SECT_HIGH_MOUNTAIN:
        case SECT_UD_WILD:        
        case SECT_CAVE:
        case SECT_JUNGLE:
        case SECT_TUNDRA:
        case SECT_TAIGA:
        
        // case SECT_UD_WATER:
        // case SECT_BEACH:
        // case SECT_RIVER:
        // case SECT_OCEAN:
        // case SECT_WATER_SWIM:
        // case SECT_WATER_NOSWIM:
        // case SECT_UNDERWATER: 
            return true;
    }
    return false;
}

bool room_has_harvest_materials(room_rnum room)
{
    if (room == NOWHERE) return false;

    int i = 0;

    for (i = 1; i < NUM_CRAFT_MATS; i++)
        if (world[room].harvest_material > 0)
            return true;

    return false;
}

void wipe_room_harvest_materials(room_rnum room)
{
    if (room == NOWHERE) return;

    int i = 0;

    for (i = 1; i < NUM_CRAFT_MATS; i++)
        world[room].harvest_material = 0;
}

int material_grade(int material)
{
    switch (material)
    {
        case CRAFT_MAT_COPPER:
        case CRAFT_MAT_TIN:
        case CRAFT_MAT_LOW_GRADE_HIDE:
        case CRAFT_MAT_ASH_WOOD:
        case CRAFT_MAT_HEMP:
            return 1;

        case CRAFT_MAT_BRONZE:
        case CRAFT_MAT_MEDIUM_GRADE_HIDE:
        case CRAFT_MAT_MAPLE_WOOD:
        case CRAFT_MAT_LINEN:
            return 2;

        case CRAFT_MAT_IRON:
        case CRAFT_MAT_COAL:
        case CRAFT_MAT_SILVER:
        case CRAFT_MAT_HIGH_GRADE_HIDE:
        case CRAFT_MAT_MAHAGONY_WOOD:
        case CRAFT_MAT_WOOL:
            return 3;

        case CRAFT_MAT_STEEL:
        case CRAFT_MAT_COLD_IRON:
        case CRAFT_MAT_ALCHEMICAL_SILVER:
        case CRAFT_MAT_GOLD:
        case CRAFT_MAT_PRISTINE_GRADE_HIDE:
        case CRAFT_MAT_VALENWOOD:
        case CRAFT_MAT_SILK:
            return 4;

        case CRAFT_MAT_MITHRIL:
        case CRAFT_MAT_PLATINUM:
        case CRAFT_MAT_ADAMANTITE:
        case CRAFT_MAT_DRAGONSCALE:
        case CRAFT_MAT_DRAGONBONE:
        case CRAFT_MAT_IRONWOOD:
        case CRAFT_MAT_SATIN:
            return 5;
        
        case CRAFT_MAT_DRAGONMETAL:
            return 6;
    }
    return 0;
}

ACMD(do_craft_survey)
{
    if (GET_LEVEL(ch) >= LVL_IMMORT)
    {
        if (world[IN_ROOM(ch)].harvest_material != CRAFT_MAT_NONE && world[IN_ROOM(ch)].harvest_material_amount > 0)
        {
            send_to_char(ch, "There are %d units of %s to be harvested here.\r\n",
                    world[IN_ROOM(ch)].harvest_material_amount, 
                    crafting_materials[world[IN_ROOM(ch)].harvest_material]);
            send_to_char(ch, "Players will see:\r\n");
            send_to_char(ch, "You find %s here.\r\n", crafting_material_nodes[world[IN_ROOM(ch)].harvest_material]);
            return;
        }
        else
        {
            send_to_char(ch, "There is nothing here to harvest.\r\n");
            return;
        }
    }
}