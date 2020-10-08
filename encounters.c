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

extern struct room_data *world;
extern struct char_data *character_list;

struct encounter_data encounter_table[NUM_ENCOUNTER_TYPES];

/* function to assign basic attributes to an encounter record */
void add_encounter_record(int encounter_record, int encounter_type, int min_level, int max_level, int encounter_group, const char *object_name, 
                          int load_chance, int min_number, int max_number, int treasure_table, int char_class, int encounter_strength,
                          int alignment, int race_type, int subrace1, int subrace2, int subrace3, bool hostile, int size)
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
    encounter_table[encounter_record].hostile = false;
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
  encounter_table[encounter_record].sector_types[ SECT_INSIDE] = true;
  encounter_table[encounter_record].sector_types[ SECT_CITY] = true;
  encounter_table[encounter_record].sector_types[ SECT_FIELD] = true;
  encounter_table[encounter_record].sector_types[ SECT_FOREST] = true;
  encounter_table[encounter_record].sector_types[ SECT_HILLS] = true;
  encounter_table[encounter_record].sector_types[ SECT_MOUNTAIN] = true;
  encounter_table[encounter_record].sector_types[ SECT_ROAD_NS] = true;
  encounter_table[encounter_record].sector_types[ SECT_ROAD_EW] = true;
  encounter_table[encounter_record].sector_types[ SECT_ROAD_INT] = true;
  encounter_table[encounter_record].sector_types[ SECT_DESERT] = true;
  encounter_table[encounter_record].sector_types[ SECT_MARSHLAND] = true;
  encounter_table[encounter_record].sector_types[ SECT_HIGH_MOUNTAIN] = true;
  encounter_table[encounter_record].sector_types[ SECT_LAVA] = true;
  encounter_table[encounter_record].sector_types[ SECT_D_ROAD_NS] = true;
  encounter_table[encounter_record].sector_types[ SECT_D_ROAD_EW] = true;
  encounter_table[encounter_record].sector_types[ SECT_D_ROAD_INT] = true;
  encounter_table[encounter_record].sector_types[ SECT_CAVE] = true;
  encounter_table[encounter_record].sector_types[ SECT_JUNGLE] = true;
  encounter_table[encounter_record].sector_types[ SECT_TUNDRA] = true;
  encounter_table[encounter_record].sector_types[ SECT_TAIGA] = true;
  encounter_table[encounter_record].sector_types[ SECT_BEACH] = true;
}

void set_encounter_terrain_all_underdark(int encounter_record)
{
  encounter_table[encounter_record].sector_types[ SECT_UD_WILD] = true;
  encounter_table[encounter_record].sector_types[ SECT_UD_CITY] = true;
  encounter_table[encounter_record].sector_types[ SECT_UD_INSIDE] = true;
}

void set_encounter_terrain_all_roads(int encounter_record)
{
  encounter_table[encounter_record].sector_types[ SECT_D_ROAD_NS] = true;
  encounter_table[encounter_record].sector_types[ SECT_D_ROAD_EW] = true;
  encounter_table[encounter_record].sector_types[ SECT_D_ROAD_INT] = true;
  encounter_table[encounter_record].sector_types[ SECT_ROAD_NS] = true;
  encounter_table[encounter_record].sector_types[ SECT_ROAD_EW] = true;
  encounter_table[encounter_record].sector_types[ SECT_ROAD_INT] = true;
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

    add_encounter_record(ENCOUNTER_TYPE_GOBLIN_1, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_GOBLINS, "goblin raider", 100, 1, 3, 
      TREASURE_TABLE_LOW_NORM, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_EVIL, RACE_TYPE_HUMANOID, 
      SUBRACE_GOBLINOID, SUBRACE_EVIL, SUBRACE_CHAOTIC, HOSTILE, SIZE_SMALL);
    set_encounter_terrain_any(ENCOUNTER_TYPE_GOBLIN_1);

    add_encounter_record(ENCOUNTER_TYPE_GOBLIN_2, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_GOBLINS, "goblin chieftain", 25, 1, 1, 
      TREASURE_TABLE_LOW_BOSS, CLASS_WARRIOR, ENCOUNTER_STRENGTH_BOSS, CHAOTIC_EVIL, RACE_TYPE_HUMANOID, 
      SUBRACE_GOBLINOID, SUBRACE_EVIL, SUBRACE_CHAOTIC, HOSTILE, SIZE_SMALL);
    set_encounter_terrain_any(ENCOUNTER_TYPE_GOBLIN_2);

    add_encounter_record(ENCOUNTER_TYPE_KING_CRAB_1, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_KING_CRABS, "king crab", 100, 1, 4, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_VERMIN, 
      SUBRACE_AQUATIC, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SIZE_TINY );
    add_encounter_sector(ENCOUNTER_TYPE_KING_CRAB_1, SECT_WATER_SWIM);      add_encounter_sector(ENCOUNTER_TYPE_KING_CRAB_1, SECT_WATER_NOSWIM);
    add_encounter_sector(ENCOUNTER_TYPE_KING_CRAB_1, SECT_UNDERWATER);      add_encounter_sector(ENCOUNTER_TYPE_KING_CRAB_1, SECT_OCEAN);
    add_encounter_sector(ENCOUNTER_TYPE_KING_CRAB_1, SECT_UD_WATER);      add_encounter_sector(ENCOUNTER_TYPE_KING_CRAB_1, SECT_UD_NOSWIM);
    add_encounter_sector(ENCOUNTER_TYPE_KING_CRAB_1, SECT_BEACH);

    add_encounter_record(ENCOUNTER_TYPE_KOBOLD_1, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_KOBOLDS, "kobold warrior", 100, 1, 4, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, LAWFUL_EVIL, RACE_TYPE_HUMANOID, 
      SUBRACE_REPTILIAN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_SMALL );
    set_encounter_terrain_any(ENCOUNTER_TYPE_KOBOLD_1);

    add_encounter_record(ENCOUNTER_TYPE_GIANT_RAT_1, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_GIANT_RATS, "giant rat", 100, 1, 6, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_VERMIN, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_TINY );
    set_encounter_terrain_any(ENCOUNTER_TYPE_GIANT_RAT_1);
  
    add_encounter_record(ENCOUNTER_TYPE_SEWER_CENTIPEDE, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_CENTIPEDES, "sewer centipede", 100, 1, 5, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_VERMIN, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_SMALL );
    set_encounter_terrain_all_underdark(ENCOUNTER_TYPE_SEWER_CENTIPEDE);      add_encounter_sector(ENCOUNTER_TYPE_SEWER_CENTIPEDE, SECT_CAVE);

    add_encounter_record(ENCOUNTER_TYPE_SEA_WASP_JELLYFISH, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_SEA_WASP_JELLYFISH, "sea wasp jellyfish", 100, 1, 4, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_VERMIN, 
      SUBRACE_AQUATIC, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_TINY );
    add_encounter_sector(ENCOUNTER_TYPE_SEA_WASP_JELLYFISH, SECT_WATER_SWIM);      add_encounter_sector(ENCOUNTER_TYPE_SEA_WASP_JELLYFISH, SECT_WATER_NOSWIM);
    add_encounter_sector(ENCOUNTER_TYPE_SEA_WASP_JELLYFISH, SECT_UNDERWATER);      add_encounter_sector(ENCOUNTER_TYPE_SEA_WASP_JELLYFISH, SECT_OCEAN);
    add_encounter_sector(ENCOUNTER_TYPE_SEA_WASP_JELLYFISH, SECT_UD_WATER);      add_encounter_sector(ENCOUNTER_TYPE_SEA_WASP_JELLYFISH, SECT_UD_NOSWIM);

    add_encounter_record(ENCOUNTER_TYPE_MANDRILL, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_MANDRILL, "mandrill", 100, 1, 4, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_SMALL );
    add_encounter_sector(ENCOUNTER_TYPE_MANDRILL, SECT_FOREST);      add_encounter_sector(ENCOUNTER_TYPE_MANDRILL, SECT_JUNGLE);

    add_encounter_record(ENCOUNTER_TYPE_MUCKDWELLER, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_MUCKDWELLER, "muckdweller", 100, 1, 4, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_MAGICAL_BEAST, 
      SUBRACE_REPTILIAN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_SMALL );
    add_encounter_sector(ENCOUNTER_TYPE_MUCKDWELLER, SECT_MARSHLAND);  

    add_encounter_record(ENCOUNTER_TYPE_BARRACUDA, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_BARRACUDA, "barracuda", 100, 1, 4, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL, 
      SUBRACE_AQUATIC, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_SMALL );
    set_encounter_terrain_all_water(ENCOUNTER_TYPE_BARRACUDA);

    add_encounter_record(ENCOUNTER_TYPE_FIRE_BEETLE, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_FIRE_BEETLE, "fire beetle", 100, 1, 5, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_VERMIN, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_SMALL );
    set_encounter_terrain_any(ENCOUNTER_TYPE_FIRE_BEETLE);

    add_encounter_record(ENCOUNTER_TYPE_DROW_WARRIOR, ENCOUNTER_CLASS_COMBAT, 1, 20, ENCOUNTER_GROUP_TYPE_DROW_PATROL, "drow warrior", 100, 1, 3, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_EVIL, RACE_TYPE_HUMANOID, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_MEDIUM );
    add_encounter_sector(ENCOUNTER_TYPE_DROW_WARRIOR, SECT_UD_WILD);      add_encounter_sector(ENCOUNTER_TYPE_DROW_WARRIOR, SECT_UD_CITY);
    add_encounter_sector(ENCOUNTER_TYPE_DROW_WARRIOR, SECT_UD_INSIDE);  

    add_encounter_record(ENCOUNTER_TYPE_DROW_PRIESTESS, ENCOUNTER_CLASS_COMBAT, 1, 20, ENCOUNTER_GROUP_TYPE_DROW_PATROL, "drow priestess", 100, 1, 1, 
      TREASURE_TABLE_NONE, CLASS_CLERIC, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_EVIL, RACE_TYPE_HUMANOID, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_MEDIUM );
    add_encounter_sector(ENCOUNTER_TYPE_DROW_PRIESTESS, SECT_UD_WILD);      add_encounter_sector(ENCOUNTER_TYPE_DROW_PRIESTESS, SECT_UD_CITY);
    add_encounter_sector(ENCOUNTER_TYPE_DROW_PRIESTESS, SECT_UD_INSIDE);  

      add_encounter_record(ENCOUNTER_TYPE_DROW_WIZARD, ENCOUNTER_CLASS_COMBAT, 1, 20, ENCOUNTER_GROUP_TYPE_DROW_PATROL, "drow wizard", 100, 1, 1, 
      TREASURE_TABLE_NONE, CLASS_WIZARD, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_EVIL, RACE_TYPE_HUMANOID, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_MEDIUM );
    add_encounter_sector(ENCOUNTER_TYPE_DROW_WIZARD, SECT_UD_WILD);      add_encounter_sector(ENCOUNTER_TYPE_DROW_WIZARD, SECT_UD_CITY);
    add_encounter_sector(ENCOUNTER_TYPE_DROW_WIZARD, SECT_UD_INSIDE);  

    add_encounter_record(ENCOUNTER_TYPE_DUERGAR_WARRIOR, ENCOUNTER_CLASS_COMBAT, 1, 20, ENCOUNTER_GROUP_TYPE_DUERGAR_PATROL, "duergar warrior", 100, 1, 4, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, LAWFUL_EVIL, RACE_TYPE_HUMANOID, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_MEDIUM );
    add_encounter_sector(ENCOUNTER_TYPE_DUERGAR_WARRIOR, SECT_UD_WILD);      add_encounter_sector(ENCOUNTER_TYPE_DUERGAR_WARRIOR, SECT_UD_CITY);
    add_encounter_sector(ENCOUNTER_TYPE_DUERGAR_WARRIOR, SECT_UD_INSIDE);
    
    add_encounter_record(ENCOUNTER_TYPE_HAWK, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_HAWK, "hawk", 100, 1, 2, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SIZE_TINY );
    add_encounter_sector(ENCOUNTER_TYPE_HAWK, SECT_FOREST);      add_encounter_sector(ENCOUNTER_TYPE_HAWK, SECT_FLYING);
    add_encounter_sector(ENCOUNTER_TYPE_HAWK, SECT_TAIGA);

    add_encounter_record(ENCOUNTER_TYPE_MERFOLK_HUNTER, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_MERFOLK, "merfolk hunter", 100, 1, 5, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_HUMANOID, 
      SUBRACE_AQUATIC, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SIZE_MEDIUM );
    set_encounter_terrain_all_water(ENCOUNTER_TYPE_MERFOLK_HUNTER);

    add_encounter_record(ENCOUNTER_TYPE_ORCISH_WARRIOR, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_ORC_PATROL, "orcish warrior", 100, 1, 6, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_EVIL, RACE_TYPE_HUMANOID, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_MEDIUM );
    add_encounter_sector(ENCOUNTER_TYPE_ORCISH_WARRIOR, SECT_FIELD);      add_encounter_sector(ENCOUNTER_TYPE_ORCISH_WARRIOR, SECT_FOREST);
    add_encounter_sector(ENCOUNTER_TYPE_ORCISH_WARRIOR, SECT_HILLS);      add_encounter_sector(ENCOUNTER_TYPE_ORCISH_WARRIOR, SECT_MOUNTAIN);
    add_encounter_sector(ENCOUNTER_TYPE_ORCISH_WARRIOR, SECT_HIGH_MOUNTAIN);      add_encounter_sector(ENCOUNTER_TYPE_ORCISH_WARRIOR, SECT_UD_WILD);
    add_encounter_sector(ENCOUNTER_TYPE_ORCISH_WARRIOR, SECT_UD_INSIDE);      add_encounter_sector(ENCOUNTER_TYPE_ORCISH_WARRIOR, SECT_CAVE);

    add_encounter_record(ENCOUNTER_TYPE_OWL, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_OWL, "owl", 100, 1, 2, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SIZE_TINY );
    add_encounter_sector(ENCOUNTER_TYPE_OWL, SECT_FOREST);      add_encounter_sector(ENCOUNTER_TYPE_OWL, SECT_FLYING);
    add_encounter_sector(ENCOUNTER_TYPE_OWL, SECT_TAIGA);

    add_encounter_record(ENCOUNTER_TYPE_RATFOLK_WANDERER, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_RATFOLK, "ratfolk wanderer", 100, 1, 5, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_HUMANOID, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_SMALL );
    add_encounter_sector(ENCOUNTER_TYPE_RATFOLK_WANDERER, SECT_CITY);      add_encounter_sector(ENCOUNTER_TYPE_RATFOLK_WANDERER, SECT_DESERT);

      add_encounter_record(ENCOUNTER_TYPE_SEAL, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_SEAL, "seal", 100, 1, 2, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL, 
      SUBRACE_AQUATIC, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SIZE_SMALL );
    add_encounter_sector(ENCOUNTER_TYPE_SEAL, SECT_WATER_SWIM);      add_encounter_sector(ENCOUNTER_TYPE_SEAL, SECT_WATER_NOSWIM);
    add_encounter_sector(ENCOUNTER_TYPE_SEAL, SECT_OCEAN);      add_encounter_sector(ENCOUNTER_TYPE_SEAL, SECT_BEACH);

    add_encounter_record(ENCOUNTER_TYPE_WANDERING_SKELETON, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_WANDERING_SKELETON, "wandering skeleton", 100, 1, 4, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_UNDEAD, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_MEDIUM );
    set_encounter_terrain_any(ENCOUNTER_TYPE_WANDERING_SKELETON);

    add_encounter_record(ENCOUNTER_TYPE_SPRITE, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_SPRITE, "sprite", 100, 1, 3, 
      TREASURE_TABLE_NONE, CLASS_WIZARD, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_NEUTRAL, RACE_TYPE_FEY, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SIZE_DIMINUTIVE );
    add_encounter_sector(ENCOUNTER_TYPE_SPRITE, SECT_FOREST);  

    add_encounter_record(ENCOUNTER_TYPE_GIANT_MOSQUITO, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_GIANT_MOSQUITO, "giant mosquito", 100, 1, 4, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_VERMIN, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_SMALL );
    add_encounter_sector(ENCOUNTER_TYPE_GIANT_MOSQUITO, SECT_MARSHLAND);  

    add_encounter_record(ENCOUNTER_TYPE_ANTELOPE, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_ANTELOPE, "antelope", 100, 1, 4, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SIZE_SMALL );
    add_encounter_sector(ENCOUNTER_TYPE_ANTELOPE, SECT_FIELD);  
  
    add_encounter_record(ENCOUNTER_TYPE_BABOON, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_BABOON, "baboon", 100, 1, 4, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_SMALL );
    add_encounter_sector(ENCOUNTER_TYPE_BABOON, SECT_FIELD);      add_encounter_sector(ENCOUNTER_TYPE_BABOON, SECT_FOREST);
    add_encounter_sector(ENCOUNTER_TYPE_BABOON, SECT_JUNGLE);  

    add_encounter_record(ENCOUNTER_TYPE_BADGER, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_BADGER, "badger", 100, 1, 5, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_SMALL );
    add_encounter_sector(ENCOUNTER_TYPE_BADGER, SECT_FOREST);      add_encounter_sector(ENCOUNTER_TYPE_BADGER, SECT_TAIGA);

    add_encounter_record(ENCOUNTER_TYPE_DOLPHIN, ENCOUNTER_CLASS_COMBAT, 1, 10, ENCOUNTER_GROUP_TYPE_DOLPHIN, "dolphin", 100, 1, 4, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL, 
      SUBRACE_AQUATIC, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SIZE_MEDIUM );
    add_encounter_sector(ENCOUNTER_TYPE_DOLPHIN, SECT_OCEAN);  

    add_encounter_record(ENCOUNTER_TYPE_EAGLE, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_EAGLE, "eagle", 100, 1, 2, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SIZE_SMALL );
    add_encounter_sector(ENCOUNTER_TYPE_EAGLE, SECT_HILLS);      add_encounter_sector(ENCOUNTER_TYPE_EAGLE, SECT_MOUNTAIN);
    add_encounter_sector(ENCOUNTER_TYPE_EAGLE, SECT_FLYING);      add_encounter_sector(ENCOUNTER_TYPE_EAGLE, SECT_HIGH_MOUNTAIN);

    add_encounter_record(ENCOUNTER_TYPE_HOBGOBLIN_SOLDIER, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_HOBGOBLINS, "hobgoblin soldier", 100, 1, 4, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, LAWFUL_EVIL, RACE_TYPE_HUMANOID, 
      SUBRACE_GOBLINOID, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_MEDIUM );
    add_encounter_sector(ENCOUNTER_TYPE_HOBGOBLIN_SOLDIER, SECT_HILLS);      add_encounter_sector(ENCOUNTER_TYPE_HOBGOBLIN_SOLDIER, SECT_MOUNTAIN);
    add_encounter_sector(ENCOUNTER_TYPE_HOBGOBLIN_SOLDIER, SECT_CAVE);  

    add_encounter_record(ENCOUNTER_TYPE_HOBGOBLIN_SERGEANT, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_HOBGOBLINS, "hobgoblin sergeant", 25, 1, 1, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_BOSS, LAWFUL_EVIL, RACE_TYPE_HUMANOID, 
      SUBRACE_GOBLINOID, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_HUGE );
    add_encounter_sector(ENCOUNTER_TYPE_HOBGOBLIN_SERGEANT, SECT_HILLS);      add_encounter_sector(ENCOUNTER_TYPE_HOBGOBLIN_SERGEANT, SECT_MOUNTAIN);
    add_encounter_sector(ENCOUNTER_TYPE_HOBGOBLIN_SERGEANT, SECT_CAVE);  

    add_encounter_record(ENCOUNTER_TYPE_BLUE_RINGED_OCTOPUS, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_BLUE_RINGED_OCTOPUS, "blue ringed octopus", 100, 1, 1, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL, 
      SUBRACE_AQUATIC, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SIZE_TINY );
    set_encounter_terrain_all_water(ENCOUNTER_TYPE_BLUE_RINGED_OCTOPUS);  

    add_encounter_record(ENCOUNTER_TYPE_SAGARI, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_SAGARI, "sagari", 100, 1, 4, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, NEUTRAL_EVIL, RACE_TYPE_ABERRATION, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_TINY );
    add_encounter_sector(ENCOUNTER_TYPE_SAGARI, SECT_INSIDE);      add_encounter_sector(ENCOUNTER_TYPE_SAGARI, SECT_FOREST);
    add_encounter_sector(ENCOUNTER_TYPE_SAGARI, SECT_TAIGA);  

    add_encounter_record(ENCOUNTER_TYPE_VIPER, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_VIPER, "viper", 100, 1, 1, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_TINY );
    add_encounter_sector(ENCOUNTER_TYPE_VIPER, SECT_FIELD);      add_encounter_sector(ENCOUNTER_TYPE_VIPER, SECT_FOREST);
    add_encounter_sector(ENCOUNTER_TYPE_VIPER, SECT_HILLS);      add_encounter_sector(ENCOUNTER_TYPE_VIPER, SECT_MOUNTAIN);
    add_encounter_sector(ENCOUNTER_TYPE_VIPER, SECT_DESERT);      add_encounter_sector(ENCOUNTER_TYPE_VIPER, SECT_MARSHLAND);
    add_encounter_sector(ENCOUNTER_TYPE_VIPER, SECT_CAVE);      add_encounter_sector(ENCOUNTER_TYPE_VIPER, SECT_JUNGLE);
    add_encounter_sector(ENCOUNTER_TYPE_VIPER, SECT_BEACH);  

    add_encounter_record(ENCOUNTER_TYPE_STINGRAY, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_STINGRAY, "stingray", 100, 1, 4, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL, 
      SUBRACE_AQUATIC, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SIZE_MEDIUM );
    add_encounter_sector(ENCOUNTER_TYPE_STINGRAY, SECT_OCEAN);  

    add_encounter_record(ENCOUNTER_TYPE_GIANT_CRAB_SPIDER, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_GIANT_CRAB_SPIDER, "giant crab spider", 100, 1, 4, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_VERMIN, 
      SUBRACE_AQUATIC, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_SMALL );
    add_encounter_sector(ENCOUNTER_TYPE_GIANT_CRAB_SPIDER, SECT_FOREST);      add_encounter_sector(ENCOUNTER_TYPE_GIANT_CRAB_SPIDER, SECT_JUNGLE);

    add_encounter_record(ENCOUNTER_TYPE_STIRGE, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_STIRGE, "stirge", 100, 1, 5, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_MAGICAL_BEAST, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_TINY );
    add_encounter_sector(ENCOUNTER_TYPE_STIRGE, SECT_FOREST);      add_encounter_sector(ENCOUNTER_TYPE_STIRGE, SECT_MARSHLAND);
    add_encounter_sector(ENCOUNTER_TYPE_STIRGE, SECT_JUNGLE);  

    add_encounter_record(ENCOUNTER_TYPE_SYLPH, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_SYLPH, "sylph", 100, 1, 3, 
      TREASURE_TABLE_NONE, CLASS_ROGUE, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_OUTSIDER, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SIZE_MEDIUM );
    set_encounter_terrain_all_surface(ENCOUNTER_TYPE_SYLPH);  

    add_encounter_record(ENCOUNTER_TYPE_TENGU, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_TENGU, "tengu", 100, 1, 4, 
      TREASURE_TABLE_NONE, CLASS_ROGUE, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_HUMANOID, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SIZE_MEDIUM );
    set_encounter_terrain_all_surface(ENCOUNTER_TYPE_TENGU);  

    add_encounter_record(ENCOUNTER_TYPE_SNAPPING_TURTLE, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_SNAPPING_TURTLE, "snapping turtle", 100, 1, 4, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL, 
      SUBRACE_AQUATIC, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SIZE_TINY );
    add_encounter_sector(ENCOUNTER_TYPE_SNAPPING_TURTLE, SECT_WATER_SWIM);      add_encounter_sector(ENCOUNTER_TYPE_SNAPPING_TURTLE, SECT_WATER_NOSWIM);
    add_encounter_sector(ENCOUNTER_TYPE_SNAPPING_TURTLE, SECT_UNDERWATER);      add_encounter_sector(ENCOUNTER_TYPE_SNAPPING_TURTLE, SECT_OCEAN);
    add_encounter_sector(ENCOUNTER_TYPE_SNAPPING_TURTLE, SECT_BEACH);  

    add_encounter_record(ENCOUNTER_TYPE_UNDINE, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_UNDINE, "undine", 100, 1, 4, 
      TREASURE_TABLE_NONE, CLASS_CLERIC, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_OUTSIDER, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SIZE_MEDIUM );
    set_encounter_terrain_all_surface(ENCOUNTER_TYPE_UNDINE);  

    add_encounter_record(ENCOUNTER_TYPE_VEGEPYGMY, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_VEGEPYGMY, "vegepygmy", 100, 1, 4, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_PLANT, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SIZE_SMALL );
    set_encounter_terrain_all_underdark(ENCOUNTER_TYPE_VEGEPYGMY);      add_encounter_sector(ENCOUNTER_TYPE_VEGEPYGMY, SECT_CAVE);

    add_encounter_record(ENCOUNTER_TYPE_VULTURE, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_VULTURE, "vulture", 100, 1, 5, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SIZE_SMALL );
    add_encounter_sector(ENCOUNTER_TYPE_VULTURE, SECT_FIELD);      add_encounter_sector(ENCOUNTER_TYPE_VULTURE, SECT_HILLS);

    add_encounter_record(ENCOUNTER_TYPE_WANDERING_ZOMBIE, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_WANDERING_ZOMBIE, "wandering zombie", 100, 1, 5, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_UNDEAD, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_MEDIUM );
    set_encounter_terrain_all_surface(ENCOUNTER_TYPE_WANDERING_ZOMBIE);      set_encounter_terrain_all_underdark(ENCOUNTER_TYPE_WANDERING_ZOMBIE);

    add_encounter_record(ENCOUNTER_TYPE_GREMLIN, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_GREMLIN, "gremlin", 100, 1, 4, 
      TREASURE_TABLE_NONE, CLASS_ROGUE, ENCOUNTER_STRENGTH_NORMAL, CHAOTIC_EVIL, RACE_TYPE_FEY, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_SMALL );
    set_encounter_terrain_all_surface(ENCOUNTER_TYPE_GREMLIN);  

    add_encounter_record(ENCOUNTER_TYPE_MANTARI, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_MANTARI, "mantari", 100, 1, 5, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, NEUTRAL_EVIL, RACE_TYPE_MAGICAL_BEAST, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_MEDIUM );
    set_encounter_terrain_all_underdark(ENCOUNTER_TYPE_MANTARI);      add_encounter_sector(ENCOUNTER_TYPE_MANTARI, SECT_CAVE);

    add_encounter_record(ENCOUNTER_TYPE_GIANT_AMOEBA, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_GIANT_AMOEBA, "giant amoeba", 100, 1, 4, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_OOZE, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_SMALL );
    set_encounter_terrain_all_underdark(ENCOUNTER_TYPE_GIANT_AMOEBA);      set_encounter_terrain_all_water(ENCOUNTER_TYPE_GIANT_AMOEBA);
    add_encounter_sector(ENCOUNTER_TYPE_GIANT_AMOEBA, SECT_CAVE);  

    add_encounter_record(ENCOUNTER_TYPE_GIANT_BEE, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_GIANT_BEE, "giant bee", 100, 1, 4, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_VERMIN, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_MEDIUM );
    add_encounter_sector(ENCOUNTER_TYPE_GIANT_BEE, SECT_FIELD);      add_encounter_sector(ENCOUNTER_TYPE_GIANT_BEE, SECT_HILLS);
    add_encounter_sector(ENCOUNTER_TYPE_GIANT_BEE, SECT_MARSHLAND);      add_encounter_sector(ENCOUNTER_TYPE_GIANT_BEE, SECT_JUNGLE);
    add_encounter_sector(ENCOUNTER_TYPE_GIANT_BEE, SECT_BEACH);  

    add_encounter_record(ENCOUNTER_TYPE_BROWNIE, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_BROWNIE, "brownie", 100, 1, 4, 
      TREASURE_TABLE_NONE, CLASS_ROGUE, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_FEY, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SIZE_TINY );
    add_encounter_sector(ENCOUNTER_TYPE_BROWNIE, SECT_FIELD);      add_encounter_sector(ENCOUNTER_TYPE_BROWNIE, SECT_FOREST);

    add_encounter_record(ENCOUNTER_TYPE_CAMEL, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_CAMEL, "camel", 100, 1, 4, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_ANIMAL, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, NON_HOSTILE, SIZE_LARGE );
    add_encounter_sector(ENCOUNTER_TYPE_CAMEL, SECT_DESERT);  

    add_encounter_record(ENCOUNTER_TYPE_CLAWBAT, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_CLAWBAT, "clawbat", 100, 1, 4, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_MAGICAL_BEAST, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_SMALL );
    add_encounter_sector(ENCOUNTER_TYPE_CLAWBAT, SECT_HILLS);      add_encounter_sector(ENCOUNTER_TYPE_CLAWBAT, SECT_CAVE);

    add_encounter_record(ENCOUNTER_TYPE_DARKMANTLE, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_DARKMANTLE, "darkmantle", 100, 1, 4, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, TRUE_NEUTRAL, RACE_TYPE_MAGICAL_BEAST, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_SMALL );
    set_encounter_terrain_all_underdark(ENCOUNTER_TYPE_DARKMANTLE);      add_encounter_sector(ENCOUNTER_TYPE_DARKMANTLE, SECT_CAVE);

    add_encounter_record(ENCOUNTER_TYPE_DIRE_CORBY, ENCOUNTER_CLASS_COMBAT, 1, 15, ENCOUNTER_GROUP_TYPE_DIRE_CORBY, "dire corby", 100, 1, 4, 
      TREASURE_TABLE_NONE, CLASS_WARRIOR, ENCOUNTER_STRENGTH_NORMAL, NEUTRAL_EVIL, RACE_TYPE_MONSTROUS_HUMANOID, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, HOSTILE, SIZE_MEDIUM );
    set_encounter_terrain_all_underdark(ENCOUNTER_TYPE_DIRE_CORBY);      add_encounter_sector(ENCOUNTER_TYPE_DIRE_CORBY, SECT_CAVE);




}

int encounter_chance(struct char_data *ch)
{
    int chance = 5;

    return chance;
}

int get_exploration_dc(struct char_data *ch)
{
    if (!ch) return 9999;
    
    if (STATE(ch->desc) != CON_PLAYING) return 9999;

    int dc = ((int) GET_LEVEL(ch) / 1.5) + 17;

    return dc;
}

void check_random_encounter(struct char_data *ch)
{

  if (!ch || IN_ROOM(ch) == NOWHERE) return;

  if (dice(1, 100) > encounter_chance(ch))
    return;

  // check if avoiding encounters or not
  
  int i = 0, j = 0, count = 0, roll = 0, group_type = ENCOUNTER_GROUP_TYPE_NONE;
  int groups[NUM_ENCOUNTER_GROUP_TYPES];
  int room_sect = world[IN_ROOM(ch)].sector_type;
  int highest_level = get_max_party_level_same_room(ch);
  //int avg_level = get_avg_party_level_same_room(ch);
  int party_size = get_party_size_same_room(ch);
  int num_mobs = 0, num_mobs_roll = 0;
  struct char_data *mob = NULL;
  char mob_descs[MEDIUM_STRING];

  // initiatize
  for (i = 0; i < NUM_ENCOUNTER_GROUP_TYPES; i++)
    groups[i] = false;

  for (i = 0; i < NUM_ENCOUNTER_GROUP_TYPES; i++)
  {
    for (j = 0; j < NUM_ENCOUNTER_TYPES; j++)
      if (encounter_table[j].encounter_group == i)
        if (encounter_table[j].sector_types[room_sect] == true)
          if (highest_level <= encounter_table[j].max_level)
            groups[i] = true;
        
  }

  for (i = 0; i < NUM_ENCOUNTER_GROUP_TYPES; i++)
    if (groups[i] == true) count++;

  roll = dice(1, count);

  count = 0;

  for (i = 0; i < NUM_ENCOUNTER_GROUP_TYPES; i++) {
    if (groups[i] == true) {
      count++;
      if (count == roll) {
        group_type = i;
        break;
      }
    }
  }

  if (group_type == ENCOUNTER_GROUP_TYPE_NONE) {
    //send_to_char(ch, "Group type error\r\n");
    return;
  }

  for (j = 0; j < NUM_ENCOUNTER_TYPES; j++) {
    if (encounter_table[j].encounter_group == group_type)
     {
      num_mobs_roll = dice(encounter_table[j].min_number, encounter_table[j].max_number);
      for (i = 0; (i < num_mobs_roll) && (num_mobs < party_size); i++) {
        if (dice(1, 100) > encounter_table[j].load_chance) continue;
        mob = read_mobile(8100, VIRTUAL);
        if (!mob) {
          //send_to_char(ch, "Mob load error.\r\n");
          return;
        }
        // set descriptions
        mob->player.name = strdup(encounter_table[j].object_name);
        sprintf(mob_descs, "%s %s", AN(encounter_table[j].object_name), encounter_table[j].object_name);
        mob->player.short_descr = strdup(mob_descs);
        sprintf(mob_descs, "%s %s is here.\r\n", AN(encounter_table[j].object_name), encounter_table[j].object_name);
        mob->player.long_descr = strdup(mob_descs);
        sprintf(mob_descs, "%s %s is here before you.\r\n", AN(encounter_table[j].object_name), encounter_table[j].object_name);
        mob->player.description = strdup(mob_descs);    
        // set stats
        GET_CLASS(mob) = encounter_table[j].char_class;
        GET_LEVEL(mob) = MAX(1, highest_level - 2);
        autoroll_mob(mob, TRUE, FALSE);
        GET_EXP(mob) = (GET_LEVEL(mob) * GET_LEVEL(mob) * 75);
        GET_GOLD(mob) = (GET_LEVEL(mob) * 10);
        set_alignment(mob, encounter_table[j].alignment);
        // set flags
        SET_BIT_AR(MOB_FLAGS(mob), MOB_ENCOUNTER);
        SET_BIT_AR(MOB_FLAGS(mob), MOB_HELPER);

        X_LOC(mob) = world[IN_ROOM(ch)].coords[0];
        Y_LOC(mob) = world[IN_ROOM(ch)].coords[1];
        char_to_room(mob, IN_ROOM(ch));
        
        send_to_room(IN_ROOM(ch), "\tY%s has ambushed you!\tn\r\n", CAP(mob->player.short_descr));
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
    if (!IS_NPC(tch)) continue;
    if (MOB_FLAGGED(tch, MOB_ENCOUNTER))
      return true;
  }
  return false;
}

ACMD(do_encounters)
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