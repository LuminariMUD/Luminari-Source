#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "constants.h"
#include "domain_event_runtime.h"
#include "domain_event_world.h"
#include "door_state.h"

static uint64_t next_exit_identity = 1U;

static struct room_direction_data *find_exit(room_rnum room, int direction)
{
  if (world == NULL || room == NOWHERE || room > top_of_world || direction < 0 ||
      direction >= NUM_OF_DIRS)
    return NULL;
  return world[room].dir_option[direction];
}

uint64_t door_state_identity(room_rnum room, int direction)
{
  struct room_direction_data *exit = find_exit(room, direction);

  if (exit == NULL)
    return 0U;
  if (exit->event_identity == 0U)
  {
    if (next_exit_identity == UINT64_MAX)
    {
      log("SYSERR: Exhausted door event identities.");
      return 0U;
    }
    exit->event_identity = next_exit_identity++;
  }
  return exit->event_identity;
}

static void capture_side(struct door_state_operation *operation, room_rnum room, int direction,
                         enum domain_door_change_cause cause)
{
  struct domain_door_state_changed *side = &operation->sides[operation->count++];

  operation->destinations[operation->count - 1U] =
      domain_event_room_handle(find_exit(room, direction)->to_room);
  side->room = domain_event_room_handle(room);
  side->direction = direction;
  side->exit_identity = door_state_identity(room, direction);
  side->previous_state = (uint16_t)find_exit(room, direction)->exit_info;
  side->current_state = side->previous_state;
  side->cause = cause;
}

bool door_state_begin(struct door_state_operation *operation, room_rnum room, int direction,
                      bool paired, enum domain_door_change_cause cause)
{
  struct room_direction_data *exit;
  struct room_direction_data *back;
  room_rnum destination;

  if (operation == NULL)
    return false;
  memset(operation, 0, sizeof(*operation));
  exit = find_exit(room, direction);
  if (exit == NULL)
    return false;
  destination = exit->to_room;
  capture_side(operation, room, direction, cause);
  back = paired ? find_exit(destination, rev_dir[direction]) : NULL;
  if (back != NULL && back->to_room == room &&
      !(destination == room && rev_dir[direction] == direction))
    capture_side(operation, destination, rev_dir[direction], cause);
  return true;
}

static room_rnum resolve_side(const struct domain_door_state_changed *side)
{
  struct room_data *room;
  struct domain_event_bus *bus = domain_event_runtime_bus();

  /* At bootstrap there is no bus. The room handle still identifies a vnum. */
  if (bus == NULL)
    return real_room((room_vnum)(side->room.runtime_id - 1U));
  room = domain_event_resolve(bus, side->room, DOMAIN_ENTITY_ROOM);
  return room != NULL ? real_room(room->number) : NOWHERE;
}

void door_state_apply(struct door_state_operation *operation, int clear_flags, int set_flags)
{
  size_t index;

  if (operation == NULL)
    return;
  for (index = 0U; index < operation->count; index++)
  {
    struct domain_door_state_changed *side = &operation->sides[index];
    room_rnum room = resolve_side(side);
    struct room_direction_data *exit = find_exit(room, side->direction);

    if (exit != NULL && door_state_identity(room, side->direction) == side->exit_identity)
      exit->exit_info = (exit->exit_info & ~clear_flags) | set_flags;
  }
}

void door_state_finish(struct door_state_operation *operation)
{
  struct domain_door_state_changed facts[2];
  struct domain_event_bus *bus = domain_event_runtime_bus();
  size_t count = 0U;
  size_t index;

  if (operation == NULL)
    return;
  /* Snapshot every side before the first handler can mutate or extract anything. */
  for (index = 0U; index < operation->count; index++)
  {
    struct domain_door_state_changed fact = operation->sides[index];
    room_rnum room = resolve_side(&fact);
    struct room_direction_data *exit = find_exit(room, fact.direction);
    uint64_t identity;

    if (exit != NULL && !domain_entity_handle_equal(domain_event_room_handle(exit->to_room),
                                                    operation->destinations[index]))
      exit->event_identity = 0U;
    identity = door_state_identity(room, fact.direction);

    fact.current_state = exit != NULL ? (uint16_t)exit->exit_info : 0U;
    if (identity != fact.exit_identity)
    {
      fact.cause = DOMAIN_DOOR_EDIT;
      fact.exit_identity = identity;
    }
    else if (fact.current_state == fact.previous_state)
      continue;
    facts[count++] = fact;
  }
  operation->count = 0U;
  if (bus == NULL)
    return;
  for (index = 0U; index < count; index++)
  {
    struct domain_event_topic topic;

    topic.role = DOMAIN_EVENT_TOPIC_SUBJECT;
    topic.entity = facts[index].room;
    (void)DOMAIN_EVENT_PUBLISH_ROUTED(bus, DOMAIN_EVENT_DOOR_STATE_CHANGED, &topic, 1U,
                                      &facts[index]);
  }
}

void door_state_update(room_rnum room, int direction, int clear_flags, int set_flags, bool paired,
                       enum domain_door_change_cause cause)
{
  struct door_state_operation operation;

  if (!door_state_begin(&operation, room, direction, paired, cause))
    return;
  door_state_apply(&operation, clear_flags, set_flags);
  door_state_finish(&operation);
}

void door_state_replace(room_rnum room, int direction, int flags,
                        enum domain_door_change_cause cause)
{
  door_state_update(room, direction, ~0, flags, false, cause);
}
