/* ************************************************************************
 *    File:   transport.c                            Part of LuminariMUD  *
 * Purpose:   To provide auto travel functionality                        *
 *  Header:   transport.h                                                 *
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
#include "wilderness.h"

extern struct room_data *world;
extern struct char_data *character_list;

/* External Functions */
room_rnum find_target_room(struct char_data *ch, char *rawroomstr);
int is_player_grouped(struct char_data *target, struct char_data *group);
int find_first_step(room_rnum src, room_rnum target);

/* To get the map coords, use the coords found in the wilderness area where the zone connects.
   Same applies to the sailing map points below. Map point will be the spot where the sailing tower is. */

/* location name, carriage stop room vnum, cost to travel here, continent name (matched below),
zone description, mapp coord x, map coord y */
#if defined(CAMPAIGN_DL)
const char *carriage_locales[][CARRIAGE_LOCALES_FIELDS] = {
  {"palanthas gate",                 "15204", "25",   "Solamnia", "good alignment starting city", "1075", "2525" },
  {"vingaard keep",                  "15206", "25",   "Solamnia", "levels 13-20", "1320", "2925"},
  {"thelgaard keep",                 "15207", "25",  "Solamnia", "levels 21-24", "1765", "2555"},
  {"caergoth gate",                  "15210", "25",  "Solamnia", "levels 8-16, boat to Abanasinia", "2210", "2260"},
  {"solace",                         "15200", "25",   "Abanasinia", "city for neutral or unfactioned people of any alignment", "2690", "2535"},
  {"que-shu village",                "15202", "25",   "Abanasinia", "level 18 npcs", "2730", "2650"},
  {"xak tsaroth",                    "15203", "25",   "Abanasinia", "level 30 npcs, boat to sanction (taman busuk)", "2705", "2670"},
  {"fireside tavern",                "15211", "25",   "Abanasinia",  "level 12 npcs", "2615", "2650"},
  {"new sea docks",                  "15212", "25",  "Abanasinia",  "boat to solamnia", "2430", "2660"},
  {"qualinost",                      "15213", "25",  "Abanasinia", "level 20 npcs", "2855", "2485"},
  {"pax tharkas",                    "15214", "25",  "Abanasinia", "halfway between solace and tarsis", "2915", "2520"},
  {"tarsis",                         "15215", "25",  "Abanasinia", "levels 18-25", "3595", "2630"},
  {"sanction gate",                  "15205", "25",   "Taman Busuk",  "evil alignment starting city", "2020", "3765"},
  {"neraka",                         "15216", "25",   "Taman Busuk", "levels 14-25", "1805", "3950"},
  {"city of morning dew",            "15217", "25",  "Taman Busuk", "level 40 npcs", "3055", "3645"},
  {"plains of dust",                 "15218", "25",  "Taman Busuk", "near the onyx obelisk epic level zone", "3590", "3200"},
  {"always the last item",           "0",     "0",    "Nowhere", "nothing", "0", "0"}
};


/* continent name, ship dock room vnum, Cost in gold, faction name,
     contintent description, map coord x, map coord y */
const char *sailing_locales[][SAILING_LOCALES_FIELDS] = {
    {"palanthas dock", "2459", "50", "Any", "Palanthas, Jewel of Solamnia, is the city of the Solamnic Knights & Forces of Whitestone", "1075", "2525"},
    {"caergoth dock / northern new sea", "4430", "50", "Any", "Caergoth is a city in Western Solamnia and its greatest port besides Palanthas.", "2210", "2260"},
    {"abanasinia / southern new sea", "4429", "50", "Any", "Abanasinia is a temperate-climed land of many different peoples, cultures and races.", "2430", "2660"},
    {"sanction dock", "6500", "50", "Any", "Sanction is the economic center of the Dragonarmies of Takhisis, and the home to many evil races and sects.", "2020", "3765"},
    {"bethel island", "9201", "50", "Any", "Bethel Island is a small island in the Bay of Branchala north of Palanthas", "975", "2530"},
    {"eastern abanasinia", "2966", "50", "Any", "A travelers dock commonly used for trade in eastern Abanasinia.", "2830", "2570"},
    {"undomesticated island", "2501", "50", "Any", "A small island in the Eastern New Sea, near Sanction.", "1075", "2525"},
    {"northern ergoth", "34700", "50", "Any", "A neutral trade dock located in Northern Ergoth", "1765", "1210"},

    {"always the last item", "0", "0", "Nowhere", "nothing", "0", "0"},
};

#else
const char *carriage_locales[][CARRIAGE_LOCALES_FIELDS] = {
    {"ashenport", "103000", "10", "Ondius", "central city for low to mid levels and main quest line", "-59", "92"},
    {"mosswood village", "145387", "10", "Ondius", "starting area, levels 1-5", "-51", "99"},
    {"ardeep forest", "144062", "45", "Ondius", "level 3-12 mobs", "-40", "82"},
    {"dollhouse", "11899", "65", "Ondius", "level 5-8 mobs, questline", "169", "171"},
    {"blindbreak rest", "40400", "105", "Ondius", "level 10-11 mobs, questline", "-53", "63"},
    {"graven hollow", "6766", "70", "Ondius", "level 7-12 mobs, questline", "5", "67"},
    {"mere of dead men", "126860", "110", "Ondius", "level 9-30 mobs", "305", "313"},
    {"memlin caverns", "2701", "50", "Ondius", "level 8-20 mobs", "80", "115"},
    {"mosaic cave", "40600", "120", "Ondius", "level 16-22 mobs, questline", "3", "4"},
    {"neverwinter catacombs", "123200", "140", "Ondius", "level 16-30 mobs", "60", "54"},
    {"orc ruins", "106200", "30", "Ondius", "level 9-30 mobs", "210", "233"},
    {"orcish fort", "148100", "30", "Ondius", "level 14-16 mobs", "-54", "118"},

    {"bloodfist caverns", "102501", "40", "East Ubdina", "level 1-23 mobs", "-66", "-676"},

    {"corm orp", "105001", "40", "Selerish", "level 1-10 mobs", "167", "-85"},

    {"evereska", "120800", "65", "Quechian", "level 1-4 mobs", "-767", "157"},
    {"giant darkwood tree", "6901", "70", "Quechian", "level 15-20 mobs", "-717", "-51"},

    {"frozen castle", "1101", "65", "West Ubdina", "level 25-30 mobs", "-662", "-595"},
    {"lizard marsh", "121200", "70", "West Ubdina", "level 10-30 mobs", "-821", "-413"},

    {"glass tower", "11410", "65", "Carstan", "level 20-23 mobs", "641", "87"},
    {"hardbuckler", "118594", "70", "Carstan", "level 1-12 mobs", "624", "114"},

    {"grunwald", "117400", "70", "Hir", "level 1-16 mobs", "-509", "-170"},

    {"mithril hall", "108101", "120", "Kellust", "level 6-27 mobs", "349", "769"},

    {"neverwinter", "122413", "140", "Continent3", "level 1-24 mobs", "-591", "805"},
    {"pesh", "125900", "30", "Continent7", "level 1-19 mobs", "-150", "-180"},
    {"quagmire", "13240", "30", "Ondius", "level 1-30 mobs", "-58", "192"},
    {"rat hills", "115500", "70", "Ondius", "level 3-12 mobs", "-54", "78"},
    {"reaching woods", "127265", "70", "Continent9", "level 1-9, 16-19, 27 mobs", "-764", "138"},
    {"ruined keep", "101701", "70", "Continent7", "level 6-18 mobs", "-121", "-99"},
    {"sanctus", "140", "70", "Continent5", "level 3-20 mobs, major city of eastern continents", "695", "-240"},
    {"spider swamp", "199", "70", "Ondius", "level 10-20 mobs", "-44", "128"},
    {"the depths", "9200", "70", "Continent5", "level 20-22 mobs", "643", "-10"},
    {"tugrahk gol", "199", "70", "Continent7", "level 6-30 mobs", "-54", "-320"},
    {"wizard training mansion", "5900", "10", "Ondius", "level 3-6 mobs, questline", "-20", "99"},
    {"zhentil keep", "119100", "70", "Continent8", "level 1-30 mobs", "-563", "-583"},

    {"always the last item", "0", "0", "Nowhere", "nothing", "0", "0"},
};

/* continent name, ship dock room vnum, Cost in gold, faction name,
     contintent description, map coord x, map coord y */
const char *sailing_locales[][SAILING_LOCALES_FIELDS] = {
    {"ondius - ashenport", "34801", "100", "Any", "Ashenport is the main city hub for the main questline and many shops & services.", "-63", "89"},
    {"ondius - northwest seaport", "1000280", "100", "Any", "Nearby zones: Quagmire", "-25", "198"},
    {"ondius - southeast seaport", "1000281", "100", "Any", "Nearby zones: Neverwinter Catacombs", "104", "39"},
    {"ondius - northeast seaport", "1000282", "100", "Any", "Nearby zones: Tilverton, Orc Ruins, Mere of Dead Men", "191", "295"},

    {"selerish - corm orp seaport", "1000337", "100", "Any", "Nearby zones: Corm Orp", "161", "-79"},
    {"selerish - east seaport", "1000284", "100", "Any", "Nearby zones: Unknown", "358", "-200"},
    {"selerish - south seaport", "1000283", "100", "Any", "Nearby zones: Unknown", "363", "-295"},

    {"carstan - west seaport", "1000331", "100", "Any", "Nearby zones: Hardbuckler, Glass Tower", "575", "75"},
    {"carstan - east seaport", "1000332", "100", "Any", "Nearby zones: The Depths", "743", "-22"},

    {"axtros - sanctus", "1000333", "100", "Any", "Sanctus is a major city in Lumia with some unique products & services.", "688", "-241"},
    {"axtros - northeast seaport", "1000334", "100", "Any", "Nearby zones: South Wood", "866", "-284"},
    {"axtros - southwest seaport", "1000335", "100", "Any", "Nearby zones: Crimson Flame, Beregost", "591", "-524"},
    {"axtros - south seaport", "1000336", "100", "Any", "Nearby zones: Unknown", "606", "-719"},

    {"hir - southwest seaport", "1000364", "100", "Any", "Nearby zones: ", "-442", "-303"},
    {"hir - northwest seaport", "1000363", "100", "Any", "Nearby zones: ", "-507", "-123"},
    {"hir - northeast seaport", "1000366", "100", "Any", "Nearby zones: ", "-20", "-85"},
    {"hir - east seaport", "1000365", "100", "Any", "Nearby zones: ", "-57", "-330"},

    {"quechian - east seaport", "1000350", "100", "Any", "Nearby zones: Evereska, Reaching Woods, Aumvor's Castle", "-651", "-4"},
    {"quechian - southwest seaport", "1000351", "100", "Any", "Nearby zones: Dragon Cult Fortress", "-782", "-110"},
    {"quechian - northeast seaport", "1000349", "100", "Any", "Nearby zones: Giant Darkwood Tree", "-703", "155"},

    {"vailand - west seaport", "1000359", "100", "Any", "Nearby zones: Unknown", "-772", "473"},
    {"vailand - north seaport", "1000360", "100", "Any", "Nearby zones: Zzsessak Zuhl", "-599", "455"},
    {"vailand - central seaport", "1000362", "100", "Any", "Nearby zones: Unknown", "-467", "204"},
    {"vailand - south seaport", "1000361", "100", "Any", "Nearby zones: Shadowdale, Flaming Tower", "-512", "99"},

    {"oorpii - north seaport", "1000339", "100", "Any", "Nearby zones: Soubar", "-112", "785"},
    {"oorpii - east seaport", "1000338", "100", "Any", "Nearby zones: Skull Gorge", "-105", "510"},
    {"oorpii - west seaport", "1000279", "100", "Any", "Nearby zones: Mount Hotenow", "-316", "520"},
    {"oorpii - northwest seaport", "1000278", "100", "Any", "Nearby zones: Neverwinter", "-597", "804"},

    {"kellust - north seaport", "1000352", "100", "Any", "Nearby zones: Mithril Hall", "286", "885"},
    {"kellust - northeast seaport", "1000358", "100", "Any", "Nearby zones: Lost City of Thunderholme", "423", "779"},
    {"kellust - east seaport", "1000357", "100", "Any", "Nearby zones: Temple of Twisted Flesh", "644", "644"},
    {"kellust - southeast seaport", "1000356", "100", "Any", "Nearby zones: Unknown", "519", "539"},
    {"kellust - southwest seaport", "1000355", "100", "Any", "Nearby zones: Neverwinter Wood", "371", "431"},
    {"kellust - northwest seaport", "1000353", "100", "Any", "Nearby zones: Unknown", "161", "789"},
    {"kellust - west seaport", "1000354", "100", "Any", "Nearby zones: Dwarven Mines", "283", "724"},

    {"east ubdina - southwest seaport", "1000345", "100", "Any", "Nearby zones: Unknown", "-268", "-758"},
    {"east ubdina - south seaport", "1000346", "100", "Any", "Nearby zones: Bloodfist Caverns", "-110", "-722"},
    {"east ubdina - east seaport", "1000348", "100", "Any", "Nearby zones: Forest of Wyrms", "73", "-603"},
    {"east ubdina - north seaport", "1000347", "100", "Any", "Nearby zones: Settlestone", "-71", "-506"},

    {"west ubdina - west seaport", "1000340", "100", "Any", "Nearby zones: Frozen Castle", "-683", "-626"},
    {"west ubdina - northwest seaport", "1000341", "100", "Any", "Nearby zones: Lizard Marsh", "-824", "-406"},
    {"west ubdina - north seaport", "1000342", "100", "Any", "Nearby zones: Dagger Falls", "-554", "-489"},
    {"west ubdina - south seaport", "1000343", "100", "Any", "Nearby zones: Llawryn Keep Graveyard", "-566", "-677"},
    {"west ubdina - southeast seaport", "1000344", "100", "Any", "Nearby zones: Hulburg Trail", "-371", "-789"},

    {"always the last item", "0", "0", "Nowhere", "nothing", "0", "0"},
};

#endif

/* zone, destination vnum, title, details */
const char *walkto_landmarks[][WALKTO_LANDMARKS_FIELDS] = {
    /* Ashenport */
    {"1030", "103009", "jade jug inn", "Alerion, Henchmen, Huntsmaster, Missions"},
    {"1030", "103006", "bazaar", "Purchase gear with quest points"},
    {"1030", "103000", "north gate", "north gate of ashenport, fast travel carriages"},
    {"1030", "103451", "east gate", "east gate of ashenport"},
    {"1030", "103002", "south gate", "south gate of ashenport"},
    {"1030", "103051", "magic shop", "magic items"},
    {"1030", "103022", "general store", "general items, bags, lights"},
    {"1030", "103059", "crafting shop", "weapon molds for crafting"},
    {"1030", "103456", "bard guild", "musical instruments"},
    {"1030", "103465", "black market", "rogue tools, weapon poisons"},
    {"1030", "103385", "stables", "mounts for sale"},
    {"1030", "103021", "bank", "deposit and withdraw coins"},
    {"1030", "103047", "library", "research wizard spells"},
    {"1030", "103487", "armor shop", "sells +1 and +2 armor"},
    {"1030", "103488", "weapon shop", "sells +1 and +2 weapons"},
    {"1030", "103016", "post office", "send and receive mail"},
    {"1030", "103053", "grocer", "food and drink"},
    {"1030", "103052", "baker", "bread & pastries"},
    {"1030", "103070", "elfstone tavern", "specialty drinks"},

    /* always last! */
    {"0", "", "always last item", ""},
};

ACMDU(do_carriage)
{

  skip_spaces(&argument);

  int i = 0;
  bool found = false;

  while (atoi(carriage_locales[i][1]) != 0)
  {
    if (GET_ROOM_VNUM(IN_ROOM(ch)) == atoi(carriage_locales[i][1]))
    {
      found = true;
      break;
    }
    i++;
  }

  int here = i;

  if (!found)
  {
    send_to_char(ch, "You are not at a valid %s.\r\n", "carriage stand");
    return;
  }

  if (!*argument)
  {
    found = false;
    i = 0;
    send_to_char(ch, "Available %s Destinations:\r\n", "Carriage");
    send_to_char(ch, "%-30s %4s %10s %10s (%s)\r\n", "Carriage Destination:", "Cost", "Distance", "Time (sec)", "Area Note");
    int j = 0;
    for (j = 0; j < 80; j++)
      send_to_char(ch, "~");
    send_to_char(ch, "\r\n");
    while (atoi(carriage_locales[i][1]) != 0)
    {
      if (GET_ROOM_VNUM(IN_ROOM(ch)) != atoi(carriage_locales[i][1]) && ((here != 999) ? (carriage_locales[here][3] == carriage_locales[i][3]) : TRUE))
      {
        found = true;
        send_to_char(ch, "%-30s %4s %10d %10d (%s)\r\n", carriage_locales[i][0], carriage_locales[i][2], get_distance(ch, i, here, TRAVEL_CARRIAGE), get_travel_time(ch, 10, i, here, TRAVEL_CARRIAGE), carriage_locales[i][4]);
      }
      i++;
    }

    if (found)
    {
      send_to_char(ch, "\r\nTo take a carriage, type carriage <name of destination>\r\n");
      return;
    }
    else
    {
      send_to_char(ch, "There are no available destinations from this carriage stand.\r\n");
    }
    return;
  }
  else
  {
    i = 0;
    found = false;
    while (atoi(carriage_locales[i][1]) != 0)
    {
      if (GET_ROOM_VNUM(IN_ROOM(ch)) != atoi(carriage_locales[i][1]) && ((here != 999) ? (carriage_locales[here][3] == carriage_locales[i][3]) : TRUE))
      {
        if (is_abbrev(argument, carriage_locales[i][0]))
        {
          found = true;
          if (here != 999 && (atoi(carriage_locales[here][1]) != 30036 && atoi(carriage_locales[here][1]) != 30037))
          {
            if (GET_GOLD(ch) < atoi(carriage_locales[i][2]))
            {
              send_to_char(ch, "You are denied entry as you cannot pay the fee of %s.\r\n", carriage_locales[i][2]);
              return;
            }
            else
            {
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
    if (!found)
    {
      send_to_char(ch, "There is no carriage destination in this nation by that name.  Type carriage by itself to see a list of destinations.\r\n");
      return;
    }
  }
}

ACMDU(do_sail)
{

  skip_spaces(&argument);

  int i = 0;
  char buf[200];
  bool found = false;

  while (atoi(sailing_locales[i][1]) != 0)
  {
    if (GET_ROOM_VNUM(IN_ROOM(ch)) == atoi(sailing_locales[i][1]))
    {
      found = true;
      break;
    }
    i++;
  }

  if (!found)
  {
    send_to_char(ch, "You are not at a valid sea port.\r\n");
    return;
  }

  int here = i;

  if (!*argument)
  {
    found = false;
    i = 0;
    send_to_char(ch, "Available Sailing Destinations:\r\n");
    send_to_char(ch, "%-35s %4s %10s %10s (%s)\r\n", "Sailing Destination:", "Cost", "Distance", "Time (sec)", "Area Note");
    int j = 0;
    for (j = 0; j < 80; j++)
      send_to_char(ch, "~");
    send_to_char(ch, "\r\n");
    while (atoi(sailing_locales[i][1]) != 0)
    {
      if (GET_ROOM_VNUM(IN_ROOM(ch)) != atoi(sailing_locales[i][1]) && valid_sailing_travel(here, i))
      {
        found = true;
        send_to_char(ch, "%-35s %4s %10d %10d (%s)\r\n", sailing_locales[i][0], sailing_locales[i][2], get_distance(ch, i, here, TRAVEL_SAILING), get_travel_time(ch, 10, i, here, TRAVEL_SAILING), sailing_locales[i][4]);
      }
      i++;
    }

    if (found)
    {
      send_to_char(ch, "\r\nTo sail somewhere, type sail <name of destination>\r\n");
      send_to_char(ch, "\r\nYou can view our world map online at https://luminarimud.com/new-revised-worldmap-eat-your-heart-out/\r\n");
      return;
    }
    else
    {
      send_to_char(ch, "There are no available destinations from this sea port.\r\n");
    }
    return;
  }
  else
  {
    i = 0;
    found = false;
    while (atoi(sailing_locales[i][1]) != 0)
    {
      if (GET_ROOM_VNUM(IN_ROOM(ch)) != atoi(sailing_locales[i][1]) && valid_sailing_travel(here, i))
      {
        if (is_abbrev(argument, sailing_locales[i][0]))
        {
          found = true;
          if (GET_GOLD(ch) < atoi(sailing_locales[i][2]))
          {
            send_to_char(ch, "You are denied boarding as you cannot pay the fee of %s.\r\n", sailing_locales[i][2]);
            return;
          }
          else
          {
            send_to_char(ch, "You give the ship's captain your fee of %s.\r\n", sailing_locales[i][2]);
            GET_GOLD(ch) -= atoi(sailing_locales[i][2]);
          }
          room_rnum to_room = NOWHERE;
          snprintf(buf, sizeof(buf), "%s", sailing_locales[i][1]);
          if ((to_room = find_target_room(ch, strdup(buf))) == NOWHERE)
          {
            send_to_char(ch, "There is an error with that destination.  Please report to staff.\r\n");
            return;
          }
          enter_transport(ch, i, TRAVEL_SAILING, here);
          return;
        }
      }
      i++;
    }
    if (!found)
    {
      send_to_char(ch, "There is no sailing destination by that name.  Type sail by itself to see a list of destinations.\r\n");
      return;
    }
  }
}

int valid_sailing_travel(int here, int i)
{

  // When sailing is set up, this will make any checks necessary to allow sailing travel from the existing locale

  return true;
}

void enter_transport(struct char_data *ch, int locale, int type, int here)
{
  int cnt = 0, found = false;
  char air[200], car[200];

  for (cnt = 0; cnt <= top_of_world; cnt++)
  {
    if (world[cnt].number < 66700 || world[cnt].number > 66799)
      continue;
    if (world[cnt].people)
      continue;
    found = false;
    break;
  }

  room_rnum to_room = NOWHERE;

  if (type == TRAVEL_CARRIAGE)
  {
    snprintf(car, sizeof(car), "%s", carriage_locales[locale][1]);
  }
  else if (type == TRAVEL_SAILING)
  {
    snprintf(air, sizeof(air), "%s", sailing_locales[locale][1]);
  }
  else if (type == TRAVEL_OVERLAND_FLIGHT)
  {
#ifdef CAMPAIGN_FR
    snprintf(car, sizeof(car), "%s", zone_entrances[locale][0]);
#else
    snprintf(car, sizeof(car), "%s", carriage_locales[locale][1]);
#endif
  }
  else if (type == TRAVEL_OVERLAND_FLIGHT_SAIL)
  {
#ifdef CAMPAIGN_FR
    snprintf(car, sizeof(car), "%s", zone_entrances[locale][0]);
#else
    snprintf(car, sizeof(car), "%s", sailing_locales[locale][1]);
#endif
  }

  if ((to_room = find_target_room(ch, (type == TRAVEL_SAILING) ? strdup(air) : strdup(car))) == NOWHERE)
  {
    send_to_char(ch, "There is an error with that destination.  Please report on the to a staff member. ERRENTCAR001\r\n");
    return;
  }

  room_rnum taxi = cnt;

  if (taxi == NOWHERE)
  {
    if (type == TRAVEL_CARRIAGE)
      send_to_char(ch, "There are no carriages available currently.\r\n");
    else if (type == TRAVEL_SAILING)
      send_to_char(ch, "There are no ships available currently.\r\n");
    else
      send_to_char(ch, "The skies are too tumultous right now.\r\n");
    return;
  }

  struct follow_type *f = NULL;
  struct char_data *tch = NULL;

  for (f = ch->followers; f; f = f->next)
  {
    tch = f->follower;

    if (IN_ROOM(tch) != IN_ROOM(ch))
      continue;
    if (!is_player_grouped(ch, tch))
      continue;
    if (FIGHTING(tch))
      continue;
    if (GET_POS(tch) < POS_STANDING)
      continue;
    // overland flight is for the caster only
    if (type == TRAVEL_OVERLAND_FLIGHT && ch != tch)
      continue;
    if (type == TRAVEL_OVERLAND_FLIGHT_SAIL && ch != tch)
      continue;

    if (type == TRAVEL_CARRIAGE)
    {
      act("$n boards a carriage which heads off into the distance.", FALSE, tch, 0, 0, TO_ROOM);
      send_to_char(tch, "Your group leader ushers you into a nearby carriage.\r\n");
      send_to_char(tch, "You hop into a carriage and head off towards %s.\r\n\r\n", carriage_locales[locale][0]);
    }
    else if (type == TRAVEL_SAILING)
    {
      act("$n boards a caravel, which pulls anchor and heads for the open sea.", FALSE, tch, 0, 0, TO_ROOM);
      send_to_char(tch, "Your group leader ushers you into the ship.\r\n");
      send_to_char(tch, "You board the caravel and sail off to %s.\r\n\r\n", sailing_locales[locale][0]);
    }
    else if (type == TRAVEL_OVERLAND_FLIGHT)
    {
      act("$n leaps into the air and flies off into the distance.", FALSE, tch, 0, 0, TO_ROOM);
#ifdef CAMPAIGN_FR
      send_to_char(tch, "You leap into the air and fly off towards %s.\r\n\r\n", zone_entrances[locale][0]);
#else
      send_to_char(tch, "You leap into the air and fly off towards %s.\r\n\r\n", carriage_locales[locale][0]);
#endif
    }
    else if (type == TRAVEL_OVERLAND_FLIGHT_SAIL)
    {
      act("$n leaps into the air and flies off into the distance.", FALSE, tch, 0, 0, TO_ROOM);
#ifdef CAMPAIGN_FR
      send_to_char(tch, "You leap into the air and fly off towards %s.\r\n\r\n", sailing_locales[locale][0]);
#else
      send_to_char(tch, "You leap into the air and fly off towards %s.\r\n\r\n", carriage_locales[locale][0]);
#endif
    }

    char_from_room(tch);
    char_to_room(tch, taxi);
    char_pets_to_char_loc(tch);
    tch->player_specials->destination = to_room;
    // need to take care of this part still for overland flight spell
    tch->player_specials->travel_timer = get_travel_time(tch, 10, locale, here, type);
    tch->player_specials->travel_type = type;
    tch->player_specials->travel_locale = locale;
    look_at_room(tch, 0);
    entry_memory_mtrigger(tch);
    greet_mtrigger(tch, -1);
    greet_memory_mtrigger(tch);
  }

  if (type == TRAVEL_CARRIAGE)
  {
    act("$n boards a carriage which heads off into the distance.", FALSE, ch, 0, 0, TO_ROOM);
    send_to_char(ch, "You hop into a carriage and head off towards %s.\r\n\r\n", carriage_locales[locale][0]);
  }
  else if (type == TRAVEL_SAILING)
  {
    act("$n boards a caravel which sails off into the distance.", FALSE, ch, 0, 0, TO_ROOM);
    send_to_char(ch, "You board the caravel and sail off towards %s.\r\n\r\n", sailing_locales[locale][0]);
  }

  char_from_room(ch);
  char_to_room(ch, taxi);
  char_pets_to_char_loc(tch);
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
  struct descriptor_data *d = NULL;
  room_rnum to_room = NOWHERE;
  char sail[200], car[200];

  for (d = descriptor_list; d; d = d->next)
  {
    ch = d->character;

    if (!ch || IS_NPC(ch) || !ch->desc)
      continue;

    if (STATE(ch->desc) != CON_PLAYING)
      continue;
    if (IN_ROOM(ch) == NOWHERE)
      continue;

    if (world[IN_ROOM(ch)].number < 66700 || world[IN_ROOM(ch)].number > 66799)
      continue;

    if (ch->player_specials->destination == 0 || ch->player_specials->destination == NOWHERE)
    {
#if defined(CAMPAIGN_DL)
      to_room = real_room(16500);
#else
      to_room = real_room(14100);
#endif
      char_from_room(ch);
      char_to_room(ch, to_room);
      char_pets_to_char_loc(ch);
      look_at_room(ch, 0);
      entry_memory_mtrigger(ch);
      greet_mtrigger(ch, -1);
      greet_memory_mtrigger(ch);

      continue;
    }
    else
    {
      ch->player_specials->travel_timer--;
      if (ch->player_specials->travel_timer < 1)
      {
        if (ch->player_specials->travel_type == TRAVEL_CARRIAGE)
        {
          snprintf(car, sizeof(car), "%s", carriage_locales[ch->player_specials->travel_locale][1]);
        }
        else if (ch->player_specials->travel_type == TRAVEL_SAILING)
        {
          snprintf(sail, sizeof(sail), "%s", sailing_locales[ch->player_specials->travel_locale][1]);
        }
        else if (ch->player_specials->travel_type == TRAVEL_OVERLAND_FLIGHT)
        {
#ifdef CAMPAIGN_FR
          snprintf(car, sizeof(car), "%s", zone_entrances[ch->player_specials->travel_locale][2]);
#else
          snprintf(car, sizeof(car), "%s", carriage_locales[ch->player_specials->travel_locale][1]);
#endif
        }
        else if (ch->player_specials->travel_type == TRAVEL_OVERLAND_FLIGHT_SAIL)
        {
#ifdef CAMPAIGN_FR
          snprintf(car, sizeof(car), "%s", zone_entrances[ch->player_specials->travel_locale][2]);
#else
          snprintf(car, sizeof(car), "%s", sailing_locales[ch->player_specials->travel_locale][1]);
#endif
        }

        if (ch->player_specials->travel_type == TRAVEL_SAILING)
        {
          if (atoi(sail) < 1000000)
          {
            if ((to_room = find_target_room(ch, (char *)(sail))) == NOWHERE)
            {
              char_from_room(ch);
              char_to_room(ch, to_room);
              char_pets_to_char_loc(ch);
              look_at_room(ch, 0);
              entry_memory_mtrigger(ch);
              greet_mtrigger(ch, -1);
              greet_memory_mtrigger(ch);
            }
          }
          /* Have two args, that means coordinates (potentially) */
          else if ((to_room = find_room_by_coordinates(atoi(sailing_locales[ch->player_specials->travel_locale][5]),
                                                       atoi(sailing_locales[ch->player_specials->travel_locale][6]))) == NOWHERE)
          {
            if ((to_room = find_available_wilderness_room()) == NOWHERE)
            {
              char_from_room(ch);
              char_to_room(ch, to_room);
              char_pets_to_char_loc(ch);
              look_at_room(ch, 0);
              entry_memory_mtrigger(ch);
              greet_mtrigger(ch, -1);
              greet_memory_mtrigger(ch);
            }
            else
            {
              /* Must set the coords, etc in the going_to room. */
              assign_wilderness_room(to_room, atoi(sailing_locales[ch->player_specials->travel_locale][5]),
                                     atoi(sailing_locales[ch->player_specials->travel_locale][6]));
            }
          }
        }
        else if ((to_room = find_target_room(ch, (char *)(car))) == NOWHERE)
        {
          char_from_room(ch);
          char_to_room(ch, to_room);
          char_pets_to_char_loc(ch);
          look_at_room(ch, 0);
          entry_memory_mtrigger(ch);
          greet_mtrigger(ch, -1);
          greet_memory_mtrigger(ch);
        }

        char_from_room(ch);

        if (ZONE_FLAGGED(GET_ROOM_ZONE(to_room), ZONE_WILDERNESS))
        {
          //    char_to_coords(ch, world[to_room].coords[0], world[to_room].coords[1], 0);
          X_LOC(ch) = world[to_room].coords[0];
          Y_LOC(ch) = world[to_room].coords[1];
        }

        char_to_room(ch, to_room);
        char_pets_to_char_loc(ch);
        look_at_room(ch, 0);
        entry_memory_mtrigger(ch);
        greet_mtrigger(ch, -1);
        greet_memory_mtrigger(ch);

        if (ch->player_specials->travel_type == TRAVEL_CARRIAGE)
        {
          act("$n disembarks a horse-drawn carriage that grinds to a halt before you.", FALSE, ch, 0, 0, TO_ROOM);
          send_to_char(ch, "You hop out of your carriage arriving at the %s.\r\n\r\n", carriage_locales[ch->player_specials->travel_locale][0]);
        }
        else if (ch->player_specials->travel_type == TRAVEL_SAILING)
        {
          act("$n disembarks a caravel that just docked here.", false, ch, 0, 0, TO_ROOM);
          send_to_char(ch, "You disembark the caravel arriving at %s.\r\n\r\n", sailing_locales[ch->player_specials->travel_locale][0]);
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
  int xf = 0, xt = 0, yf = 0, yt = 0;

#ifdef CAMPAIGN_FR

  int room = 0;

  if (type == TRAVEL_CARRIAGE || type == TRAVEL_SAILING)
  {
    xf = atoi(((type == TRAVEL_SAILING) ? sailing_locales : carriage_locales)[here][5]);
    xt = atoi(((type == TRAVEL_SAILING) ? sailing_locales : carriage_locales)[locale][5]);
    yf = atoi(((type == TRAVEL_SAILING) ? sailing_locales : carriage_locales)[here][6]);
    yt = atoi(((type == TRAVEL_SAILING) ? sailing_locales : carriage_locales)[locale][6]);
  }
  else if (type == TRAVEL_OVERLAND_FLIGHT)
  {
    set_x_y_coords(atoi(zone_entrances[locale][2]), &xf, &yf, &room);
    set_x_y_coords(here, &xt, &yt, &room);
  }

#else
  if (type == TRAVEL_CARRIAGE || type == TRAVEL_SAILING)
  {
    xf = atoi(((type == TRAVEL_SAILING) ? sailing_locales : carriage_locales)[here][5]);
    xt = atoi(((type == TRAVEL_SAILING) ? sailing_locales : carriage_locales)[locale][5]);
    yf = atoi(((type == TRAVEL_SAILING) ? sailing_locales : carriage_locales)[here][6]);
    yt = atoi(((type == TRAVEL_SAILING) ? sailing_locales : carriage_locales)[locale][6]);
  }
  else if (type == TRAVEL_OVERLAND_FLIGHT)
  {
    xf = ch->coords[0];
    xt = atoi(carriage_locales[locale][5]);
    yf = ch->coords[1];
    yt = atoi(carriage_locales[locale][6]);
  }
  else if (type == TRAVEL_OVERLAND_FLIGHT_SAIL)
  {
    xf = ch->coords[0];
    xt = atoi(sailing_locales[locale][5]);
    yf = ch->coords[1];
    yt = atoi(sailing_locales[locale][6]);
  }
#endif

#if defined(CAMPAIGN_DL)
  xf = yf = 0;
#endif

  int dx, dy;

  dx = xt - xf;
  dy = yt - yf;

  int total = pow(dx, 2) + pow(dy, 2);
  int dist = sqrt(total);

  return dist / 2;
}

int get_travel_time(struct char_data *ch, int speed, int locale, int here, int type)
{
  int distance = get_distance(ch, locale, here, type);

  distance *= 10;


  if (speed == 0)
    speed = 10;

#ifdef CAMPAIGN_FR
  distance *= 5;
#endif

  distance /= speed;

  distance /= 2;

#if defined(CAMPAIGN_DL)
  distance /= 3;
#endif

  return distance;
}

const char *get_walkto_location_name(int locale_vnum)
{

  int i = 0;

  if (locale_vnum == 0)
  {
    return "WALKTO NAME ERROR 1!";
  }

  while (atoi(walkto_landmarks[i][0]) != 0)
  {
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

  while ((zone = atoi(walkto_landmarks[i][0])) != 0)
  {
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

  while ((zone = atoi(walkto_landmarks[i][0])) != 0)
  {
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
    if (!ch)
      continue;
    if (!GET_WALKTO_LOC(ch))
      continue;
    destination = real_room(GET_WALKTO_LOC(ch));
    if (destination == NOWHERE)
      continue;
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

/* EoF */
