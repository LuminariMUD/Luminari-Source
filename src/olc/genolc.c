/**************************************************************************
 *  File: genolc.c                                     Part of LuminariMUD *
 *  Usage: Generic OLC Library - General.                                  *
 *                                                                         *
 *  Copyright 1996 by Harvey Gilpin, 1997-2001 by George Greer.            *
 **************************************************************************/

#define __GENOLC_C__

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "comm.h"
#include "obj/shop.h"
#include "oasis.h"
#include "genolc.h"
#include "genwld.h"
#include "genmob.h"
#include "genshp.h"
#include "genzon.h"
#include "genobj.h"
#include "dgscript/dg_olc.h"
#include "constants.h"
#include "interpreter.h"
#include "act.h"    /* for the space_to_minus function */
#include "modify.h" /* for smash_tilde */
#include "quest/quest.h"
#include "craft/craft.h" // get_obj_material

#define WORLDMAP_ZONE_WIDTH 160
#define WORLDMAP_ZONE_MAX_HEIGHT 160
#define WORLDMAP_EXPORT_MAX_WIDTH (WORLDMAP_ZONE_WIDTH * 2)
#define WORLDMAP_EXPORT_MAX_HEIGHT (WORLDMAP_ZONE_MAX_HEIGHT * 2)
#define WORLDMAP_MARKER_MAX 1024

struct worldmap_marker_data
{
  int id;
  int x;
  int y;
  int sector;
  char title[MAX_INPUT_LENGTH];
};

struct worldmap_zone_export_data
{
  zone_rnum zrnum;
  zone_vnum zvnum;
  int x_offset;
  int y_offset;
  bool use_asciimap_points;
};

/* Global variables defined here, used elsewhere */
/* List of zones to be saved. */
struct save_list_data *save_list;

/* Local (file scope) variables */

/* Structure defining all known save types. */
static struct
{
  int save_type;
  int (*func)(IDXTYPE rnum);
  const char *message;
} save_types[] = {
    {SL_MOB, save_mobiles, "mobile"}, {SL_OBJ, save_objects, "object"},
    {SL_SHP, save_shops, "shop"},     {SL_WLD, save_rooms, "room"},
    {SL_ZON, save_zone, "zone"},      {SL_CFG, save_config, "config"},
    {SL_QST, save_quests, "quest"},   {SL_ACT, NULL, "social"},
    {SL_HLP, NULL, "help"},           {-1, NULL, NULL},
};
/* for Zone Export */
static int zone_exits = 0;
/* Max size for zone export filenames (zone names are typically <64 chars) */
#define MAX_EXPORT_FILENAME 128

/* Local (file scope) functions */
/* Zone export functions */
static int export_save_shops(zone_rnum zrnum);
static int export_save_mobiles(zone_rnum rznum);
static int export_save_zone(zone_rnum zrnum);
static int export_save_objects(zone_rnum zrnum);
static int export_save_rooms(zone_rnum zrnum);
static int export_save_triggers(zone_rnum zrnum);
static int export_mobile_record(mob_vnum mvnum, struct char_data *mob, FILE *fd);
static void export_script_save_to_disk(FILE *fp, void *item, int type);
static int export_info_file(zone_rnum zrnum);
static room_vnum worldmap_zone_anchor(zone_vnum zvnum);
static bool worldmap_vnum_to_xy(zone_vnum zvnum, room_vnum rvnum, int *x, int *y);
static int worldmap_zone_height(zone_rnum zrnum);
static void worldmap_html_write_escaped(FILE *out, const char *text);
static void worldmap_html_write_span(FILE *out, int sector);
static void worldmap_html_write_marker(FILE *out, const struct worldmap_marker_data *marker);
static void worldmap_init_export_buffers(int sector_grid[WORLDMAP_EXPORT_MAX_HEIGHT][WORLDMAP_EXPORT_MAX_WIDTH],
                                         int marker_lookup[WORLDMAP_EXPORT_MAX_HEIGHT][WORLDMAP_EXPORT_MAX_WIDTH]);
static void worldmap_html_write_document(FILE *out, const char *subtitle,
                                         int sector_grid[WORLDMAP_EXPORT_MAX_HEIGHT][WORLDMAP_EXPORT_MAX_WIDTH],
                                         int marker_lookup[WORLDMAP_EXPORT_MAX_HEIGHT][WORLDMAP_EXPORT_MAX_WIDTH],
                                         const struct worldmap_marker_data *markers, int marker_count, int map_width,
                                         int map_height);
static int worldmap_add_marker(struct worldmap_marker_data *markers,
                               int marker_lookup[WORLDMAP_EXPORT_MAX_HEIGHT][WORLDMAP_EXPORT_MAX_WIDTH],
                               int *marker_count, int x, int y, int sector, const char *title);
static void worldmap_collect_zone_grid(const struct worldmap_zone_export_data *zone,
                                       int sector_grid[WORLDMAP_EXPORT_MAX_HEIGHT][WORLDMAP_EXPORT_MAX_WIDTH],
                                       int *max_x, int *max_y);
static void worldmap_collect_zone_markers(const struct worldmap_zone_export_data *zone,
                                          int sector_grid[WORLDMAP_EXPORT_MAX_HEIGHT][WORLDMAP_EXPORT_MAX_WIDTH],
                                          int marker_lookup[WORLDMAP_EXPORT_MAX_HEIGHT][WORLDMAP_EXPORT_MAX_WIDTH],
                                          struct worldmap_marker_data *markers, int *marker_count);
static int export_worldmap_html(zone_rnum zrnum, const char *output_file, char *saved_path, size_t saved_path_size);
static int export_full_worldmap_html(const char *output_file, char *saved_path, size_t saved_path_size);

int genolc_checkstring(struct descriptor_data *d __attribute__((unused)), char *arg)
{
  smash_tilde(arg);
  parse_at(arg);
  return TRUE;
}

char *str_udup(const char *txt)
{
  return strdup((txt && *txt) ? txt : "undefined");
}

static room_vnum worldmap_zone_anchor(zone_vnum zvnum)
{
  return (room_vnum)(zvnum * 100 + 161);
}

static bool worldmap_vnum_to_xy(zone_vnum zvnum, room_vnum rvnum, int *x, int *y)
{
  room_vnum anchor = worldmap_zone_anchor(zvnum);
  room_vnum offset;

  if (rvnum < anchor)
    return FALSE;

  offset = rvnum - anchor;
  *x = offset % WORLDMAP_ZONE_WIDTH;
  *y = offset / WORLDMAP_ZONE_WIDTH;

  if (*x < 0 || *x >= WORLDMAP_ZONE_WIDTH || *y < 0 || *y >= WORLDMAP_ZONE_MAX_HEIGHT)
    return FALSE;

  return TRUE;
}

static int worldmap_zone_height(zone_rnum zrnum)
{
  zone_vnum zvnum = zone_table[zrnum].number;
  room_vnum i;
  int x;
  int y;
  int max_y = -1;

  for (i = genolc_zone_bottom(zrnum); i <= zone_table[zrnum].top; i++)
  {
    room_rnum rnum = real_room(i);

    if (rnum == NOWHERE)
      continue;

    if (!worldmap_vnum_to_xy(zvnum, i, &x, &y))
      continue;

    max_y = MAX(max_y, y);
  }

  return max_y >= 0 ? (max_y + 1) : 0;
}

static void worldmap_html_write_escaped(FILE *out, const char *text)
{
  const unsigned char *ptr = (const unsigned char *)(text ? text : "");

  while (*ptr)
  {
    switch (*ptr)
    {
    case '&':
      fputs("&amp;", out);
      break;
    case '<':
      fputs("&lt;", out);
      break;
    case '>':
      fputs("&gt;", out);
      break;
    case '"':
      fputs("&quot;", out);
      break;
    case '\'':
      fputs("&#39;", out);
      break;
    default:
      fputc(*ptr, out);
      break;
    }
    ptr++;
  }
}

static void worldmap_html_write_span(FILE *out, int sector)
{
  fprintf(out, "<span class=\"worldmap-cell\" style=\"color: %s;\">%s</span>", ascii_webmap_colors[sector],
          sector_map_letters[sector]);
}

static void worldmap_html_write_marker(FILE *out, const struct worldmap_marker_data *marker)
{
  fprintf(out, "<a class=\"worldmap-cell\" id=\"%d\" href=\"#\" style=\"color: red; text-decoration: none;\" title=\"",
          marker->id);
  worldmap_html_write_escaped(out, marker->title);
  fprintf(out, "\">%s</a>", sector_map_letters[marker->sector]);
}

static void worldmap_init_export_buffers(int sector_grid[WORLDMAP_EXPORT_MAX_HEIGHT][WORLDMAP_EXPORT_MAX_WIDTH],
                                         int marker_lookup[WORLDMAP_EXPORT_MAX_HEIGHT][WORLDMAP_EXPORT_MAX_WIDTH])
{
  int x;
  int y;

  for (y = 0; y < WORLDMAP_EXPORT_MAX_HEIGHT; y++)
  {
    for (x = 0; x < WORLDMAP_EXPORT_MAX_WIDTH; x++)
    {
      sector_grid[y][x] = SECT_INSIDE;
      marker_lookup[y][x] = -1;
    }
  }
}

static void worldmap_html_write_document(FILE *out, const char *subtitle,
                                         int sector_grid[WORLDMAP_EXPORT_MAX_HEIGHT][WORLDMAP_EXPORT_MAX_WIDTH],
                                         int marker_lookup[WORLDMAP_EXPORT_MAX_HEIGHT][WORLDMAP_EXPORT_MAX_WIDTH],
                                         const struct worldmap_marker_data *markers, int marker_count, int map_width,
                                         int map_height)
{
  int i;
  int x;
  int y;

  fprintf(out,
          "<html>\n"
          "<head>\n"
          "<title>Faerun MUD World Map</title>\n"
          "<script src=\"https://code.jquery.com/jquery-3.6.0.slim.min.js\" integrity=\"sha256-u7e5khyithlIdTpu22PHhENmPcRdFiHRjhAuHcs05RI=\" crossorigin=\"anonymous\"></script>\n"
          "<style>\n"
          "  body {\n"
          "    font-family: Courier New;\n"
          "    background-color: black;\n"
          "    color: white;\n"
          "  }\n"
          "  h1, h2, h3 {\n"
          "    color: white;\n"
          "  }\n"
          "  .worldmap-grid {\n"
          "    display: inline-block;\n"
          "    font-family: Courier New, monospace;\n"
          "    line-height: 1;\n"
          "    white-space: nowrap;\n"
          "  }\n"
          "  .worldmap-cell {\n"
          "    display: inline-block;\n"
          "    width: 1ch;\n"
          "    min-width: 1ch;\n"
          "    text-align: center;\n"
          "    white-space: nowrap;\n"
          "  }\n"
          ".blinking{\n"
          "    animation:blinkingText 1.2s infinite;\n"
          "}\n"
          "@keyframes blinkingText{\n"
          "    0%%{ color: red; }\n"
          "    49%%{ color: red; }\n"
          "    60%%{ color: transparent; }\n"
          "    99%%{ color: transparent; }\n"
          "    100%%{ color: red; }\n"
          "}\n"
          "</style>\n"
          "<script>\n"
          "$(document).ready(function(){\n"
          "  $('#selZone').change(function(){\n"
          "    $('a').each(function(){\n"
          "      $(this).attr('class', '');\n"
          "    });\n"
          "    $('#'+$(this).val()).attr('class', 'blinking');\n"
          "    $('#'+$(this).val()).focus();\n"
          "  });\n"
          "});\n"
          "</script>\n"
          "</head>\n"
          "<body>\n"
          "  <h1>Faerun MUD World Map</h1>\n"
          "  <h2>");
  worldmap_html_write_escaped(out, subtitle ? subtitle : "Connected Wilderness Zones");
  fprintf(out, "</h2>\n");
  fprintf(out,
          "<div style=\"text-align: left;\">\n"
          "  SELECT A ZONE (The Red <span style=\"color: red;\">X</span> Will Start to Blink):\n"
          "  <select id=\"selZone\" style=\"width: 250px; font-size: 16px;\">\n"
          "    <option value=\"0\">Pick a Zone</option>\n");

  for (i = 0; i < marker_count; i++)
  {
    fprintf(out, "<option value=\"%d\">", markers[i].id);
    worldmap_html_write_escaped(out, markers[i].title);
    fputs("</option>\n", out);
  }

  fprintf(out,
          "  </select>\n"
          "</div>\n"
          "<h3>Map Legend</h3>"
          "<table style='text-align: left; width: 80%%;'>"
          "<tr><td width='20%%'><span style=\"color: grey;\">C</span>&nbsp;&nbsp;City</td><td width='20%%'><span style=\"color: lightgreen;\">,</span>&nbsp;&nbsp;Field</td></tr>"
          "<tr><td width='20%%'><span style=\"color: green;\">Y</span>&nbsp;&nbsp;Forest</td><td width='20%%'><span style=\"color: brown;\">^</span>&nbsp;&nbsp;Hills</td><td width='20%%'><span style=\"color: darkred;\">m</span>&nbsp;&nbsp;Low Mountains</td><td width='20%%'><span style=\"color: blue;\">~</span>&nbsp;&nbsp;Water (Swim)</td></tr>"
          "<tr><td width='20%%'><span style=\"color: blue;\">=</span>&nbsp;&nbsp;Water (No Swim)</td><td width='20%%'><span style=\"color: red;\">X</span>&nbsp;&nbsp;Zone Entrance</td><td width='20%%'><span style=\"color: #FFC300;\">|</span>&nbsp;&nbsp;Road North-South</td><td width='20%%'><span style=\"color: #FFC300;\">-</span>&nbsp;&nbsp;Road East-West</td></tr>"
          "<tr><td width='20%%'><span style=\"color: #FFC300;\">+</span>&nbsp;&nbsp;Road Intersection</td><td width='20%%'><span style=\"color: yellow;\">.</span>&nbsp;&nbsp;Desert</td><td width='20%%'><span style=\"color: blue;\">o</span>&nbsp;&nbsp;Ocean</td><td width='20%%'><span style=\"color: violet;\">`</span>&nbsp;&nbsp;Marshland</td></tr>"
          "<tr><td width='20%%'><span style=\"color: crimson;\">M</span>&nbsp;&nbsp;High Mountains</td><td width='20%%'><span style=\"color: crimson;\">]</span>&nbsp;&nbsp;Lava</td><td width='20%%'><span style=\"color: #FFC300;\">|</span>&nbsp;&nbsp;Dirt Road North-South</td><td width='20%%'><span style=\"color: #FFC300;\">-</span>&nbsp;&nbsp;Dirt Road East-West</td></tr>"
          "<tr><td width='20%%'><span style=\"color: #FFC300;\">+</span>&nbsp;&nbsp;Dirt Road Intersection</td><td width='20%%'><span style=\"color: lightgreen;\">&amp;</span>&nbsp;&nbsp;Jungle</td><td width='20%%'><span style=\"color: lightgrey;\">.</span>&nbsp;&nbsp;Tundra</td><td width='20%%'><span style=\"color: olive;\">A</span>&nbsp;&nbsp;Taiga</td></tr>"
          "<tr><td width='20%%'><span style=\"color: orange;\">:</span>&nbsp;&nbsp;Beach</td><td width='20%%'><span style=\"color: red;\">*</span>&nbsp;&nbsp;Sea Port</td><td width='20%%'><span style=\"color: cyan;\">~</span>&nbsp;&nbsp;River</td></tr>"
          "</table><br />");

  if (marker_count > 0)
    fputs("If you're using a mouse, hover over the red zone entrance markers to see the location name.<br /><br />", out);

  fputs("<div class=\"worldmap-grid\">", out);

  for (y = 0; y < map_height; y++)
  {
    if (y > 0)
      fputs("<br />", out);

    for (x = 0; x < map_width; x++)
    {
      int marker_index = marker_lookup[y][x];
      int sector = sector_grid[y][x];

      if (sector < 0 || sector >= NUM_ROOM_SECTORS)
        sector = SECT_INSIDE;

      if (marker_index >= 0)
        worldmap_html_write_marker(out, &markers[marker_index]);
      else
        worldmap_html_write_span(out, sector);
    }
  }

  fputs("</div>\n</body>\n</html>\n", out);
}

static int worldmap_add_marker(struct worldmap_marker_data *markers,
                               int marker_lookup[WORLDMAP_EXPORT_MAX_HEIGHT][WORLDMAP_EXPORT_MAX_WIDTH],
                               int *marker_count, int x, int y, int sector, const char *title)
{
  int idx;

  if (x < 0 || x >= WORLDMAP_EXPORT_MAX_WIDTH || y < 0 || y >= WORLDMAP_EXPORT_MAX_HEIGHT)
    return FALSE;

  idx = marker_lookup[y][x];
  if (idx >= 0)
  {
    markers[idx].sector = sector;
    snprintf(markers[idx].title, sizeof(markers[idx].title), "%s", (title && *title) ? title : "Unknown marker");
    return TRUE;
  }

  if (*marker_count >= WORLDMAP_MARKER_MAX)
    return FALSE;

  idx = *marker_count;
  markers[idx].id = idx + 1;
  markers[idx].x = x;
  markers[idx].y = y;
  markers[idx].sector = sector;
  snprintf(markers[idx].title, sizeof(markers[idx].title), "%s", (title && *title) ? title : "Unknown marker");
  marker_lookup[y][x] = idx;
  (*marker_count)++;
  return TRUE;
}

static void worldmap_collect_zone_grid(const struct worldmap_zone_export_data *zone,
                                       int sector_grid[WORLDMAP_EXPORT_MAX_HEIGHT][WORLDMAP_EXPORT_MAX_WIDTH],
                                       int *max_x, int *max_y)
{
  room_vnum i;
  int x;
  int y;

  for (i = genolc_zone_bottom(zone->zrnum); i <= zone_table[zone->zrnum].top; i++)
  {
    room_rnum rnum = real_room(i);
    int map_x;
    int map_y;

    if (rnum == NOWHERE)
      continue;

    if (!worldmap_vnum_to_xy(zone->zvnum, i, &x, &y))
      continue;

    map_x = x + zone->x_offset;
    map_y = y + zone->y_offset;
    if (map_x < 0 || map_x >= WORLDMAP_EXPORT_MAX_WIDTH || map_y < 0 || map_y >= WORLDMAP_EXPORT_MAX_HEIGHT)
      continue;

    sector_grid[map_y][map_x] = world[rnum].sector_type;
    *max_x = MAX(*max_x, map_x);
    *max_y = MAX(*max_y, map_y);
  }
}

static void worldmap_collect_zone_markers(const struct worldmap_zone_export_data *zone,
                                          int sector_grid[WORLDMAP_EXPORT_MAX_HEIGHT][WORLDMAP_EXPORT_MAX_WIDTH],
                                          int marker_lookup[WORLDMAP_EXPORT_MAX_HEIGHT][WORLDMAP_EXPORT_MAX_WIDTH],
                                          struct worldmap_marker_data *markers, int *marker_count)
{
  room_vnum rvnum;
  int i;
  int x;
  int y;

  if (zone->use_asciimap_points)
  {
    for (i = 0; i < NUM_MAP_POINTS; i++)
    {
      room_vnum point_vnum;
      int map_x;
      int map_y;

      if (!*asciimap_points[i][0])
        continue;

      point_vnum = atoi(asciimap_points[i][1]);
      if (!worldmap_vnum_to_xy(zone->zvnum, point_vnum, &x, &y))
        continue;

      map_x = x + zone->x_offset;
      map_y = y + zone->y_offset;
      if (!worldmap_add_marker(markers, marker_lookup, marker_count, map_x, map_y, sector_grid[map_y][map_x],
                               asciimap_points[i][0]))
        mudlog(BRF, LVL_STAFF, TRUE, "SYSERR: export_worldmap_html: marker table full for zone %d", zone->zvnum);
    }
    return;
  }

  for (rvnum = genolc_zone_bottom(zone->zrnum); rvnum <= zone_table[zone->zrnum].top; rvnum++)
  {
    room_rnum rnum = real_room(rvnum);
    int map_x;
    int map_y;
    char title_buf[MAX_INPUT_LENGTH];

    if (rnum == NOWHERE)
      continue;

    if (!worldmap_vnum_to_xy(zone->zvnum, rvnum, &x, &y))
      continue;

    if (world[rnum].sector_type != SECT_ZONE_START && world[rnum].sector_type != SECT_SEAPORT)
      continue;

    map_x = x + zone->x_offset;
    map_y = y + zone->y_offset;
    snprintf(title_buf, sizeof(title_buf), "%s (%d)",
             world[rnum].name ? world[rnum].name : "Unknown Room", (int)rvnum);
    if (!worldmap_add_marker(markers, marker_lookup, marker_count, map_x, map_y, world[rnum].sector_type, title_buf))
      mudlog(BRF, LVL_STAFF, TRUE, "SYSERR: export_worldmap_html: marker table full for zone %d", zone->zvnum);
  }
}

static int export_worldmap_html(zone_rnum zrnum, const char *output_file, char *saved_path, size_t saved_path_size)
{
  FILE *out;
  int sector_grid[WORLDMAP_EXPORT_MAX_HEIGHT][WORLDMAP_EXPORT_MAX_WIDTH];
  int marker_lookup[WORLDMAP_EXPORT_MAX_HEIGHT][WORLDMAP_EXPORT_MAX_WIDTH];
  int marker_count = 0;
  int max_x = -1;
  int max_y = -1;
  zone_vnum zvnum = zone_table[zrnum].number;
  room_vnum anchor = worldmap_zone_anchor(zvnum);
  struct worldmap_marker_data markers[WORLDMAP_MARKER_MAX];
  struct worldmap_zone_export_data zone = {zrnum, zvnum, 0, 0, FALSE};
  char subtitle[MAX_INPUT_LENGTH];

  if (real_room(anchor) == NOWHERE)
    return FALSE;

  worldmap_init_export_buffers(sector_grid, marker_lookup);
  zone.use_asciimap_points = (zvnum == 6000);
  worldmap_collect_zone_grid(&zone, sector_grid, &max_x, &max_y);
  worldmap_collect_zone_markers(&zone, sector_grid, marker_lookup, markers, &marker_count);

  if (max_y < 0)
    return FALSE;

  snprintf(saved_path, saved_path_size, "../lib/world/export/%s", output_file);
  if (!(out = fopen_restricted(saved_path + 7, "w")))
  {
    mudlog(BRF, LVL_STAFF, TRUE, "SYSERR: export_worldmap_html: Cannot open %s", saved_path);
    return FALSE;
  }

  snprintf(subtitle, sizeof(subtitle), "%s (%d)", zone_table[zrnum].name, zvnum);
  worldmap_html_write_document(out, subtitle, sector_grid, marker_lookup, markers, marker_count, WORLDMAP_ZONE_WIDTH,
                               max_y + 1);
  fclose(out);
  return TRUE;
}

static int export_full_worldmap_html(const char *output_file, char *saved_path, size_t saved_path_size)
{
  static const zone_vnum full_map_zone_vnums[4] = {6000, 7000, 8000, 9000};
  struct worldmap_zone_export_data zones[4];
  struct worldmap_marker_data markers[WORLDMAP_MARKER_MAX];
  int sector_grid[WORLDMAP_EXPORT_MAX_HEIGHT][WORLDMAP_EXPORT_MAX_WIDTH];
  int marker_lookup[WORLDMAP_EXPORT_MAX_HEIGHT][WORLDMAP_EXPORT_MAX_WIDTH];
  int zone_heights[4];
  int marker_count = 0;
  int top_row_height;
  int bottom_row_height;
  int map_height;
  int max_x = -1;
  int max_y = -1;
  int i;
  FILE *out;

  for (i = 0; i < 4; i++)
  {
    zones[i].zvnum = full_map_zone_vnums[i];
    zones[i].zrnum = real_zone(zones[i].zvnum);
    if (zones[i].zrnum == NOWHERE || real_room(worldmap_zone_anchor(zones[i].zvnum)) == NOWHERE)
      return FALSE;

    zone_heights[i] = worldmap_zone_height(zones[i].zrnum);
    if (zone_heights[i] <= 0)
      return FALSE;
  }

  top_row_height = MAX(zone_heights[0], zone_heights[1]);
  bottom_row_height = MAX(zone_heights[2], zone_heights[3]);
  map_height = top_row_height + bottom_row_height;

  zones[0].x_offset = 0;
  zones[0].y_offset = 0;
  zones[0].use_asciimap_points = TRUE;

  zones[1].x_offset = WORLDMAP_ZONE_WIDTH;
  zones[1].y_offset = 0;
  zones[1].use_asciimap_points = FALSE;

  zones[2].x_offset = 0;
  zones[2].y_offset = top_row_height;
  zones[2].use_asciimap_points = FALSE;

  zones[3].x_offset = WORLDMAP_ZONE_WIDTH;
  zones[3].y_offset = top_row_height;
  zones[3].use_asciimap_points = FALSE;

  worldmap_init_export_buffers(sector_grid, marker_lookup);

  for (i = 0; i < 4; i++)
  {
    worldmap_collect_zone_grid(&zones[i], sector_grid, &max_x, &max_y);
    worldmap_collect_zone_markers(&zones[i], sector_grid, marker_lookup, markers, &marker_count);
  }

  if (max_x < 0 || max_y < 0)
    return FALSE;

  snprintf(saved_path, saved_path_size, "../lib/world/export/%s", output_file);
  if (!(out = fopen_restricted(saved_path + 7, "w")))
  {
    mudlog(BRF, LVL_STAFF, TRUE, "SYSERR: export_full_worldmap_html: Cannot open %s", saved_path);
    return FALSE;
  }

  worldmap_html_write_document(out, "Connected Wilderness Zones (6000, 7000, 8000, 9000)", sector_grid,
                               marker_lookup, markers, marker_count, WORLDMAP_ZONE_WIDTH * 2, map_height);
  fclose(out);
  return TRUE;
}

char *str_udupnl(const char *txt)
{
  char *str = NULL, undef[] = "undefined";
  const char *ptr = NULL;
  size_t size;

  ptr = (txt && *txt) ? txt : undef;
  size = strlen(ptr) + 3;
  CREATE(str, char, size);

  strlcpy(str, ptr, size);
  strlcat(str, "\r\n", size);

  return str;
}

/* Original use: to be called at shutdown time. */
int save_all(void)
{
  while (save_list)
  {
    if (save_list->type < 0 || save_list->type > SL_MAX)
    {
      switch (save_list->type)
      {
      case SL_ACT:
        log("Actions not saved - can not autosave. Use 'aedit save'.");
        save_list = save_list->next; /* Fatal error, skip this one. */
        break;
      case SL_HLP:
        log("Help not saved - can not autosave. Use 'hedit save'.");
        save_list = save_list->next; /* Fatal error, skip this one. */
        break;
      default:
        log("SYSERR: GenOLC: Invalid save type %d in save list.\n", save_list->type);
        break;
      }
    }
    else if ((*save_types[save_list->type].func)(real_zone(save_list->zone)) < 0)
      save_list = save_list->next; /* Fatal error, skip this one. */
  }
  return TRUE;
}

/* NOTE: This changes the buffer passed in. */
void strip_cr(char *buffer)
{
  int rpos, wpos;

  if (buffer == NULL)
    return;

  for (rpos = 0, wpos = 0; buffer[rpos]; rpos++)
  {
    buffer[wpos] = buffer[rpos];
    wpos += (buffer[rpos] != '\r');
  }
  buffer[wpos] = '\0';
}

/* NOTE: This changes the buffer passed in. */
void strip_nl(char *buffer)
{
  int rpos, wpos;

  if (buffer == NULL)
    return;

  for (rpos = 0, wpos = 0; buffer[rpos]; rpos++)
  {
    buffer[wpos] = buffer[rpos];
    wpos += (buffer[rpos] != '\n');
  }
  buffer[wpos] = '\0';
}

void copy_ex_descriptions(struct extra_descr_data **to, struct extra_descr_data *from)
{
  struct extra_descr_data *wpos;

  CREATE(*to, struct extra_descr_data, 1);
  wpos = *to;

  for (; from; from = from->next, wpos = wpos->next)
  {
    wpos->keyword = str_udup(from->keyword);
    wpos->description = str_udup(from->description);
    if (from->next)
      CREATE(wpos->next, struct extra_descr_data, 1);
  }
}

void free_ex_descriptions(struct extra_descr_data *head)
{
  struct extra_descr_data *thised, *next_one;

  if (!head)
  {
    log("free_ex_descriptions: NULL pointer or NULL data.");
    return;
  }

  for (thised = head; thised; thised = next_one)
  {
    next_one = thised->next;
    if (thised->keyword)
      free(thised->keyword);
    if (thised->description)
      free(thised->description);
    free(thised);
  }
}

int remove_from_save_list(zone_vnum zone, int type)
{
  struct save_list_data *ritem, *temp;

  for (ritem = save_list; ritem; ritem = ritem->next)
    if (ritem->zone == zone && ritem->type == type)
      break;

  if (ritem == NULL)
  {
    log("SYSERR: remove_from_save_list: Saved item not found. (%d/%d)", zone, type);
    return FALSE;
  }
  REMOVE_FROM_LIST(ritem, save_list, next);
  free(ritem);
  return TRUE;
}

int add_to_save_list(zone_vnum zone, int type)
{
  struct save_list_data *nitem;
  zone_rnum rznum;

  if (type == SL_CFG)
    return FALSE;

  rznum = real_zone(zone);
  if (rznum == NOWHERE || rznum > top_of_zone_table)
  {
    if (zone != AEDIT_PERMISSION && zone != HEDIT_PERMISSION)
    {
      log("SYSERR: add_to_save_list: Invalid zone number passed. (%d => %d, 0-%d)", zone, rznum,
          top_of_zone_table);
      return FALSE;
    }
  }

  for (nitem = save_list; nitem; nitem = nitem->next)
    if (nitem->zone == zone && nitem->type == type)
      return FALSE;

  CREATE(nitem, struct save_list_data, 1);
  nitem->zone = zone;
  nitem->type = type;
  nitem->next = save_list;
  save_list = nitem;
  return TRUE;
}

int in_save_list(zone_vnum zone, int type)
{
  struct save_list_data *nitem;

  for (nitem = save_list; nitem; nitem = nitem->next)
    if (nitem->zone == zone && nitem->type == type)
      return TRUE;

  return FALSE;
}

void free_save_list(void)
{
  struct save_list_data *sld, *next_sld;

  for (sld = save_list; sld; sld = next_sld)
  {
    next_sld = sld->next;
    free(sld);
  }
}

/* Used from do_show(), ideally. */
ACMD(do_show_save_list)
{
  if (save_list == NULL)
    send_to_char(ch, "All world files are up to date.\r\n");
  else
  {
    struct save_list_data *item;

    send_to_char(ch, "The following files need saving:\r\n");
    for (item = save_list; item; item = item->next)
    {
      if (item->type != SL_CFG)
        send_to_char(ch, " - %s data for zone %d.\r\n", save_types[item->type].message, item->zone);
      else
        send_to_char(ch, " - Game configuration data.\r\n");
    }
  }
}

room_vnum genolc_zonep_bottom(struct zone_data *zone)
{
  return zone->bot;
}

zone_vnum genolc_zone_bottom(zone_rnum rznum)
{
  return zone_table[rznum].bot;
}

int sprintascii(char *out, bitvector_t bits)
{
  int i, j = 0;
  /* 32 bits, don't just add letters to try to get more unless your bitvector_t is also as large. */
  const char *flags = "abcdefghijklmnopqrstuvwxyzABCDEF";

  for (i = 0; flags[i] != '\0'; i++)
    if (bits & ((bitvector_t)1 << i))
      out[j++] = flags[i];

  if (j == 0) /* Didn't write anything. */
    out[j++] = '0';

  /* NUL terminate the output string. */
  out[j++] = '\0';
  return j;
}

/* converts illegal filename chars into appropriate equivalents */
const char *fix_filename(char *str)
{
  static char good_file_name[MAX_EXPORT_FILENAME] = {'\0'};
  char *cindex = good_file_name;

  while (*str)
  {
    switch (*str)
    {
    case ' ':
      *cindex = '_';
      cindex++;
      break;
    case '(':
      *cindex = '{';
      cindex++;
      break;
    case ')':
      *cindex = '}';
      cindex++;
      break;

      /* skip the following */
    case '\'':
      break;
    case '"':
      break;

      /* Legal character */
    default:
      *cindex = *str;
      cindex++;
      break;
    }
    str++;
  }
  *cindex = '\0';

  return good_file_name;
}

/* Export command by Kyle */
ACMD(do_export_zone)
{
  zone_rnum zrnum;
  zone_vnum zvnum;
  char sysbuf[MAX_INPUT_LENGTH] = {'\0'};
  char zone_name[MAX_EXPORT_FILENAME] = {'\0'}; /* zone names are short */
  const char *f;
  int success;

  /* system command locations are relative to
   * where the binary IS, not where it was run
   * from, thus we act like we are in the bin
   * folder, because we are*/
  const char *path = "../lib/world/export/";

  if (IS_NPC(ch) || GET_LEVEL(ch) < LVL_IMPL)
    return;

  skip_spaces_c(&argument);
  if (!*argument)
  {
    send_to_char(ch, "Syntax: export <zone vnum>");
    return;
  }

  zvnum = atoi(argument);
  zrnum = real_zone(zvnum);

  if (zrnum == NOWHERE)
  {
    send_to_char(ch, "Export which zone?\r\n");
    return;
  }

  /* If we fail, it might just be because the
   *   directory didn't exist.  Can't hurt to try
   *     again. Do it silently though ( no logs ). */
  if (!export_info_file(zrnum))
  {
    snprintf(sysbuf, sizeof(sysbuf), "mkdir %s", path);
    if (system(sysbuf) == -1)
    {
      log("SYSERR: Failed to create directory %s", path);
    }
  }

  if (!(success = export_info_file(zrnum)))
    send_to_char(ch, "Info file not saved!\r\n");
  if (!(success = export_save_shops(zrnum)))
    send_to_char(ch, "Shops not saved!\r\n");
  if (!(success = export_save_mobiles(zrnum)))
    send_to_char(ch, "Mobiles not saved!\r\n");
  if (!(success = export_save_objects(zrnum)))
    send_to_char(ch, "Objects not saved!\r\n");
  if (!(success = export_save_zone(zrnum)))
    send_to_char(ch, "Zone info not saved!\r\n");
  if (!(success = export_save_rooms(zrnum)))
    send_to_char(ch, "Rooms not saved!\r\n");
  if (!(success = export_save_triggers(zrnum)))
    send_to_char(ch, "Triggers not saved!\r\n");

  /* If anything went wrong, don't try to tar the files. */
  if (success)
  {
    send_to_char(ch, "Individual files saved to /lib/world/export.\r\n");
    snprintf(zone_name, sizeof(zone_name), "%s", zone_table[zrnum].name);
  }
  else
  {
    send_to_char(ch, "Ran into problems writing to files.\r\n");
    return;
  }
  /* Make sure the name of the zone doesn't make the filename illegal. */
  f = fix_filename(zone_name);

  /* Remove the old copy. */
  snprintf(sysbuf, sizeof(sysbuf), "rm %s%s.tar.gz", path, f);
  if (system(sysbuf) == -1)
  {
    log("SYSERR: Failed to remove old tar file");
  }

  /* Tar the new copy. */
  snprintf(sysbuf, sizeof(sysbuf),
           "tar -cf %s%s.tar %sqq.info %sqq.wld %sqq.zon %sqq.mob %sqq.obj %sqq.trg %sqq.shp", path,
           f, path, path, path, path, path, path, path);
  if (system(sysbuf) == -1)
  {
    log("SYSERR: Failed to create tar file");
  }

  /* Gzip it. */
  snprintf(sysbuf, sizeof(sysbuf), "gzip %s%s.tar", path, f);
  if (system(sysbuf) == -1)
  {
    log("SYSERR: Failed to gzip tar file");
  }

  send_to_char(ch, "Files tar'ed to \"%s%s.tar.gz\"\r\n", path, f);
}

ACMD(do_export_map)
{
  zone_rnum zrnum;
  zone_vnum zvnum;
  char arg_copy[MAX_INPUT_LENGTH] = {'\0'};
  char zone_arg[MAX_INPUT_LENGTH] = {'\0'};
  char file_arg[MAX_INPUT_LENGTH] = {'\0'};
  char sysbuf[MAX_INPUT_LENGTH] = {'\0'};
  char output_file[MAX_INPUT_LENGTH] = {'\0'};
  char safe_name[MAX_EXPORT_FILENAME] = {'\0'};
  char saved_path[MAX_INPUT_LENGTH] = {'\0'};
  const char *path = "../lib/world/export/";
  const char *fixed_name;

  if (IS_NPC(ch) || GET_LEVEL(ch) < LVL_IMPL)
    return;

  snprintf(arg_copy, sizeof(arg_copy), "%s", argument ? argument : "");
  half_chop(arg_copy, zone_arg, file_arg);
  if (!*zone_arg)
  {
    send_to_char(ch, "Syntax: exportmap <zone vnum> [filename.html]\r\n");
    return;
  }

  zvnum = atoi(zone_arg);
  zrnum = real_zone(zvnum);

  if (zrnum == NOWHERE)
  {
    send_to_char(ch, "Export which worldmap zone?\r\n");
    return;
  }

  if (real_room(worldmap_zone_anchor(zvnum)) == NOWHERE)
  {
    send_to_char(ch, "That zone does not have a worldmap anchor room ending in 161.\r\n");
    return;
  }

  snprintf(sysbuf, sizeof(sysbuf), "mkdir -p %s", path);
  if (system(sysbuf) == -1)
    log("SYSERR: Failed to create directory %s", path);

  if (*file_arg)
  {
    fixed_name = fix_filename(file_arg);
    snprintf(output_file, sizeof(output_file), "%s", fixed_name);
  }
  else
  {
    snprintf(safe_name, sizeof(safe_name), "%s_zone_%d", zone_table[zrnum].name, zvnum);
    fixed_name = fix_filename(safe_name);
    snprintf(output_file, sizeof(output_file), "%s.html", fixed_name);
  }

  if (!strstr(output_file, ".html"))
  {
    size_t used = strlen(output_file);

    if (used + 5 < sizeof(output_file))
      snprintf(output_file + used, sizeof(output_file) - used, ".html");
  }

  if (!export_worldmap_html(zrnum, output_file, saved_path, sizeof(saved_path)))
  {
    send_to_char(ch, "Worldmap HTML export failed.\r\n");
    return;
  }

  send_to_char(ch, "Worldmap saved to \"%s\"\r\n", saved_path);
}

ACMD(do_export_worldmap)
{
  char file_arg[MAX_INPUT_LENGTH] = {'\0'};
  char sysbuf[MAX_INPUT_LENGTH] = {'\0'};
  char output_file[MAX_INPUT_LENGTH] = {'\0'};
  char saved_path[MAX_INPUT_LENGTH] = {'\0'};
  const char *path = "../lib/world/export/";
  const char *fixed_name;

  if (IS_NPC(ch) || GET_LEVEL(ch) < LVL_IMPL)
    return;

  one_argument(argument ? argument : "", file_arg, sizeof(file_arg));

  snprintf(sysbuf, sizeof(sysbuf), "mkdir -p %s", path);
  if (system(sysbuf) == -1)
    log("SYSERR: Failed to create directory %s", path);

  if (*file_arg)
  {
    fixed_name = fix_filename(file_arg);
    snprintf(output_file, sizeof(output_file), "%s", fixed_name);
  }
  else
  {
    fixed_name = fix_filename("faerun_worldmap");
    snprintf(output_file, sizeof(output_file), "%s.html", fixed_name);
  }

  if (!strstr(output_file, ".html"))
  {
    size_t used = strlen(output_file);

    if (used + 5 < sizeof(output_file))
      snprintf(output_file + used, sizeof(output_file) - used, ".html");
  }

  if (!export_full_worldmap_html(output_file, saved_path, sizeof(saved_path)))
  {
    send_to_char(ch, "Connected worldmap HTML export failed.\r\n");
    return;
  }

  send_to_char(ch, "Connected worldmap saved to \"%s\"\r\n", saved_path);
}

static int export_info_file(zone_rnum zrnum)
{
  room_vnum i;
  FILE *info_file;

  if (!(info_file = fopen_restricted("world/export/qq.info", "w")))
  {
    mudlog(BRF, LVL_STAFF, TRUE, "SYSERR: export_info_file : Cannot open file!");
    return FALSE;
  }
  else if (fprintf(info_file, "LuminariMUD Area file.\n") < 0)
  {
    mudlog(BRF, LVL_STAFF, TRUE, "SYSERR: export_info_file: Cannot write to file!");
    fclose(info_file);
    return FALSE;
  }

  fprintf(info_file, "The files accompanying this info file contain the area: %s\n",
          zone_table[zrnum].name);
  fprintf(info_file, "It was written by: %s.\n\n", zone_table[zrnum].builders);
  fprintf(info_file,
          "The author has given permission to distribute the area, provided credit is\n");
  fprintf(info_file,
          "given. The area may be modified as you see fit, except you are not allowed to\n");
  fprintf(info_file, "remove the builder name or credits.\n\n");
  fprintf(info_file, "Implementation:\n");
  fprintf(info_file,
          "1. All the files have been QQ'ed. This means all occurences of the zone number\n");
  fprintf(info_file,
          "   have been changed to QQ. In other words, if you decide to have this zone as\n");
  fprintf(info_file,
          "   zone 123, replace all occurences of QQ with 123 and rename the qq.zon file\n");
  fprintf(info_file,
          "   to 123.zon (etc.). And of course add 123.zon to the respective index file.\n");
  if (zone_exits)
  {
    fprintf(info_file,
            "2. Exits out of this zone have been ZZ'd. So all doors leading out have ZZ??\n");
    fprintf(info_file, "   instead of the room vnum (?? are numbers 00 - 99).\n");
    fprintf(info_file, "   In this zone, the exit rooms in question are:\n");

    for (i = genolc_zone_bottom(zrnum); i <= zone_table[zrnum].top; i++)
    {
      room_rnum rnum = real_room(i);
      struct room_data *room;
      int j;

      if (rnum == NOWHERE)
        continue;

      room = &world[rnum];

      for (j = 0; j < DIR_COUNT; j++)
      {
        if (!R_EXIT(room, j))
          continue;

        if (R_EXIT(room, j)->to_room == NOWHERE || world[R_EXIT(room, j)->to_room].zone == zrnum)
          continue;

        fprintf(info_file, "      Room QQ%02d : Exit to the %s\n", room->number % 100, dirs[j]);
      }
    }
    zone_exits = 0;
  }
  else
  {
    fprintf(info_file, "2. This area doesn't have any exits _out_ of the zone.\n");
    fprintf(info_file,
            "   More info on connections can be found in the zone description room (QQ00).\n");
  }

  fprintf(info_file,
          "\nAdditional zone information is available in the zone description room QQ00.\n");
  fprintf(info_file, "Luminari MUD is maintaining and improving these zones.\n\n");

  fclose(info_file);
  return TRUE;
}

static int export_save_shops(zone_rnum zrnum)
{
  shop_vnum i;
  shop_rnum rshop;
  int j;
  FILE *shop_file;
  struct shop_data *shop;

  if (!(shop_file = fopen_restricted("world/export/qq.shp", "w")))
  {
    mudlog(BRF, LVL_STAFF, TRUE, "SYSERR: export_save_shops : Cannot open shop file!");
    return FALSE;
  }
  else if (fprintf(shop_file, "LuminariMUD v3.0 Shop File~\n") < 0)
  {
    mudlog(BRF, LVL_STAFF, TRUE, "SYSERR: export_save_shops: Cannot write to shop file!");
    fclose(shop_file);
    return FALSE;
  }
  /* Search database for shops in this zone. */
  for (i = genolc_zone_bottom(zrnum); i <= zone_table[zrnum].top; i++)
  {
    if ((rshop = real_shop(i)) != NOWHERE)
    {
      fprintf(shop_file, "#QQ%02d~\n", i % 100);
      shop = &shop_index[rshop];

      /* Save the products. */
      for (j = 0; S_PRODUCT(shop, j) != NOTHING; j++)
      {
        if (obj_index[S_PRODUCT(shop, j)].vnum < genolc_zone_bottom(zrnum) ||
            obj_index[S_PRODUCT(shop, j)].vnum > zone_table[zrnum].top)
          continue;

        fprintf(shop_file, "QQ%02d\n", obj_index[S_PRODUCT(shop, j)].vnum % 100);
      }
      fprintf(shop_file, "-1\n");

      /* Save the rates. */
      fprintf(shop_file,
              "%1.2f\n"
              "%1.2f\n",
              S_BUYPROFIT(shop), S_SELLPROFIT(shop));

      /* Save the buy types and namelists. */
      for (j = 0; S_BUYTYPE(shop, j) != (int)NOTHING; j++)
        fprintf(shop_file, "%d%s\n", S_BUYTYPE(shop, j),
                S_BUYWORD(shop, j) ? S_BUYWORD(shop, j) : "");
      fprintf(shop_file, "-1\n");

      /* Save messages. Added some defaults as sanity checks. */
      fprintf(
          shop_file,
          "%s~\n"
          "%s~\n"
          "%s~\n"
          "%s~\n"
          "%s~\n"
          "%s~\n"
          "%s~\n"
          "%d\n"
          "%ld\n"
          "QQ%02d\n"
          "%d\n",
          S_NOITEM1(shop) ? S_NOITEM1(shop) : "%s Ke?!",
          S_NOITEM2(shop) ? S_NOITEM2(shop) : "%s Ke?!", S_NOBUY(shop) ? S_NOBUY(shop) : "%s Ke?!",
          S_NOCASH1(shop) ? S_NOCASH1(shop) : "%s Ke?!",
          S_NOCASH2(shop) ? S_NOCASH2(shop) : "%s Ke?!", S_BUY(shop) ? S_BUY(shop) : "%s Ke?! %d?",
          S_SELL(shop) ? S_SELL(shop) : "%s Ke?! %d?", S_BROKE_TEMPER(shop), S_BITVECTOR(shop),
          mob_index[S_KEEPER(shop)].vnum % 100, S_NOTRADE(shop));

      /* Save the rooms. */
      for (j = 0; S_ROOM(shop, j) != NOWHERE; j++)
      {
        if (S_ROOM(shop, j) < genolc_zone_bottom(zrnum) || S_ROOM(shop, j) > zone_table[zrnum].top)
          continue;

        fprintf(shop_file, "QQ%02d\n", S_ROOM(shop, j) % 100);
      }
      fprintf(shop_file, "-1\n");

      /* Save open/closing times. */
      fprintf(shop_file, "%d\n%d\n%d\n%d\n", S_OPEN1(shop), S_CLOSE1(shop), S_OPEN2(shop),
              S_CLOSE2(shop));
    }
  }
  fprintf(shop_file, "$~\n");
  fclose(shop_file);

  return TRUE;
}

static int export_save_mobiles(zone_rnum rznum)
{
  FILE *mob_file;
  mob_vnum i;
  mob_rnum rmob;

  if (!(mob_file = fopen_restricted("world/export/qq.mob", "w")))
  {
    mudlog(BRF, LVL_STAFF, TRUE, "SYSERR: export_save_mobiles : Cannot open file!");
    return FALSE;
  }

  for (i = genolc_zone_bottom(rznum); i <= zone_table[rznum].top; i++)
  {
    if ((rmob = real_mobile(i)) == NOBODY)
      continue;
    check_mobile_strings(&mob_proto[rmob]);
    if (export_mobile_record(i, &mob_proto[rmob], mob_file) < 0)
      log("SYSERR: export_save_mobiles: Error writing mobile #%d.", i);
  }
  fputs("$\n", mob_file);
  fclose(mob_file);

  return TRUE;
}

static int export_mobile_record(mob_vnum mvnum, struct char_data *mob, FILE *fd)
{
  int pos = GET_DEFAULT_POS(mob);
  char ldesc[MAX_STRING_LENGTH] = {'\0'};
  char ddesc[MAX_STRING_LENGTH] = {'\0'};

  ldesc[MAX_STRING_LENGTH - 1] = '\0';
  ddesc[MAX_STRING_LENGTH - 1] = '\0';
  strip_cr(strncpy(ldesc, GET_LDESC(mob), MAX_STRING_LENGTH - 1));
  strip_cr(strncpy(ddesc, GET_DDESC(mob), MAX_STRING_LENGTH - 1));

  fprintf(fd,
          "#QQ%02d\n"
          "%s%c\n"
          "%s%c\n"
          "%s%c\n"
          "%s%c\n",
          mvnum % 100, GET_ALIAS(mob), STRING_TERMINATOR, GET_SDESC(mob), STRING_TERMINATOR, ldesc,
          STRING_TERMINATOR, ddesc, STRING_TERMINATOR);

  fprintf(fd,
          "%d %d %d %d %d %d %d %d %d E\n"
          "%d %d %d %dd%d+%d %dd%d+%d\n",
          MOB_FLAGS(mob)[0], MOB_FLAGS(mob)[1], MOB_FLAGS(mob)[2], MOB_FLAGS(mob)[3],
          AFF_FLAGS(mob)[0], AFF_FLAGS(mob)[1], AFF_FLAGS(mob)[2], AFF_FLAGS(mob)[3],
          GET_ALIGNMENT(mob), GET_LEVEL(mob), 20 - GET_HITROLL(mob), GET_AC(mob) / 10, GET_HIT(mob),
          GET_PSP(mob), GET_MOVE(mob), GET_NDD(mob), GET_SDD(mob), GET_DAMROLL(mob));

  /* pos_fighting is deprecated */
  if (pos == POS_FIGHTING)
    pos = POS_STANDING;

  fprintf(fd,
          "%d %ld\n"
          "%d %d %d\n",
          GET_GOLD(mob), GET_EXP(mob), GET_POS(mob), pos, GET_SEX(mob));

  if (write_mobile_espec(mvnum, mob, fd) < 0)
    log("SYSERR: GenOLC: Error writing E-specs for mobile #%d.", mvnum);

  export_script_save_to_disk(fd, mob, MOB_TRIGGER);

  return TRUE;
}

static int export_save_zone(zone_rnum zrnum)
{
  int subcmd;
  FILE *zone_file;

  if (!(zone_file = fopen_restricted("world/export/qq.zon", "w")))
  {
    mudlog(BRF, LVL_STAFF, TRUE, "SYSERR: export_save_zone : Cannot open file!");
    return FALSE;
  }

  /* Print zone header to file. */
  fprintf(zone_file,
          "#QQ\n"
          "%s~\n"
          "%s~\n"
          "QQ%02d QQ%02d %d %d\n",
          (zone_table[zrnum].builders && *zone_table[zrnum].builders) ? zone_table[zrnum].builders
                                                                      : "None.",
          (zone_table[zrnum].name && *zone_table[zrnum].name) ? zone_table[zrnum].name
                                                              : "undefined",
          genolc_zone_bottom(zrnum) % 100, zone_table[zrnum].top % 100, zone_table[zrnum].lifespan,
          zone_table[zrnum].reset_mode);

  /* Handy Quick Reference Chart for Zone Values.
   *
   * Field #1    Field #3   Field #4  Field #5
   * -------------------------------------------------
   * M (Mobile)  Mob-Vnum   Wld-Max   Room-Vnum
   * O (Object)  Obj-Vnum   Wld-Max   Room-Vnum
   * G (Give)    Obj-Vnum   Wld-Max   Unused
   * E (Equip)   Obj-Vnum   Wld-Max   EQ-Position
   * P (Put)     Obj-Vnum   Wld-Max   Target-Obj-Vnum
   * D (Door)    Room-Vnum  Door-Dir  Door-State
   * R (Remove)  Room-Vnum  Obj-Vnum  Unused
   * T (Trigger) Trig-type  Trig-Vnum Room-Vnum
   * V (var)     Trig-type  Context   Room-Vnum Varname Value
   * ------------------------------------------------- */

  for (subcmd = 0; ZCMD(zrnum, subcmd).command != 'S'; subcmd++)
  {
    switch (ZCMD(zrnum, subcmd).command)
    {
    case 'M':
      fprintf(zone_file, "M %d QQ%02d %d QQ%02d \t(%s)\n", ZCMD(zrnum, subcmd).if_flag,
              mob_index[ZCMD(zrnum, subcmd).arg1].vnum % 100, ZCMD(zrnum, subcmd).arg2,
              world[ZCMD(zrnum, subcmd).arg3].number % 100,
              mob_proto[ZCMD(zrnum, subcmd).arg1].player.short_descr);
      break;
    case 'O':
      fprintf(zone_file, "O %d QQ%02d %d QQ%02d \t(%s)\n", ZCMD(zrnum, subcmd).if_flag,
              obj_index[ZCMD(zrnum, subcmd).arg1].vnum % 100, ZCMD(zrnum, subcmd).arg2,
              world[ZCMD(zrnum, subcmd).arg3].number % 100,
              obj_proto[ZCMD(zrnum, subcmd).arg1].short_description);
      break;
    case 'G':
      fprintf(zone_file, "G %d QQ%02d %d -1 \t(%s)\n", ZCMD(zrnum, subcmd).if_flag,
              obj_index[ZCMD(zrnum, subcmd).arg1].vnum % 100, ZCMD(zrnum, subcmd).arg2,
              obj_proto[ZCMD(zrnum, subcmd).arg1].short_description);
      break;
    case 'E':
      fprintf(zone_file, "E %d QQ%02d %d %d \t(%s)\n", ZCMD(zrnum, subcmd).if_flag,
              obj_index[ZCMD(zrnum, subcmd).arg1].vnum % 100, ZCMD(zrnum, subcmd).arg2,
              ZCMD(zrnum, subcmd).arg3, obj_proto[ZCMD(zrnum, subcmd).arg1].short_description);
      break;
    case 'P':
      fprintf(zone_file, "P %d QQ%02d %d QQ%02d \t(%s)\n", ZCMD(zrnum, subcmd).if_flag,
              obj_index[ZCMD(zrnum, subcmd).arg1].vnum % 100, ZCMD(zrnum, subcmd).arg2,
              obj_index[ZCMD(zrnum, subcmd).arg3].vnum % 100,
              obj_proto[ZCMD(zrnum, subcmd).arg1].short_description);
      break;
    case 'D':
      fprintf(zone_file, "D %d QQ%02d %d %d \t(%s)\n", ZCMD(zrnum, subcmd).if_flag,
              world[ZCMD(zrnum, subcmd).arg1].number % 100, ZCMD(zrnum, subcmd).arg2,
              ZCMD(zrnum, subcmd).arg3, world[ZCMD(zrnum, subcmd).arg1].name);
      break;
    case 'R':
      fprintf(zone_file, "R %d QQ%02d QQ%02d %d \t(%s)\n", ZCMD(zrnum, subcmd).if_flag,
              world[ZCMD(zrnum, subcmd).arg1].number % 100,
              obj_index[ZCMD(zrnum, subcmd).arg2].vnum % 100,
              ZCMD(zrnum, subcmd).arg4 ? ZCMD(zrnum, subcmd).arg3 : -1,
              obj_proto[ZCMD(zrnum, subcmd).arg2].short_description);
      break;
    case 'F':
      fprintf(zone_file, "F %d QQ%02d QQ%02d QQ%02d %d \t(RoL follow/group/mount)\n",
              ZCMD(zrnum, subcmd).if_flag, world[ZCMD(zrnum, subcmd).arg1].number % 100,
              mob_index[ZCMD(zrnum, subcmd).arg2].vnum % 100,
              mob_index[ZCMD(zrnum, subcmd).arg3].vnum % 100, ZCMD(zrnum, subcmd).arg4);
      break;
    case 'K':
      fprintf(zone_file, "K %d QQ%02d %d %d %d \t(RoL legacy door state)\n",
              ZCMD(zrnum, subcmd).if_flag, world[ZCMD(zrnum, subcmd).arg1].number % 100,
              ZCMD(zrnum, subcmd).arg2, ZCMD(zrnum, subcmd).arg3, ZCMD(zrnum, subcmd).arg4);
      break;
    case 'X':
      if (ZCMD(zrnum, subcmd).arg1 == -1)
        fprintf(zone_file, "X %d -1 QQ%02d %d %d \t(RoL mobile removal)\n",
                ZCMD(zrnum, subcmd).if_flag, mob_index[ZCMD(zrnum, subcmd).arg2].vnum % 100,
                ZCMD(zrnum, subcmd).arg3, ZCMD(zrnum, subcmd).arg4);
      else
        fprintf(zone_file, "X %d QQ%02d QQ%02d %d %d \t(RoL mobile removal)\n",
                ZCMD(zrnum, subcmd).if_flag, world[ZCMD(zrnum, subcmd).arg1].number % 100,
                mob_index[ZCMD(zrnum, subcmd).arg2].vnum % 100, ZCMD(zrnum, subcmd).arg3,
                ZCMD(zrnum, subcmd).arg4);
      break;
    case 'C':
      fprintf(zone_file, "C %d %d %d %d %d \t(RoL calendar predicate)\n",
              ZCMD(zrnum, subcmd).if_flag, ZCMD(zrnum, subcmd).arg1, ZCMD(zrnum, subcmd).arg2,
              ZCMD(zrnum, subcmd).arg3, ZCMD(zrnum, subcmd).arg4);
      break;
    case 'T':
      fprintf(zone_file, "T %d %d QQ%02d QQ%02d \t(%s)\n", ZCMD(zrnum, subcmd).if_flag,
              ZCMD(zrnum, subcmd).arg1, trig_index[ZCMD(zrnum, subcmd).arg2]->vnum % 100,
              world[ZCMD(zrnum, subcmd).arg3].number % 100,
              GET_TRIG_NAME(trig_index[ZCMD(zrnum, subcmd).arg2]->proto));
      break;
    case 'V':
      fprintf(zone_file, "V %d %d %d QQ%02d %s %s\n", ZCMD(zrnum, subcmd).if_flag,
              ZCMD(zrnum, subcmd).arg1, ZCMD(zrnum, subcmd).arg2,
              world[ZCMD(zrnum, subcmd).arg3].number % 100, ZCMD(zrnum, subcmd).sarg1,
              ZCMD(zrnum, subcmd).sarg2);
      break;
    case '*':
      /* Invalid commands are replaced with '*' - Ignore them. */
      continue;
    default:
      mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: export_save_zone(): Unknown cmd '%c' - NOT saving",
             ZCMD(zrnum, subcmd).command);
      continue;
    }
  }
  fputs("S\n$\n", zone_file);
  fclose(zone_file);

  return TRUE;
}

static int export_save_objects(zone_rnum zrnum)
{
  char buf[MAX_STRING_LENGTH] = {'\0'};
  char ebuf1[MAX_STRING_LENGTH] = {'\0'}, ebuf2[MAX_STRING_LENGTH] = {'\0'},
       ebuf3[MAX_STRING_LENGTH] = {'\0'}, ebuf4[MAX_STRING_LENGTH] = {'\0'};
  char wbuf1[MAX_STRING_LENGTH] = {'\0'}, wbuf2[MAX_STRING_LENGTH] = {'\0'},
       wbuf3[MAX_STRING_LENGTH] = {'\0'}, wbuf4[MAX_STRING_LENGTH] = {'\0'};
  char pbuf1[MAX_STRING_LENGTH] = {'\0'}, pbuf2[MAX_STRING_LENGTH] = {'\0'},
       pbuf3[MAX_STRING_LENGTH] = {'\0'}, pbuf4[MAX_STRING_LENGTH] = {'\0'};
  obj_rnum ornum;
  obj_vnum ovnum;
  int i;
  FILE *obj_file;
  struct obj_data *obj;
  struct extra_descr_data *ex_desc;

  if (!(obj_file = fopen_restricted("world/export/qq.obj", "w")))
  {
    mudlog(BRF, LVL_STAFF, TRUE, "SYSERR: export_save_objects : Cannot open file!");
    return FALSE;
  }
  /* Start running through all objects in this zone. */
  for (ovnum = genolc_zone_bottom(zrnum); ovnum <= zone_table[zrnum].top; ovnum++)
  {
    if ((ornum = real_object(ovnum)) != NOTHING)
    {
      if ((obj = &obj_proto[ornum])->action_description)
      {
        strncpy(buf, obj->action_description, sizeof(buf) - 1);
        strip_cr(buf);
      }
      else
        *buf = '\0';

      fprintf(obj_file,
              "#QQ%02d\n"
              "%s~\n"
              "%s~\n"
              "%s~\n"
              "%s~\n",

              GET_OBJ_VNUM(obj) % 100, (obj->name && *obj->name) ? obj->name : "undefined",
              (obj->short_description && *obj->short_description) ? obj->short_description
                                                                  : "undefined",
              (obj->description && *obj->description) ? obj->description : "undefined", buf);

      sprintascii(ebuf1, GET_OBJ_EXTRA(obj)[0]);
      sprintascii(ebuf2, GET_OBJ_EXTRA(obj)[1]);
      sprintascii(ebuf3, GET_OBJ_EXTRA(obj)[2]);
      sprintascii(ebuf4, GET_OBJ_EXTRA(obj)[3]);
      sprintascii(wbuf1, GET_OBJ_WEAR(obj)[0]);
      sprintascii(wbuf2, GET_OBJ_WEAR(obj)[1]);
      sprintascii(wbuf3, GET_OBJ_WEAR(obj)[2]);
      sprintascii(wbuf4, GET_OBJ_WEAR(obj)[3]);
      sprintascii(pbuf1, GET_OBJ_AFFECT(obj)[0]);
      sprintascii(pbuf2, GET_OBJ_AFFECT(obj)[1]);
      sprintascii(pbuf3, GET_OBJ_AFFECT(obj)[2]);
      sprintascii(pbuf4, GET_OBJ_AFFECT(obj)[3]);

      fprintf(obj_file, "%d %s %s %s %s %s %s %s %s %s %s %s %s\n", GET_OBJ_TYPE(obj), ebuf1, ebuf2,
              ebuf3, ebuf4, wbuf1, wbuf2, wbuf3, wbuf4, pbuf1, pbuf2, pbuf3, pbuf4);

      if (GET_OBJ_TYPE(obj) != ITEM_CONTAINER && GET_OBJ_TYPE(obj) != ITEM_AMMO_POUCH)
        fprintf(obj_file, "%d %d %d %d\n", GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1),
                GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 3));
      else
        fprintf(obj_file, "%d %d %s%02d %d\n", GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1),
                GET_OBJ_VAL(obj, 2) == -1 ? "" : "QQ", /* key */
                GET_OBJ_VAL(obj, 2) == -1 ? -1 : GET_OBJ_VAL(obj, 2) % 100, GET_OBJ_VAL(obj, 3));

      fprintf(obj_file, "%d %d %d %d\n", GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), GET_OBJ_RENT(obj),
              GET_OBJ_LEVEL(obj));

      /* dunno what the point of this is unless they have our file format - zusuk*/
      fprintf(obj_file, "%d %d\n", GET_OBJ_SIZE(obj), GET_OBJ_MATERIAL(obj));

      /* Do we have script(s) attached? */
      export_script_save_to_disk(obj_file, obj, OBJ_TRIGGER);

      /* Do we have extra descriptions? */
      if (obj->ex_description)
      { /* Yes, save them too. */
        for (ex_desc = obj->ex_description; ex_desc; ex_desc = ex_desc->next)
        {
          /* Sanity check to prevent nasty protection faults. */
          if (!ex_desc->keyword || !ex_desc->description || !*ex_desc->keyword ||
              !*ex_desc->description)
          {
            mudlog(BRF, LVL_IMMORT, TRUE, "SYSERR: OLC: export_save_objects: Corrupt ex_desc!");
            continue;
          }
          strncpy(buf, ex_desc->description, sizeof(buf) - 1);
          strip_cr(buf);
          fprintf(obj_file,
                  "E\n"
                  "%s~\n"
                  "%s~\n",
                  ex_desc->keyword, buf);
        }
      }
      /* Do we have affects? */
      for (i = 0; i < MAX_OBJ_AFFECT; i++)
        if (obj->affected[i].modifier)
          fprintf(obj_file,
                  "A\n"
                  "%d %d\n",
                  obj->affected[i].location, obj->affected[i].modifier);
    }
  }

  /* Write the final line, close the file. */
  fprintf(obj_file, "$~\n");
  fclose(obj_file);

  return TRUE;
}

static int export_save_rooms(zone_rnum zrnum)
{
  room_vnum i;
  struct room_data *room;
  FILE *room_file;
  char buf[MAX_STRING_LENGTH] = {'\0'};
  char buf1[MAX_STRING_LENGTH] = {'\0'};

  if (!(room_file = fopen_restricted("world/export/qq.wld", "w")))
  {
    mudlog(BRF, LVL_STAFF, TRUE, "SYSERR: export_save_rooms : Cannot open file!");
    return FALSE;
  }

  for (i = genolc_zone_bottom(zrnum); i <= zone_table[zrnum].top; i++)
  {
    room_rnum rnum;

    if ((rnum = real_room(i)) != NOWHERE)
    {
      int j;

      room = &world[rnum];

      /* Copy the description and strip off trailing newlines. */
      strncpy(buf, room->description ? room->description : "Empty room.", sizeof(buf) - 1);
      strip_cr(buf);

      /* Save the numeric and string section of the file. */
      fprintf(room_file,
              "#QQ%02d\n"
              "%s%c\n"
              "%s%c\n"
              "QQ %d %d %d %d %d\n",
              room->number % 100, room->name ? room->name : "Untitled", STRING_TERMINATOR, buf,
              STRING_TERMINATOR, room->room_flags[0], room->room_flags[1], room->room_flags[2],
              room->room_flags[3], room->sector_type);

      /* Now you write out the exits for the room. */
      for (j = 0; j < DIR_COUNT; j++)
      {
        if (R_EXIT(room, j))
        {
          int dflag;
          if (R_EXIT(room, j)->general_description)
          {
            strncpy(buf, R_EXIT(room, j)->general_description, sizeof(buf) - 1);
            strip_cr(buf);
          }
          else
            *buf = '\0';

          /* Figure out door flag. */
          if (IS_SET(R_EXIT(room, j)->exit_info, EX_ISDOOR))
          {
            if (IS_SET(R_EXIT(room, j)->exit_info, EX_PICKPROOF))
              dflag = 2;
            else
              dflag = 1;
            if (IS_SET(R_EXIT(room, j)->exit_info, EX_HIDDEN))
              dflag += 2;
            if (IS_SET(R_EXIT(room, j)->exit_info, EX_BLOCKED))
              dflag += 4;
          }
          else
            dflag = 0;

          if (R_EXIT(room, j)->keyword)
            strncpy(buf1, R_EXIT(room, j)->keyword, sizeof(buf1) - 1);
          else
            *buf1 = '\0';

          /* Now write the exit to the file. */
          if (R_EXIT(room, j)->to_room == NOWHERE ||
              R_EXIT(room, j)->to_room > top_of_world ||
              world[R_EXIT(room, j)->to_room].zone == zrnum)
            fprintf(room_file,
                    "D%d\n"
                    "%s~\n"
                    "%s~\n"
                    "%d %s%02d %s%02d\n",
                    j, buf, buf1, dflag, R_EXIT(room, j)->key == NOTHING ? "" : "QQ",
                    R_EXIT(room, j)->key == NOTHING ? -1 : (int)(R_EXIT(room, j)->key % 100),
                    R_EXIT(room, j)->to_room == NOWHERE ||
                            R_EXIT(room, j)->to_room > top_of_world
                        ? ""
                        : "QQ",
                    R_EXIT(room, j)->to_room != NOWHERE &&
                            R_EXIT(room, j)->to_room <= top_of_world
                        ? (int)(world[R_EXIT(room, j)->to_room].number % 100)
                        : -1);
          else
          {
            fprintf(room_file,
                    "D%d\n"
                    "%s~\n"
                    "%s~\n"
                    "%d %s%02d ZZ%02d\n",
                    j, buf, buf1, dflag, R_EXIT(room, j)->key == NOTHING ? "" : "QQ",
                    R_EXIT(room, j)->key == NOTHING ? -1 : (int)(R_EXIT(room, j)->key % 100),
                    (int)(world[R_EXIT(room, j)->to_room].number % 100));
            zone_exits++;
          }
        }
      }

      if (room->ex_description)
      {
        struct extra_descr_data *xdesc;

        for (xdesc = room->ex_description; xdesc; xdesc = xdesc->next)
        {
          strncpy(buf, xdesc->description, sizeof(buf) - 1);
          buf[sizeof(buf) - 1] = '\0';
          strip_cr(buf);
          fprintf(room_file,
                  "E\n"
                  "%s~\n"
                  "%s~\n",
                  xdesc->keyword, buf);
        }
      }
      fprintf(room_file, "S\n");
      export_script_save_to_disk(room_file, room, WLD_TRIGGER);
    }
  }

  /* Write the final line and close it. */
  fprintf(room_file, "$~\n");
  fclose(room_file);

  return TRUE;
}

static void export_script_save_to_disk(FILE *fp, void *item, int type)
{
  struct trig_proto_list *t;

  if (type == MOB_TRIGGER)
    t = ((struct char_data *)item)->proto_script;
  else if (type == OBJ_TRIGGER)
    t = ((struct obj_data *)item)->proto_script;
  else if (type == WLD_TRIGGER)
    t = ((struct room_data *)item)->proto_script;
  else
  {
    log("SYSERR: Invalid type passed to export_script_save_to_disk()");
    return;
  }

  while (t)
  {
    fprintf(fp, "T QQ%02d\n", t->vnum % 100);
    t = t->next;
  }
}

/* save the zone's triggers to internal memory and to disk */
static int export_save_triggers(zone_rnum zrnum)
{
  trig_vnum i;
  trig_data *trig;
  struct cmdlist_element *cmd;
  FILE *trig_file;
  char bitBuf[MAX_INPUT_LENGTH] = {'\0'};

  if (!(trig_file = fopen_restricted("world/export/qq.trg", "w")))
  {
    mudlog(BRF, LVL_STAFF, TRUE, "SYSERR: export_save_triggers : Cannot open file!");
    return FALSE;
  }

  for (i = genolc_zone_bottom(zrnum); i <= zone_table[zrnum].top; i++)
  {
    trig_rnum rnum;

    if ((rnum = real_trigger(i)) != NOTHING)
    {
      trig = trig_index[rnum]->proto;

      fprintf(trig_file, "#QQ%02d\n", i % 100);

      sprintascii(bitBuf, GET_TRIG_TYPE(trig));
      fprintf(trig_file,
              "%s%c\n"
              "%d %s %d\n"
              "%s%c\n",
              (GET_TRIG_NAME(trig)) ? (GET_TRIG_NAME(trig)) : "unknown trigger", STRING_TERMINATOR,
              trig->attach_type, *bitBuf ? bitBuf : "0", GET_TRIG_NARG(trig),
              GET_TRIG_ARG(trig) ? GET_TRIG_ARG(trig) : "", STRING_TERMINATOR);

      fprintf(trig_file,
              "* This trigger has been exported 'as is'. This means that vnums\n"
              "* in this file are not changed, and will have to be edited by hand.\n"
              "* This zone was number %d on The Builder Academy, so you\n"
              "* should be looking for %dxx, where xx is 00-99.\n",
              zone_table[zrnum].number, zone_table[zrnum].number);
      for (cmd = trig->cmdlist; cmd; cmd = cmd->next)
      {
        fprintf(trig_file, "%s\n", cmd->cmd);
      }
      fprintf(trig_file, "%c\n", STRING_TERMINATOR);
    }
  }

  fprintf(trig_file, "$%c\n", STRING_TERMINATOR);
  fclose(trig_file);
  return TRUE;
}
