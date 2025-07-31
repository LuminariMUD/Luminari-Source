#define HUNT_TYPE_NONE              0
#define HUNT_TYPE_BASILISK          1
#define HUNT_TYPE_MANTICORE         2
#define HUNT_TYPE_WRAITH            3
#define HUNT_TYPE_SIREN             4
#define HUNT_TYPE_WILL_O_WISP       5
#define HUNT_TYPE_BARGHEST          6
#define HUNT_TYPE_BLACK_PUDDING     7
#define HUNT_TYPE_CHIMERA           8
#define HUNT_TYPE_GHOST             9
#define HUNT_TYPE_MEDUSA            10
#define HUNT_TYPE_BEHIR             11
#define HUNT_TYPE_EFREETI           12
#define HUNT_TYPE_DJINNI            13
#define HUNT_TYPE_YOUNG_RED_DRAGON  14
#define HUNT_TYPE_ADULT_RED_DRAGON  15
#define HUNT_TYPE_OLD_RED_DRAGON    16
#define HUNT_TYPE_YOUNG_BLUE_DRAGON 17
#define HUNT_TYPE_ADULT_BLUE_DRAGON 18
#define HUNT_TYPE_OLD_BLUE_DRAGON   19
#define HUNT_TYPE_YOUNG_GREEN_DRAGON 20
#define HUNT_TYPE_ADULT_GREEN_DRAGON 21
#define HUNT_TYPE_OLD_GREEN_DRAGON  22
#define HUNT_TYPE_YOUNG_BLACK_DRAGON 23
#define HUNT_TYPE_ADULT_BLACK_DRAGON 24
#define HUNT_TYPE_OLD_BLACK_DRAGON  25
#define HUNT_TYPE_YOUNG_WHITE_DRAGON 26
#define HUNT_TYPE_ADULT_WHITE_DRAGON 27
#define HUNT_TYPE_OLD_WHITE_DRAGON  28
#define HUNT_TYPE_DRAGON_TURTLE     29
#define HUNT_TYPE_ROC               30
#define HUNT_TYPE_PURPLE_WORM       31
#define HUNT_TYPE_PYROHYDRA         32
#define HUNT_TYPE_BANDERSNATCH      33
#define HUNT_TYPE_BANSHEE           34

#define NUM_HUNT_TYPES              35

#define HUNT_ABIL_NONE              0
#define HUNT_ABIL_PETRIFY           1
#define HUNT_ABIL_TAIL_SPIKES       2
#define HUNT_ABIL_LEVEL_DRAIN       3
#define HUNT_ABIL_CHARM             4
#define HUNT_ABIL_BLINK             5
#define HUNT_ABIL_ENGULF            6
#define HUNT_ABIL_CAUSE_FEAR        7
#define HUNT_ABIL_CORRUPTION        8
#define HUNT_ABIL_SWALLOW           9
#define HUNT_ABIL_FLIGHT            10
#define HUNT_ABIL_POISON            11
#define HUNT_ABIL_REGENERATION      12
#define HUNT_ABIL_PARALYZE          13
#define HUNT_ABIL_FIRE_BREATH       14
#define HUNT_ABIL_LIGHTNING_BREATH  15
#define HUNT_ABIL_POISON_BREATH     16
#define HUNT_ABIL_ACID_BREATH       17
#define HUNT_ABIL_FROST_BREATH      18
#define HUNT_ABIL_MAGIC_IMMUNITY    19
#define HUNT_ABIL_INVISIBILITY      20
#define HUNT_ABIL_GRAPPLE           21

#define NUM_HUNT_ABILITIES          22

#define AHUNT_1 5
#define AHUNT_2 7

#define MIN_HUNT_DROP_VNUM          60000
#define MAX_HUNT_DROP_VNUM          60039

#define HUNT_REWARD_TYPE_TRINKET    1
#define HUNT_REWARD_TYPE_WEAPON_OIL 2

#if defined(CAMPAIGN_DL)
#define HUNTS_MOB_VNUM              60001
#define HUNTS_REWARD_ITEM_VNUM      60090
#else
#define HUNTS_MOB_VNUM              8101
#define HUNTS_REWARD_ITEM_VNUM      60100
#endif

struct hunt_type {

  int hunt_type;
  int level;
  const char * name;
  const char * description;
  const char * long_description;
  int char_class;
  int alignment;
  int race_type;
  int subrace[3];
  int size;
  int abilities[NUM_HUNT_ABILITIES];

};

extern struct hunt_type hunt_table[NUM_HUNT_TYPES];
extern int active_hunts[5][7];
extern int hunt_reset_timer;

void select_hunt_coords(int which_hunt);
void select_reported_hunt_coords(int which_hunt, int times_called);
void load_hunts(void);
void create_hunts(void);
int select_a_hunt(int level);
void check_hunt_room(room_rnum room);
void create_hunt_mob(room_rnum room, int which_hunt);
SPECIAL_DECL(huntsmaster);
void award_hunt_materials(struct char_data *ch, int which_hunt);
void drop_hunt_mob_rewards(struct char_data *ch, struct char_data *hunt_mob);
void list_hunt_rewards(struct char_data *ch, int type);
int hunts_special_weapon_type(int hunt_record);
int hunts_special_armor_type(int hunt_record);
int get_hunt_armor_drop_vnum(int hunt_record);
int get_hunt_weapon_drop_vnum(int hunt_record);
bool is_hunt_trophy_a_trinket(int vnum);
void remove_hunts_mob(int which_hunt);
bool is_hunt_mob_in_room(room_rnum room, int which_hunt);
bool weapon_specab_desc_position(int specab);
bool is_weapon_specab_compatible(struct char_data *ch, int weapon_type, int specab, bool output);
int obj_vnum_to_hunt_type(int vnum);
bool is_specab_upgradeable(int specab_source, int specab_apply);
int get_hunt_room(int start, int x, int y);