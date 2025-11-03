/**
 * @file routing.h                LuminariMUD
 * Headers for Functions used to route mud options to the proper campaign or
 * cedit option.
 */

#ifndef _ROUTING_H_ /* Begin header file protection */
#define _ROUTING_H_

#include "structs.h"
#include "utils.h"

 #define IS_CAMPAIGN_DL (CONFIG_CAMPAIGN == CAMPAIGN_DRAGONLANCE)
 #define IS_CAMPAIGN_FR (CONFIG_CAMPAIGN == CAMPAIGN_FORGOTTEN_REALMS)
 #define IS_CAMPAIGN_LUMINARI (CONFIG_CAMPAIGN == CAMPAIGN_LUMINARI)
 #define IS_CAMPAIGN_DEFAULT (IS_CAMPAIGN_LUMINARI)


 // functions in routing.c
const char * get_transport_zone_entrance_name(int locale, int type);
const char * get_transport_sailing_name(int locale);
const char * get_transport_carriage_name(int locale);
int get_carriage_locale_vnum(int locale);
int get_sailing_locale_vnum(int locale);
const char * get_carriage_locale_region(int locale);
int get_carriage_locale_cost(int locale);
const char * get_carriage_locale_notes(int locale);
int get_sailing_locale_cost(int locale);
const char * get_sailing_locale_notes(int locale);
int get_sailing_locale_x(int locale);
int get_sailing_locale_y(int locale);
int get_carriage_locale_x(int locale);
int get_carriage_locale_y(int locale);
int get_walkto_landmark_vnum(int locale);
const char * get_walkto_landmark_region(int locale);
const char * get_walkto_landmark_name(int locale);
const char * get_walkto_landmark_notes(int locale);

void start_flight_to_destination_luminari(struct char_data *ch, const char *zone);
void start_flight_to_zone_dl(struct char_data *ch, const char *zone);
void start_fr_flight_to_zone(struct char_data *ch, const char *zone);

 #endif /* _UTILS_H_ */

/*EOF*/