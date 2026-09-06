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
#include "olc/oasis.h"
#include "screen.h"
#include "interpreter.h"
#include "modify.h"
#include "magic/spells.h"
#include "character/feats.h"
#include "character/class.h"
#include "handler.h"
#include "constants.h"
#include "combat/assign_wpn_armor.h"
#include "magic/domains_schools.h"
#include "magic/spell_prep.h"
#include "craft/alchemy.h"
#include "character/race.h"
#include "transport.h"
#include "transport_jobs.h"
#include "domain_event_world.h"
#include "domain_event_runtime.h"
#include "../character_periodic.h"
#include "dgscript/dg_scripts.h"
#include "wilderness/wilderness.h"
#include "graph.h"
#include "routing.h"

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
const char *carriage_locales_lumi[][CARRIAGE_LOCALES_FIELDS] = {
    {"ashenport", "103000", "10", "Ondius",
     "central city for low to mid levels and main quest line", "-59", "92"},
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
    {"sanctus", "140", "70", "Continent5", "level 3-20 mobs, major city of eastern continents",
     "695", "-240"},
    {"spider swamp", "199", "70", "Ondius", "level 10-20 mobs", "-44", "128"},
    {"the depths", "9200", "70", "Continent5", "level 20-22 mobs", "643", "-10"},
    {"tugrahk gol", "199", "70", "Continent7", "level 6-30 mobs", "-54", "-320"},
    {"wizard training mansion", "5900", "10", "Ondius", "level 3-6 mobs, questline", "-20", "99"},
    {"zhentil keep", "119100", "70", "Continent8", "level 1-30 mobs", "-563", "-583"},

    {"always the last item", "0", "0", "Nowhere", "nothing", "0", "0"},
};

/* continent name, ship dock room vnum, Cost in gold, faction name,
     contintent description, map coord x, map coord y */
const char *sailing_locales_lumi[][SAILING_LOCALES_FIELDS] = {
    {"ondius - ashenport", "34801", "100", "Any",
     "Ashenport is the main city hub for the main questline and many shops & services.", "-63",
     "89"},
    {"ondius - northwest seaport", "1000280", "100", "Any", "Nearby zones: Quagmire", "-25", "198"},
    {"ondius - southeast seaport", "1000281", "100", "Any", "Nearby zones: Neverwinter Catacombs",
     "104", "39"},
    {"ondius - northeast seaport", "1000282", "100", "Any",
     "Nearby zones: Tilverton, Orc Ruins, Mere of Dead Men", "191", "295"},

    {"selerish - corm orp seaport", "1000337", "100", "Any", "Nearby zones: Corm Orp", "161",
     "-79"},
    {"selerish - east seaport", "1000284", "100", "Any", "Nearby zones: Unknown", "358", "-200"},
    {"selerish - south seaport", "1000283", "100", "Any", "Nearby zones: Unknown", "363", "-295"},

    {"carstan - west seaport", "1000331", "100", "Any", "Nearby zones: Hardbuckler, Glass Tower",
     "575", "75"},
    {"carstan - east seaport", "1000332", "100", "Any", "Nearby zones: The Depths", "743", "-22"},

    {"axtros - sanctus", "1000333", "100", "Any",
     "Sanctus is a major city in Lumia with some unique products & services.", "688", "-241"},
    {"axtros - northeast seaport", "1000334", "100", "Any", "Nearby zones: South Wood", "866",
     "-284"},
    {"axtros - southwest seaport", "1000335", "100", "Any", "Nearby zones: Crimson Flame, Beregost",
     "591", "-524"},
    {"axtros - south seaport", "1000336", "100", "Any", "Nearby zones: Unknown", "606", "-719"},

    {"hir - southwest seaport", "1000364", "100", "Any", "Nearby zones: ", "-442", "-303"},
    {"hir - northwest seaport", "1000363", "100", "Any", "Nearby zones: ", "-507", "-123"},
    {"hir - northeast seaport", "1000366", "100", "Any", "Nearby zones: ", "-20", "-85"},
    {"hir - east seaport", "1000365", "100", "Any", "Nearby zones: ", "-57", "-330"},

    {"quechian - east seaport", "1000350", "100", "Any",
     "Nearby zones: Evereska, Reaching Woods, Aumvor's Castle", "-651", "-4"},
    {"quechian - southwest seaport", "1000351", "100", "Any", "Nearby zones: Dragon Cult Fortress",
     "-782", "-110"},
    {"quechian - northeast seaport", "1000349", "100", "Any", "Nearby zones: Giant Darkwood Tree",
     "-703", "155"},

    {"vailand - west seaport", "1000359", "100", "Any", "Nearby zones: Unknown", "-772", "473"},
    {"vailand - north seaport", "1000360", "100", "Any", "Nearby zones: Zzsessak Zuhl", "-599",
     "455"},
    {"vailand - central seaport", "1000362", "100", "Any", "Nearby zones: Unknown", "-467", "204"},
    {"vailand - south seaport", "1000361", "100", "Any", "Nearby zones: Shadowdale, Flaming Tower",
     "-512", "99"},

    {"oorpii - north seaport", "1000339", "100", "Any", "Nearby zones: Soubar", "-112", "785"},
    {"oorpii - east seaport", "1000338", "100", "Any", "Nearby zones: Skull Gorge", "-105", "510"},
    {"oorpii - west seaport", "1000279", "100", "Any", "Nearby zones: Mount Hotenow", "-316",
     "520"},
    {"oorpii - northwest seaport", "1000278", "100", "Any", "Nearby zones: Neverwinter", "-597",
     "804"},

    {"kellust - north seaport", "1000352", "100", "Any", "Nearby zones: Mithril Hall", "286",
     "885"},
    {"kellust - northeast seaport", "1000358", "100", "Any",
     "Nearby zones: Lost City of Thunderholme", "423", "779"},
    {"kellust - east seaport", "1000357", "100", "Any", "Nearby zones: Temple of Twisted Flesh",
     "644", "644"},
    {"kellust - southeast seaport", "1000356", "100", "Any", "Nearby zones: Unknown", "519", "539"},
    {"kellust - southwest seaport", "1000355", "100", "Any", "Nearby zones: Neverwinter Wood",
     "371", "431"},
    {"kellust - northwest seaport", "1000353", "100", "Any", "Nearby zones: Unknown", "161", "789"},
    {"kellust - west seaport", "1000354", "100", "Any", "Nearby zones: Dwarven Mines", "283",
     "724"},

    {"east ubdina - southwest seaport", "1000345", "100", "Any", "Nearby zones: Unknown", "-268",
     "-758"},
    {"east ubdina - south seaport", "1000346", "100", "Any", "Nearby zones: Bloodfist Caverns",
     "-110", "-722"},
    {"east ubdina - east seaport", "1000348", "100", "Any", "Nearby zones: Forest of Wyrms", "73",
     "-603"},
    {"east ubdina - north seaport", "1000347", "100", "Any", "Nearby zones: Settlestone", "-71",
     "-506"},

    {"west ubdina - west seaport", "1000340", "100", "Any", "Nearby zones: Frozen Castle", "-683",
     "-626"},
    {"west ubdina - northwest seaport", "1000341", "100", "Any", "Nearby zones: Lizard Marsh",
     "-824", "-406"},
    {"west ubdina - north seaport", "1000342", "100", "Any", "Nearby zones: Dagger Falls", "-554",
     "-489"},
    {"west ubdina - south seaport", "1000343", "100", "Any", "Nearby zones: Llawryn Keep Graveyard",
     "-566", "-677"},
    {"west ubdina - southeast seaport", "1000344", "100", "Any", "Nearby zones: Hulburg Trail",
     "-371", "-789"},

    {"always the last item", "0", "0", "Nowhere", "nothing", "0", "0"},
};

/* zone, destination vnum, title, details */
const char *walkto_landmarks_lumi[][WALKTO_LANDMARKS_FIELDS] = {
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

bool transport_locale_valid(int type, int locale)
{
  if (locale < 0)
    return false;
  if (type == TRAVEL_CARRIAGE || type == TRAVEL_OVERLAND_FLIGHT)
    return (size_t)locale + 1 < sizeof(carriage_locales_lumi) / sizeof(carriage_locales_lumi[0]);
  if (type == TRAVEL_SAILING || type == TRAVEL_OVERLAND_FLIGHT_SAIL)
    return (size_t)locale + 1 < sizeof(sailing_locales_lumi) / sizeof(sailing_locales_lumi[0]);
  return false;
}

static bool enter_transport_paid(struct char_data *ch, int locale, int type, int here, int fare);

ACMDU(do_carriage)
{
  skip_spaces(&argument);

  int i = 0, cost = 0;
  bool found = false;

  while (get_carriage_locale_vnum(i) != 0)
  {
    if (GET_ROOM_VNUM(IN_ROOM(ch)) == (room_vnum)get_carriage_locale_vnum(i))
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
    send_to_char(ch, "%-30s %4s %10s %10s (%s)\r\n", "Carriage Destination:", "Cost", "Distance",
                 "Time (sec)", "Area Note");
    int j = 0;
    for (j = 0; j < 80; j++)
      send_to_char(ch, "~");
    send_to_char(ch, "\r\n");
    while (get_carriage_locale_vnum(i) != 0)
    {
      if (GET_ROOM_VNUM(IN_ROOM(ch)) != (room_vnum)get_carriage_locale_vnum(i) &&
          ((here != 999) ? (get_carriage_locale_region(here) == get_carriage_locale_region(i))
                         : TRUE))
      {
        found = true;
        send_to_char(ch, "%-30s %4d %10d %10d (%s)\r\n", get_transport_carriage_name(i),
                     get_carriage_locale_cost(i), get_distance(ch, i, here, TRAVEL_CARRIAGE),
                     get_travel_time(ch, 5, i, here, TRAVEL_CARRIAGE),
                     get_carriage_locale_notes(i));
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
    while (get_carriage_locale_vnum(i) != 0)
    {
      if (GET_ROOM_VNUM(IN_ROOM(ch)) != (room_vnum)get_carriage_locale_vnum(i) &&
          ((here != 999) ? (get_carriage_locale_region(here) == get_carriage_locale_region(i))
                         : TRUE))
      {
        if (is_abbrev(argument, get_transport_carriage_name(i)))
        {
          found = true;
          if (here != 999 &&
              (get_carriage_locale_vnum(here) != 30036 && get_carriage_locale_vnum(here) != 30037))
          {
            cost = get_carriage_locale_cost(i);
            if (GET_GOLD(ch) < cost)
            {
              send_to_char(ch, "You are denied entry as you cannot pay the fee of %d.\r\n", cost);
              return;
            }
          }
          (void)enter_transport_paid(ch, i, TRAVEL_CARRIAGE, here, cost);
          return;
        }
      }
      i++;
    }
    if (!found)
    {
      send_to_char(ch, "There is no carriage destination in this nation by that name.  Type "
                       "carriage by itself to see a list of destinations.\r\n");
      return;
    }
  }
}

ACMDU(do_sail)
{
  skip_spaces(&argument);

  int i = 0, cost;
  bool found = false;

  while (get_sailing_locale_vnum(i) != 0)
  {
    if (GET_ROOM_VNUM(IN_ROOM(ch)) == (room_vnum)get_sailing_locale_vnum(i))
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
    send_to_char(ch, "%-30s %6s %8s %8s  %-45s\r\n", "Destination", "Cost", "Dist", "Time", "Note");
    send_to_char(ch, "%-30s %6s %8s %8s  %-45s\r\n", "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~", "~~~~",
                 "~~~~", "~~~~", "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    while (get_sailing_locale_vnum(i) != 0)
    {
      if (GET_ROOM_VNUM(IN_ROOM(ch)) != (room_vnum)get_sailing_locale_vnum(i) &&
          valid_sailing_travel(here, i))
      {
        found = true;
        cost = get_sailing_locale_cost(i);
        if (HAS_FEAT(ch, FEAT_BG_SAILOR))
          cost = 0;
        /* Trim the long area note for a cleaner table */
        const char *full_note = get_sailing_locale_notes(i);
        char note_buf[64];
        snprintf(note_buf, sizeof(note_buf), "%.60s", full_note ? full_note : "");
        if (full_note && strlen(full_note) > 60)
        {
          size_t len = strlen(note_buf);
          if (len > 3)
          {
            note_buf[len - 3] = '.';
            note_buf[len - 2] = '.';
            note_buf[len - 1] = '.';
          }
        }

        send_to_char(ch, "%-30s %6d %8d %8d  %-45s\r\n", get_transport_sailing_name(i), cost,
                     get_distance(ch, i, here, TRAVEL_SAILING),
                     get_travel_time(ch, 10, i, here, TRAVEL_SAILING), note_buf);
      }
      i++;
    }

    if (found)
    {
      send_to_char(ch, "\r\nTo sail somewhere, type sail <name of destination>\r\n");
      send_to_char(ch, "\r\nYou can view our world map online at "
                       "https://luminarimud.com/new-revised-worldmap-eat-your-heart-out/\r\n");

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
    while (get_sailing_locale_vnum(i) != 0)
    {
      if (GET_ROOM_VNUM(IN_ROOM(ch)) != (room_vnum)get_sailing_locale_vnum(i) &&
          valid_sailing_travel(here, i))
      {
        if (is_abbrev(argument, get_transport_sailing_name(i)))
        {
          found = true;
          cost = get_sailing_locale_cost(i);
          if (HAS_FEAT(ch, FEAT_BG_SAILOR))
            cost = 0;
          if (GET_GOLD(ch) < cost)
          {
            send_to_char(ch, "You cannot afford the sailing fare of %d coins.\r\n", cost);
            return;
          }
          (void)enter_transport_paid(ch, i, TRAVEL_SAILING, here, cost);
          return;
        }
      }
      i++;
    }
    if (!found)
    {
      send_to_char(ch, "There is no sailing destination by that name.  Type sail by itself to see "
                       "a list of destinations.\r\n");
      return;
    }
  }
}

int valid_sailing_travel(int here __attribute__((unused)), int i __attribute__((unused)))
{
  // When sailing is set up, this will make any checks necessary to allow sailing travel from the existing locale

  return true;
}

static bool enter_transport_paid(struct char_data *ch, int locale, int type, int here, int fare)
{
  room_rnum taxi = NOWHERE, destination, index;
  struct follow_type *f;
  struct char_data *passenger;
  struct domain_entity_handle origin, leader, transit, *passengers;
  struct room_data *transit_room;
  size_t capacity = 1, count = 0, i;
  char target[32];
  const char *name;

  if (ch == NULL || IS_NPC(ch) || !transport_locale_valid(type, locale) || fare < 0 ||
      GET_GOLD(ch) < fare || transport_is_transit_room(IN_ROOM(ch)))
    return false;
  for (index = 0; index <= top_of_world; index++)
    if (transport_is_transit_room(index) && world[index].people == NULL)
    {
      taxi = index;
      break;
    }
  if (taxi == NOWHERE)
  {
    send_to_char(ch, "No transport is available right now.\r\n");
    return false;
  }
  snprintf(target, sizeof(target), "%d",
           type == TRAVEL_SAILING || type == TRAVEL_OVERLAND_FLIGHT_SAIL
               ? get_sailing_locale_vnum(locale)
               : get_carriage_locale_vnum(locale));
  destination = find_target_room(ch, target);
  if (destination == NOWHERE)
  {
    send_to_char(ch, "That transport destination is unavailable. Please report it to staff.\r\n");
    return false;
  }
  for (f = ch->followers; f != NULL; f = f->next)
    capacity++;
  passengers = calloc(capacity, sizeof(*passengers));
  if (passengers == NULL)
    return false;
  origin = domain_event_room_handle(IN_ROOM(ch));
  transit = domain_event_room_handle(taxi);
  leader = domain_event_character_handle(ch);
  if (type == TRAVEL_CARRIAGE || type == TRAVEL_SAILING)
    for (f = ch->followers; f != NULL; f = f->next)
    {
      passenger = f->follower;
      if (passenger != NULL && !IS_NPC(passenger) && passenger->desc != NULL &&
          STATE(passenger->desc) == CON_PLAYING && IN_ROOM(passenger) == IN_ROOM(ch) &&
          is_player_grouped(ch, passenger) && FIGHTING(passenger) == NULL &&
          GET_POS(passenger) >= POS_STANDING)
        passengers[count++] = domain_event_character_handle(passenger);
    }
  passengers[count++] = leader;
  /* Admit every passenger before charging or invoking any movement callback. */
  for (i = 0; i < count; i++)
  {
    passenger = domain_event_world_resolve_character(passengers[i]);
    if (passenger == NULL ||
        !transport_job_start(passenger, taxi, destination,
                             get_travel_time(passenger, 10, locale, here, type), type, locale))
    {
      while (i > 0)
      {
        passenger = domain_event_world_resolve_character(passengers[--i]);
        transport_job_cancel(passenger, false);
      }
      send_to_char(ch, "Your journey could not be scheduled. No fare was charged.\r\n");
      free(passengers);
      return false;
    }
  }
  if (fare != 0)
  {
    GET_GOLD(ch) -= fare;
    send_to_char(ch, "You pay the transport fare of %d coins.\r\n", fare);
  }
  name = type == TRAVEL_SAILING || type == TRAVEL_OVERLAND_FLIGHT_SAIL
             ? get_transport_sailing_name(locale)
             : get_transport_carriage_name(locale);
  for (i = 0; i < count; i++)
  {
    passenger = domain_event_world_resolve_character(passengers[i]);
    if (passenger == NULL)
      continue;
    transit_room = domain_event_resolve(domain_event_runtime_bus(), transit, DOMAIN_ENTITY_ROOM);
    if (transit_room == NULL ||
        !domain_entity_handle_equal(domain_event_room_handle(IN_ROOM(passenger)), origin))
    {
      transport_job_cancel(passenger, false);
      continue;
    }
    taxi = (room_rnum)(transit_room - world);
    send_to_char(passenger, "You begin your journey to %s.\r\n\r\n", name);
    if (type == TRAVEL_CARRIAGE)
      act("$n boards a carriage which heads off into the distance.", FALSE, passenger, NULL, NULL,
          TO_ROOM);
    else if (type == TRAVEL_SAILING)
      act("$n boards a caravel which sails off into the distance.", FALSE, passenger, NULL, NULL,
          TO_ROOM);
    else
      act("$n leaps into the air and flies off into the distance.", FALSE, passenger, NULL, NULL,
          TO_ROOM);
    char_from_room(passenger);
    char_to_room_cause(passenger, taxi, domain_event_world_resolve_character(leader),
                       DOMAIN_RELOCATION_TRANSPORT, -1);
    passenger = domain_event_world_resolve_character(passengers[i]);
    if (passenger == NULL ||
        !domain_entity_handle_equal(domain_event_room_handle(IN_ROOM(passenger)), transit))
      continue;
    char_pets_to_char_loc(passenger);
    passenger = domain_event_world_resolve_character(passengers[i]);
    if (passenger == NULL ||
        !domain_entity_handle_equal(domain_event_room_handle(IN_ROOM(passenger)), transit))
      continue;
    look_at_room(passenger, 0);
    entry_memory_mtrigger(passenger);
    passenger = domain_event_world_resolve_character(passengers[i]);
    if (passenger == NULL ||
        !domain_entity_handle_equal(domain_event_room_handle(IN_ROOM(passenger)), transit))
      continue;
    greet_mtrigger(passenger, -1);
    passenger = domain_event_world_resolve_character(passengers[i]);
    if (passenger == NULL ||
        !domain_entity_handle_equal(domain_event_room_handle(IN_ROOM(passenger)), transit))
      continue;
    greet_memory_mtrigger(passenger);
  }
  free(passengers);
  return true;
}

void enter_transport(struct char_data *ch, int locale, int type, int here)
{
  (void)enter_transport_paid(ch, locale, type, here, 0);
}

int get_distance(struct char_data *ch, int locale, int here, int type)
{
  int xf = 0, xt = 0, yf = 0, yt = 0;
  int dx, dy;
  int total;
  int dist;

  if (type == TRAVEL_CARRIAGE)
  {
    xf = get_carriage_locale_x(here);
    xt = get_carriage_locale_x(locale);
    yf = get_carriage_locale_y(here);
    yt = get_carriage_locale_y(locale);
  }
  else if (type == TRAVEL_SAILING)
  {
    xf = get_sailing_locale_x(here);
    xt = get_sailing_locale_x(locale);
    yf = get_sailing_locale_y(here);
    yt = get_sailing_locale_y(locale);
  }
  else if (type == TRAVEL_OVERLAND_FLIGHT)
  {
    xf = ch->coords[0];
    xt = get_carriage_locale_x(locale);
    yf = ch->coords[1];
    yt = get_carriage_locale_y(locale);
  }
  else if (type == TRAVEL_OVERLAND_FLIGHT_SAIL)
  {
    xf = ch->coords[0];
    xt = get_sailing_locale_x(locale);
    yf = ch->coords[1];
    yt = get_sailing_locale_y(locale);
  }

  dx = xt - xf;
  dy = yt - yf;

  total = pow(dx, 2) + pow(dy, 2);
  dist = sqrt(total);

  return dist / 2;
}

int get_travel_time(struct char_data *ch, int speed, int locale, int here, int type)
{
  int distance = get_distance(ch, locale, here, type);

  distance *= 10;

  if (speed == 0)
    speed = 2;


  distance /= speed;

  distance /= 2;


  if (HAS_FEAT(ch, FEAT_BG_SAILOR))
    distance /= 2;

  return distance;
}

ACMDU(do_walkto)
{
  switch (CONFIG_LANDMARK_SYSTEM)
  {
  case LANDMARK_SYSTEM_WORLD:
    do_walkto_full(ch, argument, cmd, subcmd);
    break;
  case LANDMARK_SYSTEM_CITIES:
    do_walkto_city(ch, argument, cmd, subcmd);
    break;
  default:
    send_to_char(ch, "This command is not implemented yet.\r\n");
    return;
  }
}

ACMDU(do_landmarks)
{
  char arg[MAX_INPUT_LENGTH];

  switch (CONFIG_LANDMARK_SYSTEM)
  {
  case LANDMARK_SYSTEM_WORLD:
    one_argument(argument, arg, sizeof(arg));
    if (*arg && is_abbrev(arg, "city"))
      do_landmarks_city(ch, argument, cmd, subcmd);
    else
      do_landmarks_full(ch, argument, cmd, subcmd);
    break;
  case LANDMARK_SYSTEM_CITIES:
    do_landmarks_city(ch, argument, cmd, subcmd);
    break;
  default:
    send_to_char(ch, "This command is not implemented yet.\r\n");
    return;
  }
}

ACMDU(do_walkto_full)
{
  int i = 0, j = 0;
  bool found = false;
  int vnum = 0, specified = 0;
  int landmark = 0;
  char landmark_name[200];
  char specified_name[200];

  skip_spaces(&argument);

  if (!*argument)
  {
    send_to_char(ch, "You need to specify the landmark you wish to travel to.  Type 'LANDMARKS' "
                     "for a list.\r\n");
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
    send_to_char(ch, "You stop walking to '%s'",
                 get_walkto_landmark_name(walkto_vnum_to_list_row(GET_WALKTO_LOC(ch))));
    GET_WALKTO_LOC(ch) = 0;
    return;
  }

  while (get_walkto_landmark_region(i)[0] != '0')
  {
    vnum = get_walkto_landmark_vnum(i);
    specified = atoi(argument);
    if (vnum == specified)
    {
      landmark = get_walkto_landmark_vnum(i);
      found = true;
      break;
    }
    else
    {
      snprintf(landmark_name, sizeof(landmark_name), "%s", argument);
      for (j = 0; (size_t)j < strlen(landmark_name); j++)
      {
        landmark_name[j] = LOWER(landmark_name[j]);
      }

      snprintf(specified_name, sizeof(specified_name), "%s", get_walkto_landmark_name(i));
      for (j = 0; (size_t)j < strlen(specified_name); j++)
      {
        specified_name[j] = LOWER(specified_name[j]);
      }

      if (is_abbrev(landmark_name, specified_name))
      {
        landmark = get_walkto_landmark_vnum(i);
        found = true;
        break;
      }
    }
    i++;
  }

  if (!found)
  {
    send_to_char(ch, "That is not a valid landmark you can to travel to.  Type 'LANDMARKS' for a "
                     "list, and type: walkto (room #)\r\n");
    return;
  }

  GET_WALKTO_LOC(ch) = landmark;
  character_periodic_sync(ch);
  send_to_char(ch, "You begin walking to '%s'.\r\n",
               get_walkto_landmark_name(walkto_vnum_to_list_row(landmark)));
}

ACMDU(do_walkto_city)
{
  int i = 0;
  bool found = false;
  zone_vnum zone = 0;
  room_vnum landmark = 0;

  skip_spaces(&argument);

  if (!*argument)
  {
    send_to_char(ch, "You need to specify the landmark you wish to travel to.  Type 'LANDMARKS' "
                     "for a list.\r\n");
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
    send_to_char(ch, "You stop walking to the %s",
                 get_walkto_landmark_name(walkto_vnum_to_list_row(GET_WALKTO_LOC(ch))));
    GET_WALKTO_LOC(ch) = 0;
    return;
  }

  while ((zone = atoidx(get_walkto_landmark_region(i))) != 0)
  {
    if (zone == zone_table[world[IN_ROOM(ch)].zone].number)
    {
      if (is_abbrev(argument, get_walkto_landmark_name(i)))
      {
        landmark = get_walkto_landmark_vnum(i);
        found = true;
        break;
      }
    }
    i++;
  }

  if (!found)
  {
    send_to_char(
        ch, "That is not a valid landmark you to travel to.  Type 'LANDMARKS' for a list.\r\n");
    return;
  }

  GET_WALKTO_LOC(ch) = landmark;
  character_periodic_sync(ch);
  send_to_char(ch, "You begin walking to the %s.\r\n",
               get_walkto_landmark_name(walkto_vnum_to_list_row(landmark)));
}

zone_vnum get_walkto_landmark_region_vnum(const char *selector)
{
  const char *region_name;
  zone_rnum zone;
  zone_vnum region;
  int i;

  if (selector == NULL || !*selector)
    return NOWHERE;

  for (i = 0; get_walkto_landmark_region(i)[0] != '0'; i++)
  {
    region = atoidx(get_walkto_landmark_region(i));
    if (region <= 0)
      continue;

    if (is_number(selector))
    {
      if (atoidx(selector) == region)
        return region;
      continue;
    }

    zone = real_zone(region);
    if (zone == NOWHERE || zone_table[zone].name == NULL)
      continue;

    region_name = zone_table[zone].name;
    if (is_abbrev(selector, region_name) || isname(selector, region_name))
      return region;
  }

  return NOWHERE;
}

static void show_walkto_landmark_regions(struct char_data *ch)
{
  zone_rnum zone;
  zone_vnum region;
  bool already_listed;
  int i;
  int j;

  send_to_char(ch, "Available landmark areas:\r\n");

  for (i = 0; get_walkto_landmark_region(i)[0] != '0'; i++)
  {
    region = atoidx(get_walkto_landmark_region(i));
    if (region <= 0)
      continue;

    already_listed = false;
    for (j = 0; j < i; j++)
    {
      if (atoidx(get_walkto_landmark_region(j)) == region)
      {
        already_listed = true;
        break;
      }
    }
    if (already_listed)
      continue;

    zone = real_zone(region);
    if (zone != NOWHERE && zone_table[zone].name != NULL)
      send_to_char(ch, "  [%d] %s\r\n", region, zone_table[zone].name);
    else
      send_to_char(ch, "  [%d]\r\n", region);
  }

  send_to_char(ch, "\r\nUse 'landmarks <area name or zone number>' to list an area's landmarks.\r\n"
                   "Use 'landmarks city' to list landmarks in your current area.\r\n");
}

ACMD(do_landmarks_full)
{
  int i = 0, count = 0, dir = 0, distance = -1;
  room_rnum destination = NOWHERE;
  zone_vnum requested_region;
  bool found = false;
  char arg1[200], direction[50];

  if (IN_ROOM(ch) == NOWHERE)
  {
    send_to_char(ch, "You cannot use this command right now.\r\n");
    return;
  }

  one_argument(argument, arg1, sizeof(arg1));

  if (!*arg1)
  {
    show_walkto_landmark_regions(ch);
    return;
  }

  requested_region = get_walkto_landmark_region_vnum(arg1);
  if (requested_region == NOWHERE)
  {
    send_to_char(ch, "There is no landmark area matching '%s'.  Type 'landmarks' for a list.\r\n",
                 arg1);
    return;
  }

  while (get_walkto_landmark_region(i)[0] != '0')
  {
    if (atoidx(get_walkto_landmark_region(i)) == requested_region)
    {
      if (count == 0)
      {
        send_to_char(ch, "\tC%-35s | %6.6s | %-15s | %s\tn\r\n", "LANDMARK NAME", "ROOM #",
                     "DIRECTION", "DISTANCE");
      }
      distance = -1;
      destination = real_room(get_walkto_landmark_vnum(i));
      if (destination == NOWHERE)
        snprintf(direction, sizeof(direction), "Not Accessible From Here");
      else if ((dir = find_first_step(IN_ROOM(ch), destination)) == BFS_ALREADY_THERE)
        snprintf(direction, sizeof(direction), "You've Arrived!");
      else if (dir < 0)
        snprintf(direction, sizeof(direction), "Not Accessible");
      else
        snprintf(direction, sizeof(direction), "%s", dirs[dir]);
      if (destination != NOWHERE)
        distance = count_rooms_between(IN_ROOM(ch), destination);
      send_to_char(ch, "%-35s | %-6.6d | %-15s | %3d rooms\r\n", get_walkto_landmark_name(i),
                   get_walkto_landmark_vnum(i), direction, distance);
      found = true;
      count++;
    }
    i++;
  }

  send_to_char(ch, "\r\n");

  if (!found)
    send_to_char(ch, "There are no landmarks for that region.\r\n");
}

ACMD(do_landmarks_city)
{
  int i = 0, count = 0;
  zone_vnum zone = 0;
  bool found = false;

  if (IN_ROOM(ch) == NOWHERE)
  {
    send_to_char(ch, "You cannot use this command right now.\r\n");
    return;
  }

  while ((zone = atoidx(get_walkto_landmark_region(i))) != 0)
  {
    if (zone == zone_table[world[IN_ROOM(ch)].zone].number)
    {
      if (count == 0)
      {
        send_to_char(ch, "\tC%-25s - %s\tn\r\n", "LANDMARK NAME", "DESCRIPTION");
      }
      send_to_char(ch, "%-25s - %s\r\n", get_walkto_landmark_name(i), get_walkto_landmark_notes(i));
      found = true;
      count++;
    }
    i++;
  }

  send_to_char(ch, "\r\n");

  if (!found)
    send_to_char(ch, "There are no landmarks in this area.\r\n");
}


void process_walkto_action(struct char_data *ch)
{
  int dir = 0;
  room_rnum destination = NOWHERE;

  if (ch == NULL || IS_NPC(ch) || ch->player_specials == NULL || !GET_WALKTO_LOC(ch))
    return;
  if (ch->desc == NULL || STATE(ch->desc) != CON_PLAYING || IN_ROOM(ch) == NOWHERE)
  {
    GET_WALKTO_LOC(ch) = 0;
    return;
  }
  destination = real_room(GET_WALKTO_LOC(ch));
  if (destination == NOWHERE)
  {
    send_to_char(ch,
                 "Your walk to '%s' has been interrupted because that landmark is no longer "
                 "available.\r\n",
                 get_walkto_landmark_name(walkto_vnum_to_list_row(GET_WALKTO_LOC(ch))));
    GET_WALKTO_LOC(ch) = 0;
    return;
  }
  if ((dir = find_first_step(IN_ROOM(ch), destination)) < 0)
  {
    send_to_char(ch, "Your walk to '%s' has been interrupted %d.\r\n",
                 get_walkto_landmark_name(walkto_vnum_to_list_row(GET_WALKTO_LOC(ch))), dir);
    GET_WALKTO_LOC(ch) = 0;
    return;
  }

  perform_move(ch, dir, 1);
  if (IN_ROOM(ch) == destination)
  {
    send_to_char(ch, "You have arrived at the '%s' landmark.\r\n",
                 get_walkto_landmark_name(walkto_vnum_to_list_row(GET_WALKTO_LOC(ch))));
    GET_WALKTO_LOC(ch) = 0;
  }
  else if (GET_WALKTO_LOC(ch))
  {
    send_to_char(ch, "You continue walking to '%s'.  Type walkto cancel to stop.\r\n",
                 get_walkto_landmark_name(walkto_vnum_to_list_row(GET_WALKTO_LOC(ch))));
  }
}


int walkto_vnum_to_list_row(int vnum)
{
  if (vnum <= 0)
    return 0;

  int i = 0;

  while (get_walkto_landmark_region(i)[0] != '0')
  {
    if (get_walkto_landmark_vnum(i) == vnum)
    {
      return i;
    }
    i++;
  }
  return 0; // not found
}

#if !defined(USE_WALKTO_LANDMARKS) && !defined(USE_CITY_LANDMARKS_ONLY)
/* Stub function when neither USE_WALKTO_LANDMARKS nor USE_CITY_LANDMARKS_ONLY is defined */
ACMD(do_landmarks)
{
  send_to_char(ch, "The landmarks system is not enabled on this MUD.\r\n");
}
#endif

/* EoF */
