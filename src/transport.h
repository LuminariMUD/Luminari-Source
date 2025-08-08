/* ************************************************************************
 *      File:   transport.h                          Part of LuminariMUD  *
 *   Purpose:   To provide auto travel functionality                      *
 * Servicing:   transport.c                                               *
 *    Author:   Gicker                                                    *
 ************************************************************************ */

/* defines */
#define TRAVEL_CARRIAGE 1
#define TRAVEL_SAILING 2
#define TRAVEL_OVERLAND_FLIGHT 3
#define TRAVEL_OVERLAND_FLIGHT_SAIL 4

#define SAILING_LOCALES_FIELDS 7
#define CARRIAGE_LOCALES_FIELDS 7
#define WALKTO_LANDMARKS_FIELDS 4

/* externals */
extern const char *sailing_locales_dl[][SAILING_LOCALES_FIELDS];
extern const char *carriage_locales_dl[][CARRIAGE_LOCALES_FIELDS];
extern const char *walkto_landmarks_dl[][WALKTO_LANDMARKS_FIELDS];

extern const char *sailing_locales_lumi[][SAILING_LOCALES_FIELDS];
extern const char *carriage_locales_lumi[][CARRIAGE_LOCALES_FIELDS];
extern const char *walkto_landmarks_lumi[][WALKTO_LANDMARKS_FIELDS];

/* functions */
int get_travel_time(struct char_data *ch, int speed, int locale, int here, int type);
int get_distance(struct char_data *ch, int locale, int here, int type);
void travel_tickdown(void);
void enter_transport(struct char_data *ch, int locale, int type, int here);
int valid_sailing_travel(int here, int i);
int walkto_vnum_to_list_row(int vnum);

/* commands */
ACMD_DECL(do_sail);
ACMD_DECL(do_carriage);
ACMD_DECL(do_walkto);
ACMD_DECL(do_landmarks);
ACMD_DECL(do_walkto_full);
ACMD_DECL(do_landmarks_full);
ACMD_DECL(do_walkto_city);
ACMD_DECL(do_landmarks_city);

/* EoF */
