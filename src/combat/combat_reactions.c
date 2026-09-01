#include "combat/combat_reactions.h"

#include "domain_event_world.h"

void combat_reaction_queue_init(struct combat_reaction_queue *queue)
{
  if (queue != NULL)
    memset(queue, 0, sizeof(*queue));
}

bool combat_reaction_enqueue_damage(struct combat_reaction_queue *queue, struct char_data *source,
                                    struct char_data *target, int amount, int ability,
                                    int damage_type, int attack_type)
{
  struct combat_reaction_damage *damage;
  size_t tail;

  if (queue == NULL || source == NULL || target == NULL || amount < 0)
    return false;
  if (queue->count >= COMBAT_REACTION_CAPACITY || queue->scheduled >= COMBAT_REACTION_CAPACITY)
  {
    queue->dropped++;
    return false;
  }

  tail = (queue->head + queue->count) % COMBAT_REACTION_CAPACITY;
  damage = &queue->damage[tail];
  damage->source = domain_event_character_handle(source);
  damage->target = domain_event_character_handle(target);
  if (!domain_entity_handle_is_valid(damage->source) ||
      !domain_entity_handle_is_valid(damage->target))
    return false;
  damage->amount = amount;
  damage->ability = ability;
  damage->damage_type = damage_type;
  damage->attack_type = attack_type;
  queue->count++;
  queue->scheduled++;
  return true;
}

enum combat_reaction_dequeue_status combat_reaction_dequeue_damage(
    struct combat_reaction_queue *queue, struct combat_reaction_damage *damage,
    struct char_data **source, struct char_data **target)
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
