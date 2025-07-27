/**
 * @file weather.c                          LuminariMUD
 * Functions that handle the in game progress of time and weather changes.
 *
 * Part of the core tbaMUD source code distribution, which is a derivative
 * of, and continuation of, CircleMUD.
 *
 * All rights reserved.  See license for complete information.
 * Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
 * CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"

#define NUM_WEATHER_CHANGES 6

static void another_hour(int mode);
static void weather_change(void);
void send_weather(int weather_change);

/** Call this function every mud hour to increment the gametime (by one hour)
 * and the weather patterns.
 * @param mode Really, this parameter has the effect of a boolean. In the
 * current incarnation of the function and utility functions, as long as mode
 * is non-zero, the gametime will increment one hour and the weather will be
 * changed.
 */
void weather_and_time(int mode)
{
  another_hour(mode);
  if (mode)
    weather_change();
}

/* a nice little cheesy function used to reset dailies, currently
 dailies are resetting every 6 game hours */
void reset_dailies()
{
  struct char_data *ch = NULL, *next_ch = NULL;
  int changes = 0;

  /* reset dailies (like shapechange) */
  for (ch = character_list; ch; ch = next_ch)
  {
    next_ch = ch->next; /* Cache next char before potential extraction */
    
    if (!ch)
      continue;

    if (IS_NPC(ch))
      continue;

    if (CLASS_LEVEL(ch, CLASS_DRUID) < 5)
      continue;

    changes = CLASS_LEVEL(ch, CLASS_DRUID) / 3;
    if (GET_SHAPECHANGES(ch) < changes)
    {
      GET_SHAPECHANGES(ch) = changes;
      send_to_char(ch, "Your shapechanges refresh to:  %d!\r\n", changes);
    }
  }
}

/** Increment the game time by one hour (no matter what) and display any time
 * dependent messages via send_to_outdoors() (if parameter is non-zero).
 * @param mode Really, this parameter has the effect of a boolean. If non-zero,
 * display day/night messages to all eligible players.
 */
static void another_hour(int mode)
{

  time_info.hours++;

  if (mode)
  {
    switch (time_info.hours)
    {
    case 1:
      /* we are resetting dailies (such as shapechange) every 6 game hours */
      reset_dailies();
      break;
    case 5:
      weather_info.sunlight = SUN_RISE;
      send_to_outdoor("\tyThe \tYsun\ty rises in the east.\tn\r\n");
      break;
    case 6:
      weather_info.sunlight = SUN_LIGHT;
      send_to_outdoor("\tYThe day has begun.\tn\r\n");
      break;
    case 7:
      /* we are resetting dailies (such as shapechange) every 6 game hours */
      reset_dailies();
      break;
    case 12:
      send_to_outdoor("\tYThe sun is at its \tWzenith\tY.\tn\r\n");
      break;
    case 13:
      /* we are resetting dailies (such as shapechange) every 6 game hours */
      reset_dailies();
      break;
    case 19:
      /* we are resetting dailies (such as shapechange) every 6 game hours */
      reset_dailies();
      break;
    case 21:
      weather_info.sunlight = SUN_SET;
      send_to_outdoor("\tDThe \tYsun\tD slowly disappears in the west.\tn\r\n");
      break;
    case 22:
      weather_info.sunlight = SUN_DARK;
      send_to_outdoor("\tDThe night has begun.\tn\r\n");
      break;
    case 24:
      send_to_outdoor("\tDThe \tRintense\tD darkness of midnight envelops you.\tn\r\n");
      break;
    default:
      break;
    }
  }
  if (time_info.hours > 23)
  { /* Changed by HHS due to bug ??? */
    time_info.hours -= 24;
    time_info.day++;

    if (time_info.day > 34)
    {
      time_info.day = 0;
      time_info.month++;

      if (time_info.month > 16)
      {
        time_info.month = 0;
        time_info.year++;
      }
    }
  }
}

/** Controls the in game weather system. If the weather changes, an information
 * update is sent via send_to_outdoors().
 * @todo There are some hard coded values that could be extracted to make
 * customizing the weather patterns easier.
 */
static void weather_change(void)
{
  int diff, change;

  if ((time_info.month >= 9) && (time_info.month <= 16))
    diff = (weather_info.pressure > 985 ? -2 : 2);
  else
    diff = (weather_info.pressure > 1015 ? -2 : 2);

  weather_info.change += (dice(1, 4) * diff + dice(2, 6) - dice(2, 6));

  weather_info.change = MIN(weather_info.change, 12);
  weather_info.change = MAX(weather_info.change, -12);

  weather_info.pressure += weather_info.change;

  weather_info.pressure = MIN(weather_info.pressure, 1040);
  weather_info.pressure = MAX(weather_info.pressure, 960);

  change = 0;

  switch (weather_info.sky)
  {
  case SKY_CLOUDLESS:
    if (weather_info.pressure < 990)
      change = 1;
    else if (weather_info.pressure < 1010)
      if (dice(1, 4) == 1)
        change = 1;
    break;
  case SKY_CLOUDY:
    if (weather_info.pressure < 970)
      change = 2;
    else if (weather_info.pressure < 990)
    {
      if (dice(1, 4) == 1)
        change = 2;
      else
        change = 0;
    }
    else if (weather_info.pressure > 1030)
      if (dice(1, 4) == 1)
        change = 3;

    break;
  case SKY_RAINING:
    if (weather_info.pressure < 970)
    {
      if (dice(1, 4) == 1)
        change = 4;
      else
        change = 0;
    }
    else if (weather_info.pressure > 1030)
      change = 5;
    else if (weather_info.pressure > 1010)
      if (dice(1, 4) == 1)
        change = 5;

    break;
  case SKY_LIGHTNING:
    if (weather_info.pressure > 1010)
      change = 6;
    else if (weather_info.pressure > 990)
      if (dice(1, 4) == 1)
        change = 6;

    break;
  default:
    change = 0;
    weather_info.sky = SKY_CLOUDLESS;
    break;
  }

  /* the result is 6 different weather transitions */
  send_weather(change);

  switch (change)
  {
  case 0:
    break;
  case 1:
    //    send_to_outdoor("\tcThe sky starts to get cloudy.\tn\r\n");
    weather_info.sky = SKY_CLOUDY;
    break;
  case 2:
    //    send_to_outdoor("It starts to \tbrain\tn.\r\n");
    weather_info.sky = SKY_RAINING;
    break;
  case 3:
    //    send_to_outdoor("\tCThe clouds disappear.\tn\r\n");
    weather_info.sky = SKY_CLOUDLESS;
    break;
  case 4:
    //    send_to_outdoor("\tBLightning\tn starts to show in the sky.\r\n");
    weather_info.sky = SKY_LIGHTNING;
    break;
  case 5:
    //    send_to_outdoor("The \tbrain\tn stops.\r\n");
    weather_info.sky = SKY_CLOUDY;
    break;
  case 6:
    //    send_to_outdoor("The \tBlightning\tn stops.\r\n");
    weather_info.sky = SKY_RAINING;
    break;
  default:
    //    send_to_outdoor("\twThe weather remains unchanging.\tn\r\n");
    break;
  }
}

/* sector-type weather messages by Jamdog and Guilem */
bool sect_no_weather(struct char_data *ch)
{
  int s_type;

  s_type = world[IN_ROOM(ch)].sector_type;

  switch (s_type)
  {
  case SECT_INSIDE:
  case SECT_CITY:
  case SECT_UNDERWATER:
  case SECT_PLANES:
  case SECT_UD_WILD:
  case SECT_UD_CITY:
  case SECT_UD_INSIDE:
  case SECT_UD_WATER:
  case SECT_UD_NOSWIM:
  case SECT_UD_NOGROUND:
  case SECT_LAVA:
    // case SECT_UNDERGROUND:
    return TRUE;
  }

  return FALSE;
}

struct weather_msg
{
  int sector_type;
  char msg[NUM_WEATHER_CHANGES][MEDIUM_STRING];
} weather_messages[] = {

    {SECT_TUNDRA,
     {"A cool wind passes through the icy landscape, whipping up snow.",
      "The wind settles down and the sun shines through the heavy cloud cover.",
      "The icy wind grows stronger and snow begins to fall, whipping your body mercilessly.",
      "The wind becomes less fierce and it stops snowing.",
      "A blizzard rolls in, strong winds and biting cold beating at you.",
      "The blizzard has passed, but icy winds continue to whip at you."}},

    {SECT_DESERT,
     {"A searing wind starts to blow, sand billowing on the dunes.",
      "The sun starts to bake at you through a cloudless sky.",
      "Hot grains of sand stings your face as the searing wind blows hard.",
      "The searing hot wind calms itself slightly, but that doesn't make it any less hot.",
      "A mighty sandstorm rolls in, blistering sand beating at you mercilessly.",
      "The sandstorm moves on, all tracks covered and new dunes formed by the unrelenting wind."}},

    {SECT_WATER_SWIM,
     {"Soft fluffy clouds begin to drift in over the watery expanse.",
      "The soft fluffy clouds dissipate and the sun shines through.",
      "Heavy droplets begin to fall, the clouds above growing thicker.",
      "The heavy droplets of water stops falling and the cloud cover grows thinner.",
      "Loud booms announce the arrival of a fierce thunderstorm.",
      "The rolling thunder stops, but the heavy raincover remains."}},

    {SECT_WATER_NOSWIM,
     {"Soft fluffy clouds begin to drift in over the watery expanse.",
      "The soft fluffy clouds dissipate and the sun shines through.",
      "Heavy droplets begin to fall, the clouds above growing thicker.",
      "The heavy droplets of water stops falling and the cloud cover grows thinner.",
      "Loud booms announce the arrival of a fierce thunderstorm.",
      "The rolling thunder stops, but the heavy raincover remains."}},

    {SECT_OCEAN,
     {"Soft fluffy clouds begin to drift in over the watery expanse.",
      "The soft fluffy clouds dissipate and the sun shines through.",
      "Heavy droplets begin to fall, the clouds above growing thicker.",
      "The heavy droplets of water stops falling and the cloud cover grows thinner.",
      "Loud booms announce the arrival of a fierce thunderstorm.",
      "The rolling thunder stops, but the heavy raincover remains."}},

    {SECT_MARSHLAND,
     {"The sun becomes obscured between the openings in the canopy above as heavy clouds roll in.",
      "The mucky ground below you becomes spotty with sun beams as it shines through the canopy above.",
      "Heavy droplets begin to fall, the clouds above growing thicker, darkening the swamp.",
      "The heavy droplets of water stops falling and the cloud cover grows thinner.",
      "Loud booms announce the arrival of a fierce thunderstorm.",
      "The rolling thunder stops, but the heavy rain cover remains."}},

    {SECT_BEACH,
     {"Soft fluffy clouds begin to drift in over the beach.",
      "The soft fluffy clouds dissipate and the sun shines through.",
      "Heavy droplets begin to fall, the clouds above growing thicker.",
      "The heavy droplets of water stops falling and the cloud cover grows thinner.",
      "Loud booms announces the arrival of a fierce thunderstorm.",
      "The rolling thunder stops, but the heavy raincover remains."}},

    {SECT_FOREST,
     {"Soft fluffy clouds begin to drift in over the forested landscape.",
      "The soft fluffy clouds dissipate and the sun shines through the canopy above.",
      "Heavy droplets begin to fall, the clouds above growing thicker.",
      "The heavy droplets of water stops falling and the cloud cover grows thinner.",
      "Loud booms announce the arrival of a fierce thunderstorm.",
      "The rolling thunder stops, but the heavy raincover remains."}},

    {SECT_JUNGLE,
     {"Great fluffy clouds roll over the rainforest.",
      "The clouds dissipate and sun shines through the canopy overhead.",
      "Huge heavy drops of water starts to fall, making your clothing soggy.",
      "It stops raining but the heavy cloudcover overhead remains.",
      "Thunder and lightening arches across the sky as a monsoon rolls over the rainforest.",
      "The rolling thunder quiets down and the rain becomes a little less intense."}},

    {SECT_HILLS,
     {"An icy wind starts to blow, whipping you and stirring up the clouds above.",
      "The cloudcover dissipates and the whipping wind calms itself.",
      "Cold, heavy drops announces the arrival of a mountain rain.",
      "The rain stops, but the cool wind continues to whip at you.",
      "Earsplitting booms echo through the mountains as it starts to thunder.",
      "The thunder stops but the cold rain and heavy winds keeps whipping at you."}},

    {SECT_MOUNTAIN,
     {"An icy wind starts to blow, whipping you and stirring up the clouds above.",
      "The cloudcover dissipates and the whipping wind calms itself.",
      "Cold, heavy drops announces the arrival of a mountain rain.",
      "The rain stops, but the cool wind continues to whip at you.",
      "Earsplitting booms echo through the mountains as it starts to thunder.",
      "The thunder stops but the cold rain and heavy winds keeps whipping at you."}},

    {SECT_HIGH_MOUNTAIN,
     {"An icy wind starts to blow, whipping you and stirring up the clouds above.",
      "The cloudcover dissipates and the whipping wind calms itself.",
      "Cold, heavy drops announces the arrival of a mountain rain.",
      "The rain stops, but the cool wind continues to whip at you.",
      "Earsplitting booms echo through the mountains as it starts to thunder.",
      "The thunder stops but the cold rain and heavy winds keeps whipping at you."}},

    {SECT_INSIDE, /* Always last - inside doesn't show weather, so default messages below */
     {"Soft fluffy clouds begin to drift in over the landscape.",
      "The soft fluffy clouds dissipate and the sun shines through.",
      "Heavy droplets begin to fall, the clouds above growing thicker.",
      "The heavy droplets of water stops falling and the cloud cover grows thinner.",
      "Loud booms announce the arrival of a fierce thunderstorm.",
      "The rolling thunder stops, but the heavy rain cover remains."}},
};

void send_weather(int weather_change)
{
  bool found = FALSE;
  struct descriptor_data *i = NULL;
  int j = 0;

  if ((weather_change < 1) || (weather_change > 6))
    return;

  for (i = descriptor_list; i; i = i->next)
  {

    if (!i)
      continue;

    if (STATE(i) != CON_PLAYING || i->character == NULL)
      continue;

    if (!AWAKE(i->character) || !OUTSIDE(i->character))
      continue;

    if (sect_no_weather(i->character))
      continue;

    if (IN_ROOM(i->character) != NOWHERE && (ZONE_FLAGGED(world[IN_ROOM(i->character)].zone, ZONE_WILDERNESS)))
      continue; /* Wilderness weather is handled elsewhere */

    for (j = 0; weather_messages[j].sector_type != SECT_INSIDE; j++)
    {
      if (weather_messages[j].sector_type ==
          world[IN_ROOM(i->character)].sector_type)
      {
        send_to_char(i->character, "%s\r\n", weather_messages[j].msg[(weather_change - 1)]);
        found = TRUE;
      }
    }

    /* Use default, which j is now pointing to */
    if (!found)
    {
      send_to_char(i->character, "%s\r\n", weather_messages[j].msg[(weather_change - 1)]);
    }
  }
}
