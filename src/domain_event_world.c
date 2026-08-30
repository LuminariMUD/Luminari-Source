#include "domain_event_world.h"

#include "db.h"

static uint64_t next_room_generation = 1U;
static uint64_t next_character_generation = 1U;

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
  struct char_data *ch;

  (void)resolver_context;
  if (handle.runtime_id == 0U)
    return NULL;
  for (ch = character_list; ch != NULL; ch = ch->next)
    if ((uint64_t)(uintptr_t)ch == handle.runtime_id &&
        ch->domain_event_generation == handle.generation)
      return ch;
  return NULL;
}

enum domain_event_status domain_event_world_register_resolvers(struct domain_event_bus *bus)
{
  enum domain_event_status status;

  status = domain_event_register_resolver(bus, DOMAIN_ENTITY_ROOM, resolve_room, NULL);
  if (status != DOMAIN_EVENT_OK)
    return status;
  return domain_event_register_resolver(bus, DOMAIN_ENTITY_CHARACTER, resolve_character, NULL);
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
