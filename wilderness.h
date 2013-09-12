
#ifndef _WILDERNESS_H_
#define _WILDERNESS_H_

#define X_COORD 0
#define Y_COORD 1

#define WILD_X_SIZE 1024
#define WILD_Y_SIZE 1024

#define WILD_ROOM_VNUM_START         1000000 /* The start of the STATIC wilderness rooms. */
#define WILD_ROOM_VNUM_END           1003999 /* The end of the STATIC wilderness rooms. */

#define WILD_DYNAMIC_ROOM_VNUM_START 1004000 /* The start of the vnums for the dynamic room pool. */
#define WILD_DYNAMIC_ROOM_VNUM_END   1005999 /* The end of the vnums for the dynamic room pool. */

#define WATERLINE               128
#define SHALLOW_WATER_THRESHOLD  20 
#define COASTLINE_THRESHOLD      10
#define PLAINS_THRESHOLD         35
#define HIGH_MOUNTAIN_THRESHOLD  40
#define MOUNTAIN_THRESHOLD       55
#define HILL_THRESHOLD           65

#define TERRAIN_TILE_TYPE_SHALLOW_WATER  "\tB~\tn"
#define TERRAIN_TILE_TYPE_DEEP_WATER     "\tb~\tn"
#define TERRAIN_TILE_TYPE_COASTLINE      "\tY.\tn"
#define TERRAIN_TILE_TYPE_PLAINS         "\tG.\tn"
#define TERRAIN_TILE_TYPE_MOUNTAIN       "\tw^\tn"
#define TERRAIN_TILE_TYPE_HILL           "\tyn\tn"
#define TERRAIN_TILE_TYPE_FOREST         "\tgY\tn"

/* Wilderness noise definitions */
#define NOISE_MATERIAL_PLANE_ELEV       0
#define NOISE_MATERIAL_PLANE_ELEV_DIST  1
#define NOISE_MATERIAL_PLANE_MOISTURE   2

#define NUM_NOISE                       3 /* Always < MAX_GENERATED_NOISE (24) in perlin.h */

#define NOISE_MATERIAL_PLANE_ELEV_SEED         27023//113//93932//->27023//823852//24242//300 //242423 //Yang //3743
#define NOISE_MATERIAL_PLANE_MOISTURE_SEED     3//6737
#define NOISE_MATERIAL_PLANE_ELEV_DIST_SEED    1


extern struct kdtree* kd_wilderness_rooms;

void get_map(int xsize, int ysize, int center_x, int center_y, int **map);

room_rnum find_available_wilderness_room();         /* Get the next empty room in the dynamic room pool. */
room_rnum find_room_by_coordinates(int x, int y);   /* Get the room at coordinates (x,y) */
void assign_wilderness_room(room_rnum room, int x, int y); /* Assign the room to the provided coordinates, adjusting
                                                            * descriptions, etc. */
int get_sector_type(int elevation, int temperature, int moisture);
void show_wilderness_map(struct char_data *ch, int size, int x, int y);
void save_map_to_file(const char *fn, int xsize, int ysize);
void save_noise_to_file(int idx, const char* fn, int xsize, int ysize);

void initialize_wilderness_lists();
#endif
