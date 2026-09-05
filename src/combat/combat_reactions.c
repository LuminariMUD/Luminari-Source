#include "combat/combat_reactions.h"

#include "domain_event_world.h"

/* Reset a reaction queue to an empty, unscheduled state.
 * Safe to call on a NULL queue. */
void combat_reaction_queue_init(struct combat_reaction_queue *queue)
{
  if (queue != NULL)
    memset(queue, 0, sizeof(*queue));
}

/* Schedule one reactive damage packet for later processing.
 * Participants are stored as entity handles so an extraction between enqueue
 * and drain is detected instead of dereferenced. Returns false when the packet
 * cannot be scheduled: handles that no longer resolve are counted in
 * queue->stale, packets refused at the safety bound in queue->dropped. */
bool combat_reaction_enqueue_damage(struct combat_reaction_queue *queue, struct char_data *source,
                                    struct char_data *target, int amount, int ability,
                                    int damage_type, int attack_type)
{
  struct combat_reaction_damage *damage;
  struct domain_entity_handle source_handle;
  struct domain_entity_handle target_handle;
  size_t tail;

  if (queue == NULL || source == NULL || target == NULL || amount < 0)
    return false;

  source_handle = domain_event_character_handle(source);
  target_handle = domain_event_character_handle(target);
  if (!domain_entity_handle_is_valid(source_handle) ||
      !domain_entity_handle_is_valid(target_handle))
  {
    queue->stale++;
    return false;
  }

  if (queue->count >= COMBAT_REACTION_CAPACITY || queue->scheduled >= COMBAT_REACTION_CAPACITY)
  {
    queue->dropped++;
    return false;
  }

  tail = (queue->head + queue->count) % COMBAT_REACTION_CAPACITY;
  damage = &queue->damage[tail];
  damage->source = source_handle;
  damage->target = target_handle;
  damage->amount = amount;
  damage->ability = ability;
  damage->damage_type = damage_type;
  damage->attack_type = attack_type;
  queue->count++;
  queue->scheduled++;
  return true;
}

/* Take the oldest scheduled packet off the queue.
 * Reports EMPTY when nothing is pending, STALE when either participant has
 * since been extracted (the packet is discarded and counted in queue->stale),
 * and READY with both characters resolved otherwise. */
enum combat_reaction_dequeue_status
combat_reaction_dequeue_damage(struct combat_reaction_queue *queue,
                               struct combat_reaction_damage *damage, struct char_data **source,
                               struct char_data **target)
{
  if (queue == NULL || damage == NULL || source == NULL || target == NULL || queue->count == 0U)
    return COMBAT_REACTION_DEQUEUE_EMPTY;

  *damage = queue->damage[queue->head];
  queue->head = (queue->head + 1U) % COMBAT_REACTION_CAPACITY;
  queue->count--;
  *source = domain_event_world_resolve_character(damage->source);
  *target = domain_event_world_resolve_character(damage->target);
  if (*source == NULL || *target == NULL)
  {
    queue->stale++;
    return COMBAT_REACTION_DEQUEUE_STALE;
  }
  return COMBAT_REACTION_DEQUEUE_READY;
}
