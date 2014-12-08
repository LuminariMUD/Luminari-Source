/* LuminariMUD Procedurally generated wilderness. */

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

struct kdtree* kd_wilderness_rooms = NULL;

int wild_waterline = 128;

struct wild_map_info_type {
  int sector_type;
  char disp[20];
  
};

/* \t= changes a color to be BACKGROUND. */
static struct wild_map_info_type wild_map_info[] ={
  /* 0 */
  { SECT_INSIDE, 	"\tn.\tn"}, 
  { SECT_CITY, 		"\twC\tn"},
  { SECT_FIELD, 	"\tg,\tn"},
  { SECT_FOREST, 	"\tGY\tn"},
  { SECT_HILLS, 	"\tyn\tn"},
  /* 5 */
  { SECT_MOUNTAIN, 	"\tw^\tn"}, 
  { SECT_WATER_SWIM, 	"\tB~\tn"},
  { SECT_WATER_NOSWIM, 	"\tb=\tn"},
  { SECT_FLYING, 	"\tC^\tn"},
  { SECT_UNDERWATER, 	"\tbU\tn"},
  /* 10 */
  { SECT_ZONE_START, 	"\tRX\tn"}, 
  { SECT_ROAD_NS, 	"\tD|\tn"},
  { SECT_ROAD_EW, 	"\tD-\tn"},
  { SECT_ROAD_INT, 	"\tD+\tn"},
  { SECT_DESERT, 	"\tY.\tn"},
  /* 15 */
  { SECT_OCEAN, 	"\tb~\tn"},
  { SECT_MARSHLAND, 	"\tM,\tn"},
  { SECT_HIGH_MOUNTAIN, "\tW^\tn"}, 
  { SECT_PLANES, 	"\tM.\tn"}, 
  { SECT_UD_WILD, 	"\tMY\tn"},
  /* 20 */
  { SECT_UD_CITY, 	"\tmC\tn"}, 
  { SECT_UD_INSIDE, 	"\tm.\tn"},
  { SECT_UD_WATER, 	"\tm~\tn"},
  { SECT_UD_NOSWIM, 	"\tM=\tn"},
  { SECT_UD_NOGROUND, 	"\tm^\tn"},
  /* 25 */
  { SECT_LAVA, 		"\tR.\tn"},  
  { SECT_D_ROAD_NS, 	"\ty|\tn"},
  { SECT_D_ROAD_EW, 	"\ty-\tn"},
  { SECT_D_ROAD_INT, 	"\ty+\tn"},
  { SECT_CAVE, 		"\tDC\tn"},
  /* 30 */
  { SECT_JUNGLE, 	"\tg&\tn"},
  { SECT_TUNDRA, 	"\tW.\tn"},
  { SECT_TAIGA, 	"\tgA\tn"},
  { SECT_BEACH, 	"\ty:\tn"},

  { -1, ""},  /* RESERVED, NUM_ROOM_SECTORS */
};

/* Initialize the kd-tree that indexes the static rooms of the wilderness.
 * This procedure can be used to do whatever initialization is needed, 
 * but be aweare that it is run whenever a room is added or deleted from 
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
  for(i = WILD_ROOM_VNUM_START + 1; i < WILD_DYNAMIC_ROOM_VNUM_START; i++) {   
    if(real_room(i) != NOWHERE) {
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
  int bounding_boxes[1][4] = { {-1024, -1024, 1024, 1024}};

  for (box = 0; box < 1; box++) {
    if( ((x > bounding_boxes[box][0]) && (x <= bounding_boxes[box][2])) &&
        ((y > bounding_boxes[box][1]) && (y <= bounding_boxes[box][3]))) {
      /* We are within this bounding box. */
      xsize = (bounding_boxes[box][2] - bounding_boxes[box][0]);
      ysize = (bounding_boxes[box][3] - bounding_boxes[box][1]);


      /* Get distance to the edges of the bounding box. */
      xdist = MIN(x - bounding_boxes[box][0], bounding_boxes[box][2] - x);
      ydist = MIN(y - bounding_boxes[box][1], bounding_boxes[box][3] - y);

      dist = MIN(xdist, ydist);

      /* Invert dist */
      dist = xsize/8 - dist;

      if (dist < 0)
        dist = 0;

      distrec = (double)(dist/(xsize/8.0)); 
     
      cx = (bounding_boxes[box][0] + bounding_boxes[box][2])/2;
      cy = (bounding_boxes[box][1] + bounding_boxes[box][3])/2;
  
      dist = sqrt((x - cx)*(x - cx) + (y - cy)*(y - cy));  

      dist = dist -(xsize/2.0 - (xsize/16.0));
      if (dist < 0)
        dist = 0; 

      /* Add some noise */
      dist *= ((PerlinNoise1D(NOISE_MATERIAL_PLANE_ELEV, dist/256.0, 2, 2, 1)+ 1)/2.0);
  
      dist = (double)(dist/(xsize/16.0));
      return 1.0 - ((dist+2*distrec)/3.0);  
    }
  }
  return 0;
}


int get_elevation(int map, int x, int y) {
  double trans_x;
  double trans_y;
  double result;
  double dist;

  trans_x = x/(double)(WILD_X_SIZE/2.0);
  trans_y = y/(double)(WILD_Y_SIZE/2.0);


  result = PerlinNoise2D(map, trans_x, trans_y, 2.0, 2.0, 16);
  
  /* Compress the data a little, makes better mountains. */
  result = (result > .8 ? .8 : result);
  result = (result < -.8 ? -.8 : result);

  /* Normalize between -1 and 1 */
  result = (result)/.8;
  

  /* Used ridged perlin. */
  result = 1 -  (result < 0 ? -result : result);

  /* Attenuate. */
  result *= result;
  result *= result;

  trans_x = x/(double)(WILD_X_SIZE/8.0);
  trans_y = y/(double)(WILD_Y_SIZE/8.0);


  /* get the distortion */
  dist = PerlinNoise2D(NOISE_MATERIAL_PLANE_ELEV_DIST, trans_x, trans_y, 1.5, 2.0, 16);

  /* Take a weighted average, normalize over [0..1] */
  result = ((result +  dist) + 1)/3.0;

  /* Apply the radial gradient. */
  result *= get_radial_gradient(x, y);

  return 255*result;
}

int get_moisture(int map, int x, int y) {
  double trans_x;
  double trans_y;
  double result;

  trans_x = x/(double)(WILD_X_SIZE/8.0);
  trans_y = y/(double)(WILD_Y_SIZE/8.0);

  result = PerlinNoise2D(map, trans_x, trans_y, 1.5, 2.0, 8);

  /* Normalize over 0..1 */
  result = ( result + 1 ) / 2.0;

  return 255*result;

}

int get_temperature(int map, int x, int y) {
  /* This is a gradient in the y direction, modified
   * by terrain height. */

  int max_temp =  35;
  int min_temp = -30;
  int dist = 0;
  int equator = 0; /* Y coord of the equator */
  int temp;

  double pct; /* percentage value, used to calulate gradient. */

  /* Find the absolute value of the distance of y to the equator. */
  dist = abs(y - equator);
   
  /* calculate the gradiant. */
  pct = (double)(dist/(double)(WILD_Y_SIZE - equator));    

  /* Return the temp. */
  temp = (max_temp - (max_temp - min_temp)*pct) - (MAX(1.5*get_elevation(map, x, y) - WATERLINE, 0))/10;
   
  return temp;
}

/* Generate a height map centered on center_x and center_y. */
void get_map(int xsize, int ysize, int center_x, int center_y, struct wild_map_tile **map) {

  int x, y;
  int x_offset, y_offset;
  int trans_x, trans_y;

  /* Below is for looking up static rooms. */
  room_rnum* room; 
  double loc[2], pos[2];
  void* set;

  x_offset = (center_x - ((xsize-1)/2));
  y_offset = (center_y - ((ysize-1)/2));

  /* map MUST be big enough! */
  for(y = 0; y < ysize; y++) {
    for(x = 0; x < xsize; x++) {
      map[x][y].vis = 0;
      map[x][y].sector_type = get_sector_type(get_elevation(NOISE_MATERIAL_PLANE_ELEV,    x + x_offset, y + y_offset),
                              get_temperature(NOISE_MATERIAL_PLANE_ELEV,  x + x_offset, y + y_offset),
                              get_moisture(NOISE_MATERIAL_PLANE_MOISTURE, x + x_offset, y + y_offset));
    }
  }

  /* use the kd_wilderness_rooms kd-tree index to look up the nearby rooms */
  loc[0] = center_x;
  loc[1] = center_y;
  set = kd_nearest_range(kd_wilderness_rooms, loc, ((xsize-1)/2) + 1);

  while( !kd_res_end( set ) ) {
    room = (room_rnum *)kd_res_item( set, pos);
    
    /* Sanity check. */
    trans_x = MAX(0, MIN((int)pos[0] - x_offset, xsize)); 
    trans_y = MAX(0, MIN((int)pos[1] - y_offset, ysize));

    if((trans_x < xsize) && (trans_y < ysize))
      map[trans_x][trans_y].sector_type = world[*room].sector_type;

    /* go to the next entry */
    kd_res_next( set );
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
  if (elevation < waterline)
  {
    if (elevation > waterline - SHALLOW_WATER_THRESHOLD)
    {
        return SECT_WATER_SWIM; 
    }
    else
    {
      return SECT_OCEAN; 
    }
  }
  else
  {

    /* Do we have marshes along the water or beach? */    
    if (elevation < waterline + COASTLINE_THRESHOLD)
    {
      if (( moisture > 180 ) && (temperature > 8))
        return SECT_MARSHLAND;
      else
        return SECT_BEACH;

    } 
    else if (elevation < waterline + PLAINS_THRESHOLD)
    {
      if (( moisture > 180 ) && (temperature > 8))
        return SECT_MARSHLAND;
      else if ( temperature < 8)
        return SECT_TUNDRA;
      else if (( temperature > 25 ) && (moisture < 80))
        return SECT_DESERT;
      else
        return SECT_FIELD; 
    }
    else if (elevation > 255 - HIGH_MOUNTAIN_THRESHOLD)
    {
      return SECT_HIGH_MOUNTAIN;
    }
    else if (elevation > 255 - MOUNTAIN_THRESHOLD)
    {      
     return SECT_MOUNTAIN; 
    }
    else if (elevation > 255 - HILL_THRESHOLD)
    {
      if((temperature < 10) && (moisture > 128)) 
        return SECT_TAIGA;
      else
        return SECT_HILLS; 
    }
    else
    { 
      if (temperature < 10)
        return SECT_TAIGA;
      else if ((temperature > 18) && (moisture > 180))
        return SECT_JUNGLE;
      else
        return SECT_FOREST; 
    }
  }

}

room_rnum find_static_room_by_coordinates(int x, int y) {
  double loc[2], pos[2];
  void* set; 
  room_rnum* room;


  /* use the kd_wilderness_rooms kd-tree index to look up the room at (x, y) */
  loc[0] = (double)x;
  loc[1] = (double)y;

  set = kd_nearest_range(kd_wilderness_rooms, loc, 0.5);
  while( !kd_res_end( set ) ) {
    room = (room_rnum *)kd_res_item( set, pos);
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

  if((room = find_static_room_by_coordinates(x, y)) != NOWHERE) {
    return room;
  }
  /* Check the dynamic rooms. */
  for(i = WILD_DYNAMIC_ROOM_VNUM_START; (i <= WILD_DYNAMIC_ROOM_VNUM_END) && (real_room(i) != NOWHERE); i++) {
    if((ROOM_FLAGGED(real_room(i), ROOM_OCCUPIED)) && 
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

  for(i = WILD_DYNAMIC_ROOM_VNUM_START; i <= WILD_DYNAMIC_ROOM_VNUM_END; i++) {
    /* Skip gaps. */
    if(real_room(i) == NOWHERE)
      continue;

    if(!ROOM_FLAGGED(real_room(i), ROOM_OCCUPIED)) {
      /* Here is our room. */
      return real_room(i);
    }
  }
  /* If we get here, there is a problem. */
  return NOWHERE;
}

void assign_wilderness_room(room_rnum room, int x, int y) {  

  static char *wilderness_name = "The Wilderness of Luminari";
  static char *wilderness_desc = "The wilderness extends in all directions.";
  struct region_list *regions;

  if (room == NOWHERE) {/* This is not a room! */
    log("SYSERR: Attempted to assign NOWHERE as a new wilderness location at (%d, %d)", x, y);
    return;
  }

  /* Here we will set the coordinates, build the descriptions, set the exits, sector type, etc. */
  world[room].coords[0]=x;
  world[room].coords[1]=y;
 
  /* Get the enclosing regions. */
  regions = get_enclosing_regions(GET_ROOM_ZONE(room), x, y);

  if (world[room].name && world[room].name != wilderness_name)
    free(world[room].name);
  if (world[room].description && world[room].description != wilderness_desc)
    free(world[room].description);

  world[room].description = wilderness_desc;

  if(regions) {
    world[room].name = strdup(region_table[regions->rnum].name);
  } else {
    world[room].name = wilderness_name;
  }

  world[room].sector_type = get_sector_type(get_elevation(NOISE_MATERIAL_PLANE_ELEV, x, y),
                            get_temperature(NOISE_MATERIAL_PLANE_ELEV, x, y),
                            get_moisture(NOISE_MATERIAL_PLANE_MOISTURE, x, y));
}

void line_vis(struct wild_map_tile **map, int x,int y,int x2, int y2) {
  int i = 0;
  int visibility = 10;
  int orig_x = x, orig_y = y;

  int w = x2 - x ;
  int h = y2 - y ;
  int dx1 = 0, dy1 = 0, dx2 = 0, dy2 = 0 ;
  if (w<0) dx1 = -1 ; else if (w>0) dx1 = 1 ;
  if (h<0) dy1 = -1 ; else if (h>0) dy1 = 1 ;
  if (w<0) dx2 = -1 ; else if (w>0) dx2 = 1 ;
  int longest = abs(w) ;
  int shortest = abs(h) ;
  if (!(longest>shortest)) {
      longest = abs(h) ;
      shortest = abs(w) ;
      if (h<0) dy2 = -1 ; else if (h>0) dy2 = 1 ;
      dx2 = 0 ;            
  } 
  int numerator = longest >> 1 ;
  for (i=0;i<=longest;i++) {
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

    if (round(sqrt(((double)x - (double)orig_x) * ((double)x - (double)orig_x) + ((double)y - (double)orig_y) * ((double)y - (double)orig_y))) <= (visibility + 1))
      map[x][y].vis = 1; 
    numerator += shortest ;
    if (!(numerator<longest)) {
      numerator -= longest ;
      x += dx1 ;
      y += dy1 ;
    } else {
      x += dx2 ;
      y += dy2 ;
    }
  }
}


static char* wilderness_map_to_string (struct wild_map_tile ** map, int size) {
  static char strmap[32768];
  char* mp =strmap;
  int x, y;

  int centerx = ((size - 1)/2);
  int centery = ((size - 1)/2);

  for ( y = size - 1; y >= 0; y--) {
    for ( x = 0; x < size; x++) {
     if(sqrt((centerx - x)*(centerx - x) + (centery - y)*(centery - y)) <= (((size-1)/2)+1 )) {
        if((x == centerx) && (y == centery)) {
          strcpy(mp, "\tM*\tn");
          mp += strlen("\tM*\tn"); 
        } else {
          strcpy(mp, (map[x][y].vis == 0 ? " " : wild_map_info[map[x][y].sector_type].disp));
          mp += strlen((map[x][y].vis == 0 ? " " : wild_map_info[map[x][y].sector_type].disp));
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
  int j;

  int xsize = size;
  int ysize = size;
  int centerx = ((xsize-1)/2);
  int centery = ((ysize-1)/2);

  struct wild_map_tile *data = malloc(sizeof(struct wild_map_tile) * xsize * ysize);
 
  map = malloc(sizeof(struct wild_map_tile*) * xsize);
 
  for (i = 0; i < xsize; i++) {
        map[i] = data + (i*ysize);
  }

  get_map(xsize, ysize, x, y, map);

  for(i = 0; i < xsize; i++) {
    line_vis(map, centerx, centery, i, 0);
    line_vis(map, centerx, centery, i, ysize - 1);
  }
  for(i = 0; i < ysize; i ++) {
    line_vis(map, centerx, centery, 0, i);
    line_vis(map, centerx, centery, xsize - 1, i);
  }
  
//  send_to_char(ch, "%s", wilderness_map_to_string(map, size));

send_to_char(ch, "%s", strpaste(wilderness_map_to_string(map, size), strfrmt(world[IN_ROOM(ch)].description, GET_SCREEN_WIDTH(ch) - size,size, FALSE, TRUE, TRUE), " \tn"));

  send_to_char(ch, " Current Location  : (\tC%d\tn, \tC%d\tn)\r\n", 
/*                   " Current Elevation : %.3d   "
                   " Current Moisture  : %d\r\n"
                   " Gradient          : %f   "
                   " Current Temp.     : %d\r\n"
                   " Current Sector    : %s\r\n",  */
                   x, y);
/*                   get_elevation(NOISE_MATERIAL_PLANE_ELEV, x, y),
                   get_moisture(NOISE_MATERIAL_PLANE_MOISTURE, x, y),
                   get_radial_gradient(x, y),
                   get_temperature(NOISE_MATERIAL_PLANE_ELEV, x, y),
                   sector_types[world[IN_ROOM(ch)].sector_type]);
*/

  if (map[0]) free(map[0]);
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

  if(rnum == NOWHERE)
    return 0;
  else 
    room = &world[rnum];

  /* Check to see if the room is occupied.  Check the following: 
   * - Characters (Players/Mobiles)
   * - Objects
   * - Room Effects    
   */
  if((room->room_affections == 0) &&
     (room->contents == NULL) &&
     (room->people  == NULL) &&
     (room->events && room->events->iSize == 1)) {
  
    REMOVE_BIT_AR(ROOM_FLAGS(rnum), ROOM_OCCUPIED);
    return 0; /* No need to continue checking! */
    
  } else {
    return 10 RL_SEC; /* Keep checking every 10 seconds. */
  }

  /* If we got here something went terribly wrong. */
  return 0;
}

char * gen_ascii_wilderness_map(int size, int x, int y) {
  struct wild_map_tile **map;
  int i;

  int xsize = size;
  int ysize = size;
  int centerx = ((xsize-1)/2);
  int centery = ((ysize-1)/2);
  
  char *mapstring = NULL;

  struct wild_map_tile *data = malloc(sizeof(struct wild_map_tile) * xsize * ysize);
 
  map = malloc(sizeof(struct wild_map_tile*) * xsize);
 
  for (i = 0; i < xsize; i++) {
        map[i] = data + (i*ysize);
  }

  get_map(xsize, ysize, x, y, map);

  for(i = 0; i < xsize; i++) 
    for(j = 0; j < ysize; j++)
      map[i][j].vis = 1;
 

  mapstring = wilderness_map_to_string(map, size);

  if (map[0]) free(map[0]);
  free(map);
  
  return mapstring;
}

void save_map_to_file(const char* fn, int xsize, int ysize) {
  gdImagePtr im; //declaration of the image
  FILE *out;     //output file
  int white, black, blue, gray[255];
  int i, x, y;
  int color_by_sector[100]; /* HUGE HACK! */
  int sector_color;

  im = gdImageCreate(xsize,ysize); //create an image   

  white = gdImageColorAllocate(im, 255, 255, 255);
  black = gdImageColorAllocate(im, 0, 0, 0);
  blue  = gdImageColorAllocate(im, 0, 0, 255);
  
  color_by_sector[SECT_WATER_SWIM] = gdImageColorAllocate(im, 0, 0, 255);
  color_by_sector[SECT_OCEAN]      = gdImageColorAllocate(im, 0, 0, 128);
  color_by_sector[SECT_DESERT]     = gdImageColorAllocate(im, 255, 236, 159);
  color_by_sector[SECT_FIELD]      = gdImageColorAllocate(im, 0, 128, 0);
  color_by_sector[SECT_HILLS]      = gdImageColorAllocate(im, 139, 69, 19);
  color_by_sector[SECT_FOREST]     = gdImageColorAllocate(im, 0, 100, 0);
  color_by_sector[SECT_JUNGLE]     = gdImageColorAllocate(im, 85, 107, 47);
  color_by_sector[SECT_BEACH]      = gdImageColorAllocate(im, 215, 208, 19);
  color_by_sector[SECT_TAIGA]      = gdImageColorAllocate(im, 107, 142, 35);
  color_by_sector[SECT_MOUNTAIN]   = gdImageColorAllocate(im, 176, 176, 176);
  color_by_sector[SECT_TUNDRA]     = gdImageColorAllocate(im, 240, 248, 255);
  color_by_sector[SECT_ZONE_START] = gdImageColorAllocate(im, 128, 0, 0);
  color_by_sector[SECT_MARSHLAND]  = gdImageColorAllocate(im, 33, 146, 75);

  for (i=0; i<=255; i++){
    gray[i] = gdImageColorAllocate(im, i,i,i);
  }

  for(y = (-ysize/2); y < ysize/2; y++) {
    for(x = (-xsize/2); x < xsize/2; x++) {
 
      sector_color = get_sector_type(get_elevation(NOISE_MATERIAL_PLANE_ELEV, x, -y),
                                  get_temperature(NOISE_MATERIAL_PLANE_ELEV,  x, -y),
                                  get_moisture(NOISE_MATERIAL_PLANE_MOISTURE, x, -y));
      if (sector_color == SECT_HIGH_MOUNTAIN) 
        gdImageSetPixel(im, x + xsize/2, ysize/2 - y, gray[get_elevation(NOISE_MATERIAL_PLANE_ELEV, x,-y)]);
      else
        gdImageSetPixel(im, x + xsize/2, ysize/2 - y, color_by_sector[sector_color]);

    }
  }

  out = fopen(fn, "wb");
  gdImagePng(im, out);
  fclose(out);
  gdImageDestroy(im);
}

void save_noise_to_file(int idx, const char* fn, int xsize, int ysize, int zoom) {

  gdImagePtr im; //declaration of the image
  FILE *out;     //output file
  int white, black, gray[255];
  int i, x, y;
  double pixel;
//  double dist;
  double trans_x, trans_y;
  
//  int canvas_x = (zoom == 0 ? xsize : xsize/(2*zoom));
//  int canvas_y = (zoom == 0 ? ysize : ysize/(2*zoom));
  int canvas_x = xsize;
  int canvas_y = ysize;

  im = gdImageCreate(canvas_x,canvas_y); //create an image   
 
  white = gdImageColorAllocate(im, 255, 255, 255);
  black = gdImageColorAllocate(im, 0, 0, 0);

  

  for (i=0; i<=255; i++){
    gray[i] = gdImageColorAllocate(im, i,i,i);
  }
 
  for(y = 0; y <= canvas_x; y++) {
    for(x = 0; x <= canvas_y; x++) {

      trans_x = x/(double)((xsize/4.0) * (zoom == 0 ? 1 : 0.5*zoom));
      trans_y = y/(double)((ysize/4.0) * (zoom == 0 ? 1 : 0.5*zoom));


      pixel = PerlinNoise2D(idx, trans_x, trans_y, 2.0, 2.0, 16);

      pixel = (pixel + 1)/2.0; 
//      pixel =1.0 -  (pixel < 0 ? -pixel : pixel);
//      pixel *= pixel;   

//      dist = PerlinNoise2D(idx+1, trans_x, trans_y, 2.0, 2.0, 16);

//      pixel = pixel + 2.0*dist;

//      pixel = (pixel + 1.6)/4.0; 
      
      gdImageSetPixel(im, x, y, gray[(int)(255*pixel)]);

    }
  }

  out = fopen(fn, "wb");
  gdImagePng(im, out);
  fclose(out);
  gdImageDestroy(im);
  
}

