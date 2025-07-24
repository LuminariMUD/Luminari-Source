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
#include "spec_procs.h" /* For GET_ABILITY() */
#include "item.h"
#include "quest.h"
#include "assign_wpn_armor.h"
#include "genolc.h"
#include "crafting_new.h"
#include "oasis.h"
#include "feats.h"
#include "class.h"
#include "improved-edit.h"
#include "vnums.h"
#include "crafting_recipes.h"

int copy_object(struct obj_data *to, struct obj_data *from);

int materials_sort_info[NUM_CRAFT_MATS];

#define NEWCRAFT_RESIZE_SYNTAX  "Syntax is as follows: resize (object-name|add|remove|show|bagin) (new-size)\r\n"

#define CRAFT_MOTE_NOARG        "Please specity add or remove, and the bonus slot.\r\n" \
                                "Eg. craft motes add 1\r\n" \
                                "-- This will add the required type and number of motes to bonus slot 1.\r\n"

#define NEWCRAFT_CREATE_NOARG1  "See HELP CRAFTING for more information on how to craft new items.\r\n" \
                                "Options are:\r\n" \
                                "craft itemtype (weapon|armor|jewelry|instrument|misc)\r\n" \
                                "craft specifictype (type)\r\n" \
                                "craft variant (variant name)\r\n" \
                                "craft keywords (keyword string)\r\n" \
                                "craft shortdesc (short desc string)\r\n" \
                                "craft roomdesc (room desc string)\r\n" \
                                "craft extradesc (extra desc string\r\n" \
                                "craft bonuses (slot) (bonus location) (bonus type) (modifier) (specific)\r\n" \
                                "craft enhancement (enhancement modifier\r\n" \
                                "craft instrument (quality|effectiveness|breakability) (amount)" \
                                "craft materials (add|remove) (material type)\r\n" \
                                "craft motes (add|remove) (enhancement|quality|effectiveness|breakability|bonus slot #)\r\n" \
                                "craft leveladjust (level adjustment)\r\n" \
                                "craft score\r\n" \
                                "craft show\r\n" \
                                "craft check\r\n" \
                                "craft reset (no argument|motes|materials|enhancement|instrument|bonuses|descriptions|refine|rezize)\r\n" \
                                "craft start\r\n"

#define NEWCRAFT_CREATE_TYPES   ("What item type do you wish to make?\r\n" \
                                "-- armor       includes shields\r\n" \
                                "-- weapons     all types\r\n" \
                                "-- instruments lyres, drums, etc. for bards\r\n" \
                                "-- misc        rings, necklaces, earrings, cloaks, belts, etc.\r\n")
#define NEWCRAFT_CREATE_BONUSES_NOARG   "You need to specify all of the bonus information:\r\n" \
                                        "Eg. craft bonuses (slot) (bonus location) (bonus type) (modifier) (specific)\r\n" \
                                        "-- slot needs to be 1-6 as an item can only have 6 bonuses total\r\n" \
                                        "-- bonus location is what the bonus affects. Eg. strength, hit-points, reflex-save, etc. Type 'applies' for a list.\r\n" \
                                        "-- bonus type is either enhancement or universal. See HELP CRAFTING for more info\r\n" \
                                        "-- modifier is how much the bonus will affect that associated stat\r\n" \
                                        "-- specific is only required for feat, skill, and spell slot bonus types\r\n" \
                                        "\r\n" \
                                        "To reset a bonus slot type: craft bonus [slot] reset\r\n"

#define SUPPLY_ORDER_NOARG1 "Please specify what supply order action you'd like to take.\r\n" \
                            "supplyorder request : Request a new supply order project.\r\n" \
                            "supplyorder info    : Show information on current supply order project.\r\n" \
                            "supplyorder material: Add materials to current project\r\n" \
                            "supplyorder reset   : Reset current supply order project to default values.\r\n" \
                            "supplyorder complete: Turn in a completed supply order for reward.\r\n" \
                            "supplyorder abandon : Cancel existing supply order project entirely.\r\n" \
                            "\r\n"

#define HARVEST_NODE_PERCENT_CHANCE         33
#define HARVEST_BASE_TIME                   10
#define CREATE_BASE_TIME                    10
#define SURVEY_BASE_TIME                     3
#define HARVEST_BASE_DC                      5
#define CREATE_BASE_DC                      10
#define RESIZE_BASE_DC                      10
#define HARVEST_MOTE_DICE_SIZE               4
#define HARVEST_MOTE_CHANCE                 10
#define HARVEST_BASE_AMOUNT                 dice(2, 2)
#define HARVEST_BASE_EXP                    20
#define CREATE_BASE_EXP                     50
#define RESIZE_BASE_EXP                     10
#define REFINE_BASE_EXP                     10
#define NSUPPLY_ORDER_DURATION              10
#define NSUPPLY_ORDER_NUM_REQUIRED           5
#define NSUPPLY_ORDER_BASE_EXP              50

#define CRAFT_MOTES_REQ_30  80
#define CRAFT_MOTES_REQ_25  60
#define CRAFT_MOTES_REQ_20  40
#define CRAFT_MOTES_REQ_15  25
#define CRAFT_MOTES_REQ_10  15
#define CRAFT_MOTES_REQ_5   8
#define CRAFT_MOTES_REQ_1   3



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

    // small chance for the grade level to be bumped up by one
    if (dice(1, 20) == 1)
    {
        grade++;
    }

    grade = MAX(1, grade);

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
                case 1: case 2: return dice(1, 2) == 1 ? CRAFT_MAT_ZINC : CRAFT_MAT_TIN;
                case 3: case 4: return dice(1, 3) == 1 ? CRAFT_MAT_COAL : CRAFT_MAT_IRON;
                case 5: return dice(1, 2) == 1 ? CRAFT_MAT_MITHRIL : CRAFT_MAT_ADAMANTINE;
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
                case 2: return CRAFT_MAT_FLAX;
                case 3: return CRAFT_MAT_WOOL;
                case 4: return CRAFT_MAT_COTTON;
                case 5: return CRAFT_MAT_SILK;
            }
            break;
    }
    return CRAFT_MAT_NONE;
}

int craft_material_level_adjustment(int material)
{
    switch (material)
    {
        case CRAFT_MAT_TIN: return -2;

        case CRAFT_MAT_BRONZE:
        case CRAFT_MAT_COPPER:
        case CRAFT_MAT_LOW_GRADE_HIDE:
        case CRAFT_MAT_ASH_WOOD:
        case CRAFT_MAT_HEMP:
            return 0;

        case CRAFT_MAT_IRON:
        case CRAFT_MAT_BRASS:
        case CRAFT_MAT_MEDIUM_GRADE_HIDE:
        case CRAFT_MAT_MAPLE_WOOD:
        case CRAFT_MAT_WOOL:
            return 2;

        case CRAFT_MAT_STEEL:
        case CRAFT_MAT_SILVER:
        case CRAFT_MAT_MAHAGONY_WOOD:
        case CRAFT_MAT_LINEN:
            return 4;

        case CRAFT_MAT_COLD_IRON: 
        case CRAFT_MAT_ALCHEMAL_SILVER:
        case CRAFT_MAT_GOLD:
        case CRAFT_MAT_HIGH_GRADE_HIDE:
        case CRAFT_MAT_VALENWOOD:
        case CRAFT_MAT_COTTON:
            return 5;

        case CRAFT_MAT_MITHRIL:
        case CRAFT_MAT_SILK:
            return 7;

        case CRAFT_MAT_ADAMANTINE: 
        case CRAFT_MAT_PLATINUM:
        case CRAFT_MAT_PRISTINE_GRADE_HIDE:
        case CRAFT_MAT_IRONWOOD:
        case CRAFT_MAT_SATIN:
            return 8;

        case CRAFT_MAT_DRAGONMETAL:
        case CRAFT_MAT_DRAGONSCALE:
        case CRAFT_MAT_DRAGONBONE:
            return 10;    
    }
    return 0;
}

int harvesting_skill_by_material(int material)
{
    switch (material)
    {
        case CRAFT_MAT_TIN:
        case CRAFT_MAT_BRONZE:
        case CRAFT_MAT_IRON:
        case CRAFT_MAT_STEEL:
        case CRAFT_MAT_COLD_IRON:
        case CRAFT_MAT_ALCHEMAL_SILVER:
        case CRAFT_MAT_MITHRIL:
        case CRAFT_MAT_ADAMANTINE:
        case CRAFT_MAT_DRAGONMETAL:
        case CRAFT_MAT_COPPER:
        case CRAFT_MAT_SILVER:
        case CRAFT_MAT_GOLD:
        case CRAFT_MAT_PLATINUM:
        case CRAFT_MAT_COAL:
        case CRAFT_MAT_ZINC:
            return ABILITY_HARVEST_MINING;

        case CRAFT_MAT_LOW_GRADE_HIDE:
        case CRAFT_MAT_MEDIUM_GRADE_HIDE:
        case CRAFT_MAT_HIGH_GRADE_HIDE:
        case CRAFT_MAT_PRISTINE_GRADE_HIDE:
        case CRAFT_MAT_DRAGONSCALE:
            return ABILITY_HARVEST_HUNTING;

        case CRAFT_MAT_ASH_WOOD:
        case CRAFT_MAT_MAPLE_WOOD:
        case CRAFT_MAT_MAHAGONY_WOOD:
        case CRAFT_MAT_VALENWOOD:
        case CRAFT_MAT_IRONWOOD:
        case CRAFT_MAT_DRAGONBONE:
            return ABILITY_HARVEST_FORESTRY;
        
        case CRAFT_MAT_HEMP:
        case CRAFT_MAT_WOOL:
        case CRAFT_MAT_LINEN:
        case CRAFT_MAT_FLAX:
        case CRAFT_MAT_SATIN:
        case CRAFT_MAT_COTTON:
        case CRAFT_MAT_SILK:
            return ABILITY_HARVEST_GATHERING;
    }
    return 0;
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
    return dice(2, 6);
}

bool will_room_have_harvest_materials(room_rnum room)
{

    if (room == NOWHERE)
        return false;

    if (ROOM_FLAGGED(room, ROOM_HARVEST_NODE))
        return true;

    int chance = HARVEST_NODE_PERCENT_CHANCE;

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
        case CRAFT_MAT_ZINC:
            return 1;

        case CRAFT_MAT_BRONZE:
        case CRAFT_MAT_MEDIUM_GRADE_HIDE:
        case CRAFT_MAT_MAPLE_WOOD:
        case CRAFT_MAT_LINEN:
        case CRAFT_MAT_FLAX:
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
        case CRAFT_MAT_ALCHEMAL_SILVER:
        case CRAFT_MAT_GOLD:
        case CRAFT_MAT_PRISTINE_GRADE_HIDE:
        case CRAFT_MAT_VALENWOOD:
        case CRAFT_MAT_SILK:
            return 4;

        case CRAFT_MAT_MITHRIL:
        case CRAFT_MAT_PLATINUM:
        case CRAFT_MAT_ADAMANTINE:
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

int craft_group_by_material(int material)
{
    switch (material)
    {
        case CRAFT_MAT_TIN:
        case CRAFT_MAT_BRONZE:
        case CRAFT_MAT_IRON:
        case CRAFT_MAT_STEEL:
        case CRAFT_MAT_COLD_IRON:
        case CRAFT_MAT_ALCHEMAL_SILVER:
        case CRAFT_MAT_MITHRIL:
        case CRAFT_MAT_ADAMANTINE:
        case CRAFT_MAT_DRAGONMETAL:
        case CRAFT_MAT_ZINC:
            return CRAFT_GROUP_HARD_METALS;
        
        case CRAFT_MAT_COPPER:
        case CRAFT_MAT_BRASS:
        case CRAFT_MAT_SILVER:
        case CRAFT_MAT_GOLD:
        case CRAFT_MAT_PLATINUM:
            return CRAFT_GROUP_SOFT_METALS;

        case CRAFT_MAT_LOW_GRADE_HIDE:
        case CRAFT_MAT_MEDIUM_GRADE_HIDE:
        case CRAFT_MAT_HIGH_GRADE_HIDE:
        case CRAFT_MAT_PRISTINE_GRADE_HIDE:
        case CRAFT_MAT_DRAGONSCALE:
            return CRAFT_GROUP_HIDES;

        case CRAFT_MAT_ASH_WOOD:
        case CRAFT_MAT_MAPLE_WOOD:
        case CRAFT_MAT_MAHAGONY_WOOD:
        case CRAFT_MAT_VALENWOOD:
        case CRAFT_MAT_IRONWOOD:
        case CRAFT_MAT_DRAGONBONE:
            return CRAFT_GROUP_WOOD;
        
        case CRAFT_MAT_HEMP:
        case CRAFT_MAT_WOOL:
        case CRAFT_MAT_LINEN:
        case CRAFT_MAT_FLAX:
        case CRAFT_MAT_COTTON:
        case CRAFT_MAT_SATIN:
        case CRAFT_MAT_SILK:
            return CRAFT_GROUP_CLOTH;

        case CRAFT_MAT_COAL:
            return CRAFT_GROUP_REFINING;
    }
    return CRAFT_GROUP_NONE;
}

void survey_complete(struct char_data *ch)
{
    ch->player_specials->surveyed_room = true;
    if (world[IN_ROOM(ch)].harvest_material != CRAFT_MAT_NONE && world[IN_ROOM(ch)].harvest_material_amount > 0)
    {
        if (GET_LEVEL(ch) >= LVL_IMMORT)
        {
            send_to_char(ch, "There are %d units of %s to be harvested here.\r\n",
                    world[IN_ROOM(ch)].harvest_material_amount, 
                    crafting_materials[world[IN_ROOM(ch)].harvest_material]);
            send_to_char(ch, "Players will see:\r\n");
        }
        send_to_char(ch, "You find %s here.\r\n", crafting_material_nodes[world[IN_ROOM(ch)].harvest_material]);
    }
    else
    {
        send_to_char(ch, "There is nothing here to harvest.\r\n");
    }
    GET_CRAFT(ch).crafting_method = 0;
    GET_CRAFT(ch).craft_duration = 0;
    GET_CRAFT(ch).survey_rooms = d20(ch) + 5;
    send_to_char(ch, "You have surveyed %d rooms of the surrounding area.\r\n", GET_CRAFT(ch).survey_rooms);
    act("$n finishes surveying.", FALSE, ch, 0, 0, TO_ROOM);
}

void set_crafting_itemtype(struct char_data *ch, char *arg2)
{
    int i = 0;

    if (GET_CRAFT(ch).crafting_item_type != CRAFT_TYPE_NONE)
    {
        send_to_char(ch, "You have already set the item type. You need to reset the crafting project to change this, by typing 'craft reset'.\r\n");
        return;
    }

    if (!*arg2)
    {
        send_to_char(ch, "%s", NEWCRAFT_CREATE_TYPES);
        return;
    }
    for (i = 1; i < NUM_CRAFT_TYPES; i++)
    {
        if (is_abbrev(arg2, crafting_types[i]))
            break;
    }
    if (i >= NUM_CRAFT_TYPES)
    {
        send_to_char(ch, "That is not a valid crafting type.\r\n");
        send_to_char(ch, "%s", NEWCRAFT_CREATE_TYPES);
        return;
    }

    if (i == CRAFT_TYPE_INSTRUMENT)
    {
        GET_CRAFT(ch).instrument_breakability = INSTRUMENT_BREAKABILITY_DEFAULT;
    }

    send_to_char(ch, "Crafting item type set to: %s\r\n", crafting_types[i]);

    if (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_WEAPON || GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_ARMOR)
    {
        if (GET_CRAFT(ch).crafting_item_type != i)
        {
            send_to_char(ch, "The enhancement bonus has been reset to zero.\r\n");
            GET_CRAFT(ch).enhancement = 0;
        }
    }

    GET_CRAFT(ch).crafting_item_type = i;

}

bool is_valid_craft_weapon(int weapon)
{
    switch (weapon)
    {
        case WEAPON_TYPE_COMPOSITE_LONGBOW:
        case WEAPON_TYPE_COMPOSITE_LONGBOW_2:
        case WEAPON_TYPE_COMPOSITE_LONGBOW_3:
        case WEAPON_TYPE_COMPOSITE_LONGBOW_4:
        case WEAPON_TYPE_COMPOSITE_SHORTBOW:
        case WEAPON_TYPE_COMPOSITE_SHORTBOW_2:
        case WEAPON_TYPE_COMPOSITE_SHORTBOW_3:
        case WEAPON_TYPE_COMPOSITE_SHORTBOW_4:
            return false;
    }
    return true;
}

void craft_show_weapon_types(struct char_data *ch)
{
  int i = 0, count = 0;

  for (i = 1; i < NUM_WEAPON_TYPES; i++)
  {
    if (!is_valid_craft_weapon(i)) continue;
    send_to_char(ch, "%-25s ", weapon_list[i].name);
    if ((count % 3) == 0) send_to_char(ch, "\r\n");
    count++;
  }
  if ((count % 3) != 0) send_to_char(ch, "\r\n");
}

void set_craft_weapon_type(struct char_data *ch, char *arg2)
{
    int i = 0;

    if (!*arg2)
    {
        send_to_char(ch, "\tCYou need to specify a weapon type:\tn\r\n");
        craft_show_weapon_types(ch);
        return;
    }

    for (i = 1; i < NUM_WEAPON_TYPES; i++)
    {
        if (is_abbrev(arg2, weapon_list[i].name))
            break;
    }

    if (i >= NUM_WEAPON_TYPES)
    {
        send_to_char(ch, "That is not a valid weapon type.\r\n");
        craft_show_weapon_types(ch);    
        return;
    }
    
    GET_CRAFT(ch).crafting_specific = i;
    send_to_char(ch, "Crafting weapon type set to: %s\r\n", weapon_list[i].name);
}

void craft_show_armor_types(struct char_data *ch)
{
  int i = 0;

  for (i = 1; i < NUM_SPEC_ARMOR_TYPES; i++)
  {
    send_to_char(ch, "%-25s ", armor_list[i].name);
    if ((i % 3) == 0) send_to_char(ch, "\r\n");
  }
  if ((i % 3) != 0) send_to_char(ch, "\r\n");
}

void set_craft_armor_type(struct char_data *ch, char *arg2)
{
    int i = 0;

    if (!*arg2)
    {
        send_to_char(ch, "\tCYou need to specify an armor type:\tn\r\n");
        craft_show_armor_types(ch);
        return;
    }

    for (i = 1; i < NUM_SPEC_ARMOR_TYPES; i++)
    {
        if (is_abbrev(arg2, armor_list[i].name))
            break;
    }
    
    if (i >= NUM_SPEC_ARMOR_TYPES)
    {
        send_to_char(ch, "That is not a valid armor type.\r\n");
        craft_show_armor_types(ch);    
        return;
    }

    GET_CRAFT(ch).crafting_specific = i;
    send_to_char(ch, "Crafting armor type set to: %s\r\n", armor_list[i].name);
}

void craft_show_instrument_types(struct char_data *ch)
{
  int i = 0;

  for (i = 1; i < NUM_CRAFT_INSTRUMENT_TYPES; i++)
  {
    send_to_char(ch, "%-25s ", crafting_instrument_types[i]);
    if ((i % 3) == 0) send_to_char(ch, "\r\n");
  }
  if ((i % 3) != 0) send_to_char(ch, "\r\n");
}

void set_craft_instrument_type(struct char_data *ch, char *arg2)
{
    int i = 0;

    if (!*arg2)
    {
        send_to_char(ch, "\tCYou need to specify an instrument type:\tn\r\n");
        craft_show_instrument_types(ch);
        return;
    }

    for (i = 1; i < NUM_CRAFT_INSTRUMENT_TYPES; i++)
    {
        if (is_abbrev(arg2, crafting_instrument_types[i]))
            break;
    }
    
    if (i >= NUM_CRAFT_INSTRUMENT_TYPES)
    {
        send_to_char(ch, "That is not a valid instrument type.\r\n");
        craft_show_instrument_types(ch);
        return;
    }

    GET_CRAFT(ch).crafting_specific = i;
    send_to_char(ch, "Crafting instrument type set to: %s\r\n", crafting_instrument_types[i]);
}

void craft_show_misc_types(struct char_data *ch)
{
  int i = 0;

  for (i = 1; i < NUM_CRAFT_MISC_TYPES; i++)
  {
    send_to_char(ch, "%-25s ", crafting_misc_types[i]);
    if ((i % 3) == 0) send_to_char(ch, "\r\n");
  }
  if ((i % 3) != 0) send_to_char(ch, "\r\n");
}

void set_craft_misc_type(struct char_data *ch, char *arg2)
{
    int i = 0;

    if (!*arg2)
    {
        send_to_char(ch, "\tCYou need to specify a misc type:\tn\r\n");
        craft_show_misc_types(ch);
        return;
    }

    for (i = 1; i < NUM_CRAFT_MISC_TYPES; i++)
    {
        if (is_abbrev(arg2, crafting_misc_types[i]))
            break;
    }
    
    if (i >= NUM_CRAFT_MISC_TYPES)
    {
        send_to_char(ch, "That is not a valid misc type.\r\n");
        craft_show_misc_types(ch);
        return;
    }

    GET_CRAFT(ch).crafting_specific = i;
    send_to_char(ch, "Crafting misc type set to: %s\r\n", crafting_misc_types[i]);
}

void set_crafting_keywords(struct char_data *ch, const char *arg2)
{
    if (!*arg2)
    {
        send_to_char(ch, "You need to specify the keyword list. Do not add dashes to the keywords.\r\n");
        return;
    }
    if (strstr(arg2, "-"))
    {
        send_to_char(ch, "Please do not use dashes in the keyword list, as it can affect the ability to target the object.\r\n");
        return;
    }
    if (strlen(arg2) > 100)
    {
        send_to_char(ch, "The keyword list must be less than 100 characters.\r\n");
        return;
    }
    if (GET_CRAFT(ch).crafting_item_type == 0 || GET_CRAFT(ch).crafting_specific == 0 || GET_CRAFT(ch).craft_variant == -1 || GET_CRAFT(ch).crafting_recipe == 0)
    {
        send_to_char(ch, "You must set item type, specific type and variant first.\r\n");
        return;
    }
    if (GET_CRAFT(ch).materials[ crafting_recipes[GET_CRAFT(ch).crafting_recipe].materials[0][GET_CRAFT(ch).craft_variant][0] ] [1] == 0)
    {
        send_to_char(ch, "You must add the primary material before you can set keywords.\r\n");
        return;
    }
    if (!strstr(arg2, crafting_recipes[GET_CRAFT(ch).crafting_recipe].variant_descriptions[GET_CRAFT(ch).craft_variant]) ||
        !strstr(arg2, crafting_material_descriptions[ GET_CRAFT(ch).materials[ crafting_recipes[GET_CRAFT(ch).crafting_recipe].materials[0][GET_CRAFT(ch).craft_variant][0] ] [0] ]))
    {
        send_to_char(ch, "You need to have the variant type '%s' and material type '%s' in your keyword list.\r\n", 
            crafting_recipes[GET_CRAFT(ch).crafting_recipe].variant_descriptions[GET_CRAFT(ch).craft_variant],
            crafting_material_descriptions[ GET_CRAFT(ch).materials[ crafting_recipes[GET_CRAFT(ch).crafting_recipe].materials[0][GET_CRAFT(ch).craft_variant][0] ] [0] ]
            );
        return;
    }
    GET_CRAFT(ch).keywords = strdup(arg2);
    send_to_char(ch, "You have set the keywords for your crafting item to:\r\n-- %s\r\n", arg2);
    return;
}

void set_crafting_short_desc(struct char_data *ch, const char *arg2)
{
    if (!*arg2)
    {
        send_to_char(ch, "You need to specify the object's short description.\r\n");
        return;
    }
    
    if (strlen(arg2) > 100)
    {
        send_to_char(ch, "The short description must be less than 100 characters.\r\n");
        return;
    }
    if (GET_CRAFT(ch).crafting_item_type == 0 || GET_CRAFT(ch).crafting_specific == 0 || GET_CRAFT(ch).craft_variant == -1 || GET_CRAFT(ch).crafting_recipe == 0)
    {
        send_to_char(ch, "You must set item type, specific type and variant first.\r\n");
        return;
    }
    if (GET_CRAFT(ch).materials[ crafting_recipes[GET_CRAFT(ch).crafting_recipe].materials[0][GET_CRAFT(ch).craft_variant][0] ] [1] == 0)
    {
        send_to_char(ch, "You must add the primary material before you can set short description.\r\n");
        return;
    }
    if (!strstr(arg2, crafting_recipes[GET_CRAFT(ch).crafting_recipe].variant_descriptions[GET_CRAFT(ch).craft_variant]) ||
        !strstr(arg2, crafting_material_descriptions[ GET_CRAFT(ch).materials[ crafting_recipes[GET_CRAFT(ch).crafting_recipe].materials[0][GET_CRAFT(ch).craft_variant][0] ] [0] ]))
    {
        send_to_char(ch, "You need to have the variant type '%s' and material type '%s' in your short description.\r\n", 
            crafting_recipes[GET_CRAFT(ch).crafting_recipe].variant_descriptions[GET_CRAFT(ch).craft_variant],
            crafting_material_descriptions[ GET_CRAFT(ch).materials[ crafting_recipes[GET_CRAFT(ch).crafting_recipe].materials[0][GET_CRAFT(ch).craft_variant][0] ] [0] ]
            );
        return;
    }
    GET_CRAFT(ch).short_description = strdup(arg2);
    send_to_char(ch, "You have set the short description for your crafting item to:\r\n-- %s\r\n", arg2);
    return;
}

void set_crafting_room_desc(struct char_data *ch, const char *arg2)
{
    if (!*arg2)
    {
        send_to_char(ch, "You need to specify the object's short description. This displays as the name of the item.\r\n");
        return;
    }
    
    if (strlen(arg2) > 120)
    {
        send_to_char(ch, "The room description must be less than 120 characters. This is what shows when you type 'look' in a room.\r\n");
        return;
    }
    if (GET_CRAFT(ch).crafting_item_type == 0 || GET_CRAFT(ch).crafting_specific == 0 || GET_CRAFT(ch).craft_variant == -1 || GET_CRAFT(ch).crafting_recipe == 0)
    {
        send_to_char(ch, "You must set item type, specific type and variant first.\r\n");
        return;
    }
    if (GET_CRAFT(ch).materials[ crafting_recipes[GET_CRAFT(ch).crafting_recipe].materials[0][GET_CRAFT(ch).craft_variant][0] ] [1] == 0)
    {
        send_to_char(ch, "You must add the primary material before you can set the long/room description\r\n");
        return;
    }
    if (!strstr(arg2, crafting_recipes[GET_CRAFT(ch).crafting_recipe].variant_descriptions[GET_CRAFT(ch).craft_variant]) ||
        !strstr(arg2, crafting_material_descriptions[ GET_CRAFT(ch).materials[ crafting_recipes[GET_CRAFT(ch).crafting_recipe].materials[0][GET_CRAFT(ch).craft_variant][0] ] [0] ]))
    {
        send_to_char(ch, "You need to have the variant type '%s' and material type '%s' in your long/room description.\r\n", 
            crafting_recipes[GET_CRAFT(ch).crafting_recipe].variant_descriptions[GET_CRAFT(ch).craft_variant],
            crafting_material_descriptions[ GET_CRAFT(ch).materials[ crafting_recipes[GET_CRAFT(ch).crafting_recipe].materials[0][GET_CRAFT(ch).craft_variant][0] ] [0] ]
            );
        return;
    }
    GET_CRAFT(ch).room_description = strdup(arg2);
    send_to_char(ch, "You have set the room description for your crafting item to:\r\n-- %s\r\n", arg2);
    return;
}

void set_crafting_extra_desc(struct char_data *ch, const char *arg2)
{
    if (GET_CRAFT(ch).keywords == NULL)
    {
        send_to_char(ch, "You must set the object's keywords before you can add the extra description.\r\n");
        return;
    }

    if (!*arg2)
    {
        send_to_char(ch, "You need to specify the object's extra description. This displays when you type 'look (item)'.\r\n");
        return;
    }
    
    if (strlen(arg2) > MAX_EXTRA_DESC)
    {
        send_to_char(ch, "The extra description must be less than %d characters. This is what shows when you type 'look (item)'.\r\n", MAX_EXTRA_DESC);
        return;
    }

    GET_CRAFT(ch).ex_description = strdup(arg2);
    send_to_char(ch, "You have set the extra description for your crafting item to:\r\n-- %s\r\n", arg2);
    return;
}

int get_enhancement_mote_type(struct char_data *ch, int type, int spec)
{
    switch (type)
    {
        case CRAFT_TYPE_WEAPON:
            switch (weapon_list[spec].weaponFamily)
            {
                case WEAPON_FAMILY_MONK: return CRAFTING_MOTE_AIR;
                case WEAPON_FAMILY_LIGHT_BLADE: return CRAFTING_MOTE_DARK;
                case WEAPON_FAMILY_HAMMER: return CRAFTING_MOTE_EARTH;
                case WEAPON_FAMILY_RANGED: return CRAFTING_MOTE_FIRE;
                case WEAPON_FAMILY_HEAVY_BLADE: return CRAFTING_MOTE_ICE;
                case WEAPON_FAMILY_POLEARM: return CRAFTING_MOTE_LIGHT;
                case WEAPON_FAMILY_DOUBLE: return CRAFTING_MOTE_LIGHTNING;
                case WEAPON_FAMILY_AXE: return CRAFTING_MOTE_WATER;
                default: return CRAFTING_MOTE_NONE;
            }
            break;
        case CRAFT_TYPE_ARMOR:
            switch(armor_list[spec].armorType)
            {
                case ARMOR_TYPE_HEAVY: return CRAFTING_MOTE_LIGHTNING;
                case ARMOR_TYPE_LIGHT: return CRAFTING_MOTE_FIRE;
                case ARMOR_TYPE_MEDIUM: return CRAFTING_MOTE_EARTH;
                case ARMOR_TYPE_SHIELD: return CRAFTING_MOTE_WATER;
                case ARMOR_TYPE_TOWER_SHIELD: return CRAFTING_MOTE_AIR;
                case ARMOR_TYPE_NONE: return  CRAFTING_MOTE_ICE;
                default: CRAFTING_MOTE_NONE;
            }
            break;
        default:
            return CRAFTING_MOTE_NONE;
    }
    return CRAFTING_MOTE_NONE;
}

void set_crafting_motes(struct char_data *ch, const char *argument)
{
    int slot = 0, enhancement = 0, method = 0;
    int have = 0, required = 0, mote_type = 0, allocated = 0;
    int location = 0, modifier = 0, bonus_type = 0, specific = 0;
    char arg1[100], arg2[100];

    two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

    if (!*arg1 || !*arg2)
    {
        send_to_char(ch, "%s", CRAFT_MOTE_NOARG);
        return;
    }

    // weapon / armor / shield enhancement bonus
    if (is_abbrev(arg2, "enhancement"))
    {
        enhancement = GET_CRAFT(ch).enhancement;

        if (enhancement <= 0)
        {
            send_to_char(ch, "The enhancement bonus on this project is zero. If it is an armor, shield or weapon, you can set it using 'craft enhancement'\r\n");
            return;
        }

        if (GET_CRAFT(ch).crafting_item_type == 0 || GET_CRAFT(ch).crafting_specific == 0)
        {
            send_to_char(ch, "You must set the item type and specific type before continuing.\r\n");
            return;
        }

        if (GET_CRAFT(ch).crafting_item_type != CRAFT_TYPE_ARMOR && GET_CRAFT(ch).crafting_item_type != CRAFT_TYPE_WEAPON)
        {
            send_to_char(ch, "You can only set motes for armor, shields and weapons.\r\n");
            return;
        }

        mote_type = get_enhancement_mote_type(ch, GET_CRAFT(ch).crafting_item_type, GET_CRAFT(ch).crafting_specific);
        have = GET_CRAFT_MOTES(ch, mote_type);
        required = craft_motes_required(0, 0, 0, enhancement);
        method = 1;
    }
    else if (is_abbrev(arg2, "quality"))
    {
        if (GET_CRAFT(ch).crafting_item_type != CRAFT_TYPE_INSTRUMENT)
        {
            send_to_char(ch, "You can only set motes for the quality of an instrument.\r\n");
            return;
        }

        mote_type = get_crafting_instrument_motes(ch, 1, false);
        have = GET_CRAFT_MOTES(ch, mote_type);
        required = get_crafting_instrument_motes(ch, 1, true);
        method = 3;
    }
    else if (is_abbrev(arg2, "effectiveness"))
    {
        if (GET_CRAFT(ch).crafting_item_type != CRAFT_TYPE_INSTRUMENT)
        {
            send_to_char(ch, "You can only set motes for the effectiveness of an instrument.\r\n");
            return;
        }

        mote_type = get_crafting_instrument_motes(ch, 2, false);
        have = GET_CRAFT_MOTES(ch, mote_type);
        required = get_crafting_instrument_motes(ch, 2, true);
        method = 4;
    }
    else if (is_abbrev(arg2, "breakability"))
    {
        if (GET_CRAFT(ch).crafting_item_type != CRAFT_TYPE_INSTRUMENT)
        {
            send_to_char(ch, "You can only set motes for the breakability of an instrument.\r\n");
            return;
        }

        mote_type = get_crafting_instrument_motes(ch, 3, false);
        have = GET_CRAFT_MOTES(ch, mote_type);
        required = get_crafting_instrument_motes(ch, 3, true);
        method = 5;
    }
    else
    {
        slot = atoi(arg2);

        if (slot < 1 || slot > (MAX_OBJ_AFFECT+1))
        {
            send_to_char(ch, "Please select a bonus slot between 1 and 6.\r\n");
            return;
        }

        // bonus array is 0-5, but for user simplicity we have them enter in 1-6
        slot--;

        if (!is_valid_apply(GET_CRAFT(ch).affected[slot].location) || GET_CRAFT(ch).affected[slot].modifier == 0)
        {
            send_to_char(ch, "There is no bonus set in that slot. Type craft show or help craft-bonuses for more info.\r\n");
            return;
        }

        location = GET_CRAFT(ch).affected[slot].location;
        modifier = GET_CRAFT(ch).affected[slot].modifier;
        bonus_type = GET_CRAFT(ch).affected[slot].bonus_type;
        specific = GET_CRAFT(ch).affected[slot].specific;

        mote_type = crafting_mote_by_bonus_location(location, specific, bonus_type);

        have = GET_CRAFT_MOTES(ch, mote_type);
        required = craft_motes_required(location, modifier, bonus_type, 0);

        method = 2;
    }

    if (is_abbrev(arg1, "add"))
    {
        if (GET_CRAFT(ch).enhancement_motes_required > 0 && method == 1)
        {
            send_to_char(ch, "You have already assigned motes for the enhancement bonus.\r\n");
            return;
        }
        else if (GET_CRAFT(ch).instrument_motes[1] > 0 && method == 3)
        {
            send_to_char(ch, "You have already assigned motes for the instrument quality.\r\n");
            return;
        }
        else if (GET_CRAFT(ch).instrument_motes[2] > 0 && method == 4)
        {
            send_to_char(ch, "You have already assigned motes for the instrument effectiveness.\r\n");
            return;
        }
        else if (GET_CRAFT(ch).instrument_motes[3] > 0 && method == 5)
        {
            send_to_char(ch, "You have already assigned motes for the instrument breakability.\r\n");
            return;
        }
        else if (GET_CRAFT(ch).motes_required[slot] > 0 && method == 2)
        {
            send_to_char(ch, "You have already assigned motes for bonus slot %d.\r\n", slot+1);
            return;
        }
        if (have < required)
        {
            send_to_char(ch, "You require %d %ss, but only have %d.\r\n", required, crafting_motes[mote_type], have);
            return;
        }
        if (method == 2)
            GET_CRAFT(ch).motes_required[slot] = required;
        else if (method == 3)
            GET_CRAFT(ch).instrument_motes[1] = required;
        else if (method == 4)
            GET_CRAFT(ch).instrument_motes[2] = required;
        else if (method == 5)
            GET_CRAFT(ch).instrument_motes[3] = required;
        else
            GET_CRAFT(ch).enhancement_motes_required = required;
        GET_CRAFT_MOTES(ch, mote_type) -= required;
        send_to_char(ch, "You assign %d %ss to your project. Type craft show to review your projects.\r\n", required, crafting_motes[mote_type]);
    }
    else if (is_abbrev(arg1, "remove"))
    {
        if (method == 2)
        {
            allocated = GET_CRAFT(ch).motes_required[slot];
            if (allocated <= 0)
            {
                send_to_char(ch, "There are no motes assigned to that bonus slot yet.\r\n");
                return;
            }
            GET_CRAFT_MOTES(ch, mote_type) += allocated;
            GET_CRAFT(ch).motes_required[slot] = 0;
        }
        if (method == 3)
        {
            allocated = GET_CRAFT(ch).instrument_motes[1];
            if (allocated <= 0)
            {
                send_to_char(ch, "There are no motes assigned for your instrument quality yet.\r\n");
                return;
            }
            GET_CRAFT_MOTES(ch, mote_type) += allocated;
            GET_CRAFT(ch).instrument_motes[1] = 0;
        }
        else if (method == 4)
        {
            allocated = GET_CRAFT(ch).instrument_motes[2];
            if (allocated <= 0)
            {
                send_to_char(ch, "There are no motes assigned for your instrument effectiveness yet.\r\n");
                return;
            }
            GET_CRAFT_MOTES(ch, mote_type) += allocated;
            GET_CRAFT(ch).instrument_motes[2] = 0;
        }
        else if (method == 5)
        {
            allocated = GET_CRAFT(ch).instrument_motes[3];
            if (allocated <= 0)
            {
                send_to_char(ch, "There are no motes assigned for your instrument breakability yet.\r\n");
                return;
            }
            GET_CRAFT_MOTES(ch, mote_type) += allocated;
            GET_CRAFT(ch).instrument_motes[3] = 0;
        }
        else
        {
            allocated = GET_CRAFT(ch).enhancement_motes_required;
            if (allocated <= 0)
            {
                send_to_char(ch, "There are no motes assigned for your item enhancement yet.\r\n");
                return;
            }
            GET_CRAFT_MOTES(ch, mote_type) += allocated;
            GET_CRAFT(ch).enhancement_motes_required = 0;
        }            
        send_to_char(ch, "You recover %d %ss from your project. Type craft show to review your projects.\r\n", required, crafting_motes[mote_type]);
    }
    else
    {
        send_to_char(ch, "%s", CRAFT_MOTE_NOARG);
    }
}

int get_crafting_instrument_dc_modifier(struct char_data *ch)
{
    int dc_mod = 0;
    int quality = GET_CRAFT(ch).instrument_quality;
    int effectiveness = GET_CRAFT(ch).instrument_effectiveness;
    int breakability = GET_CRAFT(ch).instrument_breakability;

    dc_mod += quality / 3;
    dc_mod += effectiveness;
    if (breakability == 0)
        dc_mod += 10;
    else
        dc_mod += (INSTRUMENT_BREAKABILITY_DEFAULT - breakability) / 5;

    return dc_mod;
}

// type is the object value associated with the query: 1 = quality, 2 = effectiveness, 3 = breakability
// get_amount is false if you want to get the type of mote, or true if you want to get the number of motes
int get_crafting_instrument_motes(struct char_data *ch, int type, bool get_amount)
{
    switch (type)
    {
        case 1: // quality
            if (!get_amount)
            {
                return CRAFTING_MOTE_AIR;
            }
            else
            {
                return GET_CRAFT(ch).instrument_quality / 3; // 1-30 quality, so 0-10 motes
            }
            break;
        case 2: // effectiveness
            if (!get_amount)
            {
                return CRAFTING_MOTE_WATER;
            }
            else
            {
                return GET_CRAFT(ch).instrument_effectiveness; // 1-10 effectiveness, so 1-10 motes
            }
            break;
        case 3: // breakability
            if (!get_amount)
            {
                return CRAFTING_MOTE_EARTH;
            }
            else
            {
                if (GET_CRAFT(ch).instrument_breakability == 0)
                    return 15; // 0 breakability means unbreakable, so 15 motes
                else
                    return (INSTRUMENT_BREAKABILITY_DEFAULT - GET_CRAFT(ch).instrument_breakability) / 5; // 0-30 breakability, so 0-6 motes
            }
            break;
    }
    return 0;
}

void set_crafting_instrument(struct char_data *ch, char *arg2)
{
    int value = 0;
    char arg3[200], arg4[200];

    if (GET_CRAFT(ch).craft_variant == -1)
    {
        send_to_char(ch, "You must set the crafting variant first.\r\n");
        return;
    }

    two_arguments(arg2, arg3, sizeof(arg3), arg4, sizeof(arg4));

    if (!*arg3 || !*arg4)
    {
        send_to_char(ch, "Please enter one of the following:\r\n"
                         "-- craft instrument quality [1-30]\r\n"
                         "-- craft instrument effectiveness [1-10]\r\n"
                         "-- craft instrument breakability [0-%d]\r\n", INSTRUMENT_BREAKABILITY_DEFAULT);
        return;
    }

    // intrument type val0 - this will be set upon craft instrument complete

    if (is_abbrev(arg3, "quality"))
    {
        value = atoi(arg4);
        if (value < 1 || value > 30)
        {
            send_to_char(ch, "The quality must be between 1 and 30.\r\n"
                             "This will reduce the DC of the instrument performance by that amount.\r\n");
            return;
        }
        GET_CRAFT(ch).instrument_quality = value;
        send_to_char(ch, "You set the instrument quality to %d.\r\n", value);
    }
    else if (is_abbrev(arg3, "effectiveness"))
    {
        value = atoi(arg4);
        if (value < 1 || value > 10)
        {
            send_to_char(ch, "The effectiveness must be between 1 and 10.\r\n"
                             "This will increase the effect of the instrument performance by that amount.\r\n");
            return;
        }
        GET_CRAFT(ch).instrument_effectiveness = value;
        send_to_char(ch, "You set the instrument effectiveness to %d.\r\n", value);
        return;
    }
    else if (is_abbrev(arg3, "breakability"))
    {
        value = atoi(arg4);
        if (value < 0 || value > INSTRUMENT_BREAKABILITY_DEFAULT)
        {
            send_to_char(ch, "The breakability must be between 0 and %d.\r\n"
                             "This will set the chance of breaking the instrument by that amount in 11,111.\r\n"
                             "Ie. breakability 0 means unbreakable, 1 means 1 in 11,111 chance to break, 30 means 30 in 11,111 chance to break.\r\n", INSTRUMENT_BREAKABILITY_DEFAULT);
            return;
        }
        GET_CRAFT(ch).instrument_breakability = value;
        send_to_char(ch, "You set the instrument breakability to %d.\r\n", value);
        return;
    }
    else
    {
        send_to_char(ch, "You need to specify one of the following:\r\n"
                         "-- craft instrument quality [1-30]\r\n"
                         "-- craft instrument effectiveness [1-10]\r\n"
                         "-- craft instrument breakability [0-%d]\r\n", INSTRUMENT_BREAKABILITY_DEFAULT);
        return;
    }
}

int get_craft_level_adjust_dc_change(int adjust)
{
    return -(adjust * 5);
}

void set_craft_level_adjust(struct char_data *ch, char *arg2)
{
    int adjust = 0;

    if (!*arg2)
    {
        send_to_char(ch, "You need to specify the level adjustment for the crafting item. Each -1 to the final object level adds +5 to the craft dc, and vice versa for increasing object level.\r\n");
        return;
    }

    adjust = atoi(arg2);

    send_to_char(ch, "You've set the crafting level adjustment to %d.\r\n", adjust);
    send_to_char(ch, "This will adjust the final object level by %s%d and the dc will change by %s%d.\r\n", adjust > 0 ? "+" : "", adjust, get_craft_level_adjust_dc_change(adjust) > 0 ? "+" : "", get_craft_level_adjust_dc_change(adjust));
    GET_CRAFT(ch).level_adjust = adjust;
}

void set_crafting_bonuses(struct char_data *ch, const char *argument)
{
    char arg1[100], // bonus slot (0-5)
         arg2[100], // bonus location
         arg3[100], // bonus type (enhancement or universal)
         arg4[100], // bonus modifier
         arg5[100], // bonus specific
         temp[100],
         spectext[100];
    int i = 0, j = 0,
        slot = 0,
        location = 0,
        modifier = 0, max_modifier = 0,
        bonus_type = 0,
        specific = 0,
        cr_type = 0,
        cr_spec_type = 0,
        cr_variant = 0,
        cr_recipe = -1,
        wear_loc = 0;

    if (!ch)
        return;

    cr_type = GET_CRAFT(ch).crafting_item_type;
    cr_spec_type = GET_CRAFT(ch).crafting_specific;
    cr_variant = GET_CRAFT(ch).craft_variant;
    cr_recipe = GET_CRAFT(ch).crafting_recipe;

    if (!cr_type || !cr_spec_type || !cr_variant || cr_recipe == -1)
    {
        send_to_char(ch, "You must set the following before you can apply bonuses.\r\n"
                         "-- craft type [weapon, armor, shield, instrument, misc]\r\n"
                         "-- craft specific [weapon type, armor type, instrument type, misc type]\r\n"
                         "-- craft variant [variant name]\r\n"
                         "-- craft recipe [recipe name]\r\n");
        return;
    }

    if (!*argument)
    {
        send_to_char(ch, "%s", NEWCRAFT_CREATE_BONUSES_NOARG);
        return;
    }

    five_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2), arg3, sizeof(arg3), arg4, sizeof(arg4), arg5, sizeof(arg5));

    if (!*arg1)
    {
        send_to_char(ch, "%s", NEWCRAFT_CREATE_BONUSES_NOARG);
        return;
    }

    // determine bonus slot
    slot = atoi(arg1);

    if (slot < 1 || slot > 6)
    {
        send_to_char(ch, "The slot must be between 1 and 6.\r\n");
        return;
    }
    
    slot--; // array is 0-5

    if (*arg2 && is_abbrev(arg2, "reset"))
    {
        location = GET_CRAFT(ch).affected[slot].location;
        bonus_type = GET_CRAFT(ch).affected[slot].bonus_type;
        modifier = GET_CRAFT(ch).affected[slot].modifier;
        specific = GET_CRAFT(ch).affected[slot].specific;
        int mote_type = crafting_mote_by_bonus_location(location, specific, bonus_type);
        int num_motes = GET_CRAFT(ch).motes_required[slot];
        send_to_char(ch, "You've reset the bonus in slot %d.\r\n", slot+1);
        if (num_motes > 0 && mote_type != CRAFTING_MOTE_NONE)
        {
            GET_CRAFT_MOTES(ch, mote_type) += num_motes;
            send_to_char(ch, "You've recovered %d %s.\r\n", num_motes, crafting_motes[mote_type]);
        }
        GET_CRAFT(ch).affected[slot].location = 0;
        GET_CRAFT(ch).affected[slot].bonus_type = 0;
        GET_CRAFT(ch).affected[slot].modifier = 0;
        GET_CRAFT(ch).affected[slot].specific = 0;
        GET_CRAFT(ch).motes_required[slot] = 0;
        return;
    }

    if (!*arg1 || !*arg2 || !*arg3 || !*arg4)
    {
        send_to_char(ch, "%s", NEWCRAFT_CREATE_BONUSES_NOARG);
        return;
    }

    // determine bonus location

    if (GET_CRAFT(ch).affected[slot].location != APPLY_NONE)
    {
        send_to_char(ch, "You have already set the apply type for this bonus slot. You'll have to do 'craft bonus %d reset' to change it.\r\n", slot+1);
        return;
    }

    for (i = 1; i < NUM_APPLIES; i++)
    {
        if (!is_valid_apply(i)) continue;
        snprintf(temp, sizeof(temp), "%s", apply_types[i]);
        for (j = 0; j < strlen(temp); j++)
        {
            temp[j] = tolower(temp[j]);
        }
        if (is_abbrev(arg2, temp))
            break;
    }

    if (i >= NUM_APPLIES)
    {
        send_to_char(ch, "You need to specify a valid bonus location. Type 'applies' for a list.\r\n");
        return;
    }

    location = i;

    wear_loc = get_craft_wear_loc(ch);

    if (!is_bonus_valid_for_where_slot(location, wear_loc))
    {
        send_to_char(ch, "You cannot set a bonus of type %s on a %s.\r\n", apply_types[location], wear_bits[wear_loc]);
        send_to_char(ch, "You can see valid bonus types for wear slots by typing: wearapplies.\r\n");
        return;
    }

    if (GET_CRAFT(ch).affected[slot].bonus_type != BONUS_TYPE_UNDEFINED)
    {
        send_to_char(ch, "You have already set the bonus type for this bonus slot. You'll have to do 'craft bonus %d reset' to change it.\r\n", slot+1);
        return;
    }

    // determine bonus type
    for (i = 0; i < NUM_BONUS_TYPES; i++)
    {
        snprintf(temp, sizeof(temp), "%s", bonus_types[i]);
        for (j = 0; j < strlen(temp); j++)
        {
            temp[j] = tolower(temp[j]);
        }
        if (is_abbrev(arg3, temp))
            break;
    }

    if (i >= NUM_BONUS_TYPES)
    {
        send_to_char(ch, "You need to specify a valid bonus type. Valid bonus types are:\r\n"
                         "Armor Class: Enhancement, Deflection, Natural, Universal\r\n"
                         "Other Bonuses: Enhancement, Universal\r\n"
                         "Universal bonuses are divided by 3, since it stacks.\r\n");
        return;
    }

    bonus_type = i;

    switch (location)
    {
        case APPLY_AC_NEW:
            switch (bonus_type)
            {
                case BONUS_TYPE_ENHANCEMENT:
                case BONUS_TYPE_DEFLECTION:
                case BONUS_TYPE_NATURALARMOR:
                case BONUS_TYPE_UNIVERSAL:
                    break;
                default:
                    send_to_char(ch, "Armor class bonus types are restricted to: enhancement, natural, deflection and universal.\r\n");
                    return;
            }
            break;
        default:
            switch (bonus_type)
            {
                case BONUS_TYPE_ENHANCEMENT:
                case BONUS_TYPE_UNIVERSAL:
                    break;
                default:
                    send_to_char(ch, "Bonus types are restricted to: enhancement and universal.\r\n");
                    return;
            }
            break;
    }

    if (GET_CRAFT(ch).affected[slot].modifier != 0)
    {
        send_to_char(ch, "You have already set the stat modifier for this bonus slot. You'll have to do 'craft bonus %d reset' to change it.\r\n", slot+1);
        return;
    }

    // determine bonus modifier
    modifier = atoi(arg4);

    if (modifier < 1)
    {
        send_to_char(ch, "Bonus modifier must be greater than zero.\r\n");
        return;
    }

    max_modifier = get_gear_bonus_amount_by_level(location, 30);

    if (bonus_type == BONUS_TYPE_ENHANCEMENT)
        max_modifier *= 2;

    if (modifier > max_modifier)
    {
        send_to_char(ch, "The max bonus modifier for %s of type (%s) is %d.\r\n", 
                     apply_types[location], bonus_types[bonus_type], max_modifier);
        return;
    }

    if (does_craft_apply_type_have_specific_value(location) && GET_CRAFT(ch).affected[slot].specific != 0)
    {
        send_to_char(ch, "You have already set the specific modifier for this bonus slot. You'll have to do 'craft bonus %d reset' to change it.\r\n", slot+1);
        return;
    }

    // determine specifier for certain bonus locations
    snprintf(spectext, sizeof(spectext), "N/A");

    if (*arg5)
    {
        switch (location)
        {
            case APPLY_SKILL:
                for (i = START_GENERAL_ABILITIES; i <= NUM_ABILITIES; i++)
                {
                    if (!is_valid_craft_ability(i)) continue;
                    snprintf(temp, sizeof(temp), "%s", ability_names[i]);
                    for (j = 0; j < strlen(temp); j++)
                    {
                        temp[j] = tolower(temp[j]);
                    }
                    if (is_abbrev(arg5, temp))
                        break;
                }
                if (i > NUM_ABILITIES)
                {
                    send_to_char(ch, "That is not a valid skill. For a list of skills type: skills and craftskills\r\n");
                    return;
                }
                specific = i;
                snprintf(spectext, sizeof(spectext), "%s", ability_names[specific]);
                break;
            case APPLY_FEAT:
                for (i = 1; i < FEAT_LAST_FEAT; i++)
                {
                    if (!is_valid_craft_feat(i)) continue;
                    snprintf(temp, sizeof(temp), "%s", feat_list[i].name);
                    for (j = 0; j < strlen(temp); j++)
                    {
                        temp[j] = tolower(temp[j]);
                    }
                    if (is_abbrev(arg5, temp))
                        break;
                }
                if (i >= FEAT_LAST_FEAT)
                {
                    send_to_char(ch, "That is not a valid feat. For a list of feats type: feats all\r\n"
                                     "Note that not all feats are allowed on gear. A general rule is:\r\n"
                                     "-- not epic, not a class or race feat, doesn't require a specific subtype\r\n"
                                     "-- such as weapon focus or skill focus, is of type general, combat, spellcasting,\r\n"
                                     "psionic, metamagic and teamwork.\r\n");
                    return;
                }
                specific = i;
                snprintf(spectext, sizeof(spectext), "%s", feat_list[specific].name);
                break;
            case APPLY_SPELL_CIRCLE_1:
            case APPLY_SPELL_CIRCLE_2:
            case APPLY_SPELL_CIRCLE_3:
            case APPLY_SPELL_CIRCLE_4:
            case APPLY_SPELL_CIRCLE_5:
            case APPLY_SPELL_CIRCLE_6:
            case APPLY_SPELL_CIRCLE_7:
            case APPLY_SPELL_CIRCLE_8:
            case APPLY_SPELL_CIRCLE_9:
                for (i = CLASS_WIZARD; i < NUM_CLASSES; i++)
                {
                    if (!is_valid_craft_class(i, location)) continue;
                    snprintf(temp, sizeof(temp), "%s", class_list[i].name);
                    for (j = 0; j < strlen(temp); j++)
                    {
                        temp[j] = tolower(temp[j]);
                    }
                    if (is_abbrev(arg5, temp))
                        break;
                }
                if (i >= NUM_CLASSES)
                {
                    send_to_char(ch, "That is either not a valid spellcasting class, or the spell slot is too high for that class.\r\n");
                    return;
                }
                specific = i;
                snprintf(spectext, sizeof(spectext), "%s", class_list[specific].name);
                break;
            default:
                break;
        }
    }
    else
    {
        specific = 0;
    }

    // ok we have our info, let's start assigning it
    send_to_char(ch, "You have set your crafting object's affect in slot %d to the following:\r\n"
                     "-- Bonus Location: %s\r\n"
                     "-- Bonus Type    : %s\r\n"
                     "-- Bonus Modifier: +%d\r\n"
                     "-- Bonus Specific: %s\r\n",
                     slot, apply_types[location], bonus_types[bonus_type], modifier, spectext);

    GET_CRAFT(ch).affected[slot].location = location;
    GET_CRAFT(ch).affected[slot].bonus_type = bonus_type;
    GET_CRAFT(ch).affected[slot].modifier = modifier;
    GET_CRAFT(ch).affected[slot].specific = specific;
}

int craft_motes_required(int location, int modifier, int bonus_type, int enhancement)
{
    int level;

    if (enhancement)
        level = get_level_adjustment_by_enhancement_bonus(enhancement);
    else
        level = get_level_adjustment_by_apply_and_modifier(location, modifier, bonus_type);

    if (level >= 30)
        return CRAFT_MOTES_REQ_30;
    else if (level >= 25)
        return CRAFT_MOTES_REQ_25;
    if (level >= 20)
        return CRAFT_MOTES_REQ_20;
    if (level >= 15)
        return CRAFT_MOTES_REQ_15;
    if (level >= 10)
        return CRAFT_MOTES_REQ_10;
    if (level >= 5)
        return CRAFT_MOTES_REQ_5;
    else
        return CRAFT_MOTES_REQ_1;
}

int crafting_mote_by_bonus_location(int location, int specific, int bonus_type)
{
    switch (location)
    {
        case APPLY_STR:
        case APPLY_HIT:
        case APPLY_RES_FIRE:
        case APPLY_RES_SLICE:
        case APPLY_HP_REGEN:
        case APPLY_SPELL_PENETRATION:
            return CRAFTING_MOTE_FIRE;
        case APPLY_DEX:
        case APPLY_SAVING_REFL:
        case APPLY_RES_ELECTRIC:
        case APPLY_RES_SOUND:
        case APPLY_INITIATIVE:
        case APPLY_SPELL_DURATION:
            return CRAFTING_MOTE_LIGHTNING;
        case APPLY_INT:
        case APPLY_PSP:
        case APPLY_RES_COLD:
        case APPLY_POWER_RES:
        case APPLY_PSP_REGEN:
        case APPLY_SPELL_CIRCLE_1:
        case APPLY_SPELL_CIRCLE_2:
        case APPLY_SPELL_CIRCLE_3:
        case APPLY_SPELL_CIRCLE_4:
        case APPLY_SPELL_CIRCLE_5:
        case APPLY_SPELL_CIRCLE_6:
        case APPLY_SPELL_CIRCLE_7:
        case APPLY_SPELL_CIRCLE_8:
        case APPLY_SPELL_CIRCLE_9:
            return CRAFTING_MOTE_ICE;
        case APPLY_WIS:
        case APPLY_SAVING_WILL:
        case APPLY_RES_PUNCTURE:
        case APPLY_RES_POISON:
        case APPLY_RES_WATER:
        case APPLY_ENCUMBRANCE:
            return CRAFTING_MOTE_WATER;
        case APPLY_CON:
        case APPLY_MOVE:
        case APPLY_SAVING_FORT:
        case APPLY_RES_EARTH:
        case APPLY_RES_ACID:
        case APPLY_MV_REGEN:
            return CRAFTING_MOTE_EARTH;
        case APPLY_CHA:
        case APPLY_RES_AIR:
        case APPLY_RES_FORCE:
        case APPLY_RES_ILLUSION:
        case APPLY_RES_ENERGY:
        case APPLY_FAST_HEALING:
            return CRAFTING_MOTE_AIR;
        case APPLY_HITROLL:
        case APPLY_SPELL_RES:
        case APPLY_RES_HOLY:
        case APPLY_RES_MENTAL:
        case APPLY_RES_LIGHT:
        case APPLY_SPELL_DC:
            return CRAFTING_MOTE_LIGHT;
        case APPLY_DAMROLL:
        case APPLY_RES_UNHOLY:
        case APPLY_RES_DISEASE:
        case APPLY_RES_NEGATIVE:
        case APPLY_SPELL_POTENCY:
            return CRAFTING_MOTE_DARK;
        
        case APPLY_AC_NEW:
            switch (bonus_type)
            {
                case BONUS_TYPE_DEFLECTION:
                    return CRAFTING_MOTE_FIRE;
                case BONUS_TYPE_NATURALARMOR:
                    return CRAFTING_MOTE_EARTH;
                case BONUS_TYPE_DODGE:
                    return CRAFTING_MOTE_LIGHTNING;
            }
            break;

        case APPLY_SKILL:
            switch (specific)
            {
                case ABILITY_ACROBATICS:
                case ABILITY_STEALTH:
                case ABILITY_RIDE:
                case ABILITY_SLEIGHT_OF_HAND:
                case ABILITY_DISABLE_DEVICE:
                    return CRAFTING_MOTE_LIGHTNING;
                case ABILITY_RELIGION:
                case ABILITY_MEDICINE:
                case ABILITY_SPELLCRAFT:
                case ABILITY_APPRAISE:
                case ABILITY_ARCANA:
                case ABILITY_HISTORY:
                case ABILITY_NATURE:
                    return CRAFTING_MOTE_ICE;
                case ABILITY_PERCEPTION:
                case ABILITY_DISCIPLINE:
                case ABILITY_HANDLE_ANIMAL:
                case ABILITY_INSIGHT:
                    return CRAFTING_MOTE_WATER;
                case ABILITY_ATHLETICS:
                    return CRAFTING_MOTE_FIRE;
                case ABILITY_CONCENTRATION:
                case ABILITY_TOTAL_DEFENSE:
                    return CRAFTING_MOTE_EARTH;
                case ABILITY_INTIMIDATE:
                case ABILITY_DECEPTION:
                case ABILITY_PERSUASION:
                case ABILITY_DISGUISE:
                case ABILITY_USE_MAGIC_DEVICE:
                case ABILITY_PERFORM:
                    return CRAFTING_MOTE_AIR;
            }
            break;

    }
    return CRAFTING_MOTE_NONE;
};

void show_current_craft(struct char_data *ch)
{

    char spec_item_type[100];
    char temp[MAX_EXTRA_DESC];
    char extra_desc[MAX_EXTRA_DESC+5];
    char spectext[100];
    int i = 0;
    bool found = false;
    int base_group= 0;
    int base_amount = 0;
    int project_material = 0;
    int project_amount = 0;
    int skill = 0, dc = 0, spec_type = 0;

    snprintf(extra_desc, sizeof(extra_desc), " ");

    if (GET_CRAFT(ch).ex_description != NULL)
    {
        snprintf(temp, sizeof(temp), "%s", GET_CRAFT(ch).ex_description);
        snprintf(extra_desc, sizeof(extra_desc), "\r\n%s", strfrmt(temp, 80, 1, FALSE, FALSE, FALSE));
    }

    snprintf(spec_item_type, sizeof(spec_item_type), " ");

    switch (GET_CRAFT(ch).crafting_item_type)
    {
        case CRAFT_TYPE_WEAPON:
            snprintf(spec_item_type, sizeof(spec_item_type), "%s", weapon_list[GET_CRAFT(ch).crafting_specific].name);
            break;
        case CRAFT_TYPE_ARMOR:
            snprintf(spec_item_type, sizeof(spec_item_type), "%s", armor_list[GET_CRAFT(ch).crafting_specific].name);
            break;
        case CRAFT_TYPE_INSTRUMENT:
            snprintf(spec_item_type, sizeof(spec_item_type), "%s", crafting_instrument_types[GET_CRAFT(ch).crafting_specific]);
            break;
        case CRAFT_TYPE_MISC:
            snprintf(spec_item_type, sizeof(spec_item_type), "%s", crafting_misc_types[GET_CRAFT(ch).crafting_specific]);
            break;
    }

    send_to_char(ch, "\tCCurrent Craft Project:\tn\r\n");
    send_to_char(ch, "\tc   GENERAL INFO\tn\r\n");
    send_to_char(ch, "-- craft type: %s\r\n", crafting_types[GET_CRAFT(ch).crafting_item_type]);
    send_to_char(ch, "-- item type : %s\r\n", spec_item_type);
    if (GET_CRAFT(ch).crafting_item_type && GET_CRAFT(ch).crafting_specific && GET_CRAFT(ch).crafting_recipe)
        send_to_char(ch, "-- variant   : %s\r\n", GET_CRAFT(ch).craft_variant == -1 ? "not-selected" : crafting_recipes[GET_CRAFT(ch).crafting_recipe].variant_descriptions[GET_CRAFT(ch).craft_variant]);
    send_to_char(ch, "\r\n");
    send_to_char(ch, "\tc   DESCRIPTIONS: \tn\r\n");
    // send_to_char(ch, "-- keywords  : %s\r\n", GET_CRAFT(ch).keywords != NULL ? (strlen(GET_CRAFT(ch).keywords) < 5 ? " " : GET_CRAFT(ch).keywords) : " ");
    send_to_char(ch, "-- keywords  : %s\r\n", (!GET_CRAFT(ch).keywords || is_abbrev(GET_CRAFT(ch).keywords, "(null)")) ? "not set" : GET_CRAFT(ch).keywords);
    send_to_char(ch, "-- short desc: %s\r\n", (!GET_CRAFT(ch).short_description || is_abbrev(GET_CRAFT(ch).short_description, "(null)")) ? "not set" : GET_CRAFT(ch).short_description);
    send_to_char(ch, "-- room desc : %s\r\n", (!GET_CRAFT(ch).room_description || is_abbrev(GET_CRAFT(ch).room_description, "(null)")) ? "not set" : GET_CRAFT(ch).room_description);
    send_to_char(ch, "-- extra desc: %s\r\n", (strstr(extra_desc, "(null)")) ? "not set" : extra_desc);
    send_to_char(ch, "\r\n");
    send_to_char(ch, "\tc   MATERIALS: \tn\r\n");
    if (GET_CRAFT(ch).crafting_item_type && GET_CRAFT(ch).crafting_specific &&  GET_CRAFT(ch).craft_variant != -1 && GET_CRAFT(ch).crafting_recipe)
    {
        base_group = crafting_recipes[GET_CRAFT(ch).crafting_recipe].materials[0][GET_CRAFT(ch).craft_variant][0];
        base_amount = crafting_recipes[GET_CRAFT(ch).crafting_recipe].materials[0][GET_CRAFT(ch).craft_variant][1];
        project_material = GET_CRAFT(ch).materials[base_group][0];
        project_amount = GET_CRAFT(ch).materials[base_group][1];
        if (base_group != CRAFT_GROUP_NONE)
        {
            send_to_char(ch, "-- %d %-12s: %d %s allocated (%s%d level adjustment)\r\n", 
                base_amount, crafting_material_groups[base_group], project_amount, project_material ? crafting_materials[project_material] : "units",
                -craft_material_level_adjustment(project_material) > 0 ? "+" : "", -craft_material_level_adjustment(project_material));
        }
        base_group = crafting_recipes[GET_CRAFT(ch).crafting_recipe].materials[1][GET_CRAFT(ch).craft_variant][0];
        base_amount = crafting_recipes[GET_CRAFT(ch).crafting_recipe].materials[1][GET_CRAFT(ch).craft_variant][1];
        project_material = GET_CRAFT(ch).materials[base_group][0];
        project_amount = GET_CRAFT(ch).materials[base_group][1];
        if (base_group != CRAFT_GROUP_NONE)
        {
            send_to_char(ch, "-- %d %-12s: %d %s allocated (%s%d level adjustment)\r\n", 
                base_amount, crafting_material_groups[base_group], project_amount, project_material ? crafting_materials[project_material] : "units",
-                -craft_material_level_adjustment(project_material) > 0 ? "+" : "", -craft_material_level_adjustment(project_material));
        }
        base_group = crafting_recipes[GET_CRAFT(ch).crafting_recipe].materials[2][GET_CRAFT(ch).craft_variant][0];
        base_amount = crafting_recipes[GET_CRAFT(ch).crafting_recipe].materials[2][GET_CRAFT(ch).craft_variant][1];
        project_material = GET_CRAFT(ch).materials[base_group][0];
        project_amount = GET_CRAFT(ch).materials[base_group][1];
        if (base_group != CRAFT_GROUP_NONE)
        {
            send_to_char(ch, "-- %d %-12s: %d %s allocated (%s%d level adjustment)\r\n", 
                base_amount, crafting_material_groups[base_group], project_amount, project_material ? crafting_materials[project_material] : "units",
                -craft_material_level_adjustment(project_material) > 0 ? "+" : "", -craft_material_level_adjustment(project_material));
        }
        send_to_char(ch, "-- Final level adjustment for material quality: %s%d. (average of all materials)\r\n", 
                get_craft_material_final_level_adjustment(ch) > 0 ? "+" : "", get_craft_material_final_level_adjustment(ch));
    }
    else
    {
        send_to_char(ch, "-- You must set craft type, item type and variant type to view materials.\r\n");
    }

    if (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_INSTRUMENT)
    {
        send_to_char(ch, "\r\n");
        send_to_char(ch, "\tc   INSTRUMENT INFO:\tn\r\n");
        send_to_char(ch, "-- quality       : %d (motes: %2d/%2d %s%s)\r\n", GET_CRAFT(ch).instrument_quality, GET_CRAFT(ch).instrument_motes[1],
            get_crafting_instrument_motes(ch, 1, true), crafting_motes[get_crafting_instrument_motes(ch, 1, false)], get_crafting_instrument_motes(ch, 1, true) == 1 ? "" : "s");
        send_to_char(ch, "-- effectiveness : %d (motes: %2d/%2d %s%s)\r\n", GET_CRAFT(ch).instrument_effectiveness,  GET_CRAFT(ch).instrument_motes[2],
            get_crafting_instrument_motes(ch, 2, true), crafting_motes[get_crafting_instrument_motes(ch, 2, false)], get_crafting_instrument_motes(ch, 1, true) == 2 ? "" : "s");
        send_to_char(ch, "-- breakability  : %d (motes: %2d/%2d %s%s)\r\n", GET_CRAFT(ch).instrument_breakability,  GET_CRAFT(ch).instrument_motes[3],
            get_crafting_instrument_motes(ch, 3, true), crafting_motes[get_crafting_instrument_motes(ch, 3, false)], get_crafting_instrument_motes(ch, 1, true) == 3 ? "" : "s");
    }

    if (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_WEAPON || GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_ARMOR)
    {
        send_to_char(ch, "\r\n");
        send_to_char(ch, "\tc   ENHANCEMENT BONUS:\tn %s%d", GET_CRAFT(ch).enhancement > 0 ? "+" : "", GET_CRAFT(ch).enhancement);
        if (GET_CRAFT(ch).enhancement > 0)
            send_to_char(ch, " %d/%d %ss required", GET_CRAFT(ch).enhancement_motes_required,  craft_motes_required(0, 0, 0, GET_CRAFT(ch).enhancement), crafting_motes[get_enhancement_mote_type(ch, GET_CRAFT(ch).crafting_item_type, GET_CRAFT(ch).crafting_specific)]);
        send_to_char(ch, "\r\n");
        send_to_char(ch, "\r\n");
    }
    send_to_char(ch, "\r\n");
    send_to_char(ch, "\tc   BONUSES:\tn\r\n");

    for (i = 0; i < 6; i++)
    {
        if (GET_CRAFT(ch).affected[i].location != APPLY_NONE)
        {
            found = true;
            snprintf(spectext, sizeof(spectext), " ");
            switch (GET_CRAFT(ch).affected[i].location)
            {
                case APPLY_FEAT:
                    snprintf(spectext, sizeof(spectext), " [%s]", feat_list[GET_CRAFT(ch).affected[i].specific].name);
                    break;
                case APPLY_SKILL:
                    snprintf(spectext, sizeof(spectext), " [%s]", ability_names[GET_CRAFT(ch).affected[i].specific]);
                    break;
                case APPLY_SPELL_CIRCLE_1:
                case APPLY_SPELL_CIRCLE_2:
                case APPLY_SPELL_CIRCLE_3:
                case APPLY_SPELL_CIRCLE_4:
                case APPLY_SPELL_CIRCLE_5:
                case APPLY_SPELL_CIRCLE_6:
                case APPLY_SPELL_CIRCLE_7:
                case APPLY_SPELL_CIRCLE_8:
                case APPLY_SPELL_CIRCLE_9:
                    snprintf(spectext, sizeof(spectext), " [%s]", class_list[GET_CRAFT(ch).affected[i].specific].name);
                    break;
            }
            send_to_char(ch, "-- slot %d: +%d to %s%s (%s) %d/%d %ss required.\r\n", i+1, GET_CRAFT(ch).affected[i].modifier, apply_types[GET_CRAFT(ch).affected[i].location],
                         spectext, bonus_types[GET_CRAFT(ch).affected[i].bonus_type], GET_CRAFT(ch).motes_required[i], 
                         craft_motes_required(GET_CRAFT(ch).affected[i].location, GET_CRAFT(ch).affected[i].modifier, GET_CRAFT(ch).affected[i].bonus_type, 0),
                         crafting_motes[crafting_mote_by_bonus_location(GET_CRAFT(ch).affected[i].location, GET_CRAFT(ch).affected[i].specific, GET_CRAFT(ch).affected[i].bonus_type)]);
        }
    }
    if (!found)
    {
        send_to_char(ch, "-- none\r\n");
    }

    if (GET_CRAFT(ch).crafting_item_type && GET_CRAFT(ch).crafting_specific)
    {
        spec_type = GET_CRAFT(ch).crafting_specific;
        if (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_WEAPON)
        {
            setup_craft_weapon(ch, spec_type);
        }
        if (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_ARMOR)
        {
            setup_craft_armor(ch, spec_type);
        }
        if (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_MISC)
        {
            setup_craft_misc(ch, craft_misc_spec_to_vnum(spec_type));
        }
        if (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_INSTRUMENT)
        {
            setup_craft_instrument(ch, spec_type);
        }
        skill = GET_CRAFT(ch).skill_type;
        dc = GET_CRAFT(ch).dc + get_craft_level_adjust_dc_change(GET_CRAFT(ch).level_adjust);
        send_to_char(ch, "\r\n");
        send_to_char(ch, "Skill Required: %s [rank %d].\r\n", ability_names[skill], get_craft_skill_value(ch, skill));
        if (GET_CRAFT(ch).level_adjust)
            send_to_char(ch, "Level Adjust  : %s%d (%s%d to dc).\r\n",  GET_CRAFT(ch).level_adjust > 0 ? "+" : "", GET_CRAFT(ch).level_adjust, 
                         get_craft_level_adjust_dc_change(GET_CRAFT(ch).level_adjust) > 0 ? "+" : "", get_craft_level_adjust_dc_change(GET_CRAFT(ch).level_adjust));
        send_to_char(ch, "Project DC    : %d.\r\n", dc);
        send_to_char(ch, "Object Level  : %d.\r\n", GET_CRAFT(ch).obj_level);
        send_to_char(ch, "\r\n");
    }

    send_to_char(ch, "\r\n");

    if (!is_craft_ready(ch, false))
    {
        send_to_char(ch, "\tRThis craft is not yet ready to begin. Type 'craft check' to see what's missing.\tn\r\n");
    }
    else
    {
        send_to_char(ch, "\tGThis craft is ready to begin, though you still may want to add more to it. Type 'craft start' to begin crafting.\tn\r\n");
    }

    send_to_char(ch, "\r\n");
}

void reset_craft_materials(struct char_data *ch, bool verbose, bool reimburse)
{
    int i = 0;

    // reimburse materials
    for (i = 1; i < NUM_CRAFT_GROUPS; i++)
    {
        if (GET_CRAFT(ch).materials[i][0] == 0 || GET_CRAFT(ch).materials[i][1] == 0)
            continue;
        if (reimburse)
        {
            if (verbose)
            {
                send_to_char(ch, "You have recovered %d unit%s of %s.\r\n",
                    GET_CRAFT(ch).materials[i][1], GET_CRAFT(ch).materials[i][1] > 0 ? "s" : "",
                    crafting_materials[GET_CRAFT(ch).materials[i][0]]
                );
            }
            GET_CRAFT_MAT(ch, GET_CRAFT(ch).materials[i][0]) += GET_CRAFT(ch).materials[i][1];
        }
        GET_CRAFT(ch).materials[i][0] = 0;
        GET_CRAFT(ch).materials[i][1] = 0;
    }
}

#define CR_RESET_ALL            0
#define CR_RESET_MOTES          1
#define CR_RESET_MATERIALS      2
#define CR_RESET_ENHANCEMENT    3
#define CR_RESET_INSTRUMENT     4
#define CR_RESET_BONUSES        5
#define CR_RESET_DESCRIPTIONS   6
#define CR_RESET_REFINE         7
#define CR_RESET_RESIZE         8

void reset_current_craft(struct char_data *ch, char *arg2, bool verbose, bool reimburse)
{
    int i = 0, mote;
    int mode = 0;

    if (arg2 != NULL)
    {
        if (is_abbrev(arg2, "motes"))
            mode = CR_RESET_MOTES;
        else if (is_abbrev(arg2, "materials"))
            mode = CR_RESET_MATERIALS;
        else if (is_abbrev(arg2, "enhancement"))
            mode = CR_RESET_ENHANCEMENT;
        else if (is_abbrev(arg2, "instrument"))
            mode = CR_RESET_INSTRUMENT;
        else if (is_abbrev(arg2, "bonuses"))
            mode = CR_RESET_BONUSES;
        else if (is_abbrev(arg2, "descriptions"))
            mode = CR_RESET_DESCRIPTIONS;
        else if (is_abbrev(arg2, "refine"))
            mode = CR_RESET_REFINE;
        else if (is_abbrev(arg2, "resize"))
            mode = CR_RESET_RESIZE;
    }

    // reimburse motes

    if (mode == CR_RESET_ALL || mode == CR_RESET_MOTES || mode == CR_RESET_ENHANCEMENT)
    {
        if (GET_CRAFT(ch).enhancement_motes_required > 0)
        {
            mote = get_enhancement_mote_type(ch, GET_CRAFT(ch).crafting_item_type, GET_CRAFT(ch).crafting_specific);
            if (mote != CRAFTING_MOTE_NONE && reimburse)
            {
                GET_CRAFT_MOTES(ch, mote) += GET_CRAFT(ch).enhancement_motes_required;
                if (verbose)
                {
                    send_to_char(ch, "You have recovered %d %ss.\r\n", GET_CRAFT(ch).enhancement_motes_required, crafting_motes[mote]);
                }
            }
            GET_CRAFT(ch).enhancement_motes_required = 0;
        }
    }

    if (mode == CR_RESET_ALL || mode == CR_RESET_ENHANCEMENT)
        GET_CRAFT(ch).enhancement = 0;

    if (mode == CR_RESET_ALL || mode == CR_RESET_MOTES || mode == CR_RESET_BONUSES)
    {
        for (i = 0; i < MAX_OBJ_AFFECT; i++)
        {
            if (GET_CRAFT(ch).motes_required[i] == 0) continue;
            mote = crafting_mote_by_bonus_location(GET_CRAFT(ch).affected[i].location, GET_CRAFT(ch).affected[i].specific, GET_CRAFT(ch).affected[i].bonus_type);
            if (mote != CRAFTING_MOTE_NONE && reimburse)
            {
                GET_CRAFT_MOTES(ch, mote) += GET_CRAFT(ch).motes_required[i];   
                if (verbose)
                {
                    send_to_char(ch, "You have recovered %d %ss.\r\n", GET_CRAFT(ch).motes_required[i], crafting_motes[mote]);
                }
            }
            GET_CRAFT(ch).motes_required[i] = 0;
        }

        // bonuses / applies
        for (i = 0; i < MAX_OBJ_AFFECT; i++)
        {
            GET_CRAFT(ch).affected[i].location = 0;
            GET_CRAFT(ch).affected[i].modifier = 0;
            GET_CRAFT(ch).affected[i].bonus_type = 0;
            GET_CRAFT(ch).affected[i].specific = 0;
        }
    }

    if (mode == CR_RESET_ALL || mode == CR_RESET_MATERIALS || mode == CR_RESET_DESCRIPTIONS)
    {
        // reimburse materials
        reset_craft_materials(ch, verbose, reimburse);
    }

    if (mode == CR_RESET_ALL || mode == CR_RESET_MATERIALS || mode == CR_RESET_REFINE)
    {
        for (i = 0; i < 3; i++)
        {
            if (GET_CRAFT(ch).refining_materials[i][1] > 0)
            {
                if (reimburse)
                {
                    GET_CRAFT_MAT(ch, GET_CRAFT(ch).refining_materials[i][0]) += GET_CRAFT(ch).refining_materials[i][1];
                    if (verbose)
                    {
                        send_to_char(ch, "You have recovered %d %s.\r\n", GET_CRAFT(ch).refining_materials[i][1], crafting_materials[GET_CRAFT(ch).refining_materials[i][0]]);
                    }
                }
                GET_CRAFT(ch).refining_materials[i][0] = GET_CRAFT(ch).refining_materials[i][1] = 0;
            }
        }
        
        GET_CRAFT(ch).refining_result[0] = GET_CRAFT(ch).refining_result[1] = 0;
        if (verbose && mode != CR_RESET_ALL)
            send_to_char(ch, "You have reset refining values to the default.\r\n");
    }

    if (mode == CR_RESET_ALL || mode == CR_RESET_MATERIALS || mode == CR_RESET_RESIZE)
    {
        if (GET_CRAFT(ch).new_size)
        {
            if (reimburse)
            {
                GET_CRAFT_MAT(ch, GET_CRAFT(ch).resize_mat_type) += GET_CRAFT(ch).resize_mat_num;
                if (verbose)
                {
                    send_to_char(ch, "You have recovered %d %s.\r\n", GET_CRAFT(ch).resize_mat_num, crafting_materials[GET_CRAFT(ch).resize_mat_type]);
                }
            }

            GET_CRAFT(ch).new_size = GET_CRAFT(ch).resize_mat_type, GET_CRAFT(ch).resize_mat_num = 0;
            reset_crafting_obj(ch);

            if (verbose && mode != CR_RESET_ALL)
                send_to_char(ch, "You have reset resizing values to the default.\r\n");
        }
    }

    if (mode == CR_RESET_ALL || mode == CR_RESET_INSTRUMENT || mode == CR_RESET_MOTES)
    {
        for (i = 0; i < 4; i++)
        {
            if (GET_CRAFT(ch).instrument_motes[i] > 0 && reimburse)
            {
                GET_CRAFT_MOTES(ch, get_crafting_instrument_motes(ch, i, false)) += GET_CRAFT(ch).instrument_motes[i];
                if (verbose)
                {
                    send_to_char(ch, "You have recovered %d %s.\r\n", GET_CRAFT(ch).instrument_motes[i], crafting_motes[get_crafting_instrument_motes(ch, i, false)]);
                }
            }
            GET_CRAFT(ch).instrument_motes[i] = 0;
        }
    }
    
    if (mode == CR_RESET_INSTRUMENT || mode == CR_RESET_ALL)
    {
        GET_CRAFT(ch).instrument_quality = 0;
        GET_CRAFT(ch).instrument_effectiveness = 0;
        GET_CRAFT(ch).instrument_breakability = INSTRUMENT_BREAKABILITY_DEFAULT;
        if (verbose && mode != CR_RESET_ALL)
            send_to_char(ch, "You have reset instrument values to the default.\r\n");
    }

    if (mode == CR_RESET_ALL || mode == CR_RESET_DESCRIPTIONS || mode == CR_RESET_MATERIALS)
    {
        GET_CRAFT(ch).keywords = strdup("not set");
        GET_CRAFT(ch).short_description = strdup("not set");
        GET_CRAFT(ch).room_description = strdup("not set");
        GET_CRAFT(ch).ex_description = NULL;
        if (verbose && mode != CR_RESET_ALL)
            send_to_char(ch, "You have reset the descriptions to default values.\r\n");
    }

    if (mode == CR_RESET_ALL)
    {
        GET_CRAFT(ch).crafting_method = 0;
        GET_CRAFT(ch).crafting_item_type = 0;
        GET_CRAFT(ch).crafting_specific = 0;
        GET_CRAFT(ch).skill_type = 0;
        GET_CRAFT(ch).skill_roll = 0;
        GET_CRAFT(ch).dc = 0;
        GET_CRAFT(ch).craft_variant = -1;
        GET_CRAFT(ch).level_adjust = 0;
    }

    if (verbose)
    {
        send_to_char(ch, "Your project has been reset to default values. All materials and motes have been refunded.\r\n");
    }    
}
void reset_crafting_obj(struct char_data *ch)
{
    GET_CRAFT(ch).craft_obj_rnum = NOTHING;
}

bool is_craft_ready(struct char_data *ch, bool verbose)
{
    bool ready = true;
    int i = 0;
    int required = 0;;
    int location = 0, modifier = 0, bonus_type = 0, specific = 0;
    int base_group, base_amount, project_material, project_amount;

    if (verbose)
        send_to_char(ch, "\r\n");
    
    if (!GET_CRAFT(ch).keywords || is_abbrev(GET_CRAFT(ch).keywords, "(null)") || !strcmp(GET_CRAFT(ch).keywords, "not set"))
    {
        ready = false;
        if (verbose)
            send_to_char(ch, "There are no keywords set.\r\n");
    }
    if (!GET_CRAFT(ch).short_description || is_abbrev(GET_CRAFT(ch).short_description, "(null)") || !strcmp(GET_CRAFT(ch).short_description, "not set"))
    {
        ready = false;
        if (verbose)
            send_to_char(ch, "The short description is not set.\r\n");
    }
    if (!GET_CRAFT(ch).room_description || is_abbrev(GET_CRAFT(ch).room_description, "(null)") || !strcmp(GET_CRAFT(ch).room_description, "not set"))
    {
        ready = false;
        if (verbose)
            send_to_char(ch, "The item's room description is not set.\r\n");
    }
    if (GET_CRAFT(ch).crafting_item_type == 0)
    {
        ready = false;
        if (verbose)
            send_to_char(ch, "The crafting item type is not set.\r\n");
    }
    if (GET_CRAFT(ch).crafting_specific == 0)
    {
        ready = false;
        if (verbose)
            send_to_char(ch, "The crafting item specific type is not set.\r\n");
    }
    if (GET_CRAFT(ch).craft_variant == -1)
    {
        if (verbose)
            send_to_char(ch, "The crafting variant type is not set.\r\n");
    }

    if (GET_CRAFT(ch).crafting_item_type && GET_CRAFT(ch).crafting_specific &&  GET_CRAFT(ch).craft_variant != -1 && GET_CRAFT(ch).crafting_recipe)
    {
        base_group = crafting_recipes[GET_CRAFT(ch).crafting_recipe].materials[0][GET_CRAFT(ch).craft_variant][0];
        base_amount = crafting_recipes[GET_CRAFT(ch).crafting_recipe].materials[0][GET_CRAFT(ch).craft_variant][1];
        project_material = GET_CRAFT(ch).materials[base_group][0];
        project_amount = GET_CRAFT(ch).materials[base_group][1];
        if (base_group != CRAFT_GROUP_NONE)
        {
            if (project_amount < base_amount)
            {
                ready = false;
                if (verbose)
                    send_to_char(ch, "The project requires %d unit%s of %s allocated\r\n", base_amount, base_amount == 1 ? "s" : "", 
                                    crafting_material_groups[base_group]);
            }
        }
        base_group = crafting_recipes[GET_CRAFT(ch).crafting_recipe].materials[1][GET_CRAFT(ch).craft_variant][0];
        base_amount = crafting_recipes[GET_CRAFT(ch).crafting_recipe].materials[1][GET_CRAFT(ch).craft_variant][1];
        project_material = GET_CRAFT(ch).materials[base_group][0];
        project_amount = GET_CRAFT(ch).materials[base_group][1];
        if (base_group != CRAFT_GROUP_NONE)
        {
            if (project_amount < base_amount)
            {
                ready = false;
                if (verbose)
                    send_to_char(ch, "The project requires %d unit%s of %s allocated\r\n", base_amount, base_amount == 1 ? "s" : "", 
                                    crafting_material_groups[base_group]);
            }
        }
        base_group = crafting_recipes[GET_CRAFT(ch).crafting_recipe].materials[2][GET_CRAFT(ch).craft_variant][0];
        base_amount = crafting_recipes[GET_CRAFT(ch).crafting_recipe].materials[2][GET_CRAFT(ch).craft_variant][1];
        project_material = GET_CRAFT(ch).materials[base_group][0];
        project_amount = GET_CRAFT(ch).materials[base_group][1];
        if (base_group != CRAFT_GROUP_NONE)
        {
            if (project_amount < base_amount)
            {
                ready = false;
                if (verbose)
                    send_to_char(ch, "The project requires %d unit%s of %s allocated\r\n", base_amount, base_amount == 1 ? "s" : "", 
                                    crafting_material_groups[base_group]);
            }
        }
    }

    if (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_INSTRUMENT)
    {
        if (GET_CRAFT(ch).instrument_quality > 0)
        {
            if (GET_CRAFT(ch).instrument_motes[1] != get_crafting_instrument_motes(ch, 1, true))
            {
                ready = false;
                if (verbose)
                    send_to_char(ch, "The instrument quality requires %d %ss.\r\n", get_crafting_instrument_motes(ch, 1, true), crafting_motes[get_crafting_instrument_motes(ch, 1, false)]);
            }
        }
        if (GET_CRAFT(ch).instrument_effectiveness > 0)
        {
            if (GET_CRAFT(ch).instrument_motes[2] != get_crafting_instrument_motes(ch, 2, true))
            {
                ready = false;
                if (verbose)
                    send_to_char(ch, "The instrument effectiveness requires %d %ss.\r\n", get_crafting_instrument_motes(ch, 2, true), 
                    crafting_motes[get_crafting_instrument_motes(ch, 2, false)]);
            }
        }
        if (GET_CRAFT(ch).instrument_breakability != INSTRUMENT_BREAKABILITY_DEFAULT)
        {
            if (GET_CRAFT(ch).instrument_motes[3] != get_crafting_instrument_motes(ch, 3, true))
            {
                ready = false;
                if (verbose)
                    send_to_char(ch, "The instrument breakability requires %d %ss.\r\n", get_crafting_instrument_motes(ch, 3, true), 
                    crafting_motes[get_crafting_instrument_motes(ch, 3, false)]);
            }
        }
    }

    for (i = 0; i < MAX_OBJ_AFFECT; i++)
    {
        if (GET_CRAFT(ch).affected[i].location != APPLY_NONE || GET_CRAFT(ch).affected[i].modifier != 0)
        {
            if (GET_CRAFT(ch).motes_required[i] != craft_motes_required(GET_CRAFT(ch).affected[i].location, 
                GET_CRAFT(ch).affected[i].modifier, GET_CRAFT(ch).affected[i].bonus_type, 0))
            {
                ready = false;
                if (verbose)
                {   
                    location = GET_CRAFT(ch).affected[i].location;
                    modifier = GET_CRAFT(ch).affected[i].modifier;
                    bonus_type = GET_CRAFT(ch).affected[i].bonus_type;
                    specific = GET_CRAFT(ch).affected[i].specific;
                    required = craft_motes_required(location, modifier, bonus_type, 0);
                    send_to_char(ch, "The bonus in slot %d requires %d %ss.\r\n", i+1, required, 
                                crafting_motes[crafting_mote_by_bonus_location(location, specific, bonus_type)]);
                }
            }
        }
    }

    if (GET_CRAFT(ch).enhancement > 0)
    {
        if (GET_CRAFT(ch).enhancement_motes_required < craft_motes_required(0, 0, 0, GET_CRAFT(ch).enhancement))
        {
            ready = false;
            send_to_char(ch, "You require %d %ss for the object's enhancement bonus.\r\n", craft_motes_required(0, 0, 0, GET_CRAFT(ch).enhancement), crafting_motes[get_enhancement_mote_type(ch, GET_CRAFT(ch).crafting_item_type, GET_CRAFT(ch).crafting_specific)]);
        }
    }

    if (get_craft_project_level(ch) > 30)
    {
        send_to_char(ch, "The object level based on the existing bonuses and enhancement bonus (weapons, armor, shields only) is too high.\r\n"
                        "You must downgrade the enhanceent bonus, some of the other bonuses or try adding higher quality materials.\r\n");
        ready = false;
    }

    return ready;
}

void begin_current_craft(struct char_data *ch)
{
    if (!is_craft_ready(ch, true))
    {
        send_to_char(ch, "\tCPlease fix the above errors before continuing.\tn\r\n");
        return;
    }

    int seconds = CREATE_BASE_TIME;

    GET_CRAFT(ch).craft_duration = seconds;
    GET_CRAFT(ch).crafting_method = SCMD_NEWCRAFT_CREATE;

    send_to_char(ch, "You begin creating %s. This will take a total of %d minutes and %d seconds.\r\n", GET_CRAFT(ch).short_description, seconds / 60, seconds % 60);
    act("$n starts crafting.", FALSE, ch, 0, 0, TO_ROOM);
}

void set_craft_item_descs(struct char_data *ch, struct obj_data *obj)
{
    obj->name = strdup(GET_CRAFT(ch).keywords);
    obj->short_description = strdup(GET_CRAFT(ch).short_description);
    obj->description = strdup(GET_CRAFT(ch).room_description);
    if (GET_CRAFT(ch).ex_description != NULL)
    {
        struct extra_descr_data *new_descr;
        char extra[MAX_EXTRA_DESC+10];
        CREATE(new_descr, struct extra_descr_data, 1);
        new_descr->keyword = strdup(obj->name);
        snprintf(extra, sizeof(extra), "%s\n", GET_CRAFT(ch).ex_description);
        new_descr->description = strdup(extra);
        new_descr->next = obj->ex_description;
        obj->ex_description = new_descr;
    }
}

void set_craft_item_affects(struct char_data *ch, struct obj_data *obj)
{
    int i = 0;

    for (i = 0; i < MAX_OBJ_AFFECT; i++)
    {
        if (GET_CRAFT(ch).affected[i].location != APPLY_NONE)
        {
            obj->affected[i].location = GET_CRAFT(ch).affected[i].location;
            obj->affected[i].modifier = GET_CRAFT(ch).affected[i].modifier;
            obj->affected[i].bonus_type = GET_CRAFT(ch).affected[i].bonus_type;
            obj->affected[i].specific = GET_CRAFT(ch).affected[i].specific;
        }
    }
}

void set_craft_item_flags(struct char_data *ch, struct obj_data *obj)
{
    SET_OBJ_FLAG(obj, ITEM_CRAFTED);
    SET_OBJ_FLAG(obj, ITEM_IDENTIFIED);
    REMOVE_OBJ_FLAG(obj, ITEM_MOLD);
}

int material_to_craft_skill(int item_type, int material)
{
    switch (item_type)
    {
        case ITEM_WEAPON:
            if (IS_WOOD(material))
                return ABILITY_CRAFT_WOODWORKING;
            else if (IS_LEATHER(material))
                return ABILITY_CRAFT_LEATHERWORKING;
            else
                return ABILITY_CRAFT_WEAPONSMITHING;
        
        case ITEM_ARMOR:
            if (IS_CLOTH(material))
                return ABILITY_CRAFT_TAILORING;
            else if (IS_LEATHER(material))
                return ABILITY_CRAFT_LEATHERWORKING;
            else
                return ABILITY_CRAFT_ARMORSMITHING;

        case ITEM_WORN:
            if (IS_CLOTH(material))
                return ABILITY_CRAFT_TAILORING;
            else if (IS_LEATHER(material))
                return ABILITY_CRAFT_LEATHERWORKING;
            else
                return ABILITY_CRAFT_JEWELCRAFTING;
            break;

        case ITEM_INSTRUMENT:
            if (IS_WOOD(material))
                return ABILITY_CRAFT_WOODWORKING;
            else if (IS_LEATHER(material))
                return ABILITY_CRAFT_LEATHERWORKING;
            else if (IS_PRECIOUS_METAL(material))
                return ABILITY_CRAFT_JEWELCRAFTING;
            else
                return ABILITY_CRAFT_METALWORKING;
            break;
    }
    return ABILITY_CRAFT_METALWORKING;
}

bool create_craft_skill_check(struct char_data *ch, struct obj_data *obj, int skill, char *method, int exp, int dc)
{
    if (!ch || !obj) return false;
    int roll, skill_mod;

    roll = d20(ch);
    skill_mod = get_craft_skill_value(ch, skill);

    if ((20 + skill_mod) < dc)
    {
        send_to_char(ch, "You don't have the skill to craft %s.\r\n", obj->short_description);
        return false;
    }

    // critical failure. Lose the item, materials and motes.
    if (roll == 1)
    {
        send_to_char(ch, "\tM[CRITICAL FAILURE]\tn You rolled a natural 1! The %s failed and you lost your materials and motes.\r\n", method);
        reset_current_craft(ch, NULL, false, false);
        return false;
    }
    // critical success. Item is masterwork quality.
    else if (roll == 20)
    {
        send_to_char(ch, "\tM[CRITICAL SUCCESS]\tn You rolled a natural 20! The %s succeeded and is of masterwork quality.\r\n", method);
        SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MASTERWORK);
        return true;
    }
    else if ((roll + skill_mod) < dc)
    {
        send_to_char(ch, "You rolled %d + your skill in %s of %d = total of %d vs. dc %d. The %s attempt failed, but you may try again.\r\n",
                        roll, ability_names[skill], skill_mod, roll + skill_mod, dc, method);
        gain_craft_exp(ch, exp, skill, true);
        return false;
    }
    else
    {
        send_to_char(ch, "You rolled %d + your skill in %s of %d = total of %d vs. dc %d. The %s attempt succeeded!\r\n",
                        roll, ability_names[skill], skill_mod, roll + skill_mod, dc, method);
        return true;
    }
    return false;
}

int obj_material_to_craft_material(int material)
{
    switch (material)
    {
        case MATERIAL_COPPER: return CRAFT_MAT_COPPER;
        case MATERIAL_TIN: return CRAFT_MAT_TIN;
        case MATERIAL_BRONZE: return CRAFT_MAT_BRONZE;
        case MATERIAL_IRON: return CRAFT_MAT_IRON;
        case MATERIAL_COAL: return CRAFT_MAT_COAL;
        case MATERIAL_STEEL: return CRAFT_MAT_STEEL;
        case MATERIAL_COLD_IRON: return CRAFT_MAT_COLD_IRON;
        case MATERIAL_ALCHEMAL_SILVER: return CRAFT_MAT_ALCHEMAL_SILVER;
        case MATERIAL_MITHRIL: return CRAFT_MAT_MITHRIL;
        case MATERIAL_ADAMANTINE: return CRAFT_MAT_ADAMANTINE;
        case MATERIAL_SILVER: return CRAFT_MAT_SILVER;
        case MATERIAL_GOLD: return CRAFT_MAT_GOLD;
        case MATERIAL_PLATINUM: return CRAFT_MAT_PLATINUM;
        case MATERIAL_DRAGONMETAL: return CRAFT_MAT_DRAGONMETAL;
        case MATERIAL_DRAGONSCALE: return CRAFT_MAT_DRAGONSCALE;
        case MATERIAL_DRAGONBONE: return CRAFT_MAT_DRAGONBONE;
        case MATERIAL_LEATHER: return CRAFT_MAT_LOW_GRADE_HIDE;
        case MATERIAL_ASH: return CRAFT_MAT_ASH_WOOD;
        case MATERIAL_MAPLE: return CRAFT_MAT_MAPLE_WOOD;
        case MATERIAL_MAHAGONY: return CRAFT_MAT_MAHAGONY_WOOD;
        case MATERIAL_VALENWOOD: return CRAFT_MAT_VALENWOOD;
        case MATERIAL_IRONWOOD: return CRAFT_MAT_IRONWOOD;
        case MATERIAL_HEMP: return CRAFT_MAT_HEMP;
        case MATERIAL_WOOL: return CRAFT_MAT_WOOL;
        case MATERIAL_LINEN: return CRAFT_MAT_LINEN;
        case MATERIAL_SATIN: return CRAFT_MAT_SATIN;
        case MATERIAL_SILK: return CRAFT_MAT_SILK;
        case MATERIAL_ZINC: return CRAFT_MAT_ZINC;
        case MATERIAL_COTTON: return CRAFT_MAT_COTTON;
        case MATERIAL_BRASS: return CRAFT_MAT_BRASS;
        case MATERIAL_FLAX: return CRAFT_MAT_FLAX;
        case MATERIAL_BONE: return CRAFT_MAT_BONE;
    }
    return CRAFT_MAT_NONE;
}
int craft_material_to_obj_material(int craftmat)
{
    switch (craftmat)
    {
        case CRAFT_MAT_COPPER: return MATERIAL_COPPER;
        case CRAFT_MAT_TIN: return MATERIAL_TIN;
        case CRAFT_MAT_BRONZE: return MATERIAL_BRONZE;
        case CRAFT_MAT_IRON: return MATERIAL_IRON;
        case CRAFT_MAT_COAL: return MATERIAL_COAL;
        case CRAFT_MAT_STEEL: return MATERIAL_STEEL;
        case CRAFT_MAT_COLD_IRON: return MATERIAL_COLD_IRON;
        case CRAFT_MAT_ALCHEMAL_SILVER: return MATERIAL_ALCHEMAL_SILVER;
        case CRAFT_MAT_MITHRIL: return MATERIAL_MITHRIL;
        case CRAFT_MAT_ADAMANTINE: return MATERIAL_ADAMANTINE;
        case CRAFT_MAT_SILVER: return MATERIAL_SILVER;
        case CRAFT_MAT_GOLD: return MATERIAL_GOLD;
        case CRAFT_MAT_PLATINUM: return MATERIAL_PLATINUM;
        case CRAFT_MAT_DRAGONMETAL: return MATERIAL_DRAGONMETAL;
        case CRAFT_MAT_DRAGONSCALE: return MATERIAL_DRAGONSCALE;
        case CRAFT_MAT_DRAGONBONE: return MATERIAL_DRAGONBONE;
        case CRAFT_MAT_LOW_GRADE_HIDE: return MATERIAL_LEATHER;
        case CRAFT_MAT_MEDIUM_GRADE_HIDE: return MATERIAL_LEATHER;
        case CRAFT_MAT_HIGH_GRADE_HIDE: return MATERIAL_LEATHER;
        case CRAFT_MAT_PRISTINE_GRADE_HIDE: return MATERIAL_LEATHER;
        case CRAFT_MAT_ASH_WOOD: return MATERIAL_ASH;
        case CRAFT_MAT_MAPLE_WOOD: return MATERIAL_MAPLE;
        case CRAFT_MAT_MAHAGONY_WOOD: return MATERIAL_MAHAGONY;
        case CRAFT_MAT_VALENWOOD: return MATERIAL_VALENWOOD;
        case CRAFT_MAT_IRONWOOD: return MATERIAL_IRONWOOD;
        case CRAFT_MAT_HEMP: return MATERIAL_HEMP;
        case CRAFT_MAT_WOOL: return MATERIAL_WOOL;
        case CRAFT_MAT_LINEN: return MATERIAL_LINEN;
        case CRAFT_MAT_SATIN: return MATERIAL_SATIN;
        case CRAFT_MAT_SILK: return MATERIAL_SILK;
        case CRAFT_MAT_ZINC: return MATERIAL_ZINC;
        case CRAFT_MAT_COTTON: return MATERIAL_COTTON;
        case CRAFT_MAT_BRASS: return MATERIAL_BRASS;
        case CRAFT_MAT_FLAX: return MATERIAL_FLAX;
    }
    return MATERIAL_UNDEFINED;
}

struct obj_data *setup_craft_weapon(struct char_data *ch, int w_type)
{
    struct obj_data *obj;
    int skill = 0;
    int dc = 0;
    
    if ((obj = read_object(WEAPON_PROTO, VIRTUAL)) == NULL)
    {
        return NULL;
    }

    // set up default values for the weapon type
    set_weapon_object(obj, w_type);

    // set descriptions
    set_craft_item_descs(ch, obj);

    // set obj affects
    set_craft_item_affects(ch, obj);

    // set enhancement bonus
    GET_OBJ_VAL(obj, 4) = GET_CRAFT(ch).enhancement;

    // set obj flags
    set_craft_item_flags(ch, obj);

    skill = material_to_craft_skill(GET_OBJ_TYPE(obj), GET_OBJ_MATERIAL(obj));

    // set the obj material to the main craft material used
    GET_OBJ_MATERIAL(obj) = craft_material_to_obj_material( GET_CRAFT(ch).materials[ crafting_recipes[GET_CRAFT(ch).crafting_recipe].materials[0][GET_CRAFT(ch).craft_variant][0] ] [0]  );

    GET_CRAFT(ch).obj_level = MAX(1, GET_OBJ_LEVEL(obj) = get_craft_obj_level(obj, ch));

    dc = (CREATE_BASE_DC + GET_OBJ_LEVEL(obj) - GET_CRAFT(ch).level_adjust);

    GET_CRAFT(ch).skill_type = skill;
    GET_CRAFT(ch).dc = dc;
    
    return obj;
}

void create_craft_weapon(struct char_data *ch)
{
    int w_type = GET_CRAFT(ch).crafting_specific;
    struct obj_data *obj;
    int skill = ABILITY_CRAFT_WEAPONSMITHING;
    int dc = 0;

    if ((obj = setup_craft_weapon(ch, w_type) ) == NULL)
    {
        log("SYSERR: create_craft_weapon created NULL object");
        return;
    }

    dc = GET_CRAFT(ch).dc + get_craft_level_adjust_dc_change(GET_CRAFT(ch).level_adjust);

    GET_CRAFT(ch).skill_type = skill;

    // skill check to determine success or failure
    if (!create_craft_skill_check(ch, obj, skill, "craft", CREATE_BASE_EXP / 2, dc))
    {
        // failure means we end things here.
        return;
    }

    gain_craft_exp(ch, MAX(CREATE_BASE_EXP, GET_OBJ_LEVEL(obj) * CREATE_BASE_EXP), skill, true);

    send_to_char(ch, "You've created %s!\r\n", obj->short_description);
    obj_to_char(obj, ch);
    reset_current_craft(ch, NULL, false, false);
}

struct obj_data *setup_craft_armor(struct char_data *ch, int a_type)
{
    struct obj_data *obj;
    int skill = 0;
    int dc = 0;
    
    if ((obj = read_object(ARMOR_PROTO, VIRTUAL)) == NULL)
    {
        return NULL;
    }

    // set up default values for the weapon type
    set_armor_object(obj, a_type);

    // set descriptions
    set_craft_item_descs(ch, obj);

    // set obj affects
    set_craft_item_affects(ch, obj);

    // set enhancement bonus
    GET_OBJ_VAL(obj, 4) = GET_CRAFT(ch).enhancement;

    // set obj flags
    set_craft_item_flags(ch, obj);

    skill = material_to_craft_skill(GET_OBJ_TYPE(obj), GET_OBJ_MATERIAL(obj));

    // set the obj material to the main craft material used
    // GET_OBJ_MATERIAL(obj) = craft_material_to_obj_material(GET_CRAFT(ch).materials[0][0]);
    GET_OBJ_MATERIAL(obj) = craft_material_to_obj_material( GET_CRAFT(ch).materials[ crafting_recipes[GET_CRAFT(ch).crafting_recipe].materials[0][GET_CRAFT(ch).craft_variant][0] ] [0]  );

    GET_CRAFT(ch).obj_level = MAX(1, GET_OBJ_LEVEL(obj) = get_craft_obj_level(obj, ch));

    dc = (CREATE_BASE_DC + GET_OBJ_LEVEL(obj) - GET_CRAFT(ch).level_adjust);

    GET_CRAFT(ch).skill_type = skill;
    GET_CRAFT(ch).dc = dc;
    
    return obj;
}

void create_craft_armor(struct char_data *ch)
{
    int a_type = GET_CRAFT(ch).crafting_specific;
    struct obj_data *obj;
    int skill = ABILITY_CRAFT_ARMORSMITHING;
    int dc = 0;

    if ((obj = setup_craft_armor(ch, a_type) ) == NULL)
    {
        log("SYSERR: create_craft_armor created NULL object");
        return;
    }

    dc = GET_CRAFT(ch).dc + get_craft_level_adjust_dc_change(GET_CRAFT(ch).level_adjust);

    GET_CRAFT(ch).skill_type = skill;

    // skill check to determine success or failure
    if (!create_craft_skill_check(ch, obj, skill, "craft", CREATE_BASE_EXP / 2, dc))
    {
        // failure means we end things here.
        return;
    }

    gain_craft_exp(ch, MAX(CREATE_BASE_EXP, GET_OBJ_LEVEL(obj) * CREATE_BASE_EXP), skill, true);

    send_to_char(ch, "You've created %s!\r\n", obj->short_description);
    obj_to_char(obj, ch);
    reset_current_craft(ch, NULL, false, false);
}

void set_craft_instrument_object(struct obj_data *obj, struct char_data *ch)
{
  int wear_inc;

  GET_OBJ_TYPE(obj) = ITEM_INSTRUMENT;

  // Instrument Type
  GET_OBJ_VAL(obj, 0) = craft_instrument_type_to_actual(GET_CRAFT(ch).crafting_specific);
  // Quality
  GET_OBJ_VAL(obj, 1) = GET_CRAFT(ch).instrument_quality;
  // Effecitveness
  GET_OBJ_VAL(obj, 2) = GET_CRAFT(ch).instrument_effectiveness;
  // Breakability
  GET_OBJ_VAL(obj, 3) = GET_CRAFT(ch).instrument_breakability;

  /* for convenience we are going to go ahead and set some other values */
  GET_OBJ_COST(obj) = 100;
  GET_OBJ_WEIGHT(obj) = 1;

  /* going to go ahead and reset all the bits off */
  for (wear_inc = 0; wear_inc < NUM_ITEM_WEARS; wear_inc++)
  {
    REMOVE_BIT_AR(GET_OBJ_WEAR(obj), wear_inc);
  }

  /* now set take bit */
  TOGGLE_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
  TOGGLE_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_INSTRUMENT);
}

/**
 * @brief Converts a given instrument type to its actual representation.
 *
 * This function takes an integer representing an instrument type and
 * returns the corresponding actual instrument value. The mapping of
 * types to actual values is defined within the function.
 *
 * @param type An integer representing the instrument type to be converted.
 * @return An integer representing the actual instrument value.
 */
int craft_instrument_type_to_actual(int type)
{
    switch (type)
    {
        case CRAFT_INSTRUMENT_LYRE:
            return INSTRUMENT_LYRE;
        case CRAFT_INSTRUMENT_FLUTE:
            return INSTRUMENT_FLUTE;
        case CRAFT_INSTRUMENT_HARP:
            return INSTRUMENT_HARP;
        case CRAFT_INSTRUMENT_DRUM:
            return INSTRUMENT_DRUM;
        case CRAFT_INSTRUMENT_HORN:
            return INSTRUMENT_HORN;
        case CRAFT_INSTRUMENT_MANDOLIN:
            return INSTRUMENT_MANDOLIN;        
    }
    return INSTRUMENT_LYRE; // default to lyre if not found
}

struct obj_data *setup_craft_instrument(struct char_data *ch, int a_type)
{
    struct obj_data *obj;
    int skill = 0;
    int dc = 0;
    
    if ((obj = read_object(INSTRUMENT_PROTO, VIRTUAL)) == NULL)
    {
        return NULL;
    }

    // set up default values for the weapon type
    set_craft_instrument_object(obj, ch);

    // set descriptions
    set_craft_item_descs(ch, obj);

    // set obj affects
    set_craft_item_affects(ch, obj);

    // set obj flags
    set_craft_item_flags(ch, obj);

    // set the obj material to the main craft material used
    // GET_OBJ_MATERIAL(obj) = craft_material_to_obj_material(GET_CRAFT(ch).materials[0][0]);
    GET_OBJ_MATERIAL(obj) = craft_material_to_obj_material( GET_CRAFT(ch).materials[ crafting_recipes[GET_CRAFT(ch).crafting_recipe].materials[0][GET_CRAFT(ch).craft_variant][0] ] [0]  );

    skill = material_to_craft_skill(GET_OBJ_TYPE(obj), GET_OBJ_MATERIAL(obj));

    GET_CRAFT(ch).obj_level = MAX(1, GET_OBJ_LEVEL(obj) = (get_craft_obj_level(obj, ch) + get_crafting_instrument_dc_modifier(ch)));

    dc = (CREATE_BASE_DC + GET_OBJ_LEVEL(obj) - GET_CRAFT(ch).level_adjust);

    GET_CRAFT(ch).skill_type = skill;
    GET_CRAFT(ch).dc = dc;
    
    return obj;
}

void create_craft_instrument(struct char_data *ch)
{
    int i_type = GET_CRAFT(ch).crafting_specific;
    struct obj_data *obj;
    int skill = 0;
    int dc = 0;

    if ((obj = setup_craft_instrument(ch, i_type) ) == NULL)
    {
        log("SYSERR: create_craft_instrument created NULL object");
        return;
    }

    skill = GET_CRAFT(ch).skill_type;

    dc = GET_CRAFT(ch).dc + get_craft_level_adjust_dc_change(GET_CRAFT(ch).level_adjust);

    // skill check to determine success or failure
    if (!create_craft_skill_check(ch, obj, skill, "craft", CREATE_BASE_EXP / 2, dc))
    {
        // failure means we end things here.
        return;
    }

    gain_craft_exp(ch, MAX(CREATE_BASE_EXP, GET_OBJ_LEVEL(obj) * CREATE_BASE_EXP), skill, true);

    send_to_char(ch, "You've created %s!\r\n", obj->short_description);
    obj_to_char(obj, ch);
    reset_current_craft(ch, NULL, false, false);
}

struct obj_data *setup_craft_misc(struct char_data *ch, int vnum)
{
    struct obj_data *obj;
    int skill = 0;
    int dc = 0;
    
    if ((obj = read_object(vnum, VIRTUAL)) == NULL)
    {
        return NULL;
    }

    // set descriptions
    set_craft_item_descs(ch, obj);

    // set obj affects
    set_craft_item_affects(ch, obj);

    // set obj flags
    set_craft_item_flags(ch, obj);

    // set the obj material to the main craft material used
    // GET_OBJ_MATERIAL(obj) = craft_material_to_obj_material(GET_CRAFT(ch).materials[0][0]);
    GET_OBJ_MATERIAL(obj) = craft_material_to_obj_material( GET_CRAFT(ch).materials[ crafting_recipes[GET_CRAFT(ch).crafting_recipe].materials[0][GET_CRAFT(ch).craft_variant][0] ] [0]  );
    send_to_char(ch, "Mat: %d\r\n", GET_OBJ_MATERIAL(obj));

    skill = material_to_craft_skill(GET_OBJ_TYPE(obj), GET_OBJ_MATERIAL(obj));

    GET_CRAFT(ch).obj_level = MAX(1, GET_OBJ_LEVEL(obj) = get_craft_obj_level(obj, ch));

    dc = (CREATE_BASE_DC + GET_OBJ_LEVEL(obj) - GET_CRAFT(ch).level_adjust);

    GET_CRAFT(ch).skill_type = skill;
    GET_CRAFT(ch).dc = dc;
    
    return obj;
}

int craft_misc_spec_to_vnum(int s_type)
{
    int vnum = 0;

    switch (s_type)
    {
        case CRAFT_JEWELRY_BRACELET:
            vnum = WRIST_MOLD; break;
        case CRAFT_JEWELRY_EARRING:
            vnum = EARS_MOLD; break;
        case CRAFT_JEWELRY_GLASSES:
            vnum = EYES_MOLD; break;
        case CRAFT_JEWELRY_NECKLACE:
            vnum = NECKLACE_MOLD; break;
        case CRAFT_JEWELRY_RING:
            vnum = RING_MOLD; break;
        case CRAFT_MISC_BELT:
            vnum = BELT_MOLD; break;
        case CRAFT_MISC_BOOTS:
            vnum = BOOTS_MOLD; break;
        case CRAFT_MISC_CLOAK:
            vnum = CLOAK_MOLD; break;
        case CRAFT_MISC_GLOVES:
            vnum = GLOVES_MOLD; break;
        case CRAFT_MISC_MASK:
            vnum = FACE_MOLD; break;
        case CRAFT_MISC_SHOULDERS:
            vnum = SHOULDERS_MOLD; break;
        case CRAFT_MISC_ANKLET:
            vnum = ANKLET_MOLD; break;
            break;
    }
    return vnum;
}

void create_craft_misc(struct char_data *ch)
{
    int m_type = GET_CRAFT(ch).crafting_item_type;
    int s_type = GET_CRAFT(ch).crafting_specific;
    struct obj_data *obj;
    int vnum = 0;
    int skill = ABILITY_CRAFT_TAILORING;
    int dc = 0;

    switch (m_type)
    {
        case CRAFT_TYPE_MISC:
            vnum = craft_misc_spec_to_vnum(s_type);
            break;
    }

    if ((obj = setup_craft_misc(ch, vnum) ) == NULL)
    {
        log("SYSERR: create_craft_misc created NULL object");
        return;
    }

    skill = GET_CRAFT(ch).skill_type;

    dc = GET_CRAFT(ch).dc + get_craft_level_adjust_dc_change(GET_CRAFT(ch).level_adjust);

    // skill check to determine success or failure
    if (!create_craft_skill_check(ch, obj, skill, "craft", CREATE_BASE_EXP / 2, dc))
    {
        // failure means we end things here.
        return;
    }

    gain_craft_exp(ch, MAX(CREATE_BASE_EXP, GET_OBJ_LEVEL(obj) * CREATE_BASE_EXP), skill, true);

    send_to_char(ch, "You've created %s!\r\n", obj->short_description);
    obj_to_char(obj, ch);
    reset_current_craft(ch, NULL, false, false);
}

void craft_create_complete(struct char_data *ch)
{
    switch (GET_CRAFT(ch).crafting_item_type)
    {
        case CRAFT_TYPE_WEAPON:
            create_craft_weapon(ch);
            break;
        case CRAFT_TYPE_ARMOR:
            create_craft_armor(ch);
            break;
        case CRAFT_TYPE_MISC:
            create_craft_misc(ch);
            break;
        case CRAFT_TYPE_INSTRUMENT:
            create_craft_instrument(ch);
            break;
    }
    act("$n finishes crafting.", FALSE, ch, 0, 0, TO_ROOM);
}

void check_current_craft(struct char_data *ch, bool verbose)
{
    if (!is_craft_ready(ch, verbose))
    {
        send_to_char(ch, "\tRThis craft is not yet ready to begin. Please fix the above errors.\tn\r\n");
    }
    else
    {
        send_to_char(ch, "\tGThis craft is ready to begin, though you still may want to add more to it. Type 'craft start' to begin crafting.\tn\r\n");
    }
        
}

void set_crafting_variant(struct char_data *ch, char *arg2)
{

    int i = 0, j = 0, variant = 0, recipe = 0;
    bool found = false;
    char materials[200];
    char mat_one[60], mat_two[60], mat_three[60];

    if (GET_CRAFT(ch).craft_variant != -1)
    {
        send_to_char(ch, "You've already set a variant type. To change it you must reset the crafting project with: craft reset.\r\n");
        return;
    }

    if (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_NONE)
    {
        send_to_char(ch, "You need to set the item type first with 'craft itemtype'.\r\n");
        return;
    }

    if (GET_CRAFT(ch).crafting_specific == 0)
    {
        send_to_char(ch, "You need to set the item specific type first with 'craft specifictype'.\r\n");
        return;
    }

    if (!*arg2)
    {
        send_to_char(ch, "You can select from the following variants:\r\n");
        for (i = 0; i < NUM_CRAFTING_RECIPES; i++)
        {
            if (crafting_recipes[i].object_type == craft_recipe_by_type(GET_CRAFT(ch).crafting_item_type) &&
               (crafting_recipes[i].practical_type == GET_CRAFT(ch).crafting_specific))
            {
                for (j = 0; j < NUM_CRAFT_VARIANTS; j++)
                {
                    if (crafting_recipes[i].materials[0][j][0] == 0)
                        continue;
                    snprintf(mat_one, sizeof(mat_one), "\tn");
                    snprintf(mat_two, sizeof(mat_two), "\tn");
                    snprintf(mat_three, sizeof(mat_three), "\tn");
                    if (crafting_recipes[i].materials[0][j][0] > 0)
                        snprintf(mat_one, sizeof(mat_one), "%d unit%s of %s", crafting_recipes[i].materials[0][j][1], crafting_recipes[i].materials[0][j][1] > 1 ? "s" : "",
                                crafting_material_groups[crafting_recipes[i].materials[0][j][0]]);
                    if (crafting_recipes[i].materials[1][j][0] > 0)
                        snprintf(mat_two, sizeof(mat_two), ", %d unit%s of %s", crafting_recipes[i].materials[1][j][1], crafting_recipes[i].materials[1][j][1] > 1 ? "s" : "",
                                crafting_material_groups[crafting_recipes[i].materials[1][j][0]]);
                    if (crafting_recipes[i].materials[2][j][0] > 0)
                        snprintf(mat_three, sizeof(mat_three), ", %d unit%s of %s", crafting_recipes[i].materials[2][j][1], crafting_recipes[i].materials[2][j][1] > 1 ? "s" : "",
                                crafting_material_groups[crafting_recipes[i].materials[2][j][0]]);
                    snprintf(materials, sizeof(materials), "%s%s%s", mat_one, mat_two, mat_three);
                    send_to_char(ch, "%-30s : %s\r\n", crafting_recipes[i].variant_descriptions[j], materials);
                }
            }
        }
    }
    else
    {
        for (i = 0; i < NUM_CRAFTING_RECIPES; i++)
        {
            if (crafting_recipes[i].object_type == craft_recipe_by_type(GET_CRAFT(ch).crafting_item_type) &&
               (crafting_recipes[i].practical_type == GET_CRAFT(ch).crafting_specific))
            {
                for (j = 0; j < NUM_CRAFT_VARIANTS; j++)
                {
                    if (crafting_recipes[i].materials[0][j][0] == 0)
                        continue;
                    if (is_abbrev(arg2, crafting_recipes[i].variant_descriptions[j]))
                    {
                        variant = j;
                        recipe = i;
                        found = true;
                        break;
                    }
                }
                if (found)
                    break;
            }
        }
        if (!found)
        {
            send_to_char(ch, "That is not a valid variant type. Enter 'craft variant' by itself to see options.\r\n");
            return;
        }
        GET_CRAFT(ch).craft_variant = variant;
        GET_CRAFT(ch).crafting_recipe = recipe;
        send_to_char(ch, "You have chosen '%s' as the variant type for your craft.\r\n", crafting_recipes[recipe].variant_descriptions[variant]);
        GET_CRAFT(ch).keywords = strdup("not set");
        GET_CRAFT(ch).short_description = strdup("not set");
        GET_CRAFT(ch).room_description = strdup("not set");
        GET_CRAFT(ch).ex_description = NULL;
        return;
    }
}

void set_crafting_materials(struct char_data *ch, const char *arg2)
{
    if (!*arg2)
    {
        send_to_char(ch, "You need to specify the material transaction and type. Eg. craft materials add steel or craft materials remove steel.\r\n");
        return;
    }
    char mat[100], amount[100];
    int num_mats = 0, mat_type = 0, group = 0, i = 0;

    half_chop_c(arg2, amount, sizeof(amount), mat, sizeof(mat));

    if (!*amount || !*mat)
    {
        send_to_char(ch, "You need to specify the material transaction and type. Eg. craft materials add steel or craft materials remove steel.\r\n");
        return;
    }

    for (i = 0; i < NUM_CRAFT_MATS; i++)
    {
        if (is_abbrev(mat, crafting_materials[i]))
        {
            mat_type = i;
            break;
        }
    }

    if (mat_type == 0)
    {
        send_to_char(ch, "That is not a valid material type. Type 'materials' for a list of which materials you possess.\r\n");
        return;
    }

    if (is_abbrev(amount, "add"))
        num_mats = 1;
    else if (is_abbrev(amount, "remove"))
        num_mats = -1;
    else
    {
        send_to_char(ch, "You need to specify the material transaction and type. Eg. craft materials add steel or craft materials remove steel.\r\n");
        return;
    }

    group = craft_group_by_material(mat_type);

    if (group == CRAFT_GROUP_NONE)
    {
        send_to_char(ch, "That material type cannot be used in crafting recipes.\r\n");
        return;
    }

    if (GET_CRAFT(ch).crafting_item_type == 0 || GET_CRAFT(ch).crafting_specific == 0 || GET_CRAFT(ch).craft_variant == -1)
    {
        send_to_char(ch, "You must select the item type, specific type and variant type before adding materials.\r\n");
        return;
    }

    for (i = 0; i < 3; i++)
    {
        if (crafting_recipes[GET_CRAFT(ch).crafting_recipe].materials[i][GET_CRAFT(ch).craft_variant][0] == group)
        {
            process_crafting_materials(ch, group, mat_type, num_mats, i);
            return;
        }
    }
    
    // crafting material type is not used in this recipe
    send_to_char(ch, "The crafting project does not use that type of material.\r\n");
    return;
}

void process_crafting_materials(struct char_data *ch, int group, int mat_type, int num_mats, int mat_slot)
{

    int base_amount = 0,
        owned_amount = 0;

    // removing mats
    if (num_mats < 0)
    {
        if (GET_CRAFT(ch).materials[group][0] != mat_type && GET_CRAFT(ch).materials[group][0] != CRAFT_GROUP_NONE)
        {
            send_to_char(ch, "You are currently using '%s' for this project.\r\n", crafting_materials[GET_CRAFT(ch).materials[group][0]]);
            return;
        }
        base_amount = GET_CRAFT(ch).materials[group][1];
        GET_CRAFT_MAT(ch, mat_type) += base_amount;
        send_to_char(ch, "You recover %d unit%s of %s (%s) from the crafting project.\r\n", base_amount, base_amount == 1 ? "" : "s",
                     crafting_materials[mat_type], crafting_material_groups[craft_group_by_material(mat_type)]);
        GET_CRAFT(ch).materials[group][0] = 0;
        GET_CRAFT(ch).materials[group][1] = 0;
        return;
    }
    else
    {
        if (GET_CRAFT(ch).materials[group][1] > 0)
        {
            send_to_char(ch, "There are already %d units of %s allocated. To remove them, type: craft materials remove %s.\r\n",
                        GET_CRAFT(ch).materials[group][1], crafting_material_groups[GET_CRAFT(ch).materials[group][0]], crafting_material_groups[GET_CRAFT(ch).materials[group][0]]);
            return;
        }
        base_amount = crafting_recipes[GET_CRAFT(ch).crafting_recipe].materials[mat_slot][GET_CRAFT(ch).craft_variant][1];
        owned_amount = GET_CRAFT_MAT(ch, mat_type);
        if (GET_CRAFT_MAT(ch, mat_type) < base_amount)
        {
            send_to_char(ch, "This project requires %d unit%s of %s (%s), but you only have %d unit%s.\r\n",
                         base_amount, base_amount == 1 ? "" : "s", crafting_materials[mat_type],
                         crafting_material_groups[craft_group_by_material(mat_type)], owned_amount, owned_amount == 1 ? "" : "s");
            return;
        }
        GET_CRAFT_MAT(ch, mat_type) -= GET_CRAFT(ch).materials[group][1];
        send_to_char(ch, "You add %d unit%s of %s (%s) to the crafting project.\r\n", base_amount, base_amount == 1 ? "" : "s",
                     crafting_materials[mat_type], crafting_material_groups[craft_group_by_material(mat_type)]);
        GET_CRAFT(ch).materials[group][0] = mat_type;
        GET_CRAFT(ch).materials[group][1] = base_amount;
        GET_CRAFT_MAT(ch, mat_type) -= base_amount;
    }
}

const int craft_skills_alphabetic[END_HARVEST_ABILITIES-START_CRAFT_ABILITIES+1] =
{    
    ABILITY_CRAFT_ALCHEMY,
    ABILITY_CRAFT_ARMORSMITHING,
    ABILITY_CRAFT_BOWMAKING,
    ABILITY_CRAFT_BREWING,
    ABILITY_CRAFT_COOKING,
    ABILITY_CRAFT_FISHING,
    ABILITY_HARVEST_FORESTRY,
    ABILITY_HARVEST_GATHERING,
    ABILITY_HARVEST_HUNTING,
    ABILITY_CRAFT_JEWELCRAFTING,
    ABILITY_CRAFT_LEATHERWORKING,
    ABILITY_CRAFT_METALWORKING,
    ABILITY_HARVEST_MINING,
    ABILITY_CRAFT_POISONMAKING,
    ABILITY_CRAFT_TAILORING,
    ABILITY_CRAFT_TRAPMAKING,
    ABILITY_CRAFT_WEAPONSMITHING,
    ABILITY_CRAFT_WOODWORKING
};

void show_craft_score(struct char_data *ch, const char *arg2)
{
    int i = 0, abil = 0;

    send_to_char(ch, "\r\n");


    send_to_char(ch, "\tC%-25s %-10s %-4s %-6s %-6s\tn\r\n", "SKILL", "TYPE", "RANK", "EXP", "TNL");
    send_to_char(ch, "\tc");
    draw_line(ch, 90, '-', '-');
    send_to_char(ch, "\tn");

    for (i = START_CRAFT_ABILITIES; i <= END_HARVEST_ABILITIES; i++)
    {
        abil = craft_skills_alphabetic[i-START_CRAFT_ABILITIES+1];
        if (crafting_skill_type(abil) != CRAFT_SKILL_TYPE_CRAFT) continue;
        send_to_char(ch, "%-25s %-10s %-4d %-6d %-6d\r\n",
                    ability_names[abil], crafting_skill_type(abil) == CRAFT_SKILL_TYPE_CRAFT ? "Craft" : "Harvest", get_craft_skill_value(ch, abil),
                    GET_CRAFT_SKILL_EXP(ch, abil), craft_skill_level_exp(ch, get_craft_skill_value(ch, abil)+1));
    }

    send_to_char(ch, "\tc");
    draw_line(ch, 90, '-', '-');
    send_to_char(ch, "\tn");

    for (i = START_CRAFT_ABILITIES; i <= END_HARVEST_ABILITIES; i++)
    {
        abil = craft_skills_alphabetic[i-START_CRAFT_ABILITIES+1];
        if (crafting_skill_type(abil) != CRAFT_SKILL_TYPE_HARVEST) continue;
        send_to_char(ch, "%-25s %-10s %-4d %-6d %-6d\r\n",
                    ability_names[abil], crafting_skill_type(abil) == CRAFT_SKILL_TYPE_CRAFT ? "Craft" : "Harvest", get_craft_skill_value(ch, abil),
                    GET_CRAFT_SKILL_EXP(ch, abil), craft_skill_level_exp(ch, get_craft_skill_value(ch, abil)+1));
    }
    send_to_char(ch, "\tc");
    draw_line(ch, 90, '-', '-');
    send_to_char(ch, "\tn");
}

void set_crafting_enhancement(struct char_data *ch, const char *arg2)
{
    int amount = 0,
        max = 8;

    if (GET_CRAFT(ch).short_description == NULL || GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_NONE)
    {
        send_to_char(ch, "You must provide the crafting object short description and item type first.\r\n");
        return;
    }

    if (!*arg2)
    {
        send_to_char(ch, "Please specify the enhancement amount (weapons and armor only).\r\n");
        return;
    }
    
    if (GET_CRAFT(ch).crafting_item_type != CRAFT_TYPE_ARMOR && GET_CRAFT(ch).crafting_item_type != CRAFT_TYPE_WEAPON)
    {
        send_to_char(ch, "You can only set an enhancement bonus on weapons, armor and shields.\r\n");
        return;
    }

    if (is_abbrev(arg2, "reset"))
    {
        send_to_char(ch, "You reset your project's enhancement bonus to 0.\r\n");
        int mote_type = get_enhancement_mote_type(ch, GET_CRAFT(ch).crafting_item_type, GET_CRAFT(ch).crafting_specific);
        if (GET_CRAFT(ch).enhancement_motes_required > 0)
        {
            send_to_char(ch, "You've recovered %d %s.\r\n", GET_CRAFT(ch).enhancement_motes_required, crafting_motes[mote_type]);
            GET_CRAFT_MOTES(ch, mote_type) += GET_CRAFT(ch).enhancement_motes_required;
            GET_CRAFT(ch).enhancement_motes_required = 0;
        }
        GET_CRAFT(ch).enhancement = 0;
        return;
    }

    if (GET_CRAFT(ch).enhancement > 0)
    {
        send_to_char(ch, "You have already set the  item's enhancement bonus. To change it you'll need to reset it first with: craft enhancement reset.\r\n");
        return;
    }

    amount = atoi(arg2);

    if (amount <= 0 || amount > max)
    {
        send_to_char(ch, "Please specify an amount between 1 and %d.\r\n", max);
        return;
    }

    GET_CRAFT(ch).enhancement = amount;
    send_to_char(ch, "You set your project's enhancement bonus to %d.\r\n", amount);

}

void newcraft_create(struct char_data *ch, const char *argument)
{
    char arg1[200], arg2[MAX_EXTRA_DESC];

    half_chop_c(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

    if (!*arg1)
    {
        send_to_char(ch, "%s", NEWCRAFT_CREATE_NOARG1);
        return;
    }
    else if (is_abbrev(arg1, "itemtype") || is_abbrev(arg1, "type"))
    {
        set_crafting_itemtype(ch, arg2);
        return;
    }
    else if (is_abbrev(arg1, "specifictype"))
    {
        if (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_NONE)
        {
            send_to_char(ch, "You need to set the crafting type first, using: craft itemtype (type)\r\n");
            return;
        }
        if (GET_CRAFT(ch).crafting_specific != 0)
        {
            send_to_char(ch, "You have already set the crafting specific type. To change it, you'll need to reset it first with: craft reset.\r\n");
            return;
        }
        switch (GET_CRAFT(ch).crafting_item_type)
        {
            case CRAFT_TYPE_WEAPON:
                set_craft_weapon_type(ch, arg2);
                break;
            case CRAFT_TYPE_ARMOR:
                set_craft_armor_type(ch, arg2);
                break;
            case CRAFT_TYPE_INSTRUMENT:
                set_craft_instrument_type(ch, arg2);
                break;
            case CRAFT_TYPE_MISC:
                set_craft_misc_type(ch, arg2);
                break;
            default:
                send_to_char(ch, "You need to set the crafting type first, using: craft itemtype (type)\r\n");
                return;
        }
        return;
    }
    else if (is_abbrev(arg1, "variant"))
    {
        set_crafting_variant(ch, arg2);
        return;
    }
    else if (is_abbrev(arg1, "keywords"))
    {
        set_crafting_keywords(ch, arg2);
    }
    else if (is_abbrev(arg1, "shortdesc"))
    {
        set_crafting_short_desc(ch, arg2);
    }
    else if (is_abbrev(arg1, "roomdesc"))
    {
        set_crafting_room_desc(ch, arg2);
    }
    else if (is_abbrev(arg1, "extradesc"))
    {
        set_crafting_extra_desc(ch, arg2);
    }
    else if (is_abbrev(arg1, "bonuses"))
    {
        set_crafting_bonuses(ch, arg2);
    }
    else if (is_abbrev(arg1, "enhancement"))
    {
        set_crafting_enhancement(ch, arg2);
    }
    else if (is_abbrev(arg1, "materials"))
    {
        set_crafting_materials(ch, arg2);
    }
    else if (is_abbrev(arg1, "motes"))
    {
        set_crafting_motes(ch, arg2);
    }
    else if (is_abbrev(arg1, "instrument"))
    {
        set_crafting_instrument(ch, arg2);
    }
    else if (is_abbrev(arg1, "leveladjust"))
    {
        set_craft_level_adjust(ch, arg2);
    }
    else if (is_abbrev(arg1, "score"))
    {
        show_craft_score(ch, arg2);
    }
    else if (is_abbrev(arg1, "display") || is_abbrev(arg1, "show") || is_abbrev(arg1, "review") || is_abbrev(arg1, "information"))
    {
        show_current_craft(ch);
    }
    else if (is_abbrev(arg1, "reset"))
    {
        reset_current_craft(ch, arg2, true, true);
    }
    else if (is_abbrev(arg1, "check"))
    {
        check_current_craft(ch, true);
    }
    else if (is_abbrev(arg1, "start") || is_abbrev(arg1, "begin"))
    {
        begin_current_craft(ch);
    }
    else
    {
        send_to_char(ch, "%s", NEWCRAFT_CREATE_NOARG1);
        return;
    }
}

void newcraft_survey(struct char_data *ch, const char *argument)
{
    int seconds = 0;

    if (GET_CRAFT(ch).craft_duration > 0)
    {
        send_to_char(ch, "You cannot survey until you complete your current task. To cancel, type 'cancel craft'.\r\n");
        return;
    }

    if (!is_valid_harvesting_sector(world[IN_ROOM(ch)].sector_type))
    {
        send_to_char(ch, "This locale does not provide harvested resources.\r\n");
        return;
    }

    if (GET_LEVEL(ch) >= LVL_IMMORT)
        seconds = 1;
    else
        seconds = SURVEY_BASE_TIME;

    GET_CRAFT(ch).crafting_method = SCMD_NEWCRAFT_SURVEY;
    GET_CRAFT(ch).craft_duration = seconds;

    send_to_char(ch, "You begin surveying the immediate area for harvestable materials.\r\n");
    act("$n starts surveying.", FALSE, ch, 0, 0, TO_ROOM);
}

void craft_refine_complete(struct char_data *ch)
{
    int roll, dc, skill, skill_type, num = 0;

    if (GET_CRAFT(ch).refining_result[0] == 0 || GET_CRAFT(ch).refining_result[1] == 0 )
    {
        send_to_char(ch, "Refining result error. Please inform staff.\r\n ");
        return;
    }

    roll = d20(ch);
    dc = GET_CRAFT(ch).dc;
    skill_type = GET_CRAFT(ch).skill_type;
    skill = get_craft_skill_value(ch, skill_type);
    num = GET_CRAFT(ch).refining_result[1];

    if ((20 + skill) < dc)
    {
        send_to_char(ch, "That refining type is too complex for you.\r\n");
        reset_current_craft(ch, NULL, true, true);
        return;
    }
    else if (roll == 1)
    {
        send_to_char(ch, "\tM[CRITICAL FAILURE]\tn You rolled a natural 1! Your refining attempt failed and you lost your materials.\r\n");
        reset_current_craft(ch, NULL, false, false);
        return;
    }
    else if (roll == 20)
    {
        send_to_char(ch, "\tM[CRITICAL SUCCESS]\tn You rolled a natural 20! Your refining attempt succeeded and you gained an extra unit of %s.\r\n", crafting_materials[GET_CRAFT(ch).refining_result[0]]);
        num++;
        gain_craft_exp(ch, (REFINE_BASE_EXP+dc)*num, skill_type, true);
    }
    else if ((roll + skill) < dc)
    {
        send_to_char(ch, "You rolled %d + skill %d for a total of %d < dc of %d. You failed your refining attempt but may try again.\r\n", roll, skill, roll + skill, dc);
        return;
    }
    else
    {
        send_to_char(ch, "You rolled %d + skill %d for a total of %d >= dc of %d. You succeed!\r\n", roll, skill, roll + skill, dc);
        gain_craft_exp(ch, (REFINE_BASE_EXP)*num, skill_type, true);
    }

    GET_CRAFT_MAT(ch, GET_CRAFT(ch).refining_result[0]) += GET_CRAFT(ch).refining_result[1];
    send_to_char(ch, "You refine %d unit%s of %s.\r\n", GET_CRAFT(ch).refining_result[1], GET_CRAFT(ch).refining_result[1] > 1 ? "s" : "", crafting_materials[GET_CRAFT(ch).refining_result[0]]);
    reset_current_craft(ch, NULL, false, false);
    act("$n finishes refining.", FALSE, ch, 0, 0, TO_ROOM);
}

void harvest_complete(struct char_data *ch)
{
    int skill = 0, skill_roll= 0, roll = 0, dc = 0, amount = 0, bonus = 0, harvest_level = 0;
    bool motes_found = false;

    if (world[IN_ROOM(ch)].harvest_material == CRAFT_MAT_NONE || world[IN_ROOM(ch)].harvest_material_amount <= 0)
    {
        send_to_char(ch, "The resource is depleted.\r\n");
        return;
    }

    skill = harvesting_skill_by_material(world[IN_ROOM(ch)].harvest_material);

    if (skill == 0)
    {
        send_to_char(ch, "There was an error harvesting %s. Please inform staff.\r\n", crafting_materials[world[IN_ROOM(ch)].harvest_material]);
        return;
    }

    roll = d20(ch);
    skill_roll = get_craft_skill_value(ch, skill);
    harvest_level = MAX(1, material_grade(world[IN_ROOM(ch)].harvest_material) * 5);
    dc = HARVEST_BASE_DC + (harvest_level);

    if ((20 + skill_roll) < dc)
    {
        send_to_char(ch, "You don't have the skill to harvest %s.\r\n", crafting_material_nodes[world[IN_ROOM(ch)].harvest_material]);
        return;
    }

    if (roll == 1)
    {
        amount = dice(1, 6);
        amount = MIN(amount, world[IN_ROOM(ch)].harvest_material_amount);
        send_to_char(ch, "\tM[CRITICAL FAILURE]\tn You rolled a natural 1! %d of the %s units in this room have been ruined!\r\n",
                     amount, crafting_materials[world[IN_ROOM(ch)].harvest_material]);
        
        world[IN_ROOM(ch)].harvest_material_amount -= amount;
        
        if (world[IN_ROOM(ch)].harvest_material_amount <= 0)
        {
            send_to_char(ch, "%s has been depleted.\r\n", crafting_material_nodes[world[IN_ROOM(ch)].harvest_material]);
            world[IN_ROOM(ch)].harvest_material = CRAFT_MAT_NONE;
            world[IN_ROOM(ch)].harvest_material_amount = 0;
        }
        GET_CRAFT(ch).craft_duration = 0;
        GET_CRAFT(ch).crafting_method = 0;
        return;
    }
    else if ((roll + skill_roll) < dc)
    {
        send_to_char(ch, "Roll [%d] + Skill [%d] = Total [%d] vs. DC [%d]. Failure. You have failed to harvest %s.\r\n", 
                         roll, skill_roll, roll + skill_roll, dc, crafting_materials[world[IN_ROOM(ch)].harvest_material]);
        GET_CRAFT(ch).craft_duration = 0;
        GET_CRAFT(ch).crafting_method = 0;
        gain_craft_exp(ch, HARVEST_BASE_EXP/2, skill, true);
        return;
    }
    else 
    {
        amount = HARVEST_BASE_AMOUNT;
        amount = MIN(amount, world[IN_ROOM(ch)].harvest_material_amount);

        if (roll == 20)
        {
            bonus = HARVEST_BASE_AMOUNT;
            send_to_char(ch, "\tM[CRITICAL SUCCESS!]\tn You rolled a natural 20! You've harvested %d units of %s, plus an extra %d units!\r\n", 
                         amount, crafting_materials[world[IN_ROOM(ch)].harvest_material], bonus);
            gain_craft_exp(ch, HARVEST_BASE_EXP + (HARVEST_BASE_EXP * harvest_level), skill, true);
            motes_found = true;
        }
        else
        {
            send_to_char(ch, "Roll [%d] + Skill [%d] = Total [%d] vs. DC [%d]. Success! You have harvested %d %s from %s.\r\n", 
                         roll, skill_roll, roll + skill_roll, dc, amount, 
                         crafting_materials[world[IN_ROOM(ch)].harvest_material], crafting_material_nodes[world[IN_ROOM(ch)].harvest_material]);
            gain_craft_exp(ch, HARVEST_BASE_EXP + ((HARVEST_BASE_EXP/2) * harvest_level), skill, true);
        }
        
        world[IN_ROOM(ch)].harvest_material_amount -= amount;

        GET_CRAFT_MAT(ch, world[IN_ROOM(ch)].harvest_material) += amount + bonus;
        
        if (world[IN_ROOM(ch)].harvest_material_amount <= 0)
        {
            send_to_char(ch, "%s has been depleted.\r\n", crafting_material_nodes[world[IN_ROOM(ch)].harvest_material]);
            world[IN_ROOM(ch)].harvest_material = CRAFT_MAT_NONE;
            world[IN_ROOM(ch)].harvest_material_amount = 0;
        }

        // random motes
        if ((dice(1, 100) <= HARVEST_MOTE_CHANCE) || motes_found)
        {
            int num_motes = dice(MAX(1, material_grade(world[IN_ROOM(ch)].harvest_material)), HARVEST_MOTE_DICE_SIZE);
            int mote_type = dice(1, NUM_CRAFT_MOTES - 1);
            send_to_char(ch, "\tYYou have extracted a small cache of %d %ss!.\r\n", num_motes, crafting_motes[mote_type]);
            GET_CRAFT_MOTES(ch, mote_type) += num_motes;
        }
        
        GET_CRAFT(ch).craft_duration = 0;
        GET_CRAFT(ch).crafting_method = 0;
        act("$n finishes harvesting.", FALSE, ch, 0, 0, TO_ROOM);
    }
}

void newcraft_harvest(struct char_data *ch, const char *argument)
{
    int seconds = 0;

    if (GET_CRAFT(ch).craft_duration > 0)
    {
        send_to_char(ch, "You cannot harvest until you complete your current task. To cancel, type 'cancel craft'.\r\n");
        return;
    }

    if (!ch->player_specials->surveyed_room)
    {
        send_to_char(ch, "You must first survey to locate any resources here.\r\n");
        return;
    }

    if (world[IN_ROOM(ch)].harvest_material == CRAFT_MAT_NONE || world[IN_ROOM(ch)].harvest_material_amount <= 0)
    {
        send_to_char(ch, "There's nothing here to harvest.\r\n");
        return;
    }

    if (GET_LEVEL(ch) >= LVL_IMMORT)
        seconds = 1;
    else
        seconds = HARVEST_BASE_TIME;

    GET_CRAFT(ch).crafting_method = SCMD_NEWCRAFT_HARVEST;
    GET_CRAFT(ch).craft_duration = seconds;

    send_to_char(ch, "You begin %s.\r\n", harvesting_messages[world[IN_ROOM(ch)].harvest_material]);
    act("$n starts harvesting.", FALSE, ch, 0, 0, TO_ROOM);
}

void gain_craft_exp(struct char_data *ch, int exp, int abil, bool verbose)
{
    GET_CRAFT_SKILL_EXP(ch, abil) += exp;
    if (verbose)
        send_to_char(ch, "You've gained %d experience points in the '%s' skill.\r\n", exp, ability_names[abil]);
    if (GET_CRAFT_SKILL_EXP(ch, abil) >= craft_skill_level_exp(ch, get_craft_skill_value(ch, abil)+1))
    {
        send_to_char(ch, "\tYYour skill in '%s' has increased from %d to %d!\r\n\tn", ability_names[abil], get_craft_skill_value(ch, abil), get_craft_skill_value(ch, abil)+1);
        SET_ABILITY(ch, abil, get_craft_skill_value(ch, abil)+1);
    }
}

void show_refine_noargs(struct char_data *ch)
{
    int i, j;

    send_to_char(ch, "To refine you need to add materials and have the appropriate refining equipment in the same room as you.\r\n"
                     "You need to specify one of the following command parameters: refine add (refine type), refine remove or refine begin.\r\n"
                     "Here are the available refining recipes:\r\n");
    for (i = 1; i < NUM_REFINING_RECIPES; i++)
    {
        send_to_char(ch, "-- %20s (x%d): ", crafting_materials[refining_recipes[i].result[0]], refining_recipes[i].result[1]);
        for (j = 0; j < 3; j++)
        {
            if (refining_recipes[i].materials[j][0] == CRAFT_MAT_NONE) continue;
            if (j > 0)
                send_to_char(ch, " & ");
            send_to_char(ch, "%s (x%d)", crafting_materials[refining_recipes[i].materials[j][0]], refining_recipes[i].materials[j][1]);
        }
        send_to_char(ch, " - requires %.*s.\r\n", 9, extra_bits[refining_recipes[i].crafting_station_flag]+9);

    }
}

void newcraft_refine(struct char_data *ch, const char *argument)
{
    char arg1[50], arg2[50], output[200];
    int i = 0, recipe = 0, material = 0;
    struct obj_data *obj;
    bool fail = false, station = false;

    two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

    if (!*arg1)
    {
        show_refine_noargs(ch);
        return;
    }

    if (is_abbrev(arg1, "add"))
    {
        if (!*arg2)
        {
            show_refine_noargs(ch);
            return;
        }

        if (GET_CRAFT(ch).refining_result[0] != 0)
        {
            send_to_char(ch, "You've already prepared a refining project for %s. Type 'refine remove' to start over.\r\n", crafting_materials[GET_CRAFT(ch).refining_result[0]]);
            return;
        }

        for (i = 1; i < NUM_REFINING_RECIPES; i++)
        {
            if (is_abbrev(arg2, crafting_materials[refining_recipes[i].result[0]]))
            {
                break;
            }
        }
        
        if (i >= NUM_REFINING_RECIPES)
        {
            send_to_char(ch, "That is not a valid refining recipe.\r\n");
            return;
        }

        recipe = i;

        for (i = 0; i < 3; i++)
        {
            if ((material = refining_recipes[recipe].materials[i][0]) != 0)
            {
                if (GET_CRAFT_MAT(ch, material) <= refining_recipes[recipe].materials[i][1])
                {
                    send_to_char(ch, "You need %d units of %s to make %d units of %s.\r\n",
                                    refining_recipes[recipe].materials[i][1], crafting_materials[refining_recipes[recipe].materials[i][0]],
                                    refining_recipes[recipe].result[1], crafting_materials[refining_recipes[recipe].result[0]]);
                    fail = true;
                }
            }
        }

        for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj->next_content)
        {
            if (OBJ_FLAGGED(obj, refining_recipes[recipe].crafting_station_flag))
            {
                station = true;
                break;
            }
        }

        if (!station)
        {
            send_to_char(ch, "You need a %.*s in the room with you.\r\n", 9, extra_bits[refining_recipes[recipe].crafting_station_flag]+9);
            fail = true;
        }


        if (fail)
        {
            send_to_char(ch, "You do not meet all of the requirements to refine %s.\r\n", crafting_materials[refining_recipes[recipe].result[0]]);
            return;
        }

        for (i = 0; i < 3; i++)
        {
            if ((material = refining_recipes[recipe].materials[i][0]) != 0)
            {
                GET_CRAFT_MAT(ch, material) -= refining_recipes[recipe].materials[i][1];
                GET_CRAFT(ch).refining_materials[i][0] = material;
                GET_CRAFT(ch).refining_materials[i][1] = refining_recipes[recipe].materials[i][1];
                send_to_char(ch, "You allocate %d units of %s to your refining of %d units of %s.\r\n",
                                refining_recipes[recipe].materials[i][1], crafting_materials[refining_recipes[recipe].materials[i][0]],
                                refining_recipes[recipe].result[1], crafting_materials[refining_recipes[recipe].result[0]]);
            }
        }

        GET_CRAFT(ch).crafting_recipe = recipe;
        GET_CRAFT(ch).dc = refining_recipes[recipe].dc;
        GET_CRAFT(ch).refining_result[0] = refining_recipes[recipe].result[0];
        GET_CRAFT(ch).refining_result[1] = refining_recipes[recipe].result[1];

        send_to_char(ch, "You are ready to begin refining your %s. Type 'refine begin' to execute.\r\n", crafting_materials[refining_recipes[recipe].result[0]]);
        return;
    }
    else if (is_abbrev(arg1, "remove"))
    {

        if (GET_CRAFT(ch).refining_result[0] == 0)
        {
            send_to_char(ch, "You don't have a refining project going.\r\n");
            return;
        }

        send_to_char(ch, "You cancel your refining project.\r\n");
        reset_current_craft(ch, NULL, true, true);
        return;
    }
    else if (is_abbrev(arg1, "show") || is_abbrev(arg1, "display") || is_abbrev(arg1, "info"))
    {
        if (GET_CRAFT(ch).refining_result[0] == 0)
        {
            send_to_char(ch, "You have not started a refining project.\r\n");
            return;
        }

        send_to_char(ch, "\tc");
        snprintf(output, sizeof(output), "REFINING %s", crafting_materials[GET_CRAFT(ch).refining_result[0]]);
        for (i = 0; i < strlen(output); i++)
            output[i] = toupper(output[i]);
        text_line(ch, output, 80, '-', '-');
        send_to_char(ch, "\tc");

        send_to_char(ch, "MATERIALS:\r\n");
        
        // materials
        for (i = 0; i < 3; i++)
        {
            if (GET_CRAFT(ch).refining_materials[i][0] != 0)
            {
                send_to_char(ch, "-- %d/%d %s unit%s allocated.\r\n",
                             GET_CRAFT(ch).refining_materials[i][1], refining_recipes[GET_CRAFT(ch).crafting_recipe].materials[i][1],
                             crafting_materials[GET_CRAFT(ch).refining_materials[i][0]], GET_CRAFT(ch).refining_materials[i][1] > 1 ? "" : "s");
            }
        }

        send_to_char(ch, "\r\n");

        // result
        send_to_char(ch, "RESULT:\r\n");
        send_to_char(ch, "-- %d unit%s of %s.\r\n", GET_CRAFT(ch).refining_result[1], GET_CRAFT(ch).refining_result[1] > 1 ? "" : "s", crafting_materials[GET_CRAFT(ch).refining_result[0]]);
        send_to_char(ch, "\r\n");
        
        // dc and skill
        send_to_char(ch, "SKILL CHECK:\r\n");
        send_to_char(ch, "-- 1d20 + %s skill of %d vs. dc of %d.\r\n", ability_names[refining_recipes[GET_CRAFT(ch).crafting_recipe].skill],
                    get_craft_skill_value(ch, refining_recipes[GET_CRAFT(ch).crafting_recipe].skill), GET_CRAFT(ch).dc);

        send_to_char(ch, "\tc");
        draw_line(ch, 80, '-', '-');
        send_to_char(ch, "\tn");

        return;
    }
    else if (is_abbrev(arg1, "begin") || is_abbrev(arg1, "start"))
    {
        if (GET_CRAFT(ch).refining_result[0] == 0)
        {
            send_to_char(ch, "You don't have a refining project going.\r\n");
            return;
        }

        if (is_refine_ready(ch, true))
        {
            send_to_char(ch, "Your refining project is not ready yet. Please type: refine add (refine type).\r\n");
            return;
        }

        for (i = 0; i < NUM_REFINING_RECIPES; i++)
        {
            if (refining_recipes[i].result[0] == GET_CRAFT(ch).refining_result[0])
            {
                break;
            }
        }

        if (i >= NUM_REFINING_RECIPES)
        {
            send_to_char(ch, "There was an error beginning you refining project. Please inform a staff member.\r\n");
            return;
        }

        recipe = i;

        GET_CRAFT(ch).skill_type = refining_recipes[recipe].skill;
        GET_CRAFT(ch).dc = refining_recipes[recipe].dc;

        GET_CRAFT(ch).crafting_method = SCMD_NEWCRAFT_REFINE;
        GET_CRAFT(ch).craft_duration = 10*GET_CRAFT(ch).refining_result[1];
        send_to_char(ch, "You begin refining %s.\r\n", crafting_materials[GET_CRAFT(ch).refining_result[0]]);
        act("$n starts refining.", FALSE, ch, 0, 0, TO_ROOM);
        return;
    }
    else
    {
        show_refine_noargs(ch);
        return;
    }    
}

int get_craft_skill_value(struct char_data *ch, int skill_num)
{
    return GET_ABILITY(ch, skill_num);
}

bool is_refine_ready(struct char_data *ch, bool verbose)
{
    bool fail = false;

    if (GET_CRAFT(ch).refining_result[0] == 0)
    {
        if (verbose)
            send_to_char(ch, "You haven't set a refining result/type\r\n");
        fail = true;
    }

    if (GET_CRAFT(ch).refining_result[1] == 0)
    {
        if (verbose)
            send_to_char(ch, "You haven't set a refining result/type\r\n");
        fail = true;
    }

    if ((refining_recipes[GET_CRAFT(ch).crafting_recipe].materials[0][1] != 0 &&
        (GET_CRAFT(ch).refining_materials[0][1] < refining_recipes[GET_CRAFT(ch).crafting_recipe].materials[0][1])))
    {
        if (verbose)
            send_to_char(ch, "You haven't added the primary refining ingredient.\r\n");
        fail = true;
    }

    if ((refining_recipes[GET_CRAFT(ch).crafting_recipe].materials[1][1] != 0 &&
        (GET_CRAFT(ch).refining_materials[1][1] < refining_recipes[GET_CRAFT(ch).crafting_recipe].materials[1][1])))
    {
        if (verbose)
            send_to_char(ch, "You haven't added the secondary refining ingredient.\r\n");
        fail = true;
    }

    if ((refining_recipes[GET_CRAFT(ch).crafting_recipe].materials[2][1] != 0 &&
        (GET_CRAFT(ch).refining_materials[2][1] < refining_recipes[GET_CRAFT(ch).crafting_recipe].materials[2][1])))
    {
        if (verbose)
            send_to_char(ch, "You haven't added the tertiary refining ingredient.\r\n");
        fail = true;
    }

    return fail;

}

bool is_valid_craft_ability(int ability)
{
    switch (ability)
    {
        case ABILITY_ACROBATICS:
        case ABILITY_STEALTH:
        case ABILITY_RELIGION:
        case ABILITY_PERCEPTION:
        case ABILITY_ATHLETICS:
        case ABILITY_MEDICINE:
        case ABILITY_INTIMIDATE:
        case ABILITY_CONCENTRATION:
        case ABILITY_SPELLCRAFT:
        case ABILITY_APPRAISE:
        case ABILITY_DISCIPLINE:
        case ABILITY_TOTAL_DEFENSE:
        case ABILITY_ARCANA:
        case ABILITY_RIDE:
        case ABILITY_HISTORY:
        case ABILITY_SLEIGHT_OF_HAND:
        case ABILITY_DECEPTION:
        case ABILITY_PERSUASION:
        case ABILITY_DISABLE_DEVICE:
        case ABILITY_DISGUISE:
        case ABILITY_HANDLE_ANIMAL:
        case ABILITY_INSIGHT:
        case ABILITY_NATURE:
        case ABILITY_USE_MAGIC_DEVICE:
        case ABILITY_PERFORM:
        case ABILITY_CRAFT_WOODWORKING:
        case ABILITY_CRAFT_TAILORING:
        case ABILITY_CRAFT_ALCHEMY:
        case ABILITY_CRAFT_ARMORSMITHING:
        case ABILITY_CRAFT_WEAPONSMITHING:
        case ABILITY_CRAFT_BOWMAKING:
        case ABILITY_CRAFT_JEWELCRAFTING:
        case ABILITY_CRAFT_LEATHERWORKING:
        case ABILITY_CRAFT_TRAPMAKING:
        case ABILITY_CRAFT_POISONMAKING:
        case ABILITY_CRAFT_METALWORKING:
        case ABILITY_CRAFT_FISHING:
        case ABILITY_CRAFT_COOKING:
        case ABILITY_CRAFT_BREWING:
        case ABILITY_HARVEST_MINING:
        case ABILITY_HARVEST_HUNTING:
        case ABILITY_HARVEST_FORESTRY:
        case ABILITY_HARVEST_GATHERING:
            return true;
    }
    return false;
}

int crafting_skill_type(int skill)
{
    switch (skill)
    {
        case ABILITY_CRAFT_WOODWORKING:
        case ABILITY_CRAFT_TAILORING:
        case ABILITY_CRAFT_ARMORSMITHING:
        case ABILITY_CRAFT_WEAPONSMITHING:
        case ABILITY_CRAFT_LEATHERWORKING:
        case ABILITY_CRAFT_JEWELCRAFTING:
        case ABILITY_CRAFT_METALWORKING:
            return CRAFT_SKILL_TYPE_CRAFT;
        
        case ABILITY_HARVEST_MINING:
        case ABILITY_HARVEST_HUNTING:
        case ABILITY_HARVEST_FORESTRY:
        case ABILITY_HARVEST_GATHERING:
            return CRAFT_SKILL_TYPE_HARVEST;

        case ABILITY_CRAFT_ALCHEMY:
        case ABILITY_CRAFT_BOWMAKING:
        case ABILITY_CRAFT_TRAPMAKING:
        case ABILITY_CRAFT_POISONMAKING:
        case ABILITY_CRAFT_FISHING:
        case ABILITY_CRAFT_COOKING:
        case ABILITY_CRAFT_BREWING:
            return CRAFT_SKILL_TYPE_NONE;
    }
    return CRAFT_SKILL_TYPE_NONE;
}

bool is_valid_craft_feat(int feat)
{
    if (feat <= FEAT_UNDEFINED || feat >= FEAT_LAST_FEAT)
        return false;

    // must bne learnable
    if (!feat_list[feat].can_learn)
        return false;

    // cannot be epic
    if (feat_list[feat].epic)
        return false;

    // no combat feats for now. Requires extra code to pick which weapon that we aren't spending time on yet
    if (!feat_list[feat].combat_feat)
        return false;

    // no skill or spell focus for similar reason
    switch (feat)
    {
        case FEAT_SPELL_FOCUS:
        case FEAT_GREATER_SPELL_FOCUS:
        case FEAT_SKILL_FOCUS:
            return false;
    }
    
    // we only allow general, combat, spellcasting, metamagic, psionic and teamwork feats
    if (feat_list[feat].feat_type == FEAT_TYPE_GENERAL ||
        feat_list[feat].feat_type == FEAT_TYPE_COMBAT ||
        feat_list[feat].feat_type == FEAT_TYPE_SPELLCASTING ||
        feat_list[feat].feat_type == FEAT_TYPE_PSIONIC ||
        feat_list[feat].feat_type == FEAT_TYPE_METAMAGIC ||
        feat_list[feat].feat_type == FEAT_TYPE_TEAMWORK)
        return true;

    return false;
}

bool is_valid_craft_class(int ch_class, int location)
{
    if (!IS_SPELLCASTER_CLASS(ch_class))
        return false;
    
    if (ch_class == CLASS_WARLOCK)
        return false;

    switch (location)
    {
    case APPLY_SPELL_CIRCLE_1:
    case APPLY_SPELL_CIRCLE_2:
    case APPLY_SPELL_CIRCLE_3:
    case APPLY_SPELL_CIRCLE_4:
        switch (ch_class)
        {
            case CLASS_WIZARD:
            case CLASS_SORCERER:
            case CLASS_BARD:
            case CLASS_INQUISITOR:
            case CLASS_SUMMONER:
            case CLASS_RANGER:
            case CLASS_PALADIN:
            case CLASS_CLERIC:
            case CLASS_DRUID:
            case CLASS_ALCHEMIST:
                return true;
        }
        break;
    case APPLY_SPELL_CIRCLE_5:
    case APPLY_SPELL_CIRCLE_6:
        switch (ch_class)
        {
            case CLASS_WIZARD:
            case CLASS_SORCERER:
            case CLASS_BARD:
            case CLASS_INQUISITOR:
            case CLASS_SUMMONER:
            case CLASS_CLERIC:
            case CLASS_DRUID:
            case CLASS_ALCHEMIST:
                return true;
        }
        break;
    case APPLY_SPELL_CIRCLE_7:
    case APPLY_SPELL_CIRCLE_8:
    case APPLY_SPELL_CIRCLE_9:
        switch (ch_class)
        {
            case CLASS_WIZARD:
            case CLASS_SORCERER:
            case CLASS_CLERIC:
            case CLASS_DRUID:
                return true;
        }
        break;
    }
    return false;
}

int craft_recipe_by_type(int type)
{
    switch (type)
    {
        case CRAFT_TYPE_WEAPON:
            return ITEM_WEAPON;
        case CRAFT_TYPE_ARMOR:
            return ITEM_ARMOR;
        case CRAFT_TYPE_MISC:
            return ITEM_WORN;
        case CRAFT_TYPE_INSTRUMENT:
            return ITEM_INSTRUMENT;
    }
    return 0;
}

int craft_misc_type_by_wear_loc(int wear_loc)
{
    switch (wear_loc)
    {
        case ITEM_WEAR_FINGER:
        case ITEM_WEAR_NECK:
        case ITEM_WEAR_WRIST:
        case ITEM_WEAR_EAR:
        case ITEM_WEAR_EYES:
        case ITEM_WEAR_FEET:
        case ITEM_WEAR_HANDS:
        case ITEM_WEAR_ABOUT:
        case ITEM_WEAR_WAIST:
        case ITEM_WEAR_FACE:
        case ITEM_WEAR_ANKLE:
        case ITEM_WEAR_SHOULDERS:
            return CRAFT_TYPE_MISC;
    }
    return CRAFT_TYPE_NONE;
}

void craft_update(void)
{
    struct descriptor_data *d;
    struct char_data *ch;
    int i = 0;
    char buf[200];

    for (d = descriptor_list; d; d = d->next)
    {
        ch = d->character;

        if (!ch) continue;

        if (GET_CRAFT(ch).craft_duration > 0)
        {
            GET_CRAFT(ch).craft_duration--;
            GET_CRAFT(ch).craft_duration = MAX(GET_CRAFT(ch).craft_duration, 0);

            if (GET_CRAFT(ch).craft_duration == 0)
            {
                switch (GET_CRAFT(ch).crafting_method)
                {
                    case SCMD_NEWCRAFT_CREATE:
                        craft_create_complete(ch);
                        break;
                    case SCMD_NEWCRAFT_REFINE:
                        craft_refine_complete(ch);
                        break;
                    case SCMD_NEWCRAFT_RESIZE:
                        craft_resize_complete(ch);
                        break;
                    case SCMD_NEWCRAFT_SURVEY:
                        if (GET_CRAFT(ch).craft_duration && !PRF_FLAGGED(ch, PRF_NO_CRAFT_PROGRESS))
                        {
                            send_to_char(ch, "Surveying. ");
                            for (i = 0; i < GET_CRAFT(ch).craft_duration ; i++)
                                send_to_char(ch, "*");
                            send_to_char(ch, "\r\n");
                        }
                        else
                        {
                            survey_complete(ch);
                        }
                        break;
                    case SCMD_NEWCRAFT_HARVEST:
                        if (GET_CRAFT(ch).craft_duration && !PRF_FLAGGED(ch, PRF_NO_CRAFT_PROGRESS))
                        {
                            if (crafting_material_nodes[world[IN_ROOM(ch)].harvest_material_amount] <= 0)
                            {
                                send_to_char(ch, "The resource is depleted.\r\n");
                                GET_CRAFT(ch).crafting_method = 0;
                                GET_CRAFT(ch).craft_duration = 0;
                            }
                            else
                            {
                                snprintf(buf, sizeof(buf), "%s", harvesting_messages[world[IN_ROOM(ch)].harvest_material]);
                                CAP(buf);
                                send_to_char(ch, "%s. ", buf);
                                for (i = 0; i < GET_CRAFT(ch).craft_duration ; i++)
                                    send_to_char(ch, "*");
                                send_to_char(ch, "\r\n");
                            }
                        }
                        else
                        {
                            harvest_complete(ch);
                        }
                        break;
                    case SCMD_NEWCRAFT_SUPPLYORDER:
                        craft_supplyorder_complete(ch);
                        break;
                }
            }
            else
            {
                if (!PRF_FLAGGED(ch, PRF_NO_CRAFT_PROGRESS))
                {
                    switch (GET_CRAFT(ch).crafting_method)
                    {
                        case SCMD_NEWCRAFT_CREATE:
                            send_to_char(ch, "Crafting %s. ", GET_CRAFT(ch).short_description);
                            for (i = 0; i < GET_CRAFT(ch).craft_duration ; i++)
                                send_to_char(ch, "*");
                            send_to_char(ch, "\r\n");
                            break;
                        case SCMD_NEWCRAFT_REFINE:
                            send_to_char(ch, "Refining %s. ", crafting_materials[GET_CRAFT(ch).refining_result[0]]);
                            for (i = 0; i < GET_CRAFT(ch).craft_duration ; i++)
                                send_to_char(ch, "*");
                            send_to_char(ch, "\r\n");
                            break;
                        case SCMD_NEWCRAFT_SURVEY:
                            send_to_char(ch, "Surveying. ");
                            for (i = 0; i < GET_CRAFT(ch).craft_duration ; i++)
                                send_to_char(ch, "*");
                            send_to_char(ch, "\r\n");
                            break;
                        case SCMD_NEWCRAFT_RESIZE:
                            send_to_char(ch, "Resizing. ");
                            for (i = 0; i < GET_CRAFT(ch).craft_duration ; i++)
                                send_to_char(ch, "*");
                            send_to_char(ch, "\r\n");
                            break;
                        case SCMD_NEWCRAFT_HARVEST:
                            if (crafting_material_nodes[world[IN_ROOM(ch)].harvest_material_amount] <= 0)
                            {
                                send_to_char(ch, "The resource is depleted.\r\n");
                                GET_CRAFT(ch).crafting_method = 0;
                                GET_CRAFT(ch).craft_duration = 0;
                            }
                            else
                            {
                                snprintf(buf, sizeof(buf), "%s", harvesting_messages[world[IN_ROOM(ch)].harvest_material]);
                                CAP(buf);
                                send_to_char(ch, "%s. ", buf);
                                for (i = 0; i < GET_CRAFT(ch).craft_duration ; i++)
                                    send_to_char(ch, "*");
                                send_to_char(ch, "\r\n");
                            }
                            break;
                        case SCMD_NEWCRAFT_SUPPLYORDER:
                            send_to_char(ch, "Supply Order Requistion #%d. ", GET_CRAFT(ch).supply_num_required - num_supply_order_requisitions_to_go(ch) + 1);
                            for (i = 0; i < GET_CRAFT(ch).craft_duration ; i++)
                                send_to_char(ch, "*");
                            send_to_char(ch, "\r\n");
                            break;
                    }
                }
            }
        }
    }
}

ACMD(do_setmaterial)
{
    int i = 0, j = 0, type = 0, amount = 0;
    char target[100], mat_type[100], mat_amount[100], buf[200];
    struct char_data *tch = NULL;

    three_arguments(argument, target, sizeof(target), mat_type, sizeof(mat_type), mat_amount, sizeof(mat_amount));

    if (!*target || !*mat_type || !*mat_amount)
    {
        send_to_char(ch, "You need to specify whose materials to change, the material type, and how many to give/reduce.\r\n"
                         "Eg. setmaterial gicker alchemical-silver 10.\r\n"
                         "This will set gicker's alchemical silver material count to 10.\r\n");
    }

    if (!(tch = get_char_vis(ch, target, NULL, FIND_CHAR_WORLD)))
    {
        send_to_char(ch, "There is no one by that name online.\r\n");
        return;
    }

    if (IS_NPC(tch))
    {
        send_to_char(ch, "You cannot set materials on NPCs.\r\n");
        return;
    }

    for (i = 1; i < NUM_CRAFT_MATS; i++)
    {
        for (j = 0; j < strlen(mat_type); j++)
            if (mat_type[j] == '-')
                mat_type[j] = ' ';
        if (is_abbrev(mat_type, crafting_materials[i]))
        {
            type = i;
            break;
        }
    }

    if (type == 0)
    {
        for (i = 1; i < NUM_CRAFT_MOTES; i++)
        {
            for (j = 0; j < strlen(mat_type); j++)
                if (mat_type[j] == '-')
                    mat_type[j] = ' ';
            if (is_abbrev(mat_type, crafting_motes[i]))
            {
                type = i;
                break;
            }
        }
        if (type == 0)
        {
            send_to_char(ch, "That is not a valid crafting material. If the material name has multiple words, connect them with a dash - instead of a space.\r\n");
            return;
        }
        if ((amount = atoi(mat_amount)) == 0)
        {
            send_to_char(ch, "You must specify a positive or negative number. Positive will give mote units, negative will take them away.\r\n");
            return;
        }
        else
        {
            GET_CRAFT_MOTES(tch, type) = amount;
            if (GET_CRAFT_MOTES(tch, type) < 0)
            {
                snprintf(buf, sizeof(buf), "$n has set your %s units to 0.", crafting_motes[type]);
                act(buf, FALSE, ch, 0, tch, TO_VICT);
                snprintf(buf, sizeof(buf), "You have set $N's %s units to 0.", crafting_motes[type]);
                act(buf, FALSE, ch, 0, tch, TO_CHAR);
                GET_CRAFT_MOTES(tch, type) = 0;
            }
            else
            {
                snprintf(buf, sizeof(buf), "$n has set your %s units to %d.", crafting_motes[type], GET_CRAFT_MOTES(tch, type));
                act(buf, FALSE, ch, 0, tch, TO_VICT);
                snprintf(buf, sizeof(buf), "You have set $N's %s units to %d.", crafting_motes[type], GET_CRAFT_MOTES(tch, type));
                act(buf, FALSE, ch, 0, tch, TO_CHAR);   
            }
            return;
        }
    }

    if ((amount = atoi(mat_amount)) == 0)
    {
        send_to_char(ch, "You must specify a positive or negative number. Positive will give material units, negative will take them away.\r\n");
        return;
    }
    else
    {
        GET_CRAFT_MAT(tch, type) = amount;
        if (GET_CRAFT_MAT(tch, type) < 0)
        {
            snprintf(buf, sizeof(buf), "$n has set your %s material units to 0.", crafting_materials[type]);
            act(buf, FALSE, ch, 0, tch, TO_VICT);
            snprintf(buf, sizeof(buf), "You have set $N's %s material units to 0.", crafting_materials[type]);
            act(buf, FALSE, ch, 0, tch, TO_CHAR);
            GET_CRAFT_MAT(tch, type) = 0;
        }
        else
        {
            snprintf(buf, sizeof(buf), "$n has set your %s material units to %d.", crafting_materials[type], GET_CRAFT_MAT(tch, type));
            act(buf, FALSE, ch, 0, tch, TO_VICT);
            snprintf(buf, sizeof(buf), "You have set $N's %s material units to %d.", crafting_materials[type], GET_CRAFT_MAT(tch, type));
            act(buf, FALSE, ch, 0, tch, TO_CHAR);   
        }
    }
}

ACMD(do_list_craft_materials)
{
    int i = 0, mat = 0, count = 0;
    
    send_to_char(ch, "\tc");
    text_line(ch, "HARD METALS", 80, '-', '-');
    send_to_char(ch, "\tn");
    
    for (i = 2; i < NUM_CRAFT_MATS+1; i++)
    {
        mat = materials_sort_info[i];
        if (craft_group_by_material(mat) != CRAFT_GROUP_HARD_METALS) continue;
        send_to_char(ch, "%5d %-20s ", GET_CRAFT_MAT(ch, mat), crafting_materials[mat]);
        if ((count % 3) == 2)
            send_to_char(ch, "\r\n");
        count++;
    }
    
    if ((count % 3) == 1)
        send_to_char(ch, "\r\n");

    count = 0;
    send_to_char(ch, "\tc");
    text_line(ch, "SOFT METALS", 80, '-', '-');
    send_to_char(ch, "\tn");
    
    for (i = 2; i < NUM_CRAFT_MATS+1; i++)
    {
        mat = materials_sort_info[i];
        if (craft_group_by_material(mat) != CRAFT_GROUP_SOFT_METALS) continue;
        send_to_char(ch, "%5d %-20s ", GET_CRAFT_MAT(ch, mat), crafting_materials[mat]);
        if ((count % 3) == 2)
            send_to_char(ch, "\r\n");
        count++;
    }
    
    if ((count % 3) == 0)
        send_to_char(ch, "\r\n");
    send_to_char(ch, "\r\n");

    count = 0;
    send_to_char(ch, "\tc");
    text_line(ch, "HIDES", 80, '-', '-');
    send_to_char(ch, "\tn");
    
    for (i = 2; i < NUM_CRAFT_MATS+1; i++)
    {
        mat = materials_sort_info[i];
        if (craft_group_by_material(mat) != CRAFT_GROUP_HIDES) continue;
        send_to_char(ch, "%5d %-20s ", GET_CRAFT_MAT(ch, mat), crafting_materials[mat]);
        if ((count % 3) == 2)
            send_to_char(ch, "\r\n");
        count++;
    }
    
    if ((count % 3) == 0)
        send_to_char(ch, "\r\n");
    send_to_char(ch, "\r\n");

    count = 0;
    send_to_char(ch, "\tc");
    text_line(ch, "WOOD", 80, '-', '-');
    send_to_char(ch, "\tn");
    
    for (i = 2; i < NUM_CRAFT_MATS+1; i++)
    {
        mat = materials_sort_info[i];
        if (craft_group_by_material(mat) != CRAFT_GROUP_WOOD) continue;
        send_to_char(ch, "%5d %-20s ", GET_CRAFT_MAT(ch, mat), crafting_materials[mat]);
        if ((count % 3) == 2)
            send_to_char(ch, "\r\n");
        count++;
    }
    
    if ((count % 3) == 1)
        send_to_char(ch, "\r\n");

    count = 0;
    send_to_char(ch, "\tc");
    text_line(ch, "CLOTH", 80, '-', '-');
    send_to_char(ch, "\tn");
    
    for (i = 2; i < NUM_CRAFT_MATS+1; i++)
    {
        mat = materials_sort_info[i];
        if (craft_group_by_material(mat) != CRAFT_GROUP_CLOTH) continue;
        send_to_char(ch, "%5d %-20s ", GET_CRAFT_MAT(ch, mat), crafting_materials[mat]);
        if ((count % 3) == 2)
            send_to_char(ch, "\r\n");
        count++;
    }
    
    if ((count % 3) == 0)
        send_to_char(ch, "\r\n");
    send_to_char(ch, "\r\n");

    count = 0;
    send_to_char(ch, "\tc");
    text_line(ch, "REFINING MATERIALS", 80, '-', '-');
    send_to_char(ch, "\tn");
    
    for (i = 2; i < NUM_CRAFT_MATS+1; i++)
    {
        mat = materials_sort_info[i];
        if (craft_group_by_material(mat) != CRAFT_GROUP_REFINING) continue;
        send_to_char(ch, "%5d %-20s ", GET_CRAFT_MAT(ch, mat), crafting_materials[mat]);
        if ((count % 3) == 2)
            send_to_char(ch, "\r\n");
        count++;
    }
    
    if ((count % 3) == 0)
        send_to_char(ch, "\r\n");
    send_to_char(ch, "\r\n");

    count = 0;
    send_to_char(ch, "\tc");
    text_line(ch, "ELEMENTAL MOTES", 80, '-', '-');
    send_to_char(ch, "\tn");
    
    for (i = 1; i < NUM_CRAFT_MOTES; i++)
    {
        send_to_char(ch, "%5d %-20s ", GET_CRAFT_MOTES(ch, i), crafting_motes[i]);
        if ((count % 3) == 2)
            send_to_char(ch, "\r\n");
        count++;
    }
    
    if ((count % 3) == 0)
        send_to_char(ch, "\r\n");
    send_to_char(ch, "\r\n");
    
    send_to_char(ch, "\tc");
    draw_line(ch, 80, '-', '-');
    send_to_char(ch, "\tn");
}

int compare_materials(const void *x, const void *y)
{
  int a = *(const int *)x,
      b = *(const int *)y;

  return strcmp(crafting_materials[a], crafting_materials[b]);
}

/* sort materials called at boot up */
void sort_materials(void)
{
  int a;

  /* initialize array, avoiding reserved. */
  for (a = 1; a < NUM_CRAFT_MATS; a++)
    materials_sort_info[a] = a;

  qsort(&materials_sort_info[1], NUM_CRAFT_MATS, sizeof(int), compare_materials);
}

int craft_skill_level_exp(struct char_data *ch, int level)
{
    if (level == 0)
        return 0;
    else
        return (level * 1000) + craft_skill_level_exp(ch, level-1);
}

int get_level_adjustment_by_apply_and_modifier(int apply, int mod, int btype)
{
    if (!is_valid_apply(apply) || mod == 0 || btype == BONUS_TYPE_UNDEFINED)
        return 0;

    int level_adj = 0;  // this is the level adjustment returned, added to the object min level to use
    float div = 1.0;    // this is how much to divide the modifier by.

    switch (apply)
    {
        case APPLY_STR:
        case APPLY_DEX:
        case APPLY_INT:
        case APPLY_WIS:
        case APPLY_CON:
        case APPLY_CHA:
        case APPLY_SPELL_PENETRATION:
        case APPLY_PSP_REGEN:
        case APPLY_HP_REGEN:
        case APPLY_FAST_HEALING:
        case APPLY_DAMROLL:
        case APPLY_SKILL:
            div = 6.0;
            break;

        case APPLY_HIT:
            div = 0.5;
            break;

        case APPLY_RES_FIRE:
        case APPLY_RES_SLICE:
        case APPLY_RES_ELECTRIC:
        case APPLY_RES_SOUND:
        case APPLY_RES_COLD:
        case APPLY_RES_PUNCTURE:
        case APPLY_RES_POISON:
        case APPLY_RES_WATER:
        case APPLY_RES_EARTH:
        case APPLY_RES_ACID:
        case APPLY_RES_AIR:
        case APPLY_RES_FORCE:
        case APPLY_RES_ILLUSION:
        case APPLY_RES_ENERGY:
        case APPLY_RES_HOLY:
        case APPLY_RES_MENTAL:
        case APPLY_RES_LIGHT:
        case APPLY_RES_UNHOLY:
        case APPLY_RES_DISEASE:
        case APPLY_RES_NEGATIVE:
        case APPLY_POWER_RES:
        case APPLY_SPELL_DURATION:
        case APPLY_SPELL_POTENCY:
        case APPLY_ENCUMBRANCE:
            div = 3.0;
            break;
        
        case APPLY_SAVING_REFL:
        case APPLY_SAVING_WILL:
        case APPLY_SAVING_FORT:
        case APPLY_INITIATIVE:
        case APPLY_AC_NEW:
            div = 5.0;
            break;
        
        case APPLY_PSP:
            div = 1.0;
            break;        
        
        case APPLY_SPELL_CIRCLE_1:
        case APPLY_SPELL_CIRCLE_2:
        case APPLY_SPELL_CIRCLE_3:
        case APPLY_SPELL_RES:
        case APPLY_SPELL_DC:
            div = 10.0;
            break;
        case APPLY_SPELL_CIRCLE_4:
        case APPLY_SPELL_CIRCLE_5:
        case APPLY_SPELL_CIRCLE_6:
            div = 20.0;
            break;
        case APPLY_SPELL_CIRCLE_7:
        case APPLY_SPELL_CIRCLE_8:
        case APPLY_SPELL_CIRCLE_9:
        case APPLY_FEAT:
            div = 30.0;
            break;

        case APPLY_MOVE:
            div = 0.3;
            break;
        
        case APPLY_MV_REGEN:
            div = 0.6;
            break;

        case APPLY_HITROLL:
            div = 7.5;
            break;
    }

    if (btype == BONUS_TYPE_ENHANCEMENT)
        div /= 2;

    level_adj = (int) MAX(1, mod * div);

    return level_adj;
}

int get_level_adjustment_by_enhancement_bonus(int bonus_amt)
{
    return bonus_amt * 3.75;
}

int get_craft_obj_level(struct obj_data *obj, struct char_data *ch)
{
    if (!obj) return 1;

    int i = 0,
        level = 0;

    for (i = 0; i < MAX_OBJ_AFFECT; i++)
    {
        if (!is_valid_apply(obj->affected[i].location)) continue;
        if (obj->affected[i].modifier == 0) continue;
        if (obj->affected[i].bonus_type != BONUS_TYPE_UNIVERSAL && obj->affected[i].bonus_type != BONUS_TYPE_ENHANCEMENT) continue;
        level += get_level_adjustment_by_apply_and_modifier(obj->affected[i].location, obj->affected[i].modifier, obj->affected[i].bonus_type);
        // send_to_char(ch, "%s (%s) +%d = %d\r\n", 
        //     apply_types[obj->affected[i].location], bonus_types[obj->affected[i].bonus_type], obj->affected[i].modifier,
        //     get_level_adjustment_by_apply_and_modifier(obj->affected[i].location, obj->affected[i].modifier, obj->affected[i].bonus_type));
    }

    if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
    {
        level += get_level_adjustment_by_enhancement_bonus(GET_OBJ_VAL(obj, 4));
        // send_to_char(ch, "Weapon Enh +%d = %d\r\n", GET_OBJ_VAL(obj, 4), get_level_adjustment_by_enhancement_bonus(GET_OBJ_VAL(obj, 4)));
    }
    else if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
    {
        level += get_level_adjustment_by_enhancement_bonus(GET_OBJ_VAL(obj, 4));
        // send_to_char(ch, "Armor Enh +%d = %d\r\n", GET_OBJ_VAL(obj, 4), get_level_adjustment_by_enhancement_bonus(GET_OBJ_VAL(obj, 4)));
    }

    // material adjustment
    level += get_craft_material_final_level_adjustment(ch);
    // send_to_char(ch, "Material Adjustment = %d\r\n", get_craft_material_final_level_adjustment(ch));

    // crafdter's attempted level adjustment
    level += GET_CRAFT(ch).level_adjust;

    return level;
}

int get_craft_project_level(struct char_data *ch)
{
    int i = 0,
        level = 0;

    // bonuses
    for (i = 0; i < MAX_OBJ_AFFECT; i++)
    {
        if (!is_valid_apply(GET_CRAFT(ch).affected[i].location)) continue;
        if (GET_CRAFT(ch).affected[i].modifier == 0) continue;
        if (GET_CRAFT(ch).affected[i].bonus_type != BONUS_TYPE_UNIVERSAL && GET_CRAFT(ch).affected[i].bonus_type != BONUS_TYPE_ENHANCEMENT) continue;
        level += get_level_adjustment_by_apply_and_modifier(GET_CRAFT(ch).affected[i].location, GET_CRAFT(ch).affected[i].modifier, GET_CRAFT(ch).affected[i].bonus_type);         
    }

    // enhancement bonus
    if (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_WEAPON)
        level += get_level_adjustment_by_enhancement_bonus(GET_CRAFT(ch).enhancement);
    else if (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_ARMOR)
        level += get_level_adjustment_by_enhancement_bonus(GET_CRAFT(ch).enhancement);

    // material adjustment
    level += get_craft_material_final_level_adjustment(ch);

    if (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_INSTRUMENT)
        level += get_crafting_instrument_dc_modifier(ch);

    return MAX(1, level);
}

int get_craft_material_final_level_adjustment(struct char_data *ch)
{
    int i = 0,
        level = 0,
        mat = 0,
        num_mats = 0,
        total_mats = 0,
        mat_level = 0;

    for (i = 0; i < NUM_CRAFT_GROUPS; i++)
    {
        mat = GET_CRAFT(ch).materials[i][0];
        num_mats = GET_CRAFT(ch).materials[i][1];
        if (mat != CRAFT_MAT_NONE && num_mats > 0)
        {
            mat_level += craft_material_level_adjustment(mat) * num_mats;
            total_mats += num_mats;
        }        
    }
    // the material level adjustment is the average adjustment for all materials
    if (total_mats > 0)
        level -= (mat_level / total_mats);

    return level;
}

ACMD(do_craftbonuses)
{

    int i = 0, count = 0;

    send_to_char(ch, "\tCPOSSIBLE CRAFTING BONUSES\tn\r\n");
    send_to_char(ch, "\tc");
    draw_line(ch, 80, '-', '-');
    send_to_char(ch, "\tn");

    for (i = 0; i < NUM_APPLIES; i++)
    {
        if (!is_valid_apply(i)) continue;
        send_to_char(ch, "%-25s ", apply_types[i]);
        if ((count % 3) == 2)
            send_to_char(ch, "\r\n");
        count++;
    }
    if ((count % 3) == 0)
            send_to_char(ch, "\r\n");
}

ACMD(do_craft_score)
{
    show_craft_score(ch, argument);
}

struct obj_data *find_obj_rnum_in_inventory(struct char_data *ch, obj_rnum obj_rnum)
{
    struct obj_data *obj;

    for (obj = ch->carrying; obj; obj = obj->next_content)
    {
        if (GET_OBJ_RNUM(obj) == obj_rnum)
         return obj;
    }
    return NULL;
}

void craft_resize_complete(struct char_data *ch)
{
    int skill, skill_rank, mat, cmat, size, dc;
    struct obj_data *obj = find_obj_rnum_in_inventory(ch, GET_CRAFT(ch).craft_obj_rnum);

    if (!obj)
    {
        send_to_char(ch, "There's an issue with your resize project. Please inform a staff member.\r\n");
        return;
    }

    cmat = GET_CRAFT(ch).resize_mat_type;
    mat = craft_material_to_obj_material(cmat);
    size = GET_CRAFT(ch).new_size;
    skill = harvesting_skill_by_material(cmat);
    skill_rank = get_craft_skill_value(ch, skill);

    dc = MAX(RESIZE_BASE_DC, GET_OBJ_LEVEL(obj));

    // skill check to determine success or failure
    if (!create_craft_skill_check(ch, obj, skill, "resize", RESIZE_BASE_EXP / 2, dc))
    {
        // failure means we end things here.
        return;
    }

    gain_craft_exp(ch, MAX(RESIZE_BASE_EXP, GET_OBJ_LEVEL(obj) * RESIZE_BASE_EXP/5), skill, true);

    send_to_char(ch, "You've resized %s to %s!\r\n", obj->short_description, sizes[size]);
    GET_OBJ_SIZE(obj) = size;
    reset_crafting_obj(ch);
    GET_CRAFT(ch).new_size = GET_CRAFT(ch).resize_mat_type = GET_CRAFT(ch).resize_mat_num = 0;
    reset_current_craft(ch, NULL, false, false);
}

/**
 * Retrieves the description of a supply order item for a given character.
 *
 * @param ch The character for which to retrieve the supply order item description.
 * @return A pointer to the description of the supply order item.
 */
char *get_supply_order_item_desc(struct char_data *ch)
{
    int recipe = get_current_craft_project_recipe(ch);
    int variant = GET_CRAFT(ch).craft_variant;

    if (recipe == CRAFT_RECIPE_NONE || variant == -1)
    {
        return "unknown item";
    }

    return strdup(crafting_recipes[recipe].variant_descriptions[variant]);
}

int determine_supply_order_exp(struct char_data *ch)
{
    int base = NSUPPLY_ORDER_BASE_EXP, exp = 0;
    int mat_level_adj[NUM_CRAFT_GROUPS] = {0,0,0,0,0,0,0,0};
    int recipe = 0, variant = 0, i = 0, j = 0;
    int material = 0, num_mats = 0, level_adj = 0, group = 0, total_mats = 0;

    recipe = get_current_craft_project_recipe(ch);
    variant = GET_CRAFT(ch).craft_variant;

    if (recipe <= CRAFT_RECIPE_NONE || variant == -1)
    {
        return 0;
    }

    for (i = 0; i < NUM_CRAFT_VARIANTS; i++)
    {
        for (j = 0; j < 3; j++)
        {
            
            material = crafting_recipes[recipe].materials[j][i][0];
            num_mats = crafting_recipes[recipe].materials[j][i][1];
            if (material != CRAFT_MAT_NONE || num_mats == 0)
                continue;
            total_mats += num_mats;
            group = craft_group_by_material(material);
            level_adj = MAX(1, 1 + craft_material_level_adjustment(material));
            mat_level_adj[group] = level_adj * num_mats;
        }
    }

    for (i = 0; i < NUM_CRAFT_GROUPS; i++)
    {
        if (mat_level_adj[i] > 0)
            exp += mat_level_adj[i] * 10;
    }

    exp /= total_mats;

    exp += base;

    return MAX(NSUPPLY_ORDER_BASE_EXP, exp);
}

void craft_supplyorder_complete(struct char_data *ch)
{

    int exp = 0;

    GET_NSUPPLY_NUM_MADE(ch)++;

    send_to_char(ch, "You've completed part of your supply order for %ss.", get_supply_order_item_desc(ch));

    if ((GET_CRAFT(ch).supply_num_required - GET_NSUPPLY_NUM_MADE(ch)) > 0)
    {
        send_to_char(ch, " %d to go.", num_supply_order_requisitions_to_go(ch));
    }
    send_to_char(ch, "\r\n");

    exp = determine_supply_order_exp(ch);

    gain_craft_exp(ch, exp, GET_CRAFT(ch).skill_type, true);
}

bool check_resize(struct char_data *ch, bool verbose)
{
    bool fail = false;
    struct obj_data *obj = find_obj_rnum_in_inventory(ch, GET_CRAFT(ch).craft_obj_rnum);

    if (verbose)
    {
        send_to_char(ch, "\r\n");
        send_to_char(ch, "\tc");
    }
    if (!obj)
    {
        if (verbose)
        {
            send_to_char(ch, "You need to set an object to resize first. Use 'resize (object-name) (new-size)\r\n");
            send_to_char(ch, NEWCRAFT_RESIZE_SYNTAX);
        }
        fail = true;
    }

    if (GET_CRAFT(ch).new_size == 0)
    {
        if (verbose)
        {
            send_to_char(ch, "You need to set the new object size. Use 'resize (object-name) (new-size)\r\n");    
        }
        fail = true;
    }

    if (GET_CRAFT(ch).resize_mat_type == 0)
    {
        if (verbose)
        {
            send_to_char(ch, "You haven't set your resize materials. Use 'resize add'.\r\n");
        }
        fail = true;
    }

    if (verbose)
    {
        send_to_char(ch, "\tn");
        send_to_char(ch, "\r\n");
    }
    return (!fail);
}

void newcraft_resize(struct char_data *ch, const char *argument)
{
    struct obj_data *obj;
    int i, size, mat, cmat, num, mod, old_size, total;
    char arg1[200], arg2[200], buf[200];

    two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

    if (!*arg1)
    {
        send_to_char(ch, NEWCRAFT_RESIZE_SYNTAX);
        return;
    }

    if (is_abbrev(arg1, "add"))
    {
        if (!(obj = find_obj_rnum_in_inventory(ch, GET_CRAFT(ch).craft_obj_rnum)))
        {
            send_to_char(ch, "You need to set an object to resize first.\r\n");
            send_to_char(ch, NEWCRAFT_RESIZE_SYNTAX);
            return;
        }

        mat = GET_OBJ_MATERIAL(obj);

        cmat = obj_material_to_craft_material(mat);
        old_size = GET_OBJ_SIZE(obj);
        size = GET_CRAFT(ch).new_size;
        num = MAX(1, GET_OBJ_WEIGHT(obj) / 10);
        mod = MAX(1, size - old_size);
        total = num * mod;

        if (cmat == CRAFT_MAT_NONE)
        {
            send_to_char(ch, "That object is made of %s, which is not a valid resize material. Please inform a staff member if you feel it should be added.\r\n", material_name[mat]);
            return;
        }

        if (GET_CRAFT_MAT(ch, cmat) < total)
        {
            send_to_char(ch, "You need %d unit%s of %s, but only have %d.\r\n", total, total > 1 ? "s" : "", crafting_materials[cmat], GET_CRAFT_MAT(ch, cmat));
            return;
        }

        send_to_char(ch, "You've added %d unit%s of %s to resize %s from %s to %s.\r\n", total, total > 1 ? "s" : "", crafting_materials[cmat], obj->short_description, sizes[old_size], sizes[size]);
        send_to_char(ch, "Type: resize begin to execute.\r\n");

        GET_CRAFT(ch).resize_mat_type = cmat;
        GET_CRAFT(ch).resize_mat_num = total;
        GET_CRAFT_MAT(ch, cmat) -= total;
        return;

    }
    else if (is_abbrev(arg1, "remove") || is_abbrev(arg1, "reset"))
    {
        if (!(obj = find_obj_rnum_in_inventory(ch, GET_CRAFT(ch).craft_obj_rnum)))
        {
            send_to_char(ch, "You don't have any object set to resize.\r\n");
            send_to_char(ch, NEWCRAFT_RESIZE_SYNTAX);
            return;
        }

        if (!GET_CRAFT(ch).resize_mat_type)
        {
            send_to_char(ch, "No materials have been allocated for the resizing, however your crafting object and size have been reset\r\n");
            GET_CRAFT(ch).new_size = GET_CRAFT(ch).resize_mat_type = GET_CRAFT(ch).resize_mat_num = 0;
        }
        else if (GET_CRAFT(ch).new_size)
        {
            send_to_char(ch, "You recover %d unit%s of %s and cancel your resizing of %s.\r\n", GET_CRAFT(ch).resize_mat_num, GET_CRAFT(ch).resize_mat_num > 1 ? "s" : "", 
                        crafting_materials[GET_CRAFT(ch).resize_mat_type], obj->short_description);
            GET_CRAFT_MAT(ch, GET_CRAFT(ch).resize_mat_type) += GET_CRAFT(ch).resize_mat_num;
            GET_CRAFT(ch).new_size = GET_CRAFT(ch).resize_mat_type = GET_CRAFT(ch).resize_mat_num = 0;
        }
        else
        {
            send_to_char(ch, "There seems to be an error with recovering your resizing materials. Please inform a staff member. We will reimburse lost materials.\r\n");
        }
        reset_crafting_obj(ch);
        return;
    }
    else if (is_abbrev(arg1, "show") || is_abbrev(arg1, "check"))
    {
        if (!(obj = find_obj_rnum_in_inventory(ch, GET_CRAFT(ch).craft_obj_rnum)))
        {
            check_resize(ch, true);
        }
        else
        {
            send_to_char(ch, "\r\n");
            send_to_char(ch, "\tc");
            text_line(ch, "RESIZING", 80, '-', '-');
            send_to_char(ch, "\tn");
            send_to_char(ch, "-- Resize Object : %s\r\n", obj->short_description);
            send_to_char(ch, "-- Existing Size : %s\r\n", sizes[GET_OBJ_SIZE(obj)]);
            send_to_char(ch, "-- New Size      : %s\r\n", GET_CRAFT(ch).new_size ? sizes[GET_CRAFT(ch).new_size] : "Not Set");
            send_to_char(ch, "-- Material Type : %s\r\n", GET_CRAFT(ch).resize_mat_type ? crafting_materials[GET_CRAFT(ch).resize_mat_type] : "Not Set");
            send_to_char(ch, "-- Material Amount: %d\r\n", GET_CRAFT(ch).resize_mat_num);
            send_to_char(ch, "\tc");
            draw_line(ch, 80, '-', '-');
            send_to_char(ch, "\tn");
        }
        return;
    }
    else if (is_abbrev(arg1, "start") || is_abbrev(arg1, "begin"))
    {
        if (!check_resize(ch, true))
        {
            send_to_char(ch, "\tRYou are not ready to resize yet.\tn\r\n");
            return;
        }
        if (!(obj = find_obj_rnum_in_inventory(ch, GET_CRAFT(ch).craft_obj_rnum)))
        {
            send_to_char(ch, "You need to set you resize object first.\r\n");
            return;
        }
        GET_CRAFT(ch).crafting_method = SCMD_NEWCRAFT_RESIZE;
        GET_CRAFT(ch).craft_duration = 10;
        send_to_char(ch, "You begin resizing %s to %s.\r\n", obj->short_description, sizes[GET_CRAFT(ch).new_size]);
        return;
    }
    else
    {
        if ((obj = find_obj_rnum_in_inventory(ch, GET_CRAFT(ch).craft_obj_rnum)))
        {
            send_to_char(ch, "You have already assigned an object to resize: %s\r\n", obj->short_description);
            send_to_char(ch, NEWCRAFT_RESIZE_SYNTAX);
            return;
        }

        if (!(obj = get_obj_in_list_vis(ch, arg1, 0, ch->carrying)))
        {
            send_to_char(ch, "There's no object by that name in your inventory.\r\n");
            send_to_char(ch, NEWCRAFT_RESIZE_SYNTAX);
            return;
        }

        if (!*arg2)
        {
            send_to_char(ch, "Please specify the size you would like to change %s to.\r\n", obj->short_description);
            return;
        }

        for (i = 0; i < NUM_SIZES; i++)
        {
            snprintf(buf, sizeof(buf), "%s", size_names[i]);
            buf[0] = tolower(buf[0]);
            if (is_abbrev(arg2, buf))
                break;
        }

        size = i;

        if (size >= NUM_SIZES)
        {
            send_to_char(ch, "That is not a valid size name. Please select one of:\r\n");
            for (i = 0; i < NUM_SIZES; i++)
            {
                snprintf(buf, sizeof(buf), "%s", size_names[i]);
                buf[0] = tolower(buf[0]);
                send_to_char(ch, "-- %s\r\n", buf);
            }
            send_to_char(ch, "\r\n");
            return;
        }

        if (GET_OBJ_SIZE(obj) == size)
        {
            act("$p is already that size.", TRUE, ch, obj, 0, TO_CHAR);
            return;
        }

        GET_CRAFT(ch).craft_obj_rnum = GET_OBJ_RNUM(obj);

        send_to_char(ch, "You've set %s to be resized from %s to %s.\r\n", obj->short_description, size_names[GET_OBJ_SIZE(obj)], size_names[size]);
        send_to_char(ch, "Type: resize begin to execute.\r\n");
        GET_CRAFT(ch).new_size = size;
    }
}


ACMD(do_newcraft)
{
    if (IS_NPC(ch))
    {
        send_to_char(ch, "NPCs cannot craft.\r\n");
        return;
    }

    if (GET_CRAFT(ch).crafting_method != subcmd && GET_CRAFT(ch).crafting_method != 0)
    {
        send_to_char(ch, "You are already working on another project of type: %s. Please finish or cancel it before continuing.\r\n", crafting_methods_short[GET_CRAFT(ch).crafting_method]);
        return;
    }

    if (subcmd == SCMD_NEWCRAFT_CREATE)
    {
        newcraft_create(ch, argument);
        return;
    }
    else if (subcmd == SCMD_NEWCRAFT_SURVEY)
    {
        newcraft_survey(ch, argument);
        return;
    }
    else if (subcmd == SCMD_NEWCRAFT_HARVEST)
    {
        newcraft_harvest(ch, argument);
        return;
    }
    else if (subcmd == SCMD_NEWCRAFT_REFINE)
    {
        newcraft_refine(ch, argument);
        return;
    }
    else if (subcmd == SCMD_NEWCRAFT_RESIZE)
    {
        newcraft_resize(ch, argument);
        return;
    }
}

bool does_craft_apply_type_have_specific_value(int location)
{
    switch (location)
    {
        case APPLY_SKILL:
        case APPLY_FEAT:
        case APPLY_SPELL_CIRCLE_1:
        case APPLY_SPELL_CIRCLE_2:
        case APPLY_SPELL_CIRCLE_3:
        case APPLY_SPELL_CIRCLE_4:
        case APPLY_SPELL_CIRCLE_5:
        case APPLY_SPELL_CIRCLE_6:
        case APPLY_SPELL_CIRCLE_7:
        case APPLY_SPELL_CIRCLE_8:
        case APPLY_SPELL_CIRCLE_9:
            return true;
    }
    return false;
}

/**
 * Retrieves the craft material associated with the given name.
 *
 * @param ch The character data structure.
 * @param arg2 The name of the craft material.
 * @return The craft material associated with the given name, or NULL if not found.
 */
int get_craft_material_by_name(struct char_data *ch, char *arg)
{
    int i = 0;
    char buf[200];

    if (!*arg)
    {
        send_to_char(ch, "You need to specify a material type.\r\n");
        return CRAFT_MAT_NONE;
    }

    for (i = 1; i < NUM_CRAFT_MATS; i++)
    {
        snprintf(buf, sizeof(buf), "%s", crafting_materials[i]);
        buf[0] = tolower(buf[0]);
        if (is_abbrev(arg, buf))
            return i;
    }
    return CRAFT_MAT_NONE;
}

/**
 * Retrieves the recipe for the current craft project of a character.
 *
 * @param ch A pointer to the character data structure.
 * @return The recipe for the current craft project, or NULL if no project is active.
 */
int get_current_craft_project_recipe(struct char_data *ch)
{
    int item_type = GET_CRAFT(ch).crafting_item_type;
    int crafting_specific = GET_CRAFT(ch).crafting_specific;
    int i = 0;

    if (GET_CRAFT(ch).crafting_recipe != CRAFT_RECIPE_NONE)
    {
        return GET_CRAFT(ch).crafting_recipe;
    }

    if (item_type == 0 || crafting_specific == 0)
    {
        return CRAFT_RECIPE_NONE;
    }

    for (i = 0; i < NUM_CRAFTING_RECIPES; i++)
    {
        if (craft_recipe_by_type(item_type) == crafting_recipes[i].object_type)
        {
            if (crafting_specific == crafting_recipes[i].object_subtype)
            {
                return i;
            }
        }
    }
    return CRAFT_RECIPE_NONE;
}

/**
 * Retrieves the number of materials required by a specific material type and craft recipe.
 *
 * @param ch The character data.
 * @param material The material type.
 * @param recipe The craft recipe.
 * @return The number of materials required.
 */
int get_num_mats_required_by_material_type_and_craft_recipe(struct char_data *ch, int material, int recipe)
{
    int i = 0, j = 0;
    int num_mats = 0;
    int variant = GET_CRAFT(ch).craft_variant;
    int mat_type = CRAFT_GROUP_NONE;

    if (variant == -1)
    {
        send_to_char(ch, "You need to set the variant type first.\r\n");
        return 0;
    }

    if (material <= CRAFT_MAT_NONE || recipe <= CRAFT_RECIPE_NONE)
    {
        return 0;
    }

    if ((mat_type = craft_group_by_material(material)) <= CRAFT_GROUP_NONE)
    {
        return 0;
    }

    for (i = 0; i < NUM_CRAFTING_RECIPES; i++)
    {
        if (recipe == i)
        {
            for (j = 0; j < 3; j++)
            {
                if (crafting_recipes[i].materials[j][variant][0] == mat_type)
                {
                    num_mats = crafting_recipes[i].materials[j][variant][1];
                    return num_mats;
                }
            }
        }
    }
    return num_mats;
}

bool remove_supply_order_materials(struct char_data *ch)
{
    bool found = false;
    int i;
    int material, num_mats;

    for (i = 1; i < NUM_CRAFT_GROUPS; i++)
    {
        if (GET_CRAFT(ch).materials[i][0] > CRAFT_MAT_NONE)
        {
            material = GET_CRAFT(ch).materials[i][0];
            num_mats = GET_CRAFT(ch).materials[i][1];
            GET_CRAFT_MAT(ch, material) += num_mats;
            send_to_char(ch, "You've removed %d unit%s of %s from your supply order.\r\n", 
                            num_mats, num_mats > 1 ? "s" : "", crafting_materials[material]);
            GET_CRAFT(ch).materials[i][0] = 0;
            GET_CRAFT(ch).materials[i][1] = 0;
            found = true;
        }
    }
    return found;
}


/**
 * Sets the supply order materials for a character.
 *
 * This function sets the supply order materials for the specified character.
 * It takes in a character data structure, `ch`, and two string arguments, `arg` and `arg2`,
 * which represent the materials for the supply order.
 *
 * @param ch    The character for which to set the supply order materials.
 * @param arg   Whether to 'add' or 'remove' materials
 * @param arg2  The material type for the supply order. Ie. 'steel'
 */
void set_supply_order_materials(struct char_data *ch, char *arg, char *arg2)
{

    int material = 0;
    int num_mats = 0;
    int recipe = 0;
    int mat_type = 0;
    bool found = false;

    if (!*arg)
    {
        send_to_char(ch, "You need to specify whether to add or remove materials.\r\n");
    }

    if (!is_abbrev(arg, "add") && !is_abbrev(arg, "remove"))
    {
        send_to_char(ch, "You need to specify whether to add or remove materials.\r\n");
        return;
    }
    
    if (!*arg2 && is_abbrev(arg, "add"))
    {
        send_to_char(ch, "You need to specify a material type.\r\n");
        return;
    }

    // remove will remove all materials currently assigned.
    if (is_abbrev(arg, "remove"))
    {
        found = remove_supply_order_materials(ch);
        if (!found)
        {
            send_to_char(ch, "You don't have any materials assigned to your supply order.\r\n");
            return;
        }
        return;
    }

    if ((material = get_craft_material_by_name(ch, arg2)) <= CRAFT_MAT_NONE)
    {
        send_to_char(ch, "That is not a valid type of material. Type 'materials' for a list.\r\n");
        return;
    }

    if ((recipe = get_current_craft_project_recipe(ch)) <= CRAFT_RECIPE_NONE)
    {
        send_to_char(ch, "You need to set the supply order item type, specific type and variant type first.\r\n");
        return;
    }

    if ((num_mats = get_num_mats_required_by_material_type_and_craft_recipe(ch, material, recipe)) <= 0)
    {
        send_to_char(ch, "That is not a valid type of material for this supply order.\r\n");
        return;
    }

    if ((mat_type = craft_group_by_material(material)) <= CRAFT_GROUP_NONE)
    {
        send_to_char(ch, "That is not a valid type of material.\r\n");
        return;
    }
    
    // add requires specifying which material to add, as certain materials provide higher bonuses
    if (is_abbrev(arg, "add"))
    {
        if (GET_CRAFT(ch).materials[mat_type][0] > CRAFT_GROUP_NONE)
        {
            send_to_char(ch, "You already have a material of that type assigned to your supply order. Please remove it first.\r\n");
            return;
        }
        
        if (GET_CRAFT_MAT(ch, material) < num_mats)
        {
            send_to_char(ch, "You need %d unit%s of %s, but only have %d.\r\n", num_mats, num_mats > 1 ? "s" : "", crafting_materials[material], GET_CRAFT_MAT(ch, material));
            return;
        }

        GET_CRAFT(ch).materials[mat_type][0] = material;
        GET_CRAFT(ch).materials[mat_type][1] = num_mats;
        GET_CRAFT_MAT(ch, material) -= num_mats;
        send_to_char(ch, "You've added %d unit%s of %s to your supply order.\r\n", num_mats, num_mats > 1 ? "s" : "", crafting_materials[material]);
        return;
    }
    else
    {
        send_to_char(ch, "You need to specify whether to add or remove materials.\r\n");
        return;
    }
}

int select_random_craft_recipe(void)
{
    int type = 0;
    int choice = 0;

    // -2 insteadf of -1 for now, as we're not including instruments yet
    type = craft_recipe_by_type(dice(CRAFT_TYPE_NONE+1, NUM_CRAFT_TYPES - 2));

    choice = dice(1, NUM_CRAFTING_RECIPES - 1);

    while (type != crafting_recipes[choice].object_type)
    {
        choice = dice(1, NUM_CRAFTING_RECIPES - 1);
    }

    return choice;
}

int select_random_craft_variant(int recipe)
{

    if (recipe <= CRAFT_RECIPE_NONE || recipe >= NUM_CRAFTING_RECIPES)
    {
        return -1;
    }

    // we're not ready for instruments yet
    if (crafting_recipes[recipe].object_type == ITEM_INSTRUMENT)
    {
        return -1;
    }

    int variant = 0;

    variant = dice(1, NUM_CRAFT_VARIANTS) - 1;

    while (crafting_recipes[recipe].variant_skill[variant] == 0)
    {
        variant = dice(1, NUM_CRAFT_VARIANTS) - 1;
    }

    return variant;
}

bool player_has_supply_order(struct char_data *ch)
{
    if (GET_CRAFT(ch).crafting_method == SCMD_NEWCRAFT_SUPPLYORDER)
    {
        return true;
    }
    return false;
}

void request_new_supply_order(struct char_data *ch)
{

    int recipe = 0;
    int variant = 0;

    if (GET_CRAFT(ch).crafting_method == SCMD_NEWCRAFT_SUPPLYORDER)
    {
        send_to_char(ch, "You are already working on a supply order. Type supplyorder show to see the details or supplyorder reset to start over.\r\n");
        return;
    }

    if (GET_CRAFT(ch).crafting_method != 0)
    {
        send_to_char(ch, "You are already working on a crafting project.\r\n");
        return;
    }

    if (GET_CRAFT(ch).crafting_item_type > CRAFT_TYPE_NONE ||
        GET_CRAFT(ch).crafting_specific > 0 ||
        GET_CRAFT(ch).craft_variant >= 0)
    {
        send_to_char(ch, "You already have a supply order going. Type supplyorder show to see the details or supplyorder reset to start over.\r\n");
        return;
    }

    if ((recipe = select_random_craft_recipe()) <= CRAFT_RECIPE_NONE)
    {
        send_to_char(ch, "There seems to be an issue with your supply order. Please type supplyorder reset to start over. This is error 1.\r\n");
        return;
    }

    if ((variant = select_random_craft_variant(recipe)) < 0)
    {
        send_to_char(ch, "There seems to be an issue with your supply order. Please type supplyorder reset to start over. This is error 2.\r\n");
        return;
    }

    if (GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_NONE)
    {
        GET_CRAFT(ch).crafting_recipe = recipe;
        GET_CRAFT(ch).crafting_item_type = crafting_recipes[recipe].object_type;
        GET_CRAFT(ch).crafting_specific = crafting_recipes[recipe].object_subtype;
        GET_CRAFT(ch).craft_variant = variant;
        GET_CRAFT(ch).crafting_method = SCMD_NEWCRAFT_SUPPLYORDER;
        GET_CRAFT(ch).supply_num_required = NSUPPLY_ORDER_NUM_REQUIRED;
        GET_CRAFT(ch).skill_type = crafting_recipes[recipe].variant_skill[variant];
        send_to_char(ch, "You've requested a new supply order to make %d %ss.\r\n", NSUPPLY_ORDER_NUM_REQUIRED, crafting_recipes[recipe].variant_descriptions[variant]);
    }
    else
    {
        send_to_char(ch, "There seems to be an issue with your supply order. Please type supplyorder reset to start over. This is error 3.\r\n");
        return;
    }
}

int num_supply_order_requisitions_to_go(struct char_data *ch)
{
    int num_required = 0, num_done = 0, num_to_go = 0;

    num_required = GET_CRAFT(ch).supply_num_required;
    num_done = GET_NSUPPLY_NUM_MADE(ch);
    num_to_go = num_required - num_done;

    return num_to_go;
}

void start_supply_order(struct char_data *ch)
{
    if (!player_has_supply_order(ch))
    {
        send_to_char(ch, "You need to request a supply order first.\r\n");
    }
    else if (num_supply_order_requisitions_to_go(ch) == 0)
    {
        send_to_char(ch, "You have already completed your supply order. Go to a supply order requisition NPC and type 'supplyorder complete' for your reward.\r\n");
    }
    else
    {
        GET_CRAFT(ch).craft_duration = NSUPPLY_ORDER_DURATION;
        send_to_char(ch, "You begin working on your supply order.\r\n");
    }  
}

void show_supply_order_materials(struct char_data *ch, int recipe, int variant)
{
    int i = 0;
    int material = 0, num_mats = 0, mat_type = 0;
    for (i = 0; i < 3; i++)
    {
        mat_type = crafting_recipes[recipe].materials[i][variant][0];
        num_mats = crafting_recipes[recipe].materials[i][variant][1];
        if (mat_type == CRAFT_GROUP_NONE || num_mats == 0)
            continue;
        send_to_char(ch, "%-15s (x%d) ", crafting_material_groups[mat_type], num_mats);
        if ((material = GET_CRAFT(ch).materials[mat_type][0])  != CRAFT_MAT_NONE)
        {
            send_to_char(ch, "(%s/x%d)\r\n", crafting_materials[material], num_mats);
        }
        else
        {
            send_to_char(ch, "(unassigned)\r\n");
        }
    }
}

void show_supply_order(struct char_data *ch)
{

    int recipe = 0;
    int variant = -1;

    if (GET_CRAFT(ch).crafting_method != SCMD_NEWCRAFT_SUPPLYORDER)
    {
        send_to_char(ch, "You don't have a supply order in progress.\r\n");
        return;
    }

    if ((recipe = get_current_craft_project_recipe(ch)) == CRAFT_RECIPE_NONE)
    {
        send_to_char(ch, "You do not have a supply order project type created. Please reset the supply order and request a new one. Error #1.\r\n");
        return;
    }

    if ((variant = GET_CRAFT(ch).craft_variant) == -1)
    {
        send_to_char(ch, "You do not have a supply order type created. Please reset the supply order and request a new one. Error #2.\r\n");
        return;
    }

    text_line(ch, "SUPPLY ORDER DETAILS", 90, '-', '-');
    
    // show the type of item being created
    send_to_char(ch, "-- Item: %s\r\n", get_supply_order_item_desc(ch));
    
    // the materials required
    send_to_char(ch, "-- Materials Required:\r\n");
    show_supply_order_materials(ch, recipe, variant);

    // the number still needed to make
    // the exp per instance
    // the reward info
    // check and show completion

}

void reset_supply_order(struct char_data *ch)
{
    int i = 0;
    GET_CRAFT(ch).crafting_method = 0;
    GET_CRAFT(ch).crafting_item_type = 0;
    GET_CRAFT(ch).crafting_specific = 0;
    GET_CRAFT(ch).craft_variant = -1;
    GET_CRAFT(ch).supply_num_required = 0;
    GET_CRAFT(ch).skill_type = 0;
    for (i = 0; i < NUM_CRAFT_GROUPS; i++)
    {
        GET_CRAFT(ch).materials[i][0] = 0;
        GET_CRAFT(ch).materials[i][1] = 0;
    }
    send_to_char(ch, "You have reset your supply order, have lost all progress, and will need to request a new one.\r\n");
}


/**
 * @brief Handles the special behavior for new supply orders.
 */
SPECIAL(new_supply_orders)
{

    if (!CMD_IS("supplyorder"))
    {
        return 0;
    }

    char arg1[200], arg2[200], arg3[200];

    three_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2), arg3, sizeof(arg3));

    /**
     * Handle different commands related to crafting supply orders.
     *
     * @param arg1 The command argument.
     * @param ch The character executing the command.
     * @return 1 if the command was handled successfully, 0 otherwise.
     */
    if (!*arg1)
    {
        send_to_char(ch, "%s", SUPPLY_ORDER_NOARG1);
        return 1;
    }

    if (is_abbrev(arg1, "request"))
    {
        request_new_supply_order(ch);
    }
    else if (is_abbrev(arg1, "info") || is_abbrev(arg1, "show"))
    {
        show_supply_order(ch);
    }
    else if (is_abbrev(arg1, "start"))
    {
        start_supply_order(ch);      
    }
    else if (is_abbrev(arg1, "material"))
    {
        set_supply_order_materials(ch, arg2, arg3);
    }
    else if (is_abbrev(arg1, "complete"))
    {

    }
    else if (is_abbrev(arg1, "reset"))
    {
        reset_supply_order(ch);
    }
    else if (is_abbrev(arg1, "abandon"))
    {

    }
    else
    {
        send_to_char(ch, "%s", SUPPLY_ORDER_NOARG1);
        return 1;
    }

    return 1;

}

void show_mote_bonuses(struct char_data *ch, int mote)
{
    int i, j, length = 0;
    bool found = false;

    send_to_char(ch, "\r\n");

    // weapon enhancements
    send_to_char(ch, "\tcWeapon Enhancement Bonuses for:\tn\r\n");
    for (i = 1; i < NUM_WEAPON_TYPES; i++)
    {
        if (get_enhancement_mote_type(ch, CRAFT_TYPE_WEAPON, i) == mote)
        {
            send_to_char(ch, "%s", weapon_list[i].name);
            send_to_char(ch, ", ");
            length += strlen(weapon_list[i].name);
            if (length > 80)
            {
                send_to_char(ch, "\r\n");
                length = 0;
            }
            found = true;
        }
    }
    if (!found)
        send_to_char(ch, "None");
    send_to_char(ch, "\r\n");
    send_to_char(ch, "\r\n");

    // armor enhancements
    found = false;
    length = 0;
    send_to_char(ch, "\tcArmor Enhancement Bonuses for:\tn\r\n");
    for (i = 1; i < NUM_SPEC_ARMOR_TYPES; i++)
    {
        if (get_enhancement_mote_type(ch, CRAFT_TYPE_ARMOR, i) == mote)
        {
            send_to_char(ch, "%s", armor_list[i].name);
            send_to_char(ch, ", ");
            length += strlen(armor_list[i].name);
            if (length > 80)
            {
                send_to_char(ch, "\r\n");
                length = 0;
            }
            
            found = true;
        }
    }
    if (!found)
        send_to_char(ch, "None");
    send_to_char(ch, "\r\n");
    send_to_char(ch, "\r\n");


    send_to_char(ch, "\tcOther Bonuses:\tn\r\n");
    found = false;
    length = 0;
    for (i = 0; i < NUM_APPLIES; i++)
    {
        switch (i)
        {
            case APPLY_SKILL:
                for (j = 1; j <= END_GENERAL_ABILITIES; j++)
                {
                    if (crafting_mote_by_bonus_location(i, j, 0) == mote)
                    {
                        send_to_char(ch, "%s (%s), ", apply_types[i], ability_names[j]);
                        length += strlen(apply_types[i]) + strlen(ability_names[j]) + 2; // +2 for the parentheses and comma
                        if (length > 80)
                        {
                            send_to_char(ch, "\r\n");
                            length = 0;
                        }
                        found = true;
                    }
                }
                break;
            case APPLY_AC_NEW:
                if (crafting_mote_by_bonus_location(i, 0, BONUS_TYPE_DEFLECTION) == mote)
                {
                    send_to_char(ch, "%s (Deflection), ", apply_types[i]);
                    length += strlen(apply_types[i]) + 14; // +14 for " (Deflection), "
                    if (length > 80)
                    {
                        send_to_char(ch, "\r\n");
                        length = 0;
                    }
                    found = true;
                }
                if (crafting_mote_by_bonus_location(i, 0, BONUS_TYPE_NATURALARMOR) == mote)
                {
                    send_to_char(ch, "%s (Natural), ", apply_types[i]);
                    length += strlen(apply_types[i]) + 12; // +12 for " (Natural), "
                    if (length > 80)
                    {
                        send_to_char(ch, "\r\n");
                        length = 0;
                    }
                    found = true;
                }
                if (crafting_mote_by_bonus_location(i, 0, BONUS_TYPE_DODGE) == mote)
                {
                    send_to_char(ch, "%s (Dodge), ", apply_types[i]);send_to_char(ch, "%s, ", ability_names[j]);
                    length += strlen(apply_types[i]) + 8; // +8 for " (Dodge), "
                    if (length > 80)
                    {
                        send_to_char(ch, "\r\n");
                        length = 0;
                    }
                    found = true;
                }
                break;
            default:
                if (crafting_mote_by_bonus_location(i, 0, 0) == mote)
                {
                    send_to_char(ch, "%s, ", apply_types[i]);
                    length += strlen(apply_types[i]) + 2; // +2 for the comma
                    if (length > 80)
                    {
                        send_to_char(ch, "\r\n");
                        length = 0;
                    }
                    found = true;
                }
                break;
        }
    }
    send_to_char(ch, "\r\n");
    
    // crafting_mote_by_bonus_location
    
}

ACMDU(do_motes)
{
    int i;
    char mote[50];

    skip_spaces(&argument);

    if (!*argument)
    {
        send_to_char(ch, "Please specify one of the following mote types to see associated bonuses:\r\n");
        for (i = 1; i < NUM_CRAFT_MOTES; i++)
        {
            if (i > 1)
                send_to_char(ch, ", ");
            send_to_char(ch, "%s", crafting_motes[i]);
        }
        send_to_char(ch, ".\r\n");
        return;
    }

    for (i = 1; i < NUM_CRAFT_MOTES; i++)
    {
        if (is_abbrev(argument, crafting_motes[i]))
        {
            snprintf(mote, sizeof(mote), "%s", crafting_motes[i]);
            send_to_char(ch, "\tC%ss provide the following bonuses:\tn\r\n", CAP(mote));
            show_mote_bonuses(ch, i);
            return;
        }
    }

    send_to_char(ch, "That is not a valid mote type. Please specify one of the following:\r\n");
    for (i = 1; i < NUM_CRAFT_MOTES; i++)
    {
        if (i > 1)
            send_to_char(ch, ", ");
        send_to_char(ch, "%s", crafting_motes[i]);
    }
    send_to_char(ch, ".\r\n");

}

int get_craft_wear_loc(struct char_data *ch)
{
    if (!ch) return ITEM_WEAR_TAKE;

    int cr_type = GET_CRAFT(ch).crafting_item_type;
    int cr_specific = GET_CRAFT(ch).crafting_specific;
    int cr_recipe = GET_CRAFT(ch).crafting_recipe;

    if (cr_type <= CRAFT_TYPE_NONE || cr_specific <= 0 || cr_recipe <= CRAFT_RECIPE_NONE)
    {
        return ITEM_WEAR_TAKE; // default to take if no crafting project is set
    }

    if (cr_type == CRAFT_TYPE_WEAPON)
    {
        return ITEM_WEAR_WIELD;
    }
    else if (cr_type == CRAFT_TYPE_ARMOR)
    {
        return get_wear_location_by_armor_type(crafting_recipes[cr_recipe].object_subtype);
    }
    else if (cr_type == ITEM_INSTRUMENT)
    {
        return ITEM_WEAR_INSTRUMENT;
    }
    else
    {
        return crafting_recipes[cr_recipe].object_subtype;
    }
     return ITEM_WEAR_TAKE;   
}


// Todo: 
// greygem shards - extract motes, get feats out, store feat motes separately
// restring, reforge, other old craft commands
// ability to trade materials and motes with other players and shops
// ability to augment existing items.
// crafting feat system
// show craft object level and dc in 'show'
// supply order system
// add effects for new materials, not craft materials, but object materials (ie dragonmetal)