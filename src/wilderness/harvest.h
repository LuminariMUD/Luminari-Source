#ifndef LUMINARI_WILDERNESS_HARVEST_H
#define LUMINARI_WILDERNESS_HARVEST_H

#include "core/structs.h"

/* Which verb reached the wilderness harvest: each limits the categories it accepts. */
#define WILDERNESS_CMD_HARVEST 0
#define WILDERNESS_CMD_GATHER 1
#define WILDERNESS_CMD_MINE 2

bool wilderness_harvest_available(struct char_data *ch, int category, bool verbose);
int start_wilderness_crafting_harvest(struct char_data *ch, int category);
int start_wilderness_material_harvest(struct char_data *ch, int material);
int wilderness_harvest_rank(struct char_data *ch, int category);
int wilderness_harvest_tool_quality(struct char_data *ch);
int wilderness_harvest_material(int category, int subtype, int quality);
int wilderness_harvest_mote(int category, int subtype);
int award_wilderness_harvest(struct char_data *ch, int category, int subtype, int quality,
                             int quantity);

/* Targeted gathering (docs/systems/WILDERNESS_HARVESTING.md). */
int wilderness_material_category(int material);
int wilderness_sector_material_pool(int sector, int *out, int max);
int wilderness_material_pool(struct char_data *ch, int category, int *out, int max);
int wilderness_richness_tier(double level);
int wilderness_quality_tier_from(int skill_tier, double level, int tool_tier);
int wilderness_quality_tier(struct char_data *ch, int category, int success, int rank);
int wilderness_material_difficulty(int category, double level, int material);
bool wilderness_material_reachable(int material, int tier);
const char *wilderness_material_refusal(struct char_data *ch, int material);
int wilderness_parse_material(const char *arg);
void wilderness_show_pools(struct char_data *ch, int mode);
void wilderness_harvest_command(struct char_data *ch, const char *argument, int mode);

#endif
