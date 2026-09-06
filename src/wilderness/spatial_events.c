#include "conf.h"
#include "sysdep.h"
#include <math.h>
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "domain_event_types.h"
#include "domain_event_world.h"
#include "character/abilities.h"
#include "magic/spells.h"
#include "wilderness/kdtree.h"
#include "wilderness/wilderness.h"
#include "wilderness/spatial_core.h"
#include "wilderness/spatial_events.h"
#include "wilderness/spatial_audio.h"
#include "wilderness/spatial_visual.h"

#define SPATIAL_EVENT_MAX_ROOM_RANGE 8
#define SPATIAL_EVENT_MAX_VISITED_ROOMS 256
#define SPATIAL_EVENT_MAX_PERCEPTIONS 256U
#define SPATIAL_EVENT_MAX_CANDIDATES 1024U
#define SPATIAL_EVENT_MAX_COORDINATE_RANGE 32
#define SPATIAL_EVENT_REJECTION_LOG_INTERVAL 100U

struct perception_candidate
{
  struct domain_entity_handle observer;
  uint32_t senses;
  unsigned int distance;
  float intensity;
};

static size_t perception_limit = SPATIAL_EVENT_MAX_PERCEPTIONS;
static uint64_t perception_rejections;

static void note_perception_rejection(const char *reason)
{
  perception_rejections++;
  if (perception_rejections == 1U ||
      perception_rejections % SPATIAL_EVENT_REJECTION_LOG_INTERVAL == 0U)
    log("WARNING: spatial NPC perception rejected (%s); rejected=%llu.", reason,
        (unsigned long long)perception_rejections);
}

static bool observer_can_use_sense(const struct char_data *observer, uint32_t sense)
{
  if (observer == NULL || !IS_NPC(observer) || GET_POS(observer) <= POS_STUNNED ||
      MOB_FLAGGED(observer, MOB_NO_AI))
    return false;
  return (((sense & DOMAIN_WORLD_PHENOMENON_VISUAL) != 0U && !AFF_FLAGGED(observer, AFF_BLIND)) ||
          ((sense & DOMAIN_WORLD_PHENOMENON_AUDIBLE) != 0U && !AFF_FLAGGED(observer, AFF_DEAF)));
}

static bool add_perception_candidate(struct perception_candidate *candidates, size_t *count,
                                     struct char_data *observer, uint32_t sense,
                                     unsigned int distance, float intensity)
{
  struct domain_entity_handle identity;
  size_t index;

  if (!observer_can_use_sense(observer, sense))
    return true;
  identity = domain_event_character_handle(observer);
  if (!domain_entity_handle_is_valid(identity))
    return true;
  for (index = 0U; index < *count; index++)
    if (domain_entity_handle_equal(candidates[index].observer, identity))
    {
      candidates[index].senses |= sense;
      candidates[index].distance = MIN(candidates[index].distance, distance);
      candidates[index].intensity = MAX(candidates[index].intensity, intensity);
      return true;
    }
  if (*count >= perception_limit || *count >= SPATIAL_EVENT_MAX_PERCEPTIONS)
  {
    note_perception_rejection("candidate capacity");
    return false;
  }
  candidates[*count].observer = identity;
  candidates[*count].senses = sense;
  candidates[*count].distance = distance;
  candidates[*count].intensity = intensity;
  (*count)++;
  return true;
}

static bool source_is_known(struct domain_event_bus *bus,
                            const struct domain_world_phenomenon *phenomenon,
                            struct char_data *observer, uint32_t senses)
{
  struct char_data *source;

  source = domain_event_resolve(bus, phenomenon->source, DOMAIN_ENTITY_CHARACTER);
  if (source == NULL)
    return false;
  if (phenomenon->source_faction > 0 && GET_FACTION(observer) == phenomenon->source_faction)
    return true;
  return (senses & DOMAIN_WORLD_PHENOMENON_VISUAL) != 0U && CAN_SEE(observer, source);
}

static void publish_perceptions(struct domain_event_bus *bus,
                                const struct domain_world_phenomenon *phenomenon,
                                struct perception_candidate *candidates, size_t count)
{
  struct domain_phenomenon_perceived perceived;
  struct domain_event_topic topics[3];
  struct char_data *observer;
  size_t index;
  size_t topic_count;

  for (index = 0U; index < count; index++)
  {
    observer = domain_event_world_resolve_character(candidates[index].observer);
    if (observer == NULL)
      continue;
    if (phenomenon->stealth_dc > 0 && compute_ability(observer, ABILITY_PERCEPTION) + 10 <
                                          phenomenon->stealth_dc + (int)candidates[index].distance)
      continue;
    memset(&perceived, 0, sizeof(perceived));
    perceived.phenomenon_id = phenomenon->phenomenon_id;
    perceived.source_room = phenomenon->source_room;
    perceived.observer = candidates[index].observer;
    perceived.kind = phenomenon->kind;
    perceived.senses = candidates[index].senses;
    perceived.distance = candidates[index].distance;
    perceived.intensity = candidates[index].intensity;
    perceived.source_known = source_is_known(bus, phenomenon, observer, perceived.senses);
    perceived.phenomenon_source =
        perceived.source_known ? phenomenon->source : (struct domain_entity_handle){0};
    topic_count = 0U;
    topics[topic_count++] =
        (struct domain_event_topic){DOMAIN_EVENT_TOPIC_SUBJECT, perceived.observer};
    if (domain_entity_handle_is_valid(perceived.phenomenon_source))
      topics[topic_count++] =
          (struct domain_event_topic){DOMAIN_EVENT_TOPIC_SOURCE, perceived.phenomenon_source};
    if (domain_entity_handle_is_valid(perceived.source_room))
      topics[topic_count++] =
          (struct domain_event_topic){DOMAIN_EVENT_TOPIC_SOURCE, perceived.source_room};
    if (DOMAIN_EVENT_PUBLISH_ROUTED(bus, DOMAIN_EVENT_PHENOMENON_PERCEIVED, topics, topic_count,
                                    &perceived) != DOMAIN_EVENT_OK)
      note_perception_rejection("publication");
  }
}

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
                                  bool pass_closed_doors, uint32_t sense,
                                  struct perception_candidate *candidates, size_t *candidate_count)
{
  struct room_data *source;
  room_rnum rooms[SPATIAL_EVENT_MAX_VISITED_ROOMS];
  unsigned char distances[SPATIAL_EVENT_MAX_VISITED_ROOMS];
  room_rnum source_room;
  size_t head = 0U;
  size_t count = 0U;
  int max_range;

  source = domain_event_resolve(bus, phenomenon->source_room, DOMAIN_ENTITY_ROOM);
  if (source == NULL)
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
    {
      for (ch = world[room].people; ch != NULL; ch = ch->next_in_room)
      {
        if (IS_NPC(ch))
          (void)add_perception_candidate(candidates, candidate_count, ch, sense, distance,
                                         phenomenon->intensity / (float)(distance + 1U));
        else if (ch->desc != NULL && description != NULL)
          send_to_char(ch, "\r\n%s\r\n", description);
      }
    }

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
      if (room_already_visited(rooms, count, next_room) || count >= SPATIAL_EVENT_MAX_VISITED_ROOMS)
        continue;
      rooms[count] = next_room;
      distances[count++] = (unsigned char)(distance + 1U);
    }
  }
}

static float coordinate_channel_intensity(struct char_data *observer,
                                          const struct domain_world_phenomenon *phenomenon,
                                          uint32_t sense)
{
  struct spatial_context *spatial;
  struct spatial_system *system;
  const char *description;
  float intensity = 0.0f;

  if (!observer_can_use_sense(observer, sense))
    return 0.0f;
  system = sense == DOMAIN_WORLD_PHENOMENON_VISUAL ? &visual_system : &audio_system;
  description = sense == DOMAIN_WORLD_PHENOMENON_VISUAL ? phenomenon->visual_description
                                                        : phenomenon->audio_description;
  if (description == NULL)
    description = "a distant phenomenon";
  spatial = spatial_create_context();
  if (spatial == NULL)
  {
    note_perception_rejection("spatial context allocation");
    return 0.0f;
  }
  if (spatial_setup_context(spatial, phenomenon->source_x, phenomenon->source_y,
                            phenomenon->source_z, observer, description) == SPATIAL_SUCCESS)
  {
    spatial->base_intensity = phenomenon->intensity;
    spatial->audio_frequency = phenomenon->audio_frequency;
    spatial->effective_range = sense == DOMAIN_WORLD_PHENOMENON_VISUAL ? phenomenon->visual_range
                                                                       : phenomenon->audio_range;
    if (spatial_process_stimulus(spatial, system) == SPATIAL_SUCCESS)
      intensity = spatial->final_intensity;
  }
  spatial_free_context(spatial);
  return intensity;
}

static bool collect_coordinate_room(const struct domain_world_phenomenon *phenomenon,
                                    room_rnum room, int max_range,
                                    struct perception_candidate *candidates,
                                    size_t *candidate_count, size_t *examined)
{
  struct char_data *observer;
  uint32_t senses;
  float visual_intensity;
  float audio_intensity;
  float distance;
  int x;
  int y;

  if (room == NOWHERE || room > top_of_world || !world[room].wilderness_coordinates_set)
    return true;
  x = world[room].coords[X_COORD];
  y = world[room].coords[Y_COORD];
  distance = hypotf((float)(x - phenomenon->source_x), (float)(y - phenomenon->source_y));
  if (distance < (float)phenomenon->minimum_range || distance > (float)max_range)
    return true;
  for (observer = world[room].people; observer != NULL; observer = observer->next_in_room)
  {
    if (!IS_NPC(observer))
      continue;
    if (++(*examined) > SPATIAL_EVENT_MAX_CANDIDATES)
    {
      note_perception_rejection("examination capacity");
      return false;
    }
    senses = 0U;
    visual_intensity = 0.0f;
    audio_intensity = 0.0f;
    if ((phenomenon->channels & DOMAIN_WORLD_PHENOMENON_VISUAL) != 0U &&
        distance <= (float)phenomenon->visual_range)
    {
      visual_intensity =
          coordinate_channel_intensity(observer, phenomenon, DOMAIN_WORLD_PHENOMENON_VISUAL);
      if (visual_intensity > 0.0f)
        senses |= DOMAIN_WORLD_PHENOMENON_VISUAL;
    }
    if ((phenomenon->channels & DOMAIN_WORLD_PHENOMENON_AUDIBLE) != 0U &&
        distance <= (float)phenomenon->audio_range)
    {
      audio_intensity =
          coordinate_channel_intensity(observer, phenomenon, DOMAIN_WORLD_PHENOMENON_AUDIBLE);
      if (audio_intensity > 0.0f)
        senses |= DOMAIN_WORLD_PHENOMENON_AUDIBLE;
    }
    if (senses != 0U && !add_perception_candidate(candidates, candidate_count, observer, senses,
                                                  (unsigned int)ceilf(distance),
                                                  MAX(visual_intensity, audio_intensity)))
      return false;
  }
  return true;
}

static void collect_coordinate_perceptions(const struct domain_world_phenomenon *phenomenon,
                                           struct perception_candidate *candidates,
                                           size_t *candidate_count)
{
  double location[2];
  double indexed_position[2];
  struct kdres *indexed_rooms;
  room_rnum *indexed_room;
  room_rnum room;
  int max_range;
  size_t examined = 0U;

  max_range = MAX(phenomenon->visual_range, phenomenon->audio_range);
  max_range = MIN(MAX(0, max_range), SPATIAL_EVENT_MAX_COORDINATE_RANGE);
  if (kd_wilderness_rooms != NULL)
  {
    location[X_COORD] = (double)phenomenon->source_x;
    location[Y_COORD] = (double)phenomenon->source_y;
    indexed_rooms = kd_nearest_range(kd_wilderness_rooms, location, (double)max_range + 0.01);
    if (indexed_rooms == NULL)
    {
      note_perception_rejection("wilderness room index query");
      return;
    }
    while (!kd_res_end(indexed_rooms))
    {
      indexed_room = kd_res_item(indexed_rooms, indexed_position);
      if (indexed_room != NULL && !collect_coordinate_room(phenomenon, *indexed_room, max_range,
                                                           candidates, candidate_count, &examined))
      {
        kd_res_free(indexed_rooms);
        return;
      }
      kd_res_next(indexed_rooms);
    }
    kd_res_free(indexed_rooms);
  }

  room = real_room(WILD_DYNAMIC_ROOM_VNUM_START);
  for (;
       room != NOWHERE && room <= top_of_world && GET_ROOM_VNUM(room) <= WILD_DYNAMIC_ROOM_VNUM_END;
       room++)
    if (ROOM_FLAGGED(room, ROOM_OCCUPIED) &&
        !collect_coordinate_room(phenomenon, room, max_range, candidates, candidate_count,
                                 &examined))
      return;
}

static void handle_world_phenomenon(const struct domain_event_context *context,
                                    void *handler_context)
{
  const struct domain_world_phenomenon *phenomenon = context->payload;
  struct perception_candidate candidates[SPATIAL_EVENT_MAX_PERCEPTIONS] = {0};
  size_t candidate_count = 0U;

  (void)handler_context;
  if (phenomenon->propagation == DOMAIN_WORLD_PROPAGATE_ROOMS)
  {
    if ((phenomenon->channels & DOMAIN_WORLD_PHENOMENON_VISUAL) != 0U)
      deliver_through_rooms(context->bus, phenomenon, phenomenon->visual_description,
                            phenomenon->visual_range, false, DOMAIN_WORLD_PHENOMENON_VISUAL,
                            candidates, &candidate_count);
    if ((phenomenon->channels & DOMAIN_WORLD_PHENOMENON_AUDIBLE) != 0U)
      deliver_through_rooms(context->bus, phenomenon, phenomenon->audio_description,
                            phenomenon->audio_range, true, DOMAIN_WORLD_PHENOMENON_AUDIBLE,
                            candidates, &candidate_count);
    publish_perceptions(context->bus, phenomenon, candidates, candidate_count);
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
  collect_coordinate_perceptions(phenomenon, candidates, &candidate_count);
  publish_perceptions(context->bus, phenomenon, candidates, candidate_count);
}

enum domain_event_status spatial_event_register_handlers(struct domain_event_bus *bus)
{
  const struct domain_event_handler_config handler = {DOMAIN_EVENT_WORLD_PHENOMENON,
                                                      "wilderness.spatial_delivery", 0,
                                                      handle_world_phenomenon, NULL};

  return domain_event_register_handler(bus, &handler);
}

uint64_t spatial_event_perception_rejections(void)
{
  return perception_rejections;
}

#ifdef LUMINARI_CUTEST
void spatial_event_set_perception_limit_for_test(size_t limit)
{
  perception_limit = MIN(limit, SPATIAL_EVENT_MAX_PERCEPTIONS);
}
#endif
