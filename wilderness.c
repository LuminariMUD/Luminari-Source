/* LuminariMUD Procedurally generated wilderness.
   Author:  Ornir
 */

#include <math.h>
#include <gd.h>

#include "perlin.h"
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "constants.h"
#include "mud_event.h"
#include "wilderness.h"
#include "kdtree.h"

#include "mysql.h"
#include "desc_engine.h"

void insert_path(struct path_data *path);

struct kdtree* kd_wilderness_rooms = NULL;

int wild_waterline = 128;

/* \t= changes a color to be BACKGROUND. */
struct wild_map_info_type wild_map_info[] = {
  /* 0 */
  { SECT_INSIDE, "\tn\t[u65294/.]\tn",
    { NULL}},
  { SECT_CITY, "\tw\t[u127984/C]\tn",
    { NULL}},
  { SECT_FIELD, "\tg\t[u65292],\tn",
    { "\t[F120]\t[u65292],\tn", "\t[F121]\t[u65292],\tn", "\t[F130]\t[u65292],\tn", "\t[F131]\t[u65292],\tn"}},
  { SECT_FOREST, "\tG\t[u127795/Y]\tn",
    {"\t[f020]\t[u127795/Y]\tn", "\t[f030]\t[u127795/Y]\tn", "\t[f040]\t[u127795/Y]\tn", "\t[f050]\t[u127795/Y]\tn"}},
  { SECT_HILLS, "\ty\t[u65358]\t[u65358/n]\tn",
    { NULL}},
  /* 5 */
  { SECT_MOUNTAIN, "\tw\t[u127956/^]\tn",
    { NULL}},
  { SECT_WATER_SWIM, "\tB\t[u65374/~]\tn",
    { NULL}},
  { SECT_WATER_NOSWIM, "\tb\t[u65309/=]\tn",
    { NULL}},
  { SECT_FLYING, "\tC\t[u65342/^]\tn",
    { NULL}},
  { SECT_UNDERWATER, "\tb\t[u65333/U]\tn",
    { NULL}},
  /* 10 */
  { SECT_ZONE_START, "\tR\t[u65336/X]\tn",
    { NULL}},
  { SECT_ROAD_NS, "\tD\t[u65372]\t[u65372/|]\tn",
    { NULL}}, /* This is somewhat obsolete. */
  { SECT_ROAD_EW, "\tD\t[u65293]\t[u65293/-]\tn",
    { NULL}}, /* This is somewhat obsolete. */
  { SECT_ROAD_INT, "\tD\t[u65291]\t[u65291/+]\tn",
    { NULL}}, /* This is somewhat obsolete. */
  { SECT_DESERT, "\tY\t[u65294]\t[u65294/.]\tn",
    { NULL }},
  /* 15 */
  { SECT_OCEAN, "\tb\t[u65374/~]\tn",
    { NULL}},
  { SECT_MARSHLAND, "\tM\t[u65292]\t[u65292/,]\tn",
    { NULL}},
  { SECT_HIGH_MOUNTAIN, "\tW\t[u127956/^]\tn",
    { NULL}},
  { SECT_PLANES, "\tM\t[u65294/.]\tn",
    { NULL}},
  { SECT_UD_WILD, "\tM\t[u65337/Y]\tn",
    { NULL}},
  /* 20 */
  { SECT_UD_CITY, "\tm\t[u65315/C]\tn",
    { NULL}},
  { SECT_UD_INSIDE, "\tm\t[u65294/.]\tn",
    { NULL}},
  { SECT_UD_WATER, "\tm\t[u65374/~]\tn",
    { NULL}},
  { SECT_UD_NOSWIM, "\tM\t[u65309/=]\tn",
    { NULL}},
  { SECT_UD_NOGROUND, "\tm\t[u65342/^]\tn",
    { NULL}},
  /* 25 */
  { SECT_LAVA, "\tR\t[u65294]\t[u65294/.]\tn",
    { NULL}},
  { SECT_D_ROAD_NS, "\ty\t[u65372]\t[u65372/|]\tn",
    { NULL}}, /* This is somewhat obsolete. */
  { SECT_D_ROAD_EW, "\ty\t[u65293]\t[u65293/-]\tn",
    { NULL}}, /* This is somewhat obsolete. */
  { SECT_D_ROAD_INT, "\ty\t[u65291]\t[u65291/+]\tn",
    { NULL}}, /* This is somewhat obsolete. */
  { SECT_CAVE, "\tD\t[u65315]\t[u65315/C]\tn",
    { NULL}},
  /* 30 */
  { SECT_JUNGLE, "\tg\t[u127796/&]\tn",
    { NULL}},
  { SECT_TUNDRA, "\tW\t[u65294/.]\tn",
    { NULL}},
  { SECT_TAIGA, "\tg\t[u127794/A]\tn",
    { NULL}},
  { SECT_BEACH, "\ty\t[u65306]\t[u65306/:]\tn",
    { NULL}},

  { -1, "",
    { NULL}}, /* RESERVED, NUM_ROOM_SECTORS */
};

/* Initialize the kd-tree that indexes the static rooms of the wilderness.
 * This procedure can be used to do whatever initialization is needed,
 * but be aware that it is run whenever a room is added or deleted from
 * the wilderness zone. */
void initialize_wilderness_lists() {
  int i;
  double loc[2];
  room_vnum * rm;

  if (kd_wilderness_rooms != NULL)
    kd_free(kd_wilderness_rooms);

  kd_wilderness_rooms = kd_create(2);

  /* The +1 for the initializer is so that the 'magic' room is not
   * included in the index. */
  for (i = WILD_ROOM_VNUM_START + 1; i < WILD_DYNAMIC_ROOM_VNUM_START; i++) {
    if (real_room(i) != NOWHERE) {
      CREATE(rm, room_rnum, 1);
      *rm = real_room(i);
      loc[0] = world[real_room(i)].coords[0];
      loc[1] = world[real_room(i)].coords[1];

      kd_insert(kd_wilderness_rooms, loc, rm);

    }
  }
}

/* Get the value of the radial/box gradient at the specified (x,y) coordinate. */
double get_radial_gradient(int x, int y) {
  int cx, cy;
  int xsize = WILD_X_SIZE;
  int ysize = WILD_Y_SIZE;
  int xdist, ydist;
  double dist = WILD_X_SIZE;
  double distrec = WILD_X_SIZE;


  int box;

  /* Find the gradiant at point (x,y) on the wilderness map.
   * (x,y) can be from (-WILD_X_SIZE, -WILD_Y_SIZE) to
   * (WILD_X_SIZE, WILD_Y_SIZE).  This region is divided up into
   * several bounding boxes that determine where the continents should
   * roughly appear. */

  /* Bounding boxes are stored as [ll_x, lly, ur_x, ur_y] arrays. */
  int bounding_boxes[1][4] = {
    {-1024, -1024, 1024, 1024}
  };

  for (box = 0; box < 1; box++) {
    if (((x > bounding_boxes[box][0]) && (x <= bounding_boxes[box][2])) &&
            ((y > bounding_boxes[box][1]) && (y <= bounding_boxes[box][3]))) {
      /* We are within this bounding box. */
      xsize = (bounding_boxes[box][2] - bounding_boxes[box][0]);
      ysize = (bounding_boxes[box][3] - bounding_boxes[box][1]);


      /* Get distance to the edges of the bounding box. */
      xdist = MIN(x - bounding_boxes[box][0], bounding_boxes[box][2] - x);
      ydist = MIN(y - bounding_boxes[box][1], bounding_boxes[box][3] - y);

      dist = MIN(xdist, ydist);

      /* Invert dist */
      dist = xsize / 8 - dist;

      if (dist < 0)
        dist = 0;

      distrec = (double) (dist / (xsize / 8.0));

      cx = (bounding_boxes[box][0] + bounding_boxes[box][2]) / 2;
      cy = (bounding_boxes[box][1] + bounding_boxes[box][3]) / 2;

      dist = sqrt((x - cx)*(x - cx) + (y - cy)*(y - cy));

      dist = dist - (xsize / 2.0 - (xsize / 16.0));
      if (dist < 0)
        dist = 0;

      /* Add some noise */
      dist *= ((PerlinNoise1D(NOISE_MATERIAL_PLANE_ELEV, dist / 256.0, 2, 2, 1) + 1) / 2.0);

      dist = (double) (dist / (xsize / 16.0));
      return 1.0 - ((dist + 2 * distrec) / 3.0);
    }
  }
  return 0;
}

int get_elevation(int map, int x, int y) {
  double trans_x;
  double trans_y;
  double result;
  double dist;

  trans_x = x / (double) (WILD_X_SIZE / 2.0);
  trans_y = y / (double) (WILD_Y_SIZE / 2.0);


  result = PerlinNoise2D(map, trans_x, trans_y, 2.0, 2.0, 16);

  /* Compress the data a little, makes better mountains. */
  result = (result > .8 ? .8 : result);
  result = (result < -.8 ? -.8 : result);

  /* Normalize between -1 and 1 */
  result = (result) / .8;


  /* Used ridged perlin. */
  result = 1 - (result < 0 ? -result : result);

  /* Attenuate. */
  result *= result;
  result *= result;

  trans_x = x / (double) (WILD_X_SIZE / 8.0);
  trans_y = y / (double) (WILD_Y_SIZE / 8.0);


  /* get the distortion */
  dist = PerlinNoise2D(NOISE_MATERIAL_PLANE_ELEV_DIST, trans_x, trans_y, 1.5, 2.0, 16);

  /* Take a weighted average, normalize over [0..1] */
  result = ((result + dist) + 1) / 3.0;

  /* Apply the radial gradient. */
  result *= get_radial_gradient(x, y);

  return 255 * result;
}

int get_weather(int x, int y) {
  double trans_x;
  double trans_y;
  double result;
  double time_base;
  time_t now;


  now = time(NULL);
  time_base = now % 100000;

  time_base = time_base / (double) (100000.0);
  trans_x = x / (double) (WILD_X_SIZE / 1.0);
  trans_y = y / (double) (WILD_Y_SIZE / 1.0);

  result = PerlinNoise3D(NOISE_WEATHER, trans_x * 50.0, trans_y * 50.0, time_base * 100, 2.0, 2.0, 8);

  result = (result + 1) / 2.0;
  //log("DEBUG: Weather - %f %f %f %f", trans_x, trans_y, time_base, result);

  return 255 * result;
}

int get_moisture(int map, int x, int y) {
  double trans_x;
  double trans_y;
  double result;

  trans_x = x / (double) (WILD_X_SIZE / 8.0);
  trans_y = y / (double) (WILD_Y_SIZE / 8.0);

  result = PerlinNoise2D(map, trans_x, trans_y, 1.5, 2.0, 8);

  /* Normalize over 0..1 */
  result = (result + 1) / 2.0;

  return 255 * result;

}

int get_temperature(int map, int x, int y) {
  /* This is a gradient in the y direction, modified
   * by terrain height. */

  int max_temp = 35;
  int min_temp = -30;
  int dist = 0;
  int equator = 0; /* Y coord of the equator */
  int temp;

  double pct; /* percentage value, used to calulate gradient. */

  /* Find the absolute value of the distance of y to the equator. */
  dist = abs(y - equator);

  /* calculate the gradiant. */
  pct = (double) (dist / (double) (WILD_Y_SIZE - equator));

  /* Return the temp. */
  temp = (max_temp - (max_temp - min_temp) * pct) - (MAX(1.5 * get_elevation(map, x, y) - WATERLINE, 0)) / 10;

  return temp;
}

/* 
 * Generate a height map centered on center_x and center_y. 
 */
void get_map(int xsize, int ysize, int center_x, int center_y, struct wild_map_tile **map) {

  int x, y;
  int x_offset, y_offset;
  int trans_x, trans_y;

  /* Below is for looking up static rooms. */
  room_rnum* room;
  double loc[2], pos[2];
  void* set;

  x_offset = (center_x - ((xsize - 1) / 2));
  y_offset = (center_y - ((ysize - 1) / 2));

  /* map MUST be big enough! */
  for (y = 0; y < ysize; y++) {
    for (x = 0; x < xsize; x++) {
      map[x][y].vis = 0;
      map[x][y].sector_type = get_sector_type(get_elevation(NOISE_MATERIAL_PLANE_ELEV, x + x_offset, y + y_offset),
              get_temperature(NOISE_MATERIAL_PLANE_ELEV, x + x_offset, y + y_offset),
              get_moisture(NOISE_MATERIAL_PLANE_MOISTURE, x + x_offset, y + y_offset));
      map[x][y].glyph = NULL;
      map[x][y].num_regions = 0;
      map[x][y].weather = get_weather(x + x_offset, y + y_offset);

      /* Map should reflect changes from regions */
      struct region_list *regions = NULL;
      struct region_list *curr_region = NULL;
      struct path_list *paths = NULL;
      struct path_list *curr_path = NULL;

      /* Get the enclosing regions. */
      regions = get_enclosing_regions(real_zone(WILD_ZONE_VNUM),
              x + x_offset,
              y + y_offset);
      paths = get_enclosing_paths(real_zone(WILD_ZONE_VNUM),
              x + x_offset,
              y + y_offset);
      //log("-> MAP: Processing location (%d, %d)", x + x_offset, y + y_offset);
      /* Override default values with region-based values. */
      for (curr_region = regions; curr_region != NULL; curr_region = curr_region->next) {

        /* Add this region to the tile's region list */
        if (map[x][y].num_regions < 24) {
          map[x][y].regions[map[x][y].num_regions] = curr_region->rnum;
          map[x][y].num_regions++;
        }

        switch (region_table[curr_region->rnum].region_type) {
          case REGION_SECTOR:
            map[x][y].sector_type = region_table[curr_region->rnum].region_props;
            //log("  -> MAP: Changing (%d, %d) to sector : %d", x + x_offset, y + y_offset, region_table[curr_region->rnum].region_props);
            break;
          case REGION_SECTOR_TRANSFORM:
            break;
          case REGION_GEOGRAPHIC:
          case REGION_ENCOUNTER:
          default:
            break;
        }
      }

      /* Override default values with path-based values. */
      for (curr_path = paths; curr_path != NULL; curr_path = curr_path->next) {
        //if (curr_path->rnum != NOWHERE) /*added by zusuk*/
        switch (path_table[curr_path->rnum].path_type) {
          case PATH_ROAD:
          case PATH_DIRT_ROAD:
          case PATH_RIVER:
            map[x][y].sector_type = path_table[curr_path->rnum].path_props;
            map[x][y].glyph = path_table[curr_path->rnum].glyphs[curr_path->glyph_type];
            break;
          default:
            break;
        }
      }

      /* Check if the sector type has variant glyphs */
      if (wild_map_info[map[x][y].sector_type].variant_disp[0]) {
        if (map[x][y].sector_type == SECT_FIELD ||
                map[x][y].sector_type == SECT_MOUNTAIN ||
                map[x][y].sector_type == SECT_HIGH_MOUNTAIN ||
                map[x][y].sector_type == SECT_WATER_NOSWIM ||
                map[x][y].sector_type == SECT_OCEAN)
          map[x][y].glyph = wild_map_info[map[x][y].sector_type].variant_disp[get_elevation(NOISE_MATERIAL_PLANE_MOISTURE, x + x_offset, y + y_offset) % NUM_VARIANT_GLYPHS];
        else
          map[x][y].glyph = wild_map_info[map[x][y].sector_type].variant_disp[get_moisture(NOISE_MATERIAL_PLANE_MOISTURE, x + x_offset, y + y_offset) % NUM_VARIANT_GLYPHS];
      }
    }
  }

  /* use the kd_wilderness_rooms kd-tree index to look up the nearby rooms */
  loc[0] = center_x;
  loc[1] = center_y;
  set = kd_nearest_range(kd_wilderness_rooms, loc, ((xsize - 1) / 2) + 1);

  while (!kd_res_end(set)) {
    room = (room_rnum *) kd_res_item(set, pos);

    /* Sanity check. */
    trans_x = MAX(0, MIN((int) pos[0] - x_offset, xsize));
    trans_y = MAX(0, MIN((int) pos[1] - y_offset, ysize));

    if ((trans_x < xsize) && (trans_y < ysize)) {
      // log ("Altering room (%f, %f) based on kd-tree index!\r\n", pos[0], pos[1]);
      map[trans_x][trans_y].sector_type = world[*room].sector_type;
      map[trans_x][trans_y].glyph = NULL;
    }
    /* go to the next entry */
    kd_res_next(set);
  }

  kd_res_free(set);
}

/* Get the sector type based on the three variables -
 *   elevation - given by the heightmap
 *   temperature - given by the temperature gradient
 *   moisture - given by the moisture map
 */
int get_sector_type(int elevation, int temperature, int moisture) {

  int waterline = wild_waterline;

  /* Water */
  if (elevation < waterline) {
    if (elevation > waterline - SHALLOW_WATER_THRESHOLD) {
      return SECT_WATER_SWIM;
    } else {
      return SECT_OCEAN;
    }
  } else {

    /* Do we have marshes along the water or beach? */
    if (elevation < waterline + COASTLINE_THRESHOLD) {
      if ((moisture > 180) && (temperature > 8))
        return SECT_MARSHLAND;
      else
        return SECT_BEACH;

    } else if (elevation < waterline + PLAINS_THRESHOLD) {
      if ((moisture > 180) && (temperature > 8))
        return SECT_MARSHLAND;
      else if (temperature < 8)
        return SECT_TUNDRA;
      else if ((temperature > 25) && (moisture < 80))
        return SECT_DESERT;
      else
        return SECT_FIELD;
    } else if (elevation > 255 - HIGH_MOUNTAIN_THRESHOLD) {
      return SECT_HIGH_MOUNTAIN;
    } else if (elevation > 255 - MOUNTAIN_THRESHOLD) {
      return SECT_MOUNTAIN;
    } else if (elevation > 255 - HILL_THRESHOLD) {
      if ((temperature < 10) && (moisture > 128))
        return SECT_TAIGA;
      else
        return SECT_HILLS;
    } else {
      if (temperature < 10)
        return SECT_TAIGA;
      else if ((temperature > 18) && (moisture > 180))
        return SECT_JUNGLE;
      else
        return SECT_FOREST;
    }
  }

}

/* Get the sector type, modified by regions and paths. */
int get_modified_sector_type(zone_rnum zone, int x, int y) {

  struct region_list *regions = NULL;
  struct region_list *curr_region = NULL;
  struct path_list *paths = NULL;
  struct path_list *curr_path = NULL;
  int sector_type;
  int elev, temp, mois;

  /* Get the enclosing regions. */
  regions = get_enclosing_regions(zone, x, y);
  /* Get the enclosing paths. */
  paths = get_enclosing_paths(zone, x, y);

  elev = get_elevation(NOISE_MATERIAL_PLANE_ELEV, x, y);
  temp = get_temperature(NOISE_MATERIAL_PLANE_ELEV, x, y);
  mois = get_moisture(NOISE_MATERIAL_PLANE_ELEV, x, y);

  sector_type = get_sector_type(elev, temp, mois);

  /* Override default values with region-based values. */
  for (curr_region = regions; curr_region != NULL; curr_region = curr_region->next) {
    log("-> Processing REGION_TYPE : %d", region_table[curr_region->rnum].region_type);
    switch (region_table[curr_region->rnum].region_type) {
      case REGION_GEOGRAPHIC:       
        break;
      case REGION_SECTOR:
        sector_type = region_table[curr_region->rnum].region_props;
        log("  -> Changing (%d, %d) to sector : %d", x, y, region_table[curr_region->rnum].region_props);
        break;
      case REGION_SECTOR_TRANSFORM:
        elev += region_table[curr_region->rnum].region_props;
        log("  -> Adjusting elevation at (%d, %d) by : %d", x, y, region_table[curr_region->rnum].region_props);
        sector_type = get_sector_type(elev, temp, mois);
        break;
      case REGION_ENCOUNTER:
        break;
      default:
        break;
    }
  }

  /* Override default values with path-based values. */
  for (curr_path = paths; curr_path != NULL; curr_path = curr_path->next) {
    if (curr_path->rnum != NOWHERE) { /*added by zusuk*/
      log("PATH: %s found!", path_table[curr_path->rnum].name);
      switch (path_table[curr_path->rnum].path_type) {
        case PATH_ROAD:
        case PATH_DIRT_ROAD:
        case PATH_RIVER:
            sector_type = path_table[curr_path->rnum].path_props;
          break;
        default:
          break;
      }
    }
  }
  return sector_type;
}

room_rnum find_static_room_by_coordinates(int x, int y) {
  double loc[2], pos[2];
  void* set;
  room_rnum* room;


  /* use the kd_wilderness_rooms kd-tree index to look up the room at (x, y) */
  loc[0] = (double) x;
  loc[1] = (double) y;

  set = kd_nearest_range(kd_wilderness_rooms, loc, 0.5);
  while (!kd_res_end(set)) {
    room = (room_rnum *) kd_res_item(set, pos);
    return *room;
  }
  return NOWHERE;
}

/* Function to retreive a room based on coordinates.  The coordinates are
 * stored in a wrapper, struct wild_room_data, that contains a coordinate
 * vector and a pointer to struct room_data (the actual room!). */
room_rnum find_room_by_coordinates(int x, int y) {

  int i = 0;
  room_rnum room = NOWHERE;

  if ((room = find_static_room_by_coordinates(x, y)) != NOWHERE) {
    return room;
  }
  /* Check the dynamic rooms. */
  for (i = WILD_DYNAMIC_ROOM_VNUM_START; (i <= WILD_DYNAMIC_ROOM_VNUM_END) && (real_room(i) != NOWHERE); i++) {
    if ((ROOM_FLAGGED(real_room(i), ROOM_OCCUPIED)) &&
            (world[real_room(i)].coords[X_COORD] == x) &&
            (world[real_room(i)].coords[Y_COORD] == y)) {
      /* Match */
      return real_room(i);
    }
  }

  /* No rooms currently allocated for (x,y), so allocate one. */
  //  if((room = find_available_wilderness_room()) == NOWHERE)
  //    return NOWHERE; /* No rooms available. */

  //  assign_wilderness_room(room, x, y);

  return room;
}

room_rnum find_available_wilderness_room() {
  int i = 0;

  for (i = WILD_DYNAMIC_ROOM_VNUM_START; i <= WILD_DYNAMIC_ROOM_VNUM_END; i++) {
    /* Skip gaps. */
    if (real_room(i) == NOWHERE)
      continue;

    if (!ROOM_FLAGGED(real_room(i), ROOM_OCCUPIED)) {
      /* Here is our room. */
      return real_room(i);
    }
  }
  /* If we get here, there is a problem. */
  return NOWHERE;
}

void assign_wilderness_room(room_rnum room, int x, int y) {

  /* Set defaults */
  static char *wilderness_name = "The Wilderness of Luminari";
  static char *wilderness_desc = "The wilderness extends in all directions.";

  struct region_list *regions = NULL;
  struct region_list *curr_region = NULL;
  struct path_list *paths = NULL;
  struct path_list *curr_path = NULL;

  if (room == NOWHERE) {/* This is not a room! */
    log("SYSERR: Attempted to assign NOWHERE as a new wilderness location at (%d, %d)", x, y);
    return;
  }

  /* Here we will set the coordinates, build the descriptions, set the exits, sector type, etc. */
  world[room].coords[0] = x;
  world[room].coords[1] = y;

  /* Get the enclosing regions. */
  regions = get_enclosing_regions(GET_ROOM_ZONE(room), x, y);
  /* Get the enclosing paths. */
  paths = get_enclosing_paths(GET_ROOM_ZONE(room), x, y);

  if (world[room].name && world[room].name != wilderness_name)
    free(world[room].name);
  if (world[room].description && world[room].description != wilderness_desc)
    free(world[room].description);

  /* Assign the default values. */

  world[room].name = wilderness_name;
  world[room].sector_type = get_sector_type(get_elevation(NOISE_MATERIAL_PLANE_ELEV, x, y),
          get_temperature(NOISE_MATERIAL_PLANE_ELEV, x, y),
          get_moisture(NOISE_MATERIAL_PLANE_MOISTURE, x, y));

  /* Override default values with region-based values. */
  for (curr_region = regions; curr_region != NULL; curr_region = curr_region->next) {
    log("-> Processing REGION_TYPE : %d", region_table[curr_region->rnum].region_type);
    switch (region_table[curr_region->rnum].region_type) {
      case REGION_GEOGRAPHIC:
        world[room].name = strdup(region_table[curr_region->rnum].name);
        break;
      case REGION_SECTOR:
        world[room].sector_type = region_table[curr_region->rnum].region_props;
        log("  -> Changing (%d, %d) to sector : %d", x, y, region_table[curr_region->rnum].region_props);
        break;
      case REGION_SECTOR_TRANSFORM:
        break;
      case REGION_ENCOUNTER:
        break;
      default:
        break;
    }
  }
  /* Override default values with path-based values. */
  for (curr_path = paths; curr_path != NULL; curr_path = curr_path->next) {
    if (curr_path->rnum != NOWHERE) { /*added by zusuk*/
      log("PATH: %s found!", path_table[curr_path->rnum].name);
      switch (path_table[curr_path->rnum].path_type) {
        case PATH_ROAD:
        case PATH_DIRT_ROAD:
        case PATH_RIVER:
          world[room].name = strdup(path_table[curr_path->rnum].name);
          world[room].sector_type = path_table[curr_path->rnum].path_props;
          break;
        default:
          break;
      }
    }
  }

  /* Generate the description, now that everything else is set up. */
  world[room].description = wilderness_desc;
}

void line_vis(struct wild_map_tile **map, int x, int y, int x2, int y2) {
  int i = 0;
  int visibility = 10;
  int orig_x = x, orig_y = y;

  int w = x2 - x;
  int h = y2 - y;
  int dx1 = 0, dy1 = 0, dx2 = 0, dy2 = 0;
  if (w < 0) dx1 = -1;
  else if (w > 0) dx1 = 1;
  if (h < 0) dy1 = -1;
  else if (h > 0) dy1 = 1;
  if (w < 0) dx2 = -1;
  else if (w > 0) dx2 = 1;
  int longest = abs(w);
  int shortest = abs(h);
  if (!(longest > shortest)) {
    longest = abs(h);
    shortest = abs(w);
    if (h < 0) dy2 = -1;
    else if (h > 0) dy2 = 1;
    dx2 = 0;
  }
  int numerator = longest >> 1;
  for (i = 0; i <= longest; i++) {
    /* Here is where we check transparency. */
    switch (map[x][y].sector_type) {
      case SECT_MOUNTAIN:
        visibility -= 3;
        break;
      case SECT_HIGH_MOUNTAIN:
        visibility = 1;
        break;
      case SECT_JUNGLE:
        visibility -= 2;
        break;
      case SECT_FOREST:
      case SECT_TAIGA:
        visibility -= 1;
        break;
      case SECT_HILLS:
        visibility -= 3;
        break;
    }

    if (round(sqrt(((double) x - (double) orig_x) * ((double) x - (double) orig_x) + ((double) y - (double) orig_y) * ((double) y - (double) orig_y))) <= (visibility + 1))
      map[x][y].vis = 1;
    numerator += shortest;
    if (!(numerator < longest)) {
      numerator -= longest;
      x += dx1;
      y += dy1;
    } else {
      x += dx2;
      y += dy2;
    }
  }
}

static char* wilderness_map_to_string(struct wild_map_tile ** map, int size, int shape, int map_type) {
  static char strmap[32768];
  char* mp = strmap;
  int x, y, i;
  bool region_colored = FALSE;
  int centerx = ((size - 1) / 2);
  int centery = ((size - 1) / 2);

  int weather_value = 0;


  for (y = size - 1; y >= 0; y--) {
    for (x = 0; x < size; x++) {

      if (((shape == WILD_MAP_SHAPE_CIRCLE) &&
              (sqrt((centerx - x)*(centerx - x) + (centery - y)*(centery - y)) <= (((size - 1) / 2) + 1))) ||
              (shape == WILD_MAP_SHAPE_RECT)) {
        if ((x == centerx) && (y == centery)) {
          strcpy(mp, "\tM\t[u128946/*]\tn");
          mp += strlen("\tMt[u128946/*]\tn");
        } else {
          /* Here we have to check the flag on the player, if they are viewing regions (only for STAFF) then the regions will show up
           * in different colors on the map using background colors. */
          for (i = 0; i < map[x][y].num_regions; i++) {
            if (FALSE && region_table[map[x][y].regions[i]].region_type == 1) {
              /* Geographic region */
              /* Pick a color */
              strcpy(mp, "\033[1;46m");
              mp += strlen("\033[1;46m");
              region_colored = TRUE;
              break;
            }
          }
          if ((map_type == MAP_TYPE_NORMAL) ||
                  (map_type == MAP_TYPE_WEATHER && map[x][y].weather < 178)) {
            strcpy(mp, (map[x][y].vis == 0 ? " " : (map[x][y].glyph == NULL ? wild_map_info[map[x][y].sector_type].disp : map[x][y].glyph)));
            mp += strlen((map[x][y].vis == 0 ? " " : (map[x][y].glyph == NULL ? wild_map_info[map[x][y].sector_type].disp : map[x][y].glyph)));
          }

          /* Check the map_type - if this is a weather map then overlay weather glyphs on the map */
          if (map_type == MAP_TYPE_WEATHER) {
            weather_value = map[x][y].weather;
            if (weather_value >= 225) {
              strcpy(mp, "\tYL\tn");
              mp += strlen("\tYL\tn");
            } else if (weather_value >= 200) {
              strcpy(mp, "\tBR\tn");
              mp += strlen("\tBR\tn");
            } else if (weather_value >= 178) {
              strcpy(mp, "\tbR\tn");
              mp += strlen("\tbR\tn");
            }
          }

          if (region_colored == TRUE) {
            /* Set the background color back */
            strcpy(mp, "\033[1;40m");
            mp += strlen("\033[1;40m");
            region_colored = FALSE;
          }
        }
      } else {
        strcpy(mp, " ");
        mp += 1;
      }
    }
    strcpy(mp, "\r\n");
    mp += 2;
  }

  *mp = '\0';
  return strmap;
}

/* Print a map with size 'size', centered on (x,y) */
void show_wilderness_map(struct char_data* ch, int size, int x, int y) {
  struct wild_map_tile **map;
  int i;
  //int j;
  char* generated_desc = NULL;

  int xsize = size;
  int ysize = size;
  int centerx = ((xsize - 1) / 2);
  int centery = ((ysize - 1) / 2);

  struct wild_map_tile *data = malloc(sizeof (struct wild_map_tile) * xsize * ysize);

  map = malloc(sizeof (struct wild_map_tile*) * xsize);

  for (i = 0; i < xsize; i++) {
    map[i] = data + (i * ysize);
  }

  get_map(xsize, ysize, x, y, map);

  for (i = 0; i < xsize; i++) {
    line_vis(map, centerx, centery, i, 0);
    line_vis(map, centerx, centery, i, ysize - 1);
  }
  for (i = 0; i < ysize; i++) {
    line_vis(map, centerx, centery, 0, i);
    line_vis(map, centerx, centery, xsize - 1, i);
  }

  //  send_to_char(ch, "%s", wilderness_map_to_string(map, size));

  if (!IS_NPC(ch))
    if (IS_SET_AR(ROOM_FLAGS(IN_ROOM(ch)), ROOM_GENDESC) || IS_DYNAMIC(IN_ROOM(ch))) {
      generated_desc = gen_room_description(ch, IN_ROOM(ch));
      if (PRF_FLAGGED(ch, PRF_GUI_MODE)) {
        // Display the map inline, with tags.
        send_to_char(ch,
                "<WILDERNESS_MAP>\n"
                "%s"
                "</WILDERNESS_MAP>\n"
                "\tn%s\tn\n",
                wilderness_map_to_string(map, size, WILD_MAP_SHAPE_CIRCLE, MAP_TYPE_NORMAL),
                generated_desc
                );
      } else {
        send_to_char(ch,
                "%s",
                strpaste(wilderness_map_to_string(map, size, WILD_MAP_SHAPE_CIRCLE, MAP_TYPE_NORMAL),
                strfrmt(generated_desc,
                GET_SCREEN_WIDTH(ch) - size,
                size,
                FALSE,
                TRUE,
                TRUE
                ),
                " \tn")
                );
      }
      free(generated_desc);
    } else {
      if (PRF_FLAGGED(ch, PRF_GUI_MODE)) {
        // Display the map inline, with tags.
        send_to_char(ch,
                "<WILDERNESS_MAP>\n"
                "%s"
                "</WILDERNESS_MAP>\n"
                "\tn%s\tn\n",
                wilderness_map_to_string(map, size, WILD_MAP_SHAPE_CIRCLE, MAP_TYPE_NORMAL),
                world[IN_ROOM(ch)].description
                );
      } else {
        send_to_char(ch,
                "%s",
                strpaste(wilderness_map_to_string(map, size, WILD_MAP_SHAPE_CIRCLE, MAP_TYPE_NORMAL),
                strfrmt(world[IN_ROOM(ch)].description,
                GET_SCREEN_WIDTH(ch) - size,
                size,
                FALSE,
                TRUE,
                TRUE),
                " \tn")
                );
      }
    } else
    send_to_char(ch,
          "%s",
          strpaste(wilderness_map_to_string(map, size, WILD_MAP_SHAPE_CIRCLE, MAP_TYPE_NORMAL),
          strfrmt(world[IN_ROOM(ch)].description,
          80 - size,
          size,
          FALSE,
          TRUE,
          TRUE),
          " \tn")
          );

  send_to_char(ch, " Current Location  : (\tC%d\tn, \tC%d\tn)\r\n"
          " Weather           : %d\r\n",
          /*                   " Current Elevation : %.3d   "
              " Current Moisture  : %d\r\n"
              " Gradient          : %f   "
              " Current Temp.     : %d\r\n"
              " Current Sector    : %s\r\n",  */
          x, y, get_weather(x, y));
  /*                   get_elevation(NOISE_MATERIAL_PLANE_ELEV, x, y),
                     get_moisture(NOISE_MATERIAL_PLANE_MOISTURE, x, y),
                     get_radial_gradient(x, y),
                     get_temperature(NOISE_MATERIAL_PLANE_ELEV, x, y),
                     sector_types[world[IN_ROOM(ch)].sector_type]);
   */

  if (map[0]) {
    free(map[0]);
  }
  free(map);

}

EVENTFUNC(event_check_occupied) {
  struct mud_event_data *pMudEvent = NULL;
  struct room_data *room = NULL;
  room_vnum rvnum = NOWHERE;
  room_rnum rnum = NOWHERE;

  pMudEvent = (struct mud_event_data *) event_obj;

  if (!pMudEvent)
    return 0;

  if (!pMudEvent->iId)
    return 0;

  rvnum = *((room_vnum *) pMudEvent->pStruct);
  rnum = real_room(rvnum);

  if (rnum == NOWHERE)
    return 0;
  else
    room = &world[rnum];

  /* Check to see if the room is occupied.  Check the following:
   * - Characters (Players/Mobiles)
   * - Objects
   * - Room Effects
   */
  if ((room->room_affections == 0) &&
          (room->contents == NULL) &&
          (room->people == NULL) &&
          (room->events && room->events->iSize == 1)) {

    REMOVE_BIT_AR(ROOM_FLAGS(rnum), ROOM_OCCUPIED);
    return 0; /* No need to continue checking! */

  } else {
    return 10 RL_SEC; /* Keep checking every 10 seconds. */
  }

  /* If we got here something went terribly wrong. */
  return 0;
}

char * gen_ascii_wilderness_map(int size, int x, int y, int map_type) {
  struct wild_map_tile **map;
  int i;
  int j;

  int xsize = size;
  int ysize = size;
  //int centerx = ((xsize - 1) / 2);
  //int centery = ((ysize - 1) / 2);

  char *mapstring = NULL;

  struct wild_map_tile *data = malloc(sizeof (struct wild_map_tile) * xsize * ysize);

  map = malloc(sizeof (struct wild_map_tile*) * xsize);

  for (i = 0; i < xsize; i++) {
    map[i] = data + (i * ysize);
  }

  get_map(xsize, ysize, x, y, map);

  for (i = 0; i < xsize; i++)
    for (j = 0; j < ysize; j++)
      map[i][j].vis = 10;


  mapstring = wilderness_map_to_string(map, size, WILD_MAP_SHAPE_RECT, map_type);

  if (map[0]) free(map[0]);
  free(map);

  return mapstring;
}

void save_map_to_file(const char* fn, int xsize, int ysize) {
  gdImagePtr im; //declaration of the image
  FILE *out; //output file
  int white, black, blue, gray[255];
  int i, x, y;
  int color_by_sector[NUM_ROOM_SECTORS];
  int sector_type;
  room_rnum* room;
  double loc[2], pos[2];
  void* set;

  im = gdImageCreate(xsize, ysize); //create an image

  white = gdImageColorAllocate(im, 255, 255, 255);
  black = gdImageColorAllocate(im, 0, 0, 0);
  blue = gdImageColorAllocate(im, 0, 0, 255);

  //#define SECT_PLANES          18  // non-prime (no effect yet)
  //#define SECT_UD_WILD         19  // the outdoors of the underdark
  //#define SECT_UD_CITY	    20  // city in the underdark
  //#define SECT_UD_INSIDE 	    21  // inside in the underdark
  //#define SECT_UD_WATER	    22  // water in the underdark
  //#define SECT_UD_NOSWIM	    23  // water, boat needed, in the underdark
  //#define SECT_UD_NOGROUND     24  // chasm in the underdark (Flying)

  color_by_sector[SECT_LAVA] = gdImageColorAllocate(im, 245, 57, 0);
  color_by_sector[SECT_UNDERWATER] = gdImageColorAllocate(im, 25, 250, 237);
  color_by_sector[SECT_FLYING] = gdImageColorAllocate(im, 168, 234, 247);
  color_by_sector[SECT_INSIDE] = gdImageColorAllocate(im, 161, 161, 161);
  color_by_sector[SECT_CITY] = gdImageColorAllocate(im, 0, 0, 0);
  color_by_sector[SECT_CAVE] = gdImageColorAllocate(im, 77, 77, 77);
  color_by_sector[SECT_ROAD_NS] = gdImageColorAllocate(im, 97, 87, 82);
  color_by_sector[SECT_ROAD_EW] = gdImageColorAllocate(im, 97, 87, 82);
  color_by_sector[SECT_ROAD_INT] = gdImageColorAllocate(im, 97, 87, 82);
  color_by_sector[SECT_D_ROAD_NS] = gdImageColorAllocate(im, 107, 83, 48);
  color_by_sector[SECT_D_ROAD_EW] = gdImageColorAllocate(im, 107, 83, 48);
  color_by_sector[SECT_D_ROAD_INT] = gdImageColorAllocate(im, 107, 83, 48);
  color_by_sector[SECT_WATER_SWIM] = gdImageColorAllocate(im, 0, 0, 255);
  color_by_sector[SECT_OCEAN] = gdImageColorAllocate(im, 0, 0, 128);
  color_by_sector[SECT_WATER_NOSWIM] = gdImageColorAllocate(im, 0, 0, 128);
  color_by_sector[SECT_DESERT] = gdImageColorAllocate(im, 255, 236, 159);
  color_by_sector[SECT_FIELD] = gdImageColorAllocate(im, 0, 128, 0);
  color_by_sector[SECT_HILLS] = gdImageColorAllocate(im, 139, 69, 19);
  color_by_sector[SECT_FOREST] = gdImageColorAllocate(im, 0, 100, 0);
  color_by_sector[SECT_JUNGLE] = gdImageColorAllocate(im, 85, 107, 47);
  color_by_sector[SECT_BEACH] = gdImageColorAllocate(im, 215, 208, 19);
  color_by_sector[SECT_TAIGA] = gdImageColorAllocate(im, 107, 142, 35);
  color_by_sector[SECT_MOUNTAIN] = gdImageColorAllocate(im, 176, 176, 176);
  color_by_sector[SECT_TUNDRA] = gdImageColorAllocate(im, 240, 248, 255);
  color_by_sector[SECT_ZONE_START] = gdImageColorAllocate(im, 128, 0, 0);
  color_by_sector[SECT_MARSHLAND] = gdImageColorAllocate(im, 33, 146, 75);

  for (i = 0; i < 255; i++) {
    gray[i] = gdImageColorAllocate(im, i, i, i);
  }

  for (y = (-ysize / 2); y < ysize / 2; y++) {
    for (x = (-xsize / 2); x < xsize / 2; x++) {
      /* We need to check for prebuilt rooms at these coordinates, as well
       * as regions that might change the sector type.  */
      /* Start with the default - The value returned for the generated wilderness. */
      sector_type = get_sector_type(get_elevation(NOISE_MATERIAL_PLANE_ELEV, x, -y),
              get_temperature(NOISE_MATERIAL_PLANE_ELEV, x, -y),
              get_moisture(NOISE_MATERIAL_PLANE_MOISTURE, x, -y));


      /* Map should reflect changes from regions */
      struct region_list *regions = NULL;
      struct region_list *curr_region = NULL;
      struct path_list *paths = NULL;
      struct path_list *curr_path = NULL;

      /* Get the enclosing regions. */
      regions = get_enclosing_regions(real_zone(WILD_ZONE_VNUM),
              x,
              -y);
      /* Get the enclosing paths. */
      paths = get_enclosing_paths(real_zone(WILD_ZONE_VNUM),
              x,
              -y);

      /* Override default values with region-based values. */
      for (curr_region = regions; curr_region != NULL; curr_region = curr_region->next) {
        switch (region_table[curr_region->rnum].region_type) {
          case REGION_SECTOR:
            sector_type = region_table[curr_region->rnum].region_props;

            break;
          case REGION_SECTOR_TRANSFORM:
            break;
          case REGION_GEOGRAPHIC:
          case REGION_ENCOUNTER:
          default:
            break;
        }
      }

      /* Override default values with path-based values. */
      for (curr_path = paths; curr_path != NULL; curr_path = curr_path->next) {
        switch (path_table[curr_path->rnum].path_type) {
          case PATH_ROAD:
          case PATH_RIVER:
            sector_type = path_table[curr_path->rnum].path_props;
            break;
          default:
            break;
        }
      }

      /* use the kd_wilderness_rooms kd-tree index to look up the nearby rooms */
      loc[0] = x;
      loc[1] = -y;
      set = kd_nearest_range(kd_wilderness_rooms, loc, 1); /* size is 1, we check each coord. */

      while (!kd_res_end(set)) {
        room = (room_rnum *) kd_res_item(set, pos);
        sector_type = world[*room].sector_type;

        /* go to the next entry */
        kd_res_next(set);
      }

      kd_res_free(set);


      /* Use greytones for impassable mountains. */
      if (sector_type == SECT_HIGH_MOUNTAIN)
        gdImageSetPixel(im, x + xsize / 2, ysize / 2 + y, gray[get_elevation(NOISE_MATERIAL_PLANE_ELEV, x, -y)]);
      else
        gdImageSetPixel(im, x + xsize / 2, ysize / 2 + y, color_by_sector[sector_type]);

    }
  }

  out = fopen(fn, "wb");
  gdImagePng(im, out);
  fclose(out);
  gdImageDestroy(im);
}

void save_noise_to_file(int idx, const char* fn, int xsize, int ysize, int zoom) {

  gdImagePtr im; //declaration of the image
  FILE *out; //output file
  int white, black, gray[255];
  int i, x, y;
  double pixel;
  //  double dist;
  double trans_x, trans_y;

  //  int canvas_x = (zoom == 0 ? xsize : xsize/(2*zoom));
  //  int canvas_y = (zoom == 0 ? ysize : ysize/(2*zoom));
  int canvas_x = xsize;
  int canvas_y = ysize;

  im = gdImageCreate(canvas_x, canvas_y); //create an image

  white = gdImageColorAllocate(im, 255, 255, 255);
  black = gdImageColorAllocate(im, 0, 0, 0);



  for (i = 0; i < 255; i++) {
    gray[i] = gdImageColorAllocate(im, i, i, i);
  }

  for (y = 0; y <= canvas_x; y++) {
    for (x = 0; x <= canvas_y; x++) {

      trans_x = x / (double) ((xsize / 4.0) * (zoom == 0 ? 1 : 0.5 * zoom));
      trans_y = y / (double) ((ysize / 4.0) * (zoom == 0 ? 1 : 0.5 * zoom));


      pixel = PerlinNoise2D(idx, trans_x, trans_y, 2.0, 2.0, 16);

      pixel = (pixel + 1) / 2.0;
      //      pixel =1.0 -  (pixel < 0 ? -pixel : pixel);
      //      pixel *= pixel;

      //      dist = PerlinNoise2D(idx+1, trans_x, trans_y, 2.0, 2.0, 16);

      //      pixel = pixel + 2.0*dist;

      //      pixel = (pixel + 1.6)/4.0;

      gdImageSetPixel(im, x, y, gray[(int) (255 * pixel)]);

    }
  }

  out = fopen(fn, "wb");
  gdImagePng(im, out);
  fclose(out);
  gdImageDestroy(im);

}

void generate_river(struct char_data* ch, int dir, region_vnum vnum, char *name) {
  /* Start at your current wilderness location, then create a river that meanders in direction 'dir' */
  int x, y, vtx, i;
  /* int elevation; */
  struct vertex vertices[1024];
  int num_vertices = 0;
  int sector_type;
  room_rnum* room;
  double loc[2], pos[2];
  void* set;
  int move_dir = -1;
  int new_move_dir = -1;

  /* Path structure. */
  struct path_data river;

  if (IN_ROOM(ch) != NOWHERE && !IS_WILDERNESS_VNUM(world[IN_ROOM(ch)].number)) {
    send_to_char(ch, "This command is only valid in the wilderness.");
    return;
  }

  /* Now we are sure we are in the wilderness.  Get our coords. */
  x = world[IN_ROOM(ch)].coords[X_COORD];
  y = world[IN_ROOM(ch)].coords[Y_COORD];

  num_vertices = 0;
  vtx = 0;

  sector_type = world[IN_ROOM(ch)].sector_type;

  while ((sector_type != SECT_WATER_SWIM) &&
          (sector_type != SECT_WATER_NOSWIM) &&
          (sector_type != SECT_OCEAN)) {

    /* dummy check */
    if (num_vertices == 1024) {
      break;
    }

    //CREATE(vertices, struct vertex, 1);

    vertices[vtx].x = x;
    vertices[vtx].y = y;

    num_vertices++;
    vtx++;

    /* Find the next room, using gradient descent: */
    int new_x = x;
    int new_y = y;
    int elev = get_elevation(NOISE_MATERIAL_PLANE_ELEV, x, y);
    int n_elev = get_elevation(NOISE_MATERIAL_PLANE_ELEV, x, y + 1);
    int e_elev = get_elevation(NOISE_MATERIAL_PLANE_ELEV, x + 1, y);
    int s_elev = get_elevation(NOISE_MATERIAL_PLANE_ELEV, x, y - 1);
    int w_elev = get_elevation(NOISE_MATERIAL_PLANE_ELEV, x - 1, y);

    if (move_dir != SOUTH && n_elev < elev) {
      new_move_dir = NORTH;
      elev = n_elev;
      new_x = x;
      new_y = y + 1;
    }
    if (move_dir != WEST && e_elev < elev) {
      new_move_dir = EAST;
      elev = e_elev;
      new_x = x + 1;
      new_y = y;
    }
    if (move_dir != NORTH && s_elev < elev) {
      new_move_dir = SOUTH;
      elev = s_elev;
      new_x = x;
      new_y = y - 1;
    }
    if (move_dir != EAST && w_elev < elev) {
      new_move_dir = WEST;
      elev = w_elev;
      new_x = x - 1;
      new_y = y;
    }

    /* Check if we have visited this location before */
    for (i = 0; i < num_vertices; i++) {
      if ((x == new_x && y == new_y) || (vertices[i].x == new_x && vertices[i].y == new_y)) {
        if (new_move_dir != NORTH) {
          new_move_dir = NORTH;
          new_x = x;
          new_y = y + 1;
        }
        if (new_move_dir != EAST) {
          new_move_dir = EAST;
          new_x = x + 1;
          new_y = y;
        }
        if (new_move_dir != SOUTH) {
          new_move_dir = SOUTH;
          new_x = x;
          new_y = y - 1;
        }
        if (new_move_dir != WEST) {
          new_move_dir = WEST;
          new_x = x - 1;
          new_y = y;
        }
      }
    }

    move_dir = new_move_dir;
    x = new_x;
    y = new_y;

    /* We need to check for prebuilt rooms at these coordinates, as well
     * as regions that might change the sector type.  */
    /* Start with the default - The value returned for the generated wilderness. */
    sector_type = get_sector_type(get_elevation(NOISE_MATERIAL_PLANE_ELEV, x, y),
            get_temperature(NOISE_MATERIAL_PLANE_ELEV, x, y),
            get_moisture(NOISE_MATERIAL_PLANE_MOISTURE, x, y));


    /* Should reflect changes from regions */
    struct region_list *regions = NULL;
    struct region_list *curr_region = NULL;
    struct path_list *paths = NULL;
    struct path_list *curr_path = NULL;

    /* Get the enclosing regions. */
    regions = get_enclosing_regions(real_zone(WILD_ZONE_VNUM),
            x,
            y);
    /* Get the enclosing paths. */
    paths = get_enclosing_paths(real_zone(WILD_ZONE_VNUM),
            x,
            y);

    /* Override default values with region-based values. */
    for (curr_region = regions; curr_region != NULL; curr_region = curr_region->next) {
      switch (region_table[curr_region->rnum].region_type) {
        case REGION_SECTOR:
          sector_type = region_table[curr_region->rnum].region_props;

          break;
        case REGION_SECTOR_TRANSFORM:
          break;
        case REGION_GEOGRAPHIC:
        case REGION_ENCOUNTER:
        default:
          break;
      }
    }

    /* Override default values with path-based values. */
    for (curr_path = paths; curr_path != NULL; curr_path = curr_path->next) {
      switch (path_table[curr_path->rnum].path_type) {
        case PATH_ROAD:
        case PATH_RIVER:
          sector_type = path_table[curr_path->rnum].path_props;
          break;
        default:
          break;
      }
    }

    /* use the kd_wilderness_rooms kd-tree index to look up the nearby rooms */
    loc[0] = x;
    loc[1] = -y;
    set = kd_nearest_range(kd_wilderness_rooms, loc, 1); /* size is 1, we check each coord. */

    while (!kd_res_end(set)) {
      room = (room_rnum *) kd_res_item(set, pos);
      sector_type = world[*room].sector_type;

      /* go to the next entry */
      kd_res_next(set);
    }
    kd_res_free(set);
  }

  /* Create the structure for the path */
  river.vnum = vnum;
  river.zone = world[IN_ROOM(ch)].zone;
  river.name = STRALLOC(name);
  river.path_type = 5; /* Corresponds to river glyphs in the db */
  river.path_props = PATH_STREAM;
  river.num_vertices = num_vertices;

  CREATE(river.vertices, struct vertex, num_vertices);

  for (vtx = 0; vtx < num_vertices; vtx++) {
    river.vertices[vtx].x = vertices[vtx].x;
    river.vertices[vtx].y = vertices[vtx].y;
  }

  insert_path(&river);

  /* Done with this. */
  free(river.name);
  free(river.vertices);

  /* Now display the vertices to the player */
  /*sprintf(buf, "River Path: ");
  for (vtx = 0; vtx < num_vertices; vtx++) {
    char buf2[100];
    sprintf(buf2, "%d %d,", vertices[vtx].x, vertices[vtx].y);
    strcat(buf, buf2);
  }
  send_to_char(ch, buf);  
   */
}
