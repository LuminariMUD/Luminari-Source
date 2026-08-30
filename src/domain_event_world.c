#include "domain_event_world.h"

#include "db.h"

static uint64_t next_room_generation = 1U;

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

enum domain_event_status domain_event_world_register_resolvers(struct domain_event_bus *bus)
{
  return domain_event_register_resolver(bus, DOMAIN_ENTITY_ROOM, resolve_room, NULL);
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
