#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "constants.h"
#include "db.h"
#include "active_world.h"
#include "domain_event_types.h"
#include "domain_event_world.h"
#include "dotenv.h"
#include "dgscript/dg_event.h"
#include "event_runtime.h"
#include "mob/mob_act.h"

#define ACTIVE_WORLD_MAX_MOBILES 65536U
#define ACTIVE_WORLD_REJECTION_LOG_INTERVAL 100U
#define ACTIVE_WORLD_REASON_COUNT 10U
#define ACTIVE_WORLD_REGISTRY_BUCKETS 8192U

struct active_world_registry_entry
{
  struct domain_entity_handle handle;
  struct char_data *character;
  struct active_world_registry_entry *previous;
  struct active_world_registry_entry *next;
  struct active_world_registry_entry *hash_next;
};

struct active_world_event_payload
{
  struct domain_entity_handle character;
};

static struct active_world_registry_entry *scheduled_mobiles;
static struct active_world_registry_entry *registry_buckets[ACTIVE_WORLD_REGISTRY_BUCKETS];
static struct char_data *dispatching_mobile;
static bool dispatching_mobile_forgotten;
static size_t active_mobile_count;
static size_t cooling_mobile_count;
static size_t reason_counts[ACTIVE_WORLD_REASON_COUNT];
static uint64_t admission_rejections;
static uint64_t mobile_callbacks;
static bool initial_snapshot_logged;
static bool initialized;
static bool enabled;
static bool bootstrap_loading;
static size_t admission_limit = ACTIVE_WORLD_MAX_MOBILES;
static game_event_type_id_t mobile_agenda_event_type;

#ifdef LUMINARI_CUTEST
static bool test_selection_set;
static bool test_selection;
#endif

static bool configured_enabled(void)
{
#if defined(LUMINARI_ENABLE_EVENT_ROLLBACK) || defined(LUMINARI_EVENT_ROLLBACK_TESTS)
  const char *value;

  if (event_backend_current() != EVENT_BACKEND_GAME_SCHEDULER)
    return false;
#ifdef LUMINARI_CUTEST
  if (test_selection_set)
    return test_selection;
#endif
  value = getenv("LUMINARI_ACTIVE_WORLD");
  if (value == NULL || *value == '\0')
    value = get_env_value("LUMINARI_ACTIVE_WORLD");
  if (value == NULL || *value == '\0' || !strcasecmp(value, "active") ||
      !strcasecmp(value, "scheduler") || !strcasecmp(value, "event"))
    return true;
  if (!strcasecmp(value, "legacy") || !strcasecmp(value, "heartbeat") ||
      !strcasecmp(value, "off"))
    return false;
  log("WARNING: Unknown LUMINARI_ACTIVE_WORLD '%s'; using active scheduling.", value);
  return true;
#else
  return true;
#endif
}

static bool mobile_is_live(struct char_data *ch)
{
  return ch != NULL && IS_MOB(ch) && world != NULL && IN_ROOM(ch) != NOWHERE &&
         IN_ROOM(ch) <= top_of_world && !MOB_FLAGGED(ch, MOB_NOTDEADYET) &&
         !MOB_FLAGGED(ch, MOB_NO_AI);
}

static int reason_index(uint32_t reason)
{
  unsigned int index;

  if (reason == 0U || (reason & (reason - 1U)) != 0U)
    return -1;
  for (index = 0U; index < ACTIVE_WORLD_REASON_COUNT; index++)
    if (reason == (1U << index))
      return (int)index;
  return -1;
}

static void set_reasons(struct char_data *ch, mobile_work_mask reasons)
{
  mobile_work_mask changed;
  unsigned int index;

  if (ch == NULL || ch->active_world_work_reasons == reasons)
    return;
  changed = ch->active_world_work_reasons ^ reasons;
  for (index = 0U; index < ACTIVE_WORLD_REASON_COUNT; index++)
  {
    mobile_work_mask bit = (mobile_work_mask)(1U << index);

    if (!(changed & bit))
      continue;
    if (reasons & bit)
      reason_counts[index]++;
    else if (reason_counts[index] > 0U)
      reason_counts[index]--;
  }
  ch->active_world_work_reasons = reasons;
}

static size_t registry_bucket(uint64_t runtime_id)
{
  runtime_id ^= runtime_id >> 33U;
  runtime_id *= UINT64_C(0xff51afd7ed558ccd);
  runtime_id ^= runtime_id >> 33U;
  return (size_t)runtime_id & (ACTIVE_WORLD_REGISTRY_BUCKETS - 1U);
}

static struct active_world_registry_entry *registry_find(struct domain_entity_handle handle)
{
  struct active_world_registry_entry *entry;

  if (!domain_entity_handle_is_valid(handle) || handle.kind != DOMAIN_ENTITY_CHARACTER)
    return NULL;
  for (entry = registry_buckets[registry_bucket(handle.runtime_id)]; entry != NULL;
       entry = entry->hash_next)
    if (domain_entity_handle_equal(entry->handle, handle))
      return entry;
  return NULL;
}

static struct active_world_registry_entry *registry_find_character(struct char_data *ch)
{
  struct active_world_registry_entry *entry;
  uint64_t runtime_id;

  if (ch == NULL)
    return NULL;
  runtime_id = (uint64_t)(uintptr_t)ch;
  for (entry = registry_buckets[registry_bucket(runtime_id)]; entry != NULL;
       entry = entry->hash_next)
    if (entry->character == ch)
      return entry;
  return NULL;
}

static bool registry_insert(struct char_data *ch)
{
  struct active_world_registry_entry *entry;
  struct domain_entity_handle handle;
  size_t bucket;

  if (registry_find_character(ch) != NULL)
    return true;
  handle = domain_event_character_handle(ch);
  if (!domain_entity_handle_is_valid(handle))
    return false;
  entry = calloc(1, sizeof(*entry));
  if (entry == NULL)
    return false;
  entry->handle = handle;
  entry->character = ch;
  entry->next = scheduled_mobiles;
  if (scheduled_mobiles != NULL)
    scheduled_mobiles->previous = entry;
  scheduled_mobiles = entry;
  bucket = registry_bucket(handle.runtime_id);
  entry->hash_next = registry_buckets[bucket];
  registry_buckets[bucket] = entry;
  return true;
}

static void registry_remove(struct char_data *ch)
{
  struct active_world_registry_entry **cursor;
  struct active_world_registry_entry *entry;
  size_t bucket;

  entry = registry_find_character(ch);
  if (entry == NULL)
    return;
  if (entry->previous != NULL)
    entry->previous->next = entry->next;
  else
    scheduled_mobiles = entry->next;
  if (entry->next != NULL)
    entry->next->previous = entry->previous;
  bucket = registry_bucket(entry->handle.runtime_id);
  cursor = &registry_buckets[bucket];
  while (*cursor != NULL && *cursor != entry)
    cursor = &(*cursor)->hash_next;
  if (*cursor == entry)
    *cursor = entry->hash_next;
  free(entry);
}

static void set_state(struct char_data *ch, enum active_world_mobile_state state)
{
  enum active_world_mobile_state previous;

  previous = (enum active_world_mobile_state)ch->active_world_state;
  if (previous == state)
    return;
  if (previous == ACTIVE_WORLD_MOBILE_ACTIVE && active_mobile_count > 0U)
    active_mobile_count--;
  else if (previous == ACTIVE_WORLD_MOBILE_COOLING && cooling_mobile_count > 0U)
    cooling_mobile_count--;
  if (state == ACTIVE_WORLD_MOBILE_ACTIVE)
    active_mobile_count++;
  else if (state == ACTIVE_WORLD_MOBILE_COOLING)
    cooling_mobile_count++;
  ch->active_world_state = (unsigned char)state;
}

static struct game_event_owner mobile_owner(struct char_data *ch)
{
  struct game_event_owner owner;
  struct domain_entity_handle handle;

  handle = domain_event_character_handle(ch);
  owner = game_event_owner_none();
  if (!domain_entity_handle_is_valid(handle))
    return owner;
  owner.kind = GAME_EVENT_OWNER_CHARACTER;
  owner.runtime_id = handle.runtime_id;
  owner.generation = handle.generation;
  return owner;
}

static bool deadline_due(unsigned long deadline)
{
  return deadline != 0U && (long)(pulse - deadline) >= 0L;
}

static long deadline_delay(unsigned long deadline)
{
  unsigned long distance;

  if (deadline == 0U)
    return LONG_MAX;
  if (deadline_due(deadline))
    return 1L;
  distance = deadline - pulse;
  return distance > (unsigned long)LONG_MAX ? LONG_MAX : (long)distance;
}

static long next_mobile_delay(struct char_data *ch)
{
  long delay = LONG_MAX;
  long candidate;

  candidate = deadline_delay(ch->active_world_fixed_due);
  if (candidate < delay)
    delay = candidate;
  candidate = deadline_delay(ch->active_world_wander_due);
  if (candidate < delay)
    delay = candidate;
  candidate = deadline_delay(ch->active_world_reaction_due);
  if (candidate < delay)
    delay = candidate;
  candidate = deadline_delay(ch->active_world_resource_due);
  if (candidate < delay)
    delay = candidate;
  return delay == LONG_MAX ? 0L : delay;
}

static bool runtime_handle_matches(struct event_runtime_handle handle,
                                   const struct game_event_context *context)
{
  return context != NULL && handle.id == context->event_id;
}

static void active_world_mobile_cleanup(void *event_obj)
{
  free(event_obj);
}

static struct game_event_result
active_world_mobile_event(const struct game_event_context *context);

static bool register_mobile_agenda_event_type(void)
{
  struct game_event_type_config config;
  const char *registered_name;
  enum game_scheduler_status status;

  if (!event_runtime_is_initialized())
    return false;
  registered_name = event_runtime_type_name(mobile_agenda_event_type);
  if (registered_name != NULL && !strcmp(registered_name, "mobile.autonomous.agenda"))
    return true;
  mobile_agenda_event_type = 0U;
  memset(&config, 0, sizeof(config));
  config.name = "mobile.autonomous.agenda";
  config.handler = active_world_mobile_event;
  config.cleanup = active_world_mobile_cleanup;
  config.lateness_policy = GAME_EVENT_LATENESS_RUN_ONCE;
  config.max_events = ACTIVE_WORLD_MAX_MOBILES;
  config.max_events_per_owner = 1U;
  config.requires_owner = true;
  status = event_runtime_register_type(&config, &mobile_agenda_event_type);
  if (status != GAME_SCHEDULER_OK)
  {
    log("SYSERR: unable to register native event type 'mobile.autonomous.agenda' "
        "(status %d).",
        status);
    return false;
  }
  return true;
}

static bool schedule_mobile(struct char_data *ch)
{
  struct active_world_event_payload *payload;
  struct game_event_owner owner;
  long delay;

  delay = next_mobile_delay(ch);
  if (delay <= 0L)
    return false;
  owner = mobile_owner(ch);
  if (!game_event_owner_is_valid(owner))
    return false;
  payload = malloc(sizeof(*payload));
  if (payload == NULL)
    return false;
  payload->character.kind = DOMAIN_ENTITY_CHARACTER;
  payload->character.runtime_id = owner.runtime_id;
  payload->character.generation = owner.generation;
  if (event_runtime_schedule_owned_after(mobile_agenda_event_type, owner,
                                         (game_tick_t)delay, payload,
                                         &ch->active_world_event_handle) !=
      GAME_SCHEDULER_OK)
  {
    free(payload);
    return false;
  }
  return true;
}

static void note_admission_rejection(void)
{
  admission_rejections++;
  if (admission_rejections == 1U ||
      admission_rejections % ACTIVE_WORLD_REJECTION_LOG_INTERVAL == 0U)
    log("WARNING: Active-world agenda limit reached or scheduling failed (%u); rejected=%llu.",
        (unsigned int)admission_limit, (unsigned long long)admission_rejections);
}

static void retire_current_mobile(struct char_data *ch)
{
  if (ch == NULL)
    return;
  registry_remove(ch);
  if (ch->active_world_state != ACTIVE_WORLD_MOBILE_DORMANT)
    set_state(ch, ACTIVE_WORLD_MOBILE_DORMANT);
  set_reasons(ch, MOBILE_WORK_NONE);
  ch->active_world_fixed_due = 0U;
  ch->active_world_wander_due = 0U;
  ch->active_world_reaction_due = 0U;
  ch->active_world_resource_due = 0U;
  ch->active_world_event_handle = EVENT_RUNTIME_HANDLE_NONE;
}

static struct game_event_result
active_world_mobile_event(const struct game_event_context *context)
{
  struct active_world_event_payload *payload = context != NULL ? context->payload : NULL;
  struct active_world_registry_entry *entry;
  struct char_data *ch;
  mobile_work_mask due = MOBILE_WORK_NONE;

  if (!initial_snapshot_logged)
  {
    log("Active-world initial agendas: %zu; reasons spec=%zu echo=%zu scavenge=%zu patrol=%zu "
        "hunt=%zu wander=%zu posture=%zu room=%zu combat=%zu recovery=%zu.",
        active_mobile_count, reason_counts[0], reason_counts[1], reason_counts[2],
        reason_counts[3], reason_counts[4], reason_counts[5], reason_counts[6],
        reason_counts[7], reason_counts[8], reason_counts[9]);
    initial_snapshot_logged = true;
  }
  if (payload == NULL)
    return game_event_result_complete();
  entry = registry_find(payload->character);
  if (entry == NULL)
  {
    event_note_stale_owner_outcome();
    return game_event_result_complete();
  }
  ch = entry->character;
  if (!runtime_handle_matches(ch->active_world_event_handle, context))
    return game_event_result_complete();
  if (!mobile_is_live(ch) || ch->active_world_work_reasons == MOBILE_WORK_NONE)
  {
    retire_current_mobile(ch);
    return game_event_result_complete();
  }

  if (deadline_due(ch->active_world_reaction_due))
  {
    due |= ch->active_world_work_reasons & MOBILE_WORK_REACTION_MASK;
    set_reasons(ch, ch->active_world_work_reasons & ~MOBILE_WORK_REACTION_MASK);
    ch->active_world_reaction_due = 0U;
  }
  if (deadline_due(ch->active_world_fixed_due))
  {
    due |= ch->active_world_work_reasons & MOBILE_WORK_FIXED_CADENCE_MASK;
    ch->active_world_fixed_due = pulse + (unsigned long)PULSE_MOBILE;
  }
  if (deadline_due(ch->active_world_wander_due))
  {
    due |= ch->active_world_work_reasons & MOBILE_WORK_WANDER;
    ch->active_world_wander_due = pulse + (unsigned long)mobile_activity_next_wander_delay();
  }
  if (deadline_due(ch->active_world_resource_due))
  {
    due |= ch->active_world_work_reasons & MOBILE_WORK_RESOURCE_RECOVERY;
    ch->active_world_resource_due = 0U;
  }

  if (due != MOBILE_WORK_NONE)
  {
    mobile_callbacks++;
    dispatching_mobile = ch;
    dispatching_mobile_forgotten = false;
    mobile_activity_run_scheduled(ch, due);
    if (dispatching_mobile_forgotten)
    {
      dispatching_mobile = NULL;
      return game_event_result_complete();
    }
    active_world_sync_mobile(ch);
    dispatching_mobile = NULL;
  }

  if (!mobile_is_live(ch) || ch->active_world_work_reasons == MOBILE_WORK_NONE)
  {
    retire_current_mobile(ch);
    return game_event_result_complete();
  }
  return game_event_result_reschedule_after((game_tick_t)next_mobile_delay(ch));
}

static unsigned long fixed_initial_deadline(struct char_data *ch)
{
  struct game_event_owner owner = mobile_owner(ch);
  uint64_t spread;

  if (!game_event_owner_is_valid(owner))
    return pulse + 1U;
  spread = (owner.runtime_id >> 4U) ^ owner.generation;
  spread *= UINT64_C(11400714819323198485);
  return pulse + (unsigned long)(spread % (uint64_t)PULSE_MOBILE) + 1U;
}

static void refresh_deadlines(struct char_data *ch, mobile_work_mask old_reasons,
                              mobile_work_mask new_reasons)
{
  if ((new_reasons & MOBILE_WORK_FIXED_CADENCE_MASK) &&
      !(old_reasons & MOBILE_WORK_FIXED_CADENCE_MASK))
    ch->active_world_fixed_due = fixed_initial_deadline(ch);
  else if (!(new_reasons & MOBILE_WORK_FIXED_CADENCE_MASK))
    ch->active_world_fixed_due = 0U;

  if ((new_reasons & MOBILE_WORK_WANDER) && !(old_reasons & MOBILE_WORK_WANDER))
    ch->active_world_wander_due =
        pulse + (unsigned long)mobile_activity_next_wander_delay();
  else if (!(new_reasons & MOBILE_WORK_WANDER))
    ch->active_world_wander_due = 0U;

  if ((new_reasons & MOBILE_WORK_REACTION_MASK) &&
      !(old_reasons & MOBILE_WORK_REACTION_MASK))
    ch->active_world_reaction_due = pulse + 1U;
  else if (!(new_reasons & MOBILE_WORK_REACTION_MASK))
    ch->active_world_reaction_due = 0U;

  if ((new_reasons & MOBILE_WORK_RESOURCE_RECOVERY) && ch->active_world_resource_due == 0U)
    ch->active_world_resource_due =
        pulse + (unsigned long)mobile_activity_next_resource_recovery_delay(ch);
  else if (!(new_reasons & MOBILE_WORK_RESOURCE_RECOVERY))
    ch->active_world_resource_due = 0U;
}

static void reschedule_mobile(struct char_data *ch)
{
  struct event_runtime_handle handle;
  game_tick_t remaining;
  long delay;

  if (ch == dispatching_mobile)
    return;
  delay = next_mobile_delay(ch);
  if (delay <= 0L)
    return;
  if (!event_runtime_handle_is_none(ch->active_world_event_handle) &&
      event_runtime_remaining(ch->active_world_event_handle, &remaining) ==
          GAME_SCHEDULER_OK &&
      remaining == (game_tick_t)delay)
    return;
  if (!event_runtime_handle_is_none(ch->active_world_event_handle))
  {
    handle = ch->active_world_event_handle;
    ch->active_world_event_handle = EVENT_RUNTIME_HANDLE_NONE;
    (void)event_runtime_cancel(handle);
  }
  if (!schedule_mobile(ch))
  {
    note_admission_rejection();
    retire_current_mobile(ch);
  }
}

void active_world_sync_mobile(struct char_data *ch)
{
  mobile_work_mask old_reasons;
  mobile_work_mask new_reasons;

  if (!initialized || !enabled || ch == NULL || !IS_NPC(ch))
    return;
  old_reasons = ch->active_world_work_reasons;
  new_reasons = old_reasons & MOBILE_WORK_REACTION_MASK;
  new_reasons |= mobile_activity_recurring_reasons(ch);
  refresh_deadlines(ch, old_reasons, new_reasons);
  set_reasons(ch, new_reasons);

  if (new_reasons == MOBILE_WORK_NONE)
  {
    active_world_forget_character(ch);
    return;
  }
  if (registry_find_character(ch) == NULL)
  {
    if (active_mobile_count + cooling_mobile_count >= admission_limit)
    {
      note_admission_rejection();
      retire_current_mobile(ch);
      return;
    }
    if (!registry_insert(ch))
    {
      note_admission_rejection();
      retire_current_mobile(ch);
      return;
    }
    set_state(ch, ACTIVE_WORLD_MOBILE_ACTIVE);
  }
  reschedule_mobile(ch);
}

static void wake_mobile(struct char_data *ch, mobile_work_mask reason)
{
  mobile_work_mask old_reasons;
  mobile_work_mask new_reasons;

  if (!initialized || !enabled || !mobile_is_live(ch))
    return;
  reason &= MOBILE_WORK_REACTION_MASK;
  if (reason == MOBILE_WORK_NONE)
    return;
  old_reasons = ch->active_world_work_reasons;
  new_reasons = old_reasons | reason;
  refresh_deadlines(ch, old_reasons, new_reasons);
  set_reasons(ch, new_reasons);
  active_world_sync_mobile(ch);
}

void active_world_forget_character(struct char_data *ch)
{
  struct event_runtime_handle handle;

  if (ch == NULL)
    return;
  if (dispatching_mobile == ch)
    dispatching_mobile_forgotten = true;
  registry_remove(ch);
  if (ch->active_world_state != ACTIVE_WORLD_MOBILE_DORMANT)
    set_state(ch, ACTIVE_WORLD_MOBILE_DORMANT);
  set_reasons(ch, MOBILE_WORK_NONE);
  ch->active_world_fixed_due = 0U;
  ch->active_world_wander_due = 0U;
  ch->active_world_reaction_due = 0U;
  ch->active_world_resource_due = 0U;
  if (event_runtime_handle_is_none(ch->active_world_event_handle))
    return;
  handle = ch->active_world_event_handle;
  ch->active_world_event_handle = EVENT_RUNTIME_HANDLE_NONE;
  if (dispatching_mobile != ch)
    (void)event_runtime_cancel(handle);
}

static struct room_data *resolve_room(struct domain_event_bus *bus,
                                      struct domain_entity_handle handle)
{
  if (handle.kind != DOMAIN_ENTITY_ROOM)
    return NULL;
  return domain_event_resolve(bus, handle, DOMAIN_ENTITY_ROOM);
}

static void sync_room_mobiles(struct room_data *room)
{
  struct char_data *ch;
  struct char_data *next;

  if (room == NULL)
    return;
  for (ch = room->people; ch != NULL; ch = next)
  {
    next = ch->next_in_room;
    active_world_sync_mobile(ch);
  }
}

static void wake_room(struct room_data *room, bool combat)
{
  struct char_data *ch;
  struct char_data *next;

  if (room == NULL)
    return;
  for (ch = room->people; ch != NULL; ch = next)
  {
    mobile_work_mask reason;

    next = ch->next_in_room;
    reason = combat ? mobile_activity_combat_reaction_reasons(ch)
                    : mobile_activity_room_reaction_reasons(ch);
    wake_mobile(ch, reason);
  }
}

static void wake_adjacent(room_rnum room, bool combat)
{
  int direction;

  if (room == NOWHERE || room > top_of_world)
    return;
  for (direction = 0; direction < DIR_COUNT; direction++)
  {
    struct room_direction_data *exit = world[room].dir_option[direction];
    struct char_data *ch;
    struct char_data *next;

    if (exit == NULL || exit->to_room == NOWHERE || exit->to_room > top_of_world)
      continue;
    for (ch = world[exit->to_room].people; ch != NULL; ch = next)
    {
      next = ch->next_in_room;
      if (combat && MOB_FLAGGED(ch, MOB_LISTEN))
        wake_mobile(ch, MOBILE_WORK_COMBAT_REACTION);
      else if (!combat && MOB_FLAGGED(ch, MOB_ROL_ARCHER))
        wake_mobile(ch, MOBILE_WORK_ROOM_REACTION);
    }
  }
}

static void handle_character_moved(const struct domain_event_context *context,
                                   void *handler_context)
{
  const struct domain_character_moved *event = context->payload;
  struct char_data *ch;
  struct room_data *to_room;

  (void)handler_context;
  if (bootstrap_loading)
    return;
  ch = domain_event_resolve(context->bus, event->character, DOMAIN_ENTITY_CHARACTER);
  if (ch == NULL)
    return;
  to_room = resolve_room(context->bus, event->to_room);
  if (IS_NPC(ch))
  {
    active_world_sync_mobile(ch);
    wake_mobile(ch, mobile_activity_room_reaction_reasons(ch));
    if (!IS_PET(ch))
      return;
  }
  wake_room(to_room, false);
  if (to_room != NULL)
    wake_adjacent((room_rnum)(to_room - world), false);
}

static void handle_combat_state_changed(const struct domain_event_context *context,
                                        void *handler_context)
{
  const struct domain_combat_state_changed *event = context->payload;
  struct char_data *ch;
  room_rnum room;

  (void)handler_context;
  if (bootstrap_loading)
    return;
  ch = domain_event_resolve(context->bus, event->character, DOMAIN_ENTITY_CHARACTER);
  if (ch == NULL || IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) > top_of_world)
    return;
  room = IN_ROOM(ch);
  sync_room_mobiles(&world[room]);
  if (event->in_combat)
  {
    wake_room(&world[room], true);
    wake_adjacent(room, true);
  }
}

static void handle_object_moved(const struct domain_event_context *context,
                                void *handler_context)
{
  const struct domain_object_moved *event = context->payload;
  struct room_data *room;

  (void)handler_context;
  if (bootstrap_loading)
    return;
  room = resolve_room(context->bus, event->from_owner);
  sync_room_mobiles(room);
  room = resolve_room(context->bus, event->to_owner);
  sync_room_mobiles(room);
}

static void handle_entity_extracted(const struct domain_event_context *context,
                                    void *handler_context)
{
  const struct domain_entity_extracted *event = context->payload;
  struct char_data *ch;

  (void)handler_context;
  if (event->entity.kind != DOMAIN_ENTITY_CHARACTER)
    return;
  ch = domain_event_resolve(context->bus, event->entity, DOMAIN_ENTITY_CHARACTER);
  active_world_forget_character(ch);
}

enum domain_event_status active_world_register_handlers(struct domain_event_bus *bus)
{
  struct domain_event_handler_config handlers[] = {
      {DOMAIN_EVENT_CHARACTER_MOVED, "active-world-character-moved", 100,
       handle_character_moved, NULL},
      {DOMAIN_EVENT_COMBAT_STATE_CHANGED, "active-world-combat-state", 100,
       handle_combat_state_changed, NULL},
      {DOMAIN_EVENT_OBJECT_MOVED, "active-world-object-moved", 100,
       handle_object_moved, NULL},
      {DOMAIN_EVENT_ENTITY_EXTRACTED, "active-world-entity-extracted", 100,
       handle_entity_extracted, NULL},
  };
  size_t index;
  enum domain_event_status status;
  bool requested;

  if (bus == NULL || initialized)
    return DOMAIN_EVENT_INVALID_ARGUMENT;
  requested = configured_enabled();
  if (requested && !register_mobile_agenda_event_type())
    return DOMAIN_EVENT_BUSY;
  enabled = requested;
  initialized = true;
#if defined(LUMINARI_ENABLE_EVENT_ROLLBACK) || defined(LUMINARI_EVENT_ROLLBACK_TESTS)
  log("Active-world mobile scheduling: %s (explicit agenda limit %u).",
      enabled ? "demand driven" : "legacy heartbeat", (unsigned int)admission_limit);
#else
  log("Active-world mobile scheduling: %s (explicit agenda limit %u).",
      enabled ? "demand driven" : "unavailable", (unsigned int)admission_limit);
#endif
  if (!enabled)
    return DOMAIN_EVENT_OK;
  for (index = 0U; index < sizeof(handlers) / sizeof(handlers[0]); index++)
  {
    status = domain_event_register_handler(bus, &handlers[index]);
    if (status != DOMAIN_EVENT_OK)
      return status;
  }

  return DOMAIN_EVENT_OK;
}

void active_world_begin_bootstrap(void)
{
  if (initialized)
    bootstrap_loading = true;
}

void active_world_end_bootstrap(void)
{
  struct char_data *ch;

  if (!initialized)
    return;
  bootstrap_loading = false;
  if (!enabled)
    return;

  /* Final world state is the sole permitted population discovery pass. */
  for (ch = character_list; ch != NULL; ch = ch->next)
    active_world_sync_mobile(ch);
}

void active_world_shutdown(void)
{
  struct active_world_registry_entry *entry;
  struct char_data *ch;

  while ((entry = scheduled_mobiles) != NULL)
  {
    ch = entry->character;
    if (ch != NULL)
      active_world_forget_character(ch);
    else
    {
      scheduled_mobiles = entry->next;
      free(entry);
    }
  }
  scheduled_mobiles = NULL;
  memset(registry_buckets, 0, sizeof(registry_buckets));
  dispatching_mobile = NULL;
  dispatching_mobile_forgotten = false;
  active_mobile_count = 0U;
  cooling_mobile_count = 0U;
  memset(reason_counts, 0, sizeof(reason_counts));
  initial_snapshot_logged = false;
  initialized = false;
  enabled = false;
  bootstrap_loading = false;
}

bool active_world_enabled(void)
{
  return initialized && enabled;
}

size_t active_world_mobile_count(enum active_world_mobile_state state)
{
  if (state == ACTIVE_WORLD_MOBILE_ACTIVE)
    return active_mobile_count;
  if (state == ACTIVE_WORLD_MOBILE_COOLING)
    return cooling_mobile_count;
  return 0U;
}

size_t active_world_mobile_admission_limit(void)
{
  return admission_limit;
}

size_t active_world_mobile_reason_count(uint32_t reason)
{
  int index = reason_index(reason);

  return index < 0 ? 0U : reason_counts[index];
}

uint64_t active_world_mobile_callbacks(void)
{
  return mobile_callbacks;
}

uint64_t active_world_mobile_admission_rejections(void)
{
  return admission_rejections;
}

void active_world_reset_telemetry(void)
{
  admission_rejections = 0U;
  mobile_callbacks = 0U;
}

#ifdef LUMINARI_CUTEST
void active_world_reset_for_test(void)
{
  active_world_shutdown();
  active_world_reset_telemetry();
  admission_limit = ACTIVE_WORLD_MAX_MOBILES;
  test_selection_set = false;
  test_selection = false;
}

void active_world_set_admission_limit_for_test(size_t limit)
{
  admission_limit = limit;
}

void active_world_select_for_test(bool use_active_world)
{
  test_selection_set = true;
  test_selection = use_active_world;
}
#endif
