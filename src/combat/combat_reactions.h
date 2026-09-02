#ifndef COMBAT_REACTIONS_H
#define COMBAT_REACTIONS_H

#include "domain_events.h"
#include "structs.h"

/* Safety bound on pending reactive damage packets.  Reaction chains beyond
 * this are dropped rather than allowed to grow without limit. */
#define COMBAT_REACTION_CAPACITY 64U

/* One deferred reactive damage packet.  Participants are stored as entity
 * handles so extraction between enqueue and drain is detected, not
 * dereferenced. */
struct combat_reaction_damage
{
  struct domain_entity_handle source;
  struct domain_entity_handle target;
  int amount;
  int ability;
  int damage_type;
  int attack_type;
};

/* Bounded FIFO of reactive damage owned by the outermost damage call.
 * count is live occupancy and falls as packets drain; scheduled is the
 * lifetime total enqueued and never falls, so it -- not count -- is what bounds
 * a self-feeding reaction chain (A reflects onto B, B reflects back onto A).
 * dropped counts packets refused at that bound; stale counts packets discarded
 * because a participant left the world. */
struct combat_reaction_queue
{
  struct combat_reaction_damage damage[COMBAT_REACTION_CAPACITY];
  size_t head;
  size_t count;
  size_t scheduled;
  size_t dropped;
  size_t stale;
};

/* Result of taking the next packet off a reaction queue. */
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
enum combat_reaction_dequeue_status
combat_reaction_dequeue_damage(struct combat_reaction_queue *queue,
                               struct combat_reaction_damage *damage, struct char_data **source,
                               struct char_data **target);

#endif /* COMBAT_REACTIONS_H */
