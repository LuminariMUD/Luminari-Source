#include "conf.h"
#include "sysdep.h"
#include "domain_object_transfer.h"
#include "utils.h"
#include "db.h"
#include "domain_event_runtime.h"
#include "domain_event_world.h"

static uint64_t next_transfer_id = 1;
static struct domain_transfer_context *current_context;
static struct domain_object_transfer_operation *current_operation;

static struct domain_object_holder empty_holder(void)
{
  struct domain_object_holder holder = {0};

  holder.slot = -1;
  return holder;
}

struct domain_object_holder domain_object_holder(const struct obj_data *object)
{
  struct domain_object_holder holder = empty_holder();

  if (object == NULL)
    return holder;
  if (object->worn_by != NULL)
  {
    holder.kind = DOMAIN_HOLDER_EQUIPMENT;
    holder.entity = domain_event_character_handle(object->worn_by);
    holder.slot = object->worn_on;
  }
  else if (object->in_obj != NULL)
  {
    holder.kind = DOMAIN_HOLDER_CONTAINER;
    holder.entity = domain_event_object_handle(object->in_obj);
  }
  else if (object->carried_by != NULL)
  {
    holder.kind = DOMAIN_HOLDER_INVENTORY;
    holder.entity = domain_event_character_handle(object->carried_by);
  }
  else if (IN_ROOM(object) != NOWHERE)
  {
    holder.kind = DOMAIN_HOLDER_ROOM;
    holder.entity = domain_event_room_handle(IN_ROOM(object));
  }
  else if (object->transfer_bag.kind == DOMAIN_HOLDER_BAG)
    holder = object->transfer_bag;
  return holder;
}

static bool same_holder(struct domain_object_holder left, struct domain_object_holder right)
{
  return left.kind == right.kind && left.slot == right.slot &&
         domain_entity_handle_equal(left.entity, right.entity);
}

static void publish_transfer(struct domain_object_moved *event)
{
  struct domain_event_bus *bus = domain_event_runtime_bus();
  struct domain_event_topic topics[4];
  size_t count = 0;
  struct domain_entity_handle entities[4];
  enum domain_event_topic_role roles[4] = {DOMAIN_EVENT_TOPIC_SUBJECT, DOMAIN_EVENT_TOPIC_SOURCE,
                                           DOMAIN_EVENT_TOPIC_DESTINATION,
                                           DOMAIN_EVENT_TOPIC_OWNER};
  size_t i;

  if (bus == NULL || same_holder(event->source, event->destination))
    return;
  if (next_transfer_id == 0)
  {
    log("SYSERR: Object transfer identity exhausted.");
    return;
  }
  event->transfer_id = next_transfer_id++;
  event->from_owner = event->source.entity;
  event->to_owner = event->destination.entity;
  entities[0] = event->object;
  entities[1] = event->from_owner;
  entities[2] = event->to_owner;
  entities[3] = event->actor;
  for (i = 0; i < 4; i++)
    if (domain_entity_handle_is_valid(entities[i]))
    {
      topics[count].role = roles[i];
      topics[count++].entity = entities[i];
    }
  (void)DOMAIN_EVENT_PUBLISH_ROUTED(bus, DOMAIN_EVENT_OBJECT_MOVED, topics, count, event);
}

void domain_transfer_context_begin(struct domain_transfer_context *context, struct char_data *actor,
                                   enum domain_transfer_cause cause)
{
  context->actor = domain_event_character_handle(actor);
  context->cause =
      cause == DOMAIN_TRANSFER_COMMAND && current_context != NULL ? current_context->cause : cause;
  context->previous = current_context;
  current_context = context;
}

void domain_transfer_context_finish(struct domain_transfer_context *context)
{
  if (current_context != context)
  {
    log("SYSERR: Object transfer context finished out of order.");
    return;
  }
  current_context = context->previous;
}

void domain_object_transfer_begin(struct domain_object_transfer_operation *operation,
                                  struct obj_data *object, struct char_data *actor,
                                  enum domain_transfer_cause cause)
{
  memset(operation, 0, sizeof(*operation));
  if (object == NULL)
    return;
  operation->event.object = domain_event_object_handle(object);
  operation->event.source = domain_object_holder(object);
  operation->event.actor = domain_event_character_handle(actor);
  operation->event.cause =
      cause == DOMAIN_TRANSFER_COMMAND && current_context != NULL ? current_context->cause : cause;
  operation->previous = current_operation;
  operation->active = true;
  current_operation = operation;
}

void domain_object_transfer_finish(struct domain_object_transfer_operation *operation)
{
  struct obj_data *object;
  struct domain_object_transfer_operation *parent;

  if (!operation->active)
    return;
  if (current_operation != operation)
  {
    log("SYSERR: Object transfer operation finished out of order.");
    return;
  }
  current_operation = operation->previous;
  operation->active = false;
  object = domain_event_world_resolve_object(operation->event.object);
  if (object == NULL && !operation->extracted)
    return;
  operation->event.destination = domain_object_holder(object);
  if (!operation->extracted && operation->event.destination.kind == DOMAIN_HOLDER_NONE)
    return;
  for (parent = current_operation; parent != NULL; parent = parent->previous)
    if (domain_entity_handle_equal(parent->event.object, operation->event.object))
    {
      if (!same_holder(operation->event.source, operation->event.destination))
      {
        parent->event.actor = operation->event.actor;
        parent->event.cause = operation->event.cause;
      }
      return;
    }
  publish_transfer(&operation->event);
}

void domain_object_detaching(struct obj_data *object)
{
  if (object == NULL || object->transfer_pending)
    return;
  object->transfer_source = domain_object_holder(object);
  object->transfer_pending = true;
}

static struct domain_object_transfer_operation *find_operation(struct domain_entity_handle object)
{
  struct domain_object_transfer_operation *operation;

  for (operation = current_operation; operation != NULL; operation = operation->previous)
    if (domain_entity_handle_equal(operation->event.object, object))
      return operation;
  return NULL;
}

void domain_object_placed(struct obj_data *object)
{
  struct domain_object_moved event = {0};
  struct domain_object_transfer_operation *operation;

  if (object == NULL)
    return;
  event.object = domain_event_object_handle(object);
  event.source = object->transfer_pending ? object->transfer_source : empty_holder();
  event.destination = domain_object_holder(object);
  object->transfer_pending = false;
  operation = find_operation(event.object);
  if (operation != NULL)
  {
    if (current_context != NULL && current_context->cause != DOMAIN_TRANSFER_COMMAND)
    {
      operation->event.actor = current_context->actor;
      operation->event.cause = current_context->cause;
    }
    return;
  }
  if (current_context != NULL)
  {
    event.actor = current_context->actor;
    event.cause = current_context->cause;
  }
  publish_transfer(&event);
}

void domain_object_disposed(struct obj_data *object)
{
  struct domain_object_transfer_operation *operation;
  struct domain_object_moved event = {0};
  bool captured = false;

  if (object == NULL || object->transfer_disposed)
    return;
  object->transfer_disposed = true;
  event.object = domain_event_object_handle(object);
  for (operation = current_operation; operation != NULL; operation = operation->previous)
    if (domain_entity_handle_equal(operation->event.object, event.object))
    {
      operation->extracted = true;
      operation->event.cause = DOMAIN_TRANSFER_EXTRACT;
      captured = true;
    }
  event.source = object->transfer_pending ? object->transfer_source : domain_object_holder(object);
  event.destination = empty_holder();
  event.cause = DOMAIN_TRANSFER_EXTRACT;
  if (current_context != NULL)
    event.actor = current_context->actor;
  object->transfer_pending = false;
  if (!captured)
    publish_transfer(&event);
}
