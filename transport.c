/* ************************************************************************
 *    File:   transport.c                            Part of LuminariMUD  *
 * Purpose:   To provide auto travel functionality                        *
 *  Author:   Gicker                                                      *
 ************************************************************************ */

#include <math.h>

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
int find_first_step(room_rnum src, room_rnum target);

// To get the map coords, use the coords found in the wilderness area where the zone connects.
// Same applies to the airship map points below. Map point will be the spot where the airship tower is.

// location name, carriage stop room vnum, cost to travel here, continent name (matched below), zone description, mapp coord x, map coord y
const char *carriage_locales[][7] = {
  {"ardeep forest",                         "144062", "45",   "Continent1", "level 3-12 mobs", "-40", "82"},
  {"ashenport",                             "103000", "10",   "Continent1", "central city for low to mid levels and main quest line", "-59", "92"},
  {"blindbreak rest",                       "40400",  "105",  "Continent1", "level 10-11 mobs, questline", "-53", "63"},
  {"bloodfist caverns",                     "102501", "40",   "Continent8", "level 1-23 mobs", "-66", "-676"},
  {"corm orp",                              "105001", "40",   "Continent4", "level 1-10 mobs", "167", "-85"},
  {"dollhouse",                             "11899",  "65",   "Continent1", "level 5-8 mobs, questline", "169", "171"},
  {"evereska",                              "120800", "65",   "Continent9", "level 1-4 mobs", "-767", "157"},
  {"frozen castle",                         "1101",   "65",   "Continent8", "level 25-30 mobs", "-662", "-595"},
  {"giant darkwood tree",                   "6901",   "70",   "Continent9", "level 15-20 mobs", "-717", "-51"},
  {"glass tower",                           "11410",  "65",   "Continent5", "level 20-23 mobs", "641", "87"},
  {"graven hollow",                         "6766",   "70",   "Continent1", "level 7-12 mobs, questline", "5", "67"},
  {"grunwald",                              "117400", "70",   "Continent7", "level 1-16 mobs", "-509", "-170"},
  {"hardbuckler",                           "118594", "70",   "Continent5", "level 1-12 mobs", "624", "114"},
  {"lizard marsh",                          "121200", "70",   "Continent8", "level 10-30 mobs", "-821", "-413"},
  {"mere of dead men",                      "126860", "110",  "Continent1", "level 9-30 mobs", "305", "313"},
  {"memlin caverns",                        "2701",   "50",   "Continent1", "level 8-20 mobs", "80", "115"},
  {"mithril hall",                          "108101", "120",  "Continent2", "level 6-27 mobs", "349", "769"},
  {"mosaic cave",                           "40600",  "120",  "Continent1", "level 16-22 mobs, questline", "3", "4"},
  {"mosswood village",                      "145387", "10",   "Continent1", "starting area, levels 1-5", "-51", "99"},
  {"neverwinter",                           "122413", "140",  "Continent3", "level 1-24 mobs", "-591", "805"},
  {"neverwinter catacombs",                 "123200", "140",  "Continent1", "level 16-30 mobs", "60", "54"},
  {"orc ruins",                             "106200", "30",   "Continent1", "level 9-30 mobs", "210", "233"},
  {"orcish fort",                           "148100", "30",   "Continent1", "level 14-16 mobs", "-54", "118"},
  {"pesh",                                  "125900", "30",   "Continent7", "level 1-19 mobs", "-150", "-180"},
  {"quagmire",                              "13240",  "30",   "Continent1", "level 1-30 mobs", "-58", "192"},
  {"rat hills",                             "115500", "70",   "Continent1", "level 3-12 mobs", "-54", "78"},
  {"reaching woods",                        "127265", "70",   "Continent9", "level 1-9, 16-19, 27 mobs", "-764", "138"},
  {"ruined keep",                           "101701", "70",   "Continent7", "level 6-18 mobs", "-121", "-99"},
  {"sanctus",                               "140",    "70",   "Continent5", "level 3-20 mobs, major city of eastern continents", "695", "-240"},
  {"spider swamp",                          "199",    "70",   "Continent1", "level 10-20 mobs", "-44", "128"},
  {"the depths",                            "9200",   "70",   "Continent5", "level 20-22 mobs", "643", "-10"},
  {"tugrahk gol",                           "199",    "70",   "Continent7", "level 6-30 mobs", "-54", "-320"},
  {"wizard training mansion",               "5900",   "10",   "Continent1", "level 3-6 mobs, questline", "-20", "99"},
  {"zhentil keep",                          "119100", "70",   "Continent8", "level 1-30 mobs", "-563", "-583"},
  {"always the last item",                  "0",      "0",    "Nowhere",    "nothing", "0", "0"}
};

// continent name, airshop dock room vnum, Cost in gold to travel here, faction name, contintent description, map coord x, map coord y
const char *airship_locales[][7] = {
  {"Continent1",                      "15209", "3000",   "No Faction", "Low and mid level zones, main questline", "0", "0"},
  {"Continent2",                      "15208", "3000",   "No Faction", "home of the dragonarmies & knights of takhisis", "500", "500"},
  {"always the last item",            "0",     "0",      "Nowhere", "nothing", "0", "0"}
};

const char * walkto_landmarks[][4] = {
  // Ashenport
  {"1030", "103009", "jade jug inn",     "Alerion, Henchmen, Huntsmaster, Missions"},
  {"1030", "103006", "bazaar",           "Purchase gear with quest points"},
  {"1030", "103000", "north gate",       "north gate of ashenport, fast travel carriages"},
  {"1030", "103451", "east gate",        "east gate of ashenport"},
  {"1030", "103002", "south gate",       "south gate of ashenport"},
  {"1030", "103051", "magic shop",       "magic items"},
  {"1030", "103022", "general store",    "general items, bags, lights"},
  {"1030", "103059", "crafting shop",    "weapon molds for crafting"},
  {"1030", "103456", "bard guild",       "musical instruments"},
  {"1030", "103465", "black market",     "rogue tools, weapon poisons"},
  {"1030", "103385", "stables",          "mounts for sale"},
  {"1030", "103021", "bank",             "deposit and withdraw coins"},
  {"1030", "103047", "library",          "research wizard spells"},
  {"1030", "103487", "armor shop",       "sells +1 armor"},
  {"1030", "103488", "weapon shop",      "sells +1 weapons"},
  {"1030", "103016", "post office",      "send and receive mail"},
  {"1030", "103053", "grocer",           "food and drink"},
  {"1030", "103052", "baker",            "bread & pastries"},
  {"1030", "103070", "elfstone tavern",  "specialty drinks"},
  {"0",    "", "always last item", ""}
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

    if (STATE(ch->desc) != CON_PLAYING) continue;
    if (IN_ROOM(ch) == NOWHERE) continue;

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

  dx = xt - xf;
  dy = yt - yf;

  int total = pow(dx, 2) + pow(dy, 2);
  int dist = sqrt(total);

  return dist;
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

const char *get_walkto_location_name(int locale_vnum)
{

  int i = 0;

  if (locale_vnum == 0)
  {
    return "WALKTO NAME ERROR 1!";
  }

  while (atoi(walkto_landmarks[i][0]) != 0) {
    if (locale_vnum == atoi(walkto_landmarks[i][1]))
    {
      return walkto_landmarks[i][2];
    }
    i++;
  }

  return "WALKTO NAME ERROR 2!";

}

ACMDU(do_walkto)
{

  int i = 0;
  bool found = false;
  int zone = 0;
  int landmark = 0;

  skip_spaces(&argument);

  if (!*argument)
  {
    send_to_char(ch, "You need to specify the landmark you wish to travel to.  Type 'LANDMARKS' for a list.\r\n");
    send_to_char(ch, "You can type 'walkto cancel' to cancel your current walkto action.\r\n");
    return;
  }

  if (IN_ROOM(ch) == NOWHERE)
  {
    send_to_char(ch, "You cannot use this command here.\r\n");
    return;
  }

  if (is_abbrev(argument, "cancel"))
  {
    send_to_char(ch, "You stop walking to the %s", get_walkto_location_name(GET_WALKTO_LOC(ch)));
    GET_WALKTO_LOC(ch) = 0;
    return;
  }

  while ((zone = atoi(walkto_landmarks[i][0])) != 0) {
    if (zone == zone_table[world[IN_ROOM(ch)].zone].number)
    {
      if (is_abbrev(argument, walkto_landmarks[i][2]))
      {
        landmark = atoi(walkto_landmarks[i][1]);
        found = true;
        break;
      }
    }
    i++;
  }

  if (!found)
  {
    send_to_char(ch, "That is not a valid landmark you to travel to.  Type 'LANDMARKS' for a list.\r\n");
    return;
  }

  GET_WALKTO_LOC(ch) = landmark;
  send_to_char(ch, "You begin walking to the %s.\r\n", get_walkto_location_name(landmark));

}

ACMD(do_landmarks)
{
  int i = 0, zone = 0, count = 0;
  bool found = false;

  if (IN_ROOM(ch) == NOWHERE)
  {
    send_to_char(ch, "You cannot use this command right now.\r\n");
    return;
  }
  
  while ((zone = atoi(walkto_landmarks[i][0])) != 0) {
    if (zone == zone_table[world[IN_ROOM(ch)].zone].number)
    {
      if (count == 0)
      {
        send_to_char(ch, "\tC%-25s - %s\tn\r\n", "LANDMARK NAME", "DESCRIPTION");
      }
      send_to_char(ch, "%-25s - %s\r\n", walkto_landmarks[i][2], walkto_landmarks[i][3]);
      found = true;
      count++;
    }
    i++;
  }

  send_to_char(ch, "\r\n");

  if (!found)
    send_to_char(ch, "There are no landmarks in this area.\r\n");
}

void process_walkto_actions(void)
{
  struct descriptor_data *d = NULL;
  struct char_data *ch = NULL;
  int dir = 0;
  room_rnum destination = NOWHERE;

  for (d = descriptor_list; d; d = d->next)
  {
    ch = d->character;
    if (!ch) continue;
    if (!GET_WALKTO_LOC(ch)) continue;
    destination = real_room(GET_WALKTO_LOC(ch));
    if (destination == NOWHERE) continue;
    if ((dir = find_first_step(IN_ROOM(ch), destination)) < 0)
    {
      send_to_char(ch, "Your walk to the %s has been interrupted %d.\r\n", get_walkto_location_name(GET_WALKTO_LOC(ch)), dir);
      GET_WALKTO_LOC(ch) = 0;
      continue;
    }
    else
    {
      perform_move(ch, dir, 1);
      if (IN_ROOM(ch) == destination)
      {
        send_to_char(ch, "You have arrived at the %s.\r\n", get_walkto_location_name(GET_WALKTO_LOC(ch)));
        GET_WALKTO_LOC(ch) = 0;
      }
      else
      {
        send_to_char(ch, "You continue walking to the %s.  Type walkto cancel to stop.\r\n", get_walkto_location_name(GET_WALKTO_LOC(ch)));
      }
    }

  }
}