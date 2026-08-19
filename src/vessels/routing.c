/**
 * @file routing.c                LuminariMUD
 * Functions used to route Luminari travel options.
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "magic/spells.h"
#include "handler.h"
#include "interpreter.h"
#include "character/class.h"
#include "character/race.h"
#include "act.h"
#include "character/feats.h"
#include "constants.h"
#include "routing.h"
#include "transport.h"

const char *get_transport_zone_entrance_name(int locale, int type)
{
  if (type == TRAVEL_CARRIAGE || type == TRAVEL_OVERLAND_FLIGHT)
    return get_transport_carriage_name(locale);
  else if (type == TRAVEL_SAILING || type == TRAVEL_OVERLAND_FLIGHT_SAIL)
    return sailing_locales_lumi[locale][0];

  return "Unknown Transport";
}

const char *get_transport_carriage_name(int locale)
{
  return carriage_locales_lumi[locale][0];
}

const char *get_transport_sailing_name(int locale)
{
  return sailing_locales_lumi[locale][0];
}

int get_carriage_locale_vnum(int locale)
{
  return atoi(carriage_locales_lumi[locale][1]);
}

int get_sailing_locale_vnum(int locale)
{
  return atoi(sailing_locales_lumi[locale][1]);
}

int get_sailing_locale_x(int locale)
{
  return atoi(sailing_locales_lumi[locale][5]);
}

int get_sailing_locale_y(int locale)
{
  return atoi(sailing_locales_lumi[locale][6]);
}

const char *get_carriage_locale_region(int locale)
{
  return carriage_locales_lumi[locale][3];
}

int get_carriage_locale_cost(int locale)
{
  return atoi(carriage_locales_lumi[locale][2]);
}

const char *get_carriage_locale_notes(int locale)
{
  return carriage_locales_lumi[locale][4];
}

int get_carriage_locale_x(int locale)
{
  return atoi(carriage_locales_lumi[locale][5]);
}

int get_carriage_locale_y(int locale)
{
  return atoi(carriage_locales_lumi[locale][6]);
}

int get_sailing_locale_cost(int locale)
{
  return atoi(sailing_locales_lumi[locale][2]);
}

const char *get_sailing_locale_notes(int locale)
{
  return sailing_locales_lumi[locale][4];
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
    send_to_char(
        ch, "Please specify a valid area you'd like to fly to.  Type flightlist for a list.\r\n");
    return;
  }

  send_to_char(ch, "You begin flying to %s.\r\n", get_transport_carriage_name(i));

  enter_transport(ch, i, TRAVEL_OVERLAND_FLIGHT, GET_ROOM_VNUM(IN_ROOM(ch)));
}

int get_walkto_landmark_vnum(int locale)
{
  return atoi(walkto_landmarks_lumi[locale][1]);
}

const char *get_walkto_landmark_region(int locale)
{
  return walkto_landmarks_lumi[locale][0];
}

const char *get_walkto_landmark_name(int locale)
{
  return walkto_landmarks_lumi[locale][2];
}

const char *get_walkto_landmark_notes(int locale)
{
  return walkto_landmarks_lumi[locale][3];
}
