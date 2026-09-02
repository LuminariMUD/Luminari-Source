#ifndef COMBAT_DAMAGE_H
#define COMBAT_DAMAGE_H

#include "domain_events.h"
#include "structs.h"

/* Outcome of a damage submission.
 * REJECTED   - the packet could not be applied or scheduled at all.
 * NO_EFFECT  - it was applied but changed nothing.
 * APPLIED    - hit points were removed and the target survived.
 * QUEUED     - deferred onto the active reaction queue, not yet applied.
 * TARGET_DIED- the target died from this packet. */
enum combat_damage_status
{
  COMBAT_DAMAGE_REJECTED = 0,
  COMBAT_DAMAGE_NO_EFFECT,
  COMBAT_DAMAGE_APPLIED,
  COMBAT_DAMAGE_QUEUED,
  COMBAT_DAMAGE_TARGET_DIED
};

/* Structured result of one damage submission.  legacy_result carries the
 * value the old damage() signature would have returned. */
struct combat_damage_result
{
  enum combat_damage_status status;
  struct domain_entity_handle source;
  struct domain_entity_handle target;
  int requested;
  int applied;
  int legacy_result;
};

/* Constructors mapping a raw outcome onto a typed result. */
struct combat_damage_result combat_damage_result_from_legacy(struct char_data *source,
                                                             struct char_data *target,
                                                             int requested, int legacy_result);
struct combat_damage_result combat_damage_result_queued(struct char_data *source,
                                                        struct char_data *target, int requested);
struct combat_damage_result combat_damage_result_rejected(struct char_data *source,
                                                          struct char_data *target, int requested);
/* Apply damage, draining any reactive damage it provokes on a bounded queue. */
struct combat_damage_result combat_damage_apply(struct char_data *source, struct char_data *target,
                                                int amount, int ability, int damage_type,
                                                int attack_type);

#endif /* COMBAT_DAMAGE_H */
