/* ************************************************************************
 *    File:   encounters.c                           Part of LuminariMUD  *
 * Purpose:   Random encounter system for wilderness area                 *
 *  Author:   Gicker                                                      *
 ************************************************************************ */

#include <math.h>

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "oasis.h"
#include "screen.h"
#include "interpreter.h"
#include "modify.h"
#include "spells.h"
#include "feats.h"
#include "class.h"
#include "handler.h"
#include "constants.h"
#include "assign_wpn_armor.h"
#include "domains_schools.h"
#include "spell_prep.h"
#include "alchemy.h"
#include "race.h"
#include "encounters.h"
#include "dg_scripts.h"
#include "prefedit.h"
#include "mud_event.h"
#include "act.h"

extern struct room_data *world;
extern struct char_data *character_list;

struct encounter_data encounter_table[NUM_ENCOUNTER_TYPES];

/* function to assign basic attributes to an encounter record */
void add_encounter_record(int encounter_record, int encounter_type, int min_level, int max_level, int encounter_group, const char *object_name,
                          int load_chance, int min_number, int max_number, int treasure_table, int char_class, int encounter_strength,
                          int alignment, int race_type, int subrace1, int subrace2, int subrace3, bool hostile, bool sentient, int size)
{
  encounter_table[encounter_record].encounter_type = encounter_type;
  encounter_table[encounter_record].min_level = min_level;
  encounter_table[encounter_record].max_level = max_level;
  encounter_table[encounter_record].encounter_group = encounter_group;
  encounter_table[encounter_record].object_name = object_name;
  encounter_table[encounter_record].load_chance = load_chance;
  encounter_table[encounter_record].min_number = min_number;
  encounter_table[encounter_record].max_number = max_number;
  encounter_table[encounter_record].treasure_table = treasure_table;
  encounter_table[encounter_record].char_class = char_class;
  encounter_table[encounter_record].encounter_strength = encounter_strength;
  encounter_table[encounter_record].alignment = alignment;
  encounter_table[encounter_record].race_type = race_type;
  encounter_table[encounter_record].subrace[0] = subrace1;
  encounter_table[encounter_record].subrace[1] = subrace2;
  encounter_table[encounter_record].subrace[2] = subrace3;
  encounter_table[encounter_record].size = size;
  encounter_table[encounter_record].hostile = hostile;
  encounter_table[encounter_record].sentient = sentient;
}

void set_encounter_description(int encounter_record, const char *description)
{
  encounter_table[encounter_record].description = description;
}

void set_encounter_long_description(int encounter_record, const char *long_description)
{
  encounter_table[encounter_record].long_description = long_description;
}

void initialize_encounter_table(void)
{
  int i, j;

  /* initialize the list of encounters */
  for (i = 0; i < NUM_ENCOUNTER_TYPES; i++)
  {
    encounter_table[i].encounter_type = ENCOUNTER_TYPE_NONE;
    encounter_table[i].max_level = 0;
    encounter_table[i].min_level = 0;
    encounter_table[i].encounter_group = ENCOUNTER_GROUP_TYPE_NONE;
    encounter_table[i].object_name = "Unused Encounter";
    encounter_table[i].description = "Nothing";
    encounter_table[i].long_description = "Nothing";
    encounter_table[i].load_chance = 0;
    encounter_table[i].min_number = 0;
    encounter_table[i].max_number = 0;
    encounter_table[i].treasure_table = TREASURE_TABLE_NONE;
    encounter_table[i].char_class = CLASS_WARRIOR;
    encounter_table[i].encounter_strength = ENCOUNTER_STRENGTH_NORMAL;
    encounter_table[i].alignment = TRUE_NEUTRAL;
    encounter_table[i].race_type = RACE_TYPE_HUMANOID;
    encounter_table[i].subrace[0] = SUBRACE_UNKNOWN;
    encounter_table[i].subrace[1] = SUBRACE_UNKNOWN;
    encounter_table[i].subrace[2] = SUBRACE_UNKNOWN;
    encounter_table[i].size = SIZE_MEDIUM;
    encounter_table[i].hostile = false;
    encounter_table[i].sentient = false;
    for (j = 0; j < NUM_ROOM_SECTORS; j++)
      encounter_table[i].sector_types[j] = false;
  }
}

void add_encounter_sector(int encounter_record, int sector_type)
{
  encounter_table[encounter_record].sector_types[sector_type] = true;
}

void set_encounter_terrain_any(int encounter_record)
{
  int j;
  for (j = 0; j < NUM_ROOM_SECTORS; j++)
    encounter_table[encounter_record].sector_types[j] = true;
}

void set_encounter_terrain_all_surface(int encounter_record)
{
  encounter_table[encounter_record].sector_types[SECT_INSIDE] = true;
  encounter_table[encounter_record].sector_types[SECT_INSIDE_ROOM] = true;
  encounter_table[encounter_record].sector_types[SECT_CITY] = true;
  encounter_table[encounter_record].sector_types[SECT_FIELD] = true;
  encounter_table[encounter_record].sector_types[SECT_FOREST] = true;
  encounter_table[encounter_record].sector_types[SECT_HILLS] = true;
  encounter_table[encounter_record].sector_types[SECT_MOUNTAIN] = true;
  encounter_table[encounter_record].sector_types[SECT_ROAD_NS] = true;
  encounter_table[encounter_record].sector_types[SECT_ROAD_EW] = true;
  encounter_table[encounter_record].sector_types[SECT_ROAD_INT] = true;
  encounter_table[encounter_record].sector_types[SECT_DESERT] = true;
  encounter_table[encounter_record].sector_types[SECT_MARSHLAND] = true;
  encounter_table[encounter_record].sector_types[SECT_HIGH_MOUNTAIN] = true;
  encounter_table[encounter_record].sector_types[SECT_LAVA] = true;
  encounter_table[encounter_record].sector_types[SECT_D_ROAD_NS] = true;
  encounter_table[encounter_record].sector_types[SECT_D_ROAD_EW] = true;
  encounter_table[encounter_record].sector_types[SECT_D_ROAD_INT] = true;
  encounter_table[encounter_record].sector_types[SECT_CAVE] = true;
  encounter_table[encounter_record].sector_types[SECT_JUNGLE] = true;
  encounter_table[encounter_record].sector_types[SECT_TUNDRA] = true;
  encounter_table[encounter_record].sector_types[SECT_TAIGA] = true;
  encounter_table[encounter_record].sector_types[SECT_BEACH] = true;
}

void set_encounter_terrain_all_underdark(int encounter_record)
{
  encounter_table[encounter_record].sector_types[SECT_UD_WILD] = true;
  encounter_table[encounter_record].sector_types[SECT_UD_CITY] = true;
  encounter_table[encounter_record].sector_types[SECT_UD_INSIDE] = true;
}

void set_encounter_terrain_all_roads(int encounter_record)
{
  encounter_table[encounter_record].sector_types[SECT_D_ROAD_NS] = true;
  encounter_table[encounter_record].sector_types[SECT_D_ROAD_EW] = true;
  encounter_table[encounter_record].sector_types[SECT_D_ROAD_INT] = true;
  encounter_table[encounter_record].sector_types[SECT_ROAD_NS] = true;
  encounter_table[encounter_record].sector_types[SECT_ROAD_EW] = true;
  encounter_table[encounter_record].sector_types[SECT_ROAD_INT] = true;
}

void set_encounter_terrain_all_water(int encounter_record)
{
  encounter_table[encounter_record].sector_types[SECT_WATER_SWIM] = true;
  encounter_table[encounter_record].sector_types[SECT_WATER_NOSWIM] = true;
  encounter_table[encounter_record].sector_types[SECT_UNDERWATER] = true;
  encounter_table[encounter_record].sector_types[SECT_OCEAN] = true;
  encounter_table[encounter_record].sector_types[SECT_UD_WATER] = true;
  encounter_table[encounter_record].sector_types[SECT_UD_NOSWIM] = true;
}

void populate_encounter_table(void)
{
  initialize_encounter_table();

  // CR 1 and Under

  add_encounter_record(ENCOUNTER_TYPE_GOBLIN_1, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_GOBLINS, "goblin raider", 100, 1, 3,
                       TREASURE_TABLE_LOW_NORM, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_EVIL, RACE_TYPE_HUMANOID,
                       SUBRACE_GOBLINOID, SUBRACE_EVIL, SUBRACE_CHAOTIC, HOSTILE, SENTIENT, SIZE_SMALL);
  set_encounter_terrain_any(ENCOUNTER_TYPE_GOBLIN_1);

  add_encounter_record(ENCOUNTER_TYPE_GOBLIN_2, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_GOBLINS, "goblin chieftain", 25, 1, 1,
                       TREASURE_TABLE_LOW_BOSS, CLASS_WARRIOR, ENCOUNTER_STRENGTH_BOSS, CHAOTIC_EVIL, RACE_TYPE_HUMANOID,
                       SUBRACE_GOBLINOID, SUBRACE_EVIL, SUBRACE_CHAOTIC, HOSTILE, SENTIENT, SIZE_SMALL);
  set_encounter_terrain_any(ENCOUNTER_TYPE_GOBLIN_2);

  add_encounter_record(ENCOUNTER_TYPE_KING_CRAB_1, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_KING_CRABS, "king crab", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_VERMIN,
                       SUBRACE_AQUATIC, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, NON_SENTIENT, SIZE_TINY);
  add_encounter_sector(ENCOUNTER_TYPE_KING_CRAB_1, SECT_WATER_SWIM);
  add_encounter_sector(ENCOUNTER_TYPE_KING_CRAB_1, SECT_WATER_NOSWIM);
  add_encounter_sector(ENCOUNTER_TYPE_KING_CRAB_1, SECT_UNDERWATER);
  add_encounter_sector(ENCOUNTER_TYPE_KING_CRAB_1, SECT_OCEAN);
  add_encounter_sector(ENCOUNTER_TYPE_KING_CRAB_1, SECT_UD_WATER);
  add_encounter_sector(ENCOUNTER_TYPE_KING_CRAB_1, SECT_UD_NOSWIM);
  add_encounter_sector(ENCOUNTER_TYPE_KING_CRAB_1, SECT_BEACH);

  add_encounter_record(ENCOUNTER_TYPE_KOBOLD_1, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_KOBOLDS, "kobold warrior", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, LAWFUL_EVIL, RACE_TYPE_HUMANOID,
                       SUBRACE_REPTILIAN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_SMALL);
  set_encounter_terrain_any(ENCOUNTER_TYPE_KOBOLD_1);

  add_encounter_record(ENCOUNTER_TYPE_GIANT_RAT_1, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_GIANT_RATS, "giant rat", 100, 1, 6,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_VERMIN,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_TINY);
  set_encounter_terrain_any(ENCOUNTER_TYPE_GIANT_RAT_1);

  add_encounter_record(ENCOUNTER_TYPE_SEWER_CENTIPEDE, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_CENTIPEDES, "sewer centipede", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_VERMIN,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_SMALL);
  set_encounter_terrain_all_underdark(ENCOUNTER_TYPE_SEWER_CENTIPEDE);
  add_encounter_sector(ENCOUNTER_TYPE_SEWER_CENTIPEDE, SECT_CAVE);

  add_encounter_record(ENCOUNTER_TYPE_SEA_WASP_JELLYFISH, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_SEA_WASP_JELLYFISH, "sea wasp jellyfish", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_VERMIN,
                       SUBRACE_AQUATIC, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_TINY);
  add_encounter_sector(ENCOUNTER_TYPE_SEA_WASP_JELLYFISH, SECT_WATER_SWIM);
  add_encounter_sector(ENCOUNTER_TYPE_SEA_WASP_JELLYFISH, SECT_WATER_NOSWIM);
  add_encounter_sector(ENCOUNTER_TYPE_SEA_WASP_JELLYFISH, SECT_UNDERWATER);
  add_encounter_sector(ENCOUNTER_TYPE_SEA_WASP_JELLYFISH, SECT_OCEAN);
  add_encounter_sector(ENCOUNTER_TYPE_SEA_WASP_JELLYFISH, SECT_UD_WATER);
  add_encounter_sector(ENCOUNTER_TYPE_SEA_WASP_JELLYFISH, SECT_UD_NOSWIM);

  add_encounter_record(ENCOUNTER_TYPE_MANDRILL, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_MANDRILL, "mandrill", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_SMALL);
  add_encounter_sector(ENCOUNTER_TYPE_MANDRILL, SECT_FOREST);
  add_encounter_sector(ENCOUNTER_TYPE_MANDRILL, SECT_JUNGLE);

  add_encounter_record(ENCOUNTER_TYPE_MUCKDWELLER, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_MUCKDWELLER, "muckdweller", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_MAGICAL_BEAST,
                       SUBRACE_REPTILIAN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_SMALL);
  add_encounter_sector(ENCOUNTER_TYPE_MUCKDWELLER, SECT_MARSHLAND);

  add_encounter_record(ENCOUNTER_TYPE_BARRACUDA, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_BARRACUDA, "barracuda", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_AQUATIC, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_SMALL);
  set_encounter_terrain_all_water(ENCOUNTER_TYPE_BARRACUDA);

  add_encounter_record(ENCOUNTER_TYPE_FIRE_BEETLE, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_FIRE_BEETLE, "fire beetle", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_VERMIN,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_SMALL);
  set_encounter_terrain_any(ENCOUNTER_TYPE_FIRE_BEETLE);

#if !defined(CAMPAIGN_DL)
  add_encounter_record(ENCOUNTER_TYPE_DROW_WARRIOR, ENCOUNTER_CLASS_COMBAT, 1, 20, ENCOUNTER_GROUP_TYPE_DROW_PATROL, "drow warrior", 100, 1, 3,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_EVIL, RACE_TYPE_HUMANOID,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_MEDIUM);
  add_encounter_sector(ENCOUNTER_TYPE_DROW_WARRIOR, SECT_UD_WILD);
  add_encounter_sector(ENCOUNTER_TYPE_DROW_WARRIOR, SECT_UD_CITY);
  add_encounter_sector(ENCOUNTER_TYPE_DROW_WARRIOR, SECT_UD_INSIDE);

  add_encounter_record(ENCOUNTER_TYPE_DROW_PRIESTESS, ENCOUNTER_CLASS_COMBAT, 1, 20, ENCOUNTER_GROUP_TYPE_DROW_PATROL, "drow priestess", 100, 1, 1,
                       TREASURE_TABLE_NONE, CLASS_CLERIC, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_EVIL, RACE_TYPE_HUMANOID,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_MEDIUM);
  add_encounter_sector(ENCOUNTER_TYPE_DROW_PRIESTESS, SECT_UD_WILD);
  add_encounter_sector(ENCOUNTER_TYPE_DROW_PRIESTESS, SECT_UD_CITY);
  add_encounter_sector(ENCOUNTER_TYPE_DROW_PRIESTESS, SECT_UD_INSIDE);

  add_encounter_record(ENCOUNTER_TYPE_DROW_WIZARD, ENCOUNTER_CLASS_COMBAT, 1, 20, ENCOUNTER_GROUP_TYPE_DROW_PATROL, "drow wizard", 100, 1, 1,
                       TREASURE_TABLE_NONE, CLASS_WIZARD, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_EVIL, RACE_TYPE_HUMANOID,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_MEDIUM);
  add_encounter_sector(ENCOUNTER_TYPE_DROW_WIZARD, SECT_UD_WILD);
  add_encounter_sector(ENCOUNTER_TYPE_DROW_WIZARD, SECT_UD_CITY);
  add_encounter_sector(ENCOUNTER_TYPE_DROW_WIZARD, SECT_UD_INSIDE);
#endif  

  add_encounter_record(ENCOUNTER_TYPE_DUERGAR_WARRIOR, ENCOUNTER_CLASS_COMBAT, 1, 20, ENCOUNTER_GROUP_TYPE_DUERGAR_PATROL, "duergar warrior", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, LAWFUL_EVIL, RACE_TYPE_HUMANOID,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_MEDIUM);
  add_encounter_sector(ENCOUNTER_TYPE_DUERGAR_WARRIOR, SECT_UD_WILD);
  add_encounter_sector(ENCOUNTER_TYPE_DUERGAR_WARRIOR, SECT_UD_CITY);
  add_encounter_sector(ENCOUNTER_TYPE_DUERGAR_WARRIOR, SECT_UD_INSIDE);

  add_encounter_record(ENCOUNTER_TYPE_HAWK, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_HAWK, "hawk", 100, 1, 2,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, NON_SENTIENT, SIZE_TINY);
  add_encounter_sector(ENCOUNTER_TYPE_HAWK, SECT_FOREST);
  add_encounter_sector(ENCOUNTER_TYPE_HAWK, SECT_FLYING);
  add_encounter_sector(ENCOUNTER_TYPE_HAWK, SECT_TAIGA);

  add_encounter_record(ENCOUNTER_TYPE_MERFOLK_HUNTER, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_MERFOLK, "merfolk hunter", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_HUMANOID,
                       SUBRACE_AQUATIC, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SENTIENT, SIZE_MEDIUM);
  set_encounter_terrain_all_water(ENCOUNTER_TYPE_MERFOLK_HUNTER);

#if !defined(CAMPAIGN_DL)
  add_encounter_record(ENCOUNTER_TYPE_ORCISH_WARRIOR, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_ORC_PATROL, "orcish warrior", 100, 1, 6,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_EVIL, RACE_TYPE_HUMANOID,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_MEDIUM);
  add_encounter_sector(ENCOUNTER_TYPE_ORCISH_WARRIOR, SECT_FIELD);
  add_encounter_sector(ENCOUNTER_TYPE_ORCISH_WARRIOR, SECT_FOREST);
  add_encounter_sector(ENCOUNTER_TYPE_ORCISH_WARRIOR, SECT_HILLS);
  add_encounter_sector(ENCOUNTER_TYPE_ORCISH_WARRIOR, SECT_MOUNTAIN);
  add_encounter_sector(ENCOUNTER_TYPE_ORCISH_WARRIOR, SECT_HIGH_MOUNTAIN);
  add_encounter_sector(ENCOUNTER_TYPE_ORCISH_WARRIOR, SECT_UD_WILD);
  add_encounter_sector(ENCOUNTER_TYPE_ORCISH_WARRIOR, SECT_UD_INSIDE);
  add_encounter_sector(ENCOUNTER_TYPE_ORCISH_WARRIOR, SECT_CAVE);
#endif

  add_encounter_record(ENCOUNTER_TYPE_OWL, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_OWL, "owl", 100, 1, 2,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, NON_SENTIENT, SIZE_TINY);
  add_encounter_sector(ENCOUNTER_TYPE_OWL, SECT_FOREST);
  add_encounter_sector(ENCOUNTER_TYPE_OWL, SECT_FLYING);
  add_encounter_sector(ENCOUNTER_TYPE_OWL, SECT_TAIGA);

  add_encounter_record(ENCOUNTER_TYPE_RATFOLK_WANDERER, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_RATFOLK, "ratfolk wanderer", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_HUMANOID,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_SMALL);
  add_encounter_sector(ENCOUNTER_TYPE_RATFOLK_WANDERER, SECT_CITY);
  add_encounter_sector(ENCOUNTER_TYPE_RATFOLK_WANDERER, SECT_DESERT);

  add_encounter_record(ENCOUNTER_TYPE_SEAL, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_SEAL, "seal", 100, 1, 2,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_AQUATIC, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, NON_SENTIENT, SIZE_SMALL);
  add_encounter_sector(ENCOUNTER_TYPE_SEAL, SECT_WATER_SWIM);
  add_encounter_sector(ENCOUNTER_TYPE_SEAL, SECT_WATER_NOSWIM);
  add_encounter_sector(ENCOUNTER_TYPE_SEAL, SECT_OCEAN);
  add_encounter_sector(ENCOUNTER_TYPE_SEAL, SECT_BEACH);

  add_encounter_record(ENCOUNTER_TYPE_WANDERING_SKELETON, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_WANDERING_SKELETON, "wandering skeleton", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_UNDEAD,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_MEDIUM);
  set_encounter_terrain_any(ENCOUNTER_TYPE_WANDERING_SKELETON);

  add_encounter_record(ENCOUNTER_TYPE_SPRITE, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_SPRITE, "sprite", 100, 1, 3,
                       TREASURE_TABLE_NONE, CLASS_WIZARD, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_NEUTRAL, RACE_TYPE_FEY,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SENTIENT, SIZE_DIMINUTIVE);
  add_encounter_sector(ENCOUNTER_TYPE_SPRITE, SECT_FOREST);

  add_encounter_record(ENCOUNTER_TYPE_GIANT_MOSQUITO, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_GIANT_MOSQUITO, "giant mosquito", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_VERMIN,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_SMALL);
  add_encounter_sector(ENCOUNTER_TYPE_GIANT_MOSQUITO, SECT_MARSHLAND);

  add_encounter_record(ENCOUNTER_TYPE_ANTELOPE, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_ANTELOPE, "antelope", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, NON_SENTIENT, SIZE_SMALL);
  add_encounter_sector(ENCOUNTER_TYPE_ANTELOPE, SECT_FIELD);

  add_encounter_record(ENCOUNTER_TYPE_BABOON, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_BABOON, "baboon", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_SMALL);
  add_encounter_sector(ENCOUNTER_TYPE_BABOON, SECT_FIELD);
  add_encounter_sector(ENCOUNTER_TYPE_BABOON, SECT_FOREST);
  add_encounter_sector(ENCOUNTER_TYPE_BABOON, SECT_JUNGLE);

  add_encounter_record(ENCOUNTER_TYPE_BADGER, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_BADGER, "badger", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_SMALL);
  add_encounter_sector(ENCOUNTER_TYPE_BADGER, SECT_FOREST);
  add_encounter_sector(ENCOUNTER_TYPE_BADGER, SECT_TAIGA);

  add_encounter_record(ENCOUNTER_TYPE_DOLPHIN, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_DOLPHIN, "dolphin", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_AQUATIC, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, NON_SENTIENT, SIZE_MEDIUM);
  add_encounter_sector(ENCOUNTER_TYPE_DOLPHIN, SECT_OCEAN);

  add_encounter_record(ENCOUNTER_TYPE_EAGLE, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_EAGLE, "eagle", 100, 1, 2,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, NON_SENTIENT, SIZE_SMALL);
  add_encounter_sector(ENCOUNTER_TYPE_EAGLE, SECT_HILLS);
  add_encounter_sector(ENCOUNTER_TYPE_EAGLE, SECT_MOUNTAIN);
  add_encounter_sector(ENCOUNTER_TYPE_EAGLE, SECT_FLYING);
  add_encounter_sector(ENCOUNTER_TYPE_EAGLE, SECT_HIGH_MOUNTAIN);

  add_encounter_record(ENCOUNTER_TYPE_HOBGOBLIN_SOLDIER, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_HOBGOBLINS, "hobgoblin soldier", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, LAWFUL_EVIL, RACE_TYPE_HUMANOID,
                       SUBRACE_GOBLINOID, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_MEDIUM);
  add_encounter_sector(ENCOUNTER_TYPE_HOBGOBLIN_SOLDIER, SECT_HILLS);
  add_encounter_sector(ENCOUNTER_TYPE_HOBGOBLIN_SOLDIER, SECT_MOUNTAIN);
  add_encounter_sector(ENCOUNTER_TYPE_HOBGOBLIN_SOLDIER, SECT_CAVE);

  add_encounter_record(ENCOUNTER_TYPE_HOBGOBLIN_SERGEANT, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_HOBGOBLINS, "hobgoblin sergeant", 25, 1, 1,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_BOSS, LAWFUL_EVIL, RACE_TYPE_HUMANOID,
                       SUBRACE_GOBLINOID, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_HUGE);
  add_encounter_sector(ENCOUNTER_TYPE_HOBGOBLIN_SERGEANT, SECT_HILLS);
  add_encounter_sector(ENCOUNTER_TYPE_HOBGOBLIN_SERGEANT, SECT_MOUNTAIN);
  add_encounter_sector(ENCOUNTER_TYPE_HOBGOBLIN_SERGEANT, SECT_CAVE);

  add_encounter_record(ENCOUNTER_TYPE_BLUE_RINGED_OCTOPUS, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_BLUE_RINGED_OCTOPUS, "blue ringed octopus", 100, 1, 1,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_AQUATIC, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, NON_SENTIENT, SIZE_TINY);
  set_encounter_terrain_all_water(ENCOUNTER_TYPE_BLUE_RINGED_OCTOPUS);

  add_encounter_record(ENCOUNTER_TYPE_SAGARI, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_SAGARI, "sagari", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, NEUTRAL_EVIL, RACE_TYPE_ABERRATION,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_TINY);
  add_encounter_sector(ENCOUNTER_TYPE_SAGARI, SECT_INSIDE);
  add_encounter_sector(ENCOUNTER_TYPE_SAGARI, SECT_FOREST);
  add_encounter_sector(ENCOUNTER_TYPE_SAGARI, SECT_TAIGA);
  add_encounter_sector(ENCOUNTER_TYPE_SAGARI, SECT_INSIDE_ROOM);

  add_encounter_record(ENCOUNTER_TYPE_VIPER, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_VIPER, "viper", 100, 1, 1,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_TINY);
  add_encounter_sector(ENCOUNTER_TYPE_VIPER, SECT_FIELD);
  add_encounter_sector(ENCOUNTER_TYPE_VIPER, SECT_FOREST);
  add_encounter_sector(ENCOUNTER_TYPE_VIPER, SECT_HILLS);
  add_encounter_sector(ENCOUNTER_TYPE_VIPER, SECT_MOUNTAIN);
  add_encounter_sector(ENCOUNTER_TYPE_VIPER, SECT_DESERT);
  add_encounter_sector(ENCOUNTER_TYPE_VIPER, SECT_MARSHLAND);
  add_encounter_sector(ENCOUNTER_TYPE_VIPER, SECT_CAVE);
  add_encounter_sector(ENCOUNTER_TYPE_VIPER, SECT_JUNGLE);
  add_encounter_sector(ENCOUNTER_TYPE_VIPER, SECT_BEACH);

  add_encounter_record(ENCOUNTER_TYPE_STINGRAY, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_STINGRAY, "stingray", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_AQUATIC, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, NON_SENTIENT, SIZE_MEDIUM);
  add_encounter_sector(ENCOUNTER_TYPE_STINGRAY, SECT_OCEAN);

  add_encounter_record(ENCOUNTER_TYPE_GIANT_CRAB_SPIDER, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_GIANT_CRAB_SPIDER, "giant crab spider", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_VERMIN,
                       SUBRACE_AQUATIC, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_SMALL);
  add_encounter_sector(ENCOUNTER_TYPE_GIANT_CRAB_SPIDER, SECT_FOREST);
  add_encounter_sector(ENCOUNTER_TYPE_GIANT_CRAB_SPIDER, SECT_JUNGLE);

  add_encounter_record(ENCOUNTER_TYPE_STIRGE, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_STIRGE, "stirge", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_MAGICAL_BEAST,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_TINY);
  add_encounter_sector(ENCOUNTER_TYPE_STIRGE, SECT_FOREST);
  add_encounter_sector(ENCOUNTER_TYPE_STIRGE, SECT_MARSHLAND);
  add_encounter_sector(ENCOUNTER_TYPE_STIRGE, SECT_JUNGLE);

  add_encounter_record(ENCOUNTER_TYPE_SYLPH, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_SYLPH, "sylph", 100, 1, 3,
                       TREASURE_TABLE_NONE, CLASS_ROGUE, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_OUTSIDER,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SENTIENT, SIZE_MEDIUM);
  set_encounter_terrain_all_surface(ENCOUNTER_TYPE_SYLPH);

  add_encounter_record(ENCOUNTER_TYPE_TENGU, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_TENGU, "tengu", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_ROGUE, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_HUMANOID,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SENTIENT, SIZE_MEDIUM);
  set_encounter_terrain_all_surface(ENCOUNTER_TYPE_TENGU);

  add_encounter_record(ENCOUNTER_TYPE_SNAPPING_TURTLE, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_SNAPPING_TURTLE, "snapping turtle", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_AQUATIC, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, NON_SENTIENT, SIZE_TINY);
  add_encounter_sector(ENCOUNTER_TYPE_SNAPPING_TURTLE, SECT_WATER_SWIM);
  add_encounter_sector(ENCOUNTER_TYPE_SNAPPING_TURTLE, SECT_WATER_NOSWIM);
  add_encounter_sector(ENCOUNTER_TYPE_SNAPPING_TURTLE, SECT_UNDERWATER);
  add_encounter_sector(ENCOUNTER_TYPE_SNAPPING_TURTLE, SECT_OCEAN);
  add_encounter_sector(ENCOUNTER_TYPE_SNAPPING_TURTLE, SECT_BEACH);

  add_encounter_record(ENCOUNTER_TYPE_UNDINE, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_UNDINE, "undine", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_CLERIC, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_OUTSIDER,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SENTIENT, SIZE_MEDIUM);
  set_encounter_terrain_all_surface(ENCOUNTER_TYPE_UNDINE);

  add_encounter_record(ENCOUNTER_TYPE_VEGEPYGMY, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_VEGEPYGMY, "vegepygmy", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_PLANT,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SENTIENT, SIZE_SMALL);
  set_encounter_terrain_all_underdark(ENCOUNTER_TYPE_VEGEPYGMY);
  add_encounter_sector(ENCOUNTER_TYPE_VEGEPYGMY, SECT_CAVE);

  add_encounter_record(ENCOUNTER_TYPE_VULTURE, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_VULTURE, "vulture", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, NON_SENTIENT, SIZE_SMALL);
  add_encounter_sector(ENCOUNTER_TYPE_VULTURE, SECT_FIELD);
  add_encounter_sector(ENCOUNTER_TYPE_VULTURE, SECT_HILLS);

  add_encounter_record(ENCOUNTER_TYPE_WANDERING_ZOMBIE, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_WANDERING_ZOMBIE, "wandering zombie", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_UNDEAD,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_MEDIUM);
  set_encounter_terrain_all_surface(ENCOUNTER_TYPE_WANDERING_ZOMBIE);
  set_encounter_terrain_all_underdark(ENCOUNTER_TYPE_WANDERING_ZOMBIE);

  add_encounter_record(ENCOUNTER_TYPE_GREMLIN, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_GREMLIN, "gremlin", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_ROGUE, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_EVIL, RACE_TYPE_FEY,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_SMALL);
  set_encounter_terrain_all_surface(ENCOUNTER_TYPE_GREMLIN);

  add_encounter_record(ENCOUNTER_TYPE_MANTARI, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_MANTARI, "mantari", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, NEUTRAL_EVIL, RACE_TYPE_MAGICAL_BEAST,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_MEDIUM);
  set_encounter_terrain_all_underdark(ENCOUNTER_TYPE_MANTARI);
  add_encounter_sector(ENCOUNTER_TYPE_MANTARI, SECT_CAVE);

  add_encounter_record(ENCOUNTER_TYPE_GIANT_AMOEBA, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_GIANT_AMOEBA, "giant amoeba", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_OOZE,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_SMALL);
  set_encounter_terrain_all_underdark(ENCOUNTER_TYPE_GIANT_AMOEBA);
  set_encounter_terrain_all_water(ENCOUNTER_TYPE_GIANT_AMOEBA);
  add_encounter_sector(ENCOUNTER_TYPE_GIANT_AMOEBA, SECT_CAVE);

  add_encounter_record(ENCOUNTER_TYPE_GIANT_BEE, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_GIANT_BEE, "giant bee", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_VERMIN,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_MEDIUM);
  add_encounter_sector(ENCOUNTER_TYPE_GIANT_BEE, SECT_FIELD);
  add_encounter_sector(ENCOUNTER_TYPE_GIANT_BEE, SECT_HILLS);
  add_encounter_sector(ENCOUNTER_TYPE_GIANT_BEE, SECT_MARSHLAND);
  add_encounter_sector(ENCOUNTER_TYPE_GIANT_BEE, SECT_JUNGLE);
  add_encounter_sector(ENCOUNTER_TYPE_GIANT_BEE, SECT_BEACH);

  add_encounter_record(ENCOUNTER_TYPE_BROWNIE, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_BROWNIE, "brownie", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_ROGUE, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_FEY,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SENTIENT, SIZE_TINY);
  add_encounter_sector(ENCOUNTER_TYPE_BROWNIE, SECT_FIELD);
  add_encounter_sector(ENCOUNTER_TYPE_BROWNIE, SECT_FOREST);

  add_encounter_record(ENCOUNTER_TYPE_CAMEL, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_CAMEL, "camel", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, NON_SENTIENT, SIZE_LARGE);
  add_encounter_sector(ENCOUNTER_TYPE_CAMEL, SECT_DESERT);

  add_encounter_record(ENCOUNTER_TYPE_CLAWBAT, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_CLAWBAT, "clawbat", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_MAGICAL_BEAST,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_SMALL);
  add_encounter_sector(ENCOUNTER_TYPE_CLAWBAT, SECT_HILLS);
  add_encounter_sector(ENCOUNTER_TYPE_CLAWBAT, SECT_CAVE);

  add_encounter_record(ENCOUNTER_TYPE_DARKMANTLE, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_DARKMANTLE, "darkmantle", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_MAGICAL_BEAST,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_SMALL);
  set_encounter_terrain_all_underdark(ENCOUNTER_TYPE_DARKMANTLE);
  add_encounter_sector(ENCOUNTER_TYPE_DARKMANTLE, SECT_CAVE);

  add_encounter_record(ENCOUNTER_TYPE_DIRE_CORBY, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_DIRE_CORBY, "dire corby", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, NEUTRAL_EVIL, RACE_TYPE_MONSTROUS_HUMANOID,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_MEDIUM);
  set_encounter_terrain_all_underdark(ENCOUNTER_TYPE_DIRE_CORBY);
  add_encounter_sector(ENCOUNTER_TYPE_DIRE_CORBY, SECT_CAVE);

  add_encounter_record(ENCOUNTER_TYPE_ELK, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_ELK, "elk", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, NON_SENTIENT, SIZE_MEDIUM);
  add_encounter_sector(ENCOUNTER_TYPE_ELK, SECT_TUNDRA);
  add_encounter_sector(ENCOUNTER_TYPE_ELK, SECT_TAIGA);

  add_encounter_record(ENCOUNTER_TYPE_GIANT_FROG, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_GIANT_FROG, "giant frog", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, NON_SENTIENT, SIZE_MEDIUM);
  add_encounter_sector(ENCOUNTER_TYPE_GIANT_FROG, SECT_WATER_SWIM);
  add_encounter_sector(ENCOUNTER_TYPE_GIANT_FROG, SECT_MARSHLAND);

  add_encounter_record(ENCOUNTER_TYPE_GHOUL, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_GHOUL, "ghoul", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_EVIL, RACE_TYPE_UNDEAD,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_MEDIUM);
  set_encounter_terrain_all_surface(ENCOUNTER_TYPE_GHOUL);
  set_encounter_terrain_all_underdark(ENCOUNTER_TYPE_GHOUL);

  add_encounter_record(ENCOUNTER_TYPE_GNOLL, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_GNOLL, "gnoll", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_EVIL, RACE_TYPE_MONSTROUS_HUMANOID,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_MEDIUM);
  add_encounter_sector(ENCOUNTER_TYPE_GNOLL, SECT_FIELD);
  add_encounter_sector(ENCOUNTER_TYPE_GNOLL, SECT_FOREST);
  add_encounter_sector(ENCOUNTER_TYPE_GNOLL, SECT_HILLS);
  add_encounter_sector(ENCOUNTER_TYPE_GNOLL, SECT_DESERT);

  add_encounter_record(ENCOUNTER_TYPE_HIPPOCAMPUS, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_HIPPOCAMPUS, "hippocampus", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_MAGICAL_BEAST,
                       SUBRACE_AQUATIC, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, NON_SENTIENT, SIZE_LARGE);
  set_encounter_terrain_all_water(ENCOUNTER_TYPE_HIPPOCAMPUS);

  add_encounter_record(ENCOUNTER_TYPE_HYENA, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_HYENA, "hyena", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_MEDIUM);
  add_encounter_sector(ENCOUNTER_TYPE_HYENA, SECT_FIELD);

  add_encounter_record(ENCOUNTER_TYPE_LIZARDFOLK, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_LIZARDFOLK, "lizardfolk", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_HUMANOID,
                       SUBRACE_REPTILIAN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_MEDIUM);
  add_encounter_sector(ENCOUNTER_TYPE_LIZARDFOLK, SECT_MARSHLAND);
  set_encounter_description(ENCOUNTER_TYPE_LIZARDFOLK, "This reptilian humanoid has green scales, a short and toothy snout, and a thick alligator-like tail.");

  add_encounter_record(ENCOUNTER_TYPE_GIANT_GECKO_LIZARD, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_GIANT_GECKO_LIZARD, "giant gecko lizard", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_REPTILIAN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, NON_SENTIENT, SIZE_MEDIUM);
  add_encounter_sector(ENCOUNTER_TYPE_GIANT_GECKO_LIZARD, SECT_FOREST);
  add_encounter_sector(ENCOUNTER_TYPE_GIANT_GECKO_LIZARD, SECT_HILLS);
  add_encounter_sector(ENCOUNTER_TYPE_GIANT_GECKO_LIZARD, SECT_MOUNTAIN);
  add_encounter_sector(ENCOUNTER_TYPE_GIANT_GECKO_LIZARD, SECT_JUNGLE);
  set_encounter_description(ENCOUNTER_TYPE_GIANT_GECKO_LIZARD, "With large bulging eyes to spot prey from afar, this oversized, smooth-scaled lizard has splayed, padded feet and a toothy maw.");

  add_encounter_record(ENCOUNTER_TYPE_MONGRELMAN, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_MONGRELMAN, "mongrelman", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, LAWFUL_NEUTRAL, RACE_TYPE_MONSTROUS_HUMANOID,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SENTIENT, SIZE_MEDIUM);
  set_encounter_description(ENCOUNTER_TYPE_MONGRELMAN, "Ivory tusks, insect chitin, matted fur, scaly flesh, and more combine to form a hideous humanoid shape.");
  set_encounter_terrain_all_underdark(ENCOUNTER_TYPE_MONGRELMAN);
  add_encounter_sector(ENCOUNTER_TYPE_MONGRELMAN, SECT_CAVE);

  add_encounter_record(ENCOUNTER_TYPE_NINGYO, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_NINGYO, "ningyo", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, NEUTRAL_EVIL, RACE_TYPE_MONSTROUS_HUMANOID,
                       SUBRACE_AQUATIC, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_SMALL);
  set_encounter_description(ENCOUNTER_TYPE_NINGYO, "This hideous sea monstrosity combines the most ferocious features of simian and carp, its fish-like tail sprouting a grotesquely primitive humanoid torso, head, and limbs. Although little more than 2 feet long, the nasty thing gibbers wildly as it gnashes its curling fangs and swipes at prey with webbed claws.");
  add_encounter_sector(ENCOUNTER_TYPE_NINGYO, SECT_OCEAN);

  add_encounter_record(ENCOUNTER_TYPE_NIXIE, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_NIXIE, "nixie", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_FEY,
                       SUBRACE_AQUATIC, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SENTIENT, SIZE_SMALL);
  set_encounter_description(ENCOUNTER_TYPE_NIXIE, "This green-skinned fey has webbed hands and feet. Its hair is the color of seaweed, and is decorated with shells.");
  set_encounter_long_description(ENCOUNTER_TYPE_NIXIE, "A green-skinned nixie darts around in front of you.");
  add_encounter_sector(ENCOUNTER_TYPE_NIXIE, SECT_WATER_SWIM);
  add_encounter_sector(ENCOUNTER_TYPE_NIXIE, SECT_WATER_NOSWIM);
  add_encounter_sector(ENCOUNTER_TYPE_NIXIE, SECT_UNDERWATER);
  add_encounter_sector(ENCOUNTER_TYPE_NIXIE, SECT_OCEAN);
  add_encounter_sector(ENCOUNTER_TYPE_NIXIE, SECT_BEACH);

  add_encounter_record(ENCOUNTER_TYPE_PSEUDODRAGON, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_PSEUDODRAGON, "pseudodragon", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_SORCERER, ENCOUNTER_STRENGTH_NORMAL, NEUTRAL_GOOD, RACE_TYPE_DRAGON,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SENTIENT, SIZE_TINY);
  set_encounter_description(ENCOUNTER_TYPE_PSEUDODRAGON, "This housecat-sized miniature dragon has fine scales, sharp horns, wicked little teeth, and a tail tipped with a barbed stinger.");
  set_encounter_long_description(ENCOUNTER_TYPE_PSEUDODRAGON, "A miniature, cat-sized dragon whips around the air before you.");
  add_encounter_sector(ENCOUNTER_TYPE_PSEUDODRAGON, SECT_FOREST);
  add_encounter_sector(ENCOUNTER_TYPE_PSEUDODRAGON, SECT_JUNGLE);

  add_encounter_record(ENCOUNTER_TYPE_RAM, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_RAM, "ram", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, NON_SENTIENT, SIZE_MEDIUM);
  set_encounter_description(ENCOUNTER_TYPE_RAM, "A formidable pair of heavy horns curls from the forehead of this sturdy, brown-and-white-furred ram.");
  set_encounter_long_description(ENCOUNTER_TYPE_RAM, "A large, brown and white furred ram paces the ground.");
  add_encounter_sector(ENCOUNTER_TYPE_RAM, SECT_HILLS);
  add_encounter_sector(ENCOUNTER_TYPE_RAM, SECT_MOUNTAIN);

  add_encounter_record(ENCOUNTER_TYPE_MANTA_RAY, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_MANTA_RAY, "manta ray", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_AQUATIC, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, NON_SENTIENT, SIZE_LARGE);
  set_encounter_description(ENCOUNTER_TYPE_MANTA_RAY, "Gliding gracefully through the water on wing-like fins, this large ray scoops up tiny morsels in its wide mouth.");
  set_encounter_long_description(ENCOUNTER_TYPE_MANTA_RAY, "A dark grey manta ray glides through the water.");
  add_encounter_sector(ENCOUNTER_TYPE_MANTA_RAY, SECT_WATER_SWIM);
  add_encounter_sector(ENCOUNTER_TYPE_MANTA_RAY, SECT_WATER_NOSWIM);
  add_encounter_sector(ENCOUNTER_TYPE_MANTA_RAY, SECT_UNDERWATER);
  add_encounter_sector(ENCOUNTER_TYPE_MANTA_RAY, SECT_OCEAN);

  add_encounter_record(ENCOUNTER_TYPE_REEFCLAW, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_REEFCLAW, "reefclaw", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ABERRATION,
                       SUBRACE_AQUATIC, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_SMALL);
  set_encounter_description(ENCOUNTER_TYPE_REEFCLAW, "Blood-red spines run the length of this frightening creature, which resembles a lobster in the front and an eel in the back. ");
  set_encounter_long_description(ENCOUNTER_TYPE_REEFCLAW, "A reefclaw scuttles before you, half lobster, half eel.");
  set_encounter_terrain_all_water(ENCOUNTER_TYPE_REEFCLAW);

  add_encounter_record(ENCOUNTER_TYPE_GIANT_SPIDER, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_GIANT_SPIDER, "giant spider", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_VERMIN,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_MEDIUM);
  set_encounter_description(ENCOUNTER_TYPE_GIANT_SPIDER, "A spider the size of a man crawls silently from the depths of its funnel-shaped web.");
  set_encounter_long_description(ENCOUNTER_TYPE_GIANT_SPIDER, "A large, man-sized spider creeps menacingly  before you.");
  set_encounter_terrain_all_surface(ENCOUNTER_TYPE_GIANT_SPIDER);
  set_encounter_terrain_all_underdark(ENCOUNTER_TYPE_GIANT_SPIDER);

  add_encounter_record(ENCOUNTER_TYPE_SQUID, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_SQUID, "squid", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_AQUATIC, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, NON_SENTIENT, SIZE_MEDIUM);
  set_encounter_description(ENCOUNTER_TYPE_SQUID, "This slender red squid darts through the water with alacrity. Two large eyes stare from above the creature’s tentacles.");
  set_encounter_long_description(ENCOUNTER_TYPE_SQUID, "A large, red squid dangles its long tentacles in the water before you.");
  set_encounter_terrain_all_water(ENCOUNTER_TYPE_SQUID);

  add_encounter_record(ENCOUNTER_TYPE_STRIX, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_STRIX, "strix", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_HUMANOID,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_MEDIUM);
  set_encounter_description(ENCOUNTER_TYPE_STRIX, "Monstrous black-feathered wings cloak this leanly muscled, onyx-skinned humanoid.");
  set_encounter_long_description(ENCOUNTER_TYPE_STRIX, "A black-skinned strix flexes its large, feathered wings.");
  add_encounter_sector(ENCOUNTER_TYPE_STRIX, SECT_HILLS);
  add_encounter_sector(ENCOUNTER_TYPE_STRIX, SECT_MOUNTAIN);

  add_encounter_record(ENCOUNTER_TYPE_TROGLODYTE, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_TROGLODYTE, "troglodyte", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_EVIL, RACE_TYPE_HUMANOID,
                       SUBRACE_REPTILIAN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_MEDIUM);
  set_encounter_description(ENCOUNTER_TYPE_TROGLODYTE, "This humanoid’s scaly hide is dull gray. Its frame resembles that of a cave lizard, with a long tail and crests on its head and back.");
  set_encounter_long_description(ENCOUNTER_TYPE_TROGLODYTE, "A putrid troglodyte whips its tail back and forth as it claws the air before you.");
  add_encounter_sector(ENCOUNTER_TYPE_TROGLODYTE, SECT_HILLS);
  add_encounter_sector(ENCOUNTER_TYPE_TROGLODYTE, SECT_MOUNTAIN);
  add_encounter_sector(ENCOUNTER_TYPE_TROGLODYTE, SECT_MARSHLAND);
  add_encounter_sector(ENCOUNTER_TYPE_TROGLODYTE, SECT_CAVE);

  add_encounter_record(ENCOUNTER_TYPE_WOLF, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_WOLF, "wolf", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_MEDIUM);
  set_encounter_description(ENCOUNTER_TYPE_WOLF, "This powerful canine watches its prey with piercing yellow eyes, darting its tongue across sharp white teeth.");
  set_encounter_long_description(ENCOUNTER_TYPE_WOLF, "A large, grey-furred wolf snarls menacingly before you.");
  add_encounter_sector(ENCOUNTER_TYPE_WOLF, SECT_FOREST);
  add_encounter_sector(ENCOUNTER_TYPE_WOLF, SECT_HILLS);
  add_encounter_sector(ENCOUNTER_TYPE_WOLF, SECT_TUNDRA);
  add_encounter_sector(ENCOUNTER_TYPE_WOLF, SECT_TAIGA);

  add_encounter_record(ENCOUNTER_TYPE_HALF_OGRE, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_HALF_OGRE, "half ogre", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_EVIL, RACE_TYPE_GIANT,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_MEDIUM);
  set_encounter_description(ENCOUNTER_TYPE_HALF_OGRE, "This being resembles a somewhat ugly human with dark toned skin and matted dark hair. It wears tattered skins over a suit of hide armor.");
  set_encounter_long_description(ENCOUNTER_TYPE_HALF_OGRE, "A large half ogre is here before you with a snarling grin on its face.");
  add_encounter_sector(ENCOUNTER_TYPE_HALF_OGRE, SECT_HILLS);
  add_encounter_sector(ENCOUNTER_TYPE_HALF_OGRE, SECT_MOUNTAIN);

  add_encounter_record(ENCOUNTER_TYPE_MANDRAGORA, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_MANDRAGORA, "mandragora", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, NEUTRAL_EVIL, RACE_TYPE_PLANT,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_SMALL);
  set_encounter_description(ENCOUNTER_TYPE_MANDRAGORA, "This small vaguely humanoid-looking plant creature is mottled green and brown. Its lower roots are splayed and resemble legs and feet. Its upper roots are long and resemble humanoid arms. Its head, if it could be called that, is a mass of solid vegetable matter covered in lumps. ");
  set_encounter_long_description(ENCOUNTER_TYPE_MANDRAGORA, "A plant-like mandragora shambles around.");
  add_encounter_sector(ENCOUNTER_TYPE_MANDRAGORA, SECT_FIELD);
  add_encounter_sector(ENCOUNTER_TYPE_MANDRAGORA, SECT_FOREST);

#if !defined(CAMPAIGN_DL)
  add_encounter_record(ENCOUNTER_TYPE_OGRILLON, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_OGRILLON, "ogrillon", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_EVIL, RACE_TYPE_GIANT,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_MEDIUM);
  set_encounter_description(ENCOUNTER_TYPE_OGRILLON, "This ugly brute appears to be a mix of ogre and perhaps orc. Its skin is covered in closely fitting bony plates and nodes akin to an alligator. Its hair is greasy, ragged, and generally unkempt. It exudes a strong sour odor from its body. ");
  set_encounter_long_description(ENCOUNTER_TYPE_OGRILLON, "A towering ogrillon, half-orc, half-ogre, grins wickedly at you.");
  set_encounter_terrain_all_surface(ENCOUNTER_TYPE_OGRILLON);
  set_encounter_terrain_all_underdark(ENCOUNTER_TYPE_OGRILLON);
#endif

  add_encounter_record(ENCOUNTER_TYPE_OROG, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_OROG, "orog", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, LAWFUL_EVIL, RACE_TYPE_HUMANOID,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_MEDIUM);
  set_encounter_description(ENCOUNTER_TYPE_OROG, "This creature looks like a gray-skinned stocky humanoid with coarse dark hair and dark eyes. Small upward curving tusks jut from its lower jaw. ");
  set_encounter_long_description(ENCOUNTER_TYPE_OROG, "A fearsome orog grins as he hefts a large, wicked blade in his hands.");
  add_encounter_sector(ENCOUNTER_TYPE_OROG, SECT_HILLS);
  add_encounter_sector(ENCOUNTER_TYPE_OROG, SECT_MOUNTAIN);

  // CR 2

  add_encounter_record(ENCOUNTER_TYPE_GIANT_ANT, ENCOUNTER_CLASS_COMBAT, 6, 18, ENCOUNTER_GROUP_TYPE_GIANT_ANT, "giant ant", 100, 1, 6,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_VERMIN,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, NON_SENTIENT, SIZE_MEDIUM);
  set_encounter_description(ENCOUNTER_TYPE_GIANT_ANT, "A thin, six-legged ant the size of a pony stands at the ready, its mandibles chittering and its stinger dripping with venom.");
  set_encounter_long_description(ENCOUNTER_TYPE_GIANT_ANT, "A giant human-sized ant clacks its mandibles menacingly.");
  set_encounter_terrain_all_surface(ENCOUNTER_TYPE_GIANT_ANT);
  set_encounter_terrain_all_underdark(ENCOUNTER_TYPE_GIANT_ANT);

  add_encounter_record(ENCOUNTER_TYPE_AXE_BEAK, ENCOUNTER_CLASS_COMBAT, 6, 18, ENCOUNTER_GROUP_TYPE_AXE_BEAK, "axe beak", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_LARGE);
  set_encounter_description(ENCOUNTER_TYPE_AXE_BEAK, "This stout flightless bird stands upon two long, taloned legs, but it is its axe-shaped beak that looks the most ferocious.");
  set_encounter_long_description(ENCOUNTER_TYPE_AXE_BEAK, "A large axe beak digs its talons into the ground in front of you.");
  add_encounter_sector(ENCOUNTER_TYPE_AXE_BEAK, SECT_FIELD);
  add_encounter_sector(ENCOUNTER_TYPE_AXE_BEAK, SECT_BEACH);

  add_encounter_record(ENCOUNTER_TYPE_DIRE_BADGER, ENCOUNTER_CLASS_COMBAT, 5, 18, ENCOUNTER_GROUP_TYPE_DIRE_BADGER, "dire badger", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_MEDIUM);
  set_encounter_description(ENCOUNTER_TYPE_DIRE_BADGER, "A tremendous badger snarls and scrapes its wicked, shovel-like claws. Stocky muscles ripple beneath its streaked and shaggy fur.");
  set_encounter_long_description(ENCOUNTER_TYPE_DIRE_BADGER, "A huge badger is here, snarling dangerously.");
  add_encounter_sector(ENCOUNTER_TYPE_DIRE_BADGER, SECT_FOREST);

  add_encounter_record(ENCOUNTER_TYPE_DIRE_BAT, ENCOUNTER_CLASS_COMBAT, 5, 18, ENCOUNTER_GROUP_TYPE_DIRE_BAT, "dire bat", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_LARGE);
  set_encounter_description(ENCOUNTER_TYPE_DIRE_BAT, "This giant, furry bat is nearly the size of an ox, with dark leathery wings that open wider than two men with arms outstretched.");
  set_encounter_long_description(ENCOUNTER_TYPE_DIRE_BAT, "A massivel-sized bat flies in the air around you.");
  add_encounter_sector(ENCOUNTER_TYPE_DIRE_BAT, SECT_FOREST);
  add_encounter_sector(ENCOUNTER_TYPE_DIRE_BAT, SECT_HILLS);
  add_encounter_sector(ENCOUNTER_TYPE_DIRE_BAT, SECT_FLYING);
  add_encounter_sector(ENCOUNTER_TYPE_DIRE_BAT, SECT_MARSHLAND);
  add_encounter_sector(ENCOUNTER_TYPE_DIRE_BAT, SECT_JUNGLE);

  add_encounter_record(ENCOUNTER_TYPE_BLINK_DOG, ENCOUNTER_CLASS_COMBAT, 5, 20, ENCOUNTER_GROUP_TYPE_BLINK_DOG, "blink dog", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, LAWFUL_GOOD, RACE_TYPE_MONSTROUS_HUMANOID,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SENTIENT, SIZE_MEDIUM);
  set_encounter_description(ENCOUNTER_TYPE_BLINK_DOG, "This sleek canine has a coarse, tawny coat, pointed ears, and pale eyes. A faint blue nimbus seems to dance upon its fur.");
  set_encounter_long_description(ENCOUNTER_TYPE_BLINK_DOG, "Phasing in-and-out, a sleek blink dog watches you closely.");
  add_encounter_sector(ENCOUNTER_TYPE_BLINK_DOG, SECT_FIELD);
  add_encounter_sector(ENCOUNTER_TYPE_BLINK_DOG, SECT_FOREST);

  add_encounter_record(ENCOUNTER_TYPE_BOAR, ENCOUNTER_CLASS_COMBAT, 5, 18, ENCOUNTER_GROUP_TYPE_BOAR, "boar", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_MEDIUM);
  set_encounter_description(ENCOUNTER_TYPE_BOAR, "This ill-tempered beast’s tiny, bloodshot eyes glare angrily above a mouth filled with sharp tusks.");
  set_encounter_long_description(ENCOUNTER_TYPE_BOAR, "A bad-tempered boar snorts angrily before you.");
  add_encounter_sector(ENCOUNTER_TYPE_BOAR, SECT_FOREST);
  add_encounter_sector(ENCOUNTER_TYPE_BOAR, SECT_JUNGLE);

  add_encounter_record(ENCOUNTER_TYPE_BOGGARD, ENCOUNTER_CLASS_COMBAT, 5, 18, ENCOUNTER_GROUP_TYPE_BOGGARD, "boggard", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_EVIL, RACE_TYPE_HUMANOID,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_MEDIUM);
  set_encounter_description(ENCOUNTER_TYPE_BOGGARD, "Bulbous eyes glare atop this creature’s decidedly toad-like head. A multitude of warts and bumps decorate its greenish skin.");
  set_encounter_long_description(ENCOUNTER_TYPE_BOGGARD, "A hideous boggard looks at you with death in its eyes.");
  add_encounter_sector(ENCOUNTER_TYPE_BOGGARD, SECT_MARSHLAND);

  add_encounter_record(ENCOUNTER_TYPE_BOG_STRIDER, ENCOUNTER_CLASS_COMBAT, 5, 18, ENCOUNTER_GROUP_TYPE_BOG_STRIDER, "bog strider", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_MONSTROUS_HUMANOID,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_MEDIUM);
  set_encounter_description(ENCOUNTER_TYPE_BOG_STRIDER, "A narrow, beetle-like creature glides across the water’s dark surface on four brown, spindly legs. It stands just over five feet tall, holding its head and thorax upright while clutching an intricately carved hunting spear in two clawed forelimbs. Powerful mandibles click in rhythm with the reed-thin antennae waving upon its head as if testing the air for the scent of prey.");
  set_encounter_long_description(ENCOUNTER_TYPE_BOG_STRIDER, "A large, spindly bog strider glides through the marshland.");
  add_encounter_sector(ENCOUNTER_TYPE_BOG_STRIDER, SECT_MARSHLAND);

  add_encounter_record(ENCOUNTER_TYPE_BUGBEAR, ENCOUNTER_CLASS_COMBAT, 5, 18, ENCOUNTER_GROUP_TYPE_BUGBEAR, "bugbear", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_EVIL, RACE_TYPE_HUMANOID,
                       SUBRACE_GOBLINOID, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_MEDIUM);
  set_encounter_description(ENCOUNTER_TYPE_BUGBEAR, "This ugly creature resembles a twisted shadow puppet’s silhouette, a wild thing of flared black and brown fur whose pelt juts out from its body at freakish angles. The crouching shadow’s ears are large and floppy, draping shoulder-length, adding to the creature’s unnatural shape. Its eyes are too big for its head. Tremendous milk-white ovals loom on either side of the thing’s wheezy pig-like nose. Its panting mouth is filled with bristly needle-teeth spider-webbed in disgusting strands of yellowish saliva, all vibrating to the tune of its wheezing breath.");
  set_encounter_long_description(ENCOUNTER_TYPE_BUGBEAR, "An ugly and fearsome bugbear grins wickedly.");
  add_encounter_sector(ENCOUNTER_TYPE_BUGBEAR, SECT_HILLS);
  add_encounter_sector(ENCOUNTER_TYPE_BUGBEAR, SECT_MOUNTAIN);

  add_encounter_record(ENCOUNTER_TYPE_CROCODILE, ENCOUNTER_CLASS_COMBAT, 5, 18, ENCOUNTER_GROUP_TYPE_CROCODILE, "crocodile", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_REPTILIAN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_LARGE);
  set_encounter_description(ENCOUNTER_TYPE_CROCODILE, "This reptile lunges out of the placid water with shocking speed. Its jaw gapes open in a roar, its powerful tail lashing behind.");
  set_encounter_long_description(ENCOUNTER_TYPE_CROCODILE, "A long, spiny crocodile swims through the water toward you.");
  add_encounter_sector(ENCOUNTER_TYPE_CROCODILE, SECT_WATER_SWIM);
  add_encounter_sector(ENCOUNTER_TYPE_CROCODILE, SECT_MARSHLAND);

  add_encounter_record(ENCOUNTER_TYPE_FAERIE_DRAGON, ENCOUNTER_CLASS_COMBAT, 5, 18, ENCOUNTER_GROUP_TYPE_FAERIE_DRAGON, "faerie dragon", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WIZARD, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_GOOD, RACE_TYPE_DRAGON,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SENTIENT, SIZE_TINY);
  set_encounter_description(ENCOUNTER_TYPE_FAERIE_DRAGON, "A pair of brightly colored butterfly wings sprouts from the back of this miniature dragon.");
  set_encounter_long_description(ENCOUNTER_TYPE_FAERIE_DRAGON, "A whimsical faerie dragon floats in the air before you.");
  add_encounter_sector(ENCOUNTER_TYPE_FAERIE_DRAGON, SECT_FOREST);
  add_encounter_sector(ENCOUNTER_TYPE_FAERIE_DRAGON, SECT_JUNGLE);

  add_encounter_record(ENCOUNTER_TYPE_DRAUGR, ENCOUNTER_CLASS_COMBAT, 5, 18, ENCOUNTER_GROUP_TYPE_DRAUGR, "draugr", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_EVIL, RACE_TYPE_UNDEAD,
                       SUBRACE_WATER, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_MEDIUM);
  set_encounter_description(ENCOUNTER_TYPE_DRAUGR, "This barnacle-encrusted walking corpse looks like a zombie, but is dripping with water and gives off a nauseating stench.");
  set_encounter_long_description(ENCOUNTER_TYPE_DRAUGR, "A barnicle covered draugr shambles toward you.");
  add_encounter_sector(ENCOUNTER_TYPE_DRAUGR, SECT_BEACH);

  add_encounter_record(ENCOUNTER_TYPE_GORILLA, ENCOUNTER_CLASS_COMBAT, 5, 18, ENCOUNTER_GROUP_TYPE_GORILLA, "gorilla", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_LARGE);
  set_encounter_description(ENCOUNTER_TYPE_GORILLA, "Large, deep-set eyes peer from beneath this great ape’s thick brow as it lumbers forward on its legs and knuckles.");
  set_encounter_long_description(ENCOUNTER_TYPE_GORILLA, "A large, black furred gorrila pounds his chest at you.");
  add_encounter_sector(ENCOUNTER_TYPE_GORILLA, SECT_JUNGLE);

  add_encounter_record(ENCOUNTER_TYPE_HIPPOGRIFF, ENCOUNTER_CLASS_COMBAT, 8, 20, ENCOUNTER_GROUP_TYPE_HIPPOGRIFF, "hippogriff", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_MAGICAL_BEAST,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_LARGE);
  set_encounter_description(ENCOUNTER_TYPE_HIPPOGRIFF, "This large, brown, horse-like creature has a hawk’s wings, talons, and hooked beak.");
  set_encounter_long_description(ENCOUNTER_TYPE_HIPPOGRIFF, "A large, half-horse, half-hawk, hippogriff claws at the ground with its talons.");
  add_encounter_sector(ENCOUNTER_TYPE_HIPPOGRIFF, SECT_FIELD);
  add_encounter_sector(ENCOUNTER_TYPE_HIPPOGRIFF, SECT_HILLS);

  // CR 3

  add_encounter_record(ENCOUNTER_TYPE_ALLIP, ENCOUNTER_CLASS_COMBAT, 10, 20, ENCOUNTER_GROUP_TYPE_ALLIP, "allip", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_BARD, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_EVIL, RACE_TYPE_UNDEAD,
                       SUBRACE_INCORPOREAL, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_MEDIUM);
  set_encounter_description(ENCOUNTER_TYPE_ALLIP, "This malignant cloud of shadows boils in the air, its skeletal maw eerily babbling as the creature’s claws manifest from the darkness.");
  set_encounter_long_description(ENCOUNTER_TYPE_ALLIP, "A black, humanoid cloud, an allip, hovers in the air with evil in its red eyes.");
  set_encounter_terrain_any(ENCOUNTER_TYPE_ALLIP);

  add_encounter_record(ENCOUNTER_TYPE_ANKHEG, ENCOUNTER_CLASS_COMBAT, 10, 20, ENCOUNTER_GROUP_TYPE_ANKHEG, "ankheg", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_MAGICAL_BEAST,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_LARGE);
  set_encounter_description(ENCOUNTER_TYPE_ANKHEG, "This burrowing, bug-like monster scuttles about on six legs, drooling noxious green ichor from its clacking mandibles.");
  set_encounter_long_description(ENCOUNTER_TYPE_ANKHEG, "A large, insect-like ankheg clacks it\'s acid-lined mandibles");
  set_encounter_terrain_any(ENCOUNTER_TYPE_ANKHEG);

  add_encounter_record(ENCOUNTER_TYPE_CENTAUR, ENCOUNTER_CLASS_COMBAT, 10, 20, ENCOUNTER_GROUP_TYPE_CENTAUR, "centaur", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_RANGER, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_MONSTROUS_HUMANOID,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SENTIENT, SIZE_LARGE);
  set_encounter_description(ENCOUNTER_TYPE_CENTAUR, "This creature has the sun-bronzed upper body of a seasoned warrior and the lower body of a sleek warhorse.");
  set_encounter_long_description(ENCOUNTER_TYPE_CENTAUR, "A brown-skinned centaur draws a bow at you, ready to fire if needed.");
  set_encounter_terrain_all_surface(ENCOUNTER_TYPE_CENTAUR);

  add_encounter_record(ENCOUNTER_TYPE_OGRE, ENCOUNTER_CLASS_COMBAT, 10, 20, ENCOUNTER_GROUP_TYPE_OGRE, "ogre", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_EVIL, RACE_TYPE_GIANT,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_LARGE);
  set_encounter_description(ENCOUNTER_TYPE_OGRE, "This creature’s python-thick apish arms and stumpy legs conspire to drag its dirty knuckles through the wet grass and mud. The stooped giant blinks its dim eyes and an excess of soupy drool spills over its bulbous lips. Its misshapen features resemble a man’s face rendered in watercolor, then distorted by a careless splash. It snarls as it charges, a sound the offspring of bear and man might make, showing flat black teeth well suited for grinding bones to paste.");
  set_encounter_long_description(ENCOUNTER_TYPE_OGRE, "A cruel-looking ogre grins a crude smile, broken yellow teeth dripping with sailva.");
  set_encounter_terrain_all_surface(ENCOUNTER_TYPE_OGRE);

  // CR 4

  add_encounter_record(ENCOUNTER_TYPE_GRIZZLY_BEAR, ENCOUNTER_CLASS_COMBAT, 12, 22, ENCOUNTER_GROUP_TYPE_GRIZZLY_BEAR, "grizzly bear", 100, 1, 4,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, NON_SENTIENT, SIZE_LARGE);
  set_encounter_description(ENCOUNTER_TYPE_GRIZZLY_BEAR, "Broad, powerful muscles move beneath this massive bear’s brown fur, promising both speed and lethal force.");
  set_encounter_long_description(ENCOUNTER_TYPE_GRIZZLY_BEAR, "A large grizzly bear lumbers toward you.");
  add_encounter_sector(ENCOUNTER_TYPE_GRIZZLY_BEAR, SECT_FIELD);
  add_encounter_sector(ENCOUNTER_TYPE_GRIZZLY_BEAR, SECT_FOREST);
  add_encounter_sector(ENCOUNTER_TYPE_GRIZZLY_BEAR, SECT_HILLS);
  add_encounter_sector(ENCOUNTER_TYPE_GRIZZLY_BEAR, SECT_TAIGA);

  add_encounter_record(ENCOUNTER_TYPE_GIANT_STAG_BEETLE, ENCOUNTER_CLASS_COMBAT, 12, 22, ENCOUNTER_GROUP_TYPE_GIANT_STAG_BEETLE, "giant stag beetle", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_VERMIN,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, NON_SENTIENT, SIZE_LARGE);
  set_encounter_description(ENCOUNTER_TYPE_GIANT_STAG_BEETLE, "Nearly 10 feet long, this giant stag beetle is so called due to its antler-like mandibles.");
  set_encounter_long_description(ENCOUNTER_TYPE_GIANT_STAG_BEETLE, "A giant stag beetle chitters nearby.");
  add_encounter_sector(ENCOUNTER_TYPE_GIANT_STAG_BEETLE, SECT_FIELD);
  add_encounter_sector(ENCOUNTER_TYPE_GIANT_STAG_BEETLE, SECT_FOREST);
  add_encounter_sector(ENCOUNTER_TYPE_GIANT_STAG_BEETLE, SECT_HILLS);

  add_encounter_record(ENCOUNTER_TYPE_BISON, ENCOUNTER_CLASS_COMBAT, 12, 22, ENCOUNTER_GROUP_TYPE_BISON, "bison", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, NON_SENTIENT, SIZE_LARGE);
  set_encounter_description(ENCOUNTER_TYPE_BISON, "This creature has small, upward-pointing horns, a shaggy coat of fur, and a large hump on its shoulders.");
  set_encounter_long_description(ENCOUNTER_TYPE_BISON, "A large bison snorts and stamps its hooves nearby.");
  add_encounter_sector(ENCOUNTER_TYPE_BISON, SECT_FIELD);
  add_encounter_sector(ENCOUNTER_TYPE_BISON, SECT_TUNDRA);

  add_encounter_record(ENCOUNTER_TYPE_DIRE_BOAR, ENCOUNTER_CLASS_COMBAT, 14, 24, ENCOUNTER_GROUP_TYPE_DIRE_BOAR, "dire boar", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_LARGE);
  set_encounter_description(ENCOUNTER_TYPE_DIRE_BOAR, "The back of this horse-sized boar rises in a steep slope. Its tiny red eyes are crusted with filth and its bristly flank crawls with flies.");
  set_encounter_long_description(ENCOUNTER_TYPE_DIRE_BOAR, "A dire boar is nearby snorting and pacing in circles.");
  add_encounter_sector(ENCOUNTER_TYPE_DIRE_BOAR, SECT_FIELD);
  add_encounter_sector(ENCOUNTER_TYPE_DIRE_BOAR, SECT_FOREST);
  add_encounter_sector(ENCOUNTER_TYPE_DIRE_BOAR, SECT_HILLS);

  add_encounter_record(ENCOUNTER_TYPE_BOTFLY_SWARM, ENCOUNTER_CLASS_COMBAT, 12, 22, ENCOUNTER_GROUP_TYPE_BOTFLY_SWARM, "botfly swarm", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_VERMIN,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_FINE);
  set_encounter_description(ENCOUNTER_TYPE_BOTFLY_SWARM, "Like a cloud of black dust, a swirling swarm of insects hovers in the air. From within comes the low, droning buzz of thousands of tiny flies.");
  set_encounter_long_description(ENCOUNTER_TYPE_BOTFLY_SWARM, "A swarm of botflies buzzes nearby.");
  add_encounter_sector(ENCOUNTER_TYPE_BOTFLY_SWARM, SECT_MARSHLAND);
  add_encounter_sector(ENCOUNTER_TYPE_BOTFLY_SWARM, SECT_JUNGLE);

  add_encounter_record(ENCOUNTER_TYPE_GIANT_DRAGONFLY, ENCOUNTER_CLASS_COMBAT, 12, 22, ENCOUNTER_GROUP_TYPE_GIANT_DRAGONFLY, "giant dragonfly", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_VERMIN,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, NON_SENTIENT, SIZE_MEDIUM);
  set_encounter_description(ENCOUNTER_TYPE_GIANT_DRAGONFLY, "This glittering blue dragonfly is about the size of a horse and is large enough to carry off small farm animals or people.");
  set_encounter_long_description(ENCOUNTER_TYPE_GIANT_DRAGONFLY, "A giant dragonfly hovvers in the air nearby.");
  add_encounter_sector(ENCOUNTER_TYPE_GIANT_DRAGONFLY, SECT_FIELD);
  add_encounter_sector(ENCOUNTER_TYPE_GIANT_DRAGONFLY, SECT_FOREST);
  add_encounter_sector(ENCOUNTER_TYPE_GIANT_DRAGONFLY, SECT_HILLS);
  add_encounter_sector(ENCOUNTER_TYPE_GIANT_DRAGONFLY, SECT_MOUNTAIN);
  add_encounter_sector(ENCOUNTER_TYPE_GIANT_DRAGONFLY, SECT_MARSHLAND);
  add_encounter_sector(ENCOUNTER_TYPE_GIANT_DRAGONFLY, SECT_JUNGLE);
  add_encounter_sector(ENCOUNTER_TYPE_GIANT_DRAGONFLY, SECT_TUNDRA);
  add_encounter_sector(ENCOUNTER_TYPE_GIANT_DRAGONFLY, SECT_BEACH);

  add_encounter_record(ENCOUNTER_TYPE_FOREST_DRAKE, ENCOUNTER_CLASS_COMBAT, 14, 24, ENCOUNTER_GROUP_TYPE_FOREST_DRAKE, "forest drake", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_SORCERER, ENCOUNTER_STRENGTH_NORMAL, LAWFUL_EVIL, RACE_TYPE_DRAGON,
                       SUBRACE_EARTH, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_LARGE);
  set_encounter_description(ENCOUNTER_TYPE_FOREST_DRAKE, "This green-scaled dragon has two powerful legs and a pair of long, leathery wings. A long spike adorns its thrashing tail.");
  set_encounter_long_description(ENCOUNTER_TYPE_FOREST_DRAKE, "A green-scaled forest drake is nearby.");
  add_encounter_sector(ENCOUNTER_TYPE_FOREST_DRAKE, SECT_FOREST);
  add_encounter_sector(ENCOUNTER_TYPE_FOREST_DRAKE, SECT_TAIGA);

  add_encounter_record(ENCOUNTER_TYPE_DUST_DIGGER, ENCOUNTER_CLASS_COMBAT, 14, 24, ENCOUNTER_GROUP_TYPE_DUST_DIGGER, "dust digger", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ABERRATION,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_LARGE);
  set_encounter_description(ENCOUNTER_TYPE_DUST_DIGGER, "A tremendous starfish-like creature emerges from the sand, its five long arms surrounding a circular toothy maw.");
  set_encounter_long_description(ENCOUNTER_TYPE_DUST_DIGGER, "A starfish-like dust-digger emerges from the sand.");
  add_encounter_sector(ENCOUNTER_TYPE_DUST_DIGGER, SECT_DESERT);

  add_encounter_record(ENCOUNTER_TYPE_HYDRA, ENCOUNTER_CLASS_COMBAT, 20, 30, ENCOUNTER_GROUP_TYPE_HYDRA, "hydra", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_MAGICAL_BEAST,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_HUGE);
  set_encounter_description(ENCOUNTER_TYPE_HYDRA, "Multiple angry snake-like heads rise from the sleek, serpentine body of this terrifying monster.");
  set_encounter_long_description(ENCOUNTER_TYPE_HYDRA, "A large, multi-headed hydra snaps at you with its squirming, twisting serpentine heads.");
  set_encounter_terrain_all_surface(ENCOUNTER_TYPE_HYDRA);

  // CR 5

  add_encounter_record(ENCOUNTER_TYPE_TROLL, ENCOUNTER_CLASS_COMBAT, 15, 25, ENCOUNTER_GROUP_TYPE_TROLL, "troll", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_EVIL, RACE_TYPE_GIANT,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_LARGE);
  set_encounter_description(ENCOUNTER_TYPE_TROLL, "This tall creature has rough, green hide. Its hands end in claws, and its bestial face has a hideous, tusked underbite.");
  set_encounter_long_description(ENCOUNTER_TYPE_TROLL, "A green, leathery skinned troll grins at you hungrily.");
  set_encounter_terrain_all_surface(ENCOUNTER_TYPE_TROLL);

  add_encounter_record(ENCOUNTER_TYPE_CYCLOPS, ENCOUNTER_CLASS_COMBAT, 20, 30, ENCOUNTER_GROUP_TYPE_CYCLOPS, "cyclops", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, NEUTRAL_EVIL, RACE_TYPE_GIANT,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_LARGE);
  set_encounter_description(ENCOUNTER_TYPE_CYCLOPS, "A single huge eye stares from the forehead of this nine-foot-tall giant. Below this sole orb, an even larger mouth gapes like a cave.");
  set_encounter_long_description(ENCOUNTER_TYPE_CYCLOPS, "A one-eyed cyclops swings his club in the air menacingly.");
  set_encounter_terrain_all_surface(ENCOUNTER_TYPE_CYCLOPS);

  // CR 6

  add_encounter_record(ENCOUNTER_TYPE_ETTIN, ENCOUNTER_CLASS_COMBAT, 15, 25, ENCOUNTER_GROUP_TYPE_ETTIN, "ettin", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_EVIL, RACE_TYPE_GIANT,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_LARGE);
  set_encounter_description(ENCOUNTER_TYPE_ETTIN, "This lumbering, filthy, two-headed giant wears tattered remnants of leather armor and clutches a large flail in each fist.");
  set_encounter_long_description(ENCOUNTER_TYPE_ETTIN, "A towering, two-headed ettin utters a guttural, thundering laugh as it sizes you up.");
  set_encounter_terrain_all_surface(ENCOUNTER_TYPE_ETTIN);

  add_encounter_record(ENCOUNTER_TYPE_LAMIA, ENCOUNTER_CLASS_COMBAT, 15, 25, ENCOUNTER_GROUP_TYPE_LAMIA, "lamia", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WIZARD, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_EVIL, RACE_TYPE_MONSTROUS_HUMANOID,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_LARGE);
  set_encounter_description(ENCOUNTER_TYPE_LAMIA, "This creature’s upper torso is that of a comely woman with cat’s eyes and sharp fangs, while her lower body is that of a lion.");
  set_encounter_long_description(ENCOUNTER_TYPE_LAMIA, "An exotic-looking lamia grins slyly at you.");
  set_encounter_terrain_all_surface(ENCOUNTER_TYPE_LAMIA);

  add_encounter_record(ENCOUNTER_TYPE_MOTHMAN, ENCOUNTER_CLASS_COMBAT, 15, 25, ENCOUNTER_GROUP_TYPE_MOTHMAN, "mothman", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_NEUTRAL, RACE_TYPE_MONSTROUS_HUMANOID,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_MEDIUM);
  set_encounter_description(ENCOUNTER_TYPE_MOTHMAN, "A shroud of dark wings cloaks this thin, humanoid shape. Two monstrous red eyes glare malevolently from its narrow face.");
  set_encounter_long_description(ENCOUNTER_TYPE_MOTHMAN, "The red, gleaming eyes of a mothman glare at you with hatred.");
  set_encounter_terrain_all_surface(ENCOUNTER_TYPE_MOTHMAN);

  add_encounter_record(ENCOUNTER_TYPE_REVENANT, ENCOUNTER_CLASS_COMBAT, 15, 25, ENCOUNTER_GROUP_TYPE_REVENANT, "revenant", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, LAWFUL_EVIL, RACE_TYPE_UNDEAD,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_MEDIUM);
  set_encounter_description(ENCOUNTER_TYPE_REVENANT, "This shambling corpse is twisted and mutilated. Fingers of sharpened bone reach out with malevolent intent. ");
  set_encounter_long_description(ENCOUNTER_TYPE_REVENANT, "A revenant\'s mutilated corpse advances upon you malevolently.");
  set_encounter_terrain_all_surface(ENCOUNTER_TYPE_REVENANT);

  add_encounter_record(ENCOUNTER_TYPE_SHAMBLING_MOUND, ENCOUNTER_CLASS_COMBAT, 15, 25, ENCOUNTER_GROUP_TYPE_SHAMBLING_MOUND, "shambling mound", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_PLANT,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_LARGE);
  set_encounter_description(ENCOUNTER_TYPE_SHAMBLING_MOUND, "A mass of tangled vines and dripping slime rises on two trunk-like legs, reeking of rot and freshly turned earth.");
  set_encounter_long_description(ENCOUNTER_TYPE_SHAMBLING_MOUND, "A shambling mound shuffles loudly toward you.");
  set_encounter_terrain_all_surface(ENCOUNTER_TYPE_SHAMBLING_MOUND);

  add_encounter_record(ENCOUNTER_TYPE_WYVERN, ENCOUNTER_CLASS_COMBAT, 15, 25, ENCOUNTER_GROUP_TYPE_WYVERN, "wyvern", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_DRAGON,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_LARGE);
  set_encounter_description(ENCOUNTER_TYPE_WYVERN, "A dark blue dragon, its wings immense and its tail tipped with a hooked stinger, lands on two taloned feet and roars a challenge.");
  set_encounter_long_description(ENCOUNTER_TYPE_WYVERN, "A blue-scaled wyvern snaps its poison, barbed tail in your direction.");
  set_encounter_terrain_all_surface(ENCOUNTER_TYPE_WYVERN);

  // CR 7

  add_encounter_record(ENCOUNTER_TYPE_NAGA, ENCOUNTER_CLASS_COMBAT, 20, 30, ENCOUNTER_GROUP_TYPE_NAGA, "naga", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WIZARD, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ABERRATION,
                       SUBRACE_AQUATIC, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_LARGE);
  set_encounter_description(ENCOUNTER_TYPE_NAGA, "Slender spines and brightly colored frills stretch back from the human-like face of this massive water snake. Every motion of the serpent’s long form sets its brightly patterned scales and glistening fins to flashing like gems in the surf.");
  set_encounter_long_description(ENCOUNTER_TYPE_NAGA, "A slender, writhing naga flicks its barbed tongue at you wickedly.");
  set_encounter_terrain_all_surface(ENCOUNTER_TYPE_NAGA);
  set_encounter_terrain_all_water(ENCOUNTER_TYPE_NAGA);

  add_encounter_record(ENCOUNTER_TYPE_HILL_GIANT, ENCOUNTER_CLASS_COMBAT, 10, 20, ENCOUNTER_GROUP_TYPE_HILL_GIANT, "hill giant", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_EVIL, RACE_TYPE_GIANT,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_LARGE);
  set_encounter_description(ENCOUNTER_TYPE_HILL_GIANT, "This hunched giant exudes power and a crude, stupid anger, its filthy fur clothing bespeaking a brutish and backwoods lifestyle.");
  set_encounter_long_description(ENCOUNTER_TYPE_HILL_GIANT, "A huge hill giant towers menacingly here.");
  add_encounter_sector(ENCOUNTER_TYPE_HILL_GIANT, SECT_FIELD);
  add_encounter_sector(ENCOUNTER_TYPE_HILL_GIANT, SECT_HILLS);
  add_encounter_sector(ENCOUNTER_TYPE_HILL_GIANT, SECT_MOUNTAIN);
  add_encounter_sector(ENCOUNTER_TYPE_HILL_GIANT, SECT_CAVE);

  // CR 8

  add_encounter_record(ENCOUNTER_TYPE_STONE_GIANT, ENCOUNTER_CLASS_COMBAT, 20, 30, ENCOUNTER_GROUP_TYPE_STONE_GIANT, "stone giant", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_GIANT,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_LARGE);
  set_encounter_description(ENCOUNTER_TYPE_STONE_GIANT, "This giant has chiseled, muscular features and a flat, forward-sloping head, looking almost as if it were carved of stone.");
  set_encounter_long_description(ENCOUNTER_TYPE_STONE_GIANT, "A massive stone giant stomps toward you with hostility.");
  set_encounter_terrain_all_surface(ENCOUNTER_TYPE_STONE_GIANT);

  add_encounter_record(ENCOUNTER_TYPE_GORGON, ENCOUNTER_CLASS_COMBAT, 20, 30, ENCOUNTER_GROUP_TYPE_GORGON, "gorgon", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_MAGICAL_BEAST,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_LARGE);
  set_encounter_description(ENCOUNTER_TYPE_GORGON, "This bull-like creature seems to be made of interlocking metallic plates. Faint plumes of green smoke puff from its mouth.");
  set_encounter_long_description(ENCOUNTER_TYPE_GORGON, "A stone-like gorgon tramples the ground as it snorts and bucks before you.");
  set_encounter_terrain_all_surface(ENCOUNTER_TYPE_GORGON);

  add_encounter_record(ENCOUNTER_TYPE_OGRE_MAGE, ENCOUNTER_CLASS_COMBAT, 20, 30, ENCOUNTER_GROUP_TYPE_OGRE_MAGE, "ogre mage", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WIZARD, ENCOUNTER_STRENGTH_NORMAL, LAWFUL_EVIL, RACE_TYPE_GIANT,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_LARGE);
  set_encounter_description(ENCOUNTER_TYPE_OGRE_MAGE, "Clad in beautiful armor, this exotically garbed giant roars, its tusks glistening and its eyes afire with murderous intent.");
  set_encounter_long_description(ENCOUNTER_TYPE_OGRE_MAGE, "A large, blue-skinned ogre mage sneers at you wickedly.");
  set_encounter_terrain_all_surface(ENCOUNTER_TYPE_OGRE_MAGE);

  // CR 9

  add_encounter_record(ENCOUNTER_TYPE_VAMPIRE, ENCOUNTER_CLASS_COMBAT, 20, 30, ENCOUNTER_GROUP_TYPE_VAMPIRE, "vampire", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_EVIL, RACE_TYPE_UNDEAD,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_MEDIUM);
  set_encounter_description(ENCOUNTER_TYPE_VAMPIRE, "This alluring, raven-haired beauty casually wipes a trickle of blood from a pale cheek, then smiles to reveal needle-sharp fangs.");
  set_encounter_long_description(ENCOUNTER_TYPE_VAMPIRE, "A voluptuous, pale-skinned vampiress smiles at you wickedly.");
  set_encounter_terrain_all_surface(ENCOUNTER_TYPE_VAMPIRE);

  // CR 10

  add_encounter_record(ENCOUNTER_TYPE_COUATL, ENCOUNTER_CLASS_COMBAT, 20, 30, ENCOUNTER_GROUP_TYPE_COUATL, "couatl", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_SORCERER, ENCOUNTER_STRENGTH_NORMAL, LAWFUL_GOOD, RACE_TYPE_OUTSIDER,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, NON_SENTIENT, SIZE_LARGE);
  set_encounter_description(ENCOUNTER_TYPE_COUATL, "This great serpent has multicolored wings and eyes that glimmer with intense awareness.");
  set_encounter_long_description(ENCOUNTER_TYPE_COUATL, "Flapping its multi-colored wings to stay aloft, a couatl eyes you with interest.");
  set_encounter_terrain_all_surface(ENCOUNTER_TYPE_COUATL);

  add_encounter_record(ENCOUNTER_TYPE_GIANT_FLYTRAP, ENCOUNTER_CLASS_COMBAT, 20, 30, ENCOUNTER_GROUP_TYPE_GIANT_FLYTRAP, "giant flytrap", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_PLANT,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, NON_SENTIENT, SIZE_HUGE);
  set_encounter_description(ENCOUNTER_TYPE_GIANT_FLYTRAP, "This towering plant is a mass of vines and barbs. Several stalks are horribly mobile, each ending in a set of green, toothy jaws.");
  set_encounter_long_description(ENCOUNTER_TYPE_GIANT_FLYTRAP, "A giant flytrap lashes it\'s towering mass of vines and barbs in your direction.");
  set_encounter_terrain_all_surface(ENCOUNTER_TYPE_GIANT_FLYTRAP);

  add_encounter_record(ENCOUNTER_TYPE_RAKSHASA, ENCOUNTER_CLASS_COMBAT, 20, 30, ENCOUNTER_GROUP_TYPE_RAKSHASA, "rakshasa", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WIZARD, ENCOUNTER_STRENGTH_NORMAL, LAWFUL_EVIL, RACE_TYPE_OUTSIDER,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_MEDIUM);
  set_encounter_description(ENCOUNTER_TYPE_RAKSHASA, "This figure’s backward-bending fingers and its bestial, snarling visage leave little doubt as to its fiendish nature.");
  set_encounter_long_description(ENCOUNTER_TYPE_RAKSHASA, "A wicked rakshasa snarls at you as its fiendish claws curl back unnaturally.");
  set_encounter_terrain_all_surface(ENCOUNTER_TYPE_RAKSHASA);

  add_encounter_record(ENCOUNTER_TYPE_FIRE_GIANT, ENCOUNTER_CLASS_COMBAT, 20, 30, ENCOUNTER_GROUP_TYPE_FIRE_GIANT, "fire giant", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_BERSERKER, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_EVIL, RACE_TYPE_GIANT,
                       SUBRACE_FIRE, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SENTIENT, SIZE_LARGE);
  set_encounter_description(ENCOUNTER_TYPE_FIRE_GIANT, "This lumbering giant has short stumpy legs and powerful, muscular arms. Its hair and beard seem to be made of fire.");
  set_encounter_long_description(ENCOUNTER_TYPE_FIRE_GIANT, "A massive fire giant pounds a gigantic, pitch-black into the ground.");
  add_encounter_sector(ENCOUNTER_TYPE_FIRE_GIANT, SECT_FIELD);
  add_encounter_sector(ENCOUNTER_TYPE_FIRE_GIANT, SECT_HILLS);
  add_encounter_sector(ENCOUNTER_TYPE_FIRE_GIANT, SECT_LAVA);

  // CR 11

  add_encounter_record(ENCOUNTER_TYPE_STONE_GOLEM, ENCOUNTER_CLASS_COMBAT, 20, 30, ENCOUNTER_GROUP_TYPE_STONE_GOLEM, "stone golem", 100, 1, 5,
                       TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_CONSTRUCT,
                       SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, NON_SENTIENT, SIZE_LARGE);
  set_encounter_description(ENCOUNTER_TYPE_STONE_GOLEM, "This towering stone automaton bears the likeness of an archaic, armored warrior. It moves with ponderous but inexorable steps.");
  set_encounter_long_description(ENCOUNTER_TYPE_STONE_GOLEM, "A large stone golem stands impassively before you.");
  set_encounter_terrain_all_surface(ENCOUNTER_TYPE_STONE_GOLEM);
}

int encounter_chance(struct char_data *ch)
{

  if (IS_NPC(ch))
    return 0;

  // no encounters for newbies
  if (GET_LEVEL(ch) < 6)
    return 0;

  // out of 1,000.  Base chance of 3%
  int chance = 30;

  if (PRF_FLAGGED(ch, PRF_AVOID_ENCOUNTERS))
  {
    chance -= combat_skill_roll(ch, ABILITY_SURVIVAL) * 5;
  }
  else if (PRF_FLAGGED(ch, PRF_SEEK_ENCOUNTERS))
    chance += combat_skill_roll(ch, ABILITY_SURVIVAL) * 5;

  // send_to_char(ch, " Chance: %d\r\n", chance);

  return MAX(5, chance);
}

int get_exploration_dc(struct char_data *ch)
{
  if (!ch)
    return 9999;

  if (STATE(ch->desc) != CON_PLAYING)
    return 9999;

  int dc = ((int)GET_LEVEL(ch) / 1.5) + 15;

  return dc;
}

void check_random_encounter(struct char_data *ch)
{

  if (!ch || IN_ROOM(ch) == NOWHERE)
    return;

  if (in_encounter_room(ch))
    return;

  int i = 0, j = 0, count = 0, roll = 0, group_type = ENCOUNTER_GROUP_TYPE_NONE;
  int groups[NUM_ENCOUNTER_GROUP_TYPES];
#if !defined(CAMPAIGN_DL)
  int room_sect = world[IN_ROOM(ch)].sector_type;
#endif
  int highest_level = get_max_party_level_same_room(ch);
  // int avg_level = get_avg_party_level_same_room(ch);
  int party_size = get_party_size_same_room(ch);
  int num_mobs = 0, num_mobs_roll = 0;
  struct char_data *mob = NULL;
  char mob_descs[MEDIUM_STRING] = {'\0'};
  bool stealth_success = false;

  roll = dice(1, 1000);

  // send_to_char(ch, "Roll: %d, ", roll);

  if (roll > encounter_chance(ch))
    return;

  if (PRF_FLAGGED(ch, PRF_AVOID_ENCOUNTERS))
  {
    if (AFF_FLAGGED(ch, AFF_SNEAK) && skill_roll(ch, ABILITY_STEALTH) >= (GET_LEVEL(ch) + 5))
    {
      stealth_success = true;
    }
  }

  roll = 0;

  // initiatize
  for (i = 0; i < NUM_ENCOUNTER_GROUP_TYPES; i++)
    groups[i] = false;

  for (i = 0; i < NUM_ENCOUNTER_GROUP_TYPES; i++)
  {
    for (j = 0; j < NUM_ENCOUNTER_TYPES; j++)
      if (encounter_table[j].encounter_group == i)
      // Dragonlance encounters will load in any terrain, so we'll just skip this 'if statement' if campaign is DL
#if !defined(CAMPAIGN_DL)
        if (encounter_table[j].sector_types[room_sect] == true)
#endif
          if (highest_level <= encounter_table[j].max_level)
            groups[i] = true;
  }

  for (i = 0; i < NUM_ENCOUNTER_GROUP_TYPES; i++)
    if (groups[i] == true)
      count++;

  roll = dice(1, count);

  count = 0;

  for (i = 0; i < NUM_ENCOUNTER_GROUP_TYPES; i++)
  {
    if (groups[i] == true)
    {
      count++;
      if (count == roll)
      {
        group_type = i;
        break;
      }
    }
  }

  if (group_type == ENCOUNTER_GROUP_TYPE_NONE)
  {
    // send_to_char(ch, "Group type error\r\n");
    return;
  }

  for (j = 0; j < NUM_ENCOUNTER_TYPES; j++)
  {
    if (encounter_table[j].encounter_group == group_type)
    {
      num_mobs_roll = dice(encounter_table[j].min_number, encounter_table[j].max_number);
      for (i = 0; (i < num_mobs_roll) && (num_mobs < party_size); i++)
      {
        if (dice(1, 100) > encounter_table[j].load_chance)
          continue;
        mob = read_mobile(ENCOUNTER_MOB_VNUM, VIRTUAL);
        if (!mob)
        {
          // send_to_char(ch, "Mob load error.\r\n");
          return;
        }
        // set descriptions
        mob->player.name = strdup(encounter_table[j].object_name);
        sprintf(mob_descs, "%s %s", AN(encounter_table[j].object_name), encounter_table[j].object_name);
        mob->player.short_descr = strdup(mob_descs);
        if (!strcmp(encounter_table[j].long_description, "Nothing"))
        {
          sprintf(mob_descs, "%s %s is here.\r\n", AN(encounter_table[j].object_name), encounter_table[j].object_name);
          mob->player.long_descr = strdup(mob_descs);
        }
        else
        {
          sprintf(mob_descs, "%s\r\n", encounter_table[j].long_description);
          mob->player.long_descr = strdup(mob_descs);
        }
        if (!strcmp(encounter_table[j].description, "Nothing"))
        {
          sprintf(mob_descs, "%s %s is here before you.\r\n", AN(encounter_table[j].object_name), encounter_table[j].object_name);
          mob->player.description = strdup(mob_descs);
        }
        else
        {
          sprintf(mob_descs, "%s\r\n", encounter_table[j].description);
          mob->player.description = strdup(mob_descs);
        }
        // If we've stealthed successfully, we don't need to add further details
        if (!stealth_success)
        {
          // set mob details
          GET_REAL_RACE(mob) = encounter_table[j].race_type;
          GET_SUBRACE(mob, 0) - encounter_table[j].subrace[0];
          GET_SUBRACE(mob, 1) - encounter_table[j].subrace[1];
          GET_SUBRACE(mob, 2) - encounter_table[j].subrace[2];
          mob->mob_specials.hostile = encounter_table[j].hostile;
          if (mob->mob_specials.hostile)
            mob->mob_specials.aggro_timer = 5;
          mob->mob_specials.sentient = encounter_table[j].sentient;
          mob->mob_specials.extract_timer = -1;
          mob->mob_specials.peaceful_timer = -1;

          // set stats
          GET_CLASS(mob) = encounter_table[j].char_class;
          GET_LEVEL(mob) = MAX(1, highest_level - 2);
          autoroll_mob(mob, TRUE, FALSE);
          GET_EXP(mob) = (GET_LEVEL(mob) * GET_LEVEL(mob) * 75);
          GET_GOLD(mob) = (GET_LEVEL(mob) * 10);
          set_alignment(mob, encounter_table[j].alignment);
          // set flags
          SET_BIT_AR(MOB_FLAGS(mob), MOB_ENCOUNTER);
          SET_BIT_AR(MOB_FLAGS(mob), MOB_SENTINEL);
          SET_BIT_AR(MOB_FLAGS(mob), MOB_HELPER);

          X_LOC(mob) = world[IN_ROOM(ch)].coords[0];
          Y_LOC(mob) = world[IN_ROOM(ch)].coords[1];
          char_to_room(mob, IN_ROOM(ch));
        }
        if (encounter_table[j].hostile)
          send_to_room(IN_ROOM(ch), "\tY%s has ambushed you!\tn\r\n", CAP(mob->player.short_descr));
        else
          send_to_room(IN_ROOM(ch), "\tYYou come across %s.\tn\r\n", CAP(mob->player.short_descr));

        if (stealth_success)
        {
          send_to_room(IN_ROOM(ch), "\tYYou managed to sneak past %s, and they continue on their way.\tn\r\n", mob->player.short_descr);
          return;
        }
        num_mobs++;
      }
    }
  }

  // set up the encounter
  // determine what and how many mobs based on party size and average level
  // create mobs and place in room
  // set treasure type
  // prevent party from exiting the room using normal movement
  // set despawn timer. Timer only goes down if no players are in the room with them
}

bool in_encounter_room(struct char_data *ch)
{
  if (!ch || IN_ROOM(ch) == NOWHERE)
    return false;

  struct char_data *tch = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (!IS_NPC(tch))
      continue;
    if (MOB_FLAGGED(tch, MOB_ENCOUNTER))
      return true;
  }
  return false;
}

ACMD(do_encounterinfo)
{
  char arg1[200], arg2[200];
  int i = 0, j = 0;
  int min_level = 50, max_level = 0;
  int count = 0;

  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  if (!*arg1)
  {
    send_to_char(ch, "Please specify what you'd like to see.  Options: terrain\r\n");
    return;
  }

  if (is_abbrev(arg1, "terrains"))
  {
    send_to_char(ch, "%-30s - %-19s - %s\r\n", "Terrain Type", "Num Encounter Types", "Level Ranges Covered");
    for (i = 0; i < NUM_ROOM_SECTORS; i++)
    {
      min_level = 50;
      max_level = 0;
      count = 0;
      for (j = 0; j < NUM_ENCOUNTER_TYPES; j++)
      {
        if (encounter_table[j].sector_types[i])
        {
          if (encounter_table[j].min_level < min_level && encounter_table[j].min_level != 0)
            min_level = encounter_table[j].min_level;
          if (encounter_table[j].max_level > max_level && encounter_table[j].max_level != 40)
            max_level = encounter_table[j].max_level;
          count++;
        }
      }
      send_to_char(ch, "%-30s - %3d encounter types - levels %2d to %2d\r\n", sector_types[i], count, min_level, max_level);
    }
  }
  send_to_char(ch, "\r\nTotal number of encounters: %d\r\n", NUM_ENCOUNTER_GROUP_TYPES);
}

int encounter_bribe_amount(struct char_data *ch)
{
  if (!ch)
    return 0;

  return (GET_LEVEL(ch) * MAX(1, GET_LEVEL(ch) / 2) * 10);
}

void set_encounter_peaceful(struct char_data *ch)
{
  struct char_data *tch = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (IS_NPC(tch) && MOB_FLAGGED(tch, MOB_ENCOUNTER))
    {
      tch->mob_specials.peaceful_timer = 10;
      if (tch->mob_specials.hostile)
        tch->mob_specials.aggro_timer = 5;
    }
  }
}

bool is_peaceful_encounter(struct char_data *ch)
{
  if (!ch)
    return false;

  if (IN_ROOM(ch) == NOWHERE)
    return false;

  if (!in_encounter_room(ch))
  {
    return false;
  }

  struct char_data *tch = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (IS_NPC(tch) && MOB_FLAGGED(tch, MOB_ENCOUNTER) && tch->mob_specials.peaceful_timer > 0)
      return true;
  }
  return false;
}

void set_encounter_to_peaceful(struct char_data *ch)
{
  if (!ch)
    return;

  if (IN_ROOM(ch) == NOWHERE)
    return;

  if (!in_encounter_room(ch))
  {
    return;
  }

  struct char_data *tch = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (IS_NPC(tch) && MOB_FLAGGED(tch, MOB_ENCOUNTER))
      tch->mob_specials.peaceful_timer = 10;
  }
}

bool is_hostile_encounter(struct char_data *ch)
{
  if (!ch)
    return false;

  if (IN_ROOM(ch) == NOWHERE)
    return false;

  if (!in_encounter_room(ch))
  {
    return false;
  }

  struct char_data *tch = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (IS_NPC(tch) && MOB_FLAGGED(tch, MOB_ENCOUNTER) && tch->mob_specials.hostile && (tch->mob_specials.peaceful_timer == -1))
      return true;
  }
  return false;
}

bool can_coerce_encounter(struct char_data *ch, int attempt_type)
{
  if (!ch)
    return false;

  if (IN_ROOM(ch) == NOWHERE)
    return false;

  if (!in_encounter_room(ch))
  {
    return false;
  }

  struct char_data *tch = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (IS_NPC(tch) && MOB_FLAGGED(tch, MOB_ENCOUNTER))
    {
      if (tch->mob_specials.sentient)
        return true;
      if (IS_ANIMAL(tch) && HAS_FEAT(ch, FEAT_WILD_EMPATHY) && attempt_type != ENCOUNTER_SCMD_BRIBE)
        return true;
    }
  }
  return false;
}

int get_party_slowest_speed(struct char_data *ch)
{

  if (IN_ROOM(ch) == NOWHERE)
    return 0;

  struct char_data *tch = NULL;
  int speed = get_speed(ch, false);

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (is_player_grouped(ch, tch))
      if (get_speed(tch, false) < speed)
        speed = get_speed(tch, false);
  }
  return speed;
}

int get_encounter_mobs_speed(struct char_data *ch)
{

  if (IN_ROOM(ch) == NOWHERE)
    return 0;

  struct char_data *tch = NULL;
  int speed = 0;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (IS_NPC(tch))
      if (MOB_FLAGGED(tch, MOB_ENCOUNTER))
        if (get_speed(tch, false) > speed)
          speed = get_speed(tch, false);
  }
  return speed;
}

bool can_encounter_mobs_see_party(struct char_data *ch)
{
  if (IN_ROOM(ch) == NOWHERE)
    return false;

  struct char_data *mch = NULL, *tch = NULL;

  for (mch = world[IN_ROOM(ch)].people; mch; mch = mch->next_in_room)
  {
    if (!IS_NPC(mch) || !MOB_FLAGGED(mch, MOB_ENCOUNTER))
      continue;
    for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
    {
      if (IS_NPC(tch) || !is_player_grouped(tch, ch))
        continue;
      if (CAN_SEE(mch, tch))
        return true;
    }
  }
  return false;
}

bool encounter_mobs_can_move(struct char_data *ch)
{
  if (IN_ROOM(ch) == NOWHERE)
    return 0;
  bool can_move = true;

  struct char_data *tch = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (IS_NPC(tch))
    {
      if (MOB_FLAGGED(tch, MOB_ENCOUNTER))
      {
        can_move = true;
        if (AFF_FLAGGED(tch, AFF_GRAPPLED) || AFF_FLAGGED(tch, AFF_ENTANGLED))
          can_move = false;
        else if (affected_by_spell(tch, SKILL_DEFENSIVE_STANCE) && !HAS_FEAT(tch, FEAT_MOBILE_DEFENSE))
          can_move = false;
        else if (AFF_FLAGGED(tch, AFF_STUN) || AFF_FLAGGED(tch, AFF_PARALYZED) || char_has_mud_event(tch, eSTUNNED))
          can_move = false;
        else if (AFF_FLAGGED(tch, AFF_DAZED))
          can_move = false;
        else if (AFF_FLAGGED(tch, AFF_FEAR))
          can_move = false;
        else if (GET_POS(tch) <= POS_SLEEPING)
          can_move = false;
      }
    }
  }
  return can_move;
}

bool encounter_coerce_attempted(struct char_data *ch, int attempt_type)
{
  if (!ch)
    return false;

  struct char_data *tch = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (IS_NPC(tch))
    {
      if (MOB_FLAGGED(tch, MOB_ENCOUNTER))
      {
        if (tch->mob_specials.coersion_attempted[attempt_type])
          return true;
      }
    }
  }
  return false;
}

void set_coersion_attempted(struct char_data *ch, int attempt_type)
{
  if (!ch)
    return;

  struct char_data *tch = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (IS_NPC(tch))
    {
      if (MOB_FLAGGED(tch, MOB_ENCOUNTER))
      {
        tch->mob_specials.coersion_attempted[attempt_type] = true;
      }
    }
  }
}

void give_gold_to_encounter_mob(struct char_data *ch, int amount)
{
  if (!ch)
    return;

  struct char_data *tch = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (IS_NPC(tch))
    {
      if (MOB_FLAGGED(tch, MOB_ENCOUNTER))
      {
        GET_GOLD(tch) += amount;
        return;
      }
    }
  }
}

void encounter_command_description(struct char_data *ch)
{
  if (!ch)
    return;

  send_to_char(ch, "Please choose an action to take:\r\n"
                   "depart     - leave a non-hostile encounter peacefully\r\n"
                   "escape     - leave a hostile encounter through various means (HELP ESCAPE)\r\n"
                   "distract   - leave a hostile encounter using stealth skill\r\n"
                   "intimidate - make a sentient hostile mob non-hostile using intimidate skill\r\n"
                   "diplomacy  - make a sentient hostile mob non-hostile using diplomacy skill\r\n"
                   "bluff      - make a sentient hostile mob non-hostile using bluff skill\r\n"
                   "bribe      - make a sentient hostile mob non-hostile by giving them %d gold\r\n"
                   "\r\n"
                   "Any of these options will make it possible to leave the encounter room for 60 seconds, at which point\r\n"
                   "the encounter will become active again, and an attempt must be made again. Note that the same tactic\r\n"
                   "cannot be used more than once per encounter.\r\n"
                   "\r\n",
               encounter_bribe_amount(ch));
}

ACMD(do_encounter)
{

  char arg[200], escape_buf[200];
  one_argument(argument, arg, sizeof(arg));
  bool can_escape = false;

  if (!*arg)
  {
    encounter_command_description(ch);
    return;
  }

  if (is_abbrev(arg, "depart"))
  {
    if (is_hostile_encounter(ch))
    {
      send_to_char(ch, "You cannot 'depart' a hostile ancounter.\r\n");
      return;
    }
    set_encounter_to_peaceful(ch);
    send_to_room(IN_ROOM(ch), "This encounter has become peaceful for 60 seconds.  You may now leave freely if desired.\r\n");
    return;
  }
  else if (is_abbrev(arg, "escape"))
  {

    if (get_party_slowest_speed(ch) > get_encounter_mobs_speed(ch))
    {
      snprintf(escape_buf, sizeof(escape_buf), "Your party is not fast enough to escape.\r\n");
      can_escape = true;
    }
    else if (!can_encounter_mobs_see_party(ch))
    {
      snprintf(escape_buf, sizeof(escape_buf), "Your escape is blocked by your enemies.\r\n");
      can_escape = true;
    }
    else if (!encounter_mobs_can_move(ch))
    {
      can_escape = true;
      snprintf(escape_buf, sizeof(escape_buf), "Your escape is blocked by your enemies.\r\n");
    }

    if (can_escape)
    {
      set_encounter_to_peaceful(ch);
      send_to_char(ch, "Your attempt to escape is successful.\r\n");
      send_to_room(IN_ROOM(ch), "This encounter has become peaceful for 60 seconds.  You may now leave freely if desired.\r\n");
      return;
    }
    else
    {
      send_to_char(ch, "%s", escape_buf);
      return;
    }
  }
  else if (is_abbrev(arg, "distract"))
  {
    if (encounter_coerce_attempted(ch, ENCOUNTER_SCMD_DISTRACT))
    {
      send_to_char(ch, "Your party has already attempted to distract in this encounter");
    }
    else if (skill_roll(ch, ABILITY_STEALTH) > get_exploration_dc(ch))
    {
      set_encounter_to_peaceful(ch);
      send_to_char(ch, "Your attempt to distract the enemy has succeeded.\r\n");
      send_to_room(IN_ROOM(ch), "This encounter has become peaceful for 60 seconds.  You may now leave freely if desired.\r\n");
    }
    else
    {
      send_to_char(ch, "Your attempt to distract the enemy has failed.\r\n");
      set_coersion_attempted(ch, ENCOUNTER_SCMD_DISTRACT);
    }
  }
  else if (is_abbrev(arg, "intimidate"))
  {
    if (!can_coerce_encounter(ch, ENCOUNTER_SCMD_INTIMIDATE))
    {
      send_to_char(ch, "You cannot attempt to intimidate this type of enemy.\r\n");
    }
    else if (encounter_coerce_attempted(ch, ENCOUNTER_SCMD_INTIMIDATE))
    {
      send_to_char(ch, "Your party has already attempted to intimidate in this encounter");
    }
    else if (skill_roll(ch, ABILITY_INTIMIDATE) > get_exploration_dc(ch))
    {
      set_encounter_to_peaceful(ch);
      send_to_char(ch, "Your attempt to intimidate the enemy has succeeded.\r\n");
      send_to_room(IN_ROOM(ch), "This encounter has become peaceful for 60 seconds.  You may now leave freely if desired.\r\n");
    }
    else
    {
      send_to_char(ch, "Your attempt to intimidate the enemy has failed.\r\n");
      set_coersion_attempted(ch, ENCOUNTER_SCMD_INTIMIDATE);
    }
  }
  else if (is_abbrev(arg, "diplomacy"))
  {
    if (!can_coerce_encounter(ch, ENCOUNTER_SCMD_DIPLOMACY))
    {
      send_to_char(ch, "You cannot attempt to use diplomacy on this type of enemy.\r\n");
    }
    else if (encounter_coerce_attempted(ch, ENCOUNTER_SCMD_DIPLOMACY))
    {
      send_to_char(ch, "Your party has already attempted to use diplomacy in this encounter");
    }
    else if (skill_roll(ch, ABILITY_DIPLOMACY) > get_exploration_dc(ch))
    {
      set_encounter_to_peaceful(ch);
      send_to_char(ch, "Your attempt to use diplomacy on the enemy has succeeded.\r\n");
      send_to_room(IN_ROOM(ch), "This encounter has become peaceful for 60 seconds.  You may now leave freely if desired.\r\n");
    }
    else
    {
      send_to_char(ch, "Your attempt to use diplomacy on the enemy has failed.\r\n");
      set_coersion_attempted(ch, ENCOUNTER_SCMD_DIPLOMACY);
    }
  }
  else if (is_abbrev(arg, "bluff"))
  {
    if (!can_coerce_encounter(ch, ENCOUNTER_SCMD_BLUFF))
    {
      send_to_char(ch, "You cannot attempt to bluff this type of enemy.\r\n");
    }
    else if (encounter_coerce_attempted(ch, ENCOUNTER_SCMD_BLUFF))
    {
      send_to_char(ch, "Your party has already attempted to bluff in this encounter");
    }
    else if (skill_roll(ch, ABILITY_BLUFF) > get_exploration_dc(ch))
    {
      set_encounter_to_peaceful(ch);
      send_to_char(ch, "Your attempt to bluff the enemy has succeeded.\r\n");
      send_to_room(IN_ROOM(ch), "This encounter has become peaceful for 60 seconds.  You may now leave freely if desired.\r\n");
    }
    else
    {
      send_to_char(ch, "Your attempt to bluff the enemy has failed.\r\n");
      set_coersion_attempted(ch, ENCOUNTER_SCMD_BLUFF);
    }
  }
  else if (is_abbrev(arg, "bribe"))
  {
    if (!can_coerce_encounter(ch, ENCOUNTER_SCMD_BRIBE))
    {
      send_to_char(ch, "You cannot attempt to bribe this type of enemy.\r\n");
    }
    else if (GET_GOLD(ch) < encounter_bribe_amount(ch))
    {
      send_to_char(ch, "You don't have enough gold to bribe this enemy.  You need %d.\r\n", encounter_bribe_amount(ch));
    }
    else
    {
      GET_GOLD(ch) -= encounter_bribe_amount(ch);
      give_gold_to_encounter_mob(ch, encounter_bribe_amount(ch));
      set_encounter_to_peaceful(ch);
      send_to_char(ch, "Your attempt to bribe the enemy has succeeded. It cost you %d gold.\r\n", encounter_bribe_amount(ch));
      send_to_room(IN_ROOM(ch), "This encounter has become peaceful for 60 seconds.  You may now leave freely if desired.\r\n");
    }
  }
  else
  {
    encounter_command_description(ch);
    return;
  }
}

void set_expire_cooldown(room_rnum rnum)
{
  if (rnum == NOWHERE)
    return;

  struct char_data *tch = NULL;

  for (tch = world[rnum].people; tch; tch = tch->next_in_room)
  {
    if (IS_NPC(tch))
    {
      if (MOB_FLAGGED(tch, MOB_ENCOUNTER))
      {
        if (tch->mob_specials.extract_timer == -1)
          tch->mob_specials.extract_timer = 10;
      }
    }
  }
}

void reset_expire_cooldown(room_rnum rnum)
{
  if (rnum == NOWHERE)
    return;

  struct char_data *tch = NULL;

  for (tch = world[rnum].people; tch; tch = tch->next_in_room)
  {
    if (IS_NPC(tch))
    {
      if (MOB_FLAGGED(tch, MOB_ENCOUNTER))
      {
        if (tch->mob_specials.extract_timer != -1)
          tch->mob_specials.extract_timer = -1;
      }
    }
  }
}