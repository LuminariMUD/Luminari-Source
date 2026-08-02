/* ************************************************************************
 *      File:   vessels_tactical.c                   Part of LuminariMUD  *
 *   Purpose:   Wilderness-backed vessel tactical chart.                 *
 * ********************************************************************** */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h"
#include "vessels.h"
#include "wilderness.h"

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

#define VESSEL_TACTICAL_SIZE 21
#define VESSEL_TACTICAL_RADIUS ((VESSEL_TACTICAL_SIZE - 1) / 2)
#define VESSEL_TACTICAL_CONTACT_LIMIT 20
#define VESSEL_TACTICAL_REGION_LIMIT 32

struct vessel_tactical_contact
{
  int shipnum;
  float range;
  int bearing;
  int delta_z;
};

bool vessel_tactical_sector_is_water(int sector_type)
{
  switch (sector_type)
  {
  case SECT_WATER_SWIM:
  case SECT_WATER_NOSWIM:
  case SECT_UNDERWATER:
  case SECT_OCEAN:
  case SECT_UD_WATER:
  case SECT_UD_NOSWIM:
  case SECT_RIVER:
    return TRUE;
  default:
    return FALSE;
  }
}

char vessel_tactical_terrain_symbol(int sector_type, bool coastal)
{
  switch (sector_type)
  {
  case SECT_OCEAN:
  case SECT_WATER_NOSWIM:
  case SECT_UD_NOSWIM:
    return '~';
  case SECT_WATER_SWIM:
  case SECT_UD_WATER:
    return '.';
  case SECT_RIVER:
    return '=';
  case SECT_UNDERWATER:
    return 'u';
  case SECT_BEACH:
    return ':';
  case SECT_SEAPORT:
    return 'D';
  default:
    break;
  }

  if (sector_type < 0 || sector_type >= NUM_ROOM_SECTORS)
  {
    return '?';
  }
  return coastal ? '#' : '^';
}

int vessel_tactical_range_ring(int delta_x, int delta_y)
{
  long long distance_squared;

  distance_squared = (long long)delta_x * delta_x + (long long)delta_y * delta_y;
  if (distance_squared >= 21 && distance_squared <= 30)
  {
    return 5;
  }
  if (distance_squared >= 91 && distance_squared <= 110)
  {
    return 10;
  }
  return 0;
}

bool vessel_tactical_region_type_visible(int region_type)
{
  switch (region_type)
  {
  case REGION_GEOGRAPHIC:
  case REGION_BATHYMETRIC:
  case REGION_ALTITUDE_LANE:
  case REGION_SKY_ISLAND:
    return TRUE;
  default:
    return FALSE;
  }
}

char vessel_tactical_contact_symbol(int status, int contact_count)
{
  if (contact_count <= 0)
  {
    return ' ';
  }
  if (contact_count > 1)
  {
    return 'M';
  }

  switch (status)
  {
  case VESSEL_STATUS_SOUND:
    return 'V';
  case VESSEL_STATUS_BATTERED:
    return 'B';
  case VESSEL_STATUS_CRIPPLED:
    return 'C';
  case VESSEL_STATUS_SINKING:
    return 'X';
  default:
    return '?';
  }
}

static const char *vessel_tactical_weather_name(int weather)
{
  if (weather <= 127)
  {
    return "clear";
  }
  if (weather <= 177)
  {
    return "overcast";
  }
  if (weather <= 199)
  {
    return "rain";
  }
  if (weather <= 224)
  {
    return "storm";
  }
  return "thunderstorm";
}

static const char *vessel_tactical_direction(int bearing)
{
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

static const char *vessel_tactical_region_type_name(int region_type)
{
  switch (region_type)
  {
  case REGION_GEOGRAPHIC:
    return "geographic";
  case REGION_BATHYMETRIC:
    return "bathymetric";
  case REGION_ALTITUDE_LANE:
    return "altitude lane";
  case REGION_SKY_ISLAND:
    return "sky island";
  default:
    return "hidden";
  }
}

static bool vessel_tactical_region_valid(region_rnum rnum)
{
  return region_table != NULL && rnum != NOWHERE && rnum <= top_of_region_table;
}

static bool vessel_tactical_tile_has_region(const struct wild_map_tile *tile, region_rnum wanted)
{
  int i;

  if (tile == NULL)
  {
    return FALSE;
  }

  for (i = 0; i < tile->num_regions; i++)
  {
    if (tile->regions[i] == wanted && vessel_tactical_region_valid(wanted) &&
        vessel_tactical_region_type_visible(region_table[wanted].region_type))
    {
      return TRUE;
    }
  }
  return FALSE;
}

static int vessel_tactical_visible_region_count(const struct wild_map_tile *tile)
{
  int count;
  int i;

  if (tile == NULL)
  {
    return 0;
  }

  count = 0;
  for (i = 0; i < tile->num_regions; i++)
  {
    if (vessel_tactical_region_valid(tile->regions[i]) &&
        vessel_tactical_region_type_visible(region_table[tile->regions[i]].region_type))
    {
      count++;
    }
  }
  return count;
}

static bool vessel_tactical_region_sets_differ(const struct wild_map_tile *first,
                                               const struct wild_map_tile *second)
{
  int first_count;
  int second_count;
  int i;

  first_count = vessel_tactical_visible_region_count(first);
  second_count = vessel_tactical_visible_region_count(second);
  if (first_count != second_count)
  {
    return TRUE;
  }

  for (i = 0; i < first->num_regions; i++)
  {
    if (vessel_tactical_region_valid(first->regions[i]) &&
        vessel_tactical_region_type_visible(region_table[first->regions[i]].region_type) &&
        !vessel_tactical_tile_has_region(second, first->regions[i]))
    {
      return TRUE;
    }
  }
  return FALSE;
}

static bool vessel_tactical_region_boundary(struct wild_map_tile **map, int x, int y)
{
  static const int offsets[4][2] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
  int neighbor_x;
  int neighbor_y;
  int i;

  if (vessel_tactical_visible_region_count(&map[x][y]) == 0)
  {
    return FALSE;
  }

  for (i = 0; i < 4; i++)
  {
    neighbor_x = x + offsets[i][0];
    neighbor_y = y + offsets[i][1];
    if (neighbor_x < 0 || neighbor_x >= VESSEL_TACTICAL_SIZE || neighbor_y < 0 ||
        neighbor_y >= VESSEL_TACTICAL_SIZE)
    {
      continue;
    }
    if (vessel_tactical_region_sets_differ(&map[x][y], &map[neighbor_x][neighbor_y]))
    {
      return TRUE;
    }
  }
  return FALSE;
}

static bool vessel_tactical_is_coastal(struct wild_map_tile **map, int x, int y)
{
  static const int offsets[4][2] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
  int neighbor_x;
  int neighbor_y;
  int i;

  if (vessel_tactical_sector_is_water(map[x][y].sector_type))
  {
    return FALSE;
  }

  for (i = 0; i < 4; i++)
  {
    neighbor_x = x + offsets[i][0];
    neighbor_y = y + offsets[i][1];
    if (neighbor_x >= 0 && neighbor_x < VESSEL_TACTICAL_SIZE && neighbor_y >= 0 &&
        neighbor_y < VESSEL_TACTICAL_SIZE &&
        vessel_tactical_sector_is_water(map[neighbor_x][neighbor_y].sector_type))
    {
      return TRUE;
    }
  }
  return FALSE;
}

static int vessel_tactical_compare_contacts(const void *first, const void *second)
{
  const struct vessel_tactical_contact *first_contact;
  const struct vessel_tactical_contact *second_contact;

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

static bool vessel_tactical_region_recorded(const region_rnum *regions, int count, region_rnum rnum)
{
  int i;

  for (i = 0; i < count; i++)
  {
    if (regions[i] == rnum)
    {
      return TRUE;
    }
  }
  return FALSE;
}

static int vessel_tactical_collect_regions(struct wild_map_tile **map, region_rnum *regions)
{
  region_rnum rnum;
  int count;
  int x;
  int y;
  int i;

  count = 0;
  for (x = 0; x < VESSEL_TACTICAL_SIZE; x++)
  {
    for (y = 0; y < VESSEL_TACTICAL_SIZE; y++)
    {
      for (i = 0; i < map[x][y].num_regions; i++)
      {
        rnum = map[x][y].regions[i];
        if (!vessel_tactical_region_valid(rnum) ||
            !vessel_tactical_region_type_visible(region_table[rnum].region_type) ||
            vessel_tactical_region_recorded(regions, count, rnum))
        {
          continue;
        }
        if (count < VESSEL_TACTICAL_REGION_LIMIT)
        {
          regions[count++] = rnum;
        }
      }
    }
  }
  return count;
}

static int
vessel_tactical_collect_contacts(const struct greyhawk_ship_data *ship,
                                 struct vessel_tactical_contact *contacts,
                                 int contact_counts[VESSEL_TACTICAL_SIZE][VESSEL_TACTICAL_SIZE],
                                 int contact_status[VESSEL_TACTICAL_SIZE][VESSEL_TACTICAL_SIZE])
{
  const struct greyhawk_ship_data *other;
  float range;
  int sight_range;
  int ship_x;
  int ship_y;
  int ship_z;
  int other_x;
  int other_y;
  int other_z;
  int map_x;
  int map_y;
  int status;
  int count;
  int i;

  sight_range = vessel_sight_range(ship);
  ship_x = vessel_autopilot_grid_coordinate(ship->x);
  ship_y = vessel_autopilot_grid_coordinate(ship->y);
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
    if (range > sight_range)
    {
      continue;
    }

    other_x = vessel_autopilot_grid_coordinate(other->x);
    other_y = vessel_autopilot_grid_coordinate(other->y);
    other_z = vessel_autopilot_grid_coordinate(other->z);
    contacts[count].shipnum = i;
    contacts[count].range = range;
    contacts[count].bearing = greyhawk_bearing(ship->x, ship->y, other->x, other->y);
    contacts[count].delta_z = other_z - ship_z;
    count++;

    map_x = other_x - ship_x + VESSEL_TACTICAL_RADIUS;
    map_y = other_y - ship_y + VESSEL_TACTICAL_RADIUS;
    if (map_x < 0 || map_x >= VESSEL_TACTICAL_SIZE || map_y < 0 || map_y >= VESSEL_TACTICAL_SIZE)
    {
      continue;
    }

    status = vessel_status(other);
    contact_counts[map_x][map_y]++;
    if (status > contact_status[map_x][map_y])
    {
      contact_status[map_x][map_y] = status;
    }
  }

  if (count > 1)
  {
    qsort(contacts, count, sizeof(*contacts), vessel_tactical_compare_contacts);
  }
  return count;
}

static void vessel_tactical_render_regions(struct char_data *ch, const region_rnum *regions,
                                           int region_count)
{
  const struct region_data *region;
  int i;

  if (region_count == 0)
  {
    send_to_char(ch, "   Charted regions: none\r\n");
    return;
  }

  send_to_char(ch, "   Charted regions:\r\n");
  for (i = 0; i < region_count; i++)
  {
    region = &region_table[regions[i]];
    send_to_char(ch, "     - %s (%s)\r\n", region->name ? region->name : "Unnamed region",
                 vessel_tactical_region_type_name(region->region_type));
  }
}

static void vessel_tactical_render_contacts(struct char_data *ch,
                                            const struct vessel_tactical_contact *contacts,
                                            int contact_count)
{
  const struct greyhawk_ship_data *contact_ship;
  const char *name;
  int shown;
  int i;

  if (contact_count == 0)
  {
    send_to_char(ch, "\r\n   Contacts within visibility: none\r\n");
    return;
  }

  shown = MIN(contact_count, VESSEL_TACTICAL_CONTACT_LIMIT);
  send_to_char(ch, "\r\n   CONTACTS (nearest first)\r\n");
  send_to_char(ch, "   ID   VESSEL                   STATE       RANGE  BRG DIR   DZ\r\n");
  send_to_char(ch, "   ---------------------------------------------------------------\r\n");
  for (i = 0; i < shown; i++)
  {
    contact_ship = &greyhawk_ships[contacts[i].shipnum];
    name = contact_ship->name[0] ? contact_ship->name : "Unknown Vessel";
    send_to_char(ch, "   %-4d %-24.24s %-9s %6.1f %4d %-3s %+4d\r\n", contacts[i].shipnum, name,
                 vessel_status_name(vessel_status(contact_ship)), contacts[i].range,
                 contacts[i].bearing, vessel_tactical_direction(contacts[i].bearing),
                 contacts[i].delta_z);
  }
  send_to_char(ch, "   Detected: %d%s\r\n", contact_count,
               contact_count > shown ? " (nearest 20 shown)" : "");
}

ACMD(do_greyhawk_tactical)
{
  struct greyhawk_ship_data *ship;
  struct wild_map_tile map_data[VESSEL_TACTICAL_SIZE * VESSEL_TACTICAL_SIZE];
  struct wild_map_tile *map[VESSEL_TACTICAL_SIZE];
  struct vessel_tactical_contact contacts[GREYHAWK_ACTIVE_SHIP_CAPACITY];
  region_rnum regions[VESSEL_TACTICAL_REGION_LIMIT];
  char display[VESSEL_TACTICAL_SIZE][VESSEL_TACTICAL_SIZE];
  int contact_counts[VESSEL_TACTICAL_SIZE][VESSEL_TACTICAL_SIZE];
  int contact_status[VESSEL_TACTICAL_SIZE][VESSEL_TACTICAL_SIZE];
  char line[64];
  int contact_count;
  int region_count;
  int ship_x;
  int ship_y;
  int ship_z;
  int weather;
  int sight_range;
  int hull_current;
  int hull_maximum;
  int hull_percent;
  int ring;
  int x;
  int y;
  int line_position;

  ship = get_ship_from_room(IN_ROOM(ch));
  if (!is_valid_ship(ship))
  {
    send_to_char(ch, "You must be aboard a vessel to view the tactical chart.\r\n");
    return;
  }

  ship_x = vessel_autopilot_grid_coordinate(ship->x);
  ship_y = vessel_autopilot_grid_coordinate(ship->y);
  ship_z = vessel_autopilot_grid_coordinate(ship->z);
  weather = get_weather(ship_x, ship_y);
  sight_range = vessel_sight_range(ship);
  hull_current = vessel_total_internal(ship);
  hull_maximum = vessel_max_internal(ship);
  hull_percent = hull_maximum > 0 ? hull_current * 100 / hull_maximum : 0;

  memset(contact_counts, 0, sizeof(contact_counts));
  memset(contact_status, 0, sizeof(contact_status));
  for (x = 0; x < VESSEL_TACTICAL_SIZE; x++)
  {
    map[x] = map_data + x * VESSEL_TACTICAL_SIZE;
  }
  get_map(VESSEL_TACTICAL_SIZE, VESSEL_TACTICAL_SIZE, ship_x, ship_y, map);

  for (x = 0; x < VESSEL_TACTICAL_SIZE; x++)
  {
    for (y = 0; y < VESSEL_TACTICAL_SIZE; y++)
    {
      display[x][y] = vessel_tactical_terrain_symbol(map[x][y].sector_type,
                                                     vessel_tactical_is_coastal(map, x, y));
      ring = vessel_tactical_range_ring(x - VESSEL_TACTICAL_RADIUS, y - VESSEL_TACTICAL_RADIUS);
      if (ring == 5)
      {
        display[x][y] = 'o';
      }
      else if (ring == 10)
      {
        display[x][y] = 'O';
      }
      if (vessel_tactical_region_boundary(map, x, y))
      {
        display[x][y] = '+';
      }
    }
  }

  contact_count = vessel_tactical_collect_contacts(ship, contacts, contact_counts, contact_status);
  for (x = 0; x < VESSEL_TACTICAL_SIZE; x++)
  {
    for (y = 0; y < VESSEL_TACTICAL_SIZE; y++)
    {
      if (contact_counts[x][y] > 0)
      {
        display[x][y] = vessel_tactical_contact_symbol(contact_status[x][y], contact_counts[x][y]);
      }
    }
  }
  display[VESSEL_TACTICAL_RADIUS][VESSEL_TACTICAL_RADIUS] = '@';
  region_count = vessel_tactical_collect_regions(map, regions);

  send_to_char(ch, "\r\n              WILDERNESS TACTICAL CHART\r\n");
  send_to_char(ch, "   Position: (%d, %d, %d)   Heading: %d deg %s\r\n", ship_x, ship_y, ship_z,
               ship->heading, vessel_tactical_direction(ship->heading));
  send_to_char(ch, "   Weather: %s (%d/255)   Visibility: %d units\r\n",
               vessel_tactical_weather_name(weather), weather, sight_range);
  send_to_char(ch, "   Hull: %s, %d/%d internal (%d%%)\r\n",
               vessel_status_name(vessel_status(ship)), hull_current, hull_maximum, hull_percent);
  send_to_char(ch, "                 N\r\n");
  send_to_char(ch, "     +---------------------+\r\n");
  for (y = VESSEL_TACTICAL_SIZE - 1; y >= 0; y--)
  {
    line_position = snprintf(line, sizeof(line), y == VESSEL_TACTICAL_RADIUS ? " W   |" : "     |");
    for (x = 0; x < VESSEL_TACTICAL_SIZE && line_position < (int)sizeof(line) - 8; x++)
    {
      line[line_position++] = display[x][y];
    }
    snprintf(line + line_position, sizeof(line) - line_position,
             y == VESSEL_TACTICAL_RADIUS ? "|   E\r\n" : "|\r\n");
    send_to_char(ch, "%s", line);
  }
  send_to_char(ch, "     +---------------------+\r\n");
  send_to_char(ch, "                 S\r\n");
  send_to_char(ch,
               "\r\n   Terrain: ~ Deep  . Shoal  = River  # Coast  ^ Land  : Beach  D Port\r\n");
  send_to_char(ch, "   Overlays: + Region edge  o 5u ring  O 10u ring  @ Your vessel\r\n");
  send_to_char(ch, "   Contacts: V Sound  B Battered  C Crippled  X Sinking  M Multiple\r\n");
  vessel_tactical_render_regions(ch, regions, region_count);
  vessel_tactical_render_contacts(ch, contacts, contact_count);
}
