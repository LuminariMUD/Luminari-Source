/* ************************************************************************
 *    File:   transport.c                            Part of LuminariMUD  *
 * Purpose:   To provide auto travel functionality                        *
 *  Author:   Gicker                                                      *
 ************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "oasis.h"
#include "screen.h"
#include "interpreter.h"
#include "modify.h"
#include "spells.h"
#include "feats.h"
#include "class.h"
#include "handler.h"
#include "constants.h"
#include "assign_wpn_armor.h"
#include "domains_schools.h"
#include "spell_prep.h"
#include "alchemy.h"
#include "race.h"
#include "transport.h"
#include "dg_scripts.h"

extern struct room_data *world;
extern struct char_data *character_list;

// External Functions
room_rnum find_target_room(struct char_data *ch, char *rawroomstr);
int is_player_grouped(struct char_data *target, struct char_data *group);

// to get the map coords, using photoshop, enable rulers, zoom to 100%, press f8 and mouse over the position on the map image.
// map image located at: https://luminarimud.com/wiki/images/7/7e/Luminari-World-Map-Update-April-14-2020.png
// same applies to the airship map points below. Map point will be the spot where the airship tower is.

// location name, carriage stop room vnum, cost to travel here, continent name (matched below), zone description, mapp coord x, map coord y
const char *carriage_locales[][7] = {
  {"mosswood village",                      "145387", "10",   "Continent1", "starting area, levels 1-5", "964", "935"},
  {"ashenport",                             "103000", "10",   "Continent1", "central city for low to mid levels and main quest line", "973", "927"},
  {"wizard training mansion",               "5900",   "10",   "Continent1", "level 3-6 mobs, questline", "1002", "995"},
  {"graven hollow",                         "6766",   "100",  "Continent1", "level 7-12 mobs, questline", "1028", "958"},
  {"dollhouse",                             "11899",  "150",  "Continent1", "level 5-8 mobs, questline", "1192", "855"},
  {"blindbreak rest",                       "40400",  "200",  "Continent1", "level 10-11 mobs, questline", "972", "962"},
  {"mosaic cave",                           "40600",  "250",  "Continent1", "level 16-22 mobs, questline", "1028", "1022"},
  {"always the last item",                  "0",      "0",    "Nowhere", "nothing", "0", "0"}
};

// continent name, airshop dock room vnum, Cost in gold to travel here, faction name, contintent description, map coord x, map coord y
const char *airship_locales[][7] = {
  {"Continent1",                      "15209", "3000",   "No Faction", "Low and mid level zones, main questline", "0", "0"},
  {"Continent2",                      "15208", "3000",   "No Faction", "home of the dragonarmies & knights of takhisis", "500", "500"},
  {"always the last item",            "0",     "0",      "Nowhere", "nothing", "0", "0"}
};

ACMDU(do_carriage) {

  skip_spaces(&argument);

  int i = 0;
  bool found = false;
  while (atoi(carriage_locales[i][1]) != 0) {
    if (GET_ROOM_VNUM(IN_ROOM(ch)) == atoi(carriage_locales[i][1])) {
      found = true;
      break; 
    }
    i++;
  }

  int here = i;

  if (!found) {
	send_to_char(ch, "You are not at a valid %s.\r\n", "carriage stand");
    return;
  }

  if (!*argument) {
    found = false;
    i = 0;
	send_to_char(ch, "Available %s Destinations:\r\n", "Carriage");
	send_to_char(ch, "%-30s %4s %10s %10s (%s)\r\n", "Carriage Destination:", "Cost", "Distance", "Time (sec)", "Area Note");
    int j = 0;
    for (j = 0; j < 80; j++) 
      send_to_char(ch, "~");
    send_to_char(ch, "\r\n");
    while (atoi(carriage_locales[i][1]) != 0) {
      if (GET_ROOM_VNUM(IN_ROOM(ch)) != atoi(carriage_locales[i][1]) && ((here != 999) ? 
          (carriage_locales[here][3] == carriage_locales[i][3]) : TRUE)) {
        found = true;
		send_to_char(ch, "%-30s %4s %10d %10d (%s)\r\n", carriage_locales[i][0],  carriage_locales[i][2], get_distance(ch, i, here, TRAVEL_CARRIAGE), get_travel_time(ch, 10, i, here, TRAVEL_CARRIAGE), carriage_locales[i][4]);
      }
      i++;
    }

    if (found) {
      send_to_char(ch, "\r\nTo take a carriage, type carriage <name of destination>\r\n");
      return;
    } else {
      send_to_char(ch, "There are no available destinations from this carriage stand.\r\n");
    }
    return;
  } else {
    i = 0;
    found = false;
    while (atoi(carriage_locales[i][1]) != 0) {
      if (GET_ROOM_VNUM(IN_ROOM(ch)) != atoi(carriage_locales[i][1]) && ((here != 999) ? (carriage_locales[here][3] == carriage_locales[i][3]) : TRUE)) {
        if (is_abbrev(argument, carriage_locales[i][0])) {
          found = true;
          if (here != 999 && (atoi(carriage_locales[here][1]) != 30036 && atoi(carriage_locales[here][1]) != 30037)) {
          if (GET_GOLD(ch) < atoi(carriage_locales[i][2])) {
              send_to_char(ch, "You are denied entry as you cannot pay the fee of %s.\r\n", carriage_locales[i][2]);
              return;
          } else {
              send_to_char(ch, "You give the carriage driver your fee of %s.\r\n", carriage_locales[i][2]);
              GET_GOLD(ch) -= atoi(carriage_locales[i][2]);
          }
          }
          enter_transport(ch, i, TRAVEL_CARRIAGE, here);
          return;
        }
      }
      i++;
    }
    if (!found) {
      send_to_char(ch, "There is no carriage destination in this nation by that name.  Type carriage by itself to see a list of destinations.\r\n");
      return;
    }
  }
}

ACMDU(do_airship) {

  skip_spaces(&argument);

  int i = 0;
  char buf[200];
  bool found = false;
  while (atoi(airship_locales[i][1]) != 0) {
    if (GET_ROOM_VNUM(IN_ROOM(ch)) == atoi(airship_locales[i][1])) {
      found = true;
      break; 
    }
    i++;
  }

  if (!found) {
    send_to_char(ch, "You are not at a valid airship tower.\r\n");
    return;
  }

  int here = i;

  if (!*argument) {
    found = false;
    i = 0;
    send_to_char(ch, "Available airship Destinations:\r\n");
    send_to_char(ch, "%-30s %4s %10s %10s (%s)\r\n", "Airship Destination:", "Cost", "Distance", "Time (sec)", "Area Note");
    int j = 0;
    for (j = 0; j < 80; j++) 
      send_to_char(ch, "~");
    send_to_char(ch, "\r\n");
    while (atoi(airship_locales[i][1]) != 0) {
      if (GET_ROOM_VNUM(IN_ROOM(ch)) != atoi(airship_locales[i][1]) && valid_airship_travel(here, i)) {
        found = true;
        send_to_char(ch, "%-30s %4s %10d %10d (%s)\r\n", airship_locales[i][0],  airship_locales[i][2], get_distance(ch, i, here, TRAVEL_AIRSHIP), get_travel_time(ch, 10, i, here, TRAVEL_AIRSHIP), airship_locales[i][4]);
      }
      i++;
    }

    if (found) {
      send_to_char(ch, "\r\nTo take an airship, type airship <name of destination>\r\n");
      return;
    } else {
      send_to_char(ch, "There are no available destinations from this airship tower.\r\n");
    }
    return;
  } else {
    i = 0;
    found = false;
    while (atoi(airship_locales[i][1]) != 0) {
      if (GET_ROOM_VNUM(IN_ROOM(ch)) != atoi(airship_locales[i][1]) && valid_airship_travel(here, i)) {
        if (is_abbrev(argument, airship_locales[i][0])) {
          found = true;
          if (GET_GOLD(ch) < atoi(airship_locales[i][2])) {
              send_to_char(ch, "You are denied entry as you cannot pay the fee of %s.\r\n", airship_locales[i][2]);
              return;
          } else {
              send_to_char(ch, "You give the airship pilot your fee of %s.\r\n", airship_locales[i][2]);
              GET_GOLD(ch) -= atoi(airship_locales[i][2]);
          }
          room_rnum to_room = NOWHERE;
          snprintf(buf, sizeof(buf), "%s", airship_locales[i][1]);
          if ((to_room = find_target_room(ch, strdup(buf))) == NOWHERE) {
            send_to_char(ch, "There is an error with that destination.  Please report on the forums.\r\n");
            return;
          }
          enter_transport(ch, i, TRAVEL_AIRSHIP, here);
          return;
        }
      }
      i++;
    }
    if (!found) {
      send_to_char(ch, "There is no airship destination by that name.  Type airship by itself to see a list of destinations.\r\n");
      return;
    }
  }
}

int valid_airship_travel(int here, int i)
{

  //When airships are set up, this will make any checks necessary to allow airship travel from the existing locale

   return false;
}

void enter_transport(struct char_data *ch, int locale, int type, int here)
{
    int cnt = 0, found = false;
    char air[200], car[200];

    for (cnt = 0; cnt <= top_of_world; cnt++) {
        if (world[cnt].number < 66700 || world[cnt].number > 66799)
            continue;
        if (world[cnt].people)
            continue;
        found = false;
        break;
    }

    room_rnum to_room = NOWHERE;
    snprintf(air, sizeof(air), "%s", airship_locales[locale][1]);
    snprintf(car, sizeof(car), "%s", carriage_locales[locale][1]);
    if ((to_room = find_target_room(ch, (type == TRAVEL_AIRSHIP) ? strdup(air) : strdup(car))) == NOWHERE) {
    send_to_char(ch, "There is an error with that destination.  Please report on the to a staff member. ERRENTCAR001\r\n");
    return;
    }
    room_rnum taxi = cnt;

    if (taxi == NOWHERE) {
    if (type != TRAVEL_AIRSHIP)
        send_to_char(ch, "There are no carriages available currently.\r\n");
    else
        send_to_char(ch, "There are no airships available currently.\r\n");
    return;
    }

    struct follow_type *f = NULL;;
    struct char_data *tch = NULL;

    for (f = ch->followers; f; f = f->next) {
        tch = f->follower;
        if (IN_ROOM(tch) != IN_ROOM(ch))
            continue;
        if (!is_player_grouped(ch, tch))
            continue;
        if (FIGHTING(tch))
            continue;
        if (GET_POS(tch) < POS_STANDING)
            continue;
        if (type == TRAVEL_CARRIAGE) {
            act("$n boards a carriage which heads off into the distance.", FALSE, tch, 0, 0, TO_ROOM);
            send_to_char(tch, "Your group leader ushers you into a nearby carriage.\r\n");
            send_to_char(tch, "You hop into a carriage and head off towards %s.\r\n\r\n", carriage_locales[locale][0]);
        } else if (type == TRAVEL_AIRSHIP) {
            act("$n boards an airship which flies off into the distance.", FALSE, tch, 0, 0, TO_ROOM);
            send_to_char(tch, "Your group leader ushers you into the airship.\r\n");
            send_to_char(tch, "You board the airship and fly off towards %s.\r\n\r\n", airship_locales[locale][0]);
        }
        
        char_from_room(tch);
        char_to_room(tch, taxi);
        tch->player_specials->destination = to_room;
        tch->player_specials->travel_timer = get_travel_time(tch, 10, locale, here, type);
        tch->player_specials->travel_type = type;
        tch->player_specials->travel_locale = locale;
        look_at_room(tch, 0);
        entry_memory_mtrigger(tch);
        greet_mtrigger(tch, -1);
        greet_memory_mtrigger(tch);
    }

    if (type == TRAVEL_CARRIAGE) {
        act("$n boards a carriage which heads off into the distance.", FALSE, ch, 0, 0, TO_ROOM);
        send_to_char(ch, "You hop into a carriage and head off towards %s.\r\n\r\n", carriage_locales[locale][0]);
    } else if (type == TRAVEL_AIRSHIP) {
        act("$n boards an airship which flies off into the distance.", FALSE, ch, 0, 0, TO_ROOM);
        send_to_char(ch, "You board the airship and fly off towards %s.\r\n\r\n", airship_locales[locale][0]);
    }

    char_from_room(ch);
    char_to_room(ch, taxi);
    ch->player_specials->destination = to_room;
    ch->player_specials->travel_timer = get_travel_time(ch, 10, locale, here, type);
    ch->player_specials->travel_type = type;
    ch->player_specials->travel_locale = locale;
    look_at_room(ch, 0);
    entry_memory_mtrigger(ch);
    greet_mtrigger(ch, -1);
    greet_memory_mtrigger(ch);

}

void travel_tickdown(void)
{

  struct char_data *ch = NULL;
  room_rnum to_room = NOWHERE;
  char air[200], car[200];

  for (ch = character_list; ch; ch = ch->next) {

    if (IS_NPC(ch) || !ch->desc)
      continue;

    if (world[IN_ROOM(ch)].number < 66700 || world[IN_ROOM(ch)].number > 66799)
      continue;

    if (ch->player_specials->destination == 0 || ch->player_specials->destination == NOWHERE) {
      to_room = real_room(14100);
      char_from_room(ch);
      char_to_room(ch, to_room);
      look_at_room(ch, 0);
      entry_memory_mtrigger(ch);
      greet_mtrigger(ch, -1);
      greet_memory_mtrigger(ch); 

      continue;

    } else {
      ch->player_specials->travel_timer--;
      if (ch->player_specials->travel_timer < 1) {
        snprintf(air, sizeof(air), "%s", airship_locales[ch->player_specials->travel_locale][1]);
        snprintf(car, sizeof(car), "%s", carriage_locales[ch->player_specials->travel_locale][1]);
        if ((to_room = find_target_room(ch, ch->player_specials->travel_type == TRAVEL_AIRSHIP ? strdup(air) : strdup(car))) == NOWHERE) {
          char_from_room(ch);
          char_to_room(ch, to_room);
          look_at_room(ch, 0);
          entry_memory_mtrigger(ch);
          greet_mtrigger(ch, -1);
          greet_memory_mtrigger(ch);     
        }

        char_from_room(ch);
        char_to_room(ch, to_room);
        look_at_room(ch, 0);
        entry_memory_mtrigger(ch);
        greet_mtrigger(ch, -1);
        greet_memory_mtrigger(ch);

        if (ch->player_specials->travel_type == TRAVEL_CARRIAGE) {
          act("$n disembarks a horse-drawn carriage that grinds to a halt before you.", FALSE, ch, 0, 0, TO_ROOM);
          send_to_char(ch, "You hop out of your carriage arriving at the %s.\r\n\r\n",  carriage_locales[ch->player_specials->travel_locale][0]);
        }
        else if (ch->player_specials->travel_type == TRAVEL_AIRSHIP) {
          act("$n disembarks an airship that just landed here.", false, ch, 0, 0, TO_ROOM);
          send_to_char(ch, "You disembark your airship arriving at %s.\r\n\r\n", airship_locales[ch->player_specials->travel_locale][0]);
        }
        ch->player_specials->destination = NOWHERE;
        ch->player_specials->travel_timer = 0;
        ch->player_specials->travel_type = 0;
        ch->player_specials->travel_locale = 0;
      }
      continue;
    }
  }
}

int get_distance(struct char_data *ch, int locale, int here, int type)
{
  int xf, xt, yf, yt;
  xf = atoi(((type == TRAVEL_AIRSHIP)?airship_locales:carriage_locales)[here][5]);
  xt = atoi(((type == TRAVEL_AIRSHIP)?airship_locales:carriage_locales)[locale][5]);
  yf = atoi(((type == TRAVEL_AIRSHIP)?airship_locales:carriage_locales)[here][6]);
  yt = atoi(((type == TRAVEL_AIRSHIP)?airship_locales:carriage_locales)[locale][6]);

  int dx, dy;

  if (xf > xt)
    dx = xf - xt;
  else
    dx = xt - xf;

  if (yf > yt)
    dy = yf - yt;
  else
    dy = yt - yf;

  int distance = dx + dy;

  return distance;
}

int get_travel_time(struct char_data *ch, int speed, int locale, int here, int type)
{
  int distance = get_distance(ch, locale, here, type);

  distance *= 10;

  if (speed == 0) speed = 10;

  distance /= speed;

  distance /= 2;

  return distance;
}