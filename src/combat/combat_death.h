#ifndef COMBAT_DEATH_H
#define COMBAT_DEATH_H

#include "domain_events.h"
#include "structs.h"

enum combat_death_cause
{
  COMBAT_DEATH_UNSPECIFIED = 0,
  COMBAT_DEATH_COMBAT,
  COMBAT_DEATH_SCRIPT,
  COMBAT_DEATH_ATTRITION,
  COMBAT_DEATH_ADMINISTRATIVE
};

struct combat_death_result
{
  struct domain_entity_handle victim;
  struct domain_entity_handle killer;
  enum combat_death_cause cause;
  bool processed;
};

struct combat_death_result combat_death_apply(struct char_data *victim,
                                              struct char_data *killer,
                                              enum combat_death_cause cause);

#endif /* COMBAT_DEATH_H */
