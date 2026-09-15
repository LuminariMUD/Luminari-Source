#ifndef LUMINARI_WILDERNESS_HARVEST_H
#define LUMINARI_WILDERNESS_HARVEST_H

#include "core/structs.h"

bool wilderness_harvest_crafting_enabled(void);
bool wilderness_harvest_available(struct char_data *ch, int category, bool verbose);
int start_wilderness_crafting_harvest(struct char_data *ch, int category);
int wilderness_harvest_tool_quality(struct char_data *ch);
int wilderness_harvest_material(int category, int subtype, int quality);
int wilderness_harvest_mote(int category, int subtype);
int award_wilderness_harvest(struct char_data *ch, int category, int subtype, int quality,
                             int quantity);

#endif
