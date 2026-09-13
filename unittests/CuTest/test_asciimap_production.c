#include "CuTest.h"

#include "conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/asciimap.h"
#include "../../src/comm.h"
#include "../../src/db.h"
#include "../../src/interpreter.h"
#include "../../src/net/protocol.h"
#include "../../src/obj/shop.h"

/* Synthetic VNUMs deliberately differ from the room array indices. */
#define MAP_TEST_VNUM_BASE 910000

static void strip_map_terminal_styles(char *text)
{
  char *read = text;
  char *write = text;

  while (*read != '\0')
  {
    if (*read == '\033' && read[1] == '[')
    {
      read += 2;
      while (*read != '\0' && *read != 'm')
        read++;
      if (*read == 'm')
        read++;
    }
    else
      *write++ = *read++;
  }
  *write = '\0';
}

static void assert_shop_map(CuTest *tc, const char *argument, bool standing_in_shop,
                            bool duplicate_shop, bool no_shops, bool default_world,
                            const char *expected_row)
{
  struct room_data rooms[4] = {0};
  struct zone_data zone = {0};
  struct room_direction_data exits[4] = {0};
  struct shop_data shops[3] = {0};
  room_vnum shop_rooms[] = {MAP_TEST_VNUM_BASE, MAP_TEST_VNUM_BASE + 3, NOWHERE, NOWHERE};
  struct char_data player = {0};
  struct player_special_data specials = {0};
  struct descriptor_data descriptor = {0};
  struct room_data *saved_world = world;
  struct zone_data *saved_zones = zone_table;
  struct shop_data *saved_shops = shop_index;
  room_rnum saved_top_world = top_of_world;
  zone_rnum saved_top_zone = top_of_zone_table;
  int saved_top_shop = top_shop;
  int saved_map_mode = CONFIG_MAP;
  int saved_map_size = CONFIG_MAP_SIZE;
  int saved_minimap_size = CONFIG_MINIMAP_SIZE;
  char output[MAX_STRING_LENGTH];
  char minimap[MAX_STRING_LENGTH];
  bool legend_present;
  int i;

  descriptor.pProtocol = ProtocolCreate();
  CuAssertPtrNotNull(tc, descriptor.pProtocol);
  descriptor.pProtocol->pVariables[eMSDP_ANSI_COLORS]->ValueInt = 0;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.character = &player;
  player.desc = &descriptor;
  player.player_specials = &specials;
  GET_LEVEL(&player) = LVL_IMMORT;
  IN_ROOM(&player) = 2;

  for (i = 0; i < 4; i++)
  {
    rooms[i].number = MAP_TEST_VNUM_BASE + i;
    rooms[i].sector_type = SECT_INSIDE;
    rooms[i].light = 1;
  }
  rooms[1].sector_type = SECT_CITY;
  rooms[3].sector_type = SECT_FOREST;
  rooms[1].dir_option[EAST] = &exits[0];
  exits[0].to_room = 2;
  rooms[2].dir_option[WEST] = &exits[1];
  exits[1].to_room = 1;
  rooms[2].dir_option[EAST] = &exits[2];
  exits[2].to_room = 3;
  rooms[3].dir_option[WEST] = &exits[3];
  exits[3].to_room = 2;
  if (default_world)
    SET_BIT_AR(zone.zone_flags, ZONE_WORLDMAP);

  /* The first shop has no room list. The last shop uses its second room.
   * No keepers exist, and impossible opening hours model a closed shop. */
  shops[1].in_room = shop_rooms;
  shops[1].open1 = shops[1].open2 = 25;
  shops[1].close1 = shops[1].close2 = 26;
  if (standing_in_shop)
    shop_rooms[2] = rooms[2].number;
  shops[2] = shops[1];

  world = rooms;
  top_of_world = 3;
  zone_table = &zone;
  top_of_zone_table = 0;
  shop_index = no_shops ? NULL : shops;
  top_shop = no_shops ? -1 : (duplicate_shop ? 2 : 1);
  CONFIG_MAP = MAP_ON;
  CONFIG_MAP_SIZE = 4;
  CONFIG_MINIMAP_SIZE = 4;

  do_map(&player, argument, 0, 0);
  snprintf(output, sizeof(output), "%s", descriptor.output);
  strip_map_terminal_styles(output);
  legend_present = strstr(output, "[$] Shop") != NULL;
  snprintf(minimap, sizeof(minimap), "%s", get_map_string(&player, IN_ROOM(&player)));

  world = saved_world;
  top_of_world = saved_top_world;
  zone_table = saved_zones;
  top_of_zone_table = saved_top_zone;
  shop_index = saved_shops;
  top_shop = saved_top_shop;
  CONFIG_MAP = saved_map_mode;
  CONFIG_MAP_SIZE = saved_map_size;
  CONFIG_MINIMAP_SIZE = saved_minimap_size;
  ProtocolDestroy(descriptor.pProtocol);
  if (descriptor.large_outbuf != NULL)
  {
    free(descriptor.large_outbuf->text);
    free(descriptor.large_outbuf);
    buf_largecount--;
  }

  /* Assert actual neighboring map cells, so a '$' in the legend cannot pass. */
  CuAssert(tc, output, strstr(output, expected_row) != NULL);
  CuAssertTrue(tc, legend_present);
  CuAssertTrue(tc, strstr(output, "OVERFLOW") == NULL);
  CuAssertTrue(tc, (strchr(minimap, '$') != NULL) == !no_shops);
  CuAssertTrue(tc, strchr(minimap, '&') != NULL);
}

void Test_asciimap_shop_and_terrain_render_through_map_commands(CuTest *tc)
{
  assert_shop_map(tc, "", FALSE, FALSE, FALSE, FALSE, "[C] - [&] - [$]");
  assert_shop_map(tc, "world", FALSE, FALSE, FALSE, FALSE, "C&$");
  assert_shop_map(tc, "4 normal", FALSE, FALSE, FALSE, FALSE, "[C] - [&] - [$]");
  assert_shop_map(tc, "4 world", FALSE, FALSE, FALSE, FALSE, "C&$");
}

void Test_asciimap_player_marker_wins_in_shop_in_both_modes(CuTest *tc)
{
  assert_shop_map(tc, "", TRUE, FALSE, FALSE, FALSE, "[C] - [&] - [$]");
  assert_shop_map(tc, "world", TRUE, FALSE, FALSE, FALSE, "C&$");
}

void Test_asciimap_multiple_shops_keep_one_marker(CuTest *tc)
{
  assert_shop_map(tc, "", FALSE, TRUE, FALSE, FALSE, "[C] - [&] - [$]");
  assert_shop_map(tc, "world", FALSE, TRUE, FALSE, FALSE, "C&$");
}

void Test_asciimap_without_shops_keeps_terrain(CuTest *tc)
{
  assert_shop_map(tc, "", FALSE, FALSE, TRUE, FALSE, "[C] - [&] - [Y]");
  assert_shop_map(tc, "world", FALSE, FALSE, TRUE, FALSE, "C&Y");
}

void Test_asciimap_zone_default_and_mode_override(CuTest *tc)
{
  assert_shop_map(tc, "", FALSE, FALSE, FALSE, TRUE, "C&$");
  assert_shop_map(tc, "normal", FALSE, FALSE, FALSE, TRUE, "[C] - [&] - [$]");
}
