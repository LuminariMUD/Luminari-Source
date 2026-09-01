#include "domain_event_runtime.h"

#include "active_world.h"
#include "activity_manager.h"
#include "affected_owners.h"
#include "character_periodic.h"
#include "combat/combat_encounters.h"
#include "domain_event_types.h"
#include "domain_event_world.h"
#include "dgscript/dg_event.h"
#include "dgscript/dg_scripts.h"
#include "mud_event.h"
#include "periodic_owners.h"
#include "point_update_periodic.h"
#include "vessels/vessel_periodic.h"
#include "wilderness/spatial_events.h"

static struct domain_event_bus *runtime_bus;

enum domain_event_status domain_event_runtime_init(void)
{
  enum domain_event_status status;

  if (runtime_bus != NULL)
    return DOMAIN_EVENT_BUSY;
  runtime_bus = domain_event_bus_create(NULL, &status);
  if (runtime_bus == NULL)
    return status;
  periodic_owners_init();
  (void)dg_wait_runtime_init();
  if (event_backend_current() == EVENT_BACKEND_GAME_SCHEDULER &&
      !mud_event_runtime_init())
    status = DOMAIN_EVENT_ALLOCATION_FAILED;
  affected_owners_init();
  character_periodic_init();
  point_update_periodic_init();
  vessel_periodic_init();
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
    domain_event_world_shutdown();
    runtime_bus = NULL;
  }
  return status;
}

struct domain_event_bus *domain_event_runtime_bus(void)
{
  return runtime_bus;
}

enum domain_event_status domain_event_runtime_character_moved(struct char_data *ch,
                                                              room_rnum from_room,
                                                              room_rnum to_room, int direction)
{
  struct domain_character_moved event;

  if (runtime_bus == NULL || ch == NULL)
    return DOMAIN_EVENT_NOT_FOUND;
  event.character = domain_event_character_handle(ch);
  event.from_room = domain_event_room_handle(from_room);
  event.to_room = domain_event_room_handle(to_room);
  event.direction = direction;
  return DOMAIN_EVENT_PUBLISH(runtime_bus, DOMAIN_EVENT_CHARACTER_MOVED, &event);
}

enum domain_event_status domain_event_runtime_character_damaged(struct char_data *target,
                                                                struct char_data *source,
                                                                int amount, int damage_type)
{
  struct domain_character_damaged event;

  if (runtime_bus == NULL || target == NULL)
    return DOMAIN_EVENT_NOT_FOUND;
  event.target = domain_event_character_handle(target);
  event.source = domain_event_character_handle(source);
  event.amount = amount;
  event.damage_type = damage_type;
  return DOMAIN_EVENT_PUBLISH(runtime_bus, DOMAIN_EVENT_CHARACTER_DAMAGED, &event);
}

enum domain_event_status domain_event_runtime_combat_state_changed(struct char_data *ch,
                                                                   struct char_data *opponent,
                                                                   bool in_combat)
{
  struct domain_combat_state_changed event;

  if (runtime_bus == NULL || ch == NULL)
    return DOMAIN_EVENT_NOT_FOUND;
  event.character = domain_event_character_handle(ch);
  event.opponent = domain_event_character_handle(opponent);
  event.in_combat = in_combat;
  return DOMAIN_EVENT_PUBLISH(runtime_bus, DOMAIN_EVENT_COMBAT_STATE_CHANGED, &event);
}

enum domain_event_status domain_event_runtime_character_died(struct char_data *ch,
                                                             struct char_data *killer)
{
  struct domain_character_died event;

  if (runtime_bus == NULL || ch == NULL)
    return DOMAIN_EVENT_NOT_FOUND;
  event.character = domain_event_character_handle(ch);
  event.killer = domain_event_character_handle(killer);
  return DOMAIN_EVENT_PUBLISH(runtime_bus, DOMAIN_EVENT_CHARACTER_DIED, &event);
}

enum domain_event_status domain_event_runtime_character_extracted(struct char_data *ch,
                                                                  uint32_t reason)
{
  struct domain_entity_extracted event;
  struct game_event_owner owner;
  enum domain_event_status status;

  if (runtime_bus == NULL || ch == NULL)
    return DOMAIN_EVENT_NOT_FOUND;
  event.entity = domain_event_character_handle(ch);
  event.reason = reason;
  status = DOMAIN_EVENT_PUBLISH(runtime_bus, DOMAIN_EVENT_ENTITY_EXTRACTED, &event);
  if (domain_entity_handle_is_valid(event.entity))
  {
    owner = game_event_owner_none();
    owner.kind = GAME_EVENT_OWNER_CHARACTER;
    owner.runtime_id = event.entity.runtime_id;
    owner.generation = event.entity.generation;
    (void)event_cancel_owner(owner);
  }
  return status;
}

enum domain_event_status domain_event_runtime_object_moved(struct obj_data *obj,
                                                           room_rnum from_room,
                                                           room_rnum to_room)
{
  struct domain_object_moved event;

  if (runtime_bus == NULL || obj == NULL)
    return DOMAIN_EVENT_NOT_FOUND;
  event.object = domain_event_object_handle(obj);
  event.from_owner = domain_event_room_handle(from_room);
  event.to_owner = domain_event_room_handle(to_room);
  return DOMAIN_EVENT_PUBLISH(runtime_bus, DOMAIN_EVENT_OBJECT_MOVED, &event);
}
