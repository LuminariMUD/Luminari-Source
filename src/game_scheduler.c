#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "game_scheduler.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define GAME_SCHEDULER_WHEEL_BITS 6U
#define GAME_SCHEDULER_WHEEL_MASK (GAME_SCHEDULER_WHEEL_SLOTS - 1U)
#define GAME_SCHEDULER_WHEEL_HORIZON (UINT64_C(1) << 30U)
#define GAME_SCHEDULER_INVALID_INDEX SIZE_MAX

struct game_event_type
{
  char *name;
  game_event_handler handler;
  game_event_cleanup cleanup;
  enum game_event_lateness_policy lateness_policy;
  uint32_t catch_up_limit;
  size_t max_events;
  size_t max_events_per_owner;
  bool requires_owner;
  bool cleanup_on_null_payload;
  size_t live_events;
};

struct game_event_owner_entry;

struct game_event
{
  game_event_id_t event_id;
  game_event_type_id_t event_type;
  enum game_event_state state;
  enum game_event_location location;
  game_tick_t deadline_tick;
  game_tick_t interval_ticks;
  uint64_t insertion_sequence;
  uint32_t catch_up_runs;
  void *payload;
  bool cleanup_pending;
  struct game_event_owner owner;
  struct game_event_owner_entry *owner_entry;
  struct game_event *owner_previous;
  struct game_event *owner_next;

  struct game_event *wheel_previous;
  struct game_event *wheel_next;
  uint32_t wheel_level;
  uint32_t wheel_slot;
  size_t location_heap_index;
  size_t deadline_heap_index;
  bool in_deadline_heap;

  struct game_event *registry_previous;
  struct game_event *registry_next;
};

struct game_event_owner_entry
{
  struct game_event_owner owner;
  struct game_event *events_head;
  struct game_event *events_tail;
  size_t live_events;
  struct game_event_owner_entry *hash_next;
};

struct game_event_heap
{
  struct game_event **items;
  size_t count;
  size_t capacity;
  enum game_event_location location;
};

struct game_deadline_heap
{
  struct game_event **items;
  size_t count;
  size_t capacity;
};

struct game_scheduler
{
  struct game_scheduler_config config;
  game_tick_t current_tick;
  game_event_id_t next_event_id;
  uint64_t next_insertion_sequence;
  bool event_id_exhausted;
  bool insertion_sequence_exhausted;
  bool shutting_down;
  unsigned int dispatch_depth;

  struct game_event *wheel_head[GAME_SCHEDULER_WHEEL_LEVELS][GAME_SCHEDULER_WHEEL_SLOTS];
  struct game_event *wheel_tail[GAME_SCHEDULER_WHEEL_LEVELS][GAME_SCHEDULER_WHEEL_SLOTS];
  struct game_event_heap overflow_heap;
  struct game_event_heap ready_heap;
  struct game_deadline_heap deadline_heap;

  struct game_event **registry_buckets;
  size_t registry_bucket_count;
  struct game_event_owner_entry **owner_buckets;
  size_t owner_bucket_count;
  size_t owner_count;
  struct game_event_type *event_types;
  size_t event_type_count;
  bool event_types_sealed;
  size_t event_count;

  uint64_t total_scheduled;
  uint64_t total_callbacks;
  uint64_t total_cancelled;
  uint64_t total_completed;
  uint64_t total_failed;
  uint64_t total_rescheduled;
  uint64_t total_late_callbacks;
  uint64_t total_missed_occurrences;
  uint64_t total_skipped_occurrences;
  uint64_t total_coalesced_occurrences;
  uint64_t total_capacity_rejections;
  uint64_t total_type_capacity_rejections;
  uint64_t total_invalid_owner_rejections;
  uint64_t total_owner_capacity_rejections;
  uint64_t total_owner_type_capacity_rejections;
  uint64_t total_ticks_advanced;
  uint64_t total_cascade_slots;
  uint64_t total_cascaded_events;
  uint64_t total_overflow_promotions;
  uint64_t total_large_advances;
  uint64_t total_large_advance_events;
  uint64_t largest_cascade;
};

static bool event_less(const struct game_event *left, const struct game_event *right)
{
  if (left->deadline_tick != right->deadline_tick)
    return left->deadline_tick < right->deadline_tick;

  return left->insertion_sequence < right->insertion_sequence;
}

static void location_heap_swap(struct game_event_heap *heap, size_t left, size_t right)
{
  struct game_event *temporary;

  temporary = heap->items[left];
  heap->items[left] = heap->items[right];
  heap->items[right] = temporary;
  heap->items[left]->location_heap_index = left;
  heap->items[right]->location_heap_index = right;
}

static void location_heap_sift_up(struct game_event_heap *heap, size_t index)
{
  size_t parent;

  while (index > 0)
  {
    parent = (index - 1U) / 2U;
    if (!event_less(heap->items[index], heap->items[parent]))
      break;
    location_heap_swap(heap, index, parent);
    index = parent;
  }
}

static void location_heap_sift_down(struct game_event_heap *heap, size_t index)
{
  size_t left;
  size_t right;
  size_t smallest;

  for (;;)
  {
    left = index * 2U + 1U;
    right = left + 1U;
    smallest = index;
    if (left < heap->count && event_less(heap->items[left], heap->items[smallest]))
      smallest = left;
    if (right < heap->count && event_less(heap->items[right], heap->items[smallest]))
      smallest = right;
    if (smallest == index)
      break;
    location_heap_swap(heap, index, smallest);
    index = smallest;
  }
}

static bool location_heap_push(struct game_event_heap *heap, struct game_event *event)
{
  size_t index;

  if (heap->count >= heap->capacity)
    return false;

  index = heap->count++;
  heap->items[index] = event;
  event->location = heap->location;
  event->location_heap_index = index;
  location_heap_sift_up(heap, index);
  return true;
}

static struct game_event *location_heap_remove_at(struct game_event_heap *heap, size_t index)
{
  struct game_event *event;

  if (index >= heap->count)
    return NULL;

  event = heap->items[index];
  if (event == NULL)
    return NULL;
  heap->count--;
  if (index != heap->count)
  {
    heap->items[index] = heap->items[heap->count];
    heap->items[index]->location_heap_index = index;
    if (index > 0 && event_less(heap->items[index], heap->items[(index - 1U) / 2U]))
      location_heap_sift_up(heap, index);
    else
      location_heap_sift_down(heap, index);
  }
  heap->items[heap->count] = NULL;
  event->location = GAME_EVENT_LOCATION_NONE;
  event->location_heap_index = GAME_SCHEDULER_INVALID_INDEX;
  return event;
}

static struct game_event *location_heap_pop(struct game_event_heap *heap)
{
  return location_heap_remove_at(heap, 0);
}

static void deadline_heap_swap(struct game_deadline_heap *heap, size_t left, size_t right)
{
  struct game_event *temporary;

  temporary = heap->items[left];
  heap->items[left] = heap->items[right];
  heap->items[right] = temporary;
  heap->items[left]->deadline_heap_index = left;
  heap->items[right]->deadline_heap_index = right;
}

static void deadline_heap_sift_up(struct game_deadline_heap *heap, size_t index)
{
  size_t parent;

  while (index > 0)
  {
    parent = (index - 1U) / 2U;
    if (!event_less(heap->items[index], heap->items[parent]))
      break;
    deadline_heap_swap(heap, index, parent);
    index = parent;
  }
}

static void deadline_heap_sift_down(struct game_deadline_heap *heap, size_t index)
{
  size_t left;
  size_t right;
  size_t smallest;

  for (;;)
  {
    left = index * 2U + 1U;
    right = left + 1U;
    smallest = index;
    if (left < heap->count && event_less(heap->items[left], heap->items[smallest]))
      smallest = left;
    if (right < heap->count && event_less(heap->items[right], heap->items[smallest]))
      smallest = right;
    if (smallest == index)
      break;
    deadline_heap_swap(heap, index, smallest);
    index = smallest;
  }
}

static bool deadline_heap_push(struct game_deadline_heap *heap, struct game_event *event)
{
  size_t index;

  if (heap->count >= heap->capacity || event->in_deadline_heap)
    return false;

  index = heap->count++;
  heap->items[index] = event;
  event->deadline_heap_index = index;
  event->in_deadline_heap = true;
  deadline_heap_sift_up(heap, index);
  return true;
}

static struct game_event *deadline_heap_remove_at(struct game_deadline_heap *heap, size_t index)
{
  struct game_event *event;

  if (index >= heap->count)
    return NULL;

  event = heap->items[index];
  heap->count--;
  if (index != heap->count)
  {
    heap->items[index] = heap->items[heap->count];
    heap->items[index]->deadline_heap_index = index;
    if (index > 0 && event_less(heap->items[index], heap->items[(index - 1U) / 2U]))
      deadline_heap_sift_up(heap, index);
    else
      deadline_heap_sift_down(heap, index);
  }
  heap->items[heap->count] = NULL;
  event->deadline_heap_index = GAME_SCHEDULER_INVALID_INDEX;
  event->in_deadline_heap = false;
  return event;
}

static void deadline_heap_remove(struct game_deadline_heap *heap, struct game_event *event)
{
  if (event != NULL && event->in_deadline_heap)
    deadline_heap_remove_at(heap, event->deadline_heap_index);
}

static uint64_t hash_event_id(game_event_id_t event_id)
{
  uint64_t value;

  value = event_id;
  value ^= value >> 30U;
  value *= UINT64_C(0xbf58476d1ce4e5b9);
  value ^= value >> 27U;
  value *= UINT64_C(0x94d049bb133111eb);
  value ^= value >> 31U;
  return value;
}

static size_t registry_bucket(const struct game_scheduler *scheduler, game_event_id_t event_id)
{
  return (size_t)(hash_event_id(event_id) & (scheduler->registry_bucket_count - 1U));
}

static struct game_event *registry_find(const struct game_scheduler *scheduler,
                                        game_event_id_t event_id)
{
  struct game_event *event;
  size_t bucket;

  if (scheduler == NULL || event_id == 0)
    return NULL;

  bucket = registry_bucket(scheduler, event_id);
  for (event = scheduler->registry_buckets[bucket]; event != NULL; event = event->registry_next)
  {
    if (event->event_id == event_id)
      return event;
  }
  return NULL;
}

static void registry_insert(struct game_scheduler *scheduler, struct game_event *event)
{
  size_t bucket;

  bucket = registry_bucket(scheduler, event->event_id);
  event->registry_previous = NULL;
  event->registry_next = scheduler->registry_buckets[bucket];
  if (event->registry_next != NULL)
    event->registry_next->registry_previous = event;
  scheduler->registry_buckets[bucket] = event;
}

static void registry_remove(struct game_scheduler *scheduler, struct game_event *event)
{
  size_t bucket;

  bucket = registry_bucket(scheduler, event->event_id);
  if (event->registry_previous != NULL)
    event->registry_previous->registry_next = event->registry_next;
  else
    scheduler->registry_buckets[bucket] = event->registry_next;
  if (event->registry_next != NULL)
    event->registry_next->registry_previous = event->registry_previous;
  event->registry_previous = NULL;
  event->registry_next = NULL;
}

static uint64_t hash_owner(struct game_event_owner owner)
{
  uint64_t value;

  value = hash_event_id(owner.runtime_id);
  value ^= hash_event_id(owner.generation + UINT64_C(0x9e3779b97f4a7c15));
  value ^= hash_event_id((uint64_t)owner.kind + UINT64_C(0x517cc1b727220a95));
  return hash_event_id(value);
}

static size_t owner_bucket(const struct game_scheduler *scheduler, struct game_event_owner owner)
{
  return (size_t)(hash_owner(owner) & (scheduler->owner_bucket_count - 1U));
}

static struct game_event_owner_entry *owner_find(const struct game_scheduler *scheduler,
                                                 struct game_event_owner owner)
{
  struct game_event_owner_entry *entry;
  size_t bucket;

  if (scheduler == NULL || !game_event_owner_is_valid(owner))
    return NULL;
  bucket = owner_bucket(scheduler, owner);
  for (entry = scheduler->owner_buckets[bucket]; entry != NULL; entry = entry->hash_next)
  {
    if (game_event_owner_equal(entry->owner, owner))
      return entry;
  }
  return NULL;
}

static void owner_entry_insert(struct game_scheduler *scheduler,
                               struct game_event_owner_entry *entry)
{
  size_t bucket;

  bucket = owner_bucket(scheduler, entry->owner);
  entry->hash_next = scheduler->owner_buckets[bucket];
  scheduler->owner_buckets[bucket] = entry;
  scheduler->owner_count++;
}

static void owner_entry_remove(struct game_scheduler *scheduler,
                               struct game_event_owner_entry *entry)
{
  struct game_event_owner_entry **cursor;
  size_t bucket;

  bucket = owner_bucket(scheduler, entry->owner);
  cursor = &scheduler->owner_buckets[bucket];
  while (*cursor != NULL && *cursor != entry)
    cursor = &(*cursor)->hash_next;
  if (*cursor == entry)
    *cursor = entry->hash_next;
  if (scheduler->owner_count > 0)
    scheduler->owner_count--;
  free(entry);
}

static void owner_event_insert(struct game_event_owner_entry *entry, struct game_event *event)
{
  event->owner_entry = entry;
  event->owner_previous = entry->events_tail;
  event->owner_next = NULL;
  if (entry->events_tail != NULL)
    entry->events_tail->owner_next = event;
  else
    entry->events_head = event;
  entry->events_tail = event;
  entry->live_events++;
}

static void owner_event_remove(struct game_scheduler *scheduler, struct game_event *event)
{
  struct game_event_owner_entry *entry;

  entry = event->owner_entry;
  if (entry == NULL)
    return;
  if (event->owner_previous != NULL)
    event->owner_previous->owner_next = event->owner_next;
  else
    entry->events_head = event->owner_next;
  if (event->owner_next != NULL)
    event->owner_next->owner_previous = event->owner_previous;
  else
    entry->events_tail = event->owner_previous;
  event->owner_entry = NULL;
  event->owner_previous = NULL;
  event->owner_next = NULL;
  if (entry->live_events > 0)
    entry->live_events--;
  if (entry->live_events == 0)
    owner_entry_remove(scheduler, entry);
}

static size_t owner_type_event_count(const struct game_event_owner_entry *entry,
                                     game_event_type_id_t event_type)
{
  const struct game_event *event;
  size_t count;

  count = 0;
  for (event = entry != NULL ? entry->events_head : NULL; event != NULL;
       event = event->owner_next)
  {
    if (event->event_type == event_type)
      count++;
  }
  return count;
}

static struct game_event_type *find_event_type(struct game_scheduler *scheduler,
                                               game_event_type_id_t event_type)
{
  if (scheduler == NULL || event_type == 0 || event_type > scheduler->event_type_count)
    return NULL;
  return &scheduler->event_types[event_type - 1U];
}

static bool next_power_of_two(size_t value, size_t *result)
{
  size_t power;

  power = 1U;
  while (power < value)
  {
    if (power > SIZE_MAX / 2U)
      return false;
    power *= 2U;
  }
  *result = power;
  return true;
}

static bool assign_event_identity(struct game_scheduler *scheduler, game_event_id_t *event_id,
                                  uint64_t *sequence)
{
  if (scheduler->event_id_exhausted || scheduler->insertion_sequence_exhausted)
    return false;

  *event_id = scheduler->next_event_id;
  *sequence = scheduler->next_insertion_sequence;
  if (scheduler->next_event_id == UINT64_MAX)
    scheduler->event_id_exhausted = true;
  else
    scheduler->next_event_id++;
  if (scheduler->next_insertion_sequence == UINT64_MAX)
    scheduler->insertion_sequence_exhausted = true;
  else
    scheduler->next_insertion_sequence++;
  return true;
}

static bool assign_insertion_sequence(struct game_scheduler *scheduler, uint64_t *sequence)
{
  if (scheduler->insertion_sequence_exhausted)
    return false;

  *sequence = scheduler->next_insertion_sequence;
  if (scheduler->next_insertion_sequence == UINT64_MAX)
    scheduler->insertion_sequence_exhausted = true;
  else
    scheduler->next_insertion_sequence++;
  return true;
}

static enum game_scheduler_status normalize_deadline(const struct game_scheduler *scheduler,
                                                     game_tick_t requested, game_tick_t *normalized)
{
  if (requested <= scheduler->current_tick)
  {
    if (scheduler->current_tick == UINT64_MAX)
      return GAME_SCHEDULER_INVALID_DEADLINE;
    *normalized = scheduler->current_tick + 1U;
  }
  else
  {
    *normalized = requested;
  }
  return GAME_SCHEDULER_OK;
}

static enum game_scheduler_status deadline_after(const struct game_scheduler *scheduler,
                                                 game_tick_t delay_ticks,
                                                 game_tick_t *deadline_tick)
{
  if (delay_ticks == 0)
    delay_ticks = 1U;
  if (delay_ticks > UINT64_MAX - scheduler->current_tick)
    return GAME_SCHEDULER_INVALID_DEADLINE;
  *deadline_tick = scheduler->current_tick + delay_ticks;
  return GAME_SCHEDULER_OK;
}

static void wheel_insert(struct game_scheduler *scheduler, struct game_event *event, uint32_t level,
                         uint32_t slot)
{
  event->wheel_previous = scheduler->wheel_tail[level][slot];
  event->wheel_next = NULL;
  if (event->wheel_previous != NULL)
    event->wheel_previous->wheel_next = event;
  else
    scheduler->wheel_head[level][slot] = event;
  scheduler->wheel_tail[level][slot] = event;
  event->wheel_level = level;
  event->wheel_slot = slot;
  event->location = GAME_EVENT_LOCATION_WHEEL;
  event->location_heap_index = GAME_SCHEDULER_INVALID_INDEX;
  event->state = GAME_EVENT_STATE_QUEUED;
}

static void wheel_remove(struct game_scheduler *scheduler, struct game_event *event)
{
  uint32_t level;
  uint32_t slot;

  level = event->wheel_level;
  slot = event->wheel_slot;
  if (event->wheel_previous != NULL)
    event->wheel_previous->wheel_next = event->wheel_next;
  else
    scheduler->wheel_head[level][slot] = event->wheel_next;
  if (event->wheel_next != NULL)
    event->wheel_next->wheel_previous = event->wheel_previous;
  else
    scheduler->wheel_tail[level][slot] = event->wheel_previous;
  event->wheel_previous = NULL;
  event->wheel_next = NULL;
  event->wheel_level = UINT32_MAX;
  event->wheel_slot = UINT32_MAX;
  event->location = GAME_EVENT_LOCATION_NONE;
}

static bool place_event_storage(struct game_scheduler *scheduler, struct game_event *event)
{
  game_tick_t delta;
  uint32_t level;
  uint32_t slot;

  if (event->deadline_tick <= scheduler->current_tick)
  {
    event->state = GAME_EVENT_STATE_READY;
    return location_heap_push(&scheduler->ready_heap, event);
  }

  delta = event->deadline_tick - scheduler->current_tick;
  for (level = 0; level < GAME_SCHEDULER_WHEEL_LEVELS; level++)
  {
    if (delta < (UINT64_C(1) << ((level + 1U) * GAME_SCHEDULER_WHEEL_BITS)))
    {
      slot = (uint32_t)((event->deadline_tick >> (level * GAME_SCHEDULER_WHEEL_BITS)) &
                        GAME_SCHEDULER_WHEEL_MASK);
      wheel_insert(scheduler, event, level, slot);
      return true;
    }
  }

  event->state = GAME_EVENT_STATE_QUEUED;
  return location_heap_push(&scheduler->overflow_heap, event);
}

static bool queue_event(struct game_scheduler *scheduler, struct game_event *event)
{
  if (!place_event_storage(scheduler, event))
    return false;
  if (!deadline_heap_push(&scheduler->deadline_heap, event))
  {
    if (event->location == GAME_EVENT_LOCATION_WHEEL)
      wheel_remove(scheduler, event);
    else if (event->location == GAME_EVENT_LOCATION_READY)
      location_heap_remove_at(&scheduler->ready_heap, event->location_heap_index);
    else if (event->location == GAME_EVENT_LOCATION_OVERFLOW)
      location_heap_remove_at(&scheduler->overflow_heap, event->location_heap_index);
    return false;
  }
  return true;
}

static void detach_event_location(struct game_scheduler *scheduler, struct game_event *event)
{
  switch (event->location)
  {
  case GAME_EVENT_LOCATION_WHEEL:
    wheel_remove(scheduler, event);
    break;
  case GAME_EVENT_LOCATION_OVERFLOW:
    location_heap_remove_at(&scheduler->overflow_heap, event->location_heap_index);
    break;
  case GAME_EVENT_LOCATION_READY:
    location_heap_remove_at(&scheduler->ready_heap, event->location_heap_index);
    break;
  case GAME_EVENT_LOCATION_NONE:
  case GAME_EVENT_LOCATION_DISPATCHING:
    break;
  }
}

static void finalize_event(struct game_scheduler *scheduler, struct game_event *event,
                           enum game_event_state terminal_state)
{
  struct game_event_type *event_type;

  event_type = find_event_type(scheduler, event->event_type);
  event->state = terminal_state;
  event->location = GAME_EVENT_LOCATION_NONE;
  deadline_heap_remove(&scheduler->deadline_heap, event);
  owner_event_remove(scheduler, event);
  registry_remove(scheduler, event);
  if (event_type != NULL && event_type->live_events > 0)
    event_type->live_events--;
  if (scheduler->event_count > 0)
    scheduler->event_count--;

  if (event->cleanup_pending)
  {
    if (event_type != NULL && event_type->cleanup != NULL)
      event_type->cleanup(event->payload);
    event->cleanup_pending = false;
    event->payload = NULL;
  }

  switch (terminal_state)
  {
  case GAME_EVENT_STATE_COMPLETED:
    scheduler->total_completed++;
    break;
  case GAME_EVENT_STATE_CANCELLED:
    scheduler->total_cancelled++;
    break;
  case GAME_EVENT_STATE_FAILED:
    scheduler->total_failed++;
    break;
  default:
    break;
  }
  free(event);
}

static void promote_overflow(struct game_scheduler *scheduler)
{
  struct game_event *event;
  game_tick_t delta;

  while (scheduler->overflow_heap.count > 0)
  {
    event = scheduler->overflow_heap.items[0];
    if (event->deadline_tick > scheduler->current_tick)
    {
      delta = event->deadline_tick - scheduler->current_tick;
      if (delta >= GAME_SCHEDULER_WHEEL_HORIZON)
        break;
    }
    location_heap_pop(&scheduler->overflow_heap);
    place_event_storage(scheduler, event);
    scheduler->total_overflow_promotions++;
  }
}

static void cascade_wheel_slot(struct game_scheduler *scheduler, uint32_t level, uint32_t slot)
{
  struct game_event *event;
  struct game_event *next;
  uint64_t cascaded;

  cascaded = 0;
  event = scheduler->wheel_head[level][slot];
  scheduler->wheel_head[level][slot] = NULL;
  scheduler->wheel_tail[level][slot] = NULL;
  while (event != NULL)
  {
    next = event->wheel_next;
    event->wheel_previous = NULL;
    event->wheel_next = NULL;
    event->wheel_level = UINT32_MAX;
    event->wheel_slot = UINT32_MAX;
    event->location = GAME_EVENT_LOCATION_NONE;
    place_event_storage(scheduler, event);
    cascaded++;
    event = next;
  }
  if (cascaded > 0)
  {
    scheduler->total_cascade_slots++;
    scheduler->total_cascaded_events += cascaded;
    if (cascaded > scheduler->largest_cascade)
      scheduler->largest_cascade = cascaded;
  }
}

static void drain_current_wheel_slot(struct game_scheduler *scheduler)
{
  uint32_t slot;

  slot = (uint32_t)(scheduler->current_tick & GAME_SCHEDULER_WHEEL_MASK);
  cascade_wheel_slot(scheduler, 0, slot);
}

static void advance_one_tick(struct game_scheduler *scheduler)
{
  uint32_t level;
  uint32_t slot;

  scheduler->current_tick++;
  if ((scheduler->current_tick & GAME_SCHEDULER_WHEEL_MASK) == 0)
  {
    for (level = 1; level < GAME_SCHEDULER_WHEEL_LEVELS; level++)
    {
      slot = (uint32_t)((scheduler->current_tick >> (level * GAME_SCHEDULER_WHEEL_BITS)) &
                        GAME_SCHEDULER_WHEEL_MASK);
      if (slot != 0)
      {
        cascade_wheel_slot(scheduler, level, slot);
        break;
      }
    }
    if (level == GAME_SCHEDULER_WHEEL_LEVELS)
    {
      for (level = GAME_SCHEDULER_WHEEL_LEVELS - 1U; level > 0; level--)
        cascade_wheel_slot(scheduler, level, 0);
    }
    else
    {
      while (level > 1U)
      {
        level--;
        cascade_wheel_slot(scheduler, level, 0);
      }
    }
  }
  promote_overflow(scheduler);
  drain_current_wheel_slot(scheduler);
}

static void advance_large(struct game_scheduler *scheduler, game_tick_t now_tick)
{
  struct game_event *detached_head;
  struct game_event *detached_tail;
  struct game_event *event;
  struct game_event *next;
  uint32_t level;
  uint32_t slot;
  uint64_t reclassified;

  reclassified = 0;
  detached_head = NULL;
  detached_tail = NULL;
  for (level = 0; level < GAME_SCHEDULER_WHEEL_LEVELS; level++)
  {
    for (slot = 0; slot < GAME_SCHEDULER_WHEEL_SLOTS; slot++)
    {
      event = scheduler->wheel_head[level][slot];
      scheduler->wheel_head[level][slot] = NULL;
      scheduler->wheel_tail[level][slot] = NULL;
      while (event != NULL)
      {
        next = event->wheel_next;
        event->wheel_previous = detached_tail;
        event->wheel_next = NULL;
        event->wheel_level = UINT32_MAX;
        event->wheel_slot = UINT32_MAX;
        event->location = GAME_EVENT_LOCATION_NONE;
        if (detached_tail != NULL)
          detached_tail->wheel_next = event;
        else
          detached_head = event;
        detached_tail = event;
        reclassified++;
        event = next;
      }
    }
  }

  scheduler->current_tick = now_tick;
  event = detached_head;
  while (event != NULL)
  {
    next = event->wheel_next;
    event->wheel_previous = NULL;
    event->wheel_next = NULL;
    place_event_storage(scheduler, event);
    event = next;
  }
  promote_overflow(scheduler);
  scheduler->total_large_advances++;
  scheduler->total_large_advance_events += reclassified;
}

static bool calculate_first_future_deadline(game_tick_t deadline_tick, game_tick_t interval_ticks,
                                            game_tick_t now_tick, game_tick_t *future_tick,
                                            uint64_t *skipped)
{
  uint64_t steps;

  if (interval_ticks == 0 || deadline_tick > now_tick)
  {
    *future_tick = deadline_tick;
    *skipped = 0;
    return true;
  }

  steps = (now_tick - deadline_tick) / interval_ticks + 1U;
  if (steps > (UINT64_MAX - deadline_tick) / interval_ticks)
    return false;
  *future_tick = deadline_tick + steps * interval_ticks;
  *skipped = steps;
  return true;
}

static uint64_t calculate_missed_occurrences(const struct game_scheduler *scheduler,
                                             const struct game_event *event)
{
  if (event->interval_ticks == 0 || scheduler->current_tick <= event->deadline_tick)
    return 0;
  return (scheduler->current_tick - event->deadline_tick) / event->interval_ticks;
}

static bool dispatch_time_budget_reached(const struct game_scheduler *scheduler,
                                         const struct game_scheduler_budget *budget,
                                         uint64_t started_usec)
{
  uint64_t current_usec;

  if (budget == NULL || budget->max_usec == 0)
    return false;
  current_usec = scheduler->config.monotonic_usec_now(scheduler->config.clock_context);
  if (current_usec < started_usec)
    return false;
  return current_usec - started_usec >= budget->max_usec;
}

static bool reschedule_dispatched_event(struct game_scheduler *scheduler, struct game_event *event,
                                        struct game_event_result result,
                                        struct game_scheduler_dispatch_report *report)
{
  struct game_event_type *event_type;
  game_tick_t deadline_tick;
  game_tick_t future_tick;
  game_tick_t interval_ticks;
  uint64_t skipped;
  uint64_t sequence;
  bool catch_up_again;

  event_type = find_event_type(scheduler, event->event_type);
  if (event_type == NULL)
    return false;
  deadline_tick = 0;
  future_tick = 0;
  interval_ticks = 0;
  skipped = 0;
  catch_up_again = false;

  if (result.kind == GAME_EVENT_RESULT_RESCHEDULE_AT)
  {
    if (normalize_deadline(scheduler, result.value, &deadline_tick) != GAME_SCHEDULER_OK)
      return false;
    event->interval_ticks = 0;
    event->catch_up_runs = 0;
  }
  else if (result.kind == GAME_EVENT_RESULT_RESCHEDULE_AFTER)
  {
    interval_ticks = result.value == 0 ? 1U : result.value;
    if (interval_ticks > UINT64_MAX - event->deadline_tick)
      return false;
    deadline_tick = event->deadline_tick + interval_ticks;
    event->interval_ticks = interval_ticks;

    if (deadline_tick <= scheduler->current_tick)
    {
      if (event_type->lateness_policy == GAME_EVENT_LATENESS_CATCH_UP_BOUNDED &&
          event->catch_up_runs + 1U < event_type->catch_up_limit)
      {
        event->catch_up_runs++;
        catch_up_again = true;
      }
      else
      {
        if (!calculate_first_future_deadline(deadline_tick, interval_ticks, scheduler->current_tick,
                                             &future_tick, &skipped))
          return false;
        deadline_tick = future_tick;
        if (event_type->lateness_policy == GAME_EVENT_LATENESS_COALESCE)
          report->coalesced_occurrences += skipped;
        else
          report->skipped_occurrences += skipped;
        event->catch_up_runs = 0;
      }
    }
    else
    {
      event->catch_up_runs = 0;
    }
  }
  else
  {
    return false;
  }

  if (!assign_insertion_sequence(scheduler, &sequence))
    return false;
  event->deadline_tick = deadline_tick;
  event->insertion_sequence = sequence;
  event->state = catch_up_again ? GAME_EVENT_STATE_READY : GAME_EVENT_STATE_QUEUED;
  if (!queue_event(scheduler, event))
    return false;
  report->rescheduled++;
  return true;
}

static bool skip_late_event(struct game_scheduler *scheduler, struct game_event *event,
                            struct game_scheduler_dispatch_report *report)
{
  struct game_event_type *event_type;
  game_tick_t future_tick;
  uint64_t skipped;
  uint64_t sequence;

  event_type = find_event_type(scheduler, event->event_type);
  if (event_type == NULL)
  {
    finalize_event(scheduler, event, GAME_EVENT_STATE_FAILED);
    report->failed++;
    return true;
  }
  if (event_type->lateness_policy != GAME_EVENT_LATENESS_SKIP_MISSED ||
      event->interval_ticks == 0 || scheduler->current_tick <= event->deadline_tick)
    return false;

  if (!calculate_first_future_deadline(event->deadline_tick, event->interval_ticks,
                                       scheduler->current_tick, &future_tick, &skipped) ||
      !assign_insertion_sequence(scheduler, &sequence))
  {
    event->state = GAME_EVENT_STATE_FAILED;
    finalize_event(scheduler, event, GAME_EVENT_STATE_FAILED);
    report->failed++;
    return true;
  }

  event->deadline_tick = future_tick;
  event->insertion_sequence = sequence;
  event->catch_up_runs = 0;
  event->state = GAME_EVENT_STATE_QUEUED;
  if (!queue_event(scheduler, event))
  {
    finalize_event(scheduler, event, GAME_EVENT_STATE_FAILED);
    report->failed++;
    return true;
  }
  report->skipped_occurrences += skipped;
  report->rescheduled++;
  return true;
}

static void dispatch_ready_events(struct game_scheduler *scheduler,
                                  const struct game_scheduler_budget *budget,
                                  struct game_scheduler_dispatch_report *report)
{
  struct game_event_context context;
  struct game_event_result result;
  struct game_event *event;
  struct game_event_type *event_type;
  uint64_t missed_occurrences;
  uint64_t started_usec;

  started_usec = 0;
  if (budget != NULL && budget->max_usec > 0)
    started_usec = scheduler->config.monotonic_usec_now(scheduler->config.clock_context);

  while (scheduler->ready_heap.count > 0 && !scheduler->shutting_down)
  {
    if (budget != NULL && budget->max_callbacks > 0 && report->callbacks >= budget->max_callbacks)
    {
      report->callback_budget_exhausted = true;
      break;
    }
    if (dispatch_time_budget_reached(scheduler, budget, started_usec))
    {
      report->time_budget_exhausted = true;
      break;
    }

    event = location_heap_pop(&scheduler->ready_heap);
    if (event == NULL)
      break;
    deadline_heap_remove(&scheduler->deadline_heap, event);
    if (skip_late_event(scheduler, event, report))
      continue;

    event_type = find_event_type(scheduler, event->event_type);
    if (event_type == NULL)
    {
      finalize_event(scheduler, event, GAME_EVENT_STATE_FAILED);
      report->failed++;
      continue;
    }
    missed_occurrences = calculate_missed_occurrences(scheduler, event);
    memset(&context, 0, sizeof(context));
    context.scheduler = scheduler;
    context.event_id = event->event_id;
    context.event_type = event->event_type;
    context.deadline_tick = event->deadline_tick;
    context.now_tick = scheduler->current_tick;
    context.missed_occurrences =
        event_type->lateness_policy == GAME_EVENT_LATENESS_RUN_ONCE ? 0 : missed_occurrences;
    context.owner = event->owner;
    context.payload = event->payload;

    event->state = GAME_EVENT_STATE_DISPATCHING;
    event->location = GAME_EVENT_LOCATION_DISPATCHING;
    scheduler->dispatch_depth++;
    result = event_type->handler(&context);
    scheduler->dispatch_depth--;
    scheduler->total_callbacks++;
    report->callbacks++;
    if (scheduler->current_tick > event->deadline_tick)
      report->late_callbacks++;
    report->missed_occurrences += context.missed_occurrences;

    if (event->state == GAME_EVENT_STATE_CANCEL_PENDING || scheduler->shutting_down)
    {
      finalize_event(scheduler, event, GAME_EVENT_STATE_CANCELLED);
      report->cancelled++;
      continue;
    }

    if (result.kind == GAME_EVENT_RESULT_COMPLETE)
    {
      finalize_event(scheduler, event, GAME_EVENT_STATE_COMPLETED);
      report->completed++;
    }
    else if (result.kind == GAME_EVENT_RESULT_FAILED)
    {
      finalize_event(scheduler, event, GAME_EVENT_STATE_FAILED);
      report->failed++;
    }
    else if (!reschedule_dispatched_event(scheduler, event, result, report))
    {
      finalize_event(scheduler, event, GAME_EVENT_STATE_FAILED);
      report->failed++;
    }
  }
}

struct game_event_result game_event_result_complete(void)
{
  struct game_event_result result;

  memset(&result, 0, sizeof(result));
  result.kind = GAME_EVENT_RESULT_COMPLETE;
  return result;
}

struct game_event_result game_event_result_reschedule_at(game_tick_t deadline_tick)
{
  struct game_event_result result;

  memset(&result, 0, sizeof(result));
  result.kind = GAME_EVENT_RESULT_RESCHEDULE_AT;
  result.value = deadline_tick;
  return result;
}

struct game_event_result game_event_result_reschedule_after(game_tick_t delay_ticks)
{
  struct game_event_result result;

  memset(&result, 0, sizeof(result));
  result.kind = GAME_EVENT_RESULT_RESCHEDULE_AFTER;
  result.value = delay_ticks;
  return result;
}

struct game_event_result game_event_result_failed(uint32_t diagnostic_code)
{
  struct game_event_result result;

  memset(&result, 0, sizeof(result));
  result.kind = GAME_EVENT_RESULT_FAILED;
  result.diagnostic_code = diagnostic_code;
  return result;
}

struct game_event_owner game_event_owner_none(void)
{
  struct game_event_owner owner;

  memset(&owner, 0, sizeof(owner));
  return owner;
}

bool game_event_owner_is_none(struct game_event_owner owner)
{
  return owner.kind == GAME_EVENT_OWNER_NONE && owner.runtime_id == 0 && owner.generation == 0;
}

bool game_event_owner_is_valid(struct game_event_owner owner)
{
  return owner.kind > GAME_EVENT_OWNER_NONE && owner.kind <= GAME_EVENT_OWNER_SERVICE &&
         owner.runtime_id != 0 && owner.generation != 0;
}

bool game_event_owner_equal(struct game_event_owner left, struct game_event_owner right)
{
  return left.kind == right.kind && left.runtime_id == right.runtime_id &&
         left.generation == right.generation;
}

struct game_scheduler *game_scheduler_create(const struct game_scheduler_config *config,
                                             enum game_scheduler_status *status)
{
  struct game_scheduler *scheduler;
  struct game_scheduler_config resolved;
  size_t registry_target;
  size_t registry_buckets;

  if (status != NULL)
    *status = GAME_SCHEDULER_INVALID_ARGUMENT;
  if (config == NULL || config->tick_now == NULL)
    return NULL;

  resolved = *config;
  if (resolved.max_events == 0)
    resolved.max_events = GAME_SCHEDULER_DEFAULT_MAX_EVENTS;
  if (resolved.max_event_types == 0)
    resolved.max_event_types = GAME_SCHEDULER_DEFAULT_MAX_EVENT_TYPES;
  if (resolved.max_events > SIZE_MAX / sizeof(struct game_event *) ||
      resolved.max_event_types > SIZE_MAX / sizeof(struct game_event_type) ||
      resolved.max_events > SIZE_MAX / 2U)
  {
    if (status != NULL)
      *status = GAME_SCHEDULER_ALLOCATION_FAILED;
    return NULL;
  }
  registry_target = resolved.max_events * 2U;
  if (registry_target < 16U)
    registry_target = 16U;
  if (!next_power_of_two(registry_target, &registry_buckets))
  {
    if (status != NULL)
      *status = GAME_SCHEDULER_ALLOCATION_FAILED;
    return NULL;
  }

  scheduler = calloc(1, sizeof(*scheduler));
  if (scheduler == NULL)
  {
    if (status != NULL)
      *status = GAME_SCHEDULER_ALLOCATION_FAILED;
    return NULL;
  }
  scheduler->config = resolved;
  scheduler->registry_bucket_count = registry_buckets;
  scheduler->owner_bucket_count = registry_buckets;
  scheduler->overflow_heap.capacity = resolved.max_events;
  scheduler->overflow_heap.location = GAME_EVENT_LOCATION_OVERFLOW;
  scheduler->ready_heap.capacity = resolved.max_events;
  scheduler->ready_heap.location = GAME_EVENT_LOCATION_READY;
  scheduler->deadline_heap.capacity = resolved.max_events;
  scheduler->next_event_id = 1U;
  scheduler->next_insertion_sequence = 1U;

  scheduler->overflow_heap.items = calloc(resolved.max_events, sizeof(struct game_event *));
  scheduler->ready_heap.items = calloc(resolved.max_events, sizeof(struct game_event *));
  scheduler->deadline_heap.items = calloc(resolved.max_events, sizeof(struct game_event *));
  scheduler->registry_buckets = calloc(registry_buckets, sizeof(struct game_event *));
  scheduler->owner_buckets = calloc(registry_buckets, sizeof(struct game_event_owner_entry *));
  scheduler->event_types = calloc(resolved.max_event_types, sizeof(struct game_event_type));
  if (scheduler->overflow_heap.items == NULL || scheduler->ready_heap.items == NULL ||
      scheduler->deadline_heap.items == NULL || scheduler->registry_buckets == NULL ||
      scheduler->owner_buckets == NULL || scheduler->event_types == NULL)
  {
    free(scheduler->event_types);
    free(scheduler->owner_buckets);
    free(scheduler->registry_buckets);
    free(scheduler->deadline_heap.items);
    free(scheduler->ready_heap.items);
    free(scheduler->overflow_heap.items);
    free(scheduler);
    if (status != NULL)
      *status = GAME_SCHEDULER_ALLOCATION_FAILED;
    return NULL;
  }

  scheduler->current_tick = resolved.tick_now(resolved.clock_context);
  if (status != NULL)
    *status = GAME_SCHEDULER_OK;
  return scheduler;
}

enum game_scheduler_status game_scheduler_shutdown(struct game_scheduler *scheduler)
{
  struct game_event *event;
  struct game_event *next;
  size_t bucket;

  if (scheduler == NULL)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  if (scheduler->shutting_down)
    return GAME_SCHEDULER_OK;

  scheduler->shutting_down = true;
  for (bucket = 0; bucket < scheduler->registry_bucket_count; bucket++)
  {
    event = scheduler->registry_buckets[bucket];
    while (event != NULL)
    {
      next = event->registry_next;
      if (event->state == GAME_EVENT_STATE_DISPATCHING ||
          event->state == GAME_EVENT_STATE_CANCEL_PENDING)
      {
        event->state = GAME_EVENT_STATE_CANCEL_PENDING;
      }
      else
      {
        detach_event_location(scheduler, event);
        deadline_heap_remove(&scheduler->deadline_heap, event);
        finalize_event(scheduler, event, GAME_EVENT_STATE_CANCELLED);
      }
      event = next;
    }
  }
  return GAME_SCHEDULER_OK;
}

enum game_scheduler_status game_scheduler_destroy(struct game_scheduler *scheduler)
{
  size_t event_type;

  if (scheduler == NULL)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  if (scheduler->dispatch_depth > 0)
    return GAME_SCHEDULER_BUSY;

  game_scheduler_shutdown(scheduler);
  for (event_type = 0; event_type < scheduler->event_type_count; event_type++)
    free(scheduler->event_types[event_type].name);
  free(scheduler->event_types);
  free(scheduler->owner_buckets);
  free(scheduler->registry_buckets);
  free(scheduler->deadline_heap.items);
  free(scheduler->ready_heap.items);
  free(scheduler->overflow_heap.items);
  free(scheduler);
  return GAME_SCHEDULER_OK;
}

enum game_scheduler_status game_scheduler_register_type(struct game_scheduler *scheduler,
                                                        const struct game_event_type_config *config,
                                                        game_event_type_id_t *event_type)
{
  struct game_event_type *registered;
  char *name;
  size_t index;
  size_t name_length;

  if (scheduler == NULL || config == NULL || event_type == NULL || config->name == NULL ||
      *config->name == '\0' || config->handler == NULL)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  if (scheduler->shutting_down)
    return GAME_SCHEDULER_SHUTTING_DOWN;
  if (scheduler->event_types_sealed)
    return GAME_SCHEDULER_REGISTRATION_CLOSED;
  if (config->lateness_policy < GAME_EVENT_LATENESS_RUN_ONCE ||
      config->lateness_policy > GAME_EVENT_LATENESS_CATCH_UP_BOUNDED ||
      (config->lateness_policy == GAME_EVENT_LATENESS_CATCH_UP_BOUNDED &&
       config->catch_up_limit == 0))
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  if (scheduler->event_type_count >= scheduler->config.max_event_types ||
      scheduler->event_type_count >= UINT32_MAX)
    return GAME_SCHEDULER_CAPACITY_REACHED;

  for (index = 0; index < scheduler->event_type_count; index++)
  {
    if (strcmp(scheduler->event_types[index].name, config->name) == 0)
      return GAME_SCHEDULER_INVALID_TYPE;
  }

  name_length = strlen(config->name);
  if (name_length == SIZE_MAX)
    return GAME_SCHEDULER_ALLOCATION_FAILED;
  name = malloc(name_length + 1U);
  if (name == NULL)
    return GAME_SCHEDULER_ALLOCATION_FAILED;
  memcpy(name, config->name, name_length + 1U);

  registered = &scheduler->event_types[scheduler->event_type_count];
  registered->name = name;
  registered->handler = config->handler;
  registered->cleanup = config->cleanup;
  registered->lateness_policy = config->lateness_policy;
  registered->catch_up_limit = config->catch_up_limit;
  registered->max_events = config->max_events;
  registered->max_events_per_owner = config->max_events_per_owner;
  registered->requires_owner = config->requires_owner;
  registered->cleanup_on_null_payload = config->cleanup_on_null_payload;
  scheduler->event_type_count++;
  *event_type = (game_event_type_id_t)scheduler->event_type_count;
  return GAME_SCHEDULER_OK;
}

enum game_scheduler_status
game_scheduler_rollback_type_registrations(struct game_scheduler *scheduler,
                                           size_t registered_type_count)
{
  size_t event_type;

  if (scheduler == NULL || registered_type_count > scheduler->event_type_count)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  if (scheduler->shutting_down)
    return GAME_SCHEDULER_SHUTTING_DOWN;
  if (scheduler->event_types_sealed)
    return GAME_SCHEDULER_REGISTRATION_CLOSED;
  for (event_type = registered_type_count; event_type < scheduler->event_type_count; event_type++)
    if (scheduler->event_types[event_type].live_events > 0U)
      return GAME_SCHEDULER_BUSY;

  for (event_type = registered_type_count; event_type < scheduler->event_type_count; event_type++)
  {
    free(scheduler->event_types[event_type].name);
    memset(&scheduler->event_types[event_type], 0, sizeof(scheduler->event_types[event_type]));
  }
  scheduler->event_type_count = registered_type_count;
  return GAME_SCHEDULER_OK;
}

enum game_scheduler_status game_scheduler_seal_types(struct game_scheduler *scheduler)
{
  if (scheduler == NULL)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  if (scheduler->shutting_down)
    return GAME_SCHEDULER_SHUTTING_DOWN;
  scheduler->event_types_sealed = true;
  return GAME_SCHEDULER_OK;
}

bool game_scheduler_types_are_sealed(const struct game_scheduler *scheduler)
{
  return scheduler != NULL && scheduler->event_types_sealed;
}

const char *game_scheduler_type_name(const struct game_scheduler *scheduler,
                                     game_event_type_id_t event_type)
{
  if (scheduler == NULL || event_type == 0 || event_type > scheduler->event_type_count)
    return NULL;
  return scheduler->event_types[event_type - 1U].name;
}

game_tick_t game_scheduler_current_tick(const struct game_scheduler *scheduler)
{
  return scheduler != NULL ? scheduler->current_tick : 0U;
}

size_t game_scheduler_event_count(const struct game_scheduler *scheduler)
{
  return scheduler != NULL ? scheduler->event_count : 0U;
}

enum game_scheduler_status game_scheduler_type_live_count(
    const struct game_scheduler *scheduler, game_event_type_id_t event_type,
    size_t *live_count)
{
  if (scheduler == NULL || live_count == NULL)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  if (event_type == 0U || event_type > scheduler->event_type_count)
    return GAME_SCHEDULER_INVALID_TYPE;
  *live_count = scheduler->event_types[event_type - 1U].live_events;
  return GAME_SCHEDULER_OK;
}

static enum game_scheduler_status schedule_normalized(struct game_scheduler *scheduler,
                                                      game_event_type_id_t event_type,
                                                      struct game_event_owner owner,
                                                      game_tick_t deadline_tick, void *payload,
                                                      game_event_id_t *event_id)
{
  struct game_event_owner_entry *owner_entry;
  struct game_event_type *registered_type;
  struct game_event *event;
  game_event_id_t assigned_id;
  uint64_t assigned_sequence;

  registered_type = find_event_type(scheduler, event_type);
  if (registered_type == NULL)
    return GAME_SCHEDULER_INVALID_TYPE;
  if (payload != NULL && registered_type->cleanup == NULL)
    return GAME_SCHEDULER_INVALID_PAYLOAD;
  if (!game_event_owner_is_none(owner) && !game_event_owner_is_valid(owner))
  {
    scheduler->total_invalid_owner_rejections++;
    return GAME_SCHEDULER_INVALID_OWNER;
  }
  if (registered_type->requires_owner && game_event_owner_is_none(owner))
  {
    scheduler->total_invalid_owner_rejections++;
    return GAME_SCHEDULER_INVALID_OWNER;
  }
  if (scheduler->event_count >= scheduler->config.max_events)
  {
    scheduler->total_capacity_rejections++;
    return GAME_SCHEDULER_CAPACITY_REACHED;
  }
  if (registered_type->max_events > 0 &&
      registered_type->live_events >= registered_type->max_events)
  {
    scheduler->total_type_capacity_rejections++;
    return GAME_SCHEDULER_TYPE_CAPACITY_REACHED;
  }
  if (scheduler->event_id_exhausted || scheduler->insertion_sequence_exhausted)
    return GAME_SCHEDULER_ID_EXHAUSTED;

  owner_entry = game_event_owner_is_none(owner) ? NULL : owner_find(scheduler, owner);
  if (owner_entry != NULL && scheduler->config.max_events_per_owner > 0 &&
      owner_entry->live_events >= scheduler->config.max_events_per_owner)
  {
    scheduler->total_owner_capacity_rejections++;
    return GAME_SCHEDULER_OWNER_CAPACITY_REACHED;
  }
  if (owner_entry != NULL && registered_type->max_events_per_owner > 0 &&
      owner_type_event_count(owner_entry, event_type) >= registered_type->max_events_per_owner)
  {
    scheduler->total_owner_type_capacity_rejections++;
    return GAME_SCHEDULER_OWNER_TYPE_CAPACITY_REACHED;
  }

  event = calloc(1, sizeof(*event));
  if (event == NULL)
    return GAME_SCHEDULER_ALLOCATION_FAILED;
  if (!game_event_owner_is_none(owner) && owner_entry == NULL)
  {
    owner_entry = calloc(1, sizeof(*owner_entry));
    if (owner_entry == NULL)
    {
      free(event);
      return GAME_SCHEDULER_ALLOCATION_FAILED;
    }
    owner_entry->owner = owner;
  }
  if (!assign_event_identity(scheduler, &assigned_id, &assigned_sequence))
  {
    if (owner_entry != NULL && owner_entry->live_events == 0)
      free(owner_entry);
    free(event);
    return GAME_SCHEDULER_ID_EXHAUSTED;
  }

  event->event_id = assigned_id;
  event->event_type = event_type;
  event->state = GAME_EVENT_STATE_CREATED;
  event->location = GAME_EVENT_LOCATION_NONE;
  event->deadline_tick = deadline_tick;
  event->insertion_sequence = assigned_sequence;
  event->payload = payload;
  event->owner = owner;
  event->wheel_level = UINT32_MAX;
  event->wheel_slot = UINT32_MAX;
  event->location_heap_index = GAME_SCHEDULER_INVALID_INDEX;
  event->deadline_heap_index = GAME_SCHEDULER_INVALID_INDEX;

  if (!queue_event(scheduler, event))
  {
    if (owner_entry != NULL && owner_entry->live_events == 0)
      free(owner_entry);
    free(event);
    return GAME_SCHEDULER_ALLOCATION_FAILED;
  }
  registry_insert(scheduler, event);
  if (owner_entry != NULL)
  {
    if (owner_entry->live_events == 0)
      owner_entry_insert(scheduler, owner_entry);
    owner_event_insert(owner_entry, event);
  }
  event->cleanup_pending =
      (payload != NULL || registered_type->cleanup_on_null_payload) &&
      registered_type->cleanup != NULL;
  scheduler->event_count++;
  registered_type->live_events++;
  scheduler->total_scheduled++;
  *event_id = assigned_id;
  return GAME_SCHEDULER_OK;
}

enum game_scheduler_status game_scheduler_schedule_at(struct game_scheduler *scheduler,
                                                      game_event_type_id_t event_type,
                                                      game_tick_t deadline_tick, void *payload,
                                                      game_event_id_t *event_id)
{
  enum game_scheduler_status status;
  game_tick_t normalized;

  if (scheduler == NULL || event_id == NULL)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  if (scheduler->shutting_down)
    return GAME_SCHEDULER_SHUTTING_DOWN;
  status = normalize_deadline(scheduler, deadline_tick, &normalized);
  if (status != GAME_SCHEDULER_OK)
    return status;
  return schedule_normalized(scheduler, event_type, game_event_owner_none(), normalized, payload,
                             event_id);
}

enum game_scheduler_status game_scheduler_schedule_after(struct game_scheduler *scheduler,
                                                         game_event_type_id_t event_type,
                                                         game_tick_t delay_ticks, void *payload,
                                                         game_event_id_t *event_id)
{
  enum game_scheduler_status status;
  game_tick_t deadline_tick;

  if (scheduler == NULL || event_id == NULL)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  if (scheduler->shutting_down)
    return GAME_SCHEDULER_SHUTTING_DOWN;
  status = deadline_after(scheduler, delay_ticks, &deadline_tick);
  if (status != GAME_SCHEDULER_OK)
    return status;
  return schedule_normalized(scheduler, event_type, game_event_owner_none(), deadline_tick, payload,
                             event_id);
}

enum game_scheduler_status game_scheduler_schedule_owned_at(
    struct game_scheduler *scheduler, game_event_type_id_t event_type,
    struct game_event_owner owner, game_tick_t deadline_tick, void *payload,
    game_event_id_t *event_id)
{
  enum game_scheduler_status status;
  game_tick_t normalized;

  if (scheduler == NULL || event_id == NULL)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  if (scheduler->shutting_down)
    return GAME_SCHEDULER_SHUTTING_DOWN;
  status = normalize_deadline(scheduler, deadline_tick, &normalized);
  if (status != GAME_SCHEDULER_OK)
    return status;
  return schedule_normalized(scheduler, event_type, owner, normalized, payload, event_id);
}

enum game_scheduler_status game_scheduler_schedule_owned_after(
    struct game_scheduler *scheduler, game_event_type_id_t event_type,
    struct game_event_owner owner, game_tick_t delay_ticks, void *payload,
    game_event_id_t *event_id)
{
  enum game_scheduler_status status;
  game_tick_t deadline_tick;

  if (scheduler == NULL || event_id == NULL)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  if (scheduler->shutting_down)
    return GAME_SCHEDULER_SHUTTING_DOWN;
  status = deadline_after(scheduler, delay_ticks, &deadline_tick);
  if (status != GAME_SCHEDULER_OK)
    return status;
  return schedule_normalized(scheduler, event_type, owner, deadline_tick, payload, event_id);
}

enum game_event_cancel_result game_scheduler_cancel(struct game_scheduler *scheduler,
                                                    game_event_id_t event_id)
{
  struct game_event *event;

  event = registry_find(scheduler, event_id);
  if (event == NULL)
    return GAME_EVENT_CANCEL_NOT_FOUND;
  if (event->state == GAME_EVENT_STATE_DISPATCHING ||
      event->state == GAME_EVENT_STATE_CANCEL_PENDING)
  {
    event->state = GAME_EVENT_STATE_CANCEL_PENDING;
    return GAME_EVENT_CANCEL_PENDING;
  }

  detach_event_location(scheduler, event);
  deadline_heap_remove(&scheduler->deadline_heap, event);
  finalize_event(scheduler, event, GAME_EVENT_STATE_CANCELLED);
  return GAME_EVENT_CANCELLED;
}

enum game_scheduler_status game_scheduler_cancel_owner(struct game_scheduler *scheduler,
                                                       struct game_event_owner owner,
                                                       size_t *cancelled_count)
{
  struct game_event_owner_entry *entry;
  struct game_event *event;
  struct game_event *next;
  size_t count;

  if (scheduler == NULL || cancelled_count == NULL)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  *cancelled_count = 0;
  if (!game_event_owner_is_valid(owner))
    return GAME_SCHEDULER_INVALID_OWNER;

  entry = owner_find(scheduler, owner);
  if (entry == NULL)
    return GAME_SCHEDULER_OK;

  count = 0;
  event = entry->events_head;
  while (event != NULL)
  {
    next = event->owner_next;
    game_scheduler_cancel(scheduler, event->event_id);
    count++;
    event = next;
  }
  *cancelled_count = count;
  return GAME_SCHEDULER_OK;
}

static enum game_scheduler_status reschedule_normalized(struct game_scheduler *scheduler,
                                                        game_event_id_t event_id,
                                                        game_tick_t deadline_tick)
{
  struct game_event *event;
  uint64_t sequence;

  if (scheduler->shutting_down)
    return GAME_SCHEDULER_SHUTTING_DOWN;
  event = registry_find(scheduler, event_id);
  if (event == NULL)
    return GAME_SCHEDULER_NOT_FOUND;
  if (event->state == GAME_EVENT_STATE_DISPATCHING ||
      event->state == GAME_EVENT_STATE_CANCEL_PENDING)
    return GAME_SCHEDULER_BUSY;
  if (!assign_insertion_sequence(scheduler, &sequence))
    return GAME_SCHEDULER_ID_EXHAUSTED;

  detach_event_location(scheduler, event);
  deadline_heap_remove(&scheduler->deadline_heap, event);
  event->deadline_tick = deadline_tick;
  event->interval_ticks = 0;
  event->catch_up_runs = 0;
  event->insertion_sequence = sequence;
  if (!queue_event(scheduler, event))
  {
    finalize_event(scheduler, event, GAME_EVENT_STATE_FAILED);
    return GAME_SCHEDULER_ALLOCATION_FAILED;
  }
  return GAME_SCHEDULER_OK;
}

enum game_scheduler_status game_scheduler_reschedule_at(struct game_scheduler *scheduler,
                                                        game_event_id_t event_id,
                                                        game_tick_t deadline_tick)
{
  enum game_scheduler_status status;
  game_tick_t normalized;

  if (scheduler == NULL)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  status = normalize_deadline(scheduler, deadline_tick, &normalized);
  if (status != GAME_SCHEDULER_OK)
    return status;
  return reschedule_normalized(scheduler, event_id, normalized);
}

enum game_scheduler_status game_scheduler_reschedule_after(struct game_scheduler *scheduler,
                                                           game_event_id_t event_id,
                                                           game_tick_t delay_ticks)
{
  enum game_scheduler_status status;
  game_tick_t deadline_tick;

  if (scheduler == NULL)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  status = deadline_after(scheduler, delay_ticks, &deadline_tick);
  if (status != GAME_SCHEDULER_OK)
    return status;
  return reschedule_normalized(scheduler, event_id, deadline_tick);
}

enum game_scheduler_status game_scheduler_remaining(const struct game_scheduler *scheduler,
                                                    game_event_id_t event_id,
                                                    game_tick_t *remaining_ticks)
{
  struct game_event *event;

  if (scheduler == NULL || remaining_ticks == NULL)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  event = registry_find(scheduler, event_id);
  if (event == NULL)
    return GAME_SCHEDULER_NOT_FOUND;
  if (event->deadline_tick <= scheduler->current_tick)
    *remaining_ticks = 0;
  else
    *remaining_ticks = event->deadline_tick - scheduler->current_tick;
  return GAME_SCHEDULER_OK;
}

enum game_scheduler_status game_scheduler_advance(struct game_scheduler *scheduler,
                                                  const struct game_scheduler_budget *budget,
                                                  struct game_scheduler_dispatch_report *report)
{
  game_tick_t now_tick;
  game_tick_t delta;
  uint64_t cascade_slots_before;
  uint64_t cascaded_events_before;
  uint64_t overflow_promotions_before;
  uint64_t large_advance_events_before;

  if (scheduler == NULL || report == NULL)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  if (scheduler->shutting_down)
    return GAME_SCHEDULER_SHUTTING_DOWN;
  if (scheduler->dispatch_depth > 0)
    return GAME_SCHEDULER_BUSY;
  if (budget != NULL && budget->max_usec > 0 && scheduler->config.monotonic_usec_now == NULL)
    return GAME_SCHEDULER_INVALID_ARGUMENT;

  memset(report, 0, sizeof(*report));
  report->previous_tick = scheduler->current_tick;
  cascade_slots_before = scheduler->total_cascade_slots;
  cascaded_events_before = scheduler->total_cascaded_events;
  overflow_promotions_before = scheduler->total_overflow_promotions;
  large_advance_events_before = scheduler->total_large_advance_events;
  now_tick = scheduler->config.tick_now(scheduler->config.clock_context);
  if (now_tick < scheduler->current_tick)
    return GAME_SCHEDULER_CLOCK_REVERSED;
  delta = now_tick - scheduler->current_tick;
  scheduler->total_ticks_advanced += delta;
  if (delta > GAME_SCHEDULER_LARGE_ADVANCE_TICKS)
  {
    advance_large(scheduler, now_tick);
    report->used_large_advance = true;
  }
  else
  {
    while (scheduler->current_tick < now_tick)
      advance_one_tick(scheduler);
  }

  dispatch_ready_events(scheduler, budget, report);
  report->current_tick = scheduler->current_tick;
  report->ticks_advanced = delta;
  report->cascade_slots = scheduler->total_cascade_slots - cascade_slots_before;
  report->cascaded_events = scheduler->total_cascaded_events - cascaded_events_before;
  report->overflow_promotions = scheduler->total_overflow_promotions - overflow_promotions_before;
  report->large_advance_events =
      scheduler->total_large_advance_events - large_advance_events_before;
  report->ready_remaining = scheduler->ready_heap.count;
  report->events_remaining = scheduler->event_count;
  scheduler->total_rescheduled += report->rescheduled;
  scheduler->total_late_callbacks += report->late_callbacks;
  scheduler->total_missed_occurrences += report->missed_occurrences;
  scheduler->total_skipped_occurrences += report->skipped_occurrences;
  scheduler->total_coalesced_occurrences += report->coalesced_occurrences;
  return GAME_SCHEDULER_OK;
}

enum game_scheduler_status game_scheduler_next_deadline(const struct game_scheduler *scheduler,
                                                        game_tick_t *deadline_tick,
                                                        bool *has_deadline)
{
  if (scheduler == NULL || deadline_tick == NULL || has_deadline == NULL)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  if (scheduler->deadline_heap.count == 0)
  {
    *has_deadline = false;
    *deadline_tick = 0;
  }
  else
  {
    *has_deadline = true;
    *deadline_tick = scheduler->deadline_heap.items[0]->deadline_tick;
  }
  return GAME_SCHEDULER_OK;
}

enum game_scheduler_status game_scheduler_inspect(const struct game_scheduler *scheduler,
                                                  game_event_id_t event_id,
                                                  struct game_event_snapshot *snapshot)
{
  struct game_event *event;

  if (scheduler == NULL || snapshot == NULL)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  event = registry_find(scheduler, event_id);
  if (event == NULL)
    return GAME_SCHEDULER_NOT_FOUND;

  memset(snapshot, 0, sizeof(*snapshot));
  snapshot->event_id = event->event_id;
  snapshot->event_type = event->event_type;
  snapshot->state = event->state;
  snapshot->location = event->location;
  snapshot->deadline_tick = event->deadline_tick;
  snapshot->interval_ticks = event->interval_ticks;
  snapshot->insertion_sequence = event->insertion_sequence;
  snapshot->wheel_level = event->wheel_level;
  snapshot->wheel_slot = event->wheel_slot;
  snapshot->owner = event->owner;
  return GAME_SCHEDULER_OK;
}

enum game_scheduler_status game_scheduler_inspect_owner(
    const struct game_scheduler *scheduler, struct game_event_owner owner,
    struct game_event_snapshot *snapshots, size_t snapshot_capacity, size_t *event_count)
{
  struct game_event_owner_entry *entry;
  struct game_event *event;
  size_t index;

  if (scheduler == NULL || event_count == NULL ||
      (snapshot_capacity > 0 && snapshots == NULL))
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  *event_count = 0;
  if (!game_event_owner_is_valid(owner))
    return GAME_SCHEDULER_INVALID_OWNER;
  entry = owner_find(scheduler, owner);
  if (entry == NULL)
    return GAME_SCHEDULER_OK;

  index = 0;
  for (event = entry->events_head; event != NULL; event = event->owner_next)
  {
    if (index < snapshot_capacity)
    {
      memset(&snapshots[index], 0, sizeof(snapshots[index]));
      snapshots[index].event_id = event->event_id;
      snapshots[index].event_type = event->event_type;
      snapshots[index].state = event->state;
      snapshots[index].location = event->location;
      snapshots[index].deadline_tick = event->deadline_tick;
      snapshots[index].interval_ticks = event->interval_ticks;
      snapshots[index].insertion_sequence = event->insertion_sequence;
      snapshots[index].wheel_level = event->wheel_level;
      snapshots[index].wheel_slot = event->wheel_slot;
      snapshots[index].owner = event->owner;
    }
    index++;
  }
  *event_count = index;
  return GAME_SCHEDULER_OK;
}

enum game_scheduler_status game_scheduler_inspect_all(
    const struct game_scheduler *scheduler, struct game_event_snapshot *snapshots,
    size_t snapshot_capacity, size_t *event_count)
{
  struct game_event *event;
  size_t bucket;
  size_t index;

  if (scheduler == NULL || event_count == NULL ||
      (snapshot_capacity > 0 && snapshots == NULL))
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  index = 0;
  for (bucket = 0; bucket < scheduler->registry_bucket_count; bucket++)
  {
    for (event = scheduler->registry_buckets[bucket]; event != NULL;
         event = event->registry_next)
    {
      if (index < snapshot_capacity)
      {
        memset(&snapshots[index], 0, sizeof(snapshots[index]));
        snapshots[index].event_id = event->event_id;
        snapshots[index].event_type = event->event_type;
        snapshots[index].state = event->state;
        snapshots[index].location = event->location;
        snapshots[index].deadline_tick = event->deadline_tick;
        snapshots[index].interval_ticks = event->interval_ticks;
        snapshots[index].insertion_sequence = event->insertion_sequence;
        snapshots[index].wheel_level = event->wheel_level;
        snapshots[index].wheel_slot = event->wheel_slot;
        snapshots[index].owner = event->owner;
      }
      index++;
    }
  }
  *event_count = index;
  return GAME_SCHEDULER_OK;
}

void game_scheduler_get_stats(const struct game_scheduler *scheduler,
                              struct game_scheduler_stats *stats)
{
  const struct game_event_owner_entry *owner_entry;
  const struct game_event *event;
  size_t bucket;
  size_t level;
  size_t slot;

  if (stats == NULL)
    return;
  memset(stats, 0, sizeof(*stats));
  if (scheduler == NULL)
    return;

  stats->current_tick = scheduler->current_tick;
  stats->event_count = scheduler->event_count;
  stats->ready_count = scheduler->ready_heap.count;
  stats->overflow_count = scheduler->overflow_heap.count;
  for (level = 0; level < GAME_SCHEDULER_WHEEL_LEVELS; level++)
  {
    for (slot = 0; slot < GAME_SCHEDULER_WHEEL_SLOTS; slot++)
    {
      for (event = scheduler->wheel_head[level][slot]; event != NULL;
           event = event->wheel_next)
        stats->wheel_level_counts[level]++;
    }
  }
  if (scheduler->ready_heap.count > 0 &&
      scheduler->ready_heap.items[0]->deadline_tick < scheduler->current_tick)
    stats->oldest_overdue_ticks =
        scheduler->current_tick - scheduler->ready_heap.items[0]->deadline_tick;
  stats->registered_type_count = scheduler->event_type_count;
  stats->owner_count = scheduler->owner_count;
  for (bucket = 0; bucket < scheduler->owner_bucket_count; bucket++)
  {
    for (owner_entry = scheduler->owner_buckets[bucket]; owner_entry != NULL;
         owner_entry = owner_entry->hash_next)
      stats->owner_counts[owner_entry->owner.kind]++;
  }
  stats->total_scheduled = scheduler->total_scheduled;
  stats->total_callbacks = scheduler->total_callbacks;
  stats->total_cancelled = scheduler->total_cancelled;
  stats->total_completed = scheduler->total_completed;
  stats->total_failed = scheduler->total_failed;
  stats->total_rescheduled = scheduler->total_rescheduled;
  stats->total_late_callbacks = scheduler->total_late_callbacks;
  stats->total_missed_occurrences = scheduler->total_missed_occurrences;
  stats->total_skipped_occurrences = scheduler->total_skipped_occurrences;
  stats->total_coalesced_occurrences = scheduler->total_coalesced_occurrences;
  stats->total_capacity_rejections = scheduler->total_capacity_rejections;
  stats->total_type_capacity_rejections = scheduler->total_type_capacity_rejections;
  stats->total_invalid_owner_rejections = scheduler->total_invalid_owner_rejections;
  stats->total_owner_capacity_rejections = scheduler->total_owner_capacity_rejections;
  stats->total_owner_type_capacity_rejections =
      scheduler->total_owner_type_capacity_rejections;
  stats->total_ticks_advanced = scheduler->total_ticks_advanced;
  stats->total_cascade_slots = scheduler->total_cascade_slots;
  stats->total_cascaded_events = scheduler->total_cascaded_events;
  stats->total_overflow_promotions = scheduler->total_overflow_promotions;
  stats->total_large_advances = scheduler->total_large_advances;
  stats->total_large_advance_events = scheduler->total_large_advance_events;
  stats->largest_cascade = scheduler->largest_cascade;
}

#ifdef LUMINARI_CUTEST
enum game_scheduler_status game_scheduler_test_set_sequences(struct game_scheduler *scheduler,
                                                             game_event_id_t next_event_id,
                                                             uint64_t next_insertion_sequence)
{
  if (scheduler == NULL || next_event_id == 0 || next_insertion_sequence == 0)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  if (scheduler->event_count > 0 || scheduler->dispatch_depth > 0)
    return GAME_SCHEDULER_BUSY;

  scheduler->next_event_id = next_event_id;
  scheduler->next_insertion_sequence = next_insertion_sequence;
  scheduler->event_id_exhausted = false;
  scheduler->insertion_sequence_exhausted = false;
  return GAME_SCHEDULER_OK;
}
#endif
