/* Faction Mission System Designed by Gicker aka Stephen Squires */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"
#include "mud_event.h"
#include "mail.h" /**< For the has_mail function */
#include "act.h"
#include "class.h"
#include "race.h"
#include "fight.h"
#include "modify.h"
#include "asciimap.h"
#include "spells.h"
#include "clan.h"
#include "craft.h" // auto crafting quest
#include "wilderness.h"
#include "quest.h" /* so you can identify questmaster mobiles */
#include "feats.h"
#include "assign_wpn_armor.h"
#include "domains_schools.h"
#include "desc_engine.h"
#include "crafts.h"
#include "alchemy.h"
#include "premadebuilds.h"
#include "missions.h"
#include "random_names.h"
#include "spec_procs.h"
#include "oasis.h"
#include "mudlim.h"
#include "genmob.h"
#include "treasure.h" /* for magic awards */
#include "hunts.h"

/* inits */
int gain_exp(struct char_data *ch, int gain, int mode);
int is_player_grouped(struct char_data *target, struct char_data *group);

/* constants */
const char *const mission_details[][MISSION_DETAIL_FIELDS] = {
    // We want to order this from highest level to lowest level
    //         FACTIONS ALLOWED (1 (one) if allowed, 0 (zero) if not)
    // Lvl  R    E    H    F    Greater Location               Room Vnum    Zone Name
    {"7", "0", "0", "0", "1", "Prime Material Plane - Surface", "6721", "Graven Hollow"},
    {"3", "0", "0", "0", "1", "Prime Material Plane - Surface", "5915", "Wizard Training Mansion"},
    {"17", "0", "0", "0", "1", "Prime Material Plane - Surface", "2721", "Memlin Caverns"},
    {"16", "0", "0", "0", "1", "Prime Material Plane - Surface", "1901", "Spider Swamp"},
    {"14", "0", "0", "0", "1", "Prime Material Plane - Surface", "148107", "Orcish Fort"},
    {"12", "0", "0", "0", "1", "Prime Material Plane - Surface", "40403", "Blindbreak Rest"},
    {"10", "0", "0", "0", "1", "Prime Material Plane - Surface", "406", "Jade Forest"},
    {"10", "0", "0", "0", "1", "Prime Material Plane - Surface", "29009", "Lizard Lair"},
    {"10", "0", "0", "0", "1", "Prime Material Plane - Surface", "13130", "Quagmire"},
    {"7", "0", "0", "0", "1", "Prime Material Plane - Surface", "6721", "Graven Hollow"},
    {"3", "0", "0", "0", "1", "Prime Material Plane - Surface", "5915", "Wizard Training Mansion"},

    //    {"1" , "0", "0", "0", "1", "Prime Material Plane - Surface", "3014", "Northern Midgen"},
    //    {"1" , "0", "0", "0", "1", "Prime Material Plane - Surface", "3104", "Southern Midgen"},
    //    {"1" , "0", "0", "0", "1", "Prime Material Plane - Surface", "103163", "Ashenport"},

    {"0", "0", "0", "0", "0", "0", "0", "0"} // This must always be last
};

const char *const mission_targets[MISSION_TARGET_FIELDS] = {
    "person",
    "traitor",
    "darkling",
    "fugitive",
    "fugitive",
};

const char *const mission_difficulty[NUM_MISSION_DIFFICULTIES] = {
    "easy",
    "normal",
    "tough",
    "challenging",
    "arduous",
    "severe",
};

const char *const target_difficulty[NUM_MISSION_DIFFICULTIES] = {
    "of easy strength",
    "of normal strength",
    "above normal strength",
    "well above normal strength",
    "far above normal strength",
    "far above normal strength",
};

const char *const guard_difficulty[NUM_MISSION_DIFFICULTIES] = {
    "of easy strength",
    "of normal strength",
    "of normal strength",
    "above normal strength",
    "above normal strength",
    "well above normal strength",
};

/* begin fuctnions */
int mission_details_to_faction(int faction)
{
#if defined(CAMPAIGN_DL)
    switch (faction)
    {
    case FACTION_NONE:
        return MISSION_HUTTS;
    case FACTION_FORCES_OF_WHITESTONE:
        return MISSION_REBELS;
    case FACTION_DRAGONARMIES:
        return MISSION_EMPIRE;
    }
#else
    switch (faction)
    {
    case FACTION_THE_ORDER:
        return MISSION_REBELS;
    case FACTION_DARKLINGS:
        return MISSION_EMPIRE;
    case FACTION_CRIMINAL:
        return MISSION_HUTTS;
    }
#endif
    return MISSION_FREELANCERS;
}

SPECIAL(faction_mission)
{
    if (!CMD_IS("mission"))
        return 0;

    skip_spaces(&argument);
    if (!*argument)
    {
        return 0;
    }

    if (is_abbrev(argument, "decline"))
    {
        if (GET_CURRENT_MISSION(ch) <= 0)
        {
            send_to_char(ch, "You do not currently have a mission.\r\n");
            return 1;
        }

        if (!GET_MISSION_DECLINE(ch))
        {
            send_to_char(
                ch,
                "If you wish to confirm this mission decline, type \tYmission decline\tn a second time.\r\n"
                "If you do not wish to decline the mission, type \tYmission accept\tn to clear the decline flag.\r\n"
                "Please note that declining will reduce your %s faction by 50 points.\r\n",
                faction_names_lwr[GET_MISSION_FACTION(ch)]);
            GET_MISSION_DECLINE(ch) = true;
            return 1;
        }
        GET_MISSION_DECLINE(ch) = false;
        GET_FACTION_STANDING(ch, GET_MISSION_FACTION(ch)) -= 50;
        send_to_char(
            ch,
            "You have turned down the offered mission.  This has reduced your faction standing with the %s by 50 points. Your cooldown has been halved.\r\n",
            faction_names_lwr[GET_MISSION_FACTION(ch)]);
        GET_CURRENT_MISSION(ch) = -1;
        GET_MISSION_COOLDOWN(ch) /= 2;
        clear_mission(ch);
        save_char(ch, 0);
        return 1;
    }

    if (GET_CURRENT_MISSION(ch) > 0)
    {
        send_to_char(ch, "You are currently on a mission.  Type 'mission decline' before taking a new one.\r\n");
        return TRUE;
    }

    if (GET_MISSION_COOLDOWN(ch) > 0 && GET_LEVEL(ch) < LVL_GRSTAFF)
    {
        send_to_char(ch, "take a mission right now.  Check the cooldowns command for more info.\r\n");
        return 1;
    }

    struct char_data *mob = (struct char_data *)me;
    int level = GET_LEVEL(ch);
    int faction = GET_FACTION(mob);
    int difficulty = 0;
    int i = 0, count = 0, mDet = 0;

    for (i = 0; i < NUM_MISSION_DIFFICULTIES; i++)
    {
        if (is_abbrev(argument, mission_difficulty[i]))
            break;
    }

    if (i >= NUM_MISSION_DIFFICULTIES)
    {
        send_to_char(
            ch,
            "That is not a valid difficulty level.  Select among the following difficulty levels:\r\n");
        for (i = 0; i < NUM_MISSION_DIFFICULTIES; i++)
            send_to_char(
                ch,
                "-- %11s      - challenge level %d: %s target and %s guards\r\n",
                mission_difficulty[i], i + 1, target_difficulty[i],
                guard_difficulty[i]);
        return 1;
    }

    //  difficulty = i+1;
    difficulty = i;

    i = 0;

    while (atoi(mission_details[i][0]) != 0)
    {
        if (atoi(mission_details[i][mission_details_to_faction(faction)]) == 0)
        {
            i++;
            continue;
        }
        if (atoi(mission_details[i][0]) > level)
        {
            i++;
            continue;
        }
        i++;
        count++;
    }

    mDet = rand_number(1, count);
    count = i = 0;

    while (atoi(mission_details[i][0]) != 0)
    {
        if (atoi(mission_details[i][mission_details_to_faction(faction)]) == 0)
        {
            i++;
            continue;
        }
        if (atoi(mission_details[i][0]) > level)
        {
            i++;
            continue;
        }
        i++;
        count++;
        if (mDet == count)
            break;
    }

    mDet = MAX(1, i - 1);

    GET_CURRENT_MISSION(ch) = mDet;
    GET_MISSION_FACTION(ch) = faction;
    GET_MISSION_DIFFICULTY(ch) = difficulty;
    GET_MISSION_STANDING(ch) = get_mission_reward(ch, MISSION_STANDING);
    GET_MISSION_REP(ch) = get_mission_reward(ch, MISSION_REP);
    GET_MISSION_CREDITS(ch) = get_mission_reward(ch, MISSION_CREDITS);
    GET_MISSION_EXP(ch) = get_mission_reward(ch, MISSION_EXP);
    GET_MISSION_NPC_NAME_NUM(ch) = dice(1, NUM_RANDOM_NPC_NAMES) - 1;

    create_mission_mobs(ch);

    char buf[MAX_STRING_LENGTH] = {'\0'};

    snprintf(
        buf, sizeof(buf),
        "\tM$N tells you, 'I've got something for you from the %s.\r\n"
        "We're having some trouble with %s %s named %s.\r\n"
        "They were last seen somewhere near %s.\r\n"
        "We need you to terminate them with extreme predjudice. They will likely not be \r\n"
        "alone, so be prepared to fight multiple enemies.\r\n"
        "Take note: this mission is of %s difficulty. Your target will be %s and the guards will be %s.'\r\n",
        faction_names_lwr[faction],
        AN(mission_targets[mission_details_to_faction(faction)]),
        mission_targets[mission_details_to_faction(faction)],
        random_npc_names[GET_MISSION_NPC_NAME_NUM(ch)],
#if defined(CAMPAIGN_DL)
        get_mission_zone_name(ch),
#else
        mission_details[mDet][MISSION_ZONE_NAME],
#endif
        //        mission_details[mDet][MISSION_PLANET],
        mission_difficulty[difficulty],
        target_difficulty[difficulty], guard_difficulty[difficulty]);
    act(buf, FALSE, ch, 0, (struct char_data *)me, TO_CHAR);

    send_to_char(
        ch,
        "Your reward will be %ld gold coins, %ld %s standing, %ld quest points and %ld experience points.\r\n\tn",
        GET_MISSION_CREDITS(ch), GET_MISSION_STANDING(ch),
        faction_names_lwr[GET_MISSION_FACTION(ch)], GET_MISSION_REP(ch),
        GET_MISSION_EXP(ch));
        
    GET_MISSION_COOLDOWN(ch) = 100; // ten minutes

    return 1;
}

const char * get_mission_zone_name(struct char_data *ch)
{
#if defined(CAMPAIGN_DL)
    if (GET_CURRENT_MISSION_ROOM(ch) == NOWHERE)
        return "Unknown";
    char name[400];
    snprintf(name, sizeof(name), "%s", zone_table[world[GET_CURRENT_MISSION_ROOM(ch)].zone].name);
    if (is_abbrev(name, "the") || is_abbrev(name, "The"))
    {
        return zone_table[world[GET_CURRENT_MISSION_ROOM(ch)].zone].name;
    }
    else
    {
        snprintf(name, sizeof(name), "the %s", zone_table[world[GET_CURRENT_MISSION_ROOM(ch)].zone].name);
        return strdup(name);
    }
#else
    return mission_details[GET_CURRENT_MISSION(ch)][MISSION_ZONE_NAME];
#endif
}

ACMD(do_missions)
{
    int i = 0;

    if (GET_CURRENT_MISSION(ch) <= 0)
    {
        send_to_char(ch, "You are not currently on a mission.");
        send_to_char(ch,
            "You may start a new mission by specifying a difficulty level (you must be in a "
            "bounty/mission office). Select among the following difficulty levels:\r\n");
        for (i = 0; i < NUM_MISSION_DIFFICULTIES; i++)
            send_to_char(ch, "-- %11s      - challenge level %d: %s target and %s guards\r\n",
                mission_difficulty[i], i + 1, target_difficulty[i], guard_difficulty[i]);
        return;
    }
    send_to_char(ch,
        "Your mission is to terminate %s %s named %s.\r\n"
        "They were last seen somewhere near %s.\r\n"
        "Your reward will be %ld gold coins, %ld %s standing, %ld quest points and %ld experience points.\r\n",
        AN(mission_targets[mission_details_to_faction(GET_MISSION_FACTION(ch))]),
        mission_targets[mission_details_to_faction(GET_MISSION_FACTION(ch))],
        random_npc_names[GET_MISSION_NPC_NAME_NUM(ch)],
        get_mission_zone_name(ch),
        //        mission_details[GET_CURRENT_MISSION(ch)][MISSION_PLANET],
        GET_MISSION_CREDITS(ch), GET_MISSION_STANDING(ch),
        faction_names_lwr[GET_MISSION_FACTION(ch)], GET_MISSION_REP(ch),
        GET_MISSION_EXP(ch));
}

long get_mission_reward(char_data *ch, int reward_type)
{
    int reward = 0;
    int level = GET_LEVEL(ch);
    float mult = MAX(0, GET_MISSION_DIFFICULTY(ch));

    if (mult == 0)
        mult = 0.5;

    switch (reward_type)
    {
    case MISSION_CREDITS:
        reward = 40 + dice(level * 2, 3);
        reward *= 5 + (mult * 2);
        break;
    case MISSION_STANDING:
        reward = (int)(50 * mult);
        break;
    case MISSION_REP:
        reward = (int)(level * MAX(1, level / 2) * mult / 4);
        break;
    case MISSION_EXP:
        reward = (int)(level * MAX(1, level / 4) * mult * 300);
// rewards are way too low when compared to the DL campaign exp tables.
#if defined(CAMPAIGN_DL)
        reward *= 5;
#endif
        break;
    }

    if (reward_type != MISSION_CREDITS)
        reward = (int)(reward * 0.25); // we'll adjust this based on what the cooldown is

    int bonus = d20(ch);
    if (affected_by_spell(ch, SPELL_HONEYED_TONGUE))
        bonus = MAX(bonus, d20(ch));
    bonus += compute_ability(ch, ABILITY_DIPLOMACY);
    reward = (reward * (100 + bonus)) / 100;

    return MAX(1, reward);
}

void clear_mission_mobs(char_data *ch)
{
    struct char_data *mob = NULL;
    struct char_data *mnext = NULL;

    for (mob = character_list; mob; mob = mnext)
    {
        mnext = mob->next;
        if (!IS_NPC(mob))
            continue;
        if (GET_IDNUM(ch) == mob->mission_owner)
        {
            if (IN_ROOM(mob) != NOWHERE)
            {
                //              char_from_room(mob);
                extract_char(mob);
            }
        }
    }
}

void increase_mob_difficulty(struct char_data *mob, int difficulty)
{
    switch (difficulty)
    {
    case MISSION_DIFF_EASY:
        GET_REAL_MAX_HIT(mob) = GET_REAL_MAX_HIT(mob) * 0.5;
        GET_HITROLL(mob) -= 2;
        GET_DAMROLL(mob) -= 2;
        mob->points.armor -= 30;
        break;
    case MISSION_DIFF_TOUGH:
        GET_REAL_MAX_HIT(mob) = GET_REAL_MAX_HIT(mob) * 2;
        GET_HITROLL(mob) += 2;
        GET_DAMROLL(mob) += 2;
        mob->points.armor += 20;
        break;
    case MISSION_DIFF_CHALLENGING:
        GET_REAL_MAX_HIT(mob) = GET_REAL_MAX_HIT(mob) * 3;
        GET_HITROLL(mob) += 3;
        GET_DAMROLL(mob) += 3;
        mob->points.armor += 50;
        break;
    case MISSION_DIFF_ARDUOUS:
        GET_REAL_MAX_HIT(mob) = GET_REAL_MAX_HIT(mob) * 5;
        GET_HITROLL(mob) += 5;
        GET_DAMROLL(mob) += 5;
        mob->points.armor += 80;
        break;
    case MISSION_DIFF_SEVERE:
        GET_REAL_MAX_HIT(mob) = GET_REAL_MAX_HIT(mob) * 7.5;
        GET_HITROLL(mob) += 6;
        GET_DAMROLL(mob) += 6;
        mob->points.armor += 100;
        break;
    }
    GET_EXP(mob) = GET_LEVEL(mob) * GET_LEVEL(mob) * (75 + (10 * difficulty));
    GET_GOLD(mob) = GET_LEVEL(mob) * (10 + difficulty);
}

int select_mission_coords(int start)
{
    int x = 0, y = 0;
    int terrain = 0;
    room_rnum room = NOWHERE;
    int room_vnum = 0;

    y = dice(1, 21) - 10;
    x = dice(1, 21) - 10;

    room_vnum = get_hunt_room(start, x, y);
    room = real_room(room_vnum);

    terrain = world[room].sector_type;

    switch (terrain)
    {
    case SECT_OCEAN:
    case SECT_UD_NOSWIM:
    case SECT_UD_WATER:
    case SECT_UNDERWATER:
    case SECT_WATER_NOSWIM:
    case SECT_WATER_SWIM:
    case SECT_INSIDE:
    case SECT_INSIDE_ROOM:
        select_hunt_coords(start);
        return room_vnum;
    }

    return room_vnum;
}

#ifdef CAMPAIGN_FR

void create_mission_mobs(char_data *ch)
{
    struct char_data *mob = NULL;
    struct char_data *leader = NULL;
    int i = 0, randName = 0;
    room_vnum to_room = 0;
    if (GET_CURRENT_MISSION(ch) > 0)
        to_room = select_mission_coords(atoi(mission_details[GET_CURRENT_MISSION(ch)][6]));
    char buf[MAX_STRING_LENGTH];

    for (i = 0; i < 4; i++)
    {
        mob = read_mobile(MISSION_MOB_DFLT_VNUM, VIRTUAL);
        if (!mob)
            return;
        GET_SEX(mob) = dice(1, 2);
        if (i > 0)
        {
            GET_LEVEL(mob) = GET_LEVEL(ch) - 2;
            SET_BIT_AR(MOB_FLAGS(mob), MOB_GUARD);
            SET_BIT_AR(MOB_FLAGS(mob), MOB_SENTINEL);
            add_follower(mob, leader);
        }
        else
        {
            GET_LEVEL(mob) = GET_LEVEL(ch);
            SET_BIT_AR(MOB_FLAGS(mob), MOB_CITIZEN);
            leader = mob;
        }
        autoroll_mob(mob, TRUE, FALSE);
        mob->points.armor -= 40;
        GET_REAL_MAX_HIT(mob) = GET_HIT(mob);
        GET_NDD(mob) = GET_SDD(mob) = MAX(2, GET_LEVEL(mob) / 6) + GET_MISSION_DIFFICULTY(ch);
        GET_EXP(mob) = (GET_LEVEL(mob) * GET_LEVEL(mob) * 75);
        GET_GOLD(mob) = (GET_LEVEL(mob) * 10);
        switch (GET_MISSION_DIFFICULTY(ch))
        {
        case MISSION_DIFF_EASY:
            increase_mob_difficulty(mob, MISSION_DIFF_EASY);
            break;
        case MISSION_DIFF_TOUGH:
            if (i == 0)
            {
                increase_mob_difficulty(mob, MISSION_DIFF_TOUGH);
            }
            break;
        case MISSION_DIFF_CHALLENGING:
            if (i == 0)
                increase_mob_difficulty(mob, MISSION_DIFF_CHALLENGING);
            else
                increase_mob_difficulty(mob, MISSION_DIFF_TOUGH);
            break;
        case MISSION_DIFF_ARDUOUS:
            if (i == 0)
                increase_mob_difficulty(mob, MISSION_DIFF_ARDUOUS);
            else
                increase_mob_difficulty(mob, MISSION_DIFF_TOUGH);
        case MISSION_DIFF_SEVERE:
            if (i == 0)
                increase_mob_difficulty(mob, MISSION_DIFF_SEVERE);
            else
                increase_mob_difficulty(mob, MISSION_DIFF_CHALLENGING);
        }
        GET_MAX_HIT(mob) = GET_REAL_MAX_HIT(mob);
        GET_HIT(mob) = GET_MAX_HIT(mob);
        GET_FACTION(mob) = GET_MISSION_FACTION(ch);
        randName = GET_MISSION_NPC_NAME_NUM(ch);
        mob->mission_owner = GET_IDNUM(ch);
        sprintf(buf, "%s %s %s %ld -%s",
                AN(mission_targets[mission_details_to_faction(
                    GET_MISSION_FACTION(ch))]),
                mission_targets[mission_details_to_faction(
                    GET_MISSION_FACTION(ch))],
                (i > 0) ? " guard" : random_npc_names[randName],
                (i == 0) ? GET_IDNUM(ch) : 0, GET_NAME(ch));
        mob->player.name = strdup(buf);
        sprintf(buf, "%s %s%s%s",
                AN(mission_targets[mission_details_to_faction(
                    GET_MISSION_FACTION(ch))]),
                mission_targets[mission_details_to_faction(
                    GET_MISSION_FACTION(ch))],
                (i > 0) ? "" : " named ",
                (i > 0) ? " guard" : random_npc_names[randName]);
        mob->player.short_descr = strdup(buf);
        sprintf(buf, "%s %s%s%s (%s) is here.\r\n",
                AN(mission_targets[mission_details_to_faction(
                    GET_MISSION_FACTION(ch))]),
                mission_targets[mission_details_to_faction(
                    GET_MISSION_FACTION(ch))],
                (i > 0) ? "" : " named ",
                (i > 0) ? " guard" : random_npc_names[randName],
                GET_NAME(ch));
        mob->player.long_descr = strdup(buf);
        if (real_room(to_room) != NOWHERE)
        {
            char_to_room(mob, real_room(to_room));
            if (i > 0)
            {
                sprintf(buf, "%ld", GET_IDNUM(ch));
                do_follow(mob, strdup(buf), 0, 0);
            }
        }
    }
}

#elif defined(CAMPAIGN_DL)
void create_mission_mobs(char_data *ch)
{
    struct char_data *mob = NULL;
    struct char_data *leader = NULL;
    int i = 0, randName = 0;
    room_rnum to_room = 0;
    char player_name[MAX_NAME_LENGTH];

    if (GET_CURRENT_MISSION(ch) > 0)
    {
        to_room = get_random_road_room(1);
        GET_CURRENT_MISSION_ROOM(ch) = to_room;
    }

    if (to_room == NOWHERE)
    {
        log("Cannot create mission mobs for %s. Random road room was NOWHERE.", GET_NAME(ch));
        return;
    }

    snprintf(player_name, sizeof(player_name), "%s", GET_NAME(ch));
    for (i = 0; i < strlen(player_name); i++)
        player_name[i] = tolower(player_name[i]);

    char buf[MAX_STRING_LENGTH];

    for (i = 0; i < 4; i++)
    {
        mob = read_mobile(MISSION_MOB_DFLT_VNUM, VIRTUAL);
        if (!mob)
            return;
        GET_SEX(mob) = dice(1, 2);
        if (i > 0)
        {
            GET_LEVEL(mob) = GET_LEVEL(ch) - 2;
            SET_BIT_AR(MOB_FLAGS(mob), MOB_GUARD);
            SET_BIT_AR(MOB_FLAGS(mob), MOB_SENTINEL);
            add_follower(mob, leader);
        }
        else
        {
            GET_LEVEL(mob) = GET_LEVEL(ch);
            SET_BIT_AR(MOB_FLAGS(mob), MOB_CITIZEN);
            leader = mob;
        }
        autoroll_mob(mob, TRUE, FALSE);
        mob->points.armor -= 40;
        GET_REAL_MAX_HIT(mob) = GET_HIT(mob);
        GET_NDD(mob) = GET_SDD(mob) = MAX(2, GET_LEVEL(mob) / 6) + GET_MISSION_DIFFICULTY(ch);
        GET_EXP(mob) = (GET_LEVEL(mob) * GET_LEVEL(mob) * 75);
        GET_GOLD(mob) = (GET_LEVEL(mob) * 10);
        switch (GET_MISSION_DIFFICULTY(ch))
        {
        case MISSION_DIFF_EASY:
            increase_mob_difficulty(mob, MISSION_DIFF_EASY);
            break;
        case MISSION_DIFF_TOUGH:
            if (i == 0)
            {
                increase_mob_difficulty(mob, MISSION_DIFF_TOUGH);
            }
            break;
        case MISSION_DIFF_CHALLENGING:
            if (i == 0)
                increase_mob_difficulty(mob, MISSION_DIFF_CHALLENGING);
            else
                increase_mob_difficulty(mob, MISSION_DIFF_TOUGH);
            break;
        case MISSION_DIFF_ARDUOUS:
            if (i == 0)
                increase_mob_difficulty(mob, MISSION_DIFF_ARDUOUS);
            else
                increase_mob_difficulty(mob, MISSION_DIFF_TOUGH);
        case MISSION_DIFF_SEVERE:
            if (i == 0)
                increase_mob_difficulty(mob, MISSION_DIFF_SEVERE);
            else
                increase_mob_difficulty(mob, MISSION_DIFF_CHALLENGING);
        }
        GET_MAX_HIT(mob) = GET_REAL_MAX_HIT(mob);
        GET_HIT(mob) = GET_MAX_HIT(mob);
        GET_FACTION(mob) = GET_MISSION_FACTION(ch);
        randName = GET_MISSION_NPC_NAME_NUM(ch);
        mob->mission_owner = GET_IDNUM(ch);
        sprintf(buf, "%s %s %s %ld %s", AN(mission_targets[mission_details_to_faction(GET_MISSION_FACTION(ch))]),
                mission_targets[mission_details_to_faction(GET_MISSION_FACTION(ch))],
                (i > 0) ? " guard" : random_npc_names[randName],
                (i == 0) ? GET_IDNUM(ch) : 0, player_name);
        mob->player.name = strdup(buf);
        sprintf(buf, "%s %s%s%s",
                AN(mission_targets[mission_details_to_faction(GET_MISSION_FACTION(ch))]),
                mission_targets[mission_details_to_faction(GET_MISSION_FACTION(ch))],
                (i > 0) ? "" : " named ",
                (i > 0) ? " guard" : random_npc_names[randName]);
        mob->player.short_descr = strdup(buf);
        sprintf(buf, "%s %s%s%s (%s) is here.\r\n",
                AN(mission_targets[mission_details_to_faction(GET_MISSION_FACTION(ch))]),
                mission_targets[mission_details_to_faction(GET_MISSION_FACTION(ch))],
                (i > 0) ? "" : " named ",
                (i > 0) ? " guard" : random_npc_names[randName],
                GET_NAME(ch));
        mob->player.long_descr = strdup(buf);
        char_to_room(mob, to_room);
        if (i > 0)
        {
            sprintf(buf, "%ld", GET_IDNUM(ch));
            do_follow(mob, strdup(buf), 0, 0);
        }
    }
}
#else
void create_mission_mobs(char_data *ch)
{
    struct char_data *mob = NULL;
    struct char_data *leader = NULL;
    int i = 0, randName = 0;
    room_vnum to_room = 0;
    char buf[MAX_STRING_LENGTH] = {'\0'};

    if (GET_CURRENT_MISSION(ch) > 0)
        to_room = atoi(mission_details[GET_CURRENT_MISSION(ch)][6]);

    for (i = 0; i < 4; i++)
    {
        mob = read_mobile(MISSION_MOB_DFLT_VNUM, VIRTUAL);

        if (!mob)
            return;

        GET_SEX(mob) = dice(1, 2);

        if (i > 0)
        {
            GET_LEVEL(mob) = GET_LEVEL(ch) - 2;
            SET_BIT_AR(MOB_FLAGS(mob), MOB_GUARD);
            SET_BIT_AR(MOB_FLAGS(mob), MOB_SENTINEL);
            add_follower(mob, leader);
        }
        else
        {
            GET_LEVEL(mob) = GET_LEVEL(ch);
            SET_BIT_AR(MOB_FLAGS(mob), MOB_CITIZEN);
            leader = mob;
        }

        autoroll_mob(mob, TRUE, FALSE);

        mob->points.armor -= 40;

        GET_REAL_MAX_HIT(mob) = GET_HIT(mob);
        GET_NDD(mob) = GET_SDD(mob) = MAX(2, GET_LEVEL(mob) / 6) + GET_MISSION_DIFFICULTY(ch);
        GET_EXP(mob) = (GET_LEVEL(mob) * GET_LEVEL(mob) * 75);
        GET_GOLD(mob) = (GET_LEVEL(mob) * 10);

        switch (GET_MISSION_DIFFICULTY(ch))
        {
        case MISSION_DIFF_EASY:
            increase_mob_difficulty(mob, MISSION_DIFF_EASY);
            break;
        case MISSION_DIFF_TOUGH:
            if (i == 0)
            {
                increase_mob_difficulty(mob, MISSION_DIFF_TOUGH);
            }
            break;
        case MISSION_DIFF_CHALLENGING:
            if (i == 0)
                increase_mob_difficulty(mob, MISSION_DIFF_CHALLENGING);
            else
                increase_mob_difficulty(mob, MISSION_DIFF_TOUGH);
            break;
        case MISSION_DIFF_ARDUOUS:
            if (i == 0)
                increase_mob_difficulty(mob, MISSION_DIFF_ARDUOUS);
            else
                increase_mob_difficulty(mob, MISSION_DIFF_TOUGH);
        case MISSION_DIFF_SEVERE:
            if (i == 0)
                increase_mob_difficulty(mob, MISSION_DIFF_SEVERE);
            else
                increase_mob_difficulty(mob, MISSION_DIFF_CHALLENGING);
        }

        GET_MAX_HIT(mob) = GET_REAL_MAX_HIT(mob);
        GET_HIT(mob) = GET_MAX_HIT(mob);
        GET_FACTION(mob) = GET_MISSION_FACTION(ch);

        randName = GET_MISSION_NPC_NAME_NUM(ch);

        mob->mission_owner = GET_IDNUM(ch);

        sprintf(buf, "%s %s %s %ld -%s",
                AN(mission_targets[mission_details_to_faction(
                    GET_MISSION_FACTION(ch))]),
                mission_targets[mission_details_to_faction(
                    GET_MISSION_FACTION(ch))],
                (i > 0) ? " guard" : random_npc_names[randName],
                (i == 0) ? GET_IDNUM(ch) : 0, GET_NAME(ch));
        mob->player.name = strdup(buf);

        sprintf(buf, "%s %s%s%s",
                AN(mission_targets[mission_details_to_faction(
                    GET_MISSION_FACTION(ch))]),
                mission_targets[mission_details_to_faction(
                    GET_MISSION_FACTION(ch))],
                (i > 0) ? "" : " named ",
                (i > 0) ? " guard" : random_npc_names[randName]);
        mob->player.short_descr = strdup(buf);

        sprintf(buf, "%s %s%s%s (%s) is here.\r\n",
                AN(mission_targets[mission_details_to_faction(
                    GET_MISSION_FACTION(ch))]),
                mission_targets[mission_details_to_faction(
                    GET_MISSION_FACTION(ch))],
                (i > 0) ? "" : " named ",
                (i > 0) ? " guard" : random_npc_names[randName],
                GET_NAME(ch));
        mob->player.long_descr = strdup(buf);

        if (real_room(to_room) != NOWHERE)
        {
            if (ZONE_FLAGGED(GET_ROOM_ZONE(real_room(to_room)), ZONE_WILDERNESS))
            {
                X_LOC(mob) = world[real_room(to_room)].coords[0];
                Y_LOC(mob) = world[real_room(to_room)].coords[1];
            }

            char_to_room(mob, real_room(to_room));

            if (i > 0)
            {
                sprintf(buf, "%ld", GET_IDNUM(ch));
                do_follow(mob, strdup(buf), 0, 0);
            }
        }
    }
}

#endif

bool are_mission_mobs_loaded(char_data *ch)
{
    struct char_data *mob = NULL;

    // we return true because they aren't on a mission
    // and this function is used in limits.c on a 6 second
    // pulse to see if mission awards should be applied
    if (GET_CURRENT_MISSION(ch) <= 0)
    {
        return true;
    }

    for (mob = character_list; mob; mob = mob->next)
    {
        if (!IS_NPC(mob))
        {
            continue;
        }
        if (mob->dead)
        {
            continue;
        }
        if (GET_IDNUM(ch) == mob->mission_owner)
        {
            return true;
        }
    }

    return false;
}

void apply_mission_rewards(char_data *ch)
{
    /* easy out */
    if (GET_MISSION_COMPLETE(ch))
        return;

    int level = GET_LEVEL(ch);

    switch (GET_MISSION_DIFFICULTY(ch))
    {

    case MISSION_DIFF_EASY:
        level -= 3;
        break;

    case MISSION_DIFF_NORMAL:
        level -= 1;
        break;

    case MISSION_DIFF_TOUGH:
        break;

    case MISSION_DIFF_CHALLENGING:
        level += 1;
        break;

    case MISSION_DIFF_ARDUOUS:
        level += 2;
        break;

    case MISSION_DIFF_SEVERE:
        level += 4;
        break;

    default:
        break;
    }

    if (level <= 0)
        level = 1;
    if (level >= LVL_IMPL)
        level = LVL_IMPL;

    send_to_char(ch, "\r\nYou've completed your mission!\r\n\r\n");

    GET_FACTION_STANDING(ch, GET_MISSION_FACTION(ch)) += GET_MISSION_STANDING(ch);
    send_to_char(ch, "You've received %ld %s faction standing.\r\n",
                 GET_MISSION_STANDING(ch),
                 faction_names_lwr[GET_MISSION_FACTION(ch)]);

    GET_QUESTPOINTS(ch) += GET_MISSION_REP(ch);
    send_to_char(ch, "You've received %ld quest points.\r\n",
                 GET_MISSION_REP(ch));

    GET_GOLD(ch) += GET_MISSION_CREDITS(ch);
    send_to_char(ch, "You have received %ld gold coins.\r\n", GET_MISSION_CREDITS(ch));

    send_to_char(ch, "You have earned %d experience points for completing your mission.\r\n",
                 gain_exp(ch, GET_MISSION_EXP(ch), GAIN_EXP_MODE_QUEST));

    send_to_char(ch, "You've received a random loot drop!\r\n");
    award_magic_item(1, ch, quick_grade_check(level));

    send_to_char(ch, "\r\n");

    /* autoquest system checkpoint -zusuk */
    autoquest_trigger_check(ch, NULL, NULL, GET_MISSION_DIFFICULTY(ch), AQ_COMPLETE_MISSION);

    // bug fix.  After doing one mission can't complete any others.
    //    GET_MISSION_COMPLETE(ch) = true;
    clear_mission(ch);
}

bool is_mission_mob(char_data *ch, char_data *mob)
{
    if (!ch || !mob)
        return false;

    if (!IS_NPC(mob))
        return true;

    if (!mob->mission_owner)
        return true;

    if (mob->mission_owner == GET_IDNUM(ch))
        return true;

    struct char_data *tch = NULL;

    for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
    {
        if (IS_NPC(tch) && tch->master && !IS_NPC(tch->master) && is_player_grouped(ch, tch->master) && mob->mission_owner == GET_IDNUM(tch->master))
            return true;
        if (!IS_NPC(tch) && is_player_grouped(ch, tch) && mob->mission_owner == GET_IDNUM(tch))
            return true;
    }

    return false;
}

void clear_mission(char_data *ch)
{
    clear_mission_mobs(ch);
    GET_CURRENT_MISSION(ch) = 0;
    GET_MISSION_FACTION(ch) = 0;
    GET_MISSION_DIFFICULTY(ch) = 0;
    GET_MISSION_STANDING(ch) = 0;
    GET_MISSION_REP(ch) = 0;
    GET_MISSION_CREDITS(ch) = 0;
    GET_MISSION_EXP(ch) = 0;
    GET_MISSION_NPC_NAME_NUM(ch) = 0;
}

void create_mission_on_entry(char_data *ch)
{
    if (GET_CURRENT_MISSION(ch) <= 0)
        return;

    if (!are_mission_mobs_loaded(ch))
        create_mission_mobs(ch);
}

// This will return a random road room. A road room is one with the sector
// type being road north/south, east/west or intersection. It will also
// include rooms where the zone is flagged for missions when type is 1 and
// rooms where the zone is flagged for hunts when the type is 2. Random
// Encounters is type 3.
room_rnum get_random_road_room(int type)
{

    room_rnum cnt = NOWHERE;
    room_rnum selected_room = NOWHERE;
    int tot_rooms = 0;
    int rand_room = 0;
    int rand_count = 0;

    for (cnt = 0; cnt <= top_of_world; cnt++)
    {
        if (is_road_room(cnt, type))
            tot_rooms++;
    }

    rand_room = dice(1, tot_rooms);

    for (cnt = 0; cnt <= top_of_world; cnt++)
    {
        if (is_road_room(cnt, type))
            rand_count++;
        if (rand_room == rand_count)
        {
            selected_room = cnt;
            break;
        }
    }

    return selected_room;
}