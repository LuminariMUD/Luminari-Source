/* LuminariMUD Procedurally generated wilderness. */

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

#include <gd.h>


struct wild_map_info_type {
  int sector_type;
  char disp[20];
};

static struct wild_map_info_type wild_map_info[] ={
  { SECT_INSIDE, "\tn.\tn"}, /* 0 */
  { SECT_CITY, "\twC\tn"},
  { SECT_FIELD, "\tg,\tn"},
  { SECT_FOREST, "\tG\t=Y\tn"},
  { SECT_HILLS, "\tyn\tn"},
  { SECT_MOUNTAIN, "\tw^\tn"}, /* 5 */
  { SECT_WATER_SWIM, "\tB\t=~\tn"},
  { SECT_WATER_NOSWIM, "\tb\t==\tn"},
  { SECT_FLYING, "\tC^\tn"},
  { SECT_UNDERWATER, "\tbU\tn"},
  { SECT_ZONE_START, "\tRX\tn"}, /* 10 */
  { SECT_ROAD_NS, "\tD|\tn"}, /* 11 */
  { SECT_ROAD_EW, "\tD-\tn"}, /* 12 */
  { SECT_ROAD_INT, "\tD+\tn"}, /* 13 */
  { SECT_DESERT, "\tY.\tn"}, /* 14 */
  { SECT_OCEAN, "\tb\t=~\tn"}, /* 15 */
  { SECT_MARSHLAND, "\tM,\tn"}, /* 16 */
  { SECT_HIGH_MOUNTAIN, "\tW^\tn"}, /* 17 */
  { SECT_PLANES, "\tM.\tn"}, /* 18 */
  { SECT_UD_WILD, "\tM\t=Y\tn"},
  { SECT_UD_CITY, "\tmC\tn"},  // 20
  { SECT_UD_INSIDE, "\tm.\tn"},
  { SECT_UD_WATER, "\tm\t=~\tn\tn"},
  { SECT_UD_NOSWIM, "\tM\t==\tn\tn"},
  { SECT_UD_NOGROUND, "\tm^\tn"},
  { SECT_LAVA, "\tR.\tc]\tn"},  //25  
  { SECT_D_ROAD_NS, "\ty|\tn"},
  { SECT_D_ROAD_EW, "\ty-\tn"},
  { SECT_D_ROAD_INT, "\ty+\tn"},
  { SECT_CAVE, "\tD\t=C\tn"},
  { SECT_JUNGLE, "\tg&\tn"},
  { SECT_TUNDRA, "\tW.\tn"},
  { SECT_TAIGA, "\tgA\tn"},
  { SECT_BEACH, "\ty:\tn"},

  { -1, ""},  /* RESERVED, NUM_ROOM_SECTORS */
};


double get_radial_gradient(int x, int y) {
  int cx, cy;
  int xsize = WILD_X_SIZE;
  int ysize = WILD_Y_SIZE;
  int xdist, ydist;
  double dist, distrec;

  /* Trying a different way, a rectangular radial gradient :) */
  /* Distance to the closest border... */
  xdist = (x < (xsize - x) ? x : (xsize - x));
  ydist = (y < (ysize - y) ? y : (ysize - y));

  dist = MIN(xdist, ydist);

  /* So we want a gradient from 300 to 0. */
  /* Invert dist */
  dist = 128 - dist;

  if (dist < 0)
    dist = 0;

  /* Add some noise */
  dist *= ((PerlinNoise1D(NOISE_MATERIAL_PLANE_ELEV, dist/128.0, 2, 2, 1) + 1)/2);

  distrec = (double)(dist/128.0);
      
  cx = xsize/2;
  cy = ysize/2;

  dist = sqrt((x - cx)*(x - cx) + (y - cy)*(y - cy));  

  if (dist > 512.0) 
    dist = 512.0;  


  dist = dist - 300.0;

  if (dist < 0)
    dist = 0; 

  /* Add some noise */
  dist *= ((PerlinNoise1D(NOISE_MATERIAL_PLANE_ELEV, dist/256.0, 2, 2, 1) + .7)/1.4);

  dist = (double)(dist/300.0);

  return (dist*9+distrec)/10.0;  
//  return (dist + distrec)/2.0;
//
//    return distrec;

//  return (dist + distrec)/2;

}


int get_elevation(int map, int x, int y) {
  double trans_x;
  double trans_y;
  double orig_result;
  double result;
  double dist;

  trans_x = x/(double)(WILD_X_SIZE/4.0);
  trans_y = y/(double)(WILD_Y_SIZE/4.0);

//  result = PerlinNoise2D(map, trans_x, trans_y, 1.5, 2, 16);
//  result = ( result + 1.0 ) / 2.0;

  result = PerlinNoise2D(map, trans_x, trans_y, 1.5, 2.0, 16);
  
  /* elip the data a little, makes better mountains. */
  result = (result > .8 ? result = .8 : result);
  result = (result < -.8 ? result = -.8 : result);

  /* Normalize between -1 and 1 */
  result = (result)/.8;
  

  /* Used ridged perlin. */
  result = 1 -  (result < 0 ? -result : result);

  /* Attenuate. */
  result *= result;

  /* get the distortion. HACKS! */
  dist = PerlinNoise2D(NOISE_MATERIAL_PLANE_ELEV_DIST, trans_x, trans_y, 2.0, 2.0, 16);

  /* Clip the data a little, makes better mountains. */
  dist = (dist > .8 ? dist = .8 : dist);
  dist = (dist < -.8 ? dist = -.8 : dist);

  /* Normalize between -1 and 1 */
  dist = (dist)/.8;

  /* Take a weighted average. */
  result = result + dist;
  result = (result + 1)/3.0;
 
  /* Apply the radial gradient. */
  result -= 1.5*(get_radial_gradient(x, y))*result;

  return 255*result;
}

int get_moisture(int map, int x, int y) {
  double trans_x;
  double trans_y;
  double result;

  trans_x = x/(double)(WILD_X_SIZE/4.0);
  trans_y = y/(double)(WILD_Y_SIZE/4.0);

  result = PerlinNoise2D(map, trans_x, trans_y, 1.5, 2, 4);
/*  if (result < -.7)
    result = -.7;
  else if (result > .7)
    result = .7;

  result = (result + .7)/1.4;
*/

  result = ( result + 1 ) / 2.0;
  return 255*result;

}

int get_temperature(int map, int x, int y) {
   /* This is basically a gradient in the y direction, modified
    * by terrain height. */

   int max_temp =  35;
   int min_temp = -20;
   int dist = 0;
   int equator = 256; /* Y coord of the equator */
   
   double pct; /* percentage value, used to calulate gradient. */

   /* Find the absolute value of the distance of y to the equator. */
   dist = abs(y - equator);
   
   /* calculate the gradiant. */
   pct = (double)(dist/(double)(WILD_Y_SIZE - equator));    

   /* Return the temp. */
   return (max_temp - (max_temp - min_temp)*pct) - (MAX(get_elevation(map, x, y) - WATERLINE, 0))/10;
}

/* Generate a height map centered on center_x and center_y. */
void get_map(int xsize, int ysize, int center_x, int center_y, int **map) {
  int x, y;
  int x_offset, y_offset;
  double trans_x, trans_y;

  x_offset = (center_x - ((xsize-1)/2));
  y_offset = (center_y - ((ysize-1)/2));

  /* map MUST be big enough! */
  for(y = 0; y < ysize; y++) {
    for(x = 0; x < xsize; x++) {
      map[x][y] = get_sector_type(get_elevation(NOISE_MATERIAL_PLANE_ELEV,    x + x_offset, y + y_offset),
                                  get_temperature(NOISE_MATERIAL_PLANE_ELEV,  x + x_offset, y + y_offset),
                                  get_moisture(NOISE_MATERIAL_PLANE_MOISTURE, x + x_offset, y + y_offset));
    }
  }
}


/* Helper function to choose terrain type, for maps. Elevation is 0-255. */
const char* terrain_by_elevation(int elevation) {
  int waterline = WATERLINE;

  if (elevation < waterline)
  {
    if (elevation > waterline - SHALLOW_WATER_THRESHOLD)
    {
        return TERRAIN_TILE_TYPE_SHALLOW_WATER;
    }
    else
    {
      return TERRAIN_TILE_TYPE_DEEP_WATER;
    }
  }
  else
  {
    if (elevation < waterline + COASTLINE_THRESHOLD)
    {
      return TERRAIN_TILE_TYPE_COASTLINE;
    }
    else if (elevation < waterline + PLAINS_THRESHOLD)
    {
      return TERRAIN_TILE_TYPE_PLAINS;
    }
    else if (elevation > 255 - MOUNTAIN_THRESHOLD)
    {
      return TERRAIN_TILE_TYPE_MOUNTAIN;
    }
    else if (elevation > 255 - HILL_THRESHOLD)
    {
      return TERRAIN_TILE_TYPE_HILL;
    }
    else
    {
      return TERRAIN_TILE_TYPE_FOREST;
    }
  }
}

int sector_type_by_elevation(int elevation) {
  int waterline = WATERLINE;

  // Modified this to use moisture map and a temperature gradient.

  if (elevation < waterline)
  {
    if (elevation > waterline - SHALLOW_WATER_THRESHOLD)
    {
        return SECT_WATER_SWIM; //TERRAIN_TILE_TYPE_SHALLOW_WATER;
    }
    else
    {
      return SECT_OCEAN; //TERRAIN_TILE_TYPE_DEEP_WATER;
    }
  }
  else
  {
    if (elevation < waterline + COASTLINE_THRESHOLD)
    {
      return SECT_BEACH; //TERRAIN_TILE_TYPE_COASTLINE;
    }
    else if (elevation < waterline + PLAINS_THRESHOLD)
    {
      return SECT_FIELD; //TERRAIN_TILE_TYPE_PLAINS;
    }
    else if (elevation > 255 - MOUNTAIN_THRESHOLD)
    {
      return SECT_MOUNTAIN; //TERRAIN_TILE_TYPE_MOUNTAIN;
    }
    else if (elevation > 255 - HILL_THRESHOLD)
    {
      return SECT_HILLS; //TERRAIN_TILE_TYPE_HILL;
    }
    else
    {
      return SECT_FOREST; //TERRAIN_TILE_TYPE_FOREST;
    }
  }
}

/* Get the sector type based on the three variables - 
 *   elevation - given by the heightmap
 *   temperature - given by the temperature gradient
 *   moisture - given by the moisture map
 */
int get_sector_type(int elevation, int temperature, int moisture) {
  int waterline = WATERLINE;

  // Modified this to use moisture map and a temperature gradient.
 
  /* Water */
  if (elevation < waterline)
  {
    if (elevation > waterline - SHALLOW_WATER_THRESHOLD)
    {
        return SECT_WATER_SWIM; //TERRAIN_TILE_TYPE_SHALLOW_WATER;
    }
    else
    {
      return SECT_OCEAN; //TERRAIN_TILE_TYPE_DEEP_WATER;
    }
  }
  else
  {
    
    if (elevation < waterline + COASTLINE_THRESHOLD)
    {
      if (( moisture > 180 ) && (temperature > 8))
        return SECT_MARSHLAND;
      else
        return SECT_BEACH; //TERRAIN_TILE_TYPE_COASTLINE;

    } 
    else if (elevation < waterline + PLAINS_THRESHOLD)
    {
      if (( moisture > 180 ) && (temperature > 8))
        return SECT_MARSHLAND;
      else if ( temperature < 8)
        return SECT_TUNDRA;
      else if (( temperature > 15 ) && (moisture < 100))
        return SECT_DESERT;
      else
        return SECT_FIELD; //TERRAIN_TILE_TYPE_PLAINS;
    }
    else if (elevation > 255 - HIGH_MOUNTAIN_THRESHOLD)
    {
      return SECT_HIGH_MOUNTAIN;
    }
    else if (elevation > 255 - MOUNTAIN_THRESHOLD)
    {      
     return SECT_MOUNTAIN; //TERRAIN_TILE_TYPE_MOUNTAIN;
    }
    else if (elevation > 255 - HILL_THRESHOLD)
    {
      if((temperature < 10) && (moisture > 128)) 
        return SECT_TAIGA;
      else
        return SECT_HILLS; //TERRAIN_TILE_TYPE_HILL;
    }
    else
    { 
      if (temperature < 10)
        return SECT_TAIGA;
      else if ((temperature > 18) && (moisture > 128))
        return SECT_JUNGLE;
      else
        return SECT_FOREST; //TERRAIN_TILE_TYPE_FOREST;
    }
  }

}

room_rnum find_static_room_by_coordinates(int x, int y) {

  int i = 0;

  /* Check the list of wilderness rooms for the coordinates. Two loops because there are gaps between. */
  for(i = WILD_ROOM_VNUM_START; (i< WILD_DYNAMIC_ROOM_VNUM_START) && (real_room(i) != NOWHERE); i++) {
    if((world[real_room(i)].coords[X_COORD] == x) && (world[real_room(i)].coords[Y_COORD] == y)) {
      /* Match! */
      return real_room(i);
    }
  }

  return NOWHERE;
}

/* Function to retreive a room based on coordinates.  The coordinates are 
 * stored in a wrapper, struct wild_room_data, that contains a coordinate 
 * vector and a pointer to struct room_data (the actual room!). */
room_rnum find_room_by_coordinates(int x, int y) {

  int i = 0;
  room_rnum room = NOWHERE;

  if((room = find_static_room_by_coordinates(x, y)) != NOWHERE)
    return room;
  
  /* Check the dynamic rooms. */
  for(i = WILD_DYNAMIC_ROOM_VNUM_START; (i <= WILD_DYNAMIC_ROOM_VNUM_END) && (real_room(i) != NOWHERE); i++) {
    if((ROOM_FLAGGED(real_room(i), ROOM_OCCUPIED)) && 
       (world[real_room(i)].coords[X_COORD] == x) && 
       (world[real_room(i)].coords[Y_COORD] == y)) {
      /* Match */
      return real_room(i);
    }
  } 
  
  /* No rooms assigned to (x,y) */
  return NOWHERE; 
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

  if (room == NOWHERE) {/* This is not a room! */
    log("SYSERR: Attempted to assign NOWHERE as a new wilderness location at (%d, %d)", x, y);
    return;
  }

  /* Here we will set the coordinates, build the descriptions, set the exits, sector type, etc. */
  world[room].coords[0]=x;
  world[room].coords[1]=y;

  /* The following will be dynamically generated. */
//  world[room].name = "The Wilderness";
//  world[room].description = "The wilderness.\r\n";
  world[room].sector_type = get_sector_type(get_elevation(NOISE_MATERIAL_PLANE_ELEV, x, y),
                            get_temperature(NOISE_MATERIAL_PLANE_ELEV, x, y),
                            get_moisture(NOISE_MATERIAL_PLANE_MOISTURE, x, y));
}

/* Print a map with size 'size', centered on (x,y) */
void show_wilderness_map(struct char_data* ch, int size, int x, int y) {
  int **map;
  int i, j;
  room_rnum room = NOWHERE; 

 
  int xsize = size;
  int ysize = size;
  int centerx = ((xsize-1)/2);
  int centery = ((ysize-1)/2);

  int *data = malloc(sizeof(double) * xsize * ysize);

  map = malloc(sizeof(int*) * xsize);
  for (i = 0; i < xsize; i++) {
        map[i] = data + (i*ysize);
  }

  get_map(xsize, ysize, x, y, map);
 
  for(j = ysize - 1; j >= 0 ; j--) {
    for(i = 0; i < xsize; i++) {
      if(sqrt((centerx - i)*(centerx - i) + (centery - j)*(centery - j)) <= (((size-1)/2)+1 )) {
        if((i == centerx) && (j == centery)) {
          send_to_char(ch, "\tM*\t");
        } else if ((room = find_static_room_by_coordinates(x + (i-centerx), y + (j-centery))) != NOWHERE ) { 
          /* There is a prebuilt room at these coords! */
          send_to_char(ch, "%s", wild_map_info[world[room].sector_type].disp);
        } else {
          send_to_char(ch, "%s", wild_map_info[map[i][j]].disp);
        }
      } else {
        send_to_char(ch, " ");
      }
    }  
    send_to_char(ch, "\r\n");
  }  

  send_to_char(ch, " Current Location  : (\tC%d\tn, \tC%d\tn)\r\n" 
                   " Current Elevation : %.3d   "
                   " Current Moisture  : %d\r\n"
                   " Current Temp.     : %d\r\n"
                   " Current Sector    : %s\r\n",  
                   x, y, 
                   get_elevation(NOISE_MATERIAL_PLANE_ELEV, x, y),
                   get_moisture(NOISE_MATERIAL_PLANE_MOISTURE, x, y),
                   get_temperature(NOISE_MATERIAL_PLANE_ELEV, x, y),
                   sector_types[world[IN_ROOM(ch)].sector_type]);


  if (map[0]) free(map[0]);
  free(map);

}

EVENTFUNC(event_check_occupied) {
  struct mud_event_data *pMudEvent = NULL;
  struct room_data *room = NULL;
  room_rnum rnum = NOWHERE;

  pMudEvent = (struct mud_event_data *) event_obj;

  if (!pMudEvent)
    return 0;

  if (!pMudEvent->iId)
    return 0;

   room = (struct room_data *) pMudEvent->pStruct;
   rnum = real_room(room->number);

  if(rnum == NOWHERE)
    return 0;

  /* Check to see if the room is occupied.  Check the following: 
   * - Characters (Players/Mobiles)
   * - Objects
   * - Room Effects    
   */
  if((room->room_affections == 0) &&
     (room->contents == NULL) &&
     (room->people  == NULL)) {
    REMOVE_BIT_AR(ROOM_FLAGS(rnum), ROOM_OCCUPIED);
    return 0; /* No need to continue checking! */
  } else {
    return 10 RL_SEC; /* Keep checking every 10 seconds. */
  }

  /* If we got here something went terribly wrong. */
  return 0;
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

  for(y = 0; y < ysize; y++) {
    for(x = 0; x < xsize; x++) {
 
      sector_color = get_sector_type(get_elevation(NOISE_MATERIAL_PLANE_ELEV, x, ysize - y),
                                  get_temperature(NOISE_MATERIAL_PLANE_ELEV,  x, ysize - y),
                                  get_moisture(NOISE_MATERIAL_PLANE_MOISTURE, x, ysize - y));
      if (sector_color == SECT_HIGH_MOUNTAIN) 
        gdImageSetPixel(im, x, y, gray[get_elevation(NOISE_MATERIAL_PLANE_ELEV, x,ysize-y)]);
      else
        gdImageSetPixel(im, x, y, color_by_sector[sector_color]);

    }
  }

  out = fopen(fn, "wb");
  gdImagePng(im, out);
  fclose(out);
  gdImageDestroy(im);
}

void save_noise_to_file(int idx, const char* fn, int xsize, int ysize) {

  gdImagePtr im; //declaration of the image
  FILE *out;     //output file
  int white, black, gray[255];
  int i, x, y;
  double pixel;
  double dist;
  double trans_x, trans_y;
 
  im = gdImageCreate(xsize,ysize); //create an image   
 
  white = gdImageColorAllocate(im, 255, 255, 255);
  black = gdImageColorAllocate(im, 0, 0, 0);

  

  for (i=0; i<=255; i++){
    gray[i] = gdImageColorAllocate(im, i,i,i);
  }
 
  for(y = 0; y <= ysize; y++) {
    for(x = 0; x <= xsize; x++) {

      trans_x = x/((double)(xsize/4.0));
      trans_y = y/((double)(ysize/4.0));


      pixel = PerlinNoise2D(idx, trans_x, trans_y, 2.0, 2.0, 16);

 
      pixel =1.0 -  (pixel < 0 ? -pixel : pixel);
      pixel *= pixel;   

      dist = PerlinNoise2D(idx+1, trans_x, trans_y, 2.0, 2.0, 16);

      pixel = pixel + 2.0*dist;

      pixel = (pixel + 1.6)/4.0; 
      
      gdImageSetPixel(im, x, y, gray[(int)(255*pixel)]);

    }
  }

  out = fopen(fn, "wb");
  gdImagePng(im, out);
  fclose(out);
  gdImageDestroy(im);
  
}
