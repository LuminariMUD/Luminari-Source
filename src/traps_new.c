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
 * 
 * If ZONE_RANDOM_TRAPS flag is set, generates traps randomly throughout the zone
 * at a rate of approximately 1 trap per NUM_OF_ZONE_ROOMS_PER_RANDOM_TRAP rooms.
 * 
 * If individual rooms have ROOM_RANDOM_TRAP flag, they will ALWAYS get a trap
 * regardless of the zone flag.
 */
void auto_generate_zone_traps(zone_rnum zone)
{
    room_rnum room;
    int num_traps = 0, zone_level, total_rooms = 0;
    bool zone_has_flag;
    
    if (zone < 0 || zone > top_of_zone_table)
        return;
    
    zone_level = zone_table[zone].min_level;
    zone_has_flag = ZONE_FLAGGED(zone, ZONE_RANDOM_TRAPS);
    
    // Count total rooms in zone first (for percentage calculation)
    for (room = 0; room <= top_of_world; room++)
    {
        if (world[room].zone == zone)
            total_rooms++;
    }
    
    // Generate traps for rooms in this zone
    for (room = 0; room <= top_of_world; room++)
    {
        if (world[room].zone != zone)
            continue;
        
        // ALWAYS generate trap if room has ROOM_RANDOM_TRAP flag
        if (ROOM_FLAGGED(room, ROOM_RANDOM_TRAP))
        {
            auto_generate_room_trap(room, zone_level);
            num_traps++;
        }
        // If zone has ZONE_RANDOM_TRAPS flag, randomly generate traps
        else if (zone_has_flag)
        {
            // Random chance based on NUM_OF_ZONE_ROOMS_PER_RANDOM_TRAP
            // This gives approximately 1 trap per 33 rooms
            if (dice(1, NUM_OF_ZONE_ROOMS_PER_RANDOM_TRAP) == 1)
            {
                auto_generate_room_trap(room, zone_level);
                num_traps++;
            }
        }
    }
    
    if (num_traps > 0)
    {
        log("TRAP: Auto-generated %d traps in zone %d (%d rooms total, ~1 trap per %d rooms)", 
            num_traps, zone_table[zone].number, total_rooms, NUM_OF_ZONE_ROOMS_PER_RANDOM_TRAP);
    }
}


/* ============================================================================ */
/* Trap Detection and Disarming Functions                                       */
/* ============================================================================ */

/**
 * Get trap detect DC including perk bonuses.
 */
int get_trap_detect_dc(struct trap_data *trap, struct char_data *ch)
{
    int dc = trap->detect_dc;
    
    // Trapfinding feat reduces DC
    if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_TRAPFINDING))
        dc -= 4;
    
    // Trapfinding Expert perks
    dc -= get_trapfinding_bonus(ch);
    
    return MAX(1, dc);  // Minimum DC of 1
}

/**
 * Get trap disarm DC including perk bonuses.
 */
int get_trap_disarm_dc(struct trap_data *trap, struct char_data *ch)
{
    int dc = trap->disarm_dc;
    
    // Trapfinding Expert perks
    dc -= get_trapfinding_bonus(ch);
    
    return MAX(1, dc);  // Minimum DC of 1
}

/**
 * Find the first undetected trap in a room.
 */
struct trap_data *find_trap_in_room(struct char_data *ch, room_rnum room)
{
    struct trap_data *trap;
    
    if (room < 0 || room > top_of_world)
        return NULL;
    
    for (trap = world[room].traps; trap; trap = trap->next)
    {
        if (!IS_SET(trap->flags, TRAP_FLAG_DETECTED))
            return trap;
    }
    
    return NULL;
}

/**
 * Find trap on an object.
 */
struct trap_data *find_trap_on_object(struct obj_data *obj)
{
    if (!obj)
        return NULL;
    
    return obj->trap;
}

/**
 * Detect a trap - called from search command.
 */
bool detect_trap(struct char_data *ch, struct trap_data *trap)
{
    int dc, roll;
    
    if (!ch || !trap)
        return FALSE;
    
    // Already detected
    if (IS_SET(trap->flags, TRAP_FLAG_DETECTED))
        return FALSE;
    
    dc = get_trap_detect_dc(trap, ch);
    roll = skill_check(ch, ABILITY_PERCEPTION, dc);
    
    if (roll)
    {
        SET_BIT(trap->flags, TRAP_FLAG_DETECTED);
        return TRUE;
    }
    
    return FALSE;
}

/**
 * Disarm a trap - called from disable device command.
 */
bool disarm_trap(struct char_data *ch, struct trap_data *trap)
{
    int dc, roll;
    
    if (!ch || !trap)
        return FALSE;
    
    // Must be detected first
    if (!IS_SET(trap->flags, TRAP_FLAG_DETECTED))
    {
        send_to_char(ch, "You don't see any trap to disarm.\r\n");
        return FALSE;
    }
    
    // Already disarmed
    if (IS_SET(trap->flags, TRAP_FLAG_DISARMED))
    {
        send_to_char(ch, "That trap has already been disarmed.\r\n");
        return FALSE;
    }
    
    dc = get_trap_disarm_dc(trap, ch);
    roll = skill_check(ch, ABILITY_DISABLE_DEVICE, dc);
    
    if (roll)
    {
        SET_BIT(trap->flags, TRAP_FLAG_DISARMED);
        return TRUE;
    }
    else if (roll <= -5)  // Critical failure - trigger trap!
    {
        return FALSE; // Caller will handle triggering
    }
    
    return FALSE;
}

/* ============================================================================ */
/* Trap Triggering Functions                                                    */
/* ============================================================================ */

/**
 * Check if trap should trigger based on action type.
 */
bool check_trap_trigger(struct char_data *ch, int trigger_type, room_rnum room,
                       struct obj_data *obj, int direction)
{
    struct trap_data *trap;
    
    if (!ch || IS_NPC(ch))
        return FALSE;
    
    // Check room traps
    if (room >= 0 && room <= top_of_world)
    {
        for (trap = world[room].traps; trap; trap = trap->next)
        {
            // Skip if already triggered or disarmed
            if (IS_SET(trap->flags, TRAP_FLAG_TRIGGERED) ||
                IS_SET(trap->flags, TRAP_FLAG_DISARMED))
                continue;
            
            // Check if trigger type matches
            if (trap->trigger_type == trigger_type)
            {
                // For door traps, check direction
                if ((trigger_type == TRAP_TRIGGER_OPEN_DOOR ||
                     trigger_type == TRAP_TRIGGER_UNLOCK_DOOR) &&
                    trap->trigger_direction != direction)
                    continue;
                
                // Trigger the trap!
                trigger_trap(ch, trap, room);
                return TRUE;
            }
        }
    }
    
    // Check object traps
    if (obj && obj->trap)
    {
        trap = obj->trap;
        
        if (!IS_SET(trap->flags, TRAP_FLAG_TRIGGERED) &&
            !IS_SET(trap->flags, TRAP_FLAG_DISARMED) &&
            trap->trigger_type == trigger_type)
        {
            trigger_trap(ch, trap, IN_ROOM(ch));
            return TRUE;
        }
    }
    
    return FALSE;
}

/**
 * Trigger a trap - apply effects to character(s).
 */
void trigger_trap(struct char_data *ch, struct trap_data *trap, room_rnum room)
{
    if (!ch || !trap)
        return;
    
    // Mark as triggered
    SET_BIT(trap->flags, TRAP_FLAG_TRIGGERED);
    
    // Send trigger messages
    if (trap->trigger_message_char)
        act(trap->trigger_message_char, FALSE, ch, 0, 0, TO_CHAR);
    if (trap->trigger_message_room)
        act(trap->trigger_message_room, FALSE, ch, 0, 0, TO_ROOM);
    
    // Apply trap effects
    if (IS_SET(trap->flags, TRAP_FLAG_AREA_EFFECT))
        apply_trap_to_area(trap, room, ch);
    else
        apply_trap_damage(ch, trap);
    
    // Apply special effects
    if (trap->special_effect != TRAP_SPECIAL_NONE)
        apply_trap_special_effect(ch, trap);
}

/* ============================================================================ */
/* Trap Effect Application Functions                                            */
/* ============================================================================ */

/**
 * Apply trap damage to a single character.
 */
void apply_trap_damage(struct char_data *ch, struct trap_data *trap)
{
    int dam = 0, save_roll = 0, save_dc = 0;
    bool saved = FALSE;
    
    if (!ch || !trap)
        return;
    
    // Calculate damage
    if (trap->damage_dice_num > 0 && trap->damage_dice_size > 0)
    {
        dam = dice(trap->damage_dice_num, trap->damage_dice_size);
    }
    
    // Apply trap sense bonus to saves
    save_dc = trap->save_dc;
    
    // Make saving throw if applicable
    if (trap->save_type != TRAP_SAVE_NONE)
    {
        int save_type_idx = SAVING_FORT; // Default
        
        switch (trap->save_type)
        {
            case TRAP_SAVE_REFLEX:
                save_type_idx = SAVING_REFL;
                break;
            case TRAP_SAVE_FORTITUDE:
                save_type_idx = SAVING_FORT;
                break;
            case TRAP_SAVE_WILL:
                save_type_idx = SAVING_WILL;
                break;
        }
        
        // Add trap sense bonus to save
        save_dc -= get_trap_sense_bonus(ch);
        
        save_roll = savingthrow(ch, save_type_idx, save_dc, 0);
        saved = (save_roll > 0);
        
        // Successful save usually halves damage (or negates special effect)
        if (saved && trap->special_effect == TRAP_SPECIAL_NONE)
        {
            dam /= 2;
            send_to_char(ch, "You partially resist the trap's effects!\r\n");
        }
    }
    
    // Apply damage
    if (dam > 0)
    {
        // Apply trap sense AC bonus (handled in combat code, but noted here)
        damage(ch, ch, dam, -1, trap->damage_type, -1);
    }
    
    // Special damage types
    switch (trap->trap_type)
    {
        case TRAP_TYPE_HOLY:
            // Extra damage vs undead
            if (IS_UNDEAD(ch))
            {
                int bonus_dam = dice(trap->damage_dice_num * 2, trap->damage_dice_size);
                damage(ch, ch, bonus_dam, -1, trap->damage_type, -1);
            }
            break;
            
        case TRAP_TYPE_NEGATIVE:
            // Heals undead
            if (IS_UNDEAD(ch))
            {
                GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + dam);
                send_to_char(ch, "The negative energy heals you!\r\n");
            }
            break;
    }
}

/**
 * Apply trap effects to area (multiple targets).
 */
void apply_trap_to_area(struct trap_data *trap, room_rnum room, struct char_data *triggerer)
{
    struct char_data *vict, *next_vict;
    int targets_hit = 0;
    
    if (!trap || room < 0 || room > top_of_world)
        return;
    
    // Apply to all characters in room
    for (vict = world[room].people; vict && targets_hit < trap->max_targets; vict = next_vict)
    {
        next_vict = vict->next_in_room;
        
        // Skip NPCs unless they're pets
        if (IS_NPC(vict) && !IS_PET(vict))
            continue;
        
        // Apply damage and effects
        apply_trap_damage(vict, trap);
        targets_hit++;
    }
}

/**
 * Apply special trap effects (paralysis, stun, poison, etc).
 */
void apply_trap_special_effect(struct char_data *ch, struct trap_data *trap)
{
    struct affected_type af;
    int save_roll, save_dc, save_type_idx;
    bool saved = FALSE;
    
    if (!ch || !trap || trap->special_effect == TRAP_SPECIAL_NONE)
        return;
    
    // Make saving throw to resist special effect
    save_dc = trap->save_dc - get_trap_sense_bonus(ch);
    
    switch (trap->save_type)
    {
        case TRAP_SAVE_REFLEX:
            save_type_idx = SAVING_REFL;
            break;
        case TRAP_SAVE_FORTITUDE:
            save_type_idx = SAVING_FORT;
            break;
        case TRAP_SAVE_WILL:
            save_type_idx = SAVING_WILL;
            break;
        default:
            save_type_idx = SAVING_FORT;
            break;
    }
    
    save_roll = savingthrow(ch, save_type_idx, save_dc, 0);
    saved = (save_roll > 0);
    
    if (saved && trap->special_effect != TRAP_SPECIAL_SUMMON_CREATURE)
    {
        send_to_char(ch, "You resist the trap's special effect!\r\n");
        return;
    }
    
    // Initialize affect
    new_affect(&af);
    af.duration = trap->special_duration;
    
    // Apply the special effect
    switch (trap->special_effect)
    {
        case TRAP_SPECIAL_PARALYSIS:
            if (!paralysis_immunity(ch))
            {
                af.spell = SPELL_HOLD_PERSON;
                SET_BIT_AR(af.bitvector, AFF_PARALYZED);
                send_to_char(ch, "You are paralyzed!\r\n");
                act("$n is paralyzed!", FALSE, ch, 0, 0, TO_ROOM);
            }
            break;
            
        case TRAP_SPECIAL_SLOW:
            af.spell = SPELL_SLOW;
            SET_BIT_AR(af.bitvector, AFF_SLOW);
            send_to_char(ch, "You feel sluggish!\r\n");
            break;
            
        case TRAP_SPECIAL_STUN:
            af.spell = SPELL_POWER_WORD_STUN;
            SET_BIT_AR(af.bitvector, AFF_STUN);
            send_to_char(ch, "You are stunned!\r\n");
            act("$n is stunned!", FALSE, ch, 0, 0, TO_ROOM);
            break;
            
        case TRAP_SPECIAL_POISON:
            if (can_poison(ch))
            {
                af.spell = SPELL_POISON;
                SET_BIT_AR(af.bitvector, AFF_POISON);
                send_to_char(ch, "You feel poison coursing through your veins!\r\n");
            }
            break;
            
        case TRAP_SPECIAL_ABILITY_DRAIN:
            // Strength drain for negative energy traps
            af.location = APPLY_STR;
            af.modifier = -(1 + (trap->severity / 2));
            send_to_char(ch, "You feel your strength draining away!\r\n");
            break;
            
        case TRAP_SPECIAL_ENTANGLE:
            af.spell = SPELL_WEB;
            SET_BIT_AR(af.bitvector, AFF_ENTANGLED);
            send_to_char(ch, "You are entangled!\r\n");
            break;
            
        case TRAP_SPECIAL_FEEBLEMIND:
            af.spell = SPELL_FEEBLEMIND;
            af.location = APPLY_INT;
            af.modifier = -10;
            send_to_char(ch, "Your mind goes blank!\r\n");
            break;
            
        case TRAP_SPECIAL_SUMMON_CREATURE:
            // Summon hostile creatures
            {
                int mob_vnum = 0, count = 0, i;
                
                switch (trap->trap_type)
                {
                    case TRAP_TYPE_AMBUSH:
                        mob_vnum = TRAP_DARK_WARRIOR_MOBILE;
                        count = 1 + (GET_LEVEL(ch) / 5);  // 1-4 based on level
                        break;
                    case TRAP_TYPE_SPIDER_HORDE:
                        mob_vnum = TRAP_SPIDER_MOBILE;
                        count = dice(1, 3);
                        break;
                    default:
                        return;
                }
                
                for (i = 0; i < count; i++)
                {
                    struct char_data *mob = read_mobile(mob_vnum, VIRTUAL);
                    if (mob)
                    {
                        if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS))
                        {
                            X_LOC(mob) = world[IN_ROOM(ch)].coords[0];
                            Y_LOC(mob) = world[IN_ROOM(ch)].coords[1];
                        }
                        char_to_room(mob, IN_ROOM(ch));
                        remember(mob, ch);
                        
                        // Auto-purge after 3 minutes
                        attach_mud_event(new_mud_event(ePURGEMOB, mob, NULL), (180 * PASSES_PER_SEC));
                    }
                }
                send_to_char(ch, "Hostile creatures emerge from the trap!\r\n");
            }
            return;  // Don't apply affect for summons
    }
    
    // Apply the affect if it's not a summon
    if (trap->special_effect != TRAP_SPECIAL_SUMMON_CREATURE)
    {
        affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
    }
}

/* ============================================================================ */
/* Trap Information and Utility Functions                                       */
/* ============================================================================ */

/**
 * Get trap name.
 */
const char *get_trap_name(struct trap_data *trap)
{
    if (!trap)
        return "unknown";
    
    if (trap->trap_name)
        return trap->trap_name;
    
    if (trap->trap_type >= 0 && trap->trap_type < NUM_TRAP_TYPES)
        return trap_type_table[trap->trap_type].name;
    
    return "unknown";
}

/**
 * Get trap severity name.
 */
const char *get_trap_severity_name(int severity)
{
    if (severity < 0 || severity >= NUM_TRAP_SEVERITIES)
        return "unknown";
    
    return trap_severity_table[severity].name;
}

/**
 * Get trap type name.
 */
const char *get_trap_type_name(int trap_type)
{
    if (trap_type < 0 || trap_type >= NUM_TRAP_TYPES)
        return "unknown";
    
    return trap_type_table[trap_type].name;
}

/**
 * Get trap component value for recovery.
 */
int get_trap_component_value(struct trap_data *trap)
{
    if (!trap)
        return 0;
    
    // Value based on severity
    return trap_severity_table[trap->severity].detect_dc_base * 10;
}

/**
 * Show detailed trap information to character.
 */
void show_trap_info(struct char_data *ch, struct trap_data *trap)
{
    if (!ch || !trap)
        return;
    
    if (!IS_SET(trap->flags, TRAP_FLAG_DETECTED))
    {
        send_to_char(ch, "You don't see any trap.\r\n");
        return;
    }
    
    send_to_char(ch, "\r\n\tcTrap Information:\tn\r\n");
    send_to_char(ch, "  Type: %s\r\n", get_trap_name(trap));
    send_to_char(ch, "  Severity: %s\r\n", get_trap_severity_name(trap->severity));
    send_to_char(ch, "  Status: %s%s%s\r\n",
                 IS_SET(trap->flags, TRAP_FLAG_DISARMED) ? "\tGDisarmed\tn" : "",
                 IS_SET(trap->flags, TRAP_FLAG_TRIGGERED) ? "\tRTriggered\tn" : "",
                 (!IS_SET(trap->flags, TRAP_FLAG_DISARMED) && !IS_SET(trap->flags, TRAP_FLAG_TRIGGERED)) ? "\tYArmed\tn" : "");
    
    // Show DCs to those with high enough skill
    if (GET_ABILITY(ch, ABILITY_DISABLE_DEVICE) >= 5)
    {
        send_to_char(ch, "  Detect DC: %d\r\n", trap->detect_dc);
        send_to_char(ch, "  Disarm DC: %d\r\n", trap->disarm_dc);
        
        if (trap->damage_dice_num > 0)
            send_to_char(ch, "  Damage: %dd%d %s\r\n", 
                        trap->damage_dice_num, trap->damage_dice_size,
                        damtype_display[trap->damage_type]);
    }
    
    send_to_char(ch, "\r\n");
}

/* ============================================================================ */
/* Perk Integration Functions                                                   */
/* ============================================================================ */

/**
 * Get trapfinding bonus from perks.
 */
int get_trapfinding_bonus(struct char_data *ch)
{
    int bonus = 0;
    
    if (!ch || IS_NPC(ch))
        return 0;
    
    // Trapfinding Expert I: +3 per rank
    bonus += get_perk_trapfinding_bonus(ch);
    
    return bonus;
}

/**
 * Get trap sense bonus from perks.
 */
int get_trap_sense_bonus(struct char_data *ch)
{
    int bonus = 0;
    
    if (!ch || IS_NPC(ch))
        return 0;
    
    // Trap Sense perk: +2 per rank
    bonus += get_perk_trap_sense_bonus(ch);
    
    return bonus;
}

/**
 * Check if character can recover trap components.
 */
bool can_recover_trap_components(struct char_data *ch)
{
    if (!ch || IS_NPC(ch))
        return FALSE;
    
    // Must have Trap Scavenger perk
    return HAS_PERK(ch, PERK_ROGUE_TRAP_SCAVENGER);
}

/**
 * Recover trap components after disarming.
 */
void recover_trap_components(struct char_data *ch, struct trap_data *trap)
{
    struct obj_data *components;
    int value;
    
    if (!ch || !trap || !can_recover_trap_components(ch))
        return;
    
    if (!IS_SET(trap->flags, TRAP_FLAG_DISARMED))
        return;
    
    value = get_trap_component_value(trap);
    
    // Create a generic "trap components" object (you'll need to create this object)
    // For now, just give gold as a placeholder
    increase_gold(ch, value);
    send_to_char(ch, "You salvage trap components worth %d gold coins!\r\n", value);
}

/* ============================================================================ */
/* Player Commands                                                              */
/* ============================================================================ */

/**
 * Search command integration - detect traps.
 * This should be called from the existing do_search command in act.other.c
 */
int search_for_traps(struct char_data *ch)
{
    struct trap_data *trap;
    int exp, found = FALSE;
    
    if (!ch)
        return FALSE;
    
    // Find first undetected trap in room
    trap = find_trap_in_room(ch, IN_ROOM(ch));
    
    if (!trap)
        return FALSE;  // No traps to find
    
    // Try to detect it
    if (detect_trap(ch, trap))
    {
        act("You have detected a \tRtrap\tn!", FALSE, ch, 0, 0, TO_CHAR);
        act("$n has detected a \tRtrap\tn!", FALSE, ch, 0, 0, TO_ROOM);
        
        // Show what kind of trap it is
        if (trap->trap_type >= 0 && trap->trap_type < NUM_TRAP_TYPES)
            act(trap_type_table[trap->trap_type].detect_msg, FALSE, ch, 0, 0, TO_CHAR);
        
        // Grant experience
        exp = trap->detect_dc * 100;
        send_to_char(ch, "You receive %d experience points.\r\n", 
                    gain_exp(ch, exp, GAIN_EXP_MODE_TRAP));
        
        return TRUE;
    }
    
    return FALSE;
}

/**
 * Perform autosearch for traps - used when PRF_AUTOSEARCH is enabled.
 * This function automatically checks for traps at half perception skill
 * when entering a room. It operates silently unless a trap is found.
 */
void perform_autosearch(struct char_data *ch)
{
    struct trap_data *trap;
    int perception_bonus, dc, roll;
    
    if (!ch || IS_NPC(ch))
        return;
    
    // Find first undetected trap in room
    trap = find_trap_in_room(ch, IN_ROOM(ch));
    
    if (!trap)
        return;  // No traps to find
    
    // Calculate perception at HALF skill (penalty for autosearch)
    perception_bonus = GET_ABILITY(ch, ABILITY_PERCEPTION) / 2;
    perception_bonus += get_trapfinding_bonus(ch);
    perception_bonus += get_trap_sense_bonus(ch);
    
    // Get trap detect DC
    dc = get_trap_detect_dc(trap, ch);
    
    // Make the check: 1d20 + (perception/2) + bonuses vs DC
    roll = dice(1, 20) + perception_bonus;
    
    if (roll >= dc)
    {
        // Success! Detected the trap
        SET_BIT(trap->flags, TRAP_FLAG_DETECTED);
        
        act("\tyYour cautious movement alerts you to a \tRtrap\ty!\tn", FALSE, ch, 0, 0, TO_CHAR);
        
        // Show what kind of trap it is
        if (trap->trap_type >= 0 && trap->trap_type < NUM_TRAP_TYPES)
            send_to_char(ch, "%s\r\n", trap_type_table[trap->trap_type].detect_msg);
        
        // Grant reduced experience (half of normal, since it's automatic)
        int exp = (trap->detect_dc * 100) / 2;
        if (exp > 0)
        {
            send_to_char(ch, "\tyYou receive %d experience points for your alertness.\tn\r\n", 
                        gain_exp(ch, exp, GAIN_EXP_MODE_TRAP));
        }
    }
    // On failure, no message (silent failure for autosearch)
}

/**
 * Disable trap command.
 */
ACMD(do_disabletrap)
{
    struct trap_data *trap = NULL;
    int exp, result;
    
    if (!GET_ABILITY(ch, ABILITY_DISABLE_DEVICE))
    {
        send_to_char(ch, "You don't know how to disable traps.\r\n");
        return;
    }
    
    // Find a detected trap in the room
    for (trap = world[IN_ROOM(ch)].traps; trap; trap = trap->next)
    {
        if (IS_SET(trap->flags, TRAP_FLAG_DETECTED) &&
            !IS_SET(trap->flags, TRAP_FLAG_DISARMED))
            break;
    }
    
    if (!trap)
    {
        send_to_char(ch, "There are no detected traps here to disable.\r\n");
        return;
    }
    
    act("$n carefully attempts to disable the trap...", FALSE, ch, 0, 0, TO_ROOM);
    send_to_char(ch, "You carefully attempt to disable the trap...\r\n");
    
    if (disarm_trap(ch, trap))
    {
        act("$n successfully disables the trap!", FALSE, ch, 0, 0, TO_ROOM);
        send_to_char(ch, "You successfully disable the trap!\r\n");
        
        // Grant experience
        exp = trap->disarm_dc * trap->disarm_dc * 100;
        send_to_char(ch, "You receive %d experience points.\r\n",
                    gain_exp(ch, exp, GAIN_EXP_MODE_TRAP));
        
        // Try to recover components
        if (can_recover_trap_components(ch))
            recover_trap_components(ch, trap);
        
        // Remove auto-generated traps after disarming
        if (IS_SET(trap->flags, TRAP_FLAG_AUTO_GENERATED))
        {
            remove_trap_from_room(trap, IN_ROOM(ch));
            free_trap(trap);
        }
    }
    else
    {
        send_to_char(ch, "You fail to disable the trap.\r\n");
        
        // Check if we critically failed (triggers trap)
        result = skill_check(ch, ABILITY_DISABLE_DEVICE, trap->disarm_dc);
        if (result <= -5)
        {
            send_to_char(ch, "\tRYour clumsy attempt triggers the trap!\tn\r\n");
            trigger_trap(ch, trap, IN_ROOM(ch));
        }
    }
    
    USE_FULL_ROUND_ACTION(ch);
}

/**
 * Show trap info command.
 */
ACMD(do_trapinfo)
{
    struct trap_data *trap;
    
    // Find a detected trap in the room
    for (trap = world[IN_ROOM(ch)].traps; trap; trap = trap->next)
    {
        if (IS_SET(trap->flags, TRAP_FLAG_DETECTED))
        {
            show_trap_info(ch, trap);
            return;
        }
    }
    
    send_to_char(ch, "You don't see any detected traps here.\r\n");
}

/* ============================================================================ */
/* Legacy System Compatibility                                                  */
/* ============================================================================ */

/**
 * Legacy trap check function - maintained for backward compatibility.
 */
bool check_trap(struct char_data *ch, int trap_type, int room, struct obj_data *obj, int dir)
{
    // Convert old trap type to new trigger type
    int trigger_type;
    
    switch (trap_type)
    {
        case 0: trigger_type = TRAP_TRIGGER_LEAVE_ROOM; break;
        case 1: trigger_type = TRAP_TRIGGER_OPEN_DOOR; break;
        case 2: trigger_type = TRAP_TRIGGER_UNLOCK_DOOR; break;
        case 3: trigger_type = TRAP_TRIGGER_OPEN_CONTAINER; break;
        case 4: trigger_type = TRAP_TRIGGER_UNLOCK_CONTAINER; break;
        case 5: trigger_type = TRAP_TRIGGER_GET_OBJECT; break;
        case 6: trigger_type = TRAP_TRIGGER_ENTER_ROOM; break;
        default: return FALSE;
    }
    
    return check_trap_trigger(ch, trigger_type, room, obj, dir);
}

/**
 * Legacy set_off_trap - placeholder for old ITEM_TRAP objects.
 */
void set_off_trap(struct char_data *ch, struct obj_data *trap)
{
    // This is for old ITEM_TRAP objects - kept for compatibility
    // New system uses trap_data structures instead
    send_to_char(ch, "Ooops, you triggered something!\r\n");
}

/**
 * Legacy is_trap_detected.
 */
bool is_trap_detected(struct obj_data *trap)
{
    return (GET_OBJ_VAL(trap, 4) > 0);
}

/**
 * Legacy set_trap_detected.
 */
void set_trap_detected(struct obj_data *trap)
{
    GET_OBJ_VAL(trap, 4) = 1;
}

/**
 * Legacy perform_detecttrap.
 */
int perform_detecttrap(struct char_data *ch, bool silent)
{
    if (!silent)
        USE_FULL_ROUND_ACTION(ch);
    
    return search_for_traps(ch);
}

/**
 * Legacy do_detecttrap command.
 */
ACMD(do_detecttrap)
{
    if (!GET_ABILITY(ch, ABILITY_PERCEPTION))
    {
        send_to_char(ch, "You don't know how to detect traps.\r\n");
        return;
    }
    
    if (search_for_traps(ch))
    {
        USE_FULL_ROUND_ACTION(ch);
        return;
    }
    
    send_to_char(ch, "You don't detect any traps.\r\n");
    USE_FULL_ROUND_ACTION(ch);
}

/* END OF FILE */
