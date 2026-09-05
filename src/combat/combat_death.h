#ifndef COMBAT_DEATH_H
#define COMBAT_DEATH_H

#include "domain_events.h"
#include "structs.h"

/* Why a character died, recorded on the death event for listeners that need
 * to tell combat kills from script, attrition, and staff-driven deaths. */
enum combat_death_cause
{
  COMBAT_DEATH_UNSPECIFIED = 0,
  COMBAT_DEATH_COMBAT,
  COMBAT_DEATH_SCRIPT,
  COMBAT_DEATH_ATTRITION,
  COMBAT_DEATH_ADMINISTRATIVE
};

/* Structured result of a death submission.  processed is false when there was
 * no victim to kill. */
struct combat_death_result
{
  struct domain_entity_handle victim;
  struct domain_entity_handle killer;
  enum combat_death_cause cause;
  bool processed;
};

/* Kill a character with an explicit cause; killer may be NULL. */
struct combat_death_result combat_death_apply(struct char_data *victim, struct char_data *killer,
                                              enum combat_death_cause cause);

#endif /* COMBAT_DEATH_H */
