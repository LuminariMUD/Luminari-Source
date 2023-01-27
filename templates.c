/*
 * Template system by Gicker aka Stephen Squires for LuminariMUD 2020
 **/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "modify.h"
#include "screen.h"
#include "spells.h"
#include "handler.h"
#include "interpreter.h"
#include "class.h"
#include "race.h"
#include "spec_procs.h" // for compute_ability
#include "mud_event.h"  // for purgemob event
#include "feats.h"
#include "spec_abilities.h"
#include "assign_wpn_armor.h"
#include "wilderness.h"
#include "domains_schools.h"
#include "constants.h"
#include "dg_scripts.h"
#include "templates.h"
#include "mysql.h"
#include "oasis.h"

extern MYSQL *conn;

void gain_template_level(struct char_data *ch, int t_type, int level)
{
    MYSQL_RES *res = NULL;
    MYSQL_ROW row = NULL;

    LEVELUP(ch) = NULL;
    CREATE(LEVELUP(ch), struct level_data, 1);

    char query[1000];
    struct descriptor_data *d = ch->desc;
    long int level_id = -1;

    // Check the connection, reconnect if necessary.
    mysql_ping(conn);

    snprintf(query, sizeof(query),
             "SELECT level_id, class_number FROM player_levelups WHERE character_name='%s' AND level_number='%d'",
             template_db_names[t_type], level);
    if (mysql_query(conn, query))
        log("%s", query);
    res = mysql_use_result(conn);
    if (res != NULL)
    {
        if ((row = mysql_fetch_row(res)) != NULL)
        {
            level_id = atol(row[0]);
            log("ERROR: %s", query);
            LEVELUP(ch)->class = atoi(row[1]);
        }
    }

    LEVELUP(ch)->level = level;

    mysql_free_result(res);

    snprintf(query, sizeof(query),
             "SELECT skill_num, skill_ranks FROM player_levelup_skills WHERE level_id='%ld'",
             level_id);
    if (mysql_query(conn, query))
        log("%s", query);
    res = mysql_use_result(conn);
    if (res != NULL)
    {
        while ((row = mysql_fetch_row(res)) != NULL)
        {
            LEVELUP(ch)->skills[atoi(row[0])] += atoi(row[1]);
        }
    }

    mysql_free_result(res);

    snprintf(query, sizeof(query),
             "SELECT feat_num, subfeat FROM player_levelup_feats WHERE level_id='%ld'",
             level_id);
    if (mysql_query(conn, query))
        log("%s", query);
    res = mysql_use_result(conn);
    if (res != NULL)
    {
        while ((row = mysql_fetch_row(res)) != NULL)
        {
            LEVELUP(ch)->feats[atoi(row[0])] = 1;
            switch (atoi(row[0]))
            {
            case FEAT_IMPROVED_CRITICAL:
            case FEAT_WEAPON_FINESSE:
            case FEAT_WEAPON_FOCUS:
            case FEAT_WEAPON_SPECIALIZATION:
            case FEAT_GREATER_WEAPON_FOCUS:
            case FEAT_GREATER_WEAPON_SPECIALIZATION:
            case FEAT_IMPROVED_WEAPON_FINESSE:
            case FEAT_EXOTIC_WEAPON_PROFICIENCY:
            case FEAT_MONKEY_GRIP:
            case FEAT_POWER_CRITICAL:
            case FEAT_WEAPON_MASTERY:
            case FEAT_WEAPON_FLURRY:
            case FEAT_WEAPON_SUPREMACY:
                LEVELUP(ch)->feat_weapons[atoi(row[0])] = atoi(row[1]);
                break;

            case FEAT_SKILL_FOCUS:
            case FEAT_EPIC_SKILL_FOCUS:
                LEVELUP(ch)->feat_skills[atoi(row[0])] = atoi(row[1]);
                break;
            }
        }
    }

    mysql_free_result(res);

    snprintf(query, sizeof(query),
             "SELECT ability_score FROM player_levelup_ability_scores WHERE level_id='%ld'",
             level_id);
    if (mysql_query(conn, query))
        log("%s", query);
    res = mysql_use_result(conn);
    if (res != NULL)
    {
        while ((row = mysql_fetch_row(res)) != NULL)
        {
            LEVELUP(ch)->boosts[atoi(row[0])] = 1;
        }
    }

    mysql_free_result(res);

    // Let's apply the changes

    send_to_char(
        ch,
        "@YYou have gained a level as in the %s class using the %s template build.\r\n\r\n",
        class_names[LEVELUP(ch)->class],
        template_types[GET_TEMPLATE(ch)]);
    /* our function for leveling up, takes in class that is being advanced */
    advance_level(ch, LEVELUP(ch)->class);
    finalize_study(d);
    save_char(d->character, 0);
    cleanup_olc(d, CLEANUP_ALL);
    free(LEVELUP(d->character));
    LEVELUP(d->character) = NULL;
    send_to_char(ch, "@n");

    mysql_close(conn);
}

SPECIAL(select_templates)
{
    if (!CMD_IS("settemplate") && !CMD_IS("north"))
        return false;

    if (CMD_IS("north"))
    {
        if (GET_TEMPLATE(ch) == 0)
        {
            send_to_char(ch, "You cannot proceed without choosing a template. If you wish to do a custom build, type recall.\r\n");
            return true;
        }

        return false;
    }

    skip_spaces(&argument);

    if (is_abbrev(argument, "champion"))
    {
        set_template(ch, TEMPLATE_CHAMPION);
    }
    else
    {
        send_to_char(
            ch,
            "That is not an available template. Please type @Ylisttemplates@n to see which templates are available, and select again.\r\n");
        return true;
    }

    return true;
}

void set_template(struct char_data *ch, int template_type)
{
    bool template_set = true;

    switch (template_type)
    {
    case TEMPLATE_CHAMPION:
        ch->real_abils.str = 14;
        ch->real_abils.dex = 13;
        ch->real_abils.con = 10;
        ch->real_abils.intel = 13;
        ch->real_abils.wis = 16;
        ch->real_abils.cha = 13;
        GET_TEMPLATE(ch) = template_type;
        send_to_char(ch, "You have been set to use the %s template type.\r\n",
                     template_types[template_type]);
        break;

    default:
        template_set = false;
        break;
    }

    if (template_set)
    {
        //        GET_STAT_POINTS(ch) = get_race_bonus_stat_points(GET_RACE(ch));
    }
}

void show_level_history(struct char_data *ch, int level)
{
    MYSQL_RES *res = NULL;
    MYSQL_ROW row = NULL;

    char query[1000];
    long int level_id = -1;

    mysql_ping(conn);

    snprintf(query, sizeof(query),
             "SELECT * FROM player_levels WHERE char_name=\"%s\" AND char_level='%d'",
             GET_NAME(ch), level);
    if (mysql_query(conn, query))
        log("%s", query);
    res = mysql_use_result(conn);
    if (res != NULL)
    {
        if ((row = mysql_fetch_row(res)) != NULL)
        {
            level_id = atol(row[0]);
            send_to_char(ch, "@uLevel:@n %2d @uClass:@n %s\r\n", level, row[4]);
        }
    }
    mysql_free_result(res);

    char buf[2000];
    int feat_num = 0, sub_feat = 0, num_found = 0;
    snprintf(buf, sizeof(buf), "@uFeats:@n ");
    snprintf(query, sizeof(query), "SELECT * FROM player_levelup_feats WHERE level_id='%ld'",
             level_id);
    if (mysql_query(conn, query))
        log("%s", query);
    res = mysql_use_result(conn);
    if (res != NULL)
    {
        while ((row = mysql_fetch_row(res)) != NULL)
        {
            feat_num = atoi(row[2]);
            sub_feat = atoi(row[3]);

            if (num_found > 0)
                strlcat(buf, ", ", sizeof(buf));

            strlcat(buf, feat_list[feat_num].name, sizeof(buf));

            switch (feat_num)
            {
            case FEAT_IMPROVED_CRITICAL:
            case FEAT_WEAPON_FINESSE:
            case FEAT_WEAPON_FOCUS:
            case FEAT_WEAPON_SPECIALIZATION:
            case FEAT_GREATER_WEAPON_FOCUS:
            case FEAT_GREATER_WEAPON_SPECIALIZATION:
            case FEAT_IMPROVED_WEAPON_FINESSE:
            case FEAT_EXOTIC_WEAPON_PROFICIENCY:
            case FEAT_MONKEY_GRIP:
            case FEAT_POWER_CRITICAL:
            case FEAT_WEAPON_MASTERY:
            case FEAT_WEAPON_FLURRY:
            case FEAT_WEAPON_SUPREMACY:
            {
                char res_buf[128];
                snprintf(res_buf, sizeof(res_buf), " (%s)", weapon_family[sub_feat]);
                strlcat(buf, res_buf, sizeof(buf));
                break;
            }
            case FEAT_SKILL_FOCUS:
            case FEAT_EPIC_SKILL_FOCUS:
                sprintf(buf + strlen(buf), " (%s)", spell_info[sub_feat].name);
                break;
            }
            num_found++;
        }
    }

    mysql_free_result(res);

    int len = 0;
    char *temp = NULL;

    temp = strtok(buf, " ");
    while (temp != NULL)
    {
        send_to_char(ch, "%s ", temp);
        len += strlen(temp);
        if (strstr(temp, "\n"))
            len = 0;
        if (len > 70)
        {
            len = 0;
            send_to_char(ch, "\r\n");
        }
        temp = strtok(NULL, " ");
    }
    if (len != 0)
        send_to_char(ch, "\r\n");

    mysql_close(conn);
}

ACMD(do_templates)
{
    skip_spaces_c(&argument);

    // we're going to use the levelinfo code to let them see what they'll get from their template
    if (*argument)
    {

        char arg1[100], arg2[100];

        two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

        if (!*arg1)
        {
            send_to_char(ch, "That is not a known template type.  Please type @Y'templates'@n for a list.\r\n"
                             "You may need to replace spaces with a dash.  Eg: templates champion.\r\n");
            return;
        }

        if (!*arg2)
        {
            send_to_char(ch, "You need to specify a level to view between 1 and %d.\r\n", LVL_IMMORT - 1);
            return;
        }

        int level_num = atoi(arg2);

        if (level_num < 1 || level_num >= (LVL_IMMORT - 1))
        {
            send_to_char(ch, "You need to specify a level to view between 1 and %d.\r\n", (LVL_IMMORT - 1));
            return;
        }

        int ttype = 0;

        if (is_abbrev(arg1, "champion"))
        {
            ttype = TEMPLATE_CHAMPION;
        }
        else
        {
            send_to_char(ch, "That is not a known template type.  Please type @Y'templates'@n for a list.\r\n"
                             "You may need to replace spaces with a dash.  Eg: templates champion.\r\n");
            return;
        }

        if (ttype == 0)
        {
            send_to_char(ch, "That is not a known template type.  Please type @Y'templates'@n for a list.\r\n"
                             "You may need to replace spaces with a dash.  Eg: templates champion.\r\n");
            return;
        }

        // we need to get the level_id in the db first
        char *chname = strdup(template_db_names[ttype]);
        level_num = get_level_id_by_level_num(level_num, chname);
        free(chname);

        // Now we can find the level info from our new level_num/id
        chname = strdup(template_db_names[ttype]);
        show_levelinfo_for_specific_level(ch, level_num, chname);
        free(chname);

        return;

    } // end has argument

    send_to_char(
        ch,
        "The following templates are currently available:\r\n"
        "\r\n"
        "crusader            : A skilled warrior wielding two handed blades and heavy armor.\r\n"
        "\r\n");
}

long get_level_id_by_level_num(int level_num, char *chname)
{

    if (level_num < 1 || level_num >= (LVL_IMMORT - 1))
    {
        return 0;
    }

    if (!*chname)
        return 0;

    char query[MAX_INPUT_LENGTH] = {'\0'};

    MYSQL_RES *res = NULL;
    MYSQL_ROW row = NULL;
    long level_id = 0;

    mysql_ping(conn);

    snprintf(query, sizeof(query), "SELECT level_id from player_levelups WHERE character_name='%s' AND level_number='%d'", chname, level_num);
    mysql_query(conn, query);
    res = mysql_use_result(conn);
    if (res != NULL)
    {
        if ((row = mysql_fetch_row(res)) != NULL)
        {
            level_id = atol(row[0]);
        }
    }

    mysql_free_result(res);

    mysql_close(conn);

    return level_id;
}

void show_levelinfo_for_specific_level(struct char_data *ch, long level_id, char *chname)
{

    MYSQL_RES *res = NULL;
    MYSQL_ROW row = NULL;
    int level_num = 0, class_number = 0, i = 0;
    char query[500];

    char display_name[100], template_name[100];

    snprintf(display_name, sizeof(display_name), "%s", chname);

    for (i = 0; i < NUM_TEMPLATES; i++)
    {
        snprintf(template_name, sizeof(template_name), "%s", template_db_names[i]);
        if (!strcmp(template_name, chname))
        {
            snprintf(display_name, sizeof(display_name), "%s", template_types_capped[i]);
            break;
        }
    }

    mysql_ping(conn);

    snprintf(query, sizeof(query),
             "SELECT level_number, class_number FROM player_levelups WHERE level_id='%ld' AND character_name='%s'",
             level_id, chname);

    if (mysql_query(conn, query))
        log("NO_LEVEL_INFO QUERY: %s\n", query);
    res = mysql_use_result(conn);
    if (res != NULL)
    {
        if ((row = mysql_fetch_row(res)) != NULL)
        {
            level_num = atoi(row[0]);
            class_number = atoi(row[1]);
        }
    }
    mysql_free_result(res);

    if (level_num == 0)
    {
        send_to_char(
            ch,
            "We're sorry, but there is no level information for your character %s with level_id %d.\r\nQ: %s\r\n", chname, level_num, query);
        send_to_char(ch, "\r\n%s\r\n", LEVELINFO_SYNTAX);
        mysql_close(conn);
        return;
    }

    send_to_char(
        ch,
        "Level Information for '%s', for level '%d', having taken class '%s' and with level_id '%ld'\r\n",
        display_name, level_num,
        class_names[class_number], level_id);
    send_to_char(
        ch,
        "--------------------------------------------------------------------------------\r\n");

    send_to_char(ch, "@5@oFEATS@n\r\n");
    snprintf(query, sizeof(query),
             "SELECT feat_num, subfeat FROM player_levelup_feats WHERE level_id='%ld'",
             level_id);
    mysql_query(conn, query);
    res = mysql_use_result(conn);
    if (res != NULL)
    {
        while ((row = mysql_fetch_row(res)) != NULL)
        {
            display_levelinfo_feats(ch, atoi(row[0]), atoi(row[1]));
        }
    }
    mysql_free_result(res);
    send_to_char(ch, "\r\n");

    send_to_char(ch, "@5@oSKILLS@n\r\n");
    snprintf(query, sizeof(query),
             "SELECT skill_num, skill_ranks FROM player_levelup_skills WHERE level_id='%ld'",
             level_id);
    mysql_query(conn, query);
    res = mysql_use_result(conn);
    if (res != NULL)
    {
        while ((row = mysql_fetch_row(res)) != NULL)
        {
            if (atoi(row[0]) < START_GENERAL_ABILITIES)
                continue;
            send_to_char(ch, "%-30s +%s ranks\r\n",
                         spell_info[atoi(row[0])].name, row[1]);
        }
    }
    mysql_free_result(res);
    send_to_char(ch, "\r\n");

    send_to_char(ch, "@5@oLANGUAGES@n\r\n");
    snprintf(query, sizeof(query),
             "SELECT skill_num FROM player_levelup_skills WHERE level_id='%ld'",
             level_id);
    mysql_query(conn, query);
    res = mysql_use_result(conn);
    if (res != NULL)
    {
        while ((row = mysql_fetch_row(res)) != NULL)
        {
            if (atoi(row[0]) > MAX_LANGUAGES)
                continue;
            send_to_char(ch, "%-30s learned\r\n",
                         spell_info[atoi(row[0])].name);
        }
    }
    mysql_free_result(res);
    send_to_char(ch, "\r\n");

    send_to_char(ch, "@5@oABILITY SCORES@n\r\n");
    snprintf(query, sizeof(query),
             "SELECT ability_score FROM player_levelup_ability_scores WHERE level_id='%ld'",
             level_id);
    mysql_query(conn, query);
    res = mysql_use_result(conn);
    if (res != NULL)
    {
        while ((row = mysql_fetch_row(res)) != NULL)
        {
            display_levelinfo_ability_scores(ch, atoi(row[0]));
        }
    }
    mysql_free_result(res);
    send_to_char(ch, "\r\n");

    mysql_close(conn);
}

ACMD(do_levelinfo)
{
    MYSQL_RES *res = NULL;
    MYSQL_ROW row = NULL;
    char query[500];
    long long int level_id = 0;

    char arg1[200], arg2[700], arg3[200], arg4[500];

    half_chop_c(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

    mysql_ping(conn);

    if (!*arg1)
    {
        send_to_char(ch, "Level Information for %s\r\n", GET_NAME(ch));
        send_to_char(
            ch,
            "--------------------------------------------------------------------------------\r\n");
        send_to_char(ch, "%-20s %-14s %s\r\n", "Level ID", "Level Number",
                     "Class Taken");
        send_to_char(
            ch,
            "--------------------------------------------------------------------------------\r\n");

        snprintf(query, sizeof(query),
                 "SELECT level_id, level_number, class_number FROM player_levelups WHERE character_name='%s' ORDER BY level_number DESC",
                 GET_NAME(ch));
        mysql_query(conn, query);
        res = mysql_use_result(conn);
        if (res != NULL)
        {
            while ((row = mysql_fetch_row(res)) != NULL)
            {
                send_to_char(ch, "%-20s %-14s %s\r\n", row[0], row[1],
                             class_names[atoi(row[2])]);
            }
        }

        mysql_free_result(res);
        send_to_char(ch, "\r\n%s\r\n", LEVELINFO_SYNTAX);
    }
    else
    {
        if (is_abbrev(arg1, "search"))
        {
            if (!*arg2)
            {
                send_to_char(
                    ch,
                    "You need to specify whether you are searching for a feat, skill (or language), or an ability score,\r\n"
                    "As well as which feat/skill/language/ability score you are searching for.\r\n");
                send_to_char(ch, "\r\n%s\r\n", LEVELINFO_SYNTAX);
                mysql_close(conn);
                return;
            }
            half_chop(arg2, arg3, arg4);

            if (!*arg3)
            {
                send_to_char(
                    ch,
                    "You need to specify whether you are searching for a feat, skill, language, or an ability score.\r\n");
                send_to_char(ch, "\r\n%s\r\n", LEVELINFO_SYNTAX);
                mysql_close(conn);
                return;
            }
            if (!*arg4)
            {
                send_to_char(
                    ch,
                    "You need to specify which feat/skill/language/ability score you are searching for.\r\n");
                send_to_char(ch, "\r\n%s\r\n", LEVELINFO_SYNTAX);
                mysql_close(conn);
                return;
            }

            if (is_abbrev(arg3, "feat"))
            {
                levelinfo_search(ch, 1, strdup(arg4));
            }
            else if (is_abbrev(arg3, "skill"))
            {
                levelinfo_search(ch, 2, strdup(arg4));
            }
            else if (is_abbrev(arg3, "language"))
            {
                levelinfo_search(ch, 3, strdup(arg4));
            }
            else if (is_abbrev(arg3, "ability score"))
            {
                levelinfo_search(ch, 4, strdup(arg4));
            }
            else
            {
                send_to_char(
                    ch,
                    "You need to specify whether you are searching for a feat, skill (or language), or an ability score.\r\n");
                send_to_char(ch, "\r\n%s\r\n", LEVELINFO_SYNTAX);
                mysql_close(conn);
                return;
            }
        }
        else if ((level_id = atol(arg1)) > 0)
        {
            char *chname = strdup(GET_NAME(ch));
            show_levelinfo_for_specific_level(ch, level_id, chname);
            free(chname);
        }
        else
        {
            send_to_char(ch, "%s", LEVELINFO_SYNTAX);
            if (conn)
                mysql_close(conn);
            return;
        }
    }
    mysql_close(conn);
}

void display_levelinfo_feats(struct char_data *ch, int feat_num, int subfeat)
{
    if (feat_num < 0 || feat_num >= FEAT_LAST_FEAT)
        return;

    if (!ch)
        return;

    switch (feat_num)
    {
    case FEAT_IMPROVED_CRITICAL:
    case FEAT_WEAPON_FINESSE:
    case FEAT_WEAPON_FOCUS:
    case FEAT_WEAPON_SPECIALIZATION:
    case FEAT_GREATER_WEAPON_FOCUS:
    case FEAT_GREATER_WEAPON_SPECIALIZATION:
    case FEAT_IMPROVED_WEAPON_FINESSE:
    case FEAT_EXOTIC_WEAPON_PROFICIENCY:
    case FEAT_MONKEY_GRIP:
    case FEAT_POWER_CRITICAL:
    case FEAT_WEAPON_MASTERY:
    case FEAT_WEAPON_FLURRY:
    case FEAT_WEAPON_SUPREMACY:
        send_to_char(
            ch, "%-30s (%s)", feat_list[feat_num].name,
            weapon_damage_types[subfeat]);
        break;
    case FEAT_SKILL_FOCUS:
    case FEAT_EPIC_SKILL_FOCUS:
        send_to_char(ch, "%-30s (%s)", feat_list[feat_num].name,
                     spell_info[subfeat].name);
        break;
    default:
        send_to_char(ch, "%-30s", feat_list[feat_num].name);
        break;
    }
    send_to_char(ch, "\r\n");
}

void display_levelinfo_ability_scores(struct char_data *ch, int ability_score)
{
    if (!ch)
        return;

    switch (ability_score)
    {
    case 0:
        send_to_char(ch, "%-30s increased by one\r\n", "Strength");
        break;
    case 1:
        send_to_char(ch, "%-30s increased by one\r\n", "Dexterity");
        break;
    case 2:
        send_to_char(ch, "%-30s increased by one\r\n", "Constitution");
        break;
    case 3:
        send_to_char(ch, "%-30s increased by one\r\n", "Intelligence");
        break;
    case 4:
        send_to_char(ch, "%-30s increased by one\r\n", "Wisdom");
        break;
    case 5:
        send_to_char(ch, "%-30s increased by one\r\n", "Charisma");
        break;
    }
}

void levelinfo_search(struct char_data *ch, int type, char *searchString)
{
    char query[MAX_INPUT_LENGTH] = {'\0'};

    int i = 0;

    mysql_ping(conn);

    MYSQL_RES *res = NULL;
    MYSQL_ROW row = NULL;

    if (type == 1)
    { // feats

        for (i = 0; i < NUM_FEATS; i++)
        {
            if (!strcmp(feat_list[i].name, "Unused Feat"))
                continue;
            if (is_abbrev(searchString, feat_list[i].name))
                break;
        }

        if (i >= NUM_FEATS)
        {
            send_to_char(
                ch,
                "We are sorry, but there is no feat by that name. Please try again.\r\n");
            mysql_close(conn);
            return;
        }

        send_to_char(
            ch, "@5@oLEVELUPS WHEREIN THE FOLLOWING FEAT WAS SELECTED: %s@n\r\n",
            feat_list[i].name);

        snprintf(query, sizeof(query),
                 "SELECT a.feat_num, a.subfeat, b.level_id, b.level_number FROM player_levelup_feats a LEFT JOIN player_levelups b ON a.level_id=b.level_id WHERE b.character_name='%s' AND a.feat_num='%d' ORDER BY b.level_number DESC",
                 GET_NAME(ch), i);
        // log("%s", query);
        mysql_query(conn, query);
        res = mysql_use_result(conn);
        if (res != NULL)
        {
            while ((row = mysql_fetch_row(res)) != NULL)
            {
                send_to_char(ch, "Level ID = %s, Level = %s, ", row[2], row[3]);
                display_levelinfo_feats(ch, atoi(row[0]), atoi(row[1]));
            }
        }

        mysql_free_result(res);
    }
    else if (type == 2)
    { // skills and languages

        for (i = START_GENERAL_ABILITIES; i <= END_GENERAL_ABILITIES; i++)
        {
            if (spell_info[i].min_position == POS_DEAD)
                continue;
            if (is_abbrev(searchString, spell_info[i].name))
                break;
        }

        if (i < START_GENERAL_ABILITIES || i > END_GENERAL_ABILITIES)
        {
            send_to_char(
                ch,
                "We are sorry, but there is no skill by that name. Please try again.\r\n");
            mysql_close(conn);
            return;
        }

        send_to_char(
            ch, "@5@oLEVELUPS WHEREIN THE FOLLOWING SKILL WAS SELECTED: %s@n\r\n",
            spell_info[i].name);

        snprintf(query, sizeof(query),
                 "SELECT a.skill_num, a.skill_ranks, b.level_id, b.level_number FROM player_levelup_skills a LEFT JOIN player_levelups b ON a.level_id=b.level_id WHERE b.character_name='%s' AND a.skill_num='%d' ORDER BY b.level_number DESC",
                 GET_NAME(ch), i);
        // log("%s", query);
        mysql_query(conn, query);
        res = mysql_use_result(conn);
        if (res != NULL)
        {
            while ((row = mysql_fetch_row(res)) != NULL)
            {
                if (atoi(row[0]) < START_GENERAL_ABILITIES)
                    continue;
                send_to_char(ch, "Level ID = %s, Level = %s, ", row[2], row[3]);
                send_to_char(ch, "%-30s +%s ranks\r\n",
                             spell_info[atoi(row[0])].name, row[1]);
            }
        }

        mysql_free_result(res);
    }
    else if (type == 3)
    { // languages

        for (i = MIN_LANGUAGES; i <= MAX_LANGUAGES; i++)
        {
            if (spell_info[i].min_position == POS_DEAD)
                continue;
            if (is_abbrev(searchString, spell_info[i].name))
                break;
        }

        if (i < MIN_LANGUAGES || i > MAX_LANGUAGES)
        {
            send_to_char(
                ch,
                "Were sorry, but there is no language by that name. Please try again.\r\n");
            mysql_close(conn);
            return;
        }

        send_to_char(
            ch,
            "@5@oLEVELUPS WHEREIN THE FOLLOWING LANGUAGE WAS SELECTED: %s@n\r\n",
            spell_info[i].name);

        snprintf(query, sizeof(query),
                 "SELECT a.skill_num, a.skill_ranks, b.level_id, b.level_number FROM player_levelup_skills a LEFT JOIN player_levelups b ON a.level_id=b.level_id WHERE b.character_name='%s' AND a.skill_num='%d' ORDER BY b.level_number DESC",
                 GET_NAME(ch), i);
        // log("%s", query);
        mysql_query(conn, query);
        res = mysql_use_result(conn);
        if (res != NULL)
        {
            while ((row = mysql_fetch_row(res)) != NULL)
            {
                if (atoi(row[0]) > MAX_LANGUAGES)
                    continue;
                send_to_char(ch, "Level ID = %s, Level = %s, ", row[2], row[3]);
                send_to_char(ch, "%-30s learned\r\n",
                             spell_info[atoi(row[0])].name);
            }
        }

        mysql_free_result(res);
    }
    else if (type == 4)
    { // ability scores

        for (i = 0; i < 6; i++)
        {
            if (is_abbrev(searchString, levelup_ability_scores[i]))
                break;
        }

        if (i < 0 || i > 6)
        {
            send_to_char(
                ch,
                "We are sorry, but there is no ability score by that name.\r\n");
            mysql_close(conn);
            return;
        }

        send_to_char(
            ch,
            "@5@oLEVELUPS WHEREIN THE FOLLOWING ABILITY SCORE WAS RAISED: %s@n\r\n",
            levelup_ability_scores[i]);

        snprintf(query, sizeof(query),
                 "SELECT a.ability_score, b.level_id, b.level_number FROM player_levelup_ability_scores a LEFT JOIN player_levelups b ON a.level_id=b.level_id WHERE b.character_name='%s' AND a.ability_score='%d' ORDER BY b.level_number DESC",
                 GET_NAME(ch), i);
        // log("%s", query);
        mysql_query(conn, query);
        res = mysql_use_result(conn);
        if (res != NULL)
        {
            while ((row = mysql_fetch_row(res)) != NULL)
            {
                if (atoi(row[0]) > MAX_LANGUAGES)
                    continue;
                send_to_char(ch, "Level ID = %s, Level = %s, ", row[1], row[2]);
                send_to_char(ch, "%-30s raised by one\r\n",
                             levelup_ability_scores[i]);
            }
        }

        mysql_free_result(res);
    }

    mysql_close(conn);
}

void erase_levelup_info(struct char_data *ch)
{
    if (!ch)
        return;

    char query[MAX_INPUT_LENGTH] = {'\0'};

    MYSQL_RES *res = NULL;
    MYSQL_ROW row = NULL;

    mysql_ping(conn);

    int found = true;

    long long int level_id = 0;

    while (found)
    {
        found = false;
        level_id = 0;
        snprintf(query, sizeof(query),
                 "SELECT level_id FROM player_levelups WHERE character_name='%s' LIMIT 1",
                 GET_NAME(ch));
        //    log("%s", query);
        mysql_query(conn, query);
        res = mysql_use_result(conn);
        if (res != NULL)
        {
            if ((row = mysql_fetch_row(res)) != NULL)
            {
                found = true;
                level_id = atoi(row[0]);
            }
        }
        mysql_free_result(res);

        if (level_id > 0)
        {
            snprintf(query, sizeof(query), "DELETE FROM player_levelups WHERE level_id='%lld'",
                     level_id);
            //      log("%s", query);
            mysql_query(conn, query);
            snprintf(query, sizeof(query),
                     "DELETE FROM player_levelup_skills WHERE level_id='%lld'",
                     level_id);
            //      log("%s", query);
            mysql_query(conn, query);
            snprintf(query, sizeof(query),
                     "DELETE FROM player_levelup_feats WHERE level_id='%lld'",
                     level_id);
            //      log("%s", query);
            mysql_query(conn, query);
            snprintf(query, sizeof(query),
                     "DELETE FROM player_levelup_ability_scores WHERE level_id='%lld'",
                     level_id);
            //      log("%s", query);
            mysql_query(conn, query);
        }
    }
    mysql_close(conn);
}

const char *const template_types[NUM_TEMPLATES] = {
    "none",
    "champion",
};

const char *const template_types_capped[NUM_TEMPLATES] = {
    "None",
    "Champion",
};

const char *const template_db_names[NUM_TEMPLATES] = {
    "none",
    "Templatechampion",
};

const char *const levelup_ability_scores[6] = {
    "strength",
    "dexterity",
    "constitution",
    "intelligence",
    "wisdom",
    "charisma"};
