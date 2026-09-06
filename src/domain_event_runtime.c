#include "domain_event_runtime.h"

#include "active_world.h"
#include "utils.h"
#include "activity_manager.h"
#include "ai_service.h"
#include "affected_owners.h"
#include "character_periodic.h"
#include "combat/combat_encounters.h"
#include "domain_event_types.h"
#include "domain_event_world.h"
#include "dgscript/dg_event.h"
#include "dgscript/dg_scripts.h"
#include "event_runtime.h"
#include "mud_event.h"
#include "periodic_owners.h"
#include "point_update_periodic.h"
#include "ready_action.h"
#include "quest/quest.h"
#include "vessels/vessel_periodic.h"
#include "wilderness/spatial_events.h"

static struct domain_event_bus *runtime_bus;
static struct domain_relocation_operation *relocation_top;

static void append_topic(struct domain_event_topic *topics, size_t *count,
                         enum domain_event_topic_role role, struct domain_entity_handle entity)
{
  if (!domain_entity_handle_is_valid(entity) || *count >= DOMAIN_EVENT_MAX_PUBLICATION_TOPICS)
    return;
  topics[*count].role = role;
  topics[*count].entity = entity;
  (*count)++;
}

enum domain_event_status domain_event_runtime_init(void)
{
  enum game_scheduler_status scheduler_status;
  enum domain_event_status status;
  size_t event_type_checkpoint;
  bool event_type_transaction;

  if (runtime_bus != NULL)
    return DOMAIN_EVENT_BUSY;
  runtime_bus = domain_event_bus_create(NULL, &status);
  if (runtime_bus == NULL)
    return status;
  event_type_checkpoint = 0U;
  event_type_transaction = false;
  if (event_backend_current() == EVENT_BACKEND_GAME_SCHEDULER)
  {
    scheduler_status = event_runtime_registration_checkpoint(&event_type_checkpoint);
    if (scheduler_status != GAME_SCHEDULER_OK)
      status = DOMAIN_EVENT_ALLOCATION_FAILED;
    else
      event_type_transaction = true;
  }
  if (status == DOMAIN_EVENT_OK)
    periodic_owners_init();
  if (event_backend_current() == EVENT_BACKEND_GAME_SCHEDULER)
  {
    if (status == DOMAIN_EVENT_OK && !dg_wait_runtime_init())
      status = DOMAIN_EVENT_ALLOCATION_FAILED;
    if (status == DOMAIN_EVENT_OK && !mud_event_runtime_init())
      status = DOMAIN_EVENT_ALLOCATION_FAILED;
    if (status == DOMAIN_EVENT_OK && !ai_events_runtime_init())
      status = DOMAIN_EVENT_ALLOCATION_FAILED;
  }
  if (status == DOMAIN_EVENT_OK && !ready_action_runtime_init())
    status = DOMAIN_EVENT_ALLOCATION_FAILED;
  affected_owners_init();
  character_periodic_init();
  point_update_periodic_init();
  vessel_periodic_init();
#ifndef LUMINARI_CUTEST
  /* Focused tests may deliberately disable unrelated owner services. */
  if (!periodic_autoproc_enabled() || !periodic_dg_random_enabled() ||
      !affected_owner_events_enabled() || !character_periodic_events_enabled() ||
      !point_update_events_enabled() || (CONFIG_VESSEL_SYSTEM && !vessel_periodic_events_enabled()))
    status = DOMAIN_EVENT_ALLOCATION_FAILED;
#endif
  if (status == DOMAIN_EVENT_OK)
    status = domain_event_register_foundation_types(runtime_bus);
  if (status == DOMAIN_EVENT_OK)
    status = domain_event_world_register_resolvers(runtime_bus);
  if (status == DOMAIN_EVENT_OK)
    status = primary_activity_manager_init(runtime_bus);
  if (status == DOMAIN_EVENT_OK)
    status = combat_encounter_runtime_init(runtime_bus);
  if (status == DOMAIN_EVENT_OK)
    status = character_periodic_register_handlers(runtime_bus);
  if (status == DOMAIN_EVENT_OK)
    status = quest_register_commit_handlers(runtime_bus);
  if (status == DOMAIN_EVENT_OK)
    status = spatial_event_register_handlers(runtime_bus);
  if (status == DOMAIN_EVENT_OK)
    status = active_world_register_handlers(runtime_bus);
  if (status == DOMAIN_EVENT_OK)
    status = domain_event_seal(runtime_bus);
  if (status != DOMAIN_EVENT_OK)
  {
    primary_activity_manager_shutdown();
    active_world_shutdown();
    combat_encounter_runtime_shutdown();
    periodic_owners_shutdown();
    affected_owners_shutdown();
    character_periodic_shutdown();
    point_update_periodic_shutdown();
    vessel_periodic_shutdown();
    ready_action_runtime_shutdown();
    if (event_type_transaction &&
        event_runtime_rollback_type_registrations(event_type_checkpoint) != GAME_SCHEDULER_OK)
      log("SYSERR: Unable to roll back partial timed-event type registration.");
    domain_event_bus_destroy(runtime_bus);
    domain_event_world_shutdown();
    runtime_bus = NULL;
  }
  return status;
}

enum domain_event_status domain_event_runtime_shutdown(void)
{
  enum domain_event_status status;

  if (runtime_bus == NULL)
    return DOMAIN_EVENT_OK;
  primary_activity_manager_shutdown();
  active_world_shutdown();
  combat_encounter_runtime_shutdown();
  periodic_owners_shutdown();
  affected_owners_shutdown();
  character_periodic_shutdown();
  point_update_periodic_shutdown();
  vessel_periodic_shutdown();
  status = domain_event_bus_destroy(runtime_bus);
  if (status == DOMAIN_EVENT_OK)
  {
    ready_action_runtime_shutdown();
    domain_event_world_shutdown();
    runtime_bus = NULL;
  }
  return status;
}

struct domain_event_bus *domain_event_runtime_bus(void)
{
  return runtime_bus;
}

static void publish_relocation(const struct domain_character_moved *event)
{
  struct domain_event_topic topics[4];
  size_t count = 0;

  if (runtime_bus == NULL || domain_entity_handle_equal(event->from_room, event->to_room))
    return;
  append_topic(topics, &count, DOMAIN_EVENT_TOPIC_SUBJECT, event->character);
  append_topic(topics, &count, DOMAIN_EVENT_TOPIC_SOURCE, event->from_room);
  append_topic(topics, &count, DOMAIN_EVENT_TOPIC_DESTINATION, event->to_room);
  append_topic(topics, &count, DOMAIN_EVENT_TOPIC_OWNER, event->actor);
  (void)DOMAIN_EVENT_PUBLISH_ROUTED(runtime_bus, DOMAIN_EVENT_CHARACTER_MOVED, topics, count,
                                    event);
}

void domain_relocation_begin(struct domain_relocation_operation *operation, struct char_data *ch,
                             struct char_data *actor, enum domain_relocation_cause cause,
                             int direction)
{
  struct domain_relocation_operation *parent;

  memset(operation, 0, sizeof(*operation));
  if (ch == NULL)
    return;
  operation->event.character = domain_event_character_handle(ch);
  operation->event.from_room = domain_event_room_handle(IN_ROOM(ch));
  operation->event.actor = domain_event_character_handle(actor);
  operation->event.cause = cause;
  operation->event.direction = direction;
  /* Locomotion used by a forced operation retains its initiating cause. */
  if (cause == DOMAIN_RELOCATION_WALK)
    for (parent = relocation_top; parent != NULL; parent = parent->previous)
      if (domain_entity_handle_equal(parent->event.character, operation->event.character))
      {
        operation->event.cause = parent->event.cause;
        operation->event.actor = parent->event.actor;
        break;
      }
  operation->previous = relocation_top;
  operation->active = true;
  relocation_top = operation;
}

void domain_relocation_finish(struct domain_relocation_operation *operation)
{
  struct domain_relocation_operation *parent;
  struct char_data *ch;

  if (!operation->active)
    return;
  if (relocation_top != operation)
  {
    log("SYSERR: Relocation operations finished out of order.");
    return;
  }
  relocation_top = operation->previous;
  operation->active = false;
  ch = domain_event_world_resolve_character(operation->event.character);
  if (ch == NULL)
    return;
  operation->event.to_room = domain_event_room_handle(IN_ROOM(ch));
  if (domain_entity_handle_equal(operation->event.from_room, operation->event.to_room))
    return;
  for (parent = relocation_top; parent != NULL; parent = parent->previous)
  {
    if (domain_entity_handle_equal(parent->event.character, operation->event.character))
    {
      parent->event.actor = operation->event.actor;
      parent->event.cause = operation->event.cause;
      parent->event.direction = operation->event.direction;
      return;
    }
  }
  publish_relocation(&operation->event);
}

void domain_relocation_placed(struct char_data *ch, struct domain_entity_handle from_room,
                              room_rnum to_room, struct char_data *actor,
                              enum domain_relocation_cause cause, int direction)
{
  struct domain_character_moved event;
  struct domain_relocation_operation *operation;

  if (ch == NULL || runtime_bus == NULL)
    return;
  memset(&event, 0, sizeof(event));
  event.character = domain_event_character_handle(ch);
  for (operation = relocation_top; operation != NULL; operation = operation->previous)
    if (domain_entity_handle_equal(operation->event.character, event.character))
    {
      if (cause != DOMAIN_RELOCATION_UNKNOWN)
      {
        operation->event.cause = cause;
        operation->event.actor = domain_event_character_handle(actor);
        operation->event.direction = direction;
      }
      return;
    }
  event.from_room = from_room;
  event.to_room = domain_event_room_handle(to_room);
  event.actor = domain_event_character_handle(actor);
  event.cause = !domain_entity_handle_is_valid(from_room) && cause == DOMAIN_RELOCATION_UNKNOWN
                    ? DOMAIN_RELOCATION_SPAWN
                    : cause;
  event.direction = direction;
  publish_relocation(&event);
}

enum domain_event_status domain_event_runtime_character_moved(struct char_data *ch,
                                                              room_rnum from_room,
                                                              room_rnum to_room, int direction)
{
  struct domain_character_moved event;
  struct domain_event_topic topics[3];
  size_t topic_count = 0U;

  if (runtime_bus == NULL || ch == NULL)
    return DOMAIN_EVENT_NOT_FOUND;
  memset(&event, 0, sizeof(event));
  event.character = domain_event_character_handle(ch);
  event.from_room = domain_event_room_handle(from_room);
  event.to_room = domain_event_room_handle(to_room);
  event.direction = direction;
  append_topic(topics, &topic_count, DOMAIN_EVENT_TOPIC_SUBJECT, event.character);
  append_topic(topics, &topic_count, DOMAIN_EVENT_TOPIC_SOURCE, event.from_room);
  append_topic(topics, &topic_count, DOMAIN_EVENT_TOPIC_DESTINATION, event.to_room);
  return DOMAIN_EVENT_PUBLISH_ROUTED(runtime_bus, DOMAIN_EVENT_CHARACTER_MOVED, topics, topic_count,
                                     &event);
}

enum domain_event_status domain_event_runtime_character_damaged(struct char_data *target,
                                                                struct char_data *source,
                                                                int amount, int damage_type)
{
  struct domain_character_damaged event;
  struct domain_event_topic topics[2];
  size_t topic_count = 0U;

  if (runtime_bus == NULL || target == NULL)
    return DOMAIN_EVENT_NOT_FOUND;
  event.target = domain_event_character_handle(target);
  event.source = domain_event_character_handle(source);
  event.amount = amount;
  event.damage_type = damage_type;
  append_topic(topics, &topic_count, DOMAIN_EVENT_TOPIC_SUBJECT, event.target);
  append_topic(topics, &topic_count, DOMAIN_EVENT_TOPIC_SOURCE, event.source);
  return DOMAIN_EVENT_PUBLISH_ROUTED(runtime_bus, DOMAIN_EVENT_CHARACTER_DAMAGED, topics,
                                     topic_count, &event);
}

enum domain_event_status domain_event_runtime_combat_state_changed(struct char_data *ch,
                                                                   struct char_data *opponent,
                                                                   bool in_combat)
{
  struct domain_combat_state_changed event;
  struct domain_event_topic topics[2];
  size_t topic_count = 0U;

  if (runtime_bus == NULL || ch == NULL)
    return DOMAIN_EVENT_NOT_FOUND;
  event.character = domain_event_character_handle(ch);
  event.opponent = domain_event_character_handle(opponent);
  event.in_combat = in_combat;
  append_topic(topics, &topic_count, DOMAIN_EVENT_TOPIC_SUBJECT, event.character);
  append_topic(topics, &topic_count, DOMAIN_EVENT_TOPIC_SOURCE, event.opponent);
  return DOMAIN_EVENT_PUBLISH_ROUTED(runtime_bus, DOMAIN_EVENT_COMBAT_STATE_CHANGED, topics,
                                     topic_count, &event);
}

/* Publish a character-death event with no specific cause recorded. */
enum domain_event_status domain_event_runtime_character_died(struct char_data *ch,
                                                             struct char_data *killer)
{
  return domain_event_runtime_character_died_with_cause(ch, killer, 0U);
}

/* Publish a character-death event carrying the cause of death.
 * cause is a combat_death_cause value widened to uint32_t so the event layer
 * stays independent of the combat headers. */
enum domain_event_status domain_event_runtime_character_died_with_cause(struct char_data *ch,
                                                                        struct char_data *killer,
                                                                        uint32_t cause)
{
  struct domain_character_died event;
  struct domain_event_topic topics[2];
  size_t topic_count = 0U;

  if (runtime_bus == NULL || ch == NULL)
    return DOMAIN_EVENT_NOT_FOUND;
  event.character = domain_event_character_handle(ch);
  event.killer = domain_event_character_handle(killer);
  event.cause = cause;
  append_topic(topics, &topic_count, DOMAIN_EVENT_TOPIC_SUBJECT, event.character);
  append_topic(topics, &topic_count, DOMAIN_EVENT_TOPIC_SOURCE, event.killer);
  return DOMAIN_EVENT_PUBLISH_ROUTED(runtime_bus, DOMAIN_EVENT_CHARACTER_DIED, topics, topic_count,
                                     &event);
}

enum domain_event_status domain_event_runtime_character_extracted(struct char_data *ch,
                                                                  uint32_t reason)
{
  struct domain_entity_extracted event;
  struct game_event_owner owner;
  enum domain_event_status status;
  struct domain_event_topic topic;
  size_t cancelled;

  if (runtime_bus == NULL || ch == NULL)
    return DOMAIN_EVENT_NOT_FOUND;
  event.entity = domain_event_character_handle(ch);
  event.reason = reason;
  topic.role = DOMAIN_EVENT_TOPIC_SUBJECT;
  topic.entity = event.entity;
  status =
      DOMAIN_EVENT_PUBLISH_ROUTED(runtime_bus, DOMAIN_EVENT_ENTITY_EXTRACTED, &topic,
                                  domain_entity_handle_is_valid(event.entity) ? 1U : 0U, &event);
  if (domain_entity_handle_is_valid(event.entity))
  {
    cancelled = 0U;
    (void)domain_event_unsubscribe_owner(runtime_bus, event.entity, &cancelled);
    owner = game_event_owner_none();
    owner.kind = GAME_EVENT_OWNER_CHARACTER;
    owner.runtime_id = event.entity.runtime_id;
    owner.generation = event.entity.generation;
    cancelled = 0U;
    (void)event_runtime_cancel_owner(owner, &cancelled);
  }
  return status;
}
