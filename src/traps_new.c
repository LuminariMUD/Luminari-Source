/****************************************************************************
 *  Realms of Luminari
 *  File:     traps.c
 *  Usage:    Comprehensive trap system based on NWN mechanics
 *  Header:   traps.h
 *  Authors:  Homeland (ported to Luminari by Zusuk)
 *            Updated with NWN-style trap system
 ****************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "mud_event.h"
#include "actions.h"
#include "mudlim.h"
#include "fight.h"
#include "spells.h"
#include "act.h"
#include "perks.h"
#include "traps.h"

/* ============================================================================ */
/* Global Trap Data Tables                                                      */
/* ============================================================================ */

/* Trap Severity Table - Defines DCs and damage multipliers by severity */
const struct trap_severity_data trap_severity_table[NUM_TRAP_SEVERITIES] = {
    /* TRAP_SEVERITY_MINOR */
    {"minor", 15, 15, 15, 1},
    /* TRAP_SEVERITY_AVERAGE */
    {"average", 20, 20, 20, 2},
    /* TRAP_SEVERITY_STRONG */
    {"strong", 25, 25, 25, 3},
    /* TRAP_SEVERITY_DEADLY */
    {"deadly", 25, 30, 25, 4},
    /* TRAP_SEVERITY_EPIC */
    {"epic", 35, 35, 35, 5}
};

/* Trap Type Template Table - Defines properties for each trap type */
const struct trap_type_template trap_type_table[NUM_TRAP_TYPES] = {
    /* TRAP_TYPE_ACID_BLOB */
    {"acid blob", DAM_ACID, TRAP_SAVE_REFLEX, TRAP_SPECIAL_PARALYSIS,
     "\tgYou are engulfed in a \tGblob of acid\tg, feeling your flesh burn as you're paralyzed!\tn",
     "\tg$n is engulfed in a \tGblob of acid\tg, screaming in agony!\tn",
     "You notice an \tGacid trap\tn mechanism!", FALSE, 0},
    
    /* TRAP_TYPE_ACID_SPLASH */
    {"acid splash", DAM_ACID, TRAP_SAVE_REFLEX, TRAP_SPECIAL_NONE,
     "\tgA splash of \tGacid\tg burns your skin!\tn",
     "\tg$n is splashed with \tGacid\tg!\tn",
     "You notice an \tGacid splash trap\tn!", FALSE, 0},
    
    /* TRAP_TYPE_ELECTRICAL */
    {"electrical", DAM_ELECTRIC, TRAP_SAVE_REFLEX, TRAP_SPECIAL_NONE,
     "\tYA massive electrical discharge \tCCRACKLES\tY through the air, shocking you!\tn",
     "\tYA massive electrical discharge \tCCRACKLES\tY through the air, shocking $n!\tn",
     "You notice an \tYelectrical trap\tn!", TRUE, 15},
    
    /* TRAP_TYPE_FIRE */
    {"fire", DAM_FIRE, TRAP_SAVE_REFLEX, TRAP_SPECIAL_NONE,
     "\tRA blast of \tRflames\tR erupts around you!\tn",
     "\tRA blast of \tRflames\tR erupts around $n!\tn",
     "You notice a \tRfire trap\tn!", TRUE, 10},
    
    /* TRAP_TYPE_FROST */
    {"frost", DAM_COLD, TRAP_SAVE_FORTITUDE, TRAP_SPECIAL_PARALYSIS,
     "\tbA freezing blast of \tCice\tb chills you to the bone, leaving you paralyzed!\tn",
     "\tbA freezing blast of \tCice\tb chills $n to the bone!\tn",
     "You notice a \tbfrost trap\tn!", FALSE, 0},
    
    /* TRAP_TYPE_GAS */
    {"gas", DAM_POISON, TRAP_SAVE_FORTITUDE, TRAP_SPECIAL_POISON,
     "\tgA cloud of \tGpoisonous gas\tg surrounds you!\tn",
     "\tgA cloud of \tGpoisonous gas\tg fills the area!\tn",
     "You notice a \tggas trap\tn!", TRUE, 15},
    
    /* TRAP_TYPE_HOLY */
    {"holy", DAM_HOLY, TRAP_SAVE_NONE, TRAP_SPECIAL_NONE,
     "\tWA burst of \tYholy light\tW sears you!\tn",
     "\tWA burst of \tYholy light\tW sears $n!\tn",
     "You notice a \tYholy trap\tn!", FALSE, 0},
    
    /* TRAP_TYPE_NEGATIVE */
    {"negative energy", DAM_NEGATIVE, TRAP_SAVE_FORTITUDE, TRAP_SPECIAL_ABILITY_DRAIN,
     "\tDA wave of \tDnegative energy\tD drains your life force!\tn",
     "\tDA wave of \tDnegative energy\tD strikes $n!\tn",
     "You notice a \tDnegative energy trap\tn!", FALSE, 0},
    
    /* TRAP_TYPE_SONIC */
    {"sonic", DAM_SOUND, TRAP_SAVE_WILL, TRAP_SPECIAL_STUN,
     "\twA \tCdeafening sonic blast\tw overwhelms you!\tn",
     "\twA \tCdeafening sonic blast\tw strikes $n!\tn",
     "You notice a \twsonic trap\tn!", TRUE, 10},
    
    /* TRAP_TYPE_SPIKE */
    {"spike", DAM_PUNCTURE, TRAP_SAVE_REFLEX, TRAP_SPECIAL_NONE,
     "\tLA deadly \tWspike\tL shoots up from below, impaling you!\tn",
     "\tLA deadly \tWspike\tL shoots up, impaling $n!\tn",
     "You notice a \tWspike trap\tn!", FALSE, 0},
    
    /* TRAP_TYPE_TANGLE */
    {"tangle", DAM_RESERVED_DBC, TRAP_SAVE_REFLEX, TRAP_SPECIAL_SLOW,
     "\twYou are caught in tangling \twstrands\tw!\tn",
     "\tw$n is caught in tangling \twstrands\tw!\tn",
     "You notice a \twtangle trap\tn!", TRUE, 5},
    
    /* TRAP_TYPE_DART */
    {"dart", DAM_PUNCTURE, TRAP_SAVE_REFLEX, TRAP_SPECIAL_NONE,
     "\tLA \tRpoisoned dart\tL strikes you!\tn",
     "\tLA \tRpoisoned dart\tL strikes $n!\tn",
     "You notice a \tRdart trap\tn!", FALSE, 0},
    
    /* TRAP_TYPE_PIT */
    {"pit", DAM_PUNCTURE, TRAP_SAVE_REFLEX, TRAP_SPECIAL_NONE,
     "\tLYou fall into a \tRspiked pit\tL!\tn",
     "\tL$n falls into a \tRspiked pit\tL!\tn",
     "You notice a \tRpit trap\tn!", FALSE, 0},
    
    /* TRAP_TYPE_DISPEL */
    {"dispel magic", DAM_FORCE, TRAP_SAVE_NONE, TRAP_SPECIAL_NONE,
     "\tCA flash of light dispels your magical protections!\tn",
     "\tCA flash of light dispels $n's magical protections!\tn",
     "You notice a \tCdispel magic trap\tn!", FALSE, 0},
    
    /* TRAP_TYPE_AMBUSH */
    {"ambush", DAM_RESERVED_DBC, TRAP_SAVE_NONE, TRAP_SPECIAL_SUMMON_CREATURE,
     "\tRYou are ambushed by hidden assailants!\tn",
     "\tR$n is ambushed by hidden assailants!\tn",
     "You notice signs of an \tRambush trap\tn!", FALSE, 0},
    
    /* TRAP_TYPE_BOULDER */
    {"boulder", DAM_FORCE, TRAP_SAVE_REFLEX, TRAP_SPECIAL_NONE,
     "\tyA massive \tyboulder\ty crashes down on you!\tn",
     "\tyA massive \tyboulder\ty crashes down on $n!\tn",
     "You notice a \tyboulder trap\tn!", FALSE, 0},
    
    /* TRAP_TYPE_WALL_SMASH */
    {"wall smash", DAM_FORCE, TRAP_SAVE_REFLEX, TRAP_SPECIAL_NONE,
     "\tcA wall suddenly smashes into you!\tn",
     "\tcA wall suddenly smashes into $n!\tn",
     "You notice a \tcwall trap\tn!", FALSE, 0},
    
    /* TRAP_TYPE_SPIDER_HORDE */
    {"spider horde", DAM_PUNCTURE, TRAP_SAVE_REFLEX, TRAP_SPECIAL_SUMMON_CREATURE,
     "\tmA horde of \tRspiders\tm drops onto you from above!\tn",
     "\tmA horde of \tRspiders\tm drops onto $n from above!\tn",
     "You notice a \tmspider trap\tn!", FALSE, 0},
    
    /* TRAP_TYPE_GLYPH */
    {"dark glyph", DAM_MENTAL, TRAP_SAVE_WILL, TRAP_SPECIAL_FEEBLEMIND,
     "\tDA \tDdark glyph\tD sears your mind!\tn",
     "\tDA \tDdark glyph\tD sears $n's mind!\tn",
     "You notice a \tDdark glyph\tn!", FALSE, 0},
    
    /* TRAP_TYPE_SKELETAL_HANDS */
    {"skeletal hands", DAM_COLD, TRAP_SAVE_REFLEX, TRAP_SPECIAL_NONE,
     "\tDSkeletal hands reach up from below, grasping at you!\tn",
     "\tDSkeletal hands reach up from below, grasping at $n!\tn",
     "You notice signs of \tDundead\tn lurking!", FALSE, 0}
};

/* ============================================================================ */
/* Trap Creation and Management Functions                                       */
/* ============================================================================ */

/**
 * Creates a new trap with the specified parameters.
 * Allocates memory and initializes all fields.
 */
struct trap_data *create_trap(int trap_type, int severity, int trigger_type)
{
    struct trap_data *trap;
    const struct trap_type_template *template;
    const struct trap_severity_data *sev_data;
    
    if (trap_type < 0 || trap_type >= NUM_TRAP_TYPES)
        trap_type = TRAP_TYPE_SPIKE;
    if (severity < 0 || severity >= NUM_TRAP_SEVERITIES)
        severity = TRAP_SEVERITY_MINOR;
    if (trigger_type < 0 || trigger_type >= NUM_TRAP_TRIGGERS)
        trigger_type = TRAP_TRIGGER_ENTER_ROOM;
    
    CREATE(trap, struct trap_data, 1);
    
    template = &trap_type_table[trap_type];
    sev_data = &trap_severity_table[severity];
    
    trap->trap_type = trap_type;
    trap->severity = severity;
    trap->trigger_type = trigger_type;
    trap->detect_dc = sev_data->detect_dc_base;
    trap->disarm_dc = sev_data->disarm_dc_base;
    trap->save_dc = sev_data->save_dc_base;
    trap->save_type = template->save_type;
    trap->damage_type = template->damage_type;
    trap->special_effect = template->special_effect;
    trap->flags = TRAP_FLAG_ONE_SHOT; // Default: one-shot traps
    
    /* Set damage dice based on trap type and severity */
    switch (trap_type)
    {
        case TRAP_TYPE_ACID_BLOB:
            trap->damage_dice_num = 3 + (severity * 3);
            trap->damage_dice_size = 6;
            trap->special_duration = 2 + severity;
            break;
        case TRAP_TYPE_ACID_SPLASH:
            trap->damage_dice_num = 2 + severity;
            trap->damage_dice_size = 8;
            break;
        case TRAP_TYPE_ELECTRICAL:
            trap->damage_dice_num = 8 + (severity * 5);
            trap->damage_dice_size = 6;
            trap->area_radius = 15;
            trap->max_targets = 4 + severity;
            SET_BIT(trap->flags, TRAP_FLAG_AREA_EFFECT);
            break;
        case TRAP_TYPE_FIRE:
            trap->damage_dice_num = 5 + (severity * 5);
            trap->damage_dice_size = 6;
            trap->area_radius = (severity < 2) ? 5 : 10;
            trap->max_targets = 99; // hits everyone in area
            SET_BIT(trap->flags, TRAP_FLAG_AREA_EFFECT);
            break;
        case TRAP_TYPE_FROST:
            trap->damage_dice_num = 2 + severity;
            trap->damage_dice_size = 4;
            trap->special_duration = 1 + severity;
            break;
        case TRAP_TYPE_GAS:
            // Gas traps don't do direct damage, they poison
            trap->special_duration = 10 + (severity * 2);
            trap->area_radius = 15;
            SET_BIT(trap->flags, TRAP_FLAG_AREA_EFFECT);
            break;
        case TRAP_TYPE_HOLY:
            trap->damage_dice_num = 2 + (severity * 2);
            trap->damage_dice_size = 4;
            break;
        case TRAP_TYPE_NEGATIVE:
            trap->damage_dice_num = 2 + severity;
            trap->damage_dice_size = 6;
            trap->special_duration = 10;
            break;
        case TRAP_TYPE_SONIC:
            trap->damage_dice_num = 2 + severity;
            trap->damage_dice_size = 4;
            trap->special_duration = 2 + severity;
            trap->area_radius = 10;
            trap->max_targets = 99;
            SET_BIT(trap->flags, TRAP_FLAG_AREA_EFFECT);
            break;
        case TRAP_TYPE_SPIKE:
            trap->damage_dice_num = 2 + severity;
            trap->damage_dice_size = 6;
            break;
        case TRAP_TYPE_TANGLE:
            trap->special_duration = 3 + severity;
            trap->area_radius = (severity < 2) ? 5 : 10;
            SET_BIT(trap->flags, TRAP_FLAG_AREA_EFFECT);
            break;
        default:
            trap->damage_dice_num = 2 + severity;
            trap->damage_dice_size = 6;
            break;
    }
    
    /* Copy template messages */
    if (template->trigger_msg_char)
        trap->trigger_message_char = strdup(template->trigger_msg_char);
    if (template->trigger_msg_room)
        trap->trigger_message_room = strdup(template->trigger_msg_room);
    
    trap->trap_name = strdup(template->name);
    trap->next = NULL;
    
    return trap;
}

/**
 * Frees a trap and all its allocated memory.
 */
void free_trap(struct trap_data *trap)
{
    if (!trap)
        return;
    
    if (trap->trap_name)
        free(trap->trap_name);
    if (trap->trigger_message_char)
        free(trap->trigger_message_char);
    if (trap->trigger_message_room)
        free(trap->trigger_message_room);
    
    free(trap);
}

/**
 * Creates a copy of a trap (deep copy).
 */
struct trap_data *copy_trap(struct trap_data *source)
{
    struct trap_data *trap;
    
    if (!source)
        return NULL;
    
    CREATE(trap, struct trap_data, 1);
    *trap = *source;  // Copy all fields
    
    // Deep copy strings
    if (source->trap_name)
        trap->trap_name = strdup(source->trap_name);
    if (source->trigger_message_char)
        trap->trigger_message_char = strdup(source->trigger_message_char);
    if (source->trigger_message_room)
        trap->trigger_message_room = strdup(source->trigger_message_room);
    
    trap->next = NULL;  // Don't copy the list link
    
    return trap;
}

/**
 * Attaches a trap to a room (adds to trap list).
 */
void attach_trap_to_room(struct trap_data *trap, room_rnum room)
{
    if (!trap || room < 0 || room > top_of_world)
        return;
    
    // Add to front of room's trap list
    trap->next = world[room].traps;
    world[room].traps = trap;
    
    // Set room flag
    SET_BIT_AR(ROOM_FLAGS(room), ROOM_HASTRAP);
}

/**
 * Attaches a trap to an object.
 */
void attach_trap_to_object(struct trap_data *trap, struct obj_data *obj)
{
    if (!trap || !obj)
        return;
    
    // Objects can only have one trap
    if (obj->trap)
        free_trap(obj->trap);
    
    obj->trap = trap;
    trap->next = NULL;  // Objects don't use trap lists
}

/**
 * Removes a specific trap from a room's trap list.
 */
void remove_trap_from_room(struct trap_data *trap, room_rnum room)
{
    struct trap_data *temp, *prev = NULL;
    
    if (!trap || room < 0 || room > top_of_world)
        return;
    
    for (temp = world[room].traps; temp; prev = temp, temp = temp->next)
    {
        if (temp == trap)
        {
            if (prev)
                prev->next = temp->next;
            else
                world[room].traps = temp->next;
            
            // Check if room has any more traps
            if (!world[room].traps)
                REMOVE_BIT_AR(ROOM_FLAGS(room), ROOM_HASTRAP);
            
            return;
        }
    }
}

/**
 * Removes trap from an object.
 */
void remove_trap_from_object(struct obj_data *obj)
{
    if (!obj || !obj->trap)
        return;
    
    free_trap(obj->trap);
    obj->trap = NULL;
}

/* ============================================================================ */
/* Trap Generation Functions                                                    */
/* ============================================================================ */

/**
 * Determines trap severity based on zone level.
 */
int determine_trap_severity(int zone_level)
{
    int roll = dice(1, 100);
    
    if (zone_level <= 5)
    {
        // Low level zones: mostly minor/average
        if (roll <= 60) return TRAP_SEVERITY_MINOR;
        else return TRAP_SEVERITY_AVERAGE;
    }
    else if (zone_level <= 10)
    {
        // Mid-low level: minor/average/some strong
        if (roll <= 30) return TRAP_SEVERITY_MINOR;
        else if (roll <= 80) return TRAP_SEVERITY_AVERAGE;
        else return TRAP_SEVERITY_STRONG;
    }
    else if (zone_level <= 15)
    {
        // Mid level: average/strong
        if (roll <= 40) return TRAP_SEVERITY_AVERAGE;
        else return TRAP_SEVERITY_STRONG;
    }
    else if (zone_level <= 20)
    {
        // Mid-high level: strong/deadly
        if (roll <= 40) return TRAP_SEVERITY_STRONG;
        else return TRAP_SEVERITY_DEADLY;
    }
    else
    {
        // Epic level: deadly/epic
        if (roll <= 60) return TRAP_SEVERITY_DEADLY;
        else return TRAP_SEVERITY_EPIC;
    }
}

/**
 * Gets a random trap type.
 */
int get_random_trap_type(void)
{
    return dice(1, NUM_TRAP_TYPES) - 1;
}

/**
 * Generates a random trap based on zone level.
 */
struct trap_data *generate_random_trap(int zone_level)
{
    int trap_type, severity, trigger_type;
    struct trap_data *trap;
    
    trap_type = get_random_trap_type();
    severity = determine_trap_severity(zone_level);
    
    // Determine trigger type
    trigger_type = dice(1, 100);
    if (trigger_type <= 60)
        trigger_type = TRAP_TRIGGER_ENTER_ROOM;
    else if (trigger_type <= 80)
        trigger_type = TRAP_TRIGGER_LEAVE_ROOM;
    else
        trigger_type = TRAP_TRIGGER_OPEN_CONTAINER;
    
    trap = create_trap(trap_type, severity, trigger_type);
    
    if (trap)
    {
        SET_BIT(trap->flags, TRAP_FLAG_AUTO_GENERATED);
        SET_BIT(trap->flags, TRAP_FLAG_MECHANICAL); // Most random traps are mechanical
    }
    
    return trap;
}

/**
 * Auto-generates trap in a specific room based on zone level.
 */
void auto_generate_room_trap(room_rnum room, int zone_level)
{
    struct trap_data *trap;
    
    if (room < 0 || room > top_of_world)
        return;
    
    // Don't add if room already has traps
    if (world[room].traps)
        return;
    
    trap = generate_random_trap(zone_level);
    if (trap)
    {
        attach_trap_to_room(trap, room);
        log("TRAP: Auto-generated %s trap (severity: %s) in room %d",
            get_trap_type_name(trap->trap_type),
            get_trap_severity_name(trap->severity),
            GET_ROOM_VNUM(room));
    }
}

/**
 * Auto-generates trap on an object based on zone level.
 */
void auto_generate_object_trap(struct obj_data *obj, int zone_level)
{
    struct trap_data *trap;
    
    if (!obj)
        return;
    
    // Don't add if object already has a trap
    if (obj->trap)
        return;
    
    trap = generate_random_trap(zone_level);
    if (trap)
    {
        // Object traps are typically OPEN_CONTAINER or UNLOCK_CONTAINER
        trap->trigger_type = (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) ?
            TRAP_TRIGGER_OPEN_CONTAINER : TRAP_TRIGGER_OPEN_DOOR;
        
        attach_trap_to_object(trap, obj);
        log("TRAP: Auto-generated %s trap (severity: %s) on object %d",
            get_trap_type_name(trap->trap_type),
            get_trap_severity_name(trap->severity),
            GET_OBJ_VNUM(obj));
    }
}

/**
 * Auto-generates traps throughout a zone during zone reset.
 */
void auto_generate_zone_traps(zone_rnum zone)
{
    room_rnum room;
    int num_traps = 0, zone_level;
    
    if (zone < 0 || zone > top_of_zone_table)
        return;
    
    zone_level = zone_table[zone].min_level;
    
    // Generate traps for rooms in this zone
    for (room = 0; room <= top_of_world; room++)
    {
        if (world[room].zone != zone)
            continue;
        
        // Check if this room should have a trap
        if (ROOM_FLAGGED(room, ROOM_RANDOM_TRAP))
        {
            auto_generate_room_trap(room, zone_level);
            num_traps++;
        }
    }
    
    if (num_traps > 0)
    {
        log("TRAP: Auto-generated %d traps in zone %d", num_traps, zone_table[zone].number);
    }
}

/* This is part 1 of the trap system implementation. The file continues... */
