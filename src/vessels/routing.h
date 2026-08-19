/**
 * @file routing.h                LuminariMUD
 * Headers for Luminari travel routing functions.
 */

#ifndef _ROUTING_H_ /* Begin header file protection */
#define _ROUTING_H_

#include "structs.h"
#include "utils.h"

/* Functions in routing.c. */
const char *get_transport_zone_entrance_name(int locale, int type);
const char *get_transport_sailing_name(int locale);
const char *get_transport_carriage_name(int locale);
int get_carriage_locale_vnum(int locale);
int get_sailing_locale_vnum(int locale);
const char *get_carriage_locale_region(int locale);
int get_carriage_locale_cost(int locale);
const char *get_carriage_locale_notes(int locale);
int get_sailing_locale_cost(int locale);
const char *get_sailing_locale_notes(int locale);
int get_sailing_locale_x(int locale);
int get_sailing_locale_y(int locale);
int get_carriage_locale_x(int locale);
int get_carriage_locale_y(int locale);
int get_walkto_landmark_vnum(int locale);
const char *get_walkto_landmark_region(int locale);
const char *get_walkto_landmark_name(int locale);
const char *get_walkto_landmark_notes(int locale);

void start_flight_to_destination_luminari(struct char_data *ch, const char *zone);

#endif /* _ROUTING_H_ */

/*EOF*/
