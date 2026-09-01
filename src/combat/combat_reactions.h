#ifndef COMBAT_REACTIONS_H
#define COMBAT_REACTIONS_H

#include "domain_events.h"
#include "structs.h"

#define COMBAT_REACTION_CAPACITY 64U

struct combat_reaction_damage
{
  struct domain_entity_handle source;
  struct domain_entity_handle target;
  int amount;
  int ability;
  int damage_type;
  int attack_type;
};

struct combat_reaction_queue
{
  struct combat_reaction_damage damage[COMBAT_REACTION_CAPACITY];
  size_t head;
  size_t count;
  size_t scheduled;
  size_t dropped;
  size_t stale;
};

enum combat_reaction_dequeue_status
{
  COMBAT_REACTION_DEQUEUE_EMPTY = 0,
  COMBAT_REACTION_DEQUEUE_READY,
  COMBAT_REACTION_DEQUEUE_STALE
};

void combat_reaction_queue_init(struct combat_reaction_queue *queue);
bool combat_reaction_enqueue_damage(struct combat_reaction_queue *queue, struct char_data *source,
                                    struct char_data *target, int amount, int ability,
                                    int damage_type, int attack_type);
enum combat_reaction_dequeue_status combat_reaction_dequeue_damage(
    struct combat_reaction_queue *queue, struct combat_reaction_damage *damage,
    struct char_data **source, struct char_data **target);

#endif /* COMBAT_REACTIONS_H */
