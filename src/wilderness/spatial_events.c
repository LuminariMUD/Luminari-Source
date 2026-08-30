#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "domain_event_types.h"
#include "wilderness/spatial_events.h"
#include "wilderness/spatial_audio.h"
#include "wilderness/spatial_visual.h"

#define SPATIAL_EVENT_MAX_ROOM_RANGE 8
#define SPATIAL_EVENT_MAX_VISITED_ROOMS 256

static bool room_already_visited(const room_rnum *rooms, size_t count, room_rnum room)
{
  size_t index;

  for (index = 0; index < count; index++)
    if (rooms[index] == room)
      return true;
  return false;
}

static void deliver_through_rooms(struct domain_event_bus *bus,
                                  const struct domain_world_phenomenon *phenomenon,
                                  const char *description, int requested_range,
                                  bool pass_closed_doors)
{
  struct room_data *source;
  room_rnum rooms[SPATIAL_EVENT_MAX_VISITED_ROOMS];
  unsigned char distances[SPATIAL_EVENT_MAX_VISITED_ROOMS];
  room_rnum source_room;
  size_t head = 0U;
  size_t count = 0U;
  int max_range;

  source = domain_event_resolve(bus, phenomenon->source_room, DOMAIN_ENTITY_ROOM);
  if (source == NULL || description == NULL)
    return;
  source_room = real_room(source->number);
  if (source_room == NOWHERE)
    return;
  max_range = requested_range;
  if (max_range < 0)
    return;
  if (max_range > SPATIAL_EVENT_MAX_ROOM_RANGE)
    max_range = SPATIAL_EVENT_MAX_ROOM_RANGE;

  rooms[count] = source_room;
  distances[count++] = 0U;
  while (head < count)
  {
    room_rnum room = rooms[head];
    unsigned int distance = distances[head++];
    struct char_data *ch;
    int direction;

    if ((int)distance >= phenomenon->minimum_range)
      for (ch = world[room].people; ch != NULL; ch = ch->next_in_room)
        if (!IS_NPC(ch) && ch->desc != NULL)
          send_to_char(ch, "\r\n%s\r\n", description);

    if ((int)distance >= max_range)
      continue;
    for (direction = 0; direction < NUM_OF_DIRS; direction++)
    {
      struct room_direction_data *exit = world[room].dir_option[direction];
      room_rnum next_room;

      if (exit == NULL || exit->to_room == NOWHERE ||
          (!pass_closed_doors && EXIT_FLAGGED(exit, EX_CLOSED)))
        continue;
      next_room = exit->to_room;
      if (room_already_visited(rooms, count, next_room) ||
          count >= SPATIAL_EVENT_MAX_VISITED_ROOMS)
        continue;
      rooms[count] = next_room;
      distances[count++] = (unsigned char)(distance + 1U);
    }
  }
}

static void handle_world_phenomenon(const struct domain_event_context *context,
                                    void *handler_context)
{
  const struct domain_world_phenomenon *phenomenon = context->payload;

  (void)handler_context;
  if (phenomenon->propagation == DOMAIN_WORLD_PROPAGATE_ROOMS)
  {
    if ((phenomenon->channels & DOMAIN_WORLD_PHENOMENON_VISUAL) != 0U)
      deliver_through_rooms(context->bus, phenomenon, phenomenon->visual_description,
                            phenomenon->visual_range, false);
    if ((phenomenon->channels & DOMAIN_WORLD_PHENOMENON_AUDIBLE) != 0U)
      deliver_through_rooms(context->bus, phenomenon, phenomenon->audio_description,
                            phenomenon->audio_range, true);
    return;
  }
  if ((phenomenon->channels & DOMAIN_WORLD_PHENOMENON_VISUAL) != 0U &&
      phenomenon->visual_description != NULL)
  {
    spatial_visual_emit(phenomenon->source_x, phenomenon->source_y, phenomenon->source_z,
                        phenomenon->visual_description, phenomenon->intensity,
                        phenomenon->visual_range);
  }
  if ((phenomenon->channels & DOMAIN_WORLD_PHENOMENON_AUDIBLE) != 0U &&
      phenomenon->audio_description != NULL)
  {
    spatial_audio_emit(phenomenon->source_x, phenomenon->source_y, phenomenon->source_z,
                       phenomenon->audio_description, phenomenon->intensity,
                       phenomenon->audio_frequency, phenomenon->audio_range);
  }
}

enum domain_event_status spatial_event_register_handlers(struct domain_event_bus *bus)
{
  const struct domain_event_handler_config handler = {DOMAIN_EVENT_WORLD_PHENOMENON,
                                                       "wilderness.spatial_delivery", 0,
                                                       handle_world_phenomenon, NULL};

  return domain_event_register_handler(bus, &handler);
}
