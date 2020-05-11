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
#include "spec_procs.h"
#include "oasis.h"
#include "mudlim.h"
#include "genmob.h"

int gain_exp(struct char_data *ch, int gain, int mode);
int is_player_grouped(struct char_data *target, struct char_data *group);

const char * const mission_details[][8] = {
    // We want to order this from highest level to lowest level
    //         FACTIONS ALLOWED (1 (one) if allowed, 0 (zero) if not)
    //  Lvl    R    E    H    F   Planet         Room Vnum Zone Name
    {"7" , "0", "0", "0", "1", "Prime Material Plane - Surface", "6721", "Graven Hollow"},
    {"3" , "0", "0", "0", "1", "Prime Material Plane - Surface", "5915", "Wizard Training Mansion"},
    {"17", "0", "0", "0", "1", "Prime Material Plane - Surface", "2721", "Memlin Caverns"},
    {"16", "0", "0", "0", "1", "Prime Material Plane - Surface", "1901", "Spider Swamp"},
    {"14", "0", "0", "0", "1", "Prime Material Plane - Surface", "148107", "Orcish Fort"},
    {"12", "0", "0", "0", "1", "Prime Material Plane - Surface", "40403", "Blindbreak Rest"},
    {"10", "0", "0", "0", "1", "Prime Material Plane - Surface", "406", "Jade Forest"},
    {"10", "0", "0", "0", "1", "Prime Material Plane - Surface", "29009", "Lizard Lair"},
    {"10", "0", "0", "0", "1", "Prime Material Plane - Surface", "13130", "Quagmire"},
    {"7" , "0", "0", "0", "1", "Prime Material Plane - Surface", "6721", "Graven Hollow"},
    {"3" , "0", "0", "0", "1", "Prime Material Plane - Surface", "5915", "Wizard Training Mansion"},
//    {"1" , "0", "0", "0", "1", "Prime Material Plane - Surface", "3014", "Northern Midgen"},
//    {"1" , "0", "0", "0", "1", "Prime Material Plane - Surface", "3104", "Southern Midgen"},
//    {"1" , "0", "0", "0", "1", "Prime Material Plane - Surface", "103163", "Ashenport"},
    {"0" , "0", "0", "0", "0", "0", "0", "0"} // This must always be last
};

const char * const mission_targets[5] = { "person", "traitor", "darkling", "fugitive", "fugitive" };

const char * const mission_difficulty[NUM_MISSION_DIFFICULTIES] = { "easy", "normal", "tough", "challenging", "arduous", "severe" };

const char * const target_difficulty[NUM_MISSION_DIFFICULTIES] = {
    "of easy strength", "of normal strength", "above normal strength", "well above normal strength",
    "far above normal strength", "far above normal strength" };

const char * const guard_difficulty[NUM_MISSION_DIFFICULTIES] = {
    "of easy strength",
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

    if (GET_CURRENT_MISSION(ch) > 0) {
        send_to_char(ch, "You are currently on a mission.  Type 'mission decline' before taking a new one.\r\n");
        return TRUE;
    }

    if (GET_MISSION_COOLDOWN(ch) > 0) {
      send_to_char(ch, "You are not ready to take a mission right now.  Check the cooldowns command for more info.\r\n");
      return 1;
    }
    GET_MISSION_COOLDOWN(ch) = 100; // ten minutes

    struct char_data *mob = (struct char_data *) me;
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

    mDet = MAX(1, i - 1);

    GET_CURRENT_MISSION(ch) = mDet;
    GET_MISSION_FACTION(ch) = faction;
    GET_MISSION_DIFFICULTY(ch) = difficulty;
    GET_MISSION_STANDING(ch) = get_mission_reward(ch, MISSION_STANDING);
    GET_MISSION_REP(ch) = get_mission_reward(ch, MISSION_REP);
    GET_MISSION_CREDITS(ch) = get_mission_reward(ch, MISSION_CREDITS);
    GET_MISSION_EXP(ch) = get_mission_reward(ch, MISSION_EXP);
    GET_MISSION_NPC_NAME_NUM(ch) = dice(1, NUM_RANDOM_NPC_NAMES) - 1;

    char buf[MAX_STRING_LENGTH];

    snprintf(
        buf, sizeof(buf), 
        "\tM$N tells you, 'I've got something for you from the %s.  We're having some trouble with %s %s named %s, located somewhere in "
        "%s, or in the surrounding wilderness.  We need you to terminate them with extreme predjudice. They "
        "will likely not be alone, so be prepared to fight multiple enemies.  When the deed is done, "
        "return to me for your pay.  Take note: this mission is of %s difficulty. Your target will be "
        "%s and the guards will be %s.'\r\n",
        faction_names_lwr[faction],
        AN(mission_targets[mission_details_to_faction(faction)]),
        mission_targets[mission_details_to_faction(faction)],
        random_npc_names[GET_MISSION_NPC_NAME_NUM(ch)],
        mission_details[mDet][MISSION_ZONE_NAME],
//        mission_details[mDet][MISSION_PLANET], 
        mission_difficulty[difficulty],
        target_difficulty[difficulty], guard_difficulty[difficulty]);
    act(buf, FALSE, ch, 0, (struct char_data *) me, TO_CHAR);

    send_to_char(
        ch,
        "Your reward will be %ld gold coins, %ld %s standing, %ld quest points and %ld experience points.\r\n\tn",
        GET_MISSION_CREDITS(ch), GET_MISSION_STANDING(ch),
        faction_names_lwr[GET_MISSION_FACTION(ch)], GET_MISSION_REP(ch),
        GET_MISSION_EXP(ch));


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
        for (i = 0; i < NUM_MISSION_DIFFICULTIES; i++)
            send_to_char(
                ch,
                "-- %11s      - challenge level %d: %s target and %s guards\r\n",
                mission_difficulty[i], i + 1, target_difficulty[i],
                guard_difficulty[i]);
        return;
    }
    send_to_char(
        ch,
        "Your mission is to terminate %s %s named %s, located somewhere in %s or the surrounding wilderness.  Your reward will be %ld gold coins, %ld %s standing, %ld quest points and %ld experience points.\r\n",
        AN(mission_targets[mission_details_to_faction(
            GET_MISSION_FACTION(ch))]),
        mission_targets[mission_details_to_faction(GET_MISSION_FACTION(ch))],
        random_npc_names[GET_MISSION_NPC_NAME_NUM(ch)],
        mission_details[GET_CURRENT_MISSION(ch)][MISSION_ZONE_NAME],
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
        reward *= 5 + (mult*2);
        break;
    case MISSION_STANDING:
        reward = (int) (50 * mult);
        break;
    case MISSION_REP:
        reward = (int) (level * MAX(1, level / 2) * mult / 4);
        break;
    case MISSION_EXP:
        reward = (int) (level * MAX(1, level / 4) * mult * 300);
        break;
    }

    if (reward_type != MISSION_CREDITS)
      reward = (int) (reward * 0.25); // we'll adjust this based on what the cooldown is

    int bonus = dice(1, 20) + compute_ability(ch, ABILITY_DIPLOMACY);
    reward = (reward * (100+bonus)) / 100;

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
            if (IN_ROOM(mob) != NOWHERE) {
//              char_from_room(mob);
              extract_char(mob);
            }
        }
    }
}

void increase_mob_difficulty(struct char_data *mob, int difficulty)
{
    switch (difficulty) {
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

void create_mission_mobs(char_data *ch)
{
    struct char_data *mob = NULL;
    struct char_data *leader = NULL;
    int i = 0, randName = 0;
    room_vnum to_room = 0;
    if (GET_CURRENT_MISSION(ch) > 0)
        to_room = atoi(mission_details[GET_CURRENT_MISSION(ch)][6]);
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
        } else {
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
            if (i == 0) {
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

bool are_mission_mobs_loaded(char_data *ch)
{
    struct char_data *mob = NULL;

    // we return true because they aren't on a mission
    // and this function is used in limits.c on a 6 second
    // pulse to see if mission awards should be applied
    if (GET_CURRENT_MISSION(ch) <= 0)
        return true;

    for (mob = character_list; mob; mob = mob->next)
    {
        if (!IS_NPC(mob)) {
            continue;
	}
        if (mob->dead) {
            continue;
        }
        if (GET_IDNUM(ch) == mob->mission_owner) {
            return true;
        }
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
        faction_names_lwr[GET_MISSION_FACTION(ch)]);
    GET_FACTION_STANDING(ch, GET_MISSION_FACTION(ch)) += GET_MISSION_STANDING(ch);

    send_to_char(ch, "You've received %ld quest points.\r\n",
        GET_MISSION_REP(ch));
    GET_QUESTPOINTS(ch) += GET_MISSION_REP(ch);

    send_to_char(ch,
        "You have received %ld gold coins.\r\n", GET_MISSION_CREDITS(ch));
    GET_GOLD(ch) += GET_MISSION_CREDITS(ch);

    send_to_char(
        ch,
        "You have earned %d experience points for completing your mission.\r\n", gain_exp(ch, GET_MISSION_EXP(ch), GAIN_EXP_MODE_QUEST));

    send_to_char(ch, "\r\n");

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

    if (!mob->mission_owner) return true;

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
