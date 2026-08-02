/* ************************************************************************
 *      File:   vessels_lookout.c                    Part of LuminariMUD  *
 *   Purpose:   Wilderness-backed vessel lookout view.                   *
 * ********************************************************************** */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h"
#include "constants.h"
#include "vessels.h"
#include "wilderness.h"

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];
extern int wild_waterline;

#define VESSEL_LOOKOUT_DIRECTION_COUNT 8
#define VESSEL_LOOKOUT_SAMPLE_LIMIT 6
#define VESSEL_LOOKOUT_CONTACT_LIMIT 10

struct vessel_lookout_direction
{
  const char *name;
  int delta_x;
  int delta_y;
};

struct vessel_lookout_contact
{
  int shipnum;
  float range;
  int bearing;
  int delta_z;
};

static const struct vessel_lookout_direction vessel_lookout_directions[] = {
    {"North", 0, 1},  {"Northeast", 1, 1},   {"East", 1, 0},  {"Southeast", 1, -1},
    {"South", 0, -1}, {"Southwest", -1, -1}, {"West", -1, 0}, {"Northwest", -1, 1}};

int vessel_lookout_sample_distances(int visibility, int *distances, int capacity)
{
  int half_visibility;
  int count;
  int distance;

  if (visibility <= 0 || distances == NULL || capacity <= 0)
  {
    return 0;
  }

  half_visibility = (visibility + 1) / 2;
  count = 0;
  for (distance = 1; distance <= visibility && count < capacity; distance++)
  {
    if (distance == 1 || distance == 3 || distance == 5 || distance == 10 ||
        distance == half_visibility || distance == visibility)
    {
      distances[count++] = distance;
    }
  }
  return count;
}

int vessel_lookout_build_bands(const int *sectors, const int *distances, int sample_count,
                               struct vessel_lookout_band *bands, int capacity)
{
  int count;
  int i;

  if (sectors == NULL || distances == NULL || bands == NULL || sample_count <= 0 || capacity <= 0)
  {
    return 0;
  }

  count = 0;
  for (i = 0; i < sample_count; i++)
  {
    if (count > 0 && bands[count - 1].sector_type == sectors[i])
    {
      bands[count - 1].last_distance = distances[i];
      continue;
    }
    if (count >= capacity)
    {
      break;
    }
    bands[count].sector_type = sectors[i];
    bands[count].first_distance = distances[i];
    bands[count].last_distance = distances[i];
    count++;
  }
  return count;
}

const char *vessel_lookout_compass_direction(int bearing)
{
  bearing %= 360;
  if (bearing < 0)
  {
    bearing += 360;
  }

  if (bearing >= 337 || bearing < 23)
  {
    return "N";
  }
  if (bearing < 68)
  {
    return "NE";
  }
  if (bearing < 113)
  {
    return "E";
  }
  if (bearing < 158)
  {
    return "SE";
  }
  if (bearing < 203)
  {
    return "S";
  }
  if (bearing < 248)
  {
    return "SW";
  }
  if (bearing < 293)
  {
    return "W";
  }
  return "NW";
}

static const char *vessel_lookout_sector_name(int sector_type)
{
  if (sector_type < 0 || sector_type >= NUM_ROOM_SECTORS)
  {
    return "Unknown terrain";
  }
  return sector_types[sector_type];
}

static int vessel_lookout_sample_sector(int center_x, int center_y,
                                        const struct vessel_lookout_direction *direction,
                                        int distance)
{
  room_rnum static_room;
  int step;
  int sample_x;
  int sample_y;

  step = distance;
  if (direction->delta_x != 0 && direction->delta_y != 0)
  {
    step = MAX(1, (distance * 181 + 128) / 256);
  }
  sample_x = center_x + direction->delta_x * step;
  sample_y = center_y + direction->delta_y * step;

  static_room = find_static_room_by_coordinates(sample_x, sample_y);
  if (static_room != NOWHERE && static_room <= top_of_world)
  {
    return world[static_room].sector_type;
  }
  return get_modified_sector_type(real_zone(WILD_ZONE_VNUM), sample_x, sample_y);
}

static void vessel_lookout_render_direction(struct char_data *ch, int center_x, int center_y,
                                            int visibility, int direction_index)
{
  struct vessel_lookout_band bands[VESSEL_LOOKOUT_SAMPLE_LIMIT];
  int distances[VESSEL_LOOKOUT_SAMPLE_LIMIT];
  int sectors[VESSEL_LOOKOUT_SAMPLE_LIMIT];
  int sample_count;
  int band_count;
  int i;

  sample_count =
      vessel_lookout_sample_distances(visibility, distances, VESSEL_LOOKOUT_SAMPLE_LIMIT);
  for (i = 0; i < sample_count; i++)
  {
    sectors[i] = vessel_lookout_sample_sector(
        center_x, center_y, &vessel_lookout_directions[direction_index], distances[i]);
  }
  band_count = vessel_lookout_build_bands(sectors, distances, sample_count, bands,
                                          VESSEL_LOOKOUT_SAMPLE_LIMIT);

  send_to_char(ch, "  %-9s: ", vessel_lookout_directions[direction_index].name);
  if (band_count == 0)
  {
    send_to_char(ch, "nothing can be made out.\r\n");
    return;
  }
  if (band_count == 1)
  {
    send_to_char(ch, "%s from nearby to the %du horizon.\r\n",
                 vessel_lookout_sector_name(bands[0].sector_type), visibility);
    return;
  }

  send_to_char(ch, "%s nearby", vessel_lookout_sector_name(bands[0].sector_type));
  for (i = 1; i < band_count; i++)
  {
    send_to_char(ch, "; %s by %du", vessel_lookout_sector_name(bands[i].sector_type),
                 bands[i].first_distance);
  }
  send_to_char(ch, "; %s reaches the %du horizon.\r\n",
               vessel_lookout_sector_name(bands[band_count - 1].sector_type), visibility);
}

static int vessel_lookout_compare_contacts(const void *first, const void *second)
{
  const struct vessel_lookout_contact *first_contact;
  const struct vessel_lookout_contact *second_contact;

  first_contact = first;
  second_contact = second;
  if (first_contact->range < second_contact->range)
  {
    return -1;
  }
  if (first_contact->range > second_contact->range)
  {
    return 1;
  }
  return first_contact->shipnum - second_contact->shipnum;
}

static int vessel_lookout_collect_contacts(const struct greyhawk_ship_data *ship, int visibility,
                                           struct vessel_lookout_contact *contacts)
{
  const struct greyhawk_ship_data *other;
  float range;
  int ship_z;
  int count;
  int i;

  ship_z = vessel_autopilot_grid_coordinate(ship->z);
  count = 0;
  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
  {
    other = &greyhawk_ships[i];
    if (other == ship || !is_valid_ship(other))
    {
      continue;
    }

    range = greyhawk_range(ship->x, ship->y, ship->z, other->x, other->y, other->z);
    if (range > visibility)
    {
      continue;
    }

    contacts[count].shipnum = i;
    contacts[count].range = range;
    contacts[count].bearing = greyhawk_bearing(ship->x, ship->y, other->x, other->y);
    contacts[count].delta_z = vessel_autopilot_grid_coordinate(other->z) - ship_z;
    count++;
  }

  if (count > 1)
  {
    qsort(contacts, count, sizeof(*contacts), vessel_lookout_compare_contacts);
  }
  return count;
}

static void vessel_lookout_render_contacts(struct char_data *ch,
                                           const struct vessel_lookout_contact *contacts,
                                           int contact_count)
{
  const struct greyhawk_ship_data *contact_ship;
  const char *name;
  int shown;
  int i;

  send_to_char(ch, "\r\nVisible vessels (nearest first):\r\n");
  if (contact_count == 0)
  {
    send_to_char(ch, "  No vessels in sight.\r\n");
    return;
  }

  shown = MIN(contact_count, VESSEL_LOOKOUT_CONTACT_LIMIT);
  for (i = 0; i < shown; i++)
  {
    contact_ship = &greyhawk_ships[contacts[i].shipnum];
    name = contact_ship->name[0] ? contact_ship->name : "Unknown Vessel";
    send_to_char(ch, "  [%d] %-24.24s %-9s %5.1fu %s (%d deg), dz %+d\r\n", contacts[i].shipnum,
                 name, vessel_status_name(vessel_status(contact_ship)), contacts[i].range,
                 vessel_lookout_compass_direction(contacts[i].bearing), contacts[i].bearing,
                 contacts[i].delta_z);
  }
  if (contact_count > shown)
  {
    send_to_char(ch, "  %d more contact%s are visible; use TACTICAL for the full roster.\r\n",
                 contact_count - shown, contact_count - shown == 1 ? "" : "s");
  }
}

ACMD(do_look_outside)
{
  struct vessel_lookout_contact contacts[GREYHAWK_ACTIVE_SHIP_CAPACITY];
  struct greyhawk_ship_data *ship;
  int ship_x;
  int ship_y;
  int ship_z;
  int terrain_type;
  int elevation;
  int depth_units;
  int weather;
  int visibility;
  int contact_count;
  int i;
  char *at_sea_description;

  ship = get_ship_from_room(IN_ROOM(ch));
  if (!is_valid_ship(ship))
  {
    send_to_char(ch, "You need to be on a vessel to look outside.\r\n");
    return;
  }
  if (!room_has_outside_view(IN_ROOM(ch)))
  {
    send_to_char(ch, "You can't see outside from here.\r\n");
    return;
  }

  ship_x = vessel_autopilot_grid_coordinate(ship->x);
  ship_y = vessel_autopilot_grid_coordinate(ship->y);
  ship_z = vessel_autopilot_grid_coordinate(ship->z);
  terrain_type = get_ship_terrain_type(ship->shipnum);
  elevation = get_modified_elevation(ship_x, ship_y);
  depth_units = MAX(0, wild_waterline - elevation);
  weather = get_weather(ship_x, ship_y);
  visibility = vessel_sight_range(ship);
  contact_count = vessel_lookout_collect_contacts(ship, visibility, contacts);

  send_to_char(ch, "\r\nLOOKOUT VIEW FROM %s\r\n", ship->name);
  send_to_char(ch, "Position: (%d, %d, %d)   Heading: %d deg %s\r\n", ship_x, ship_y, ship_z,
               ship->heading, vessel_lookout_compass_direction(ship->heading));
  send_to_char(ch, "Conditions: %s (%d/255); visibility %d units%s.\r\n",
               vessel_weather_condition_name(weather), weather, visibility,
               vessel_lookout_bonus(ship) > 0 ? " with a posted lookout" : "");
  send_to_char(ch, "Current sector: %s; natural elevation %d; water column %d units.\r\n",
               vessel_lookout_sector_name(terrain_type), elevation, depth_units);
  at_sea_description = vessel_create_at_sea_description(ch, ship);
  if (at_sea_description != NULL)
  {
    send_to_char(ch, "At sea: %s\r\n", at_sea_description);
    free(at_sea_description);
  }
  send_to_char(ch, "\r\nSurrounding wilderness (sampled to the visible horizon):\r\n");
  for (i = 0; i < VESSEL_LOOKOUT_DIRECTION_COUNT; i++)
  {
    vessel_lookout_render_direction(ch, ship_x, ship_y, visibility, i);
  }
  vessel_lookout_render_contacts(ch, contacts, contact_count);
}
