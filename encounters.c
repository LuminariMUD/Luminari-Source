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
static void add_encounter(int encounter_record, int encounter_type, int max_level, int encounter_group, const char *object_name, 
                          int load_chance, int min_number, int max_number, int treasure_table)
{
  encounter_table[encounter_record].encounter_type = encounter_type;
  encounter_table[encounter_record].max_level = max_level;
  encounter_table[encounter_record].encounter_group = encounter_group;
  encounter_table[encounter_record].object_name = object_name;
  encounter_table[encounter_record].load_chance = load_chance;
  encounter_table[encounter_record].min_number = min_number;
  encounter_table[encounter_record].max_number = max_number;
  encounter_table[encounter_record].treasure_table = treasure_table;
}

void initialize_encounter_table(void)
{
  int i, j;

  /* initialize the list of encounters */
  for (i = 0; i < NUM_ENCOUNTER_TYPES; i++)
  {
    encounter_table[i].encounter_type = 0;
    encounter_table[i].max_level = 0;
    encounter_table[i].encounter_group = 0;
    encounter_table[i].object_name = "Unused Encounter";
    encounter_table[i].load_chance = 0;
    encounter_table[i].min_number = 0;
    encounter_table[i].max_number = 0;
    encounter_table[i].treasure_table = 0;
    for (j = 0; j < NUM_ROOM_SECTORS; j++)
      encounter_table[i].sector_types[j] = false;
  }
}

void add_encounter_sector(int encounter_record, int sector_type)
{
    encounter_table[encounter_record].sector_types[sector_type] = true;
}

void populate_encounter_table(void)
{
    initialize_encounter_table();

    add_encounter(ENCOUNTER_TYPE_GOBLIN_1, ENCOUNTER_CLASS_COMBAT, 10, ENCOUNTER_GROUP_TYPE_GOBLINS, "goblin raider", 100, 1, 3, TREASURE_TABLE_LOW_NORM);
    add_encounter_sector(ENCOUNTER_TYPE_GOBLIN_1, SECT_FIELD); add_encounter_sector(ENCOUNTER_TYPE_GOBLIN_1, SECT_CAVE);
    add_encounter_sector(ENCOUNTER_TYPE_GOBLIN_1, SECT_FOREST); add_encounter_sector(ENCOUNTER_TYPE_GOBLIN_1, SECT_HILLS);
    add_encounter_sector(ENCOUNTER_TYPE_GOBLIN_1, SECT_MOUNTAIN); add_encounter_sector(ENCOUNTER_TYPE_GOBLIN_1, SECT_MARSHLAND);

    add_encounter(ENCOUNTER_TYPE_GOBLIN_2, ENCOUNTER_CLASS_COMBAT, 10, ENCOUNTER_GROUP_TYPE_GOBLINS, "goblin chieftain", 25, 1, 1, TREASURE_TABLE_LOW_BOSS);
    add_encounter_sector(ENCOUNTER_TYPE_GOBLIN_2, SECT_FIELD); add_encounter_sector(ENCOUNTER_TYPE_GOBLIN_2, SECT_CAVE);
    add_encounter_sector(ENCOUNTER_TYPE_GOBLIN_2, SECT_FOREST); add_encounter_sector(ENCOUNTER_TYPE_GOBLIN_2, SECT_HILLS);
    add_encounter_sector(ENCOUNTER_TYPE_GOBLIN_2, SECT_MOUNTAIN); add_encounter_sector(ENCOUNTER_TYPE_GOBLIN_2, SECT_MARSHLAND);

}

int encounter_chance(struct char_data *ch)
{
    int base_chance = 5;

    return 0;
}

int get_exploration_dc(struct char_data *ch)
{
    if (!ch) return 9999;
    
    if (STATE(ch->desc) != CON_PLAYING) return 9999;

    int dc = ((int) GET_LEVEL(ch) / 1.5) + 17;

    return dc;
}

