/* Faction Mission System Designed for d20MUD Star Wars by Gicker aka Stephen Squires */

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

const char * const mission_details[][8] = {
    // We want to order this from highest level to lowest level
    //         FACTIONS ALLOWED (1 (one) if allowed, 0 (zero) if not)
    //  Lvl    R    E    H    F   Planet         Room Vnum Zone Name
    {""}
    {"17", "0", "0", "0", "1", "Prime Material Plane - Surface", "2701", "Memlin Caverns"},
    {"16", "0", "0", "0", "1", "Prime Material Plane - Surface", "1901", "Spider Swamp"},
    {"14", "0", "0", "0", "1", "Prime Material Plane - Surface", "148100", "Orcish Fort"},
    {"12", "0", "0", "0", "1", "Prime Material Plane - Surface", "40400", "Blindbreak Rest"},
    {"10", "0", "0", "0", "1", "Prime Material Plane - Surface", "401", "Jade Forest"},
    {"10", "0", "0", "0", "1", "Prime Material Plane - Surface", "29000", "Lizard Lair"},
    {"10", "0", "0", "0", "1", "Prime Material Plane - Surface", "13100", "Quagmire"},
    {"7" , "0", "0", "0", "1", "Prime Material Plane - Surface", "6701", "Graven Hollow"},
    {"3" , "0", "0", "0", "1", "Prime Material Plane - Surface", "5900", "Wizard Training Mansion"},
    {"1" , "0", "0", "0", "1", "Prime Material Plane - Surface", "7700", "Crystal Swamp"},
    {"1" , "0", "0", "0", "1", "Prime Material Plane - Surface", "3000", "Northern Midgen"},
    {"1" , "0", "0", "0", "1", "Prime Material Plane - Surface", "3100", "Southern Midgen"},
    {"1" , "0", "0", "0", "1", "Prime Material Plane - Surface", "103000", "Ashenport"},
    {"0" , "0", "0", "0", "0", "0", "0", "0"} // This must always be last
};

const char * const mission_targets[5] = { "person", "traitor", "darkling", "fugitive", "fugitive" };

const char * const mission_difficulty[5]
= { "normal", "tough", "challenging", "arduous", "severe" };

const char * const target_difficulty[5] = {
    "of normal strength", "above normal strength", "well above normal strength",
    "far above normal strength ", "far above normal strength " };

const char * const guard_difficulty[5] = {
    "of normal strength",         "of normal strength",
    "above normal strength",      "above normal strength",
    "well above normal strength",
};

int mission_details_to_faction(int faction)
{
    switch (faction)
    {
    case FACTION_THE_ORDER:
        return MISSION_REBELS;
    case FACTION_DARKLINGS:
        return MISSION_EMPIRE;
    case FACTION_CRIMINAL:
        return MISSION_HUTTS;
    }
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
        if (GET_CURRENT_MISSION(ch) < 0)
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
                faction_names[GET_MISSION_FACTION(ch)]);
            GET_MISSION_DECLINE(ch) = true;
            clear_mission_mobs(ch);
            return 1;
        }
        GET_MISSION_DECLINE(ch) = false;
        GET_FACTION_STANDING(ch, GET_MISSION_FACTION(ch)) -= 50;
        send_to_char(
            ch,
            "You have turned down the offered mission.  This has reduced your faction standing with the %s by 50 points.\r\n",
            faction_names[GET_MISSION_FACTION(ch)]);
        GET_CURRENT_MISSION(ch) = -1;
        save_char(ch);
        return 1;
    }

    if (is_abbrev(argument, "accept"))
    {
        if (GET_CURRENT_MISSION(ch) < 0)
        {
            send_to_char(ch, "You do not currently have a mission.\r\n");
            return 1;
        }
        if (!GET_MISSION_DECLINE(ch))
        {
            send_to_char(ch, "You have already accepted this mission.\r\n");
            return 1;
        }
        GET_MISSION_DECLINE(ch) = false;
        send_to_char(ch, "You have accepted the mission offered to you.\r\n");
        return 1;
    }

    struct char_data *mob = (struct char_data *) me;
    int level = GET_LEVEL(ch);
    int faction = GET_FACTION(mob);
    int difficulty = 0;
    int i = 0, count = 0, mDet = 0;

    for (i = 0; i < 5; i++)
    {
        if (is_abbrev(argument, mission_difficulty[i]))
            break;
    }

    if (i >= 5)
    {
        send_to_char(
            ch,
            "That is not a valid difficulty level.  Select among the following difficulty levels:\r\n");
        for (i = 0; i < 5; i++)
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
        if (atoi(mission_details[i][mission_details_to_faction(faction)])
            == 0)
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
        if (atoi(mission_details[i][mission_details_to_faction(faction)])
            == 0)
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

    mDet = i - 1;

    GET_CURRENT_MISSION(ch) = mDet;
    GET_MISSION_FACTION(ch) = faction;
    GET_MISSION_DIFFICULTY(ch) = difficulty;
    GET_MISSION_STANDING(ch) = get_mission_reward(ch, MISSION_STANDING);
    GET_MISSION_REP(ch) = get_mission_reward(ch, MISSION_REP);
    GET_MISSION_CREDITS(ch) = get_mission_reward(ch, MISSION_CREDITS);
    GET_MISSION_EXP(ch) = get_mission_reward(ch, MISSION_EXP);
    GET_MISSION_NPC_NAME_NUM(ch) = dice(1, NUM_RANDOM_NPC_NAMES) - 1;

    char buf[MAX_STRING_LENGTH];

    sprintf(
        buf,
        "%s I've got something for you from the %s.  We're having some trouble with %s %s named %s, located somewhere in the "
        "%s part of %s.  We need you to terminate them with extreme predjudice.  We have uploaded "
        "information to your commlink, designating your target with a label showing your name.  They "
        "will likely not be alone, so be prepared to fight multiple enemies.  When the deed is done, "
        "return to me for your pay.  Take note: this mission is of %s difficulty. Your target will be "
        "%s and the guards will be %s. That is all.\r\n",
        GET_NAME(ch), faction_names[faction],
        AN(mission_targets[mission_details_to_faction(faction)]),
        mission_targets[mission_details_to_faction(faction)],
        random_npc_names[GET_MISSION_NPC_NAME_NUM(ch)],
        mission_details[mDet][MISSION_ZONE_NAME],
        mission_details[mDet][MISSION_PLANET], mission_difficulty[difficulty],
        target_difficulty[difficulty], guard_difficulty[difficulty]);

    do_tell(mob, strdup(buf), 0, 0);

    clear_mission_mobs(ch);
    create_mission_mobs(ch);

    return 1;
}

ACMD(do_missions)
{
    int i = 0;

    if (GET_CURRENT_MISSION(ch) <= 0)
    {
        send_to_char(ch, "You are not currently on a mission.");
        send_to_char(
            ch,
            "You may start a new mission by specifying a difficulty level.  Select among the following difficulty levels:\r\n");
        for (i = 0; i < 5; i++)
            send_to_char(
                ch,
                "-- %11s      - challenge level %d: %s target and %s guards\r\n",
                mission_difficulty[i], i + 1, target_difficulty[i],
                guard_difficulty[i]);
        return;
    }
    send_to_char(
        ch,
        "Your mission is to terminate %s %s located somewhere in the %s part of %s.  Your reward will be %ld credits, %ld %s standing, %ld overall reputation and %ld experience points.\r\n",
        AN(mission_targets[mission_details_to_faction(
            GET_MISSION_FACTION(ch))]),
        mission_targets[mission_details_to_faction(GET_MISSION_FACTION(ch))],
        mission_details[GET_CURRENT_MISSION(ch)][MISSION_ZONE_NAME],
        mission_details[GET_CURRENT_MISSION(ch)][MISSION_PLANET],
        GET_MISSION_CREDITS(ch), GET_MISSION_STANDING(ch),
        faction_names[GET_MISSION_FACTION(ch)], GET_MISSION_REP(ch),
        GET_MISSION_EXP(ch));
}

long get_mission_reward(const char_data *ch, int reward_type)
{
    int reward = 0;
    int level = GET_LEVEL(ch);
    int mult = MAX(1, GET_MISSION_DIFFICULTY(ch));

    switch (reward_type)
    {
    case MISSION_CREDITS:
        reward = level * MAX(1, level / 2) * mult * 50 / 4;
        break;
    case MISSION_STANDING:
        reward = 50 + (50 * mult);
        break;
    case MISSION_REP:
        reward = level * MAX(1, level / 2) * mult / 4;
        break;
    case MISSION_EXP:
        reward = level * MAX(1, level / 4) * mult * 50;
        break;
    }

    int bonus = compute_ability(ch, ABILITY_DIPLOMACY);
      reward = (reward * (100+bonus)) / 100;

    return reward;
}

void clear_mission_mobs(const char_data *ch)
{
    struct char_data *mob = NULL;

    for (mob = character_list; mob; mob = mob->next)
    {
        if (!IS_NPC(mob))
            continue;
        if (GET_IDNUM(ch) == mob->mission_owner)
        {
            char_from_room(mob);
            extract_char(mob);
        }
    }
}

void increase_mob_difficulty(struct char_data *mob, int difficulty)
{
    switch (difficulty) {
        case MISSION_DIFF_TOUGH:
            GET_MAX_HIT(mob) = GET_MAX_HIT(mob) * 2;
            GET_HITROLL(mob) += 2;
            GET_DAMROLL(mob) += 2;
            GET_AC(mob) += 20;
            break;
        case MISSION_DIFF_TOUGH:
            GET_MAX_HIT(mob) = GET_MAX_HIT(mob) * 4;
            GET_HITROLL(mob) += 3;
            GET_DAMROLL(mob) += 3;
            GET_AC(mob) += 30;
            break;
        case MISSION_DIFF_CHALLENGING:
            GET_MAX_HIT(mob) = GET_MAX_HIT(mob) * 8;
            GET_HITROLL(mob) += 5;
            GET_DAMROLL(mob) += 5;
            GET_AC(mob) += 50;
            break;
        case MISSION_DIFF_ARDUOUS:
            GET_MAX_HIT(mob) = GET_MAX_HIT(mob) * 12;
            GET_HITROLL(mob) += 8;
            GET_DAMROLL(mob) += 8;
            GET_AC(mob) += 120;
            break;
        case MISSION_DIFF_SEVERE:
            GET_MAX_HIT(mob) = GET_MAX_HIT(mob) * 16;
            GET_HITROLL(mob) += 10;
            GET_DAMROLL(mob) += 10;
            GET_AC(mob) += 160;
            break;
    }
}

void create_mission_mobs(const char_data *ch)
{
    struct char_data *mob = NULL;
    int i = 0, randName = 0;
    room_vnum to_room = 0;
    if (GET_CURRENT_MISSION(ch) >= 0)
        to_room = atoi(mission_details[GET_CURRENT_MISSION(ch)][6]);
    char buf[MAX_STRING_LENGTH];

    for (i = 0; i < 4; i++)
    {
        mob = read_mobile(60000, VIRTUAL);
        if (!mob)
            return;
        GET_HITDICE(mob) = GET_LEVEL(ch);
        medit_autoroll_stats(d);
        if (i > 0)
        {
            SET_BIT_AR(MOB_FLAGS(mob), MOB_SENTINEL);
        }
        switch (GET_MISSION_DIFFICULTY(ch))
        {
        case MISSION_DIFF_TOUGH:
            if (i == 0) {
              increase_mob_difficulty(mob, MISSION_DIFF_TOUGH)
            }
            break;
        case MISSION_DIFF_CHALLENGING:
            if (i == 0)
                increase_mob_difficulty(mob, MISSION_DIFF_CHALLENGING)
            else
                increase_mob_difficulty(mob, MISSION_DIFF_TOUGH)
            break;
        case MISSION_DIFF_ARDUOUS:
            if (i == 0)
                increase_mob_difficulty(mob, MISSION_DIFF_ARDUOUS)
            else
                increase_mob_difficulty(mob, MISSION_DIFF_TOUGH)
        case MISSION_DIFF_SEVERE:
            if (i == 0)
                increase_mob_difficulty(mob, MISSION_DIFF_SEVERE)
            else
                increase_mob_difficulty(mob, MISSION_DIFF_CHALLENGING)
        }
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
        mob->name = strdup(buf);
        sprintf(buf, "%s %s%s%s",
            AN(mission_targets[mission_details_to_faction(
                GET_MISSION_FACTION(ch))]),
            mission_targets[mission_details_to_faction(
                GET_MISSION_FACTION(ch))],
                (i > 0) ? "" : " named ",
            (i > 0) ? " guard" : random_npc_names[randName]);
        mob->short_descr = strdup(buf);
        sprintf(buf, "%s %s%s%s (%s) is here.\r\n",
            AN(mission_targets[mission_details_to_faction(
                GET_MISSION_FACTION(ch))]),
            mission_targets[mission_details_to_faction(
                GET_MISSION_FACTION(ch))],
                (i > 0) ? "" : " named ",
            (i > 0) ? " guard" : random_npc_names[randName],
            GET_NAME(ch));
        mob->long_descr = strdup(buf);
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

bool are_mission_mobs_loaded(const char_data *ch)
{
    struct char_data *mob = NULL;

    if (GET_MISSION_COMPLETE(ch))
        return false;

    for (mob = character_list; mob; mob = mob->next)
    {
        if (!IS_NPC(mob))
            continue;
        if (mob->dead)
            continue;
        if (GET_IDNUM(ch) == mob->mission_owner)
            return true;
    }

    return false;
}

void apply_mission_rewards(char_data *ch)
{
    if (GET_MISSION_COMPLETE(ch))
        return;

    send_to_char(ch, "\r\nYou've completed your mission!\r\n\r\n");

    send_to_char(ch, "You've received %ld %s faction standing.\r\n",
        GET_MISSION_STANDING(ch),
        faction_names[GET_MISSION_FACTION(ch)]);
    ch->faction_standing[GET_MISSION_FACTION(ch)] += GET_MISSION_STANDING(ch);

    send_to_char(ch, "You've received %ld general reputation.\r\n",
        GET_MISSION_REP(ch));
    GET_QUESTPOINTS(ch) += GET_MISSION_REP(ch);
    GET_REPUTATION(ch) += GET_MISSION_REP(ch);

    send_to_char(ch,
        "A sum of credits have been wired to your bank account.\r\n");
    gain_gold(ch, GET_MISSION_CREDITS(ch), GOLD_BANK);

    send_to_char(
        ch,
        "You have earned experience points for completing your mission.\r\n");
    gain_exp(ch, GET_MISSION_EXP(ch));

    send_to_char(ch, "\r\n");

// bug fix.  After doing one mission can't complete any others.
//    GET_MISSION_COMPLETE(ch) = true;
    clear_mission(ch);
}

bool is_mission_mob(const char_data *ch, const char_data *mob)
{
    if (!ch || !mob)
        return false;

    if (!IS_NPC(mob))
        return false;

    if (mob->mission_owner == GET_IDNUM(ch))
        return true;

    struct char_data *tch = NULL;

    for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
    {
        if (IS_NPC(tch) && tch->master && !IS_NPC(tch->master)
            && is_player_grouped(ch, tch->master)
            && mob->mission_owner == GET_IDNUM(tch->master))
            return true;
        if (!IS_NPC(tch) && is_player_grouped(ch, tch)
            && mob->mission_owner == GET_IDNUM(tch))
            return true;
    }

    return false;
}

void clear_mission(char_data *ch)
{
    GET_CURRENT_MISSION(ch) = 0;
    GET_MISSION_FACTION(ch) = 0;
    GET_MISSION_DIFFICULTY(ch) = 0;
    GET_MISSION_STANDING(ch) = 0;
    GET_MISSION_REP(ch) = 0;
    GET_MISSION_CREDITS(ch) = 0;
    GET_MISSION_EXP(ch) = 0;
    GET_MISSION_NPC_NAME_NUM(ch) = 0;
}

void create_mission_on_entry(const char_data *ch)
{
    if (GET_CURRENT_MISSION(ch) <= 0)
        return;

    if (!are_mission_mobs_loaded(ch))
        create_mission_mobs(ch);
}
