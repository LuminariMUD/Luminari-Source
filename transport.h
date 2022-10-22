
#define TRAVEL_CARRIAGE         1
#define TRAVEL_SAILING          2

extern const char *sailing_locales[][7];
extern const char *carriage_locales[][7];
extern const char *walkto_landmarks[][4];

int get_travel_time(struct char_data *ch, int speed, int locale, int here, int type);
int get_distance(struct char_data *ch, int locale, int here, int type);
void travel_tickdown(void);
void enter_transport(struct char_data *ch, int locale, int type, int here);
int valid_sailing_travel(int here, int i);
ACMD_DECL(do_sail);
ACMD_DECL(do_carriage);
ACMD_DECL(do_walkto);
ACMD_DECL(do_landmarks);
const char *get_walkto_location_name(int locale_vnum);
