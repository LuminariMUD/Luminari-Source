/**
 * @file routing.c                LuminariMUD
 * Functions used to route mud options to the proper campaign or
 * cedit option.
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "interpreter.h"
#include "class.h"
#include "race.h"
#include "act.h"
#include "feats.h"
#include "constants.h"
#include "routing.h"
#include "transport.h"

const char * get_transport_zone_entrance_name(int locale, int type)
{
    if (IS_CAMPAIGN_DL)
    {
        if (type == TRAVEL_CARRIAGE)
            return get_transport_carriage_name(locale);
        else if (type == TRAVEL_SAILING)
            return sailing_locales_dl[locale][0];
        else if (type == TRAVEL_OVERLAND_FLIGHT)
            return get_transport_carriage_name(locale);
        else if (type == TRAVEL_OVERLAND_FLIGHT_SAIL)
            return sailing_locales_dl[locale][0];
    }
    else if (IS_CAMPAIGN_FR)
    {
        return zone_entrances_fr[locale][0];
    }
    else if (IS_CAMPAIGN_LUMINARI)
    {
        if (type == TRAVEL_CARRIAGE)
            return get_transport_carriage_name(locale);
        else if (type == TRAVEL_SAILING)
            return sailing_locales_lumi[locale][0];
        else if (type == TRAVEL_OVERLAND_FLIGHT)
            return get_transport_carriage_name(locale);
        else if (type == TRAVEL_OVERLAND_FLIGHT_SAIL)
            return sailing_locales_lumi[locale][0];
    }
    return "Unknown Transport";
}

const char * get_transport_carriage_name(int locale)
{
    if (IS_CAMPAIGN_DL)
        return carriage_locales_dl[locale][0];
    else if (IS_CAMPAIGN_FR)
        return zone_entrances_fr[locale][0];
    else if (IS_CAMPAIGN_LUMINARI)
        return carriage_locales_lumi[locale][0];
    else
        return "Unknown Carriage Route";
}

const char * get_transport_sailing_name(int locale)
{
    if (IS_CAMPAIGN_DL)
        return sailing_locales_dl[locale][0];
    else if (IS_CAMPAIGN_FR)
        return zone_entrances_fr[locale][0];
    else if (IS_CAMPAIGN_LUMINARI)
        return sailing_locales_lumi[locale][0];
    else
        return "Unknown Sailing Route";
}


int get_carriage_locale_vnum(int locale)
{
    if (IS_CAMPAIGN_DL)
        return atoi(carriage_locales_dl[locale][1]);
    else if (IS_CAMPAIGN_FR)
        return atoi(zone_entrances_fr[locale][2]);
    else if (IS_CAMPAIGN_LUMINARI)
        return atoi(carriage_locales_lumi[locale][1]);
    else
        return 0; // Unknown Campaign
}

int get_sailing_locale_vnum(int locale)
{
    if (IS_CAMPAIGN_DL)
        return atoi(sailing_locales_dl[locale][1]);
    else if (IS_CAMPAIGN_FR)
        return atoi(zone_entrances_fr[locale][2]);
    else if (IS_CAMPAIGN_LUMINARI)
        return atoi(sailing_locales_lumi[locale][1]);
    else
        return 0; // Unknown Campaign
}

int get_sailing_locale_x(int locale)
{
    if (IS_CAMPAIGN_DL)
        return atoi(sailing_locales_dl[locale][5]);
    else if (IS_CAMPAIGN_FR)
        return 0; // FR does not use sailing locales currently
    else if (IS_CAMPAIGN_LUMINARI)
        return atoi(sailing_locales_lumi[locale][5]);
    else
        return 0; // Unknown Campaign
}

int get_sailing_locale_y(int locale)
{
    if (IS_CAMPAIGN_DL)
        return atoi(sailing_locales_dl[locale][6]);
    else if (IS_CAMPAIGN_FR)
        return 0; // FR does not use sailing locales currently
    else if (IS_CAMPAIGN_LUMINARI)
        return atoi(sailing_locales_lumi[locale][6]);
    else
        return 0; // Unknown Campaign
}

const char * get_carriage_locale_region(int locale)
{
    if (IS_CAMPAIGN_DL)
        return carriage_locales_dl[locale][3];
    else if (IS_CAMPAIGN_FR)
        return "";
    else if (IS_CAMPAIGN_LUMINARI)
        return carriage_locales_lumi[locale][3];
    else
        return "Unknown Region";
}

int get_carriage_locale_cost(int locale)
{
    if (IS_CAMPAIGN_DL)
        return atoi(carriage_locales_dl[locale][2]);
    else if (IS_CAMPAIGN_FR)
        return 0; // FR does not use carriage locales currently
    else if (IS_CAMPAIGN_LUMINARI)
        return atoi(carriage_locales_lumi[locale][2]);
    else
        return 0; // Unknown Campaign
}

const char * get_carriage_locale_notes(int locale)
{
    if (IS_CAMPAIGN_DL)
        return carriage_locales_dl[locale][4];
    else if (IS_CAMPAIGN_FR)
        return "";
    else if (IS_CAMPAIGN_LUMINARI)
        return carriage_locales_lumi[locale][4];
    else
        return "Unknown Notes";
}

int get_carriage_locale_x(int locale)
{
    if (IS_CAMPAIGN_DL)
        return atoi(carriage_locales_dl[locale][5]);
    else if (IS_CAMPAIGN_FR)
        return 0; // FR does not use carriage locales currently
    else if (IS_CAMPAIGN_LUMINARI)
        return atoi(carriage_locales_lumi[locale][5]);
    else
        return 0; // Unknown Campaign
}

int get_carriage_locale_y(int locale)
{
    if (IS_CAMPAIGN_DL)
        return atoi(carriage_locales_dl[locale][6]);
    else if (IS_CAMPAIGN_FR)
        return 0; // FR does not use carriage locales currently
    else if (IS_CAMPAIGN_LUMINARI)
        return atoi(carriage_locales_lumi[locale][6]);
    else
        return 0; // Unknown Campaign
}

int get_sailing_locale_cost(int locale)
{
    if (IS_CAMPAIGN_DL)
        return atoi(sailing_locales_dl[locale][2]);
    else if (IS_CAMPAIGN_FR)
        return 0; // FR does not use sailing locales currently
    else if (IS_CAMPAIGN_LUMINARI)
        return atoi(sailing_locales_lumi[locale][2]);
    else
        return 0; // Unknown Campaign
}

const char * get_sailing_locale_notes(int locale)
{
    if (IS_CAMPAIGN_DL)
        return sailing_locales_dl[locale][4];
    else if (IS_CAMPAIGN_FR)
        return "";
    else if (IS_CAMPAIGN_LUMINARI)
        return sailing_locales_lumi[locale][4];
    else
        return "Unknown Notes";
}

void start_flight_to_destination_luminari(struct char_data *ch, const char *zone)
{
    int i = 0;

    if (!ZONE_FLAGGED(world[IN_ROOM(ch)].zone, ZONE_WILDERNESS))
    {
        send_to_char(ch, "You can only use this spell in the wilderness.\r\n");
        return;
    }

    while (get_carriage_locale_vnum(i) != 0)
    {
        if (is_abbrev(zone, get_transport_carriage_name(i)))
            break;

        i++;
    }

    if (get_carriage_locale_vnum(i) == 0)
    {
        send_to_char(ch, "Please specify a valid area you'd like to fly to.  Type flightlist for a list.\r\n");
        return;
    }

    send_to_char(ch, "You begin flying to %s.\r\n", get_transport_carriage_name(i));

    enter_transport(ch, i, TRAVEL_OVERLAND_FLIGHT, GET_ROOM_VNUM(IN_ROOM(ch)));
}

void start_flight_to_zone_dl(struct char_data *ch, const char *zone)
{
    int i = 0;

    if (!OUTSIDE(ch))
    {
        send_to_char(ch, "That spell only functions outside.\r\n");
        return;
    }

    // Try carriage destinations first
    while (get_carriage_locale_vnum(i) != 0)
    {
        if (is_abbrev(zone, get_transport_carriage_name(i)))
            break;
        i++;
    }

    if (get_carriage_locale_vnum(i) != 0)
    {
        send_to_char(ch, "You begin flying to %s.\r\n", get_transport_carriage_name(i));
        enter_transport(ch, i, TRAVEL_OVERLAND_FLIGHT, GET_ROOM_VNUM(IN_ROOM(ch)));
        return;
    }

    // If not found, try sailing destinations
    i = 0;
    while (get_sailing_locale_vnum(i) != 0)
    {
        if (is_abbrev(zone, get_transport_sailing_name(i)))
            break;
        i++;
    }

    if (get_sailing_locale_vnum(i) != 0)
    {
        send_to_char(ch, "You begin flying to %s.\r\n", get_transport_sailing_name(i));
        enter_transport(ch, i, TRAVEL_OVERLAND_FLIGHT_SAIL, GET_ROOM_VNUM(IN_ROOM(ch)));
    }
    else
    {
        send_to_char(ch, "Please specify a valid area you'd like to fly to.  Type flightlist for a list.\r\n");
    }
}

void start_fr_flight_to_zone(struct char_data *ch, const char *zone)
{
    int i;

    if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_WORLDMAP))
    {
        send_to_char(ch, "You can only use this spell on the world map.\r\n");
        return;
    }

    for (i = 0; i < NUM_ZONE_ENTRANCES; i++)
    {
        if (is_abbrev(zone, zone_entrances_fr[i][0]))
            break;
    }

    if (i >= NUM_ZONE_ENTRANCES)
    {
        send_to_char(ch, "Please specify a valid area you'd like to fly to. Type flightlist for a list.\r\n");
        return;
    }

    send_to_char(ch, "You begin flying to %s.\r\n", zone_entrances_fr[i][0]);
    enter_transport(ch, i, TRAVEL_OVERLAND_FLIGHT, GET_ROOM_VNUM(IN_ROOM(ch)));
}

int get_walkto_landmark_vnum(int locale)
{
    if (IS_CAMPAIGN_DL)
        return atoi(walkto_landmarks_dl[locale][1]);
    else if (IS_CAMPAIGN_FR)
        return 0; // FR does not use walkto landmarks currently
    else if (IS_CAMPAIGN_LUMINARI)
        return atoi(walkto_landmarks_lumi[locale][1]);
    else
        return 0; // Unknown Campaign
}

const char * get_walkto_landmark_region(int locale)
{
    if (IS_CAMPAIGN_DL)
        return walkto_landmarks_dl[locale][0];
    else if (IS_CAMPAIGN_FR)
        return "";
    else if (IS_CAMPAIGN_LUMINARI)
        return walkto_landmarks_lumi[locale][0];
    else
        return "Unknown Region";
}

const char * get_walkto_landmark_name(int locale)
{
    if (IS_CAMPAIGN_DL)
        return walkto_landmarks_dl[locale][2];
    else if (IS_CAMPAIGN_FR)
        return "";
    else if (IS_CAMPAIGN_LUMINARI)
        return walkto_landmarks_lumi[locale][2];
    else
        return "Unknown Landmark";
}

const char * get_walkto_landmark_notes(int locale)
{
    if (IS_CAMPAIGN_DL)
        return walkto_landmarks_dl[locale][3];
    else if (IS_CAMPAIGN_FR)
        return "";
    else if (IS_CAMPAIGN_LUMINARI)
        return walkto_landmarks_lumi[locale][3];
    else
        return "Unknown Notes";
}