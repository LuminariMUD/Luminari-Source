/********************************************************************
 * Name:   mysql.h 
 * Author: Ornir (James McLaughlin)
 *
 * MySQL Header file for Luminari MUD.
 ********************************************************************/

#ifndef _MYSQL_H
#define _MYSQL_H

#include <mysql/mysql.h> /* System headerfile for mysql. */

extern MYSQL *conn;

void connect_to_mysql();
void disconnect_from_mysql();
void disconnect_from_mysql2();
void disconnect_from_mysql3();

/* Wilderness */
struct wilderness_data *load_wilderness(zone_vnum zone);
void load_regions();
struct region_list *get_enclosing_regions(zone_rnum zone, int x, int y);
void load_paths();
struct path_list *get_enclosing_paths(zone_rnum zone, int x, int y);
bool get_random_region_location(region_vnum region, int *x, int *y);
struct region_proximity_list *get_nearby_regions(zone_rnum zone, int x, int y, int r);
char **tokenize(const char *input, const char *delim);

#endif
