#ifndef COMBAT_DAMAGE_H
#define COMBAT_DAMAGE_H

#include "domain_events.h"
#include "structs.h"

enum combat_damage_status
{
  COMBAT_DAMAGE_REJECTED = 0,
  COMBAT_DAMAGE_NO_EFFECT,
  COMBAT_DAMAGE_APPLIED,
  COMBAT_DAMAGE_QUEUED,
  COMBAT_DAMAGE_TARGET_DIED
};

struct combat_damage_result
{
  enum combat_damage_status status;
  struct domain_entity_handle source;
  struct domain_entity_handle target;
  int requested;
  int applied;
  int legacy_result;
};

struct combat_damage_result combat_damage_result_from_legacy(struct char_data *source,
                                                             struct char_data *target,
                                                             int requested, int legacy_result);
struct combat_damage_result combat_damage_result_queued(struct char_data *source,
                                                        struct char_data *target, int requested);
struct combat_damage_result combat_damage_result_rejected(struct char_data *source,
                                                          struct char_data *target, int requested);
struct combat_damage_result combat_damage_apply(struct char_data *source, struct char_data *target,
                                                int amount, int ability, int damage_type,
                                                int attack_type);

#endif /* COMBAT_DAMAGE_H */
