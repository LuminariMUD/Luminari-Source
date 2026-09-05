#include "domain_event_world.h"

#include "db.h"
#include "domain_event_runtime.h"
#include "domain_event_types.h"

#define DOMAIN_WORLD_REGISTRY_BUCKETS 16384U

struct domain_world_registry_entry
{
  uint64_t runtime_id;
  uint64_t generation;
  void *entity;
  struct domain_world_registry_entry *next;
};

static uint64_t next_room_generation = 1U;
static uint64_t next_character_generation = 1U;
static uint64_t next_object_generation = 1U;
static struct domain_world_registry_entry *character_registry[DOMAIN_WORLD_REGISTRY_BUCKETS];
static struct domain_world_registry_entry *object_registry[DOMAIN_WORLD_REGISTRY_BUCKETS];

static size_t registry_bucket(uint64_t runtime_id)
{
  runtime_id ^= runtime_id >> 33U;
  runtime_id *= UINT64_C(0xff51afd7ed558ccd);
  runtime_id ^= runtime_id >> 33U;
  return (size_t)runtime_id & (DOMAIN_WORLD_REGISTRY_BUCKETS - 1U);
}

static bool registry_register(struct domain_world_registry_entry **registry, uint64_t runtime_id,
                              uint64_t generation, void *entity)
{
  struct domain_world_registry_entry *entry;
  size_t bucket;

  bucket = registry_bucket(runtime_id);
  for (entry = registry[bucket]; entry != NULL; entry = entry->next)
  {
    if (entry->runtime_id != runtime_id)
      continue;
    entry->generation = generation;
    entry->entity = entity;
    return true;
  }
  entry = malloc(sizeof(*entry));
  if (entry == NULL)
    return false;
  entry->runtime_id = runtime_id;
  entry->generation = generation;
  entry->entity = entity;
  entry->next = registry[bucket];
  registry[bucket] = entry;
  return true;
}

static void *registry_resolve(struct domain_world_registry_entry **registry, uint64_t runtime_id,
                              uint64_t generation)
{
  struct domain_world_registry_entry *entry;

  for (entry = registry[registry_bucket(runtime_id)]; entry != NULL; entry = entry->next)
    if (entry->runtime_id == runtime_id && entry->generation == generation)
      return entry->entity;
  return NULL;
}

static void registry_forget(struct domain_world_registry_entry **registry, void *entity)
{
  struct domain_world_registry_entry **cursor;
  struct domain_world_registry_entry *entry;
  uint64_t runtime_id;

  if (entity == NULL)
    return;
  runtime_id = (uint64_t)(uintptr_t)entity;
  cursor = &registry[registry_bucket(runtime_id)];
  while (*cursor != NULL)
  {
    entry = *cursor;
    if (entry->runtime_id == runtime_id && entry->entity == entity)
    {
      *cursor = entry->next;
      free(entry);
      return;
    }
    cursor = &entry->next;
  }
}

static void registry_clear(struct domain_world_registry_entry **registry)
{
  struct domain_world_registry_entry *entry;
  struct domain_world_registry_entry *next;
  size_t bucket;

  for (bucket = 0U; bucket < DOMAIN_WORLD_REGISTRY_BUCKETS; bucket++)
  {
    for (entry = registry[bucket]; entry != NULL; entry = next)
    {
      next = entry->next;
      free(entry);
    }
    registry[bucket] = NULL;
  }
}

static void *resolve_room(struct domain_entity_handle handle, void *resolver_context)
{
  room_vnum vnum;
  room_rnum room;

  (void)resolver_context;
  if (handle.runtime_id == 0U || handle.runtime_id - 1U > INT32_MAX)
    return NULL;
  vnum = (room_vnum)(handle.runtime_id - 1U);
  room = real_room(vnum);
  if (room == NOWHERE || world[room].event_owner_generation != handle.generation)
    return NULL;
  return &world[room];
}

static void *resolve_character(struct domain_entity_handle handle, void *resolver_context)
{
  (void)resolver_context;
  return domain_event_world_resolve_character(handle);
}

static void *resolve_object(struct domain_entity_handle handle, void *resolver_context)
{
  (void)resolver_context;
  if (handle.runtime_id == 0U)
    return NULL;
  return registry_resolve(object_registry, handle.runtime_id, handle.generation);
}

enum domain_event_status domain_event_world_register_resolvers(struct domain_event_bus *bus)
{
  enum domain_event_status status;

  status = domain_event_register_resolver(bus, DOMAIN_ENTITY_ROOM, resolve_room, NULL);
  if (status != DOMAIN_EVENT_OK)
    return status;
  status = domain_event_register_resolver(bus, DOMAIN_ENTITY_CHARACTER, resolve_character, NULL);
  if (status != DOMAIN_EVENT_OK)
    return status;
  return domain_event_register_resolver(bus, DOMAIN_ENTITY_OBJECT, resolve_object, NULL);
}

void domain_event_world_shutdown(void)
{
  registry_clear(character_registry);
  registry_clear(object_registry);
}

void domain_event_world_forget_character(struct char_data *ch)
{
  registry_forget(character_registry, ch);
}

void domain_event_world_forget_object(struct obj_data *obj)
{
  struct domain_entity_handle owner;
  struct domain_event_bus *bus = domain_event_runtime_bus();

  if (obj != NULL && obj->event_owner_generation != 0U && bus != NULL)
  {
    owner.kind = DOMAIN_ENTITY_OBJECT;
    owner.runtime_id = (uint64_t)(uintptr_t)obj;
    owner.generation = obj->event_owner_generation;
    {
      struct domain_entity_extracted event = {owner, 0U};
      struct domain_event_topic topic = {DOMAIN_EVENT_TOPIC_SUBJECT, owner};

      (void)DOMAIN_EVENT_PUBLISH_ROUTED(bus, DOMAIN_EVENT_ENTITY_EXTRACTED, &topic, 1U, &event);
    }
    (void)domain_event_unsubscribe_owner(bus, owner, NULL);
  }
  registry_forget(object_registry, obj);
}

struct char_data *domain_event_world_resolve_character(struct domain_entity_handle handle)
{
  if (!domain_entity_handle_is_valid(handle) || handle.kind != DOMAIN_ENTITY_CHARACTER)
    return NULL;
  return registry_resolve(character_registry, handle.runtime_id, handle.generation);
}

struct domain_entity_handle domain_event_character_handle(struct char_data *ch)
{
  struct domain_entity_handle handle = domain_entity_handle_none();

  if (ch == NULL)
    return handle;
  if (ch->domain_event_generation == 0U)
  {
    if (next_character_generation == 0U)
      return handle;
    ch->domain_event_generation = next_character_generation++;
  }
  if (!registry_register(character_registry, (uint64_t)(uintptr_t)ch, ch->domain_event_generation,
                         ch))
    return handle;
  handle.kind = DOMAIN_ENTITY_CHARACTER;
  handle.runtime_id = (uint64_t)(uintptr_t)ch;
  handle.generation = ch->domain_event_generation;
  return handle;
}

struct domain_entity_handle domain_event_room_handle(room_rnum room)
{
  struct domain_entity_handle handle = domain_entity_handle_none();

  if (room == NOWHERE || room > top_of_world)
    return handle;
  if (world[room].event_owner_generation == 0U)
  {
    if (next_room_generation == 0U)
      return handle;
    world[room].event_owner_generation = next_room_generation++;
  }
  handle.kind = DOMAIN_ENTITY_ROOM;
  handle.runtime_id = (uint64_t)(uint32_t)GET_ROOM_VNUM(room) + 1U;
  handle.generation = world[room].event_owner_generation;
  return handle;
}

struct domain_entity_handle domain_event_object_handle(struct obj_data *obj)
{
  struct domain_entity_handle handle = domain_entity_handle_none();

  if (obj == NULL)
    return handle;
  if (obj->event_owner_generation == 0U)
  {
    if (next_object_generation == 0U)
      return handle;
    obj->event_owner_generation = next_object_generation++;
  }
  if (!registry_register(object_registry, (uint64_t)(uintptr_t)obj, obj->event_owner_generation,
                         obj))
    return handle;
  handle.kind = DOMAIN_ENTITY_OBJECT;
  handle.runtime_id = (uint64_t)(uintptr_t)obj;
  handle.generation = obj->event_owner_generation;
  return handle;
}
