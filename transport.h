
#define TRAVEL_CARRIAGE         1
#define TRAVEL_AIRSHIP          2

extern const char *airship_locales[][7];
extern const char *carriage_locales[][5];

int get_travel_time(struct char_data *ch, int speed, int locale, int here);
int get_distance(struct char_data *ch, int locale, int here);
void travel_tickdown(void);
void enter_carriage(struct char_data *ch, int locale, int type, int here);
int valid_airship_travel(int here, int i);
ACMD_DECL(do_airship);
ACMD_DECL(do_carriage);
