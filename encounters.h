
#define ENCOUNTER_CLASS_NONE               0
#define ENCOUNTER_CLASS_COMBAT             1

#define NUM_ENCOUNTER_CLASSES              2

#define ENCOUNTER_TYPE_NONE                0
#define ENCOUNTER_TYPE_GOBLIN_1            1
#define ENCOUNTER_TYPE_GOBLIN_2            2

#define NUM_ENCOUNTER_TYPES                3

#define ENCOUNTER_GROUP_TYPE_NONE          0
#define ENCOUNTER_GROUP_TYPE_GOBLINS       1

#define NUM_ENCOUNTER_GROUP_TYPES          2

#define ENCOUNTER_STRENGTH_NORMAL          0
#define ENCOUNTER_STRENGTH_BOSS            1

#define NUM_ENCOUNTER_STRENGTHS            2

#define TREASURE_TABLE_NONE                0
#define TREASURE_TABLE_LOW_NORM            1
#define TREASURE_TABLE_LOW_BOSS            2
#define TREASURE_TABLE_LOW_MID_NORM        3
#define TREASURE_TABLE_LOW_MID_BOSS        4
#define TREASURE_TABLE_MID_NORM            5
#define TREASURE_TABLE_MID_BOSS            6
#define TREASURE_TABLE_MID_HIGH_NORM       7
#define TREASURE_TABLE_MID_HIGH_BOSS       8
#define TREASURE_TABLE_HIGH_NORM           9
#define TREASURE_TABLE_HIGH_BOSS           10
#define TREASURE_TABLE_EPIC_LOW_NORM       11
#define TREASURE_TABLE_EPIC_LOW_BOSS       12
#define TREASURE_TABLE_EPIC_MID_NORM       13
#define TREASURE_TABLE_EPIC_MID_BOSS       14
#define TREASURE_TABLE_EPIC_HIGH_NORM      15
#define TREASURE_TABLE_EPIC_HIGH_BOSS      16

#define NUM_TREASURE_TABLES                17

#define ENCOUNTER_MOB_VNUM                 8100

struct encounter_data {

  int encounter_type;
  int max_level;
  bool sector_types[NUM_ROOM_SECTORS];
  int encounter_group;
  const char * object_name;
  int load_chance;
  int min_number;
  int max_number;
  int treasure_table;
  int char_class;
  int encounter_strength;
};

extern struct encounter_data encounter_table[NUM_ENCOUNTER_TYPES];

int get_exploration_dc(struct char_data *ch);
bool in_encounter_room(struct char_data *ch);
void check_random_encounter(struct char_data *ch);
int encounter_chance(struct char_data *ch);
void populate_encounter_table(void);