/* ************************************************************************
 *      File:   vessels_narrative.c                  Part of LuminariMUD  *
 *   Purpose:   Contextual at-sea descriptions and vessel ambience.      *
 * ********************************************************************** */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "vessels.h"
#include "wilderness.h"
#include "systems/narrative_weaver/narrative_weaver.h"

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

static int vessel_narrative_ticks = 0;

/**
 * Return the player-facing name for one raw wilderness weather value.
 */
const char *vessel_weather_condition_name(int weather)
{
  if (weather < VESSEL_WEATHER_CLOUDY)
  {
    return "clear skies";
  }
  if (weather < VESSEL_WEATHER_FOG)
  {
    return "overcast skies";
  }
  if (weather < VESSEL_WEATHER_STORM)
  {
    return "rain";
  }
  if (weather < VESSEL_WEATHER_GALE)
  {
    return "a heavy storm";
  }
  return "a thunderstorm";
}

/**
 * Convert one raw wilderness weather value to vessel hazard severity.
 */
int vessel_weather_severity_from_value(int weather)
{
  if (weather >= VESSEL_WEATHER_GALE)
  {
    return 3;
  }
  if (weather >= VESSEL_WEATHER_STORM)
  {
    return 2;
  }
  if (weather >= VESSEL_WEATHER_SQUALL)
  {
    return 1;
  }
  return 0;
}

static const char *vessel_ambient_class_detail(enum vessel_class vessel_type, int z)
{
  switch (vessel_type)
  {
  case VESSEL_RAFT:
    return "The raft flexes over each passing swell.";
  case VESSEL_BOAT:
    return "The boat's light hull skips across the water.";
  case VESSEL_SHIP:
    return "The ship's timbers work with the sea.";
  case VESSEL_WARSHIP:
    return "The warship's armored hull shoulders through the water.";
  case VESSEL_AIRSHIP:
    return "The airship's envelope and rigging hum above the world.";
  case VESSEL_SUBMARINE:
    if (z < 0)
    {
      return "The submarine's pressure hull murmurs in the deep.";
    }
    return "The submarine's low hull parts the surface.";
  case VESSEL_TRANSPORT:
    return "The transport's laden hull rolls with deliberate weight.";
  case VESSEL_MAGICAL:
    return "Arcane currents shimmer along the magical vessel.";
  default:
    return NULL;
  }
}

static const char *vessel_ambient_speed_detail(int speed, int maxspeed)
{
  int speed_percent;

  if (speed <= 0)
  {
    return "It lies still.";
  }
  if (maxspeed <= 0)
  {
    return "It makes cautious headway.";
  }

  speed_percent = speed * 100 / maxspeed;
  if (speed_percent <= 25)
  {
    return "It makes cautious headway.";
  }
  if (speed_percent <= 70)
  {
    return "It holds a steady pace.";
  }
  return "It drives at near full speed.";
}

static const char *vessel_ambient_weather_detail(int weather, bool submerged)
{
  if (submerged)
  {
    if (weather < VESSEL_WEATHER_CLOUDY)
    {
      return "The calm surface reaches the hull as a faint, even pressure.";
    }
    if (weather < VESSEL_WEATHER_FOG)
    {
      return "The overcast surface is only a muted change in the water.";
    }
    if (weather < VESSEL_WEATHER_STORM)
    {
      return "Surface rain is reduced to a soft patter of pressure.";
    }
    if (weather < VESSEL_WEATHER_GALE)
    {
      return "The storm above arrives as slow pressure pulses through the deep.";
    }
    return "Thunder above reaches the hull as distant, rolling pressure.";
  }

  if (weather < VESSEL_WEATHER_CLOUDY)
  {
    return "Clear light runs cleanly to the horizon.";
  }
  if (weather < VESSEL_WEATHER_FOG)
  {
    return "Cloud cover flattens the light across the horizon.";
  }
  if (weather < VESSEL_WEATHER_STORM)
  {
    return "Rain stipples the surrounding water.";
  }
  if (weather < VESSEL_WEATHER_GALE)
  {
    return "Storm winds drive dark water across the deck.";
  }
  return "Lightning throws the vessel and waves into sharp relief.";
}

/**
 * Build one deterministic class-, weather-, and speed-aware ambient line.
 */
bool vessel_build_ambient_message(enum vessel_class vessel_type, int weather, int speed,
                                  int maxspeed, int z, char *output, size_t output_size)
{
  const char *class_detail;
  const char *speed_detail;
  const char *weather_detail;
  int written;

  if (output == NULL || output_size == 0)
  {
    return FALSE;
  }
  output[0] = '\0';

  class_detail = vessel_ambient_class_detail(vessel_type, z);
  if (class_detail == NULL)
  {
    return FALSE;
  }
  speed_detail = vessel_ambient_speed_detail(speed, maxspeed);
  weather_detail = vessel_ambient_weather_detail(weather, vessel_type == VESSEL_SUBMARINE && z < 0);

  written = snprintf(output, output_size, "%s %s %s", class_detail, speed_detail, weather_detail);
  return written >= 0 && (size_t)written < output_size;
}

static const char *vessel_description_class_name(enum vessel_class vessel_type)
{
  static const char *class_names[NUM_VESSEL_TYPES] = {
      "raft", "boat", "ship", "warship", "airship", "submarine", "transport", "magical vessel"};

  if (vessel_type < 0 || vessel_type >= NUM_VESSEL_TYPES)
  {
    return NULL;
  }
  return class_names[vessel_type];
}

static const char *vessel_description_speed(int speed, int maxspeed)
{
  int speed_percent;

  if (speed <= 0)
  {
    return "lying still";
  }
  if (maxspeed <= 0)
  {
    return "making cautious headway";
  }

  speed_percent = speed * 100 / maxspeed;
  if (speed_percent <= 25)
  {
    return "making cautious headway";
  }
  if (speed_percent <= 70)
  {
    return "holding steady way";
  }
  return "running near full speed";
}

/**
 * Build the compact LOOKOUT description and add one geographic region hint.
 */
char *vessel_create_at_sea_description(struct char_data *ch, const struct greyhawk_ship_data *ship)
{
  const char *class_name;
  const char *article;
  const char *speed_description;
  const char *weather_name;
  char base_description[512];
  zone_rnum zone;
  int ship_x;
  int ship_y;
  int ship_z;
  int weather;

  (void)ch;
  if (!is_valid_ship(ship))
  {
    return NULL;
  }

  class_name = vessel_description_class_name(ship->vessel_type);
  if (class_name == NULL)
  {
    return NULL;
  }

  ship_x = vessel_autopilot_grid_coordinate(ship->x);
  ship_y = vessel_autopilot_grid_coordinate(ship->y);
  ship_z = vessel_autopilot_grid_coordinate(ship->z);
  weather = get_weather(ship_x, ship_y);
  speed_description = vessel_description_speed(ship->speed, ship->maxspeed);
  weather_name = vessel_weather_condition_name(weather);
  article = ship->vessel_type == VESSEL_AIRSHIP ? "An" : "A";

  if (ship->vessel_type == VESSEL_SUBMARINE && ship_z < 0)
  {
    snprintf(base_description, sizeof(base_description),
             "A submerged %s is %s through the deep, sheltered beneath %s.", class_name,
             speed_description, weather_name);
  }
  else
  {
    snprintf(base_description, sizeof(base_description), "%s %s is %s under %s.", article,
             class_name, speed_description, weather_name);
  }

  zone = real_zone(WILD_ZONE_VNUM);
  return weave_vessel_wilderness_description(base_description, zone, ship_x, ship_y);
}

static bool vessel_has_connected_player(const struct greyhawk_ship_data *ship)
{
  struct descriptor_data *descriptor;

  for (descriptor = descriptor_list; descriptor != NULL; descriptor = descriptor->next)
  {
    if (STATE(descriptor) == CON_PLAYING && descriptor->character != NULL &&
        !IS_NPC(descriptor->character) &&
        get_ship_from_room(IN_ROOM(descriptor->character)) == ship)
    {
      return TRUE;
    }
  }
  return FALSE;
}

static bool vessel_narrative_broadcast(struct greyhawk_ship_data *ship)
{
  char message[MAX_STRING_LENGTH];
  int weather;
  int ship_x;
  int ship_y;
  int ship_z;

  if (!is_valid_ship(ship))
  {
    return FALSE;
  }

  ship_x = vessel_autopilot_grid_coordinate(ship->x);
  ship_y = vessel_autopilot_grid_coordinate(ship->y);
  ship_z = vessel_autopilot_grid_coordinate(ship->z);
  weather = get_weather(ship_x, ship_y);
  if (!vessel_build_ambient_message(ship->vessel_type, weather, ship->speed, ship->maxspeed, ship_z,
                                    message, sizeof(message)))
  {
    return FALSE;
  }

  send_to_ship(ship, "%s", message);
  return TRUE;
}

/**
 * Periodically narrate occupied moving vessels without touching empty fleets.
 */
void vessel_narrative_tick(void)
{
  struct greyhawk_ship_data *ship;
  int i;

  vessel_narrative_ticks++;
  if (vessel_narrative_ticks < VESSEL_NARRATIVE_INTERVAL)
  {
    return;
  }
  vessel_narrative_ticks = 0;

  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
  {
    ship = &greyhawk_ships[i];
    if (!is_valid_ship(ship) || ship->speed <= 0 || !vessel_has_connected_player(ship))
    {
      continue;
    }
    vessel_narrative_broadcast(ship);
  }
}

/**
 * Let staff exercise the same ambient path immediately on one vessel.
 */
bool vessel_narrative_force_ship(struct greyhawk_ship_data *ship)
{
  return vessel_narrative_broadcast(ship);
}
